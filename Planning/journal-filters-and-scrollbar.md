# Journal Filters and Scrollbar

Status: Ready

## Story

This story depends on [The game state owns the journal](Done/game-state-owns-the-journal.md), which is Done. That story ships first.

The player opens the journal. Every line stays. The list grows well past what fits on the board. The player filters the lines and moves through them with a scrollbar.

## Acceptance

- A new line does not drop an older one. The journal keeps every line.
- The board keeps the height the view already gives it. More lines do not make the board taller. The title, the filter words, the Time and Summary headings, and `F10 to close` stay in place. The lines between the headings and the close hint move.
- The filter words are All, Event, Stronghold, and Lair, in that order, under the title. The page opens on All. The chosen word uses the title color. The other words use the ink color.
- All shows every line, in the order they were written. A detail stays under its event.
- Event shows events and their details. Stronghold shows stronghold lines. Lair shows lair lines.
- Key 1 chooses All, key 2 chooses Event, key 3 chooses Stronghold, and key 4 chooses Lair. The number pad does the same. A click on a word chooses it. A click on the board away from those words leaves the filter as it is. Keys 5 through 8 leave the filter as it is.
- When the chosen lines fit, they start under the headings and there is no bar. When they do not fit, the latest matching line sits just above the close hint.
- Opening the journal shows All that way. Choosing a filter shows that filter that way.
- A filter with no lines shows one empty line and no bar. All says `No lines yet.` Event says `No events yet.` Stronghold says `No stronghold lines yet.` Lair says `No lair lines yet.`
- The bar is on the right of the moving lines, and only when those lines are taller than that area. The thumb is the fraction of those lines that is visible.
- Rolling the wheel up moves one line toward the older entries. Rolling it down moves one line toward the newer entries. Dragging the thumb moves the lines with the pointer. At either end, the window stays there.
- Esc and F10 close the journal. The next open starts on All, with the latest line in view when the list is taller than the area.

## Implementation notes

- `USolidJournal::Remember` drops the oldest line at `MaxEntries`, which is 64. Remove the cap and the constant. No current test asserts it. The lines stay `Entries` on that journal. This story does not add a save.
- `BoardSize` currently grows the height with the measured lines, then clamps it to the screen. Stop letting the line count raise the height. Keep the minimum, the view fraction, and the margin. Width may still follow the widest line, as it does now.
- The filter row, the column headings, and the close hint are outside the scrolling window. `DrawJournalBoard` stops at the hint today and drops the rest. Draw the window instead.
- `USolidJournal::CollectRows` gains the filter. The default is All, so the current call sites stay on the full list. A detail is a row under its event, not its own journal entry. Stronghold and Lair omit those details.
- The empty copy for All replaces `EmptyText`, which is `No events yet.` `SolidCore1.Journal.Empty` asserts that literal. Before the product change, change only that assertion so it reads `USolidJournal::EmptyText`. It passes on the current code and on this story. Push that commit alone. The product commit does not edit that test. It sets `EmptyText` to `No lines yet.` and adds the other three strings. `SolidCore1.Journal.TriggeredOrder` uses `No events yet.` only in `TestFalse` on a filled journal, so it still passes.
- The filter and the scroll offset live on the journal page. `SetJournalVisible(true)` selects All and scrolls to the latest line. Closing the page discards both.
- The main menu closes before the journal opens. `ChooseMainMenuIndex` already returns when the menu is closed, and keys 1–8 and the number pad call it. Handle the journal there, before the menu check. Keys 5–8 do nothing on this page.
- The board stays canvas-drawn. Filter words and the thumb are rectangles in that same pixel space. A press on the thumb drags until release. The wheel already reaches this page: zoom returns while any menu overlay is open, and the journal is one.
- Scroll by rows. One wheel notch is one row. A drag maps the thumb's place on the track to the first visible row. When the rows fit, the offset is the top and the bar is absent.
- Existing journal tests keep passing, apart from the empty-copy commit above. The product commit adds tests for the cap, the four filters, the empty lines, the window at the latest line, the wheel, and the drag. Those tests do not spawn PIE.
