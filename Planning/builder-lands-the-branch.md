# The builder lands the branch

Status: New

## Story

A coding agent writes one story on a branch and pushes that branch. It stops. The builder on the Windows machine lands that branch on `main` when `ue.lock` is Free, builds, and runs the tests. A pass names the build id, and the coding agent continues the story. A fail names the failure, and the coding agent updates the same branch.

The developer plays and edits the chart on that same checkout. The developer does not pull.

Several coding agents can have a branch waiting. The builder lands one branch at a time. Starting Play In Editor or the World Editor holds the checkout until that session exits.

A story that is already In Progress on `main` stays on `main` until it is Done. This story does not move it onto a branch.

No game concept. This is the developer's build, not a player action.

## Stages

Agree this order before a stage is planned in detail. A stage ships before the next stage is planned. Through every stage, Play In Editor and the World Editor keep their present words, an In Progress story on `main` can still push `main`, and `tools/cloud_agent_check.py` still accepts `main`.

1. **The lock gates the build.** `tools\build.bat` and `tools\test.bat` take `ue.lock` before they run, and they refuse it while the word is `PIE`, `Editor`, or `Dump`. Two agents cannot both pass a Free lock. The developer can reserve the next slot, and can clear a dead agent id without clearing a live `PIE` or `Editor` session. Agents still push `main`. No branch is introduced.
2. **One branch can be landed.** The builder can land one named branch when asked. It fetches during `PIE` or `Editor`. It rebases, builds, and tests only when the word is Free, then leaves `main` checked out and the lock Free. The landing bumps `SOLID_BUILD_ID` once and reports pass or fail to that coding agent. No story is required to use the branch. The check accepts that branch pattern and still rejects a `cursor/` branch and a pull request.
3. **One new story uses it.** One story that is not already In Progress goes through a coding agent and the builder for every product landing. The coding agent does not push `main` and does not edit `SolidBuildId.h`. Each pass continues that story. Each fail returns to the same branch. Every other open story still ships on `main`.
4. **The queue.** More than one coding agent can have a branch waiting. The builder lands the oldest ready branch, one at a time. A `PIE` or `Editor` session holds the checkout. The coding agents keep writing. The Windows checkout has one agent in it at a time, and it is on `main` whenever the lock is Free.
5. **New product work uses the builder.** A coding agent that starts a story which is not already In Progress on `main` uses the branch. Stories that were already In Progress may finish on `main`. The lines that tell the developer to pull after a cloud push are replaced for this machine: the builder has already updated the checkout.

## Acceptance

New product work reaches `main` through the builder. The developer plays that checkout without pulling. Stories already In Progress on `main` are left to finish there.

- A coding agent on a new story pushes one branch named for that story. It does not push `main`. It does not change `SOLID_BUILD_ID` or `SOLID_BUILD_NOTE`.
- The builder lands that branch on the Windows checkout only while `ue.lock` is Free. It builds, runs the `SolidCore1` tests, bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` once for that landing, pushes `main`, and leaves the checkout on `main` with the lock Free.
- While the word is `PIE`, `Editor`, or `Dump`, the builder does not rebase, build, or test. It may fetch.
- A pass report names the build id that landed. The coding agent continues the story from that `main`, pushes the same branch, and stops for the builder again.
- A fail report names the rebase conflict or the failed tests. The coding agent updates the same branch and stops for the builder again.
- The coding agent stops for a decision when the story's acceptance criteria are implemented, or when it needs one. The story stays `In Progress` across landings. `Done` still waits for the developer's play and an agreement.
- More than one coding agent can have a branch waiting. The builder lands one branch at a time.
- The developer starts Play In Editor with `tools\pie.bat` and the World Editor with `tools\editor.bat` on that checkout. Neither session pulls.
- A story that was `In Progress` on `main` when this switch started can still ship on `main` until it is Done.
- `tools\build.bat` and `tools\test.bat` do not run while the developer holds the lock.
- `tools/cloud_agent_check.py` accepts `main` and the story branch. It fails a `cursor/` branch. It does not ask for a pull request.
- The plan commits do not bump `SOLID_BUILD_ID`. This story ships no game code.

## Implementation notes

- The pass report is the coding agent's cue to write the next slice. It is not a cue to set `Done`, and it is not a cue to wait for play. Play notes are a separate message. They start whatever change the developer asked for.
- The branch lives for the story. The builder does not delete it after a landing. The next slice is more commits on that branch, based on the `main` that just landed.
- The builder is the only agent that writes the Windows checkout. A coding agent runs on the cloud VM. It does not run `tools\build.bat`, `tools\test.bat`, the editor, or PIE.
- The builder reads `agent.ok` before it builds, as a solidcore1-ue agent does today. A missing marker skips the build and the tests, and the report says so.
- `tools\pie.bat`, `tools\editor.bat`, and `tools\dump-chart.bat` stay the writers of `PIE`, `Editor`, and `Dump`. They still start only from a missing file or the word `Free`.
- Reserving the next slot stops the builder before its next rebase, build, test, or push. It does not kill a build or a test that has already started.
- Clearing a dead holder applies to an agent id whose build or test is gone. It does not apply to `PIE` or `Editor`. Those words clear when the editor process exits, as they do now.
- A chart change rides in on a branch and reaches the checkout only while the lock is Free. An open World Editor session is not the tree the builder writes.
- One story uses one path. A story still shipping on `main` bumps `SOLID_BUILD_ID` itself, as today. The builder bumps it for a branch it lands. The two do not edit that header in the same landing.
- The Windows worker runs one assignment in this checkout at a time. A second build does not share the tree.
- The exact branch prefix, the lock acquire, the yield file, and the way the pass or fail report reaches the coding conversation are planned with the stage that introduces them.
