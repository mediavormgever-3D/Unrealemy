"""Validatortypen voor het ``validate``-blok van een stap.

Een check is een dict met ``type`` (een geregistreerd validatortype), of een
combinatie ``{"all": [...]}`` / ``{"any": [...]}``. Elke check mag een eigen
``message`` hebben; die vervangt de standaardmelding als de check faalt.

Nieuw type toevoegen: schrijf een functie met ``@validator("naam")`` en voeg
het type toe aan ``Curriculum/schema/curriculum.schema.json``.
"""

from __future__ import annotations

import operator
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List

from .scene import (ActorView, Scene, angle_between, angle_diff, describe, distance, fmt,
                    forward_vector)


@dataclass
class Result:
    passed: bool
    message: str = ""


@dataclass
class Context:
    scene: Scene
    baseline: Dict[str, Any] = field(default_factory=dict)


ValidatorFn = Callable[[Dict[str, Any], Context], Result]
REGISTRY: Dict[str, ValidatorFn] = {}


def validator(name: str) -> Callable[[ValidatorFn], ValidatorFn]:
    def register(fn: ValidatorFn) -> ValidatorFn:
        REGISTRY[name] = fn
        return fn
    return register


def evaluate(spec: Dict[str, Any], ctx: Context) -> Result:
    if "all" in spec:
        for sub in spec["all"]:
            result = evaluate(sub, ctx)
            if not result.passed:
                return _with_override(spec, result)
        return Result(True)
    if "any" in spec:
        first_failure = None
        for sub in spec["any"]:
            result = evaluate(sub, ctx)
            if result.passed:
                return result
            first_failure = first_failure or result
        return _with_override(spec, first_failure or Result(False))
    fn = REGISTRY.get(spec.get("type", ""))
    if fn is None:
        raise ValueError(f"Onbekend validatortype: {spec.get('type')!r}")
    return _with_override(spec, fn(spec, ctx))


def _with_override(spec: Dict[str, Any], result: Result) -> Result:
    if not result.passed and spec.get("message"):
        return Result(False, spec["message"])
    return result


def _missing(selector: Dict[str, str]) -> Result:
    return Result(False, f"Ik kan {describe(selector)} niet vinden. Kwijt? Gebruik 'Reset stap'.")


def _xyz(values) -> str:
    return f"X {fmt(values[0])}, Y {fmt(values[1])}, Z {fmt(values[2])}"


def _pretty_property(path: str) -> str:
    return path.rsplit(".", 1)[-1].replace("_", " ").title()


# --- actors ---------------------------------------------------------------------------

@validator("actor_exists")
def actor_exists(spec, ctx):
    found = ctx.scene.find(spec["actor"])
    needed = spec.get("min_count", 1)
    if len(found) >= needed:
        return Result(True)
    if needed == 1:
        return Result(False, f"Er staat nog geen {describe(spec['actor']).replace('een ', '')} in de scène.")
    return Result(False, f"Er staan er {len(found)}, nodig: {needed}.")


@validator("transform_near")
def transform_near(spec, ctx):
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])

    def check(actor: ActorView):
        if "location" in spec and distance(actor.location, spec["location"]) > spec.get("tolerance", 1.0):
            return f"{actor.label} staat op {_xyz(actor.location)}. Doel: {_xyz(spec['location'])}."
        if "rotation" in spec:
            tol = spec.get("rotation_tolerance", 1.0)
            if any(angle_diff(a, b) > tol for a, b in zip(actor.rotation, spec["rotation"])):
                return f"Rotation van {actor.label} is {_xyz(actor.rotation)}. Doel: {_xyz(spec['rotation'])}."
        if "scale" in spec:
            tol = spec.get("scale_tolerance", 0.01)
            if any(abs(a - b) > tol for a, b in zip(actor.scale, spec["scale"])):
                return f"Scale van {actor.label} is {_xyz(actor.scale)}. Doel: {_xyz(spec['scale'])}."
        return None

    problems = [check(actor) for actor in found]
    if any(problem is None for problem in problems):
        return Result(True)
    return Result(False, problems[0])


@validator("actor_moved")
def actor_moved(spec, ctx):
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])
    start = ctx.baseline.get("actors", {})
    needed = spec["min_distance"]
    best = 0.0
    for actor in found:
        if actor.label in start:
            best = max(best, distance(actor.location, start[actor.label]))
    if best >= needed:
        return Result(True)
    return Result(False, f"{found[0].label} is {fmt(best)} cm verplaatst. Sleep minstens {fmt(needed)} cm.")


@validator("selection_is")
def selection_is(spec, ctx):
    selected = ctx.scene.selected_labels()
    targets = [actor.label for actor in ctx.scene.find(spec["actor"])]
    if not targets:
        return _missing(spec["actor"])
    if len(selected) == 1 and selected[0] in targets:
        return Result(True)
    if not selected:
        return Result(False, "Er is nog niets geselecteerd.")
    return Result(False, f"Selecteer alleen {describe(spec['actor'])}. Nu geselecteerd: {', '.join(selected)}.")


_OPS = {"==": operator.eq, "!=": operator.ne, "<": operator.lt, "<=": operator.le,
        ">": operator.gt, ">=": operator.ge}


def _is_number(value) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def _show(value) -> str:
    if isinstance(value, bool):
        return "aan" if value else "uit"
    return fmt(value) if _is_number(value) else repr(value)


@validator("property_compare")
def property_compare(spec, ctx):
    """Met ``component`` wordt de eigenschap op elke component van die klasse gelezen."""
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])

    if "component" in spec:
        components = [c for actor in found for c in ctx.scene.components(actor, spec["component"])]
        if not components:
            return Result(False, f"{found[0].label} heeft nog geen {spec['component']}.")
        values = [ctx.scene.get_component_property(c, spec["property"]) for c in components]
    else:
        values = [ctx.scene.get_property(actor, spec["property"]) for actor in found]

    op, target = spec["op"], spec["value"]
    tolerance = spec.get("tolerance", 0.0)
    for value in values:
        if _is_number(target) and op in ("==", "!="):
            equal = abs(float(value) - float(target)) <= tolerance
            if equal == (op == "=="):
                return Result(True)
        elif _OPS[op](value, target):
            return Result(True)

    name = _pretty_property(spec["property"])
    if isinstance(target, bool) and op == "==":
        return Result(False, f"{name} staat nog {_show(values[0])}. Zet hem {_show(target)}.")
    return Result(False, f"{name} is {_show(values[0])}. Doel: {_OP_WORDS[op]}{_show(target)}.")


_OP_WORDS = {"==": "", "!=": "niet ", "<": "minder dan ", "<=": "hoogstens ", ">": "meer dan ", ">=": "minstens "}


# --- Outliner en components -----------------------------------------------------------

@validator("actor_in_folder")
def actor_in_folder(spec, ctx):
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])
    if any(actor.folder == spec["folder"] for actor in found):
        return Result(True)
    actor = found[0]
    if not actor.folder:
        return Result(False, f"{actor.label} staat nog niet in een map.")
    return Result(False, f"{actor.label} staat in de map '{actor.folder}'. Verwacht: '{spec['folder']}'.")


@validator("attached_to")
def attached_to(spec, ctx):
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])
    parents = {parent.label for parent in ctx.scene.find(spec["parent"])}
    if not parents:
        return _missing(spec["parent"])
    if any(actor.parent in parents for actor in found):
        return Result(True)
    actor = found[0]
    if actor.parent:
        return Result(False, f"{actor.label} hangt onder '{actor.parent}' in plaats van onder {describe(spec['parent'])}.")
    return Result(False, f"{actor.label} hangt nog niet onder {describe(spec['parent'])}. "
                         f"Sleep hem in de Outliner op {describe(spec['parent'])}.")


@validator("component_exists")
def component_exists(spec, ctx):
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])
    needed = spec.get("min_count", 1)
    counts = [len(ctx.scene.components(actor, spec["class"])) for actor in found]
    if max(counts) >= needed:
        return Result(True)
    if max(counts) == 0:
        return Result(False, f"{found[0].label} heeft nog geen {spec['class']}. Voeg hem toe met + Add in het Details-paneel.")
    return Result(False, f"{found[0].label} heeft {max(counts)} keer {spec['class']}. Nodig: {needed}.")


# --- viewport-camera ------------------------------------------------------------------

def _camera_or_fail(ctx):
    camera = ctx.scene.camera()
    if camera is None:
        return None, Result(False, "Geen viewport gevonden. Klik eerst in het 3D-venster.")
    return camera, None


@validator("viewport_camera_moved")
def viewport_camera_moved(spec, ctx):
    camera, failure = _camera_or_fail(ctx)
    if failure:
        return failure
    start = ctx.baseline.get("camera") or camera
    moved = distance(camera[0], start[0])
    if moved >= spec["min_distance"]:
        return Result(True)
    return Result(False, f"Je bent {fmt(moved)} cm gevlogen. Vlieg nog wat verder.")


@validator("viewport_camera_rotated")
def viewport_camera_rotated(spec, ctx):
    camera, failure = _camera_or_fail(ctx)
    if failure:
        return failure
    start = ctx.baseline.get("camera") or camera
    turned = angle_between(forward_vector(camera[1]), forward_vector(start[1]))
    if turned >= spec["min_degrees"]:
        return Result(True)
    return Result(False, f"Je hebt {fmt(turned)}° rondgekeken. Kijk nog wat verder om je heen.")


@validator("viewport_looks_at")
def viewport_looks_at(spec, ctx):
    camera, failure = _camera_or_fail(ctx)
    if failure:
        return failure
    found = ctx.scene.find(spec["actor"])
    if not found:
        return _missing(spec["actor"])
    for actor in found:
        to_target = [t - c for c, t in zip(camera[0], actor.location)]
        angle = angle_between(forward_vector(camera[1]), to_target)
        if angle <= spec.get("max_angle", 10.0) and distance(camera[0], actor.location) <= spec.get("max_distance", 1000.0):
            return Result(True)
    return Result(False, f"De camera kijkt nog niet van dichtbij naar {describe(spec['actor'])}.")


# --- assets ---------------------------------------------------------------------------

@validator("material_input_from")
def material_input_from(spec, ctx):
    """Let op: de Material Editor werkt op een kopie. Laat de student eerst op Apply klikken."""
    connected = ctx.scene.material_input(spec["material"], spec["input"])
    pin = spec["input"].replace("_", " ").title()
    expected = spec["expression_class"].replace("MaterialExpression", "")
    if connected is None:
        return Result(False, f"Er is nog niets aangesloten op {pin}. Vergeet niet op Apply te klikken.")
    class_name, parameter = connected
    if class_name != spec["expression_class"]:
        return Result(False, f"Op {pin} zit een {class_name.replace('MaterialExpression', '')}, verwacht: {expected}.")
    if "parameter_name" in spec and parameter != spec["parameter_name"]:
        return Result(False, f"Geef de parameter de naam '{spec['parameter_name']}'.")
    return Result(True)


@validator("blueprint_exec_linked")
def blueprint_exec_linked(spec, ctx):
    if ctx.scene.blueprint_exec_linked(spec["blueprint"], spec["event"], spec["function"]):
        return Result(True)
    return Result(False, "De nodes zijn nog niet met elkaar verbonden. Sleep een lijn van het witte pijltje. "
                         "Klik daarna op Compile.")


def known_types() -> List[str]:
    return sorted(REGISTRY)
