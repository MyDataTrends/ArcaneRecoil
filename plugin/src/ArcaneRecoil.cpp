#include "ArcaneRecoil.h"
#include "FormIDs.h"
#include "Settings.h"

#include <chrono>
#include <mutex>
#include <unordered_map>

// Arcane Recoil v1.1 (design: SpellShield_Mod_Spec.md, "v1.1 changes").
//
// Where damage happens (measured in game 2026-10-15 with the debug log, a wolf against a forced-charge player):
// the TESHitEvent arrives BEFORE health drops (health 150 at the event, then -40); melee damage does NOT pass through
// ActorValueOwner::RestoreActorValue; Actor::HandleHealthDamage (vfunc 0x104) runs for every damage instance, with the
// attacker, right after health drops. v1.1 absorbs inside HandleHealthDamage: the health comes back in the same call,
// before the original handler (hit reactions, bleedout) sees the reduced damage, instead of a frame later (v1.0).

namespace ArcaneRecoil {
namespace {
using Clock = std::chrono::steady_clock;

namespace School {
constexpr int Alteration = 0;
constexpr int Conjuration = 1;
constexpr int Destruction = 2;
constexpr int Illusion = 3;
constexpr int Restoration = 4;
constexpr int Unknown = -1;
} // namespace School

namespace Element {
constexpr int Fire = 0;
constexpr int Frost = 1;
constexpr int Shock = 2;
constexpr int Unknown = -1;
} // namespace Element

// Vanilla keyword FormIDs in Skyrim.esm (checked against the file 2026-10-15; v1.0's values matched no keyword,
// so schools and elements were never recognized).
namespace Keyword {
constexpr RE::FormID MagicSchoolAlteration = 0x1076F1;
constexpr RE::FormID MagicSchoolConjuration = 0x1076F4;
constexpr RE::FormID MagicSchoolDestruction = 0x1076F0;
constexpr RE::FormID MagicSchoolIllusion = 0x1076F2;
constexpr RE::FormID MagicSchoolRestoration = 0x1076F3;
constexpr RE::FormID MagicDamageFire = 0x01CEAD;
constexpr RE::FormID MagicDamageFrost = 0x01CEAE;
constexpr RE::FormID MagicDamageShock = 0x01CEAF;
constexpr RE::FormID MagicWard = 0x01EA69;
} // namespace Keyword

struct Forms {
  RE::BGSKeyword *alteration = nullptr, *conjuration = nullptr, *destruction = nullptr, *illusion = nullptr,
                 *restoration = nullptr, *fire = nullptr, *frost = nullptr, *shock = nullptr, *ward = nullptr;
  RE::SpellItem *burstFire = nullptr, *burstFrost = nullptr, *burstShock = nullptr, *burn = nullptr,
                *stagger = nullptr, *heal = nullptr, *harden = nullptr;
} g;

// One absorbed hit, carried from the damage hook to the next-frame effects and to the hit event.
struct Recoil {
  RE::ActorHandle actor;
  RE::ActorHandle attacker;  // none = environmental (fall, trap) or a scripted effect
  RE::SpellItem *spell = nullptr;
  RE::MagicSystem::CastingSource source = RE::MagicSystem::CastingSource::kLeftHand;
  int school = School::Unknown;
  int element = Element::Unknown;
  float absorbed = 0.0f;
  bool ward = false;
  bool concentration = false;
  bool perfect = false;
};

Clock::time_point g_castStart[2];
std::unordered_map<RE::FormID, Clock::time_point> g_concCooldownUntil;  // per actor, after a concentration recoil
bool g_wardModPresent = false;  // Perfectly Valid Wards / Wards Functionalities Extended detected at data load

// Ward overhauls that already make wards block physical hits (file names from their Nexus archives, 2026-10-15).
void DetectWardMods() {
  auto *dh = RE::TESDataHandler::GetSingleton();
  for (auto name : {"PerfectlyValidWards.esp", "WardsFunctionalitiesExtended.esp"}) {
    if (dh->LookupLoadedModByName(name) || dh->LookupLoadedLightModByName(name)) {
      g_wardModPresent = true;
      LOG_INFO("ArcaneRecoil: ward overhaul detected ({}); wards are left to it while 'defer' is on", name);
    }
  }
  if (!g_wardModPresent && GetModuleHandle(L"PerfectlyValidWards.dll")) {
    g_wardModPresent = true;
    LOG_INFO("ArcaneRecoil: ward overhaul detected (PerfectlyValidWards.dll)");
  }
}  // player: when the left [0] / right [1] hand started casting
bool g_inEffects = false;          // our own spells and magicka changes must not recoil again


bool IsPlayer(const RE::Actor *actor) { return actor && actor == RE::PlayerCharacter::GetSingleton(); }

template <class T> T *Own(RE::FormID id) {
  return RE::TESDataHandler::GetSingleton()->LookupForm<T>(id, FormIDs::Plugin);
}

void CacheForms() {
  auto *dh = RE::TESDataHandler::GetSingleton();
  auto kw = [&](RE::FormID id) { return dh->LookupForm<RE::BGSKeyword>(id, "Skyrim.esm"); };
  g.alteration = kw(Keyword::MagicSchoolAlteration);
  g.conjuration = kw(Keyword::MagicSchoolConjuration);
  g.destruction = kw(Keyword::MagicSchoolDestruction);
  g.illusion = kw(Keyword::MagicSchoolIllusion);
  g.restoration = kw(Keyword::MagicSchoolRestoration);
  g.fire = kw(Keyword::MagicDamageFire);
  g.frost = kw(Keyword::MagicDamageFrost);
  g.shock = kw(Keyword::MagicDamageShock);
  g.ward = kw(Keyword::MagicWard);
  g.burstFire = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_BurstFire);
  g.burstFrost = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_BurstFrost);
  g.burstShock = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_BurstShock);
  g.burn = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_Burn);
  g.stagger = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_Stagger);
  g.heal = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_Heal);
  g.harden = Own<RE::SpellItem>(FormIDs::ArcaneRecoil_Harden);
  for (auto [name, spell] : {std::pair{"BurstFire", g.burstFire}, std::pair{"BurstFrost", g.burstFrost},
                             std::pair{"BurstShock", g.burstShock}, std::pair{"Burn", g.burn},
                             std::pair{"Stagger", g.stagger}, std::pair{"Heal", g.heal},
                             std::pair{"Harden", g.harden}}) {
    if (!spell) {
      LOG_WARN("ArcaneRecoil: spell ArcaneRecoil_{} not found in {}", name, FormIDs::Plugin);
    }
  }
}

bool Has(const RE::SpellItem *spell, RE::BGSKeyword *keyword) { return spell && keyword && spell->HasKeyword(keyword); }

// School and element: vanilla spells carry these keywords on their magic EFFECT, not on the spell record (v1.0
// checked the spell only, so Firebolt read as "no school" in game, 2026-10-15). Use the spell's keywords when present,
// else the costliest effect's skill and keywords.
RE::EffectSetting *MainEffect(RE::SpellItem *spell) {
  auto *effect = spell ? spell->GetCostliestEffectItem() : nullptr;
  return effect ? effect->baseEffect : nullptr;
}

// Any of the spell's effects (Requiem adds effects to vanilla spells, so the costliest one isn't always the damage).
bool HasEither(RE::SpellItem *spell, RE::BGSKeyword *keyword) {
  if (!spell || !keyword) return false;
  if (Has(spell, keyword)) return true;
  for (auto *effect : spell->effects) {
    if (effect && effect->baseEffect && effect->baseEffect->HasKeyword(keyword)) return true;
  }
  return false;
}

int GetSpellSchool(RE::SpellItem *spell) {
  if (HasEither(spell, g.alteration)) return School::Alteration;
  if (HasEither(spell, g.conjuration)) return School::Conjuration;
  if (HasEither(spell, g.destruction)) return School::Destruction;
  if (HasEither(spell, g.illusion)) return School::Illusion;
  if (HasEither(spell, g.restoration)) return School::Restoration;
  if (auto *mgef = MainEffect(spell)) {
    switch (mgef->GetMagickSkill()) {
    case RE::ActorValue::kAlteration: return School::Alteration;
    case RE::ActorValue::kConjuration: return School::Conjuration;
    case RE::ActorValue::kDestruction: return School::Destruction;
    case RE::ActorValue::kIllusion: return School::Illusion;
    case RE::ActorValue::kRestoration: return School::Restoration;
    default: break;
    }
  }
  return School::Unknown;
}

int GetDestructionElement(RE::SpellItem *spell) {
  if (HasEither(spell, g.fire)) return Element::Fire;
  if (HasEither(spell, g.frost)) return Element::Frost;
  if (HasEither(spell, g.shock)) return Element::Shock;
  return Element::Unknown;
}

// Wards carry the MagicWard keyword on their magic effect.
bool IsWard(RE::SpellItem *spell) {
  if (!spell || !g.ward) return false;
  for (auto *effect : spell->effects) {
    if (effect && effect->baseEffect && effect->baseEffect->HasKeyword(g.ward)) return true;
  }
  return false;
}

// The author's balance rule (2026-10-15): only hits bigger than the held spell's magnitude trigger the recoil.
float SpellMagnitude(RE::SpellItem *spell) {
  if (!spell) return 0.0f;
  auto *effect = spell->GetCostliestEffectItem();
  return effect ? effect->GetMagnitude() : 0.0f;
}

RE::ActorValue SchoolToActorValue(int school) {
  switch (school) {
  case School::Alteration: return RE::ActorValue::kAlteration;
  case School::Conjuration: return RE::ActorValue::kConjuration;
  case School::Illusion: return RE::ActorValue::kIllusion;
  case School::Restoration: return RE::ActorValue::kRestoration;
  default: return RE::ActorValue::kDestruction;
  }
}

float AbsorptionCap(RE::Actor *actor, int school, bool ward) {
  float skill = actor->AsActorValueOwner()->GetActorValue(SchoolToActorValue(school));
  float mult = school == School::Alteration ? Settings::AlterationBonus()
               : school == School::Restoration ? Settings::RestorationBonus()
                                               : 1.0f;
  float cap = Settings::BaseAbsorption() + skill * Settings::ScalingFactor() * mult;
  return ward ? cap * Settings::WardBonus() : cap;
}

struct Charge {
  RE::SpellItem *spell = nullptr;
  RE::MagicSystem::CastingSource source = RE::MagicSystem::CastingSource::kLeftHand;
  RE::MagicCaster *caster = nullptr;
};

bool CasterActive(RE::MagicCaster *caster) {
  using S = RE::MagicCaster::State;
  auto s = caster->state.get();
  return s == S::kCharging || s == S::kCasting || s == S::kReady;  // kReady = charged and held
}

bool FindCharge(RE::Actor *actor, Charge &out) {
  for (bool left : {true, false}) {
    if (!left && Settings::LeftHandOnly()) break;
    auto *obj = actor->GetEquippedObject(left);
    auto *spell = obj ? obj->As<RE::SpellItem>() : nullptr;
    auto source = left ? RE::MagicSystem::CastingSource::kLeftHand : RE::MagicSystem::CastingSource::kRightHand;
    auto *caster = actor->GetMagicCaster(source);
    if (spell && caster && CasterActive(caster)) {
      out = {spell, source, caster};
      return true;
    }
  }
  if (IsPlayer(actor) && Settings::DebugForceCharge()) {  // test builds: pretend the left hand is charging
    auto *obj = actor->GetEquippedObject(true);
    out = {obj ? obj->As<RE::SpellItem>() : nullptr, RE::MagicSystem::CastingSource::kLeftHand,
           actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)};
    return true;
  }
  return false;
}

void Cast(RE::SpellItem *spell, RE::Actor *caster, RE::Actor *target, float magnitude) {
  if (!spell || !caster || !target) return;
  if (auto *mc = caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
    mc->CastSpellImmediate(spell, false, target, 1.0f, false, magnitude, caster);
  }
}

RE::SpellItem *ElementSpell(int element) {
  switch (element) {
  case Element::Fire: return g.burstFire;
  case Element::Frost: return g.burstFrost;
  case Element::Shock: return g.burstShock;
  default: return nullptr;
  }
}

// Stagger + element riders on whoever hit us.
void ApplyAttackerEffects(RE::Actor *self, RE::Actor *attacker, const Recoil &r) {
  g_inEffects = true;
  if (attacker && attacker != self && !attacker->IsDead()) {
    if (Settings::StaggerEnabled()) {
      Cast(g.stagger, self, attacker, Settings::StaggerStrength() * (r.perfect ? 2.0f : 1.0f));
    }
    if (Settings::Riders() && r.school == School::Destruction) {
      float mag = r.absorbed * Settings::RiderMultiplier();
      if (r.element == Element::Fire) Cast(g.burn, self, attacker, mag / 3.0f);  // per second, 3 s
      else Cast(ElementSpell(r.element), self, attacker, mag);  // frost saps stamina, shock drains magicka
    }
  }
  if (Settings::Riders()) {
    float mag = r.absorbed * Settings::RiderMultiplier();
    if (r.school == School::Restoration) Cast(g.heal, self, self, mag);
    if (r.school == School::Alteration) Cast(g.harden, self, self, std::max(20.0f, mag));
  }
  g_inEffects = false;
  LOG_DEBUG("ArcaneRecoil: attacker effects on {:X} (stagger {}, riders {})",
            attacker ? attacker->GetFormID() : 0, Settings::StaggerEnabled(), Settings::Riders());
}

// Destruction: element damage cast by the recoiling actor (resistances, aggro, XP, crime all apply).
// Other schools: an arcane shockwave that pushes enemies back.
void ApplyAoE(RE::Actor *self, const Recoil &r) {
  if (!Settings::AoEEnabled()) return;
  auto *elementSpell = ElementSpell(r.element);
  auto *spell = elementSpell ? elementSpell : g.stagger;
  float magnitude = elementSpell ? r.absorbed * Settings::AoEMultiplier() : Settings::StaggerStrength();
  float radius = Settings::AoERadius();
  auto origin = self->GetPosition();
  auto *lists = RE::ProcessLists::GetSingleton();
  if (!lists) return;
  std::vector<RE::Actor *> targets;
  for (auto &handle : lists->highActorHandles) {
    auto ptr = handle.get();
    auto *t = ptr.get();
    if (!t || t == self || t->IsDead() || origin.GetDistance(t->GetPosition()) > radius) continue;
    if (!Settings::FriendlyFire()) {
      if (IsPlayer(self) ? t->IsPlayerTeammate() : !t->IsHostileToActor(self)) continue;
    }
    targets.push_back(t);
  }
  g_inEffects = true;
  for (auto *t : targets) Cast(spell, self, t, magnitude);  // lands a few frames later (verified: 155 -> 138 -> 120 ...)
  g_inEffects = false;
  LOG_DEBUG("ArcaneRecoil: AoE hit {} actors (magnitude {})", targets.size(), magnitude);
}

void SendVFXEvent(RE::Actor *actor, const Recoil &r) {
  if (!Settings::ExplosionVFX() && !Settings::DetonationSound()) return;
  auto type = Settings::ExplosionTypeString(r.school, r.element);
  float hand = r.source == RE::MagicSystem::CastingSource::kRightHand ? 1.0f : 0.0f;
  SKSE::ModCallbackEvent event{"ArcaneRecoil_VFX", type.c_str(), hand, actor};
  SKSE::GetModCallbackEventSource()->SendEvent(&event);
}

// Next frame after an absorb: pay the cost, end the cast, burst, feedback, stagger and riders.
void ApplyRecoil(const Recoil &r) {
  auto ptr = r.actor.get();
  auto *actor = ptr.get();
  if (!actor) return;
  g_inEffects = true;
  auto *av = actor->AsActorValueOwner();
  {
    // Cost scales with what the recoil saved you (a mana shield), at least the spell's own cost, capped at a share of
    // max magicka so a massive hit stays blockable but expensive (author, 2026-10-15). Perfect recoils pay a share.
    float current = av->GetActorValue(RE::ActorValue::kMagicka);
    float spellCost = r.spell ? r.spell->CalculateMagickaCost(actor) : 0.0f;
    float ceiling = Settings::CostMaxFraction() * av->GetPermanentActorValue(RE::ActorValue::kMagicka);
    float cost = std::max(spellCost, std::min(r.absorbed * Settings::CostRatio(), ceiling));
    if (Settings::DrainAllMagicka()) cost = current;
    if (r.perfect) cost *= Settings::PerfectCostMult();
    cost = std::min(cost, std::max(current, 0.0f));
    if (cost > 0.0f) av->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -cost);
    LOG_DEBUG("ArcaneRecoil: cost {} (spell {}, scaled {}, ceiling {}, perfect {})", cost, spellCost,
              r.absorbed * Settings::CostRatio(), ceiling, r.perfect);
  }
  if (!r.ward) {
    if (auto *caster = actor->GetMagicCaster(r.source)) caster->InterruptCast(false);
  }
  g_inEffects = false;

  ApplyAoE(actor, r);
  if (IsPlayer(actor)) {
    if (Settings::ScreenFlash()) {
      if (auto *imod = Settings::FlashIMOD()) RE::ImageSpaceModifierInstanceForm::Trigger(imod, 1.0f, nullptr);
    }
    if (Settings::Notification()) {
      auto msg = std::format("{} absorbed {} damage!", r.perfect ? "Perfect Recoil!" : "Arcane Recoil",
                             static_cast<int>(r.absorbed));
      RE::DebugNotification(msg.c_str());
    }
  }
  SendVFXEvent(actor, r);
  auto attacker = r.attacker.get();
  ApplyAttackerEffects(actor, attacker.get(), r);
}

// Incoming damage (positive) on `actor` from `attacker` (may be null). Returns the damage that still applies.
float OnIncomingDamage(RE::Actor *actor, RE::Actor *attacker, float damage) {
  if (!Settings::ModEnabled() || actor->IsDead()) return damage;
  bool isPlayer = IsPlayer(actor);
  if (!isPlayer && !Settings::NPCMages()) return damage;
  if (!attacker && !Settings::EnvironmentalDamage()) return damage;  // fall, trap, scripted damage
  Charge charge;
  if (!FindCharge(actor, charge)) return damage;
  bool ward = IsWard(charge.spell);
  if (ward && (!Settings::WardsEnabled() || (Settings::WardsDeferToMods() && g_wardModPresent))) return damage;
  bool concentration = !ward && charge.spell &&
                       charge.spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
  if (concentration) {
    auto it = g_concCooldownUntil.find(actor->GetFormID());
    if (it != g_concCooldownUntil.end() && Clock::now() < it->second) {
      LOG_DEBUG("ArcaneRecoil: concentration recoil on cooldown");
      return damage;
    }
  }

  float magnitude = SpellMagnitude(charge.spell);
  // A ward's magnitude is its strength (what it is built to stop), so the "bigger than the spell" rule would be
  // backwards for it: wards recoil on any hit. Every other spell: damage must exceed its magnitude (author's rule).
  float threshold = ward ? 0.0f : magnitude * Settings::TriggerThreshold();
  if (damage <= threshold) {
    LOG_DEBUG("ArcaneRecoil: {} damage <= threshold {} (spell magnitude {}): no recoil", damage, threshold, magnitude);
    return damage;
  }
  Recoil r;
  r.actor = actor->GetHandle();
  if (attacker) r.attacker = attacker->GetHandle();
  r.spell = charge.spell;
  r.source = charge.source;
  r.school = charge.spell ? GetSpellSchool(charge.spell) : School::Destruction;
  r.element = r.school == School::Destruction ? GetDestructionElement(charge.spell) : Element::Unknown;
  r.ward = ward;
  r.concentration = concentration;
  float cap = AbsorptionCap(actor, r.school, r.ward);
  if (concentration) {  // a held stream only saves what it would deal (author, 2026-10-15)
    cap = std::min(cap, magnitude * Settings::ConcentrationCapMult());
  }
  if (!Settings::PartialAbsorption() && damage > cap) {
    LOG_DEBUG("ArcaneRecoil: shield shattered, {} damage > cap {}", damage, cap);
    return damage;
  }
  r.absorbed = std::min(damage, cap);
  if (isPlayer) {
    int hand = r.source == RE::MagicSystem::CastingSource::kRightHand ? 1 : 0;
    auto since = std::chrono::duration<float>(Clock::now() - g_castStart[hand]).count();
    r.perfect = since >= 0.0f && since <= Settings::PerfectWindow();
  }
  LOG_DEBUG("ArcaneRecoil: absorbed {} of {} from {:X} (cap {}, threshold {}, school {}, ward {}, conc {}, perfect {})",
            r.absorbed, damage, attacker ? attacker->GetFormID() : 0, cap, threshold, r.school, r.ward,
            r.concentration, r.perfect);
  if (concentration) {
    g_concCooldownUntil[actor->GetFormID()] =
        Clock::now() + std::chrono::milliseconds(static_cast<int>(Settings::ConcentrationCooldown() * 1000.0f));
  }
  SKSE::GetTaskInterface()->AddTask([r]() { ApplyRecoil(r); });
  return damage - r.absorbed;
}

// ── Damage hook: Actor::HandleHealthDamage (vfunc 0x104), called right after health drops ──
float Absorb(RE::Actor *actor, RE::Actor *attacker, float change) {  // change < 0 for damage
  if (change >= 0.0f || g_inEffects || !actor) return change;
  float damage = -change;
  float remaining = OnIncomingDamage(actor, attacker, damage);
  float absorbed = damage - remaining;
  if (absorbed > 0.0f) {
    g_inEffects = true;
    actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth,
                                                  absorbed);
    g_inEffects = false;
  }
  return -remaining;
}

struct HealthDamageHook {
  static void Player(RE::Actor *a_this, RE::Actor *a_attacker, float a_damage) {
    LOG_DEBUG("ArcaneRecoil: HandleHealthDamage {} from {:X} (health {})", a_damage,
              a_attacker ? a_attacker->GetFormID() : 0,
              a_this->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth));
    _player(a_this, a_attacker, Absorb(a_this, a_attacker, a_damage));
  }
  static void Npc(RE::Actor *a_this, RE::Actor *a_attacker, float a_damage) {
    _npc(a_this, a_attacker, Absorb(a_this, a_attacker, a_damage));
  }
  static inline REL::Relocation<decltype(Player)> _player;
  static inline REL::Relocation<decltype(Npc)> _npc;
};

// ── Cast start (player): the perfect-recoil window ─────────────────────
class AnimEventSink final : public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
public:
  RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent *event,
                                        RE::BSTEventSource<RE::BSAnimationGraphEvent> *) override {
    if (event) {
      if (event->tag == "BeginCastLeft") g_castStart[0] = Clock::now();
      else if (event->tag == "BeginCastRight") g_castStart[1] = Clock::now();
    }
    return RE::BSEventNotifyControl::kContinue;
  }
  static AnimEventSink *GetSingleton() {
    static AnimEventSink instance;
    return &instance;
  }
};
} // anonymous namespace

void Install() {
  CacheForms();
  DetectWardMods();
  Settings::CacheGlobals();
  Settings::CacheVFXForms();
  REL::Relocation<std::uintptr_t> player{RE::VTABLE_PlayerCharacter[0]};
  HealthDamageHook::_player = player.write_vfunc(0x104, HealthDamageHook::Player);
  REL::Relocation<std::uintptr_t> npc{RE::VTABLE_Character[0]};
  HealthDamageHook::_npc = npc.write_vfunc(0x104, HealthDamageHook::Npc);
  LOG_INFO("ArcaneRecoil: v1.1 installed (HandleHealthDamage hook on player and NPCs)");
}

void OnGameLoaded() {
  g_concCooldownUntil.clear();
  if (auto *player = RE::PlayerCharacter::GetSingleton()) {
    player->RemoveAnimationGraphEventSink(AnimEventSink::GetSingleton());
    player->AddAnimationGraphEventSink(AnimEventSink::GetSingleton());
  }
}
} // namespace ArcaneRecoil
