# Add furniture

Status: In Progress

## Story

Do not start this story until [Basic furniture management](Done/basic-furniture-management.md) is Done.

The player is arranging furniture in the company stronghold. Confirm on an empty hex that can hold a piece, while nothing is held, opens the furnishing browser. The browser is a column beside the floor, listing the catalog in catalog order, and it stays off the hexes. It is closed when arrange mode starts. Up and Down move through the rows, and a click on a row lifts that kind onto the hex that opened the browser, when the stronghold still has room for that kind. A fireplace and a chest stop at one each. A bed stops at three. A small table stops at ten. The column closes while the piece is held, so the wheel can turn it and the floor stays clear. Esc closes the browser and leaves the mode on. The catalog will hold dozens of kinds. The four in this story fit, so the scrollbar stays hidden.

A new chest starts empty. The chest the company already has keeps its items. Putting the new piece down writes it onto the stronghold's list, so leaving and coming back shows it with the pieces already there.

This story adds Small Table, so the browser has a new kind to place. It is one hex: a table top on four legs. A new game still starts with the fireplace, the bed, and the chest, and with no small table. The player adds one from the browser. Each furnishing kind is its own actor. Small Table is `ASolidSmallTable`. The browser is not a class of its own.

## Acceptance

- Confirm on an empty hex that can hold a piece, while nothing is held, opens the browser for that hex. Confirm on a blocked hex does not open it. Confirm on an empty hex while a piece is held tries to put that piece down, and does not open the browser.
- The stronghold's entrance hex is blocked. On a tower, that is the ground-floor hex nearest the door. The hex the captain is standing on is blocked. A blocked hex does not open the browser, and it does not take a piece. A bed is blocked when either of its hexes is blocked. A small table is blocked when its hex is blocked.
- Each catalog kind has a limit. Fireplace is 1. Bed is 3. Chest is 1. Small Table is 10. The count is how many of that kind the stronghold holds, on every floor, including a piece that is held. The company's stronghold has its own count. The new stronghold counts on its own once the company has moved.
- The company's stronghold holds one fireplace, one bed, and one chest, and no small table, so two beds and ten small tables can still be added there. Until the pieces move with the company, the new stronghold has none, so it can take one fireplace, three beds, one chest, and ten small tables.
- A row at its limit stays in the column. A click or Enter on it does not lift a new piece. Lifting a piece that is already down still works, and it does not raise the count. The Small Table row stays when the stronghold already holds ten, including one that is held.
- A placement still needs a legal hex. A floor with no legal hex refuses the piece while the count is under the limit. The limit refuses an eleventh small table while a legal hex is free.
- A chest added where the stronghold had none starts empty, two columns by three rows. The chest the company already has keeps its items when it is moved. The limit stops a second chest in that stronghold.
- A small table occupies one hex. The piece is a wooden top on four wooden legs. The top is a square slab, and it stays inside that hex at every facing. Each leg is a post under one corner of the top, and the post stands on the floor. The four posts are separate. The piece is not one block.
- A small table holds no items. A click on it, while arrange mode is off, does not open a container.
- The browser is closed when arrange mode starts, and it stays closed until an empty hex that can hold a piece is confirmed.
- Confirm on another empty hex that can hold a piece, while the browser is open, keeps the browser open and uses that hex instead. A blocked hex leaves the remembered hex as it was.
- The browser is a column of every kind in the furnishing catalog, in catalog order. This story's rows are Fireplace, Bed, Chest, and Small Table. The column sits beside the floor and does not cover a hex.
- The column shows as many rows as fit in its frame. Further kinds scroll. A scrollbar shows when the rows do not fit, and it stays hidden when they do. Fireplace, Bed, Chest, and Small Table fit, so the thumb is hidden.
- Up and Down move the highlight through the catalog while the browser is open, and the column scrolls to keep that row in the frame. A click on a row lifts that kind as a held piece, snapped to the hex that opened the browser. Enter does too, unless the pointer is over a placed piece. Up and Down do nothing while the browser is closed.
- Enter or a click on a placed piece still lifts that piece, and closes the browser if it was open. The catalog highlight does not change which placed piece a click lifts.
- Lifting a piece closes the browser. Putting the piece down, or putting it back, leaves the browser closed. A click where the column was does nothing while it is closed.
- Esc while a piece is held returns a piece that was already down, and discards a piece that was lifted from the browser and not yet put down. The mode stays on. The browser stays closed. A small table discarded this way leaves the count as it was before the lift.
- Esc while the browser is open, and nothing is held, closes the browser. The mode stays on.
- Toggling the mode off, by the sign or by `\`, returns a held piece, discards a new piece that was never put down, and closes the browser.
- A held small table turns by one facing, the same 60 degrees as the other pieces. Six steps return it to the facing it started with. Q, E, and the wheel are that turn. The top stays inside its hex at every facing.
- The pointer shows Small Table when that piece is the highlight.
- A click on the Arrange furniture sign does not open the browser. It still leaves the mode.
- A new game still starts with one fireplace, one bed, and one chest, and with no small table.
- A new piece that is put down is on the stronghold's list, with its hex and facing. Leaving and coming back shows it with the other pieces. A new game has none of those added pieces. A small table put down is on that list.
- Moving a chest keeps the items in its slots. The pack and the worn boxes stay as they were. Moving a small table moves the table.
- This story binds Up and Down for the catalog. No number key lifts a kind. Gamepad buttons stay as they are.

## Implementation notes

This story is one phase. The browser and the small table land together.

- The furnishing catalogue is `FurnitureCatalogue`. Each row is an `FItem`. The row stores how many hexes it covers, its actor class, and how many storage slots it has. A bed covers two. A fireplace, a chest, and a small table cover one. The chest has six slots, and the others have none. A new game copies that catalogue onto `USolidGameState::FurnishingCatalog`. `bEnabled` on the session row is the unlock. Fireplace, Bed, Chest, and Small Table start enabled. The catalogue is not the list that changes.
- Each stronghold keeps its placed pieces on `FSolidStronghold::Furnishings`. A placed piece is `FPlacedFurnishing`. It answers `SpanCell` and `Covers`. The cave is created with the fireplace, the bed, and the chest. `Move` carries that list onto the new stronghold's ground floor and leaves the stronghold behind empty. Chest contents stay on the session. [Multiple storage containers](multiple-storage-containers.md) is the story that changes storage.
- The browser opens on an empty-hex confirm and remembers that hex on `FSolidArrange`. The column is drawn on the HUD, to the right of the floor. Up and Down move the highlight, and a click on a row lifts that kind onto the remembered hex when the count is under the limit. Enter lifts the highlighted row unless the pointer is on a placed piece. The wall sign reads Done arranging while the mode is on, and Arrange furniture when it is off. A blocked hex does not open it, and it does not change a hex the browser already remembers. Esc closes the browser and leaves the mode on. A catalog row builds a pending piece on it and does not write the placed list until the placement confirm. Esc drops that pending piece and does not reopen the browser. The column is one scrollbox. Its rows are the session catalog in order. Fireplace is 1, Bed is 3, Chest is 1, Small Table is 10. Count matching `ItemIndex` values in that stronghold's placed list, and count a held piece once. This story does not add a filter. A later kind is another row, with its own limit, and the same column scrolls to it.
- Small Table is the row after Chest. The name is `Small Table`. It is not storage. `HexCount` is 1. Fireplace, Bed, and Chest keep their catalog indexes. `StartingFurnishings` stays the three starters.
- The top is 80 cm square and 8 cm thick. A leg is an 8 cm square post, 62 cm tall. Each leg is centered 12 cm in from the top's edges, so the four posts sit under the four corners. The top's diagonal is inside the hex's 120 cm flat-to-flat, so every facing stays on that hex. The wood is `BeigeColor`. The top sits on the legs. The floor line is the same lift the other pieces use.
- `ASolidFurnishing` holds the cell, the facing, and the ghost wash. It does not draw. `ASolidFireplace`, `ASolidBed`, `ASolidChest`, and `ASolidSmallTable` live in `Strongholds/Furnishings/Actors/` and each builds its own meshes. The small table is a top mesh and four leg meshes. The fireplace, the bed, and the chest do not have legs. The ghost washes the meshes that kind built. The chest case stays the chest. The table case does not build a chest.
- A chest added where the stronghold had none takes an empty container from [Multiple storage containers](multiple-storage-containers.md). The chest the company already has keeps `FGameState::CaveChest`. This story does not give a stronghold a second chest. A small table has no container. [Stronghold transition](Done/stronghold-transition.md) moves the placed list, including a small table put down here, and each container stays on its piece.
- [Basic furniture management](Done/basic-furniture-management.md) already writes a move of a piece that is down. This story adds a row to that same list when a new piece is put down.
- The Meagre cave is fourteen hexes. The entrance, the captain's hex, and the three starters stay blocked, so that floor runs out of hexes before ten small tables fit. The limit of ten is what stops the eleventh when a hex is free. A tower counts every floor toward the same ten.
- The product commit updates `SolidCore1.Stronghold.Furniture` because the catalog gains Small Table. That test now expects four kinds, the fourth name Small Table, one hex, not storage, and a limit of 10. The fireplace, bed, and chest checks stay. `SolidCore1.Stronghold.FurnitureVisual` gains the top and the four legs, and it keeps the fireplace, bed, and chest checks. `SolidCore1.Stronghold.ArrangePlace` still expects three starters. That assertion stays.
- The Stronghold table in `Content/Controls.md` gains the browser rows: open it with a click or Enter on an empty hex, move with Up and Down, lift the highlighted row with Enter or a click, and close it with Esc.
