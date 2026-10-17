# Stronghold transition

## Story

The company has one stronghold. At the start the player uses the Meagre Dungeon. An event unlocks a larger stronghold size. The player chooses a type and a place, and the company moves into a new stronghold of that size. The furniture from the old stronghold is in the new one, including everything that was stored in it. The company has the new stronghold. The old one is no longer theirs.

## Acceptance

- The company has one active stronghold.
- A new game's active stronghold is the Meagre Dungeon, the cave the company already has.
- Until an event unlocks a larger size, the player cannot start a transition.
- An event can unlock a size above the active stronghold's size.
- After that unlock, the player chooses a stronghold type and a place and completes the transition. The new stronghold is that type, that unlocked size, at that place.
- The player cannot transition to a size that is not unlocked, or to a size that is not larger than the active stronghold.
- After the transition, the active stronghold is the new one. The previous stronghold is not active.
- A later event can unlock a still larger size. The player can transition again, and the furniture moves from the stronghold that is active then.
- Every furnishing that was in the old stronghold is in the new one.
- The contents of each storage container move with that container: the same items, in the same slots, with the same counts.
- Those furnishings and those contents are no longer in the old stronghold.
- The captain's pack and worn items stay with the captain.

## Implementation notes

- `SolidStronghold::Starting` is the Meagre Dungeon. Sizes run Meagre, Humble, Average, Impressive, Grand, Stately, Imperious. Larger means later in that list. Types are Dungeon, Fortress, and Tower.
- The size list already says later upgrades move up the list. `EChange` has no stronghold change, and no event unlocks a size. Add a change that unlocks one size. Do not attach it to Enter Iglin, Relion, Kanfold, or Dragongate in this story. Which event fires it is not chosen.
- `FGameState` does not record an active stronghold. The cave is implied by `EnableCave` and `CaveChest`. Record the active stronghold, including type, size, and place, and carry it on the travel session the way the company is carried.
- Furnishings for the Meagre Dungeon are the fireplace, the bed, and the chest in `UsedFurnishings`. The chest is the only storage. Its contents are `FGameState::CaveChest`: the rusty sword, old boots, coins, sticks, and pitch at the start. The fireplace and the bed are not storage.
- Those pieces sit on the den's hexes. Another plan does not have those cells. The pieces and the container contents still move. This story does not invent a hex layout for every size. When the new interior exists, place the pieces on kept cells of that interior.
- The Imperious Tower on the map is a visit, not the company's stronghold. Entering it does not make it active and does not take the den's furniture. This story does not remove that marker or that door.
- F10 Enter cave still opens the Meagre den while that den is the active stronghold. This story does not retarget that row.
- Lairs are not strongholds. The Iglin tunnels stay as they are.
- Fortress interiors, dungeon sizes above Meagre, and tower sizes other than the shipped Imperious visit are their own stories. Choosing one of those types records it as the active stronghold and moves the furniture and the contents. It does not build that interior here.
- Do not add a world-map `!`. The owner places markers. The place the player chooses is stored with the active stronghold.
