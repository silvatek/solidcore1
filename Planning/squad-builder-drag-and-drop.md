# Squad builder drag and drop

Status: Ready

## Story

The player opens Build squad and arranges the squad by dragging, the same way items move in the inventory. The page is the oak-and-bronze board, and the board stays that size. Companions who are not with the captain sit in a scrollbox under Company. A filter above that box reads All, Warrior, Rogue, Mage, Cleric. All is selected when the page opens. The box lists companions who are not in the squad and who match the filter, in company order. The squad is a column of boxes, one for each companion the events allow, and that column scrolls when the boxes are taller than the frame. It has no filter. Press a box that holds a companion and the name follows the pointer. Release it on a squad box. An empty box takes that companion. An occupied box swaps. Release a squad companion on the Company box and that box is left empty, even when the filter would hide the companion. Release on the same box, or on empty space, and nothing moves.

A new game, once the page can open, shows Sam in the first box and Alex in the second. The boxes after those stay empty until the cap rises. The companions who walk with the captain are the filled boxes from top to bottom. An empty box is skipped, so it is a gap the player can drop into.

The same page arranges battle plans. Plans that are not on a key sit in a scrollbox under Plans. A filter above that box reads All, Line, Column, Mob, Parade. All is selected when the page opens. The box lists plans that are not on a key and that match the filter, in catalog order. Tight mob and Loose mob both match Mob. Eight boxes are labeled F1 through F8. Those keys do not scroll, and they have no filter. Press a plan and the name follows the pointer. Release it on a key. An empty key takes that plan. An occupied key swaps. Release a plan on the Plans box and that key is left empty, even when the filter would hide the plan. A plan sits on only one key. Dropping a plan does not select it.

The wheel over a scrollbox moves that box only. A scrollbar shows when the rows do not fit, and it stays hidden when they do. The company will hold dozens of companions, and the catalog dozens of plans. The starter five still fit, so those thumbs stay hidden.

F1 through F8 still choose the active plan while the page is closed. An empty key does nothing. The corner list shows the eight keys, and the mark follows the active plan when that plan is dragged to another key. The corner list is not itself a drag target.

## Acceptance

- Build squad stays locked until the events allow two companions. Esc or F10 closes the page and does not open the menu. Up, Down, and Enter do not change the squad or the plans.
- The page has a Company scrollbox and a column of squad boxes. The column has one box per companion the events allow, and it scrolls when those boxes are taller than the frame. The title count is the number of filled boxes.
- A filter above Company reads All, Warrior, Rogue, Mage, Cleric. The page opens on All. Choosing one shows only that vocation, still in company order. All shows every companion who is not in the squad.
- A new game shows Sam, then Alex. The other companions stay in Company, in company order: Max, Morgan, Quinn. With All selected they fit, so the Company scrollbar is hidden.
- Each label keeps the vocation initial, as in Sam (R).
- Pressing a filled box shows that name on the pointer until release.
- Releasing on an empty squad box places the companion there. Releasing on an occupied squad box swaps the two. Releasing on the Company box removes a squad companion and leaves that box empty. The companion returns to the company even when the vocation filter hides them.
- The filter does not take anyone out of the squad, and it does not change who walks with the captain. A hidden row cannot be dragged. Changing the filter shows that row again.
- Releasing on the same box, or off any box, leaves the page as it was.
- The squad never holds more companions than the events allow. A full column can still swap, and can still drop a companion back on Company.
- The companions with the captain are the filled boxes from top to bottom. An empty box is skipped. After a drag, that order is who walks with him.
- The captain's own gear is not on this page.
- The same page has a Plans scrollbox and eight key boxes, F1 through F8. The keys do not scroll and have no filter.
- A filter above Plans reads All, Line, Column, Mob, Parade. The page opens on All. Choosing one shows only that formation, still in catalog order. Mob shows Tight mob and Loose mob. All shows every plan that is not on a key.
- A new game puts Line, Column, Tight mob, Loose mob, and Parade on F1 through F5. F6, F7, and F8 are empty, and Plans is empty. The Plans scrollbar is hidden.
- A plan moves onto an empty key, swaps with the plan on an occupied key, or returns to the Plans box and leaves that key empty. The plan returns to the catalog even when the formation filter hides it. The same plan is never on two keys.
- The filter does not take a plan off a key, and it does not change the active plan.
- Releasing a plan on its own box, or off any box, leaves the keys as they were. The drop does not make that plan active, and a Parade plan does not fire its events on the drop.
- While the page is closed, F1 through F8 select those keys. An empty key does nothing. A key whose plan is Parade fires the parade events, then becomes the active plan, whichever key that is.
- Dragging the active plan onto another key moves the corner mark with it. Dragging it back to Plans leaves no plan active until a filled key is pressed. The companions keep the formation they had.
- The corner list shows the eight keys, including the empty ones, and is not a drag target.
- Test Drill still selects F1, then F2, then F3, then F4. An empty key on a leg is the same as pressing that empty key.
- Move, look, and the battle-plan keys wait while the page is open.
- The wheel over Company, the squad column, or Plans scrolls that box and leaves the others where they are. The wheel over a key does nothing. A scrollbar is shown on a box whose rows do not fit, and hidden when they do.
- A company longer than the Company box scrolls, and a catalog longer than the Plans box scrolls. Filtering a long list hides the rows that do not match, and the scroll moves to the top of what remains. Closing the page forgets both filters. The next open starts on All.

## Implementation notes

- Today Build squad is a checkbox list. `SolidSquadBuilder::CollectRows` prefixes `[x]` or `[ ]`, and `Toggle` adds or removes a company index. `ASolidCharacter::ToggleSquadBuilderRow` is the click and the Enter key. This story replaces that gesture. The highlight index and the up/down wrap go away with it.
- `CanOpen` stays. The menu row stays dim below two companions.
- `SelectedCompanionIndices` is a packed list. An empty squad box is a hole, so the stored choice has to keep that gap. `CollectFieldedCompanions` already skips a missing record. Fielding walks the boxes from top to bottom and skips the empty ones. It still stops at the event cap.
- Company order is `USolidCompany` order, not a player order. The Company list is not rearranged. A companion in a squad box is left out of it. The filter is `ESolidVocation`. Spacing is not a plan filter. The plan filter is `ESolidBattleFormation`.
- The starter roster is five companions and five plans. Do not add either in this story. The scrollboxes and the filters are on the page anyway. A test that builds a longer company, and a longer catalog, checks the window, the thumb, and that a filter hides the other rows. The starter test checks that both thumbs stay hidden.
- Dragongate sets the squad cap to `SolidEvents::RosterCount`, the whole company. A company of dozens makes the squad column dozens of boxes, so that column scrolls. It has no filter, because the box order is the order they walk. The eight keys stay eight.
- There is no scrollbar widget yet. The journal scrollbar is a different story. This page draws the thumb inside its own box. The board uses the journal rule: a longer list scrolls, and the board stays the same size.
- The inventory drag is press, a name on the pointer, and release. `SolidInventory::Move` swaps when both sides hold something, and the same place or a miss does nothing. Esc and closing the page cancel the drag. Use that gesture here. Do not add a new pointer.
- `AssignedCompanyPlanIndices` is packed, and `SetAssignedBattlePlans` drops holes. F6 through F8 are empty only because they are past the end. This story needs an empty key between filled keys, so the eight boxes are fixed slots. `SelectAssignedSlot` on an empty slot returns false.
- The catalog is Line, Column, Tight mob, Loose mob, and Parade, in that order. Do not add a plan. A plan leaves the Plans list while it sits on a key.
- Parade events are `FireParadeEvents`, called from `SelectBattlePlanSlot` when that slot's formation is Parade. The drop must not call it. Selecting the key still does, including a Parade plan that the player has moved off F5, and including Test Drill when a drill leg selects that key.
- The corner panel is `DrawBattlePlansPanel`. It already draws eight lines and an em dash for an empty slot. Keep it as a readout.
- `SolidCore1.SquadBuilder.Toggle` assumes the checkbox list, `Toggle`, and `IsChoiceAvailable`. Replace that test in the same commit as the drag, and say so. `CanOpen` stays covered. The suite line in `README.md` still says the builder toggles the company. Change that sentence in the same commit.
- The battle-plan tests pin the starter assignment and that selecting F5 fires the parade events once. Those tests keep passing while the starter assignment is unchanged. A test that Parade fires from another key, and a test that a key in the middle can be empty, belong in the same commit as the slots.
