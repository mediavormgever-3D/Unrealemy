"""Gemeenschappelijke interface voor de scène en wiskundige hulpfuncties.

Twee implementaties:
  * ``scene_unreal.UnrealScene``: de echte editor (alleen binnen Unreal);
  * ``fake_scene.FakeScene``: een nagebootste scène voor automatische tests.

Afspraken:
  * afstanden in Unreal-eenheden (1 = 1 cm);
  * rotatie als (roll, pitch, yaw) in graden: dezelfde volgorde als X, Y, Z
    bij Rotation in het Details-paneel;
  * een *selector* is een dict met ``label`` en/of ``class``.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Any, Dict, List, Optional, Sequence, Tuple

Vec3 = Tuple[float, float, float]
Rot3 = Tuple[float, float, float]
Selector = Dict[str, str]


@dataclass
class ActorView:
    """Momentopname van één actor, los van de editor."""

    label: str
    class_name: str
    location: Vec3
    rotation: Rot3
    scale: Vec3
    handle: Any = None


def matches(actor: ActorView, selector: Selector) -> bool:
    if "label" in selector and actor.label != selector["label"]:
        return False
    if "class" in selector and actor.class_name != selector["class"]:
        return False
    return True


def describe(selector: Selector) -> str:
    if "label" in selector:
        return f"'{selector['label']}'"
    return f"een {selector.get('class', 'actor')}"


def distance(a: Sequence[float], b: Sequence[float]) -> float:
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def angle_diff(a: float, b: float) -> float:
    """Kleinste verschil tussen twee hoeken in graden (0..180)."""
    return abs((a - b + 180.0) % 360.0 - 180.0)


def forward_vector(rotation: Rot3) -> Vec3:
    """Kijkrichting bij een rotatie, zoals FRotator::Vector() in Unreal."""
    pitch = math.radians(rotation[1])
    yaw = math.radians(rotation[2])
    return (math.cos(pitch) * math.cos(yaw), math.cos(pitch) * math.sin(yaw), math.sin(pitch))


def look_at_rotation(source: Vec3, target: Vec3) -> Rot3:
    dx, dy, dz = (t - s for s, t in zip(source, target))
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy)))
    return (0.0, pitch, yaw)


def angle_between(u: Sequence[float], v: Sequence[float]) -> float:
    nu = math.sqrt(sum(x * x for x in u))
    nv = math.sqrt(sum(x * x for x in v))
    if nu == 0.0 or nv == 0.0:
        return 0.0
    cos = sum(x * y for x, y in zip(u, v)) / (nu * nv)
    return math.degrees(math.acos(max(-1.0, min(1.0, cos))))


def fmt(value: float) -> str:
    """Getal zoals een student het in het Details-paneel ziet: hooguit één decimaal."""
    rounded = round(float(value), 1)
    if rounded == 0:
        rounded = 0.0  # geen '-0'
    return str(int(rounded)) if rounded.is_integer() else str(rounded)


class Scene:
    """Interface. Subklassen implementeren de methoden die NotImplementedError geven."""

    # --- lezen ---
    def actors(self) -> List[ActorView]:
        raise NotImplementedError

    def selected_labels(self) -> List[str]:
        raise NotImplementedError

    def camera(self) -> Optional[Tuple[Vec3, Rot3]]:
        raise NotImplementedError

    def get_property(self, actor: ActorView, path: str) -> Any:
        """``path`` gebruikt Python-namen, bv. ``point_light_component.attenuation_radius``."""
        raise NotImplementedError

    def material_input(self, material_path: str, input_name: str) -> Optional[Tuple[str, Optional[str]]]:
        """(klassenaam van de node aan deze ingang, parameternaam of None), of None als er niets aan hangt."""
        raise NotImplementedError

    def blueprint_exec_linked(self, blueprint_path: str, event: str, function: str) -> bool:
        raise NotImplementedError

    # --- schrijven ---
    def load_starter_map(self, map_path: str) -> None:
        """Open een *kopie* van de startmap; het origineel blijft onaangeroerd."""
        raise NotImplementedError

    def backup_level(self, asset_path: str) -> bool:
        """Bewaar het huidige level als het gewijzigd is. True als er iets is opgeslagen."""
        raise NotImplementedError

    def spawn_static_mesh(self, mesh_path: str, label: str, location: Vec3,
                          rotation: Rot3 = (0.0, 0.0, 0.0), scale: Vec3 = (1.0, 1.0, 1.0)) -> None:
        raise NotImplementedError

    def spawn_actor(self, class_name: str, location: Vec3, rotation: Rot3 = (0.0, 0.0, 0.0),
                    label: Optional[str] = None) -> None:
        raise NotImplementedError

    def destroy(self, selector: Selector) -> None:
        raise NotImplementedError

    def set_transform(self, selector: Selector, location: Optional[Vec3] = None,
                      rotation: Optional[Rot3] = None, scale: Optional[Vec3] = None) -> None:
        raise NotImplementedError

    def set_property(self, selector: Selector, path: str, value: Any) -> None:
        raise NotImplementedError

    def select(self, labels: Sequence[str]) -> None:
        raise NotImplementedError

    def set_camera(self, location: Vec3, rotation: Rot3) -> None:
        raise NotImplementedError

    # --- gedeelde hulpfuncties ---
    def find(self, selector: Selector) -> List[ActorView]:
        return [actor for actor in self.actors() if matches(actor, selector)]

    def find_one(self, selector: Selector) -> ActorView:
        found = self.find(selector)
        if not found:
            raise LookupError(f"Geen actor gevonden voor {selector}")
        return found[0]

    def _require_camera(self) -> Tuple[Vec3, Rot3]:
        camera = self.camera()
        if camera is None:
            raise RuntimeError("Geen actieve viewport-camera")
        return camera

    def rotate_camera(self, yaw: float = 0.0, pitch: float = 0.0) -> None:
        location, rotation = self._require_camera()
        self.set_camera(location, (rotation[0], rotation[1] + pitch, rotation[2] + yaw))

    def move_camera_forward(self, amount: float) -> None:
        location, rotation = self._require_camera()
        forward = forward_vector(rotation)
        self.set_camera(tuple(l + f * amount for l, f in zip(location, forward)), rotation)

    def focus_on(self, selector: Selector, distance_to_target: float = 400.0) -> None:
        """Doet wat F in de editor doet: de camera vliegt naar de actor en kijkt ernaar."""
        target = self.find_one(selector).location
        location, _ = self._require_camera()
        direction = [t - l for l, t in zip(location, target)]
        length = math.sqrt(sum(d * d for d in direction))
        if length < 1e-3:
            direction, length = [1.0, 0.0, -0.5], math.sqrt(1.25)
        new_location = tuple(t - d / length * distance_to_target for t, d in zip(target, direction))
        self.set_camera(new_location, look_at_rotation(new_location, target))
