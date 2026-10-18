# Remove unused code

Status: In Progress

## Story

A developer removes C++ symbols that product code never calls. A symbol that only its own tests call is unused, under the definition in `AGENTS.md`. Symbols the Unreal Engine calls stay.

The game already performs the work these symbols wrap or inspect. Removing them leaves that work in place.

## Acceptance

- Each symbol below is gone from `Source/`, including tests. A whole-word search for the name finds nothing under `Source/`.
- `override` methods stay. `UFUNCTION` methods the engine can call stay. Types that `Config/` names for the engine to instantiate stay.
- These product calls stay, and still behave as they do now: `SolidInventory::TakeFromChest`, `ASolidGameMode::MoveCarriedItem`, `ASolidTerrainStreamer::TryRelocateFocusToStartTown`, `SolidMainMenu::ControllerKey`, `SolidSight::ControllerToggleKey`, `USolidWorldMap::GetStartTownWorldXY`, `SolidStrongholdGrid::MeagreCells`, and `SolidLair::IsOpenCell`.
- Mesh components, floor plans, and maze cells stay. Only the unused readers and counters go.
- Existing tests keep passing. The test edit is its own commit, and it passes on the code before the deletion. The deletion commit does not edit those tests. The deletion commit says that the tests changed because these symbols went away.
- The deletion commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

The list is the result of applying the `AGENTS.md` definition. A use is a whole-word name in product C++ outside `Tests/`, in `Config/` for a type the engine loads, or in a `.Build.cs` / `.Target.cs` file. A test file belongs to a symbol when the longest product header stem that prefixes the test file name is the header that declares the symbol. References in that test file do not keep the symbol. `override` methods were left in place. No unused class or struct turned up. No `.h` / `.cpp` pair was unused as a whole.

Nothing in this list is called from product C++ outside its own declaration, its own definition, or a test of its type. Two names are used only by other symbols in this list: `SolidForage::EPointerAct` (only `ResolvePileClick` and `ClickStartsWalk`) and `SolidStrongholdTransition::NoticeableSeconds` (only `ShowsProgress`). They go with those functions.

No product caller:

- `ASolidGameMode::TakeFromCaveChest`. Drags already go through `MoveCarriedItem`. Chest moves already go through `SolidInventory::TakeFromChest`.
- `ASolidHUD::IsInventoryDragging`. The HUD reads `bInventoryDrag` itself.

Key predicates. The character already binds `ControllerKey()` and `ControllerToggleKey()`:

- `SolidMainMenu::IsControllerKey`
- `SolidSight::IsControllerToggle`

Counters and wrappers only the matching tests call:

- `USolidCompany::GetCharacterMeshCount`
- `USolidCompany::AddCharacterMesh`. The company fills `AllCharacterMeshes` itself.
- `ASolidTerrainStreamer::EnsureStartTownRelocate`. The streamer and the game mode call `TryRelocateFocusToStartTown`.
- `USolidWorldMap::HasStartTown`
- `USolidWorldMap::GetStartTownCellCount`
- `USolidWorldMap::GetLocationCount`
- `SolidEvents::EventIdLength`. Event code uses `MaxEvents`, `MaxChangesPerEvent`, and `MaxRequiresPerEvent`. It does not read this length.
- `SolidStrongholdGrid::MeagreCellCount`. `MeagreCells` is the floor.
- `SolidLair::OpenCellCount`
- `SolidLair::ShortestPathSteps`
- `SolidLair::DeadEndCount`
- `SolidLair::EndSeparation`
- `SolidLair::PastEndLocal`
- `SolidDungeonLevel::PastMouthLocal`
- `SolidStrongholdTransition::ShowsProgress`
- `SolidStrongholdTransition::NoticeableSeconds`

Click helpers only the forage tests call. Pickup in game goes through `ASolidGameMode::TakeForagePile` and `SolidForage::TakePile`.

- `SolidForage::ResolvePileClick`
- `SolidForage::ClickStartsWalk`
- `SolidForage::EPointerAct`

Readers only the matching tests call. The actors still build the components. `GetDoorMesh` and `GetCaptainSpawnWorld` stay; product code calls them.

- `ASolidTowerInterior::GetFloorCount`
- `ASolidTowerInterior::GetFloor`
- `ASolidTowerInterior::GetWallCount`
- `ASolidTowerInterior::GetWall`
- `ASolidTowerInterior::GetStairs`
- `ASolidTowerInterior::GetRails`
- `ASolidTowerInterior::GetFloorSignCount`
- `ASolidTowerInterior::GetFloorSignPlate`
- `ASolidTowerInterior::GetFloorSignDigits`
- `ASolidTowerShell::GetCourseCount`
- `ASolidTowerShell::GetCourse`
- `ASolidBridge::GetRampFromMesh`
- `ASolidBridge::GetDeckMesh`
- `ASolidBridge::GetRampToMesh`
- `ASolidTownSign::GetPoleMesh`
- `ASolidTownSign::GetBoardMesh`
- `ASolidTownSign::GetLabelText`
- `ASolidTownSign::GetOutlineText`

Test commit, before the deletion:

- Assertions that checked a fact through one of these symbols move onto a call product code already makes, when one exists. Start-town checks use `GetStartTownWorldXY`. Maze checks use `IsOpenCell`. Menu and sight key checks use `ControllerKey()` and `ControllerToggleKey()`. The Meagre floor check uses `MeagreCells`.
- An assertion that can only be stated by calling the unused symbol is removed in that commit. The bridge deck, tower course, tower floor, and town-sign text readers have no product caller. Those inspections go away with the readers.
- The forage click tests that only call `ResolvePileClick`, `ClickStartsWalk`, and `EPointerAct` go away in that commit. Pile scatter and `TakePile` tests stay.
- That commit does not delete the product symbols. It passes against the current build.

Deletion commit:

- Delete the symbols listed above and any comment that exists only to describe them.
- Do not edit the tests. Do not delete includes, unused parameters, or code inside a function that still has a product caller.
- Bump `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.
