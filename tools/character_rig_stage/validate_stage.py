#!/usr/bin/env python3
"""Validate and render a native-origin singer finger-rig staging blend."""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def parse_args(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--blend", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--front", type=Path)
    parser.add_argument("--side", type=Path)
    return parser.parse_args(argv)


def bounds(points):
    return (
        Vector(tuple(min(point[index] for point in points) for index in range(3))),
        Vector(tuple(max(point[index] for point in points) for index in range(3))),
    )


def box_distance(point, points):
    lo, hi = bounds(points)
    return math.sqrt(
        sum(max(lo[index] - point[index], 0.0, point[index] - hi[index]) ** 2 for index in range(3))
    )


def add_line_object(name, segments, color, radius):
    curve = bpy.data.curves.new(name, type="CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = radius
    curve.bevel_resolution = 2
    for start, end in segments:
        spline = curve.splines.new("POLY")
        spline.points.add(1)
        spline.points[0].co = (*start, 1.0)
        spline.points[1].co = (*end, 1.0)
    obj = bpy.data.objects.new(name, curve)
    obj.color = color
    bpy.context.collection.objects.link(obj)
    return obj


def render_view(scene, armature, mesh_lo, mesh_hi, image_path, view):
    diagonal = (mesh_hi - mesh_lo).length
    center = (mesh_lo + mesh_hi) * 0.5
    segments = []
    if view == "front":
        depth = mesh_lo.y - 1.0
        for bone in armature.data.bones:
            head = armature.matrix_world @ bone.head_local
            tail = armature.matrix_world @ bone.tail_local
            segments.append(((head.x, depth, head.z), (tail.x, depth, tail.z)))
        camera_location = (center.x, mesh_lo.y - diagonal * 1.8, center.z)
    else:
        depth = mesh_hi.x + 1.0
        for bone in armature.data.bones:
            head = armature.matrix_world @ bone.head_local
            tail = armature.matrix_world @ bone.tail_local
            segments.append(((depth, head.y, head.z), (depth, tail.y, tail.z)))
        camera_location = (mesh_hi.x + diagonal * 1.8, center.y, center.z)
    proof = add_line_object(
        f"BindSkeletonProof_{view}", segments, (1.0, 0.18, 0.02, 1.0), max(diagonal * 0.0018, 0.05)
    )
    camera_data = bpy.data.cameras.new(f"ProofCamera_{view}")
    camera = bpy.data.objects.new(f"ProofCamera_{view}", camera_data)
    bpy.context.collection.objects.link(camera)
    camera.location = camera_location
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = max(mesh_hi.z - mesh_lo.z, mesh_hi.x - mesh_lo.x, mesh_hi.y - mesh_lo.y) * 1.15
    scene.camera = camera
    scene.render.filepath = str(image_path)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(proof, do_unlink=True)
    bpy.data.objects.remove(camera, do_unlink=True)


def main(args):
    bpy.ops.wm.open_mainfile(filepath=str(args.blend))
    armature = bpy.data.objects.get("SingerArmature")
    if armature is None or armature.type != "ARMATURE":
        raise RuntimeError("SingerArmature is missing")
    meshes = [
        obj
        for obj in bpy.data.objects
        if obj.type == "MESH"
        and obj.get("ghogx_primary_bind_space", True)
        and (
            obj.parent == armature
            or any(mod.type == "ARMATURE" and mod.object == armature for mod in obj.modifiers)
        )
    ]
    mesh_points = [obj.matrix_world @ vertex.co for obj in meshes for vertex in obj.data.vertices]
    mesh_lo, mesh_hi = bounds(mesh_points)
    mesh_diagonal = (mesh_hi - mesh_lo).length
    weighted_distances = {}
    for bone in armature.data.bones:
        points = []
        for obj in meshes:
            group = obj.vertex_groups.get(bone.name)
            if group is None:
                continue
            for vertex in obj.data.vertices:
                if any(row.group == group.index and row.weight > 0.0001 for row in vertex.groups):
                    points.append(obj.matrix_world @ vertex.co)
        if points:
            weighted_distances[bone.name] = box_distance(
                armature.matrix_world @ bone.head_local, points
            )
    finger_bones = [
        bone.name
        for bone in armature.data.bones
        if any(token in bone.name.casefold() for token in ("thumb", "index", "middlefinger", "ringfinger", "pinky"))
    ]
    bone_points = [
        armature.matrix_world @ endpoint
        for bone in armature.data.bones
        for endpoint in (bone.head_local, bone.tail_local)
    ]
    bone_lo, bone_hi = bounds(bone_points)
    report = {
        "blend": str(args.blend),
        "bones": len(armature.data.bones),
        "finger_bones": len(finger_bones),
        "singer_meshes": len(meshes),
        "mesh_bounds": {"min": list(mesh_lo), "max": list(mesh_hi)},
        "mesh_object_bounds": {
            obj.name: {
                "min": list(bounds([obj.matrix_world @ vertex.co for vertex in obj.data.vertices])[0]),
                "max": list(bounds([obj.matrix_world @ vertex.co for vertex in obj.data.vertices])[1]),
                "matrix": [list(obj.matrix_world[row]) for row in range(4)],
            }
            for obj in meshes
        },
        "bone_bounds": {"min": list(bone_lo), "max": list(bone_hi)},
        "weighted_bones_checked": len(weighted_distances),
        "weighted_head_rms_distance": math.sqrt(
            sum(value * value for value in weighted_distances.values()) / len(weighted_distances)
        ),
        "weighted_head_max_distance": max(weighted_distances.values()),
        "weighted_head_distances": weighted_distances,
        "mic_named_objects": sorted(
            obj.name for obj in bpy.data.objects if "mic" in obj.name.casefold()
        ),
    }
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if report["bones"] != 56 or report["finger_bones"] != 30:
        raise RuntimeError("unexpected native/finger bone totals")
    if report["mic_named_objects"]:
        raise RuntimeError(f"microphone objects remain: {report['mic_named_objects']}")
    if report["weighted_head_rms_distance"] > mesh_diagonal * 0.08:
        raise RuntimeError("native skeleton does not overlap weighted singer geometry")
    if args.front or args.side:
        for obj in bpy.data.objects:
            obj.hide_render = obj not in meshes
        for obj in meshes:
            obj.color = (0.65, 0.69, 0.75, 1.0)
        scene = bpy.context.scene
        scene.render.engine = "BLENDER_WORKBENCH"
        scene.display.shading.light = "STUDIO"
        scene.display.shading.color_type = "OBJECT"
        scene.display.shading.show_shadows = True
        scene.display.shading.show_cavity = True
        scene.render.resolution_x = 900
        scene.render.resolution_y = 1100
        scene.render.resolution_percentage = 100
        scene.render.image_settings.file_format = "PNG"
        if args.front:
            render_view(scene, armature, mesh_lo, mesh_hi, args.front, "front")
        if args.side:
            render_view(scene, armature, mesh_lo, mesh_hi, args.side, "side")
    print("GHOGX_STAGE_VALIDATION_OK", json.dumps({key: report[key] for key in (
        "bones", "finger_bones", "weighted_bones_checked",
        "weighted_head_rms_distance", "weighted_head_max_distance"
    )}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    main(parse_args(arguments))
