#!/usr/bin/env python3
"""Land one buildable branch onto main.

Run from tools\\land.bat. SOLID_AGENT_ID is the lock holder.
No timer: this runs when it is started. One marked tip is landed.
When more than one tip is waiting, the older committer time is landed.
An equal time uses the branch name. The others stay until the next run.
A tip already marked success for solidcore1/land is not waiting.

The mark is a GitHub commit status on the commit that was pushed.
The comment on that commit is the report.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[1]
if str(_ROOT) not in sys.path:
    sys.path.insert(0, str(_ROOT))

from tools.buildable import MARKER, TIP_END, is_build_branch, parse_tip_text, tip_is_buildable
from tools.ue_lock import YIELD_NAME, acquire, read_word, release

STATUS_CONTEXT = "solidcore1/land"
BUILD_ID_PATH = Path("Source/SolidCore1/Game/SolidBuildId.h")
SESSION_WORDS = frozenset({"PIE", "Editor", "Dump"})
REFUSED_IDS = frozenset({"", "Free", "PIE", "Editor", "Dump", "Build", "Test"})
ID_RE = re.compile(r'(#define\s+SOLID_BUILD_ID\s+TEXT\(")SC1-(\d{4})("\))')
NOTE_RE = re.compile(r'(#define\s+SOLID_BUILD_NOTE\s+TEXT\(")[^"]*("\))')


class Outcome:
    def __init__(self, ok: bool, lines: list[str]) -> None:
        self.ok = ok
        self.lines = lines


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def porcelain_is_dirt(line: str) -> bool:
    path = line[3:].strip().strip('"').replace("\\", "/")
    if " -> " in path:
        path = path.split(" -> ", 1)[1].strip()
    return "__pycache__/" not in path and not path.endswith(".pyc")


def note_from_message(message: str) -> str:
    for line in message.splitlines():
        text = line.strip()
        if text and text != MARKER:
            return text.replace('"', "'")[:120]
    return "Landed a buildable branch."


def read_build_id(text: str) -> str:
    match = ID_RE.search(text)
    if not match:
        return ""
    return f"SC1-{match.group(2)}"


def bumped_header(text: str, note: str) -> tuple[str, str]:
    match = ID_RE.search(text)
    if not match:
        raise ValueError("SOLID_BUILD_ID is missing")
    number = int(match.group(2)) + 1
    ident = f"SC1-{number:04d}"
    text = ID_RE.sub(rf"\g<1>{ident}\g<3>", text, count=1)
    return NOTE_RE.sub(rf"\g<1>{note}\g<2>", text, count=1), ident


def failed_test_names(output: str) -> list[str]:
    names: list[str] = []
    reading = False
    for line in output.splitlines():
        if line.strip() == "Failed tests:":
            reading = True
            continue
        if not reading:
            continue
        if line.startswith("  - "):
            names.append(line[4:].strip())
        elif line.startswith("="):
            break
    return names


def github_repo(url: str) -> str:
    text = url.strip().removesuffix(".git")
    for mark in ("github.com:", "github.com/"):
        if mark in text:
            return text.split(mark, 1)[1]
    raise ValueError(f"origin is not a GitHub remote: {url}")


def _say(lines: list[str], text: str) -> None:
    lines.extend(text.splitlines() or [""])


def land(agent_id: str, actions) -> Outcome:
    lines: list[str] = []
    agent_id = agent_id.strip()
    if agent_id in REFUSED_IDS:
        _say(lines, f"{agent_id} is not an agent id.")
        return Outcome(False, lines)

    if actions.fetch() != 0:
        _say(lines, "git fetch failed.")
        return Outcome(False, lines)

    word = actions.lock_word()
    if word in SESSION_WORDS:
        _say(lines, f"Word: {word}")
        return Outcome(True, lines)
    if actions.yield_set():
        _say(lines, "The next slot is reserved.")
        return Outcome(True, lines)
    if word not in (None, "Free") and word != agent_id:
        _say(lines, f"Word: {word}" if word else "ue.lock is not Free.")
        return Outcome(True, lines)
    if actions.dirty():
        _say(lines, "The checkout is dirty.")
        return Outcome(False, lines)

    waiting: list[tuple[int, str, str, str]] = []
    for branch, sha, message in actions.tips():
        if not is_build_branch(branch) or not tip_is_buildable(message):
            continue
        if actions.on_main(sha):
            continue
        landed = actions.landed(sha)
        if landed is None:
            _say(lines, "Could not read the GitHub status.")
            return Outcome(False, lines)
        if not landed:
            waiting.append((actions.ready_at(sha), branch, sha, message))

    if not waiting:
        _say(lines, "No buildable branch.")
        return Outcome(True, lines)
    waiting.sort(key=lambda item: (item[0], item[1]))
    _ready, branch, sha, message = waiting[0]
    if len(waiting) > 1:
        _say(lines, f"Landing {branch}.")
        for _other_ready, other, _other_sha, _other_message in waiting[1:]:
            _say(lines, other)
    if not actions.agent_ok():
        _say(lines, "agent.ok is missing. The build and the tests were skipped.")
        return Outcome(False, lines)

    if not actions.acquire(agent_id):
        _say(lines, "ue.lock was not taken.")
        return Outcome(False, lines)

    try:
        actions.checkout_main()
        code, detail = actions.rebase(branch)
        if code != 0:
            actions.abort_rebase()
            comment = "Rebase conflict."
            if detail.strip():
                comment = comment + "\n" + detail.strip()
            _post(lines, actions, sha, False, comment)
            return Outcome(False, lines)

        if actions.touches_build_id():
            build_id = actions.current_build_id()
        else:
            build_id = actions.bump(note_from_message(message))
        if not build_id:
            _post(lines, actions, sha, False, "The build id was not bumped.")
            return Outcome(False, lines)

        if actions.build() != 0:
            _post(lines, actions, sha, False, "The build failed.")
            return Outcome(False, lines)

        test_code, failed = actions.test()
        if test_code != 0:
            if failed:
                comment = "Failed tests:\n" + "\n".join(failed)
            else:
                comment = "The tests failed."
            _post(lines, actions, sha, False, comment)
            return Outcome(False, lines)

        published = actions.publish(branch)
        if published != 0 and published != 2:
            _post(lines, actions, sha, False, "The push to main failed.")
            return Outcome(False, lines)

        comment = f"Landed {build_id}."
        if not _post(lines, actions, sha, True, comment):
            return Outcome(False, lines)
        if published == 2:
            _say(lines, "The branch was not updated.")
        return Outcome(True, lines)
    finally:
        actions.abort_rebase()
        actions.release(agent_id)
        actions.checkout_main()


def _post(lines: list[str], actions, sha: str, ok: bool, comment: str) -> bool:
    _say(lines, comment)
    if actions.post(sha, ok, comment) == 0:
        return True
    _say(lines, "The GitHub post failed.")
    return False


def _git(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )


class GitLanding:
    def __init__(self, root: Path, agent_id: str) -> None:
        self.root = root
        self.agent_id = agent_id
        self._base = ""

    def fetch(self) -> int:
        main = _git(self.root, "fetch", "origin", "main")
        if main.returncode != 0:
            return main.returncode
        builds = _git(
            self.root,
            "fetch",
            "origin",
            "+refs/heads/build/*:refs/remotes/origin/build/*",
        )
        if builds.returncode != 0:
            return builds.returncode
        base = _git(self.root, "rev-parse", "origin/main")
        self._base = base.stdout.strip()
        return 0 if base.returncode == 0 else base.returncode

    def lock_word(self) -> str | None:
        return read_word(self.root)

    def yield_set(self) -> bool:
        return (self.root / YIELD_NAME).exists()

    def dirty(self) -> bool:
        status = _git(self.root, "status", "--porcelain")
        return any(porcelain_is_dirt(line) for line in status.stdout.splitlines() if line.strip())

    def tips(self) -> list[tuple[str, str, str]]:
        listed = _git(
            self.root,
            "for-each-ref",
            "--format=%(refname:strip=3)%00%(objectname)%00%(contents)\n" + TIP_END,
            "refs/remotes/origin/build",
        )
        return [
            (name.strip(), sha.strip(), message)
            for name, sha, message in parse_tip_text(listed.stdout, 3)
        ]

    def on_main(self, sha: str) -> bool:
        completed = _git(self.root, "merge-base", "--is-ancestor", sha, "origin/main")
        return completed.returncode == 0

    def ready_at(self, sha: str) -> int:
        completed = _git(self.root, "log", "-1", "--format=%ct", sha)
        try:
            return int(completed.stdout.strip())
        except ValueError:
            return 2**62

    def landed(self, sha: str) -> bool | None:
        completed = self._gh(
            ["api", f"repos/{self._repo()}/commits/{sha}/status"],
        )
        if completed.returncode != 0:
            return None
        try:
            payload = json.loads(completed.stdout or "{}")
        except json.JSONDecodeError:
            return None
        for status in payload.get("statuses") or []:
            if status.get("context") == STATUS_CONTEXT and status.get("state") == "success":
                return True
        return False

    def agent_ok(self) -> bool:
        return (self.root / "agent.ok").is_file()

    def acquire(self, agent_id: str) -> bool:
        return acquire(self.root, agent_id, session=False).ok

    def release(self, agent_id: str) -> None:
        release(self.root, agent_id)

    def checkout_main(self) -> None:
        _git(self.root, "checkout", "main")

    def rebase(self, branch: str) -> tuple[int, str]:
        checkout = _git(self.root, "checkout", "-B", branch, f"origin/{branch}")
        if checkout.returncode != 0:
            detail = (checkout.stderr or checkout.stdout).strip()
            return checkout.returncode, detail
        merged = _git(self.root, "rebase", "origin/main")
        if merged.returncode == 0:
            return 0, ""
        names = _git(self.root, "diff", "--name-only", "--diff-filter=U")
        return merged.returncode, names.stdout.strip()

    def abort_rebase(self) -> None:
        git_dir = _git(self.root, "rev-parse", "--git-path", "rebase-merge")
        apply_dir = _git(self.root, "rev-parse", "--git-path", "rebase-apply")
        for completed in (git_dir, apply_dir):
            path = Path(completed.stdout.strip())
            if not path.is_absolute():
                path = self.root / path
            if path.exists():
                _git(self.root, "rebase", "--abort")
                return

    def touches_build_id(self) -> bool:
        diff = _git(self.root, "diff", "--name-only", "origin/main...HEAD")
        target = BUILD_ID_PATH.as_posix()
        return any(line.strip().replace("\\", "/") == target for line in diff.stdout.splitlines())

    def current_build_id(self) -> str:
        text = (self.root / BUILD_ID_PATH).read_text(encoding="utf-8")
        return read_build_id(text)

    def bump(self, note: str) -> str:
        path = self.root / BUILD_ID_PATH
        text = path.read_text(encoding="utf-8")
        try:
            updated, ident = bumped_header(text, note)
        except ValueError:
            return ""
        path.write_text(updated, encoding="utf-8")
        added = _git(self.root, "add", "--", BUILD_ID_PATH.as_posix())
        if added.returncode != 0:
            return ""
        committed = _git(self.root, "commit", "-m", f"{ident}: {note}")
        return ident if committed.returncode == 0 else ""

    def build(self) -> int:
        return self._bat("build.bat")

    def test(self) -> tuple[int, list[str]]:
        completed = self._run_bat("test.bat", "SolidCore1")
        return completed.returncode, failed_test_names(completed.stdout)

    def publish(self, branch: str) -> int:
        checkout = _git(self.root, "checkout", "main")
        if checkout.returncode != 0:
            return checkout.returncode
        merged = _git(self.root, "merge", "--ff-only", branch)
        if merged.returncode != 0:
            self._reset_main()
            return merged.returncode
        pushed = _git(self.root, "push", "origin", "main")
        if pushed.returncode != 0:
            self._reset_main()
            return pushed.returncode
        updated = _git(
            self.root,
            "push",
            "--force-with-lease",
            "origin",
            f"{branch}:{branch}",
        )
        return 0 if updated.returncode == 0 else 2

    def post(self, sha: str, ok: bool, comment: str) -> int:
        repo = self._repo()
        state = "success" if ok else "failure"
        status = self._gh(
            ["api", f"repos/{repo}/statuses/{sha}", "--input", "-"],
            input_text=json.dumps(
                {
                    "state": state,
                    "context": STATUS_CONTEXT,
                    "description": comment.splitlines()[0][:140],
                }
            ),
        )
        if status.returncode != 0:
            return status.returncode
        posted = self._gh(
            ["api", f"repos/{repo}/commits/{sha}/comments", "--input", "-"],
            input_text=json.dumps({"body": comment}),
        )
        return posted.returncode

    def _reset_main(self) -> None:
        if self._base:
            _git(self.root, "checkout", "main")
            _git(self.root, "reset", "--hard", self._base)

    def _repo(self) -> str:
        url = _git(self.root, "remote", "get-url", "origin")
        return github_repo(url.stdout)

    def _bat(self, name: str, *args: str) -> int:
        return self._run_bat(name, *args).returncode

    def _run_bat(self, name: str, *args: str) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["SOLID_AGENT_ID"] = self.agent_id
        return subprocess.run(
            ["cmd", "/c", str(self.root / "tools" / name), *args],
            cwd=self.root,
            capture_output=True,
            text=True,
            check=False,
            env=env,
        )

    def _gh(
        self,
        args: list[str],
        input_text: str | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["gh", *args],
            cwd=self.root,
            capture_output=True,
            text=True,
            check=False,
            input=input_text,
        )


def main() -> int:
    agent_id = os.environ.get("SOLID_AGENT_ID", "")
    outcome = land(agent_id, GitLanding(repo_root(), agent_id.strip()))
    for line in outcome.lines:
        print(line)
    return 0 if outcome.ok else 1


if __name__ == "__main__":
    sys.exit(main())
