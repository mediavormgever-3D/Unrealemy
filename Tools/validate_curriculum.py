#!/usr/bin/env python3
"""Controleert alle leerlijnen: JSON-schema, verwijzingen en Python-functies.

Gebruik (vanuit de hoofdmap van het project):
    python Tools/validate_curriculum.py

Exitcode 0 = in orde, 1 = fouten gevonden. Waarschuwingen laten de controle niet falen.
Vereist: pip install jsonschema
"""

from __future__ import annotations

import importlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = ROOT / "Plugins" / "CurriculumCore"
CURRICULUM = PLUGIN / "Curriculum"
sys.path.insert(0, str(PLUGIN / "Content" / "Python"))

from unrealemy.validators import known_types  # noqa: E402

# Tab-ID's die de plugin kan tonen, verbergen en markeren.
# Gebaseerd op UE 5.x; nog te controleren tegen UE 5.8.
KNOWN_TABS = {
    "LevelEditorViewport",
    "LevelEditorSceneOutliner",
    "LevelEditorSelectionDetails",
    "PlacementBrowser",
    "ContentBrowserTab1",
    "LevelEditorToolBox",
    "WorldSettingsTab",
    "OutputLog",
}


def load_schema_validator(definition: str):
    from jsonschema import Draft202012Validator

    schema = json.loads((CURRICULUM / "schema" / "curriculum.schema.json").read_text(encoding="utf-8"))
    return Draft202012Validator({"$ref": f"#/$defs/{definition}", "$defs": schema["$defs"]})


class Report:
    def __init__(self):
        self.errors: list[str] = []
        self.warnings: list[str] = []

    def error(self, path: Path, message: str) -> None:
        self.errors.append(f"FOUT  {path.relative_to(ROOT)}: {message}")

    def warn(self, path: Path, message: str) -> None:
        self.warnings.append(f"LET OP {path.relative_to(ROOT)}: {message}")


def read(path: Path, report: Report):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        report.error(path, "bestand ontbreekt")
    except json.JSONDecodeError as error:
        report.error(path, f"ongeldige JSON ({error})")
    return None


def schema_check(validator, data, path: Path, report: Report) -> bool:
    problems = sorted(validator.iter_errors(data), key=lambda e: list(e.absolute_path))
    for problem in problems:
        location = "/".join(str(p) for p in problem.absolute_path) or "(hoofdniveau)"
        report.error(path, f"{location}: {problem.message}")
    return not problems


def schema_type_enum(schema_path: Path) -> list:
    checks = json.loads(schema_path.read_text(encoding="utf-8"))["$defs"]["check"]["allOf"]
    for check in checks:
        if check.get("if") == {"required": ["type"]}:
            return check["then"]["properties"]["type"]["enum"]
    return []


def callable_exists(reference: str) -> bool:
    module_name, function_name = reference.rsplit(".", 1)
    try:
        module = importlib.import_module(f"unrealemy.chapters.{module_name}")
    except ImportError:
        return False
    return callable(getattr(module, function_name, None))


def check_types(spec, path: Path, step_id: str, report: Report, types: set) -> None:
    for key in ("all", "any"):
        for sub in spec.get(key, []):
            check_types(sub, path, step_id, report, types)
    if "type" in spec and spec["type"] not in types:
        report.error(path, f"{step_id}: validatortype '{spec['type']}' bestaat niet in validators.py")


def main() -> int:
    report = Report()
    validators = {name: load_schema_validator(name) for name in ("curriculum", "track", "chapter", "ui_profile")}
    types = set(known_types())

    schema_path = CURRICULUM / "schema" / "curriculum.schema.json"
    schema_types = set(schema_type_enum(schema_path))
    if schema_types != types:
        if types - schema_types:
            report.error(schema_path, f"validatortypen ontbreken in het schema: {sorted(types - schema_types)}")
        if schema_types - types:
            report.error(schema_path, f"schema noemt typen die niet in validators.py staan: {sorted(schema_types - types)}")

    profiles = {}
    for profile_path in sorted((CURRICULUM / "ui_profiles").glob("*.json")):
        profile = read(profile_path, report)
        if profile is None or not schema_check(validators["ui_profile"], profile, profile_path, report):
            continue
        if profile["id"] != profile_path.stem:
            report.error(profile_path, f"id '{profile['id']}' moet gelijk zijn aan de bestandsnaam")
        for tab in profile["hide_tabs"] + profile["show_tabs"]:
            if tab not in KNOWN_TABS:
                report.warn(profile_path, f"onbekende tab-ID '{tab}'")
        overlap = set(profile["hide_tabs"]) & set(profile["show_tabs"])
        if overlap:
            report.error(profile_path, f"tab zowel verborgen als getoond: {sorted(overlap)}")
        profiles[profile["id"]] = profile

    index_path = CURRICULUM / "curriculum.json"
    index = read(index_path, report)
    if index is None or not schema_check(validators["curriculum"], index, index_path, report):
        return finish(report)

    seen_ids: dict[str, Path] = {}
    total_steps = 0

    def claim(identifier: str, path: Path) -> None:
        if identifier in seen_ids:
            report.error(path, f"id '{identifier}' komt ook voor in {seen_ids[identifier].relative_to(ROOT)}")
        seen_ids[identifier] = path

    for folder in index["tracks"]:
        track_path = CURRICULUM / "tracks" / folder / "track.json"
        track = read(track_path, report)
        if track is None or not schema_check(validators["track"], track, track_path, report):
            continue
        claim(track["id"], track_path)

        for number, file_name in enumerate(track["chapters"], start=1):
            chapter_path = track_path.parent / file_name
            chapter = read(chapter_path, report)
            if chapter is None or not schema_check(validators["chapter"], chapter, chapter_path, report):
                continue
            claim(chapter["id"], chapter_path)
            if not chapter["id"].startswith(track["id"] + "."):
                report.error(chapter_path, f"id '{chapter['id']}' hoort te beginnen met '{track['id']}.'")
            if chapter["number"] != number:
                report.warn(chapter_path, f"number is {chapter['number']}, maar staat op plek {number} in track.json")
            if chapter.get("ui_profile") and chapter["ui_profile"] not in profiles:
                report.error(chapter_path, f"UI-profiel '{chapter['ui_profile']}' bestaat niet")
            if chapter.get("setup") and not callable_exists(chapter["setup"]):
                report.error(chapter_path, f"setup-functie '{chapter['setup']}' niet gevonden")

            for position, step in enumerate(chapter["steps"], start=1):
                total_steps += 1
                step_id = step["id"]
                claim(step_id, chapter_path)
                if step_id != f"{chapter['id']}.s{position:02d}":
                    report.error(chapter_path, f"stap {position} heeft id '{step_id}', verwacht '{chapter['id']}.s{position:02d}'")
                if not callable_exists(step["solve"]):
                    report.error(chapter_path, f"{step_id}: solve-functie '{step['solve']}' niet gevonden")
                check_types(step["validate"], chapter_path, step_id, report, types)

                profile_id = step.get("ui_profile") or chapter.get("ui_profile")
                if step.get("ui_profile") and step["ui_profile"] not in profiles:
                    report.error(chapter_path, f"{step_id}: UI-profiel '{step['ui_profile']}' bestaat niet")
                tab = step.get("highlight", {}).get("tab")
                if tab:
                    if tab not in KNOWN_TABS:
                        report.warn(chapter_path, f"{step_id}: onbekende tab-ID '{tab}' in highlight")
                    if profile_id in profiles and tab in profiles[profile_id]["hide_tabs"]:
                        report.error(chapter_path, f"{step_id}: markeert tab '{tab}', maar profiel '{profile_id}' verbergt die")

    print(f"{len(seen_ids)} id's gecontroleerd, waarvan {total_steps} stappen; {len(profiles)} UI-profielen.")
    return finish(report)


def finish(report: Report) -> int:
    for line in report.warnings:
        print(line)
    for line in report.errors:
        print(line)
    print("Resultaat:", "FOUTEN GEVONDEN" if report.errors else "in orde")
    return 1 if report.errors else 0


if __name__ == "__main__":
    sys.exit(main())
