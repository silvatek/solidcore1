# Dragongate is one map marker

Status: Ready

## Story

Dragongate is one town on two map markers. Both sit on row 14, in the gap through the mountains. Column 33 is the eastern cell. Column 34 is the western cell. The stone wall runs across both.

This story leaves the eastern marker. Row 14, column 33 stays `Town (Dragongate)`. Row 14, column 34 becomes mountain, so the range closes around one cell. Relion is on column 33, north of that cell, and the road from Relion still arrives there. The wall still crosses the pass from east to west. The welcome sign, the three roads, and Enter Dragongate stay with that one cell.

## Acceptance

- The shipped map names one Dragongate cell: row 14, column 33, `Town (Dragongate)`. The marker there is `!`.
- Row 14, column 34 is mountain. It has no `!` line.
- Dragongate is location 5, type WallGate, and one cell.
- The wall runs east-west from the east edge of that cell to the west edge. The gap in the middle is 4 m. One tower stands on each side of the gap, on the north face of the wall. The towers match each other in height, and that height is above the wall.
- One welcome sign stands on the town cell, north of the wall and east of the gap, clear of the opening and clear of the mountain. The board faces north.
- The roads Relion to Dragongate, Dragongate to Jethan, and Dragongate to Rathen end on that cell.
- Enter Dragongate fires when the captain steps on that cell, once. Its changes stay the ones already listed for that event.
- `Content/WorldMap.txt` and `/Game/World/WorldChart` describe the same world. The chart has one town mark named Dragongate, on that cell. The western cell's marker biome is mountain.
- Iglin stays location 0. The cave stays the Meagre Dungeon mark at row 9, column 17. The other towns stay on the cells they have now.
- A map built in a test may still give one wall-gate town two cells. Those two cells stay one town, and the wall follows the long axis. The shipped map has the single cell.

## Implementation notes

- In the text grid, column 0 is east. The left `!` on row 14 is column 33. Replacing the right `!` with `M` is the whole grid edit. Delete the `! 14, 34` line.
- North and south of the cell are grassland. East is already mountain. After the edit, west is mountain too. The wall code treats mountains on the east and west as an east-west pass. Grass on the west would leave a way around the wall.
- The sign is placed from the middle of the town's cells. With two cells that middle is their shared edge, and the pole is 400 cm east and 800 cm north of it (`DragongateSignEastCm`, `DragongateSignNorthCm`). One cell moves the middle onto the cell centre. 400 cm east of that centre is the east edge, in the mountain. Keep the 800 cm north offset, the north edge of the 16 m cell, and shorten the east offset so the pole stays on the town cell and east of the 4 m gap.
- One cell is 8 m east-west. The 4 m gap leaves 2 m of wall on each side. Each tower is 2.8 m along the wall, so it meets the mountain by about 0.8 m. That is the gate in the pass. The town stays one cell.
- `LoadDefault` reads the chart. Change the chart with the text file. The parity test fails when they differ. Saving `/Game/World/WorldChart` happens in the editor.
- Tests that require the shipped Dragongate to occupy two cells change to one cell at row 14, column 33, with mountain on column 34. `SolidCore1.Towns.WallGateDigitPair` keeps its own two-cell pass.
- [Work on the terrain grid](work-on-the-terrain-grid.md) says Dragongate is two town marks today. Update that sentence when this story ships. Two marks of one name in one painted region can still be one town. This story does not wait on that one, and it does not wait on [Remove WorldMap.txt](remove-world-map-txt.md).
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. The note says Dragongate is one map marker. This plan commit does not.
