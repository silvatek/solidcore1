# Fortress stronghold

Status: Ready

## Story

Do not start this story until [Stronghold type classes](stronghold-type-classes.md) is Done.

The player can visit a fortress. A fortress is an outer wall with one or more buildings inside it. A building wall may form part of the outer wall where the two meet.

Meagre is not a fortress size. Every fortress has a Keep. The Keep has a larger footprint than any other building in that fortress, and it has more floors than any other building there.

Humble and Average are the Keep alone. A larger fortress adds buildings in this order. Impressive adds Barracks. Grand adds a Gatehouse. Stately and Imperious add Workshops. Barracks have one floor, except in Stately and Imperious, where they have more than one. A Gatehouse has at least two floors, and it may have few hexes on a floor. Each Workshop has one floor. A fortress has no Hall, no Chapel, and no Kitchen.

An Imperious fortress has one Workshop and a Stores. Stores has one floor. Every Imperious building stands against the outer wall, and its doorway faces the yard. The Imperious Keep's ground plan is square. Its Gatehouse has no usable hex on the ground floor and four on the floor above.

A building may have more than one floor. A higher floor uses the same hexes as the floor below. The Imperious Gatehouse ground floor does not: it has no usable hex. The floors do not step inward the way a tower's floors do.

The outer wall and the building walls are straight. A usable hex lies entirely inside them. A building with more than one floor has a stair and a landing. The landing holds no usable hex, and neither does the stair.

From Average upward, the hexes on every floor of every building, added together, match the hexes on every floor of the tower of that same size. Humble sits halfway between the Meagre dungeon and the Average tower.

A size has one hex total. Dungeon, Fortress, and Tower share that total wherever the size is supported. The player does not pick a type because it holds more hexes. From Average through Imperious the tower sets that total. Its rooms are one shape, and its floors step in a constant progression, so its hex count is the constrained one. A dungeon and a fortress change the number and size of their rooms and buildings to meet it.

The captain enters and leaves through a gate in the outer wall. A fortress without a Gatehouse still has that gate. Where there is a Gatehouse, it stands on the gate. A building inside has doorways and no doors on them.

## Acceptance

- Meagre Fortress is not supported. Fortress `Supported` stays false for Meagre.
- Fortress `Supported` is true for Humble, Average, Impressive, Grand, Stately, and Imperious.
- Humble has one building. That building is the Keep. Average has one building. That building is the Keep.
- The Keep has more usable hexes on a floor than any other building in that fortress.
- The Keep has more floors than any other building in that fortress.
- Impressive has the Keep and Barracks. Its Barracks have one floor.
- Grand has the Keep, Barracks, and a Gatehouse. Its Barracks have one floor.
- Stately has the Keep, Barracks, a Gatehouse, and one or more Workshops. Its Barracks have more than one floor.
- Imperious has the Keep, Barracks, a Gatehouse, one Workshop, and a Stores. Its Barracks have more than one floor. Stores has one floor.
- Every Imperious building stands against the outer wall. Its doorway faces the yard.
- The Imperious Keep's ground plan is square.
- The Imperious Gatehouse has no usable hex on the ground floor and four on the floor above.
- A Gatehouse has at least two floors.
- A Workshop has one floor.
- A fortress has no Hall, no Chapel, and no Kitchen.
- A building may have more than one floor.
- A higher floor of a building has the same hexes as the floor below it. The Imperious Gatehouse is the exception: its ground floor has no usable hex.
- The outer wall and the building walls are straight.
- A usable hex lies entirely inside the walls.
- A building with more than one floor has a stair and a landing on each of those floors.
- The landing holds no usable hex. The stair holds no usable hex.
- For Average, Impressive, Grand, Stately, and Imperious, the hexes on all floors of all buildings equal the kept hexes on all floors of the tower of that size.
- Humble's hex total is halfway between the Meagre dungeon hex count and the Average tower's kept-hex total.
- Where more than one type supports a size, those types have the same hex total.
- The stronghold entrance and the stronghold exit are one gate in the outer wall. The captain uses that gate to enter and to leave.
- A doorway in a building inside the wall has no door. That doorway is not a second way onto the grass.
- A building wall may form part of the outer wall.
- The cave mouth, the cave interior, the tower shell, the tower door, and the tower interior stay as they are.
- This story adds no world-map `!` line.

## Implementation notes

- [Stronghold type classes](stronghold-type-classes.md) leaves fortress `Supported` false for every size, with no cliff mouth, no visit, and no ground floor. This story makes `Supported` true from Humble through Imperious and supplies the fortress through that subclass. Meagre stays false. Meagre Fortress is not a final pair.
- The hex total for a size is the sum of `SolidTowerStronghold::CollectKeptHexes` on every floor of the plan `PlanFor` returns for the tower of that size. Average, Impressive, Grand, Stately, and Imperious are those plans. Do not copy the counts into the fortress.
- Humble Tower is not a pair, and `PlanFor` returns false for it. Humble's hex total is the mean of the Meagre dungeon cell count and the Average tower kept-hex total. The Meagre count is `SolidStrongholdGrid::MeagreCells`. The Average total is the sum of `CollectKeptHexes` over every floor of `SolidTowerStronghold::Average`. Do not copy either count into the fortress. A mean that is not a whole number rounds the way `LerpCount` rounds a halfway count.
- A size has one hex total. Dungeon, Fortress, and Tower use it wherever that type supports the size. A type that does not support the size stays out of the comparison. Meagre's total is the dungeon cell count. Humble's total is the mean above, and a Humble dungeon that later becomes supported uses that same total. From Average through Imperious the total is the tower's kept hexes, and a dungeon of one of those sizes uses that same total. [Imperious dungeon](imperious-dungeon.md) uses the Imperious total.
- The tower is the constrained count from Average through Imperious. Every room is the round plan in `SolidTowerStronghold`, and the floors step from Average to Imperious by `SizeAlong`. Leave that plan as it is. A fortress meets the total by the number and size of its buildings, and by repeating a building's ground hexes on each of its floors. A dungeon meets the total by the number and size of its rooms. Humble has no tower, so its total stays the mean above.
- Buildings are added in this order: Barracks, then a Gatehouse, then Workshops. Humble and Average are the Keep alone. Impressive adds Barracks, one floor. Grand adds a Gatehouse. Stately adds one or more Workshops, and its Barracks have more than one floor. Imperious has one Workshop and a Stores, and its Barracks have more than one floor. Stores has one floor. Every Imperious building stands against the outer wall, and its doorway faces the yard. The Imperious Keep's ground plan is square. Its Gatehouse ground floor is the gate and has no usable hex. The floor above has four. The Keep has more floors than every other building, and more usable hexes on a floor. A Gatehouse has at least two floors. A Workshop has one floor. There is no Hall, no Chapel, and no Kitchen. How many floors the Keep has, how many floors a Gatehouse has past two, and how many floors Stately and Imperious Barracks have past one, are not chosen beyond that. Each floor of a building repeats that building's ground hexes, except the Imperious Gatehouse ground floor. Do not shrink an upper floor the way the tower's radius steps inward.
- A fortress without a Gatehouse still has the gate in the outer wall. The Gatehouse is the building on that gate, not the opening itself.
- The walls are straight. They do not follow the hex edges. A usable hex is one whose six vertices lie inside the inner face. The yard plan, the gate's size, and which building walls join the outer wall are not chosen. The mockup elevations use the tower's `FloorPitchCm` for a storey and add a parapet above it. This story does not choose that height.
- The stair width is the tower's `StairWidthCm`. The landing run is the tower's `LandingRunCm`. A hex whose center lies on the stair or the landing is not usable, the same exclusion `HexOnStair` and `HexOnLanding` already apply in a tower. The stair and the landing repeat on every floor of that building, so the usable hexes stay the same. A building with one floor has no stair and no landing.
- The ground inside the outer wall, outside the buildings, is not part of the hex total.
- The gate is the only stronghold entrance and exit. It faces the grass beside the cell, the same way the cave mouth and the tower door face the grass. The visit remembers that cell, the way a tower visit remembers its cell.
- The type-picker already lists a type when that subclass returns `Supported` for the site size, the size is unlocked, and the size is larger than the active stronghold. This story does not change that rule. After it, a qualifying site lists Fortress for Humble through Imperious.
- Do not add a world-map `!`. The owner places the marker. A fortress line is `Stronghold`, size then type, as in `Stronghold (Average Fortress)`. A site stays `StrongholdSite` and a size, with no type. The Imperious site already on the map is `! 16, 36 = StrongholdSite (Imperious)`.
- There is no shelf row for a fortress. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland.
- The fortress does not use the cliff mouth and does not use the tower shell.
- Furnishings stay the Meagre Dungeon catalog. This story does not place the fireplace, the bed, or the chest, and it does not decide what a fortress contains.
- Floors use the shared 1.2 m pointy-top grid.
