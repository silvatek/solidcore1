# Hide trees and buildings in full fog

## Story

Fog on a terrain point is `FSolidTerrainPoint.Fog`. Full fog means that value is `1.0f`, the same test as the white ground. The fog bands and curtains are described in [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).

Trees and buildings on a point at full fog are not drawn. They are generated when that point's `Fog` is no longer `1.0f`.

## Acceptance

- A tree on a terrain point whose `FSolidTerrainPoint.Fog` is `1.0f` is not drawn.
- A building on a terrain point whose `Fog` is `1.0f` is not drawn.
- When that point's `Fog` is no longer `1.0f`, its trees and buildings are generated.
- A point whose `Fog` is not `1.0f` can already show its trees and buildings.
