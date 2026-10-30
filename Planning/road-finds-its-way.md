# A road finds its way

Status: New

## Story

A road starts as a straight line from one end to the other. Where that line meets banned ground, the route goes around it. The search does not look at every terrain point for every step.

Mountain and sea are banned for a trail, a track, a road, and a highway. Forest and desert are banned for a road and a highway. A river is banned for all four. The route may cross a river at a right angle to the river, and that crossing costs more than one ordinary step. The other step costs only choose the route. This story does not set them.

This story introduces no new class.

## Acceptance

- A road starts as a straight line between its ends. Where that line meets banned ground, the route goes around it.
- The search does not look at every terrain point for every step.
- Mountain and sea are banned for a trail, a track, a road, and a highway. Forest and desert are banned for a road and a highway.
- A river is banned for a trail, a track, a road, and a highway. The route may cross it at a right angle to the river. That crossing costs more than one ordinary step.

## Implementation notes

`SolidRoad::Solve` scans every cell on every step. `RiverCost`, `RiverBendCost`, and `RiverBendReach` are what make a road or a highway cross a river and leave it. This story replaces that search with the straight line and the banned ground above. A river crossing costs more than one ordinary step. The story does not set the other step costs. Play does not read them. The river channel is 4 m wide.
