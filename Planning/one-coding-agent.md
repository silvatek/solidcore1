# One coding agent

Status: New

## Story

Several coding agents write on `build/<name>` and stop. The builder on the Windows checkout lands one branch, builds, and runs the tests. One agent on that checkout, writing the change and running the build and the tests itself, is the path from here.

That agent is the solidcore1-ue worker. It edits the tree the developer plays. A pass pushes `main`. The developer plays that checkout. The developer does not pull.

`tools\land.bat` and `tools/buildable.py` stay, with their tests. They are the queue for several coding agents. New product work leaves that queue unused. The scripts stay so the queue can be started again.

`ue.lock` stays in daily use. Play In Editor, the World Editor, and the chart dump still hold the checkout. The one agent waits for `Free` before it builds or tests.

No game concept. This is the developer's build, not a player action.

## Acceptance

New product work reaches `main` from the one coding agent on the Windows checkout. That agent has built and tested it.

- The agent runs on this checkout. `tools\agent.bat` still starts the worker and still writes `agent.ok`. The agent reads that marker before `tools\build.bat` and `tools\test.bat`. A missing marker skips both.
- A product change is built with `tools\build.bat` and tested with `tools\test.bat SolidCore1`. The push to `main` follows a pass. A failure is fixed on this checkout before the push. The command output is the result. The agent does not post a `solidcore1/land` status for its own work.
- The shipping commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` once. The note is the first line of that commit. A story or a process document with no product change does not bump them. A cloud agent does not edit `Source/SolidCore1/Game/SolidBuildId.h`.
- While `ue.lock` says `PIE`, `Editor`, or `Dump`, the agent does not build or test. `tools\yield-ue.bat` still reserves the next build and test. `tools\free-ue.bat` still clears a dead agent id, and it still leaves `PIE`, `Editor`, and `Dump` in place. The developer starts play with `tools\pie.bat` and the World Editor with `tools\editor.bat`. The developer does not pull.
- The story stays `In Progress` across the push. `Done` waits for the developer's play, when the story changes play, and for agreement. A test pass does not set `Done`.
- `AGENTS.md`, `AgentBuild.md`, `Planning/README.md`, `CodingGuidelines.md`, and `README.md` say this path. `AgentBuild.md` is the Windows agent's guide. A cloud agent is told to leave product work on this checkout. A cloud agent may still push a story or a process document that stands on `main` and needs no Unreal. That push does not ask for a build.
- The queue scripts are not edited: `tools/land.bat`, `tools/land.py`, `tools/land_test.py`, `tools/buildable.py`, `tools/buildable_test.py`. The lock scripts are not edited: `tools/ue_lock.py`, `tools/ue_lock_test.py`, `tools/yield-ue.bat`, `tools/free-ue.bat`, `tools/build.bat`, `tools/test.bat`, `tools/pie.bat`, `tools/editor.bat`, `tools/dump-chart.bat`, `tools/agent.bat`. `python -m unittest tools/land_test.py tools/buildable_test.py tools/ue_lock_test.py` still passes.
- `tools/cloud_agent_check.py` accepts `main` and a `build/` branch. It fails any other branch. It does not ask for a pull request. The failure text leaves product work to the Windows checkout. It does not send that work to `build/<name>`.
- The docs name the queue as the way back: `Buildable: yes` on the tip, `python tools/buildable.py` to list, `tools\land.bat` to land the oldest waiting tip, the `solidcore1/land` status, and the comment on the pushed commit. When a poll finds nothing waiting, the chat still repeats the latest build. A `build/<name>` tip that is already waiting stays waiting. This story does not land it and does not delete the branch.
- This story adds no game code. The commit that adds this file does not bump `SOLID_BUILD_ID`.

## Implementation notes

- `Planning/Done/builder-lands-the-branch.md` records why the queue was introduced. It stays a record. It does not instruct later work. The living rules are `AGENTS.md` and `AgentBuild.md`.
- Sentences that send product work to `build/<name>`, that tell a coding agent to stop for the builder, and that say a coding agent does not edit `SOLID_BUILD_ID`, are the ones this path replaces. They sit in the workflow, Git, solidcore1-ue, and Cursor Cloud sections of `AGENTS.md`, in `AgentBuild.md`, in the ship bullet of `Planning/README.md`, in the automated-tests policy of `CodingGuidelines.md`, and in the Build ID paragraph of `README.md`.
- `tools/cloud_agent_check.py` requires the workflow section to contain `cursor/<words>-<digits>`, `does not apply to branches or pull requests`, and the phrase that accepts `main` and a `build/` branch. Those phrases stay. `ship_branch_error` is the failure text that currently tells a cloud agent to move Unreal work onto `build/<name>`. That sentence changes. The accept list does not.
- The Windows agent passes its id to `tools\build.bat`. The same id is the re-entry `tools\test.bat` already allows. The agent releases that id with `tools\free-ue.bat` when the session ends, including when the build or the tests fail. It does not delete `ue.yield`. It does not write `ue.lock` itself.
- The build note is the first line of the product commit, at most the length `tools/land.py` already keeps. The id is the next `SC1-NNNN`.
- The early process, a cloud push followed by a pull on this machine, stays retired. The agent is already on the checkout the developer plays.
- The 3-minute pause stays. Play notes stay a separate message from the test result.
