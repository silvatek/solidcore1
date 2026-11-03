# Merge Auto playthrough and Debug Shot

Status: New

## Story

Auto playthrough and Debug Shot become one script runner. A script has a name and a list of commands. Several named scripts can be defined. Each script has a flag that says whether it is on the F10 menu.

The F10 menu lists each script whose flag is set, in place of the Auto playthrough row and the Debug Shot row. Choosing that row runs that script.

The commands Auto playthrough already uses stay. A script can also portal to a marker, take a playshot, and pause for a number of seconds.

The command line can launch the game on a named script. That launch runs the script and quits when the script has finished.

`FSolidAutoPlaythrough` stays the class for the script runner. This story introduces no new game concept.

## Acceptance

- More than one script can be defined, and each script has a name.
- Each script has a flag for the F10 menu. The menu lists each script whose flag is set. Choosing that row runs that script.
- A command-line launch can name a script. That launch runs the named script, and the game quits when the script has finished.
- A script can use the commands Auto playthrough already has.
- A script can use `PortalToMarker`, `TakePlayshot`, and `Pause(nSeconds)`. `Pause` waits that many seconds, then the script continues.

## Implementation notes

- The commands already on `FSolidAutoPlaythrough::FStep` are WalkTo, MoveTo, CloseMessage, CraftItem, EquipItem, DisableEncounter, ExitStronghold, EnterLair, ExitLair, WakeInStronghold, SelectBattlePlan, AimRaven, ChooseStronghold, BuildSquad, and Custom. WalkTo is built with `WalkToMarker`.
- `Pause` today is `.Pause(seconds)` on another step. This story adds `Pause(nSeconds)` as its own step.
