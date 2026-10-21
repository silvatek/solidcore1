# Work on the terrain grid

Status: New

## Story

This story depends on [Remove WorldMap.txt](remove-world-map-txt.md), which is Ready. That story ships first. It starts from the world that story leaves: `Content/WorldMap.txt` is gone, and the game loads `/Game/World/WorldChart`.

The open world is a square of terrain points, 513 on a side, 200 cm apart. The chart still stores the land as the old text cells: 128 columns by 64 rows across that square, so a cell is 8 m east–west and 16 m north–south. Those rectangles came from the character cells. A forest, a mountain, a town, or the sea fills the whole cell. The ground steps at the cell edge. A mountain rises 10 m. The other land sits 1 m up. Those steps are the blocks in the World chart view.

A road already runs inside that cell. The route is a path of cells, and the strip on the ground is 1 m, 2 m, 3 m, or 4 m wide, painted on the 200 cm terrain quads. The thin road through the blocks is that strip. A river marker is a 4 m channel in the same way.

The chart and the game work on the terrain grid, the surface that strip already uses. A terrain point carries the biome. The World chart mode works on those points.

Paint writes the points under a round brush. The developer chooses the brush width. The smallest brush is one point, 2 m across. A click stamps that brush. A drag lays it along the path, so a diagonal or a curve is one stroke at that width. Shift-drag fills the rectangle from the press to the release, and that rectangle's edges run north–south and east–west. Alt copies the biome of the point under the cursor. The palette is grassland, forest, mountain, town, desert, sea, and river. Painting river sets those points to river. Road stays a strip of its own width. The land starts as the old cells copied onto the points they cover.

A town stays what it is now. The town biome is painted with that brush, the same way as grassland or forest. A town marker links the painted town ground connected to it to one town `SolidTowns` already defines. Two towns of different names whose paint meets is not a map this story builds. If two such markers fall in one painted region, the region keeps the first marker. A stronghold, a stronghold site, a lair entrance, and a lair exit stay one point. A custom note stays one point. The point under a marker is town when the marker is a town, and grassland for every other marker. That is the rule now, and it continues. A single biome point standing on its own is a valid map.

A road keeps its route-finding. The solver keeps its steps and its costs: off mountains and the sea, extra for forest and desert on a road or a highway, and extra for a river. A step reads the biome of the terrain point under it. The search stays on those steps. The strip stays 1 m, 2 m, 3 m, or 4 m.

## Acceptance

- The terrain grid stays 513 by 513 points at 200 cm.
- The chart stores the land on that grid.
- A terrain point takes its biome from that grid. Forest, mountain, town, desert, sea, and grassland change between neighbouring points.
- The shelves follow those points. A mountain's 10 m step, and the 1 m step of the other land, is the step between points.
- A road strip stays 1 m, 2 m, 3 m, or 4 m, painted on the terrain quads. The solver keeps its steps and its costs. A step reads the biome of the terrain point under it.
- A town stays a painted town biome plus a marker. The marker links the connected town-biome points to one town `SolidTowns` already defines. Two different towns in one painted region is not a map this story builds. If it happens, the region keeps the first marker.
- A stronghold, a stronghold site, a lair entrance, a lair exit, and a custom note stay one point.
- The point under a town marker is town. The point under every other marker is grassland. A single biome point standing on its own is a valid map.
- The World chart mode reads and changes that grid.
- Paint writes the terrain points under a round brush. The developer chooses the width. The smallest width is one point, 2 m.
- A click stamps the brush. A drag lays it along the cursor path. Shift-drag fills the rectangle between the press and the release. Alt copies the biome of the point under the cursor.
- The palette is grassland, forest, mountain, town, desert, sea, and river. Painting river sets the points under the brush to river.
- The starting land is the old cells copied onto the points inside them.

## Implementation notes

- The 2:1 cell came from the text characters. A square cell would only change that shape. This story uses the terrain grid the streamer already builds: `TerrainMapSize` 513 and `TerrainPointSpacing` 200.
- `SampleBiome` paints a road strip and a 4 m river inside a cell. Every other biome returns the whole cell. That split is the chart view: a thin road, and block shelves.
- `SolidRoad::Solve` keeps its steps, its costs, and the half-quad strip offset (`TerrainQuadCm` 200). A step reads the terrain point under it. The search does not move onto the 513 by 513 points.
- A mark stores a world position. `LoadFromChart` turns that position into a cell. On the grid, the position stays on the point.
- [Remove WorldMap.txt](remove-world-map-txt.md) has already shipped. The chart this story changes is `/Game/World/WorldChart`.
- A Shift-drag is the axis-aligned fill. The round brush is the stroke that follows a diagonal or a curve. Swamp stays off the palette, as it does today.
- The point under a marker is the continuing simplification. A town marker's point is town. A stronghold, a site, a lair mouth, and a custom note use grassland. The code leaves a lone point as it was painted.
- Dragongate is two town marks today, and they are one town. That stays when both marks stand in the same painted town ground.
