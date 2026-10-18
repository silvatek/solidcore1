# Return to the stronghold

Status: New

## Story

Closing the monster message enables the Meagre size and returns the captain to the company stronghold. That stronghold is the cave. `EnableCave` is not the change that does this. The close uses `EnableStrongholdSize(ESolidStrongholdSize::Meagre)` and a new `ReturnToStronghold` change. Enter Dragongate no longer enables Meagre. The other sizes stay on that event until each has its own.

## Acceptance

- `IdKilledByMonsters` carries `EnableStrongholdSize(ESolidStrongholdSize::Meagre)`, `ReturnToStronghold`, and `ClearInventory`. It does not carry `EnableCave`.
- `EnableCave` is gone. A search of `Source` does not find that name.
- Before that close, Meagre is locked. The cave mouth ignores a click, and F10 Enter cave does nothing.
- The close enables Meagre. While the cave is still the company stronghold, the mouth accepts a click and F10 Enter cave enters.
- `ReturnToStronghold` sends the captain into the company stronghold. On this close, that is the cave. After the company has moved, the same change enters the stronghold they have then.
- Once the company has left the cave, the mouth and F10 Enter cave stay shut. Meagre stays enabled.
- Enter Dragongate does not list Meagre. It still lists Humble, Average, Impressive, Grand, Stately, and Imperious. F5 Parade does not enable Meagre and does not open the cave.
- The journal for that close records the Meagre size and the return. It does not say Cave enabled.
- Driven from the village still requires `IdKilledByMonsters`. The close still clears the inventory to Rags.

## Implementation notes

- `EnableCave` sets `bCaveEnabled` and `FResult.bEnterCave`. `PresentEventResult` calls `TravelToDungeon` when `bEnterCave` is set. `TravelToDungeon` returns immediately unless `bCaveEnabled` is set.
- `CompanyCaveAcceptsEnter`, the F10 row (`CanUseCaveRow`), the menu dimming, and the mouth passage read `bCaveEnabled`. After this story the cave is open when Meagre is enabled and the cave is still the company stronghold. `bCaveEnabled` can go once those readers have moved.
- `ReturnToStronghold` is the trip. It replaces `bEnterCave`. While the company stronghold is the Meagre cave, it uses the cave trip. When the active stronghold is a tower, it uses that tower's trip.
- Enter Dragongate already lists `EnableStrongholdSize(Meagre)`. Remove that one line here. Leave the other six sizes.
- Do not use the word den. See [Remove den from the code](remove-den-from-the-code.md).
- Tests that expect `EnableCave`, `bEnterCave`, or `bCaveEnabled` after the monster close change with this behavior. Follow the test policy in `AGENTS.md`.
