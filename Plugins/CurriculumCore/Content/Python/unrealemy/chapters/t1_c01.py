"""Track 1 Foundation, hoofdstuk 1: Eerste stappen."""

from ..scene import look_at_rotation

CUBE = {"label": "Kubus"}
LIGHT = {"class": "PointLight"}

START_CAMERA = (-700.0, 0.0, 350.0)


def setup(scene):
    scene.destroy(LIGHT)
    scene.destroy(CUBE)
    scene.spawn_static_mesh("/Engine/BasicShapes/Cube.Cube", label="Kubus", location=(250.0, -200.0, 50.0))
    scene.select([])
    scene.set_camera(START_CAMERA, look_at_rotation(START_CAMERA, (0.0, 0.0, 50.0)))


def solve_s01(scene):
    scene.rotate_camera(yaw=40.0)


def solve_s02(scene):
    scene.move_camera_forward(400.0)


def solve_s03(scene):
    scene.select(["Kubus"])


def solve_s04(scene):
    scene.focus_on(CUBE, 350.0)


def solve_s05(scene):
    x, y, z = scene.find_one(CUBE).location
    scene.set_transform(CUBE, location=(x + 150.0, y, z))


def solve_s06(scene):
    scene.set_transform(CUBE, location=(0.0, 0.0, 0.0))


def solve_s07(scene):
    scene.set_transform(CUBE, location=(0.0, 0.0, 50.0))


def solve_s08(scene):
    scene.set_transform(CUBE, rotation=(0.0, 0.0, 45.0))


def solve_s09(scene):
    scene.set_transform(CUBE, scale=(2.0, 2.0, 2.0))


def solve_s10(scene):
    scene.spawn_actor("PointLight", location=(200.0, 200.0, 200.0))


def solve_s11(scene):
    scene.set_transform(LIGHT, location=(0.0, 0.0, 300.0))


def solve_s12(scene):
    scene.set_property(LIGHT, "point_light_component.attenuation_radius", 500.0)
