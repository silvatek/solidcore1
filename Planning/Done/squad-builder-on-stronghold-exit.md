# Open the squad builder on leaving a stronghold

Status: Done

## Story

The player leaves a stronghold and is standing on the grass outside it. Build squad opens when there is something to arrange. The cave leaves by a click on the dark opening. The tower leaves by a click inside. Every stronghold uses that same check. The page is the same Build squad page the menu opens. The squad is the one they already had. Nothing is added or removed until the player changes it.

Nothing to arrange means the events allow no companion and every battle plan is one the company starts with: Line, Column, Tight mob, Loose mob, and Parade. The page stays closed. The leave still happens. A companion the events allow, or a battle plan beyond those five, opens the page.

Entering a stronghold does not open the page. Leaving the Iglin tunnels does not open it. A click that does not leave does not open it.

Esc or F10 closes the page and does not open the menu. The player can open Build squad from the menu again afterward.

## Acceptance

- Leaving the Meagre cave opens Build squad once the captain is on the grass outside the mouth, when there is something to arrange. The click on the dark opening does this.
- Leaving a tower opens Build squad once the captain is on the grass outside that door, on the same check. The stronghold type does not change the decision.
- The journal still records Exit Stronghold for that leave.
- The open page is Build squad. Esc or F10 closes it and does not open the menu. Up, Down, and Enter behave as they do when the menu opened the page.
- The squad on that page is the squad from before the leave. Sam and Alex stay selected when they were selected. No companion is added or removed by the leave itself.
- The page stays closed when the events allow no companion and the catalog is only those five starter plans. A leave still returns the captain to the grass.
- One companion allowed opens the page even while those five plans are the whole catalog. A plan beyond those five opens the page even when no companion is allowed.
- The menu row still stays dim below two companions. That lock is the menu, not this leave.
- Entering the cave, or entering a tower, does not open the page.
- Leaving the Iglin tunnels does not open the page. The journal line for that leave stays Exit Iglin tunnels.
- A click on an entrance the company has left does not open the page, and does not enter.
- Closing the page does not open it again until the next leave. F10, then Build squad, still opens it.

## Implementation notes

- `TravelToWorld` stands the captain at the cave mouth. `TravelFromTower` stands him outside the door. Both call `OpenLevel`, so the page cannot open inside the travel call. Open it once the open world is back and he is standing outside.
- The check is `HasAnythingToArrange`. It does not take a stronghold type. Cave and tower both call `OpenAfterStrongholdLeave`.
- `bReturnToLairMouth` is the tunnel return. That path does not call the offer.
- `CanOpen` stays for the menu row. Below two companions the row stays dim. The leave does not use that lock.
- The cave leave is the click on the dark opening. [Reorder the game menu](reorder-game-menu.md) removes the menu row, so there is no F10 Leave cave. The tower leave is the interior click, which calls `TravelFromTower`.
- This story does not change how the page arranges the squad. [Squad builder drag and drop](squad-builder-drag-and-drop.md) is that page.
- A test sets the squad cap and the catalog, then asserts the page opens or stays closed, without PIE. Cover no companions with the starter plans, one companion, a plan beyond those five, and a closed page that stays closed. The existing travel tests do not assert that the page stays closed.
