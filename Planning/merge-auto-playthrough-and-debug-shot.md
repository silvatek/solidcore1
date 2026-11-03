# Merge Auto playthrough and Debug Shot

Status: New

## Story

Auto playthrough and Debug Shot become one script runner. A script has a name and a list of commands. Several named scripts can be defined. Each script has a flag that says whether it is on the F10 menu.

The two scripts are Full-playthrough and Companion parade. Full-playthrough is the script Auto playthrough runs now. Companion parade is the current Debug Shot: Sam, the Parade formation, true sight, 10 m south of the Dragongate mark, facing south, a three-second wait, and a playshot.

The F10 menu lists each script whose flag is set, in place of the Auto playthrough row and the Debug Shot row. Choosing that row runs that script and leaves the game running.

The commands Auto playthrough already uses stay. A script can also portal to a marker, take a playshot, and pause for a number of seconds. `PortalToMarker` takes a mark kind, a mark name, and X and Y offsets. Companions, formation, sight, and facing are their own commands. The facing command is `FaceDirection`. `TakePlayshot` is the same capture as Alt-P, and a script can take more than one.

The command line can launch the game on a named script. That launch runs the script and quits when the script has finished.

`FSolidAutoPlaythrough` stays the class for the script runner. This story introduces no new game concept.

## Acceptance

- More than one script can be defined, and each script has a name.
- The scripts are Full-playthrough and Companion parade. Full-playthrough is the script Auto playthrough runs now. Companion parade places the company 10 m south of the Dragongate mark, with Sam, the Parade formation, true sight, and facing south. It waits three seconds and takes a playshot.
- Each script has a flag for the F10 menu. The menu lists each script whose flag is set. Choosing that row runs that script and leaves the game running.
- A command-line launch can name a script. That launch runs the named script, and the game quits when the script has finished.
- A script can use the commands Auto playthrough already has.
- A script can use `PortalToMarker`, `TakePlayshot`, `Pause(nSeconds)`, and `FaceDirection`. `PortalToMarker` takes a mark kind, a mark name, and X and Y offsets. Companions, formation, and sight are their own commands. `TakePlayshot` writes the same playshot as Alt-P, and a script can take more than one. `Pause` waits that many seconds, then the script continues. `FaceDirection` sets the facing.

## Implementation notes

- The commands already on `FSolidAutoPlaythrough::FStep` are WalkTo, MoveTo, CloseMessage, CraftItem, EquipItem, DisableEncounter, ExitStronghold, EnterLair, ExitLair, WakeInStronghold, SelectBattlePlan, AimRaven, ChooseStronghold, BuildSquad, and Custom. WalkTo is built with `WalkToMarker`.
- `WalkToMarker` takes the row label, the mark kind, and the mark name. `PortalToMarker` takes those three, plus an X offset and a Y offset. South is negative Y. Companion parade's offset is 10 m south of Dragongate, which is `SouthOfMarkCm` 1000.
- `Pause` today is `.Pause(seconds)` on another step. This story adds `Pause(nSeconds)` as its own step.
- Companion parade uses the settings in `SolidDebugShot`: Dragongate, Sam, Parade, true sight, `SignFacesSouthYawDeg`, and `CaptureDelaySeconds` 3. The offset is on `PortalToMarker`. Sam, Parade, true sight, and the facing are the companion command, the formation command, the sight command, and `FaceDirection`.
