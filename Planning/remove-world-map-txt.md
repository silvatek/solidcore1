# Remove WorldMap.txt

Status: In Progress

## Story

This story depends on [Load the world from the chart](Done/load-the-world-from-the-chart.md), which is Done, and [Paint the world chart](Done/paint-the-world-chart.md), which is Done. Those stories ship first. The developer then edits the world in the World chart mode, and the game loads that chart. `Content/WorldMap.txt` is gone.

## Acceptance

- `Content/WorldMap.txt` is not in the repo. The game loads `/Game/World/WorldChart`.
- `LoadFromString` remains for a test that builds a map from text.
- The test that compared the shipped chart to the text file is gone with the file.
- `tools/cloud_agent_check.py` passes without `Content/WorldMap.txt`.
- `AGENTS.md` and `README.md` describe the chart and the World chart mode. They no longer tell an agent to review `WorldMap.txt` before other work.
- `SolidStronghold::StartingRow` and `StartingColumn` still name the cave. A test checks that cell against the Meagre Dungeon mark on the chart.
- `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` bump. The note says the text map is gone.

## Implementation notes

Stage 1 is the biome's colour. Stage 2 deletes `Content/WorldMap.txt`, the shipped-chart comparison, the import, and the disagreement line, and checks the Meagre Dungeon mark on the chart. Stage 3 points `AGENTS.md`, `README.md`, and `tools/cloud_agent_check.py` at the chart.

### Stage 1 — a biome names its colour

`FSolidBiome` answers for an `ESolidBiome` token. `Color()` is the tint play uses, and the river uses the sea blue. `GetBiomeColor` and `DefaultColorForBiome` ask the biome. A colour line in a text map is ignored. `LoadFromString` stays. The biome also names that ground, its shelf, and its chart letter, and it says whether the ground is open or paintable. Threat stays on the terrain point, a constant play ignores. Fog follows where the captain has been.

### Stage 2 — the text file is gone

`Content/WorldMap.txt` is deleted. `ImportShippedWorldMap` and `Disagreement` are gone. `LoadFromString` and `ImportFromString` stay. `SolidCore1.WorldMap.ShippedChartMatchesText` loads the chart and checks the Meagre Dungeon mark. `tools/cloud_agent_check.py` no longer requires the file. `AGENTS.md` and `README.md` wait for stage 3.

`USolidWorldMap` is gone. The chart keeps the saved cells, marks, and roads, and it answers the queries the map answered. `LoadFromString` and `LoadFromChart` are on the chart. `LoadFromFile` is gone. Paint, a mark, and undo clear the solved routes, and the next query solves them again.

### Stage 3 — the docs

- The world-map section of `AGENTS.md` is the review rule for the text file. Replace it with the chart at `/Game/World/WorldChart`, the 128×64 cell grid, and the mark kinds. A change to that asset is what a later agent reviews.
- `README.md` still documents the marker characters and the `!` lines, and the `SolidCore1.WorldMap.*` filter blurb follows them. Point those at the chart and the mode, and at the tests that remain.
- `tools/cloud_agent_check.py` passes without `Content/WorldMap.txt`. Stage 2 removed the check that required the file. Stage 3 points the script at the chart.
- If [Package the game](package-the-game.md) is still open, its acceptance follows the chart. The loose text file is not staged.
