# The company owns the captain

Status: Ready

## Story

[The game instance owns the play state](Done/game-instance-owns-the-play-state.md) is Done. The session has one `USolidGameState`, and the company already lasts for the session.

A developer gives that company one captain. `USolidCaptain` is a `UCLASS`. `USolidCompany` owns it. A cave trip and a tower trip keep that object.

The captain holds the title, the occupied town, and the sight-enabled flags. Those fields leave `USolidGameState`. The live body stays `ASolidCharacter`. The captain keeps a weak reference to that actor for the current world. The mesh stays the Viking mesh the actor loads.

## Acceptance

- `USolidCompany` creates one `USolidCaptain` and holds it with `UPROPERTY`. `OpenLevel` keeps it. Callers ask the company for the captain.
- `USolidGameState` has no `CaptainTitle`, no `OccupiedTown`, no `bTrueSightEnabled`, and no `bRavenSightEnabled`.
- A new captain starts as Outcast, on no town, with true sight enabled and raven sight locked.
- `CaptainTitle` stays a `const TCHAR*` ordinary member. It points at the same title literals. It is not a `UPROPERTY`.
- Entering a town still sets the title and the sight flags the events name. The occupied town is still the numbered cell, or `INDEX_NONE`.
- After a cave trip and after a tower trip, the same captain still has the title, the occupied town, and the sight flags.
- The current sight mode stays on the actor and on `FSolidTravelSession::Sight`. The travel session does not copy the title, the town, or the sight flags.
- The captain holds the live actor with `TWeakObjectPtr<ASolidCharacter>`. The pawn sets it when it is possessed and clears it when it ends. The reference is not a `UPROPERTY`. `OpenLevel` still destroys the pawn. The next world sets the new pawn.
- The captain does not store a mesh. `ApplyCharacterVisuals` still loads `SK_Viking`.
- `ASolidGameMode::IsSightEnabled` reads the captain. The nameplate still shows the captain's title.
- A test that needs a captain uses `NewObject<USolidCaptain>`. It does not need a company.
- The product commit names the tests it updates: `SolidBattlePlanTests.cpp`, `SolidEventTests.cpp`, `SolidJournalTests.cpp`, `SolidLairTests.cpp`, and `SolidPortalTests.cpp`. It bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- Create the captain the first time the company is asked, the way the game instance creates the company. The header and the `.cpp` are `Source/SolidCore1/Squad/SolidCaptain.h` and `SolidCaptain.cpp`.
- `NotifyOccupiedTown` and `ApplyChange` write the title, the town, and the sight flags on the captain. They still write the journal, the fired events, and the other play state on `USolidGameState`. A notify that applies changes passes the captain through.
- `FResult::CaptainTitle` stays the title an event just set, so the pawn can copy it onto the nameplate. The kept title is the field on the captain.
- The weak actor pointer is how the captain finds his body in this world. Companions still follow that actor. They do not follow the `USolidCaptain` object.
- The mesh stays off this object. The company catalog's Viking entry is a mesh a companion can wear. The captain stays on `SK_Viking`, loaded by name in `ApplyCharacterVisuals`. A mesh pointer here would keep an asset the pawn already loads.
- `OccupiedTown` stays the numbered cell. This story does not turn it into a world position.
- [Falcon Sight and Eagle Sight](falcon-and-eagle-sight.md) adds sight modes. This story moves the two flags that exist. A later unlock flag for a sight belongs on the captain with these two.
