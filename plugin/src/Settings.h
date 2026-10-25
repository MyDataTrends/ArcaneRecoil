#pragma once
#include "PCH.h"

// =============================================================================
// Settings
// All user-configurable values live in GlobalVariable records in ArcaneRecoil.esp.
// The MCM (Papyrus) writes to those globals; we cache their pointers here once
// at kDataLoaded, then read them directly on every hit.
//
// GlobalVariable EditorIDs match what's in ArcaneRecoil_CK_Guide.md.
// =============================================================================

namespace Settings
{
    // Call once after kDataLoaded. Logs a warning for any global not found.
    void CacheGlobals();

    // Also cache the VFX form records (Explosions, IMOD) here for fast lookup.
    void CacheVFXForms();

    // ── Core toggles ──────────────────────────────────────────────────────
    [[nodiscard]] bool ModEnabled();
    [[nodiscard]] bool LeftHandOnly();
    [[nodiscard]] bool EnvironmentalDamage();

    // ── Absorption formula ────────────────────────────────────────────────
    [[nodiscard]] float BaseAbsorption();
    [[nodiscard]] float ScalingFactor();
    [[nodiscard]] float AlterationBonus();   // multiplier on ScalingFactor
    [[nodiscard]] float RestorationBonus();  // multiplier on ScalingFactor
    [[nodiscard]] bool  PartialAbsorption(); // true=partial, false=full-or-nothing

    // ── AoE ───────────────────────────────────────────────────────────────
    [[nodiscard]] bool  AoEEnabled();
    [[nodiscard]] float AoEMultiplier();
    [[nodiscard]] float AoERadius();         // in Skyrim units (feet × 22.2)
    [[nodiscard]] bool  FriendlyFire();

    // ── VFX toggles (C++ only reads; Papyrus also reads these) ───────────
    [[nodiscard]] bool ScreenFlash();
    [[nodiscard]] bool ExplosionVFX();       // sent to Papyrus via mod event
    [[nodiscard]] bool DetonationSound();    // sent to Papyrus via mod event
    [[nodiscard]] bool Notification();

    // ── v1.1 ──────────────────────────────────────────────────────────────
    [[nodiscard]] float TriggerThreshold();  // trigger only if damage > held spell magnitude x this (0 = any hit)
    [[nodiscard]] bool  DrainAllMagicka();   // false = the spell's cast cost (spec), true = whole pool (v1.0)
    [[nodiscard]] bool  StaggerEnabled();
    [[nodiscard]] float StaggerStrength();   // stagger push magnitude (doubled on a perfect recoil)
    [[nodiscard]] float PerfectWindow();     // seconds after cast start that count as a perfect recoil
    [[nodiscard]] bool  Riders();
    [[nodiscard]] float RiderMultiplier();   // rider magnitude = absorbed damage x this
    [[nodiscard]] float WardBonus();         // absorption cap multiplier while holding a ward
    [[nodiscard]] bool  NPCMages();          // enemy (and follower) mages can recoil too
    [[nodiscard]] bool  DebugLog();
    [[nodiscard]] bool  DebugForceCharge();  // test only: treat the player as charging the left hand

    // -- v1.1 balance (author, 2026-10-15) --
    [[nodiscard]] float CostRatio();             // magicka cost = absorbed x this (at least the spell's cost)
    [[nodiscard]] float CostMaxFraction();       // ...but never more than this share of max magicka
    [[nodiscard]] float PerfectCostMult();       // perfect recoil pays this share of the cost
    [[nodiscard]] float ConcentrationCapMult();  // concentration spells absorb at most magnitude x this
    [[nodiscard]] float ConcentrationCooldown(); // seconds before a concentration spell can recoil again
    [[nodiscard]] bool  WardsEnabled();
    [[nodiscard]] bool  WardsDeferToMods();      // leave wards alone when a ward overhaul is installed

    // ── Cached VFX records ────────────────────────────────────────────────
    // School indices: 0=Alt, 1=Conj, 2=Dest, 3=Illu, 4=Rest, -1=Other
    // Element indices (Destruction only): 0=Fire, 1=Frost, 2=Shock, -1=Other
    //
    // Returns a short string like "FIRE", "FROST", "SHOCK", "ALT", etc.
    // to pass as strArg in the SKSE mod event to the Papyrus VFX script.
    [[nodiscard]] std::string ExplosionTypeString(int school, int element);

    [[nodiscard]] RE::TESImageSpaceModifier* FlashIMOD();
}
