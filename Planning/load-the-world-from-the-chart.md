# Load the world from the chart

Status: Ready

## Story

This story depends on [Record the world chart](Done/record-the-world-chart.md), which is Done. That story ships first. The game then builds the world from `/Game/World/WorldChart`. Terrain, towns, roads, the cave, and the Iglin tunnels match the text file. The text file stays so a test can compare the two. [Paint the world chart](paint-the-world-chart.md) may ship before or after this story. [Remove WorldMap.txt](remove-world-map-txt.md) is Ready and depends on this story. This story ships first.

## Acceptance

- `LoadDefault` loads `/Game/World/WorldChart`. A missing asset fails the load.
- The shipped chart and `Content/WorldMap.txt` produce the same cell biomes, the same marks, and the same solved roads. A test fails when they differ.
- `LoadFromString` still builds a map from text. The tests that invent a small map keep using it.
- Town ids, events, the Dragongate wall, and `SolidStronghold::StartingRow` and `StartingColumn` stay as they are. The cave is still the Meagre Dungeon mark.
- `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` bump. The note says the world loads from the chart.
- Play In Editor is the check that the ground, the towns, the roads, the cave, and the tunnels match the session that read the text file.

## Implementation notes

- The chart has to be a cook reference. A bare `LoadObject` of a path string is not one. [Package the game](package-the-game.md) still says the cooked game reads `Content/WorldMap.txt` and stages that file as UFS. While that story is open, update it so the cook follows the chart, and drop that UFS note. If that story has already shipped, the cook change lands here.
- The text file stays. The parity test reads it. Deleting it is [Remove WorldMap.txt](remove-world-map-txt.md).
- Town facts stay in `SolidTowns` and `SolidEvents`, keyed by the id the name already has. Iglin stays location 0.
