# Dragongate is one map marker

Status: New

## Story

Dragongate is a stone wall across a gap in the mountains. The gate is a 4 m opening with a tower on each side. That model is wider than one cell. A single marker used as the whole wall does not fit, which is why the map has two markers on row 14, columns 33 and 34. The wall runs across those two cells.

The wall extends east and west from the gate until it meets a mountain. A wider gap between the mountains is filled by that wall. One Dragongate marker is the position of the gate. The town sign stands near that marker.

How wide the shipped gap is, and which cell in it holds the marker, are still to be chosen. Closing the mountains back to one cell brings the fit problem back.

## Acceptance

Not written yet.

## Implementation notes

- One cell is 8 m east to west. The opening is 4 m and each tower is 2.8 m along the wall, so the gate model is 9.6 m. `CollectPieces` spans the town's own cells. Two `!` cells are 16 m, which is enough. One cell is not.
- The new length is a walk from the marker. Step east until the next cell is mountain, and step west until the next cell is mountain. The curtain covers the ground between. Mountain stops the walk. The town's cell count is not the length.
- The opening and the two towers stay on the marker. A longer gap lengthens the curtain on each side. It does not slide the gate to the middle of the gap.
- The marker cell is the town. The other cells the wall crosses are the gap, not more Dragongate markers. Enter Dragongate, the roads, and the sign use that one cell.
- The sign offsets were measured from the middle of the two cells: 400 cm east and 800 cm north, facing north, off the opening. From one cell, 400 cm east is the east edge. Place the sign from the marker, near the gate and off the opening.
- A test with mountains several cells apart, and one marker between them, expects the wall to meet both mountains and the gate to sit on the marker. `SolidCore1.Towns.WallGateDigitPair` is the two-cell workaround.
- `Content/WorldMap.txt` and `/Game/World/WorldChart` change together once the gap and the marker cell are chosen. This plan commit does not bump `SOLID_BUILD_ID`.
