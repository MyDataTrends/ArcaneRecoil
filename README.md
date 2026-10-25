# Arcane Recoil

A magical parry for Skyrim. Charge a spell, take a hit, and instead of the damage landing, your spell detonates
outward: it soaks the blow, staggers the attacker and bursts over the enemies around you. The charge and part of your
magicka are spent, so you're left exposed for a moment. Hold your spell for offense, or keep it ready as an emergency
shield.

## What happens on a recoil
- **The hit is absorbed** inside the game's own damage handling, so even a killing blow is caught.
- **It only triggers on hits bigger than the spell you're holding** (its magnitude; adjustable), so chip damage
  doesn't waste your charge.
- **Magicka cost scales with what it saved you:** the larger of the spell's cost and half the absorbed damage,
  capped at 60% of your maximum magicka (all adjustable; or drain everything, old-school).
- **The burst is a real spell cast by you,** matched to the spell's element: resistances apply, and you get the
  aggro, the XP and the kill credit.
- **The attacker is staggered.**
- **Element riders:** fire burns, frost saps stamina, shock drains magicka; Restoration heals you, and Alteration
  briefly hardens your skin.
- **Perfect recoil:** get hit within a moment of starting your cast (0.35 s by default) for half the cost and a double
  stagger.
- **Wards are the strong option:** a bigger cap, any hit recoils, and the ward stays up. They're left alone
  automatically when Perfectly Valid Wards or Wards Functionalities Extended is installed (you can turn that off).
- **Concentration spells** (Flames, Healing...) absorb at most their own magnitude, then rest for 3 s.
- **Enemy mages can recoil too** (off by default).
- The explosion comes from your casting hand, with a sound and an optional screen flash. Bursts are throttled so a
  stream of hits on a ward doesn't spam effects.

## Settings
Every setting is in the MCM (MCM Helper page): on/off, left hand only, outside combat, enemy mages, trigger
threshold, absorption (base, per skill point, Alteration / Restoration / ward multipliers, partial absorption), cost
(ratio, ceiling, perfect recoil share, drain all), concentration cap and cooldown, wards, stagger, perfect window,
riders, burst (damage, radius, friendly fire), effects (flash, explosion, sound, notification), debug log.

## Requirements
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- For the settings page: [MCM Helper](https://www.nexusmods.com/skyrimspecialedition/mods/53000) and
  [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604). Without them the defaults apply.

Built with CommonLibSSE-NG for Skyrim SE and AE. Tested on Skyrim 1.6.1170 (Steam), in a Requiem list.
`ArcaneRecoil.esp` is flagged light (ESL) and adds its own records only.

## Building
- `plugin/`: the SKSE plugin. Visual Studio 2022 with vcpkg, then
  `cmake --preset relwithdebinfo` and `cmake --build build/relwithdebinfo --config RelWithDebInfo`.
- `Scripts/Source/`: Papyrus scripts (Creation Kit compiler).
- `arcane_recoil.toml`: the plugin's records as a declarative spec; the author's own tooling builds `ArcaneRecoil.esp`
  and `plugin/src/FormIDs.h` from it. The built plugin is included in the release.
- `SpellShield_Mod_Spec.md`: the design notes and how each rule was tested.

## License
MIT, see [LICENSE](LICENSE). Use it, change it, include it in your lists and mods.
