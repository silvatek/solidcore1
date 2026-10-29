# Smooth biome boundaries

Status: In Progress

## Story

The player walks along the join where two biomes meet. Where those biomes share a shelf, a hard edge cuts the stair on the cell diagonal. The single odd corner is cut off. A straight run stays a straight edge.

This patch is the case. Each letter is one terrain point, 2 m from the next:

```
AAAA
AABB
BBBB
```

A higher shelf still owns its quad. A road keeps a hard edge. This story introduces no new class.

## Acceptance

- On that patch, a quad with three corners of one flat colour and one corner of the other, on the same shelf, is split on the diagonal that cuts off the single corner. That triangle is the single corner's colour. The other triangle is the other colour.
- A straight run, two corners of one flat colour sharing a side and two of the other, keeps the straight grid edge. It is not cut on a diagonal.
- A quad whose corners are a single biome keeps that biome's material.
- A quad whose corners use three biomes keeps the single material it uses now.
- A checker, two opposite corners of one flat colour and the other two of another, keeps the single material it uses now.
- A quad that includes a road point keeps the single material it uses now. A trail, a track, a road, and a highway keep the width they have now.
- A quad whose corners differ in shelf keeps the higher shelf across the whole quad. Mountain against grassland shows that step. A river bank against land shows that step. Forest against sea shows that step. Sea against river is cut when the quad is three of one and one of the other, because those two share a shelf.
- A quad that includes grassland keeps the single material it uses now.
- Two jogs that share an edge may leave a one-quad notch.
- The chart points are unchanged. A road's route, a town region, and the trees still read the point biome.
- In Play In Editor, forest against desert on this stair is cut on the diagonal toward the odd corner. Forest against sea stays the shelf step.

## Implementation notes

Forest, town, desert, swamp, sea, and river are the flat colours. Grassland is not cut.

The quad is already two triangles. The cut chooses which way the diagonal runs, and which triangle keeps which colour. No new points.

Today both triangles take one material from `BorderBiome`. The cut replaces that choice on the quads the acceptance names. Those quads stop using the fade material.

A road point is baked by `SampleBiome` where the strip shares the ground's shelf. A trail is 1 m wide. The cut covers a 2 m quad, so a road corner stays on the hard material.

A test builds this patch from two flat colours of one shelf. The quad with a single odd corner is cut off that corner. A straight pair of rows is not cut. A checker, a road corner, a higher shelf, and three biomes are not cut.
