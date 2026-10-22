# Dragongate is one map marker

Status: New

## Story

Dragongate is a stone wall across a gap in the mountains. The gate is a 4 m opening with a tower on each side. That opening stays 4 m. The model is wider than one cell, 8 m east to west, so one cell is not enough ground for it. The map uses two markers on row 14, columns 33 and 34, and the wall runs across both. Column 32, to the east, is mountain. Column 35, to the west, is mountain. Those mountains stay where they are.

The wall runs east and west from the gate, across the open ground, and into the mountain on each side. Each end goes far enough into the slope that no gap shows between the wall and the hillside. One Dragongate marker is where the gate stands. The town sign stands near that marker. When the mountains are farther apart, the same wall fills the wider open ground and still ends inside the slopes.

The gate now stands on the line between column 33 and column 34, with 2 m of the opening in each cell. A marker is one whole cell, so the `!` stays on one of those two cells and the gate moves to that cell's centre. The other cell stays open ground for the wall to cross. It does not become mountain. Which of the two cells keeps the marker is still to be chosen.

## Acceptance

Not written yet.

## Implementation notes

- One cell is 8 m east to west. The opening is 4 m and each tower is 2.8 m along the wall, so the gate is 9.6 m. `CollectPieces` spans the town's own cells. Two `!` cells are 16 m, which is enough. One cell is not. `GapCm` stays 400.
- The pass sits on the 1 m shelf. A mountain point is on the 10 m shelf. Terrain points are 2 m apart, so the hillside is the quad between the last pass point and the first mountain point. The wall is 4 m high. A curtain that stops on the cell edge leaves the slope beside a bare end. The curtain crosses that quad and finishes inside the mountain, on both the east and the west.
- The gate is centered on the marker. The towers extend into the open cells beside the marker. A wider run of open ground lengthens the curtain on each side. It does not slide the gate to the middle of the gap, and it does not squeeze the towers into the marker cell.
- The marker cell is the town. The other cell the wall crosses is the gap, not a second Dragongate marker. Enter Dragongate, the roads, and the sign use that one cell.
- The sign offsets were measured from the middle of the two cells: 400 cm east and 800 cm north, facing north, off the opening. Place the sign from the marker, near the gate and off the opening.
- A test with mountains several cells apart, and one marker between them, expects the curtain to enter both mountain cells and the gate to sit on the marker. `SolidCore1.Towns.WallGateDigitPair` is the two-cell workaround.
- `Content/WorldMap.txt` and `/Game/World/WorldChart` change together once the marker cell is chosen. This plan commit does not bump `SOLID_BUILD_ID`.
