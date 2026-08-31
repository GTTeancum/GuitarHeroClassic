"""Stage GH3 Midori mesh IR into GH2 Mesh28-compatible chunks."""

from __future__ import annotations

import argparse
import gzip
import io
import json
import math
import re
from collections import Counter, defaultdict
from contextlib import contextmanager
from pathlib import Path
from typing import Any

from gh3_midori_bone_names import checksum_name_map, fallback_name, resolved_bone_name
from gh3_midori_gh2_bridge import (
    GH3_PS2_MESH_TO_GH2_SCALE,
    GH3_TO_GH2_BONES,
)
from gh3_midori_model_bundle import (
    invert_matrix12_row_vector,
    load_bone_transforms,
    load_stock_mesh_bind_transforms,
    multiply_matrix12_row_vector,
)

SKIN_BONE_SUFFIX = "mesh"


GH3_HEAD_ACCESSORY_BONES = {
    "bone_gh3_b8ca856b",
    "bone_gh3_c8a071e4",
    "bone_gh3_a6bc7033",
}


def gh2_skin_bone_name(name: str) -> str:
    if SKIN_BONE_SUFFIX == "base":
        return name
    if name.startswith("bone_") and not name.startswith("bone_gh3_") and not name.endswith(".mesh"):
        return name + ".mesh"
    return name


def gh3_mesh_position_to_gh2(position: list[float]) -> list[float]:
    x, y_up, z_depth = [float(value) for value in position]
    return [
        -x * GH3_PS2_MESH_TO_GH2_SCALE,
        z_depth * GH3_PS2_MESH_TO_GH2_SCALE,
        y_up * GH3_PS2_MESH_TO_GH2_SCALE,
    ]


def gh3_mesh_normal_to_gh2(normal: list[float]) -> list[float]:
    x, y_up, z_depth = [float(value) for value in normal]
    mapped = [-x, z_depth, y_up]
    length = math.sqrt(sum(value * value for value in mapped))
    if length <= 0.0 or not math.isfinite(length):
        return [0.0, 0.0, 1.0]
    return [value / length for value in mapped]


def transform_point(matrix: list[float], point: list[float]) -> list[float]:
    return [
        point[0] * matrix[c]
        + point[1] * matrix[3 + c]
        + point[2] * matrix[6 + c]
        + matrix[9 + c]
        for c in range(3)
    ]


def transform_direction(matrix: list[float], direction: list[float]) -> list[float]:
    return [
        direction[0] * matrix[c]
        + direction[1] * matrix[3 + c]
        + direction[2] * matrix[6 + c]
        for c in range(3)
    ]


def normalized(values: list[float], fallback: list[float]) -> list[float]:
    length = math.sqrt(sum(value * value for value in values))
    if length <= 0.0 or not math.isfinite(length):
        return list(fallback)
    return [value / length for value in values]


def cross(left: list[float], right: list[float]) -> list[float]:
    return [
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    ]


def hand_anatomical_source_transforms(
    source_transforms: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    """Build GH2-axis-compatible source frames from the authored hand chains.

    GH3's SKE inverse-bind rows and GH2's Trans rows use different local axis
    conventions.  Both formats expose the authored hierarchy, so use each
    joint-to-child direction as local +X (the GH2 hand/finger convention) and
    the palm-facing side as the roll reference.  This is the same topology
    construction community DCC importers use when turning SKE joints into
    edit bones, expressed directly in converter space.
    """
    frames: dict[str, dict[str, Any]] = {}
    for side, normal_hint in (("L", [0.0, -1.0, 0.0]), ("R", [0.0, 1.0, 0.0])):
        hand = f"bone_{side}-hand.mesh"
        chains = [
            [hand, f"bone_{side}-middlefinger01.mesh"],
            *[
                [
                    f"bone_{side}-{finger}01.mesh",
                    f"bone_{side}-{finger}02.mesh",
                    f"bone_{side}-{finger}03.mesh",
                ]
                for finger in ("index", "middlefinger", "ringfinger", "pinky", "thumb")
            ],
        ]
        for chain in chains:
            for index, name in enumerate(chain[:-1]):
                child_name = chain[index + 1]
                source = source_transforms.get(name)
                child = source_transforms.get(child_name)
                if source is None or child is None:
                    continue
                origin = [float(value) for value in source["world"][9:12]]
                child_origin = [float(value) for value in child["world"][9:12]]
                axis_x = normalized(
                    [child_origin[i] - origin[i] for i in range(3)],
                    [1.0, 0.0, 0.0],
                )
                normal_dot = sum(normal_hint[i] * axis_x[i] for i in range(3))
                axis_z = normalized(
                    [normal_hint[i] - normal_dot * axis_x[i] for i in range(3)],
                    [0.0, normal_hint[1], 0.0],
                )
                axis_y = normalized(cross(axis_z, axis_x), [0.0, 0.0, 1.0])
                frames[name] = {
                    **source,
                    "world": axis_x + axis_y + axis_z + origin,
                }
            if len(chain) < 3:
                continue
            terminal_name = chain[-1]
            parent_name = chain[-2]
            terminal = source_transforms.get(terminal_name)
            parent = source_transforms.get(parent_name)
            if terminal is None or parent is None:
                continue
            origin = [float(value) for value in terminal["world"][9:12]]
            parent_origin = [float(value) for value in parent["world"][9:12]]
            axis_x = normalized(
                [origin[i] - parent_origin[i] for i in range(3)],
                [1.0, 0.0, 0.0],
            )
            normal_dot = sum(normal_hint[i] * axis_x[i] for i in range(3))
            axis_z = normalized(
                [normal_hint[i] - normal_dot * axis_x[i] for i in range(3)],
                [0.0, normal_hint[1], 0.0],
            )
            axis_y = normalized(cross(axis_z, axis_x), [0.0, 0.0, 1.0])
            frames[terminal_name] = {
                **terminal,
                "world": axis_x + axis_y + axis_z + origin,
            }
    return frames


def gh3_mesh_uv_to_gh2(uv: list[float]) -> list[float]:
    u, v = [float(value) for value in uv]
    return [u, 1.0 - v]


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


@contextmanager
def deterministic_gzip_text(path: Path):
    with path.open("wb") as raw:
        with gzip.GzipFile(
            filename="",
            mode="wb",
            fileobj=raw,
            compresslevel=9,
            mtime=0,
        ) as compressed:
            with io.TextIOWrapper(
                compressed, encoding="utf-8", newline="\n"
            ) as text:
                yield text


def bone_names_by_index(skeleton_path: Path) -> dict[int, str]:
    data = load_json(skeleton_path)
    skeleton = data.get("skeleton", data)
    name_by_checksum = checksum_name_map(skeleton["bones"])
    return {
        int(bone["index"]): resolved_bone_name(bone, name_by_checksum)
        for bone in skeleton["bones"]
    }


def target_bone_name(source_index: int, source_names: dict[int, str]) -> str:
    source_name = source_names.get(source_index, fallback_name(None, source_index))
    if source_name in GH3_HEAD_ACCESSORY_BONES:
        return "bone_head.mesh"
    return gh2_skin_bone_name(GH3_TO_GH2_BONES.get(source_name, source_name))


def retarget_vertex_to_stock_bind(
    vertex: dict[str, Any],
    position: list[float],
    normal: list[float],
    source_names: dict[int, str],
    source_transforms: dict[str, dict[str, Any]],
    stock_mesh_bind: dict[str, dict[str, Any]],
    stock_retarget_mode: str,
    anatomical_source_transforms: dict[str, dict[str, Any]] | None = None,
) -> tuple[list[float], list[float]]:
    weighted_position = [0.0, 0.0, 0.0]
    weighted_normal = [0.0, 0.0, 0.0]
    total_weight = 0.0
    for item in vertex.get("weights", []):
        weight = float(item.get("weight", 0.0))
        if weight <= 0.0:
            continue
        source_index = int(item.get("source_bone", item["bone"]))
        source_name = source_names.get(source_index, fallback_name(None, source_index))
        bind_name = GH3_TO_GH2_BONES.get(source_name, source_name)
        if source_name in GH3_HEAD_ACCESSORY_BONES:
            bind_name = "bone_head"
        skin_name = gh2_skin_bone_name(bind_name)
        source = source_transforms.get(skin_name) or source_transforms.get(bind_name)
        target = stock_mesh_bind.get(skin_name)
        if source is None or target is None:
            continue
        anatomical_source = (anatomical_source_transforms or {}).get(skin_name)
        if stock_retarget_mode == "anatomical-hands" and anatomical_source is not None:
            source = anatomical_source
        if stock_retarget_mode in ("translation-only", "anatomical-hands") and anatomical_source is None:
            moved = [
                position[axis]
                + float(target["world"][9 + axis])
                - float(source["world"][9 + axis])
                for axis in range(3)
            ]
            moved_normal = list(normal)
        else:
            retarget = (
                multiply_matrix12_row_vector(
                    invert_matrix12_row_vector(target["world"]),
                    source["world"],
                )
                if stock_retarget_mode == "target-to-source"
                else multiply_matrix12_row_vector(
                    invert_matrix12_row_vector(source["world"]),
                    target["world"],
                )
            )
            moved = transform_point(retarget, position)
            moved_normal = transform_direction(retarget, normal)
        for axis in range(3):
            weighted_position[axis] += moved[axis] * weight
            weighted_normal[axis] += moved_normal[axis] * weight
        total_weight += weight
    if total_weight <= 0.0:
        return position, normal
    return (
        [value / total_weight for value in weighted_position],
        normalized([value / total_weight for value in weighted_normal], normal),
    )


def vertex_has_hand_detail_weight(
    vertex: dict[str, Any],
    source_names: dict[int, str],
) -> bool:
    for item in vertex.get("weights", []):
        if float(item.get("weight", 0.0)) <= 0.0:
            continue
        source_index = int(item.get("source_bone", item["bone"]))
        source_name = source_names.get(source_index, "")
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name).lower()
        if any(
            token in target_name
            for token in ("index", "middlefinger", "ringfinger", "pinky", "thumb")
        ):
            return True
    return False


def vertex_has_hand_weight(
    vertex: dict[str, Any],
    source_names: dict[int, str],
) -> bool:
    for item in vertex.get("weights", []):
        if float(item.get("weight", 0.0)) <= 0.0:
            continue
        source_index = int(item.get("source_bone", item["bone"]))
        source_name = source_names.get(source_index, "")
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name).lower()
        if any(
            token in target_name
            for token in ("-hand", "index", "middlefinger", "ringfinger", "pinky", "thumb")
        ):
            return True
    return False


def vertex_has_left_hand_detail_weight(
    vertex: dict[str, Any],
    source_names: dict[int, str],
) -> bool:
    for item in vertex.get("weights", []):
        if float(item.get("weight", 0.0)) <= 0.0:
            continue
        source_index = int(item.get("source_bone", item["bone"]))
        source_name = source_names.get(source_index, "")
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
        if target_name.startswith("bone_L-") and any(
            token in target_name.lower()
            for token in ("index", "middlefinger", "ringfinger", "pinky", "thumb")
        ):
            return True
    return False


def vertex_has_left_hand_weight(
    vertex: dict[str, Any],
    source_names: dict[int, str],
) -> bool:
    for item in vertex.get("weights", []):
        if float(item.get("weight", 0.0)) <= 0.0:
            continue
        source_index = int(item.get("source_bone", item["bone"]))
        source_name = source_names.get(source_index, "")
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
        if target_name.startswith("bone_L-") and any(
            token in target_name.lower()
            for token in ("-hand", "index", "middlefinger", "ringfinger", "pinky", "thumb")
        ):
            return True
    return False


def vertex_has_upper_limb_weight(
    vertex: dict[str, Any],
    source_names: dict[int, str],
) -> bool:
    for item in vertex.get("weights", []):
        if float(item.get("weight", 0.0)) <= 0.0:
            continue
        source_index = int(item.get("source_bone", item["bone"]))
        source_name = source_names.get(source_index, "")
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name).lower()
        if any(
            token in target_name
            for token in (
                "clavicle",
                "upperarm",
                "forearm",
                "hand",
                "index",
                "middlefinger",
                "ringfinger",
                "pinky",
                "thumb",
            )
        ):
            return True
    return False


def mirror_target_hand_name(target_name: str) -> str | None:
    if not any(
        token in target_name
        for token in ("-hand", "index", "middlefinger", "ringfinger", "pinky", "thumb")
    ):
        return None
    if target_name.startswith("bone_L-"):
        return "bone_R-" + target_name[len("bone_L-"):]
    if target_name.startswith("bone_R-"):
        return "bone_L-" + target_name[len("bone_R-"):]
    return None


def mirror_source_indices(source_names: dict[int, str]) -> dict[int, int]:
    target_to_source = {
        GH3_TO_GH2_BONES.get(source_name, source_name): index
        for index, source_name in source_names.items()
    }
    mirrors: dict[int, int] = {}
    for index, source_name in source_names.items():
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
        mirror = mirror_target_hand_name(target_name)
        if mirror and mirror in target_to_source:
            mirrors[index] = target_to_source[mirror]
    return mirrors


def hand_anchor_source_indices(source_names: dict[int, str]) -> dict[str, int]:
    target_to_source = {
        GH3_TO_GH2_BONES.get(source_name, source_name): index
        for index, source_name in source_names.items()
    }
    anchors: dict[str, int] = {}
    for side in ("L", "R"):
        hand = f"bone_{side}-hand"
        if hand in target_to_source:
            anchors[side] = target_to_source[hand]
    return anchors


def collapse_detail_to_hand_source_bone(
    source_index: int,
    source_names: dict[int, str] | None,
    hand_anchors: dict[str, int] | None,
) -> int:
    if not source_names or not hand_anchors:
        return source_index
    source_name = source_names.get(source_index, "")
    target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
    lowered = target_name.lower()
    if not any(
        token in lowered
        for token in ("index", "middlefinger", "ringfinger", "pinky", "thumb")
    ):
        return source_index
    if target_name.startswith("bone_L-") and "L" in hand_anchors:
        return hand_anchors["L"]
    if target_name.startswith("bone_R-") and "R" in hand_anchors:
        return hand_anchors["R"]
    return source_index


def side_corrected_source_bone(
    source_index: int,
    vertex: dict[str, Any],
    source_names: dict[int, str] | None,
    hand_mirrors: dict[int, int] | None,
) -> int:
    if not source_names or not hand_mirrors or source_index not in hand_mirrors:
        return source_index
    source_name = source_names.get(source_index, "")
    target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
    x = gh3_mesh_position_to_gh2(vertex["position"])[0]
    if target_name.startswith("bone_L-") and x > 1.0:
        return hand_mirrors[source_index]
    if target_name.startswith("bone_R-") and x < -1.0:
        return hand_mirrors[source_index]
    return source_index


def resolved_vertex_weights(
    mesh: dict[str, Any],
    vertex: dict[str, Any],
    source_names: dict[int, str] | None = None,
    hand_mirrors: dict[int, int] | None = None,
    hand_anchors: dict[str, int] | None = None,
) -> list[dict[str, Any]]:
    weights = []
    for item in vertex.get("weights", []):
        weight = float(item.get("weight", 0.0))
        if weight <= 0.0:
            continue
        raw_bone = int(item["bone"])
        source_bone = side_corrected_source_bone(
            raw_bone,
            vertex,
            source_names,
            hand_mirrors,
        )
        source_bone = collapse_detail_to_hand_source_bone(
            source_bone,
            source_names,
            hand_anchors,
        )
        weights.append({
            "bone": raw_bone,
            "source_bone": source_bone,
            "weight": weight,
        })
    return weights


def vertex_bones(
    mesh: dict[str, Any],
    vertex: dict[str, Any],
    source_names: dict[int, str] | None = None,
    hand_mirrors: dict[int, int] | None = None,
    hand_anchors: dict[str, int] | None = None,
) -> set[int]:
    return {
        int(item["source_bone"])
        for item in resolved_vertex_weights(
            mesh,
            vertex,
            source_names,
            hand_mirrors,
            hand_anchors,
        )
    }


def collapsed_target_bone(
    target_name: str, target_bone_collapses: dict[str, str] | None
) -> str:
    current = target_name
    visited = set()
    while target_bone_collapses and current in target_bone_collapses:
        if current in visited:
            raise ValueError(f"target bone collapse cycle at {current}")
        visited.add(current)
        current = target_bone_collapses[current]
    return current


def triangle_weight_totals(
    mesh: dict[str, Any],
    triangle: list[int],
    source_names: dict[int, str] | None = None,
    hand_mirrors: dict[int, int] | None = None,
    hand_anchors: dict[str, int] | None = None,
    vertex_target_bone_collapses: dict[int, dict[str, str]] | None = None,
) -> dict[str, float]:
    totals: dict[str, float] = defaultdict(float)
    for index in triangle:
        for item in resolved_vertex_weights(
            mesh,
            mesh["vertices"][index],
            source_names,
            hand_mirrors,
            hand_anchors,
        ):
            source_index = int(item["source_bone"])
            target = target_bone_name(source_index, source_names or {})
            vertex_collapses = (
                vertex_target_bone_collapses.get(int(index))
                if vertex_target_bone_collapses
                else None
            )
            totals[collapsed_target_bone(target, vertex_collapses)] += float(
                item["weight"]
            )
    return dict(totals)


def effective_triangle_bones(
    mesh: dict[str, Any],
    triangle: list[int],
    max_bones: int,
    source_names: dict[int, str] | None = None,
    hand_mirrors: dict[int, int] | None = None,
    hand_anchors: dict[str, int] | None = None,
    vertex_target_bone_collapses: dict[int, dict[str, str]] | None = None,
    palette_overflow_policy: str = "same-chain-first",
) -> tuple[set[str], dict[str, Any] | None]:
    totals = triangle_weight_totals(
        mesh,
        triangle,
        source_names,
        hand_mirrors,
        hand_anchors,
        vertex_target_bone_collapses,
    )
    if len(totals) <= max_bones:
        return set(totals), None
    kept = set(totals)
    hierarchy_collapses = []
    while len(kept) > max_bones:
        candidates = []
        for bone in kept:
            ancestor = (
                nearest_kept_target_ancestor(bone, kept)
                if palette_overflow_policy == "lowest-weight-ancestor"
                else nearest_kept_same_chain_ancestor(bone, kept)
            )
            if ancestor is not None:
                candidates.append((float(totals[bone]), bone, ancestor))
        if not candidates:
            break
        weight, bone, ancestor = min(candidates)
        kept.remove(bone)
        hierarchy_collapses.append(
            {
                "bone": bone,
                "ancestor": ancestor,
                "triangle_weight_total": weight,
            }
        )
    if len(kept) > max_bones:
        ranked = sorted(
            ((bone, totals[bone]) for bone in kept),
            key=lambda item: (-item[1], item[0]),
        )
        kept = {bone for bone, _weight in ranked[:max_bones]}
    dropped_bones = sorted(set(totals) - kept)
    dropped = sum(float(totals[bone]) for bone in dropped_bones)
    return kept, {
        "target_bone_count": len(totals),
        "kept_bones": sorted(kept),
        "dropped_bones": dropped_bones,
        "dropped_weight_sum": dropped,
        "hierarchy_collapses": hierarchy_collapses,
    }


def greedy_partitions(
    mesh: dict[str, Any],
    max_bones: int,
    source_names: dict[int, str] | None = None,
    hand_mirrors: dict[int, int] | None = None,
    hand_anchors: dict[str, int] | None = None,
    vertex_target_bone_collapses: dict[int, dict[str, str]] | None = None,
    palette_overflow_policy: str = "same-chain-first",
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    partitions: list[dict[str, Any]] = []
    pruned = []
    for triangle_index, triangle in enumerate(mesh["triangles"]):
        bones, prune_info = effective_triangle_bones(
            mesh,
            triangle,
            max_bones,
            source_names,
            hand_mirrors,
            hand_anchors,
            vertex_target_bone_collapses,
            palette_overflow_policy,
        )
        if prune_info:
            prune_info["triangle_index"] = triangle_index
            pruned.append(prune_info)
        best_index = None
        best_extra = None
        for index, part in enumerate(partitions):
            merged = part["bones"] | bones
            if len(merged) > max_bones:
                continue
            extra = len(merged) - len(part["bones"])
            if best_extra is None or extra < best_extra:
                best_index = index
                best_extra = extra
                if extra == 0:
                    break
        if best_index is None:
            partitions.append({"bones": set(bones), "triangles": [triangle_index]})
        else:
            partitions[best_index]["bones"].update(bones)
            partitions[best_index]["triangles"].append(triangle_index)
    return partitions, pruned


def normalize_weights(weights: list[float]) -> list[float]:
    total = sum(weights)
    if total <= 0.0 or not math.isfinite(total):
        return [0.0 for _ in weights]
    return [value / total for value in weights]


def wrist_seam_transfer_factor(
    axis_fraction: float,
    start: float,
    strength: float,
) -> float:
    if not 0.0 <= start < 1.0:
        raise ValueError("wrist seam start must be in [0, 1)")
    if not 0.0 <= strength <= 1.0:
        raise ValueError("wrist seam transfer strength must be in [0, 1]")
    if axis_fraction <= start or strength == 0.0:
        return 0.0
    amount = min(1.0, (axis_fraction - start) / (1.0 - start))
    smooth = amount * amount * (3.0 - 2.0 * amount)
    return strength * smooth


def target_source_indices(source_names: dict[int, str]) -> dict[str, int]:
    result: dict[str, int] = {}
    for source_index, source_name in source_names.items():
        target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
        result.setdefault(target_name, source_index)
    return result


def is_same_side_hand_target(target_name: str, side: str) -> bool:
    if not target_name.startswith(f"bone_{side}-"):
        return False
    lowered = target_name.lower()
    return any(
        token in lowered
        for token in ("-hand", "index", "middlefinger", "ringfinger", "pinky", "thumb")
    )


def apply_wrist_seam_weight_transfer(
    mesh: dict[str, Any],
    source_names: dict[int, str],
    source_transforms: dict[str, dict[str, Any]],
    start: float,
    strength: float,
) -> dict[str, Any]:
    """Blend distal forearm-only vertices onto the hand before partitioning."""
    wrist_seam_transfer_factor(start, start, strength)
    target_indices = target_source_indices(source_names)
    side_reports = []
    for side in ("L", "R"):
        forearm_target = f"bone_{side}-foreArm"
        hand_target = f"bone_{side}-hand"
        forearm_source = target_indices.get(forearm_target)
        hand_source = target_indices.get(hand_target)
        forearm_transform = source_transforms.get(
            gh2_skin_bone_name(forearm_target)
        ) or source_transforms.get(forearm_target)
        hand_transform = source_transforms.get(
            gh2_skin_bone_name(hand_target)
        ) or source_transforms.get(hand_target)
        if forearm_source is None or hand_source is None:
            raise ValueError(f"missing {side} forearm/hand source bone")
        if forearm_transform is None or hand_transform is None:
            raise ValueError(f"missing {side} forearm/hand bind transform")

        elbow = [float(value) for value in forearm_transform["world"][9:12]]
        wrist = [float(value) for value in hand_transform["world"][9:12]]
        axis = [wrist[index] - elbow[index] for index in range(3)]
        axis_length_squared = sum(value * value for value in axis)
        if axis_length_squared <= 0.0:
            raise ValueError(f"degenerate {side} forearm-to-hand bind axis")

        reference_vertices = []
        for source_vertex, vertex in enumerate(mesh["vertices"]):
            forearm_weight = 0.0
            hand_weight = 0.0
            has_hand_detail = False
            for item in vertex.get("weights", []):
                weight = float(item.get("weight", 0.0))
                if weight <= 0.0:
                    continue
                raw_bone = int(item["bone"])
                source_name = source_names.get(raw_bone, fallback_name(None, raw_bone))
                target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
                if target_name == forearm_target:
                    forearm_weight += weight
                elif target_name == hand_target:
                    hand_weight += weight
                elif is_same_side_hand_target(target_name, side):
                    has_hand_detail = True
            limb_weight = forearm_weight + hand_weight
            if (
                forearm_weight > 0.0
                and hand_weight > 0.0
                and not has_hand_detail
                and limb_weight > 0.0
            ):
                reference_vertices.append(
                    {
                        "source_vertex": source_vertex,
                        "position": gh3_mesh_position_to_gh2(vertex["position"]),
                        "hand_fraction": hand_weight / limb_weight,
                    }
                )
        changed_vertices = []
        transferred_total = 0.0
        maximum_transfer = 0.0
        for source_vertex, vertex in enumerate(mesh["vertices"]):
            weights = vertex.get("weights", [])
            forearm_items = []
            has_same_side_hand = False
            for item in weights:
                if float(item.get("weight", 0.0)) <= 0.0:
                    continue
                raw_bone = int(item["bone"])
                source_name = source_names.get(raw_bone, fallback_name(None, raw_bone))
                target_name = GH3_TO_GH2_BONES.get(source_name, source_name)
                if target_name == forearm_target:
                    forearm_items.append(item)
                elif is_same_side_hand_target(target_name, side):
                    has_same_side_hand = True
            if not forearm_items or has_same_side_hand:
                continue

            position = gh3_mesh_position_to_gh2(vertex["position"])
            axis_fraction = sum(
                (position[index] - elbow[index]) * axis[index]
                for index in range(3)
            ) / axis_length_squared
            factor = wrist_seam_transfer_factor(axis_fraction, start, strength)
            if factor <= 0.0:
                continue
            if not reference_vertices:
                raise ValueError(
                    f"missing authored {side} forearm/hand blend references"
                )

            reference = min(
                reference_vertices,
                key=lambda row: math.sqrt(
                    sum(
                        (
                            float(position[index])
                            - float(row["position"][index])
                        )
                        ** 2
                        for index in range(3)
                    )
                ),
            )
            reference_distance = math.sqrt(
                sum(
                    (
                        float(position[index])
                        - float(reference["position"][index])
                    )
                    ** 2
                    for index in range(3)
                )
            )
            factor *= float(reference["hand_fraction"])

            transferred = 0.0
            for item in forearm_items:
                original = float(item["weight"])
                amount = original * factor
                item["weight"] = original - amount
                transferred += amount
            vertex["weights"] = [
                item
                for item in weights
                if float(item.get("weight", 0.0)) > 1.0e-12
            ]
            vertex["weights"].append({"bone": hand_source, "weight": transferred})
            changed_vertices.append(
                {
                    "source_vertex": source_vertex,
                    "axis_fraction": axis_fraction,
                    "reference_source_vertex": reference["source_vertex"],
                    "reference_distance": reference_distance,
                    "reference_hand_fraction": reference["hand_fraction"],
                    "transferred_weight": transferred,
                }
            )
            transferred_total += transferred
            maximum_transfer = max(maximum_transfer, transferred)

        side_reports.append(
            {
                "side": side,
                "forearm_source_bone": forearm_source,
                "hand_source_bone": hand_source,
                "reference_mode": "nearest-authored-forearm-hand-blend",
                "reference_vertex_count": len(reference_vertices),
                "changed_vertex_count": len(changed_vertices),
                "transferred_weight_total": transferred_total,
                "maximum_vertex_transfer": maximum_transfer,
                "minimum_changed_axis_fraction": min(
                    (row["axis_fraction"] for row in changed_vertices),
                    default=None,
                ),
                "maximum_changed_axis_fraction": max(
                    (row["axis_fraction"] for row in changed_vertices),
                    default=None,
                ),
                "vertices": changed_vertices,
            }
        )
    return {
        "enabled": strength > 0.0,
        "start": start,
        "strength": strength,
        "changed_vertex_count": sum(
            row["changed_vertex_count"] for row in side_reports
        ),
        "transferred_weight_total": sum(
            row["transferred_weight_total"] for row in side_reports
        ),
        "maximum_vertex_transfer": max(
            (row["maximum_vertex_transfer"] for row in side_reports),
            default=0.0,
        ),
        "sides": side_reports,
    }


FINGER_TARGET_PATTERN = re.compile(
    r"^(bone_[LR]-(?:index|middlefinger|ringfinger|pinky|thumb))(\d{2})\.mesh$",
    re.IGNORECASE,
)


def nearest_kept_same_chain_ancestor(
    target_name: str, kept_names: set[str]
) -> str | None:
    """Find a retained proximal joint on the same finger chain."""

    match = FINGER_TARGET_PATTERN.match(target_name)
    if not match:
        return None
    prefix, number_text = match.groups()
    number = int(number_text)
    while number > 1:
        number -= 1
        candidate = f"{prefix}{number:02d}.mesh"
        if candidate in kept_names:
            return candidate
    return None


def nearest_kept_target_ancestor(
    target_name: str, kept_names: set[str]
) -> str | None:
    """Find the nearest representable GH2 ancestor for a dropped finger bone."""

    match = FINGER_TARGET_PATTERN.match(target_name)
    if not match:
        return None
    same_chain = nearest_kept_same_chain_ancestor(target_name, kept_names)
    if same_chain is not None:
        return same_chain
    prefix, _number_text = match.groups()
    side = "L" if prefix.casefold().startswith("bone_l-") else "R"
    hand = f"bone_{side}-hand.mesh"
    return hand if hand in kept_names else None


def palette_collapse_candidate_rank(bone: str, ancestor: str) -> tuple[int, str]:
    bone_match = FINGER_TARGET_PATTERN.match(bone)
    ancestor_match = FINGER_TARGET_PATTERN.match(ancestor)
    if bone_match and ancestor_match:
        bone_prefix, bone_number = bone_match.groups()
        ancestor_prefix, ancestor_number = ancestor_match.groups()
        if bone_prefix.casefold() == ancestor_prefix.casefold():
            return int(bone_number) - int(ancestor_number), ancestor
    if bone_match:
        side = "L" if bone_match.group(1).casefold().startswith("bone_l-") else "R"
        if ancestor == f"bone_{side}-hand.mesh":
            return 100, ancestor
    return 1000, ancestor


def stable_vertex_palette_collapses(
    mesh: dict[str, Any],
    max_bones: int,
    source_names: dict[int, str] | None = None,
    hand_mirrors: dict[int, int] | None = None,
    hand_anchors: dict[str, int] | None = None,
    palette_overflow_policy: str = "same-chain-first",
) -> tuple[dict[int, dict[str, str]], list[dict[str, Any]]]:
    """Make each required fallback stable for every copy of its source vertex."""

    collapses: dict[int, dict[str, str]] = {}
    history = []
    source_vertices_by_position: dict[tuple[float, ...], list[int]] = defaultdict(list)
    for source_vertex, vertex in enumerate(mesh["vertices"]):
        source_vertices_by_position[
            tuple(float(value) for value in vertex["position"])
        ].append(source_vertex)
    for iteration in range(64):
        _partitions, pruned = greedy_partitions(
            mesh,
            max_bones,
            source_names,
            hand_mirrors,
            hand_anchors,
            collapses,
            palette_overflow_policy,
        )
        proposals: dict[
            tuple[int, str], list[tuple[str, int, float]]
        ] = defaultdict(list)
        for report in pruned:
            triangle_index = int(report["triangle_index"])
            triangle = mesh["triangles"][triangle_index]
            for row in report["hierarchy_collapses"]:
                bone = str(row["bone"])
                ancestor = str(row["ancestor"])
                trigger_vertices = {
                    int(source_vertex) for source_vertex in triangle
                }
                position_peers = {
                    peer
                    for source_vertex in trigger_vertices
                    for peer in source_vertices_by_position[
                        tuple(
                            float(value)
                            for value in mesh["vertices"][source_vertex][
                                "position"
                            ]
                        )
                    ]
                }
                for source_vertex in sorted(position_peers):
                    vertex_collapses = collapses.get(source_vertex)
                    vertex = mesh["vertices"][source_vertex]
                    effective_bones = {
                        collapsed_target_bone(
                            target_bone_name(
                                int(item["source_bone"]), source_names or {}
                            ),
                            vertex_collapses,
                        )
                        for item in resolved_vertex_weights(
                            mesh,
                            vertex,
                            source_names,
                            hand_mirrors,
                            hand_anchors,
                        )
                        if float(item["weight"]) > 0.0
                    }
                    if bone in effective_bones:
                        proposals[(source_vertex, bone)].append(
                            (
                                ancestor,
                                triangle_index,
                                float(row["triangle_weight_total"]),
                            )
                        )
        if not proposals:
            break
        added = 0
        for source_vertex, bone in sorted(proposals):
            ancestor, triangle_index, triangle_weight_total = min(
                proposals[(source_vertex, bone)],
                key=lambda row: (
                    palette_collapse_candidate_rank(bone, row[0]),
                    row[1],
                    row[2],
                ),
            )
            vertex_collapses = collapses.setdefault(source_vertex, {})
            if bone in vertex_collapses:
                continue
            vertex_collapses[bone] = ancestor
            history.append(
                {
                    "iteration": iteration,
                    "source_vertex": source_vertex,
                    "bone": bone,
                    "ancestor": ancestor,
                    "trigger_triangle": triangle_index,
                    "triangle_weight_total": triangle_weight_total,
                }
            )
            added += 1
        if added == 0:
            break
    else:
        raise ValueError("stable vertex palette collapse did not converge")
    return collapses, history


def sphere(vertices: list[dict[str, Any]]) -> list[float]:
    if not vertices:
        return [0.0, 0.0, 0.0, 0.0]
    center = [
        sum(float(vertex["position"][axis]) for vertex in vertices) / len(vertices)
        for axis in range(3)
    ]
    radius = 0.0
    for vertex in vertices:
        pos = vertex["position"]
        radius = max(
            radius,
            math.sqrt(sum((float(pos[axis]) - center[axis]) ** 2 for axis in range(3))),
        )
    return center + [radius]


def staged_chunk(
    outfit: str,
    mesh: dict[str, Any],
    partition_index: int,
    partition: dict[str, Any],
    source_names: dict[int, str],
    source_transforms: dict[str, dict[str, Any]],
    stock_mesh_bind: dict[str, dict[str, Any]],
    stock_retarget_mode: str,
    stock_retarget_scope: str,
    anatomical_source_transforms: dict[str, dict[str, Any]],
    hand_mirrors: dict[int, int] | None,
    hand_anchors: dict[str, int] | None,
    vertex_target_bone_collapses: dict[int, dict[str, str]] | None = None,
) -> dict[str, Any]:
    source_triangles = [mesh["triangles"][index] for index in partition["triangles"]]
    used_vertex_indices = sorted({vertex for triangle in source_triangles for vertex in triangle})
    remap = {old: new for new, old in enumerate(used_vertex_indices)}
    slot_target_bones = sorted(partition["bones"])
    slot_source_bones = sorted(
        {
            int(item["source_bone"])
            for source_vertex_index in used_vertex_indices
            for item in resolved_vertex_weights(
                mesh,
                mesh["vertices"][source_vertex_index],
                source_names,
                hand_mirrors,
                hand_anchors,
            )
            if collapsed_target_bone(
                target_bone_name(int(item["source_bone"]), source_names),
                (
                    vertex_target_bone_collapses.get(source_vertex_index)
                    if vertex_target_bone_collapses
                    else None
                ),
            )
            in slot_target_bones
        }
    )
    staged_vertices = []
    dropped_weight_total = 0.0
    dropped_weight_max = 0.0
    ancestor_remapped_weight_total = 0.0
    ancestor_remapped_weight_max = 0.0
    for source_vertex_index in used_vertex_indices:
        vertex = mesh["vertices"][source_vertex_index]
        resolved_weights = resolved_vertex_weights(
            mesh,
            vertex,
            source_names,
            hand_mirrors,
            hand_anchors,
        )
        target_weights: dict[str, float] = defaultdict(float)
        for item in resolved_weights:
            source_index = int(item["source_bone"])
            target = target_bone_name(source_index, source_names)
            target_weights[
                collapsed_target_bone(
                    target,
                    (
                        vertex_target_bone_collapses.get(source_vertex_index)
                        if vertex_target_bone_collapses
                        else None
                    ),
                )
            ] += float(item["weight"])
        kept_names = set(slot_target_bones)
        representable_weights: dict[str, float] = defaultdict(float)
        ancestor_remapped_weight = 0.0
        for target, weight in target_weights.items():
            if target in kept_names:
                representable_weights[target] += weight
                continue
            ancestor = nearest_kept_target_ancestor(target, kept_names)
            if ancestor is not None:
                representable_weights[ancestor] += weight
                ancestor_remapped_weight += weight
        dropped_weight = max(0.0, sum(target_weights.values()) - sum(representable_weights.values()))
        dropped_weight_total += dropped_weight
        dropped_weight_max = max(dropped_weight_max, dropped_weight)
        ancestor_remapped_weight_total += ancestor_remapped_weight
        ancestor_remapped_weight_max = max(
            ancestor_remapped_weight_max, ancestor_remapped_weight
        )
        weights = normalize_weights(
            [representable_weights.get(name, 0.0) for name in slot_target_bones]
        )
        position = gh3_mesh_position_to_gh2(vertex["position"])
        normal = gh3_mesh_normal_to_gh2(vertex["normal"])
        should_retarget = (
            stock_mesh_bind
            and stock_retarget_scope != "none"
            and (
                stock_retarget_scope == "all"
                or (
                    stock_retarget_scope == "hand-detail"
                    and vertex_has_hand_detail_weight(vertex, source_names)
                )
                or (
                    stock_retarget_scope == "left-hand-detail"
                    and vertex_has_left_hand_detail_weight(vertex, source_names)
                )
                or (
                    stock_retarget_scope == "hands"
                    and vertex_has_hand_weight(vertex, source_names)
                )
                or (
                    stock_retarget_scope == "left-hand"
                    and vertex_has_left_hand_weight(vertex, source_names)
                )
                or (
                    stock_retarget_scope == "upper-limbs"
                    and vertex_has_upper_limb_weight(vertex, source_names)
                )
            )
        )
        if should_retarget:
            retarget_vertex = dict(vertex)
            retarget_vertex["weights"] = resolved_vertex_weights(
                mesh,
                vertex,
                source_names,
                hand_mirrors,
                hand_anchors,
            )
            position, normal = retarget_vertex_to_stock_bind(
                retarget_vertex,
                position,
                normal,
                source_names,
                source_transforms,
                stock_mesh_bind,
                stock_retarget_mode,
                anatomical_source_transforms,
            )
        staged_vertices.append({
            "source_vertex": source_vertex_index,
            "position": position,
            "normal": normal,
            "uv": gh3_mesh_uv_to_gh2(vertex["uv"]),
            "color_or_weights": weights + [0.0] * (4 - len(weights)),
            "dropped_source_weight": dropped_weight,
            "ancestor_remapped_source_weight": ancestor_remapped_weight,
        })
    staged_faces = [[remap[index] for index in triangle] for triangle in source_triangles]
    texture_checksum = str(mesh.get("texture_checksum", ""))
    material_checksum = str(mesh.get("material_checksum", ""))
    material_name = f"{outfit}_mesh{mesh['index']}_{material_checksum.replace('0x', '')}.mat"
    texture_name = f"{outfit}_{texture_checksum.replace('0x', '')}.tex"
    return {
        "name": f"{outfit}_mesh{mesh['index']}_part{partition_index}.mesh",
        "source_mesh_index": mesh["index"],
        "partition_index": partition_index,
        "material": material_name,
        "texture": texture_name,
        "texture_checksum": texture_checksum,
        "material_checksum": material_checksum,
        "source_bone_indices": slot_source_bones,
        "bone_slots": slot_target_bones,
        "vertex_count": len(staged_vertices),
        "face_count": len(staged_faces),
        "sphere": sphere(staged_vertices),
        "dropped_weight_total": dropped_weight_total,
        "dropped_weight_max": dropped_weight_max,
        "ancestor_remapped_weight_total": ancestor_remapped_weight_total,
        "ancestor_remapped_weight_max": ancestor_remapped_weight_max,
        "vertices": staged_vertices,
        "faces": staged_faces,
    }


def stage_outfit(
    mesh_path: Path,
    skeleton_names: dict[int, str],
    output_dir: Path,
    max_bones: int,
    palette_overflow_policy: str,
    source_transforms: dict[str, dict[str, Any]],
    stock_mesh_bind: dict[str, dict[str, Any]],
    stock_retarget_mode: str,
    stock_retarget_scope: str,
    anatomical_source_transforms: dict[str, dict[str, Any]],
    hand_mirrors: dict[int, int] | None,
    hand_anchors: dict[str, int] | None,
    wrist_seam_start: float,
    wrist_seam_transfer_strength: float,
) -> dict[str, Any]:
    data = load_json(mesh_path)
    outfit = data["outfit"]
    chunks_path = output_dir / f"{outfit}.mesh28_stage.jsonl.gz"
    chunks = []
    mesh_summaries = []
    total_pruned = 0
    dropped_weight_total = 0.0
    dropped_weight_max = 0.0
    ancestor_remapped_weight_total = 0.0
    ancestor_remapped_weight_max = 0.0
    stable_vertex_palette_collapse_rows = []
    wrist_seam_reports = []
    with deterministic_gzip_text(chunks_path) as handle:
        for mesh in data["meshes"]:
            wrist_seam_report = apply_wrist_seam_weight_transfer(
                mesh,
                skeleton_names,
                source_transforms,
                wrist_seam_start,
                wrist_seam_transfer_strength,
            )
            wrist_seam_reports.append(
                {
                    "source_mesh_index": int(mesh["index"]),
                    **wrist_seam_report,
                }
            )
            vertex_target_bone_collapses, collapse_history = (
                stable_vertex_palette_collapses(
                mesh,
                max_bones,
                skeleton_names,
                hand_mirrors,
                hand_anchors,
                palette_overflow_policy,
                )
            )
            stable_vertex_palette_collapse_rows.extend(
                {
                    "source_mesh_index": int(mesh["index"]),
                    **row,
                }
                for row in collapse_history
            )
            partitions, pruned = greedy_partitions(
                mesh,
                max_bones,
                skeleton_names,
                hand_mirrors,
                hand_anchors,
                vertex_target_bone_collapses,
                palette_overflow_policy,
            )
            total_pruned += len(pruned)
            mesh_chunks = []
            for partition_index, partition in enumerate(partitions):
                chunk = staged_chunk(
                    outfit,
                    mesh,
                    partition_index,
                    partition,
                    skeleton_names,
                    source_transforms,
                    stock_mesh_bind,
                    stock_retarget_mode,
                    stock_retarget_scope,
                    anatomical_source_transforms,
                    hand_mirrors,
                    hand_anchors,
                    vertex_target_bone_collapses,
                )
                dropped_weight_total += chunk["dropped_weight_total"]
                dropped_weight_max = max(dropped_weight_max, chunk["dropped_weight_max"])
                ancestor_remapped_weight_total += chunk[
                    "ancestor_remapped_weight_total"
                ]
                ancestor_remapped_weight_max = max(
                    ancestor_remapped_weight_max,
                    chunk["ancestor_remapped_weight_max"],
                )
                handle.write(json.dumps(chunk, separators=(",", ":")) + "\n")
                chunks.append(chunk)
                mesh_chunks.append({
                    "name": chunk["name"],
                    "source_bone_indices": chunk["source_bone_indices"],
                    "bone_slots": chunk["bone_slots"],
                    "vertex_count": chunk["vertex_count"],
                    "face_count": chunk["face_count"],
                    "dropped_weight_total": chunk["dropped_weight_total"],
                    "dropped_weight_max": chunk["dropped_weight_max"],
                    "ancestor_remapped_weight_total": chunk[
                        "ancestor_remapped_weight_total"
                    ],
                    "ancestor_remapped_weight_max": chunk[
                        "ancestor_remapped_weight_max"
                    ],
                })
            mesh_summaries.append({
                "source_mesh_index": mesh["index"],
                "source_vertex_count": len(mesh["vertices"]),
                "source_face_count": len(mesh["triangles"]),
                "source_used_bone_count": len({
                    bone
                    for vertex in mesh["vertices"]
                    for bone in vertex_bones(
                        mesh,
                        vertex,
                        skeleton_names,
                        hand_mirrors,
                        hand_anchors,
                    )
                }),
                "partition_count": len(partitions),
                "pruned_triangle_count": len(pruned),
                "wrist_seam_weight_transfer": wrist_seam_report,
                "stable_vertex_palette_collapses": collapse_history,
                "chunks": mesh_chunks,
            })
    return {
        "outfit": outfit,
        "source": str(mesh_path),
        "relative_path": chunks_path.relative_to(output_dir).as_posix(),
        "source_mesh_count": len(data["meshes"]),
        "chunk_count": len(chunks),
        "source_vertex_count": sum(len(mesh["vertices"]) for mesh in data["meshes"]),
        "source_face_count": sum(len(mesh["triangles"]) for mesh in data["meshes"]),
        "staged_vertex_count": sum(chunk["vertex_count"] for chunk in chunks),
        "staged_face_count": sum(chunk["face_count"] for chunk in chunks),
        "oversize_triangle_count": 0,
        "pruned_triangle_count": total_pruned,
        "dropped_weight_total": dropped_weight_total,
        "dropped_weight_max": dropped_weight_max,
        "ancestor_remapped_weight_total": ancestor_remapped_weight_total,
        "ancestor_remapped_weight_max": ancestor_remapped_weight_max,
        "stable_vertex_palette_collapse_count": len(
            stable_vertex_palette_collapse_rows
        ),
        "stable_vertex_palette_collapses": stable_vertex_palette_collapse_rows,
        "wrist_seam_weight_transfer": {
            "enabled": wrist_seam_transfer_strength > 0.0,
            "start": wrist_seam_start,
            "strength": wrist_seam_transfer_strength,
            "changed_vertex_count": sum(
                row["changed_vertex_count"] for row in wrist_seam_reports
            ),
            "transferred_weight_total": sum(
                row["transferred_weight_total"] for row in wrist_seam_reports
            ),
            "maximum_vertex_transfer": max(
                (row["maximum_vertex_transfer"] for row in wrist_seam_reports),
                default=0.0,
            ),
            "meshes": wrist_seam_reports,
        },
        "max_bones_per_chunk": max((len(chunk["bone_slots"]) for chunk in chunks), default=0),
        "mesh_coordinate_bridge": {
            "source_basis": "NXTools GH3 PS2 mesh [x, y_up, z_depth]",
            "target_basis": "GH2 renderer [x, y_depth, z_up]",
            "scale": GH3_PS2_MESH_TO_GH2_SCALE,
        },
        "stored_byte_count": chunks_path.stat().st_size,
        "meshes": mesh_summaries,
    }


def command_stage(args: argparse.Namespace) -> int:
    global SKIN_BONE_SUFFIX
    SKIN_BONE_SUFFIX = args.skin_bone_suffix
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    skeleton_names = bone_names_by_index(args.skeleton)
    source_transforms = load_bone_transforms(args.skeleton)
    anatomical_source_transforms = hand_anatomical_source_transforms(source_transforms)
    stock_mesh_bind = load_stock_mesh_bind_transforms(args.gh2_stock_rig, "gh3_midori_1")
    hand_mirrors = mirror_source_indices(skeleton_names) if args.side_correct_hand_weights else None
    hand_anchors = (
        hand_anchor_source_indices(skeleton_names)
        if args.collapse_hand_detail_weights
        else None
    )
    outfits = []
    for mesh_path in sorted(args.input.glob("*.mesh_ir.json")):
        outfits.append(
            stage_outfit(
                mesh_path,
                skeleton_names,
                output,
                args.max_bones,
                args.palette_overflow_policy,
                source_transforms,
                stock_mesh_bind,
                args.stock_retarget_mode,
                args.stock_retarget_scope,
                anatomical_source_transforms,
                hand_mirrors,
                hand_anchors,
                args.wrist_seam_start,
                args.wrist_seam_transfer_strength,
            )
        )
    target_counts = Counter()
    for source_name in skeleton_names.values():
        target_counts[GH3_TO_GH2_BONES.get(source_name, source_name)] += 1
    duplicate_targets = sorted(name for name, count in target_counts.items() if count > 1)
    used_source_indices = sorted({
        index
        for outfit in outfits
        for mesh in outfit["meshes"]
        for chunk in mesh["chunks"]
        for index in chunk["source_bone_indices"]
    })
    mapped_bones = []
    unmapped_bones = []
    for index in used_source_indices:
        source_name = skeleton_names.get(index, fallback_name(None, index))
        row = {
            "index": index,
            "source_name": source_name,
            "target_name": GH3_TO_GH2_BONES.get(source_name),
        }
        if row["target_name"]:
            mapped_bones.append(row)
        else:
            row["target_name"] = source_name
            unmapped_bones.append(row)
    manifest = {
        "format": "gh3_midori_model_stage_v1",
        "source_dir": str(args.input),
        "skeleton": str(args.skeleton),
        "max_bones_per_mesh28_chunk": args.max_bones,
        "palette_overflow_policy_mode": args.palette_overflow_policy,
        "gh2_stock_rig": str(args.gh2_stock_rig) if args.gh2_stock_rig else None,
        "stock_retarget_mode": args.stock_retarget_mode,
        "stock_retarget_scope": args.stock_retarget_scope,
        "skin_bone_suffix": args.skin_bone_suffix,
        "mesh_coordinate_bridge": {
            "source_basis": "NXTools GH3 PS2 mesh [x, y_up, z_depth]",
            "target_basis": "GH2 renderer [x, y_depth, z_up]",
            "scale": GH3_PS2_MESH_TO_GH2_SCALE,
        },
        "outfit_count": len(outfits),
        "chunk_count": sum(outfit["chunk_count"] for outfit in outfits),
        "source_face_count": sum(outfit["source_face_count"] for outfit in outfits),
        "staged_face_count": sum(outfit["staged_face_count"] for outfit in outfits),
        "oversize_triangle_count": sum(outfit["oversize_triangle_count"] for outfit in outfits),
        "pruned_triangle_count": sum(outfit["pruned_triangle_count"] for outfit in outfits),
        "dropped_weight_total": sum(outfit["dropped_weight_total"] for outfit in outfits),
        "dropped_weight_max": max((outfit["dropped_weight_max"] for outfit in outfits), default=0.0),
        "ancestor_remapped_weight_total": sum(
            outfit["ancestor_remapped_weight_total"] for outfit in outfits
        ),
        "ancestor_remapped_weight_max": max(
            (outfit["ancestor_remapped_weight_max"] for outfit in outfits),
            default=0.0,
        ),
        "stable_vertex_palette_collapse_count": sum(
            outfit["stable_vertex_palette_collapse_count"] for outfit in outfits
        ),
        "stable_vertex_palette_collapses": [
            {
                "outfit": outfit["outfit"],
                **row,
            }
            for outfit in outfits
            for row in outfit["stable_vertex_palette_collapses"]
        ],
        "palette_overflow_policy": (
            "Reduce over-palette triangles by collapsing distal finger joints "
            "into retained proximal joints on the same chain. Promote each "
            "required hierarchy collapse to a stable source-vertex rule "
            "before partitioning so every copy retains identical weights "
            "across chunks without degrading unrelated vertices on that "
            "joint. Then remap any remaining excluded finger influences to "
            "the nearest retained same-chain ancestor or same-side hand "
            "before final renormalization."
        ),
        "used_source_bone_count": len(used_source_indices),
        "mapped_source_bone_count": len(mapped_bones),
        "unmapped_source_bone_count": len(unmapped_bones),
        "unmapped_source_bones": unmapped_bones,
        "duplicate_target_bone_names": duplicate_targets,
        "side_correct_hand_weights": bool(args.side_correct_hand_weights),
        "collapse_hand_detail_weights": bool(args.collapse_hand_detail_weights),
        "wrist_seam_weight_transfer": {
            "enabled": args.wrist_seam_transfer_strength > 0.0,
            "start": args.wrist_seam_start,
            "strength": args.wrist_seam_transfer_strength,
            "changed_vertex_count": sum(
                outfit["wrist_seam_weight_transfer"]["changed_vertex_count"]
                for outfit in outfits
            ),
            "transferred_weight_total": sum(
                outfit["wrist_seam_weight_transfer"]["transferred_weight_total"]
                for outfit in outfits
            ),
            "maximum_vertex_transfer": max(
                (
                    outfit["wrist_seam_weight_transfer"]["maximum_vertex_transfer"]
                    for outfit in outfits
                ),
                default=0.0,
            ),
        },
        "outfits": outfits,
        "next_step": "Feed mesh28_stage chunks to a C++ model MILO writer that reuses serialize_mesh28/serialize_mat27/serialize_tex10.",
    }
    manifest_path = output / "gh3_midori_model_stage_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    if args.print_summary:
        roles = ",".join(f"{outfit['outfit']}:{outfit['chunk_count']}" for outfit in outfits)
        print(
            "outfits=%d chunks=%d faces=%d oversize=%d pruned=%d dropped_max=%.6f bytes=%d split=%s"
            % (
                len(outfits),
                manifest["chunk_count"],
                manifest["staged_face_count"],
                manifest["oversize_triangle_count"],
                manifest["pruned_triangle_count"],
                manifest["dropped_weight_max"],
                sum(outfit["stored_byte_count"] for outfit in outfits),
                roles,
            )
        )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("out/midori/input/outfit1_mesh_ir"))
    parser.add_argument(
        "--skeleton",
        type=Path,
        default=Path("out/midori/input/gh3_guitarist_midori.skeleton_ir.json"),
    )
    parser.add_argument("--output", type=Path, default=Path("out/midori/model_stage"))
    parser.add_argument("--max-bones", type=int, default=4)
    parser.add_argument(
        "--palette-overflow-policy",
        choices=("same-chain-first", "lowest-weight-ancestor"),
        default="same-chain-first",
        help=(
            "Choose whether over-palette triangles preserve same-chain detail "
            "first or collapse the lowest-weight influence with a legal "
            "finger/hand ancestor."
        ),
    )
    parser.add_argument(
        "--skin-bone-suffix",
        choices=("mesh", "base"),
        default="mesh",
        help="Skin Mesh28 chunks to generated .mesh aliases or base GH2 bone names.",
    )
    parser.add_argument("--gh2-stock-rig", type=Path, default=None)
    parser.add_argument(
        "--stock-retarget-mode",
        choices=("source-to-target", "target-to-source", "translation-only", "anatomical-hands"),
        default="source-to-target",
    )
    parser.add_argument(
        "--stock-retarget-scope",
        choices=(
            "all",
            "upper-limbs",
            "hands",
            "left-hand",
            "hand-detail",
            "left-hand-detail",
            "none",
        ),
        default="all",
    )
    parser.add_argument(
        "--side-correct-hand-weights",
        action="store_true",
        help="Mirror wrong-side hand/finger skin weights using the vertex bind-space X side.",
    )
    parser.add_argument(
        "--collapse-hand-detail-weights",
        action="store_true",
        help="Bind detailed hand/finger vertices to the side's hand bone while keeping animated finger bones in the skeleton.",
    )
    parser.add_argument(
        "--wrist-seam-start",
        type=float,
        default=0.8,
        help="Forearm-axis fraction where distal forearm weights begin blending to the hand.",
    )
    parser.add_argument(
        "--wrist-seam-transfer-strength",
        type=float,
        default=0.0,
        help="Maximum fraction of forearm weight transferred to the same-side hand.",
    )
    parser.add_argument("--print-summary", action="store_true")
    parser.set_defaults(func=command_stage)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
