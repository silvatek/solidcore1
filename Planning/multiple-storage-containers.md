# Multiple storage containers

Status: Ready

## Story

The player has more than one storage container in a stronghold. Each container keeps its own items. The player opens one container and sees that container's slots. While the inventory panel is open, the player cycles through every storage container in that stronghold. The page shows one of them at a time.

The chest the company already has still starts with the rusty sword, the old boots, 20 coins, five sticks, and three pitch. Its last cell is empty. A new game still places that one chest, with the fireplace and the bed. A second storage piece on the stronghold's list is another container, and it starts empty.

[Basic furniture management](basic-furniture-management.md) places at most one chest. This story does not raise that limit, and it does not add a furnishing kind.

## Acceptance

- A new game's stronghold still has one chest. Its container is `FGameState::CaveChest`, filled by `SolidStorage::StartingCaveChest`. The cells are the rusty sword, the old boots, 20 coins, five sticks, three pitch, and one empty cell. The fireplace and the bed are not containers.
- Every container is `SolidStorage::Columns` by `SolidStorage::Rows`.
- The inventory page shows at most one container, to the right of the pack. It never shows two grids.
- While the captain is inside the company stronghold and that stronghold has one or more storage pieces, `/` shows the first of those pieces. The first is the first storage piece on the furnishings list. View and Create do the same. With one piece, that is the starting chest.
- A click on a storage piece, while the inventory is closed, opens the inventory on that piece and does not start a walk. A click that misses it still walks. A world click does not reach a piece while the inventory is open.
- A second storage piece is another chest on the furnishings list, on a free hex of a floor in that stronghold. Its container starts empty. The starting chest's cells do not change when that piece is added.
- While the inventory is open, Page Down moves to the next storage piece on the furnishings list, and Page Up moves to the previous. From the last, Page Down returns to the first. From the first, Page Up returns to the last. Key repeat takes another step. Every storage piece in the company stronghold is in that cycle, on every floor. LB and L1 match Page Up. RB and R1 match Page Down.
- The heading row carries a previous mark and a next mark when the stronghold has two or more storage containers. The marks sit on that row and do not cover a slot. A click on the previous mark matches Page Up. A click on the next mark matches Page Down. With one container the marks are absent, and Page Up and Page Down leave that container open.
- The heading is `Chest` when one container is open and the stronghold has one storage piece. When the stronghold has more, the heading is `Chest`, a space, and the place in the cycle, starting at 1, as in `Chest 2`.
- A drag writes the container that is open. The other container's items and counts stay. The pack and the worn boxes change only by that drag. A cycle while a drag is held cancels the drag. The item stays where it was, and the page shows the container the cycle landed on.
- Taking the rusty sword from the starting chest into the second container empties the sword's cell in the starting chest and puts the sword in the second. The other starting cells stay. Page Up then shows the starting chest without the sword, and Page Down shows the second container with the sword. Moving the sword back restores that cell and empties the second.
- A click on the fireplace or the bed does not open a container and does not change which container is open.
- Closing the inventory forgets the place in the cycle. Leaving the stronghold forgets it too. The next `/` shows the first storage piece again.
- Outside the stronghold, in the Iglin tunnels, and in a stronghold the company has left, the container grid stays off. Page Up, Page Down, and the shoulder buttons do not open a container there. A click there does not open one.
- While a menu, the journal, a message, or the party builder is open, a click does not open a container. While arrange mode is on, a click does not open one either. This story does not add arrange mode.
- `ClearInventory` still clears worn boxes, the pack, and the crafting slots. It does not clear a container.
- A move to another stronghold keeps every container: the same items, in the same slots, with the same counts, on the same piece. The pack stays with the captain.
- A cave trip and a tower trip keep those containers. Coming back shows the same slots in the piece the player opens.
- This story does not add a catalog kind, does not place a second chest in a new game, and does not add an F10 row.

## Implementation notes

- `FGameState::CaveChest` stays the starting chest. `SolidCore1.Inventory.CaveChest`, `SolidCore1.Stronghold.Move`, and the forage check that the cave chest is unchanged keep reading it. The product commit does not edit those tests.
- `SolidStrongholdFurniture::FUsed` gains `StorageId`. `INDEX_NONE` is not storage. `0` is `CaveChest`. `StartingFurnishings` sets the chest to `0` and leaves the fireplace and the bed at `INDEX_NONE`. `SamePieces` compares item, level, cell, and facing, so the new field does not change that test.
- Further containers live in `FGameState::MoreContainers`. `StorageId` 1 is `MoreContainers[0]`, and the next id is the next entry. A second chest appended to `Furnishings` gets the next id and an empty `FContainer`. Resolving an id past that list, or `INDEX_NONE`, yields no container.
- The spawned furnishing remembers its `StorageId`. The click ray that hits the mouth and the forage piles hits a storage mesh first. A hit with a real id opens the inventory on that container. A miss falls through to the walk.
- `CompanyChestOpen` stays the inside check: the company cave, or the company tower the captain is in. The grid is that check, and a container is selected. `/` selects the first storage piece on the furnishings list. A click selects the piece that was hit. Page Up and Page Down step that selection through every storage piece in the stronghold and wrap. The selection clears when the inventory closes or the captain leaves.
- Page Up and Page Down already call `MoveCharacterEditor`, which returns while the character editor is closed. While the inventory is open, those keys cycle containers and do not move the editor. LB and RB are not bound on their own. Battle-plan chords stay blocked while the inventory is open, because the inventory counts as a menu overlay.
- The marks are pointer hits on the heading row. They are drawn only when the cycle has two or more containers. The product commit adds the Page Up, Page Down, LB, RB, L1, and R1 rows to the Inventory table in `Content/Controls.md`.
- `Layout` still takes one `FContainer*`. A null pointer is the page with no container grid. `MoveCarriedItem` writes that same pointer.
- `FGameState` is already the travel session's `Configuration`, so `CaveChest`, `MoreContainers`, and `Furnishings` survive `OpenLevel` together. `Commit` keeps the extra containers because it does not clear them. `PlaceOnFloor` edits cells and does not swap pieces, so an id stays on its piece.
- [Basic furniture management](basic-furniture-management.md) still stops at one chest. A chest it adds where the stronghold had none uses the next empty container from this store. A click while arrange mode is on does not open one.
- The product commit adds the second-piece tests: empty start, `/` opening the first, a click opening that piece, the cycle wrapping across every floor, the heading place, a drag that leaves the other container, a cycle that cancels a held drag, one container with no marks, and the containers surviving `Commit`. It does not edit the tests named above.
