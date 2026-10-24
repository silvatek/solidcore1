#!/usr/bin/env python3
"""Which branch tips the builder should land.

A coding agent pushes build/<name>. The tip is buildable when a line of
its commit message is exactly "Buildable: yes". main is not buildable.
A cursor/ branch is not a build branch.

Run: python tools/buildable.py
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

BUILD_BRANCH = re.compile(r"^build/[a-z0-9]+(?:-[a-z0-9]+)*$")
MARKER = "Buildable: yes"


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def is_build_branch(name: str) -> bool:
    return BUILD_BRANCH.fullmatch(name) is not None


def tip_is_buildable(message: str) -> bool:
    return any(line.strip() == MARKER for line in message.splitlines())


TIP_END = "@@SOLIDCORE1-TIP@@"


def parse_tip_text(raw: str, fields: int) -> list[tuple[str, ...]]:
    """Split for-each-ref output. This git prints %00 and ignores %x1e."""
    found: list[tuple[str, ...]] = []
    for record in raw.split(TIP_END):
        record = record.strip("\n")
        if not record.strip():
            continue
        parts = record.split("\0", fields - 1)
        if len(parts) < fields:
            continue
        found.append(tuple(parts))
    return found


def buildable_names(tips: list[tuple[str, str]]) -> list[str]:
    return [
        name
        for name, message in tips
        if is_build_branch(name) and tip_is_buildable(message)
    ]


def remote_tips(root: Path) -> tuple[int, list[tuple[str, str]], str]:
    """Fetch origin build/* and return (code, tips, error text)."""
    fetched = subprocess.run(
        [
            "git",
            "fetch",
            "origin",
            "+refs/heads/build/*:refs/remotes/origin/build/*",
        ],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if fetched.returncode != 0:
        detail = (fetched.stderr or fetched.stdout).strip()
        return fetched.returncode, [], detail

    listed = subprocess.run(
        [
            "git",
            "for-each-ref",
            "--format=%(refname:strip=3)%00%(contents)\n" + TIP_END,
            "refs/remotes/origin/build",
        ],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if listed.returncode != 0:
        detail = (listed.stderr or listed.stdout).strip()
        return listed.returncode, [], detail

    tips = [
        (name.strip(), message)
        for name, message in parse_tip_text(listed.stdout, 2)
    ]
    return 0, tips, ""


def main() -> int:
    code, tips, detail = remote_tips(repo_root())
    if code != 0:
        print(detail or "git fetch failed.")
        return code
    names = buildable_names(tips)
    if not names:
        print("No buildable branch.")
        return 0
    for name in names:
        print(name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
