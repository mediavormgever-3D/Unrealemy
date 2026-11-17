"""Speelt elk hoofdstuk af in een nagebootste scène.

Per stap wordt gecontroleerd:
1. de stap is bij aanvang nog NIET voltooid (anders springt hij meteen op 'Voltooid');
2. na de solve-functie IS de stap voltooid;
3. 'Reset stap' levert dezelfde beginstaat op, zodat ook stap 1 en 2 hierboven gelden.

Gebruik: python -m unittest discover -s Tools/tests
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

PLUGIN = Path(__file__).resolve().parents[2] / "Plugins" / "CurriculumCore"
sys.path.insert(0, str(PLUGIN / "Content" / "Python"))

from unrealemy.curriculum import Curriculum  # noqa: E402
from unrealemy.fake_scene import FakeScene  # noqa: E402
from unrealemy.runner import Runner  # noqa: E402

CURRICULUM = Curriculum()


def make_runner():
    reports = []
    runner = Runner(FakeScene(), lambda step_id, passed, msg: reports.append((step_id, passed, msg)),
                    curriculum=CURRICULUM, log=lambda message: None)
    return runner, reports


class ChapterReplayTest(unittest.TestCase):
    def test_curriculum_has_chapters(self):
        self.assertTrue(CURRICULUM.chapters(), "Er zijn geen hoofdstukken gevonden")

    def test_play_through_every_chapter(self):
        for chapter in CURRICULUM.chapters():
            with self.subTest(chapter=chapter.id):
                runner, _ = make_runner()
                runner.begin_step(chapter.steps[0].id, reset=True)
                for step in chapter.steps:
                    runner.begin_step(step.id)
                    before = runner.evaluate(step.id)
                    self.assertFalse(before.passed, f"{step.id} is al voltooid voordat de student iets doet")
                    self.assertTrue(before.message, f"{step.id} geeft geen feedback zolang hij niet klaar is")
                    runner.solve(step.id)
                    after = runner.evaluate(step.id)
                    self.assertTrue(after.passed, f"{step.id} na solve nog niet voltooid: {after.message}")

    def test_reset_rebuilds_start_state_of_each_step(self):
        for chapter in CURRICULUM.chapters():
            for step in chapter.steps:
                with self.subTest(step=step.id):
                    runner, _ = make_runner()
                    runner.begin_step(step.id, reset=True)
                    self.assertFalse(runner.evaluate(step.id).passed, f"{step.id} is direct na reset al voltooid")
                    runner.solve(step.id)
                    self.assertTrue(runner.evaluate(step.id).passed, f"{step.id} niet voltooid na reset + solve")

    def test_reset_makes_backup_of_changed_scene(self):
        chapter = CURRICULUM.chapters()[0]
        runner, _ = make_runner()
        runner.begin_step(chapter.steps[0].id, reset=True)
        runner.solve(chapter.steps[0].id)
        runner.scene.dirty = True
        runner.begin_step(chapter.steps[0].id, reset=True)
        self.assertEqual(len(runner.scene.backups), 1)
        self.assertIn("/Game/StudentWork/", runner.scene.backups[0])

    def test_check_reports_to_cpp(self):
        chapter = CURRICULUM.chapters()[0]
        runner, reports = make_runner()
        first = chapter.steps[0].id
        runner.begin_step(first, reset=True)
        runner.check(first)
        runner.solve(first)
        runner.check(first)
        self.assertEqual([(r[0], r[1]) for r in reports], [(first, False), (first, True)])


if __name__ == "__main__":
    unittest.main()
