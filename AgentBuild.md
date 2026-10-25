# The builder's Windows worker

The builder runs on this Windows checkout. It lands one `build/<name>` branch when `ue.lock` is Free, builds, runs the tests, bumps `SOLID_BUILD_ID`, and pushes `main`. A coding agent runs on the Cursor-hosted Linux VM. `AGENTS.md` is the ship rule.

This guide starts the `solidcore1-ue` worker so the builder can compile. The developer plays this same checkout. The developer does not pull.

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

That runs `agent worker start --name "solidcore1-ue" --worker-dir Dev\solidcore1`. When the command exits 0 it writes `agent.ok` in the repo root. The line in the file is `Passed`. The file is gitignored. The builder reads it before `tools\build.bat` and `tools\test.bat`. A missing marker skips the build and the tests.

Keep that window open. The worker stays connected until you stop it. Its git remote must be `silvatek/solidcore1`.

## Landing

`tools\land.bat` lands one buildable tip. Set `SOLID_AGENT_ID` to the builder's id. A line of that tip's message is `Buildable: yes`. While `ue.lock` says `PIE`, `Editor`, or `Dump`, the landing may fetch and does not rebase, build, or test. `tools\yield-ue.bat` reserves the next slot.

A coding agent runs on the cloud VM. It pushes `build/<name>` for work that needs Unreal. It merges `main` into that branch before the tip is marked `Buildable: yes`. The buildable commit follows the merge. It pushes `main` for a change it can finish, including a story or a process document that does not sit on unlanded product commits. A document on top of that unlanded work stays on the branch, and the tip stays buildable. It does not edit `SOLID_BUILD_ID`. It does not run `tools\land.bat`, `tools\build.bat`, `tools\test.bat`, the editor, or PIE.

Do not install Unreal Engine on the Cursor-hosted Linux VM. Fab packs are not there.
