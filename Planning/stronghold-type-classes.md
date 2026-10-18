# Stronghold type classes

Status: Ready

## Story

A developer adds each stronghold type as a subclass of one shared base. Dungeon, Fortress, and Tower each have their own subclass.

This is a refactor. In PIE the cave and the towers look and behave as they do now. Nothing new appears. The subclasses are the shared code the later stronghold stories fill in. [Fortress stronghold](fortress-stronghold.md) is the fortress visit. [Imperious dungeon](imperious-dungeon.md) is the larger dungeon floor.

## Acceptance

- PIE is unchanged. The Meagre cave mouth, the cave interior, the tower shell, the tower door, the tower interior, travel into and out of each, the visit camera, the furnishings, the journal lines, the F10 menu, and the world map are the same.
- No fortress exterior and no fortress interior appear. A site click still does not offer Fortress. No new `!` line is added.
- One C++ base, and three subclasses: Dungeon, Fortress, and Tower. They are not Unreal objects and not Blueprints.
- `ESolidStrongholdType` stays. `FSolidStronghold` stays the type and the size. Map tokens, labels, shelves, and size unlocks stay on that enum.
- A lookup by the enum returns that type's subclass. Dungeon returns the dungeon subclass, Fortress the fortress subclass, and Tower the tower subclass.
- The dungeon subclass uses the cliff mouth. It is built for Meagre only. Its visit is the cave trip. Its ground floor is the fourteen-hex cave.
- The tower subclass does not use the cliff mouth. It is built for Average through Imperious, the sizes `PlanFor` already accepts. Its visit is the tower trip. Its ground floor is the hexes that plan already keeps.
- The fortress subclass does not use the cliff mouth. It is built for no size. It has no visit and no ground floor.
- Asking whether a pair is built, whether it uses the cliff mouth, which trip it takes, and which ground hexes it keeps goes through the subclass. Those four answers match the functions the game uses today.
- The shared 1.2 m hex grid and the furnishing catalog stay shared. This story does not add a piece and does not move the cave's fireplace, bed, or chest.

## Implementation notes

- The base is a C++ class with those four questions. `TypeOf` returns a stable instance of the subclass for that enum. Call sites that answer one of those four questions call the subclass. They do not add a new `switch` on the enum for that question.
- `UsesCaveMouth`, `IsBuilt`, `TravelToDungeon`, and `TravelToTower` keep their current results. `UsesCaveMouth` and `IsBuilt` delegate to the subclass. `Commit` asks the subclass for the ground hexes instead of calling `PlanFor` itself. A tower still supplies the hexes `CollectKeptHexes` already returns. A pair with no ground hexes still refuses the move.
- Leave the tower plan math in `SolidTowerStronghold`. Leave the cave meshes in `ASolidDungeonInterior`. The subclasses call that code. This story does not merge the actors and does not add a fortress actor.
- `IsFinalPair` stays the list of pairs the game will one day offer. The fortress subclass does not make a fortress built.
- [Fortress stronghold](fortress-stronghold.md) fills in the fortress subclass. [Imperious dungeon](imperious-dungeon.md) is what marks a larger dungeon built and supplies its floor. This story does not.
- Existing stronghold tests keep passing. They do not need a prior commit. The product commit adds tests that `TypeOf` returns the subclass and that the four answers match the current mouth, built pairs, trip, and ground hexes. Those tests do not spawn PIE.
