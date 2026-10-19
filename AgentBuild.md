# Self-hosted agent build (Windows + UE 5.8)

This guide sets up a **Cursor My Machines** worker on your Windows Unreal PC so Cloud Agents can compile SolidCore1 and run the automated test suite on the same machine you use for PIE.

Cursor still runs planning and model inference in the cloud. Terminal commands, file edits, and builds run on this PC over an outbound HTTPS connection. No inbound ports or VPN are required.

Official docs: [Self-Hosted Machines](https://cursor.com/docs/cloud-agent/self-hosted), [My Machines](https://cursor.com/docs/cloud-agent/self-hosted/my-machines).

## Why this instead of the managed cloud VM

The Cursor-hosted SolidCore1 environment is a Linux **code + git workshop** (`python3 tools/cloud_agent_check.py` only). It must not install Unreal. Fab packs are gitignored and absent there.

Your Windows box already has UE 5.8, Visual Studio, Fab content, and the real verify loop (`tools\build.bat`, `tools\test.bat`, PIE). Point a My Machines worker at that checkout.

Managed cloud agents remain useful for planning and light repo edits. Use the self-hosted worker when the agent must **build or run tests**.

## Prerequisites (on the Windows PC)

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8` (or set `UE_ROOT` to your install)
- Visual Studio 2022 with **Game development with C++** (same as `README.md`)
- A local git checkout of `github.com/silvatek/solidcore1` that already opens and builds in the editor
- Fab packs restored via Epic Launcher **Add to Project** (Viking, grass, Assassin, Blacksmith). Confirm with `tools\fab_doctor.bat`
- [Git for Windows](https://git-scm.com/download/win) (includes Git Bash). The Cursor CLI installer requires this for a private worker
- Outbound HTTPS to:
  - `api2.cursor.sh` and `api2direct.cursor.sh` (agent session)
  - `downloads.cursor.com` (CLI updates)
  - `cloud-agent-artifacts.s3.us-east-1.amazonaws.com` (artifact uploads; optional but useful)

Optional: set user or system env `UE_ROOT` if the engine is not in the default path.

## Critical differences from Cursor-hosted agents

Read this before the first session.

| Managed cloud VM | My Machines worker on this PC |
| --- | --- |
| Isolated disposable checkout | Uses **your existing repo folder** |
| Does not switch your local branches for you in the same way | Keeps the checkout as-is; does **not** clone a fresh tree by default |
| Safe to leave dirty; VM is recycled | Edits land in your working tree; uncommitted work can collide with the agent |
| No Unreal / Fab | Full editor build and automation |

Before starting a self-hosted agent session:

1. Commit or stash local work you care about.
2. Prefer a clean `main` that matches GitHub (`git status`, `git pull`).
3. Close or avoid a long PIE session that holds editor locks if the agent will rebuild (UBT `-WaitMutex` waits, but a busy editor still slows or blocks builds).
4. Keep the worker process running for the whole session.

Ship rules in `AGENTS.md` still apply (checkout `main`, push to `main`, bump `SOLID_BUILD_ID` on product pushes). Documentation and test changes do not bump it. The “do not run Unreal on the cloud VM” rule applies only to **Cursor-hosted** Linux agents, not to this Windows worker.

## 1. Install the Cursor CLI

In **PowerShell**:

```powershell
irm 'https://cursor.com/install?win32=true' | iex
agent --version
```

If `agent` is not found, open a new PowerShell window (PATH refresh) and retry.

## 2. Sign in

```powershell
agent login
```

Use the same Cursor account you use for Cloud Agents. For headless/devbox login, a personal user API key from [Cursor Dashboard → API Keys](https://cursor.com/dashboard) works:

```powershell
agent worker start --api-key "your-user-api-key"
```

Service-account keys are for Team Pools (Enterprise), not My Machines.

## 3. Start the worker from the SolidCore1 checkout

```powershell
cd C:\path\to\solidcore1
agent worker start --name "solidcore1-ue"
```

Keep that window open. The worker stays connected until you stop it (Ctrl+C) and can be reused across sessions.

Useful variants:

```powershell
# Explicit repo root (any cwd)
agent worker start --name "solidcore1-ue" --worker-dir C:\path\to\solidcore1

# Preflight without starting
agent worker debug
# Or:
agent worker start --debug
```

The worker registers the git remote of `--worker-dir` (or the current directory). That remote must be `silvatek/solidcore1` so Cloud Agent runs for this repo can route here.

## 4. Launch an agent on this machine

1. Open [cursor.com/agents](https://cursor.com/agents).
2. In the environment / machine picker, choose **My Machines** → `solidcore1-ue` (or the name you passed).
3. Send a task that needs a build or tests, for example:
   - “Pull `main`, implement X, bump the build id if the product changed, run `tools\build.bat` then `tools\test.bat`, fix failures, push to `main`.”

From Slack / GitHub / Linear triggers, target the named machine with `worker=solidcore1-ue` or `machine=solidcore1-ue` (see [My Machines](https://cursor.com/docs/cloud-agent/self-hosted/my-machines)). The machine must belong to your Cursor user and be registered for this repo.

## 5. Commands the agent should use on this PC

These match your normal Windows workflow:

```bat
tools\build.bat
tools\test.bat
tools\test.bat SolidCore1.Fog
tools\fab_doctor.bat
```

Notes:

- `tools\build.bat` builds **SolidCore1Editor | Win64 | Development** via Unreal Build Tool.
- `tools\test.bat` runs UE Automation headlessly (`UnrealEditor-Cmd`, filter default `SolidCore1`) and prints ran / passed / failed. (`README.md` still mentions `run_automation_tests.bat`; the script on disk is `tools\test.bat`.)
- Do **not** install Unreal into a Linux cloud environment. Build and test only on this worker.
- PIE remains optional and human-driven unless you later enable computer use and accept the risk of the agent driving the desktop.

Lightweight checks still work here too:

```bat
python tools\cloud_agent_check.py
```

## 6. Optional: sessionStart hook

Self-hosted workers can run project hooks from `.cursor/hooks.json` in the worker directory (see Cursor Hooks docs). A simple `sessionStart` can print readiness (UE path, Fab doctor) so a failed machine shows up early. Skip this until the basic worker path works.

Example shape (do not commit until you have verified the hook schema you want):

```json
{
  "version": 1,
  "hooks": {
    "sessionStart": [
      {
        "command": "cmd /c tools\\fab_doctor.bat"
      }
    ]
  }
}
```

## 7. Optional: computer use

```powershell
agent worker --computer-use --name "solidcore1-ue" start
```

This lets the agent drive the desktop (screenshots, clicks). On Windows, treat it as advanced: the agent can interfere with whatever is on screen, including an interactive editor or PIE session. Prefer headless `tools\build.bat` / `tools\test.bat` for verification unless you explicitly want UI automation.

## 8. Cost

- Cursor bills the selected **model** (plan included usage, then on-demand API rates). There is no separate Cursor VM fee for My Machines.
- You pay to run this PC (power, hardware you already own).
- Longer compile + automation sessions use more tokens than a managed “code only” agent.

## 9. Troubleshooting

| Symptom | What to check |
| --- | --- |
| Machine missing from picker | Worker process still running; same Cursor account as the web UI; `agent worker debug` |
| Wrong repo / reject on `worker=` | Start the worker inside (or `--worker-dir`) a checkout whose `git remote` is this repo |
| `UnrealEditor-Cmd` / UBT not found | `UE_ROOT`; default `C:\Program Files\Epic Games\UE_5.8` |
| Content / mesh tests fail | Fab **Add to Project**; `tools\fab_doctor.bat` |
| Build waits forever | Another UBT/editor holds the mutex; close conflicting builds |
| Artifacts missing in dashboard | Outbound access to the S3 artifact host; blocking it only drops uploads |

## 10. Suggested first smoke test

1. Start the worker (`--name "solidcore1-ue"`).
2. From [cursor.com/agents](https://cursor.com/agents), select that machine.
3. Ask: “Run `tools\build.bat`, then `tools\test.bat`. Report pass/fail counts. Do not change code.”
4. Confirm the summary matches a local manual run.
5. Only then give the agent an implement → build → test → push task.

## Keep both environments

| Role | Where |
| --- | --- |
| Planning, map/text checks, git hygiene without UE | Cursor-hosted cloud agent (current `AGENTS.md` Cursor Cloud section) |
| Compile, automation suite, Fab-backed content tests | This Windows My Machines worker |

Do not try to install UE 5.8 into the managed Linux cloud snapshot for SolidCore1.
