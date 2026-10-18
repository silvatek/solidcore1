# Drill battle plan

Status: Ready

## Story

The player selects the Drill battle plan. The squad repeats the test drill, and it keeps repeating while Drill stays the active plan. Test Drill is not on the F10 menu.

## Acceptance

- The company catalog has a plan named Drill, immediately after Parade. Drill starts disabled. This story enables it. [Early game battle plans](early-game-battle-plans.md) puts every enabled plan on the squad while fewer than eight are enabled, in catalog order. F1 is not reserved for Line.
- The battle-plan list shows Drill on the slot that rule assigns. While it is active, that row stays highlighted.
- Selecting Drill starts the test drill from the first leg. The captain walks the same square Test Drill walks now. Leg one walks for 1.5 seconds on the Line arrangement. Legs two, three, and four each turn +90° over 0.25 seconds, then walk for 1.5 seconds, on Column, Tight mob, and Loose mob. Companions in the squad follow that square. Companions who are not in the squad stay at the stronghold.
- When the fourth leg ends, the first leg starts again. The square does not stop on its own.
- The active plan stays Drill through every leg. A leg does not switch the active plan to Line, Column, Tight mob, or Loose mob, and it does not fire the Parade events.
- Move, look, and click stay suppressed while Drill is active, as they are during Test Drill. Another battle-plan key still works.
- Selecting a different plan stops the square at once. The captain stops the forced walk, and that plan takes over. Selecting Drill again starts from the first leg.
- The drill does not add or remove companions. An empty squad still walks the captain around the square.
- The F10 menu has no Test Drill row. No key on that menu starts the square. The battle-plan key for Drill's slot does.
- Choosing Drill does not fire Enter Iglin, Enter Relion, or Enter Dragongate. It does not raise the squad cap, and it does not unlock raven sight.

## Implementation notes

- The square is `SolidSquadDrill`: four legs, `LegDurationSeconds` 1.5, `TurnDurationSeconds` 0.25, `TurnYawDegrees` 90. Leg 0 does not turn. Legs 1–3 do. `StartSquadFormationDrill` starts it, and today the fourth leg calls `StopSquadFormationDrill`. While Drill is active, wrap to leg 0 instead.
- Each leg currently calls `SelectBattlePlanSlot`, which changes the active plan. Keep the active slot on Drill. Apply that leg's arrangement (Line, then Column, then Tight mob, then Loose mob) without selecting those slots and without the Parade event check. [Early game battle plans](early-game-battle-plans.md) moves those plans off F1–F4.
- `SelectBattlePlanSlotFromInput` already ignores the menu and still runs during the drill. Leaving Drill through that path stops the square. A portal arrival already calls `StopSquadFormationDrill`. Keep that.
- The square does not end in Parade, and it does not add companions.
- [Reorder the game menu](Done/reorder-game-menu.md) also leaves Test Drill off the menu. This story does not add Inventory or the cave row back, and it does not put a row after Exit Game. The last two rows are Credits, then Exit Game.
- [Hold battle plan](hold-battle-plan.md) is a different plan. This story does not make companions stay put.
- Tests that assume five company plans, five assigned slots, and an empty F6 change before the product commit, so they pass on the current catalog and on a sixth plan. The product commit does not edit those tests. It adds Drill and the repeating square.
  - `SolidCore1.BattlePlan.CompanyDefaults` finds Line, Column, Tight mob, Loose mob, and Parade by name. Do not require an index or a count of 5. [Early game battle plans](early-game-battle-plans.md) puts Starter first.
  - `SolidCore1.BattlePlan.SquadAssigned` and `SolidCore1.BattlePlan.GameModeSelect` ask for a plan by name, as [Early game battle plans](early-game-battle-plans.md) requires. Selecting Parade by that slot still fires its events. Reject the first slot past the assigned count.
  - `SolidCore1.BattlePlan.FormationDrillLegs` checks the leg timings and `PlanSlotForLeg`. Those timings stay. Leave that test. The drill applies Line, Column, Tight mob, and Loose mob by arrangement, not by those slot numbers.
- The `SolidCore1.BattlePlan.*` line in `README.md`, and the Test Drill sentence under Battle plans, change in the product commit. The menu lines lose Test Drill there too.
