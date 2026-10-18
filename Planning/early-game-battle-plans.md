# Early game battle plans

Status: Ready

## Story

A new game is the start of act 0. The only battle plan enabled is Starter. Attack, Defend, Parade, Line, Column, Tight mob, and Loose mob are in the catalog, and they are disabled.

An event can enable a battle plan, and an event can disable one. Closing the monster message disables Starter and enables Attack and Defend. Enter Relion enables Parade. Enter Kanfold and Enter Dragongate each disable Starter, Attack, and Defend, and each enables Line, Column, Tight mob, and Loose mob. Dragongate does that when Kanfold was skipped. Those three early plans are off before the squad cap is above one.

While fewer than eight battle plans are enabled, the squad's selection is every enabled plan. The battle-plan list is hidden while that selection has fewer than two plans. Where Starter, Attack, and Defend place a companion does not matter. How companions stand relative to each other does not matter when none are out, or when only one is out.

## Acceptance

- The company catalog order is Starter, Attack, Defend, Parade, Line, Column, Tight mob, Loose mob. Starter, Attack, and Defend use the Line arrangement and standard spacing. That arrangement is not what this story is for. Parade, Line, Column, Tight mob, and Loose mob keep the arrangements they have now.
- A new game enables Starter only. The other seven are disabled. The squad's selection is Starter, on F1, and it is the active plan. F2 through F8 are empty. The battle-plan list is hidden.
- F1 through F8, and the shoulder chords, still select an assigned slot while the list is hidden. An empty slot does nothing. F1 selects Starter. Selecting Starter does not fire Enter Iglin, Enter Relion, or Enter Dragongate.
- `IdKilledByMonsters` disables Starter and enables Attack and Defend. It keeps the other changes it already has. The squad's selection becomes Attack, then Defend. The active plan becomes Attack. The list is shown. Its rows are F1 Attack, F2 Defend, and empty F3 through F8.
- Showing the monster message does not change which plans are enabled. Enter Iglin, Reach broken bridge, and Driven from the village do not either.
- Enter Relion enables Parade and keeps the changes it already has. The selection becomes Attack, Defend, Parade. A plan that was already active stays active. F3 is Parade. F4 through F8 stay empty.
- Enter Kanfold disables Starter, Attack, and Defend, and enables Line, Column, Tight mob, and Loose mob. It keeps the changes it already has, including the squad cap of 2. After that event the cap is above one and those three plans are disabled.
- When Parade was already enabled, Kanfold leaves the selection as Parade, Line, Column, Tight mob, Loose mob. An active Attack or Defend becomes Parade. An active Parade stays Parade, on F1. F6 through F8 stay empty.
- When Parade was not enabled, Kanfold leaves the selection as Line, Column, Tight mob, Loose mob. An active Attack or Defend becomes Line.
- Enter Dragongate makes the same battle-plan changes as Enter Kanfold. It disables Starter, Attack, and Defend, and it enables Line, Column, Tight mob, and Loose mob. It does not enable Parade. It keeps the changes it already has, including the full squad cap. Enabling a plan that is already on leaves it on.
- When Parade was already enabled, Dragongate leaves the selection as Parade, Line, Column, Tight mob, Loose mob. An active Attack or Defend becomes Parade. An active Parade stays Parade, on F1.
- When Parade was not enabled, Dragongate leaves the selection as Line, Column, Tight mob, Loose mob. An active Attack or Defend becomes Line.
- Selecting Parade still fires Enter Iglin if it has not fired, then Enter Relion and Enter Dragongate. It still does not fire Enter Kanfold. After that, Parade and the four formation plans are enabled, and the list is shown. Attack, Defend, Line, Column, Tight mob, and Loose mob do not fire those events.
- While fewer than eight plans are enabled, the squad's selection is exactly the enabled plans, in catalog order, skipping a disabled plan. The active plan stays the one the player had selected when that plan is still enabled. When the active plan is disabled, the active plan becomes the first plan in the new selection.
- When eight or more plans are enabled, this story does not replace the selection. [Drill battle plan](drill-battle-plan.md) and [Hold battle plan](hold-battle-plan.md) are not enabled here.
- The battle-plan list is the panel under the tech readout. It is hidden while the squad has fewer than two plans selected. With two or more, it shows the title Battle Plans, one row for each assigned plan, and an empty row through F8. The character editor still hides the panel while that page is open.
- The journal records `Starter disabled`, `Attack enabled`, `Attack disabled`, `Defend enabled`, `Defend disabled`, `Parade enabled`, `Line enabled`, `Column enabled`, `Tight mob enabled`, and `Loose mob enabled` for those changes.
- A cave trip and a tower trip keep which plans are enabled, which of them are selected, and which one is active.
- How companions stand relative to each other does not matter when the squad has no companion, or only one. This story does not add an F10 row, and it does not change where Line, Column, Tight mob, Loose mob, or Parade place a companion.

## Implementation notes

- `FGameState` stores one enabled flag per catalog plan. A new game sets Starter and clears the rest. `FGameState` is already the travel session's `Configuration`, so the flags survive `OpenLevel`. The session already stores `ActiveBattlePlanSlot`. Restoring the session syncs the selection from the flags, then selects that slot when it still holds a plan.
- Two change kinds, `EnableBattlePlan` and `DisableBattlePlan`. Each names a plan: `Starter`, `Attack`, `Defend`, `Parade`, `Line`, `Column`, `Tight mob`, or `Loose mob`. `ApplyChange` sets that flag. A name that is not in the catalog changes nothing. Disabling a plan that is already off leaves it off. Enabling a plan that is already on leaves it on. Raise `MaxChangesPerEvent` from 10 to 17. Killed by monsters grows to 6 and Enter Relion to 6. Enter Kanfold grows to 11. Enter Dragongate grows to 17.
- `IdKilledByMonsters` gains the Starter disable and the Attack and Defend enables after the changes it already lists. Enter Relion gains Parade. Enter Kanfold and Enter Dragongate each gain the three disables and then the four formation plans. [Return to the stronghold](return-to-stronghold.md) still owns replacing `EnableCave` on the monster close. This story does not remove that change.
- `USolidSquad::InitializeFromCompany` currently assigns every catalog plan, up to eight. After that init, and after an event applies, sync the selection to the enabled plans while fewer than eight are enabled. Keep the active plan by name when it is still in the list. `SelectBattlePlanSlot` still fires `FireParadeEvents` only for the Parade formation.
- The list is `DrawBattlePlansPanel`. Skip it when the assigned count is below 2. The character-editor branch already skips it. The keys in `SelectBattlePlanSlotFromInput` stay bound.
- Tests that require five plans, Line at index 0, five assigned slots, or Parade on F5 change before the product commit, so they pass on the current catalog and on this one. The product commit does not edit those tests. It adds Starter, Attack, Defend, the flags, and the event lines.
  - `SolidCore1.BattlePlan.CompanyDefaults` finds Line, Column, Tight mob, Loose mob, and Parade by name and checks the arrangements they have now. It does not require the count to stay 5, and it does not require an index.
  - `SolidCore1.BattlePlan.SquadAssigned` and `SolidCore1.BattlePlan.GameModeSelect` ask for a plan by name. A helper answers with that plan's assigned slot. On the current code the helper finds the slot the starter squad already has. On this story it enables that plan, syncs, then finds the slot. Selecting Parade by that slot still fires Enter Iglin, Enter Relion, and Enter Dragongate, and it still skips Enter Kanfold. Reject the first slot past the assigned count, not F6 by number.
- The product commit adds the tests for a new game's hidden list, the monster close, Enter Relion, Enter Kanfold and Enter Dragongate each disabling the three early plans and enabling the four, Parade skipping Kanfold and still leaving those four enabled, and the active plan surviving an enable. The `SolidCore1.BattlePlan.*` line in `README.md` gains those cases.
