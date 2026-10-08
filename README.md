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
Content/Maps/          # create L_OpenWorld here in Editor
Source/
  SolidCore1.Target.cs
  SolidCore1Editor.Target.cs
  SolidCore1/
    SolidCore1.Build.cs
    SolidCore1Character.*
    SolidCore1GameMode.*
    SolidCore1PlayerController.*
```

## Requirements

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`
- Visual Studio 2022 with **Game development with C++** (MSVC, Windows SDK)

## Notes

- This repo is source + config only. `Binaries/`, `Intermediate/`, `Saved/`, and `.sln` are gitignored and generated locally.
- Optional: create Blueprint subclasses of `SolidCore1Character` / `SolidCore1GameMode` for content-driven tuning without changing C++.
