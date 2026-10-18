# Imperious dungeon

Status: Ready

## Story

The player enters an Imperious Dungeon. It is the largest dungeon, a closed visit of the same kind as the Meagre cave, and it is not the Imperious Tower.

The captain goes in alone. Companions wait on the grass outside the mouth he used. A click on the dark opening enters. A click inside leaves, and he stands on the grass in front of that same mouth, looking out.

The floors use the same 1.2 m hex grid as the cave. The Meagre den stays the fourteen-hex cave. A click on its mouth still opens it.

## Acceptance

- An Imperious Dungeon is a dungeon visit. The tower door, the tower interior, and the tower's return to its own door are unchanged.
- The captain enters alone. Companions are not inside. Leaving stands him on the grass outside the mouth he entered, looking out over the grass.
- Leaving the Imperious Dungeon does not move him to the Meagre cave mouth. Leaving the Meagre cave does not move him to the Imperious Dungeon.
- From the open world, a click on the Meagre mouth still loads that den. The menu has no cave row.
- The interior uses the shared 1.2 m pointy-top grid.
- The Meagre den remains fourteen hexes on one floor, with its mouth, its closed-until-the-message rule, and its fireplace, bed, and chest.
- The journal lines stay "Enter Stronghold" and "Exit Stronghold".

## Implementation notes

- The floor plan is not chosen. Do not copy the tower: ten floors, a 30 m taper, round rooms, or the stair spiral. A dungeon is not a tower.
- Do not add a world-map `!` in this story. The owner places the marker. The line is `Stronghold (Imperious Dungeon)`, size then type.
- There is no shelf row for an Imperious Dungeon. Meagre Dungeon is Mountain. Imperious Tower is Grassland. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland until a shelf is chosen.
- `SolidDungeonLevel` is the dungeon trip, and the comment there says every dungeon size uses it. `ASolidDungeonInterior` still builds only the Meagre floor. `?SolidLevel=Dungeon` is that floor. The menu has no cave row.
- [Stronghold type classes](stronghold-type-classes.md) leaves dungeon `Supported` true for Meagre only, with the fourteen-hex ground floor. This story is what makes `Supported` true for Imperious and supplies that floor through the dungeon subclass.
- `UsesCaveMouth` is true for every dungeon. `FindCaveMouth` returns the first dungeon in map order, and leave-cave stands the captain there. A second dungeon mouth would spawn a cliff mouth and can take that return if its cell sorts first. The visit has to remember which mouth, the way the tower remembers its cell.
- The tower trip is `?SolidLevel=Tower`. Its door click does not call the cave travel. This dungeon must not either. The Meagre den is the click on its own mouth.
- Furnishings are catalogued for the Meagre Dungeon only. This story does not place the cave's fireplace, bed, and chest on the larger floors, and it does not decide what an Imperious Dungeon contains.
- Humble, Average, Impressive, Grand, and Stately dungeons are not this story. Only the Meagre den is built today.
