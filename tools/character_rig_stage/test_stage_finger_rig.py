"""Headless Blender qualification for the proportion-preserving graft."""

from __future__ import annotations

import sys
from pathlib import Path

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import stage_finger_rig as rig


def create_armature(name, with_fingers):
    data = bpy.data.armatures.new(name + "Data")
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    root = data.edit_bones.new("bone_pelvis.mesh")
    root.head = (0.0, 0.0, 0.0)
    root.tail = (0.0, 1.0, 0.0)
    for side, x in (("L", 3.0), ("R", -3.0)):
        hand = data.edit_bones.new(f"bone_{side}-hand.mesh")
        hand.head = (x, 0.0, 1.0)
        hand.tail = (x, 1.0, 1.0)
        hand.parent = root
        if not with_fingers:
            continue
        for finger_index, token in enumerate(
            ("thumb", "index", "middlefinger", "ringfinger", "pinky")
        ):
            parent = hand
            for segment in range(1, 4):
                bone = data.edit_bones.new(
                    f"bone_{side}-{token}{segment:02d}.mesh"
                )
                y = 0.55 + segment * 0.35
                z = 0.65 + finger_index * 0.16
                bone.head = (x, y, z)
                bone.tail = (x, y + 0.34, z)
                bone.parent = parent
                parent = bone
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.select_set(False)
    return obj


def create_hand_mesh(name, armature, half_extents):
    vertices = []
    faces = []
    side_rows = []
    for side in ("L", "R"):
        hand = rig.find_bone(armature, rig.hand_name(side))
        first = len(vertices)
        for x in (-half_extents[0], half_extents[0]):
            for y in (-half_extents[1], half_extents[1]):
                for z in (-half_extents[2], half_extents[2]):
                    vertices.append(tuple(hand.matrix_local @ Vector((x, y, z))))
        faces.extend(
            tuple(first + index for index in face)
            for face in (
                (0, 1, 3, 2),
                (4, 6, 7, 5),
                (0, 4, 5, 1),
                (2, 3, 7, 6),
                (0, 2, 6, 4),
                (1, 5, 7, 3),
            )
        )
        side_rows.append((side, range(first, len(vertices))))
    mesh_data = bpy.data.meshes.new(name + "Data")
    mesh_data.from_pydata(vertices, [], faces)
    mesh_data.update()
    obj = bpy.data.objects.new(name, mesh_data)
    bpy.context.collection.objects.link(obj)
    modifier = obj.modifiers.new("Armature", "ARMATURE")
    modifier.object = armature
    for side, indices in side_rows:
        group = obj.vertex_groups.new(name=f"bone_{side}-hand.mesh")
        group.add(list(indices), 1.0, "REPLACE")
    return obj


def original_weights(mesh):
    return [
        tuple(sorted((row.group, round(row.weight, 8)) for row in vertex.groups))
        for vertex in mesh.data.vertices
    ]


def export_selected_glb(armature, mesh, path):
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    mesh.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.export_scene.gltf(
        filepath=str(path),
        export_format="GLB",
        use_selection=True,
        export_skins=True,
        export_animations=False,
    )


def main(fixture_dir=None):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    singer = create_armature("Singer", False)
    singer_mesh = create_hand_mesh("SingerMesh", singer, (1.0, 2.0, 0.5))
    donor = create_armature("Donor", True)
    donor_mesh = create_hand_mesh("DonorMesh", donor, (0.5, 1.0, 0.25))

    if fixture_dir is not None:
        fixture_dir.mkdir(parents=True, exist_ok=True)
        export_selected_glb(singer, singer_mesh, fixture_dir / "singer.glb")
        export_selected_glb(donor, donor_mesh, fixture_dir / "donor.glb")

    before_rest = rig.rest_signature(singer)
    before_vertices = [tuple(vertex.co) for vertex in singer_mesh.data.vertices]
    before_weights = original_weights(singer_mesh)
    reports = [
        rig.stage_side(singer, donor, [singer_mesh], [donor_mesh], side, "bounds")
        for side in rig.SIDES
    ]

    assert rig.max_existing_rest_delta(before_rest, singer) == 0.0
    assert before_vertices == [tuple(vertex.co) for vertex in singer_mesh.data.vertices]
    assert before_weights == original_weights(singer_mesh)
    grafted = [name for report in reports for name in report["grafted_bones"]]
    assert len(grafted) == 30
    for report in reports:
        assert max(abs(value - 2.0) for value in report["hand_space_scale"]) < 1.0e-6
    for name in grafted:
        group = singer_mesh.vertex_groups.get(name)
        assert group is not None
        assert all(
            all(assignment.group != group.index for assignment in vertex.groups)
            for vertex in singer_mesh.data.vertices
        )
    print(
        "GHOGX_FINGER_STAGE_TEST_OK "
        f"grafted={len(grafted)} rest_delta=0 vertices_unchanged=1 weights_unchanged=1"
    )


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    fixture_dir = None
    if arguments:
        if len(arguments) != 2 or arguments[0] != "--emit-fixtures":
            raise RuntimeError("expected --emit-fixtures <directory>")
        fixture_dir = Path(arguments[1]).resolve()
    main(fixture_dir)
