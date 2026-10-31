# Bridge markers

Status: New

## Story

A bridge marker is defined in C++. That definition has a length and an orientation, north-south or east-west. The World chart stores only the marker's position. The developer places that position by hand. The bridge stands at that marker. A road definition that has to cross a river is split in two: from the origin to the bridge, and from the bridge to the destination. Each route finds the closest bridge end. This story introduces no game concept. Bridge is already `FSolidBridge`. The marker is a mark on the chart.

## Acceptance

- A bridge marker's length and orientation, north-south or east-west, are C++ data.
- The World chart stores the marker's position. It does not store the length or the orientation.
- The developer can place that position on the World chart.
- The bridge in play stands at that marker.
- A road definition that has to cross a river is two routes: from the origin to the bridge, and from the bridge to the destination.
- Each of those routes finishes at the closest bridge end.

## Implementation notes

The shipped roads are the C++ list `SolidRoad::Roads`. A road in that list that has to cross a river becomes the two routes above. `FSolidBridge` is the mesh that stands at the marker. Its length and orientation are not fields on that mesh today.
