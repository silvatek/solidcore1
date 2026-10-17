# Open the party builder on leaving a stronghold

Status: Ready

## Story

The player leaves a stronghold and is standing on the grass outside it. Build party is open. The den leaves by a click on the dark opening, or by F10 Leave cave. The tower leaves by a click inside. The page is the same Build party page the menu opens. The party is the one they already had. Nothing is added or removed until the player changes it.

The page stays closed when the events still allow fewer than two companions. The leave still happens. Entering a stronghold does not open the page. Leaving the Iglin tunnels does not open it. A click that does not leave does not open it.

Esc or F10 closes the page and does not open the menu. The player can open Build party from the menu again afterward.

## Acceptance

- Leaving the Meagre den opens Build party once the captain is on the grass outside the mouth. The click on the dark opening does this, and F10 Leave cave does this.
- Leaving a tower opens Build party once the captain is on the grass outside that door.
- The journal still records Exit Stronghold for that leave.
- The open page is Build party. Esc or F10 closes it and does not open the menu. Up, Down, and Enter behave as they do when the menu opened the page.
- The party on that page is the party from before the leave. Sam and Alex stay selected when they were selected. No companion is added or removed by the leave itself.
- The page does not open when the events allow fewer than two companions. A leave at that cap still returns the captain to the grass, and Build party stays locked.
- Entering the den, or entering a tower, does not open the page.
- Leaving the Iglin tunnels does not open the page. The journal line for that leave stays Exit Iglin tunnels.
- A click on an entrance the company has left does not open the page, and does not enter.
- Closing the page does not open it again until the next leave. F10, then Build party, still opens it.

## Implementation notes

- `TravelToWorld` stands the captain at the dungeon mouth and says the party builder stays closed. `TravelFromTower` stands him outside the door and does not open it either. Both call `OpenLevel`, so the page cannot open inside the travel call. Open it once the open world is back, when `bReturnToDungeonMouth` or `bReturnToTowerDoor` is set.
- `bReturnToLairMouth` is the tunnel return. Leave that path closed.
- `CanOpen` stays. Below two companions, skip the open. The menu row stays dim for the same reason.
- F10 Leave cave is the den only. In the tower or the tunnels that menu row returns without traveling. The tower leave is the interior click, which calls `TravelFromTower`.
- This story does not change how the page arranges the party. [Party builder drag and drop](party-builder-drag-and-drop.md) is that page.
- A test can set the session return flag and the party cap, then assert the page opens or stays closed, without PIE. Cover a den return, a tower return, a tunnel return, and a cap below two. Say so if an existing travel test assumed the page stays closed.
