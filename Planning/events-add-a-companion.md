# Events add a companion

Status: New

## Story

An event can add a companion to the company and to the squad. The change is `AddCompanion(name)`.

The company already lists Sam, Alex, Max, Morgan, and Quinn. The squad starts with nobody selected. `AddCompanion` puts the named companion in the company and selects that companion into the squad.

Enter Relion (`IdEnterRelion`) sets the max squad size to 1, then adds Sam. Enter Kanfold (`IdEnterKanfold`) sets the max squad size to 2, then adds Alex.

This story does not introduce a new class. Companion, Company, and Squad are already classes. `AddCompanion` is a change on an event, the same kind of change as `SetMaxSquadSize`.

## Acceptance

- `AddCompanion(name)` is an event change. The name is the companion's name.
- Applying it adds that companion to the company and selects them into the squad.
- A name already in the company stays one catalog entry. The squad selects that entry. Sam stays a Rogue and Alex stays a Warrior, on the Viking mesh, with the colours, weapon, and armour the company already gives those names.
- A companion already in a squad box stays in that box. `AddCompanion` fills the next box when that name is not already selected.
- `IdEnterRelion` calls `AddCompanion` with Sam as the next change after `SetMaxSquadSize(1)`. After Enter Relion fires on an empty squad, the max squad size is 1 and the selected companion is Sam.
- `IdEnterKanfold` calls `AddCompanion` with Alex as the next change after `SetMaxSquadSize(2)`. After Enter Kanfold fires, the max squad size is 2 and Alex is selected. When Enter Relion has already fired, Sam stays selected and Alex is selected with him.
- Enter Kanfold without Enter Relion selects Alex and leaves Sam unselected.
- Enter Iglin, Reach broken bridge, Killed by monsters, Driven from the village, and Enter Dragongate leave the squad selection as it was. Enter Dragongate still sets the max squad size to the roster count.
- A second visit to the same town leaves the squad as the first visit left it.
- Before either event, the squad is empty.
- The journal detail for Enter Relion still contains `Squad 1: Sam` and does not contain Alex. It also names Sam as the companion that event adds.
- The journal detail for Enter Kanfold still contains `Squad 2: Sam, Alex` and still contains `Starter disabled. Attack disabled. Defend disabled. Line enabled. Column enabled. Tight mob enabled. Loose mob enabled`. It also names Alex as the companion that event adds.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. This plan commit does not.

## Implementation notes

- `FEvent::AddCompanion` and `Change::AddCompanion` take the name, the way `SetMaxSquadSize` takes the size. `USolidCompany::AddCompanion` still takes an `FSolidCompanion`. The event change finds the catalog entry by name. When the name is missing, and it is one of the roster names, it builds the record `InitializeDefaultCompanions` builds for that name and then calls the company method. An empty name leaves the company and the squad as they were. This story's events pass Sam and Alex.
- The name sits on `FChange` the way `BattlePlanName` does.
- Relion goes from 6 changes to 7. Kanfold goes from 12 to 13. `MaxChangesPerEvent` stays 17. Enter Dragongate is already at that cap and gains no change. Max, Morgan, and Quinn stay in the company and stay out of the squad.
- `ApplyChange` sees the game state and the captain. The company is the captain's outer, the same way a battle-plan change finds it. The squad is on the game mode. `NoteTownEntry` has the squad and already calls `SyncBattlePlans` after a fired event. After `NoteTownEntry` for these two towns, the selection is on that squad. A notify with a loose captain and no squad still sets the max size.
- The journal sentence is `Sam joins` or `Alex joins`, in change order, so it sits after the squad-size line and before the title. Relion's detail must not contain Alex, because `SolidJournalTests` asserts that. The Kanfold battle-plan sentences stay one run, because `SolidBattlePlanTests` asserts that string. `SolidEventTests` still reads the cap: squad size 1 at Relion, 2 at Kanfold, and `Squad 5: Sam, Alex, Max, Morgan, Quinn` on Enter Dragongate. Those tests keep passing. The new test builds a company, its captain, and a squad, fires the town the way `NoteTownEntry` does, and checks the selected companions. It does not need a world.
- `SquadCount` stays the cap. Fielding still walks the selected boxes up to that cap, so Sam walks after Relion, and Sam and Alex walk after both events. Kanfold alone fields Alex.
