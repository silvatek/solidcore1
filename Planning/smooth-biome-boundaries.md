# Smooth biome boundaries

Status: In Progress

## Story

The player walks along the join where two biomes meet. Where those biomes share a shelf, the ground fades from one surface to the other across the 2 m quad. On a diagonal the fade runs toward the odd corner.

This patch is the case. Each letter is one terrain point, 2 m from the next:

```
AAAA
AABB
BBBB
```

A straight run fades across the line between the two rows, and that line stays the middle of the fade. A higher shelf still owns its quad. A road keeps a hard edge. This story introduces no new class.

## Acceptance

- On that patch, two biomes that share a shelf fade across each mixed quad. Each corner is fully its own biome, so the odd corner stays solid and the fade runs toward it.
- A straight run of one biome against another, on the same shelf, fades across the quads between those rows. The line halfway between the rows is the middle of the fade.
- A quad whose corners are a single biome keeps that biome's material.
- A quad whose corners use three biomes keeps the single material it uses now.
- A quad that includes a road point keeps the single material it uses now. A trail, a track, a road, and a highway keep the width they have now.
- A quad whose corners differ in shelf keeps the higher shelf across the whole quad. Mountain against grassland shows that step. A river bank against land shows that step. Sea against river fades, because those two share a shelf.
- Grassland's side of a fade is the grass material: Fab `Mat_025_grass` when it is present, and the FlatCol grass colour when it is not. Forest, town, desert, swamp, sea, and river fade as their own colours.
- The chart points are unchanged. A road's route, a town region, and the trees still read the point biome.
- In Play In Editor, forest against desert on this stair fades toward the odd corner. Grassland against forest fades the same way, with the grass material on the grassland side. Grassland against mountain shows the shelf.

## Implementation notes

The flat-colour fade ships first. Grassland joins that fade in a later change. The two do not land together.

The first change fades a quad whose corners are two flat colours on one shelf. Forest, town, desert, swamp, sea, and river are those colours. Grassland stays the hard material it has now, including where it meets one of those colours. Each corner of a fading quad is that corner's colour, and the material shows the colour interpolated across the quad. That interpolation is the fade. The material is not compiled at runtime.

The later change puts grassland on the same corners. The grass side uses the material `SolidTerrainMaterials::Resolve` already picks. It does not compile a material graph at runtime.

Today both triangles of a quad take one material from `BorderBiome`. When the shelves match, that material is the far corner's biome, so the quad whose far corner is the single B is painted entirely B. The fade replaces that choice on the quads the acceptance names.

The vertex colour already stored on the mesh is a grass tone. The fade's weight is separate from that tone.

A road point is baked by `SampleBiome` where the strip shares the ground's shelf. A trail is 1 m wide. The fade covers a 2 m quad, so a road corner stays on the hard material.

A test builds this patch from two flat colours of one shelf and reads the corner weights. The odd corner is fully its biome. The other three corners of that quad are fully the other. A straight pair of rows is fully one biome on one row and fully the other on the next. A road corner and a higher shelf produce no fade weight.
