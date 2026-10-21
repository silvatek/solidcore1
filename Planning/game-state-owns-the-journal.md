# The game state owns the journal

Status: New

## Story

[The game instance owns the play state](Done/game-instance-owns-the-play-state.md) is Done. The session has one `USolidGameState`. The journal is still an array on that object.

A developer gives that play state one journal. `USolidJournal` is a `UCLASS`. `USolidGameState` owns it. A cave trip and a tower trip keep that object.

The journal holds the lines, and the journal code that writes and reads those lines moves onto that class. The HUD still paints the page.

## Acceptance

- `USolidGameState` creates one `USolidJournal` and holds it with `UPROPERTY`. `OpenLevel` keeps it. Callers ask the game state for the journal.
- `USolidGameState` has no `Journal` array.
- `USolidJournal` holds the lines in `Entries`, a `TArray` of `FJournalEntry`. A new journal has no lines. `Entries` is not a `UPROPERTY`. `FJournalEntry` stays a plain struct.
- `EJournalType` stays Stronghold, Event, and Lair. A line still has the year, the day, the minute, the event index, and the visit text. Those types live in the journal class header. `SolidJournalEntry.h` goes away.
- `Remember`, `RememberStronghold`, and `RememberLair` are methods on `USolidJournal`. They leave `SolidEvents`. An event line, a stronghold line, and a lair line are unchanged. A caller that has the play state asks it for the journal, then calls the method.
- `Describe` and `CollectRows` are methods on `USolidJournal`. `CollectRows` reads this journal. It no longer takes the play state.
- `BoardSize`, `TitlePlate`, and `Columns` are static methods on `USolidJournal`. The HUD and the inventory page call those methods. They do not keep a copy.
- The `SolidJournal` namespace goes away. Its row types and its page constants live with the class.
- The journal still drops the oldest line at 64. This story does not remove that cap.
- `CollectRows` still builds the same page: the title, Time and Summary, the lines in the order they were written, a detail under an event, and `F10 to close`. An empty journal still says `No events yet.`
- `DrawJournalBoard` stays in the HUD. It draws the rows the journal returns. It does not record a line or build one.
- `FSolidTravelSession` does not contain the journal.
- After a cave trip and after a tower trip, the same journal still has the lines.
- A test that needs a journal uses `NewObject<USolidJournal>`. It does not need a game state.
- The product commit names the tests it updates: `SolidEventTests.cpp`, `SolidForageTests.cpp`, `SolidJournalTests.cpp`, `SolidLairTests.cpp`, `SolidPortalTests.cpp`, and `SolidSquadBuilderTests.cpp`. It bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.
- The README data structure lists `USolidJournal` under `USolidGameState`, in place of `FJournalEntry[]`.

## Implementation notes

- Create the journal the first time the game state is asked, the way the company creates the captain. The header and the `.cpp` are `Source/SolidCore1/Journal/SolidJournal.h` and `SolidJournal.cpp`. The class replaces the namespace in those files.
- `Remember` appends to `Entries` and still drops the oldest line at 64. `MaxEntries` moves with that method.
- An event still fires on the play state. The line it used to write through `SolidEvents::Remember` is `USolidJournal::Remember`.
- `Describe` does not use the stored lines. It stays a method on the class because the detail text is journal text.
- The canvas draw stays in `SolidHUD.cpp`. Moving it onto the journal would pull `UCanvas` into the play state.
- [Journal Filters and Scrollbar](journal-filters-and-scrollbar.md) still describes the lines as the array on the play state, and it removes the cap of 64. After this story those lines are `Entries` on `USolidJournal`, and the page methods are on that class. This story does not add the filters or the scrollbar.
