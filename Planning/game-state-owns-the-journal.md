# The game state owns the journal

Status: New

## Story

[The game instance owns the play state](Done/game-instance-owns-the-play-state.md) is Done. The session has one `USolidGameState`. The journal is still an array on that object.

A developer gives that play state one journal. `USolidJournal` is a `UCLASS`. `USolidGameState` owns it. A cave trip and a tower trip keep that object.

The journal holds the lines. The page that draws them stays the `SolidJournal` namespace.

## Acceptance

- `USolidGameState` creates one `USolidJournal` and holds it with `UPROPERTY`. `OpenLevel` keeps it. Callers ask the game state for the journal.
- `USolidGameState` has no `Journal` array.
- `USolidJournal` holds the lines in `Entries`, a `TArray` of `SolidJournal::FJournalEntry`. A new journal has no lines. `Entries` is not a `UPROPERTY`. `FJournalEntry` stays a plain struct.
- `EJournalType` stays Stronghold, Event, and Lair. A line still has the year, the day, the minute, the event index, and the visit text.
- `SolidEvents::Remember`, `RememberStronghold`, and `RememberLair` still write those lines, on the journal. An event line, a stronghold line, and a lair line are unchanged.
- The journal still drops the oldest line at 64. This story does not remove that cap.
- `CollectRows` still builds the same page: the title, Time and Summary, the lines in the order they were written, a detail under an event, and `F10 to close`. An empty journal still says `No events yet.`
- `FSolidTravelSession` does not contain the journal.
- After a cave trip and after a tower trip, the same journal still has the lines.
- A test that needs a journal uses `NewObject<USolidJournal>`. It does not need a game state.
- The product commit names the tests it updates: `SolidEventTests.cpp`, `SolidForageTests.cpp`, `SolidJournalTests.cpp`, `SolidLairTests.cpp`, and `SolidPortalTests.cpp`. It bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.
- The README data structure lists `USolidJournal` under `USolidGameState`, in place of `FJournalEntry[]`.

## Implementation notes

- Create the journal the first time the game state is asked, the way the company creates the captain. The header and the `.cpp` are `Source/SolidCore1/Journal/SolidJournal.h` and `SolidJournal.cpp`, beside the page namespace that already lives there.
- `SolidJournalEntry.h` still defines `FJournalEntry` and `EJournalType`. This story does not move them.
- `Remember` appends to `Entries` and still drops the oldest line at `SolidJournal::MaxEntries`.
- `CollectRows` still takes the play state. It reads the lines from the journal that state owns.
- The board, the rows, `Describe`, and the column layout stay the `SolidJournal` namespace. This story does not change the page.
- [Journal Filters and Scrollbar](journal-filters-and-scrollbar.md) still describes the lines as the array on the play state, and it removes the cap of 64. After this story those lines are `Entries` on `USolidJournal`. This story does not add the filters or the scrollbar.
