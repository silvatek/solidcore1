# Exit the game

Status: Ready

## Story

The player opens the F10 menu and chooses Exit Game. The game asks whether to exit. Yes closes the game. No leaves it running.

The game runs in PIE for now. Yes also closes Unreal Editor when that can be done.

Exit Game is the last row. Credits is the row before it. The last two rows are always Credits, then Exit Game. A later row goes before Credits. [Reorder the game menu](reorder-game-menu.md) uses that order.

## Acceptance

- The row is labeled Exit Game. It is the last row. Credits is the row before it. The last two rows are always Credits, then Exit Game. A later row goes before Credits.
- Choosing Exit Game opens a confirmation and does not close the game. The page asks `Exit the game?` The rows are Yes and No. Yes is selected.
- Up and Down move between Yes and No and wrap. Enter confirms the selected row. A click on a row selects it and confirms it. Key 1 confirms Yes. Key 2 confirms No.
- Yes closes the game.
- No, Esc, and the menu key close the confirmation and leave the game running. They do not close the game. The F10 menu is not left open.
- The confirmation is not an event message. It writes nothing in the journal.
- When the game is running in PIE, Yes also closes Unreal Editor if the editor can be closed. If it cannot, PIE still ends.
- A game that is not running in the editor only closes the game.
- Choosing any other menu row does not ask this question and does not close the game.

## Implementation notes

- Add an Exit Game item and a quit action. Confirming that row opens the confirmation. It does not quit yet. The page is a choice list, the same click and Enter path as the portal list.
- Do not use `ShowMessage`. Closing a message calls `NoteMessageClosed` and can write the journal.
- Esc and the menu key cancel the page the way they close the portal list. The main menu stays closed. Cancel does not call the quit.
- `UKismetSystemLibrary::QuitGame` ends play. In PIE that returns to the editor and leaves the editor open. After that quit is requested, and only while `WITH_EDITOR` is set, ask the editor to close when the engine can (`UUnrealEdEngine::CloseEditor`, or the same request if that call is named differently). End play first. If the editor close cannot be requested, the quit still runs. A non-editor build does not compile that call.
- A test does not call the real quit or the real editor close. It checks that Yes asks to quit, that a PIE yes also asks to close the editor, and that a non-editor yes does not. No and Esc leave the game running.
- [Reorder the game menu](reorder-game-menu.md) describes the prior menu-list test commit. That commit is on main before this story's product commit. This product commit does not edit those tests. It adds the Exit Game, confirmation, and quit-request tests. Those tests find the row by the label Exit Game.
- This story does not bring back Test Drill, Inventory, or the cave row.
