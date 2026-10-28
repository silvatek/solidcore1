# Menus own their pages

Status: New

## Story

A developer separates the menu pages from the HUD. Each page lives with that menu. `DrawHUD` paints the open page and does not own it.

The pages are the main menu, the portal, the exit confirmation, the squad builder, the credits, the character editor, the journal, the inventory, the stronghold name page, and the stronghold site page.

The stronghold site offer stays with the stronghold. It lists the unlocked sizes, disables a size smaller than the company's current stronghold, and lists a type when a listed size supports it. Confirm and Cancel belong to the site page. The captain still opens that page at a site, confirms the picked size and type, and cancels to leave.

The site page's keys follow the two lists and the buttons. Up and Down, and the number keys, no longer walk sizes, then types, then Confirm and Cancel as one list. A click still selects a size or a type. A pointer move still leaves the pick alone.

## Acceptance

- The main menu, the portal, the exit confirmation, the squad builder, the credits, the character editor, the journal, the inventory, the stronghold name page, and the stronghold site page each own their page. `ASolidHUD` does not hold that page's open flag, cursor, or picks.
- `DrawHUD` paints whichever of those pages is open.
- The debug readout, the battle-plan list, the battle board, the furnishing column, and the sight chip stay as they are.
- The stronghold offer still lists unlocked sizes, disables a size smaller than the company's stronghold, and lists a type when a listed size supports it. Confirm and Cancel are part of the site page, not rows of that offer.
- At a stronghold site the captain still opens the site page, Confirm still builds the picked size and type, and Cancel still leaves.
- The site page's keys follow the size list, the type list, and the buttons. Up and Down, and the number keys, do not walk those as one list. A click selects a size or a type. A pointer move leaves the pick alone.
- In Play In Editor the captain opens the main menu, and opens a stronghold site page that shows the size list, the type list, Confirm, and Cancel.

## Implementation notes

`SolidStrongholdTransition::Consider` in `Source/SolidCore1/Strongholds/SolidStronghold.cpp` is the offer. Today it also appends Confirm and Cancel as `FSolidStrongholdChoice` rows, and `ASolidHUD` holds the cursor, the picked size, the picked type, and `DrawStrongholdBoard`.

The squad builder already keeps its layout in `Source/SolidCore1/Menus/SolidSquadBuilder.h`. The main menu, the portal, and the exit confirmation already keep their lists under `Menus/`. Their open flags and cursors still sit on `ASolidHUD`.

SC1-0726 keeps the site pick until a click. Up and Down, and the number keys, still walk the one choice array. Which keys follow the two lists was not chosen.
