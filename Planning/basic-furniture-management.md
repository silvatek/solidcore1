# Basic furniture management

Status: Ready

## Story

The player is inside a stronghold. The interior wall carries a sign that reads Arrange furniture. A click on that sign, or `\`, starts arrange mode, and the captain is hidden so the floor is clear to see. The same click, or `\` again, leaves the mode. The pointer highlights the hex under it. A piece on that hex shows its name.

A click or Enter on a piece lifts it. The piece follows the pointer and snaps to the hex under the tip. A click or Enter on cells that can hold it puts it down. While a piece is held, Q, E, and the mouse wheel each turn it by one facing. The wheel does not zoom during that hold. Esc puts a held piece back, or closes the browser when that is what is open. Esc with nothing held, and the browser closed, leaves the mode, and the captain is visible again.

A new piece starts from an empty hex that can hold one. A click or Enter on one, while nothing is held, opens the furnishing browser. The browser is a column beside the floor, listing the catalog in catalog order, and it stays off the hexes. It is closed when arrange mode starts. The wheel over the column scrolls it. Up and Down move through the rows, and a click on a row lifts that kind onto the hex that opened the browser, when the stronghold still has room for that kind. A fireplace and a chest stop at one each. A bed stops at three. The column closes while the piece is held, so the wheel can turn it and the floor stays clear. Esc closes the browser and leaves the mode on. The catalog will hold dozens of kinds. The three in this story fit, so the scrollbar stays hidden. A click in this mode does not walk the captain and does not use the door.

## Acceptance

- `\` inside a stronghold toggles arrange mode. A click on the Arrange furniture sign does the same. The Meagre den and the tower both take it. `\` in the open world, in the Iglin tunnels, or while a menu, the inventory, the journal, a message, or the party builder is open, does nothing. The sign is not in those places.
- The sign reads Arrange furniture. It is fixed on the interior wall, facing the room, and it is clear of the door and of any floor-number plate. The Meagre den has one, beside the entrance. Each tower floor has one. It is not a furnishing, it occupies no hex, and arrange mode cannot move it.
- A click on the sign toggles the mode when the pointer is showing. While the right button looks, the click does nothing. The click does not walk the captain, does not use the door, and does not open the browser or lift a piece.
- Toggling the mode off, by the sign or by `\`, returns a held piece, discards a new piece that was never put down, and closes the browser. The captain is drawn again.
- `/` still opens the inventory. `\` does not.
- While the mode is on, the captain is not drawn. He stays on the cell he was on. WASD, the left stick, and a click do not move him. The camera stays where it was.
- His body does not take the pointer. A piece or a hex behind him can be highlighted.
- Leaving the mode draws him again, on that same cell.
- The pointer highlights the hex under it on the floor the captain is on. A piece on that hex is the highlight, and the pointer shows its catalog name.
- The pointer has to be showing. While the right button looks, nothing is highlighted and a click changes nothing.
- Confirm is a left click or Enter.
- Confirm on a piece lifts it. One piece is held. A ghost follows the pointer and snaps to a hex. The cells it occupied are empty while it is held.
- Confirm on a legal placement puts it down there, with the facing it has.
- A fireplace occupies one hex. A chest occupies one hex. A bed occupies two neighboring hexes. The second hex is the next one in the direction the bed faces.
- A placement off the floor, on a stair, on a landing, or overlapping another piece is not legal. The ghost still shows. Confirm does not put it down, and the piece stays held.
- The den entrance hex is blocked. The tower's ground-floor hex nearest the door is blocked. The hex the captain is standing on is blocked. A blocked hex does not open the browser, and it does not take a piece. A bed is blocked when either of its hexes is blocked.
- Each catalog kind has a limit. Fireplace is 1. Bed is 3. Chest is 1. The count is how many of that kind the stronghold holds, on every floor, including a piece that is held. The den and the tower count separately.
- A new game den holds one fireplace, one bed, and one chest, so two beds can still be added there. The tower starts with none, so it can take one fireplace, three beds, and one chest.
- A row at its limit stays in the column. A click or Enter on it does not lift a new piece. Lifting a piece that is already down still works, and it does not raise the count.
- A chest added where the stronghold had none starts empty, two columns by three rows. The den's chest is still the cave chest, and moving it keeps the items in its slots. The limit stops a second chest in that stronghold.
- Q and E rotate a held piece by one facing. One facing is 60 degrees, and six steps return it to the facing it started with. The mouse wheel does the same, one facing per notch. Wheel up matches Q. Wheel down matches E.
- While no piece is held and the browser is closed, the wheel still zooms. Q and E do nothing then. While the browser is open, the wheel over its column scrolls that column only, and the wheel elsewhere still zooms.
- Esc while a piece is held returns it to the cells and facing it had, and the mode stays on. A piece that was lifted from the browser, and not yet put down, is discarded. The browser stays closed.
- Esc while the browser is open, and nothing is held, closes the browser. The mode stays on. Esc with nothing held and the browser closed leaves the mode.
- The browser is closed when arrange mode starts, and it stays closed until an empty hex that can hold a piece is confirmed.
- Confirm on an empty hex that can hold a piece, while nothing is held, opens the browser for that hex. Confirm on a blocked hex does not open it. Confirm on an empty hex while a piece is held tries to put that piece down, and does not open the browser.
- Confirm on another empty hex that can hold a piece, while the browser is open, keeps the browser open and uses that hex instead. A blocked hex leaves the remembered hex as it was.
- The browser is a column of every kind in the furnishing catalog, in catalog order. This story's rows are Fireplace, Bed, and Chest. The column sits beside the floor and does not cover a hex.
- The column shows as many rows as fit in its frame. Further kinds scroll. A scrollbar shows when the rows do not fit, and it stays hidden when they do. Fireplace, Bed, and Chest fit, so the thumb is hidden.
- Up and Down move the highlight through the catalog while the browser is open, and the column scrolls to keep that row in the frame. A click on a row lifts that kind as a held piece, snapped to the hex that opened the browser. Enter does too, unless the pointer is over a placed piece. Up and Down do nothing while the browser is closed.
- Enter or a click on a placed piece still lifts that piece, and closes the browser if it was open. The catalog highlight does not change which placed piece a click lifts.
- Lifting a piece closes the browser. Putting the piece down, or putting it back, leaves the browser closed. A click where the column was does nothing while it is closed.
- A new game still starts with the fireplace, the bed, and the chest already placed in the Meagre den. This story does not add a fourth kind.
- Moving a chest keeps the items in its slots. The pack and the worn boxes stay as they were.
- A placed piece is moved or left where it is. This story does not delete one that is already down.
- While the mode is on, a click does not start a walk, and it does not enter or leave through the door. A click on the Arrange furniture sign still leaves the mode.
- Leaving the stronghold ends the mode first. A held piece goes back, a new piece that was never put down is discarded, and the captain is drawn for the leave. The leave then proceeds as it does now, including Build party when that page opens on a leave.
- After the player leaves and comes back, each piece is still on the floor and hex where they put it, with the same facing. A new game has none of those moves.
- The whole floor the captain is on can be arranged. Another floor of the tower is not.
- This story binds the mouse, `\`, Enter, Esc, Q, E, the wheel, Up, and Down. No number key lifts a kind. Gamepad buttons stay as they are. Focus, confirm, cancel, rotate, and the catalog highlight are the actions, so a later story can bind a pad to the same column without a new placement rule.

## Implementation notes

- `\` is `EKeys::Backslash`. Inventory stays `EKeys::Slash`. The sign is the same toggle.
- The sign is a light plate on the wall with the words Arrange furniture in dark letters, facing the room. Build the letters so they draw on the interior wall. A runtime text component was spawned on the tower wall and did not draw, and `ASolidTownSign` stays the outdoor welcome board. The floor-number plate is a separate sign. This plate is a pointer query hit and does not block the captain. The den plate sits on the rock beside the mouth. A tower plate sits on that floor's curved wall, inset like `FloorSignInsetCm`, clear of `FloorSignFor` and of the door.
- Zoom is `MouseWheelAxis` on `IA_Zoom`. While a piece is held, that axis rotates and is not applied as zoom. While the catalog column is under the pointer, the same axis scrolls the column, the way a wheel over a party scrollbox scrolls that box only.
- Q and E are unbound today. One facing is 60 degrees, the step `SolidStrongholdGrid::HexVertices` uses. `SolidStrongholdFurniture::SpanCell` already places the bed's second cell along `RotationDeg`.
- Hide the captain with `SetActorHiddenInGame`. The pawn stays, so the camera does not jump. His mesh and his capsule do not answer the pointer ray.
- The placed list is the stronghold's used furnishings. The starter three from `UsedFurnishings` are the new-game list. A move edits that list. Store it on the travel session, the way the pack is stored, so `OpenLevel` brings the same placements back.
- The browser opens on an empty-hex confirm and remembers that hex. A catalog row builds a pending piece on it and does not write the placed list until the placement confirm. Esc drops that pending piece and does not reopen the browser. The column is one scrollbox. Its rows are `SolidStrongholdFurniture::Catalog` in order. `FItem` gains `Limit`. Fireplace is 1, Bed is 3, Chest is 1. Count matching `ItemIndex` values in that stronghold's placed list, and count a held piece once. This story does not add a kind, and it does not add a filter. A later kind is another row in that catalog, with its own limit, and the same column scrolls to it.
- The den entrance is `SolidStrongholdGrid::MeagreEntranceCell`. The tower door hex is `SolidTowerStronghold::DoorHexCenter`. The captain's hex is the cell under his pawn when the mode is on. Stairs, landings, and hexes the tower plan already rejects stay unavailable. The den's fourteen hexes are the Meagre floor, minus the entrance and the captain's cell.
- A new chest uses an empty `SolidStorage::FContainer` of `SolidStorage::Columns` by `SolidStorage::Rows`, stored with that furnishing. The den chest keeps `FGameState::CaveChest`. [Stronghold transition](stronghold-transition.md) moves this saved list, including beds added here, and each chest keeps its own container.
