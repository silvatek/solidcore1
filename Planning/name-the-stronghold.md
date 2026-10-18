# Name the company's stronghold

Status: Ready

## Story

The player gives the company's stronghold a name. Until they choose one, the name is the size and the type, as in Average Tower. The stronghold the company starts with is size Meagre and type Dungeon, and its name is The cave.

The player opens the F10 menu and chooses the row that shows that name. The row stands immediately before Credits. The page title is the current name. The player types a name and presses Enter. That name is kept. Esc leaves the name as it was.

## Acceptance

- A new game's stronghold name is `The cave`. Its size is still Meagre and its type is still Dungeon. `SolidStronghold::Label` of that stronghold is still `Meagre Dungeon`.
- Until the player chooses a name, another stronghold is named by its size and its type, as in `Average Tower`.
- The menu row immediately before Credits shows the current name. Credits stays last. The row is on the menu in the open world and inside the stronghold.
- Choosing that row opens a page. The title is the current name. Enter keeps the typed name, trimmed, and closes the page. Esc closes the page and keeps the previous name. A name that is empty after trimming is not kept.
- A kept name is at most 24 characters. Anything past that is dropped.
- A kept name stays when the company moves to another size or type. It stays across a cave trip and a tower trip.
- With no kept name, a move away from the starting stronghold drops `The cave`. The name becomes the new size and type.
- With no kept name, the name is `The cave` only while the company stronghold is still the starting Meagre Dungeon.
- A stronghold the company has left is not given this name.
- The journal lines stay `Enter Stronghold` and `Exit Stronghold`. The pointer tips stay `Enter cave` and `Enter Stronghold`.

## Implementation notes

- `SolidStronghold::Label` is the size and the type. Do not special-case it for the starting stronghold. `SolidCore1.Stronghold` already expects `Meagre Dungeon`. Leave that test.
- Store the kept name on `FGameState`. Empty means the player has not chosen. The travel session already copies `Configuration`, so a trip keeps the name.
- The default is `The cave` when the active stronghold is `SolidStronghold::Starting` on `StartingCell()` and the kept name is empty. Otherwise the default is `Label`.
- There is no text entry on a menu page yet. Add one for this page. While it is open, letters, digits, space, and Backspace edit the line. Enter and Esc behave as the acceptance says. Other menu keys do not confirm a different row.
- The page uses the oak board the other menu pages use.
- [Reorder the game menu](reorder-game-menu.md) leaves this row immediately before Credits. This story does not put a row after Credits, and it does not bring back Test Drill, Inventory, or the cave row.
- The product commit adds the name tests. The menu tests that still name rows by the old positions are the ones [Reorder the game menu](reorder-game-menu.md) loosens first. Do not edit those tests again here.
