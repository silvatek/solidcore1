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
- [Create the open-world map](#create-the-open-world-map)
- [Project layout](#project-layout)
- [Data structure](#data-structure)
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

Grass and Viking come from Fab. There is **no official download API**. Native UE `.uasset` packs cannot be batch-exported — each listing is **Add to Project** on its own. `resolvedPath`, `resolvedFolder`, and `resolvedPaths` in `tools/fab-assets.json` are tried first. If that file or folder is missing, Viking and grass are found by **asset name** under `/Game/Viking` or `/Game/Fab`. A find is written back into the manifest. Do **not** move folders after Add to Project.

| Asset | Listing | Required | Resolved as |
|-------|---------|----------|-------------|
| Viking (Art.Hiraeth) | [fab.com/listings/ca4ba583-…](https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f) | Yes | `SK_Viking` + `Anim_Viking_*` (captain and companion clips) |
| 025 Grass | [fab.com/listings/94bfee39-…](https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a) | Yes | `Mat_025_grass` |

**Restore (Launcher + UE 5.8):**

1. Epic Games Launcher → **Unreal Engine → Fab**. Sign in.
2. Open each listing. **Add to My Library** if needed (Ctrl+click can batch this).
3. **Add to Project** → this `SolidCore1.uproject`. Done.

```bat
tools\fab_doctor.bat
```

tries each recorded path in `tools/fab-assets.json` first, then searches. Viking and grass fall back to `Content/Viking` or `Content/Fab`. A find is written back. Official notes: [Exporting Assets from Fab in Launcher](https://dev.epicgames.com/documentation/en-us/fab/exporting-assets-from-fab-in-launcher).

```bat
tools\fab_mesh_info.bat Viking
```

lists that pack's folders, mesh files (size and SKM / SK / PA), and animation names. It does not change `fab-assets.json`.

The packs are not in git. `.gitignore` keeps `Content/Fab` and `Content/Viking` out of the repository, and also `Content/Medieval_Assassin_Girl` and `Content/Elf_Blacksmith`, so a leftover local copy of those packs is not committed. A clone does not contain the packs. Add to Project, then run `tools\fab_doctor.bat`. Do not commit a pack that lands under a different folder name. Viking is Fab Standard, so its source files stay off the public repo. Grass is CC-BY.

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
.gitignore
.cursor/
  environment.json             # Cloud agent environment
AGENTS.md                      # Agent workflow
AgentBuild.md                  # Self-hosted Windows build
CodingGuidelines.md
Config/
  DefaultEditor.ini
  DefaultEngine.ini
  DefaultGame.ini
  DefaultInput.ini
Content/
  __ExternalActors__/          # World Partition actors for the maps
  __ExternalObjects__/         # World Partition objects for the maps
  Characters/                  # BP_SolidCharacter, BP_SolidGameMode
  Controls.md                  # Keyboard and gamepad defaults
  Fauna.md                     # Fauna body types
  Input/                       # Enhanced Input actions and mapping contexts
  Lairs.md
  LevelPrototyping/            # Template meshes and FlatCol
  Maps/                        # L_OpenWorld
  ThirdPerson/                 # Lvl_ThirdPerson, the startup map
  Viking/                      # README.md only; the Fab pack is gitignored
  World/                       # World chart
  WorldLore.md
  WorldMap.txt                 # ASCII 128×64 biome overlay and color key
GameVision.md                  # Design north star
ideas.md                       # Story inbox
LICENSE
NOTICE
Planning/                      # One open story per file
  README.md                    # How a story file works
  Done/                        # Finished stories
README.md
SolidCore1.uproject
Source/
  SolidCore1.Target.cs
  SolidCore1Editor.Target.cs
  SolidCore1/                  # This folder contains all the project source code
  SolidCore1Editor/            # Editor module and the World chart mode
tools/
  agent.bat                    # Start the solidcore1-ue worker; write agent.ok on success
  build.bat                    # SolidCore1Editor Win64 Development
  cloud_agent_check.py         # Branch and repo checks before a push
  editor.bat                   # Open the editor on Lvl_ThirdPerson
  fab-assets.json              # Fab listing IDs and recorded paths
  fab_doctor.bat               # Recorded path first, then search; write back a find
  fab_manifest.ps1             # Read or update one recorded path
  fab_mesh_info.bat            # List meshes and animations in a downloaded pack
  fab_mesh_info.ps1
  pie.bat                      # Play In Editor
  test.bat                     # Run the SolidCore1 automation tests
```

## Data structure

The game instance lasts for the play session. OpenLevel keeps it. The game mode and the terrain streamer belong to the loaded world.

```
USolidGameInstance                  # Session root
  USolidCompany                     # Catalog, the captain, and the company base
    USolidCaptain                   # Title, occupied town, and which sights are unlocked
      ASolidCharacter               # Weak pointer to the live pawn. Current sight stays on the actor
    FSolidBattlePlan[]              # Starter, Attack, Defend, Parade, Line, Column, Tight mob, Loose mob
    FSolidCharacterMesh[]           # Viking
    FSolidCompanion[]               # Sam, Alex, Max, Morgan, Quinn
    FSolidStronghold                # Active base: Dungeon, Fortress, or Tower
  USolidGameState                   # Events, squad cap, enabled battle plans, lairs, and the unlocked stronghold size
    USolidJournal                   # Event, stronghold, and lair lines
    FLoadout                        # Worn boxes, pack, and crafting slots
    FContainer                      # Cave chest
    FSolidStronghold[]              # Bases the company moved on from
    FUsed[]                         # Furnishings in the active stronghold
  FSolidTravelSession               # Squad choices carried across a closed visit
  float[]                           # Fog trail, copied off the terrain map before a visit
ASolidGameMode                      # World root. OpenLevel replaces it
  USolidSquad                       # Indexes into the company. Does not own it
  USolidWorldChart                  # /Game/World/WorldChart
    ESolidBiome[]                   # Marker biome of each cell
    FSolidWorldChartMark[]          # Towns, strongholds, sites, lair mouths, notes
    FSolidWorldChartRoad[]          # Two end names and a type
  ASolidTerrainStreamer
    USolidTerrainMap
      FSolidTerrainPoint[]          # Height, biome, threat, and fog
      USolidWorldMap                # The chart, loaded for play
        uint8[]                     # 128×64 markers
        FSolidWorldBang[]           # Named ! cells
        FRoute[]                    # Solved roads
    ASolidTerrainChunk[]
  ASolidCompanionCharacter[]        # Fielded squad
  ASolidStrongholdExterior[]
  ASolidStrongholdInterior          # This stronghold visit
  ASolidStrongholdSite[]
  ASolidLairEntrance[]
  ASolidLairInterior                # This tunnel visit
  ASolidTree[]
  ASolidForagePile[]
  ASolidBuilding[]
  ASolidTownSign[]
  ASolidBridge[]
```

## Requirements

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`
- Visual Studio 2022 with **Game development with C++** (MSVC, Windows SDK)

## Notes

- This repo is source + config only. `Binaries/`, `Intermediate/`, `Saved/`, and `.sln` are gitignored and generated locally.
- Original project source is Apache 2.0. `NOTICE` says what that license does not cover: Unreal Engine, the committed template Examples, and the Fab packs.
- Optional: create Blueprint subclasses of `SolidCharacter` / `SolidGameMode` for content-driven tuning without changing C++. Keep names `BP_Solid*`.
