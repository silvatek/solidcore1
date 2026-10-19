# Fog stays cleared

Status: Done

## Story

The player walks, and the fog clears along that trail. The player enters a stronghold or a lair, then leaves, and goes back to ground they already walked. That ground is still as clear as it was. Ground they have not reached is still in fog.

## Acceptance

- A terrain point whose `FSolidTerrainPoint.Fog` the trail has lowered still has that same value after the player enters and leaves a stronghold, and after the player enters and leaves a lair.
- A point whose `Fog` is still `1.0f` stays at `1.0f` across those trips.
- Those trips do not raise any point's `Fog`.
- A new game still starts with every point fully fogged, and the first bands still open around the player once they are placed.

## Implementation notes

- Fog lives on `FSolidTerrainPoint.Fog` in `USolidTerrainMap`. `Build` sets every point to `1.f`. `CenterExplorationFogOn` then replaces every point from distance to the player. `ApplyExplorationFogAround` only lowers fog: `0` inside 25 m, and `min(Fog, 0.5)` from 25 m to 50 m.
- Enter and leave travel with `OpenLevel`. Before leaving the open world for a stronghold or lair, `USolidGameInstance` stores every point's Fog. When the open world loads again, those values are applied instead of a fresh `CenterExplorationFogOn`.
- `FSolidTravelSession` still carries the company, the sight, and where to stand. Trail fog lives on the game instance beside that session.
- Bands, and the rule that fog clears from the pawn trail: [Source/SolidCore1/Fog/README.md](../../Source/SolidCore1/Fog/README.md).
