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
| Jump | Space | A / Cross |
| Sprint | Left Shift | Left stick click |

Input Actions / Mapping Context can be replaced with Content assets on the character later; if unset, C++ creates transient defaults so PIE works immediately.

## Build ID (HUD)

PIE shows a top-left debug HUD (`Build SC1-NNNN`, pawn/terrain Z, chunk load, material, camera pitch). The build string lives in `Source/SolidCore1/SolidCore1BuildId.h` (`SOLIDCORE1_BUILD_ID`) and is bumped on every GitHub push so screenshots identify which binary you ran.

## Visible mannequin (mesh + anim)

`SolidCore1Character` loads Epic’s Third Person mannequin when present (UE 5.7+ often uses the `_Simple` mesh):

- Mesh: `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`
- Anim BP: `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed` (or `Animations/ABP_Unarmed`)

If those exact names differ, the character also searches `/Game/Characters/Mannequins` via the Asset Registry.

Those `.uasset` files are **not** in git (binary content). Add them once via Migrate / Add Feature, then use the Blueprint pawn below (most reliable).

### Reliable setup: Blueprint pawn (recommended)

1. Content Browser → right-click `Content/Characters` → **Blueprint Class**.
2. Pick **SolidCore1Character** as the parent → name it `BP_SolidCore1Character` (path must be `/Game/Characters/BP_SolidCore1Character`).
3. Open it → select **Mesh (CharacterMesh0)**:
   - **Skeletal Mesh**: `SKM_Manny_Simple`
   - **Anim Class**: `ABP_Unarmed`
4. Compile & Save.
5. Close the editor, rebuild/reopen so GameMode picks up the Blueprint (it prefers this BP over the bare C++ class).
6. PIE — you should see the mannequin.

While PIE is running, **Output Log** filtered to `SolidCore1` shows whether a mesh was applied or how many meshes were found.

You can commit `Content/Characters/` to GitHub if you want the mannequin shared with the repo (large binaries; Git LFS recommended).

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

- `ASolidCore1TerrainChunk` — `UProceduralMeshComponent` from seeded fBm noise (seamless across chunk edges)
- `ASolidCore1TerrainStreamer` — loads/unloads a Chebyshev radius of chunks around the pawn
- `ASolidCore1GameMode` auto-spawns the streamer on BeginPlay (`bAutoSpawnTerrainStreamer`)

Defaults: 64 m chunks (`ChunkWorldSize=6400`), 32 quads/side, radius 2 (5×5 chunks), `Amplitude=1200`. Tunable on the streamer actor. Chunks use a green `BasicShapeMaterial` MID (lit). Avoid the engine vertex-color debug material (black in PIE) and WorldGrid-only without an underside (near mesh vanishes at grazing angles).

On first stream, the pawn is snapped onto the procedural height so you are not stuck on the empty Open World landscape. Landscape actors are hidden/collision-disabled automatically. Chunks block the Camera channel, pad section local bounds (never write local boxes into world `Bounds`), and keep a 3 cm-biased underside section so one-sided lit materials do not disappear when the camera grazes the surface.

Requires the **ProceduralMeshComponent** plugin (enabled in `SolidCore1.uproject`).

## Create the open-world map

Binary `.umap` assets are created in the Editor (not checked in as source):

1. **File → New Level → Open World** (World Partition).
2. Save as `Content/Maps/L_OpenWorld`.
3. Place a **Player Start** near the origin (or on the landscape).
4. Confirm **Project Settings → Maps & Modes**:
   - Editor Startup Map / Game Default Map → `/Game/Maps/L_OpenWorld`
   - Default GameMode → `SolidCore1GameMode`
5. **Play** (PIE).

`Config/DefaultEngine.ini` already points at `/Game/Maps/L_OpenWorld` and `SolidCore1GameMode`. Until that map exists, the editor may warn that the map is missing—create it once as above.

World Partition and Large Worlds are enabled in project config for open-world scale.

## Project layout

```
SolidCore1.uproject
Config/
Content/Maps/          # L_OpenWorld (+ World Partition externals)
Source/
  SolidCore1.Target.cs
  SolidCore1Editor.Target.cs
  SolidCore1/
    SolidCore1.Build.cs
    SolidCore1Character.*
    SolidCore1GameMode.*
    SolidCore1PlayerController.*
    Terrain/
      SolidCore1TerrainNoise.h
      SolidCore1TerrainChunk.*
      SolidCore1TerrainStreamer.*
```

## Requirements

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`
- Visual Studio 2022 with **Game development with C++** (MSVC, Windows SDK)

## Notes

- This repo is source + config only. `Binaries/`, `Intermediate/`, `Saved/`, and `.sln` are gitignored and generated locally.
- Optional: create Blueprint subclasses of `SolidCore1Character` / `SolidCore1GameMode` for content-driven tuning without changing C++.
