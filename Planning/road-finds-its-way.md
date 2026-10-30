# A road finds its way

Status: New

## Story

A road starts as a straight line from one end to the other. Where that line meets banned ground, the route goes around it. The search does not look at every terrain point for every step.

The cost of entering a biome depends on the road type:

| Biome | Trail | Track | Road | Highway |
| --- | --- | --- | --- | --- |
| Grassland | 8 | 8 | 8 | 8 |
| Forest | 8 | 8 | 16 | 32 |
| Mountain | -- | -- | -- | -- |
| Town | 8 | 8 | 8 | 8 |
| Desert | 8 | 8 | 16 | 32 |
| Swamp | 8 | 8 | 8 | 8 |
| Sea | -- | -- | -- | -- |
| River | 16 | 16 | 16 | 16 |
| Road | 8 | 8 | 8 | 8 |

An ordinary step costs 8. `--` means the route goes around that ground. A road pays 16 to enter forest or desert, and a highway pays 32. A river's 16 is a crossing at a right angle to the river. Any other step on the river is `--`.

This story introduces no new class.

## Acceptance

- A road starts as a straight line between its ends. Where that line meets banned ground, the route goes around it.
- The search does not look at every terrain point for every step.
- The cost of entering a biome is the table above.

## Implementation notes

`SolidRoad::Solve` scans every cell on every step. `RiverCost`, `RiverBendCost`, and `RiverBendReach` are what make a road or a highway cross a river and leave it. This story replaces that search with the straight line and the table above. Play does not read the step costs. The river channel is 4 m wide.
