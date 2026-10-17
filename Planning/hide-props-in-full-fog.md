# Hide trees and buildings in full fog

Status: Ready

## Story

The player looks toward a forest or a town that is still unseen. Trees and buildings on ground in full fog are not there. When the player comes close enough that the ground is no longer in full fog, those trees and buildings appear.

## Acceptance

- A tree on a terrain point whose `FSolidTerrainPoint.Fog` is `1.0f` is not drawn.
- A building on a terrain point whose `Fog` is `1.0f` is not drawn.
- When that point's `Fog` is no longer `1.0f`, its trees and buildings are generated.
- A point whose `Fog` is not `1.0f` can already show its trees and buildings.

## Implementation notes

- Full fog is `FSolidTerrainPoint.Fog` of `1.0f`, the same test as the white ground. Any other value is not full fog.
- Fog bands and curtains: [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).
