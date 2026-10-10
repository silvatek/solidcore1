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
- On every product push, bump `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` in `Source/SolidCore1/SolidBuildId.h` (`SC1-NNNN`).

## Collaborative workflow

The agent pushes to GitHub. The user verifies on their machine. Do not run the editor, automation, or PIE from the agent environment.

After a push, the user:

1. Pulls the changes.
2. Rebuilds the code.
3. Runs the tests.
4. Runs the game in PIE.
5. Typically, but not always, sends a screenshot for the agent to review.

Wait for that report before treating the change as verified. When a screenshot arrives, review it.

## Tests

Follow the Automated tests policy in `README.md`:

- Existing tests must keep passing. Do not edit them to make a change pass unless the API or behavior change requires it — and say so in the commit.
- New product code ships with tests in the same commit.
- Never pass `TObjectPtr<T>` (or a ternary that yields one) to `TestNotNull` / `TestNull`. Bind a raw `T*` or call `.Get()`.

## Fab content

- Packs under `Content/Fab` and `Content/Viking` are gitignored. Restore only via Epic Launcher **Add to Project**. See `README.md` and `tools/fab-assets.json`.
- C++ finds Viking/grass by **asset name** under `/Game/Viking` then `/Game/Fab`. Do not relocate folders after Add to Project.
- Grass is required (`Mat_025_grass`).

## Fog

- Fog clears from the **pawn trail**, not the camera.
- Before changing exploration fog, read `Source/SolidCore1/Fog/README.md`. Do not reintroduce height-fog StartDistance as a clear radius, camera/view probes, or dense per-cell lattices.

## Naming

- C++ types: `Solid*`. New Blueprints: `BP_SolidCharacter`, `BP_SolidGameMode`, paths under `/Game/Characters/`.
- Do not create new assets named `BP_SolidCore1*`.