# Basic furniture management

Status: Done

## Story

Do not start this story until [Stronghold type classes](stronghold-type-classes.md) is Done.

The player is inside a stronghold. The interior wall carries a sign that reads Arrange furniture. A click on that sign, or `\`, starts arrange mode, and the captain is hidden so the floor is clear to see. The same click, or `\` again, leaves the mode. The pointer highlights the hex under it. A piece on that hex shows its name.

A click or Enter on a piece lifts it. The piece follows the pointer and snaps to the hex under the tip. A click or Enter on cells that can hold it puts it down. While a piece is held, Q, E, and the mouse wheel each turn it by one facing. The wheel does not zoom the camera at all while arrange mode is on. Esc puts a held piece back. Esc with nothing held leaves the mode, and the captain is visible again. While the mode is on, F10 and `/` do nothing, so the main menu and the inventory stay closed. A click in this mode does not walk the captain and does not use the door.

Putting a piece down writes its hex and facing onto the stronghold's list. Leaving and coming back shows that floor. A new game still starts with the fireplace, the bed, and the chest where they already are. Adding a piece is [Add furniture](add-furniture.md). This story does not add one.

## Stages

This story is complex. The mode and the sign can be entered and left before a piece moves, and a piece can be moved before that move is remembered. A stage ships before the next stage is planned.

1. **Arrange mode and the sign.** `\` and a click on the Arrange furniture plate toggle the mode inside the company stronghold, including a new one once it is theirs. The plate is on the interior wall, facing the room, clear of the door and of any floor-number plate. It does not have to stand by the entrance. One is on a cave wall, and one is on each tower floor. It is not a furnishing. The captain is hidden and stays on his cell. WASD, the stick, and a click do not move him, and they do not use the door. The wheel does not zoom. F10 and `/` do nothing while the mode is on. Esc leaves it. `\` does nothing in the open world, in the Iglin tunnels, or while a menu, the inventory, the journal, a message, or the squad builder is open. Leaving the stronghold ends the mode first. What is true: the floor can be seen without the captain, and nothing has been lifted.
2. **Lift, turn, and place.** The pointer highlights the hex under it on the captain's floor, and names the piece on that hex. Confirm lifts one piece. A ghost follows the pointer and snaps to a hex. Q, E, and the wheel turn it by one facing. A legal confirm puts it down. Esc puts it back. Off the floor, a stair, a landing, the entrance, the captain's hex, or another piece does not take it. What is true: the three pieces already in the stronghold can be rearranged on the floor the captain is on, and a new game still starts with them where they are.
3. **Remembering a move.** A legal put-down writes that piece's hex and facing onto the stronghold's list. Esc puts a held piece back and does not write. Leaving and coming back shows each piece on that hex, with that facing. A new game has none of those moves. What is true: a visit does not forget a move.

## Acceptance

- `\` inside a stronghold toggles arrange mode. A click on the Arrange furniture sign does the same. The company's stronghold takes it, and so does the new one once it is theirs. `\` in the open world, in the Iglin tunnels, or while a menu, the inventory, the journal, a message, or the squad builder is open, does nothing. The sign is not in those places.
- The sign reads Arrange furniture. It is fixed on the interior wall, facing the room, and it is clear of the door and of any floor-number plate. It does not have to stand by the entrance. The company's stronghold has one. Each floor has one. It is not a furnishing, it occupies no hex, and arrange mode cannot move it.
- A click on the sign toggles the mode when the pointer is showing. While the right button looks, the click does nothing. The click does not walk the captain, does not use the door, and does not lift a piece.
- Toggling the mode off, by the sign or by `\`, returns a held piece. The captain is drawn again.
- Outside arrange mode, `/` still opens the inventory and F10 still opens the main menu. `\` does not open the inventory.
- While arrange mode is on, F10 does not open the main menu and `/` does not open the inventory. Esc keeps its arrange-mode jobs. After the mode ends, F10 and `/` work again.
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
- The stronghold's entrance hex is blocked. On a tower, that is the ground-floor hex nearest the door. The hex the captain is standing on is blocked. A blocked hex does not take a piece. A bed is blocked when either of its hexes is blocked.
- Q and E rotate a held piece by one facing. One facing is 60 degrees, and six steps return it to the facing it started with. The mouse wheel does the same, one facing per notch. Wheel up matches Q. Wheel down matches E.
- The wheel does not zoom the camera while arrange mode is on. While no piece is held, the wheel does nothing else either. Q and E do nothing then. Outside arrange mode the wheel zooms as it does now.
- Esc while a piece is held returns it to the cells and facing it had, and the mode stays on. That return is not written onto the list. Esc with nothing held leaves the mode.
- Confirm on an empty hex while nothing is held does nothing.
- A new game still starts with the fireplace, the bed, and the chest already placed in the company's stronghold. This story does not add a piece, and it does not add a fourth kind.
- Moving a chest keeps the items in its slots. The pack and the worn boxes stay as they were.
- A placed piece is moved or left where it is. This story does not delete one that is already down.
- A legal put-down writes that piece's hex and facing onto the stronghold's list. The other pieces on the list stay. Another floor's pieces stay.
- While the mode is on, a click does not start a walk, and it does not enter or leave through the door. A click on the Arrange furniture sign still leaves the mode. A click does not open a storage container. [Multiple storage containers](../multiple-storage-containers.md) opens one only while this mode is off.
- Leaving the stronghold ends the mode first. A held piece goes back, and the captain is drawn for the leave. The leave then proceeds as it does now, including Build squad when that page opens on a leave.
- After the player leaves and comes back, each piece is still on the floor and hex where they put it, with the same facing. A new game has none of those moves. A move to another stronghold still seats the list on that stronghold's ground floor, as [Stronghold transition](stronghold-transition.md) does.
- The whole floor the captain is on can be arranged. Another floor of the stronghold is not.
- This story binds the mouse, `\`, Enter, Esc, Q, E, and the wheel. No number key lifts a kind. Gamepad buttons stay as they are. Focus, confirm, cancel, and rotate are the actions.

## Implementation notes

- `\` is `EKeys::Backslash`. Inventory stays `EKeys::Slash`. The sign is the same toggle.
- The sign is a light plate on the wall with the words Arrange furniture in dark letters, facing the room. Build the letters so they draw on the interior wall. A runtime text component was spawned on the tower wall and did not draw, and `ASolidTownSign` stays the outdoor welcome board. The floor-number plate is a separate sign. This plate is a pointer query hit and does not block the captain. On the stronghold the company starts with, the plate sits on an interior wall clear of the mouth. It does not have to stand beside the entrance. On a tower floor it sits on that floor's curved wall, inset like `FloorSignInsetCm`, clear of `FloorSignFor` and of the door.
- Zoom is `MouseWheelAxis` on `IA_Zoom`. While arrange mode is on, that axis is not applied as zoom. While a piece is held, it rotates. Otherwise it is consumed.
- Q and E are unbound today. One facing is 60 degrees, the step `SolidStrongholdGrid::HexVertices` uses. `SolidStrongholdFurniture::SpanCell` already places the bed's second cell along `RotationDeg`.
- Hide the captain with `SetActorHiddenInGame`. The pawn stays, so the camera does not jump. His mesh and his capsule do not answer the pointer ray.
- The placed list is `USolidGameState::Furnishings`. The game instance owns that state, and `OpenLevel` keeps it, the same way it keeps the pack. The starter three from `UsedFurnishings` are the new-game list. A legal put-down writes that one entry: its hex and its facing. Esc and leaving the mode put a held piece back and do not write. The cave and the tower both place this list when the interior is built. BeginPlay may place the starters first; the list passed into the interior replaces them when it differs.
- The company's stronghold starts at size Meagre and type Dungeon. Its entrance is `SolidStrongholdGrid::MeagreEntranceCell`, and its floor is the fourteen hexes, minus that entrance and the captain's cell. A tower's ground-floor entrance is `SolidTowerStronghold::DoorHexCenter`. The captain's hex is the cell under his pawn when the mode is on. Stairs, landings, and hexes the tower plan already rejects stay unavailable.
- The chest the company already has keeps `FGameState::CaveChest`. This story does not give a stronghold a second chest. [Stronghold transition](stronghold-transition.md) still seats the list on the new ground floor. [Add furniture](add-furniture.md) is the browser and the small table.
