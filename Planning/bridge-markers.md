# Bridge markers

Status: Ready

## Story

A bridge marker is defined in C++ while this story is implemented. The definition has a length, an orientation, north-south or east-west, and an initial state, broken or intact. The default length is 18 m. That is the Iglin bridge on level banks, ramps included. The World chart stores only the marker's position. That position is the centre of the bridge span. The developer places it by hand. The bridge stands at that marker. A road that crosses a bridge is split in two, from the origin to the bridge and from the bridge to the destination. That split is a one-off design-time task, done while implementing this story. One end of the bridge is closer to the origin, and the other end is closer to the destination. Each route finishes at the end closer to where it starts. When that is not so, the road fails to route. A road that has to cross a river where no bridge is placed fails to route. This story introduces no game concept. Bridge is already `FSolidBridge`. The marker is a mark on the chart.

## Acceptance

- The bridges are defined in C++ while this story is implemented. Each definition has a length, an orientation, north-south or east-west, and an initial state, broken or intact.
- The default length is 18 m, the Iglin bridge on level banks, ramps included.
- The World chart stores the marker's position. It does not store the length, the orientation, or the initial state.
- That position is the centre of the bridge span. The developer can place it on the World chart.
- The bridge in play stands at that marker, at that length and orientation, and in that initial state.
- A road that crosses a bridge is two routes: from the origin to the bridge, and from the bridge to the destination. That split is done once, at design time, while implementing this story.
- One bridge end is closer to the origin and the other is closer to the destination. Each route finishes at the end closer to where it starts. When that is not so, the road fails to route.
- A road that has to cross a river where no bridge is placed fails to route.

## Implementation notes

The shipped roads are the C++ list `SolidRoad::Roads`. Splitting a road that crosses a bridge is a one-off design-time task. It is done in that list while implementing this story. The game does not split roads. Which bridges exist, and each one's orientation and initial state, are chosen in the C++ definitions while implementing this story.

The 18 m default is the level-bank Iglin crossing. The river channel is 4 m. The centre rests 1 m on each bank, so the span is 6 m. On level ground each ramp runs 6 m: 3.5 m down to the ground at the 50 cm sample, then 2.5 m buried. `FSolidBridge` is the mesh that stands at the marker. Length, orientation, and the broken or intact state are not fields on that mesh today.
