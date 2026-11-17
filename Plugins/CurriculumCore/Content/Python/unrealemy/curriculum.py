"""Leest de leerlijnen uit de JSON-bestanden in ``Plugins/CurriculumCore/Curriculum``.

Geen afhankelijkheid van de editor.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# unrealemy/curriculum.py -> Python -> Content -> CurriculumCore
DEFAULT_ROOT = Path(__file__).resolve().parents[3] / "Curriculum"


def read_json(path: Path) -> Any:
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


@dataclass(frozen=True)
class Step:
    id: str
    chapter_id: str
    index: int
    data: Dict[str, Any]

    @property
    def validate(self) -> Dict[str, Any]:
        return self.data["validate"]

    @property
    def solve(self) -> Optional[str]:
        return self.data.get("solve")

    @property
    def ui_profile(self) -> Optional[str]:
        return self.data.get("ui_profile")

    @property
    def highlight(self) -> Dict[str, str]:
        return self.data.get("highlight", {})


@dataclass(frozen=True)
class Chapter:
    id: str
    track_id: str
    path: Path
    data: Dict[str, Any]
    steps: Tuple[Step, ...]

    @property
    def starter_map(self) -> str:
        return self.data["starter_map"]

    @property
    def setup(self) -> Optional[str]:
        return self.data.get("setup")

    @property
    def ui_profile(self) -> Optional[str]:
        return self.data.get("ui_profile")


@dataclass(frozen=True)
class Track:
    id: str
    folder: str
    path: Path
    data: Dict[str, Any]
    chapters: Tuple[Chapter, ...]


class Curriculum:
    """Alle leerlijnen, hoofdstukken, stappen en UI-profielen."""

    def __init__(self, root: Path | str = DEFAULT_ROOT):
        self.root = Path(root)
        self.index = read_json(self.root / "curriculum.json")
        self.tracks: List[Track] = []
        self._chapters: Dict[str, Chapter] = {}
        self._steps: Dict[str, Step] = {}

        for folder in self.index["tracks"]:
            track_path = self.root / "tracks" / folder / "track.json"
            track_data = read_json(track_path)
            chapters = []
            for file_name in track_data.get("chapters", []):
                chapter_path = track_path.parent / file_name
                chapter_data = read_json(chapter_path)
                steps = tuple(
                    Step(step["id"], chapter_data["id"], index, step)
                    for index, step in enumerate(chapter_data["steps"])
                )
                chapter = Chapter(chapter_data["id"], track_data["id"], chapter_path, chapter_data, steps)
                chapters.append(chapter)
                self._chapters[chapter.id] = chapter
                for step in steps:
                    self._steps[step.id] = step
            self.tracks.append(Track(track_data["id"], folder, track_path, track_data, tuple(chapters)))

        self.ui_profiles: Dict[str, Dict[str, Any]] = {}
        for profile_path in sorted((self.root / "ui_profiles").glob("*.json")):
            profile = read_json(profile_path)
            self.ui_profiles[profile["id"]] = profile

    def step(self, step_id: str) -> Step:
        try:
            return self._steps[step_id]
        except KeyError:
            raise KeyError(f"Onbekende stap: {step_id}") from None

    def chapter(self, chapter_id: str) -> Chapter:
        try:
            return self._chapters[chapter_id]
        except KeyError:
            raise KeyError(f"Onbekend hoofdstuk: {chapter_id}") from None

    def chapters(self) -> List[Chapter]:
        return list(self._chapters.values())

    def steps(self) -> List[Step]:
        return list(self._steps.values())
