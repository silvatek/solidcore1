# Imperious dungeon

Status: Ready

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player enters an Imperious Dungeon. It is the largest dungeon, a closed visit of the same kind as the Meagre cave, and it is not the Imperious Tower.

The captain goes in alone. Companions wait on the grass outside the mouth he used. A click on the dark opening enters. A click inside leaves, and he stands on the grass in front of that same mouth, looking out.

The floors use the same 1.2 m hex grid as the cave. The Meagre den stays the fourteen-hex cave. A click on its mouth still opens it.

Rooms and hex totals follow **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. The Imperious row is three large rooms (20 hexes), four medium (12), and three small (6), **126 hexes** in all. Dungeon, Fortress, and Tower at Imperious share that kit. [Tower stronghold by size](tower-stronghold-by-size.md) puts those rooms on Imperious floors 1–10. [Fortress stronghold](fortress-stronghold.md) puts three large rooms in the Keep and the rest in other buildings.

A dungeon joins the kit with passages and stairs. Layout is flexible. Level 1 is the top (the mouth); deeper levels have higher numbers — the opposite of tower and fortress floor numbers. Passages hold no usable hexes. Shipped interiors are stone; mockup colours are plan ink only. The arrangement of the ten rooms is chosen in the mockup before coding.

## Acceptance

- An Imperious Dungeon is a dungeon visit. Until [Tower stronghold by size](tower-stronghold-by-size.md) ships, the tower door, the tower interior, and the tower's return to its own door are unchanged by this story.
- The captain enters alone. Companions are not inside. Leaving stands him on the grass outside the mouth he entered, looking out over the grass.
- Leaving the Imperious Dungeon does not move him to the Meagre cave mouth. Leaving the Meagre cave does not move him to the Imperious Dungeon.
- From the open world, a click on the Meagre mouth still loads that den. The menu has no cave row.
- The interior uses the shared 1.2 m pointy-top grid.
- The Meagre den remains fourteen hexes on one floor, with its mouth, its closed-until-the-message rule, and its fireplace, bed, and chest.
- The Imperious Dungeon matches the Imperious kit in Strongholds by size and type: three large (20), four medium (12), and three small (6). Usable hexes total 126.
- Passages and stairs hold no usable hexes. Level 1 is the mouth level; level numbers increase as the captain goes deeper.
- The journal lines stay "Enter Stronghold" and "Exit Stronghold".

## Implementation notes

- The plan of record is **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Imperious is 3×20 + 4×12 + 3×6. Do not invent a second kit. The player does not pick a type because it holds more hexes.
- Do not copy the tower: ten floors in one shaft, a 30 m taper, round rooms, or the stair spiral. A dungeon is not a tower. Join the kit with passages and stairs; wing the arrangement.
- Do not add a world-map `!` in this story. The owner places the marker. The line is `Stronghold (Imperious Dungeon)`, size then type.
- There is no shelf row for an Imperious Dungeon. Meagre Dungeon is Grassland. Imperious Tower is Grassland. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland until a shelf is chosen.
- `SolidDungeonLevel` is the dungeon trip, and the comment there says every dungeon size uses it. `ASolidDungeonInterior` still builds only the Meagre floor. `?SolidLevel=Dungeon` is that floor. The menu has no cave row.
- [Stronghold type classes](Done/stronghold-type-classes.md) leaves dungeon `Supported` true for Meagre only, with the fourteen-hex ground floor. This story is what makes `Supported` true for Imperious and supplies that floor through the dungeon subclass.
- Leave stands the captain in front of that dungeon's rock, from `LeaveStandXY`. There is no cliff mouth. A second dungeon has to remember its own exterior, the way the tower remembers its cell.
- The tower trip is `?SolidLevel=Tower`. Its door click does not call the cave travel. This dungeon must not either. The Meagre den is the click on its own mouth.
- Furnishings are catalogued for the Meagre Dungeon only. This story does not place the cave's fireplace, bed, and chest on the larger floors, and it does not decide what an Imperious Dungeon contains.
- Humble, Average, Impressive, Grand, and Stately dungeons are not this story. Only the Meagre den is built today. Their kits are in Strongholds by size and type when a later story needs them.
