# Dragongate is one map marker

Status: New

## Story

Dragongate is a stone wall across a gap in the mountains. The gate is a 4 m opening with a tower on each side. That opening stays 4 m. The model is wider than one cell, 8 m east to west. The map uses two markers on row 14, columns 33 and 34, so the wall has those two cells to stand on.

The shipped gap is three cells wide. In the text map the run is `M.!.M`: mountain, grassland, the Dragongate marker, grassland, mountain. The marker is the middle cell, halfway between the mountains. Grassland on each side of that cell gives the gate room past the one 8 m cell.

From the gate, the curtain fills the rest of each grassland cell. It then continues into the mountain on the east and on the west, far enough into the slope that no gap shows between the wall and the hillside. The town sign stands near the marker. The same walk fills a wider run of grassland when the mountains stand farther apart, and still ends inside the slopes.

## Acceptance

- The shipped map has one Dragongate marker. On its row the cells read mountain, grassland, that marker, grassland, mountain.
- The gate stands on the marker cell, halfway between those mountains. The opening is 4 m, with a tower on each side.
- The curtain fills the remaining width of each grassland cell beside the marker.
- The curtain continues into the mountain on the east and on the west, far enough that the slope meets the wall.
- The town sign stands near the marker.
- `Content/WorldMap.txt` and `/Game/World/WorldChart` describe that same run.

## Implementation notes

- One cell is 8 m east to west. The opening is 4 m and each tower is 2.8 m along the wall, so the gate is 9.6 m. Centered on the marker, the towers reach into the grassland cells. The curtain takes the rest of each grassland cell. `GapCm` stays 400. `CollectPieces` today spans only the town's own cells. The new length is the grassland beside the marker, plus the overlap into each mountain.
- In the text grid, column 0 is east, so the left `M` of `M.!.M` is the eastern mountain. The row today has `M!!M` at columns 32 to 35. `M.!.M` is one cell wider, so that edit turns one neighbouring mountain cell into grassland.
- The pass sits on the 1 m shelf. A mountain point is on the 10 m shelf. Terrain points are 2 m apart, so the hillside is the quad between the last grassland point and the first mountain point. The wall is 4 m high. The curtain crosses that quad and finishes inside the mountain.
- The marker cell is the town. The grassland cells are the gap. Enter Dragongate, the roads, and the sign use the marker cell.
- The sign offsets were measured from the middle of the two old cells: 400 cm east and 800 cm north, facing north, off the opening. Place the sign from the marker, near the gate and off the opening.
- A test of `M.!.M` expects the gate on the `!`, the curtain across both grassland cells, and the curtain inside both mountain cells. `SolidCore1.Towns.WallGateDigitPair` is the old two-cell workaround.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. This plan commit does not.
