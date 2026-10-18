# Drill battle plan

Status: Ready

## Story

The player selects the Drill battle plan. The squad repeats the test drill, and it keeps repeating while Drill stays the active plan. Test Drill is not on the F10 menu.

## Acceptance

- The company catalog has a plan named Drill, after Parade. The starter squad assigns it, so it is F6. F7 and F8 stay empty.
- The battle-plan list shows Drill on F6. While it is active, that row stays highlighted. F1 is still Line and the default.
- Selecting Drill starts the test drill from the first leg. The captain walks the same square Test Drill walks now. Leg one walks for 1.5 seconds on the Line arrangement. Legs two, three, and four each turn +90° over 0.25 seconds, then walk for 1.5 seconds, on Column, Tight mob, and Loose mob. Companions in the squad follow that square. Companions who are not in the squad stay at the stronghold.
- When the fourth leg ends, the first leg starts again. The square does not stop on its own.
- The active plan stays Drill through every leg. A leg does not switch the active plan to Line, Column, Tight mob, or Loose mob, and it does not fire the Parade events.
- Move, look, and click stay suppressed while Drill is active, as they are during Test Drill. Another battle-plan key still works.
- Selecting a different plan stops the square at once. The captain stops the forced walk, and that plan takes over. Selecting Drill again starts from the first leg.
- The drill does not add or remove companions. An empty squad still walks the captain around the square.
- The F10 menu has no Test Drill row. No key on that menu starts the square. F6 does.
- Choosing Drill does not fire Enter Iglin, Enter Relion, or Enter Dragongate. It does not raise the squad cap, and it does not unlock raven sight.

## Implementation notes

- The square is `SolidPartyDrill`: four legs, `LegDurationSeconds` 1.5, `TurnDurationSeconds` 0.25, `TurnYawDegrees` 90. Leg 0 does not turn. Legs 1–3 do. `StartPartyFormationDrill` starts it, and today the fourth leg calls `StopPartyFormationDrill`. While Drill is active, wrap to leg 0 instead.
- Each leg currently calls `SelectBattlePlanSlot`, which changes the active plan. Keep the active slot on Drill. Apply that leg's arrangement (the plans on F1–F4) without selecting those slots and without the Parade event check.
- `SelectBattlePlanSlotFromInput` already ignores the menu and still runs during the drill. Leaving Drill through that path stops the square. A portal arrival already calls `StopPartyFormationDrill`. Keep that.
- [Test Drill ends in Parade](test-drill-ends-in-parade.md) is this menu item. Do not implement it. The square does not end in Parade, and it does not add companions.
- [Reorder the game menu](reorder-game-menu.md) also leaves Test Drill off the menu. This story does not add Inventory or the cave row back, and it does not put a row after Credits.
- [Hold battle plan](hold-battle-plan.md) is a different plan. This story does not make companions stay put.
- Tests that assume five company plans, five assigned slots, and an empty F6 change before the product commit, so they pass on the current catalog and on a sixth plan. The product commit does not edit those tests. It adds Drill and the repeating square.
  - `SolidCore1.BattlePlan.CompanyDefaults` expects five plans and Parade at index 4. Keep Line, Column, Tight mob, Loose mob, and Parade in those positions. Do not require the count to stay 5.
  - `SolidCore1.BattlePlan.PartyAssigned` expects five assigned slots and rejects F6. Keep Parade on slot 4 and the default on F1. Reject the first slot past the catalog instead of F6 by number.
  - `SolidCore1.BattlePlan.GameModeSelect` expects five company plans and an empty F6. Keep the F5 Parade event checks. Do not require F6 to be empty.
  - `SolidCore1.BattlePlan.FormationDrillLegs` checks the leg timings and the F1–F4 mapping. Those timings stay. Leave that test.
- The `SolidCore1.BattlePlan.*` line in `README.md`, and the Test Drill sentence under Battle plans, change in the product commit. The menu lines lose Test Drill there too.
