# The builder lands the branch

Status: In Progress

## Story

A coding agent writes one story on a branch and pushes that branch. It stops. The builder on the Windows machine lands that branch on `main` when `ue.lock` is Free, builds, and runs the tests. A pass names the build id, and the coding agent continues the story. A fail names the failure, and the coding agent updates the same branch.

The developer plays and edits the chart on that same checkout. The developer does not pull.

Several coding agents can have a branch waiting. The builder lands one branch at a time. Starting Play In Editor or the World Editor holds the checkout until that session exits.

A story that is already In Progress on `main` stays on `main` until it is Done. This story does not move it onto a branch.

No game concept. This is the developer's build, not a player action.

## Stages

Agree this order before a stage is planned in detail. A stage ships before the next stage is planned. Through every stage, Play In Editor and the World Editor keep their present words, an In Progress story on `main` can still push `main`, and `tools/cloud_agent_check.py` still accepts `main`.

1. **The lock gates the build.** `tools\build.bat` and `tools\test.bat` take `ue.lock` before they run, and they refuse it while the word is `PIE`, `Editor`, or `Dump`. Two agents cannot both pass a Free lock. The developer can reserve the next slot, and can clear a dead agent id without clearing a live `PIE` or `Editor` session. Agents still push `main`. No branch is introduced.
2. **One branch can be landed.** The builder can land one `build/<name>` branch when its tip is buildable. A line of that commit message is `Buildable: yes`. It fetches during `PIE` or `Editor`. It rebases, builds, and tests only when the word is Free, then leaves `main` checked out and the lock Free. The landing bumps `SOLID_BUILD_ID` once. The builder posts the build and test result to GitHub on the commit that coding agent pushed. The coding agent reads that. The developer does not relay it. No story is required to use the branch. The check accepts `main` and a `build/` branch, and it rejects a `cursor/` branch and a pull request.
3. **One new story uses it.** One story that is not already In Progress goes through a coding agent and the builder for every product landing. The coding agent does not push `main` and does not edit `SolidBuildId.h`. Each pass continues that story. Each fail returns to the same branch. Every other open story still ships on `main`.
4. **The queue.** More than one coding agent can have a branch waiting. The builder lands the oldest ready branch, one at a time. A `PIE` or `Editor` session holds the checkout. The coding agents keep writing. The Windows checkout has one agent in it at a time, and it is on `main` whenever the lock is Free.
5. **New product work uses the builder.** A coding agent that starts a story which is not already In Progress on `main` uses the branch. Stories that were already In Progress may finish on `main`. The lines that tell the developer to pull after a cloud push are replaced for this machine: the builder has already updated the checkout.

## Acceptance

New product work reaches `main` through the builder. The developer plays that checkout without pulling. Stories already In Progress on `main` are left to finish there.

- A coding agent on a new story pushes one branch named for that story. It does not push `main`. It does not change `SOLID_BUILD_ID` or `SOLID_BUILD_NOTE`.
- The builder lands that branch on the Windows checkout only while `ue.lock` is Free. It builds, runs the `SolidCore1` tests, bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` once for that landing, pushes `main`, and leaves the checkout on `main` with the lock Free.
- While the word is `PIE`, `Editor`, or `Dump`, the builder does not rebase, build, or test. It may fetch.
- A pass report is a short comment on the commit the coding agent pushed. It names the build id that landed. The coding agent continues the story from that `main`, pushes the same branch, and stops for the builder again.
- A fail report is a short comment on that same commit. It names the rebase conflict or the failed tests. The coding agent updates the same branch and stops for the builder again.
- The developer does not copy those results into chat. Play notes are the developer's own message.
- The coding agent stops for a decision when the story's acceptance criteria are implemented, or when it needs one. The story stays `In Progress` across landings. `Done` still waits for the developer's play and an agreement.
- More than one coding agent can have a branch waiting. The builder lands one branch at a time.
- The developer starts Play In Editor with `tools\pie.bat` and the World Editor with `tools\editor.bat` on that checkout. Neither session pulls.
- A story that was `In Progress` on `main` when this switch started can still ship on `main` until it is Done.
- `tools\build.bat` and `tools\test.bat` do not run while the developer holds the lock.
- `tools/cloud_agent_check.py` accepts `main` and a `build/` branch. It fails a `cursor/` branch and any other name. It does not ask for a pull request.
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
- The builder posts one pass or fail mark on the commit the coding agent pushed, and a short comment on that commit. The mark ends the coding agent's wait. The comment is the report it reads: the build id when `main` moved, or the rebase conflict, or the failed test names. The automation log stays on the Windows machine. The comment does not include play. Two coding agents do not share one report file.
- The branch is `build/<name>`. The buildable marker is a line `Buildable: yes` on the tip commit. The mark is a GitHub commit status, context `solidcore1/land`, on the commit that was pushed. The comment on that commit is the report.

### Stage 1 — the lock gates the build

Stage 1 is complete. The Windows checkout ran the helper tests: 18 passed. With the next slot reserved, `tools\build.bat` printed `The next slot is reserved.` and did not compile. The lock was left `Free`. Coding agents were stopped after `SC1-0612`. No branch was part of this stage. Agents still push `main`. `SOLID_BUILD_ID` did not change. Play In Editor, the World Editor, and the chart dump still write `PIE`, `Editor`, and `Dump`, and they still start only from a missing `ue.lock` or the word `Free`.

`tools/ue_lock.py` is the only writer of the lock word. The bats call it. A direct echo into `ue.lock` is gone, including the copies in `tools\pie.bat`, `tools\editor.bat`, and `tools\dump-chart.bat`. `tools/cloud_agent_check.py` is updated in the same change so it requires that call before the editor launches, and before Unreal Build Tool or `UnrealEditor-Cmd` starts. It still requires the words `PIE`, `Editor`, and `Free`, a missing lock counting as free, and no pull request.

The helper takes the lock with a gate directory, `ue.lock.gate`, created by `mkdir`. That create succeeds for one waiter. The winner reads `ue.lock`. A missing file is free. An empty file is busy. Surrounding whitespace is ignored. When the word is `Free`, the winner writes its own word and removes the gate. Any other word removes the gate and fails, and the file is unchanged. A second waiter then sees the first word. The write replaces the file in one step. If the gate is left behind, a waiter may remove it once it is 30 seconds old and try once more. Younger than that, the waiter fails and prints the current word.

`tools\build.bat` and `tools\test.bat` take the lock before they run, and they do not write `Free` when they finish. The holder word is `SOLID_AGENT_ID`. `tools\build.bat` also accepts that id as its first argument. `tools\test.bat` keeps its first argument as the automation filter, so `tools\test.bat SolidCore1.Fog` still runs that filter. An agent sets the same id for both. The same id is a re-entry, so the test runs under the hold the build already took. If the argument and the variable are both unset, build uses the word `Build` and test uses the word `Test`. Those two words, and `Free`, `PIE`, `Editor`, and `Dump`, are refused as an agent id. A failed build leaves the id in the lock. The agent releases it after the session, including when the build or the tests fail.

`tools\free-ue.bat <id>` writes `Free` only when the word is that id. `tools\free-ue.bat` with no id is the dead holder. It refuses a missing file, an empty file, and the words `Free`, `PIE`, `Editor`, and `Dump`. For any other word it refuses while `UnrealEditor.exe`, `UnrealEditor-Cmd.exe`, or `UnrealBuildTool.exe` is running. When none of those is running, it writes `Free`. A `PIE` or `Editor` word left by a killed bat is still cleared by hand, as it is today.

`ue.yield` is the reservation. It is gitignored, like `ue.lock` and `ue.lock.gate`. `tools\yield-ue.bat` creates it. `tools\yield-ue.bat clear` deletes it. `tools\build.bat` and `tools\test.bat` see it first, print that the next slot is reserved, and exit. They do not change the lock, and they do not stop a build or a test that has already started. A re-entry is refused too, so a yield set between the build and the test stops the test. The agent then runs `tools\free-ue.bat <id>` if the word is still its id, and waits. `tools\pie.bat`, `tools\editor.bat`, and `tools\dump-chart.bat` ignore `ue.yield`.

The solidcore1-ue instructions stop telling the agent to write the lock file. The agent passes its run id to `tools\build.bat` and `tools\test.bat`. If the script prints a holder word, the agent says that word and waits, as it does today. If the script says the next slot is reserved, the agent says that, releases its own id, and waits the same way. It does not delete `ue.yield`.

Tests for the helper run under `python -m unittest` with no Unreal. They cover a missing lock, `Free`, two different ids, re-entry of the same id, a session tool that does not re-enter, the reserved words, a yield that blocks build and test only, release of a matching id, a dead clear that refuses `PIE` and `Editor`, and a dead clear that frees an agent id when the process check reports none. The process check is injected. Two overlapping acquires leave one winner. The stage is implemented when those tests pass and the bats call the helper. Play is unchanged, so this stage does not wait on a PIE report.
