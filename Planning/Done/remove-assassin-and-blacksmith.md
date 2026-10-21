# Drop the Assassin and Blacksmith packs

Status: Done

## Story

The company no longer uses the Assassin or Blacksmith Fab packs. Sam, Alex, Max, Morgan, and Quinn stay in the company, with the same vocations and the same shirt, trouser, and skin colors. Each companion wears the generated body and the Viking clips. The captain stays on `SK_Viking`. Credits name Viking and the grass. A machine that cooks the game does not need either pack installed.

## Acceptance

- The company mesh list has no Assassin entry and no Blacksmith entry. Spawn does not look up `Medieval_Assassin_Girl` or `Elf_Blacksmith`.
- A companion's body is the generated low-poly body on the Viking skeleton. Idle, walk, and run are `Anim_Viking_idle1`, `Anim_Viking_walk`, and `Anim_Viking_run`.
- Sam is a Rogue, Alex a Warrior, Max a Cleric, Morgan a Mage, and Quinn a Warrior. Sam's shirt is red and his trousers are blue. Alex wears the swap. Skin stays the light pink.
- Product C++ under `Source/SolidCore1/`, outside `Tests/`, does not name `Medieval_Assassin_Girl`, `Elf_Blacksmith`, `SKM_base_mesh_ue`, `SK_Elf_Blacksmith`, or `anim_assassin_`.
- The credits page names the Viking and the grass listings. It does not name the Assassin or Blacksmith listings.
- `tools/fab-assets.json` lists Viking and grass. `tools/fab_doctor.bat` checks those two. `README.md`, `AGENTS.md`, and `AgentBuild.md` stop requiring the other two packs.
- `Planning/package-the-game.md` no longer cooks or loads those packs.
- Tests that only covered those pack paths are gone. The company tests still cover the five companions, their vocations, and their colors. The credits test still covers Viking and grass.

## Implementation notes

- Today `USolidCompany::InitializeDefaultCharacterMeshes` seeds Viking, Assassin, and Blacksmith. Sam, Morgan, and Quinn point at Assassin. Alex and Max point at Blacksmith. `ResolveFieldedMesh` in `SolidGameMode.cpp` calls `SolidContentLookup::FindSkeletalMeshByToken`. `ApplyBodyMesh` then builds from `SK_Viking` and replaces that mesh. `CacheLocomotionAnims` uses the Assassin clips only while the component mesh still sits in `Medieval_Assassin_Girl`. The Blacksmith pack has no clips.
- Remove the token lookup, the pack-folder helpers, and the Assassin clip names with the catalog entries. A symbol that only its own tests call is unused.
- Credits live in `SolidCredits`. `tools/fab_mesh_info.ps1` hard-codes the two body paths. `Content/` gitignore lines for those folders can stay, so a leftover local pack is not committed.
- Shipped stories in `Planning/Done/` stay as the record of what they shipped.
- Tests that assert the Assassin and Blacksmith catalog names, credits, or paths fail once those are gone. Change only those tests first, so they pass on the current code and on the intended code, and push that on its own. The product change comes after and does not edit those tests.
