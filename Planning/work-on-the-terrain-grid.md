# Work on the terrain grid

Status: New

## Story

This story depends on [Remove WorldMap.txt](remove-world-map-txt.md), which is Ready. That story ships first. It starts from the world that story leaves: `Content/WorldMap.txt` is gone, and the game loads `/Game/World/WorldChart`.

The open world is a square of terrain points, 513 on a side, 200 cm apart. The chart still stores the land as the old text cells: 128 columns by 64 rows across that square, so a cell is 8 m east–west and 16 m north–south. Those rectangles came from the character cells. A forest, a mountain, a town, or the sea fills the whole cell. The ground steps at the cell edge. A mountain rises 10 m. The other land sits 1 m up. Those steps are the blocks in the World chart view.

A road already runs inside that cell. The route is a path of cells, and the strip on the ground is 1 m, 2 m, 3 m, or 4 m wide, painted on the 200 cm terrain quads. The thin road through the blocks is that strip. A river marker is a 4 m channel in the same way.

The chart and the game work on the terrain grid, the surface that strip already uses. A terrain point carries the biome. The World chart mode works on those points.

## Acceptance

- The terrain grid stays 513 by 513 points at 200 cm.
- The chart stores the land on that grid.
- A terrain point takes its biome from that grid. Forest, mountain, town, desert, sea, and grassland change between neighbouring points.
- The shelves follow those points. A mountain's 10 m step, and the 1 m step of the other land, is the step between points.
- A road strip stays 1 m, 2 m, 3 m, or 4 m, painted on the terrain quads.
- A town, a stronghold, a stronghold site, a lair mouth, and a custom note are placed on that grid.
- The World chart mode reads and changes that grid.

## Implementation notes

- The 2:1 cell came from the text characters. A square cell would only change that shape. This story uses the terrain grid the streamer already builds: `TerrainMapSize` 513 and `TerrainPointSpacing` 200.
- `SampleBiome` paints a road strip and a 4 m river inside a cell. Every other biome returns the whole cell. That split is the chart view: a thin road, and block shelves.
- `SolidRoad::Solve` walks map cells. The strip is then offset by half a terrain quad (`TerrainQuadCm` 200) so it sits on the quad. The biome joins the strip on that quad.
- A mark stores a world position. `LoadFromChart` turns that position into a cell. On the grid, the position stays on the point.
- [Remove WorldMap.txt](remove-world-map-txt.md) has already shipped. The chart this story changes is `/Game/World/WorldChart`.
