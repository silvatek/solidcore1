# HUD fog at the player

Status: Done

## Story

A developer reads the debug HUD while the captain stands in the open world. The fog number on that line is above 0 at the player's location. Fog there is always 0, so the number is not useful. It is probably the HUD showing the wrong sample. A value that is always 0 does not need to be on the line.

## Acceptance

- While the captain is in the world, the debug HUD does not report fog or mist.
- The biome line keeps the biome and the threat.

## Implementation notes

- Seen on SC1-0535, standing next to the Iglin-bridge raptor. The captain had walked there. The line read `fog=1.00 mist=1.00`.
- `fog` is `USolidTerrainMap::SamplePoint` at the pawn, a bilinear blend of the four surrounding points. `mist` is `GetRenderedFogAmount()`, which follows `SampleMistAmountAround`. That takes the max of the pawn sample and of samples 25 m and 50 m away.
- The trail, the curtains, and the clear radius were left as they were. On SC1-0636 the debug line reads `Biome … threat=…`.
- Mist sits at 1 whenever full fog is within 50 m, and height fog is off, so that number came off the line as well.
