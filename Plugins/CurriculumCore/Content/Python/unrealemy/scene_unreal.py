"""De echte editor achter de ``Scene``-interface. Alleen bruikbaar binnen Unreal.

Ongetest: geschreven tegen de Python-API van UE 5.x en nog niet gecontroleerd
in UE 5.8. Zie README, paragraaf "Nog te controleren in de editor".
"""

from __future__ import annotations

from typing import Any, List, Optional, Sequence

import unreal

from .scene import ActorView, Scene, Selector

BACKUPS_TO_KEEP = 10


def _actors_sub():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _editor_sub():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


def _vec(values) -> "unreal.Vector":
    return unreal.Vector(float(values[0]), float(values[1]), float(values[2]))


def _rot(values) -> "unreal.Rotator":
    return unreal.Rotator(roll=float(values[0]), pitch=float(values[1]), yaw=float(values[2]))


def _folder(actor) -> str:
    folder = str(actor.get_folder_path())
    return "" if folder in ("", "None") else folder


def _mesh(actor) -> Optional[str]:
    component = getattr(actor, "static_mesh_component", None)  # alleen bij StaticMeshActor
    if component is None:
        return None
    mesh = component.get_editor_property("static_mesh")
    return mesh.get_path_name() if mesh else None


def _view(actor) -> ActorView:
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    parent = actor.get_attach_parent_actor()
    return ActorView(
        label=actor.get_actor_label(),
        class_name=actor.get_class().get_name(),
        location=(loc.x, loc.y, loc.z),
        rotation=(rot.roll, rot.pitch, rot.yaw),
        scale=(scale.x, scale.y, scale.z),
        handle=actor,
        folder=_folder(actor),
        parent=parent.get_actor_label() if parent else None,
        mesh=_mesh(actor),
    )


class UnrealScene(Scene):
    # --- lezen ---
    def actors(self) -> List[ActorView]:
        return [_view(actor) for actor in _actors_sub().get_all_level_actors()]

    def selected_labels(self) -> List[str]:
        return [actor.get_actor_label() for actor in _actors_sub().get_selected_level_actors()]

    def camera(self):
        info = _editor_sub().get_level_viewport_camera_info()
        if not info:
            return None
        if len(info) == 3:  # (gelukt, locatie, rotatie) in sommige versies
            ok, location, rotation = info
            if not ok:
                return None
        else:
            location, rotation = info
        return ((location.x, location.y, location.z), (rotation.roll, rotation.pitch, rotation.yaw))

    def get_property(self, actor: ActorView, path: str) -> Any:
        value = actor.handle
        for part in path.split("."):
            value = value.get_editor_property(part)
        return value

    def material_input(self, material_path, input_name):
        material = unreal.EditorAssetLibrary.load_asset(material_path)
        if material is None:
            raise LookupError(f"Materiaal niet gevonden: {material_path}")
        prop = getattr(unreal.MaterialProperty, "MP_" + input_name.upper())
        node = unreal.MaterialEditingLibrary.get_material_property_input_node(material, prop)
        if node is None:
            return None
        parameter = None
        try:
            parameter = str(node.get_editor_property("parameter_name"))
        except Exception:  # node zonder parameternaam
            pass
        return node.get_class().get_name(), parameter

    def blueprint_exec_linked(self, blueprint_path, event, function):
        blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
        if blueprint is None:
            raise LookupError(f"Blueprint niet gevonden: {blueprint_path}")
        return bool(unreal.CurriculumInspectionLibrary.is_event_linked_to_function(blueprint, event, function, 8))

    def components(self, actor: ActorView, class_name: str) -> List[Any]:
        return list(actor.handle.get_components_by_class(getattr(unreal, class_name)))

    def get_component_property(self, component, path: str) -> Any:
        value = component
        for part in path.split("."):
            value = value.get_editor_property(part)
        return value

    # --- schrijven ---
    def load_starter_map(self, map_path: str) -> None:
        world = None
        try:
            world = unreal.EditorLoadingAndSavingUtils.new_map_from_template(map_path, False)
        except Exception as error:
            unreal.log_warning(f"Unrealemy: startmap {map_path} niet te openen ({error}); lege map wordt gebruikt.")
        if world is None:
            unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
            self._build_fallback_map()

    def _build_fallback_map(self) -> None:
        plane = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
        floor = _actors_sub().spawn_actor_from_object(plane, _vec((0, 0, 0)), _rot((0, 0, 0)))
        floor.set_actor_label("Floor")
        floor.set_actor_scale3d(_vec((20, 20, 1)))
        sun = _actors_sub().spawn_actor_from_class(unreal.DirectionalLight, _vec((0, 0, 500)), _rot((0, -45, 30)))
        sun.set_actor_label("DirectionalLight")
        _actors_sub().spawn_actor_from_class(unreal.SkyLight, _vec((0, 0, 0)), _rot((0, 0, 0)))

    def backup_level(self, asset_path: str) -> bool:
        if not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
            return False
        world = _editor_sub().get_editor_world()
        saved = bool(unreal.EditorLoadingAndSavingUtils.save_map(world, asset_path))
        if saved:
            self._prune_backups(asset_path.rsplit("/", 1)[0])
        return saved

    @staticmethod
    def _prune_backups(folder: str) -> None:
        assets = sorted(unreal.EditorAssetLibrary.list_assets(folder, recursive=False, include_folder=False))
        for old in assets[:-BACKUPS_TO_KEEP]:
            unreal.EditorAssetLibrary.delete_asset(old)

    def spawn_static_mesh(self, mesh_path, label, location, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if mesh is None:
            raise LookupError(f"Mesh niet gevonden: {mesh_path}")
        actor = _actors_sub().spawn_actor_from_object(mesh, _vec(location), _rot(rotation))
        actor.set_actor_label(label)
        actor.set_actor_scale3d(_vec(scale))

    def spawn_actor(self, class_name, location, rotation=(0.0, 0.0, 0.0), label=None):
        actor_class = getattr(unreal, class_name)
        actor = _actors_sub().spawn_actor_from_class(actor_class, _vec(location), _rot(rotation))
        if label:
            actor.set_actor_label(label)

    def destroy(self, selector: Selector) -> None:
        for actor in self.find(selector):
            _actors_sub().destroy_actor(actor.handle)

    def set_transform(self, selector, location=None, rotation=None, scale=None):
        actor = self.find_one(selector).handle
        if location is not None:
            actor.set_actor_location(_vec(location), False, False)
        if rotation is not None:
            actor.set_actor_rotation(_rot(rotation), False)
        if scale is not None:
            actor.set_actor_scale3d(_vec(scale))

    def set_property(self, selector, path, value):
        owner = self.find_one(selector).handle
        parts = path.split(".")
        for part in parts[:-1]:
            owner = owner.get_editor_property(part)
        owner.set_editor_property(parts[-1], value)

    def select(self, labels: Sequence[str]) -> None:
        wanted = set(labels)
        _actors_sub().set_selected_level_actors([a.handle for a in self.actors() if a.label in wanted])

    def set_camera(self, location, rotation):
        _editor_sub().set_level_viewport_camera_info(_vec(location), _rot(rotation))

    def rename(self, selector, new_label):
        self.find_one(selector).handle.set_actor_label(new_label)

    def set_folder(self, selector, folder):
        self.find_one(selector).handle.set_folder_path(folder)

    def attach(self, child, parent):
        keep = unreal.AttachmentRule.KEEP_WORLD
        self.find_one(child).handle.attach_to_actor(self.find_one(parent).handle, "", keep, keep, keep, False)

    def add_component(self, selector, class_name):
        # Zelfde route als '+ Add' in het Details-paneel: een instance-component op een geplaatste actor.
        actor = self.find_one(selector).handle
        subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        handles = subobjects.k2_gather_subobject_data_for_instance(actor)
        params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=getattr(unreal, class_name))
        _, failure = subobjects.add_new_subobject(params)
        if str(failure):
            raise RuntimeError(f"Component {class_name} niet toegevoegd: {failure}")

    def set_component_property(self, selector, class_name, path, value):
        components = self.components(self.find_one(selector), class_name)
        if not components:
            raise LookupError(f"Geen {class_name} op {selector}")
        owner = components[0]
        parts = path.split(".")
        for part in parts[:-1]:
            owner = owner.get_editor_property(part)
        owner.set_editor_property(parts[-1], value)
