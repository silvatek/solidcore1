# White material for full fog

## Story

The player looks at ground the party has not reached. Fog on a terrain point is `FSolidTerrainPoint.Fog`, and full fog means that value is `1.0f`. The fog bands and curtains are described in [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).

On a point at full fog, the player would see plain white ground instead of the biome color. Grassland is still the grass material, and every other biome is still its world-map color, wherever `Fog` is not `1.0f`. The full-fog wall already hides the unseen ground, so the player should not see the white. It is there if something gets past the wall. The curtains do not change. When the player comes close enough that the point's `Fog` is no longer `1.0f`, that ground shows its biome color again.

## Acceptance

- A terrain point whose `FSolidTerrainPoint.Fog` is `1.0f` uses a plain white material.
- A terrain point whose `Fog` is not `1.0f` keeps the material for its biome.
- The full-fog wall is unchanged.
- When a point's `Fog` is no longer `1.0f`, that point uses its biome material again.
