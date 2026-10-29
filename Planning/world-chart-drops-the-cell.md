# The World chart drops the cell

Status: New

## Story

The developer works in the World chart mode. Paint already stamps terrain points, 2 m apart. The chart still draws a hard outline around each cell, in that cell's colour, about 80 cm above the ground. View and Place still use that cell. The cursor box is the cell, the hover line describes the cell, and a click places the mark on the cell. The mode no longer uses a cell. It uses the terrain point Paint already uses.

This story introduces no new class.

## Acceptance

- In the World chart mode, the ground shows no outline around a cell.
- View and Place follow the terrain point under the cursor. That point is the one Paint already uses, 2 m across.
- A left click in Place puts the mark on that point. Delete removes the mark on that point.
- The hover line describes that point.

## Implementation notes

`FSolidWorldChartEdMode::RebuildOverlay` draws one rectangle per chart cell, in that cell's biome colour, 80 cm above the ground. `DrawCellOutline` draws the cell under the cursor for View and Place. Paint already draws the point brush.

Place, delete, and the View hover line use the cell. The place and delete methods on `USolidWorldChart` still take a cell. A mark stores a world position. This story lands that position on the terrain point under the cursor.

The chart still stores its cell array. A road still solves as cells. Play still reads those. The world chart dump still prints cells. This story does not change them. The View hover line today names a road when the cell sits on the solved route. The point description does not. This story does not add that road line.
