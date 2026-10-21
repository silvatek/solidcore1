# Work on the terrain grid

Status: New

## Story

This story depends on [Remove WorldMap.txt](remove-world-map-txt.md), which is Ready. That story ships first. It starts from the world that story leaves: `Content/WorldMap.txt` is gone, and the game loads `/Game/World/WorldChart`.

The open world is a square of terrain points, 513 on a side, 200 cm apart. The chart still stores the land as the old text cells: 128 columns by 64 rows across that square, so a cell is 8 m east–west and 16 m north–south. Those rectangles came from the character cells. A forest, a mountain, a town, or the sea fills the whole cell. The ground steps at the cell edge. A mountain rises 10 m. The other land sits 1 m up. Those steps are the blocks in the World chart view.

A road already runs inside that cell. The route is a path of cells, and the strip on the ground is 1 m, 2 m, 3 m, or 4 m wide, painted on the 200 cm terrain quads. The thin road through the blocks is that strip. A river marker is a 4 m channel in the same way.

The chart and the game work on the terrain grid, the surface that strip already uses. A terrain point carries the biome. The World chart mode works on those points.

Paint writes the points under a round brush. The developer chooses the brush width. The smallest brush is one point, 2 m across. A click stamps that brush. A drag lays it along the path, so a diagonal or a curve is one stroke at that width. Shift-drag fills the rectangle from the press to the release, and that rectangle's edges run north–south and east–west. Alt copies the biome of the point under the cursor. The palette is grassland, forest, mountain, town, desert, sea, and river. Painting river sets those points to river. Road stays a strip of its own width. The land starts as the old cells copied onto the points they cover.

The town biome is painted with that brush, the same way as grassland or forest. A town marker is separate. It links the painted town biome it stands on to one town `SolidTowns` already defines. The marker does not paint the ground. A stronghold, a stronghold site, a lair entrance, and a lair exit are one terrain point each. The paint around that point stays as painted. A custom note is one point as well.

## Acceptance

- The terrain grid stays 513 by 513 points at 200 cm.
- The chart stores the land on that grid.
- A terrain point takes its biome from that grid. Forest, mountain, town, desert, sea, and grassland change between neighbouring points.
- The shelves follow those points. A mountain's 10 m step, and the 1 m step of the other land, is the step between points.
- A road strip stays 1 m, 2 m, 3 m, or 4 m, painted on the terrain quads.
- The town biome is painted with the brush. A town marker links the painted town biome it stands on to one town `SolidTowns` already defines. The marker does not paint the ground.
- A stronghold, a stronghold site, a lair entrance, and a lair exit are one terrain point each. The paint around that point stays as painted. A custom note is one point.
- The World chart mode reads and changes that grid.
- Paint writes the terrain points under a round brush. The developer chooses the width. The smallest width is one point, 2 m.
- A click stamps the brush. A drag lays it along the cursor path. Shift-drag fills the rectangle between the press and the release. Alt copies the biome of the point under the cursor.
- The palette is grassland, forest, mountain, town, desert, sea, and river. Painting river sets the points under the brush to river.
- The starting land is the old cells copied onto the points inside them.

## Implementation notes

- The 2:1 cell came from the text characters. A square cell would only change that shape. This story uses the terrain grid the streamer already builds: `TerrainMapSize` 513 and `TerrainPointSpacing` 200.
- `SampleBiome` paints a road strip and a 4 m river inside a cell. Every other biome returns the whole cell. That split is the chart view: a thin road, and block shelves.
- `SolidRoad::Solve` walks map cells. The strip is then offset by half a terrain quad (`TerrainQuadCm` 200) so it sits on the quad. The biome joins the strip on that quad.
- A mark stores a world position. `LoadFromChart` turns that position into a cell. On the grid, the position stays on the point.
- [Remove WorldMap.txt](remove-world-map-txt.md) has already shipped. The chart this story changes is `/Game/World/WorldChart`.
- A Shift-drag is the axis-aligned fill. The round brush is the stroke that follows a diagonal or a curve. Swamp stays off the palette, as it does today.
- Today a named town cell, a stronghold shelf, and a lair mouth each replace the biome of the whole cell. This story stops that. The town ground is the painted town biome. The marker only names it. A lair or a stronghold is the point the game looks up, and the ground there is the paint.
- Dragongate is two town marks today, and they are one town. The painted town biome linked to the Dragongate marker is that town's ground.
