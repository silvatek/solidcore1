# Remove den from the code

Status: Ready

## Story

A developer reads the stronghold C++ and does not find the word den. Comments and test labels that used it now say the cave, or the Meagre floor. The cave is the company's first stronghold. Its size is Meagre and its type is Dungeon. No type, function, or variable changes.

## Acceptance

- `Source/SolidCore1/Strongholds/SolidStronghold.h` says the visit view looks into the cave on entry.
- `Source/SolidCore1/Strongholds/SolidStrongholdGrid.h` says the company's first stronghold is fourteen irregular cells, not a filled rectangle. The cell list is the Meagre floor, mouth to the west.
- `Source/SolidCore1/Strongholds/SolidDungeonInterior.h` says Meagre, the cave, is fourteen irregular hexes. The captain's view looks into the cave.
- `Source/SolidCore1/Strongholds/SolidDungeonLevel.h` calls the ceiling the Meagre ceiling. The visit view turns the captain into the cave. Local +X runs into the cave. The step west of the mouth is outside the cave.
- `Source/SolidCore1/Strongholds/SolidTowerStronghold.h` says the Meagre floor stays the fourteen-cell outline.
- `Source/SolidCore1/Strongholds/SolidTowerInterior.h` says the tower visit is built the same way the cave is.
- `SolidCore1.Tests.SolidDungeonLevel` says the captain yaw faces into the cave. The yaw assertion is unchanged.
- `SolidCore1.Tests.SolidStrongholdGrid` says the Meagre floor is fourteen hexes, and that the floor has an outline. Both assertions are unchanged.
- A search of `Source` for the word den finds nothing. Identifiers stay as they are. `MeagreEntranceCell`, `MeagreCaptainCell`, and `FGameState::CaveChest` stay.

## Implementation notes

- The word is only in comments and in the `TEXT()` labels of those two tests. There is no `Den` type or function to rename.
- Changing a test label does not change what the test checks. Do not edit the assertions.
