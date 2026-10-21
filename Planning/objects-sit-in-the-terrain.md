# Objects sit in the terrain

Status: In Progress

## Story

A tree, a building, a sign, a forage pile, a stronghold exterior, or a stronghold site sits on the open-world terrain. The bottom of that model runs about 10 cm below the lowest ground under it, enough that no gap shows and the edge does not flicker. Bridges already do this. The captain and the companions stay on the surface.

## Acceptance

- A non-moving object that sits on open-world terrain extends 10 cm below the lowest ground under its footprint. The ground and the bottom of the model meet. A gap does not show, and the edge does not flicker.
- This includes a tree, a town building, a town sign, a forage pile, a dungeon exterior, a tower exterior, and a stronghold site.
- The same rule covers a later object of that kind. A lair mouth is one of them.
- The part of the object the player is meant to see above the ground stays where it is. The extension is the buried part.
- A bridge ramp still runs into the ground the way it does now. This story does not change `BurialCm`.
- The captain and a companion are moving. They are not buried. Their feet stay on the surface.
- The cave interior, the tower interior, and the Iglin tunnels are not this terrain. Furnishings inside them are unchanged.

## Implementation notes

- A tree, a building, a sign, a pile, an exterior, and a site are spawned at one sample: `GetHeightAt` at that XY, plus `CollisionHeightBias` and `SnapHeightPadding`. On a slope the mesh bottom is flat at that sample, so the downhill side floats. The buried part has to cover the terrain under the whole footprint, not only the sample point.
- A bridge ramp keeps going after it meets the ground. `SolidBridge::BurialCm` is 250 cm past that meeting. `SolidCore1.Bridge.*` already checks that the ramp runs into the ground. Leave that path as it is.
- The buried part ends 10 cm below the lowest terrain sample under that object's footprint. That depth is enough to close the gap and stop the flicker. It is not the bridge's `BurialCm`.
