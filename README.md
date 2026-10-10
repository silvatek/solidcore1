# SolidCore1

UE **5.8** C++ open-world starter with World Partition–oriented config, a Viking captain, and Enhanced Input (move / look / jump / sprint).

## To-do

- Use white material for 100% fog terrain
- Don't render trees or buildings on 100% fog terrain, generate it when fog is removed
- Improve building and tree models
- Create a cave in the mountains near the starting town
- Build cave interior as a new level and transition to/from the main world at the entrance
- Replace companion models with new Fab assets
- Click to move
- HUD / Menu panel borders & themes

## Done

- **Captain + Party** — Viking pawn (Enhanced Input: move / look / zoom / jump / sprint), clip locomotion (idle / walk / run / jump), floating nameplates. The party starts empty. Entering Relion adds **Sam**; entering Kanfold adds **Alex** as well. Companions follow in battle-plan slots; the party camera frames the group and lifts off terrain.
- **Battle plans** — Company catalog and Party assigned slots. F1 Line, F2 Column, F3 Tight mob, F4 Loose mob, F5 Parade (companions in front, facing the captain, far enough to stay in his view). F1–F8 select; HUD battle-plan panel. The square-walk formation drill is the main menu's Test Drill entry and still cycles F1–F4.
- **World + biomes** — 64×64 `WorldMap.txt` overlay (Sea, Grassland, Town, Mountain, Forest, Desert, River) scaled onto a 513×513 TerrainPoint map (1024 m side, 200 cm spacing). That doubles the original 257-point / 512 m world, and each ASCII cell doubles from 8 m to 16 m. Numbered locations are towns: `0` Iglin (start), `1` Relion, `2` Kanfold, `3` Visolar, `4` Jethan. The captain starts at location 0. Per-biome height shelf (Sea/River 0, Mountain +10 m, else +1 m). Darker mountain grey.
- **No starter monolith** — the grey slab that marked the start-town centroid is gone. Location 0 still sets the captain spawn and the building clear radius.
- **Fog follows the player** — the map starts fully fogged. After the captain is placed at location 0, exploration fog is centered on the pawn, not on that cell. The two match only because that is where the pawn stands. Curtains are 110 m (half) and 160 m (full), above the 50 m max zoom. The camera boom shortens so it cannot sit above terrain with fog greater than 0.
- **Procedural terrain** — streamed chunks around the pawn, Fab grassland material, FlatCol tints for other biomes. Exploration fog-of-war (25 m / 50 m bands, marching-squares curtains) clears from the trail, not the camera.
- **Town + forest** — random non-overlapping town buildings (grey cuboid + red gable prism roof); forest trees scattered on Forest cells. Buildings stay clear of the start-town centroid and of each welcome sign.
- **Town signs** — every named WorldMap cell gets a "Welcome to {name}" sign: one thin dark-brown pole, a flat light-brown board, white letters with a black border. The board faces south. Kanfold's two cells each get a sign.
- **True sight / raven sight** — the game starts in true sight (first person). Raven sight (third person) is locked until the captain enters town 1. F9 then toggles. True sight hides the captain's body and nameplate, turns the body with the look direction, and ignores mouse-wheel zoom until you switch back. The menu and the formation drill ignore F9.
- **Events** — a list of one-shot triggers that change the company/party configuration. Enter town fires when the captain steps onto a numbered WorldMap cell; the parameter is that index. Entering town 1 (Relion) enables raven sight and sets max party size to 1 (Sam). Entering town 2 (Kanfold) sets max party size to 2 (Sam and Alex). Town 0 does not.
- **Main menu** — F10 opens it. **1 Test Drill** runs the formation drill. **2 Credits** opens the credits page (Silvatek, Cursor + Grok, Fab Viking and grass). Up/Down and Enter also work. Esc or F10 closes. Move, look, and battle-plan keys are ignored while the menu or credits page is open.
- **Content pipeline** — Fab listings restored via Launcher Add to Project; C++ finds Viking/grass by name under `/Game/Viking` or `/Game/Fab`. `tools/fab_doctor.bat` + `fab-assets.json`. Packs are gitignored (attribution README kept).
- **Automation** — `SolidCore1.*` editor tests; `tools/run_automation_tests.bat` prints failed test paths. Build ID `SC1-NNNN` on the debug HUD.
- **Editor build** — `tools\build.bat` compiles `SolidCore1Editor` Win64 Development (`-WaitMutex`). `UE_ROOT` overrides the engine install; `PROJECT` overrides the uproject. Both default from the script location.

## Controls (runtime defaults)

| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Zoom | Mouse wheel | — |
| Jump | Space | A / Cross |
| Sprint | Left Shift | Left stick click |
| True sight / raven sight | F9 (after raven sight is enabled) | — |
| Main menu | F10 | — |
| Menu up / down | Up / Down | — |
| Menu confirm | Enter, or 1 / 2 | — |
| Close menu / credits | Esc or F10 | — |

Input Actions / Mapping Context can be replaced with Content assets on the character later; if unset, C++ creates transient defaults so PIE works immediately.

## Automated tests

UE Automation tests live under `Source/SolidCore1/Tests/` (editor builds, `WITH_DEV_AUTOMATION_TESTS`).

### Policy

1. **All existing tests must keep passing** when you change product code — **without modifying those tests**.
2. If a change **must** update an existing test (API rename, intentional behavior change), say so explicitly in the commit / change description and why.
3. **New product code ships with tests in the same commit** (same PR/change set). Prefer pure logic / `NewObject` unit tests; add heavier PIE tests only when needed.
4. **Never pass `TObjectPtr<T>` to `TestNotNull` / `TestNull`.** UE 5.8 cannot deduce `const ValueType*` from it (`GetStaticMesh()` is the usual trap). Bind a raw `T*` first, or call `.Get()`.

### Suite (filter `SolidCore1`)

| Filter | Covers |
|--------|--------|
| `SolidCore1.Fog.*` | Distance bands, units, mist sampling, fog mesh build guards, camera stays on clear ground, curtain above max zoom |
| `SolidCore1.Map.*` | Build smoke, trail clear, idempotent build, sampling, bounds, biome Z shelf |
| `SolidCore1.WorldMap.*` | ASCII overlay load, key colors, numbered locations, doubled world scale, fog follows the player |
| `SolidCore1.Towns.*` | Town names keyed by the WorldMap index |
| `SolidCore1.Noise.*` | Hash / value / fBm / height / grass tone |
| `SolidCore1.Types.*` | Biome names (incl. Sea/River), height offsets, `FSolidTerrainPoint` defaults |
| `SolidCore1.Vegetation.*` | Tree RNG variation, town building pack, welcome signs |
| `SolidCore1.GameMode.*` | Default spawn flags, pawn BP resolution |
| `SolidCore1.Companion.*` | Defaults, SetFollowTarget |
| `SolidCore1.Streamer.*` | FindExisting / EnsureExists (null + idempotent) |
| `SolidCore1.Clip.*` | SelectClip idle/walk/run/jump rules |
| `SolidCore1.NameLabel.*` | Style sizes/colors/plates; Outcast / Sam defaults |
| `SolidCore1.BattlePlan.*` | Formation slots; Company/Party assign; drill legs |
| `SolidCore1.Credits.*` | Author / Cursor+Grok / Fab attribution; HUD toggle |
| `SolidCore1.MainMenu.*` | F10 menu entries (Test Drill, Credits) and selection |
| `SolidCore1.Sight.*` | True sight at start; F9 switches to raven sight and back |
| `SolidCore1.Events.*` | Enter-town events: raven sight, max party size, numbered cells |
| `SolidCore1.Build.*` | `SOLID_BUILD_ID` / note present |
| `SolidCore1.Content.*` | Required Content + Engine assets the code loads |

**Content dependency tests** assert meshes/materials/anims/BPs the C++ loaders expect (Engine BasicShapes, FlatCol for solid colors, Viking mesh+locomotion, Fab grass, pawn/GameMode BP-or-C++). Viking and Fab grass are required; Epic mannequin assets are not used.

## Fab assets (Epic Games Launcher)

Grass + Viking come from Fab. There is **no official download API**. Native UE `.uasset` packs cannot be batch-exported — each listing is **Add to Project** on its own. C++ finds them by **asset name** under `/Game/Viking` or `/Game/Fab`, so you do **not** move folders after Add to Project.

| Asset | Listing | Required | Resolved as |
|-------|---------|----------|-------------|
| Viking (Art.Hiraeth) | [fab.com/listings/ca4ba583-…](https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f) | Yes | `SK_Viking` + `Anim_Viking_*` |
| 025 Grass | [fab.com/listings/94bfee39-…](https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a) | Yes | `Mat_025_grass` |

**Restore (Launcher + UE 5.8):**

1. Epic Games Launcher → **Unreal Engine → Fab**. Sign in.
2. Open each listing. **Add to My Library** if needed (Ctrl+click can batch this).
3. **Add to Project** → this `SolidCore1.uproject`. Done.

```bat
tools\fab_doctor.bat
```

checks `SK_Viking` / clips / grass under `Content/Viking` or `Content/Fab`. Manifest: `tools/fab-assets.json`. Official notes: [Exporting Assets from Fab in Launcher](https://dev.epicgames.com/documentation/en-us/fab/exporting-assets-from-fab-in-launcher).

We still commit these folders today so a clone runs without the Launcher.

Not yet covered (need PIE): character movement, companion steering on terrain, streamer chunk load/unload, HUD drawing.

**Session Frontend:** rebuild editor → **Tools → Session Frontend → Automation** → filter `SolidCore1` → Start.

**Command line** (prints ran / passed / failed from the automation log):

```bat
tools\run_automation_tests.bat
tools\run_automation_tests.bat SolidCore1.Fog
```

The summary prints ran/passed/failed counts and, on failure, lists each failed test path (parsed from `Result={Fail}` lines in the automation log).

## Build ID (HUD)

PIE shows a top-left debug HUD (`Build SC1-NNNN`, a one-line `Change:` note, pawn/terrain Z, chunk load, material, camera pitch). Both strings live in `Source/SolidCore1/SolidBuildId.h` (`SOLID_BUILD_ID` / `SOLID_BUILD_NOTE`) and are bumped on every GitHub push so screenshots identify which binary you ran.

## Character visuals (clip locomotion)

Captain and Companions use single-node clip locomotion via `SolidClipLocomotion` (not Epic AnimBP). Each character keeps its own mesh soft pointer — Party members will use different meshes later; defaults are Fab Viking (`/Game/Viking/Mesh/SK_Viking` + idle/walk/run/jump). No Epic mannequin fallback.

### Reliable setup: Blueprint pawn (recommended)

1. Content Browser → right-click `Content/Characters` → **Blueprint Class**.
2. Pick **SolidCharacter** as the parent → name it `BP_SolidCharacter` (path `/Game/Characters/BP_SolidCharacter`).
3. Compile & Save (runtime forces the Viking mesh/clips even if the BP mesh slot is empty).
4. Close the editor, rebuild/reopen so GameMode picks up the Blueprint (it prefers this BP over the bare C++ class).
5. PIE — you should see the Viking player (and Viking companion).

**Naming:** Content Blueprints are `BP_SolidCharacter` / `BP_SolidGameMode` (parents `SolidCharacter` / `SolidGameMode`). Load paths are centralized in `SolidContentPaths.h`.

While PIE is running, **Output Log** filtered to `LogSolid` shows whether a mesh was applied.

## Open and build

### Option A — Editor

1. Double-click `SolidCore1.uproject` (associate with UE 5.8 if prompted).
2. Allow it to generate Visual Studio project files and compile the `SolidCore1` module.
3. When the editor opens, create the World Partition map (next section).

### Option B — Generate + command-line compile

From the repo root:

```bat
tools\build.bat
```

`UE_ROOT` defaults to `C:\Program Files\Epic Games\UE_5.8`. `PROJECT` defaults to `SolidCore1.uproject` next to `tools\`. Set either env var to point somewhere else.

Or right-click `SolidCore1.uproject` → **Generate Visual Studio project files**, open the `.sln`, then build **Development Editor | Win64**.

## Runtime procedural terrain chunks

C++ generates walkable terrain around the player at runtime:

- `FSolidTerrainPoint` / `ESolidBiome` — simulation cell (X, Y, Height, Biome, Threat, Fog)
- `USolidWorldMap` — 64×64 ASCII biome overlay (`Content/WorldMap.txt`) scaled across the TerrainMap world bounds
- `USolidTerrainMap` — 2D TerrainPoint grid built once at streamer startup (default 513×513 @ 200 cm); biomes from WorldMap when loaded. The ASCII map is stretched across that rectangle, so doubling the point count doubles each WorldMap cell.
- `ASolidTerrainChunk` — runtime `UStaticMesh`; vertex heights from the TerrainMap; per-quad biome materials
- `ASolidTerrainStreamer` — builds the map, relocates the pawn to location 0, then loads/unloads chunks
- Debug HUD shows biome / threat / fog at the pawn plus map size

Defaults: 64 m chunks (`ChunkWorldSize=6400`), 32 quads/side, radius 2 (5×5 chunks), `Amplitude=3000`. Grassland uses Fab `Mat_025_grass`; other biomes use FlatCol tinted by the WorldMap key colors.

Default map is `/Game/ThirdPerson/Lvl_ThirdPerson` (SC1-0022) so Open World Landscape/HLOD outer hills are not in the scene — that cleared the horizon slivers. `L_OpenWorld` remains for comparison. SC1-0023/0082 force `BP_SolidGameMode` on PIE/game worlds so template maps keep Solid HUD + Viking pawn instead of the template default. On first stream, the pawn is snapped onto the procedural height. If a Landscape is present, actors are hidden/collision-disabled once. Chunks block the Camera channel and use complex-as-simple collision on the runtime static mesh.

SC1-0007 drops `UProceduralMeshComponent` after persistent ribbon/culling failures with that path.

## Exploration fog (fog-of-war)

See [`Source/SolidCore1/Fog/README.md`](Source/SolidCore1/Fog/README.md) — trail-cleared bands, marching-squares curtains, and rejected approaches.

## WorldMap biomes (SC1-0105)

`Content/WorldMap.txt` (fallback `Source/SolidCore1/WorldMap.txt`) is a 64×64 grid of markers plus a color key (`S` Sea, `G` Grassland, `T` Town, `M` Mountain, `F` Forest, `D` Desert, `R` River). Digits are location indexes (`0` Iglin, `1` Relion, `2` Kanfold, `3` Visolar, `4` Jethan). Town names live in `Towns/SolidTowns.h`, matched by that index. A digit with no entry there has no name. Unlisted digits default to the Town biome. The grid scales across the TerrainMap world rectangle (file row 0 = north). The default rectangle is 1024 m on a side, so each marker is 16 m. Location **0** is the starting town: the streamer relocates the player there. A legacy `Z` marker is the start only on maps that have no `0` cells. Fog bands are centered on the player after that placement.

## Forest trees (SC1-0076 / SC1-0110)

- `ASolidTree` — cylinder trunk + cone canopy; scattered randomly across **Forest** TerrainPoints (`ForestTreeDensity`, capped by `MaxForestTrees`).
- Logic in `Vegetation/SolidForestTrees.*`. Toggle with `bAutoSpawnVegetation`.
- Fog curtains: see [`Fog/README.md`](Source/SolidCore1/Fog/README.md) (clear|fogged ~25 m plus a **white** half→full curtain ~50 m).

## Name labels (SC1-0091 / SC1-0093)

Floating nameplates (`SolidNameLabel`) sit above each Party member and face the view camera: TextRender plus thin cube **border** and contrasting **background** plates. Defaults: Captain **"Outcast"** (larger warm amber on dark plate), Companions muted slate plates. Override via `CharacterDisplayName` / `SetCharacterDisplayName`.

## Battle plans (SC1-0095 / SC1-0101)

- **Company** (`USolidCompany`) owns the catalog of all battle plans. Starter assigned set:
  - **F1 Line** (Standard) — companions on the flanks
  - **F2 Column** (Standard) — file behind the Captain
  - **F3 Tight mob** (Narrow) — close triangle behind
  - **F4 Loose mob** (Wide) — spread triangle behind
  - **F5 Parade** (Standard) — line in front of the Captain, facing him, far enough that the rank fits in his view
- **Party** (`USolidParty`) holds up to 8 **assigned** plans and one **active** plan (default **F1 Line**).
- Each plan has a **formation** plus **spacing** (`Narrow` / `Standard` / `Wide`) that scales follow distances.
- Captain switches assigned slots with **F1–F8** (only filled slots work).
- **Test Drill** (main menu) runs a formation drill: F1 walk 1.5s → F2 / F3 / F4 each blend +90° yaw over 250 ms then walk 1.5s (square path; player move/look suppressed while active).
- HUD shows a **Battle Plans** panel under the tech block: all 8 slots, F-key + name when assigned, active slot highlighted in amber (aligned marker column).
- Engine viewmode debug binds (wireframe/unlit/lit/…) are moved to **Ctrl+F1–F5** in `Config/DefaultInput.ini` so bare F-keys stay free for battle plans. Restart the editor after pulling.

## Companion (SC1-0024 / SC1-0093)

`ASolidCompanionCharacter` follows with simple steering (no NavMesh — works on procedural terrain). Same Fab Viking mesh + clip locomotion as the player. The roster is **Sam** (slot 0) then **Alex** (slot 1). `bAutoSpawnCompanion` spawns only as many as the current max party size, which starts at 0 and is raised by town events. Offsets come from the active Battle Plan. HUD lists each companion name and distance.

**Party camera** (SC1-0025/0026/0084): the Captain spring-arm shifts `TargetOffset` toward the Party center and may lengthen so Companions stay in frame (`bFrameCompanions`; implementation in `SolidPartyCamera.cpp`). SC1-0026 uses screen-space fit, disables boom collision while Companions are present, and zooms in much slower than out. SC1-0027 lifts the camera via `SocketOffset` when the predicted camera point would sink below procedural terrain.

## Create the open-world map

Binary `.umap` assets are created in the Editor (not checked in as source):

1. **File → New Level → Open World** (World Partition).
2. Save as `Content/Maps/L_OpenWorld`.
3. Place a **Player Start** near the origin (or on the landscape).
4. Confirm **Project Settings → Maps & Modes** (or edit `Config/DefaultEngine.ini`):
   - Editor Startup Map / Game Default Map → `/Game/ThirdPerson/Lvl_ThirdPerson` (horizon test; Open World map still available)
   - Default GameMode → `/Game/Characters/BP_SolidGameMode`
5. **Play** (PIE).

`Config/DefaultEngine.ini` currently uses `Lvl_ThirdPerson` and `BP_SolidGameMode`. After you create `Content/Maps/L_OpenWorld`, you can point the startup maps there; until then the Third Person map is the intentional default.

World Partition and Large Worlds are enabled in project config for open-world scale.

## Project layout

```
SolidCore1.uproject
Config/
Content/
  Characters/          # BP_SolidCharacter, BP_SolidGameMode
  Maps/                # optional L_OpenWorld (+ World Partition externals)
  Viking/              # Fab Viking mesh + locomotion clips (see Fab assets)
  WorldMap.txt         # ASCII 64×64 biome overlay + color key
tools/
  fab-assets.json      # Fab listing IDs + expected paths
  fab_doctor.bat       # check Viking/grass on disk; print Launcher restore
  build.bat            # SolidCore1Editor Win64 Development
  run_automation_tests.bat
Source/
  SolidCore1.Target.cs
  SolidCore1Editor.Target.cs
  SolidCore1/
    SolidCore1.Build.cs
    SolidCharacter.h / .cpp              # Captain core (move/look/zoom/sprint)
    SolidCharacterInput.cpp              # Runtime Enhanced Input factory
    SolidCharacterVisuals.cpp            # Captain mesh + clip selection
    SolidPartyCamera.cpp                 # Party framing + terrain boom lift
    SolidClipLocomotion.*  # Shared single-node clip apply/play (per-character mesh)
    SolidGameMode.*
    SolidCameraFog.h         # Boom scale that keeps the camera over Fog == 0
    SolidSight.h             # True sight / raven sight toggle
    SolidPlayerController.*
    SolidMaterials.*       # Shared FlatCol solid-color MID helper
    SolidContentPaths.h    # Canonical BP soft-class paths
    WorldMap.txt           # ASCII biome overlay (fallback copy)
    Companion/
      SolidCompanionCharacter.*          # Companion NPC follower
    Fog/
      README.md               # FoW history, rejected approaches, invariants
      SolidTerrainFog.*       # FoW bands, boundary mesh, materials, height-fog helpers
    Terrain/
      SolidTerrainTypes.h
      SolidWorldMap.*         # Parse/scale WorldMap.txt; location 0 centroid
      SolidTerrainMap.*
      SolidTerrainNoise.h
      SolidTerrainChunk.*
      SolidTerrainStreamer.*
      SolidTerrainMaterials.*   # Grass + per-biome FlatCol resolve
      SolidTerrainWorldSubsystem.*
    Vegetation/
      SolidTree.*             # Placeholder cylinder+cone tree
      SolidForestTrees.*      # Random Forest-biome tree placement
    Towns/
      SolidBuilding.*         # Grey cuboid + red gable roof
      SolidTownBuildings.*    # Non-overlapping town building pack
      SolidTownSign.*         # Pole, board, "Welcome to {name}"
      SolidTownSigns.*        # One sign per named WorldMap cell
      SolidTowns.h            # Town names, keyed by the WorldMap index
    HUD/
      SolidHUD.*              # Debug HUD, battle-plan panel, menu and credits drawing
    Menus/
      SolidMainMenu.*         # F10 menu entries (Test Drill, Credits)
      SolidCredits.*          # Credits page copy
    Tests/
      SolidTerrainTestHelpers.h   # Shared MakeSmallMap fixture
      SolidWorldMapTests.cpp
      SolidTerrainFogTests.cpp
      SolidTerrainFogMeshTests.cpp
      SolidTerrainMapTests.cpp
      SolidTerrainMapMoreTests.cpp
      SolidTerrainNoiseTests.cpp
      SolidTerrainTypesTests.cpp
      SolidVegetationTests.cpp
      SolidTownSignTests.cpp
      SolidTownsTests.cpp
      SolidGameModeTests.cpp
      SolidCompanionTests.cpp
      SolidTerrainStreamerTests.cpp
      SolidClipLocomotionTests.cpp
      SolidBuildIdTests.cpp
      SolidMainMenuTests.cpp
      SolidSightTests.cpp
      SolidContentDependencyTests.cpp
tools/
  run_automation_tests.bat
```

## Requirements

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`
- Visual Studio 2022 with **Game development with C++** (MSVC, Windows SDK)

## Notes

- This repo is source + config only. `Binaries/`, `Intermediate/`, `Saved/`, and `.sln` are gitignored and generated locally.
- Optional: create Blueprint subclasses of `SolidCharacter` / `SolidGameMode` for content-driven tuning without changing C++. Keep names `BP_Solid*`.
