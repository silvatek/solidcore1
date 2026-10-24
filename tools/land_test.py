#!/usr/bin/env python3
"""Landing one buildable branch. No Unreal. Run: python -m unittest tools/land_test.py"""

from __future__ import annotations

import unittest

from tools.land import (
    bumped_header,
    failed_test_names,
    github_repo,
    land,
    note_from_message,
    read_build_id,
)


HEADER = """\
#define SOLID_BUILD_ID TEXT("SC1-0612")
#define SOLID_BUILD_NOTE TEXT("The previous note.")
"""

MARKED = "Leaving Dragongate walks the gate opening.\n\nBuildable: yes\n"


class Fake:
    def __init__(self) -> None:
        self.calls: list[str] = []
        self.lock = "Free"
        self.yield_on = False
        self.dirty_tree = False
        self.ok_marker = True
        self.fetch_code = 0
        self.rows = [
            ("build/auto-playthrough", "abc", MARKED),
        ]
        self.landed_result: bool | None = False
        self.rebase_code = 0
        self.rebase_detail = ""
        self.in_rebase = False
        self.touches = False
        self.build_id = "SC1-0613"
        self.bump_id = "SC1-0614"
        self.build_code = 0
        self.test_code = 0
        self.failed: list[str] = []
        self.publish_code = 0
        self.post_code = 0
        self.posts: list[tuple[str, bool, str]] = []

    def fetch(self) -> int:
        self.calls.append("fetch")
        return self.fetch_code

    def lock_word(self) -> str | None:
        return self.lock

    def yield_set(self) -> bool:
        return self.yield_on

    def dirty(self) -> bool:
        return self.dirty_tree

    def tips(self) -> list[tuple[str, str, str]]:
        return list(self.rows)

    def landed(self, sha: str) -> bool | None:
        self.calls.append(f"landed:{sha}")
        return self.landed_result

    def agent_ok(self) -> bool:
        return self.ok_marker

    def acquire(self, agent_id: str) -> bool:
        self.calls.append(f"acquire:{agent_id}")
        return True

    def release(self, agent_id: str) -> None:
        self.calls.append(f"release:{agent_id}")

    def checkout_main(self) -> None:
        self.calls.append("checkout")

    def rebase(self, branch: str) -> tuple[int, str]:
        self.calls.append(f"rebase:{branch}")
        if self.rebase_code != 0:
            self.in_rebase = True
        return self.rebase_code, self.rebase_detail

    def abort_rebase(self) -> None:
        if self.in_rebase:
            self.calls.append("abort")
            self.in_rebase = False

    def touches_build_id(self) -> bool:
        return self.touches

    def current_build_id(self) -> str:
        return self.build_id

    def bump(self, note: str) -> str:
        self.calls.append(f"bump:{note}")
        return self.bump_id

    def build(self) -> int:
        self.calls.append("build")
        return self.build_code

    def test(self) -> tuple[int, list[str]]:
        self.calls.append("test")
        return self.test_code, list(self.failed)

    def publish(self, branch: str) -> int:
        self.calls.append(f"publish:{branch}")
        return self.publish_code

    def post(self, sha: str, ok: bool, comment: str) -> int:
        self.posts.append((sha, ok, comment))
        return self.post_code


class LandTests(unittest.TestCase):
    def test_header_bump_and_note(self) -> None:
        updated, ident = bumped_header(HEADER, "Leaving Dragongate walks the gate opening.")
        self.assertEqual(ident, "SC1-0613")
        self.assertEqual(read_build_id(updated), "SC1-0613")
        self.assertIn('TEXT("Leaving Dragongate walks the gate opening.")', updated)
        self.assertEqual(
            note_from_message(MARKED),
            "Leaving Dragongate walks the gate opening.",
        )

    def test_failed_test_names(self) -> None:
        output = "Failed : 1\n\nFailed tests:\n  - SolidCore1.Fog\n  - SolidCore1.Town\n====\n"
        self.assertEqual(
            failed_test_names(output),
            ["SolidCore1.Fog", "SolidCore1.Town"],
        )

    def test_github_repo(self) -> None:
        self.assertEqual(
            github_repo("git@github.com:silvatek/solidcore1.git"),
            "silvatek/solidcore1",
        )
        self.assertEqual(
            github_repo("https://github.com/silvatek/solidcore1.git"),
            "silvatek/solidcore1",
        )

    def test_session_word_fetches_and_does_not_rebase(self) -> None:
        fake = Fake()
        fake.lock = "PIE"
        outcome = land("builder-1", fake)
        self.assertTrue(outcome.ok)
        self.assertIn("Word: PIE", outcome.lines)
        self.assertEqual(fake.calls, ["fetch"])

    def test_yield_does_not_rebase(self) -> None:
        fake = Fake()
        fake.yield_on = True
        outcome = land("builder-1", fake)
        self.assertTrue(outcome.ok)
        self.assertIn("The next slot is reserved.", outcome.lines)
        self.assertNotIn("rebase:build/auto-playthrough", fake.calls)

    def test_two_tips_are_listed(self) -> None:
        fake = Fake()
        fake.rows.append(("build/other", "def", MARKED))
        outcome = land("builder-1", fake)
        self.assertTrue(outcome.ok)
        self.assertIn("build/auto-playthrough", outcome.lines)
        self.assertIn("build/other", outcome.lines)
        self.assertNotIn("rebase:build/auto-playthrough", fake.calls)

    def test_missing_agent_ok_skips_the_build(self) -> None:
        fake = Fake()
        fake.ok_marker = False
        outcome = land("builder-1", fake)
        self.assertFalse(outcome.ok)
        self.assertIn("agent.ok is missing. The build and the tests were skipped.", outcome.lines)
        self.assertEqual(fake.posts, [])
        self.assertNotIn("build", fake.calls)

    def test_rebase_conflict_posts_on_the_pushed_commit(self) -> None:
        fake = Fake()
        fake.rebase_code = 1
        fake.rebase_detail = "Source/SolidCore1/Playthrough/SolidAutoPlaythrough.cpp"
        outcome = land("builder-1", fake)
        self.assertFalse(outcome.ok)
        self.assertEqual(
            fake.posts,
            [(
                "abc",
                False,
                "Rebase conflict.\nSource/SolidCore1/Playthrough/SolidAutoPlaythrough.cpp",
            )],
        )
        self.assertIn("abort", fake.calls)
        self.assertIn("release:builder-1", fake.calls)
        self.assertNotIn("bump:Leaving Dragongate walks the gate opening.", fake.calls)

    def test_branch_that_sets_the_build_id_is_not_bumped_again(self) -> None:
        fake = Fake()
        fake.touches = True
        outcome = land("builder-1", fake)
        self.assertTrue(outcome.ok)
        self.assertEqual(fake.posts, [("abc", True, "Landed SC1-0613.")])
        self.assertNotIn("bump:Leaving Dragongate walks the gate opening.", fake.calls)
        self.assertEqual(fake.calls[-3:], ["publish:build/auto-playthrough", "release:builder-1", "checkout"])

    def test_branch_without_a_build_id_bumps_once(self) -> None:
        fake = Fake()
        outcome = land("builder-1", fake)
        self.assertTrue(outcome.ok)
        self.assertIn("bump:Leaving Dragongate walks the gate opening.", fake.calls)
        self.assertEqual(fake.posts, [("abc", True, "Landed SC1-0614.")])

    def test_failed_tests_are_named(self) -> None:
        fake = Fake()
        fake.test_code = 1
        fake.failed = ["SolidCore1.Fog"]
        outcome = land("builder-1", fake)
        self.assertFalse(outcome.ok)
        self.assertEqual(fake.posts, [("abc", False, "Failed tests:\nSolidCore1.Fog")])
        self.assertNotIn("publish:build/auto-playthrough", fake.calls)

    def test_a_landed_tip_is_not_waiting(self) -> None:
        fake = Fake()
        fake.landed_result = True
        outcome = land("builder-1", fake)
        self.assertTrue(outcome.ok)
        self.assertIn("No buildable branch.", outcome.lines)
        self.assertNotIn("acquire:builder-1", fake.calls)


if __name__ == "__main__":
    unittest.main()
