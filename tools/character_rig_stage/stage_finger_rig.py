#!/usr/bin/env python3
"""Stage a singer and donor guitarist for manual finger weight painting.

Run inside Blender.  The singer's existing armature, mesh vertices, object
transforms, materials, and weights are never rewritten.  Only donor finger
bones are grafted below the singer's factual hand bones and empty matching
vertex groups are created on the singer meshes.
"""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


FINGER_TOKENS = ("thumb", "index", "middlefinger", "ringfinger", "pinky")
SIDES = ("L", "R")


def canonical(name: str) -> str:
    return name[:-5] if name.lower().endswith(".mesh") else name


def is_finger(name: str, side: str) -> bool:
    value = canonical(name).lower()
    prefix = f"bone_{side.lower()}-"
    return value.startswith(prefix) and any(token in value for token in FINGER_TOKENS)


def hand_name(side: str) -> str:
    return f"bone_{side}-hand"


def find_bone(armature, wanted: str):
    wanted = canonical(wanted).lower()
    matches = [bone for bone in armature.data.bones if canonical(bone.name).lower() == wanted]
    if len(matches) != 1:
        raise RuntimeError(
            f"{armature.name}: expected one bone matching {wanted}, found {len(matches)}"
        )
    return matches[0]


def finger_bones(armature, side: str):
    hand = find_bone(armature, hand_name(side))
    descendants = set(hand.children_recursive)
    selected = [bone for bone in armature.data.bones if bone in descendants and is_finger(bone.name, side)]
    selected_set = set(selected)
    for bone in selected:
        if bone.parent != hand and bone.parent not in selected_set:
            raise RuntimeError(
                f"{armature.name}: finger bone {bone.name} has non-finger parent {bone.parent.name}"
            )
    if not selected:
        raise RuntimeError(f"{armature.name}: no {side}-hand finger chain found")
    selected.sort(
        key=lambda bone: sum(1 for _ in iter_parent_chain(bone))
    )
    return hand, selected


def iter_parent_chain(bone):
    current = bone.parent
    while current is not None:
        yield current
        current = current.parent


def singer_meshes(imported_objects, armature):
    result = []
    for obj in imported_objects:
        if obj.type != "MESH":
            continue
        owns_armature = obj.parent == armature
        owns_armature |= any(
            modifier.type == "ARMATURE" and modifier.object == armature
            for modifier in obj.modifiers
        )
        if owns_armature:
            result.append(obj)
    if not result:
        raise RuntimeError(f"{armature.name}: no imported skinned meshes found")
    return result


def weighted_points_in_hand_space(meshes, armature, hand, accepted_names):
    accepted = {canonical(name).lower() for name in accepted_names}
    world_to_armature = armature.matrix_world.inverted_safe()
    hand_inverse = hand.matrix_local.inverted_safe()
    points = []
    for obj in meshes:
        if not obj.get("ghogx_primary_bind_space", True):
            continue
        index_to_name = {group.index: canonical(group.name).lower() for group in obj.vertex_groups}
        mesh_to_armature = world_to_armature @ obj.matrix_world
        for vertex in obj.data.vertices:
            if not any(
                assignment.weight > 0.0001 and index_to_name.get(assignment.group) in accepted
                for assignment in vertex.groups
            ):
                continue
            points.append(hand_inverse @ (mesh_to_armature @ vertex.co))
    return points


def bounds(points):
    if not points:
        raise RuntimeError("cannot derive hand bounds: no weighted vertices")
    lo = Vector(tuple(min(point[i] for point in points) for i in range(3)))
    hi = Vector(tuple(max(point[i] for point in points) for i in range(3)))
    extent = hi - lo
    if min(abs(value) for value in extent) < 1.0e-5:
        raise RuntimeError(f"degenerate hand bounds: extent={tuple(extent)}")
    return lo, hi, extent


def rest_signature(armature):
    return {
        bone.name: {
            "parent": bone.parent.name if bone.parent else "",
            "matrix": tuple(value for row in bone.matrix_local for value in row),
        }
        for bone in armature.data.bones
    }


def _hash_text(digest, value):
    encoded = value.encode("utf-8")
    digest.update(struct.pack("<I", len(encoded)))
    digest.update(encoded)


def _hash_vector(digest, values):
    values = tuple(float(value) for value in values)
    digest.update(struct.pack("<I", len(values)))
    digest.update(struct.pack("<" + "d" * len(values), *values))


def geometry_digest(meshes, armature):
    digest = hashlib.sha256()
    _hash_text(digest, armature.name)
    _hash_vector(digest, (value for row in armature.matrix_world for value in row))
    for obj in sorted(meshes, key=lambda item: item.name):
        _hash_text(digest, obj.name)
        _hash_vector(digest, (value for row in obj.matrix_world for value in row))
        for vertex in obj.data.vertices:
            _hash_vector(digest, vertex.co)
        for key in obj.data.shape_keys.key_blocks if obj.data.shape_keys else ():
            _hash_text(digest, key.name)
            for point in key.data:
                _hash_vector(digest, point.co)
        for slot in obj.material_slots:
            _hash_text(digest, slot.material.name if slot.material else "")
    return digest.hexdigest()


def existing_weight_groups(meshes):
    return {
        obj.name: tuple(sorted(group.name for group in obj.vertex_groups))
        for obj in meshes
    }


def weight_digest(meshes, groups_by_mesh):
    digest = hashlib.sha256()
    for obj in sorted(meshes, key=lambda item: item.name):
        _hash_text(digest, obj.name)
        names = groups_by_mesh[obj.name]
        index_to_name = {
            group.index: group.name for group in obj.vertex_groups if group.name in names
        }
        for vertex in obj.data.vertices:
            digest.update(struct.pack("<I", vertex.index))
            assignments = sorted(
                (index_to_name[row.group], float(row.weight))
                for row in vertex.groups
                if row.group in index_to_name
            )
            digest.update(struct.pack("<I", len(assignments)))
            for name, weight in assignments:
                _hash_text(digest, name)
                digest.update(struct.pack("<d", weight))
    return digest.hexdigest()


def assert_new_groups_empty(meshes, names):
    for obj in meshes:
        indices = {
            group.index for group in obj.vertex_groups if group.name in names
        }
        for vertex in obj.data.vertices:
            if any(row.group in indices and row.weight != 0.0 for row in vertex.groups):
                raise RuntimeError(
                    f"new finger group unexpectedly contains weights: {obj.name} vertex {vertex.index}"
                )


def max_existing_rest_delta(before, armature):
    maximum = 0.0
    for name, row in before.items():
        bone = armature.data.bones.get(name)
        if bone is None:
            raise RuntimeError(f"pre-existing singer bone disappeared: {name}")
        if (bone.parent.name if bone.parent else "") != row["parent"]:
            raise RuntimeError(f"pre-existing singer parent changed: {name}")
        now = tuple(value for matrix_row in bone.matrix_local for value in matrix_row)
        maximum = max(maximum, max(abs(a - b) for a, b in zip(row["matrix"], now)))
    return maximum


@dataclass
class BoneStage:
    name: str
    parent: str
    head: Vector
    tail: Vector
    roll_axis: Vector
    connected: bool


def map_hand_point(point, donor_lo, singer_lo, scales):
    return Vector(
        singer_lo[index] + (point[index] - donor_lo[index]) * scales[index]
        for index in range(3)
    )


def stage_side(singer, donor, singer_mesh_list, donor_mesh_list, side, scale_mode):
    singer_hand = find_bone(singer, hand_name(side))
    donor_hand, donor_fingers = finger_bones(donor, side)
    singer_hand_name = singer_hand.name
    donor_hand_name = donor_hand.name
    existing = {
        canonical(bone.name).lower(): bone.name for bone in singer.data.bones
    }
    retained = {
        bone.name: existing[canonical(bone.name).lower()]
        for bone in donor_fingers
        if canonical(bone.name).lower() in existing
    }

    singer_points = weighted_points_in_hand_space(
        singer_mesh_list,
        singer,
        singer_hand,
        [singer_hand.name] + list(retained.values()),
    )
    donor_names = [donor_hand.name] + [bone.name for bone in donor_fingers]
    donor_points = weighted_points_in_hand_space(
        donor_mesh_list, donor, donor_hand, donor_names
    )
    singer_lo, singer_hi, singer_extent = bounds(singer_points)
    donor_lo, donor_hi, donor_extent = bounds(donor_points)
    if scale_mode == "none":
        scales = Vector((1.0, 1.0, 1.0))
    elif scale_mode == "uniform":
        ratios = sorted(singer_extent[i] / donor_extent[i] for i in range(3))
        scales = Vector((ratios[1],) * 3)
    else:
        scales = Vector(tuple(singer_extent[i] / donor_extent[i] for i in range(3)))

    donor_hand_inverse = donor_hand.matrix_local.inverted_safe()
    singer_hand_matrix = singer_hand.matrix_local.copy()
    stages = []
    donor_finger_names = {bone.name for bone in donor_fingers}
    for bone in donor_fingers:
        if bone.name in retained:
            continue
        head_hand = donor_hand_inverse @ bone.head_local
        tail_hand = donor_hand_inverse @ bone.tail_local
        mapped_head = singer_hand_matrix @ map_hand_point(
            head_hand, donor_lo, singer_lo, scales
        )
        mapped_tail = singer_hand_matrix @ map_hand_point(
            tail_hand, donor_lo, singer_lo, scales
        )
        if (mapped_tail - mapped_head).length < 1.0e-5:
            raise RuntimeError(f"retarget collapsed donor bone {bone.name}")
        donor_roll = donor_hand_inverse.to_3x3() @ (
            bone.matrix_local.to_3x3() @ Vector((0.0, 0.0, 1.0))
        )
        singer_roll = (singer_hand_matrix.to_3x3() @ donor_roll).normalized()
        parent = (
            retained.get(bone.parent.name, bone.parent.name)
            if bone.parent and bone.parent.name in donor_finger_names
            else singer_hand_name
        )
        stages.append(
            BoneStage(
                bone.name,
                parent,
                mapped_head,
                mapped_tail,
                singer_roll,
                bool(bone.use_connect and bone.parent and bone.parent.name in donor_finger_names),
            )
        )

    bpy.context.view_layer.objects.active = singer
    singer.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    created = {
        donor_name: singer.data.edit_bones.get(singer_name)
        for donor_name, singer_name in retained.items()
    }
    try:
        for stage in stages:
            bone = singer.data.edit_bones.new(stage.name)
            bone.head = stage.head
            bone.tail = stage.tail
            bone.parent = created.get(stage.parent) or singer.data.edit_bones.get(stage.parent)
            if bone.parent is None:
                raise RuntimeError(f"missing staged parent {stage.parent} for {stage.name}")
            bone.use_connect = stage.connected
            bone.align_roll(stage.roll_axis)
            created[stage.name] = bone
    finally:
        bpy.ops.object.mode_set(mode="OBJECT")

    for name in (stage.name for stage in stages):
        singer.data.bones[name]["ghogx_grafted_finger"] = True
        singer.data.bones[name]["ghogx_donor_armature"] = donor.name
        for mesh in singer_mesh_list:
            if mesh.vertex_groups.get(name) is None:
                mesh.vertex_groups.new(name=name)

    return {
        "side": side,
        # Blender invalidates Bone references across edit-mode transitions;
        # retain the factual names captured before the graft.
        "singer_hand": singer_hand_name,
        "donor_hand": donor_hand_name,
        "singer_bounds": {"min": list(singer_lo), "max": list(singer_hi)},
        "donor_bounds": {"min": list(donor_lo), "max": list(donor_hi)},
        "hand_space_scale": list(scales),
        "retained_source_finger_bones": list(retained.values()),
        "grafted_bones": [stage.name for stage in stages],
    }


def select_armature(imported_objects, requested, role, require_fingers):
    candidates = [obj for obj in imported_objects if obj.type == "ARMATURE"]
    if requested:
        candidates = [
            obj for obj in candidates if obj.name == requested or obj.data.name == requested
        ]
    qualified = []
    for armature in candidates:
        try:
            for side in SIDES:
                find_bone(armature, hand_name(side))
                if require_fingers:
                    finger_bones(armature, side)
            qualified.append(armature)
        except RuntimeError:
            pass
    if len(qualified) != 1:
        raise RuntimeError(
            f"expected one qualified {role} armature, found {[obj.name for obj in qualified]}"
        )
    return qualified[0]


def import_gltf(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(path))
    return [obj for obj in bpy.data.objects if obj not in before]


def _exact_key(name):
    return name.casefold()


def _object_map(imported_objects):
    result = {}
    for obj in imported_objects:
        key = _exact_key(obj.name)
        if key in result:
            raise RuntimeError(f"ambiguous imported object name: {obj.name}")
        result[key] = obj
    return result


def _remove_object_preserving_children(obj):
    for child in list(obj.children):
        world = child.matrix_world.copy()
        child.parent = None
        child.matrix_world = world
    bpy.data.objects.remove(obj, do_unlink=True)


def _serialized_xfm(values):
    """Convert the engine's row-vector 4x3 Xfm to Blender column-vector form."""
    if len(values) != 12:
        raise RuntimeError(f"expected 12 serialized transform values, found {len(values)}")
    return Matrix(
        (
            (values[0], values[3], values[6], values[9]),
            (values[1], values[4], values[7], values[10]),
            (values[2], values[5], values[8], values[11]),
            (0.0, 0.0, 0.0, 1.0),
        )
    )


def _signed_axis_matrices():
    for permutation in itertools.permutations(range(3)):
        for signs in itertools.product((-1.0, 1.0), repeat=3):
            matrix = Matrix.Identity(4)
            for row in range(3):
                for column in range(3):
                    matrix[row][column] = 0.0
                matrix[row][permutation[row]] = signs[row]
            yield matrix


def _fit_interval_translation(intervals):
    """Least-squares translation that moves a point into each factual interval."""
    low = min(row[0] for row in intervals)
    high = max(row[1] for row in intervals)
    for _ in range(80):
        value = (low + high) * 0.5
        derivative = 0.0
        for lo, hi in intervals:
            if value < lo:
                derivative += value - lo
            elif value > hi:
                derivative += value - hi
        if derivative < 0.0:
            low = value
        else:
            high = value
    return (low + high) * 0.5


def _point_bounds_distance(point, lo, hi):
    return math.sqrt(
        sum(max(lo[index] - point[index], 0.0, point[index] - hi[index]) ** 2 for index in range(3))
    )


def _fit_weighted_bind_alignment(bind_matrices, weighted_points):
    rows = []
    for key, matrix in bind_matrices.items():
        points = weighted_points.get(key, ())
        if not points:
            continue
        lo = Vector(tuple(min(point[index] for point in points) for index in range(3)))
        hi = Vector(tuple(max(point[index] for point in points) for index in range(3)))
        rows.append((key, matrix.translation.copy(), lo, hi))
    if len(rows) < 4:
        raise RuntimeError(f"cannot align native bind rig: only {len(rows)} weighted bones")

    best = None
    for axis in _signed_axis_matrices():
        transformed = [(key, axis @ point, lo, hi) for key, point, lo, hi in rows]
        translation = Vector(
            tuple(
                _fit_interval_translation(
                    [(lo[index] - point[index], hi[index] - point[index]) for _, point, lo, hi in transformed]
                )
                for index in range(3)
            )
        )
        distances = [
            _point_bounds_distance(point + translation, lo, hi)
            for _, point, lo, hi in transformed
        ]
        score = sum(distance * distance for distance in distances)
        candidate = (score, max(distances), axis, translation, distances)
        if best is None or candidate[0] < best[0] - 1.0e-9:
            best = candidate

    score, maximum, axis, translation, distances = best
    all_points = [point for points in weighted_points.values() for point in points]
    lo = Vector(tuple(min(point[index] for point in all_points) for index in range(3)))
    hi = Vector(tuple(max(point[index] for point in all_points) for index in range(3)))
    diagonal = (hi - lo).length
    rms = math.sqrt(score / len(distances))
    if diagonal <= 1.0e-6 or rms > diagonal * 0.08 or maximum > diagonal * 0.16:
        raise RuntimeError(
            "native bind alignment does not overlap weighted geometry: "
            f"rms={rms:.6g} max={maximum:.6g} diagonal={diagonal:.6g}"
        )
    alignment = Matrix.Translation(translation) @ axis
    return alignment, {
        "method": "source-offset-weighted-bounds",
        "weighted_bones": len(rows),
        "axis_rows": [list(axis[row][:3]) for row in range(3)],
        "translation": list(translation),
        "rms_distance_to_weight_bounds": rms,
        "max_distance_to_weight_bounds": maximum,
        "weighted_geometry_diagonal": diagonal,
    }


def _fit_source_node_alignment(object_rows, final_bind_matrices):
    rows = [
        (object_rows[key].matrix_world.translation.copy(), matrix.translation.copy())
        for key, matrix in final_bind_matrices.items()
        if key in object_rows
    ]
    if len(rows) < 3:
        raise RuntimeError("cannot align unweighted native rig nodes")
    best = None
    for axis in _signed_axis_matrices():
        offsets = [target - (axis @ source) for source, target in rows]
        translation = sum(offsets, Vector((0.0, 0.0, 0.0))) / len(offsets)
        residuals = [((axis @ source) + translation - target).length for source, target in rows]
        score = sum(value * value for value in residuals)
        candidate = (score, max(residuals), axis, translation)
        if best is None or candidate[0] < best[0] - 1.0e-9:
            best = candidate
    score, maximum, axis, translation = best
    return Matrix.Translation(translation) @ axis, {
        "matched_nodes": len(rows),
        "rms_position_residual": math.sqrt(score / len(rows)),
        "max_position_residual": maximum,
    }


def _derive_native_bind_matrices(rig, object_rows, bone_keys, excluded, role):
    candidates = {}
    for mesh in rig["meshes"]:
        key = _exact_key(mesh["name"])
        if key in excluded or not mesh.get("has_bones"):
            continue
        obj = object_rows.get(key)
        if obj is None or obj.type != "MESH":
            continue
        if len(obj.data.vertices) != mesh["vertex_count"]:
            raise RuntimeError(
                f"{role}: vertex count changed for {mesh['name']}: "
                f"MILO={mesh['vertex_count']} glTF={len(obj.data.vertices)}"
            )
        slot_points = {index: [] for index in range(len(mesh["bone_slots"]))}
        for vertex_index, weights in enumerate(mesh["weights"]):
            point = obj.matrix_world @ obj.data.vertices[vertex_index].co
            for slot_index, weight in enumerate(weights):
                if weight > 0.0001 and slot_index in slot_points:
                    slot_points[slot_index].append(point)
        for slot_index, slot in enumerate(mesh["bone_slots"]):
            if not slot["name"]:
                continue
            bone_key = _exact_key(slot["name"])
            offset = _serialized_xfm(slot["offset"])
            candidates.setdefault(bone_key, []).append(
                {
                    "matrix": obj.matrix_world @ offset.inverted_safe(),
                    "mesh": mesh["name"],
                    "points": slot_points[slot_index],
                }
            )

    raw_bind = {}
    weighted_points = {}
    max_candidate_delta = 0.0
    rejected_candidates = 0
    selected_mesh_votes = {}
    rejected_mesh_votes = {}
    for key, rows in candidates.items():
        if key not in bone_keys:
            continue
        clusters = []
        for row in rows:
            for cluster in clusters:
                delta = max(
                    abs(a - b)
                    for first_row, row_values in zip(cluster[0]["matrix"], row["matrix"])
                    for a, b in zip(first_row, row_values)
                )
                if delta <= 1.0e-3:
                    cluster.append(row)
                    break
            else:
                clusters.append([row])
        clusters.sort(
            key=lambda cluster: (
                len(cluster),
                sum(len(row["points"]) for row in cluster),
            ),
            reverse=True,
        )
        if len(clusters) > 1:
            first_score = (
                len(clusters[0]),
                sum(len(row["points"]) for row in clusters[0]),
            )
            second_score = (
                len(clusters[1]),
                sum(len(row["points"]) for row in clusters[1]),
            )
            if first_score == second_score:
                raise RuntimeError(
                    f"{role}: ambiguous source bind clusters for {key}: {first_score}"
                )
        chosen = clusters[0]
        first = chosen[0]["matrix"]
        for row in chosen[1:]:
            max_candidate_delta = max(
                max_candidate_delta,
                max(
                    abs(a - b)
                    for first_row, row_values in zip(first, row["matrix"])
                    for a, b in zip(first_row, row_values)
                ),
            )
        raw_bind[key] = first
        weighted_points[key] = [point for row in chosen for point in row["points"]]
        for row in chosen:
            mesh_key = _exact_key(row["mesh"])
            selected_mesh_votes[mesh_key] = selected_mesh_votes.get(mesh_key, 0) + 1
        for cluster in clusters[1:]:
            for row in cluster:
                mesh_key = _exact_key(row["mesh"])
                rejected_mesh_votes[mesh_key] = rejected_mesh_votes.get(mesh_key, 0) + 1
        rejected_candidates += sum(len(cluster) for cluster in clusters[1:])
    if not raw_bind:
        raise RuntimeError(f"{role}: no source bone offsets resolved against visible meshes")

    alignment, alignment_report = _fit_weighted_bind_alignment(raw_bind, weighted_points)
    geometry_bind = {key: alignment @ matrix for key, matrix in raw_bind.items()}
    source_to_geometry, source_report = _fit_source_node_alignment(object_rows, geometry_bind)
    geometry_to_source = source_to_geometry.inverted_safe()
    source_bind = {key: object_rows[key].matrix_world.copy() for key in bone_keys}
    alignment_report["max_cross_mesh_bind_delta"] = max_candidate_delta
    alignment_report["rejected_alternate_space_candidates"] = rejected_candidates
    alignment_report["primary_bind_meshes"] = sorted(
        mesh_key
        for mesh_key, selected in selected_mesh_votes.items()
        if selected > rejected_mesh_votes.get(mesh_key, 0)
    )
    alignment_report["alternate_bind_meshes"] = sorted(
        mesh_key
        for mesh_key, rejected in rejected_mesh_votes.items()
        if rejected >= selected_mesh_votes.get(mesh_key, 0)
    )
    alignment_report["source_node_fit"] = source_report
    alignment_report["source_offset_bones"] = len(raw_bind)
    alignment_report["native_source_bones"] = len(source_bind)
    alignment_report["geometry_to_native_matrix"] = [
        list(geometry_to_source[row]) for row in range(4)
    ]
    return source_bind, alignment_report, geometry_to_source


def build_native_armature(imported_objects, rig_path, role, excluded_roots):
    rig = json.loads(rig_path.read_text(encoding="utf-8"))
    if rig.get("schema") != "ghogx.milo-rig-stage.v1":
        raise RuntimeError(f"unsupported native rig sidecar: {rig_path}")
    node_rows = {_exact_key(row["name"]): row for row in rig["nodes"]}
    object_rows = _object_map(imported_objects)
    live_names = {obj.name for obj in imported_objects}

    excluded = {_exact_key(name) for name in excluded_roots}
    changed = True
    while changed:
        changed = False
        for key, row in node_rows.items():
            if key not in excluded and _exact_key(row.get("parent", "")) in excluded:
                excluded.add(key)
                changed = True
    removed = []
    for key in sorted(excluded):
        obj = object_rows.get(key)
        if obj is not None:
            removed.append(obj.name)
            live_names.discard(obj.name)
            _remove_object_preserving_children(obj)
    imported_objects[:] = [bpy.data.objects[name] for name in live_names if name in bpy.data.objects]
    object_rows = _object_map(imported_objects)

    bone_keys = {
        key
        for key, row in node_rows.items()
        if canonical(row["name"]).lower().startswith("bone_") and key not in excluded
    }
    for mesh in rig["meshes"]:
        if not mesh.get("has_bones") or _exact_key(mesh["name"]) in excluded:
            continue
        for slot in mesh["bone_slots"]:
            if slot["name"]:
                bone_keys.add(_exact_key(slot["name"]))
    pending = list(bone_keys)
    while pending:
        key = pending.pop()
        row = node_rows.get(key)
        if row is None:
            raise RuntimeError(f"{role}: referenced bone node is absent: {key}")
        parent = _exact_key(row.get("parent", ""))
        if parent and parent in node_rows and parent not in excluded and parent not in bone_keys:
            bone_keys.add(parent)
            pending.append(parent)

    missing_nodes = [node_rows[key]["name"] for key in bone_keys if key not in object_rows]
    if missing_nodes:
        raise RuntimeError(f"{role}: glTF omitted rig nodes: {missing_nodes}")

    def depth(key):
        count = 0
        seen = set()
        while key in node_rows:
            if key in seen:
                raise RuntimeError(f"{role}: cyclic node hierarchy at {node_rows[key]['name']}")
            seen.add(key)
            parent = _exact_key(node_rows[key].get("parent", ""))
            if not parent or parent not in bone_keys:
                return count
            key = parent
            count += 1
        return count

    ordered = sorted(bone_keys, key=lambda key: (depth(key), node_rows[key]["name"]))
    bind_matrices, bind_alignment, geometry_to_native = _derive_native_bind_matrices(
        rig, object_rows, bone_keys, excluded, role
    )
    primary_bind_meshes = set(bind_alignment["primary_bind_meshes"])
    imported_set = set(imported_objects)
    normalized_roots = [
        obj for obj in imported_objects if obj.parent is None or obj.parent not in imported_set
    ]
    for obj in normalized_roots:
        obj.matrix_world = geometry_to_native @ obj.matrix_world
    normalized_meshes = 0
    for key, obj in object_rows.items():
        if obj.type == "MESH":
            obj["ghogx_primary_bind_space"] = key in primary_bind_meshes
            obj["ghogx_geometry_normalized_to_native_rig"] = True
            normalized_meshes += 1
    armature_data = bpy.data.armatures.new(f"{role.title()}NativeRig")
    armature = bpy.data.objects.new(f"{role.title()}Armature", armature_data)
    bpy.context.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    created = {}
    try:
        for key in ordered:
            row = node_rows[key]
            matrix = bind_matrices[key]
            head = matrix.translation.copy()
            child_distances = []
            for child_key in bone_keys:
                if _exact_key(node_rows[child_key].get("parent", "")) == key:
                    distance = (bind_matrices[child_key].translation - head).length
                    if distance > 1.0e-5:
                        child_distances.append(distance)
            parent_key = _exact_key(row.get("parent", ""))
            if child_distances:
                length = min(child_distances)
            elif parent_key in bone_keys:
                length = max(
                    (head - bind_matrices[parent_key].translation).length * 0.7,
                    0.01,
                )
            else:
                length = 0.1
            axis = matrix.to_3x3() @ Vector((0.0, 1.0, 0.0))
            if axis.length < 1.0e-6:
                axis = Vector((0.0, 1.0, 0.0))
            roll_axis = matrix.to_3x3() @ Vector((0.0, 0.0, 1.0))
            bone = armature.data.edit_bones.new(row["name"])
            bone.head = head
            bone.tail = head + axis.normalized() * length
            if roll_axis.length > 1.0e-6:
                bone.align_roll(roll_axis.normalized())
            if parent_key in created:
                bone.parent = created[parent_key]
            created[key] = bone
    finally:
        bpy.ops.object.mode_set(mode="OBJECT")

    skinned = []
    omitted_source_skinned_meshes = []
    source_weight_rows = 0
    mesh_rows = {_exact_key(row["name"]): row for row in rig["meshes"]}
    for key, mesh in mesh_rows.items():
        if key in excluded or not mesh.get("has_bones"):
            continue
        obj = object_rows.get(key)
        if obj is None:
            # Grim follows the authored active View/LOD graph. Retain the
            # omitted source rows in the audit instead of manufacturing meshes
            # that the factual glTF scene did not export.
            omitted_source_skinned_meshes.append(mesh["name"])
            continue
        if obj.type != "MESH":
            raise RuntimeError(f"{role}: imported skinned object is not a mesh: {mesh['name']}")
        if len(obj.data.vertices) != mesh["vertex_count"]:
            raise RuntimeError(
                f"{role}: vertex count changed for {mesh['name']}: "
                f"MILO={mesh['vertex_count']} glTF={len(obj.data.vertices)}"
            )
        groups = []
        for slot in mesh["bone_slots"]:
            name = slot["name"]
            groups.append(obj.vertex_groups.get(name) or obj.vertex_groups.new(name=name) if name else None)
        for index, weights in enumerate(mesh["weights"]):
            for slot, weight in enumerate(weights):
                if groups[slot] is not None and weight > 0.0:
                    groups[slot].add([index], float(weight), "REPLACE")
                    source_weight_rows += 1
        modifier = obj.modifiers.new(name="GHOGX Source Skin", type="ARMATURE")
        modifier.object = armature
        obj["ghogx_source_skin_from_milo"] = True
        skinned.append(obj)

    for key in ordered:
        obj = object_rows.get(key)
        mesh = mesh_rows.get(key)
        if obj is not None and (mesh is None or mesh["vertex_count"] == 0):
            live_names.discard(obj.name)
            _remove_object_preserving_children(obj)
    imported_objects[:] = [bpy.data.objects[name] for name in live_names if name in bpy.data.objects]
    armature["ghogx_native_rig_source"] = str(rig_path)
    armature["ghogx_removed_subtree_objects"] = json.dumps(removed)
    return armature, {
        "source": str(rig_path),
        "bones": len(ordered),
        "skinned_meshes": len(skinned),
        "omitted_source_skinned_meshes": omitted_source_skinned_meshes,
        "source_weight_assignments": source_weight_rows,
        "bind_alignment": bind_alignment,
        "geometry_normalized_meshes": normalized_meshes,
        "geometry_normalized_roots": [obj.name for obj in normalized_roots],
        "excluded_subtree_roots": list(excluded_roots),
        "removed_objects": removed,
    }


def stage(
    singer_path,
    donor_path,
    output_path,
    report_path,
    singer_armature,
    donor_armature,
    scale_mode,
    singer_rig_path=None,
    donor_rig_path=None,
    excluded_subtree_roots=(),
):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    singer_objects = import_gltf(singer_path)
    singer_native = None
    if singer_rig_path:
        singer, singer_native = build_native_armature(
            singer_objects, singer_rig_path, "singer", excluded_subtree_roots
        )
    else:
        if excluded_subtree_roots:
            raise RuntimeError("subtree exclusion requires a native rig sidecar")
        singer = select_armature(singer_objects, singer_armature, "singer", False)
    singer_mesh_list = singer_meshes(singer_objects, singer)
    singer_before = rest_signature(singer)
    groups_before = existing_weight_groups(singer_mesh_list)
    geometry_before = geometry_digest(singer_mesh_list, singer)
    weights_before = weight_digest(singer_mesh_list, groups_before)

    donor_objects = import_gltf(donor_path)
    donor_native = None
    if donor_rig_path:
        donor, donor_native = build_native_armature(
            donor_objects, donor_rig_path, "donor", ()
        )
    else:
        donor = select_armature(donor_objects, donor_armature, "donor", True)
    donor_mesh_list = singer_meshes(donor_objects, donor)

    sides = [
        stage_side(
            singer, donor, singer_mesh_list, donor_mesh_list, side, scale_mode
        )
        for side in SIDES
    ]
    rest_delta = max_existing_rest_delta(singer_before, singer)
    if rest_delta > 1.0e-7:
        raise RuntimeError(
            f"singer rest skeleton changed during graft (max delta {rest_delta})"
        )
    geometry_after = geometry_digest(singer_mesh_list, singer)
    weights_after = weight_digest(singer_mesh_list, groups_before)
    if geometry_before != geometry_after:
        raise RuntimeError("singer geometry/object/material digest changed during graft")
    if weights_before != weights_after:
        raise RuntimeError("pre-existing singer weights changed during graft")
    grafted_names = {
        name for side in sides for name in side["grafted_bones"]
    }
    assert_new_groups_empty(singer_mesh_list, grafted_names)

    for obj in donor_objects:
        obj.hide_render = True
        obj.hide_set(True)
        obj["ghogx_donor_reference"] = True
    donor.hide_render = True
    donor.hide_set(True)
    donor["ghogx_donor_reference"] = True
    singer["ghogx_finger_weights_pending"] = True
    singer["ghogx_proportions_preserved"] = True

    output_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output_path))
    report = {
        "schema_version": 1,
        "singer_source": str(singer_path),
        "donor_source": str(donor_path),
        "output_blend": str(output_path),
        "singer_armature": singer.name,
        "donor_armature": donor.name,
        "scale_mode": scale_mode,
        "preexisting_singer_bones": len(singer_before),
        "preexisting_rest_max_delta": rest_delta,
        "singer_geometry_sha256_before": geometry_before,
        "singer_geometry_sha256_after": geometry_after,
        "singer_existing_weights_sha256_before": weights_before,
        "singer_existing_weights_sha256_after": weights_after,
        "singer_vertex_positions_changed": False,
        "singer_existing_weights_changed": False,
        "finger_weights_pending": True,
        "native_singer_import": singer_native,
        "native_donor_import": donor_native,
        "sides": sides,
    }
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(
        "GHOGX_FINGER_STAGE_OK "
        f"singer={singer.name} donor={donor.name} "
        f"grafted={sum(len(side['grafted_bones']) for side in sides)} "
        f"rest_delta={rest_delta:.9g} output={output_path}"
    )


def parse_args(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("--singer", type=Path, required=True)
    parser.add_argument("--donor", type=Path, required=True)
    parser.add_argument("--singer-rig", type=Path)
    parser.add_argument("--donor-rig", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--singer-armature")
    parser.add_argument("--donor-armature")
    parser.add_argument("--exclude-subtree-root", action="append", default=[])
    parser.add_argument(
        "--scale-mode", choices=("bounds", "uniform", "none"), default="bounds"
    )
    return parser.parse_args(argv)


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    args = parse_args(arguments)
    report = args.report or args.output.with_suffix(".finger-graft.json")
    stage(
        args.singer.resolve(),
        args.donor.resolve(),
        args.output.resolve(),
        report.resolve(),
        args.singer_armature,
        args.donor_armature,
        args.scale_mode,
        args.singer_rig.resolve() if args.singer_rig else None,
        args.donor_rig.resolve() if args.donor_rig else None,
        tuple(args.exclude_subtree_root),
    )
