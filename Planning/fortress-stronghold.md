# Fortress stronghold

Status: In Progress

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player can visit a fortress. A fortress is one rectangular outer wall with a yard inside it and one or more buildings in that yard. The wall is a closed ring of straight sides. A building's outer face may be part of that wall. Where a building does not fill a side, the wall continues as a curtain. The yard is the open ground inside the wall and outside the buildings. It is not part of the hex total. You go from one building to another across the yard.

Rooms, hex totals, and keep floors follow **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Large rooms are 20 hexes, medium 12, small 6. Dungeon, Fortress, and Tower at the same size share that kit. Meagre is not a fortress size. Humble has no tower; its target is still the kit, 18.

| Size | L | M | S | Hexes | Keep |
|------|--:|--:|--:|------:|------|
| Humble | 0 | 1 | 1 | 18 | S, then M |
| Average | 0 | 1 | 2 | 24 | S, then M |
| Impressive | 0 | 3 | 2 | 48 | 2× M |
| Grand | 1 | 4 | 2 | 80 | 1× L, 2× M |
| Stately | 2 | 4 | 2 | 100 | 2× L, 1× M |
| Imperious | 3 | 4 | 3 | 126 | 3× L |

The keep is the largest building. It has at least two floors. Keep floors are the kit rooms in the Keep column, ground floor first. The remainder of the kit goes to other buildings. [Tower stronghold by size](Done/tower-stronghold-by-size.md) is the tower side of the same plan. [Imperious dungeon](imperious-dungeon.md) is the Imperious dungeon side.

Plans try to reuse a small number of building types. The number of floors in a type can change, and so can the rooms inside it. Buildings are not named.

Humble is one building: the small room on the ground, the medium room above it. Average reuses that building and adds a second building with one small floor. A later size is drawn in a mockup before it is coded. Do not invent its buildings in a pass.

A building may have more than one floor. Every floor of that building has the same outer walls. The floors do not step inward the way a tower's floors do. A higher floor may use the same hexes as the floor below. An interior wall may close off part of a floor, and the closed part holds no usable hex. A medium room can sit above a large room. A small room and a medium room can sit side by side above a large room.

The outer wall and the building walls are straight. They do not follow the hex edges. A usable hex lies entirely inside the building: all six vertices are inside the inner face.

### Stairs

A building with two or more floors has a stair and a landing inside it. A higher floor is reached only by that stair. The stair holds no usable hex, and neither does the landing. A building with one floor has no stair and no landing, and its doorway is centered on the face. On a floor that repeats the one below, the stair and the landing occupy the same places, so the usable hexes match.

The stair is a strip along the door wall. Its clear width is `StairWidthCm`, 140 cm, and that strip is added on the door side of the room block. The landing is the first `LandingRunCm`, 200 cm, measured from the inner start of that wall. The flight is the rest of the strip. The doorway is centered on the landing.

### The gate

The captain enters and leaves through one gate in the outer wall. The gate is on the grass side of the cell, the same way the cave mouth and the tower door face the grass. It is cut through the curtain. The visit starts in the yard, inside the gate. A building doorway has no door. It faces the yard. It is not a second way onto the grass.

### The shell

The outer wall is one storey plus a parapet of 120 cm, so it is 420 cm. The gate is one storey, and it stops under that parapet. A building is one storey for each of its floors, plus that parapet on the top floor. A building of more than one floor stands above the wall, and its parapet is merlons. The corners are high. Between them, as many low sections as fit are spread evenly along the wall. A low section is half the parapet height and half as wide as each high section. The parapet band is 48 cm thick, forty percent of its height. The outer wall stays plain.

### A size in progress

A size is in progress or complete. It is complete when the usable hexes on all floors of all of its buildings equal the kit total for that size, the same total a tower of that size uses. Until then the size can still be chosen in the game, and it stays in progress.

Humble, Impressive, and Imperious had earlier drawn plans with different per-building hex counts. Those counts are superseded by Strongholds by size and type. New fortress mockups follow that table; do not ship the old 13 / 18 / 27-style splits. The two-building Humble row and the Average L are retired.

### Humble

The plan's gate side is north. In the game that side faces the grass, and the plan turns with it.

Humble is one building, and that building is the keep. Its inner face is 540 cm wide and 486.4 cm deep. The south block is the medium span, 540 cm by 346.4 cm. The north strip is the stair, 140 cm deep. The landing is the west 200 cm of that strip. The flight is the rest, 340 cm. Neither holds a hex. Both floors keep that place.

The ground floor is the small room, 2 columns by 3 rows, 6 hexes, against the west of the south block. The east 240 cm of that block is spare floor. The floor above is the medium room, 4 columns by 3 rows, 12 hexes, and it fills the south block. The doorway faces north and is centered on the landing, 100 cm from the inner west. The inner south-west corner is one wall in from the outer south-west. South and west are curtain. North and east face the yard, so the yard is an L around the building. The north yard is one storey, 300 cm. The east yard is the medium length plus one wall, 590 cm, and the east curtain makes the outer width twice the building's run.

The rooms together are the Humble kit, 18. The visit draws the ground floor: six hexes and that doorway. The stair, the landing, and the upper floor are not drawn yet. Humble is in progress.

### Average

Average reuses the Humble building and adds a second building with one floor, the other small room, 6 hexes. That building's place in the yard is not drawn. The rooms together are the Average kit, 24. The L plan is retired. Average is in progress.

Impressive, Grand, Stately, and Imperious are in progress. Each draws the empty courtyard. Their buildings are not drawn, so none of their kit hexes are in the game.

## Acceptance

- Meagre Fortress is not supported. Fortress `Supported` stays false for Meagre.
- Fortress `Supported` is true for Humble, Average, Impressive, Grand, Stately, and Imperious, including while a size is in progress.
- A size is complete when the usable hexes on all floors of all of its buildings equal the kit for that size (Humble 18, Average 24, Impressive 48, Grand 80, Stately 100, Imperious 126). Until then the size can be chosen in the game, and it is in progress.
- Keep floors match the Keep column in that plan (Humble and Average: small on the ground, medium above; Impressive: two medium; Grand: one large and two medium; Stately: two large and one medium; Imperious: three large).
- The keep is the largest building, and it has at least two floors.
- Plans try to reuse a small number of building types. The number of floors in a type can change, and so can the rooms inside it.
- The outer wall is one rectangle. It is a closed ring. The yard is the open ground inside it and outside the buildings.
- The yard is not part of the hex total.
- A building's outer face may form part of the outer wall. A stretch of wall with no building on it is curtain.
- Humble is one building, small on the ground and medium above, as the Humble section draws it. Average reuses that building and adds one small one-floor building.
- A building may have more than one floor.
- A higher floor of a building has the same outer walls as the floor below. The usable hexes may match the floor below. An interior wall may close off part of the floor: a medium room can sit above a large room, and a small room and a medium room can sit side by side above a large room. The closed part holds no usable hex.
- The outer wall and the building walls are straight.
- A usable hex has all six vertices inside the inner face.
- A building with more than one floor has a stair and a landing inside it. A higher floor is reached only by that stair. The stair is a 140 cm strip along the door wall. The landing is the first 200 cm of that strip from the inner start of the wall. The doorway is centered on the landing.
- The landing holds no usable hex. The stair holds no usable hex.
- A building with one floor has no stair and no landing, and its doorway is centered on the face.
- On a repeated floor, the stair and the landing occupy the same places.
- Where more than one type supports a size, those types have the same hex total (the room kit).
- The stronghold entrance and the stronghold exit are one gate in the curtain, on the grass side of the cell. The captain uses that gate to enter and to leave.
- The visit starts in the yard, inside the gate.
- The gate opening is 140 cm wide and one storey high.
- A doorway in a building is 140 cm wide and has no door. It faces the yard. That doorway is not a second way onto the grass.
- The outer wall is one storey plus a parapet of 120 cm, so it is 420 cm. The gate is one storey and stops under that parapet. A building is one storey for each of its floors, plus that parapet on the top floor. A building of more than one floor stands above the wall, and its parapet is merlons. The corners are high. Between them, as many low sections as fit are spread evenly along the wall. A low section is half the parapet height and half as wide as each high section. The parapet band is 48 cm thick, forty percent of its height. The outer wall stays plain.
- The cave mouth, the cave interior, the tower exterior, the tower door, and the tower interior stay as they are until [Tower stronghold by size](Done/tower-stronghold-by-size.md) changes the tower.
- This story adds no world-map `!` line.

## Where this stands

Humble through Imperious are supported, and each of those sizes is in progress. Meagre stays unsupported. None of the sizes is complete, because none yet has its kit hexes in the game. Choosing one still moves the company.

Humble's shell is in the game. Choosing Humble Fortress on a site stone moves the company, removes that site stone, draws the shell, and stands the captain one hex outside the gate, facing it. Play in Relion on SC1-0662 showed the first shell. The dark slot in the grass-side wall is the gate. It stops under the parapet. The two-floor building stands above that wall. Its parapet band is 48 cm thick. The corners are high, and each low section is half the parapet height and half as wide as the high sections beside it. The visit still draws that parapet solid. The yard is an L around that building: 300 cm of open ground to the north, up to the gate, and 590 cm to the east.

Humble names an interior actor. The visit draws the outer wall and a dark gate on the grass side. The captain arrives one hex inside that gate, looking into the yard. A click on the shell gate enters. A click on the interior gate leaves, and he stands one hex outside it, facing away. Companions stay outside, the way a tower visit does. Choosing Humble still raises the shell, because that choice does not open the visit. `Trip` is `None`. The visit draws one building in the south-west of the yard. Its ground floor is the small room, against the south-west, and the doorway faces the yard from the landing. You walk through the gap. The inside is the floor within those walls. That floor draws the hex grid. The yard has none. A ceiling closes the room at one storey. The outer wall is one storey plus the parapet, and the two-floor building stands above it. The room has a light under that ceiling. The visit draws the pieces the company carried onto those hexes. The name sign stands beside the gate, on the grass. The east wall of the yard carries an Arrange furniture plate. Arrange mode uses those same hexes.

Play on SC1-0674 moved those pieces onto other hexes. The owner was happy with that shell, then replaced the layout. The old two-building row and the Average L are retired. The face is in `AddBuildings`. The visit draws one building, six ground hexes, and the doorway on the landing. The stair and the upper floor are not drawn yet, so Humble has 6 of its 18 hexes. Average uses the same building, and its second building is not placed, so Average has 6 of its 24. Impressive and the larger sizes draw the empty courtyard and have none of their kit hexes. Choosing one raises that shell, and the carried pieces stay with the company.

Draw the stair and the upper floor before adding Average's second building. The constructor lists this Humble face. Draw the pieces the company carried. Do not add a piece of the fortress's own. Do not add a world-map `!`. The owner's chart save is already on main. Buildings are not named. Floors are the list on the building, ground floor first. Wall thickness, storey, and opening width live on `FSolidStronghold` (`WallCm`, `StoreyCm`, `OpeningCm`). The parapet stays on the fortress. `AddBuildings` calls `AdditionalCourtyard` once for the size. Humble and Average pass a north yard of one storey and an east yard of the medium length plus one wall. Every other size passes the medium width, so a size with no building still has a courtyard. A second floor keeps the stair in the Stairs section.

## Implementation notes

- The plan of record is **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Keep floors and kit totals come from that table. Do not invent a second kit.
- How the type, the exterior actor, and the interior actor are written follows **Code structure** in `Source/SolidCore1/Strongholds/README.md`.
- [Stronghold type classes](Done/stronghold-type-classes.md) leaves fortress `Supported` false for every size, with no visit and no ground floor. This story makes `Supported` true from Humble through Imperious and supplies the fortress through that subclass. Meagre stays false. Meagre Fortress is not a final pair.
- Hex totals are the shared room kits, not the sum of `CollectKeptHexes` on the tower plan. A test sums fortress usable hexes and compares them with the kit for that size. A size stays in progress until that sum matches.
- [Tower stronghold by size](Done/tower-stronghold-by-size.md) puts Average through Imperious on Imperious floor bands with one kit room per floor. [Imperious dungeon](imperious-dungeon.md) uses the Imperious kit (126) with passages and stairs. This story does not implement those.
- Average's split is the Average section. Draw a later size in a mockup before coding it. Plans try to reuse a small number of building types.
- Older mockups (Humble 13+13, Impressive 18×3+9, Imperious 27×4 + 14×2 + 15 + 10 + 4) are obsolete. Redraw against the kit before implementing.
- Each building keeps the same outer walls on every floor. A higher floor may repeat the hexes below it. An interior wall may close off part of a floor, so a medium room can sit above a large room, or a small room and a medium room can sit side by side above a large room. Do not shrink an upper floor the way the tower's radius steps inward.
- The gate is in the curtain. The walls are straight. They do not follow the hex edges. A usable hex is one whose six vertices lie inside the inner face. A hex whose center lies on the stair or the landing is not usable, the same exclusion `HexOnStair` and `HexOnLanding` already apply in a tower. Stair placement is the Stairs section. `AddBuildings` names the room block. The wall thickness is `FSolidStronghold::WallCm`.
- The storey is `FSolidStronghold::StoreyCm`, 300 cm. The outer wall is that storey plus the parapet, 420 cm. The gate stops under the parapet. A building is one storey per floor, plus the parapet on its top floor. The parapet is 120 cm high, one hex. On a building the band is 48 cm thick. The gate and a building doorway are `FSolidStronghold::OpeningCm`, 140 cm wide. The gate opening is one storey. Humble's outer face, rooms, gap, and yard are in the Humble section. Average's outer face, rooms, and yard are in the Average section. The larger sizes are still undrawn.
- The ground inside the outer wall, outside the buildings, is the yard. It is not part of the hex total. The visit starts there, inside the gate, and leaving returns through that gate to the grass beside the cell. The visit remembers that cell, the way a tower visit remembers its cell.
- The type-picker already lists a type when that subclass returns `Supported` for the site size, the size is unlocked, and the size is larger than the active stronghold. This story does not change that rule. After it, a qualifying site lists Fortress for Humble through Imperious. A size in progress is still listed.
- Do not add a world-map `!`. The owner places the marker. A fortress line is `Stronghold`, size then type, as in `Stronghold (Average Fortress)`. A site stays `StrongholdSite` and a size, with no type. The Imperious site already on the map is `! 16, 36 = StrongholdSite (Imperious)`.
- There is no shelf row for a fortress. A size and type with no row keeps the marker's own biome. Do not assume Mountain or Grassland.
- The fortress does not use the tower exterior.
- The visit draws the pieces the company carried. It does not add a piece of the fortress's own, and it does not decide a new catalog.
- Floors use the shared 1.2 m pointy-top grid.
- The product commit adds tests for the roster, the keep floors, the kit totals, and the stair and landing exclusion when that plan is drawn. The tests compare against the kit, not the tower kept-hex sum.
