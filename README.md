# SolidCore1

UE **5.8** C++ open-world starter with World Partition–oriented config, a third-person character, and Enhanced Input (move / look / jump / sprint).

## Paths (this machine)

| Item | Path |
|------|------|
| Project folder | `C:\Users\staz6\Dev\solidcore1` |
| Project file | `C:\Users\staz6\Dev\solidcore1\SolidCore1.uproject` |
| Engine | `C:\Program Files\Epic Games\UE_5.8` |

Put this repo’s contents at `C:\Users\staz6\Dev\solidcore1` (clone or sync), then open/build there.

## Controls (runtime defaults)

| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Zoom | Mouse wheel | — |
| Jump | Space | A / Cross |
| Sprint | Left Shift | Left stick click |

Input Actions / Mapping Context can be replaced with Content assets on the character later; if unset, C++ creates transient defaults so PIE works immediately.

## Automated tests

UE Automation tests live under `Source/SolidCore1/Tests/` (editor builds, `WITH_DEV_AUTOMATION_TESTS`).

### Policy

1. **All existing tests must keep passing** when you change product code — **without modifying those tests**.
2. If a change **must** update an existing test (API rename, intentional behavior change), say so explicitly in the commit / change description and why.
3. **New product code ships with tests in the same commit** (same PR/change set). Prefer pure logic / `NewObject` unit tests; add heavier PIE tests only when needed.

### Suite (filter `SolidCore1`)

| Filter | Covers |
|--------|--------|
| `SolidCore1.Fog.*` | Distance bands, units, mist sampling, fog mesh build guards |
| `SolidCore1.Map.*` | Build smoke, trail clear, idempotent build, sampling, bounds |
| `SolidCore1.WorldMap.*` | ASCII overlay load, key colors, Z town, terrain fog origin |
| `SolidCore1.Noise.*` | Hash / value / fBm / height / grass tone |
| `SolidCore1.Types.*` | Biome names (incl. Sea/River), `FSolidTerrainPoint` defaults |
| `SolidCore1.Vegetation.*` | Tree RNG variation, monolith defaults, town building pack |
| `SolidCore1.GameMode.*` | Default spawn flags, pawn BP resolution |
| `SolidCore1.Companion.*` | Defaults, SetFollowTarget |
| `SolidCore1.Streamer.*` | FindExisting / EnsureExists (null + idempotent) |
| `SolidCore1.Clip.*` | SelectClip idle/walk/run/jump rules |
| `SolidCore1.NameLabel.*` | Style sizes/colors/plates; Outcast / Sam defaults |
| `SolidCore1.BattlePlan.*` | Formation slots; Company/Party assign; F9 drill legs |
| `SolidCore1.Build.*` | `SOLID_BUILD_ID` / note present |
| `SolidCore1.Content.*` | Required/optional Content + Engine assets the code loads |

**Content dependency tests** assert meshes/materials/anims/BPs the C++ loaders expect (Engine BasicShapes, FlatCol, Viking mesh+locomotion, Fab grass or FlatCol fallback, pawn/GameMode BP-or-C++). Viking is required; Epic mannequin assets are not used.

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

From an elevated or normal **Developer** command prompt:

```bat
cd /d C:\Users\staz6\Dev\solidcore1

"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SolidCore1Editor Win64 Development -Project="C:\Users\staz6\Dev\solidcore1\SolidCore1.uproject" -WaitMutex
```

Or right-click `SolidCore1.uproject` → **Generate Visual Studio project files**, open the `.sln`, then build **Development Editor | Win64**.

## Runtime procedural terrain chunks

C++ generates walkable terrain around the player at runtime:

- `FSolidTerrainPoint` / `ESolidBiome` — simulation cell (X, Y, Height, Biome, Threat, Fog)
- `USolidWorldMap` — 64×64 ASCII biome overlay (`Content/WorldMap.txt`) scaled across the TerrainMap world bounds
- `USolidTerrainMap` — 2D TerrainPoint grid built once at streamer startup (default 257×257 @ chunk vert spacing); biomes from WorldMap when loaded
- `ASolidTerrainChunk` — runtime `UStaticMesh`; vertex heights from the TerrainMap; per-quad biome materials
- `ASolidTerrainStreamer` — builds the map, relocates the pawn to the starting town (Z), then loads/unloads chunks
- Debug HUD shows biome / threat / fog at the pawn plus map size

Defaults: 64 m chunks (`ChunkWorldSize=6400`), 32 quads/side, radius 2 (5×5 chunks), `Amplitude=3000`. Grassland uses Fab `Mat_025_grass` when present; other biomes use FlatCol tinted by the WorldMap key colors.

Default map is `/Game/ThirdPerson/Lvl_ThirdPerson` (SC1-0022) so Open World Landscape/HLOD outer hills are not in the scene — that cleared the horizon slivers. `L_OpenWorld` remains for comparison. SC1-0023/0082 force `BP_SolidGameMode` on PIE/game worlds so template maps keep Solid HUD + Viking pawn instead of the template default. On first stream, the pawn is snapped onto the procedural height. If a Landscape is present, actors are hidden/collision-disabled once. Chunks block the Camera channel and use complex-as-simple collision on the runtime static mesh.

SC1-0007 drops `UProceduralMeshComponent` after persistent ribbon/culling failures with that path.

## Exploration fog (fog-of-war)

Goal: the world starts shrouded; fog clears only where the **pawn has been** (trail), independent of camera orbit / pitch / zoom.

**Data (kept):** `FSolidTerrainPoint.Fog` on `USolidTerrainMap`. Initial fill and trail clear use the same bands — ≤25 m → `0`, 25–50 m → `0.5`, >50 m → `1`. Initial distance is measured from the WorldMap starting-town (Z) centroid when present (else world origin). Runtime: `ApplyExplorationFogAround` as the pawn moves.

**Current visual (SC1-0076):** marching-squares curtains at **clear|fogged (~25 m)** and **half|full (~50 m, white)**. Programmatic `BLEND_Translucent` unlit mist. Logic in `Terrain/SolidTerrainFog.*`. Height fog **off**.

### Approaches tried and rejected

| Approach | Builds (approx.) | Why rejected |
|----------|------------------|--------------|
| **Exponential Height Fog driven by local pawn `Fog`** | ~SC1-0056…0061, **0068** | Global atmosphere. When the pawn stands on clear trail (`Fog=0`), density goes to 0 and **the whole world looks clear** — no distant fog-of-war. |
| **Height fog + camera / look-direction probes** | ~SC1-0065…0067 | `GetPlayerViewPoint` / forward probes made mist pop in/out when orbiting or looking at clear ground. UE’s fog **StartDistance is camera-relative**, so a “25 m clear bubble” moves with the boom, not the pawn. |
| **Height fog + omnidirectional ring around pawn** (no look probe) | SC1-0067 | Amount could stay stable, but StartDistance / height falloff still made the **look** of mist change with camera pose. Still not spatial FoW. |
| **Opaque solid fog volumes / roofs** (FlatCol boxes) | ~SC1-0057…0060 | Read as **solid grey/white slabs or snow** on the ground, not mist. |
| **Opaque wall-only / prism banks** (still FlatCol) | ~SC1-0063…0065 | Less “snow roof,” but still **solid walls**; spring-arm sometimes collided until fog meshes forced `NoCollision` / ignore `ECC_Camera`. |
| **Dense fog lattice** (fins/prisms in every fogged cell) | SC1-0069 | Many overlapping layers → looks fully opaque even with translucent mats; heavy overdraw / FPS. |
| **Axis-aligned clear\|fogged grid edges only** | SC1-0071…0072 | Translucent panels, but **stair-step corner gaps** let you see through the curtain. Superseded by marching squares (SC1-0073). |
| **JumpPad glow MIDs as “translucent” fog** | SC1-0069…0071 | Load successfully but ignore Opacity / read as **solid white emissive panels**. MID cannot change BlendMode. |
| **Half-fog via fake translucency on opaque mats** | mid series | Opacity parameters ignored on opaque parents → still fully opaque. |
| **Half-fog geometric dither** (checkerboard pillars, ~50% cells empty) | SC1-0063+ | Mitigated opacity for lattice cells; superseded by boundary curtains. |
| **Height fog “thicker mist” tuning only** (density / max opacity / extinction) | SC1-0058…0065 | Could look misty in places, but never fixed **spatial trail clearing** vs **camera dependence**. |

**Do not reintroduce** camera/view sampling for fog amount, height-fog StartDistance as a 25 m clear radius, or dense per-cell fog lattices. Prefer boundary geometry in `SolidTerrainFog` tied to `TerrainPoint.Fog`.

## WorldMap biomes (SC1-0105)

`Content/WorldMap.txt` (fallback `Source/SolidCore1/WorldMap.txt`) is a 64×64 grid of markers plus a color key (`S` Sea, `G` Grassland, `T`/`Z` Town, `M` Mountain, `F` Forest, `D` Desert, `R` River). The grid scales across the TerrainMap world rectangle (file row 0 = north). `Z` cells mark the **starting town**: fog clears from that centroid, the streamer relocates the player there, and the monolith sits in the middle.

## Starter landmark + forest trees (SC1-0076 / SC1-0110)

- `ASolidMonolith` — large grey slab at the WorldMap starting-town (Z) centroid (falls back to `StarterMonolithOffsetXY`).
- `ASolidTree` — cylinder trunk + cone canopy; scattered randomly across **Forest** TerrainPoints (`ForestTreeDensity`, capped by `MaxForestTrees`).
- Logic in `Vegetation/SolidForestTrees.*`. Toggle with `bAutoSpawnVegetation`.
- Fog curtains: clear|fogged (~25 m) plus a **white** half→full curtain (~50 m).

## Name labels (SC1-0091 / SC1-0093)

Floating nameplates (`SolidNameLabel`) sit above each Party member and face the view camera: TextRender plus thin cube **border** and contrasting **background** plates. Defaults: Captain **"Outcast"** (larger warm amber on dark plate), Companions muted slate plates. Override via `CharacterDisplayName` / `SetCharacterDisplayName`.

## Battle plans (SC1-0095 / SC1-0101)

- **Company** (`USolidCompany`) owns the catalog of all battle plans. Starter assigned set:
  - **F1 Line** (Standard) — companions on the flanks
  - **F2 Column** (Standard) — file behind the Captain
  - **F3 Tight mob** (Narrow) — close triangle behind
  - **F4 Loose mob** (Wide) — spread triangle behind
- **Party** (`USolidParty`) holds up to 8 **assigned** plans and one **active** plan (default **F1 Line**).
- Each plan has a **formation** plus **spacing** (`Narrow` / `Standard` / `Wide`) that scales follow distances.
- Captain switches assigned slots with **F1–F8** (only filled slots work).
- **F9** runs a formation drill: F1 walk 1.5s → F2 / F3 / F4 each blend +90° yaw over 250 ms then walk 1.5s (square path; player move/look suppressed while active).
- HUD shows a **Battle Plans** panel under the tech block: all 8 slots, F-key + name when assigned, active slot highlighted in amber (aligned marker column).
- Engine viewmode debug binds (wireframe/unlit/lit/…) are moved to **Ctrl+F1–F5** in `Config/DefaultInput.ini` so bare F-keys stay free for battle plans. Restart the editor after pulling.

## Companion (SC1-0024 / SC1-0093)

`ASolidCompanionCharacter` spawns behind the player and follows with simple steering (no NavMesh — works on procedural terrain). Same Fab Viking mesh + clip locomotion as the player. GameMode flag: `bAutoSpawnCompanion` spawns **Sam** (slot 0) and **Alex** (slot 1); offsets come from the active Battle Plan. HUD lists each companion name and distance.

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
  Viking/              # Fab Viking mesh + locomotion clips
  WorldMap.txt         # ASCII 64×64 biome overlay + color key
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
    SolidPlayerController.*
    SolidMaterials.*       # Shared FlatCol solid-color MID helper
    SolidContentPaths.h    # Canonical BP soft-class paths
    WorldMap.txt           # ASCII biome overlay (fallback copy)
    Companion/
      SolidCompanionCharacter.*          # Companion NPC follower
    Terrain/
      SolidTerrainTypes.h
      SolidTerrainFog.*       # FoW bands, boundary mesh, materials, height-fog helpers
      SolidWorldMap.*         # Parse/scale WorldMap.txt; Z town centroid
      SolidTerrainMap.*
      SolidTerrainNoise.h
      SolidTerrainChunk.*
      SolidTerrainStreamer.*
      SolidTerrainMaterials.*   # Grass + per-biome FlatCol resolve
      SolidTerrainWorldSubsystem.*
    Vegetation/
      SolidMonolith.*         # Grey slab landmark at start town
      SolidTree.*             # Placeholder cylinder+cone tree
      SolidForestTrees.*      # Random Forest-biome tree placement
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
      SolidGameModeTests.cpp
      SolidCompanionTests.cpp
      SolidTerrainStreamerTests.cpp
      SolidClipLocomotionTests.cpp
      SolidBuildIdTests.cpp
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
