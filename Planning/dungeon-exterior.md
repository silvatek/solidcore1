# Dungeon exterior

Status: In Progress

## Story

The Meagre Dungeon sits on grassland, the same shelf as the Imperious Tower. Its exterior is a rock with a cave entrance, standing on that cell. The front face is about the size of the cave entrance that ships now. From the top of that face the rock slopes back down to the ground, over about twice its height. The rock and the entrance look rounded and lumpy, like a natural stone, rather than a shape with straight sides. A click on the entrance enters the cave.

Dungeon, Fortress, and Tower each draw their own exterior and each provide one clickable entrance. That entrance is the transition into the interior. The dungeon no longer uses a cliff mouth cut into a mountain cell.

## Acceptance

- The Meagre Dungeon shelf is Grassland. The Imperious Tower shelf stays Grassland.
- The world-map line stays `! 9, 17 = Stronghold (MeagreDungeon)`. That cell samples as grassland. The `!` stays where it is.
- The north neighbor of that cell stays grassland. The entrance still faces north.
- The dungeon exterior is a rock on the stronghold cell, with one dark cave entrance. The entrance is the clickable opening.
- The front face is the outer size of the current cave frame. Its width is `OpeningWidthCm` plus `FrameThicknessCm` on each side. Its height is `OpeningHeightCm` plus `FrameThicknessCm` on top. That height is the rock's maximum height.
- The dark opening is `OpeningWidthCm` wide and `OpeningHeightCm` tall. It sits on the ground, centered in the face. The stone on each side of it, and the stone above it, is `FrameThicknessCm` thick.
- Behind the front face, the top of the rock falls to the ground. The horizontal distance from the front face to where that slope meets the ground is twice the face height.
- The rock and the entrance have a somewhat rounded, lumpy, natural look. The sides are not straight. The mouth is not a sharp rectangle. The 380 cm by 420 cm opening and the stone around it are the overall size. The rim is the irregular part. How rounded and lumpy the surface is, inside that size, is chosen when the mesh is built.
- A click on that opening enters the cave once the cave is open. Leaving stands on the grass outside it, looking out. The cave still starts closed until the monster message is closed.
- Companions still wait outside. The interior, the fourteen hexes, the furnishings, and the journal lines stay.
- The Iglin tunnel mouths stay the cliff frame on their own mountain cells. They are not this rock, and their cells stay mountain.
- The tower still draws its shell. A click on its door still enters. It does not draw the rock.
- This story does not build a fortress. When a fortress has an exterior, that mesh is the fortress exterior, and its gate is the clickable entrance.
- Each stronghold type draws its exterior mesh and names one clickable entrance for the transition to its interior. The dungeon does not use the cliff mouth.

## Implementation notes

- The front face matches the frame `ASolidDungeonEntrance` builds. `OpeningWidthCm` is 380, `OpeningHeightCm` is 420, and `FrameThicknessCm` is 80, so the face is 540 cm wide and 500 cm tall. The slope's horizontal run is twice that height, 1000 cm. Read those three constants. Do not copy 540, 500, or 1000 into a second definition. The old frame depth, `OpeningDepthCm`, is not the depth of this rock.
- Those widths and the run are the overall size, not a box. The surface is somewhat rounded and lumpy, a natural stone, and so is the rim of the entrance. The amount of that lumpiness, inside the size above, is chosen with the mesh. Do not build the straight wedge, the flat front rectangle, or the triangular sides. The dark opening is still a solid face on the front, in the mouth color, and the rest of the rock uses the rock color. A click on the opening enters. A click on the stone does not.
- The run is longer than one world-map cell. The slope may cross the edge behind the front. Do not shorten it to stay inside the cell.
- `SolidStronghold::Shelves` lists the starting Meagre Dungeon as Mountain. This story changes that row to Grassland. The marker on the map stays `!`.
- `UsesCliffMouth` is true for the dungeon and false for the tower and the fortress. This story makes it false for the dungeon. The rock is placed on the cell and turned toward the grass neighbor, the way the tower shell is placed at the cell center and turned toward its door. It does not use `FaceInsetCm` to sit a frame on the far side of a mountain edge.
- Keep the new exterior on `FSolidDungeon`. Put a behavior on `FSolidStronghold` only when every stronghold type shares it.
- The rock is built in code, the way the tower shell is built. It is not a Fab mesh. The dark opening stays solid. A click enters. Walking up to the rock stops on the stone.
- Grassland is walkable, so the cell itself is no longer closed ground. The rock is what occupies it.
- The Iglin tunnel mouths still spawn `ASolidDungeonEntrance` from their lair lines. They do not follow the dungeon type's exterior.
- Tests that require the cave shelf to be Mountain change in their own commit before the product change, reading the shelf from `SolidStronghold` rather than a copied biome. They are `SolidCore1.Stronghold.ShippedEntrance` ("cave cell is Mountain"), `SolidCore1.Stronghold.FaceChoice` ("a stronghold is Mountain"), the road test "the cave cell keeps the mountain shelf", and the ground-move test "cave cell uses the mountain". The `SolidCore1.Stronghold.*` line and the world-map paragraph in `README.md` say Meagre Dungeon is Mountain. Those sentences change in the product commit.
- A test of the dungeon entrance may do nothing when that entrance is not on a mountain shelf. The check still runs while the shelf is Mountain, and it does not run after this story moves the shelf to Grassland.
- Lair tests that say a tunnel mouth cell is mountain stay. Those cells are the tunnels.
- [Fortress stronghold](fortress-stronghold.md) says the cave mouth stays as it is. This story replaces that cliff mouth. The fortress gate still faces the grass neighbor, the same facing the tower door uses.
- [Imperious dungeon](imperious-dungeon.md) says Meagre Dungeon is Mountain. That sentence changes when this story ships.
