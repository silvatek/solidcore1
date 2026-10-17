# Fog stays cleared

Status: Ready

## Story

The player walks, and the fog clears along that trail. The player enters a stronghold or a lair, then leaves, and goes back to ground they already walked. That ground is still as clear as it was. Ground they have not reached is still in fog.

## Acceptance

- A terrain point whose `FSolidTerrainPoint.Fog` the trail has lowered still has that same value after the player enters and leaves a stronghold, and after the player enters and leaves a lair.
- A point whose `Fog` is still `1.0f` stays at `1.0f` across those trips.
- Those trips do not raise any point's `Fog`.
- A new game still starts with every point fully fogged, and the first bands still open around the player once they are placed.

## Implementation notes

- Fog lives on `FSolidTerrainPoint.Fog` in `USolidTerrainMap`. `Build` sets every point to `1.f`. `CenterExplorationFogOn` then replaces every point from distance to the player. `ApplyExplorationFogAround` only lowers fog: `0` inside 25 m, and `min(Fog, 0.5)` from 25 m to 50 m.
- Enter and leave travel with `OpenLevel` (`TravelToDungeon`, `TravelToTower`, `TravelToLair`, `TravelToWorld`). The world, the streamer, and the terrain map are new. `BeginPlay` builds a fully fogged map and `CenterExplorationFogOnFocus` opens a fresh bubble at the landing, so a cleared point far from that landing goes back to `1`.
- `FSolidTravelSession` carries the company, the sight, and where to stand. It does not carry fog. The game instance keeps that session across the trip. The trips that reload the map today are the Meagre dungeon, the Imperious tower, and the Iglin tunnels.
- The values to keep are the point fog values after the trail, including `0` and `0.5`.
- Bands, and the rule that fog clears from the pawn trail: [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).
