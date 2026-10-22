# Dragongate is one map marker

Status: Ready

## Story

Dragongate is a stone wall across a gap in the mountains. The gate is a 4 m opening with a tower on each side. That opening stays 4 m. The model is wider than one cell, 8 m east to west. The map uses two markers on row 14, columns 33 and 34, so the wall has those two cells to stand on.

One Dragongate marker is where the gate stands. From that cell the wall runs east and west across whatever town and grassland lies between it and the mountains. `M.!.M` is one such gap. `Mo!oM` and `M.o!o..M` are others. Any mix of `o` and `.` belongs to the same wall. The marker can sit anywhere in the run. The town sign stands near it.

The person editing the map chooses the cells, and follows two rules. The cell on each side of the marker is grassland or town, so the gap is wide enough for the gate. A mountain biome lies east of the marker, and a mountain biome lies west of it, for the wall to extend to. This story does not name those cells, and the code does not hard-code them. The World chart mode does not enforce the rules.

The curtain fills the town cells and the grassland cells in that run. It then continues into the mountain on the east and on the west, far enough into the slope that no gap shows between the wall and the hillside. When the gap is too narrow, or a mountain is missing to the east or the west, or another of these rules is broken, the game draws no Dragongate model.

## Acceptance

- The person editing the map chooses the cells. A valid run has one Dragongate marker, grassland or town on each side of it, and a mountain biome to the east and to the west. The story does not name the cells.
- The World chart mode allows any edit. A gap too narrow for the gate, a missing mountain to the east or the west, or any other broken rule draws no wall, no opening, and no towers.
- The gate stands on the marker cell. The opening is 4 m, with a tower on each side.
- The curtain fills the town cells and the grassland cells between the marker and each mountain. `M.!.M`, `Mo!oM`, and `M.o!o..M` all do this. So does any other mix of `o` and `.` in that run.
- The curtain continues into the mountain on the east and on the west, far enough that the slope meets the wall.
- The town sign stands near the marker.

## Implementation notes

- One cell is 8 m east to west. The opening is 4 m and each tower is 2.8 m along the wall, so the gate is 9.6 m. `GapCm` stays 400. `CollectPieces` today spans only the town's own cells. The new length is the run of `o` and `.` from the marker to each mountain, plus the overlap into that mountain. The gate stays on the marker when one side of the run is longer, as in `M.o!o..M`.
- In the text grid, column 0 is east. `o` is a town cell. `.` is grassland. `!` is the Dragongate marker. `M` is mountain. The cells immediately east and west of the marker are grassland or town. The mountain the wall extends to can sit further along that row, as in `M.o!o..M`. The product code reads the run it is given. It does not replace the cells on row 14.
- `CollectPieces` writes nothing for a Dragongate whose gap is narrower than the gate, or that has no mountain biome to the east, or none to the west. The marker, Enter Dragongate, and the town sign stay. A test covers a one-cell gap and a run with no mountain on one side.
- Town buildings already stay off the wall and the towers. `CollectPlacements` skips a cell whose location is a wall gate. That stays. A building is not placed where it would meet the wall or a tower, including where the curtain crosses an `o`. This story does not change that rule.
- The pass and a town cell sit on the 1 m shelf. A mountain point is on the 10 m shelf. Terrain points are 2 m apart, so the hillside is the quad between the last open point and the first mountain point. The wall is 4 m high. The curtain crosses that quad and finishes inside the mountain.
- The marker cell is the town. The `o` cells are not more Dragongate markers. Enter Dragongate, the roads, and the sign use the marker cell.
- The sign offsets were measured from the middle of the two old cells: 400 cm east and 800 cm north, facing north, off the opening. Place the sign from the marker, near the gate and off the opening.
- Tests cover `M.!.M`, `Mo!oM`, and `M.o!o..M`: the gate on the `!`, the curtain across every `o` and `.` in the run, and the curtain inside both mountains. They also cover a gap that is too narrow and a run with a mountain on only one side, both of which write no pieces. `SolidCore1.Towns.WallGateDigitPair` is the old two-cell workaround.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. This plan commit does not.
