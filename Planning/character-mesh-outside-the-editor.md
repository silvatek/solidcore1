# Generate the character mesh outside the editor

Status: Ready

## Story

The low-poly body is built in a Game target, the same way Play In Editor builds it now. Companions in the world wear it. The character editor shows it on the plain stage, and height, build, and gender change that mesh. The same build can make a body for the captain. A later story is the screen where the player personalises the captain at the start of a game. This story does not add that screen. Until then the captain in the world stays on `SK_Viking`.

## Acceptance

- `SolidBodyMesh::Build` runs in a Game target and returns a skeletal mesh with a render LOD. The same seed and the same height, build, and gender produce the same body Play In Editor wears now: shirt, trousers, and skin slots, on the Viking skeleton, so `Anim_Viking_idle1`, `Anim_Viking_walk`, and `Anim_Viking_run` still play.
- `ApplySeed` wears that mesh. A build that can be drawn does not leave the previous mesh in place.
- The Game target stays `TargetType.Game`.
- The character editor still opens from the menu onto one companion. Page Up and Page Down cycle Sam, Alex, Max, Morgan, and Quinn. Up and Down select height, build, or gender. Left and Right change the selected value and the body updates. Space switches between the body and the bone lines. Home and End turn the figure. The pointer names the bone under it. Esc and F10 close the editor.
- `Build` can make that body for a captain mesh as well as a companion. The world captain still wears `SK_Viking`. There is no new-game personalisation screen.
- An editor test builds a body through `Build`, checks that it has a render LOD, the three slots, and the Viking bone names. `Build` is not wrapped in `WITH_EDITORONLY_DATA`.

## Implementation notes

- SC1-0226 (`02949d5`) built the body with `GetMeshDescription`, `CommitMeshDescription`, and `USkeletalMesh::Build`. Those write the skeletal mesh source model, which a Game target does not compile. The `#else` returns null, and `ApplySeed` logs that the body could not be built and keeps the mesh already on the component. The commit presents the body as what companions wear. It does not say the body is editor-only.
- `FSolidBodyMesh::Draw` already emits the triangles in a game build. A cooked game wears a mesh by filling `FSkeletalMeshRenderData` (positions, indices, skin weights, sections), which is the runtime half of a skeletal mesh. One path for the editor and the Game target, so the two builds match. The Viking skeleton asset stays unchanged. The same proportions can still share one transient mesh.
- The character editor is `ASolidCharacterStage` and the companion preview. It calls `RebuildBody`, which calls `ApplySeed`. It has no mesh builder of its own.
- [package-the-game.md](package-the-game.md) already says a cooked companion wears this body. That bullet is this story. The opening screen that personalises the captain is a later story.
