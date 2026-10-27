# PIE screenshots

Status: In Progress

## Story

The developer plays with `tools\pie.bat` and presses Alt-P. The game writes a picture of what the player is seeing into `Saved/Playshots`. The file name carries the build id and the local time. The solidcore1-ue worker reads those files when the developer says they are testing in Play In Editor. This story introduces no new class. The World Editor is a later story.

## Acceptance

- Alt-P writes one PNG in `Saved/Playshots`. The name is the running build id, an underscore, and the play machine's local time: `SC1-0667_20261027-124701.png`. A second press in that same second writes `SC1-0667_20261027-124701_2.png`, and the next writes `_3`.
- The picture is the Play In Editor view the player is seeing, including the HUD and an open menu, inventory, journal, message, or name page.
- P on the stronghold name page still types the letter. Alt-P on that page writes a picture and does not add a letter.
- F12 still shows and hides the debug readout.
- A pure function builds that file name from a build id, a time, and the names already in the folder. It also reports which of those names belong to a build id. The test calls that function. It does not start Play In Editor.
- In Play In Editor, Alt-P twice writes two files whose names start with the build id on the HUD, and play stays running. On the name page, P types and Alt-P does not. A shot taken with the menu open shows the menu.
- `Content/Controls.md` lists Alt-P as the Play In Editor screenshot.
- `AGENTS.md` and `AgentBuild.md` say the developer presses Alt-P in Play In Editor, and the file lands in `Saved/Playshots` under the name above. When the developer says they are testing in Play In Editor, the solidcore1-ue worker lists that folder for the build id in `Source/SolidCore1/Game/SolidBuildId.h` and reads the new PNGs. It lists again when the developer says they pressed Alt-P. It does not wait in a loop. A cloud agent has no copy of these files. A World Editor screenshot stays with the snipping tool, and the note about that tool's frame rate stays with it.

## Implementation notes

The press is a chord on P with Alt, beside the F12 binding in `ASolidCharacter`. The name page listens for any key with Alt up, so Alt-P is not a typed letter.

The capture is the game viewport with its UI, written to an absolute path under `Saved/Playshots`. `Saved/` is already gitignored. The engine's `Saved/Screenshots` folder is not the destination. The time is the local time of the play machine, `YYYYMMDD-HHMMSS`.

The editor's Play command is also Alt-P. `tools\pie.bat` starts an immersive session. The play-test is the check that Alt-P writes the file and leaves that session running.
