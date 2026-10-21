# SolidCore1

UE **5.8** C++ open-world starter with World Partition–oriented config, a Viking captain, and Enhanced Input (move / look / jump / sprint).

The design north star is [`GameVision.md`](GameVision.md). The world is [`Content/WorldLore.md`](Content/WorldLore.md). Coding guidelines are [`CodingGuidelines.md`](CodingGuidelines.md).

## Contents

- [Planning](#planning)
- [Controls (runtime defaults)](#controls-runtime-defaults)
- [Automated tests](#automated-tests)
- [Fab assets (Epic Games Launcher)](#fab-assets-epic-games-launcher)
- [Build ID (HUD)](#build-id-hud)
- [Character visuals (clip locomotion)](#character-visuals-clip-locomotion)
  - [Reliable setup: Blueprint pawn (recommended)](#reliable-setup-blueprint-pawn-recommended)
- [Open and build](#open-and-build)
  - [Option A — Editor](#option-a--editor)
  - [Option B — Generate + command-line compile](#option-b--generate--command-line-compile)
- [Runtime procedural terrain chunks](#runtime-procedural-terrain-chunks)
- [Exploration fog (fog-of-war)](#exploration-fog-fog-of-war)
- [WorldMap biomes (SC1-0105)](#worldmap-biomes-sc1-0105)
- [Forest trees (SC1-0076 / SC1-0110)](#forest-trees-sc1-0076--sc1-0110)
- [Name labels (SC1-0091 / SC1-0093)](#name-labels-sc1-0091--sc1-0093)
- [Battle plans (SC1-0095 / SC1-0101)](#battle-plans-sc1-0095--sc1-0101)
- [Companion (SC1-0024 / SC1-0093)](#companion-sc1-0024--sc1-0093)
- [Create the open-world map](#create-the-open-world-map)
- [Project layout](#project-layout)
- [Requirements](#requirements)
- [Notes](#notes)

## Planning

Open stories live in [`Planning`](Planning). Each file is one idea. A story moves to [`Planning/Done`](Planning/Done) after it has been properly tested and you have agreed with the agent that it is done. You say whether you are happy with the implementation. The agent confirms that every acceptance criterion is met.

Stories in [`Planning/Done`](Planning/Done) have been tested, and you and the agent have agreed they are done.

## Controls (runtime defaults)

Keyboard and gamepad defaults are in [`Content/Controls.md`](Content/Controls.md), one table per mode.

## Automated tests

UE Automation tests live under `Source/SolidCore1/Tests/` (editor builds, `WITH_DEV_AUTOMATION_TESTS`). The rules for writing and changing them are in [`CodingGuidelines.md`](CodingGuidelines.md). What the `SolidCore1` suite covers is in [`Source/SolidCore1/Tests/README.md`](Source/SolidCore1/Tests/README.md).

## Fab assets (Epic Games Launcher)

Grass, Viking, the Assassin, and the Blacksmith come from Fab. There is **no official download API**. Native UE `.uasset` packs cannot be batch-exported — each listing is **Add to Project** on its own. `resolvedPath`, `resolvedFolder`, and `resolvedPaths` in `tools/fab-assets.json` are tried first. If that file or folder is missing, Viking and grass are found by **asset name** under `/Game/Viking` or `/Game/Fab`, and the Assassin and Blacksmith packs are the folders `Medieval_Assassin_Girl` and `Elf_Blacksmith` anywhere under Content. A find is written back into the manifest. Do **not** move folders after Add to Project.

| Asset | Listing | Required | Resolved as |
|-------|---------|----------|-------------|
| Viking (Art.Hiraeth) | [fab.com/listings/ca4ba583-…](https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f) | Yes | `SK_Viking` + `Anim_Viking_*` (captain) |
| 025 Grass | [fab.com/listings/94bfee39-…](https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a) | Yes | `Mat_025_grass` |
| Free Medieval Assassin Girl (Dimentrius3dArtist) | [fab.com/listings/2986b290-…](https://www.fab.com/listings/2986b290-90ed-4a0e-a891-cacf442033f9) | Yes | `Meshes/base_mesh/SKM_base_mesh_ue` in `Medieval_Assassin_Girl` |
| Elf Blacksmith (Art.Hiraeth) | [fab.com/listings/d01abf8c-…](https://www.fab.com/listings/d01abf8c-9798-4bcb-8617-2d1d560d318a) | Yes | `Mesh/SK_Elf_Blacksmith` in `Elf_Blacksmith` |

**Restore (Launcher + UE 5.8):**

1. Epic Games Launcher → **Unreal Engine → Fab**. Sign in.
2. Open each listing. **Add to My Library** if needed (Ctrl+click can batch this).
3. **Add to Project** → this `SolidCore1.uproject`. Done.

```bat
tools\fab_doctor.bat
```

tries each recorded path in `tools/fab-assets.json` first, then searches. Viking and grass fall back to `Content/Viking` or `Content/Fab`. The Assassin body is `Meshes/base_mesh/SKM_base_mesh_ue.uasset` (`SK_Mannequin.uasset` beside it is the skeleton). The Blacksmith body is `Mesh/SK_Elf_Blacksmith.uasset` (`SK_Elf_Blacksmith_Skeleton.uasset` is the skeleton). A different recorded file is replaced when that body is found. Official notes: [Exporting Assets from Fab in Launcher](https://dev.epicgames.com/documentation/en-us/fab/exporting-assets-from-fab-in-launcher).

```bat
tools\fab_mesh_info.bat Medieval_Assassin_Girl
tools\fab_mesh_info.bat Elf_Blacksmith
```

lists that pack's folders, mesh files (size and SKM / SK / PA), and animation names. It does not change `fab-assets.json`.

The packs are not in git. `.gitignore` keeps `Content/Fab`, `Content/Viking`, `Content/Medieval_Assassin_Girl`, and `Content/Elf_Blacksmith` out of the repository. A clone does not contain them. Add to Project, then run `tools\fab_doctor.bat`. Do not commit a pack that lands under a different folder name. Viking and the Blacksmith are Fab Standard, so their source files stay off the public repo. Grass and the Assassin are CC-BY.

Not yet covered (need PIE): character movement, companion steering on terrain, streamer chunk load/unload, HUD drawing.

**Session Frontend:** rebuild editor → **Tools → Session Frontend → Automation** → filter `SolidCore1` → Start.

**Command line** (prints ran / passed / failed from the automation log):

```bat
tools\run_automation_tests.bat
tools\run_automation_tests.bat SolidCore1.Fog
```

The summary prints ran/passed/failed counts and, on failure, lists each failed test path (parsed from `Result={Fail}` lines in the automation log).

## Build ID (HUD)

PIE shows a top-left debug HUD (`Build SC1-NNNN`, a one-line `Change:` note, pawn/terrain Z, chunk load, material, camera pitch). Both strings live in `Source/SolidCore1/Game/SolidBuildId.h` (`SOLID_BUILD_ID` / `SOLID_BUILD_NOTE`) and are bumped when the product changes, so screenshots identify which binary you ran. A documentation change or a test change does not bump them.

## Character visuals (clip locomotion)

Captain and Companions use single-node clip locomotion via `SolidClipLocomotion` (not Epic AnimBP). The captain stays on Fab Viking (`/Game/Viking/Mesh/SK_Viking` + idle/walk/run/jump). Companions wear a generated low-poly body on that skeleton, with a matte shirt, matte trousers, light pink head and hands, and a drawn face, and play the same Viking clips. No Epic mannequin fallback.

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
- `USolidWorldMap` — 128×64 ASCII biome overlay (`Content/WorldMap.txt`) scaled across the TerrainMap world bounds. Cells are 8 m east–west and 16 m north–south.
- `USolidTerrainMap` — 2D TerrainPoint grid built once at streamer startup (default 513×513 @ 200 cm); biomes from WorldMap when loaded. The ASCII map is stretched across that rectangle, so doubling the point count doubles each WorldMap cell.
- `ASolidTerrainChunk` — runtime `UStaticMesh`; vertex heights from the TerrainMap; per-triangle biome materials, and a border uses the higher shelf
- `ASolidTerrainStreamer` — builds the map, relocates the pawn to location 0, then loads/unloads chunks
- Debug HUD shows biome / threat / fog at the pawn plus map size

Defaults: 64 m chunks (`ChunkWorldSize=6400`), 32 quads/side, radius 2 (5×5 chunks), `Amplitude=3000`. Grassland uses Fab `Mat_025_grass`; other biomes use FlatCol tinted by the WorldMap key colors.

Default map is `/Game/ThirdPerson/Lvl_ThirdPerson` (SC1-0022) so Open World Landscape/HLOD outer hills are not in the scene — that cleared the horizon slivers. `L_OpenWorld` remains for comparison. SC1-0023/0082 force `BP_SolidGameMode` on PIE/game worlds so template maps keep Solid HUD + Viking pawn instead of the template default. On first stream, the pawn is snapped onto the procedural height. If a Landscape is present, actors are hidden/collision-disabled once. Chunks block the Camera channel and use complex-as-simple collision on the runtime static mesh.

SC1-0007 drops `UProceduralMeshComponent` after persistent ribbon/culling failures with that path.

## Exploration fog (fog-of-war)

See [`Source/SolidCore1/Fog/README.md`](Source/SolidCore1/Fog/README.md) — trail-cleared bands, marching-squares curtains, and rejected approaches.

## WorldMap biomes (SC1-0105)

`Content/WorldMap.txt` is a 128×64 grid of markers plus a color key (`~` Sea, `.` Grassland, `o` Town, `M` Mountain, `#` Forest, `^` Desert, `*` River). A `!` is one cell. `! 8,21 = Town Iglin` names row 8, column 21, counting from 1 at the top-left of the grid. The identifier may be wrapped in brackets, as in `Town (Iglin)`. `! 9,12 = Stronghold MeagreDungeon` is that size and type (`Meagre Dungeon` is the same token). `! 12,12 = StrongholdSite (Humble)` names a size and no type. A site is not a town and keeps the marker's biome. `! 10,11 = LairEntrance Iglin tunnels` and `! 11,40 = LairExit Iglin tunnels` are the two mouths of that lair. Anything after `#` is a comment. A town `!` is that town. A lair `!` uses the mountain shelf and is not a town. A stronghold `!` is not a town. Its shelf comes from that stronghold: Meagre Dungeon is Grassland, and its exit faces north. Imperious Tower is Grassland. A stronghold with no shelf listed keeps the marker's biome. A `!` with no line is not a location. A coordinate outside the grid, or on a cell that is not `!`, is ignored, and the map still loads. A number on the right-hand side is ignored. Town names live in `Towns/SolidTowns.h`. `LairEntrance` and `LairExit` are the Iglin tunnel mouths. `LairEntrance` is the interior `X` end and `LairExit` is the interior `Y` end. Those `!` cells use the mountain shelf, so the cliff stays closed, and they are not towns, locations, or the cave. A digit, or the letters `C`, `Z`, `X`, and `Y`, is grassland and is not a location, a mouth, or the start. A key line for one of those does not recolor a biome. The grid scales across the TerrainMap world rectangle (file row 0 = north). The default rectangle is 1024 m on a side. A marker is 8 m east–west and 16 m north–south. A `*` marks a river through that square, not a square of water. The channel is 4 m wide. An isolated `*` is a 4 m pond. A `*` beside another `*`, or beside `~`, joins that neighbor edge to edge and stays 4 m wide however long the square is. A diagonal neighbor does not join. The dry part of the square uses the adjacent cell, so the two banks can differ (grass on one side, mountain on the other, or sea where the river meets it). A `Road:` line under the key is `start -> end, type`. An end is a town name or a stronghold name such as `MeagreDungeon`. Brackets around a name are optional. A trail is 1 m wide, a track 2 m, a road 3 m, and a highway 4 m. The route is solved on the grid and drawn in a brown close to the town colour. Houses are placed only on cells marked `o` or a `!` named as a town. Every route stays off mountains and the sea. Roads and highways also stay off forest and desert, and they pay extra for each river cell so a crossing stays short. Where the strip meets the water, the channel keeps the river texture. Location **0** (Iglin) is the starting town: the streamer relocates the player there. Fog bands are centered on the player after that placement.

## Forest trees (SC1-0076 / SC1-0110)

- `ASolidTree` — cylinder trunk + cone canopy; scattered randomly across **Forest** TerrainPoints (`ForestTreeDensity`, capped by `MaxForestTrees`).
- Logic in `Vegetation/SolidForestTrees.*`. Toggle with `bAutoSpawnVegetation`.
- Fog curtains: see [`Fog/README.md`](Source/SolidCore1/Fog/README.md) (clear|fogged ~25 m plus a **white** half→full curtain ~50 m).

## Name labels (SC1-0091 / SC1-0093)

Floating nameplates (`SolidNameLabel`) sit above each Squad member and face the view camera: TextRender plus thin cube **border** and contrasting **background** plates. Defaults: Captain **"Outcast"** (larger warm amber on dark plate), Companions muted slate plates. The captain's plate shows in raven sight only, and only when at least one companion is with him outside a stronghold. Enter Iglin renames the captain to Orphan. Driven from the village sets Outcast. Relion renames the captain to Vagrant, Kanfold to Ranger, and Dragongate to Captain. Override via `CharacterDisplayName` / `SetCharacterDisplayName`.

## Battle plans (SC1-0095 / SC1-0101)

- **Company** (`USolidCompany`) owns the catalog of all battle plans. Starter assigned set:
  - **F1 Line** (Standard) — companions on the flanks
  - **F2 Column** (Standard) — file behind the Captain
  - **F3 Tight mob** (Narrow) — close triangle behind
  - **F4 Loose mob** (Wide) — spread triangle behind
  - **F5 Parade** (Standard) — line in front of the Captain, facing him, far enough that the rank fits in his view. Selecting it fires Enter Iglin if needed, then Enter Relion and Enter Dragongate, so the rank, raven sight, and the repaired Iglin bridge are ready even at the start of a session. Those events still fire only once.
- **Squad** (`USolidSquad`) holds up to 8 **assigned** plans and one **active** plan (default **F1 Line**).
- Each plan has a **formation** plus **spacing** (`Narrow` / `Standard` / `Wide`) that scales follow distances.
- Captain switches assigned slots with **F1–F8**, or a shoulder button plus a face button (left shoulder for the first four, right shoulder for the last four). Only filled slots work. The choice stays put when town events refresh the squad.
- **Test Drill** (main menu) runs a formation drill: F1 walk 1.5s → F2 / F3 / F4 each blend +90° yaw over 250 ms then walk 1.5s (square path; player move/look suppressed while active).
- HUD shows a **Battle Plans** panel under the tech block: all 8 slots, F-key + name when assigned, active slot highlighted in amber (aligned marker column).
- Engine viewmode debug binds (wireframe/unlit/lit/…) are moved to **Ctrl+F1–F5** in `Config/DefaultInput.ini` so bare F-keys stay free for battle plans. Restart the editor after pulling.

## Companion (SC1-0024 / SC1-0093)

`ASolidCompanionCharacter` follows with simple steering (no NavMesh — works on procedural terrain). The captain stays on `SK_Viking`. A companion wears a low-poly body skinned to the Viking skeleton: a chest, a tapered cylinder over each shoulder, arms, and legs down to the feet at body thickness. The hip cylinder's bottom meets the outside of the thighs. The shirt, the trousers, and the skin on the head and hands are separate matte colors. Those colors live on the companion record and are applied on the actor, so a shared body is not recolored while it is on screen. Sam's shirt is red and his trousers are blue. Alex wears the swap. Both use light pink skin. The F10 character editor shows one companion on a plain background so those colors can be checked without the open world. Space swaps that body for the Viking skeleton, drawn as bone lines. Bones that share a joint use different colours. Home turns the figure clockwise and End turns it the other way. Resting the pointer on a bone shows that bone's name in black. Clothing and weapon bones are left off that view: axe_, sheath_, cloack_, beard_, shield_, and skirt_. Thigh and calf twist bones are left off too. calf and foot stay, so the hip-to-knee line and the knee-to-ankle line are both drawn. Page Up and Page Down cycle the company. Up and Down select height, build, or gender, and Left and Right change that value. The pointer can drag the same sliders. F10 returns to the world. The face is chosen from the same unicode points, including U+1F611. The engine font has no emoji, so the expression is drawn on the front of the head. The body stays lighter than the Viking sculpt. The three sliders run from 0 to 1 and set every proportion. Build 0 is short and wide, and build 1 is tall and thin. Gender 0 is feminine and gender 1 is masculine. Height 0.5 leaves that size; 0 shortens it and 1 lengthens it. At 0.5 on every slider every part stays at its catalogue size and every bone keeps the Viking length. Moving a slider changes those part diameters and lengths, and the bones inside the part take that size. The head and the hands are not parts, so they stay the Viking size. The body is being built one piece at a time. The pieces so far are the two thighs and the two calves. Each thigh is a truncated cone the length of calf_l or calf_r, from the hip to the knee, 20 cm across at the hip and 15 cm across at the knee. Each calf is one cone the length of foot_l or foot_r, from the knee to the ankle, 15 cm across at the knee and 12 cm at the ankle. Each foot is a cylinder the length of ball_l or ball_r, from the ankle to the toe, 10 cm across. Each toe is a truncated cone past the foot, 10 cm across at the end towards the ankle and 5 cm at the toe. The hips are one shirt piece around thigh_l, thigh_r, and spine_01. It is the length of spine_01. The bottom is 50 cm side to side and 20 cm front to back, and the top is 30 cm side to side and 20 cm front to back. Each spine bone above spine_01 is its own shirt piece, still 20 cm front to back, widening from that 30 cm waist to 40 cm side to side at the top of the spine. The shoulders are one shirt piece around clavicle_l, clavicle_r, upperarm_l, upperarm_r, and neck_01. It is the length of neck_01. Its base is the same oval as the top of the spine_03 piece. It widens past the upperarm bones, then narrows to a circle of 20 cm diameter at neck_01. The drawn body is one list of named parts. Each part draws its own triangles, and the list draws the whole character. A part's name is the region it shows, which is often different from the skeleton bone it wraps. Each part stores that bone list, a shirt, trouser, or skin colour, and the side-to-side and front-to-back diameters at the start and the end of its cone. The shoulders also store a mid section, fitted when the piece is drawn. The upper arm is a shirt cone the length of lowerarm_l or lowerarm_r, 8.4 cm at the shoulder and 6.8 cm at the elbow. The lower arm is a shirt cone the length of hand_l or hand_r, 5 cm at the elbow and 6.8 cm at the wrist. The shoulder_jnt, finger, head, and eye bones are still bare, and so are the twist bones. Hovering a piece names the bones in it. The head is opaque. A companion's face still comes from the display name, and that body is built once and reused. They play Viking clips. The company catalog still lists the Assassin mesh for Sam, Morgan, and Quinn (`anim_assassin_idle_2`, `anim_assassin_Walk`, `anim_assassin_run`; `anim_assassin_idle_1` is a crouch) and the Blacksmith mesh for Alex and Max. Those catalog meshes are replaced when the actor applies its visuals. The event roster is Sam, Alex, Max, Morgan, and Quinn. The selection starts as Sam and Alex, and only that selection walks, up to the current max squad size. That cap starts at 0 and is raised by town events; Dragongate raises it to five. Offsets come from the active Battle Plan. HUD lists each companion name and distance.

**Squad camera** (SC1-0025/0026/0084): the Captain spring-arm shifts `TargetOffset` toward the Squad center and may lengthen so Companions stay in frame (`bFrameCompanions`; implementation in `SolidSquadCamera.cpp`). SC1-0026 uses screen-space fit, disables boom collision while Companions are present, and zooms in much slower than out. SC1-0027 lifts the camera via `SocketOffset` when the predicted camera point would sink below procedural terrain.

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
  WorldMap.txt         # ASCII 128×64 biome overlay + color key
tools/
  fab-assets.json      # Fab listing IDs, plus recorded paths once a search finds them
  fab_manifest.ps1     # read or update one recorded path
  fab_doctor.bat       # recorded path first, then search; write back a find
  fab_mesh_info.bat    # list meshes and animations in a downloaded pack
  build.bat            # SolidCore1Editor Win64 Development
  run_automation_tests.bat
Source/
  SolidCore1.Target.cs
  SolidCore1Editor.Target.cs
  SolidCore1/
    SolidCore1.Build.cs
    SolidCore1.h / .cpp                # Module startup and the log
    Player/
      SolidCharacter.h / .cpp            # Captain core (move/look/zoom/sprint)
      SolidCharacterInput.cpp            # Runtime Enhanced Input factory
      SolidCharacterVisuals.cpp          # Captain mesh + clip selection
      SolidCharacterFormationDrill.cpp   # Square-walk formation drill
      SolidPlayerController.*
      SolidSquadCamera.cpp               # Squad framing + terrain boom lift
      SolidClickMove.h                   # Raven-sight click-to-move ground ray
      SolidSight.h                       # True sight / raven sight toggle
      SolidCameraFog.h                   # Boom scale that keeps the camera over Fog == 0
    Character/
      SolidNameLabel.*                   # Floating nameplates
      SolidClipLocomotion.*              # Shared single-node clip apply/play
      SolidCharacterStage.*              # Plain room for the character editor
    Game/
      SolidGameMode.*
      SolidGameState.*                   # USolidGameState: journal, inventory, events, previous strongholds
      SolidGameInstance.*                # Company (and its stronghold), travel session, fog trail, and world-time clock
      SolidWorldTime.h                   # Journal date and clock
      SolidSessionSave.h                 # How many play sessions have started
      SolidBuildId.h                     # SC1-NNNN shown on the debug HUD
    Content/
      SolidMaterials.*                   # Shared FlatCol solid-color MID helper
      SolidContentLookup.*               # Recorded Fab path, then Viking/grass by name and companion packs by folder
      SolidFabManifest.*                 # tools/fab-assets.json recorded paths
      SolidContentPaths.h                # Canonical BP soft-class paths
    Companion/
      SolidCompanionCharacter.*          # Companion NPC follower
    Fog/
      README.md               # FoW history, rejected approaches, invariants
      SolidTerrainFog.*       # FoW bands, boundary mesh, materials, height-fog helpers
    Terrain/
      SolidTerrainTypes.h
      SolidWorldMap.*         # Parse/scale Content/WorldMap.txt; location 0 centroid
      SolidRiver.h            # 4 m river channel and pond
      SolidRoad.*             # Trail, track, road, and highway routes
      SolidBridge.*           # Ramp, flat center, and ramp where a road crosses a river
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
      SolidTownSign.*         # Pole, board, "Welcome to" over the town name
      SolidTownSigns.*        # One sign per named town, at the middle of its cells
      SolidTowns.h            # Town names, type, and welcome-sign position, keyed by the WorldMap index
      SolidWallgate.*       # Stone wall and two towers for a WallGate town
    Strongholds/
      SolidStronghold.*       # Type, size, the shelf, the grass face, the cliff frame, and the move to a site
      SolidDungeonPassage.h  # Click on the dark opening to enter or leave
      SolidDungeonLevel.h        # Travel options; captain only; room bounds
      SolidTower.*                  # The tower visit. Average through Stately are Imperious floor bands. Imperious is the full shaft. Each floor may shift its hex grid by at most 30 cm
      SolidDungeonExterior.*        # Dungeon exterior: a rock with one dark mouth, centered on the stronghold cell
      Tower/Actors/SolidTowerExterior.*  # Tower exterior: smooth stone courses, a flat cap, and a dark door, centered on the tower cell
      Tower/Actors/SolidTowerInterior.*  # Tower visit: discs, arrival holes, flat landings, floor numbers, rails, treads, yellow grid, and the door that leaves
      Furnishings/SolidStrongholdGrid.h  # 1.2 m pointy-top hexes shared by every stronghold floor
      Furnishings/SolidFloorSign.h      # The floor number: dark bars on a light plate
      Furnishings/SolidHandrail.h       # A waist-high bar and the posts that hold it
      SolidStrongholdFurniture.h    # Furnishing catalog, and the pieces each stronghold uses
      SolidStrongholdSite.*         # A stronghold site, drawn as a stone block. A click lists each unlocked larger size that has a built type
      SolidFurnishing.*             # One placed piece: hearth, bed, or chest
      SolidDungeonInterior.*     # Irregular fourteen-hex Meagre floor
    Lairs/
      SolidLairEntrance.*     # Cliff frame and solid dark opening for an Iglin tunnel end
    HUD/
      SolidHUD.*              # Debug HUD, battle-plan panel, journal and inventory boards, menu and credits drawing
      SolidHudTheme.h         # Oak-and-bronze frame
      SolidPointer.h          # In-game mouse pointer: two head triangles, tip on the hotspot, no tail
    Events/
      SolidEvent.h            # Event type, change helpers, and the builders for a town, a bridge, and a closed message
      SolidEvents.h           # The shipped event list
      SolidMessages.h         # Message ids and text
    Journal/
      SolidJournalEntry.h     # One line: Event or Stronghold
      SolidJournal.*          # Journal page: time column, entry column, larger board
    Inventory/
      SolidItemType.h         # Item catalog: id, name, kind, quality, and max stack
      SolidStorage.h          # Cave chest slots; rusty sword, old boots, 20 coins, five sticks, and three pitch
      SolidInventory.*        # Worn boxes and pack store those ids; slot names above the boxes; four crafting slots; Craft spends a known recipe into the first empty pack cell; drag between slots and the chest; starting kit; clear to Rags; / to open
    Crafting/
      SolidRecipe.h           # Recipe: name, output item, inputs with counts, known flag; Torch takes one stick and one pitch
    Menus/
      SolidCharacterEditor.*  # F10 character editor: page cycle, height build and gender sliders, Sam and Alex colors
      SolidMainMenu.*         # F10 menu entries (Use portal, Test Drill, Journal, Credits, Build squad, Enter cave, Character editor, Inventory)
      SolidSquadBuilder.*    # company and plan lists, drag onto squad boxes and F1–F8, Confirm closes the page, a stronghold leave opens it when there is something to arrange, locked until two companions are allowed
      SolidPortal.h           # Town list, Iglin tunnels row, and squad landing
      SolidMenuPointer.h      # Pointer hit targets for the menu and portal list
      SolidCredits.*          # Credits page copy
    Tests/
      README.md                       # What the SolidCore1 suite covers
      SolidTerrainTestHelpers.h   # Shared MakeSmallMap fixture
      SolidWorldMapTests.cpp
      SolidRiverTests.cpp
      SolidRoadTests.cpp
      SolidTerrainFogTests.cpp
      SolidTerrainFogMeshTests.cpp
      SolidTerrainMapTests.cpp
      SolidTerrainMapMoreTests.cpp
      SolidTerrainNoiseTests.cpp
      SolidTerrainTypesTests.cpp
      SolidVegetationTests.cpp
      SolidTownSignTests.cpp
      SolidTownsTests.cpp
      SolidWallgateTests.cpp
      SolidStrongholdTests.cpp
      SolidDungeonLevelTests.cpp
      SolidStrongholdGridTests.cpp
      SolidStrongholdFurnitureTests.cpp
      SolidStrongholdTransitionTests.cpp
      SolidGameModeTests.cpp
      SolidCompanionTests.cpp
      SolidTerrainStreamerTests.cpp
      SolidClipLocomotionTests.cpp
      SolidBuildIdTests.cpp
      SolidPointerTests.cpp
      SolidClickMoveTests.cpp
      SolidCharacterEditorTests.cpp
      SolidMainMenuTests.cpp
      SolidInventoryTests.cpp
      SolidItemTypeTests.cpp
      SolidStorageTests.cpp
      SolidRecipeTests.cpp
      SolidPortalTests.cpp
      SolidMenuPointerTests.cpp
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
- Original project source is Apache 2.0. `NOTICE` says what that license does not cover: Unreal Engine, the committed template Examples, and the Fab packs.
- Optional: create Blueprint subclasses of `SolidCharacter` / `SolidGameMode` for content-driven tuning without changing C++. Keep names `BP_Solid*`.
