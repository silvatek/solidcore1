# Add furniture

Status: New

## Story

Do not start this story until [Basic furniture management](basic-furniture-management.md) is Done.

The player is arranging furniture in the company stronghold. Confirm on an empty hex that can hold a piece, while nothing is held, opens the furnishing browser. The browser is a column beside the floor, listing the catalog in catalog order, and it stays off the hexes. It is closed when arrange mode starts. The wheel over the column scrolls it. Up and Down move through the rows, and a click on a row lifts that kind onto the hex that opened the browser, when the stronghold still has room for that kind. A fireplace and a chest stop at one each. A bed stops at three. The column closes while the piece is held, so the wheel can turn it and the floor stays clear. Esc closes the browser and leaves the mode on. The catalog will hold dozens of kinds. The three in this story fit, so the scrollbar stays hidden.

A new chest starts empty. The chest the company already has keeps its items. Putting the new piece down writes it onto the stronghold's list, so leaving and coming back shows it with the pieces already there. This story does not add a fourth kind.

## Acceptance

- Confirm on an empty hex that can hold a piece, while nothing is held, opens the browser for that hex. Confirm on a blocked hex does not open it. Confirm on an empty hex while a piece is held tries to put that piece down, and does not open the browser.
- The stronghold's entrance hex is blocked. On a tower, that is the ground-floor hex nearest the door. The hex the captain is standing on is blocked. A blocked hex does not open the browser, and it does not take a piece. A bed is blocked when either of its hexes is blocked.
- Each catalog kind has a limit. Fireplace is 1. Bed is 3. Chest is 1. The count is how many of that kind the stronghold holds, on every floor, including a piece that is held. The company's stronghold has its own count. The new stronghold counts on its own once the company has moved.
- The company's stronghold holds one fireplace, one bed, and one chest, so two beds can still be added there. Until the pieces move with the company, the new stronghold has none, so it can take one fireplace, three beds, and one chest.
- A row at its limit stays in the column. A click or Enter on it does not lift a new piece. Lifting a piece that is already down still works, and it does not raise the count.
- A chest added where the stronghold had none starts empty, two columns by three rows. The chest the company already has keeps its items when it is moved. The limit stops a second chest in that stronghold.
- The browser is closed when arrange mode starts, and it stays closed until an empty hex that can hold a piece is confirmed.
- Confirm on another empty hex that can hold a piece, while the browser is open, keeps the browser open and uses that hex instead. A blocked hex leaves the remembered hex as it was.
- The browser is a column of every kind in the furnishing catalog, in catalog order. This story's rows are Fireplace, Bed, and Chest. The column sits beside the floor and does not cover a hex.
- The column shows as many rows as fit in its frame. Further kinds scroll. A scrollbar shows when the rows do not fit, and it stays hidden when they do. Fireplace, Bed, and Chest fit, so the thumb is hidden.
- Up and Down move the highlight through the catalog while the browser is open, and the column scrolls to keep that row in the frame. A click on a row lifts that kind as a held piece, snapped to the hex that opened the browser. Enter does too, unless the pointer is over a placed piece. Up and Down do nothing while the browser is closed.
- Enter or a click on a placed piece still lifts that piece, and closes the browser if it was open. The catalog highlight does not change which placed piece a click lifts.
- Lifting a piece closes the browser. Putting the piece down, or putting it back, leaves the browser closed. A click where the column was does nothing while it is closed.
- Esc while a piece is held returns a piece that was already down, and discards a piece that was lifted from the browser and not yet put down. The mode stays on. The browser stays closed.
- Esc while the browser is open, and nothing is held, closes the browser. The mode stays on.
- Toggling the mode off, by the sign or by `\`, returns a held piece, discards a new piece that was never put down, and closes the browser.
- While the browser is open, the wheel over its column scrolls that column only, and the wheel elsewhere does not zoom. While no piece is held and the browser is closed, the wheel does nothing. Outside arrange mode the wheel zooms as it does now.
- A click on the Arrange furniture sign does not open the browser. It still leaves the mode.
- A new game still starts with one fireplace, one bed, and one chest. This story does not add a fourth kind.
- A new piece that is put down is on the stronghold's list, with its hex and facing. Leaving and coming back shows it with the other pieces. A new game has none of those added pieces.
- Moving a chest keeps the items in its slots. The pack and the worn boxes stay as they were.
- This story binds Up and Down for the catalog. No number key lifts a kind. Gamepad buttons stay as they are.

## Implementation notes

- The browser opens on an empty-hex confirm and remembers that hex. A catalog row builds a pending piece on it and does not write the placed list until the placement confirm. Esc drops that pending piece and does not reopen the browser. The column is one scrollbox. Its rows are `SolidStrongholdFurniture::Catalog` in order. `FItem` gains `Limit`. Fireplace is 1, Bed is 3, Chest is 1. Count matching `ItemIndex` values in that stronghold's placed list, and count a held piece once. This story does not add a kind, and it does not add a filter. A later kind is another row in that catalog, with its own limit, and the same column scrolls to it.
- A chest added where the stronghold had none takes an empty container from [Multiple storage containers](multiple-storage-containers.md). The chest the company already has keeps `FGameState::CaveChest`. This story does not give a stronghold a second chest. [Stronghold transition](Done/stronghold-transition.md) moves the placed list, including beds added here, and each container stays on its piece.
- [Basic furniture management](basic-furniture-management.md) already writes a move of a piece that is down. This story adds a row to that same list when a new piece is put down.
