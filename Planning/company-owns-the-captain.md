# The company owns the captain

Status: In Progress

## Story

[The game instance owns the play state](Done/game-instance-owns-the-play-state.md) is Done. The session has one `USolidGameState`, and the company already lasts for the session.

A developer gives that company one captain. `USolidCaptain` is a `UCLASS`. `USolidCompany` owns it. A cave trip and a tower trip keep that object.

The captain holds the title, the occupied town, and the sight-enabled flags. Those fields leave `USolidGameState`. The titles he can hold are an enum in the captain's file: Outcast, Orphan, Vagrant, Ranger, and Captain. The live body stays `ASolidCharacter`. The captain keeps a weak reference to that actor for the current world. The mesh stays the Viking mesh the actor loads.

## Acceptance

- `USolidCompany` creates one `USolidCaptain` and holds it with `UPROPERTY`. `OpenLevel` keeps it. Callers ask the company for the captain.
- `USolidGameState` has no `CaptainTitle`, no `OccupiedTown`, no `bTrueSightEnabled`, and no `bRavenSightEnabled`.
- `ESolidCaptainTitle` is an enum class in `SolidCaptain.h`, the file that defines `USolidCaptain`. The values are Outcast, Orphan, Vagrant, Ranger, and Captain.
- The captain's title is that enum. A new captain is Outcast, on no town, with true sight enabled and raven sight locked.
- `SolidEvents::TitleOutcast`, `TitleOrphan`, `TitleVagrant`, `TitleRanger`, and `TitleCaptain` go away. `SetPlayerTitle`, `FChange::Title`, and `FResult::CaptainTitle` use `ESolidCaptainTitle`.
- That same file returns the nameplate word for a value: Outcast, Orphan, Vagrant, Ranger, and Captain. The pawn shows that word. A journal title line is still `Title ` plus that word.
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
- `FResult::CaptainTitle` is the `ESolidCaptainTitle` an event just set. The pawn shows the word that file returns for it. The kept title is the enum on the captain. The event header includes the captain header for the enum. The captain header does not include the event header.
- The weak actor pointer is how the captain finds his body in this world. Companions still follow that actor. They do not follow the `USolidCaptain` object.
- The mesh stays off this object. The company catalog's Viking entry is a mesh a companion can wear. The captain stays on `SK_Viking`, loaded by name in `ApplyCharacterVisuals`. A mesh pointer here would keep an asset the pawn already loads.
- `OccupiedTown` stays the numbered cell. This story does not turn it into a world position.
- [Falcon Sight and Eagle Sight](falcon-and-eagle-sight.md) adds sight modes. This story moves the two flags that exist. A later unlock flag for a sight belongs on the captain with these two.
