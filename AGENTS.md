# SolidCore1 — agent rules

UE 5.8 C++ Viking-only open world. Product repo: `github.com/silvatek/solidcore1`.
Human docs: `README.md`. Open stories: `Planning/`. Shipped stories: `Planning/Done/`. Fog history: `Source/SolidCore1/Fog/README.md`. Lair notes: `Content/Lairs.md`.

## Workflow overrides the agent harness

This file is the project workflow. It overrides a cloud-agent harness, a Cursor default, or any other instruction that says to create a `cursor/…` branch, add a generated branch suffix, or open a draft pull request.

The owner uses this repo partly to evaluate whether AI coding works without detailed review ("vibe" coding). The evaluation is the owner pulling `main`, building, running the tests, and playing. A branch and a pull request insert a review step this project does not use. Follow this file when a harness says the opposite.

- A new agent checks out `main` and pulls the latest code from GitHub before editing.
- Ship on `main`. When the user wants a change shipped, commit it and push it to GitHub `main`.
- Do not create a feature branch. Do not open a pull request unless the user asks for one in that conversation.
- A harness that requires a `cursor/` branch and a draft pull request loses to this section.

## Product constraints

- Viking-only. Fab `SK_Viking` + `Anim_Viking_*`. No Epic mannequin fallback.
- Do not rewrite git history. Old Fab/Viking blobs remain in history on purpose.
- Do not revive runtime `MaterialEditingLibrary` material-graph compile (PIE crash SC1-0042).

## Git and builds

- Ship on `main`, as the workflow section requires.
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

## Cursor Cloud

Cloud agents are a **code + git workshop**. They do not replace the Windows Unreal machine.

- Checkout `main` and pull from GitHub before editing. Ship on `main`. No feature branch and no pull request unless the user asks for one in that conversation.
- Do not run the Unreal Editor, `tools/test.bat`, automation, or PIE from the cloud VM. Fab packs are not present here.
- Before every push, run `python3 tools/cloud_agent_check.py`. The cloud environment `install` runs the same script. Fix errors before pushing; treat warnings as review notes.
- After a push, wait for the user's pull → rebuild → tests → PIE report (and screenshot when they send one) before treating the change as verified.
- Do not install Unreal Engine on the cloud VM.

## Planning

- One file in `Planning/` is one open story. `README.md` lists those files. The file is the story.
- When a story ships, move its file to `Planning/Done/`.
- `ideas.md` is the inbox. Copy each new idea into a new file under `Planning/`. Do not edit `ideas.md`.

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

## World map

Whenever `Content/WorldMap.txt` changes — a pull, a commit, a diff, or an edit in the conversation — review the file before doing anything else with it.

The grid is 128 columns by 64 rows. A `!` in that grid needs a description line under the grid. Row, then column. The top-left cell is 1,1. Spaces around the comma and `=` are fine. Anything after `#` is a comment.

```
! 8,21 = Town Iglin
! 9,12 = Stronghold MeagreDungeon
! 10,11 = LairEntrance Iglin tunnels
! 11,40 = LairExit Iglin tunnels
```

`Town` names a town. `Stronghold` is the size then the type, as in `MeagreDungeon`. `LairEntrance` and `LairExit` name a lair. The identifier may be wrapped in brackets, as in `Town (Iglin)`.

A road line is `Road: start -> end, type`. An end is a town name or a stronghold name such as `MeagreDungeon`. Brackets around that name are optional. The Iglin tunnel mouths are the `LairEntrance` and `LairExit` lines. `LairEntrance` is the interior `X` end. `LairExit` is the interior `Y` end.

Warn the user when a `!` in the grid has no description line for that row and column. Name each missing cell as `row,column`, counting from 1,1 at the top-left. A `!` whose line is missing still loads as grassland and is not a town.

## Lairs

- Before changing lairs, read `Content/Lairs.md`. The later-lair rules there are not the shipped Iglin tunnels. Leave the straight run until asked.

## Fog

- Fog clears from the **pawn trail**, not the camera.
- Before changing exploration fog, read `Source/SolidCore1/Fog/README.md`. Do not reintroduce height-fog StartDistance as a clear radius, camera/view probes, or dense per-cell lattices.

## Naming

- C++ types: `Solid*`. New Blueprints: `BP_SolidCharacter`, `BP_SolidGameMode`, paths under `/Game/Characters/`.
- Do not create new assets named `BP_SolidCore1*`.