# The game instance owns the company

Status: In Progress

## Story

A developer gives the play session one company. `USolidGameInstance` owns `USolidCompany`. A cave trip and a tower trip keep that company. The catalog still holds the battle plans, the meshes, and the companions. A body-slider edit and a battle plan added during play are still there after the trip.

The active stronghold is a managed property of the company. The company owns the pointer, and callers ask the company for the stronghold. `FGameState` no longer carries it. A new game is still the Meagre Dungeon on its map cell, named The cave. The squad stays on the game mode. The player still assembles it for the visit, and the trip still restores its indexes.

## Acceptance

- `USolidGameInstance` creates one `USolidCompany` and holds it with `UPROPERTY`. The game mode does not create another. `GetCompany()` returns the instance's company.
- `OpenLevel` keeps the company. The next world binds its new squad to that same company. `InitializeDefaultBattlePlans`, `InitializeDefaultCharacterMeshes`, and `InitializeDefaultCompanions` leave a catalog that already has entries.
- A height, build, or gender written with `SetCompanionBodyControls` is still stored after a cave trip and after a tower trip. A battle plan added with `AddBattlePlan` is still in the catalog, at the same index.
- The squad is still created with the game mode. The travel session still stores the selected companions, the assigned plans, the active slot, and the held formation, and the next world still applies them.
- The company owns the active stronghold with a `TUniquePtr<FSolidStronghold>`. Callers ask the company for it. `FGameState` has no `ActiveStronghold` field. Copying `FGameState` does not clone a stronghold.
- `FSolidDungeon`, `FSolidFortress`, and `FSolidTower` stay plain classes. The stronghold pointer is not a `UPROPERTY`.
- A new company starts as the Meagre Dungeon on the starting cell, facing north, named The cave.
- A move onto a tower leaves the cave on `PreviousStrongholds` and the tower as the company's stronghold. Furnishings stay on `FGameState`. `MaxStrongholdSize` and `bStrongholdsEnabled` stay on `FGameState`.
- Leaving a stronghold still places the captain outside that stronghold's entrance. The terrain streamer reads the company's stronghold.
- The product commit names the stronghold tests it updates: `SolidStrongholdTests.cpp`, `SolidStrongholdTransitionTests.cpp`, `SolidStrongholdTypeTests.cpp`, `SolidTowerInteriorTests.cpp`, and `SolidDungeonExteriorTests.cpp`. It bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- Create the company the first time the game instance is asked. Seed the defaults and the starting stronghold then.
- `EnsureCompanyAndSquad` still creates the squad and calls `InitializeFromCompany`. That call still selects Sam, Alex, and the starter plans on the new squad. `RestoreTravelSession` still overwrites those indexes when a session exists. It does not copy a company and it does not copy an active stronghold.
- `Commit` takes the company's stronghold, appends it to `FGameState::PreviousStrongholds`, and stores the new stronghold on the company. The furnishings move stays on `FGameState`.
- The exit stand in `ASolidTerrainStreamer::TryRelocateFocusToStartTown` reads the company on the game instance.
- [Name the company's stronghold](name-the-stronghold.md) still stores a kept name on `FGameState`. This story does not add that name.
- A test that only needs a company calls `NewObject<USolidCompany>`. It does not need a game instance.
