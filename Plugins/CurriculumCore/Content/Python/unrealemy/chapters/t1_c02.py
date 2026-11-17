"""Track 1 Foundation, hoofdstuk 2: Ordenen en onderdelen.

De student bouwt een lantaarn: een cilinder (Paal) met een bol (Kap) erop.
"""

from ..scene import look_at_rotation

CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"
CUBE = "/Engine/BasicShapes/Cube.Cube"

PAAL = {"label": "Paal"}
KAP = {"label": "Kap"}

START_CAMERA = (-500.0, 300.0, 450.0)


def setup(scene):
    for leftover in ({"class": "PointLight"}, {"label": "Cylinder"}, {"label": "Sphere"}, {"label": "Kubus"}):
        scene.destroy(leftover)
    # Paal: cilinder van 100 hoog, geschaald naar 300; draaipunt in het midden.
    scene.spawn_static_mesh(CYLINDER, "Cylinder", location=(300.0, -300.0, 150.0), scale=(0.3, 0.3, 3.0))
    # Kap: bol met straal 40, bovenop de paal.
    scene.spawn_static_mesh(SPHERE, "Sphere", location=(300.0, -300.0, 340.0), scale=(0.8, 0.8, 0.8))
    # Afleider, zodat de Outliner niet alleen de lantaarn toont.
    scene.spawn_static_mesh(CUBE, "Kubus", location=(-200.0, 200.0, 50.0))
    scene.select([])
    scene.set_camera(START_CAMERA, look_at_rotation(START_CAMERA, (300.0, -300.0, 200.0)))


def solve_s01(scene):
    scene.rename({"mesh": CYLINDER}, "Paal")


def solve_s02(scene):
    scene.rename({"mesh": SPHERE}, "Kap")


def solve_s03(scene):
    scene.set_folder(PAAL, "Lantaarn")


def solve_s04(scene):
    scene.set_folder(KAP, "Lantaarn")


def solve_s05(scene):
    scene.attach(KAP, PAAL)


def solve_s06(scene):
    x, y, z = scene.find_one(PAAL).location
    scene.set_transform(PAAL, location=(x + 250.0, y, z))


def solve_s07(scene):
    scene.select(["Kap"])


def solve_s08(scene):
    scene.add_component(KAP, "PointLightComponent")


def solve_s09(scene):
    scene.set_property(KAP, "static_mesh_component.cast_shadow", False)


def solve_s10(scene):
    scene.set_component_property(KAP, "PointLightComponent", "attenuation_radius", 400.0)


def solve_s11(scene):
    scene.set_transform(PAAL, location=(0.0, 0.0, 150.0))
