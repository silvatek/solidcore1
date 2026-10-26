# Dragongate is one map marker

Status: Done

## Story

Dragongate is a stone wall across a gap in the mountains. The gate is a 4 m opening with a tower on each side. That opening stays 4 m. The gate, towers included, is 9.6 m east to west.

The land is the terrain grid. The points are 2 m apart, and the map is built from those points. A Dragongate marker stands on one point. From that point the wall runs east and west across the town points and the grassland points until a mountain point. `M..!..M` is one such gap. `Moo!ooM` and `M.o!o..M` are others. Any mix of `o` and `.` belongs to the same wall. The marker can sit anywhere in the run. The town sign stands near it.

Each symbol is one terrain point, written from east to west. `o` is town. `.` is grassland. `!` is the Dragongate marker. `M` is mountain.

The person editing the map paints the points, and follows two rules. On each side of the marker, the points 2 m and 4 m away are grassland or town, so the gap is wide enough for the gate. A mountain point lies further east, and a mountain point lies further west, for the wall to extend to. This story does not name those points, and the code does not hard-code them. The World chart mode does not enforce the rules.

The shipped chart has one Dragongate marker. That marker is the gate. While two markers stand on one east–west line, the eastern one is the gate and the western one is ignored. The point under the western marker keeps its ground. It is not a second gate, and it does not pull the opening to the middle of the pair.

The curtain fills the town points and the grassland points in that run. It then continues into the mountain on the east and on the west, far enough into the slope that no gap shows between the wall and the hillside. When the gap is too narrow, or a mountain is missing to the east or the west, or another of these rules is broken, the game draws no Dragongate model. The marker the gate uses, Enter Dragongate, and the town sign stay.

## Acceptance

- The land the wall reads is terrain points 2 m apart. The Dragongate marker stands on one of those points. The story does not name the point.
- A valid run has that marker, grassland or town at 2 m and at 4 m on each side, and a mountain point further east and further west. `M..!..M`, `Moo!ooM`, and `M.o!o..M` are valid. So is any other mix of `o` and `.` that reaches those points.
- While two Dragongate markers stand, the eastern one is the gate and the western one is ignored.
- The World chart mode allows any edit. A gap too narrow for the gate, a missing mountain to the east or the west, or any other broken rule draws no wall, no opening, and no towers.
- The gate stands on the marker point it uses. The opening is 4 m, with a tower on each side. A longer run on one side, as in `M.o!o..M`, leaves the gate on that point.
- The curtain fills the town points and the grassland points between the marker and each mountain.
- The curtain continues into the mountain on the east and on the west, far enough that the slope meets the wall.
- The town sign stands near the marker the gate uses.

## Implementation notes

- The opening is 4 m and each tower is 2.8 m along the wall, so the gate is 9.6 m. `GapCm` stays 400. Half of that gate is 4.8 m. It covers the points 2 m and 4 m from the marker and stops 1.2 m short of the point 6 m away. `M.!.M` and `Mo!oM` leave one open point on a side. That gap is narrower than the gate, and `CollectPieces` writes nothing for it.
- `CollectPieces` today spans the town's own cells. The new length is the run of town and grassland points from the used marker to each mountain, plus the overlap into that mountain. The product code reads the points it is given. The shipped chart has one Dragongate marker.
- `CollectPieces` writes nothing when the gap is narrower than the gate, or when a mountain point is missing to the east or to the west. The used marker, Enter Dragongate, and the town sign stay. A test covers `M.!.M` and a run with a mountain on only one side.
- The open points sit on the 1 m shelf. A mountain point is on the 10 m shelf. The hillside is the quad between the last open point and the first mountain point. The wall is 4 m high. The curtain crosses that quad and finishes inside the mountain.
- The used marker is the gate. Other town points in the run are painted ground. Enter Dragongate, the roads, and the sign use that same marker.
- The sign offsets were measured from the middle of the two old markers: 400 cm east and 800 cm north, facing north, off the opening. Place the sign from the used marker, near the gate and off the opening.
- Town buildings already stay off the wall and the towers. The placement skip for a wall-gate location stays. A building is not placed where it would meet the wall or a tower, including where the curtain crosses town ground. This story does not change that rule.
- Tests cover `M..!..M`, `Moo!ooM`, and `M.o!o..M`: the gate on the `!`, the curtain across every `o` and `.` in the run, and the curtain inside both mountains. They also cover a gap that is too narrow and a run with a mountain on only one side, both of which write no pieces. A test with two markers uses the eastern one and ignores the western one. `SolidCore1.Towns.WallGateDigitPair` is the old pair workaround, which centred the gate between the two markers.
