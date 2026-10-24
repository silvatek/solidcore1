#!/usr/bin/env python3
"""The only writer of the ue.lock word.

A missing ue.lock counts as Free. An empty file is busy. Surrounding
whitespace on the word is ignored. Callers reach this through the bats.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

LOCK_NAME = "ue.lock"
GATE_NAME = "ue.lock.gate"
YIELD_NAME = "ue.yield"
GATE_STALE_SECONDS = 30

SESSION_WORDS = ("PIE", "Editor", "Dump")
NEVER_AN_AGENT_ID = frozenset({"Free", "PIE", "Editor", "Dump", "Build", "Test"})
DEFAULT_HOLDER_WORDS = frozenset({"Build", "Test"})
DEAD_CLEAR_REFUSED = frozenset({"Free", "PIE", "Editor", "Dump"})
PROCESS_NAMES = (
    "UnrealEditor.exe",
    "UnrealEditor-Cmd.exe",
    "UnrealBuildTool.exe",
)

RESERVED_SLOT = "The next slot is reserved."


class Outcome:
    def __init__(self, ok: bool, lines: list[str]) -> None:
        self.ok = ok
        self.lines = lines

    @property
    def text(self) -> str:
        return "\n".join(self.lines)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def read_word(root: Path) -> str | None:
    """None when the lock file is missing. Empty string when the file is blank."""
    path = root / LOCK_NAME
    if not path.is_file():
        return None
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines:
        return ""
    return lines[0].strip()


def _is_free(word: str | None) -> bool:
    return word is None or word == "Free"


def _write_word(root: Path, gate: Path, word: str) -> None:
    staged = gate / "next"
    staged.write_text(word + "\n", encoding="utf-8")
    os.replace(staged, root / LOCK_NAME)


def _try_mkdir(gate: Path) -> bool:
    try:
        gate.mkdir()
    except FileExistsError:
        return False
    except OSError:
        return False
    return True


def _remove(path: Path) -> None:
    if path.is_dir():
        shutil.rmtree(path, ignore_errors=True)
    elif path.exists():
        path.unlink()


def _gate_age(gate: Path) -> float:
    try:
        return time.time() - gate.stat().st_mtime
    except OSError:
        return 0.0


def take_gate(root: Path) -> Path | None:
    """Own ue.lock.gate, or None when a younger gate is already there.

    A gate left behind for 30 seconds is removed once, then this tries once more.
    """
    gate = root / GATE_NAME
    if _try_mkdir(gate):
        return gate
    if _gate_age(gate) < GATE_STALE_SECONDS:
        return None
    _remove(gate)
    if _try_mkdir(gate):
        return gate
    return None


def _not_free(word: str | None) -> Outcome:
    shown = "" if word is None else word
    return Outcome(False, ["ue.lock is not Free.", f"Word: {shown}"])


def _with_gate(root: Path, body) -> Outcome:
    gate = take_gate(root)
    if gate is None:
        return _not_free(read_word(root))
    try:
        return body(gate)
    finally:
        _remove(gate)


def _rejected_agent_id(word: str, default: bool) -> bool:
    if word in NEVER_AN_AGENT_ID and not (default and word in DEFAULT_HOLDER_WORDS):
        return True
    return False


def acquire(
    root: Path,
    word: str,
    *,
    session: bool,
    default: bool = False,
    hold=None,
) -> Outcome:
    word = word.strip()
    if session:
        if word not in SESSION_WORDS:
            return Outcome(False, [f"{word} is not a session word."])
    else:
        if (root / YIELD_NAME).exists():
            return Outcome(False, [RESERVED_SLOT])
        if not word or _rejected_agent_id(word, default):
            return Outcome(False, [f"{word} is not an agent id."])

    def body(gate: Path) -> Outcome:
        current = read_word(root)
        if _is_free(current):
            if hold is not None:
                hold()
            _write_word(root, gate, word)
            return Outcome(True, [f"Word: {word}"])
        if not session and current == word:
            return Outcome(True, [f"Word: {word}"])
        return _not_free(current)

    return _with_gate(root, body)


def release(root: Path, word: str) -> Outcome:
    word = word.strip()

    def body(gate: Path) -> Outcome:
        current = read_word(root)
        if current == word and word != "":
            _write_word(root, gate, "Free")
            return Outcome(True, ["Word: Free"])
        shown = "" if current is None else current
        return Outcome(False, [f"ue.lock is not held by {word}.", f"Word: {shown}"])

    return _with_gate(root, body)


def default_processes_running(names: tuple[str, ...]) -> bool:
    if os.name != "nt":
        return False
    completed = subprocess.run(
        ["tasklist", "/FO", "CSV", "/NH"],
        capture_output=True,
        text=True,
        check=False,
    )
    output = completed.stdout.casefold()
    return any(name.casefold() in output for name in names)


def clear_dead(root: Path, processes_running=None) -> Outcome:
    probe = processes_running or default_processes_running

    def body(gate: Path) -> Outcome:
        current = read_word(root)
        if current is None:
            return Outcome(False, ["ue.lock is missing."])
        if current == "" or current in DEAD_CLEAR_REFUSED:
            shown = current
            return Outcome(False, ["ue.lock is not a dead agent.", f"Word: {shown}"])
        if probe(PROCESS_NAMES):
            return Outcome(False, ["Unreal is still running.", f"Word: {current}"])
        _write_word(root, gate, "Free")
        return Outcome(True, ["Word: Free"])

    return _with_gate(root, body)


def reserve(root: Path) -> Outcome:
    (root / YIELD_NAME).write_text("", encoding="utf-8")
    return Outcome(True, [RESERVED_SLOT])


def clear_reserve(root: Path) -> Outcome:
    path = root / YIELD_NAME
    if path.exists():
        path.unlink()
    return Outcome(True, ["The next slot is open."])


def _emit(outcome: Outcome) -> int:
    for line in outcome.lines:
        print(line)
    return 0 if outcome.ok else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="ue_lock.py")
    sub = parser.add_subparsers(dest="cmd", required=True)

    acquire_cmd = sub.add_parser("acquire")
    acquire_cmd.add_argument("word")
    mode = acquire_cmd.add_mutually_exclusive_group(required=True)
    mode.add_argument("--session", action="store_true")
    mode.add_argument("--build", action="store_true")
    acquire_cmd.add_argument("--default", action="store_true")

    release_cmd = sub.add_parser("release")
    release_cmd.add_argument("word")

    sub.add_parser("clear-dead")
    sub.add_parser("yield")
    sub.add_parser("yield-clear")

    args = parser.parse_args(argv)
    root = repo_root()
    if args.cmd == "acquire":
        outcome = acquire(
            root,
            args.word,
            session=args.session,
            default=args.default,
        )
    elif args.cmd == "release":
        outcome = release(root, args.word)
    elif args.cmd == "clear-dead":
        outcome = clear_dead(root)
    elif args.cmd == "yield":
        outcome = reserve(root)
    else:
        outcome = clear_reserve(root)
    return _emit(outcome)


if __name__ == "__main__":
    sys.exit(main())
