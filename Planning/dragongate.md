# Dragongate is one map marker

Status: New

## Story

Dragongate is a stone wall across a gap in the mountains. The gate is a 4 m opening with a tower on each side. That opening stays 4 m. The model is wider than one cell, 8 m east to west. The map uses two markers on row 14, columns 33 and 34, so the wall has those two cells to stand on.

One Dragongate marker is where the gate stands. From that cell the wall runs east and west across whatever town and grassland lies between it and the mountains. `M.!.M` is one such gap. `Mo!oM` and `M.o!o..M` are others. Any mix of `o` and `.` belongs to the same wall. The marker can sit anywhere in the run. The town sign stands near it.

The curtain fills the town cells and the grassland cells in that run. It then continues into the mountain on the east and on the west, far enough into the slope that no gap shows between the wall and the hillside.

## Acceptance

- The shipped map has one Dragongate marker. The cells from that marker to the mountain on each side are town, grassland, or the marker.
- The gate stands on the marker cell. The opening is 4 m, with a tower on each side.
- The curtain fills the town cells and the grassland cells between the marker and each mountain. `M.!.M`, `Mo!oM`, and `M.o!o..M` all do this. So does any other mix of `o` and `.` in that run.
- The curtain continues into the mountain on the east and on the west, far enough that the slope meets the wall.
- The town sign stands near the marker.
- `Content/WorldMap.txt` and `/Game/World/WorldChart` describe the same run.

## Implementation notes

- One cell is 8 m east to west. The opening is 4 m and each tower is 2.8 m along the wall, so the gate is 9.6 m. `GapCm` stays 400. `CollectPieces` today spans only the town's own cells. The new length is the run of `o` and `.` from the marker to each mountain, plus the overlap into that mountain. The gate stays on the marker when one side of the run is longer, as in `M.o!o..M`.
- In the text grid, column 0 is east. `o` is a town cell. `.` is grassland. `!` is the Dragongate marker. `M` stops the run. The row today has `M!!M` at columns 32 to 35.
- The house pack skips a wall-gate location and still scatters on other town cells. An `o` in this run would grow a house on the curtain. The run carries the wall, so the pack stays off those cells.
- The pass and a town cell sit on the 1 m shelf. A mountain point is on the 10 m shelf. Terrain points are 2 m apart, so the hillside is the quad between the last open point and the first mountain point. The wall is 4 m high. The curtain crosses that quad and finishes inside the mountain.
- The marker cell is the town. The `o` cells are not more Dragongate markers. Enter Dragongate, the roads, and the sign use the marker cell.
- The sign offsets were measured from the middle of the two old cells: 400 cm east and 800 cm north, facing north, off the opening. Place the sign from the marker, near the gate and off the opening.
- Tests cover `M.!.M`, `Mo!oM`, and `M.o!o..M`: the gate on the `!`, the curtain across every `o` and `.` in the run, and the curtain inside both mountains. `SolidCore1.Towns.WallGateDigitPair` is the old two-cell workaround.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. This plan commit does not.
