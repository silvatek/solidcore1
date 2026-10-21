# Company lists use TArray

Status: Done

## Story

A developer stores the company's runtime lists in `TArray`. The lairs an event has opened, and the order events have fired, are those lists. The length of each list is the array's length. A lair still opens once. An event still records once, in the same order. Play is unchanged.

## Acceptance

- `FGameState::EnabledLairs` is a `TArray` of lair ids. `EnabledLairCount` is gone. `IsLairEnabled` reads the array. An EnableLair change appends an id that is not already there, and stops at `MaxEnabledLairs`.
- `FGameState::FiredOrder` is a `TArray` of definition indexes. `FiredCount` is gone. Firing an event appends that index, and stops at `MaxEvents`.
- `bFired` stays a bool for each definition index. Its length stays `MaxEvents`.
- Copying `FGameState` copies both lists, and still clones the active stronghold and each previous stronghold.
- Recipe inputs, event changes, event requirements, inventory slots, chest slots, the lair maze, catalogs sized with `UE_ARRAY_COUNT`, and fixed geometry such as hex corners stay C arrays.
- The same events fire in the same order. The Iglin tunnels open from the same change.
- `CodingGuidelines.md` says a runtime list is a `TArray`, and a fixed slot, a compile-time table, and a geometric tuple stay a C array.
- The product commit updates `SolidEventTests.cpp`, `SolidPortalTests.cpp`, and `SolidBattlePlanTests.cpp`, and names those tests. It bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- Use `TArray` with `TInlineAllocator` sized to the existing cap. A travel copy of the company state then keeps these lists beside the other fields. The cap check stays before `Add`. `Num()` replaces the count fields.
- `FEvent` and `FSolidRecipe` are constexpr and copied by value while their tables are built. `TArray` is not a compile-time value, so `Changes`, `Requires`, and `Inputs` stay.
- A slot's index is the slot. Worn, pack, craft, chest, and `bFired` stay.
- A catalog's length is `UE_ARRAY_COUNT`. Those tables stay.
- A hex has six corners and a triangle has three. Those locals stay.
- Call sites are the copy in `SolidGameState.cpp`, `TryFireEvent`, the EnableLair arm of `ApplyChange`, and `IsLairEnabled`.
- Do not add `FiredCount()` or `EnabledLairCount()`. A name only the tests call is unused.
