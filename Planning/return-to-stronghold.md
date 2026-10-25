# Return to the stronghold

Status: Ready

## Story

Closing the monster message enables the Meagre size and returns the captain to the company stronghold. That stronghold is the cave. `EnableCave` is not the change that does this. The close uses `EnableStrongholdSize(ESolidStrongholdSize::Meagre)` and a new `ReturnToStronghold` change. Enter Dragongate no longer enables Meagre. The other sizes stay on that event until each has its own.

## Acceptance

- `IdKilledByMonsters` carries `EnableStrongholdSize(ESolidStrongholdSize::Meagre)`, `ReturnToStronghold`, and `ClearInventory`, in that order. It does not carry `EnableCave`.
- `EnableCave` is gone. A search of `Source` does not find that name. `bEnterCave` and `bCaveEnabled` are gone with it.
- Showing the monster message does not enable Meagre and does not travel. Reach broken bridge does not either.
- Before that close, Meagre is locked. The cave mouth ignores a click.
- The close enables Meagre. While the cave is still the company stronghold, the mouth accepts a click.
- `ReturnToStronghold` sends the captain into the company stronghold. On this close, that is the cave. After the company has moved to a tower, the same change enters that tower. A second close does not travel again.
- Once the company has left the cave, the mouth stays shut. Meagre stays enabled.
- Enter Dragongate does not list Meagre. It still lists Humble, Average, Impressive, Grand, Stately, and Imperious. F5 Parade does not enable Meagre and does not open the cave.
- Enter Relion still enables Humble. Enter Kanfold still enables Average.
- The journal for that close records `Meagre size unlocked`, `Returned to the stronghold`, and `Inventory cleared`. It does not say `Cave enabled`.
- Driven from the village still requires `IdKilledByMonsters`. The close still clears the inventory to Rags.
- This story does not add a cave row to the F10 menu.

## Implementation notes

- `EnableCave` sets `bCaveEnabled` and `FResult.bEnterCave`. `PresentEventResult` calls `TravelToDungeon` when `bEnterCave` is set. `TravelToDungeon` returns immediately unless `bCaveEnabled` is set.
- The mouth check is `CompanyCaveAcceptsEnter`. It reads `bCaveEnabled` and that the active stronghold is still the starting Meagre cave. After this story that check is Meagre unlocked, and the active stronghold still that cave. The menu dimming reads the same check until [Reorder the game menu](Done/reorder-game-menu.md) removes the row. This story does not add the row back.
- `ReturnToStronghold` is the trip. It replaces `bEnterCave`. While the company stronghold is the starting Meagre cave, it uses `TravelToDungeon`. When the active stronghold is a tower, it uses `TravelToTower` for that cell. A stronghold that is neither has no trip here. The travel runs after the changes, so Meagre is already unlocked when the cave trip checks the mouth.
- Killed by monsters already enables Meagre, beside `EnableCave` and `ClearInventory`. Enter Relion already enables Humble. Enter Kanfold already enables Average. Enter Dragongate lists every size, including Meagre.
- Enter Dragongate already lists `EnableStrongholdSize(Meagre)`. Remove that one line here. Leave the other six sizes on Dragongate, and leave Humble on Relion and Average on Kanfold.
- The journal phrase for the size is the one `SolidJournal::Describe` already builds from `SizeName`. The return phrase is `Returned to the stronghold`.
- `SolidCore1.Events.*` in `README.md` says closing the monster message opens the cave. Update that line in the product commit.
- Tests that name `EnableCave`, `bEnterCave`, `bCaveEnabled`, or `Cave enabled` change before the product commit, so they pass on the current code and on this behavior. The product commit does not edit those tests. It adds the assertions for `ReturnToStronghold`, the journal phrases, and Meagre missing from Dragongate.
  - `SolidCore1.Events.Cave`, `SolidCore1.Events.Message`, and `SolidCore1.Events.ClearInventory` read those fields and the `Cave enabled` line. After the close, assert through `CompanyCaveAcceptsEnter` and through the trip the result asks for. Do not require the old names.
  - `SolidCore1.Events.DragongateUnlocksSizes` and `SolidCore1.Events.EarlySizeUnlocks` treat Meagre as one of Dragongate's sizes. Keep the other six sizes. Do not require Meagre on Dragongate.
  - `SolidCore1.Lair.Gate` and `SolidCore1.Portal.TunnelMouth` read `bCaveEnabled` after the close or after the portal. Use `CompanyCaveAcceptsEnter` instead.
  - `SolidCore1.Stronghold.SiteOffer` and `SolidCore1.Stronghold.Move` set `bCaveEnabled` before they call `CompanyCaveAcceptsEnter`. Stop assigning that field. Run the monster close instead, then set up the chest and the pack. The close opens the cave today and unlocks Meagre after this story. The move still leaves the starting cave, so the check turns false.
