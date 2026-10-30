# The World chart drops the cell

Status: New

## Story

The developer works in the World chart mode. Paint already stamps terrain points, 2 m apart. The chart still draws a hard outline around each cell, in that cell's colour, about 80 cm above the ground. View and Place still use that cell. The cursor box is the cell, the hover line describes the cell, and a click places the mark on the cell.

The chart still stores that land as cells. A road still solves as cells, and play still reads those cells. The world chart dump still prints the cells. The hover line names a road when the cell sits on the solved route.

The mode, the chart, the road, play, and the dump use the terrain point Paint already uses. The hover line names the road when the point under the cursor sits on it. The World chart section of `AGENTS.md` describes that same point grid.

A road starts as a straight line from one end to the other. Where that line meets banned ground, the route goes around it. The search does not look at every terrain point for every step. Mountain and sea are banned for a trail, a track, a road, and a highway. Forest and desert are banned for a road and a highway. A river is banned for all four. The route may cross a river at a right angle to the river, and that crossing costs more than one ordinary step. The other step costs only choose the route. This story does not set them.

This story introduces no new class.

## Acceptance

- In the World chart mode, the ground shows no outline around a cell.
- View and Place follow the terrain point under the cursor. That point is the one Paint already uses, 2 m across.
- A left click in Place puts the mark on that point. Delete removes the mark on that point.
- The hover line describes that point. It names the road, its ends, and its type when that point sits on the road.
- The chart stores the land on the terrain points.
- A road solves on those terrain points. It starts as a straight line between its ends. Where that line meets banned ground, the route goes around it. The search does not look at every terrain point for every step.
- Mountain and sea are banned for a trail, a track, a road, and a highway. Forest and desert are banned for a road and a highway.
- A river is banned for a trail, a track, a road, and a highway. The route may cross it at a right angle to the river. That crossing costs more than one ordinary step.
- Play reads those terrain points.
- The world chart dump prints those terrain points. A mark in the dump is that mark's terrain point.
- The World chart section of `AGENTS.md` describes the land as those terrain points.

## Implementation notes

`FSolidWorldChartEdMode::RebuildOverlay` draws one rectangle per chart cell, in that cell's biome colour, 80 cm above the ground. `DrawCellOutline` draws the cell under the cursor for View and Place. Paint already draws the point brush.

Place, delete, and the View hover line use the cell. The place and delete methods on `USolidWorldChart` still take a cell. A mark stores a world position. This story lands that position on the terrain point under the cursor. `DescribeCell` adds the road sentence when `SolidRoad::FRoute::Cells` contains the cell. `DescribePoint` has no road sentence.

The chart's cell array is `USolidWorldChart::Cells`. `SolidRoad::FRoute::Cells` is the solved path. The dump writes that cell grid as `rows`, with the cell `width` and `height`, and logs each mark's cell row and column. It already writes the terrain points as `points`.

The World chart section of `AGENTS.md` says the grid is 128 columns by 64 rows, and that a cell holds a marker biome. This story replaces that description with the terrain points. The fog note about per-cell lattices stays.

`SolidRoad::Solve` scans every cell on every step. `RiverCost`, `RiverBendCost`, and `RiverBendReach` are what make a road or a highway cross a river and leave it. This story replaces that search with the straight line and the banned ground above. A river crossing costs more than one ordinary step. The story does not set the other step costs. Play does not read them. The river channel is 4 m wide.
