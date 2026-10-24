# Name the company's stronghold

Status: In Progress

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player gives the company's stronghold a name. Until they choose one, the name is the size and the type, as in Average Tower. The stronghold the company starts with is size Meagre and type Dungeon, and its name is The Cave.

A name sign stands beside the company stronghold's entrance. The board shows the current name. The player clicks the sign and types a name, then presses Enter. That name is kept. Esc leaves the name as it was. The F10 menu has no row for this.

## Stages

One phase. The name is the stronghold's existing name. The sign and the page ship with it, because the name is not on screen without the sign, and the sign cannot be changed without the page.

The sign's place is an attribute of the stronghold type. A size moves that place when the type's measurements change.

## Acceptance

- A new game's stronghold name is `The Cave`. Its size is still Meagre and its type is still Dungeon. `SolidStronghold::Label` of that stronghold is still `Meagre Dungeon`.
- Until the player chooses a name, another stronghold is named by its size and its type, as in `Average Tower`.
- One name sign stands beside the company stronghold's entrance, clear of the opening and of the spot where the captain stands. The board faces the grass. It shows the current name on one line.
- The sign is there while the mouth is shut, and after the mouth is open. A click on the sign does not enter. A click on the opening does not open the name.
- Clicking the sign opens a page. The title is the current name. Enter keeps the typed name, trimmed, and closes the page. The board then shows that name. Esc closes the page and keeps the previous name. A name that is empty after trimming is not kept.
- The F10 menu has no row for the name. The last two rows are Credits, then Exit Game.
- A kept name is at most 24 characters. Anything past that is dropped.
- A kept name stays when the company moves to another size or type. It stays across a cave trip and a tower trip.
- With no kept name, a move away from the starting stronghold drops `The Cave`. The name becomes the new size and type.
- With no kept name, the name is `The Cave` only while the company stronghold is still the starting Meagre Dungeon.
- A stronghold the company has left has no name sign. After a move, the sign stands beside the new entrance.
- The journal lines stay `Enter stronghold` and `Exit stronghold`. The pointer tips stay `Enter cave` and `Enter stronghold`.

## Implementation notes

- The name is `FSolidStronghold::Name`. There is no second field. `SolidStronghold::Label` stays the size and the type. Do not special-case it. `SolidCore1.Stronghold` already expects `Meagre Dungeon`. Leave that test.
- `The Cave` is the default only for the starting Meagre Dungeon, the one on `StartingCell()`. `InitializeStartingStronghold` writes that name. Every other stronghold starts as `Label`.
- A move copies `Name` onto the new stronghold only when it is not that default. The starting cave's `The Cave` is the default, so leaving it without a chosen name names the new stronghold from its size and type. Typing `The Cave` again on that cave is the same string, so a later move still drops it. Any other string, including the label of some other stronghold, is kept. The company owns the stronghold, so a cave trip and a tower trip keep the name.
- The page is the oak board the other menu pages use. While it is open, letters, digits, space, and Backspace edit the line. Shift and caps choose a letter's case. Digits ignore that. Enter keeps the trimmed name, at most 24 characters, and closes the page. Esc and F10 close it and leave the previous name. F10 does not then open the menu. An empty trim is not kept. A click on the sign does not enter. A click while the page is open does not enter.
- The sign uses the town sign's pole and board: a thin dark-brown pole, a flat light-brown board, white letters with a black border. One line, the name. Not "Welcome to".
- `NameSign()` is where that type puts the sign, in the exterior actor's centimetres. A dungeon stands beside the mouth frame. A tower stands beside the door. The pole is a step past that edge, then half the board, so the board clears the opening. The board faces the grass. A click on the pole or the board opens the page. The mouth and the tower door keep their own hits.
- The sign is spawned for the company stronghold only. A move loads the world again, and the sign stands at the new entrance. Renaming refreshes the board in place.
- This story does not add an F10 row. [Reorder the game menu](Done/reorder-game-menu.md) does not gain one from here.
- The product commit adds the name and sign tests. It does not edit the menu order tests.
