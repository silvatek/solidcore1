# Remove unused code

Status: Ready

## Story

A developer removes C++ symbols that product code never calls. A symbol that only tests of that functionality call is unused, under the definition in `AGENTS.md`. A symbol stays when a test of other functionality uses it. A mutually supporting pair can still go: if A is only used by tests of B, and B is only used by tests of A, both can be removed. Symbols the Unreal Engine calls stay. Symbols the World chart mode calls stay.

The game already performs the work these symbols wrap or inspect. Removing them leaves that work in place.

## Acceptance

- Each symbol below is gone from `Source/`, including tests. A whole-word search for the name finds nothing under `Source/`. `Holds` stays in comments that are not this function.
- `override` methods stay. `UFUNCTION` methods the engine can call stay. Types that `Config/` names for the engine to instantiate stay. `GetTypeHash` for a hex edge stays. `ASolidCharacter::StartSquadFormationDrill` stays.
- These product calls stay, and still behave as they do now: `FSolidBattle::Advance`, `FSolidBattle::AdvanceToNextEvent`, `FSolidBattle::SquadWeapon`, `FSolidBattle::MonsterWeapon`, `FSolidBattlePlan::Attacks`, `FSolidBattlePlan::Retreats`, `ASolidHUD::ToggleDebugPanel`, `ASolidDungeonExterior::GroundDropCm`, `ASolidDungeonExterior::GetRockMesh`, `ASolidDungeonExterior::SetGroundDrop`, `ASolidTowerExterior::GetDoorMesh`, `FSolidTower::FloorCount`, `SolidDungeonLevel::EnterLabel`, `SolidDungeonLevel::LeaveLabel`, `USolidWorldChart::PaintRun`, `USolidWorldChart::PaintRectangle`, `USolidWorldChart::MarkAtCell`, `USolidWorldChart::LoadFromString`, `USolidWorldChart::LoadFromChart`, `FSolidTower::OutsideDoorXY`, and `SolidStronghold::BiomeFor`. `FSolidBattle::TimeUntilNextEvent` stays. It is private, and `AdvanceToNextEvent` is its caller.
- Shelves, visit views, finger bones on a hand, the chart cells, and the fight log stay. Only the unused readers, counters, and wrappers go.
- Existing tests keep passing. The test edit is its own commit, and it passes on the code before the deletion. The deletion commit does not edit those tests. The deletion commit says that the tests changed because these symbols went away.
- The deletion commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. The commit that adds `FSolidTower::FloorCount` bumps them too.

## Implementation notes

The list follows the unused-code definition in `AGENTS.md`. A use is a whole-word name in product C++ outside `Tests/`, in `Config/` for a type the engine loads, or in a `.Build.cs` / `.Target.cs` file. A name in `Source/SolidCore1Editor/` is a use as well. That module is the World chart mode, and it calls the chart. A symbol can go when every reference is a test of that symbol. A symbol stays when a test of other functionality uses it. Check for a mutually supporting pair. If A is only used by tests of B, and B is only used by tests of A, both can be removed. `override` methods were left in place.

`SolidStrongholdGrid::GetTypeHash` stays. `CollectSegments` puts `FEdgeKey` in a `TSet`, and that set calls `GetTypeHash` through argument-dependent lookup. No other C++ writes the name.

`ASolidCharacter::StartSquadFormationDrill` has no caller. Test Drill is off the menu. [Drill battle plan](drill-battle-plan.md) is Ready and says this function starts the square. It stays.

Nothing in the list below is called from product C++ outside its own declaration, its own definition, or a test of that same functionality. Two names are used only by other symbols in this list: `FSolidWorldChartRead` (only `ReadCell`) and `SolidForage::FPileHit` (only `NearestHitIndex`). They go with those functions.

`FSolidBattle::AdvanceToNextEvent` stays. `SolidCore1.Battle.Simulation`, the battle panel, and the fielded check call it to reach the log, a blow, armour, a kill, a retreat, and which companions are in the fight. Those tests already name the moment they expect: the opening prepare at time 0, then each weapon `Speed`. A kill loop steps until a log line or until defence reaches 0, and the captain's `Speed` and a monster's `Speed` can differ, so the test cannot pass one weapon `Speed` to `Advance` and hit the same moment. `TimeUntilNextEvent` is the private wait that `AdvanceToNextEvent` uses. The game mode calls `Advance`.

`ASolidDungeonExterior::GetRockMesh` stays. `SolidCore1.Stronghold.DungeonExterior` and `SolidCore1.Stronghold.DungeonExteriorMouth` are tests of the rock and the mouth. They read the stone through `GetRockMesh`: the mouth is a different mesh, the buried ground edge follows the drop, and the mouth front meets the rock. `RockMesh` is protected. `SetGroundDrop` stays with it. The burial measurement sets that drop, then reads the rock mesh. `GroundDropCm` stays.

`FSolidTower` owns the plan, in `StoredPlan`. `FloorCount()` returns that floor count, beside `HeightCm()`. The Imperious exterior check calls it on the tower from `Make`, instead of `GetPlan().FloorCount`. `GetPlan` goes. `Plan()` stays. This story does not remove the plan.

The World chart mode keeps `PaintRun`, `PaintRectangle`, `PlaceTown`, `PlaceStronghold`, `PlaceSite`, `PlaceLairMouth`, `PlaceCustom`, `DeleteMark`, `AddRoad`, `RouteForRoad`, `Undo`, `MarkAtCell`, `BuildChartView`, `RebuildChartView`, and `ClearChartView`. `SolidStronghold::TypeCount` stays. The mode clamps a type index with it.

No product caller:

- `ASolidHUD::CloseExitConfirm`. Closing the exit page already assigns `bShowExitConfirm`.
- `ASolidHUD::IsDebugPanelVisible`. The HUD reads `bShowDebugPanel`. The debug test is the only caller, and `bShowDebugPanel` is protected. The test commit drops those reads. F12 still calls `ToggleDebugPanel`.
- `ASolidHUD::SetDebugPanelVisible`. F12 calls `ToggleDebugPanel`. The debug test is the only caller of the setter.
- `ASolidStrongholdSite::GetSize`. `SetSite` stores the size. Nothing reads it back.
- `ASolidTerrainStreamer::IsChartView`. Tick reads `bChartView`.
- `ASolidTowerExterior::GetPlan`. The exterior builds from the tower it was given. `GetDoorMesh` stays. The floor count is `FSolidTower::FloorCount`.
- `USolidWorldChart::PaintCell`. It calls `PaintRun` with one cell. The mode paints with `PaintRun` and `PaintRectangle`.
- `USolidWorldChart::ReadCell`. The mode describes a cell from `Cells`, `MarkAtCell`, and the solved chart.
- `SolidLair::MouthMidpoint`. Tunnel placement uses `CellCenter` and `SpawnLocal`.
- `SolidInventory::CountAt`. Slot checks use `ItemAt`.
- `SolidForage::NearestHitIndex`. A click takes a pile through `TakePile`.
- `SolidDungeonLevel::MenuLabel`. The cave tip uses `EnterLabel` and `LeaveLabel`.
- `FSolidTower::OutsideDoorYawDeg`. It returns the grass yaw unchanged. `OutsideDoorXY` stays.
- `FSolidMonster::MeshFootOffsetCm`. A raptor places its mesh from the standing span. `UniformScale` and `StandingSpan` stay.
- `FSolidBattlePlan::Holds`. The plan tests assert `Holds` itself. Parade stores `AttackOrRetreat`. The fight reads `Attacks` and `Retreats`. This is the aggression value `ESolidAggression::Hold`. It is not the Hold formation in [Hold battle plan](hold-battle-plan.md).
- `FSolidBattle::GetSquadPower`. The encounter, weapons, and fielded tests assert this total. Those tests already read `SquadWeapon`. A blow reads `SquadWeapon`.
- `FSolidBattle::GetGroupPower`. The encounter and weapons tests assert this total. Those tests already read `MonsterWeapon`. A blow reads `MonsterWeapon`.

Counters only the matching tests call, or that nothing calls:

- `SolidStronghold::ShelfCount`. `BiomeFor` walks `Shelves`.
- `SolidStronghold::VisitViewCount`. `VisitViewFor` walks `VisitViews`.
- `SolidBodyMesh::LeftFingerBones`.
- `SolidBodyMesh::RightFingerBones`. A hand lists the finger bones it was given. It does not read these arrays.

Types only the unused readers use:

- `FSolidWorldChartRead`.
- `SolidForage::FPileHit`.

Test commit, before the deletion:

- `SolidCore1.HUD.DebugToggle` keeps calling `ToggleDebugPanel`. The visibility checks that call `IsDebugPanelVisible` and `SetDebugPanelVisible` go away. The HUD still reads `bShowDebugPanel`.
- `SolidCore1.TowerExterior.*` keeps `GetDoorMesh`. The Imperious floor-count check calls `FSolidTower::FloorCount()` on the tower from `Make`. That retarget is the commit that adds `FloorCount()`. That commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`. `GetPlan` is still in the code then. The deletion commit removes `GetPlan` and does not edit this test.
- `SolidCore1.Stronghold.DungeonExterior` and `SolidCore1.Stronghold.DungeonExteriorMouth` keep calling `GetRockMesh` and `SetGroundDrop`. `GroundDropCm` stays.
- `SolidCore1.DungeonLevel.*` checks the cave words through `EnterLabel` and `LeaveLabel`. `MenuLabel` goes away.
- `SolidCore1.WorldChart.Paint` calls `PaintRun` with the one cell where it now calls `PaintCell`. A painted biome is read from `Cells`. A mark is `Marks` at `MarkAtCell`. A resolved biome, and which roads cross the cell, are read from the chart after `LoadFromChart`, through `GetBiomeAtCell` and `GetRoads`. The town resolving as town and the lair mouth resolving as mountain stay. `ReadCell` and `FSolidWorldChartRead` go away.
- `SolidCore1.Body.HandFingers` keeps the finger names in the test. It stops reading `LeftFingerBones` and `RightFingerBones`.
- `SolidCore1.Monster.*` drops the `MeshFootOffsetCm` check. `UniformScale` and `StandingSpan` checks stay.
- `SolidCore1.BattlePlan.*` checks Parade and a hold aggression through `AttackOrRetreat`, `Attacks`, and `Retreats`. The `Holds` checks go away.
- `SolidCore1.Battle.*` power checks move onto `SquadWeapon` and `MonsterWeapon`. The battle tests keep calling `AdvanceToNextEvent`.
- That commit does not delete the product symbols. It passes against the current build.

Deletion commit:

- Delete the symbols listed above and any comment that exists only to describe them.
- Do not edit the tests. Do not delete includes, unused parameters, or code inside a function that still has a product caller.
- Bump `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.
