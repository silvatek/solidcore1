# Coordinates

The open world is one square in Unreal, measured in centimetres. World +X is west, world +Y is north, and world +Z is up. Unreal's forward axis is west, and Unreal's right axis is north. Later sections use only systems introduced above them.

## The systems

### World centimetres

A position in the running game is an Unreal `FVector`. X and Y are the ground plane. Z is height. Height noise is sampled from these centimetres.

The shipped square is 1024 m on a side. World centimetres are used on the square that starts at the origin, from world (0, 0) to world (102400, 102400), and on the centred square, from world (−51200, −51200) to world (51200, 51200). A world position on the square that starts at the origin shifts by the difference of the two minima to land on the centred square. The axes do not change.

An actor's yaw is a facing in this space. Yaw 0 faces west, along +X. Positive yaw turns from west toward north.

| Direction | World axis | Actor yaw |
| --- | --- | --- |
| West | +X | 0 |
| North | +Y | 90 |
| East | −X | 180 |
| South | −Y | −90 |
| Up | +Z | |

A sign that faces south uses yaw −90. A sign that faces north uses yaw 90.

### Terrain points

The ground is a square lattice of sample points. The shipped grid is 513 points on a side, 200 cm apart, so there are 512 steps from edge to edge and the steps fill the 1024 m square.

Index (0, 0) is the south-east corner, the minimum X and the minimum Y. The X index increases west. The Y index increases north, so Y index 0 is the south edge and a higher Y index is further north. The built grid lies on the centred square, so X index 0, Y index 0 is at world (−51200, −51200). Storage order is Y index × 513 + X index. Walking that order walks west, then steps north to the east end of the next Y index.

A point carries a world position, a surface height, a biome, and exploration fog. Fog is a value on the point. Distance for fog is measured in world centimetres along the ground, from the pawn's trail.

A point samples the ground at that position. Between four points, height, threat, and fog are bilinear. The biome is the nearest point's biome.

### Chart cells

The same square is also 128 columns by 64 rows of rectangles. A column is 8 m east–west. A row is 16 m north–south. Those rectangles are the old character cells. Forest, mountain, town, desert, sea, and grassland were painted a whole cell at a time. The points now hold the biome the game plays, and a cell still holds the marker biome.

Column 0 is the west edge. Row 0 is the north edge. A higher cell row is further south. Storage is row-major: index = row × 128 + column. Walking the array walks east along a row, then steps south to the west end of the next row.

An `FIntPoint` stores both grids. Its X is a point's X index, or a cell's column. Its Y is a point's Y index, or a cell's row. On a terrain point, increasing X walks west and increasing Y walks north. On a chart cell, increasing X walks east and increasing Y walks south.

A mark (a town, a stronghold, a site, a lair mouth, a note, an encounter, or a bridge) sits on one cell. Its stored position is that cell's centre, in world centimetres on the square that starts at the origin. A road names two ends, a town or a stronghold, and the solver walks terrain points between them.

On the shipped sizes, cell edges and cell centres fall on terrain points. A column is 4 point-steps wide. A row is 8 point-steps tall. The centre of column 0, row 0 is terrain point (510, 508): two steps in from the west edge and four steps in from the north edge.

### Description coordinates

The old map text, and a dump of the grid, count from 1 at the top-left. These counts are the description row and the description column. The top-left is the north-west cell, chart cell column 0, row 0, so description row 1, description column 1. A line `! 8,21 = Town Iglin` is description row 8, description column 21, which is chart cell column 20, row 7.

### Chunks and mesh quads

The rendered ground is a tiling of chunks. The shipped chunk is 64 m on a side, 6400 cm, with 32 quads along an edge. The quad step is 200 cm, the same as the terrain points.

Chunk X and chunk Y are the floor of world X and world Y divided by 6400 cm. Chunk X 0, chunk Y 0 contains the world origin and extends west and north. On the centred square, the interior falls in chunk X and chunk Y from −8 through 7. The exact north edge and the exact west edge, maximum world Y and maximum world X, fall on the next chunk X or chunk Y. Streaming treats a centimetre inside that edge as the last interior chunk.

Inside a chunk, mesh X and mesh Y grow with world X and world Y. The chunk actor stands on the chunk's minimum corner, which is the south-east corner of that tile. Mesh +X is west and mesh +Y is north.

One quad has four corners, named in mesh order 00, 10, 01, 11:

| Corner | Steps from the quad's south-east | Compass |
| --- | --- | --- |
| 00 | (0, 0) | South-east |
| 10 | one step +X | South-west |
| 01 | one step +Y | North-east |
| 11 | one step +X and one step +Y | North-west |

Corner 11 is the northwest corner the material code prefers. The next vertex to the west is the one that code reads beside it.

### Plans that stay local

A stronghold floor, a fortress plan, and a lair maze are measured in their own actors. The actor's location and yaw put a local position into world centimetres.

A stronghold floor is pointy-top hexes. Flat to flat is 120 cm. Floor columns and floor rows are measured from a floor origin in local X and local Y. The floor column increases local +X. The floor row increases local +Y by the row step, and an odd floor row shifts half a hex toward +X. The cave's interior local +X runs in from the mouth. The exterior's local +X points out of the mouth, along the yaw in the yaw table.

A fortress plan measures plan +X east and plan +Y north before the actor is turned. After placement, local −X is the gate side, which is plan north, turned toward the grass.

A lair maze is 11 maze columns by 9 maze rows of 200 cm squares in the interior actor. Maze column 0, maze row 0 is the corner at the minimum local X and Y. The centre of a square is half a side in from its minimum corner. The captain's yaw inside the maze is local: 0 at the entrance, looking into the maze, and 180 at the exit.

## Transformations

The formulas below use world X and world Y. Minimum world X and maximum world X are the edges of the square in use, the square that starts at the origin or the centred square, and the same for world Y. On the shipped square that difference is 102400 cm. There are 512 steps from one edge of the terrain points to the other.

### Square that starts at the origin, and the centred square

```
world X on the centred square = world X on the square that starts at the origin + (−51200 − 0)
world Y on the centred square = world Y on the square that starts at the origin + (−51200 − 0)
```

A stored mark is a world position on the square that starts at the origin, so this shift puts it on the centred square. The compass of each axis is unchanged. The inverse subtracts that same difference.

### Chart cell and world centimetres

Column 0 is west, so the column is measured back from the maximum world X. Row 0 is north, so the row is measured back from the maximum world Y.

```
column width = (maximum world X − minimum world X) / 128
row height   = (maximum world Y − minimum world Y) / 64

world X = maximum world X − (column + 0.5) × column width
world Y = maximum world Y − (row + 0.5) × row height

column = floor( (maximum world X − world X) / (maximum world X − minimum world X) × 128 )
row    = floor( (maximum world Y − world Y) / (maximum world Y − minimum world Y) × 64 )
```

Both indexes are clamped to the grid. A position exactly on a boundary belongs to the cell whose floor value is that integer. The west edge and the north edge of a cell stay in that cell. The east edge belongs to the column to the east, and the south edge belongs to the row to the south. An index past the last column or row clamps back into the grid, so the outer east edge and the outer south edge stay in the last cell.

From a cell, the neighbouring cells are:

| Direction | Column | Row |
| --- | --- | --- |
| North | 0 | −1 |
| East | +1 | 0 |
| South | 0 | +1 |
| West | −1 | 0 |

Description coordinates are the same cell with the origin moved to 1:

```
description row    = row + 1
description column = column + 1
```

### Terrain point and world centimetres

X index 0, Y index 0 is the minimum corner. The index is the fraction of the square, rounded.

```
X index = round( (world X − minimum world X) / (maximum world X − minimum world X) × 512 )
Y index = round( (world Y − minimum world Y) / (maximum world Y − minimum world Y) × 512 )

world X = minimum world X + X index × (maximum world X − minimum world X) / 512
world Y = minimum world Y + Y index × (maximum world Y − minimum world Y) / 512
```

On the centred square the spacing is 200 cm, so the same point is `round((world X − (−51200)) / 200)` and the same for world Y.

A step of +1 in the X index is 200 cm west. A step of +1 in the Y index is 200 cm north. The road solver walks those steps, including the four diagonals.

The point under a cell centre uses the centre's fraction of the square:

```
X index = round( (1 − (column + 0.5) / 128) × 512 )
Y index = round( (1 − (row + 0.5) / 64) × 512 )
```

For column 0, row 0 that is X index 510, Y index 508. The next column's centre is 4 points to the east. The next row's centre is 8 points to the south.

### Which points a cell keeps

Membership uses the cell inverse above, so a point exactly on an edge follows the floor rule. On the shipped grid a column therefore keeps its west-edge points and gives its east-edge points to the eastern neighbour, and a row keeps its north-edge points and gives its south-edge points to the southern neighbour. The kept points sit 100 cm west and 100 cm north of the cell's geometric centre.

### Chunk and world centimetres

```
chunk X = floor(world X / 6400)
chunk Y = floor(world Y / 6400)

world X = chunk X × 6400 + mesh X
world Y = chunk Y × 6400 + mesh Y
```

Mesh X and mesh Y are centimetres from the chunk's south-east corner, increasing west and north. A quad that starts at mesh vertex (i, j) has corner 00 there, corner 10 one step west, corner 01 one step north, and corner 11 one step west and one step north.

The painted road strip is shifted half a quad, 100 cm, toward −world X and −world Y from the terrain point the strip was rounded onto. That is 100 cm east and 100 cm south. A bridge uses that same shift.

### Actor yaw and a local plan

A local offset (local X, local Y) on an actor whose yaw is ψ becomes a world offset by Unreal's yaw rotation. Yaw 0 leaves local +X on world +X (west) and local +Y on world +Y (north). Yaw 90 turns local +X onto world +Y (north).

The exit yaw for a stronghold mouth is the world yaw table: north 90, east 180, south −90, west 0. The exterior's local +X points along that yaw, out toward the grass. The cave mouth adds a further half-step, 100 cm, in the actor's local Y. The sign of that shift depends on which way the mouth faces.

A hex centre, before the actor yaw, is:

```
local X = floor origin X + floor column × 120 + (floor row odd ? 60 : 0)
local Y = floor origin Y + floor row × row step
```

The row step is 1.5 times the hex side. The side is the centre-to-vertex distance of a pointy-top hex whose flat-to-flat width is 120 cm.

A maze square's centre, in the interior actor, is:

```
local X = (maze column + 0.5) × 200
local Y = (maze row + 0.5) × 200
```

A local position maps back to a maze column and a maze row by the floor of local X and local Y divided by 200.

## Where the code is

World axes, the two grids, and the cell and point projections live on `USolidWorldChart` in [SolidWorldChart.h](SolidWorldChart.h). `MapWidth` and `MapHeight` are the 128 by 64 cells. `PointAxis` is the 513 points. The comment on the class states the compass and which row is zero. `Cells` and `Points` are the row-major arrays.

[SolidWorldChart.cpp](SolidWorldChart.cpp) is the arithmetic:

- `DefaultWorldBounds` reads the terrain map class default, which is the square that starts at the origin.
- `TerrainPosition` adds the centred square's minimum minus the minimum of the square that starts at the origin.
- `CellCentre` and `CellToWorldXY` project a cell centre. `WorldXYToCell` and `CellAtWorld` project the other way.
- `TerrainPointToWorld` and `TerrainPointAt` project a point. The helpers next to them are `WorldAtPoint`, `PointAtWorld`, and `PointUnderCell`. `PointAtWorld` passes the fraction from the minimum X, the east edge, toward the maximum X. The parameter on that helper is named `EastFromWest`.
- `WorldMapParseBangLocation` turns a `! row,column` line into a zero-based cell. Column is `MapX`, row is `MapY`.

[SolidWorldChartMark.h](SolidWorldChartMark.h) stores `PositionCm` as the cell centre in centimetres, with +X west and +Y north. That value is on the square that starts at the origin, until `TerrainPosition` shifts it onto the centred square.

[SolidTerrainMap.h](SolidTerrainMap.h) and [SolidTerrainMap.cpp](SolidTerrainMap.cpp) are the built grid. `Build` centres `OriginXY` on the world origin. `GetWorldMinXY` and `GetWorldMaxXY` are the centred square. `GetPoint` is row-major with the same index order as the chart points. `WorldToIndex` rounds to the nearest point. `SampleHeight` and `SamplePoint` interpolate inside the surrounding quad, and the biome stays on the nearest point.

[SolidTerrainTypes.h](SolidTerrainTypes.h) is one point: `FSolidTerrainPoint` holds X, Y, and height in world centimetres. The quad-material comments name corners 00, 10, 01, and 11, and call corner 11 the northwest corner.

[SolidTerrainChunk.cpp](SolidTerrainChunk.cpp) builds a chunk mesh from `chunkOrigin + vertex × step`. The quad loop uses `I00`, `I10`, `I01`, and `I11` in that compass order. The vertex west of corner 11 is two steps of local X past the quad's south-east.

[SolidTerrainStreamer.h](SolidTerrainStreamer.h) holds `ChunkWorldSize` (6400 cm), `QuadsPerSide` (32), and `TerrainPointSpacing` (200 cm). `WorldToChunkCoord` in [SolidTerrainStreamer.cpp](SolidTerrainStreamer.cpp) is the floor division. The streamer's map comment records the 8 m by 16 m cells.

[SolidRoad.h](SolidRoad.h) holds `TerrainQuadCm` and `StripPaintOffset`, the half-quad shift toward −X and −Y. [SolidRoad.cpp](SolidRoad.cpp) walks point steps. `PointSteps` starts with `(1, 0)` and `(0, 1)`, one point west and one point north. A solved route keeps both the cells and the terrain points.

[SolidTerrainNoise.h](SolidTerrainNoise.h) samples height from world X and Y in centimetres.

River banks step in cell indexes. North, east, south, and west are in `WorldMapRiverSideStep` in [SolidWorldChart.cpp](SolidWorldChart.cpp).

Actor yaw for a mouth is `SolidStronghold::OrientationYawDeg` in [SolidStronghold.h](../Strongholds/SolidStronghold.h). The comment there is the compass: 90 is north, 0 is west, −90 is south, 180 is east. The cave's half-step in local Y is `FSolidDungeon::ShelfLateralCm` in [SolidDungeon.cpp](../Strongholds/Dungeon/SolidDungeon.cpp). The hex grid is [SolidStrongholdGrid.h](../Strongholds/Furnishings/SolidStrongholdGrid.h). `HexCenter` is the odd-row shift. The cave's local +X is described on [SolidDungeonLevel.h](../Strongholds/Dungeon/SolidDungeonLevel.h). The fortress plan's east and north, and the actor −X gate, are on [SolidFortress.h](../Strongholds/Fortress/SolidFortress.h) and the fortress actors beside it.

Town sign offsets are centimetres from the cell centre, +X west and +Y north, on [SolidTowns.h](../Towns/SolidTowns.h). The yaw constants there match the yaw table.

The lair maze is [SolidLair.h](../Lairs/SolidLair.h). `CellCm` is 200. `CellCenter` and `ContainsLocal` are the cell projection in the interior actor.
