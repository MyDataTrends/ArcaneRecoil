# Skyrim Mod Spec: Arcane Recoil — Spell Shield

## Overview

**Mod Name:** Arcane Recoil
**Tagline:** Your charged spell becomes your shield — at a cost.
**Target:** Skyrim Special Edition (SSE) with SKSE64
**Requirements:** SKSE64, SkyUI (for MCM)

### Core Fantasy

A mage charging a spell in their left hand gets struck by an enemy. Instead of taking damage, the spell detonates outward in a burst of magical energy, consuming the charge and all invested magicka. The mage is unharmed — but now vulnerable, with an empty hand and drained reserves. This rewards situational awareness and creates a high-risk, high-reward tension for spellcasters: do you hold the charge for offense, or keep it ready as an emergency shield?

---

## Core Mechanic

### Trigger Conditions (ALL must be true)

1. Player has a **spell equipped in the left hand** (not a weapon, shield, or torch)
2. The spell is **actively being charged/sustained** (the player is holding the cast button — the "concentration" or "charge" state)
3. Player receives an incoming hit that would deal damage
4. **The hit's damage is greater than the magnitude of the spell being held** (v1.1; the author's balance rule, 2026-10-15).
   Chip damage (a stray arrow, a damage-over-time tick) passes through and does not pop the shield; only a hit bigger
   than what your spell itself would deal is worth detonating for. MCM: *Trigger threshold* = spell magnitude × N
   (default 1.0; 0 = any hit).

### What Happens on Trigger

1. **Damage is absorbed** — the incoming hit deals zero damage to the player (up to the absorption cap — see Balance section)
2. **Visual explosion** — a burst effect emanates from the player's left hand matching the spell's element (fire = flame burst, frost = ice shards, shock = lightning crackle, other = generic arcane burst)
3. **Spell charge is consumed** — the spell's current charge state is cancelled; the player's hand returns to idle
4. **Magicka is lost** — the magicka invested in the charging spell (its cast cost) is consumed, not refunded.
   v1.0's code drained the whole magicka pool; v1.1 matches this spec and keeps "drain all magicka" as an MCM option.
5. **Optional AoE damage** — if enabled in MCM, nearby enemies within a radius take damage based on the spell being held (see MCM section)
6. **Sound effect** — a satisfying arcane detonation sound plays (distinct from normal spell impact sounds)
7. **Brief visual indicator** — a subtle screen flash or hand glow to give the player clear feedback that Arcane Recoil triggered

### What Does NOT Trigger Arcane Recoil

- Spells equipped but **not actively being charged** (just having a spell in hand is not enough)
- **Right-hand** spells (left hand only — this is deliberate for balance; the player must choose to dedicate their off-hand)
- **Shouts** or **powers**
- **Non-damage effects** (e.g., getting pickpocketed, poison application without direct hit damage)
- Environmental damage (fall damage, trap damage) — debatable, could be MCM toggle

---

## v1.1 changes (2026-10-15)

Reviewed against the v1.0 code (plugin/src) after building the .esp from a spec (agent/modgen/arcane_recoil.toml in
the Lalaland repo). Each change: what, why.

| # | Change | Why |
|---|--------|-----|
| 1 | **Absorb inside the engine's damage handler** (`Actor::HandleHealthDamage`, called right after health drops, with the attacker) and give the health back in the same call | v1.0 restored health a frame later: a killing blow killed you first. Measured in game: the hit event arrives *before* damage (so v1.0's "health before" reading was right), and melee damage does not pass through `RestoreActorValue` (a first attempt hooked that and only caught console damage) |
| 2 | **Trigger only if damage > held spell's magnitude** (× MCM multiplier) | the author's balance rule: chip damage no longer wastes the charge and your magicka |
| 3 | **Magicka cost scales with what it saved you**: max(spell cost, absorbed x 0.5), capped at 60% of max magicka (MCM: ratio, ceiling, or "drain all") | the author: a big magicka pool should buy survival, and a MASSIVE hit should still be blockable but punishing. v1.0 emptied the whole pool |
| 4 | **AoE as a real spell cast by the player** (element-matched, magnitude = absorbed × AoE multiplier), on loaded actors in radius | v1.0 subtracted health directly: no resistances, no aggro, no XP/kill credit, no crime, and only the current cell was searched. (v1.0's radius was fine: Settings.cpp converts feet to units.) |
| 5 | **Stagger the attacker** | makes the recoil read as a magical parry |
| 6 | **Perfect recoil**: hit within a short window (default 0.35 s) after you start casting → half cost (MCM), double stagger, "Perfect Recoil!" | rewards timing; the author preferred a reduction to a free parry |
| 7 | **Element riders** on the attacker: fire burns, frost saps stamina, shock drains magicka; Restoration heals you, Alteration hardens your skin briefly | each school's recoil feels different |
| 8 | **Burst from the casting hand** (explosion placed at the hand node) | v1.0 placed it at the player's feet |
| 9 | **Wards** are the strong option: bigger cap (x1.5), not lost on recoil, no "bigger than the spell" threshold (a ward's magnitude is its strength). MCM toggle, and wards are left alone when Perfectly Valid Wards or Wards Functionalities Extended is installed (detected by plugin/DLL name) | the author likes wards stronger than the plain spell shield; the toggle and detection avoid fighting dedicated ward overhauls |
| 9b | **Concentration spells** (Flames, Healing...) absorb at most their own magnitude, then can't recoil again for 3 s (MCM) | the author's rule: a held stream only saves what it deals, and can't be spammed |
| 10 | **Enemy mages** can recoil too (MCM toggle, default off) | fairness, and scarier mages |
| 11 | **Debug log toggle** (default off) | v1.0 logged and flushed a line on every hit taken |
| 12 | **MCM via MCM Helper** (JSON page bound to the globals, no hand-written menu script) | every setting appears without Papyrus menu code |
| 13 | **VFX script re-registers its mod event on each game load** (player alias) | SKSE mod event registrations are not kept in the save (inferred), so VFX/sound would stop after a reload |
| 14 | **One build command + self-test** (forced-charge debug global, scripted hostile hit) | repeatable builds; the absorb path is tested in game |
| 15 | **School/element detection fixed**: correct keyword form ids (v1.0's matched no keyword) and keywords read from the spell's effects | v1.0 never recognized a school or element, so school bonuses and element VFX/AoE never applied |
| 16 | **Effect spells use Target Actor delivery** | an Aimed spell cast "immediately" launches a projectile instead of applying (the burst did no damage until changed) |

Environmental damage: the damage handler receives the attacker, so "Environmental damage off" means exactly "no attacker"
(falls, traps, scripted damage).

Tested in game (2026-10-15, lab, forced charge via the debug global, a wolf in Breezehome): bites absorbed in the same
call (27.5 absorbed fully, 40 reduced to 10 at cap 30), character survives a fight that killed it without the shield;
threshold = Firebolt's magnitude (27 under Requiem) applied; Firebolt read as Destruction/fire; fire burst + burn
damage the wolf (155 -> 138 -> 120 -> 101 -> 83); MCM page registers. Balance rules tested the same way: Flames (magnitude 17
under Requiem) absorbed 17 of a 40 bite, then "on cooldown" for the next bites; Firebolt paid its own cost (39.6,
above the scaled 15 at skill 5); Lesser Ward absorbed 40-damage bites fully (cap 52.5) for its 22-magicka cost and
stayed up. Not tested automatically: the perfect-recoil window (needs a real cast), the cost ceiling on a huge hit,
ward-overhaul detection (none installed), enemy mages, the Restoration/Alteration riders, how the explosion looks.

---

## Balance Design

### Absorption Cap — Skill-Based Scaling

The maximum damage absorbed per trigger is based on the **skill level of the spell school** of the equipped spell:

```
Max Absorption = BaseAbsorption + (SkillLevel * ScalingFactor)
```

**Default values (all configurable in MCM):**

| Parameter | Default | Description |
|-----------|---------|-------------|
| BaseAbsorption | 20 | Minimum absorption even at low skill |
| ScalingFactor | 2.0 | Multiplier per skill level |
| Cap Formula Example | Destruction 50 → absorbs up to 120 damage | 20 + (50 × 2.0) |
| Cap Formula Example | Destruction 100 → absorbs up to 220 damage | 20 + (100 × 2.0) |

**Overflow behavior:** If the incoming hit exceeds the absorption cap, the player takes the difference as normal damage. Arcane Recoil still triggers (spell is still consumed, visual still plays), but it only partially protects.

### No Cooldown

- Arcane Recoil can trigger every single time the conditions are met
- Balance is inherently maintained because each trigger costs:
  - The spell charge (must re-cast)
  - All invested magicka
  - The offensive opportunity cost of that spell
- At low magicka, the player simply cannot sustain repeated triggers
- This creates natural diminishing returns — the more you rely on it, the faster you drain yourself

### School-Based Considerations

The spell's school determines the absorption cap AND (if enabled) the AoE element:

| School | Absorption Scaling | AoE Element (if enabled) |
|--------|-------------------|--------------------------|
| Destruction | Skill × ScalingFactor | Matches spell element (fire/frost/shock) |
| Conjuration | Skill × ScalingFactor | Purple/void energy |
| Alteration | Skill × (ScalingFactor × 1.25) | Golden arcane burst |
| Illusion | Skill × ScalingFactor | Shimmering psychic wave |
| Restoration | Skill × (ScalingFactor × 1.5) | White/golden holy burst |

**Design note:** Alteration and Restoration get a slight bonus multiplier since those schools thematically align with protection. This is MCM-configurable.

---

## MCM Configuration

### General Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| Mod Enabled | Toggle | ON | Master enable/disable |
| Left Hand Only | Toggle | ON | If OFF, also works with right hand |
| Environmental Damage | Toggle | OFF | Whether fall/trap damage can trigger |

### Absorption Settings

| Setting | Type | Default | Range | Description |
|---------|------|---------|-------|-------------|
| Base Absorption | Slider | 20 | 0–100 | Flat absorption amount |
| Scaling Factor | Slider | 2.0 | 0.5–5.0 | Per-skill-level multiplier |
| Alteration Bonus | Slider | 1.25 | 1.0–2.0 | Alteration school multiplier bonus |
| Restoration Bonus | Slider | 1.5 | 1.0–2.0 | Restoration school multiplier bonus |
| Overflow Behavior | Toggle | Partial | Partial / Full | Partial = take excess damage; Full = absorb entire hit or don't trigger |

### AoE Damage Settings

| Setting | Type | Default | Range | Description |
|---------|------|---------|-------|-------------|
| AoE Damage Enabled | Toggle | OFF | — | Master toggle for explosion dealing damage |
| AoE Damage Multiplier | Slider | 0.5 | 0.1–2.0 | Fraction of spell's base damage dealt as AoE |
| AoE Radius | Slider | 15 | 5–30 | Radius of the explosion in game units (feet) |
| Friendly Fire | Toggle | OFF | — | Whether AoE damages followers/friendlies |

### Visual/Audio Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| Screen Flash | Toggle | ON | Brief flash on trigger |
| Explosion VFX | Toggle | ON | Element-matched explosion visual |
| Detonation Sound | Toggle | ON | Sound effect on trigger |
| Notification Message | Toggle | ON | "Arcane Recoil absorbed X damage" |

---

## Technical Implementation Plan

### Architecture

This mod uses:
- **1 ESP plugin** — contains the spell ability, magic effects, and MCM quest/script properties
- **SKSE64** — for detecting left-hand spell charge state and intercepting incoming damage
- **Papyrus scripts** — for game logic, MCM menu, and visual effects
- **SkyUI MCM** — for configuration

### Key Scripts

#### `ArcaneRecoil_MainEffect.psc`
**Purpose:** Core logic script attached to an always-active ability on the player.

**Responsibilities:**
- Register for SKSE `OnHit` event on the player
- On each hit, check:
  1. Is left hand spell actively charging? (use `Game.GetPlayer().GetEquippedSpell(0)` and SKSE charge state detection)
  2. Calculate absorption cap from spell school + skill level + MCM settings
  3. If conditions met: negate damage, fire explosion, drain magicka, cancel charge
- Handle overflow (partial absorption)
- Send mod event for notification/VFX

**Key Functions:**
```papyrus
Event OnHit(ObjectReference akAggressor, Form akSource, Projectile akProjectile, bool abPowerAttack, bool abSneakAttack, bool abBashAttack, bool abHitBlocked)
  ; Check if mod enabled
  ; Check if left hand has spell equipped and is charging
  ; Get spell school, calculate absorption cap
  ; Apply damage negation
  ; Fire VFX and SFX
  ; Drain magicka and cancel spell charge
EndEvent
```

#### `ArcaneRecoil_MCM.psc`
**Purpose:** SkyUI MCM menu script.

**Responsibilities:**
- Register all MCM pages and options
- Store/load all configuration values as script properties
- Expose properties to MainEffect script via quest property linking

#### `ArcaneRecoil_VFX.psc`
**Purpose:** Handles visual and audio feedback.

**Responsibilities:**
- Play element-appropriate explosion art object at player's left hand node
- Play detonation sound
- Apply brief image space modifier (screen flash)
- Show notification message with damage absorbed amount

### ESP Records Needed

| Record Type | Name | Purpose |
|-------------|------|---------|
| Quest | ArcaneRecoil_MCMQuest | Holds MCM script, stores config properties |
| Spell (Ability) | ArcaneRecoil_CoreAbility | Always-active ability added to player on game load |
| Magic Effect | ArcaneRecoil_CoreEffect | Script effect carrier for OnHit detection |
| Magic Effect | ArcaneRecoil_ExplosionEffect | Element-matched explosion for AoE damage |
| Explosion | ArcaneRecoil_FireExplosion | Fire element burst |
| Explosion | ArcaneRecoil_FrostExplosion | Frost element burst |
| Explosion | ArcaneRecoil_ShockExplosion | Shock element burst |
| Explosion | ArcaneRecoil_ArcaneExplosion | Generic/other school burst |
| ImageSpaceModifier | ArcaneRecoil_FlashISM | Brief white flash on trigger |
| Sound Descriptor | ArcaneRecoil_DetonationSD | Custom detonation sound |
| Global Variable | ArcaneRecoil_Enabled | Master toggle |
| Global Variable | ArcaneRecoil_BaseAbsorption | Base absorption value |
| Global Variable | ArcaneRecoil_ScalingFactor | Per-level scaling |
| Message | ArcaneRecoil_AbsorbMsg | "Arcane Recoil absorbed {X} damage" |

### SKSE Considerations

**Critical dependency:** Detecting whether the player is actively *charging* a spell (holding the cast button) vs. merely having a spell equipped requires SKSE. Vanilla Papyrus cannot distinguish these states.

**Approach options (in order of preference):**

1. **PapyrusUtil + SKSE functions** — Use `Input.IsKeyPressed()` to detect if the left-hand cast key is held, combined with `GetEquippedSpell(0)` to confirm a spell is equipped. Simpler but less precise.

2. **CGO/custom SKSE plugin** — Write a small SKSE plugin that exposes a `IsSpellCharging(hand)` native function to Papyrus. Most accurate but requires C++ compilation.

3. **Animation event monitoring** — Register for animation events related to spell casting (`MagicLeftHandCharge`, etc.) and track state with a bool flag. Medium complexity.

**Recommendation for v1.0:** Start with approach 1 (input detection + equipped check). It covers 95% of cases. Upgrade to approach 2 in a later version for precision.

### Damage Negation Strategy

**The core challenge:** Skyrim's `OnHit` event fires *after* damage is applied in many cases. To truly negate damage:

**Option A — Restore health:** Calculate damage taken by comparing health before/after, then restore that amount instantly. Simple but can cause a visible health bar dip.

**Option B — Temporary invulnerability:** Apply a brief 100% damage resistance buff right as the charge state is detected as active, remove it on the next frame after the hit resolves. Cleaner but timing-sensitive.

**Option C — Cloak spell approach:** Use a constant cloak-like effect that checks for incoming damage and applies the absorption. This leverages the engine's own damage pipeline.

**Recommendation for v1.0:** Option A (health restoration) is most reliable and easiest to debug. The health bar dip is barely perceptible if restored in the same frame.

---

## File/Folder Structure

```
Arcane Recoil/
├── ArcaneRecoil.esp
├── Scripts/
│   ├── Source/
│   │   ├── ArcaneRecoil_MainEffect.psc
│   │   ├── ArcaneRecoil_MCM.psc
│   │   └── ArcaneRecoil_VFX.psc
│   ├── ArcaneRecoil_MainEffect.pex
│   ├── ArcaneRecoil_MCM.pex
│   └── ArcaneRecoil_VFX.pex
├── Interface/
│   └── ArcaneRecoil/
│       └── mcm_config.json    (SkyUI MCM registration)
├── Sound/
│   └── fx/
│       └── ArcaneRecoil/
│           └── detonation.wav
└── SKSE/
    └── Plugins/               (if using custom SKSE plugin later)
```

**Nexus-ready packaging:** This structure is SE-compatible and ready for FOMOD installer wrapping if desired.

---

## Development Phases

### Phase 1 — Minimum Viable Mod
- Core OnHit detection with input-based charge detection
- Damage absorption (health restore method) with flat cap
- Single generic explosion VFX (reuse vanilla explosion art)
- Basic notification message
- No MCM (hardcoded defaults)

### Phase 2 — Full Feature Set
- MCM with all settings
- Skill-based absorption cap scaling
- Element-matched explosion VFX (fire/frost/shock/arcane)
- Custom detonation sound
- Screen flash ImageSpaceModifier
- AoE damage toggle with configurable radius/multiplier

### Phase 3 — Polish & Edge Cases
- Handle dual-casting edge case (both hands charging same spell)
- Handle ward spells (should wards interact differently?)
- Compatibility patches for popular magic overhauls (Apocalypse, Mysticism, Arcanum)
- Optional perk tree integration (Ordinator compatibility)
- SKSE plugin for precise charge detection (replaces input method)

---

## Compatibility Notes

- **Wildcat/Smilodon:** Should be compatible — those mods modify damage calculations but don't override OnHit entirely. Test for absorption cap interaction.
- **Apocalypse/Mysticism:** Compatible — new spells still have school assignments and work with GetEquippedSpell.
- **Ordinator:** Compatible — no perk tree conflicts. Phase 3 could add Ordinator-aware perks.
- **CGO/MCO:** Test animation event compatibility — these mods alter combat animations and may affect charge state detection.
- **SPID:** Could use SPID to distribute the ability instead of quest-based auto-application.

---

## Nexus Description Draft

**Arcane Recoil** — your charged spell is your last line of defense.

Charging a spell in your off-hand? Get hit by an enemy? Instead of taking damage, your spell explodes outward, shielding you — but consuming your charge and magicka in the process.

**Features:**
- Reactive spell shield triggered by charging a spell in your left hand
- Damage absorption scales with your skill in the spell's school
- Element-matched explosion visuals (fire, frost, shock, arcane)
- Optional AoE damage to nearby enemies
- Fully configurable via SkyUI MCM
- No cooldown — balanced by magicka consumption
- Lightweight, script-clean, compatible with major overhauls

**Think of it as:** A magical parry system. Hold your spell, bait the hit, absorb it — but now you're drained and exposed. High risk, high reward gameplay for pure mages.
