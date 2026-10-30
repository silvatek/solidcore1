# A road finds its way

Status: Ready

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

An ordinary step costs 8. Town and road cost 0. `--` means the route goes around that ground. A road pays 16 to enter forest, desert, or swamp, and a highway pays 32. The game has no swamp yet. The swamp row is the cost for when it does. A river number is a crossing at 90 degrees to the river. A trail or a track pays 16, a road pays 32, and a highway pays 64. Any other step on the river is `--`.

This story introduces no new class.

## Acceptance

- A road starts as a straight line between its ends. Where that line meets banned ground, the route goes around it.
- The search does not look at every terrain point for every step.
- The cost of entering a biome is the table above.
- A road crosses a river at 90 degrees to the river. Any other step on the river is banned.

## Implementation notes

`SolidRoad::Solve` scans every cell on every step. `RiverCost`, `RiverBendCost`, and `RiverBendReach` are what make a road or a highway cross a river and leave it. This story replaces that search with the straight line and the table above. Play does not read the step costs. The river channel is 4 m wide.

The picture of the roads north of Dragongate, and the road into Dragongate, ran this search on the current chart.

The finished centre line keeps the 90-degree crossing. Open ground pulls out to one straight line, and that line may run on a diagonal. The first picture let that pull cut the corner on the way from Iglin to Relion. The stepped path had crossed the river east to west. The centre line then met the river on a diagonal. The sampled cost of that diagonal was no higher than the stepped path. The angle is its own test. A straight segment that meets the river stays only when the crossing is at 90 degrees. With that rule on the pull, the search cost stayed 360 and the centre line is 78 m. The cut corner had been 74 m.

The painted river there runs north to south and is more than one terrain point wide. The track crosses it in several steps, all east to west. That is one crossing.

Each of the four roads was solved on its own, on the biomes as painted. No point on this chart is the Road biome, so the 0 in that row did not bring one road onto another. The tracks from Dragongate to Jethan and from Dragongate to Rathen were outside the picture. They follow the same rule.
