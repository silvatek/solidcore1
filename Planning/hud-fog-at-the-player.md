# HUD fog at the player

Status: New

## Story

A developer reads the debug HUD while the captain stands in the open world. The fog number on that line is above 0 at the player's location. Fog there is always 0, so the number is not useful. It is probably the HUD showing the wrong sample. A value that is always 0 does not need to be on the line.

## Acceptance

- While the captain is in the world, the debug HUD does not report fog above 0 at the player's location.
- The fog number can come off that line. Showing a value that is always 0 adds nothing.

## Implementation notes

- Seen on SC1-0535, standing next to the Iglin-bridge raptor. The captain had walked there. The line read `fog=1.00 mist=1.00`.
- `fog` is `USolidTerrainMap::SamplePoint` at the pawn, a bilinear blend of the four surrounding points. `mist` is `GetRenderedFogAmount()`, which follows `SampleMistAmountAround`. That takes the max of the pawn sample and of samples 25 m and 50 m away.
- The stored trail under the player may already be 0. Leave the trail, the curtains, and the clear radius alone unless a later look shows those points are actually above 0.
- Not Ready. Pick it up if the readout matters.
