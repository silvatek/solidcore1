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

## Build ID (HUD)

PIE shows a top-left debug HUD (`Build SC1-NNNN`, a one-line `Change:` note, pawn/terrain Z, chunk load, material, camera pitch). Both strings live in `Source/SolidCore1/SolidBuildId.h` (`SOLID_BUILD_ID` / `SOLID_BUILD_NOTE`) and are bumped on every GitHub push so screenshots identify which binary you ran.

## Visible mannequin (mesh + anim)

`SolidCharacter` loads Epic’s Third Person mannequin when present (UE 5.7+ often uses the `_Simple` mesh):

- Mesh: `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`
- Anim BP: `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed` (or `Animations/ABP_Unarmed`)

If those exact names differ, the character also searches `/Game/Characters/Mannequins` via the Asset Registry.

Those `.uasset` files are **not** in git (binary content). Add them once via Migrate / Add Feature, then use the Blueprint pawn below (most reliable).

### Reliable setup: Blueprint pawn (recommended)

1. Content Browser → right-click `Content/Characters` → **Blueprint Class**.
2. Pick **SolidCharacter** as the parent → name it `BP_SolidCharacter` (path `/Game/Characters/BP_SolidCharacter`).
3. Open it → select **Mesh (CharacterMesh0)** if you want editor defaults; SC1-0034 forces Fab Viking at runtime when `bUseVikingVisuals` is true (overrides a Manny mesh/AnimBP on the BP).
4. Compile & Save.
5. Close the editor, rebuild/reopen so GameMode picks up the Blueprint (it prefers this BP over the bare C++ class).
6. PIE — you should see the Viking player (and Viking companion).

**Naming:** New Blueprints should use the `Solid*` forms (`BP_SolidCharacter`, `BP_SolidGameMode`, parent `SolidCharacter` / `SolidGameMode`, etc.). Legacy `BP_SolidCore1*` assets remain supported via CoreRedirects and fallback load paths.

While PIE is running, **Output Log** filtered to `LogSolid` shows whether a mesh was applied or how many meshes were found.

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

- `FSolidTerrainPoint` / `ESolidBiome` — simulation cell (X, Y, Height, Biome, Threat, Fog)
- `USolidTerrainMap` — 2D TerrainPoint grid built once at streamer startup (default 257×257 @ chunk vert spacing)
- `ASolidTerrainChunk` — runtime `UStaticMesh`; vertex heights sampled from the TerrainMap
- `ASolidTerrainStreamer` — builds the map, then loads/unloads a Chebyshev radius of chunks around the pawn
- Debug HUD shows biome / threat / fog at the pawn plus map size

Defaults: 64 m chunks (`ChunkWorldSize=6400`), 32 quads/side, radius 2 (5×5 chunks), `Amplitude=3000`. Material: Fab `Mat_025_grass` when present.

Default map is `/Game/ThirdPerson/Lvl_ThirdPerson` (SC1-0022) so Open World Landscape/HLOD outer hills are not in the scene — that cleared the horizon slivers. `L_OpenWorld` remains for comparison. SC1-0023 forces `BP_SolidCore1GameMode` on PIE/game worlds so template maps keep Manny + the debug HUD instead of Quinn. On first stream, the pawn is snapped onto the procedural height. If a Landscape is present, actors are hidden/collision-disabled once. Chunks block the Camera channel and use complex-as-simple collision on the runtime static mesh.

SC1-0007 drops `UProceduralMeshComponent` after persistent ribbon/culling failures with that path.

## Exploration fog (fog-of-war)

Goal: the world starts shrouded; fog clears only where the **pawn has been** (trail), independent of camera orbit / pitch / zoom.

**Data (kept):** `FSolidTerrainPoint.Fog` on `USolidTerrainMap`. Initial fill and trail clear use the same bands — ≤25 m → `0`, 25–50 m → `0.5`, >50 m → `1`. Runtime: `ApplyExplorationFogAround` as the pawn moves.

**Current visual (SC1-0073):** world-space **boundary curtains** via marching-squares isocontour (`fog≈0` | `fog>0`) so panels meet without stair-step gaps. Materials are **programmatic `BLEND_Translucent`** unlit mist. Logic in `Terrain/SolidTerrainFog.*`. Height fog **off**.

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

## Starter trees (SC1-0075)

`ASolidTree` (`Vegetation/SolidTree.*`) is a placeholder: Engine BasicShapes **cylinder** (brown trunk) + **cone** (green canopy). GameMode spawns a **line** of them from near the origin into the fog (`StarterTreeCount=16`, ~10 m spacing, random trunk/canopy size). Toggle with `bAutoSpawnStarterTrees`.

## Quinn companion (SC1-0024)

`ASolidCompanionCharacter` spawns behind the player and follows with simple steering (no NavMesh — works on procedural terrain). Both player and companion use the Fab Viking (`/Game/Viking/Mesh/SK_Viking`) with idle/walk/run/(jump) clip playback (custom skeleton — not Epic AnimBP). GameMode flag: `bAutoSpawnCompanion`. HUD shows companion mesh name and distance. Set `bUseVikingVisuals=false` on the player to fall back to Manny.

SC1-0025/0026: the player spring-arm camera shifts its `TargetOffset` toward the group center and lengthens so all companions stay in frame (`bFrameCompanions`). SC1-0026 uses screen-space fit, disables boom collision while companions are present (hill probes were collapsing the arm), and zooms in much slower than out so the shot does not pop narrow. SC1-0027 lifts the camera via spring-arm `SocketOffset` when the predicted camera point would sink below the procedural terrain height (keeps framing arm length intact).

## Create the open-world map

Binary `.umap` assets are created in the Editor (not checked in as source):

1. **File → New Level → Open World** (World Partition).
2. Save as `Content/Maps/L_OpenWorld`.
3. Place a **Player Start** near the origin (or on the landscape).
4. Confirm **Project Settings → Maps & Modes**:
   - Editor Startup Map / Game Default Map → `/Game/ThirdPerson/Lvl_ThirdPerson` (horizon test; Open World map still available)
   - Default GameMode → `SolidGameMode`
5. **Play** (PIE).

`Config/DefaultEngine.ini` already points at `/Game/Maps/L_OpenWorld` and `SolidGameMode`. Until that map exists, the editor may warn that the map is missing—create it once as above.

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
    SolidCharacter.*
    SolidGameMode.*
    SolidPlayerController.*
    Terrain/
      SolidTerrainTypes.h
      SolidTerrainFog.*       # FoW bands, boundary mesh, materials, height-fog helpers
      SolidTerrainMap.*
      SolidTerrainNoise.h
      SolidTerrainChunk.*
      SolidTerrainStreamer.*
    Vegetation/
      SolidTree.*             # Placeholder cylinder+cone tree
```

## Requirements

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`
- Visual Studio 2022 with **Game development with C++** (MSVC, Windows SDK)

## Notes

- This repo is source + config only. `Binaries/`, `Intermediate/`, `Saved/`, and `.sln` are gitignored and generated locally.
- Optional: create Blueprint subclasses of `SolidCharacter` / `SolidGameMode` for content-driven tuning without changing C++. Name new BPs `BP_Solid*` (not `BP_SolidCore1*`).
