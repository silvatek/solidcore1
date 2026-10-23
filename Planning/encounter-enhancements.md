# Encounter enhancements

Status: New

## Story

Battle code sits with the rest of combat. Encounter definitions sit with the events an encounter can wait on. An encounter can depend on one event. The monsters at that encounter's mark stay out of the world until that event has fired. Once it has, they stand at the mark.

This story introduces no game concept. Encounter is already `FSolidEncounter`. Event is already `SolidEvents::FEvent`.

This is a single phase. The folder moves, the rename, and the dependency land together.

## Acceptance

- `SolidBattle` and `SolidBattlePanel` live in `Source/SolidCore1/Combat/`, with the weapon and the armour. Includes name `Combat/`. `Source/SolidCore1/Battle/` is gone. `Source/SolidCore1/BattlePlans/` stays where it is.
- Encounter source lives in `Source/SolidCore1/Events/`: `SolidEncounter.h`, `SolidEncounter.cpp`, and `SolidEncounters.h`. The monster, the group, and the raptor stay in `Source/SolidCore1/Monster/`. Includes of the encounter name `Events/`.
- The catalog array is `SolidEncounter::StaticEncounters`. The row type stays `FDefinition`. `DefinitionCount` and `FindByName` read that array. Town, lair, message, and event catalogs keep the name `Definitions`.
- A static encounter can depend on one event, named by that event's id. An encounter with no event id is placed as it is today.
- Iglin Bridge depends on no event. Its mark still places three Small raptors.
- An encounter that depends on an event stays out of the world while that event is unfired. Its mark stays on the chart. The Place list still offers the name.
- When that event fires in the open world, each mark of that encounter is placed and seeded. The points are the ones a mark already uses. A closed visit places none.
- A later visit, after the event has already fired, places that encounter with the others when monsters are chosen. Two marks of one name are two encounters.
- An event id the event list does not contain leaves that encounter unplaced.
- If the catalog contains Giant ants when this story is implemented, that encounter depends on `SolidEvents::IdDrivenFromVillage` (`8f3c2a17`). [Insect monsters](insect-monsters.md) is Ready and is the story that adds the row named Giant ants. Until that row exists, the catalog stays without it.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

As of this story, `Source/SolidCore1/Battle/` still holds `SolidBattle` and `SolidBattlePanel`. `Source/SolidCore1/Combat/` holds `SolidWeapon` and `SolidArmour`. The catalog is `SolidEncounter::Definitions` in `Source/SolidCore1/Monster/SolidEncounters.h`, and its only row is Iglin Bridge. If those moves have already landed when work starts, the acceptance is already true and the work continues from there.

- Move `SolidBattle.h`, `SolidBattle.cpp`, `SolidBattlePanel.h`, and `SolidBattlePanel.cpp` into `Combat/`. `SolidBattle.cpp` includes `SolidBattle.h` by file name. `SolidBattle.h` includes `Combat/SolidWeapon.h`. After the move, that weapon include is `SolidWeapon.h`. Callers of `Battle/SolidBattle.h` or `Battle/SolidBattlePanel.h` include `Combat/` instead. Those callers are `SolidGameMode.h`, `SolidHUD.cpp`, `SolidBattleTests.cpp`, and `SolidBattleSimulationTests.cpp`. `BattlePlans/` stays.
- Move `SolidEncounter.h`, `SolidEncounter.cpp`, and `SolidEncounters.h` into `Events/`. `SolidEncounter.cpp` includes `SolidRaptor.h` and `SolidGroundMove.h` by file name today. Those become `Monster/SolidRaptor.h` and `Player/SolidGroundMove.h`. `SolidEncounters.h` includes `SolidEncounter.h` by file name. Callers include `Events/SolidEncounter.h` and `Events/SolidEncounters.h`. Those callers are `SolidGameMode.h`, `SolidGameMode.cpp`, `SolidWorldChart.h`, `SolidWorldChart.cpp`, `SolidWorldChartEdMode.cpp`, `SolidMonsterTests.cpp`, `SolidBattleTests.cpp`, `SolidBattleSimulationTests.cpp`, and `SolidBattleTestHelpers.h`.
- Rename the array `Definitions` in the `SolidEncounter` namespace to `StaticEncounters`. `FDefinition`, `DefinitionCount`, `FindByName`, `MaxEncounters`, and `WithGroup` keep their names. `DefinitionCount` is `UE_ARRAY_COUNT(StaticEncounters)`. Comments that name the encounter catalog say `StaticEncounters`. `SolidEvents::Definitions` stays.
- `FDefinition` stores one event id. Empty means no dependency. `Require(const TCHAR* EventId)` sets it. A second call keeps the later id. Iglin Bridge does not call it.
- `ChooseMonsters` skips a row whose event is unfired. Fired is `bFired` at `SolidEvents::FindIndexById`. An unknown id is unfired. The row is absent from `Encounters` until then, so a later pass can add it once. When monsters are already chosen and an event then fires, place and seed the rows whose event is now fired and that are not already in `Encounters`. Encounters already placed stay. `PresentEventResult` runs on every game-mode fire path, including the tunnel portal's `MarkComplete` of `IdDrivenFromVillage`. A fire before monsters have been chosen waits for `ChooseMonsters`, which then includes the row. A closed visit still returns from `WorldHasEncounters` and places none.
- The chart mark and the Place list stay as they are. A waiting encounter is a stored name. It has no members until the event.
- Giant ants is the catalog name in [Insect monsters](insect-monsters.md): `TEXT("Giant ants")`, two Tiny ants, Attack. That story has not shipped, so the array does not contain the row yet. `FindByName` ignores case, so `Giant Ants` is that same row. If the row is present, it calls `Require(SolidEvents::IdDrivenFromVillage)`. That constant is the event `8f3c2a17`. `SolidMessages::IdDrivenFromVillage` is the message `4b91d0e6`, and the dependency names the event. If Insect monsters ships after this story, its row is appended to `StaticEncounters` in `Events/` and calls that same `Require`.
- Tests keep the Iglin Bridge placement. A row that requires an unfired event places nobody from its mark, and places its members once that event has fired. When Giant ants is in the catalog, a test expects its event id to be `IdDrivenFromVillage`. The product commit updates the `SolidCore1.Monster.*` line in `Source/SolidCore1/Tests/README.md` when that line gains the wait. The tests run without a viewport. A cloud agent does not run the editor or the suite.
