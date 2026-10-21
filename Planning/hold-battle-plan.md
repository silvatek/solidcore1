# Hold battle plan

Status: Ready

## Story

The player chooses the Hold battle plan. The player then moves the captain, and the companions stay where they are.

## Acceptance

- The company catalog has a plan named Hold. Hold uses the Hold formation and standard spacing. Hold starts disabled. This story enables it. While fewer than eight battle plans are enabled, the squad's selection is every enabled plan, in catalog order. F1 is not reserved for Line.
- Enter Kanfold enables Hold after the four formation plans it already enables. Enter Dragongate does the same. Enabling Hold when it is already on leaves it on. No other event in this story enables or disables Hold.
- The battle-plan list shows Hold on the slot that rule assigns. While it is active, that row stays highlighted.
- Selecting Hold makes each companion in the squad keep the place they have at that moment. The captain still moves, looks, and clicks. Companions in the squad do not walk, run, or steer to a formation slot while Hold is active. Companions who are not in the squad stay at the stronghold.
- Selecting Hold again while it is already active keeps each companion at the place they have at that new moment.
- Selecting a different assigned plan ends Hold at once. That plan's arrangement takes over, and companions in the squad steer to its slots again.
- Choosing Hold does not fire Enter Iglin, Enter Relion, or Enter Dragongate. It does not raise the squad cap, and it does not unlock raven sight.
- Hold does not add or remove companions. An empty squad can still select Hold. There is no companion to keep in place.
- The journal records `Hold enabled` when Kanfold or Dragongate enables it.
- A cave trip and a tower trip keep Hold enabled when it was enabled, keep it on its assigned slot when it was selected, and keep it active when it was active. A companion who spawns while Hold is active holds at the place they spawn.
- The Build squad Plans filter gains Hold. All still shows every plan that is not on a key. Hold matches only the Hold filter and All.
- This story does not change where Line, Column, Tight mob, Loose mob, Parade, Starter, Attack, or Defend place a companion. It does not change Drill. It does not add an F10 row.

## Implementation notes

- Add Hold to the company catalog in `AddDefaultBattlePlans`. `HasOnlyDefaultBattlePlans` stays on the five plans the company starts with today until [Early game battle plans](early-game-battle-plans.md) rewrites that catalog. Hold is beyond those five, so a leave opens Build squad when no companion is allowed.
- `ESolidBattleFormation` gains `Hold`. `FormationName` returns `Hold`. `SlotOffset` for Hold is unused while companions hold in place. `FacesCaptain` is false for Hold. The squad builder filter list is All, Line, Column, Mob, Parade, Hold.
- Companions follow through `ASolidCompanionCharacter::UpdateFollow`, which always steers to the active formation slot around the captain. While Hold is active, skip that steer and leave each companion where they are. Selecting Hold (including again) records the freeze at that moment. Selecting another plan clears it.
- `SelectBattlePlanSlot` fires Parade events only for the Parade formation. Hold must not match that check. Spacing on Hold stays Standard and is unused while companions are frozen.
- Enter Kanfold and Enter Dragongate each gain `EnableBattlePlan` for `Hold` after the four formation enables. Raise `MaxChangesPerEvent` if those events need another slot. The change-kind name list gains `Hold`. A name that is not in the catalog still changes nothing.
- A companion who spawns or respawns while Hold is active holds at the spawn point. Do not walk them to an old freeze point from before the trip.
- [Drill battle plan](drill-battle-plan.md) is a different plan. Selecting Hold while Drill is active stops the square the way any other plan does. This story does not make the square repeat, and it does not suppress move, look, or click.
- [Companions move only with the captain](companions-move-with-captain.md) is a different story. Hold is the battle plan that leaves companions put while the captain walks away.
- Tests that assume five company plans, five assigned slots, or an empty late F-key change before the product commit, so they pass on the current catalog and on a catalog that includes Hold. The product commit does not edit those tests. It adds Hold, the formation, the Kanfold and Dragongate enables, and the freeze.
  - `SolidCore1.BattlePlan.CompanyDefaults` finds Line, Column, Tight mob, Loose mob, and Parade by name. Do not require an index or a count of 5. Find Hold by name once this story adds it.
  - `SolidCore1.BattlePlan.SquadAssigned` and `SolidCore1.BattlePlan.GameModeSelect` ask for a plan by name. Selecting Parade by that slot still fires its events. Reject the first slot past the assigned count. Selecting Hold by its slot does not fire those events.
  - `SolidCore1.SquadBuilder.*` that filters by formation gains Hold. A plan whose formation is Hold matches that filter and All, and does not match Line, Column, Mob, or Parade.
- The product commit adds tests that Hold enables on Kanfold and on Dragongate, that selecting Hold leaves companions put while the captain moves, that selecting another plan restores follow, and that Hold does not fire the Parade events. The `SolidCore1.BattlePlan.*` line in `README.md` gains those cases. The Battle plans section names Hold.
