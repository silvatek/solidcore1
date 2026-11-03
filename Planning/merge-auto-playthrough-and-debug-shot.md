# Merge Auto playthrough and Debug Shot

Status: New

## Story

Auto playthrough and Debug Shot become one script runner. A script has a name and a list of commands. Several named scripts can be defined. Each script has a flag that says whether it is on the F10 menu.

The two scripts are Full-playthrough and Companion parade. Full-playthrough is the script Auto playthrough runs now. Companion parade is the current Debug Shot: Sam, the Parade formation, true sight, 10 m south of the Dragongate mark, facing south, a three-second wait, and a playshot.

Full-playthrough's flag is set. Companion parade's flag is clear. The F10 menu lists each script whose flag is set, in place of the Auto playthrough row and the Debug Shot row. Choosing that row runs that script and leaves the game running.

The commands Auto playthrough already uses stay, including `BuildSquad` and `SelectBattlePlan`. A script can also portal to a marker, take a playshot, and pause for a number of seconds. `PortalToMarker` takes a mark kind, a mark name, and X and Y offsets. `BuildSquad` names a companion. `SelectBattlePlan` selects a formation. `EnableSight` enables a sight mode. `FaceDirection` sets the facing. `TakePlayshot` is the same capture as Alt-P, and a script can take more than one.

The command line launches the game with `-Autoplay=` and the script name. That launch runs the script and quits when the script has finished. `-Autoplay=Full-playthrough` runs Full-playthrough.

`FSolidAutoPlaythrough` stays the class for the script runner. This story introduces no new game concept.

## Acceptance

- More than one script can be defined, and each script has a name.
- The scripts are Full-playthrough and Companion parade. Full-playthrough is the script Auto playthrough runs now. Companion parade places the company 10 m south of the Dragongate mark, with Sam, the Parade formation, true sight, and facing south. It waits three seconds and takes a playshot.
- Each script has a flag for the F10 menu. Full-playthrough's flag is set, and Companion parade's flag is clear. The menu lists each script whose flag is set, so the row is Full-playthrough. Choosing that row runs that script and leaves the game running.
- The command-line switch is `-Autoplay=` followed by the script name. `-Autoplay=Full-playthrough` runs Full-playthrough, and the game quits when the script has finished.
- A script can use the commands Auto playthrough already has.
- A script can use `PortalToMarker`, `TakePlayshot`, `Pause(nSeconds)`, `EnableSight`, and `FaceDirection`. `PortalToMarker` takes a mark kind, a mark name, and X and Y offsets. `BuildSquad` names a companion, and `SelectBattlePlan` selects a formation. `EnableSight` enables a sight mode. `TakePlayshot` writes the same playshot as Alt-P, and a script can take more than one. `Pause` waits that many seconds, then the script continues. `FaceDirection` sets the facing.

## Implementation notes

- The commands already on `FSolidAutoPlaythrough::FStep` are WalkTo, MoveTo, CloseMessage, CraftItem, EquipItem, DisableEncounter, ExitStronghold, EnterLair, ExitLair, WakeInStronghold, SelectBattlePlan, AimRaven, ChooseStronghold, BuildSquad, and Custom. WalkTo is built with `WalkToMarker`.
- `WalkToMarker` takes the row label, the mark kind, and the mark name. `PortalToMarker` takes those three, plus an X offset and a Y offset. South is negative Y. Companion parade's offset is 10 m south of Dragongate, which is `SouthOfMarkCm` 1000.
- `Pause` today is `.Pause(seconds)` on another step. This story adds `Pause(nSeconds)` as its own step.
- Companion parade uses the settings in `SolidDebugShot`: Dragongate, Sam, Parade, true sight, `SignFacesSouthYawDeg`, and `CaptureDelaySeconds` 3. The offset is on `PortalToMarker`. Sam is `BuildSquad`, Parade is `SelectBattlePlan`, true sight is `EnableSight`, and the facing is `FaceDirection`.
- `-Autoplay=` is read with `FParse::Value`. Companion parade's name contains a space, so that launch quotes the name: `-Autoplay="Companion parade"`.
