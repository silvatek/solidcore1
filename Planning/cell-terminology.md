# Cell Terminology

Status: New

## Story

The developer reads the source and the tests. Cell means a world map cell, the old rectangle that held a biome. That use is deprecated. The source drops it, apart from a limited reference to the historical grid.

A stronghold piece sits on a hex. Furniture placement says hex.

A lair layout is made of slabs. The plan says slab.

An inventory position is a slot. The pack, the chest, and the crafting grid say slot.

A square of the fog curtain on a chunk is a chunk point. The curtain says chunk point.

The search that listed these uses is `tools/cell_census.py`.

This story introduces no new class.

## Acceptance

- A use of cell for a world map cell is gone from the product source and the tests, apart from a limited reference to the historical grid.
- Stronghold furniture placement says hex where it said cell.
- A lair layout says slab where it said cell.
- An inventory position says slot where it said cell.
- The comments on `FSolidWorldChartMark` call the marker's position a terrain point. They do not call it a cell.
- A square of the fog curtain says chunk point where it said cell.

## Implementation notes

`SolidStrongholdGrid::FCell` is the column and row of a floor hex. `SolidStrongholdGrid::FHex` is the hex already placed, with its centre. The cell word on `FCell` becomes hex. `FCell` does not become the existing `FHex` type. The floor those pieces occupy is that placement, including a stair, a hole, a landing, and the door hex.

`SolidLair::CellCm` is the slab width, 200 cm. `CellOf`, `CellCenter`, `IsOpenCell`, and `LairCells` are that plan.

`FSolidStronghold::Cell` stores the terrain point of the chart marker. A marker records that terrain point. The world chart has no cells. The exterior, the interior, and the passage hit copy that point as `StrongholdCell`. The word cell on that point goes. The point stays the terrain point.

The struct comment on `FSolidWorldChartMark` says the position is the terrain point of the cell centre, and that the chart turns that position into a cell and keeps the cell. The `TerrainPoint` comment says "The cell centre." Both describe the terrain point.

`FirstEmptyPackCell` and the pack cell, chest cell, and empty cell in the inventory are slots. A crafting slot already uses that word.

`EdgeOf` takes `CellX` and `CellY`, the column and row of one curtain square inside the chunk. `SlotForCell` picks the material for that square. `FogCells` is how many of those squares run along the chunk edge. The mesh params call them "Cells along one chunk edge." `FogQuadsPerSide` says "Fog curtain cells per chunk edge." Each square is 2 m, the terrain-point step. The word on that square becomes chunk point.

`tools/cell_census.py` scans `Source/` and `tools/dump-chart.bat`. A hit is the word cell, or a CamelCase segment such as `CellX`. It labels each hit World map cell, Hex cell, Lair cell, or Other. That label is the search. The names in this story are the ones the source should use. Run `python3 tools/cell_census.py`.
