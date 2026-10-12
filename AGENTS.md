# SolidCore1 — agent rules

UE 5.8 C++ Viking-only open world. Product repo: `github.com/silvatek/solidcore1`.
Human docs and the To-do list: `README.md`. Fog history: `Source/SolidCore1/Fog/README.md`.

## Product constraints

- Viking-only. Fab `SK_Viking` + `Anim_Viking_*`. No Epic mannequin fallback.
- Work on `main`. Do not open a pull request unless the user asks.
- Do not rewrite git history. Old Fab/Viking blobs remain in history on purpose.
- Do not revive runtime `MaterialEditingLibrary` material-graph compile (PIE crash SC1-0042).

## Git and builds

- Commit and push to GitHub `main` when the user wants the change shipped.
- On every product push, bump `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` in `Source/SolidCore1/Game/SolidBuildId.h` (`SC1-NNNN`).

## Collaborative workflow

The agent pushes to GitHub. The user verifies on their machine. Do not run the editor, automation, or PIE from the agent environment.

After a push, the user:

1. Pulls the changes.
2. Rebuilds the code.
3. Runs the tests.
4. Runs the game in PIE.
5. Typically, but not always, sends a screenshot for the agent to review.

Wait for that report before treating the change as verified. When a screenshot arrives, review it.

Screenshots are taken with the Windows snipping tool. That switch puts Unreal in the background, so the HUD frame rate drops. Ignore FPS in a screenshot unless the user says they are worried about it. Foreground PIE is about 120 FPS.

## Tests

Follow the Automated tests policy in `README.md`:

- Existing tests must keep passing. Do not edit them to make a change pass unless the API or behavior change requires it — and say so in the commit.
- If a product change would invalidate a test assumption, first change only the tests so they pass on the current code and on the intended code. Use a product constant instead of a copied number. Push that on its own. The product change comes after, and it does not edit those tests.
- New product code ships with tests in the same commit.
- Never pass `TObjectPtr<T>` (or a ternary that yields one) to `TestNotNull` / `TestNull`. Bind a raw `T*` or call `.Get()`.

## Fab content

- Packs under `Content/Fab` and `Content/Viking` are gitignored. Restore only via Epic Launcher **Add to Project**. See `README.md` and `tools/fab-assets.json`.
- C++ finds Viking/grass by **asset name** under `/Game/Viking` then `/Game/Fab`. The Assassin body is `Meshes/base_mesh/SKM_base_mesh_ue.uasset` inside `Medieval_Assassin_Girl` (`SK_Mannequin.uasset` is the skeleton). The Blacksmith body is `Mesh/SK_Elf_Blacksmith.uasset` inside `Elf_Blacksmith` (`SK_Elf_Blacksmith_Skeleton.uasset` is the skeleton; `SeparatedMesh` parts are not the body). `resolvedPath` / `resolvedFolder` / `resolvedPaths` in `tools/fab-assets.json` are tried first when the recorded file is that body; a different recorded file is replaced when the body is found. Do not relocate folders after Add to Project.
- Assassin companions play `anim_assassin_idle_2`, `anim_assassin_Walk`, and `anim_assassin_run` from `Medieval_Assassin_Girl`. `anim_assassin_idle_1` is a crouch. The Blacksmith pack has no locomotion clips, so those companions stay on Viking clips. `tools/fab_mesh_info.bat <folder>` lists meshes and animations in a downloaded pack and does not edit the manifest.
- Grass is required (`Mat_025_grass`).

## Fog

- Fog clears from the **pawn trail**, not the camera.
- Before changing exploration fog, read `Source/SolidCore1/Fog/README.md`. Do not reintroduce height-fog StartDistance as a clear radius, camera/view probes, or dense per-cell lattices.

## Naming

- C++ types: `Solid*`. New Blueprints: `BP_SolidCharacter`, `BP_SolidGameMode`, paths under `/Game/Characters/`.
- Do not create new assets named `BP_SolidCore1*`.