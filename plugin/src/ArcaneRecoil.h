#pragma once
#include "PCH.h"

namespace ArcaneRecoil {

// Call from Main.cpp after kDataLoaded.
// Registers the TESHitEvent sink on the source manager.
void Install();

// Call after a save loads or a new game starts.
void OnGameLoaded();
} // namespace ArcaneRecoil
