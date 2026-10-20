# Remove WorldMap.txt

Status: Ready

## Story

This story depends on [Load the world from the chart](load-the-world-from-the-chart.md), which is Ready, and [Paint the world chart](Done/paint-the-world-chart.md), which is Done. Those stories ship first. The developer then edits the world in the World chart mode, and the game loads that chart. `Content/WorldMap.txt` is gone.

## Acceptance

- `Content/WorldMap.txt` is not in the repo. The game loads `/Game/World/WorldChart`.
- `LoadFromString` remains for a test that builds a map from text.
- The test that compared the shipped chart to the text file is gone with the file.
- `tools/cloud_agent_check.py` passes without `Content/WorldMap.txt`.
- `AGENTS.md` and `README.md` describe the chart and the World chart mode. They no longer tell an agent to review `WorldMap.txt` before other work.
- `SolidStronghold::StartingRow` and `StartingColumn` still name the cave. A test checks that cell against the Meagre Dungeon mark on the chart.
- `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` bump. The note says the text map is gone.

## Implementation notes

- The world-map section of `AGENTS.md` is the review rule for the text file. Replace it with the chart at `/Game/World/WorldChart`, the 128×64 cell grid, and the mark kinds. A change to that asset is what a later agent reviews.
- `README.md` still documents the marker characters and the `!` lines, and the `SolidCore1.WorldMap.*` filter blurb follows them. Point those at the chart and the mode, and at the tests that remain.
- `tools/cloud_agent_check.py` errors when `Content/WorldMap.txt` is missing. That check leaves in the same change as the file.
- If [Package the game](package-the-game.md) is still open, its acceptance follows the chart. The loose text file is not staged.
