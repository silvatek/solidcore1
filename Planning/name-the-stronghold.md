# Name the company's stronghold

Status: Ready

## Story

The player gives the company's stronghold a name. Until they choose one, the name is the size and the type, as in Average Tower. The stronghold the company starts with is size Meagre and type Dungeon, and its name is The cave.

A name sign stands on the grass outside the stronghold, next to the entrance. The board shows the current name. The player clicks the sign and types a name, then presses Enter. That name is kept. Esc leaves the name as it was. The F10 menu has no row for this.

## Acceptance

- A new game's stronghold name is `The cave`. Its size is still Meagre and its type is still Dungeon. `SolidStronghold::Label` of that stronghold is still `Meagre Dungeon`.
- Until the player chooses a name, another stronghold is named by its size and its type, as in `Average Tower`.
- One name sign stands on the grass beside the company stronghold's entrance, clear of the opening and of the spot where the captain stands. The board faces the grass. It shows the current name on one line.
- The sign is there while the mouth is shut, and after the mouth is open. A click on the sign does not enter. A click on the opening does not open the name.
- Clicking the sign opens a page. The title is the current name. Enter keeps the typed name, trimmed, and closes the page. The board then shows that name. Esc closes the page and keeps the previous name. A name that is empty after trimming is not kept.
- The F10 menu has no row for the name. Credits stays last without this row.
- A kept name is at most 24 characters. Anything past that is dropped.
- A kept name stays when the company moves to another size or type. It stays across a cave trip and a tower trip.
- With no kept name, a move away from the starting stronghold drops `The cave`. The name becomes the new size and type.
- With no kept name, the name is `The cave` only while the company stronghold is still the starting Meagre Dungeon.
- A stronghold the company has left has no name sign. After a move, the sign stands beside the new entrance.
- The journal lines stay `Enter Stronghold` and `Exit Stronghold`. The pointer tips stay `Enter cave` and `Enter Stronghold`.

## Implementation notes

- `SolidStronghold::Label` is the size and the type. Do not special-case it for the starting stronghold. `SolidCore1.Stronghold` already expects `Meagre Dungeon`. Leave that test.
- Store the kept name on `FGameState`. Empty means the player has not chosen. The travel session already copies `Configuration`, so a trip keeps the name.
- The default is `The cave` when the active stronghold is `SolidStronghold::Starting` on `StartingCell()` and the kept name is empty. Otherwise the default is `Label`.
- There is no text entry on a page yet. Add one for this page. While it is open, letters, digits, space, and Backspace edit the line. Enter and Esc behave as the acceptance says.
- The page uses the oak board the other menu pages use.
- The sign uses the town sign's pole and board: a thin dark-brown pole, a flat light-brown board, white letters with a black border. One line, the name. Not "Welcome to".
- Place it a step to the right of the frame, on the grass. The cave mouth is `SolidStrongholds::FEntrance`. A tower uses the door that faces that entrance's grass. The board's face points at the grass, the way the entrance's local +X does. It does not cover the opening or the standing spot.
- A click hit on the pole or the board opens the page. The mouth and the tower door keep their own hits.
- This story does not add an F10 row. [Reorder the game menu](reorder-game-menu.md) does not gain one from here.
- The product commit adds the name and sign tests. It does not edit the menu order tests.
