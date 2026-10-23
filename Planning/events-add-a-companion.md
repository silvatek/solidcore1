# Events add a companion

Status: In Progress

## Story

An event can add a companion to the company and to the squad. The change is `AddCompanion(name)`.

Two lists are named `AllCompanions`. The company's list, `USolidCompany::AllCompanions`, starts empty. The other list is global and holds every companion an event can add: Sam, Alex, Max, Morgan, and Quinn. `AddCompanion` copies the named companion from the global list into `company.AllCompanions`. When the squad has room, the copy is selected into the squad.

Enter Relion (`IdEnterRelion`) sets the max squad size to 1, then adds Sam. Enter Kanfold (`IdEnterKanfold`) sets the max squad size to 2, then adds Alex. Enter Dragongate (`IdEnterDragongate`) adds Max, Morgan, and Quinn, and then sets the max squad size to the roster count. Those three are in the company. The squad selection stays as it was, and the new cap does not select them.

This story does not introduce a new class. Companion, Company, and Squad are already classes. The company the game holds stays `USolidCompany`. `AddCompanion` is a change on an event, the same kind of change as `SetMaxSquadSize`.

## Acceptance

- `AddCompanion(name)` is an event change. The name is the companion's name.
- The global `AllCompanions` holds Sam, Alex, Max, Morgan, and Quinn. Sam is a Rogue, Alex is a Warrior, Max is a Cleric, Morgan is a Mage, and Quinn is a Warrior, on the Viking mesh, with the colours, weapon, and armour the company gives those names today.
- A new company has an empty `AllCompanions`. Starting a game, creating the squad, and ensuring the company leave that list empty.
- Applying `AddCompanion` copies that global record into `company.AllCompanions`. A name already in the company's list stays one entry.
- The squad has room when the number of selected companions is below the max squad size. With room, the copy is selected into the next squad box. A companion already in a box stays in that box. With no room, the company still gains the copy and the squad selection stays as it was. Enter Dragongate's three adds are the exception below: they copy into the company and leave the squad as it was, including when a slot is already open.
- `IdEnterRelion` calls `AddCompanion` with Sam as the next change after `SetMaxSquadSize(1)`. After Enter Relion fires on an empty company and an empty squad, the company's list is Sam, the max squad size is 1, and the selected companion is Sam.
- `IdEnterKanfold` calls `AddCompanion` with Alex as the next change after `SetMaxSquadSize(2)`. After Enter Kanfold fires, Alex is in the company's list, the max squad size is 2, and Alex is selected. When Enter Relion has already fired, the company's list is Sam then Alex, and both are selected.
- Enter Kanfold without Enter Relion copies Alex into the company and selects Alex. Sam stays on the global list only.
- `IdEnterDragongate` calls `AddCompanion` for Max, then Morgan, then Quinn, and then `SetMaxSquadSize` of the roster count. The three copies land in the company. The squad selection stays as it was. The cap change does not select them.
- After Enter Dragongate fires on an empty company and an empty squad, the company's list is Max, Morgan, Quinn, the max squad size is the roster count, and the squad is empty.
- When Enter Relion and Enter Kanfold have already fired, Enter Dragongate leaves the company's list as Sam, Alex, Max, Morgan, Quinn. The squad stays Sam and Alex.
- When Enter Relion has fired and Enter Kanfold has not, Enter Dragongate leaves the company's list as Sam, Max, Morgan, Quinn. The squad stays Sam. Alex stays on the global list. Parade takes this path: it fires Enter Iglin, Enter Relion, and Enter Dragongate, and it skips Enter Kanfold.
- When Enter Kanfold has fired and Enter Relion has not, Enter Dragongate leaves the company's list as Alex, Max, Morgan, Quinn. The squad stays Alex. The cap is already 2 and only Alex is selected, so one slot is open. Players do not skip Relion. Max, Morgan, and Quinn still stay out of the squad.
- Enter Iglin, Reach broken bridge, Killed by monsters, and Driven from the village leave the company's `AllCompanions` and the squad selection as they were.
- A second visit to the same town leaves the company list and the squad as the first visit left them.
- Before Relion, Kanfold, or Dragongate, the company's `AllCompanions` is empty and the squad is empty.
- The journal detail for Enter Relion still contains `Squad 1: Sam` and does not contain Alex. It also names Sam as the companion that event adds.
- The journal detail for Enter Kanfold still contains `Squad 2: Sam, Alex` and still contains `Starter disabled. Attack disabled. Defend disabled. Line enabled. Column enabled. Tight mob enabled. Loose mob enabled`. It also names Alex as the companion that event adds.
- The journal detail for Enter Dragongate still contains `Squad 5: Sam, Alex, Max, Morgan, Quinn`, `Title Captain`, and `Starter disabled. Attack disabled. Defend disabled. Line enabled. Column enabled. Tight mob enabled. Loose mob enabled`. It also names Max, Morgan, and Quinn as the companions that event adds.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. This plan commit does not.

## Implementation notes

- `FEvent::AddCompanion` and `Change::AddCompanion` take the name, the way `SetMaxSquadSize` takes the size. The name sits on `FChange` the way `BattlePlanName` does. `USolidCompany::AddCompanion` still takes an `FSolidCompanion` and appends it to the company's list.
- The global list is not a member of `USolidCompany`. A member and a static on that class cannot share the name `AllCompanions`. The records are the ones `InitializeDefaultCompanions` builds today. The copy into the company resolves the Viking mesh on that company, because `FSolidCompanion::CharacterMeshIndex` is an index into the company's own meshes. An empty name, or a name the global list lacks, leaves the company and the squad as they were. These events pass Sam, Alex, Max, Morgan, and Quinn.
- `InitializeDefaultCompanions` stops writing `USolidCompany::AllCompanions`. The game instance, the game mode, and `USolidSquad::InitializeFromCompany` call it today and would otherwise fill the company at the start. `EnsureCompanyAndSquad` still leaves an existing selection in place.
- Relion goes from 6 changes to 7. Kanfold goes from 12 to 13. Dragongate goes from 17 changes to 20. On Dragongate the three `AddCompanion` changes come immediately before `SetMaxSquadSize`. `MaxChangesPerEvent` rises from 17 to 20 so those three fit and the size unlocks and battle-plan edits stay on the event. The battle-plan sentences stay one run at the end of the detail. [Hold](hold-battle-plan.md) also adds one battle-plan change to Kanfold and to Dragongate. A Dragongate that includes Hold as well needs 21 slots.
- `ApplyChange` sees the game state and the captain. The company is the captain's outer, the same way a battle-plan change finds it. The squad is on the game mode. `NoteTownEntry` has the squad and already calls `SyncBattlePlans` after a fired event. After `NoteTownEntry` for Relion, Kanfold, and Dragongate, the copy is on the company and the selection is on that squad. A notify with a loose captain and no squad still sets the max size.
- The journal sentence is `Sam joins`, `Alex joins`, `Max joins`, `Morgan joins`, or `Quinn joins`, in change order. On Relion and Kanfold it sits after the squad-size line and before the title. On Dragongate the three join sentences sit before the squad-size line. Relion's detail must not contain Alex, because `SolidJournalTests` asserts that. The Kanfold and Dragongate battle-plan sentences stay one run, because `SolidBattlePlanTests` asserts that string. `SolidEventTests` still reads the cap: squad size 1 at Relion, 2 at Kanfold, and `Squad 5: Sam, Alex, Max, Morgan, Quinn` on Enter Dragongate. That Dragongate line is the cap's roster list. The squad selection is unchanged by Max, Morgan, and Quinn.
- `SolidCompanyRosterTests` expects the company to hold five companions after `InitializeDefaultCompanions` and after `EnsureCompanyAndSquad` (`five companions`, `catalog seeded`, `catalog stays five`). That count moves to the global list, and a new company count is 0. `SolidCharacterEditorTests` and `SolidSquadBuilderTests` seed the company through `InitializeDefaultCompanions`. Those tests are updated in their own commit before the product change, and the product change leaves them as they then stand. The new test builds a company, its captain, and a squad, fires the town the way `NoteTownEntry` does, and checks the company's list and the selected companions. It does not need a world.
- `SquadCount` stays the cap. Fielding still walks the selected boxes up to that cap. Sam walks after Relion. Sam and Alex walk after Relion and Kanfold. Kanfold alone fields Alex. Dragongate leaves that selection as it was, so Dragongate alone fields nobody, Parade fields Sam, and all three towns field Sam and Alex. Max, Morgan, and Quinn are in the company for the squad builder and the character editor to show. The player can select them once the cap allows.
