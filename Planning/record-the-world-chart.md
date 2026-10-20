# Record the world chart

Status: Ready

## Story

The developer has the shipped world in a chart as well as in `WorldMap.txt`. The chart holds the same cells, marks, and roads. The game still builds the world from the text file. [Paint the world chart](paint-the-world-chart.md) and [Load the world from the chart](load-the-world-from-the-chart.md) are Ready and depend on this story. This story ships first.

## Acceptance

- `USolidWorldChart` is a data asset. Importing `Content/WorldMap.txt` fills one. The shipped asset is `/Game/World/WorldChart`.
- Loading that chart into a world map matches loading the text file. Every cell has the same biome. Every mark matches. Every solved road matches.
- A town mark uses the id `SolidTowns` already assigns to that name. Iglin stays location 0. Dragongate stays one town on two cells. The Meagre Dungeon mark is the cave at row 9, column 17.
- The game still reads `Content/WorldMap.txt`. Play is unchanged.
- This story does not bump `SOLID_BUILD_ID`.

## Implementation notes

- The cell stores the marker biome: sea, grassland, town, mountain, forest, desert, or river. A `!` imports as grassland. A named town, a stronghold shelf, and a lair mouth still override that biome in `GetBiomeAtCell`, as they do for the text file. Swamp and road are not marker biomes.
- A mark stores its kind, its name, and its world position in centimetres, snapped to the cell centre. A stronghold stores size and type. A site stores size only. A lair mouth stores the lair id and which end it is. On load, that position becomes a cell once. `USolidWorldMap` keeps speaking in cells after that.
- Two town marks with the same name are one town. A town name `SolidTowns` does not list is not a location. A stronghold token and a lair name resolve the same way the text parser resolves them.
- Roads on the chart are definitions: two end names and a type. The solver is the one the text file already uses. `MeagreDungeon` and `Meagre Dungeon` stay the same name.
- The parity test builds a chart from the file text and compares it with `LoadFromString`. Saving `/Game/World/WorldChart` from that import happens in the editor, and the asset is committed. This story is done when that asset is in the repo and the parity test passes. A cloud agent does not run the editor.
- `LoadDefault` still reads the text file.
