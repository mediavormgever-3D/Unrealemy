"""Nagebootste scène zonder Unreal, voor automatische tests van de leerlijnen.

Bootst alleen na wat de validators en de setup-/solve-functies gebruiken.
Vereenvoudiging: een child beweegt mee met de *locatie* van zijn parent,
niet met rotatie of schaal.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Sequence, Set, Tuple

from .scene import ActorView, Rot3, Scene, Selector, Vec3

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

COMPONENT_DEFAULTS: Dict[str, Dict[str, Any]] = {
    "PointLightComponent": {"attenuation_radius": 1000.0, "cast_shadows": True},
    "SpotLightComponent": {"attenuation_radius": 1000.0, "cast_shadows": True},
}

DEFAULT_CAMERA: Tuple[Vec3, Rot3] = ((-1000.0, 0.0, 400.0), (0.0, -15.0, 0.0))


@dataclass
class FakeComponent:
    class_name: str
    props: Dict[str, Any] = field(default_factory=dict)


@dataclass(eq=False)
class FakeActor:
    label: str
    class_name: str
    location: List[float]
    rotation: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    scale: List[float] = field(default_factory=lambda: [1.0, 1.0, 1.0])
    props: Dict[str, Any] = field(default_factory=dict)
    folder: str = ""
    parent: Optional["FakeActor"] = None
    mesh: Optional[str] = None
    components: List[FakeComponent] = field(default_factory=list)


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
        return [ActorView(a.label, a.class_name, tuple(a.location), tuple(a.rotation), tuple(a.scale), a,
                          folder=a.folder, parent=a.parent.label if a.parent else None, mesh=a.mesh)
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

    def components(self, actor: ActorView, class_name: str) -> List[FakeComponent]:
        return [c for c in actor.handle.components if c.class_name == class_name]

    def get_component_property(self, component: FakeComponent, path: str) -> Any:
        if path not in component.props:
            raise AttributeError(f"{component.class_name} heeft geen eigenschap {path}")
        return component.props[path]

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

    def _one(self, selector: Selector) -> FakeActor:
        return self.find_one(selector).handle

    def _descendants(self, actor: FakeActor) -> List[FakeActor]:
        children = [a for a in self._actors if a.parent is actor]
        result = list(children)
        for child in children:
            result.extend(self._descendants(child))
        return result

    def spawn_static_mesh(self, mesh_path, label, location, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
        self._actors.append(FakeActor(
            self._unique_label(label), "StaticMeshActor", list(location), list(rotation), list(scale),
            props={"static_mesh_component.static_mesh": mesh_path, "static_mesh_component.cast_shadow": True},
            mesh=mesh_path))
        self.dirty = True

    def spawn_actor(self, class_name, location, rotation=(0.0, 0.0, 0.0), label=None):
        self._actors.append(FakeActor(self._unique_label(label or class_name), class_name, list(location),
                                      list(rotation), props=dict(CLASS_DEFAULTS.get(class_name, {}))))
        self.dirty = True

    def destroy(self, selector: Selector) -> None:
        doomed = [a.handle for a in self.find(selector)]
        for actor in self._actors:
            if actor.parent in doomed:
                actor.parent = None  # zoals Unreal: children blijven staan, losgekoppeld
        self._actors = [a for a in self._actors if a not in doomed]
        labels = {a.label for a in doomed}
        self._selected = [label for label in self._selected if label not in labels]
        self.dirty = self.dirty or bool(doomed)

    def set_transform(self, selector, location=None, rotation=None, scale=None):
        actor = self._one(selector)
        if location is not None:
            delta = [new - old for old, new in zip(actor.location, location)]
            actor.location = list(location)
            for child in self._descendants(actor):
                child.location = [c + d for c, d in zip(child.location, delta)]
        if rotation is not None:
            actor.rotation = list(rotation)
        if scale is not None:
            actor.scale = list(scale)
        self.dirty = True

    def set_property(self, selector, path, value):
        actor = self._one(selector)
        if path not in actor.props:
            raise AttributeError(f"{actor.class_name} heeft geen eigenschap {path}")
        actor.props[path] = value
        self.dirty = True

    def select(self, labels: Sequence[str]) -> None:
        known = {a.label for a in self._actors}
        self._selected = [label for label in labels if label in known]

    def set_camera(self, location, rotation):
        self._camera = (tuple(location), tuple(rotation))

    def rename(self, selector, new_label):
        actor = self._one(selector)
        self._selected = [new_label if label == actor.label else label for label in self._selected]
        actor.label = new_label
        self.dirty = True

    def set_folder(self, selector, folder):
        self._one(selector).folder = folder
        self.dirty = True

    def attach(self, child, parent):
        child_actor, parent_actor = self._one(child), self._one(parent)
        if child_actor is parent_actor or child_actor in [parent_actor, *self._parents(parent_actor)]:
            raise ValueError("Een actor kan niet aan zichzelf of aan zijn eigen child hangen")
        child_actor.parent = parent_actor
        self.dirty = True

    def _parents(self, actor: FakeActor) -> List[FakeActor]:
        chain = []
        while actor.parent is not None:
            actor = actor.parent
            chain.append(actor)
        return chain

    def add_component(self, selector, class_name):
        self._one(selector).components.append(FakeComponent(class_name, dict(COMPONENT_DEFAULTS.get(class_name, {}))))
        self.dirty = True

    def set_component_property(self, selector, class_name, path, value):
        found = [c for c in self._one(selector).components if c.class_name == class_name]
        if not found:
            raise LookupError(f"Geen {class_name} op {selector}")
        if path not in found[0].props:
            raise AttributeError(f"{class_name} heeft geen eigenschap {path}")
        found[0].props[path] = value
        self.dirty = True
