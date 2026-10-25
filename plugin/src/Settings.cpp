#include "Settings.h"
#include "FormIDs.h"

namespace Settings
{
    // ── Internal global variable cache ────────────────────────────────────
    // Each pointer corresponds to one GlobalVariable record in the ESP.
    // Defaults are used as fallbacks if a global isn't found (shouldn't happen
    // in a properly set-up install, but avoids crashes during development).
    namespace
    {
        struct GlobalCache
        {
            RE::TESGlobal* enabled           = nullptr;
            RE::TESGlobal* leftHandOnly      = nullptr;
            RE::TESGlobal* environmental     = nullptr;
            RE::TESGlobal* baseAbsorption    = nullptr;
            RE::TESGlobal* scalingFactor     = nullptr;
            RE::TESGlobal* alterationBonus   = nullptr;
            RE::TESGlobal* restorationBonus  = nullptr;
            RE::TESGlobal* partialAbsorption = nullptr;
            RE::TESGlobal* aoeEnabled        = nullptr;
            RE::TESGlobal* aoeMultiplier     = nullptr;
            RE::TESGlobal* aoeRadius         = nullptr;
            RE::TESGlobal* friendlyFire      = nullptr;
            RE::TESGlobal* screenFlash       = nullptr;
            RE::TESGlobal* explosionVFX      = nullptr;
            RE::TESGlobal* detonationSound   = nullptr;
            RE::TESGlobal* notification      = nullptr;
            RE::TESGlobal* triggerThreshold  = nullptr;
            RE::TESGlobal* drainAllMagicka   = nullptr;
            RE::TESGlobal* staggerEnabled    = nullptr;
            RE::TESGlobal* staggerStrength   = nullptr;
            RE::TESGlobal* perfectWindow     = nullptr;
            RE::TESGlobal* riders            = nullptr;
            RE::TESGlobal* riderMultiplier   = nullptr;
            RE::TESGlobal* wardBonus         = nullptr;
            RE::TESGlobal* npcMages          = nullptr;
            RE::TESGlobal* debugLog          = nullptr;
            RE::TESGlobal* debugForceCharge  = nullptr;
            RE::TESGlobal* costRatio         = nullptr;
            RE::TESGlobal* costMaxFraction   = nullptr;
            RE::TESGlobal* perfectCostMult   = nullptr;
            RE::TESGlobal* concCapMult       = nullptr;
            RE::TESGlobal* concCooldown      = nullptr;
            RE::TESGlobal* wardsEnabled      = nullptr;
            RE::TESGlobal* wardsDefer        = nullptr;
        } g_globals;

        RE::TESImageSpaceModifier* g_flashIMOD = nullptr;

        // Helper: look up a GlobalVariable by EditorID and warn if missing.
        RE::TESGlobal* LookupGlobal(const char* editorID)
        {
            auto* g = RE::TESForm::LookupByEditorID<RE::TESGlobal>(editorID);
            if (!g) {
                LOG_WARN("ArcaneRecoil: GlobalVariable '{}' not found — using default.", editorID);
            }
            return g;
        }

        // Read a global's float value, returning `fallback` if the pointer is null.
        float ReadFloat(RE::TESGlobal* g, float fallback)
        {
            return g ? g->value : fallback;
        }

        bool ReadBool(RE::TESGlobal* g, bool fallback)
        {
            return g ? (g->value > 0.5f) : fallback;
        }
    }

    // ── Public API ────────────────────────────────────────────────────────

    void CacheGlobals()
    {
        // EditorIDs must match exactly what you create in the CK.
        g_globals.enabled           = LookupGlobal("ArcaneRecoil_Enabled");
        g_globals.leftHandOnly      = LookupGlobal("ArcaneRecoil_LeftHandOnly");
        g_globals.environmental     = LookupGlobal("ArcaneRecoil_Environmental");
        g_globals.baseAbsorption    = LookupGlobal("ArcaneRecoil_BaseAbsorption");
        g_globals.scalingFactor     = LookupGlobal("ArcaneRecoil_ScalingFactor");
        g_globals.alterationBonus   = LookupGlobal("ArcaneRecoil_AlterationBonus");
        g_globals.restorationBonus  = LookupGlobal("ArcaneRecoil_RestorationBonus");
        g_globals.partialAbsorption = LookupGlobal("ArcaneRecoil_PartialAbsorption");
        g_globals.aoeEnabled        = LookupGlobal("ArcaneRecoil_AoEEnabled");
        g_globals.aoeMultiplier     = LookupGlobal("ArcaneRecoil_AoEMultiplier");
        g_globals.aoeRadius         = LookupGlobal("ArcaneRecoil_AoERadius");
        g_globals.friendlyFire      = LookupGlobal("ArcaneRecoil_FriendlyFire");
        g_globals.screenFlash       = LookupGlobal("ArcaneRecoil_ScreenFlash");
        g_globals.explosionVFX      = LookupGlobal("ArcaneRecoil_ExplosionVFX");
        g_globals.detonationSound   = LookupGlobal("ArcaneRecoil_DetonationSound");
        g_globals.notification      = LookupGlobal("ArcaneRecoil_Notification");
        g_globals.triggerThreshold  = LookupGlobal("ArcaneRecoil_TriggerThreshold");
        g_globals.drainAllMagicka   = LookupGlobal("ArcaneRecoil_DrainAllMagicka");
        g_globals.staggerEnabled    = LookupGlobal("ArcaneRecoil_StaggerEnabled");
        g_globals.staggerStrength   = LookupGlobal("ArcaneRecoil_StaggerStrength");
        g_globals.perfectWindow     = LookupGlobal("ArcaneRecoil_PerfectWindow");
        g_globals.riders            = LookupGlobal("ArcaneRecoil_Riders");
        g_globals.riderMultiplier   = LookupGlobal("ArcaneRecoil_RiderMultiplier");
        g_globals.wardBonus         = LookupGlobal("ArcaneRecoil_WardBonus");
        g_globals.npcMages          = LookupGlobal("ArcaneRecoil_NPCMages");
        g_globals.debugLog          = LookupGlobal("ArcaneRecoil_DebugLog");
        g_globals.debugForceCharge  = LookupGlobal("ArcaneRecoil_DebugForceCharge");
        g_globals.costRatio         = LookupGlobal("ArcaneRecoil_CostRatio");
        g_globals.costMaxFraction   = LookupGlobal("ArcaneRecoil_CostMaxFraction");
        g_globals.perfectCostMult   = LookupGlobal("ArcaneRecoil_PerfectCostMult");
        g_globals.concCapMult       = LookupGlobal("ArcaneRecoil_ConcentrationCapMult");
        g_globals.concCooldown      = LookupGlobal("ArcaneRecoil_ConcentrationCooldown");
        g_globals.wardsEnabled      = LookupGlobal("ArcaneRecoil_WardsEnabled");
        g_globals.wardsDefer        = LookupGlobal("ArcaneRecoil_WardsDeferToMods");

        LOG_INFO("ArcaneRecoil: Settings globals cached.");
    }

    void CacheVFXForms()
    {
        g_flashIMOD = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESImageSpaceModifier>(
            FormIDs::ArcaneRecoil_FlashISM, FormIDs::Plugin);
        if (!g_flashIMOD) {
            LOG_WARN("ArcaneRecoil: ArcaneRecoil_FlashISM not found — screen flash disabled.");
        }

        LOG_INFO("ArcaneRecoil: VFX forms cached.");
    }

    bool  ModEnabled()        { return ReadBool (g_globals.enabled,           true);  }
    bool  LeftHandOnly()      { return ReadBool (g_globals.leftHandOnly,      true);  }
    bool  EnvironmentalDamage(){ return ReadBool(g_globals.environmental,     false); }
    float BaseAbsorption()    { return ReadFloat(g_globals.baseAbsorption,    20.0f); }
    float ScalingFactor()     { return ReadFloat(g_globals.scalingFactor,     2.0f);  }
    float AlterationBonus()   { return ReadFloat(g_globals.alterationBonus,   1.25f); }
    float RestorationBonus()  { return ReadFloat(g_globals.restorationBonus,  1.5f);  }
    bool  PartialAbsorption() { return ReadBool (g_globals.partialAbsorption, true);  }
    bool  AoEEnabled()        { return ReadBool (g_globals.aoeEnabled,        false); }
    float AoEMultiplier()     { return ReadFloat(g_globals.aoeMultiplier,     0.5f);  }
    float AoERadius()         { return ReadFloat(g_globals.aoeRadius,         15.0f) * 22.2f; } // ft → units
    bool  FriendlyFire()      { return ReadBool (g_globals.friendlyFire,      false); }
    bool  ScreenFlash()       { return ReadBool (g_globals.screenFlash,       true);  }
    bool  ExplosionVFX()      { return ReadBool (g_globals.explosionVFX,      true);  }
    bool  DetonationSound()   { return ReadBool (g_globals.detonationSound,   true);  }
    bool  Notification()      { return ReadBool (g_globals.notification,      true);  }

    float TriggerThreshold()  { return ReadFloat(g_globals.triggerThreshold,  1.0f);  }
    bool  DrainAllMagicka()   { return ReadBool (g_globals.drainAllMagicka,   false); }
    bool  StaggerEnabled()    { return ReadBool (g_globals.staggerEnabled,    true);  }
    float StaggerStrength()   { return ReadFloat(g_globals.staggerStrength,   0.5f);  }
    float PerfectWindow()     { return ReadFloat(g_globals.perfectWindow,     0.35f); }
    bool  Riders()            { return ReadBool (g_globals.riders,            true);  }
    float RiderMultiplier()   { return ReadFloat(g_globals.riderMultiplier,   0.25f); }
    float WardBonus()         { return ReadFloat(g_globals.wardBonus,         1.5f);  }
    bool  NPCMages()          { return ReadBool (g_globals.npcMages,          false); }
    bool  DebugLog()          { return ReadBool (g_globals.debugLog,          false); }
    bool  DebugForceCharge()  { return ReadBool (g_globals.debugForceCharge,  false); }
    float CostRatio()         { return ReadFloat(g_globals.costRatio,         0.5f);  }
    float CostMaxFraction()   { return ReadFloat(g_globals.costMaxFraction,   0.6f);  }
    float PerfectCostMult()   { return ReadFloat(g_globals.perfectCostMult,   0.5f);  }
    float ConcentrationCapMult()  { return ReadFloat(g_globals.concCapMult,   1.0f);  }
    float ConcentrationCooldown() { return ReadFloat(g_globals.concCooldown,  3.0f);  }
    bool  WardsEnabled()      { return ReadBool (g_globals.wardsEnabled,      true);  }
    bool  WardsDeferToMods()  { return ReadBool (g_globals.wardsDefer,        true);  }

    RE::TESImageSpaceModifier* FlashIMOD() { return g_flashIMOD; }

    std::string ExplosionTypeString(int school, int element)
    {
        // Destruction: use element sub-type for matched VFX
        if (school == 2) {
            if (element == 0) return "FIRE";
            if (element == 1) return "FROST";
            if (element == 2) return "SHOCK";
            return "ARCANE";
        }
        if (school == 0) return "ALT";
        if (school == 1) return "CONJ";
        if (school == 3) return "ILLU";
        if (school == 4) return "REST";
        return "ARCANE";
    }
}
