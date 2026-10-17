# Falcon Sight and Eagle Sight

## Story

The player is in raven sight, looking at the party from above and behind. The mouse wheel moves that camera in or out along the look. A longer arm also sets the camera farther from the captain across the ground.

The player chooses Falcon Sight, or Eagle Sight. Each one stays in third person. The player can raise the camera higher than raven sight allows. The camera does not sit any farther from the captain, across the ground, than it does in raven sight.

## Acceptance

- Falcon Sight is third person.
- Eagle Sight is third person.
- Each one allows a greater maximum camera height than raven sight.
- Neither one allows a greater horizontal distance from the captain than raven sight.
- True sight and raven sight behave as they do now.

## Implementation notes

- Add the sights in `Source/SolidCore1/Player/SolidSight.h`. `ESolidSight` is `Raven` and `True` today. `Toggle` swaps those two. F9 calls that.
- Raven zoom is the spring-arm length, `CameraZoomMin` 250 cm to `CameraZoomMax` 5000 cm. That length runs along the look, so it is not a separate height.
- Not decided: whether Eagle Sight is higher than Falcon Sight, how the player selects each one, when each one unlocks, and what the sight marker shows.
