# Remove den from the code

Status: Done

## Story

A developer reads the stronghold C++ and does not find the word den. The company's first stronghold is the cave: size Meagre, type Dungeon. Comments about that interior say cave. Comments and test labels about its fourteen-cell plan say Meagre floor, the floor, or Meagre ceiling. Labels about the company moving from one base to another say stronghold. No type, function, or variable changes.

## Acceptance

- A search of `Source` for the word den finds nothing.
- `MeagreEntranceCell`, `MeagreCaptainCell`, and `FGameState::CaveChest` stay. No type, function, or variable was renamed.
- Interior comments, and the dungeon-level yaw label, say cave. The yaw check is unchanged.
- The fourteen-cell plan is the Meagre floor. Its ceiling comment says Meagre ceiling. The grid tests say the Meagre floor is fourteen hexes and the floor has an outline. Those checks are unchanged.
