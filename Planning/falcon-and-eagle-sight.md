# Falcon Sight and Eagle Sight

## Story

The player uses raven sight as a tactical view of the party. The camera stays within 50 m of the captain, and it does not move above ground that still has fog.

The player chooses Falcon Sight. The camera sits higher than raven sight and lower than eagle sight, between 20 m and 200 m high, and within 50 m of the captain both east-west and north-south.

The player chooses Eagle Sight to see the whole map. The camera rises until the entire map is in view. On the ground it moves from the captain toward the centre of the world, along the line between them, and it stops at the closest point on that line that is still over ground with no fog.

The player presses F9. The view moves from true sight to raven sight, then to Falcon Sight, then to Eagle Sight, and then back to true sight. A mode the player does not have yet is skipped.

The player looks at the sight marker. The bird faces left, and the oval is empty behind it. Falcon Sight shows that same eagle, with a + in the space behind the bird. Eagle Sight shows that same eagle, with a small globe in the space behind the bird.

## Acceptance

- In raven sight the camera stays within 50 m of the captain, and it does not move above a terrain point whose `FSolidTerrainPoint.Fog` is greater than `0.0f`.
- In Falcon Sight the camera is between 20 m and 200 m high, and within 50 m of the captain on both the X axis and the Y axis.
- In Eagle Sight the camera is high enough that the entire map is visible.
- In Eagle Sight the camera's ground position lies on the line from the captain to the centre of the world, as close to that centre as possible while the terrain under the camera has `Fog` of `0.0f`.
- True sight behaves as it does now.
- When every sight mode is available, F9 cycles true sight, raven sight, Falcon Sight, Eagle Sight, and then true sight again.
- F9 skips a mode that is not available and uses the next available mode in that order.
- Falcon Sight shows the raven sight marker with a + in the space behind the bird.
- Eagle Sight shows the raven sight marker with a small globe, Unicode U+1F310, in the space behind the bird.

## Implementation notes

- Add the sights in `Source/SolidCore1/Player/SolidSight.h`. `ESolidSight` is `Raven` and `True` today. `Toggle` swaps those two, and F9 calls that. F9 should cycle the available modes instead of swapping a pair.
- Raven zoom is the spring-arm length, `CameraZoomMin` 250 cm to `CameraZoomMax` 5000 cm (50 m). That length runs along the look. Falcon's 50 m limit is on X and on Y separately, not that arm length.
- The world is a 1024 m square. Its centre is the middle of that square.
- Fog on a terrain point is `FSolidTerrainPoint.Fog`. The bands and the rule that the camera stays over clear ground are in [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).
- The raven marker is the eagle returned by `SolidSight::Marker` for raven sight, drawn in the translucent oval at the top right. True sight keeps the eye. The bird faces left, so the empty part of the oval is behind it.
- Falcon's + and Eagle's globe sit in that space. The globe is a small U+1F310 (globe with meridians). [falcon-eagle-markers.png](falcon-eagle-markers.png) shows both on the current raven indicator.
- Not decided: when Falcon Sight and Eagle Sight unlock, and whether Falcon's height is measured above the ground under the camera.
