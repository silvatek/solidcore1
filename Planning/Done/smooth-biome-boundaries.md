# Smooth biome boundaries

Status: Done

## What shipped

The original story overspecified the solution and has been ignored. The text below is that story. It does not describe the game.

The mesh uses NorthWestCut. Any square the three cuts do not match takes its northwest corner. A higher shelf still replaces that corner.

Three cuts:

- The other three corners match and differ from the northwest. The diagonal runs from the north-east corner to the south-west. The north-west triangle keeps the northwest corner. The south-east triangle takes the other biome.
- The south corner differs, the east and south-east corners match the northwest, and the point directly west of the northwest matches the south corner. The diagonal runs from the north-west corner to the south-east. The south-west triangle takes the south corner. The north-east triangle takes the northwest corner.
- The point west of the northwest differs, the point east matches it, and the points south-west, south, and south-east match that west point. The diagonal is the same one. The north-east triangle takes the northwest corner. The south-west triangle takes the other biome.

OddCorner, which cuts off whichever corner is the single one, is still in the enum. The mesh does not use it. The cuts add no points. Grassland is cut by the same rules. A road corner is cut when it matches one of them. Painted chart points at Relion were committed separately.

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

## Next step

Play on SC1-0732 showed a straight row of town points drawn as forest. There was no diagonal.

A straight run is two corners of one flat colour sharing a side and two of the other. `DiagonalCut` returns false, so both triangles take `BorderBiome` of the quad centre. The centre is halfway between the points. `SamplePoint` takes the nearest point, and a halfway sample rounds to corner 11, the +X +Y corner. When that corner is forest, the whole quad is forest, including the two town corners. Every quad along that row does the same.

The next slice splits that quad on the line halfway between the two rows. Add two mesh vertices, one on each edge the boundary crosses, and give each half that half's material. Town stays town on its half and forest stays forest on its half. The edge stays straight. Chart points stay unchanged. The straight-run acceptance changes with that slice: the run is split on that halfway line, and it is still not a diagonal.

Leave the 3+1 diagonal, a checker, three biomes, a road, and a shelf step as they are. Grassland stays one material for this slice. It was left out because the old fade had to blend its texture with a flat colour. This join does not blend, so grassland can use the same split in a later slice.
