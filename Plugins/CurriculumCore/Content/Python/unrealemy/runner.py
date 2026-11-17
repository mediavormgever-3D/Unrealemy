"""Brug tussen de C++-plugin en de validatie.

``UCurriculumSubsystem`` (C++) roept deze functies aan:

* ``begin_step(step_id, reset)``: bij het openen van een stap. Met ``reset=True``
  wordt eerst een back-up gemaakt, daarna de startmap van het hoofdstuk geladen,
  ``setup`` uitgevoerd en de ``solve`` van alle eerdere stappen afgespeeld.
  Zo staat de scène altijd precies in de beginstaat van die stap.
* ``check(step_id)``: vier keer per seconde zolang de stap open is.
* ``solve(step_id)``: knop 'Oplossing' (docentmodus).
* ``reload()``: knop 'Herlaad' (docentmodus), na het aanpassen van JSON of Python.
"""

from __future__ import annotations

import importlib
import os
import re
import sys
import time
import traceback
from typing import Any, Callable, Dict, Optional

from .curriculum import Curriculum, Step
from .validators import Context, Result, evaluate

Reporter = Callable[[str, bool, str], None]
CHAPTER_PACKAGE = "unrealemy.chapters"


def backup_asset_path(chapter_id: str, now: Optional[float] = None, user: Optional[str] = None) -> str:
    """Per Windows-gebruiker een eigen map, want school-pc's worden gedeeld."""
    user = user or os.environ.get("USERNAME") or os.environ.get("USER") or "student"
    safe_user = re.sub(r"[^A-Za-z0-9_]", "_", user)
    stamp = time.strftime("%Y%m%d_%H%M%S", time.localtime(now))
    return f"/Game/StudentWork/{safe_user}/Backups/{chapter_id.replace('.', '_')}_{stamp}"


class Runner:
    def __init__(self, scene, reporter: Reporter, curriculum: Optional[Curriculum] = None,
                 log: Callable[[str], None] = print):
        self.scene = scene
        self.reporter = reporter
        self.curriculum = curriculum or Curriculum()
        self.log = log
        self.active_step_id: Optional[str] = None
        self.baseline: Dict[str, Any] = {}
        self._errors_logged: set = set()

    # --- aangeroepen vanuit C++ ---
    def begin_step(self, step_id: str, reset: bool = False) -> None:
        step = self.curriculum.step(step_id)
        if reset:
            self._reset_to(step)
        self.active_step_id = step_id
        self.baseline = self._capture_baseline()
        self._errors_logged.discard(step_id)

    def evaluate(self, step_id: str) -> Result:
        step = self.curriculum.step(step_id)
        if step_id != self.active_step_id:
            self.begin_step(step_id)
        try:
            return evaluate(step.validate, Context(self.scene, self.baseline))
        except Exception:
            if step_id not in self._errors_logged:
                self._errors_logged.add(step_id)
                self.log(f"Unrealemy: fout bij het controleren van {step_id}\n{traceback.format_exc()}")
            return Result(False, "Er ging iets mis bij het controleren. Vraag je docent om in de Output Log te kijken.")

    def check(self, step_id: str) -> Result:
        result = self.evaluate(step_id)
        self.reporter(step_id, result.passed, result.message)
        return result

    def solve(self, step_id: str) -> None:
        step = self.curriculum.step(step_id)
        if not step.solve:
            raise ValueError(f"Stap {step_id} heeft geen solve-functie")
        self.call(step.solve)

    def reload(self) -> None:
        self.curriculum = Curriculum(self.curriculum.root)
        for name in sorted(sys.modules):
            if name.startswith(CHAPTER_PACKAGE + "."):
                importlib.reload(sys.modules[name])

    # --- intern ---
    def _reset_to(self, step: Step) -> None:
        chapter = self.curriculum.chapter(step.chapter_id)
        try:
            self.scene.backup_level(backup_asset_path(chapter.id))
        except Exception:
            self.log(f"Unrealemy: back-up mislukt, reset gaat door\n{traceback.format_exc()}")
        self.scene.load_starter_map(chapter.starter_map)
        if chapter.setup:
            self.call(chapter.setup)
        for previous in chapter.steps[: step.index]:
            self.call(previous.solve)

    def call(self, reference: str) -> None:
        """``"t1_c01.solve_s05"`` -> ``unrealemy.chapters.t1_c01.solve_s05(scene)``."""
        module_name, function_name = reference.rsplit(".", 1)
        module = importlib.import_module(f"{CHAPTER_PACKAGE}.{module_name}")
        getattr(module, function_name)(self.scene)

    def _capture_baseline(self) -> Dict[str, Any]:
        return {
            "camera": self.scene.camera(),
            "actors": {actor.label: actor.location for actor in self.scene.actors()},
        }


# --- ingang voor de editor -------------------------------------------------------------

_RUNNER: Optional[Runner] = None


def _editor_runner() -> Runner:
    global _RUNNER
    if _RUNNER is None:
        import unreal

        from .scene_unreal import UnrealScene

        def report(step_id: str, passed: bool, message: str) -> None:
            unreal.get_editor_subsystem(unreal.CurriculumSubsystem).report_check_result(step_id, passed, message)

        _RUNNER = Runner(UnrealScene(), report, log=unreal.log_warning)
    return _RUNNER


def begin_step(step_id: str, reset: bool = False) -> None:
    _editor_runner().begin_step(step_id, reset)


def check(step_id: str) -> None:
    _editor_runner().check(step_id)


def solve(step_id: str) -> None:
    _editor_runner().solve(step_id)


def reload() -> None:
    _editor_runner().reload()
