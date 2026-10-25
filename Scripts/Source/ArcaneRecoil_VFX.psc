; =============================================================================
; ArcaneRecoil_VFX.psc
; Script Type : Quest
; Attached To : ArcaneRecoil_VFXQuest (Quest record, Start Game Enabled)
; Purpose     : Listens for the "ArcaneRecoil_VFX" SKSE mod event sent by the
;               C++ plugin, then places the matching explosion and plays the
;               detonation sound. Screen flash and HUD notification are handled
;               entirely in C++ (no Papyrus code needed for those).
; =============================================================================

Scriptname ArcaneRecoil_VFX extends Quest


; --------------------------------------------------------------------------
; Explosion records — set each in CK to the matching Explosion record.
; See ArcaneRecoil_CK_Guide.md for which vanilla records to copy as templates.
; --------------------------------------------------------------------------
Explosion Property ExplosionFire        Auto
Explosion Property ExplosionFrost       Auto
Explosion Property ExplosionShock       Auto
Explosion Property ExplosionArcane      Auto   ; Generic fallback + other Destruction
Explosion Property ExplosionAlteration  Auto   ; Can reuse ExplosionArcane initially
Explosion Property ExplosionConjuration Auto   ; Can reuse ExplosionArcane initially
Explosion Property ExplosionIllusion    Auto   ; Can reuse ExplosionArcane initially
Explosion Property ExplosionRestoration Auto   ; Can reuse ExplosionArcane initially

; --------------------------------------------------------------------------
; Sound Descriptor — point to ArcaneRecoil_DetonationSD in the CK.
; --------------------------------------------------------------------------
Sound Property SD_Detonation Auto

; v1.1: an XMarker is moved to the casting hand so the burst comes out of the hand, not the feet
Static Property XMarker Auto

; --------------------------------------------------------------------------
; VFX toggle globals — read to decide whether to play each effect.
; Set in CK to the same GlobalVariable records the MCM reads/writes.
; --------------------------------------------------------------------------
GlobalVariable Property gExplosionVFX    Auto
GlobalVariable Property gDetonationSound Auto


; ==========================================================================
; LIFECYCLE
; ==========================================================================

Event OnInit()
    Register()
EndEvent

; Register for the SKSE mod event broadcast by the C++ plugin. Called on init and, through the player alias
; (ArcaneRecoil_VFXAlias), on every game load: registrations are not kept in the save.
; strArg: "FIRE", "FROST", "SHOCK", "ARCANE", "ALT", "CONJ", "ILLU", "REST"; numArg: 0 = left hand, 1 = right hand;
; sender: the actor that recoiled (the player, or an enemy mage).
Function Register()
    RegisterForModEvent("ArcaneRecoil_VFX", "OnArcaneRecoilVFX")
EndFunction


; ==========================================================================
; VFX EVENT HANDLER
; ==========================================================================

; v1.2: one burst per actor per VFX_COOLDOWN seconds. A ward recoils on every hit and is not used up, so a held
; Flames against an enemy's ward fired a full explosion + sound on every damage tick (user, 2026-10-18). The recoil
; itself still happens on every hit (C++); only the effects are throttled.
Float Property VFX_COOLDOWN = 1.5 AutoReadOnly
Actor[] recentActors
Float[] recentTimes

Bool Function OnCooldown(Actor a)
    If recentActors == None
        recentActors = new Actor[8]
        recentTimes = new Float[8]
    EndIf
    Float now = Utility.GetCurrentRealTime()
    Int oldest = 0
    Int i = 0
    While i < 8
        If recentActors[i] == a
            If now - recentTimes[i] < VFX_COOLDOWN
                Return True
            EndIf
            recentTimes[i] = now
            Return False
        EndIf
        If recentTimes[i] < recentTimes[oldest]
            oldest = i
        EndIf
        i += 1
    EndWhile
    recentActors[oldest] = a
    recentTimes[oldest] = now
    Return False
EndFunction

Event OnArcaneRecoilVFX(String eventName, String strArg, Float numArg, Form sender)
    Actor caster = sender as Actor
    If caster == None
        caster = Game.GetPlayer()
    EndIf
    If OnCooldown(caster)
        Return
    EndIf
    Bool exploded = False

    ; ── Explosion VFX, from the casting hand ─────────────────────────────
    If gExplosionVFX != None && gExplosionVFX.GetValue() > 0.5
        Explosion exp = GetExplosion(strArg)
        If exp != None
            ObjectReference origin = caster
            ObjectReference marker = None
            If XMarker != None
                marker = caster.PlaceAtMe(XMarker)
                If numArg > 0.5
                    marker.MoveToNode(caster, "NPC R Hand [RHnd]")
                Else
                    marker.MoveToNode(caster, "NPC L Hand [LHnd]")
                EndIf
                origin = marker
            EndIf
            origin.PlaceAtMe(exp)
            exploded = True
            If marker != None
                marker.Delete()
            EndIf
        EndIf
    EndIf

    ; ── Detonation sound ─────────────────────────────────────────────────
    ; v1.2: the impact-sized explosions carry their own short sound, so the ward-break sound is now only the fallback
    ; when the explosion is off, and at half volume (it played on top of the fireball's own boom before)
    If !exploded && gDetonationSound != None && gDetonationSound.GetValue() > 0.5
        If SD_Detonation != None
            Int inst = SD_Detonation.Play(caster)
            Sound.SetInstanceVolume(inst, 0.5)
        EndIf
    EndIf
EndEvent


; ==========================================================================
; HELPER — Map explosion type string to the right Explosion record
; ==========================================================================

Explosion Function GetExplosion(String typeStr)
    If typeStr == "FIRE"
        Return ExplosionFire
    ElseIf typeStr == "FROST"
        Return ExplosionFrost
    ElseIf typeStr == "SHOCK"
        Return ExplosionShock
    ElseIf typeStr == "ALT"
        If ExplosionAlteration != None
            Return ExplosionAlteration
        EndIf
    ElseIf typeStr == "CONJ"
        If ExplosionConjuration != None
            Return ExplosionConjuration
        EndIf
    ElseIf typeStr == "ILLU"
        If ExplosionIllusion != None
            Return ExplosionIllusion
        EndIf
    ElseIf typeStr == "REST"
        If ExplosionRestoration != None
            Return ExplosionRestoration
        EndIf
    EndIf
    Return ExplosionArcane  ; Default fallback
EndFunction
