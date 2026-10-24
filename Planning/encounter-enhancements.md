# Encounter enhancements

Status: In Progress

## Story

Battle code sits with the rest of combat. Encounter definitions sit with the events an encounter can wait on. An encounter can depend on one event. The monsters at that encounter's mark stay out of the world until that event has fired. Once it has, they stand at the mark.

Giant ants is [Insect monsters](insect-monsters.md). If that story has already shipped, Giant ants keeps spawning at startup. If it ships after this one, that story makes Giant ants wait for Driven from the village.

This story introduces no game concept. Encounter is already `FSolidEncounter`. Event is already `SolidEvents::FEvent`.

This is a single phase. The folder moves, the rename, and the dependency land together.

## Acceptance

- `SolidBattle` and `SolidBattlePanel` live in `Source/SolidCore1/Combat/`, with the weapon and the armour. Includes name `Combat/`. `Source/SolidCore1/Battle/` is gone. `Source/SolidCore1/BattlePlans/` stays where it is.
- Encounter source lives in `Source/SolidCore1/Events/`: `SolidEncounter.h`, `SolidEncounter.cpp`, and `SolidEncounters.h`. The monster, the group, and the raptor stay in `Source/SolidCore1/Monster/`. Includes of the encounter name `Events/`.
- The catalog array is `SolidEncounter::StaticEncounters`. Each row is `SolidEncounter::FStaticEncounter`, the type that is `FDefinition` today. `DefinitionCount` and `FindByName` read that array. Road and lair catalogs keep `FDefinition`. Town, message, and event catalogs keep the name `Definitions`.
- A static encounter can depend on one event, named by that event's id. An encounter with no event id is placed as it is today.
- Iglin Bridge depends on no event. Its mark still places three Small raptors.
- An encounter that depends on an event stays out of the world while that event is unfired. Its mark stays on the chart. The Place list still offers the name.
- When that event fires in the open world, each mark of that encounter is placed and seeded. The points are the ones a mark already uses. A closed visit places none.
- A later visit, after the event has already fired, places that encounter with the others when monsters are chosen. Two marks of one name are two encounters.
- An event id the event list does not contain leaves that encounter unplaced.
- [Insect monsters](insect-monsters.md) adds the row named Giant ants. If that row is already in the catalog, Insect monsters shipped first. Giant ants keeps spawning at startup, with the other encounters, and this story leaves it with no event id. If the row is absent, this story does not add it. When Insect monsters ships second, that story gives Giant ants `SolidEvents::IdDrivenFromVillage`, and the ants spawn when that event fires.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

As of this story, `Source/SolidCore1/Battle/` still holds `SolidBattle` and `SolidBattlePanel`. `Source/SolidCore1/Combat/` holds `SolidWeapon` and `SolidArmour`. The catalog is `SolidEncounter::Definitions` in `Source/SolidCore1/Monster/SolidEncounters.h`, and its only row is Iglin Bridge. If those moves have already landed when work starts, the acceptance is already true and the work continues from there.

- Move `SolidBattle.h`, `SolidBattle.cpp`, `SolidBattlePanel.h`, and `SolidBattlePanel.cpp` into `Combat/`. `SolidBattle.cpp` includes `SolidBattle.h` by file name. `SolidBattle.h` includes `Combat/SolidWeapon.h`. After the move, that weapon include is `SolidWeapon.h`. Callers of `Battle/SolidBattle.h` or `Battle/SolidBattlePanel.h` include `Combat/` instead. Those callers are `SolidGameMode.h`, `SolidHUD.cpp`, `SolidBattleTests.cpp`, and `SolidBattleSimulationTests.cpp`. `BattlePlans/` stays.
- Move `SolidEncounter.h`, `SolidEncounter.cpp`, and `SolidEncounters.h` into `Events/`. `SolidEncounter.cpp` includes `SolidRaptor.h` and `SolidGroundMove.h` by file name today. Those become `Monster/SolidRaptor.h` and `Player/SolidGroundMove.h`. `SolidEncounters.h` includes `SolidEncounter.h` by file name. Callers include `Events/SolidEncounter.h` and `Events/SolidEncounters.h`. Those callers are `SolidGameMode.h`, `SolidGameMode.cpp`, `SolidWorldChart.h`, `SolidWorldChart.cpp`, `SolidWorldChartEdMode.cpp`, `SolidMonsterTests.cpp`, `SolidBattleTests.cpp`, `SolidBattleSimulationTests.cpp`, and `SolidBattleTestHelpers.h`.
- Rename the array `Definitions` in the `SolidEncounter` namespace to `StaticEncounters`. Rename that namespace's `FDefinition` to `FStaticEncounter`. `WithGroup` returns `FStaticEncounter`. `DefinitionCount`, `FindByName`, `MaxEncounters`, and `WithGroup` keep their names. `DefinitionCount` is `UE_ARRAY_COUNT(StaticEncounters)`. `FSolidEncounter` is still built from one `FStaticEncounter`. Its field stays `Definition`. Comments that name the encounter catalog say `StaticEncounters`. `SolidRoad::FDefinition`, `SolidLair::FDefinition`, and `SolidEvents::Definitions` stay.
- `FStaticEncounter` stores one event id. Empty means no dependency. `Require(const TCHAR* EventId)` sets it. A second call keeps the later id. Iglin Bridge does not call it.
- `ChooseMonsters` skips a row whose event is unfired. Fired is `bFired` at `SolidEvents::FindIndexById`. An unknown id is unfired. The row is absent from `Encounters` until then, so a later pass can add it once. When monsters are already chosen and an event then fires, place and seed the rows whose event is now fired and that are not already in `Encounters`. Encounters already placed stay. `PresentEventResult` runs on every game-mode fire path, including the tunnel portal's `MarkComplete` of `IdDrivenFromVillage`. A fire before monsters have been chosen waits for `ChooseMonsters`, which then includes the row. A closed visit still returns from `WorldHasEncounters` and places none.
- The chart mark and the Place list stay as they are. A waiting encounter is a stored name. It has no members until the event.
- Giant ants is the catalog name in [Insect monsters](insect-monsters.md): `TEXT("Giant ants")`, two Tiny ants, Attack. `FindByName` ignores case, so `Giant Ants` is that same row. As of this story the array does not contain it. If the row is present when work starts, leave its event id empty so startup still places it. `SolidEvents::IdDrivenFromVillage` is the event `8f3c2a17`. `SolidMessages::IdDrivenFromVillage` is the message `4b91d0e6`. Insect monsters, shipping second, appends an `FStaticEncounter` to `StaticEncounters` and calls `Require` with the event id.
- Tests keep the Iglin Bridge placement. A row that requires an unfired event places nobody from its mark, and places its members once that event has fired. When Giant ants is already in the catalog, a test expects an empty event id and a startup placement. The product commit updates the `SolidCore1.Monster.*` line in `Source/SolidCore1/Tests/README.md` when that line gains the wait. The tests run without a viewport. A cloud agent does not run the editor or the suite.
