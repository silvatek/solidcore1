# A road finds its way

Status: New

## Story

A road starts as a straight line from one end to the other. Where that line meets banned ground, the route goes around it. The search does not look at every terrain point for every step.

The cost of entering a biome depends on the road type:

| Biome | Trail | Track | Road | Highway |
| --- | --- | --- | --- | --- |
| Grassland | Ordinary | Ordinary | Ordinary | Ordinary |
| Forest | Ordinary | Ordinary | Banned | Banned |
| Mountain | Banned | Banned | Banned | Banned |
| Town | Ordinary | Ordinary | Ordinary | Ordinary |
| Desert | Ordinary | Ordinary | Banned | Banned |
| Swamp | Ordinary | Ordinary | Ordinary | Ordinary |
| Sea | Banned | Banned | Banned | Banned |
| River | Crossing | Crossing | Crossing | Crossing |
| Road | Ordinary | Ordinary | Ordinary | Ordinary |

Ordinary is one ordinary step. Banned means the route goes around that ground. Crossing means the route may cross the river at a right angle to the river, and that crossing costs more than one ordinary step. Any other step on the river is banned. This story does not set the number of an ordinary step, or how much higher a crossing is.

This story introduces no new class.

## Acceptance

- A road starts as a straight line between its ends. Where that line meets banned ground, the route goes around it.
- The search does not look at every terrain point for every step.
- The cost of entering a biome is the table above.

## Implementation notes

`SolidRoad::Solve` scans every cell on every step. `RiverCost`, `RiverBendCost`, and `RiverBendReach` are what make a road or a highway cross a river and leave it. This story replaces that search with the straight line and the table above. Play does not read the step costs. The river channel is 4 m wide.
