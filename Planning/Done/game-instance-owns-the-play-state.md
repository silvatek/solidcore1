# The game instance owns the play state

Status: Done

## Story

[The game instance owns the company](Done/game-instance-owns-the-company.md) is Done. The company already lasts for the play session, and it owns the active stronghold.

A developer gives that same session one play state. `USolidGameState` is a `UCLASS`. `USolidGameInstance` owns it. A cave trip and a tower trip keep that object. The game mode does not keep a copy, and the travel session does not carry one.

The play state still holds the previous strongholds, the furnishings, the inventory, the chest, the journal, and the event record.

## Acceptance

- `USolidGameInstance` creates one `USolidGameState` and holds it with `UPROPERTY`. `OpenLevel` keeps it. `ASolidGameMode` has no `FGameState`. `GetConfiguration()` returns the instance's state.
- `FSolidTravelSession` does not contain the play state. `SaveTravelSession` and `RestoreTravelSession` still carry the squad indexes, the sight, and the visit flags.
- After a cave trip and after a tower trip, the same object still has the inventory, the journal, the chest, the taken forage piles, the fired events, the previous strongholds, and the furnishings.
- `PreviousStrongholds` stays a `TArray` of `TUniquePtr<FSolidStronghold>`. It is not a `UPROPERTY`. `FSolidDungeon`, `FSolidFortress`, and `FSolidTower` stay plain classes. `CaptainTitle`, the lair ids, and `bFired` stay ordinary members.
- There is no copy of the play state. `Commit` still moves the company's old stronghold onto `PreviousStrongholds`.
- A test that needs a play state uses `NewObject<USolidGameState>`. It does not need a game instance.
- The product commit names the tests it updates: `SolidBattlePlanTests.cpp`, `SolidEventTests.cpp`, `SolidForageTests.cpp`, `SolidInventoryTests.cpp`, `SolidJournalTests.cpp`, `SolidLairTests.cpp`, `SolidPortalTests.cpp`, `SolidStorageTests.cpp`, `SolidStrongholdTests.cpp`, `SolidStrongholdTransitionTests.cpp`, `SolidStrongholdTypeTests.cpp`, and `SolidTowerInteriorTests.cpp`. It bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- Create the state the first time the game instance is asked, the way `GetCompany` creates the company.
- `FGameState` goes away. A function that took `FGameState&` takes `USolidGameState&`.
- The copy constructor and `operator=` go away with the value. A trip no longer clones `PreviousStrongholds`.
- [Name the company's stronghold](name-the-stronghold.md) stores a kept name on the play state. After this story that field is on `USolidGameState`. This story does not add the name.
- An open story that says the travel session copies `FGameState` is describing these same fields. The session no longer copies them.
