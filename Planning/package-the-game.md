# Package the game

Status: Ready

## Story

This story depends on [Generate the character mesh outside the editor](character-mesh-outside-the-editor.md), which is Ready. That story ships first. The developer then cooks a Windows Development package and starts that executable outside the Unreal Editor. It opens on the same map as Play In Editor. The world is the `WorldMap.txt` world. The captain, the companions, the ground, the fog curtains, the buildings, and the trees match that session.

## Acceptance

- `tools/package.bat` cooks the `SolidCore1` Win64 Development Game target. `UE_ROOT` and `PROJECT` override the engine and the uproject the same way they do in `tools/build.bat`.
- The cooked game starts on `/Game/ThirdPerson/Lvl_ThirdPerson` with `BP_SolidGameMode`. `L_OpenWorld` stays in Content and is not cooked. Editor content is skipped.
- The cooked game reads `Content/WorldMap.txt`. Terrain, towns, roads, the cave, and the Iglin tunnels come from that file. Play In Editor still reads the same file.
- The cooked asset registry contains the packs the game finds by name or folder: `/Game/Viking`, `/Game/Fab`, `/Game/Medieval_Assassin_Girl`, `/Game/Elf_Blacksmith`, and `/Game/Characters`. A missing source `.uasset` on disk still resolves through that registry.
- These loads succeed in the cooked game: `/Engine/BasicShapes/Cube`, `Cylinder`, `Cone`, and `Sphere`; `/Game/LevelPrototyping/Materials/M_FlatCol` (and `MI_DefaultColorway` when FlatCol is absent); `SK_Viking` and `Anim_Viking_idle1`, `Anim_Viking_walk`, `Anim_Viking_run`, `Anim_Viking_jump`; `Mat_025_grass`; the Assassin body `SKM_base_mesh_ue` and clips `anim_assassin_idle_2`, `anim_assassin_Walk`, `anim_assassin_run`; the Blacksmith body `SK_Elf_Blacksmith`.
- Half, full, and outer fog curtains are translucent unlit two-sided materials in the cooked game, at the same colors and opacities Play In Editor uses now: half `(0.55, 0.62, 0.68)` at `0.40`, full `(0.48, 0.54, 0.60)` at `0.70`, outer `(0.92, 0.94, 0.96)` at `0.82`. The parent is a cooked content material. The curtain is that material.
- Companions in the cooked game wear the generated low-poly body from that shipped story: the same seed, shirt, trousers, skin, and clips as Play In Editor. This story does not build that mesh. The Game target stays `TargetType.Game`.
- The cook stays Development with distribution off.
- Tests cover the cook list, the staged world map, and the fog parent. They run in the editor. The Windows machine runs `tools/package.bat`, and play of that executable is the check that the cooked game matches Play In Editor.

## Implementation notes

- `Source/SolidCore1.Target.cs` is already a Game target. `tools/build.bat` compiles `SolidCore1Editor` only. `Config/DefaultGame.ini` has pak and IoStore, `BuildConfiguration=PPBC_Development`, `ForDistribution=False`, `bCookAll=False`, `bSkipEditorContent=False`, and `MapsToCook` for both `Lvl_ThirdPerson` and `L_OpenWorld`. `GameDefaultMap` is `Lvl_ThirdPerson`.
- `USolidWorldMap::LoadDefault` reads `FPaths::ProjectContentDir() / WorldMap.txt` with `FPaths::FileExists`. A pak does not contain that loose text file. `DirectoriesToAlwaysStageAsUFS` takes directories under Content, and staging all of Content would copy source uassets into the pak. Stage this one file as UFS so the path `LoadDefault` already uses is inside the pak. Keep `Content/WorldMap.txt` where it is.
- Viking, grass, the Assassin, and the Blacksmith are found from `tools/fab-assets.json` and from `FPaths::FileExists` on the source `.uasset` (`SolidFabManifest::ContentFileExists`). A pak has neither the `tools` folder nor those source files. The fallback is an asset-registry search. Soft paths set in a C++ constructor are not cook dependencies, and `BP_SolidCharacter.uasset` does not store `SK_Viking`. Cook the discovery folders with `DirectoriesToAlwaysCook` (or an equivalent explicit list). `bCookAll` stays off: it still misses `/Engine/BasicShapes` and it cooks the comparison map. The machine that cooks already has the packs from Add to Project. This story does not commit them.
- Buildings, trees, the cave, the tower, furnishings, forage, nameplates, and companion eyes `LoadObject` engine BasicShapes. Terrain color and the fog fallback `LoadObject` `M_FlatCol`. Those string loads are cooked only when a cooked package references them, or when their folders are always cooked.
- `SolidTerrainFog::CreateProgrammaticTranslucentMist` is `WITH_EDITOR` and uses `GetEditorOnlyData` plus `PostEditChange`. A game build takes the opaque FlatCol fallback. Ship a project material, translucent, unlit, two-sided, with `MistColor` and `Opacity`, and instance it at runtime in every build. The cook has to include that asset. Leave `MaterialEditingLibrary` unused (SC1-0042). Curtain sizes and the trail rules stay as in [Source/SolidCore1/Fog/README.md](../Source/SolidCore1/Fog/README.md).
- The generated body is already in the Game target when this story starts. Leave that build in the shipped character-mesh story.
- Quit already leaves a game outside the editor (`SolidExit`). Input actions are created in code. Neither needs cook work.
- Add `tools/package.bat` to the tools list in `README.md` when the script lands. A cloud agent does not cook, and does not run the editor.
- [Load the world from the chart](load-the-world-from-the-chart.md) is Ready and does not block this story. While it is open, stage `Content/WorldMap.txt` as above. When that story ships first, follow `/Game/World/WorldChart` instead and drop the UFS note for the text file. [Remove WorldMap.txt](remove-world-map-txt.md) deletes the file after the chart is what the game loads.
