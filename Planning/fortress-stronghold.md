# Fortress stronghold

Status: Ready

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player can visit a fortress. A fortress is one rectangular outer wall with a yard inside it and one or more buildings in that yard. The wall is a closed ring of straight sides. A building's outer face may be part of that wall. Where a building does not fill a side, the wall continues as a curtain. The yard is the open ground inside the wall and outside the buildings. It is not part of the hex total. You go from one building to another across the yard.

Meagre is not a fortress size. Every fortress has a Keep. The Keep has more usable hexes on a floor than any other building in that fortress, and it has more floors than any other building there.

Humble and Average are the Keep alone. A larger fortress adds buildings in this order. Impressive adds Barracks. Grand adds a Gatehouse. Stately adds Workshops. Imperious has one Workshop and a Stores. Barracks have one floor, except in Stately and Imperious. Imperious Barracks have two floors. Stately Barracks have more than one floor. A Gatehouse has at least two floors, and it may have few hexes on a floor. Each Workshop has one floor. Stores has one floor. A fortress has no Hall, no Chapel, and no Kitchen.

A building may have more than one floor. A higher floor uses the same walls and the same hexes as the floor below. The floors do not step inward the way a tower's floors do. The Imperious Gatehouse ground floor is the exception. It is the gate passage and has no usable hex.

The outer wall and the building walls are straight. They do not follow the hex edges. A usable hex lies entirely inside the building: all six vertices are inside the inner face. A building with more than one floor has a stair and a landing inside it. You reach a higher floor only by that stair. The landing holds no usable hex, and neither does the stair. A building with one floor has no stair and no landing. On a floor that repeats the one below, the stair and the landing occupy the same places, so the usable hexes match.

From Average upward, the hexes on every floor of every building, added together, match the hexes on every floor of the tower of that same size. Humble sits halfway between the Meagre dungeon and the Average tower.

A size has one hex total. Dungeon, Fortress, and Tower share that total wherever the size is supported. The player does not pick a type because it holds more hexes. From Average through Imperious the tower sets that total. Its rooms are one shape, and its floors step in a constant progression, so its hex count is the constrained one. A dungeon and a fortress change the number and size of their rooms and buildings to meet it.

The captain enters and leaves through one gate in the outer wall. The gate is on the grass side of the cell, the same way the cave mouth and the tower door face the grass. The visit starts in the yard, inside the gate. The gate opening is one storey high, so a floor above it can stand over the passage. A fortress without a Gatehouse still has that gate, cut through the curtain. Where there is a Gatehouse, the building stands on the gate. Its ground floor is the passage: the outer opening is the gate, the inner opening faces the yard, and the stair sits beside the passage. A floor above that passage is a room. A building doorway has no door. It faces the yard. It is not a second way onto the grass.

The outer wall is one storey high, with a parapet. A building of more than one floor stands above the wall.

Humble, Impressive, and Imperious are the drawn plans.

Humble is the Keep alone. The Keep has two floors and 13 usable hexes on each floor. It stands on the outer wall on the three sides away from the gate. A yard lies between the Keep and the gate, and the Keep's doorway faces that yard.

Impressive has the Keep and Barracks. The Keep has three floors and 18 usable hexes on each floor. The Barracks have one floor and 9 usable hexes. Both stand on the outer wall. The Barracks stand at one end of the grass-side wall and the Keep at the other. The Keep is the deeper building, so it also reaches the wall opposite the gate. The gate is in the curtain between them. Each doorway faces the yard.

Imperious has the Keep, Barracks, one Workshop, a Stores, and a Gatehouse. The Keep has four floors and 27 usable hexes on each floor, and its inner walls form a square. It stands on the wall opposite the gate. The Barracks have two floors and 14 usable hexes on each floor, and they stand on one side wall. The Workshop has one floor and 15 usable hexes. The Stores has one floor and 10 usable hexes. Both stand on the other side wall, the Workshop nearer the Keep and the Stores nearer the gate. The Gatehouse has two floors. It stands on the grass-side wall, and the gate passes through it. Its ground floor has no usable hex. The floor above has four, and that floor holds the landing. Every doorway faces the yard.

Average, Grand, and Stately are not drawn. They use the roster above, the Keep rules, and the hex total for that size. Their floor counts, past the roster, and their yard plans are not chosen.

## Acceptance

- Meagre Fortress is not supported. Fortress `Supported` stays false for Meagre.
- Fortress `Supported` is true for Humble, Average, Impressive, Grand, Stately, and Imperious.
- The outer wall is one rectangle. It is a closed ring. The yard is the open ground inside it and outside the buildings.
- The yard is not part of the hex total.
- A building's outer face may form part of the outer wall. A stretch of wall with no building on it is curtain.
- The Keep has more usable hexes on a floor than any other building in that fortress.
- The Keep has more floors than any other building in that fortress.
- Humble has one building. That building is the Keep. It has two floors and 13 usable hexes on each floor. It stands on the three sides away from the gate. The yard and the gate lie on the grass side, and the doorway faces the yard.
- Average has one building. That building is the Keep. Average is not drawn. Its floor count is not chosen past the Keep rules and the hex total.
- Impressive has the Keep and Barracks. The Keep has three floors and 18 usable hexes on each floor. The Barracks have one floor and 9 usable hexes. Both stand on the outer wall, at opposite ends of the grass-side wall, with the gate in the curtain between them. Each doorway faces the yard.
- Grand has the Keep, Barracks, and a Gatehouse. Its Barracks have one floor. Grand is not drawn. Its floor counts, past that roster, and its yard plan are not chosen.
- Stately has the Keep, Barracks, a Gatehouse, and one or more Workshops. Its Barracks have more than one floor. Stately is not drawn. Its floor counts, past that roster, and its yard plan are not chosen.
- Imperious has the Keep, Barracks, a Gatehouse, one Workshop, and a Stores.
- The Imperious Keep has four floors and 27 usable hexes on each floor. Its inner walls form a square. It stands on the wall opposite the gate.
- The Imperious Barracks have two floors and 14 usable hexes on each floor. They stand on one side wall.
- The Imperious Workshop has one floor and 15 usable hexes. The Stores has one floor and 10 usable hexes. Both stand on the other side wall. The Workshop is nearer the Keep. The Stores is nearer the gate.
- The Imperious Gatehouse has two floors. It stands on the grass-side wall and the gate passes through it. The ground floor has no usable hex. The floor above has four.
- Every Imperious building stands against the outer wall. Its doorway faces the yard.
- A Gatehouse has at least two floors.
- A Workshop has one floor. Stores has one floor.
- A fortress has no Hall, no Chapel, and no Kitchen.
- A building may have more than one floor.
- A higher floor of a building has the same walls and the same hexes as the floor below it. The Imperious Gatehouse ground floor is the exception.
- The outer wall and the building walls are straight.
- A usable hex has all six vertices inside the inner face.
- A building with more than one floor has a stair and a landing inside it. A higher floor is reached only by that stair.
- The landing holds no usable hex. The stair holds no usable hex.
- A building with one floor has no stair and no landing.
- On a repeated floor, the stair and the landing occupy the same places.
- The Imperious Gatehouse ground floor is the passage, with the stair beside it. The landing is on the floor above, with the four hexes.
- For Average, Impressive, Grand, Stately, and Imperious, the hexes on all floors of all buildings equal the kept hexes on all floors of the tower of that size.
- Humble's hex total is halfway between the Meagre dungeon hex count and the Average tower's kept-hex total.
- Where more than one type supports a size, those types have the same hex total.
- The stronghold entrance and the stronghold exit are one gate in the outer wall, on the grass side of the cell. The captain uses that gate to enter and to leave.
- The visit starts in the yard, inside the gate.
- The gate opening is one storey high.
- A fortress without a Gatehouse has the gate in the curtain.
- A Gatehouse stands on the gate. Its ground-floor openings face the grass and the yard. A floor above that passage is a room.
- A doorway in a building has no door. It faces the yard. That doorway is not a second way onto the grass.
- The outer wall is one storey high and has a parapet. A building of more than one floor stands above the wall.
- The cave mouth, the cave interior, the tower shell, the tower door, and the tower interior stay as they are.
- This story adds no world-map `!` line.

## Implementation notes

- [Stronghold type classes](Done/stronghold-type-classes.md) leaves fortress `Supported` false for every size, with no cliff mouth, no visit, and no ground floor. This story makes `Supported` true from Humble through Imperious and supplies the fortress through that subclass. Meagre stays false. Meagre Fortress is not a final pair.
- The hex total for a size is the sum of `SolidTowerStronghold::CollectKeptHexes` on every floor of the plan `PlanFor` returns for the tower of that size. Average, Impressive, Grand, Stately, and Imperious are those plans. Do not copy the counts into the fortress. A test sums the fortress plan and compares it with that total.
- Humble Tower is not a pair, and `PlanFor` returns false for it. Humble's hex total is the mean of the Meagre dungeon cell count and the Average tower kept-hex total. The Meagre count is `SolidStrongholdGrid::MeagreCells`. The Average total is the sum of `CollectKeptHexes` over every floor of `SolidTowerStronghold::Average`. Do not copy either count into the fortress. A mean that is not a whole number rounds the way `LerpCount` rounds a halfway count. The drawn Humble Keep, 13 hexes on each of two floors, is that mean.
- A size has one hex total. Dungeon, Fortress, and Tower use it wherever that type supports the size. A type that does not support the size stays out of the comparison. Meagre's total is the dungeon cell count. Humble's total is the mean above, and a Humble dungeon that later becomes supported uses that same total. From Average through Imperious the total is the tower's kept hexes, and a dungeon of one of those sizes uses that same total. [Imperious dungeon](imperious-dungeon.md) uses the Imperious total.
- The tower is the constrained count from Average through Imperious. Every room is the round plan in `SolidTowerStronghold`, and the floors step from Average to Imperious by `SizeAlong`. Leave that plan as it is. A fortress meets the total by the number and size of its buildings, and by repeating a building's ground hexes on each of its floors. A dungeon meets the total by the number and size of its rooms. Humble has no tower, so its total stays the mean above.
- The drawn plans are the hex counts in the story. Humble is 13 + 13. Impressive is 18 + 18 + 18 + 9. Imperious is 27 four times, 14 twice, 15, 10, and 4. Those are the fortress numbers. The mockup also found straight interiors on the shared grid that produce them, with the stair and the landing excluded:
  - Humble Keep, 540 by 620 cm, flight 340 cm.
  - Impressive Barracks, 420 by 420 cm, no stair. Impressive Keep, 520 by 680 cm, flight 320 cm.
  - Imperious Keep, 700 by 700 cm, flight 320 cm. Barracks, 460 by 680 cm, flight 280 cm. Workshop, 420 by 580 cm. Stores, 480 by 480 cm. Gatehouse, 420 by 480 cm, flight 240 cm, and the ground floor contributes no hex.
  - The grid phase matters. A box of that size counts only when every kept hex has all six vertices inside the inner face. Use a phase that yields the count in the story.
- Average, Grand, and Stately have no mockup. Do not invent a yard plan or a per-building hex split for them in a later pass unless the owner draws one. The roster, the Keep rules, and the hex total are the constraints. Stately's Barracks have more than one floor, and the count past two is not chosen. A Grand or Stately Gatehouse has at least two floors. The Imperious passage, with no hex on the ground floor and four above, is the drawn Gatehouse. It is not a requirement that Grand and Stately copy that split.
- Buildings are added in this order: Barracks, then a Gatehouse, then Workshops. Imperious then has a Stores in place of a second Workshop. There is no Hall, no Chapel, and no Kitchen. Each floor of a building repeats that building's ground hexes, except the Imperious Gatehouse ground floor. Do not shrink an upper floor the way the tower's radius steps inward.
- A fortress without a Gatehouse still has the gate in the curtain. The Gatehouse is the building on that gate. Its ground floor is the passage, open to the grass and to the yard, and the stair is inside the building beside that passage. The landing for the Imperious Gatehouse is on the floor above.
- The walls are straight. They do not follow the hex edges. A usable hex is one whose six vertices lie inside the inner face. A hex whose center lies on the stair or the landing is not usable, the same exclusion `HexOnStair` and `HexOnLanding` already apply in a tower. The stair width is the tower's `StairWidthCm`. The landing run is the tower's `LandingRunCm`. The wall thickness in the mockup is the tower's `WallCm`.
- The mockup elevations use the tower's `FloorPitchCm` for a storey and add a parapet above the top floor and above the curtain. The outer wall is one storey plus that parapet, and the gate opening is one storey. This story does not choose a different pitch or a parapet height. The gate's width is not chosen past an opening a person can walk through.
- The ground inside the outer wall, outside the buildings, is the yard. It is not part of the hex total. The visit starts there, inside the gate, and leaving returns through that gate to the grass beside the cell. The visit remembers that cell, the way a tower visit remembers its cell.
- The type-picker already lists a type when that subclass returns `Supported` for the site size, the size is unlocked, and the size is larger than the active stronghold. This story does not change that rule. After it, a qualifying site lists Fortress for Humble through Imperious.
- Do not add a world-map `!`. The owner places the marker. A fortress line is `Stronghold`, size then type, as in `Stronghold (Average Fortress)`. A site stays `StrongholdSite` and a size, with no type. The Imperious site already on the map is `! 16, 36 = StrongholdSite (Imperious)`.
- There is no shelf row for a fortress. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland.
- The fortress does not use the cliff mouth and does not use the tower shell.
- Furnishings stay the Meagre Dungeon catalog. This story does not place the fireplace, the bed, or the chest, and it does not decide what a fortress contains.
- Floors use the shared 1.2 m pointy-top grid.
- The product commit adds tests for the roster, the drawn hex counts, the Keep rules, the stair and landing exclusion, the Imperious Gatehouse ground floor, and the hex total against the tower sum or the Humble mean. The tests do not paste the tower's per-floor counts.
