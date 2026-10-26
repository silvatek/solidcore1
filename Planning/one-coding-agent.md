# One coding agent

Status: New

## Story

One coding agent runs on the Windows checkout. It writes the product change, builds it, and runs the tests. A pass pushes `main`. The agent is the solidcore1-ue worker. It edits the tree the developer plays. The developer plays that checkout. The developer does not pull.

`tools\land.bat` and `tools/buildable.py` stay, with their tests. New product work does not use that queue. The scripts stay so several coding agents can be used again.

`ue.lock` stays in daily use. Play In Editor, the World Editor, and the chart dump still hold the checkout. The agent waits for `Free` before it builds or tests.

No game concept. This is the developer's build, not a player action.

## Acceptance

New product work reaches `main` from the one coding agent on the Windows checkout. That agent has built and tested it.

- The agent runs on this checkout. `tools\agent.bat` still starts the worker and still writes `agent.ok`. The agent reads that marker before `tools\build.bat` and `tools\test.bat`. A missing marker skips both.
- A product change is built with `tools\build.bat` and tested with `tools\test.bat SolidCore1`. The push to `main` follows a pass. A failure is fixed on this checkout before the push. The command output is the result. The agent does not post a `solidcore1/land` status for its own work.
- The shipping commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` once. The note is the first line of that commit. A story or a process document with no product change does not bump them. A cloud agent does not edit `Source/SolidCore1/Game/SolidBuildId.h`.
- While `ue.lock` says `PIE`, `Editor`, or `Dump`, the agent does not build or test. `tools\yield-ue.bat` still reserves the next build and test. `tools\free-ue.bat` still clears a dead agent id, and it still leaves `PIE`, `Editor`, and `Dump` in place. The developer starts play with `tools\pie.bat` and the World Editor with `tools\editor.bat`. The developer does not pull.
- The story stays `In Progress` across the push. `Done` waits for the developer's play, when the story changes play, and for agreement. A test pass does not set `Done`.
- `AGENTS.md`, `AgentBuild.md`, `Planning/README.md`, `CodingGuidelines.md`, and `README.md` say this path. `AgentBuild.md` is the Windows agent's guide. A cloud agent is told to leave product work on this checkout. A cloud agent may still push a story or a process document that stands on `main` and needs no Unreal. That push does not ask for a build.
- The passages under Invalidated instructions are updated. The passages under Still in force are left.
- The queue scripts are not edited: `tools/land.bat`, `tools/land.py`, `tools/land_test.py`, `tools/buildable.py`, `tools/buildable_test.py`. The lock scripts are not edited: `tools/ue_lock.py`, `tools/ue_lock_test.py`, `tools/yield-ue.bat`, `tools/free-ue.bat`, `tools/build.bat`, `tools/test.bat`, `tools/pie.bat`, `tools/editor.bat`, `tools/dump-chart.bat`, `tools/agent.bat`. `python -m unittest tools/land_test.py tools/buildable_test.py tools/ue_lock_test.py` still passes.
- `tools/cloud_agent_check.py` accepts `main` and a `build/` branch. It fails any other branch. It does not ask for a pull request. The failure text leaves product work to the Windows checkout. It does not send that work to `build/<name>`.
- The docs name the queue as the way back: `Buildable: yes` on the tip, `python tools/buildable.py` to list, `tools\land.bat` to land the oldest waiting tip, the `solidcore1/land` status, and the comment on the pushed commit. When a poll finds nothing waiting, the chat still repeats the latest build. A `build/<name>` tip that is already waiting stays waiting. This story does not land it and does not delete the branch.
- This story adds no game code. The commit that adds this file does not bump `SOLID_BUILD_ID`.

## Implementation notes

### Invalidated instructions

These passages tell an agent that product work is a `build/<name>` branch, that a separate builder lands it, or that the coding agent stops before the build. This story updates them.

`AGENTS.md`:

- Workflow. "The builder has already updated it."
- Workflow. The bullet that a coding agent fetches `main`, pushes `build/<name>`, and does not push that work to `main` or edit `SOLID_BUILD_ID` or `SOLID_BUILD_NOTE`. The queue mechanics in that bullet move to the way-back description: `python tools/buildable.py`, `tools\land.bat`, `Buildable: yes`, the oldest committer time, an equal time using the branch name.
- Workflow. The bullet that a story or process document on product commits the builder has not landed stays on `build/<name>`, with `Buildable: yes`, and the builder lands the document with the code.
- Workflow. The check bullet that a change needing Unreal moves onto `build/<name>`, and that a document on unlanded product work stays there with `Buildable: yes`.
- Workflow. The paragraph that work needing Unreal is pushed as `build/<name>` and the builder lands it. `Planning/Done/builder-lands-the-branch.md` stays named as the record of that rollout.
- Git. The bullet that work needing Unreal is pushed as `build/<name>` and the builder lands that branch.
- Git. The bullet that a `Plan:` commit stays on `build/<name>` while product commits are unlanded, with `Buildable: yes`.
- Git. The bullet that the builder bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` when it lands a tip, that a coding agent does not edit them, and that a commit on `main` does not bump them.
- Chat. "After a landing, the commit on `main` is the copy, and this checkout is that commit." The copy is the Windows agent's push.
- Cloud agents. The paragraph that work needing Unreal is pushed as `build/<name>`, that the builder is the only agent that writes this checkout, and that a pass or a fail is a comment plus a `solidcore1/land` status which the coding agent reads before the next slice.
- solidcore1-ue agents. The opening that the builder writes this checkout with `tools\land.bat`, and that a coding agent does not edit it.
- solidcore1-ue agents. The sentence that the builder runs `tools\build.bat` and `tools\test.bat` from `tools\land.bat`, and that a coding agent does not run them.
- solidcore1-ue agents. The sentence that the builder reads `agent.ok`, and that the landing writes `Free` when `ue.lock` still holds its id. The Windows agent reads the marker and releases the id.
- solidcore1-ue agents. "The builder's test pass keeps the story `In Progress`." The test pass still keeps it `In Progress`. The actor is the Windows agent.
- Cursor Cloud. The bullet that work needing Unreal is pushed as `build/<name>` and that `main` is merged in before `Buildable: yes`. The same bullet's rule against another feature branch and a pull request stays.
- Cursor Cloud. "After a landing, this Windows checkout is already on that `main`."
- Planning. "Properly tested means the builder has built and run the tests."
- Tests. "The builder runs the full suite, filter `SolidCore1`, when it lands a buildable tip. A cloud agent does not run that suite."

`AgentBuild.md`:

- The title, "The builder's Windows worker."
- The opening: the builder lands one `build/<name>` branch, and a coding agent runs on the Cursor-hosted Linux VM. The guide's job becomes the Windows coding agent.
- The sentence that the builder reads `agent.ok` before `tools\build.bat` and `tools\test.bat`.
- The Landing section as the worker's own job, including `tools\land.bat` and `SOLID_AGENT_ID` as the builder's id.
- The paragraph that a coding agent on the cloud VM pushes `build/<name>` for work that needs Unreal, merges `main` before `Buildable: yes`, and does not run `tools\land.bat`, `tools\build.bat`, `tools\test.bat`, the editor, or PIE.
- The empty-poll sentence stays, inside the way-back description: when a poll finds no buildable branch, the chat repeats the latest build.

`Planning/README.md`:

- The ship bullet. A story update on product commits the builder has not landed stays on `build/<name>`. Product work that needs Unreal uses `build/<name>`.

`CodingGuidelines.md`:

- Automated tests policy item 5. The builder runs the full suite when it lands a buildable tip. A cloud agent does not run that suite. Item 6 stays. A test does not assert the biomes of the shipped world.
- The paragraph that an earlier rule shipped a test change as its own buildable tip, and that the extra landing is too slow with the builder. The test change and the product change still ship together.

`README.md`:

- Build ID. The builder writes `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` when it lands a buildable branch. A change that ships on `main` does not bump them.
- The layout comment on `AgentBuild.md`, "The builder's Windows worker."

`tools/cloud_agent_check.py`:

- `ship_branch_error`. "A change that needs Unreal moves onto build/<name> and pushes that branch." "A story or process documentation commit on top of unlanded product work stays on that branch, and the tip keeps the line Buildable: yes."
- The docstring on `ship_branch_error` that says `main` and `build/<name>` may be pushed stays true, because a `build/` branch stays legal. The failure text is what changes.

Open stories that hand the build to a second actor:

- `Planning/dracorex-encounter.md`. "A cloud agent does not run the editor or the suite. The Windows machine places the mark in the World chart mode after the row exists." The same agent writes the row, runs the suite, and the developer places the mark.
- `Planning/insect-monsters.md`. "A cloud agent does not run the editor and does not run the suite. The Windows machine adds the pack, then builds and tests." The Windows agent adds the pack, builds, and tests.

### Still in force

These mention a cloud agent, the queue, or the build id. They stay.

- `Planning/Done/builder-lands-the-branch.md` records why the queue was introduced. It does not instruct later work. The other files in `Planning/Done/` stay as they are, including the ones that mention a cloud agent or a build-id bump.
- `Source/SolidCore1/Game/SolidBuildId.h` already says to bump `SOLID_BUILD_ID` when the product changes, and that a documentation change or a test change does not. That comment matches this path.
- `.cursor/environment.json` runs `python3 tools/cloud_agent_check.py` on install. It does not name the builder.
- `Planning/package-the-game.md` and `Planning/roads-end-on-any-mark.md` say a cloud agent does not cook and does not run the editor. The Windows machine still cooks, and the editor session is still the chart check. Those sentences stay.
- The queue scripts are not edited, so their own comments stay the queue's description: the opening of `tools/buildable.py` ("Which branch tips the builder should land"), the opening of `tools/land.py`, and the remark at the top of `tools/land.bat`. Living docs point at them as the way back.
- `tools/cloud_agent_check.py` still requires the workflow section to contain `cursor/<words>-<digits>`, `does not apply to branches or pull requests`, and the phrase that accepts `main` and a `build/` branch. The accept list stays `main` and a `build/` branch.
- In `AGENTS.md`, these stay: the harness override, the 3-minute pause, no pull request unless the user asks, a `Plan:` commit, `Done` by agreement, the play wait and the screenshot rules, the lock words, `tools\yield-ue.bat` and `tools\free-ue.bat`, and the rule that this agent does not run PIE. A cloud agent still does not run the editor, automation, or PIE, and Unreal is still not installed on that VM.
- The Windows agent passes its id to `tools\build.bat`. The same id is the re-entry `tools\test.bat` already allows. The agent releases that id with `tools\free-ue.bat` when the session ends, including when the build or the tests fail. It does not delete `ue.yield`. It does not write `ue.lock` itself.
- The build note is the first line of the product commit, at most the length `tools/land.py` already keeps. The id is the next `SC1-NNNN`.
- The early process, a cloud push followed by a pull on this machine, stays retired. The agent is already on the checkout the developer plays.
- The 3-minute pause stays. Play notes stay a separate message from the test result.
