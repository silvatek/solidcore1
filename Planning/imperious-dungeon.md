# Imperious dungeon

Status: Ready

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player enters a dungeon of any size from Meagre through Imperious. Imperious is the largest. Each size is a closed visit of the same kind as the Meagre cave.

The captain goes in alone. Companions wait on the grass outside the mouth he used. A click on the dark opening enters. A click inside leaves, and he stands on the grass in front of that same mouth, looking out.

The floors use the same 1.2 m hex grid as the cave. Meagre is one irregular medium room, twelve hexes, on the level of the mouth. A click on its mouth still opens it.

Rooms and hex totals follow **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. The Imperious row is three large rooms (20 hexes), four medium (12), and three small (6), **126 hexes** in all. Dungeon, Fortress, and Tower at Imperious share that kit. [Tower stronghold by size](Done/tower-stronghold-by-size.md) puts those rooms on Imperious floors 1–10. [Fortress stronghold](fortress-stronghold.md) puts three large rooms in the Keep and the rest in other buildings.

A dungeon size is an `AddLevels` list, in the same shape as `FSolidFortress::AddBuildings`. The helpers own the measurements. A size names the pieces and where each one sits. `AddLarge`, `AddMedium`, and `AddSmall` are the rectangular rooms of 20, 12, and 6 hexes. `AddMediumIrregular` is one shared irregular outline of 12 hexes. Meagre is that room. A larger size calls it when that size wants the same room. `AddMedium` and `AddMediumIrregular` each count as one medium room in the kit. `AddPassage` joins pieces on one level and holds no usable hexes. `AddStair` drops one level and holds no usable hexes. Level 1 is the mouth. Deeper levels have higher numbers, the opposite of tower and fortress floor numbers. Shipped interiors are stone; mockup colours are plan ink only.

Meagre is `AddMediumIrregular(AtMouth())`. The piece number in a later call counts rooms, passages, and stairs from that start. `EastOf`, `NorthOf`, `WestOf`, and `SouthOf` stay on the same level. The mouth is local west, and east runs into the dungeon. The visit turns that mouth toward the grass. `Below` places the next piece on the far side of that stair, one level down, and each `Below` adds one to the level number. A piece has an opening on every face another piece touches. The mouth is the west face of the `AtMouth` piece. A passage is two cells long and one cell wide, and a stair is the same length. A size does not pass those lengths.

Every size from Meagre through Imperious is this story. Each larger size keeps the smaller size's list and adds the next pieces. Those lists are the mockup. They are written here when the plan is agreed.

## Acceptance

- An Imperious Dungeon is a dungeon visit. Until [Tower stronghold by size](Done/tower-stronghold-by-size.md) ships, the tower door, the tower interior, and the tower's return to its own door are unchanged by this story.
- The captain enters alone. Companions are not inside. Leaving stands him on the grass outside the mouth he entered, looking out over the grass, and does not move him to another dungeon's mouth.
- From the open world, a click on the Meagre mouth still loads that cave. The menu has no cave row.
- The interior uses the shared 1.2 m pointy-top grid.
- Meagre is `AddMediumIrregular(AtMouth())`: twelve usable hexes on the mouth level, with its mouth, its closed-until-the-message rule, and its fireplace, bed, and chest.
- A size that calls `AddMediumIrregular` uses that same outline.
- Every size from Meagre through Imperious is a dungeon visit. Usable hexes match that size's kit row: Meagre 12, Humble 18, Average 24, Impressive 48, Grand 80, Stately 100, Imperious 126.
- The Imperious Dungeon matches the Imperious kit in Strongholds by size and type: three large (20), four medium (12), and three small (6). Usable hexes total 126.
- Passages and stairs hold no usable hexes. Level 1 is the mouth level; level numbers increase as the captain goes deeper.
- The journal lines stay "Enter Stronghold" and "Exit Stronghold".

## Implementation notes

- The plan of record is **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Imperious is 3×20 + 4×12 + 3×6. Do not invent a second kit. The player does not pick a type because it holds more hexes.
- How the type, the exterior actor, and the interior actor are written follows **Code structure** in `Source/SolidCore1/Strongholds/README.md`.
- Do not copy the tower: ten floors in one shaft, a 30 m taper, round rooms, or the stair spiral. A dungeon is not a tower. Join the kit with the `AddLevels` list in the Story. The mockup is the proposal. A size whose list is not written there is not built.
- Do not add a world-map `!` in this story. The owner places the marker. The line is `Stronghold (Imperious Dungeon)`, size then type.
- There is no shelf row for an Imperious Dungeon. Meagre Dungeon is Grassland. Imperious Tower is Grassland. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland until a shelf is chosen.
- `SolidDungeonLevel` is the dungeon trip, and the comment there says every dungeon size uses it. `ASolidDungeonInterior` still builds only the Meagre floor. `?SolidLevel=Dungeon` is that floor. The menu has no cave row.
- [Stronghold type classes](Done/stronghold-type-classes.md) leaves dungeon `Supported` true for Meagre only. This story rebuilds that floor as `AddMediumIrregular` and makes `Supported` true from Humble through Imperious, through the dungeon subclass.
- Leave stands the captain in front of that dungeon's rock, from `LeaveStandXY`. There is no cliff mouth. A second dungeon has to remember its own exterior, the way the tower remembers its cell.
- The tower trip is `?SolidLevel=Tower`. Its door click does not call the cave travel. This dungeon must not either. The Meagre cave is the click on its own mouth.
- Furnishings are catalogued for the Meagre Dungeon only. This story does not place the cave's fireplace, bed, and chest on the larger floors, and it does not decide what an Imperious Dungeon contains.
- Meagre through Imperious are this story. Their kits stay the rows in Strongholds by size and type. Each size is built from the `AddLevels` list written for it here, after the mockup is agreed.
