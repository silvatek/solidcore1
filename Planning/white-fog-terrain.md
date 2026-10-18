# White material for full fog

Status: Ready

## Story

The player looks at ground the squad has not reached. On ground still in full fog, that ground is plain white instead of the biome color. Grassland is still the grass material, and every other biome is still its world-map color, wherever the ground is not in full fog. The full-fog wall already hides the unseen ground, so the player should not see the white. It is there if something gets past the wall. The curtains do not change. When the player comes close enough that the ground is no longer in full fog, that ground shows its biome color again.

## Acceptance

- A terrain point whose `FSolidTerrainPoint.Fog` is `1.0f` uses a plain white material.
- A terrain point whose `Fog` is not `1.0f` keeps the material for its biome.
- The full-fog wall is unchanged.
- When a point's `Fog` is no longer `1.0f`, that point uses its biome material again.

## Implementation notes

- Full fog is `FSolidTerrainPoint.Fog` of `1.0f`. Any other value is not full fog.
- Fog bands and curtains: [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).
- Ground color today comes from the biome. Grassland uses the grass material. Other biomes use a solid color from the world map.
