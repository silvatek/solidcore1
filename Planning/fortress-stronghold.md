# Fortress stronghold

Status: In Progress

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player can visit a fortress. A fortress is one rectangular outer wall with a yard inside it and one or more buildings in that yard. The wall is a closed ring of straight sides. A building's outer face may be part of that wall. Where a building does not fill a side, the wall continues as a curtain. The yard is the open ground inside the wall and outside the buildings. It is not part of the hex total. You go from one building to another across the yard.

Rooms, hex totals, and Keep floors follow **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Large rooms are 20 hexes, medium 12, small 6. Dungeon, Fortress, and Tower at the same size share that kit. Meagre is not a fortress size.

| Size | L | M | S | Hexes | Keep |
|------|--:|--:|--:|------:|------|
| Humble | 0 | 1 | 1 | 18 | S, then M |
| Average | 0 | 1 | 2 | 24 | S, then M |
| Impressive | 0 | 3 | 2 | 48 | 2× M |
| Grand | 1 | 4 | 2 | 80 | 1× L, 2× M |
| Stately | 2 | 4 | 2 | 100 | 2× L, 1× M |
| Imperious | 3 | 4 | 3 | 126 | 3× L |

Every fortress has a Keep as its main building. Keep floors are the kit rooms in the Keep column. The remainder of the kit (kit minus Keep) goes to other buildings. [Tower stronghold by size](Done/tower-stronghold-by-size.md) is the tower side of the same plan. [Imperious dungeon](imperious-dungeon.md) is the Imperious dungeon side.

A fortress always has at least one building with at least two floors. Humble is one building: the small room on the ground, the medium room above it. Average keeps that building and adds a second building with one small floor. A larger fortress adds buildings in this order. Impressive adds Barracks. Grand adds a Gatehouse. Stately adds Workshops. Imperious has one Workshop and a Stores. Barracks have one floor, except in Stately and Imperious. Imperious Barracks have two floors. Stately Barracks have more than one floor. A Gatehouse has at least two floors, and it may have few hexes on a floor. Each Workshop has one floor. Stores has one floor. A fortress has no Hall, no Chapel, and no Kitchen.

Where another building exists, the Keep has more usable hexes on a floor than that building, and more floors when the Keep has more than one floor.

A building may have more than one floor. Every floor of that building has the same outer walls. The floors do not step inward the way a tower's floors do. A higher floor may use the same hexes as the floor below. An interior wall may close off part of a floor, and the closed part holds no usable hex. A medium room can sit above a large room. A small room and a medium room can sit side by side above a large room. A Gatehouse ground floor that is only the gate passage holds no usable hex.

The outer wall and the building walls are straight. They do not follow the hex edges. A usable hex lies entirely inside the building: all six vertices are inside the inner face. A building with more than one floor has a stair and a landing inside it. You reach a higher floor only by that stair. The landing holds no usable hex, and neither does the stair. A building with one floor has no stair and no landing. On a floor that repeats the one below, the stair and the landing occupy the same places, so the usable hexes match.

The captain enters and leaves through one gate in the outer wall. The gate is on the grass side of the cell, the same way the cave mouth and the tower door face the grass. The visit starts in the yard, inside the gate. The gate opening is one storey high, so a floor above it can stand over the passage. A fortress without a Gatehouse still has that gate, cut through the curtain. Where there is a Gatehouse, the building stands on the gate. Its ground floor is the passage: the outer opening is the gate, the inner opening faces the yard, and the stair sits beside the passage. A floor above that passage is a room. A building doorway has no door. It faces the yard. It is not a second way onto the grass.

The outer wall is one storey high, 300 cm. The gate fills that storey. A parapet of 120 cm sits above a building's top floor. A building of more than one floor stands above the wall.

Humble, Impressive, and Imperious had earlier drawn plans with different per-building hex counts. Those counts are superseded by Strongholds by size and type. New fortress mockups follow that table; do not ship the old 13 / 18 / 27-style splits. The two-building Humble row and the Average L are retired. The two-floor face is drawn. The stair and the upper floor are not drawn yet. The larger sizes stay undrawn until a mockup.

### Humble

The plan's gate side is north. In the game that side faces the grass, and the plan turns with it.

Humble is one building. Its inner face is 540 cm wide and 486.4 cm deep. The south block is the medium span, 540 cm by 346.4 cm. The north strip is the stair, 140 cm deep. The landing is the west 200 cm of that strip. The flight is the rest. Neither holds a hex. Both floors keep that place.

The ground floor is the small room, 2 columns by 3 rows, 6 hexes, against the west of the south block. The east 240 cm of that block is spare floor. The floor above is the medium room, 4 columns by 3 rows, 12 hexes, and it fills the south block. The doorway faces north and is centered on the landing, 100 cm from the inner west. The inner south-west corner is one wall in from the outer south-west. South, west, and east are curtain. North faces the yard.

The rooms together are the Humble kit, 18. The visit draws the ground floor: six hexes and that doorway. The stair, the landing, and the upper floor are not drawn yet.

### Average

Average keeps the Humble building and adds a second building with one floor, the other small room, 6 hexes. That building's place in the yard is not drawn. The rooms together are the Average kit, 24. The L plan is retired.

## Acceptance

- Meagre Fortress is not supported. Fortress `Supported` stays false for Meagre.
- Fortress `Supported` is true for Humble, Average, Impressive, Grand, Stately, and Imperious.
- Usable hexes on all floors of all buildings equal the Strongholds by size and type kit for that size (Humble 18, Average 24, Impressive 48, Grand 80, Stately 100, Imperious 126).
- Keep floors match the Keep column in that plan (Humble and Average: small on the ground, medium above; Impressive: two medium; Grand: one large and two medium; Stately: two large and one medium; Imperious: three large).
- A fortress has at least one building with at least two floors.
- The outer wall is one rectangle. It is a closed ring. The yard is the open ground inside it and outside the buildings.
- The yard is not part of the hex total.
- A building's outer face may form part of the outer wall. A stretch of wall with no building on it is curtain.
- The Keep is the main building. Where another building exists, the Keep has more usable hexes on a floor than that building, and more floors when the Keep has more than one floor.
- Humble is one building, small on the ground and medium above, as the Humble section draws it. Average keeps that building and adds one small one-floor building. Impressive has the Keep and Barracks. Grand has the Keep, Barracks, and a Gatehouse. Stately has the Keep, Barracks, a Gatehouse, and Workshops. Imperious has the Keep, Barracks, a Gatehouse, one Workshop, and a Stores.
- Imperious Barracks have two floors. Stately Barracks have more than one floor. Other Barracks have one floor.
- A Gatehouse has at least two floors.
- A Workshop has one floor. Stores has one floor.
- A fortress has no Hall, no Chapel, and no Kitchen.
- A building may have more than one floor.
- A higher floor of a building has the same outer walls as the floor below. The usable hexes may match the floor below. An interior wall may close off part of the floor: a medium room can sit above a large room, and a small room and a medium room can sit side by side above a large room. The closed part holds no usable hex. A Gatehouse ground floor that is only a gate passage holds no usable hex.
- The outer wall and the building walls are straight.
- A usable hex has all six vertices inside the inner face.
- A building with more than one floor has a stair and a landing inside it. A higher floor is reached only by that stair.
- The landing holds no usable hex. The stair holds no usable hex.
- A building with one floor has no stair and no landing.
- On a repeated floor, the stair and the landing occupy the same places.
- Where more than one type supports a size, those types have the same hex total (the room kit).
- The stronghold entrance and the stronghold exit are one gate in the outer wall, on the grass side of the cell. The captain uses that gate to enter and to leave.
- The visit starts in the yard, inside the gate.
- The gate opening is 140 cm wide and one storey high.
- A fortress without a Gatehouse has the gate in the curtain.
- A Gatehouse stands on the gate. Its ground-floor openings face the grass and the yard. A floor above that passage is a room.
- A doorway in a building is 140 cm wide and has no door. It faces the yard. That doorway is not a second way onto the grass.
- The outer wall is one storey, 300 cm. The gate fills that storey. A parapet of 120 cm sits above a building's top floor. A building of more than one floor stands above the wall.
- The cave mouth, the cave interior, the tower exterior, the tower door, and the tower interior stay as they are until [Tower stronghold by size](Done/tower-stronghold-by-size.md) changes the tower.
- This story adds no world-map `!` line.

## Where this stands

Humble is the only supported fortress size. Its shell is in the game. Choosing Humble Fortress on a site stone moves the company, removes that site stone, draws the shell, and stands the captain one hex outside the gate, facing it. Play in Relion on SC1-0662 showed the first shell. The dark slot in the grass-side wall is the gate. It fills the one-storey wall. One building sits on the far side of the yard. The yard is the open ground between that building and the gate.

Humble names an interior actor. The visit draws the outer wall and a dark gate on the grass side. The captain arrives one hex inside that gate, looking into the yard. A click on the shell gate enters. A click on the interior gate leaves, and he stands one hex outside it, facing away. Companions stay outside, the way a tower visit does. Choosing Humble still raises the shell, because that choice does not open the visit. `Trip` is `None`. The visit draws one building on the far side of the yard. Its ground floor is the small room, against the south-west, and the doorway faces the yard from the landing. You walk through the gap. The inside is the floor within those walls. That floor draws the hex grid. The yard has none. A ceiling closes the room at one storey. The outer wall is that same storey. The room has a light under that ceiling. The visit draws the pieces the company carried onto those hexes. The name sign stands beside the gate, on the grass. The east wall of the yard carries an Arrange furniture plate. Arrange mode uses those same hexes.

Play on SC1-0674 moved those pieces onto other hexes. The owner was happy with that shell, then replaced the layout. Humble is one two-floor building, small on the ground and medium above. Average keeps it and adds one small one-floor building. A fortress always has at least one building with at least two floors. The old two-building row and the Average L are retired. The face is in `AddBuildings`. The visit draws one building, six ground hexes, and the doorway on the landing. The stair and the upper floor are not drawn yet. Average uses the same building, stays unsupported, and its second building is not placed. Impressive and the larger sizes are still undrawn, so this story stays In Progress.

Draw the stair and the upper floor before adding Average's second building. The constructor lists this Humble face. Draw the pieces the company carried. Do not add a piece of the fortress's own. Do not add a world-map `!`. The owner's chart save is already on main. Buildings are not named. Floors are the list on the building, ground floor first. Wall thickness, storey, and opening width live on `FSolidStronghold` (`WallCm`, `StoreyCm`, `OpeningCm`). The parapet, the gap, and the yard depth stay on the fortress. A second floor keeps a stair along the door wall. The landing is the first `LandingRunCm` from the inner start of that wall, and the doorway is centered on the landing. `AddBuildings` names the room block. The stair width is the tower plan's `StairWidthCm`.

## Implementation notes

- The plan of record is **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Keep floors and kit totals come from that table. Do not invent a second kit.
- How the type, the exterior actor, and the interior actor are written follows **Code structure** in `Source/SolidCore1/Strongholds/README.md`.
- [Stronghold type classes](Done/stronghold-type-classes.md) leaves fortress `Supported` false for every size, with no visit and no ground floor. This story makes `Supported` true from Humble through Imperious and supplies the fortress through that subclass. Meagre stays false. Meagre Fortress is not a final pair.
- Hex totals are the shared room kits, not the sum of `CollectKeptHexes` on the tower plan. A test sums fortress usable hexes and compares them with the kit for that size.
- [Tower stronghold by size](Done/tower-stronghold-by-size.md) puts Average through Imperious on Imperious floor bands with one kit room per floor. [Imperious dungeon](imperious-dungeon.md) uses the Imperious kit (126) with passages and stairs. This story does not implement those.
- Average's split is the Average section. Split a later size across its buildings in a mockup before coding; do not invent a split in a later pass unless the owner draws one.
- Older mockups (Humble 13+13, Impressive 18×3+9, Imperious 27×4 + 14×2 + 15 + 10 + 4) are obsolete. Redraw against the kit before implementing.
- Buildings are added in this order: Barracks, then a Gatehouse, then Workshops. Imperious then has a Stores in place of a second Workshop. There is no Hall, no Chapel, and no Kitchen. Each building keeps the same outer walls on every floor. A higher floor may repeat the hexes below it. An interior wall may close off part of a floor, so a medium room can sit above a large room, or a small room and a medium room can sit side by side above a large room. A Gatehouse ground floor that is only a passage holds no usable hex. Do not shrink an upper floor the way the tower's radius steps inward.
- A fortress without a Gatehouse still has the gate in the curtain. The Gatehouse is the building on that gate. Its ground floor is the passage, open to the grass and to the yard, and the stair is inside the building beside that passage.
- The walls are straight. They do not follow the hex edges. A usable hex is one whose six vertices lie inside the inner face. A hex whose center lies on the stair or the landing is not usable, the same exclusion `HexOnStair` and `HexOnLanding` already apply in a tower. The stair width is the tower's `StairWidthCm`. The landing run is the tower's `LandingRunCm`. A building with two or more floors keeps that stair along the door wall. The landing is the first landing run from the inner start of that wall, and the doorway is centered on the landing. `AddBuildings` names the room block. A one-floor building has no stair, and its doorway is centered on the face. The wall thickness is `FSolidStronghold::WallCm`.
- The storey is `FSolidStronghold::StoreyCm`, 300 cm. The outer wall is that storey. The parapet is 120 cm, one hex across, above a building's top floor. The gate and a building doorway are `FSolidStronghold::OpeningCm`, 140 cm wide. The gate opening is one storey. Humble's outer face, rooms, gap, and yard are in the Humble section. Average's outer face, rooms, and yard are in the Average section. The larger sizes are still undrawn.
- The ground inside the outer wall, outside the buildings, is the yard. It is not part of the hex total. The visit starts there, inside the gate, and leaving returns through that gate to the grass beside the cell. The visit remembers that cell, the way a tower visit remembers its cell.
- The type-picker already lists a type when that subclass returns `Supported` for the site size, the size is unlocked, and the size is larger than the active stronghold. This story does not change that rule. After it, a qualifying site lists Fortress for Humble through Imperious.
- Do not add a world-map `!`. The owner places the marker. A fortress line is `Stronghold`, size then type, as in `Stronghold (Average Fortress)`. A site stays `StrongholdSite` and a size, with no type. The Imperious site already on the map is `! 16, 36 = StrongholdSite (Imperious)`.
- There is no shelf row for a fortress. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland.
- The fortress does not use the tower exterior.
- The visit draws the pieces the company carried. It does not add a piece of the fortress's own, and it does not decide a new catalog.
- Floors use the shared 1.2 m pointy-top grid.
- The product commit adds tests for the roster, the Keep floors, the kit totals, the stair and landing exclusion, and a Gatehouse passage floor with no usable hex when that plan is drawn. The tests compare against the kit, not the tower kept-hex sum.
