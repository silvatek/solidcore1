# Falcon Sight and Eagle Sight

Status: In Progress

## Story

The player uses raven sight as a tactical view of the squad. The camera stays within 50 m of the captain, and it does not move above ground that still has fog.

The player chooses Falcon Sight. The camera sits higher than raven sight and lower than eagle sight, between 20 m and 200 m above the ground, and within 80% of the inner fog clear distance on the ground. That distance is 25 m, so the cap is 20 m. Falcon Sight keeps the camera aimed at the captain.

The player chooses Eagle Sight to see the whole map. The camera rises until the entire map is in view. On the ground it moves from the captain toward the centre of the world, along the line between them, and it stops at the closest point on that line that is still over ground with no fog.

The player enters Dragongate. Enter Dragongate fires, and from then on Falcon Sight and Eagle Sight are available. Enter Jethan enables Raven Sight, Falcon Sight, and Eagle Sight, and it does not wait for an earlier event. Entering Jethan switches the view to Eagle Sight and sets Fog to 0 on every terrain point. Before either event, the player does not have Falcon Sight or Eagle Sight.

The player presses F9. The view moves from true sight to raven sight, then to Falcon Sight, then to Eagle Sight, and then back to true sight. A mode the player does not have yet is skipped.

The player looks at the sight marker. On Falcon Sight and Eagle Sight the bird sits further left than on raven sight, leaving more room behind it. Falcon Sight shows a + in that space. Eagle Sight shows a larger globe there.

## Acceptance

- In raven sight the camera stays within 50 m of the captain, and it does not move above a terrain point whose `FSolidTerrainPoint.Fog` is greater than `0.0f`.
- In Falcon Sight the camera is between 20 m and 200 m above the ground, and its horizontal distance from the captain is at most 80% of the inner fog clear distance. The camera keeps aiming at the captain.
- In Eagle Sight the camera is high enough that the entire map is visible.
- In Eagle Sight the camera's ground position lies on the line from the captain to the centre of the world, as close to that centre as possible while the terrain under the camera has `Fog` of `0.0f`.
- True sight behaves as it does now.
- When every sight mode is available, F9 cycles true sight, raven sight, Falcon Sight, Eagle Sight, and then true sight again.
- F9 skips a mode that is not available and uses the next available mode in that order.
- Falcon Sight and Eagle Sight become available when Enter Dragongate or Enter Jethan fires, and not before either. Enter Jethan also enables Raven Sight. Entering Jethan switches the view to Eagle Sight and sets Fog to 0 on every terrain point.
- Falcon Sight shows the raven eagle shifted left, with a + in the space behind the bird.
- Eagle Sight shows the raven eagle shifted left, with a larger globe, Unicode U+1F310, in the space behind the bird.

## Implementation notes

- Add the sights in `Source/SolidCore1/Player/SolidSight.h`. `ESolidSight` is `Raven` and `True` today. `Toggle` swaps those two, and F9 calls that. F9 should cycle the available modes instead of swapping a pair.
- Raven zoom is the spring-arm length, `CameraZoomMin` 250 cm to `CameraZoomMax` 5000 cm (50 m). That length runs along the look. Falcon's horizontal cap is 80% of `SolidTerrainFog::ClearRadiusMeters`. The wheel still picks Falcon's height. The inner fog curtain is taller than that height.
- The world is a 1024 m square. Its centre is the middle of that square.
- Fog on a terrain point is `FSolidTerrainPoint.Fog`. The bands and the rule that the camera stays over clear ground are in [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).
- The raven marker is the eagle returned by `SolidSight::Marker` for raven sight, drawn in the translucent oval at the top right. True sight keeps the eye. The bird faces left, so the empty part of the oval is behind it.
- On Falcon Sight and Eagle Sight the bird is shifted left so the mark behind it can be larger. The globe is U+1F310 (globe with meridians). [falcon-eagle-markers.png](falcon-eagle-markers.png) shows the current raven indicator beside those two.
- Camera height, including Falcon's 20 m to 200 m, is measured from the ground under the camera.
- To start with, both sights are enabled by `SolidEvents::IdEnterDragongate` (`e6a4f093`), the Enter Dragongate event. That event already enables raven sight. `SolidEvents::IdEnterJethan` (`7c3a9e14`) enables Raven Sight, then Falcon Sight, then Eagle Sight, so the view switches to Eagle. Its ClearAllFog change sets every terrain point's Fog to 0.
- Eagle Sight draws every terrain chunk at the coarsest mesh, four quads on a side.
- Eagle Sight looks straight down with north at the top. North is world +Y. East is to the right.
- Eagle Sight frames the world square in the real window. The square fills the shorter side. On a wider window the camera stands at the centre, so the south edge meets the bottom and the north edge meets the top.
