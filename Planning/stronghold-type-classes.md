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
- Each subclass implements `Supported(ESolidStrongholdSize)`. It returns true when the current code can build a stronghold of that type at that size.
- Dungeon `Supported` is true for Meagre only. It is false for Humble, Average, Impressive, Grand, Stately, and Imperious.
- Tower `Supported` is true for Average, Impressive, Grand, Stately, and Imperious, the sizes `PlanFor` already accepts. It is false for Meagre and Humble.
- Fortress `Supported` is false for every size.
- The stronghold transition type-picker uses `Supported`. A type is listed only when that subclass returns true for the site's size. The picker still lists a type only when the site size is unlocked and larger than the active stronghold. The list is the same as today: Fortress is absent, a dungeon is absent, and a tower from Average through Imperious is present when the click would have offered one.
- The dungeon subclass uses the cliff mouth. Its visit is the cave trip. Its ground floor is the fourteen-hex cave.
- The tower subclass does not use the cliff mouth. Its visit is the tower trip. Its ground floor is the hexes that plan already keeps.
- The fortress subclass does not use the cliff mouth. It has no visit and no ground floor.
- Asking whether a size is supported, whether the type uses the cliff mouth, which trip it takes, and which ground hexes it keeps goes through the subclass. Those four answers match the functions the game uses today.
- The shared 1.2 m hex grid and the furnishing catalog stay shared. This story does not add a piece and does not move the cave's fireplace, bed, or chest.

## Implementation notes

- The base declares `Supported(ESolidStrongholdSize)`. Each subclass implements it. `TypeOf` returns a stable instance of the subclass for that enum. Call sites that answer one of those four questions call the subclass. They do not add a new `switch` on the enum for that question.
- `Consider` is the type-picker. It calls `Supported` on the subclass for the site size. It does not call `IsBuilt` to build that list. The unlocked-size check and the larger-than-active check stay in `Consider`.
- `IsBuilt` still requires `IsFinalPair`. The "already has an interior" half asks `Supported`, so `IsBuilt` keeps its current results. `UsesCaveMouth`, `TravelToDungeon`, and `TravelToTower` keep theirs. `Commit` asks the subclass for the ground hexes instead of calling `PlanFor` itself. A tower still supplies the hexes `CollectKeptHexes` already returns. A pair with no ground hexes still refuses the move.
- Leave the tower plan math in `SolidTowerStronghold`. Leave the cave meshes in `ASolidDungeonInterior`. The subclasses call that code. This story does not merge the actors and does not add a fortress actor.
- `IsFinalPair` stays the list of pairs the game will one day offer. `Supported` is whether the code can build that size now. A final pair can still be unsupported. Humble Dungeon is one.
- [Fortress stronghold](fortress-stronghold.md) fills in the fortress subclass. That is what makes `Supported` true from Humble through Imperious. Meagre stays false. [Imperious dungeon](imperious-dungeon.md) is what makes dungeon `Supported` true for Imperious and supplies that floor. This story does not. A size has one hex total, shared by every type that supports it. From Average through Imperious the tower plan is that total. Dungeon and fortress layouts meet it. Those later stories keep that. This story does not change a hex count.
- Existing stronghold tests keep passing. They do not need a prior commit. The product commit adds tests that `TypeOf` returns the subclass, that `Supported` matches the sizes above, and that `Consider` lists a type only when `Supported` is true. Those tests do not spawn PIE.
