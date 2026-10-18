# Reorder the game menu

Status: Ready

## Story

The player opens the game menu and finds five rows. They are Use portal, Journal, Build squad, Character editor, and Credits. Credits is the last row. A later row goes before Credits. Test Drill is not a row. [Drill](drill-battle-plan.md) is that battle plan. The stronghold name is not a row. [Name the company's stronghold](name-the-stronghold.md) is the sign beside the entrance.

Inventory is not on the menu. / still opens the inventory, and View on Xbox and Create on PlayStation still open it.

Enter cave is not on the menu. Leave cave is the same row, so it is not on the menu either. A click on the dark opening still enters the cave, and a click inside still leaves.

## Acceptance

- The menu rows, from the top, are Use portal, Journal, Build squad, Character editor, and Credits.
- The menu has no stronghold-name row.
- The menu has no Test Drill row.
- Credits is the last row.
- The menu has no Inventory row. / opens and closes the inventory. View and Create do the same. F10 closes the inventory and does not open the menu. / while the menu is open still opens the inventory and closes the menu.
- The menu has no Enter cave row and no Leave cave row, in the open world and inside the cave.
- A click on the cave mouth still enters when the cave is open and it is still the company stronghold. A click inside still leaves. The mouth still ignores a click while the cave is shut, and after the company has left it.
- Build squad stays dim until events allow at least two companions. Confirming that dim row still does nothing.
- Opening the menu selects Use portal. Down moves through the five rows and wraps from Credits to Use portal. Up from Use portal wraps to Credits.
- Keys 1 through 5 select and confirm those rows, in that order. Keys 6, 7, and 8 do nothing while the main menu is open. On the portal list and the squad builder, those keys still choose the row of that number when it exists.
- Each remaining row still does what it does now. Use portal opens the portal list. Journal opens the journal. Build squad opens the builder. Character editor opens the editor. Credits opens the credits page.

## Implementation notes

- `SolidMainMenu::CollectEntries` is the on-screen order. The enum comment says the same. Reorder the enum to match, and drop `TestDrill`, `Dungeon`, and `Inventory`. Drop `StartTestDrill`, `TravelDungeon`, and `ShowInventory`. `ToggleInventory` stays. [Drill](drill-battle-plan.md) is the square.
- `CanUseCaveRow` and the dimming of the cave row in `DrawMainMenuPopup` exist for that row. Remove them with the row. The click path is the passage check and the travel calls, not the menu.
- Keys 6, 7, and 8 call `ChooseMainMenuIndex` past the last row once the menu has five entries. That function already returns when the index is past `EntryCount`, so the main menu needs no new key code. Leave the portal and squad-builder uses of those keys.
- [Open the squad builder on leaving a stronghold](squad-builder-on-stronghold-exit.md) leaves the den by the click. [Return to the stronghold](return-to-stronghold.md) and [Imperious dungeon](imperious-dungeon.md) enter the Meagre cave by the click. [Enable a lair from an event](enable-lair.md) does not enter the tunnels from the menu. Those stories no longer ask for this row.
- The menu label is Build squad. This story keeps that label.
- The suite lines in `README.md` for `SolidCore1.MainMenu.*` name the old eight rows and the dim cave row. Update them in the product commit.
- Tests that assume the old list change before the product commit, so they pass on the current code and on this order. The product commit does not edit those tests. It adds the assertions for this order.
  - `SolidCore1.MainMenu.Entries` and `SolidCore1.MainMenu.Hud` name rows by the old positions, including Credits as the fourth row, Build squad as the fifth, and Enter cave as the sixth. Make them follow `CollectEntries`: the highlight steps through that list, and wrap follows its length. They do not require a Test Drill row, an Inventory row, or a cave row, and they do not forbid one.
  - `SolidCore1.Inventory.Menu` requires an Inventory row after the character editor, at index 7, with action `ShowInventory`. Drop those assertions. Keep a check that still passes while the character editor is listed. The slash, View, and Create checks live in `SolidCore1.Inventory.Toggle` and stay.
  - `SolidCore1.MainMenu.Cave` only checks `CanUseCaveRow`. Stop calling it in that prior commit, so the product commit can delete the function without editing the test.
- `SolidCore1.CharacterEditor.Menu` finds the editor by item and wraps from that index. It already passes on both lists. Leave it.
- `SolidCore1.MenuPointer` selects index 2 and does not read the label. Leave it. The third row here is Build squad.
