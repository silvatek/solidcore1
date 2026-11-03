# The terrain map and the world chart are one object

Status: New

## Story

The developer has one runtime object for the world the game stands on. Today `USolidWorldChart` is the painted world, and `USolidTerrainMap` is a second grid of the same 513 points on a side, 2 m apart, built from that chart. Those two runtime grids become the source array and the built array on one object.

The saved asset `/Game/World/WorldChart` stays the chart the World chart editor writes. Play loads that asset. Height and fog are not written back into it.

Marks and the solved roads stay on that same runtime object. This story introduces no new class. It does not drop the cell grid. That work is [The World chart drops the cell](Done/world-chart-drops-the-cell.md).

## Acceptance

- Play uses one runtime object for the painted world and the built ground.
- That object keeps the painted biomes as the source array: 513 points on a side, 2 m apart.
- The built array is those same places. Each point has a height, the biome after roads and bridges, and the exploration fog.
- Marks and the solved roads are on that object.
- `/Game/World/WorldChart` stays the saved asset. Play does not write height or fog into it.
- Walking still clears fog on the built points.

## Implementation notes

`USolidTerrainMap::Build` copies `/Game/World/WorldChart` onto a second `USolidWorldChart`, solves the routes and the town claims there, then fills its own points. A built point takes its height from the noise. Its biome is `SampleBiome`, so the road strip is laid on the paint, and the bridge pass can change the height. The chart's own point biome stays the paint. Fog exists only on the terrain map, and it changes as the company walks.

The two classes stay as they are until this story is Ready.
