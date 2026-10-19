# Dungeon exterior

Status: New

## Story

The Meagre Dungeon sits on grassland, the same shelf as the Imperious Tower. Its exterior is a large rock with a cave entrance, standing on that cell. A click on the entrance enters the cave.

Dungeon, Fortress, and Tower each draw their own exterior and each provide one clickable entrance. That entrance is the transition into the interior. The dungeon no longer uses a cliff mouth cut into a mountain cell.

## Acceptance

- The Meagre Dungeon shelf is Grassland. The Imperious Tower shelf stays Grassland.
- The world-map line stays `! 9, 17 = Stronghold (MeagreDungeon)`. That cell samples as grassland. The `!` stays where it is.
- The north neighbor of that cell stays grassland. The entrance still faces north.
- The dungeon exterior is a rock on the stronghold cell, with one dark cave entrance. The entrance is the clickable opening.
- A click on that opening enters the cave once the cave is open. Leaving stands on the grass outside it, looking out. The cave still starts closed until the monster message is closed.
- Companions still wait outside. The interior, the fourteen hexes, the furnishings, and the journal lines stay.
- The Iglin tunnel mouths stay the cliff frame on their own mountain cells. They are not this rock, and their cells stay mountain.
- The tower still draws its shell. A click on its door still enters. It does not draw the rock.
- This story does not build a fortress. When a fortress has an exterior, that mesh is the fortress exterior, and its gate is the clickable entrance.
- Each stronghold type draws its exterior mesh and names one clickable entrance for the transition to its interior. The dungeon does not use the cliff mouth.

## Implementation notes

- The rock's proportions are not chosen. That is why this story is still New.
- `SolidStronghold::Shelves` lists the starting Meagre Dungeon as Mountain. This story changes that row to Grassland. The marker on the map stays `!`.
- `UsesCliffMouth` is true for the dungeon and false for the tower and the fortress. This story makes it false for the dungeon. The rock is placed on the cell and turned toward the grass neighbor, the way the tower shell is placed at the cell center and turned toward its door. It does not use `FaceInsetCm` to sit a frame on the far side of a mountain edge.
- The rock is built in code, the way the tower shell is built. It is not a Fab mesh. The dark opening stays solid. A click enters. Walking up to the rock stops on the stone.
- Grassland is walkable, so the cell itself is no longer closed ground. The rock is what occupies it.
- The Iglin tunnel mouths still spawn `ASolidDungeonEntrance` from their lair lines. They do not follow the dungeon type's exterior.
- Tests that require the cave shelf to be Mountain change in their own commit before the product change, reading the shelf from `SolidStronghold` rather than a copied biome. They are `SolidCore1.Stronghold.ShippedEntrance` ("cave cell is Mountain"), `SolidCore1.Stronghold.FaceChoice` ("a stronghold is Mountain"), the road test "the cave cell keeps the mountain shelf", and the ground-move test "cave cell uses the mountain". The `SolidCore1.Stronghold.*` line and the world-map paragraph in `README.md` say Meagre Dungeon is Mountain. Those sentences change in the product commit.
- Lair tests that say a tunnel mouth cell is mountain stay. Those cells are the tunnels.
- [Fortress stronghold](fortress-stronghold.md) says the cave mouth stays as it is. This story replaces that cliff mouth. The fortress gate still faces the grass neighbor, the same facing the tower door uses.
- [Imperious dungeon](imperious-dungeon.md) says Meagre Dungeon is Mountain. That sentence changes when this story ships.
