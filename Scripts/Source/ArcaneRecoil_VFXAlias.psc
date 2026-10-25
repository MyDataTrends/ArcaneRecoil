Scriptname ArcaneRecoil_VFXAlias extends ReferenceAlias
{Arcane Recoil v1.1: SKSE does not keep mod event registrations in the save, so the VFX quest re-registers on every
game load.}

Event OnPlayerLoadGame()
    (GetOwningQuest() as ArcaneRecoil_VFX).Register()
EndEvent
