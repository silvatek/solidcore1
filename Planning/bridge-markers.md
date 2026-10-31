# Bridge markers

Status: New

## Story

A bridge marker is defined in C++. That definition has a length and an orientation, north-south or east-west. The World chart stores only the marker's position. The developer places that position by hand. The bridge stands at that marker. The road strip follows the terrain points of the route. The bridge still finds its place by locking the crossing to a cell centre and shifting half a quad toward -X and -Y. That left the deck beside the strip in the SC1-0855 playshot. That calculation is the complexity this story replaces. A road definition that has to cross a river is split in two: from the origin to the bridge, and from the bridge to the destination. Each route finds the closest bridge end. This story introduces no game concept. Bridge is already `FSolidBridge`. The marker is a mark on the chart.

## Acceptance

- A bridge marker's length and orientation, north-south or east-west, are C++ data.
- The World chart stores the marker's position. It does not store the length or the orientation.
- The developer can place that position on the World chart.
- The bridge in play stands at that marker.
- The bridge's place is not the cell centre of the crossing, and it is not that centre shifted by half a quad.
- A road definition that has to cross a river is two routes: from the origin to the bridge, and from the bridge to the destination.
- Each of those routes finishes at the closest bridge end.

## Implementation notes

`SolidBridge::Collect` builds a crossing from the terrain points of a routed road. `Straighten` then locks the deck and both ramps to the cell centre under the unshifted middle, plus `SolidRoad::StripPaintOffset`. `SC1-0855` made `IsOnRoad` measure those terrain points, so the strip moved onto the line the bridge had been aiming at, and the lock left the deck beside it. `Saved/Playshots/SC1-0855_20261031-172321.png` is that shot.

Following the point line and keeping only the half-quad shift was the other way to line them up. This story uses the marker instead.

The shipped roads are the C++ list `SolidRoad::Roads`. A road in that list that has to cross a river becomes the two routes above. `FSolidBridge` is the mesh that stands at the marker. Its length and orientation are not fields on that mesh today.
