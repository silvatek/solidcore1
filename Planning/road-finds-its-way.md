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
| Town | 0 | 0 | 0 | 0 |
| Desert | 8 | 8 | 16 | 32 |
| Swamp | 8 | 8 | 16 | 32 |
| Sea | -- | -- | -- | -- |
| River | 16 | 16 | 32 | 64 |
| Road | 0 | 0 | 0 | 0 |

An ordinary step costs 8. Town and road cost 0. `--` means the route goes around that ground. A road pays 16 to enter forest, desert, or swamp, and a highway pays 32. The game has no swamp yet. The swamp row is the cost for when it does. A river number is a crossing at a right angle to the river. A trail or a track pays 16, a road pays 32, and a highway pays 64. Any other step on the river is `--`.

This story introduces no new class.

## Acceptance

- A road starts as a straight line between its ends. Where that line meets banned ground, the route goes around it.
- The search does not look at every terrain point for every step.
- The cost of entering a biome is the table above.

## Implementation notes

`SolidRoad::Solve` scans every cell on every step. `RiverCost`, `RiverBendCost`, and `RiverBendReach` are what make a road or a highway cross a river and leave it. This story replaces that search with the straight line and the table above. Play does not read the step costs. The river channel is 4 m wide.
