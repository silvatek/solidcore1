# Coordinates

The open world is one square in Unreal, measured in centimetres. World and terrain cover it. A coordinate is that name followed by X or Y, as in `worldX` and `terrainY`. Later sections use only names introduced above them. Local coordinate systems are a separate section.

## Contents

- [Systems](#systems)
  - [World](#world)
  - [Terrain](#terrain)
- [Transformations](#transformations)
  - [Terrain and world](#terrain-and-world)
- [Legacy coordinate systems](#legacy-coordinate-systems)
  - [Chart](#chart)
  - [Cell](#cell)
    - [Cell transformations](#cell-transformations)
- [Local coordinate systems](#local-coordinate-systems)
  - [Chunk](#chunk)
  - [Actor](#actor)
- [Code](#code)

## Systems

### World

`worldX`, `worldY`, and `worldZ` are Unreal centimetres. +worldX is west, +worldY is north, and +worldZ is up. Unreal's forward axis is west, and Unreal's right axis is north.

The running game places the centre of the 1024 m square at world (0, 0). The square runs from world (−51200, −51200) to world (51200, 51200). Height noise is sampled from `worldX` and `worldY`.

Yaw is a facing in world. Yaw 0 faces west, along +worldX. Positive yaw turns from west toward north.

| Direction | World axis | Yaw |
| --- | --- | --- |
| West | +worldX | 0 |
| North | +worldY | 90 |
| East | −worldX | 180 |
| South | −worldY | −90 |
| Up | +worldZ | |

A sign that faces south uses yaw −90. A sign that faces north uses yaw 90.

### Terrain

`terrainX` and `terrainY` index a square lattice of sample points. The shipped grid is 513 points on a side, 200 cm apart, so there are 512 steps from edge to edge and the steps fill the 1024 m square.

`terrainX` 0, `terrainY` 0 is the south-east corner, at world (−51200, −51200). +terrainX is west and +terrainY is north, the same directions as world. `terrainY` 0 is the south edge. Storage order is `terrainY × 513 + terrainX`.

A terrain point carries a world position, a surface height, a biome, and exploration fog. Fog is a value on the point. Between four terrain points, height, threat, and fog are bilinear. The biome is the nearest point's biome.

The world chart asset keeps a second copy of this lattice. `Points` there is one biome per terrain point, in the same order, with the same index 0 at the south-east corner. It does not store a second set of positions. A mark stores its position as `TerrainPoint`.

## Transformations

### Terrain and world

```
worldX = (terrainX - 256) * 200
worldY = (terrainY - 256) * 200
```

256 is half of 512, the index of the world origin on the centred square.

```
terrainX = round(worldX / 200 + 256)
terrainY = round(worldY / 200 + 256)
```

A step of +1 `terrainX` is 200 cm west. A step of +1 `terrainY` is 200 cm north.

The painted road strip lies 100 cm east and 100 cm south of the terrain point it was rounded onto:

```
worldX = (terrainX - 256) * 200 - 100
worldY = (terrainY - 256) * 200 - 100
```

A bridge laid along that strip uses the same shift.

## Legacy coordinate systems

Some previous coordinate systems are no longer used.

### Chart

Chart coordinates are no longer used. They were similar to World, with different offsets.

### Cell

`cellX` and `cellY` indexed the rectangles on the world chart. There are 128 columns and 64 rows across the same square. A column is 8 m east–west. A row is 16 m north–south.

`cellX` 0 is the west edge. `cellY` 0 is the north edge. A higher `cellY` is further south. Storage order was `cellY × 128 + cellX`.

+cellX is east and +cellY is south. World and terrain use +X west and +Y north.

A mark used to sit on one cell and store that cell's centre as `TerrainPoint`. A mark now stores the terrain point itself. On the shipped sizes that centre is world (50800, 50400) and `terrainX` 510, `terrainY` 508. A column is 4 steps wide. A row is 8 steps tall.

The old map text counts from 1 at the top-left. The top-left is `cellX` 0, `cellY` 0. A line `! 8,21 = Town Iglin` is row 8, column 21 in that count, which is `cellX` 20, `cellY` 7.

#### Cell transformations

```
worldX = 51200 - (cellX + 0.5) * 800
worldY = 51200 - (cellY + 0.5) * 1600

cellX = floor((51200 - worldX) / 800)
cellY = floor((51200 - worldY) / 1600)

terrainX = round((1 - (cellX + 0.5) / 128) * 512)
terrainY = round((1 - (cellY + 0.5) / 64) * 512)
```

## Local coordinate systems

These coordinate systems are only used in specific game subsystems, and they are not referred to outside those subsystems.

### Chunk

`chunkX` and `chunkY` index the rendered ground. The shipped chunk is 64 m on a side, 6400 cm, with 32 quads along an edge. The quad step is 200 cm, the same spacing as terrain.

`chunkX` and `chunkY` are the floor of `worldX` and `worldY` divided by 6400. `chunkX` 0, `chunkY` 0 contains the world origin and extends west and north. On the centred square the interior falls in −8 through 7. The exact north edge and the exact west edge, maximum `worldY` and maximum `worldX`, fall on the next chunk.

Inside a chunk, centimetres grow with `worldX` and `worldY` from that chunk's south-east corner. One quad's corners, in mesh order 00, 10, 01, 11, are the south-east, the south-west, the north-east, and the north-west. Corner 11 is the northwest corner the material code prefers.

```
worldX = chunkX * 6400
worldY = chunkY * 6400
```

That is the chunk's south-east corner. A point in the chunk adds the centimetres from that corner. `ChunkXCm` and `ChunkYCm` are those centimetres. The chunk under a world position is:

```
chunkX = floor(worldX / 6400)
chunkY = floor(worldY / 6400)
```

### Actor

These coordinates are measured on an actor. The actor's location and yaw place them into world. They do not index the open-world square.

`localX` and `localY` are centimetres on the actor. Yaw 0 leaves +localX on +worldX, west, and +localY on +worldY, north. Yaw 90 turns +localX onto +worldY.

A stronghold floor is pointy-top hexes, 120 cm flat to flat. `floorX` increases +localX. `floorY` increases +localY, and an odd row shifts half a hex toward +localX. On the cave, interior +localX runs in from the mouth. Exterior +localX points out of the mouth along the yaw table.

A fortress plan measures `planX` east and `planY` north before the actor is turned. After placement, −localX is plan north, the gate side, turned toward the grass.

A lair maze is 11 by 9 squares of 200 cm. `mazeX` 0, `mazeY` 0 is the minimum local corner. A square's centre is half a side in from that corner. Yaw 0 at the entrance looks into the maze, and yaw 180 at the exit looks into the maze.

`SolidStronghold::OrientationYawDeg` in [SolidStronghold.h](../Strongholds/SolidStronghold.h) is the yaw table. The floor grid is [SolidStrongholdGrid.h](../Strongholds/Furnishings/SolidStrongholdGrid.h). The cave's +localX is on [SolidDungeonLevel.h](../Strongholds/Dungeon/SolidDungeonLevel.h), and the mouth's half-step in localY is `FSolidDungeon::ShelfLateralCm` in [SolidDungeon.cpp](../Strongholds/Dungeon/SolidDungeon.cpp). Plan east, plan north, and the −localX gate are on [SolidFortress.h](../Strongholds/Fortress/SolidFortress.h). The maze is [SolidLair.h](../Lairs/SolidLair.h).

## Code

World axes and terrain live on `USolidWorldChart` in [SolidWorldChart.h](SolidWorldChart.h). `PointAxis` is 513, the terrain grid. The comment on the class states the compass and which edge is index 0. `Points` is the chart asset's copy of that grid, one biome per terrain point.

[SolidWorldChart.cpp](SolidWorldChart.cpp) is the arithmetic:

- `TerrainPointToWorld` and `TerrainPointAt` project a point. `SampleBiome` reads the biome at a world position. `TerrainPointAt` writes `OutTerrainPoint`, an `FIntPoint` whose X and Y are `terrainX` and `terrainY`. The helpers next to them are `WorldAtPoint` and `PointAtWorld`. Their indexes are `TerrainX` and `TerrainY`. `PointAtWorld` passes the fraction from the minimum X, the east edge, toward the maximum X. The parameter on that helper is named `EastFromWest`.

[SolidWorldChartMark.h](SolidWorldChartMark.h) stores `TerrainPoint`. +X is west and +Y is north. Callers project `TerrainPoint` with `TerrainPointToWorld`.

[SolidTerrainMap.h](SolidTerrainMap.h) and [SolidTerrainMap.cpp](SolidTerrainMap.cpp) are terrain. `Build` centres `WorldMinCm` on the world origin, so terrain index 0 is at world (−51200, −51200). `GetWorldMinXY` and `GetWorldMaxXY` are the centred square. `GetPoint(TerrainX, TerrainY)` is row-major from the terrain grid's south-east corner. `WorldToIndex` writes `OutTerrainX` and `OutTerrainY`. `SampleHeight` and `SamplePoint` interpolate inside the surrounding quad, and the biome stays on the nearest point.

[SolidTerrainTypes.h](SolidTerrainTypes.h) is one terrain point: `FSolidTerrainPoint` holds `WorldX`, `WorldY`, and height. The quad-material comments name corners 00, 10, 01, and 11, and call corner 11 the northwest corner.

[SolidTerrainChunk.cpp](SolidTerrainChunk.cpp) builds a chunk mesh from the chunk origin plus the centimetres inside it. The quad loop uses corners `I00`, `I10`, `I01`, and `I11`. The vertex west of corner 11 is two steps past the quad's south-east.

[SolidTerrainStreamer.h](SolidTerrainStreamer.h) holds `ChunkWorldSize` (6400 cm), `QuadsPerSide` (32), and `TerrainPointSpacing` (200 cm). `WorldToChunkCoord` in [SolidTerrainStreamer.cpp](SolidTerrainStreamer.cpp) is `chunkX` and `chunkY`. `MarkFogChunksAt` names the centimetres inside that chunk `ChunkXCm` and `ChunkYCm`. The streamer's map comment still records the old 8 m by 16 m cells.

[SolidRoad.h](SolidRoad.h) holds `TerrainQuadCm` and `StripPaintOffset`, the −100 cm shift of `worldX` and `worldY`. [SolidRoad.cpp](SolidRoad.cpp) walks terrain steps. `PointSteps` starts with `(1, 0)` and `(0, 1)`, one step west and one step north. A solved route keeps its terrain points.

[SolidTerrainNoise.h](SolidTerrainNoise.h) samples height from `worldX` and `worldY`.

Town sign offsets are centimetres from the marker, +worldX west and +worldY north, on [SolidTowns.h](../Towns/SolidTowns.h). The yaw constants there match the yaw table.
