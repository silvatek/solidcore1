# The Windows coding agent

One coding agent runs on this Windows checkout. It writes the product change, bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`, builds that commit, and runs the tests. A pass pushes `main`. The agent is the solidcore1-ue worker. It edits the tree the developer plays. The developer plays this same checkout. The developer does not pull. `AGENTS.md` is the ship rule.

This guide starts that worker so the agent can compile.

A cloud agent leaves product work on this checkout. It may push a story, a process document, or a change under `tools/` straight to `main`. That push does not use `build/<name>` and it does not ask for a build. Python tests for a tools change run before the push when the cloud agent can run them. It does not edit `SOLID_BUILD_ID`. It does not run `tools\land.bat`, `tools\build.bat`, `tools\test.bat`, the editor, or PIE.

## Prerequisites

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`, or `UE_ROOT` set to the install
- Visual Studio 2022 with **Game development with C++**, as `README.md` describes
- A local git checkout of `github.com/silvatek/solidcore1` that already opens and builds in the editor
- Fab packs restored with Epic Launcher **Add to Project** (Viking and grass). Confirm with `tools\fab_doctor.bat`
- [Git for Windows](https://git-scm.com/download/win)
- Outbound HTTPS to `api2.cursor.sh`, `api2direct.cursor.sh`, and `downloads.cursor.com`

## Install and sign in

In PowerShell:

```powershell
irm 'https://cursor.com/install?win32=true' | iex
agent --version
agent login
```

Official notes: [Self-Hosted Machines](https://cursor.com/docs/cloud-agent/self-hosted), [My Machines](https://cursor.com/docs/cloud-agent/self-hosted/my-machines).

## Start the worker

From the checkout:

```bat
tools\agent.bat
```

That runs `agent worker start --name "solidcore1-ue" --worker-dir Dev\solidcore1`. When the command exits 0 it writes `agent.ok` in the repo root. The line in the file is `Passed`. The file is gitignored. The Windows agent reads it before `tools\build.bat` and `tools\test.bat`. A missing marker skips the build and the tests.

Keep that window open. The worker stays connected until you stop it. Its git remote must be `silvatek/solidcore1`.

## Build and test

Set `SOLID_AGENT_ID` to this agent's id. Pass that id to `tools\build.bat`. The same id is the re-entry `tools\test.bat` already allows. The shipping commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE` once, before `tools\build.bat` runs. The note is the first line of that commit, at most 120 characters, the length `tools/land.py` already keeps. A double quote in that line is stored as an apostrophe. The id is the next `SC1-NNNN`. The build and the tests run on that commit. `tools\test.bat SolidCore1` is the full suite. A pass pushes `main`. A failure is fixed on this checkout before the push. The command output is the result. The agent does not post a `solidcore1/land` status for its own work.

While `ue.lock` says `PIE`, `Editor`, or `Dump`, the agent does not build or test. `tools\yield-ue.bat` reserves the next build and test. `tools\free-ue.bat` clears a dead agent id when `UnrealEditor.exe`, `UnrealEditor-Cmd.exe`, and `UnrealBuildTool.exe` are not running. It leaves `PIE`, `Editor`, and `Dump` in place. The agent releases its id with `tools\free-ue.bat` when the session ends, including when the build or the tests fail. It does not delete `ue.yield`. It does not write `ue.lock` itself.

The developer starts play with `tools\pie.bat` and the World Editor with `tools\editor.bat`. They send a screenshot when they do. The test pass keeps the story `In Progress`. `Done` waits for the developer's play, when the story changes play, and for agreement.

## Queue scripts

`tools\land.bat` and `tools/buildable.py` stay, with their tests, in case several coding agents are wanted later. That is a future possibility. An agent does not use that queue. It does not push `build/<name>`, and it does not run `tools\land.bat`.

A tip is buildable when a line of its message is `Buildable: yes`. `python tools/buildable.py` lists those tips. `tools\land.bat` lands the oldest waiting tip when `ue.lock` is Free. `SOLID_AGENT_ID` is the holder in that script. A tip already contained in `main` is not waiting. When more than one tip is waiting, the older committer time is landed. An equal time uses the branch name. The others stay until the next landing. The landing posts a GitHub status `solidcore1/land` and a comment on the pushed commit. The comment names the build id, the rebase conflict, or the failed tests. When a poll finds no buildable branch, the chat says that nothing is waiting. It then repeats the most recent build: the build number, the change summary, and the build and test result. An agent does not run that poll. A `build/<name>` tip that is already waiting stays waiting. A later return to those scripts is a separate decision.

Do not install Unreal Engine on the Cursor-hosted Linux VM. Fab packs are not there.
