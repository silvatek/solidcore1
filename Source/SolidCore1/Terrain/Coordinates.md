# Coordinates

The open world is one square in Unreal, measured in centimetres. Each coordinate system below has a one-word name. A coordinate is that name followed by X or Y, as in `worldX` and `terrainY`. Later sections use only names introduced above them.

## Contents

- [Systems](#systems)
  - [World](#world)
  - [Terrain](#terrain)
  - [Chart](#chart)
  - [Cell](#cell)
  - [Text](#text)
  - [Chunk](#chunk)
  - [Mesh](#mesh)
  - [Local](#local)
  - [Floor](#floor)
  - [Plan](#plan)
  - [Maze](#maze)
- [Transformations](#transformations)
  - [Chart and terrain](#chart-and-terrain)
  - [Terrain and world](#terrain-and-world)
  - [Cell and world](#cell-and-world)
  - [Text and cell](#text-and-cell)
  - [Cell and terrain](#cell-and-terrain)
  - [Chunk and mesh](#chunk-and-mesh)
  - [Local and world](#local-and-world)
  - [Floor and local](#floor-and-local)
  - [Plan and local](#plan-and-local)
  - [Maze and local](#maze-and-local)
- [Code](#code)

## Systems

### World

`worldX`, `worldY`, and `worldZ` are Unreal centimetres. `worldX` is west, `worldY` is north, and `worldZ` is up. Unreal's forward axis is west, and Unreal's right axis is north.

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

`terrainX` 0, `terrainY` 0 is the south-east corner, at world (−51200, −51200). `terrainX` increases west. `terrainY` increases north, so `terrainY` 0 is the south edge. Storage order is `terrainY × 513 + terrainX`.

A terrain point carries a world position, a surface height, a biome, and exploration fog. Fog is a value on the point. Between four terrain points, height, threat, and fog are bilinear. The biome is the nearest point's biome.

### Chart

`chartX` and `chartY` index the world chart's own 2 m grid. It is 513 points on a side, the same count and the same spacing as terrain.

`chartX` 0, `chartY` 0 is the same south-east corner as `terrainX` 0, `terrainY` 0. `chartX` increases west. `chartY` increases north. The indexes match: that corner is 0 on both grids.

The origins in world centimetres do not match. The chart writes the south-east corner at (0, 0). Terrain writes it at world (−51200, −51200). A chart point's stored centimetres are `(chartX × 200, chartY × 200)`.

### Cell

`cellX` and `cellY` index the chart's rectangles. There are 128 columns and 64 rows across the same square. A column is 8 m east–west. A row is 16 m north–south.

`cellX` 0 is the west edge. `cellY` 0 is the north edge. A higher `cellY` is further south. Storage order is `cellY × 128 + cellX`.

`cellX` increases east, and `cellY` increases south. That is the opposite of `terrainX` and `terrainY`.

A mark (a town, a stronghold, a site, a lair mouth, a note, an encounter, or a bridge) sits on one cell. Its stored position is that cell's centre, in chart centimetres. On the shipped sizes the centre falls on a chart point, and on the matching terrain point. A column is 4 steps wide. A row is 8 steps tall. The centre of `cellX` 0, `cellY` 0 is `terrainX` 510, `terrainY` 508.

### Text

`textX` and `textY` are the old map text, and a dump of the grid. Both count from 1 at the top-left. The top-left is the north-west cell, `cellX` 0, `cellY` 0.

`textX` is the column. `textY` is the row. A line `! 8,21 = Town Iglin` is `textY` 8, `textX` 21, which is `cellX` 20, `cellY` 7.

### Chunk

`chunkX` and `chunkY` index the rendered ground. The shipped chunk is 64 m on a side, 6400 cm, with 32 quads along an edge. The quad step is 200 cm, the same spacing as terrain.

`chunkX` and `chunkY` are the floor of `worldX` and `worldY` divided by 6400. `chunkX` 0, `chunkY` 0 contains the world origin and extends west and north. On the centred square the interior falls in −8 through 7. The exact north edge and the exact west edge, maximum `worldY` and maximum `worldX`, fall on the next chunk.

### Mesh

`meshX` and `meshY` are centimetres inside one chunk, measured from that chunk's south-east corner. `meshX` increases west, with `worldX`. `meshY` increases north, with `worldY`.

One quad has four corners, named in mesh order 00, 10, 01, 11:

| Corner | From the quad's south-east | Compass |
| --- | --- | --- |
| 00 | (0, 0) | South-east |
| 10 | one step +meshX | South-west |
| 01 | one step +meshY | North-east |
| 11 | one step +meshX and one step +meshY | North-west |

Corner 11 is the northwest corner the material code prefers. The next vertex to the west is the one that code reads beside it.

### Local

`localX` and `localY` are centimetres on an actor. The actor's location and yaw put a local position into world. Yaw 0 leaves +localX on +worldX and +localY on +worldY. Yaw 90 turns +localX onto +worldY.

### Floor

`floorX` and `floorY` index a stronghold floor of pointy-top hexes. Flat to flat is 120 cm. `floorOriginX` and `floorOriginY` are the local position they are measured from. `floorX` increases +localX. `floorY` increases +localY by the row step, the distance from one `floorY` to the next, and an odd `floorY` shifts half a hex toward +localX.

The cave's interior +localX runs in from the mouth. The exterior's +localX points out of the mouth, along the yaw table.

### Plan

`planX` and `planY` are centimetres on a fortress plan, before the actor is turned. `planX` increases east. `planY` increases north. After placement, −localX is the gate side, which is plan north, turned toward the grass.

### Maze

`mazeX` and `mazeY` index a lair maze of 11 columns by 9 rows. Each square is 200 cm in the interior actor. `mazeX` 0, `mazeY` 0 is the corner at the minimum local X and local Y. The centre of a square is half a side in from its minimum corner. The captain's yaw inside the maze is local: 0 at the entrance, looking into the maze, and 180 at the exit.

## Transformations

### Chart and terrain

```
chartX = terrainX
chartY = terrainY
```

The indexes name the same corner and take the same steps. The stored centimetres do not. Chart writes index 0 at (0, 0). Terrain writes index 0 at world (−51200, −51200).

```
worldX = chartX * 200 - 51200
worldY = chartY * 200 - 51200
```

### Terrain and world

256 is half of 512, the index of the world origin on the centred square.

```
worldX = (terrainX - 256) * 200
worldY = (terrainY - 256) * 200

terrainX = round(worldX / 200 + 256)
terrainY = round(worldY / 200 + 256)
```

A step of +1 `terrainX` is 200 cm west. A step of +1 `terrainY` is 200 cm north.

### Cell and world

```
worldX = 51200 - (cellX + 0.5) * 800
worldY = 51200 - (cellY + 0.5) * 1600

cellX = floor((51200 - worldX) / 800)
cellY = floor((51200 - worldY) / 1600)
```

Both indexes clamp to the grid. A position exactly on a boundary belongs to the cell whose floor value is that integer. The west edge and the north edge stay in the cell. The east edge belongs to `cellX + 1`, and the south edge belongs to `cellY + 1`. An index past the last column or row clamps back, so the outer east edge and the outer south edge stay in the last cell.

From a cell, the neighbours are:

| Direction | cellX | cellY |
| --- | --- | --- |
| North | 0 | −1 |
| East | +1 | 0 |
| South | 0 | +1 |
| West | −1 | 0 |

On the chart's stored centimetres the same centre is `worldX + 51200` and `worldY + 51200`, because that frame's maximum is 102400 rather than 51200.

### Text and cell

```
textX = cellX + 1
textY = cellY + 1
```

The line writes the row first: `! textY,textX`.

### Cell and terrain

```
terrainX = round((1 - (cellX + 0.5) / 128) * 512)
terrainY = round((1 - (cellY + 0.5) / 64) * 512)
```

For `cellX` 0, `cellY` 0 that is `terrainX` 510, `terrainY` 508. The next column's centre is 4 steps to the east. The next row's centre is 8 steps to the south.

A terrain point exactly on an edge follows the cell inverse above. On the shipped grid a column keeps its west-edge points and gives its east-edge points to the eastern neighbour, and a row keeps its north-edge points and gives its south-edge points to the southern neighbour. The kept points sit 100 cm west and 100 cm north of the cell's geometric centre.

### Chunk and mesh

```
chunkX = floor(worldX / 6400)
chunkY = floor(worldY / 6400)

meshX = worldX - chunkX * 6400
meshY = worldY - chunkY * 6400
```

A quad that starts at mesh vertex (i, j) has corner 00 there, corner 10 one step west, corner 01 one step north, and corner 11 one step west and one step north.

The painted road strip is the terrain point's world position, shifted half a quad:

```
worldX = (terrainX - 256) * 200 - 100
worldY = (terrainY - 256) * 200 - 100
```

That is 100 cm east and 100 cm south of the terrain point. A bridge uses that same shift.

### Local and world

Yaw 0 leaves +localX on +worldX and +localY on +worldY. Yaw 90 turns +localX onto +worldY.

The exit yaw for a stronghold mouth is the yaw table: north 90, east 180, south −90, west 0. The exterior's +localX points along that yaw, out toward the grass. The cave mouth adds a further 100 cm in localY. The sign of that shift depends on which way the mouth faces.

### Floor and local

The row step is 1.5 times the hex side. The side is the centre-to-vertex distance of a pointy-top hex whose flat-to-flat width is 120 cm.

```
localX = floorOriginX + floorX * 120 + (floorY odd ? 60 : 0)
localY = floorOriginY + floorY * rowStep
```

### Plan and local

`planX` increases east and `planY` increases north, in the plan's own centimetres. Placement turns the actor so −localX lies along +planY.

### Maze and local

```
localX = (mazeX + 0.5) * 200
localY = (mazeY + 0.5) * 200

mazeX = floor(localX / 200)
mazeY = floor(localY / 200)
```

## Code

World axes, terrain, chart, and cell live on `USolidWorldChart` in [SolidWorldChart.h](SolidWorldChart.h). `MapWidth` and `MapHeight` are the 128 by 64 cells. `PointAxis` is the 513 points shared by terrain and chart. The comment on the class states the compass and which edge is index 0. `Cells` is cell storage. `Points` is chart storage, in the same index order as terrain.

[SolidWorldChart.cpp](SolidWorldChart.cpp) is the arithmetic:

- `DefaultWorldBounds` is the chart centimetre square, from (0, 0) to (102400, 102400).
- `TerrainPosition` adds −51200, taking chart centimetres to world.
- `CellCentre` and `CellToWorldXY` project a cell centre. `WorldXYToCell` and `CellAtWorld` project the other way.
- `TerrainPointToWorld` and `TerrainPointAt` project a point. The helpers next to them are `WorldAtPoint`, `PointAtWorld`, and `PointUnderCell`. `PointAtWorld` passes the fraction from the minimum X, the east edge, toward the maximum X. The parameter on that helper is named `EastFromWest`.
- `WorldMapParseBangLocation` turns a `! textY,textX` line into a cell. `textX - 1` is `MapX`. `textY - 1` is `MapY`.

[SolidWorldChartMark.h](SolidWorldChartMark.h) stores `PositionCm` as the cell centre. +X is west and +Y is north, in chart centimetres. `TerrainPosition` shifts that value into world.

[SolidTerrainMap.h](SolidTerrainMap.h) and [SolidTerrainMap.cpp](SolidTerrainMap.cpp) are terrain. `Build` centres `OriginXY` on the world origin, so terrain index 0 is at world (−51200, −51200). `GetWorldMinXY` and `GetWorldMaxXY` are the centred square. `GetPoint` uses the same storage order as chart. `WorldToIndex` rounds to the nearest terrain point. `SampleHeight` and `SamplePoint` interpolate inside the surrounding quad, and the biome stays on the nearest point.

[SolidTerrainTypes.h](SolidTerrainTypes.h) is one terrain point: `FSolidTerrainPoint` holds world X, world Y, and height. The quad-material comments name mesh corners 00, 10, 01, and 11, and call corner 11 the northwest corner.

[SolidTerrainChunk.cpp](SolidTerrainChunk.cpp) builds a chunk mesh from `chunkX × 6400 + meshX`. The quad loop uses corners `I00`, `I10`, `I01`, and `I11`. The vertex west of corner 11 is two steps of meshX past the quad's south-east.

[SolidTerrainStreamer.h](SolidTerrainStreamer.h) holds `ChunkWorldSize` (6400 cm), `QuadsPerSide` (32), and `TerrainPointSpacing` (200 cm). `WorldToChunkCoord` in [SolidTerrainStreamer.cpp](SolidTerrainStreamer.cpp) is `chunkX` and `chunkY`. The streamer's map comment records the 8 m by 16 m cells.

[SolidRoad.h](SolidRoad.h) holds `TerrainQuadCm` and `StripPaintOffset`, the −100 cm shift of `worldX` and `worldY`. [SolidRoad.cpp](SolidRoad.cpp) walks terrain steps. `PointSteps` starts with `(1, 0)` and `(0, 1)`, one step west and one step north. A solved route keeps both the cells and the terrain points.

[SolidTerrainNoise.h](SolidTerrainNoise.h) samples height from `worldX` and `worldY`.

River banks step in cell indexes. North, east, south, and west are in `WorldMapRiverSideStep` in [SolidWorldChart.cpp](SolidWorldChart.cpp).

Actor yaw for a mouth is `SolidStronghold::OrientationYawDeg` in [SolidStronghold.h](../Strongholds/SolidStronghold.h). The comment there is the yaw table: 90 is north, 0 is west, −90 is south, 180 is east. The cave's half-step in localY is `FSolidDungeon::ShelfLateralCm` in [SolidDungeon.cpp](../Strongholds/Dungeon/SolidDungeon.cpp). The floor grid is [SolidStrongholdGrid.h](../Strongholds/Furnishings/SolidStrongholdGrid.h). `HexCenter` is the odd-`floorY` shift. The cave's +localX is described on [SolidDungeonLevel.h](../Strongholds/Dungeon/SolidDungeonLevel.h). Plan east, plan north, and the −localX gate are on [SolidFortress.h](../Strongholds/Fortress/SolidFortress.h) and the fortress actors beside it.

Town sign offsets are centimetres from the cell centre, +worldX west and +worldY north, on [SolidTowns.h](../Towns/SolidTowns.h). The yaw constants there match the yaw table.

The lair maze is [SolidLair.h](../Lairs/SolidLair.h). `CellCm` is 200. `CellCenter` and `ContainsLocal` are `mazeX` and `mazeY` in the interior actor.
