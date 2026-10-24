#!/usr/bin/env python3
"""Lock helper tests. No Unreal. Run: python -m unittest tools/ue_lock_test.py"""

from __future__ import annotations

import os
import tempfile
import time
import unittest
from pathlib import Path

from tools.ue_lock import (
    GATE_NAME,
    LOCK_NAME,
    PROCESS_NAMES,
    YIELD_NAME,
    acquire,
    clear_dead,
    read_word,
    release,
)


class LockTests(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def _word_file(self, text: str) -> None:
        (self.root / LOCK_NAME).write_text(text, encoding="utf-8")

    def test_missing_lock_is_free(self) -> None:
        outcome = acquire(self.root, "alpha", session=False)
        self.assertTrue(outcome.ok)
        self.assertEqual(read_word(self.root), "alpha")
        self.assertFalse((self.root / GATE_NAME).exists())

    def test_free_word_can_be_taken(self) -> None:
        self._word_file("  Free  \n")
        outcome = acquire(self.root, "alpha", session=False)
        self.assertTrue(outcome.ok)
        self.assertEqual(read_word(self.root), "alpha")

    def test_second_id_fails_and_leaves_the_first_word(self) -> None:
        acquire(self.root, "alpha", session=False)
        outcome = acquire(self.root, "beta", session=False)
        self.assertFalse(outcome.ok)
        self.assertIn("Word: alpha", outcome.lines)
        self.assertEqual(read_word(self.root), "alpha")

    def test_same_id_reenters(self) -> None:
        first = acquire(self.root, "alpha", session=False)
        second = acquire(self.root, "alpha", session=False)
        self.assertTrue(first.ok)
        self.assertTrue(second.ok)
        self.assertEqual(read_word(self.root), "alpha")

    def test_session_does_not_reenter(self) -> None:
        first = acquire(self.root, "PIE", session=True)
        second = acquire(self.root, "PIE", session=True)
        self.assertTrue(first.ok)
        self.assertFalse(second.ok)
        self.assertEqual(read_word(self.root), "PIE")

    def test_reserved_words_are_refused_as_an_agent_id(self) -> None:
        for word in ("Free", "PIE", "Editor", "Dump", "Build", "Test"):
            with self.subTest(word=word):
                outcome = acquire(self.root, word, session=False)
                self.assertFalse(outcome.ok)
                self.assertIn(f"{word} is not an agent id.", outcome.lines)
                self.assertIsNone(read_word(self.root))

    def test_default_build_and_test_words_are_allowed(self) -> None:
        outcome = acquire(self.root, "Build", session=False, default=True)
        self.assertTrue(outcome.ok)
        self.assertEqual(read_word(self.root), "Build")

    def test_empty_file_is_busy(self) -> None:
        self._word_file("")
        outcome = acquire(self.root, "alpha", session=False)
        self.assertFalse(outcome.ok)
        self.assertEqual((self.root / LOCK_NAME).read_text(encoding="utf-8"), "")

    def test_yield_blocks_build_and_test_only(self) -> None:
        (self.root / YIELD_NAME).write_text("", encoding="utf-8")
        blocked = acquire(self.root, "alpha", session=False)
        self.assertFalse(blocked.ok)
        self.assertIn("The next slot is reserved.", blocked.lines)
        self.assertIsNone(read_word(self.root))

        session = acquire(self.root, "Editor", session=True)
        self.assertTrue(session.ok)
        self.assertEqual(read_word(self.root), "Editor")

    def test_yield_blocks_reentry(self) -> None:
        acquire(self.root, "alpha", session=False)
        (self.root / YIELD_NAME).write_text("", encoding="utf-8")
        outcome = acquire(self.root, "alpha", session=False)
        self.assertFalse(outcome.ok)
        self.assertEqual(read_word(self.root), "alpha")

    def test_release_matching_id(self) -> None:
        acquire(self.root, "alpha", session=False)
        other = release(self.root, "beta")
        self.assertFalse(other.ok)
        self.assertEqual(read_word(self.root), "alpha")
        outcome = release(self.root, "alpha")
        self.assertTrue(outcome.ok)
        self.assertEqual(read_word(self.root), "Free")

    def test_dead_clear_refuses_pie_and_editor(self) -> None:
        for word in ("PIE", "Editor", "Dump", "Free"):
            with self.subTest(word=word):
                self._word_file(word + "\n")
                outcome = clear_dead(self.root, processes_running=lambda _names: False)
                self.assertFalse(outcome.ok)
                self.assertEqual(read_word(self.root), word)

    def test_dead_clear_refuses_missing_and_empty(self) -> None:
        missing = clear_dead(self.root, processes_running=lambda _names: False)
        self.assertFalse(missing.ok)
        self._word_file("")
        empty = clear_dead(self.root, processes_running=lambda _names: False)
        self.assertFalse(empty.ok)
        self.assertEqual((self.root / LOCK_NAME).read_text(encoding="utf-8"), "")

    def test_dead_clear_frees_an_agent_when_unreal_is_gone(self) -> None:
        acquire(self.root, "alpha", session=False)
        seen: dict[str, tuple[str, ...]] = {}

        def probe(names: tuple[str, ...]) -> bool:
            seen["names"] = names
            return False

        outcome = clear_dead(self.root, processes_running=probe)
        self.assertTrue(outcome.ok)
        self.assertEqual(read_word(self.root), "Free")
        self.assertEqual(seen["names"], PROCESS_NAMES)

    def test_dead_clear_leaves_the_id_while_unreal_is_running(self) -> None:
        acquire(self.root, "alpha", session=False)
        outcome = clear_dead(self.root, processes_running=lambda _names: True)
        self.assertFalse(outcome.ok)
        self.assertIn("Unreal is still running.", outcome.lines)
        self.assertEqual(read_word(self.root), "alpha")

    def test_young_gate_fails_and_stays(self) -> None:
        gate = self.root / GATE_NAME
        gate.mkdir()
        outcome = acquire(self.root, "alpha", session=False)
        self.assertFalse(outcome.ok)
        self.assertTrue(gate.is_dir())
        self.assertIsNone(read_word(self.root))

    def test_stale_gate_is_removed_once(self) -> None:
        gate = self.root / GATE_NAME
        gate.mkdir()
        old = time.time() - 31
        os.utime(gate, (old, old))
        outcome = acquire(self.root, "alpha", session=False)
        self.assertTrue(outcome.ok)
        self.assertEqual(read_word(self.root), "alpha")
        self.assertFalse(gate.exists())

    def test_overlapping_acquires_leave_one_winner(self) -> None:
        second: dict[str, object] = {}

        def hold() -> None:
            second["outcome"] = acquire(self.root, "beta", session=False)

        outcome = acquire(self.root, "alpha", session=False, hold=hold)
        self.assertTrue(outcome.ok)
        loser = second["outcome"]
        self.assertFalse(loser.ok)  # type: ignore[attr-defined]
        self.assertEqual(read_word(self.root), "alpha")
        self.assertFalse((self.root / GATE_NAME).exists())


if __name__ == "__main__":
    unittest.main()
