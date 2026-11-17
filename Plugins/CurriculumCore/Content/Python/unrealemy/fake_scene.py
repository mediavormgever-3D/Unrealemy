"""Nagebootste scène zonder Unreal, voor automatische tests van de leerlijnen.

Bootst alleen na wat de validators en de setup-/solve-functies gebruiken.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Sequence, Set, Tuple

from .scene import ActorView, Rot3, Scene, Selector, Vec3, matches

# Wat de standaard-startmap ongeveer bevat. Precieze inhoud is niet belangrijk voor de tests.
TEMPLATE_ACTORS = [
    ("Floor", "StaticMeshActor", (0.0, 0.0, 0.0), (4.0, 4.0, 1.0)),
    ("DirectionalLight", "DirectionalLight", (0.0, 0.0, 400.0), (1.0, 1.0, 1.0)),
    ("SkyLight", "SkyLight", (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
    ("SkyAtmosphere", "SkyAtmosphere", (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
    ("ExponentialHeightFog", "ExponentialHeightFog", (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
    ("PlayerStart", "PlayerStart", (0.0, 0.0, 92.0), (1.0, 1.0, 1.0)),
]

# Standaardwaarden van eigenschappen die validators uitlezen (Python-namen).
CLASS_DEFAULTS: Dict[str, Dict[str, Any]] = {
    "PointLight": {
        "point_light_component.attenuation_radius": 1000.0,
        "point_light_component.intensity": 8.0,
    },
    "SpotLight": {
        "spot_light_component.attenuation_radius": 1000.0,
        "spot_light_component.intensity": 8.0,
    },
}

DEFAULT_CAMERA: Tuple[Vec3, Rot3] = ((-1000.0, 0.0, 400.0), (0.0, -15.0, 0.0))


@dataclass
class FakeActor:
    label: str
    class_name: str
    location: List[float]
    rotation: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    scale: List[float] = field(default_factory=lambda: [1.0, 1.0, 1.0])
    props: Dict[str, Any] = field(default_factory=dict)


class FakeScene(Scene):
    def __init__(self):
        self._actors: List[FakeActor] = []
        self._selected: List[str] = []
        self._camera: Optional[Tuple[Vec3, Rot3]] = DEFAULT_CAMERA
        self.dirty = False
        self.maps_loaded: List[str] = []
        self.backups: List[str] = []
        # Testgegevens voor asset-validators.
        self.material_inputs: Dict[Tuple[str, str], Tuple[str, Optional[str]]] = {}
        self.blueprint_links: Set[Tuple[str, str, str]] = set()

    # --- lezen ---
    def actors(self) -> List[ActorView]:
        return [ActorView(a.label, a.class_name, tuple(a.location), tuple(a.rotation), tuple(a.scale), a)
                for a in self._actors]

    def selected_labels(self) -> List[str]:
        return list(self._selected)

    def camera(self):
        return self._camera

    def get_property(self, actor: ActorView, path: str) -> Any:
        props = actor.handle.props
        if path not in props:
            raise AttributeError(f"{actor.class_name} heeft geen eigenschap {path}")
        return props[path]

    def material_input(self, material_path, input_name):
        return self.material_inputs.get((material_path, input_name))

    def blueprint_exec_linked(self, blueprint_path, event, function):
        return (blueprint_path, event, function) in self.blueprint_links

    # --- schrijven ---
    def load_starter_map(self, map_path: str) -> None:
        self.maps_loaded.append(map_path)
        self._actors = [FakeActor(label, cls, list(loc), scale=list(scale))
                        for label, cls, loc, scale in TEMPLATE_ACTORS]
        self._selected = []
        self._camera = DEFAULT_CAMERA
        self.dirty = False

    def backup_level(self, asset_path: str) -> bool:
        if not self.dirty:
            return False
        self.backups.append(asset_path)
        return True

    def _unique_label(self, base: str) -> str:
        labels = {a.label for a in self._actors}
        if base not in labels:
            return base
        number = 2
        while f"{base}{number}" in labels:
            number += 1
        return f"{base}{number}"

    def spawn_static_mesh(self, mesh_path, label, location, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
        self._actors.append(FakeActor(self._unique_label(label), "StaticMeshActor", list(location),
                                      list(rotation), list(scale),
                                      {"static_mesh_component.static_mesh": mesh_path}))
        self.dirty = True

    def spawn_actor(self, class_name, location, rotation=(0.0, 0.0, 0.0), label=None):
        self._actors.append(FakeActor(self._unique_label(label or class_name), class_name, list(location),
                                      list(rotation), props=dict(CLASS_DEFAULTS.get(class_name, {}))))
        self.dirty = True

    def destroy(self, selector: Selector) -> None:
        doomed = {a.label for a in self.find(selector)}
        self._actors = [a for a in self._actors if a.label not in doomed]
        self._selected = [label for label in self._selected if label not in doomed]
        self.dirty = self.dirty or bool(doomed)

    def set_transform(self, selector, location=None, rotation=None, scale=None):
        actor = self.find_one(selector).handle
        if location is not None:
            actor.location = list(location)
        if rotation is not None:
            actor.rotation = list(rotation)
        if scale is not None:
            actor.scale = list(scale)
        self.dirty = True

    def set_property(self, selector, path, value):
        actor = self.find_one(selector).handle
        if path not in actor.props:
            raise AttributeError(f"{actor.class_name} heeft geen eigenschap {path}")
        actor.props[path] = value
        self.dirty = True

    def select(self, labels: Sequence[str]) -> None:
        known = {a.label for a in self._actors}
        self._selected = [label for label in labels if label in known]

    def set_camera(self, location, rotation):
        self._camera = (tuple(location), tuple(rotation))
