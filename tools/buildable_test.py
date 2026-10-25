#!/usr/bin/env python3
"""Buildable-branch marker. No Unreal. Run: python -m unittest tools/buildable_test.py"""

from __future__ import annotations

import unittest

from tools.buildable import (
    MARKER,
    buildable_names,
    is_build_branch,
    parse_tip_text,
    tip_is_buildable,
)
from tools.cloud_agent_check import ship_branch_error


class BuildableTests(unittest.TestCase):
    def test_build_branch_names(self) -> None:
        self.assertTrue(is_build_branch("build/auto-playthrough"))
        self.assertTrue(is_build_branch("build/a"))
        self.assertFalse(is_build_branch("main"))
        self.assertFalse(is_build_branch("build/"))
        self.assertFalse(is_build_branch("build/Auto"))
        self.assertFalse(is_build_branch("cursor/build/auto-playthrough"))
        self.assertFalse(is_build_branch("playthrough-gate-opening"))

    def test_marker_is_one_line(self) -> None:
        self.assertTrue(tip_is_buildable(f"Subject\n\n{MARKER}\n"))
        self.assertFalse(tip_is_buildable("Subject\n\nBuildable: no\n"))
        self.assertFalse(tip_is_buildable("Subject mentions Buildable: yes inline\n"))
        self.assertFalse(tip_is_buildable("Subject\n"))

    def test_two_tips_stay_separate_when_their_messages_have_newlines(self) -> None:
        raw = (
            "build/auto-playthrough\0aaa\0SC1-0616: already landed.\n"
            "@@SOLIDCORE1-TIP@@\n"
            "build/name-the-stronghold\0bbb\0The company names its stronghold.\n\n"
            "Buildable: yes\n"
            "@@SOLIDCORE1-TIP@@\n"
        )
        records = parse_tip_text(raw, 3)
        self.assertEqual(
            [name for name, _sha, _message in records],
            ["build/auto-playthrough", "build/name-the-stronghold"],
        )
        self.assertTrue(tip_is_buildable(records[1][2]))
        self.assertFalse(tip_is_buildable(records[0][2]))

    def test_only_a_marked_tip_is_buildable(self) -> None:
        tips = [
            ("build/ready", f"Land the gate.\n\n{MARKER}\n"),
            ("build/writing", "Still writing.\n"),
            ("playthrough-gate-opening", f"Parked.\n\n{MARKER}\n"),
            ("cursor/build/ready", f"{MARKER}\n"),
        ]
        self.assertEqual(buildable_names(tips), ["build/ready"])

    def test_check_accepts_main_and_a_build_branch(self) -> None:
        self.assertIsNone(ship_branch_error("main"))
        self.assertIsNone(ship_branch_error("build/auto-playthrough"))
        self.assertIsNone(ship_branch_error(""))
        self.assertIsNone(ship_branch_error("HEAD"))
        for name in ("cursor/words-b99b", "playthrough-gate-opening", "build/"):
            message = ship_branch_error(name)
            self.assertIsNotNone(message)
            self.assertIn("do not open a pull request", message or "")
            self.assertIn("Buildable: yes", message or "")


if __name__ == "__main__":
    unittest.main()
