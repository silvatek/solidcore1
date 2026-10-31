# Bridge markers

Status: New

## Story

The developer places a bridge marker by hand on the World chart. The bridge stands at that marker. The road strip follows the terrain points of the route. The bridge still finds its place by locking the crossing to a cell centre and shifting half a quad toward -X and -Y. That left the deck beside the strip in the SC1-0855 playshot. That calculation is the complexity this story replaces. This story introduces no game concept. Bridge is already `FSolidBridge`. The marker is a mark on the chart.

## Acceptance

- The developer can place a bridge marker on the World chart.
- The bridge in play stands at that marker.
- The bridge's place is not the cell centre of the crossing, and it is not that centre shifted by half a quad.

## Implementation notes

`SolidBridge::Collect` builds a crossing from the terrain points of a routed road. `Straighten` then locks the deck and both ramps to the cell centre under the unshifted middle, plus `SolidRoad::StripPaintOffset`. `SC1-0855` made `IsOnRoad` measure those terrain points, so the strip moved onto the line the bridge had been aiming at, and the lock left the deck beside it. `Saved/Playshots/SC1-0855_20261031-172321.png` is that shot.

Following the point line and keeping only the half-quad shift was the other way to line them up. This story uses the marker instead.
