# Hills

Status: Ready

## Story

The player walks onto hills where the chart paints them. Hills are a biome. The ground uses the same material as grassland, and the shelf is 3 m. The hills roll.

Each biome has a height variance: None, Low, Medium, or High. That variance is how much the height of the terrain points in that biome varies, above the shelf. Mountains are High, so they come to sharp peaks. Hills are Medium. The sea is None, so it is flat. Every other biome is Low, so grassland and towns are nearly level, with a little rise and fall.

This story introduces no new class. Hills are another biome. The height variance is a question `FSolidBiome` answers.

## Acceptance

- Hills is a biome the chart can paint. The shipped chart is unchanged, so hills appear where a later edit paints them.
- `Content/WorldMap.txt` is gone. The chart is where hills are painted. A test that builds a map from text still reads `h` as hills, in either case.
- The hills mesh uses the material grassland uses. That is Fab `Mat_025_grass` when it is present, and the same fallback grassland uses when it is not.
- The hills shelf is 3 m (300 cm). Sea and river stay at 0. Mountain stays at 10 m. Grassland, forest, town, desert, swamp, and road stay at 1 m.
- The chart colour of hills is not grassland's light green. The mesh does not use that chart colour.
- Mountain's height variance is High. Hills are Medium. Sea is None. Grassland, forest, town, desert, swamp, river, and road are Low.
- The shelf is a constant. The variance is the part that differs from point to point inside one biome.
- At the default build amplitude of 30 m, None adds 0. Low spans 1 m. Medium spans 4 m. High spans 30 m. A different amplitude scales Low, Medium, and High by that amplitude over 30 m. None stays 0.
- A sea region is one height. A grassland or town region is not one height, and its variance stays within the Low span. A hills region varies more than that, and stays within the Medium span. A mountain region varies more than a hills region.
- High is ridged, on a shorter wavelength than the Low roll. On a mountain, some point stands more than the Low span above a neighbouring mountain point one terrain step (2 m) away.
- Hills are open ground. Sea, mountain, and the river channel stay closed.
- A road or a highway may cross hills, as it crosses grassland. The strip keeps the hills ground. The hills shelf and the road shelf differ, so the strip does not cut a trench and does not replace the grass.
- A slope still uses the higher shelf. Hills sit above the 1 m land and the water. A mountain beside hills still takes that slope.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

This story is one phase. The variance and the hills biome land together.

- Append `Hills` at the end of `ESolidBiome`. The chart asset stores the enum by value. Inserting it earlier would renumber saved cells.
- `FSolidBiome` answers `HeightVariance()`. The values are None, Low, Medium, and High. Hills are paintable, open, and take the grassland surface preference, 0. The chart letter is `h`. The chart colour is `FLinearColor(0.62f, 0.58f, 0.22f)`.
- The World chart brush list gains Hills. Swamp stays off that list. Road stays a solved strip.
- `ResolveForBiome` sends Hills through the grassland material. The chart colour is only the overlay tint.
- Today every point takes the same noise, from 0 up to the build amplitude, and then the shelf. Low and Medium scale that sample. None drops it. High keeps the full span and ridges it, so a peak is steep across one 2 m step. `ChooseBiome`, used when no world map is loaded, still reads the unscaled sample. A missing map does not invent hills.
- A hill top is the 3 m shelf plus the 4 m Medium span, 7 m, under the mountain shelf of 10 m.
- The river channel is Low, on the 0 shelf, so it has the same small rise and fall as grassland. The sea is the flat water.
- A road strip replaces the ground only when the shelves match. Hills stay hills under a route. Enter cost does not avoid them.
- [Remove WorldMap.txt](remove-world-map-txt.md) has deleted the text file. Hills are painted on `/Game/World/WorldChart`. The letter `h` stays for `LoadFromString` and for `FSolidBiome::Marker()`.
- Trees, sticks, and pitch stay on forest. An encounter mark still stands on grassland. Hills gain none of those. [Rough stones](rough-stones.md) may place a pile on hills when a neighbour is a mountain or a river. Hills are open ground, and that story does not exclude them.
- [Work on the terrain grid](work-on-the-terrain-grid.md) says the other land sits 1 m up, and its palette has no hills. When that story is implemented, those lines keep the hills shelf, the height variance, and Hills on the palette.
- `SolidCore1.Map.BiomeHeightOffset` assumes a point's height is the full-amplitude sample plus the shelf. Update that test in its own commit before the product change. It reads the variance from the biome. `SolidCore1.Types.BiomeHeightOffset` keeps the existing shelves and gains the hills shelf of 300 cm.
- The same product commit updates the `README.md` line that says other biomes use a FlatCol tint. Hills use the grass material. The amplitude there is the High span.
