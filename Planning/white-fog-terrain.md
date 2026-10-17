# White material for full fog

## Story

Ground color is chosen from the biome. Grassland uses the grass material. Every other biome uses a solid color from the world map. Fog on a terrain point is `FSolidTerrainPoint.Fog`. Full fog means that value is `1.0f`.

A point at full fog uses a plain white material instead of the biome color. Any other fog value keeps the biome material.

The full-fog wall should already hide that ground, so the white should not be visible in play. It is a fallback for anything that gets past the wall. The fog curtains stay as they are.

## Acceptance

- A terrain point whose `FSolidTerrainPoint.Fog` is `1.0f` uses a plain white material.
- A terrain point whose `Fog` is not `1.0f` keeps the material for its biome.
- The full-fog wall is unchanged.
- When a point's `Fog` is no longer `1.0f`, that point uses its biome material again.
