# Fortress stronghold

Status: Ready

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player can visit a fortress. A fortress is one rectangular outer wall with a yard inside it and one or more buildings in that yard. The wall is a closed ring of straight sides. A building's outer face may be part of that wall. Where a building does not fill a side, the wall continues as a curtain. The yard is the open ground inside the wall and outside the buildings. It is not part of the hex total. You go from one building to another across the yard.

Meagre is not a fortress size. Every fortress has a Keep. The Keep is the main building. It has more usable hexes on a floor than any other building in that fortress when those other buildings exist, and it has more floors than any other building there when it has more than one floor.

Humble and Average are the Keep alone, aside from any leftover small rooms of the shared kit that a mockup still places (annex or yard building). A larger fortress adds buildings in this order. Impressive adds Barracks. Grand adds a Gatehouse. Stately adds Workshops. Imperious has one Workshop and a Stores. Barracks have one floor, except in Stately and Imperious. Imperious Barracks have two floors. Stately Barracks have more than one floor. A Gatehouse has at least two floors, and it may have few hexes on a floor. Each Workshop has one floor. Stores has one floor. A fortress has no Hall, no Chapel, and no Kitchen.

A building may have more than one floor. A higher floor uses the same walls and the same hexes as the floor below. The floors do not step inward the way a tower's floors do. The Imperious Gatehouse ground floor is the exception. It is the gate passage and has no usable hex.

The outer wall and the building walls are straight. They do not follow the hex edges. A usable hex lies entirely inside the building: all six vertices are inside the inner face. A building with more than one floor has a stair and a landing inside it. You reach a higher floor only by that stair. The landing holds no usable hex, and neither does the stair. A building with one floor has no stair and no landing. On a floor that repeats the one below, the stair and the landing occupy the same places, so the usable hexes match.

A size has one room kit. Dungeon, Fortress, and Tower share that kit wherever the size is supported. Large rooms are 20 hexes, medium 12, small 6. The player does not pick a type because it holds more hexes. The kit for each size is in `Source/SolidCore1/Strongholds/README.md`. Tower geometry (a band of Imperious floors) constrains which rooms exist; the fortress uses the same rooms, with the Keep taking the floors listed below.

Keep floors (kit rooms stacked in the Keep):

| Size | Keep | Remainder of the kit |
|------|------|----------------------|
| Humble | 1× medium | 1× small |
| Average | 1× medium | 2× small |
| Impressive | 2× medium | 1× medium, 2× small |
| Grand | 1× large, 2× medium | 2× medium, 2× small |
| Stately | 2× large, 1× medium | 3× medium, 2× small |
| Imperious | 3× large | 4× medium, 3× small |

The remainder goes to the other buildings (Barracks, Gatehouse, Workshops, Stores), or, for Humble and Average, into the Keep compound as a mockup decides.

The captain enters and leaves through one gate in the outer wall. The gate is on the grass side of the cell, the same way the cave mouth and the tower door face the grass. The visit starts in the yard, inside the gate. The gate opening is one storey high, so a floor above it can stand over the passage. A fortress without a Gatehouse still has that gate, cut through the curtain. Where there is a Gatehouse, the building stands on the gate. Its ground floor is the passage: the outer opening is the gate, the inner opening faces the yard, and the stair sits beside the passage. A floor above that passage is a room. A building doorway has no door. It faces the yard. It is not a second way onto the grass.

The outer wall is one storey high, with a parapet. A building of more than one floor stands above the wall.

Humble, Impressive, and Imperious had earlier drawn plans with different per-building hex counts. Those counts are superseded by the room kit and the Keep table. New fortress mockups follow the kit; do not ship the old 13 / 18 / 27-style splits.

Average, Grand, and Stately use the roster above, the Keep table, and the shared kit. Their yard plans past that are not chosen until a mockup.

## Acceptance

- Meagre Fortress is not supported. Fortress `Supported` stays false for Meagre.
- Fortress `Supported` is true for Humble, Average, Impressive, Grand, Stately, and Imperious.
- The outer wall is one rectangle. It is a closed ring. The yard is the open ground inside it and outside the buildings.
- The yard is not part of the hex total.
- A building's outer face may form part of the outer wall. A stretch of wall with no building on it is curtain.
- The Keep is the main building. Where another building exists, the Keep has more usable hexes on a floor than that building, and more floors when the Keep has more than one floor.
- Keep floors match the Keep table in the story (Humble and Average: one medium floor each; Impressive: two medium; Grand: one large and two medium; Stately: two large and one medium; Imperious: three large).
- Usable hexes on all floors of all buildings equal the shared room kit for that size (Humble 18, Average 24, Impressive 48, Grand 80, Stately 100, Imperious 126).
- Humble has the Keep. Average has the Keep. Impressive has the Keep and Barracks. Grand has the Keep, Barracks, and a Gatehouse. Stately has the Keep, Barracks, a Gatehouse, and Workshops. Imperious has the Keep, Barracks, a Gatehouse, one Workshop, and a Stores.
- Imperious Barracks have two floors. Stately Barracks have more than one floor. Other Barracks have one floor.
- A Gatehouse has at least two floors.
- A Workshop has one floor. Stores has one floor.
- A fortress has no Hall, no Chapel, and no Kitchen.
- A building may have more than one floor.
- A higher floor of a building has the same walls and the same hexes as the floor below it. An Imperious Gatehouse ground floor that is only a gate passage is the exception and holds no usable hex.
- The outer wall and the building walls are straight.
- A usable hex has all six vertices inside the inner face.
- A building with more than one floor has a stair and a landing inside it. A higher floor is reached only by that stair.
- The landing holds no usable hex. The stair holds no usable hex.
- A building with one floor has no stair and no landing.
- On a repeated floor, the stair and the landing occupy the same places.
- Where more than one type supports a size, those types have the same hex total (the room kit).
- The stronghold entrance and the stronghold exit are one gate in the outer wall, on the grass side of the cell. The captain uses that gate to enter and to leave.
- The visit starts in the yard, inside the gate.
- The gate opening is one storey high.
- A fortress without a Gatehouse has the gate in the curtain.
- A Gatehouse stands on the gate. Its ground-floor openings face the grass and the yard. A floor above that passage is a room.
- A doorway in a building has no door. It faces the yard. That doorway is not a second way onto the grass.
- The outer wall is one storey high and has a parapet. A building of more than one floor stands above the wall.
- The cave mouth, the cave interior, the tower exterior, the tower door, and the tower interior stay as they are.
- This story adds no world-map `!` line.

## Implementation notes

- [Stronghold type classes](Done/stronghold-type-classes.md) leaves fortress `Supported` false for every size, with no visit and no ground floor. This story makes `Supported` true from Humble through Imperious and supplies the fortress through that subclass. Meagre stays false. Meagre Fortress is not a final pair.
- Hex totals are the shared room kits in `Source/SolidCore1/Strongholds/README.md`, not the sum of `CollectKeptHexes` on the tower plan. A test sums fortress usable hexes and compares them with the kit for that size. Do not paste tower per-floor kept counts into the fortress.
- A size has one kit. Dungeon, Fortress, and Tower use it wherever that type supports the size. [Imperious dungeon](imperious-dungeon.md) uses the Imperious kit (126). Tower sizes Average through Imperious use Imperious floors 7–9, 5–9, 3–9, 2–9, and 1–10 respectively, one kit room per floor.
- The Keep floors in the story are required. Split the remainder across the other buildings in a mockup before coding; do not invent a split in a later pass unless the owner draws one.
- Older mockups (Humble 13+13, Impressive 18×3+9, Imperious 27×4 + 14×2 + 15 + 10 + 4) are obsolete. Redraw against the kit before implementing.
- Buildings are added in this order: Barracks, then a Gatehouse, then Workshops. Imperious then has a Stores in place of a second Workshop. There is no Hall, no Chapel, and no Kitchen. Each floor of a building repeats that building's ground hexes, except a Gatehouse ground floor that is only a passage. Do not shrink an upper floor the way the tower's radius steps inward.
- A fortress without a Gatehouse still has the gate in the curtain. The Gatehouse is the building on that gate. Its ground floor is the passage, open to the grass and to the yard, and the stair is inside the building beside that passage.
- The walls are straight. They do not follow the hex edges. A usable hex is one whose six vertices lie inside the inner face. A hex whose center lies on the stair or the landing is not usable, the same exclusion `HexOnStair` and `HexOnLanding` already apply in a tower. The stair width is the tower's `StairWidthCm`. The landing run is the tower's `LandingRunCm`. The wall thickness in the mockup is the tower's `WallCm`.
- The mockup elevations use the tower's `FloorPitchCm` for a storey and add a parapet above the top floor and above the curtain. The outer wall is one storey plus that parapet, and the gate opening is one storey. This story does not choose a different pitch or a parapet height. The gate's width is not chosen past an opening a person can walk through.
- The ground inside the outer wall, outside the buildings, is the yard. It is not part of the hex total. The visit starts there, inside the gate, and leaving returns through that gate to the grass beside the cell. The visit remembers that cell, the way a tower visit remembers its cell.
- The type-picker already lists a type when that subclass returns `Supported` for the site size, the size is unlocked, and the size is larger than the active stronghold. This story does not change that rule. After it, a qualifying site lists Fortress for Humble through Imperious.
- Do not add a world-map `!`. The owner places the marker. A fortress line is `Stronghold`, size then type, as in `Stronghold (Average Fortress)`. A site stays `StrongholdSite` and a size, with no type. The Imperious site already on the map is `! 16, 36 = StrongholdSite (Imperious)`.
- There is no shelf row for a fortress. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland.
- The fortress does not use the tower exterior.
- Furnishings stay the Meagre Dungeon catalog. This story does not place the fireplace, the bed, or the chest, and it does not decide what a fortress contains.
- Floors use the shared 1.2 m pointy-top grid.
- The product commit adds tests for the roster, the Keep floors, the kit totals, the stair and landing exclusion, and a Gatehouse passage floor with no usable hex when that plan is drawn. The tests compare against the kit, not the tower kept-hex sum.
