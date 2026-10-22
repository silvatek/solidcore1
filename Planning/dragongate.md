# Dragongate is one map marker

Status: New

## Story

Dragongate is one town on two map markers. Both sit on row 14, in the gap through the mountains. Column 33 is the eastern cell. Column 34 is the western cell. The stone wall runs across both.

The change is one map marker. Which cell stays, and what the other cell becomes, are open.

## Acceptance

Not written yet.

## Implementation notes

A draft, not reviewed. One way to do it: keep row 14, column 33, the cell on Relion's column, and make row 14, column 34 mountain so the range closes around one cell. The wall would still run east to west. The welcome sign, the three roads, and Enter Dragongate would stay on that cell. The sign's east offset would shorten, because 400 cm east of a single cell's centre sits on the mountain. `Content/WorldMap.txt` and `/Game/World/WorldChart` would change together. A test map could still give one wall-gate town two cells.
