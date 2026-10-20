# Paint the world chart

Status: Ready

## Story

This story depends on [Record the world chart](record-the-world-chart.md), which is Ready. That story ships first. The developer then edits `/Game/World/WorldChart` on the open-world level. Changing a biome and placing a mark are the edits `WorldMap.txt` is used for now. The game still reads the text file, so an edit does not change play. [Remove WorldMap.txt](remove-world-map-txt.md) is Ready and depends on this story. This story ships first.

## Acceptance

- The editor target has a World chart mode on `/Game/ThirdPerson/Lvl_ThirdPerson`. The mode shows the terrain built from the chart, a cell overlay in the biome colors, and a billboard on each mark.
- Paint sets the marker biome of a cell to grassland, forest, mountain, town, desert, sea, or river. Click paints one cell. Drag paints a run. Shift-drag fills a rectangle. Alt copies the biome under the cursor. The brush is one cell, 8 m east–west and 16 m north–south.
- Mark places or edits a town, a stronghold, a stronghold site, a lair entrance, or a lair exit on a cell. A town is one `SolidTowns` already names. A stronghold asks for a size and a type the game already has. A site asks for a size. A lair mouth asks for a lair the game already has, and which end. Delete clears the mark. Painting a cell leaves its mark in place.
- Road joins a town or a stronghold to another town or stronghold and picks trail, track, road, or highway. The strip is the solved route. A missing end shows on the tool. A stronghold site is not an end.
- Read, on hover, names the resolved biome, the painted biome when a mark overrides it, the mark, and any road through the cell.
- Undo returns the chart to the edit before this one. Save writes `/Game/World/WorldChart`. The terrain rebuilds when the mouse button goes up.
- The mode says when the chart and `Content/WorldMap.txt` disagree.
- Tests cover paint, mark, and road on the chart, without a viewport. The editor session on the Windows machine is the check that the mode matches. A cloud agent does not run the editor.
- Play In Editor still reads `Content/WorldMap.txt`. This story does not bump `SOLID_BUILD_ID`.

## Implementation notes

- The mode lives in a `SolidCore1Editor` module, registered on the editor target and in the uproject. `USolidWorldChart` stays in the game module. The tools call functions on that chart. The tests call those same functions.
- The overlay shows the biome `GetBiomeAtCell` returns. Read also names the painted biome when a mark replaces it. A lair mouth is mountain. A stronghold uses its shelf. A named town is town. A site keeps the paint.
- The editor ground is the existing terrain build, run from the chart. It is a second view of that build, not a Landscape and not a new mesh path.
- The lists are the ones the game already has. A new town, a new lair, or a new stronghold size is still a code change. Swamp stays off the palette. Road stays a solved strip, not a paint biome.
- The game module's editor-only `UnrealEd` dependency can move to `SolidCore1Editor` with this mode.
