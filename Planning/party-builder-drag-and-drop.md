# Party builder drag and drop

## Story

The player opens Build party and arranges the party by dragging, the same way items move in the inventory. The page is the oak-and-bronze board. Companions who are not with the captain sit under Company. The party is a column of boxes, one for each companion the events allow. Press a box that holds a companion and the name follows the pointer. Release it on a party box. An empty box takes that companion. An occupied box swaps. Release a party companion on Company and that box is left empty. Release on the same box, or on empty space, and nothing moves.

A new game, once the page can open, shows Sam in the first box and Alex in the second. The boxes after those stay empty until the cap rises. The companions who walk with the captain are the filled boxes from top to bottom. An empty box is skipped, so it is a gap the player can drop into.

The same page arranges battle plans. Plans that are not on a key sit under Plans. Eight boxes are labeled F1 through F8. Press a plan and the name follows the pointer. Release it on a key. An empty key takes that plan. An occupied key swaps. Release a plan on Plans and that key is left empty. A plan sits on only one key. Dropping a plan does not select it.

F1 through F8 still choose the active plan while the page is closed. An empty key does nothing. The corner list shows the eight keys, and the mark follows the active plan when that plan is dragged to another key. The corner list is not itself a drag target.

## Acceptance

- Build party stays locked until the events allow two companions. Esc or F10 closes the page and does not open the menu. Up, Down, and Enter do not change the party or the plans.
- The page has a Company list and a column of party boxes. The column has one box per companion the events allow. The title count is the number of filled boxes.
- A new game shows Sam, then Alex. The other companions stay in Company, in company order: Max, Morgan, Quinn.
- Each label keeps the vocation initial, as in Sam (R).
- Pressing a filled box shows that name on the pointer until release.
- Releasing on an empty party box places the companion there. Releasing on an occupied party box swaps the two. Releasing on Company removes a party companion and leaves that box empty.
- Releasing on the same box, or off any box, leaves the page as it was.
- The party never holds more companions than the events allow. A full column can still swap, and can still drop a companion back on Company.
- The companions with the captain are the filled boxes from top to bottom. An empty box is skipped. After a drag, that order is who walks with him.
- The captain's own gear is not on this page.
- The same page has a Plans list and eight key boxes, F1 through F8.
- A new game puts Line, Column, Tight mob, Loose mob, and Parade on F1 through F5. F6, F7, and F8 are empty, and Plans is empty.
- A plan moves onto an empty key, swaps with the plan on an occupied key, or returns to Plans and leaves that key empty. The same plan is never on two keys.
- Releasing a plan on its own box, or off any box, leaves the keys as they were. The drop does not make that plan active, and a Parade plan does not fire its events on the drop.
- While the page is closed, F1 through F8 select those keys. An empty key does nothing. A key whose plan is Parade fires the parade events, then becomes the active plan, whichever key that is.
- Dragging the active plan onto another key moves the corner mark with it. Dragging it back to Plans leaves no plan active until a filled key is pressed. The companions keep the formation they had.
- The corner list shows the eight keys, including the empty ones, and is not a drag target.
- Test Drill still selects F1, then F2, then F3, then F4. An empty key on a leg is the same as pressing that empty key.
- Move, look, and the battle-plan keys wait while the page is open.

## Implementation notes

- Today Build party is a checkbox list. `SolidPartyBuilder::CollectRows` prefixes `[x]` or `[ ]`, and `Toggle` adds or removes a company index. `ASolidCharacter::TogglePartyBuilderRow` is the click and the Enter key. This story replaces that gesture. The highlight index and the up/down wrap go away with it.
- `CanOpen` stays. The menu row stays dim below two companions.
- `SelectedCompanionIndices` is a packed list. An empty party box is a hole, so the stored choice has to keep that gap. `CollectFieldedCompanions` already skips a missing record. Fielding walks the boxes from top to bottom and skips the empty ones. It still stops at the event cap.
- Company order is `USolidCompany` order, not a player order. The Company list is not rearranged. A companion in a party box is left out of it.
- The inventory drag is press, a name on the pointer, and release. `SolidInventory::Move` swaps when both sides hold something, and the same place or a miss does nothing. Esc and closing the page cancel the drag. Use that gesture here. Do not add a new pointer.
- `AssignedCompanyPlanIndices` is packed, and `SetAssignedBattlePlans` drops holes. F6 through F8 are empty only because they are past the end. This story needs an empty key between filled keys, so the eight boxes are fixed slots. `SelectAssignedSlot` on an empty slot returns false.
- The catalog is Line, Column, Tight mob, Loose mob, and Parade, in that order. Do not add a plan. A plan leaves the Plans list while it sits on a key.
- Parade events are `FireParadeEvents`, called from `SelectBattlePlanSlot` when that slot's formation is Parade. The drop must not call it. Selecting the key still does, including a Parade plan that the player has moved off F5, and including Test Drill when a drill leg selects that key.
- The corner panel is `DrawBattlePlansPanel`. It already draws eight lines and an em dash for an empty slot. Keep it as a readout.
- `SolidCore1.PartyBuilder.Toggle` assumes the checkbox list, `Toggle`, and `IsChoiceAvailable`. Replace that test in the same commit as the drag, and say so. `CanOpen` stays covered. The suite line in `README.md` still says the builder toggles the company. Change that sentence in the same commit.
- The battle-plan tests pin the starter assignment and that selecting F5 fires the parade events once. Those tests keep passing while the starter assignment is unchanged. A test that Parade fires from another key, and a test that a key in the middle can be empty, belong in the same commit as the slots.
