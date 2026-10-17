# Stronghold transition

## Story

The company has one stronghold. At the start the player uses the Meagre Dungeon. The world map has stronghold sites. Each site names a size and shows a stone block. An event unlocks a larger stronghold size. The player clicks the stone at a site of that size and chooses a type. That starts a stronghold of the site's size there, and the company moves from the stronghold they have. The furniture goes with them, including everything that was stored in it. The company has the new stronghold. The old one is no longer theirs. Its entrance stays where it is, and the player can no longer go in. The pointer shows no enter tip there, and a click does nothing. The Imperious Tower location comes off the map.

## Acceptance

- The company has one active stronghold.
- A new game's active stronghold is the Meagre Dungeon, the cave the company already has. Its line stays `! 9, 17 = Stronghold (MeagreDungeon)`.
- The world map has stronghold sites. A site line is `StrongholdSite` and a size name, as in `StrongholdSite (Humble)`. It names a size and does not name a type.
- Each site shows a stone block.
- A click on that block asks the player to choose a type: Dungeon, Fortress, or Tower.
- That choice starts creating a stronghold at that site and transitioning from the active one.
- Until an event unlocks a larger size, the choice does not create a stronghold.
- An event can unlock a size above the active stronghold's size.
- The player does not choose the size at the stone. The new size is the size named on that site.
- A site whose size is not unlocked, or is not larger than the active stronghold, does not accept the transition.
- After the transition, the active stronghold is the chosen type, the site's size, at the site they clicked. The previous stronghold is not active.
- The previous stronghold's exterior stays. The Meagre cave mouth stays when that was the one they left. A tower shell stays when a tower was the one they left.
- The pointer shows no enter tip on an entrance the company has left. A click on it does not enter.
- F10 Enter cave does not enter a stronghold the company has left.
- A later event can unlock a still larger size. The player can click another site and transition again, and the furniture moves from the stronghold that is active then.
- Every furnishing that was in the old stronghold is in the new one.
- The contents of each storage container move with that container: the same items, in the same slots, with the same counts.
- Those furnishings and those contents are no longer in the old stronghold.
- The captain's pack and worn items stay with the captain.
- `! 17, 31 = Stronghold (Imperious Tower)` is removed. That cell is no longer a tower the player can enter.

## Implementation notes

- `SolidStronghold::Starting` is the Meagre Dungeon. Sizes run Meagre, Humble, Average, Impressive, Grand, Stately, Imperious. Larger means later in that list. Types are Dungeon, Fortress, and Tower.
- The size list already says later upgrades move up the list. `EChange` has no stronghold change, and no event unlocks a size. Add a change that unlocks one size. Do not attach it to Enter Iglin, Relion, Kanfold, or Dragongate in this story. Which event fires it is not chosen.
- `FGameState` does not record an active stronghold. The cave is implied by `EnableCave` and `CaveChest`. Record the active stronghold, including type, size, and site, and carry it on the travel session the way the company is carried.
- A `!` line today is `Town`, `Stronghold`, `LairEntrance`, or `LairExit`. `Stronghold` is size then type, as in `MeagreDungeon` and `Imperious Tower`. A site is a new kind, `StrongholdSite`, then a size name and no type. `SolidStronghold::FromToken` will not parse `Humble` alone, because it expects size and type joined. Match the size names in `SolidStronghold::Sizes`. `ESolidWorldBang` and `WorldMapParseBangLocation` need the new kind. The suite line in `README.md` that lists those four kinds needs the same word.
- The site cells are not named. Do not invent them. The owner places the `!` lines. Do not delete `! 17, 31` in a commit that has not added those sites. The tower line comes out once the sites are the places a new stronghold is created.
- The stone is a block at the site cell. It is not the cave mouth and not the tower shell. A click asks for the type, then runs the transition when that site's size is unlocked and larger than the active stronghold.
- Furnishings for the Meagre Dungeon are the fireplace, the bed, and the chest in `UsedFurnishings`. The chest is the only storage. Its contents are `FGameState::CaveChest`: the rusty sword, old boots, coins, sticks, and pitch at the start. The fireplace and the bed are not storage.
- Those pieces sit on the den's hexes. Another plan does not have those cells. The pieces and the container contents still move. This story does not invent a hex layout for every size. When the new interior exists, place the pieces on kept cells of that interior.
- The tower plan, shell, and interior stay in the code. Choosing Tower uses them. Removing the map line only takes away the visit at cell 17,31. The comment in `SolidTowerStronghold.h` that the door on that cell faces north goes with the line.
- F10 Enter cave still opens the Meagre den while that den is the active stronghold. Once the company has left it, that row does not enter it. This story does not retarget the row to the new stronghold.
- The enter tips are `Enter cave` on the den mouth and `Enter Stronghold` on a tower door. A stronghold the company has left gives neither. The mouth, the shell, and the door geometry stay. They no longer count as a passage hit.
- Lairs are not strongholds. The Iglin tunnels stay as they are.
- Fortress interiors, dungeon sizes above Meagre, and a tower the player creates somewhere other than cell 17,31 are their own stories where the interior is not already built. Choosing one of those types records it as the active stronghold and moves the furniture and the contents. It does not build that missing interior here.
- `Planning/stronghold-markers-size-only.md` is this same marker. The line sets the size and does not set the type.
