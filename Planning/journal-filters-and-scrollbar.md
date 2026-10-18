# Journal Filters and Scrollbar

Status: New

## Story

The player opens the journal. Every line stays. The list grows well past 64 lines, and well past what fits on the board. The player filters the lines and moves through them with a scrollbar.

## Acceptance

- A new line does not drop an older one. The journal keeps every line.

The rest of the acceptance is not written yet.

## Implementation notes

- `SolidJournal::MaxEntries` is 64. `Remember` drops the oldest line at that count. This story removes that cap. No current test asserts it.
- The board stays the size it is now. The scrollbar moves the lines that do not fit. The filter narrows a long list. Repeated stronghold and lair trips are what fill it.
