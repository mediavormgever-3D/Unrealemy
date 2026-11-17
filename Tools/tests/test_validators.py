"""Losse tests voor validatortypen die (nog) niet in een hoofdstuk voorkomen of randgevallen hebben."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

PLUGIN = Path(__file__).resolve().parents[2] / "Plugins" / "CurriculumCore"
sys.path.insert(0, str(PLUGIN / "Content" / "Python"))

from unrealemy.fake_scene import FakeScene  # noqa: E402
from unrealemy.scene import angle_diff, fmt, look_at_rotation, forward_vector, angle_between  # noqa: E402
from unrealemy.validators import Context, evaluate  # noqa: E402


def scene_with_cube():
    scene = FakeScene()
    scene.load_starter_map("/Engine/Maps/Templates/Template_Default")
    scene.spawn_static_mesh("/Engine/BasicShapes/Cube.Cube", "Kubus", (0.0, 0.0, 50.0))
    return scene


class MathTest(unittest.TestCase):
    def test_angle_diff_wraps(self):
        self.assertAlmostEqual(angle_diff(359.0, 1.0), 2.0)
        self.assertAlmostEqual(angle_diff(-315.0, 45.0), 0.0)

    def test_look_at_points_forward(self):
        rotation = look_at_rotation((0, 0, 0), (100, 100, 0))
        self.assertAlmostEqual(angle_between(forward_vector(rotation), (1, 1, 0)), 0.0, places=4)

    def test_fmt(self):
        self.assertEqual(fmt(50.0), "50")
        self.assertEqual(fmt(12.345), "12.3")
        self.assertEqual(fmt(-0.01), "0")


class ValidatorTest(unittest.TestCase):
    def test_transform_rotation_wraps_around(self):
        scene = scene_with_cube()
        scene.set_transform({"label": "Kubus"}, rotation=(0.0, 0.0, -315.0))
        spec = {"type": "transform_near", "actor": {"label": "Kubus"}, "rotation": [0, 0, 45]}
        self.assertTrue(evaluate(spec, Context(scene)).passed)

    def test_missing_actor_gives_reset_hint(self):
        scene = FakeScene()
        scene.load_starter_map("/x")
        result = evaluate({"type": "transform_near", "actor": {"label": "Kubus"}, "location": [0, 0, 0]}, Context(scene))
        self.assertFalse(result.passed)
        self.assertIn("Reset stap", result.message)

    def test_any_and_message_override(self):
        scene = scene_with_cube()
        spec = {"any": [
            {"type": "actor_exists", "actor": {"class": "SpotLight"}},
            {"type": "actor_exists", "actor": {"class": "PointLight"}},
        ], "message": "Plaats een lamp."}
        self.assertEqual(evaluate(spec, Context(scene)).message, "Plaats een lamp.")
        scene.spawn_actor("SpotLight", (0, 0, 100))
        self.assertTrue(evaluate(spec, Context(scene)).passed)

    def test_selection_must_be_exactly_the_target(self):
        scene = scene_with_cube()
        spec = {"type": "selection_is", "actor": {"label": "Kubus"}}
        scene.select(["Kubus", "Floor"])
        self.assertFalse(evaluate(spec, Context(scene)).passed)
        scene.select(["Kubus"])
        self.assertTrue(evaluate(spec, Context(scene)).passed)

    def test_property_compare_operators(self):
        scene = scene_with_cube()
        scene.spawn_actor("PointLight", (0, 0, 300))
        base = {"type": "property_compare", "actor": {"class": "PointLight"},
                "property": "point_light_component.intensity"}
        self.assertTrue(evaluate({**base, "op": ">=", "value": 8}, Context(scene)).passed)
        self.assertFalse(evaluate({**base, "op": ">", "value": 8}, Context(scene)).passed)
        self.assertTrue(evaluate({**base, "op": "==", "value": 8.4, "tolerance": 0.5}, Context(scene)).passed)

    def test_material_roughness_on_scalar_parameter(self):
        scene = FakeScene()
        spec = {"type": "material_input_from", "material": "/Game/CurriculumAssets/M_Test", "input": "roughness",
                "expression_class": "MaterialExpressionScalarParameter", "parameter_name": "Roughness"}
        self.assertIn("Apply", evaluate(spec, Context(scene)).message)
        scene.material_inputs[("/Game/CurriculumAssets/M_Test", "roughness")] = ("MaterialExpressionConstant", None)
        self.assertIn("Constant", evaluate(spec, Context(scene)).message)
        scene.material_inputs[("/Game/CurriculumAssets/M_Test", "roughness")] = (
            "MaterialExpressionScalarParameter", "Roughness")
        self.assertTrue(evaluate(spec, Context(scene)).passed)

    def test_blueprint_begin_play_to_print_string(self):
        scene = FakeScene()
        spec = {"type": "blueprint_exec_linked", "blueprint": "/Game/CurriculumAssets/BP_Hello",
                "event": "ReceiveBeginPlay", "function": "PrintString"}
        self.assertFalse(evaluate(spec, Context(scene)).passed)
        scene.blueprint_links.add(("/Game/CurriculumAssets/BP_Hello", "ReceiveBeginPlay", "PrintString"))
        self.assertTrue(evaluate(spec, Context(scene)).passed)

    def test_unknown_type_raises(self):
        with self.assertRaises(ValueError):
            evaluate({"type": "bestaat_niet"}, Context(FakeScene()))


if __name__ == "__main__":
    unittest.main()
