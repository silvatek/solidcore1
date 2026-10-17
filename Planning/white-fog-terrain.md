# White material for full fog

## Story

Ground color is chosen from the biome. Grassland uses the grass material. Every other biome uses a solid color from the world map. This story adds one exception. A terrain point at full fog, the setting for ground not yet seen, uses a plain white material instead of that biome color.

The full-fog wall should already hide that ground, so the white should not be visible in play. It is a fallback for anything that gets past the wall.

Clear ground and half fog keep the biome material. The fog curtains stay as they are.

## Acceptance

- A terrain point with full fog uses a plain white material.
- A terrain point with clear fog or half fog keeps the material for its biome.
- The full-fog wall is unchanged.
- When a point drops below full fog, it uses its biome material again.
