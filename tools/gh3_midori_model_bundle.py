"""Pack staged Midori Mesh28 chunks into a simple binary converter input."""

from __future__ import annotations

import argparse
import gzip
import json
import struct
import zlib
from pathlib import Path
from typing import Any

from gh3_midori_bone_names import checksum_name_map, resolved_bone_name
from gh3_midori_gh2_bridge import (
    GH3_PS2_SKELETON_TO_GH2_SCALE,
    source_parent_name,
    target_bone_name,
)


MAGIC = b"GH3M2MB\0"
VERSION = 8
PACKAGE_ROOT_PARENT = "__PACKAGE_ROOT__"
DEFAULT_POSE_CLIP = "anims/rig/defaults/gh3_guitarist_midori_default.ska.ps2"
MESH_BIND_SUFFIX = "__mesh_bind"
STOCK_BIND_SCOPES = ("all", "upper-limbs-guitar")
BIND_POSE_VERTEX_WARP_SCOPES = (
    "none",
    "hands",
    "forearms-hands",
    "upper-limbs",
)
BASE_HAND_DETAIL_TOKENS = (
    "-index",
    "-middlefinger",
    "-ringfinger",
    "-pinky",
    "-thumb",
)
BIND_POSE_HAND_TOKENS = ("-hand", *BASE_HAND_DETAIL_TOKENS)

# Stock GH2 guitarist controller locals from glam1.milo_ps2. These are not
# visual mesh binds; they are the ordinary Trans targets that fret/strum IK
# clips solve against under bone_fret.mesh and bone_strum.mesh.
GH2_STOCK_GUITAR_HAND_TARGET_LOCALS = {
    "bone_fret_hand": [
        0.910481, 0.385046, 0.15088,
        0.385335, -0.922336, 0.0285125,
        0.150141, 0.0321795, -0.988141,
        -6.27464, -0.453556, -4.32057,
    ],
    "bone_strum_hand": [
        0.683027, -0.419636, 0.597812,
        0.289627, 0.90699, 0.305753,
        -0.670515, -0.0356949, 0.741037,
        -6.73835, -1.31678, -3.08712,
    ],
}

GH2_STOCK_GUITAR_MAIN_POS = [-2.99257, 9.03461, -3.23128]
GH2_STOCK_GUITAR_MAIN_QUAT_WXYZ = [0.792352, 0.132054, 0.29899, 0.515122]


GH2_MESH_ALIAS_PARENTS = {
    "bone_pelvis": "",
    "bone_spine1": "bone_pelvis",
    "bone_spine2": "bone_spine1",
    "bone_spine3": "bone_spine2",
    "bone_neck": "bone_spine3",
    "bone_head": "bone_neck",
    "bone_L-clavicle": "bone_spine3",
    "bone_L-upperArm": "bone_L-clavicle",
    "bone_L-foreArm": "bone_L-upperArm",
    "bone_L-hand": "bone_L-foreArm",
    "bone_R-clavicle": "bone_spine3",
    "bone_R-upperArm": "bone_R-clavicle",
    "bone_R-foreArm": "bone_R-upperArm",
    "bone_R-hand": "bone_R-foreArm",
    "bone_L-thigh": "bone_pelvis",
    "bone_L-knee": "bone_L-thigh",
    "bone_L-ankle": "bone_L-knee",
    "bone_L-toe": "bone_L-ankle",
    "bone_R-thigh": "bone_pelvis",
    "bone_R-knee": "bone_R-thigh",
    "bone_R-ankle": "bone_R-knee",
    "bone_R-toe": "bone_R-ankle",
    "bone_pos_guitar": "bone_pelvis",
    "bone_fret": "bone_pos_guitar",
    "bone_strum": "bone_pos_guitar",
    "bone_fret_hand": "bone_fret",
    "bone_strum_hand": "bone_strum",
    "bone_ik_hand_guitar_l": "bone_L-hand",
    "bone_ik_hand_guitar_r": "bone_R-hand",
}
CONTROL_ROOT_PELVIS_PARENT = False


def gh2_mesh_alias_parents() -> dict[str, str]:
    parents = dict(GH2_MESH_ALIAS_PARENTS)
    if CONTROL_ROOT_PELVIS_PARENT:
        parents["bone_pelvis"] = "Control_Root"
    return parents

for side, hand in (("L", "bone_L-hand"), ("R", "bone_R-hand")):
    for finger in ("index", "middlefinger", "ringfinger", "pinky"):
        GH2_MESH_ALIAS_PARENTS[f"bone_{side}-{finger}01"] = hand
        GH2_MESH_ALIAS_PARENTS[f"bone_{side}-{finger}02"] = f"bone_{side}-{finger}01"
        GH2_MESH_ALIAS_PARENTS[f"bone_{side}-{finger}03"] = f"bone_{side}-{finger}02"
    GH2_MESH_ALIAS_PARENTS[f"bone_{side}-thumb01"] = hand
    GH2_MESH_ALIAS_PARENTS[f"bone_{side}-thumb02"] = f"bone_{side}-thumb01"
    GH2_MESH_ALIAS_PARENTS[f"bone_{side}-thumb03"] = f"bone_{side}-thumb02"


def basis_change_matrix12(matrix: list[float], translation_scale: float) -> list[float]:
    # Row-vector affine basis bridge for GH3 PS2 character bones:
    # target [x_side, y_depth, z_up] = source [-x_side, z_depth, y_up].
    order = [0, 2, 1]
    signs = [-1.0, 1.0, 1.0]
    out = [0.0] * 12
    for r in range(3):
        for c in range(3):
            out[r * 3 + c] = (
                signs[r] * signs[c] * float(matrix[order[r] * 3 + order[c]])
            )
    out[9] = -float(matrix[9]) * translation_scale
    out[10] = float(matrix[11]) * translation_scale
    out[11] = float(matrix[10]) * translation_scale
    return out


def skeleton_bind_scale(skeleton: dict[str, Any]) -> float:
    del skeleton
    return GH3_PS2_SKELETON_TO_GH2_SCALE


def source_local_bind_matrix12(
    bone: dict[str, Any],
    translation_scale: float,
) -> list[float]:
    # Community importers (GHTools/NXTools) build GH3 edit bones from the
    # offset/quaternion tables. The .ske matrix block is a local inverse pose
    # helper, not the world bind matrix GH2's Trans tree needs.
    local = quat_to_matrix12(
        [float(value) for value in bone["nxtools_quat_wxyz"]],
        [float(value) for value in bone["nxtools_offset"][:3]],
    )
    order = [2, 1, 0]
    out = [0.0] * 12
    for r in range(3):
        for c in range(3):
            out[r * 3 + c] = float(local[order[r] * 3 + order[c]])
    for c in range(3):
        out[9 + c] = float(local[9 + order[c]]) * translation_scale
    return out


def source_world_bind_matrix12(
    bone: dict[str, Any],
    translation_scale: float,
) -> list[float]:
    """Decode the SKE inverse-bind matrix into a GH2-basis world bind.

    NXTools exposes the four SKE matrix rows in their stored row-vector order.
    The block is world-to-bone (inverse bind), so its affine inverse is the
    authored bone-to-world bind.  Applying the same basis bridge as the skin
    positions reproduces NXTools' fully baked Blender bone heads, without
    requiring Blender at conversion time.
    """
    inverse_bind = world_matrix12(bone["raw_matrix_rows_nxtools_order"])
    source_world = invert_matrix12_row_vector(inverse_bind)
    return basis_change_matrix12(source_world, translation_scale)


def compose_source_worlds(
    source_locals: dict[str, list[float]],
    parent_by_source: dict[str, str | None] | None = None,
) -> dict[str, list[float]]:
    source_worlds: dict[str, list[float]] = {}

    def resolve(source_name: str) -> list[float]:
        if source_name in source_worlds:
            return source_worlds[source_name]
        local = source_locals[source_name]
        parent_source = (
            source_parent_name(source_name)
            or (parent_by_source or {}).get(source_name)
        )
        if parent_source in source_locals:
            world = multiply_matrix12_row_vector(local, resolve(parent_source))
        else:
            world = list(local)
        source_worlds[source_name] = world
        return world

    for source_name in source_locals:
        resolve(source_name)
    return source_worlds


def identity_matrix12() -> list[float]:
    return [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0]


def write_u16(out: bytearray, value: int) -> None:
    out.extend(struct.pack("<H", value))


def write_u8(out: bytearray, value: int) -> None:
    out.extend(struct.pack("<B", value))


def write_u32(out: bytearray, value: int) -> None:
    out.extend(struct.pack("<I", value))


def write_f32(out: bytearray, value: float) -> None:
    out.extend(struct.pack("<f", float(value)))


def write_string(out: bytearray, value: str) -> None:
    data = value.encode("utf-8")
    write_u32(out, len(data))
    out.extend(data)


def write_bytes(out: bytearray, data: bytes) -> None:
    write_u32(out, len(data))
    out.extend(data)


def load_chunks(path: Path) -> list[dict[str, Any]]:
    chunks = []
    with gzip.open(path, "rt", encoding="utf-8") as handle:
        for line in handle:
            chunks.append(json.loads(line))
    return chunks


def quat_to_matrix12(quat_wxyz: list[float], offset_xyz: list[float]) -> list[float]:
    w, x, y, z = [float(value) for value in quat_wxyz]
    xx = x * x
    yy = y * y
    zz = z * z
    xy = x * y
    xz = x * z
    yz = y * z
    wx = w * x
    wy = w * y
    wz = w * z
    return [
        1.0 - 2.0 * (yy + zz),
        2.0 * (xy - wz),
        2.0 * (xz + wy),
        2.0 * (xy + wz),
        1.0 - 2.0 * (xx + zz),
        2.0 * (yz - wx),
        2.0 * (xz - wy),
        2.0 * (yz + wx),
        1.0 - 2.0 * (xx + yy),
        float(offset_xyz[0]),
        float(offset_xyz[1]),
        float(offset_xyz[2]),
    ]


def normalize_quat(q: list[float]) -> list[float]:
    length = sum(float(value) * float(value) for value in q) ** 0.5
    if length <= 0.0:
        return [1.0, 0.0, 0.0, 0.0]
    return [float(value) / length for value in q]


def quat_to_matrix3(quat_wxyz: list[float]) -> list[list[float]]:
    w, x, y, z = normalize_quat(quat_wxyz)
    xx = x * x
    yy = y * y
    zz = z * z
    xy = x * y
    xz = x * z
    yz = y * z
    wx = w * x
    wy = w * y
    wz = w * z
    return [
        [1.0 - 2.0 * (yy + zz), 2.0 * (xy - wz), 2.0 * (xz + wy)],
        [2.0 * (xy + wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz - wx)],
        [2.0 * (xz - wy), 2.0 * (yz + wx), 1.0 - 2.0 * (xx + yy)],
    ]


def matrix12_rotation(value: list[float]) -> list[list[float]]:
    return [[float(value[r * 3 + c]) for c in range(3)] for r in range(3)]


def set_matrix12_rotation(value: list[float], rotation: list[list[float]]) -> None:
    for r in range(3):
        for c in range(3):
            value[r * 3 + c] = float(rotation[r][c])


def mat3_transpose(matrix: list[list[float]]) -> list[list[float]]:
    return [[matrix[c][r] for c in range(3)] for r in range(3)]


def mat3_mul(left: list[list[float]], right: list[list[float]]) -> list[list[float]]:
    return [
        [sum(left[r][k] * right[k][c] for k in range(3)) for c in range(3)]
        for r in range(3)
    ]


def gh3_matrix_to_gh2_matrix(matrix: list[list[float]]) -> list[list[float]]:
    order = [2, 1, 0]
    return [
        [matrix[order[r]][order[c]] for c in range(3)]
        for r in range(3)
    ]


def from_ghwt_bone_matrix_rows(rows: list[list[float]]) -> list[list[float]]:
    # NXTools FromGHWTBoneMatrix: X2/Y2/Z2/W2, X0/Y0/Z0/W0,
    # X1/Y1/Z1/W1, X3/Y3/Z3/W3.
    x, y, z, w = rows
    return [
        [float(x[2]), float(x[0]), float(x[1]), float(w[2])],
        [-float(z[2]), -float(z[0]), -float(z[1]), -float(w[0])],
        [float(y[2]), float(y[0]), float(y[1]), float(w[1])],
        [float(z[3]), float(x[3]), float(y[3]), float(w[3])],
    ]


def source_edit_pose_by_name(skeleton: dict[str, Any], name_by_checksum: dict[str, str]) -> dict[str, dict[str, Any]]:
    poses: dict[str, dict[str, Any]] = {}
    for bone in skeleton["bones"]:
        source_name = resolved_bone_name(bone, name_by_checksum)
        matrix = from_ghwt_bone_matrix_rows(bone["raw_matrix_rows_nxtools_order"])
        poses[source_name] = {
            "rotation": [[float(matrix[r][c]) for c in range(3)] for r in range(3)],
        }
    return poses


def source_quat_to_default_pose_delta(
    quat_wxyz: list[float],
    edit_pose: dict[str, Any] | None,
) -> list[list[float]]:
    frame = quat_to_matrix3(quat_wxyz)
    if not edit_pose:
        return gh3_matrix_to_gh2_matrix(frame)
    delta = mat3_mul(mat3_transpose(edit_pose["rotation"]), frame)
    return gh3_matrix_to_gh2_matrix(delta)


def world_matrix12(rows: list[list[float]]) -> list[float]:
    return [
        float(rows[0][0]), float(rows[0][1]), float(rows[0][2]),
        float(rows[1][0]), float(rows[1][1]), float(rows[1][2]),
        float(rows[2][0]), float(rows[2][1]), float(rows[2][2]),
        float(rows[3][0]), float(rows[3][1]), float(rows[3][2]),
    ]


def multiply_matrix12_row_vector(left: list[float], right: list[float]) -> list[float]:
    out = [0.0] * 12
    for r in range(3):
        for c in range(3):
            out[r * 3 + c] = sum(left[r * 3 + k] * right[k * 3 + c] for k in range(3))
    for c in range(3):
        out[9 + c] = (
            left[9] * right[c]
            + left[10] * right[3 + c]
            + left[11] * right[6 + c]
            + right[9 + c]
        )
    return out


def invert_matrix12_row_vector(value: list[float]) -> list[float]:
    determinant = (
        value[0] * (value[4] * value[8] - value[5] * value[7])
        - value[1] * (value[3] * value[8] - value[5] * value[6])
        + value[2] * (value[3] * value[7] - value[4] * value[6])
    )
    if abs(determinant) <= 1.0e-8:
        raise ValueError("cannot invert singular transform")
    inv_det = 1.0 / determinant
    out = [0.0] * 12
    out[0] = (value[4] * value[8] - value[5] * value[7]) * inv_det
    out[1] = (value[2] * value[7] - value[1] * value[8]) * inv_det
    out[2] = (value[1] * value[5] - value[2] * value[4]) * inv_det
    out[3] = (value[5] * value[6] - value[3] * value[8]) * inv_det
    out[4] = (value[0] * value[8] - value[2] * value[6]) * inv_det
    out[5] = (value[2] * value[3] - value[0] * value[5]) * inv_det
    out[6] = (value[3] * value[7] - value[4] * value[6]) * inv_det
    out[7] = (value[1] * value[6] - value[0] * value[7]) * inv_det
    out[8] = (value[0] * value[4] - value[1] * value[3]) * inv_det
    for c in range(3):
        out[9 + c] = -(
            value[9] * out[c]
            + value[10] * out[3 + c]
            + value[11] * out[6 + c]
        )
    return out


def transform_point_row_vector(
    matrix: list[float],
    point: list[float],
) -> list[float]:
    return [
        sum(float(point[row]) * float(matrix[row * 3 + column]) for row in range(3))
        + float(matrix[9 + column])
        for column in range(3)
    ]


def transform_direction_row_vector(
    matrix: list[float],
    direction: list[float],
) -> list[float]:
    return [
        sum(
            float(direction[row]) * float(matrix[row * 3 + column])
            for row in range(3)
        )
        for column in range(3)
    ]


def bind_pose_vertex_warp_bone(name: str, scope: str) -> bool:
    if scope == "none":
        return False
    if not name.startswith(("bone_L-", "bone_R-")):
        return False
    hand = any(
        token in name
        for token in BIND_POSE_HAND_TOKENS
    )
    if scope == "hands":
        return hand
    if scope == "forearms-hands":
        return hand or "-foreArm" in name
    if scope == "upper-limbs":
        return hand or any(
            token in name for token in ("-clavicle", "-upperArm", "-foreArm")
        )
    raise ValueError(f"unsupported bind-pose vertex warp scope: {scope}")


def bind_pose_vertex_warp_factor(
    name: str, scope: str, forearm_weight: float, hand_weight: float = 1.0
) -> float:
    if not bind_pose_vertex_warp_bone(name, scope):
        return 0.0
    if any(token in name for token in BIND_POSE_HAND_TOKENS):
        return hand_weight
    if scope == "forearms-hands" and "-foreArm" in name:
        return forearm_weight
    return 1.0


def wrist_ramp_factor(
    axis_fraction: float, start: float, strength: float
) -> float:
    if axis_fraction <= start or strength <= 0.0:
        return 0.0
    normalized = min(1.0, (axis_fraction - start) / max(1.0 - start, 1.0e-9))
    smooth = normalized * normalized * (3.0 - 2.0 * normalized)
    return strength * smooth


def chunk_sphere(vertices: list[dict[str, Any]]) -> list[float]:
    if not vertices:
        return [0.0, 0.0, 0.0, 0.0]
    center = [
        sum(float(vertex["position"][axis]) for vertex in vertices) / len(vertices)
        for axis in range(3)
    ]
    radius = max(
        sum(
            (float(vertex["position"][axis]) - center[axis]) ** 2
            for axis in range(3)
        )
        ** 0.5
        for vertex in vertices
    )
    return center + [radius]


def warp_chunks_to_target_bind(
    chunks: list[dict[str, Any]],
    source_transforms: dict[str, dict[str, Any]],
    target_transforms: dict[str, dict[str, Any]],
    scope: str,
    forearm_weight: float = 1.0,
    hand_weight: float = 1.0,
    wrist_ramp_weight: float = 0.0,
    wrist_ramp_start: float = 0.6,
) -> dict[str, Any]:
    if not 0.0 <= forearm_weight <= 1.0:
        raise ValueError("bind-pose forearm warp weight must be in [0, 1]")
    if not 0.0 <= hand_weight <= 1.0:
        raise ValueError("bind-pose hand warp weight must be in [0, 1]")
    if not 0.0 <= wrist_ramp_weight <= 1.0:
        raise ValueError("bind-pose wrist ramp weight must be in [0, 1]")
    if not 0.0 <= wrist_ramp_start < 1.0:
        raise ValueError("bind-pose wrist ramp start must be in [0, 1)")
    if scope == "none":
        return {
            "scope": scope,
            "forearm_weight": forearm_weight,
            "hand_weight": hand_weight,
            "wrist_ramp_weight": wrist_ramp_weight,
            "wrist_ramp_start": wrist_ramp_start,
            "vertex_count": sum(len(chunk["vertices"]) for chunk in chunks),
            "changed_vertex_count": 0,
            "mean_displacement": 0.0,
            "max_displacement": 0.0,
            "warped_bones": [],
        }

    warp_by_bone: dict[str, list[float]] = {}
    warp_factor_by_bone: dict[str, float] = {}
    for bone in sorted(
        {
            bone
            for chunk in chunks
            for bone in chunk["bone_slots"]
            if bind_pose_vertex_warp_bone(bone, scope)
        }
    ):
        source = source_transforms.get(bone)
        target = target_transforms.get(bone)
        if source is None or target is None:
            raise KeyError(
                f"bind-pose vertex warp missing {bone}: "
                f"source={source is not None} target={target is not None}"
            )
        warp_by_bone[bone] = multiply_matrix12_row_vector(
            invert_matrix12_row_vector(source["world"]),
            target["world"],
        )
        warp_factor_by_bone[bone] = bind_pose_vertex_warp_factor(
            bone, scope, forearm_weight, hand_weight
        )

    displacements: list[float] = []
    maximum_wrist_ramp_factor = 0.0
    vertex_count = 0
    changed_vertex_count = 0
    for chunk in chunks:
        chunk_changed = False
        slots = list(chunk["bone_slots"])
        for vertex in chunk["vertices"]:
            vertex_count += 1
            position = [float(value) for value in vertex["position"]]
            normal = [float(value) for value in vertex["normal"]]
            warped_position = [0.0, 0.0, 0.0]
            warped_normal = [0.0, 0.0, 0.0]
            total_weight = 0.0
            scoped_weight = 0.0
            for slot_index, weight_value in enumerate(vertex["color_or_weights"]):
                weight = float(weight_value)
                if weight <= 0.0:
                    continue
                total_weight += weight
                bone = slots[slot_index] if slot_index < len(slots) else ""
                warp = warp_by_bone.get(bone)
                wrist_factor = 0.0
                wrist_warp = None
                if (
                    warp is None
                    and scope == "hands"
                    and wrist_ramp_weight > 0.0
                    and bone in {"bone_L-foreArm.mesh", "bone_R-foreArm.mesh"}
                ):
                    side = "L" if "bone_L-" in bone else "R"
                    hand_bone = f"bone_{side}-hand.mesh"
                    elbow = source_transforms[bone]["world"][9:12]
                    wrist = source_transforms[hand_bone]["world"][9:12]
                    axis_vector = [
                        wrist[component] - elbow[component]
                        for component in range(3)
                    ]
                    denominator = sum(value * value for value in axis_vector)
                    axis_fraction = (
                        sum(
                            (position[component] - elbow[component])
                            * axis_vector[component]
                            for component in range(3)
                        )
                        / denominator
                        if denominator > 1.0e-12
                        else 0.0
                    )
                    wrist_factor = wrist_ramp_factor(
                        axis_fraction, wrist_ramp_start, wrist_ramp_weight
                    )
                    wrist_warp = warp_by_bone.get(hand_bone)
                if warp is None:
                    if wrist_warp is not None and wrist_factor > 0.0:
                        scoped_weight += weight
                        maximum_wrist_ramp_factor = max(
                            maximum_wrist_ramp_factor, wrist_factor
                        )
                        target_position = transform_point_row_vector(
                            wrist_warp, position
                        )
                        target_normal = transform_direction_row_vector(
                            wrist_warp, normal
                        )
                        posed_position = [
                            position[axis]
                            + wrist_factor * (target_position[axis] - position[axis])
                            for axis in range(3)
                        ]
                        posed_normal = [
                            normal[axis]
                            + wrist_factor * (target_normal[axis] - normal[axis])
                            for axis in range(3)
                        ]
                    else:
                        posed_position = position
                        posed_normal = normal
                else:
                    scoped_weight += weight
                    factor = warp_factor_by_bone[bone]
                    target_position = transform_point_row_vector(warp, position)
                    target_normal = transform_direction_row_vector(warp, normal)
                    if factor >= 1.0:
                        posed_position = target_position
                        posed_normal = target_normal
                    elif factor <= 0.0:
                        posed_position = position
                        posed_normal = normal
                    else:
                        posed_position = [
                            position[axis]
                            + factor * (target_position[axis] - position[axis])
                            for axis in range(3)
                        ]
                        posed_normal = [
                            normal[axis]
                            + factor * (target_normal[axis] - normal[axis])
                            for axis in range(3)
                        ]
                for axis in range(3):
                    warped_position[axis] += weight * posed_position[axis]
                    warped_normal[axis] += weight * posed_normal[axis]
            if total_weight < 1.0:
                residual = 1.0 - total_weight
                for axis in range(3):
                    warped_position[axis] += residual * position[axis]
                    warped_normal[axis] += residual * normal[axis]
            elif total_weight > 1.0 + 1.0e-6:
                warped_position = [value / total_weight for value in warped_position]
                warped_normal = [value / total_weight for value in warped_normal]
            if scoped_weight <= 1.0e-8:
                continue
            normal_length = sum(value * value for value in warped_normal) ** 0.5
            if normal_length > 1.0e-12:
                warped_normal = [value / normal_length for value in warped_normal]
            displacement = sum(
                (warped_position[axis] - position[axis]) ** 2 for axis in range(3)
            ) ** 0.5
            vertex["position"] = warped_position
            vertex["normal"] = warped_normal
            displacements.append(displacement)
            if displacement > 1.0e-8:
                changed_vertex_count += 1
                chunk_changed = True
        if chunk_changed:
            chunk["sphere"] = chunk_sphere(chunk["vertices"])

    return {
        "scope": scope,
        "forearm_weight": forearm_weight,
        "hand_weight": hand_weight,
        "wrist_ramp_weight": wrist_ramp_weight,
        "wrist_ramp_start": wrist_ramp_start,
        "maximum_wrist_ramp_factor": maximum_wrist_ramp_factor,
        "vertex_count": vertex_count,
        "changed_vertex_count": changed_vertex_count,
        "mean_displacement": (
            sum(displacements) / len(displacements) if displacements else 0.0
        ),
        "max_displacement": max(displacements, default=0.0),
        "warped_bones": sorted(warp_by_bone),
        "warp_factors": dict(sorted(warp_factor_by_bone.items())),
    }


def retarget_guitar_hand_proxies(
    transforms: dict[str, dict[str, list[float]]],
) -> None:
    for target, desired_local in GH2_STOCK_GUITAR_HAND_TARGET_LOCALS.items():
        parent_name = GH2_MESH_ALIAS_PARENTS[target]
        parent = transforms.get(parent_name)
        if parent is None:
            continue
        target_transform = transforms.get(target)
        if target_transform is None:
            target_transform = {
                "source_name": target,
                "parent_name": parent_name,
                "local": identity_matrix12(),
                "world": identity_matrix12(),
            }
            transforms[target] = target_transform
        target_transform["parent_name"] = parent_name
        target_transform["local"] = list(desired_local)
        target_transform["world"] = multiply_matrix12_row_vector(desired_local, parent["world"])
        target_transform["retarget_reason"] = (
            "GH3 hand helpers are not GH2 fret/strum IK controller locals. "
            "Use the stock GH2 controller local under the converted fret/strum "
            "bone so authored clips produce ordinary reachable hand targets."
        )


def apply_parent_override(
    transforms: dict[str, dict[str, list[float]]],
    child_name: str,
    parent_name: str,
) -> None:
    child = transforms.get(child_name)
    parent = transforms.get(parent_name)
    if child is None or parent is None:
        return
    child["parent_name"] = parent_name
    child["local"] = multiply_matrix12_row_vector(
        child["world"],
        invert_matrix12_row_vector(parent["world"]),
    )


def refresh_child_worlds(
    transforms: dict[str, dict[str, list[float]]],
    parent_name: str,
) -> None:
    for child_name, child in transforms.items():
        if child_name == parent_name or child.get("parent_name") != parent_name:
            continue
        parent = transforms[parent_name]
        child["world"] = multiply_matrix12_row_vector(child["local"], parent["world"])
        refresh_child_worlds(transforms, child_name)


def retarget_common_bones_to_gh2_parent_graph(
    transforms: dict[str, dict[str, list[float]]],
) -> None:
    for bone_name, parent_name in gh2_mesh_alias_parents().items():
        transform = transforms.get(bone_name)
        if transform is None:
            continue
        transform["parent_name"] = parent_name
        parent = transforms.get(parent_name)
        if parent is not None:
            transform["local"] = multiply_matrix12_row_vector(
                transform["world"],
                invert_matrix12_row_vector(parent["world"]),
            )
        else:
            transform["local"] = list(transform["world"])
        transform["retarget_reason"] = (
            "GH2 guitarist animation banks drive the common bone hierarchy "
            "without GH3's Control_Root. Preserve the imported bind world but "
            "publish the same parent graph used by stock GH2 .mesh bones."
        )


def apply_gh3_helper_parent_overrides(
    transforms: dict[str, dict[str, list[float]]],
) -> None:
    for child_name, parent_name in (
        ("bone_gh3_c00e3395", "bone_pos_guitar"),
        ("bone_gh3_00d7df90", "bone_gh3_c00e3395"),
        ("bone_gh3_a2a3f6df", "bone_gh3_c00e3395"),
        ("bone_ik_hand_guitar_l", "bone_gh3_00d7df90"),
        ("bone_ik_hand_guitar_r", "bone_gh3_a2a3f6df"),
    ):
        apply_parent_override(transforms, child_name, parent_name)


def retarget_guitar_attach_root(
    transforms: dict[str, dict[str, list[float]]],
) -> None:
    guitar = transforms.get("bone_pos_guitar")
    if guitar is None:
        return
    parent_name = "Control_Root" if "Control_Root" in transforms else guitar.get("parent_name", "")
    parent = transforms.get(parent_name)
    desired_local = list(guitar["local"])
    if parent is not None:
        desired_world = multiply_matrix12_row_vector(desired_local, parent["world"])
    else:
        desired_world = list(desired_local)
    guitar["local"] = desired_local
    guitar["world"] = desired_world
    guitar["parent_name"] = parent_name
    guitar["retarget_reason"] = (
        "GH3's guitar body helper is a local instrument attach offset. Parent "
        "that authored local offset under GH2's Control_Root so the external "
        "character satisfies the stock guitarist prop contract without a "
        "runtime character-specific basis swap."
    )
    refresh_child_worlds(transforms, "bone_pos_guitar")


def add_gh2_attach_aliases(
    transforms: dict[str, dict[str, list[float]]],
) -> None:
    guitar = transforms.get("bone_pos_guitar")
    if guitar is None or "bone_pos_guitar.mesh" in transforms:
        return
    desired_local = [
        -1.26164e-06, 2.60940e-06, 1.0,
        -0.0979263, -0.995194, 2.47358e-06,
        0.995194, -0.0979263, 1.50816e-06,
        -34.3648, 1.73077, -0.0862666,
    ]
    parent_name = "bone_pelvis" if "bone_pelvis" in transforms else guitar.get("parent_name", "")
    parent = transforms.get(parent_name)
    if parent is not None:
        desired_world = multiply_matrix12_row_vector(desired_local, parent["world"])
    else:
        desired_world = list(desired_local)
    transforms["bone_pos_guitar.mesh"] = {
        "source_name": guitar["source_name"],
        "parent_name": parent_name,
        "local": desired_local,
        "world": desired_world,
        "retarget_reason": (
            "GH2 stock guitar props attach to bone_pos_guitar.mesh. Mirror "
            "the stock Alterna/GH2 local prop anchor under the converted "
            "pelvis so external characters satisfy the base guitarist prop "
            "contract without runtime character-specific code."
        ),
    }


def load_stock_mesh_bind_transforms(
    stock_rig_path: Path | None,
    package_name: str,
) -> dict[str, dict[str, Any]]:
    if stock_rig_path is None or not stock_rig_path.is_file():
        return {}
    rig = json.loads(stock_rig_path.read_text(encoding="utf-8"))
    locals_by_name = {
        str(node["name"]): [float(value) for value in node["local"]]
        for node in rig.get("nodes", [])
        if node.get("kind") == "Trans" and str(node.get("name", "")).endswith(".mesh")
    }
    parent_by_name = {
        str(node["name"]): (
            PACKAGE_ROOT_PARENT
            if str(node.get("parent", "")) == "glam1"
            else str(node.get("parent", ""))
        )
        for node in rig.get("nodes", [])
        if node.get("kind") == "Trans" and str(node.get("name", "")).endswith(".mesh")
    }
    worlds: dict[str, list[float]] = {}

    def resolve(name: str) -> list[float]:
        if name in worlds:
            return worlds[name]
        local = locals_by_name[name]
        parent = parent_by_name.get(name, "")
        if parent in locals_by_name:
            world = multiply_matrix12_row_vector(local, resolve(parent))
        else:
            world = list(local)
        worlds[name] = world
        return world

    transforms = {}
    for name, local in locals_by_name.items():
        source_name = name[:-5]
        transforms[name] = {
            "source_name": source_name,
            "parent_name": parent_by_name.get(name, "").replace("glam1", package_name),
            "local": list(local),
            "world": resolve(name),
            "retarget_reason": (
                "Stock GH2 guitarist .mesh bind imported from glam1 so "
                "external Midori skinning uses the same base-character "
                "skeleton contract as ordinary GH2 characters."
            ),
        }
    return transforms


def add_gh2_mesh_skeleton_aliases(
    transforms: dict[str, dict[str, list[float]]],
    stock_mesh_bind: dict[str, dict[str, Any]] | None = None,
    stock_hand_detail: bool = False,
    stock_bind_scope: str = "all",
    preserve_guitar_attach_local: bool = False,
) -> None:
    for bone_name, parent_base in gh2_mesh_alias_parents().items():
        source = transforms.get(bone_name)
        alias_name = bone_name + ".mesh"
        if source is None:
            stock_alias = (
                stock_mesh_bind.get(alias_name) if stock_mesh_bind else None
            )
            if stock_alias is None:
                continue
            parent_alias = (
                parent_base
                if parent_base == "Control_Root"
                else parent_base + ".mesh"
                if parent_base
                else ""
            )
            parent = transforms.get(parent_alias)
            desired_local = list(stock_alias["local"])
            desired_world = (
                multiply_matrix12_row_vector(desired_local, parent["world"])
                if parent is not None
                else list(stock_alias["world"])
            )
            transforms[alias_name] = {
                **stock_alias,
                "source_name": bone_name,
                "parent_name": parent_alias,
                "local": desired_local,
                "world": desired_world,
                "retarget_reason": (
                    "Source rig omits this unweighted GH2 controller leaf; "
                    "retain the stock bind-local alias under the converted "
                    "parent so native hand clip targets remain complete."
                ),
            }
            continue
        hand_detail = any(token in bone_name for token in BASE_HAND_DETAIL_TOKENS)
        use_stock_alias = stock_bind_scope == "all" or (
            stock_bind_scope == "upper-limbs-guitar"
            and (
                bone_name.startswith("bone_L-")
                or bone_name.startswith("bone_R-")
                or bone_name in {
                    "bone_pos_guitar",
                    "bone_fret",
                    "bone_strum",
                    "bone_fret_hand",
                    "bone_strum_hand",
                    "bone_ik_hand_guitar_l",
                    "bone_ik_hand_guitar_r",
                }
            )
        )
        if (
            use_stock_alias
            and stock_mesh_bind
            and alias_name in stock_mesh_bind
            and (stock_hand_detail or not hand_detail)
            and not (
                preserve_guitar_attach_local
                and bone_name == "bone_pos_guitar"
                and alias_name in transforms
            )
        ):
            stock_alias = dict(stock_mesh_bind[alias_name])
            parent_alias = (
                parent_base
                if parent_base == "Control_Root"
                else parent_base + ".mesh"
                if parent_base
                else ""
            )
            desired_world = list(stock_alias["world"])
            parent = transforms.get(parent_alias)
            desired_local = (
                multiply_matrix12_row_vector(
                    desired_world,
                    invert_matrix12_row_vector(parent["world"]),
                )
                if parent is not None
                else list(stock_alias["local"])
            )
            transforms[alias_name] = {
                **stock_alias,
                "source_name": source.get("source_name", bone_name),
                "parent_name": parent_alias,
                "local": desired_local,
                "world": desired_world,
                "retarget_reason": (
                    "Stock GH2 .mesh bind reused for external-file contract, "
                    "but source_name and parent are rebound to the active "
                    "Midori target graph so animation staging can resolve "
                    "the GH3 source bone."
                ),
            }
            continue
        parent_alias = (
            parent_base
            if parent_base == "Control_Root"
            else parent_base + ".mesh"
            if parent_base
            else ""
        )
        desired_world = list(source["world"])
        if alias_name in transforms and bone_name not in GH2_STOCK_GUITAR_HAND_TARGET_LOCALS:
            desired_world = list(transforms[alias_name]["world"])
        parent = transforms.get(parent_alias)
        if parent is not None:
            desired_local = multiply_matrix12_row_vector(
                desired_world,
                invert_matrix12_row_vector(parent["world"]),
            )
        else:
            desired_local = list(desired_world)
        transforms[alias_name] = {
            "source_name": source.get("source_name", bone_name),
            "parent_name": parent_alias,
            "local": desired_local,
            "world": desired_world,
            "retarget_reason": (
                "GH2 guitarist clipsets and IK bind hand/finger/guitar "
                "controls through .mesh-named skeleton Trans objects. This "
                "alias preserves the generated bind world while exposing the "
                "stock external-file contract."
            ),
        }


def omit_base_hand_overlay_transform(name: str) -> bool:
    # GH2 character packages expose one common skeleton, using .mesh-named
    # Trans objects. Keeping the imported unsuffixed common bones beside those
    # aliases makes suffix-tolerant clip/controller lookup resolve the hidden
    # tree first, so the visible skinned tree never receives the pose.
    return name in gh2_mesh_alias_parents()


def retarget_common_bones_to_stock_bind(
    transforms: dict[str, dict[str, Any]],
    stock_mesh_bind: dict[str, dict[str, Any]],
    stock_hand_detail: bool = False,
    stock_bind_scope: str = "all",
    preserve_guitar_attach_local: bool = False,
) -> None:
    if not stock_mesh_bind:
        return
    original_worlds = {
        name: list(transform["world"])
        for name, transform in transforms.items()
        if not name.endswith(".mesh")
    }
    common = set(gh2_mesh_alias_parents())
    for bone_name, parent_name in gh2_mesh_alias_parents().items():
        if stock_bind_scope == "upper-limbs-guitar" and not (
            bone_name.startswith("bone_L-")
            or bone_name.startswith("bone_R-")
            or bone_name in {
                "bone_pos_guitar",
                "bone_fret",
                "bone_strum",
                "bone_fret_hand",
                "bone_strum_hand",
                "bone_ik_hand_guitar_l",
                "bone_ik_hand_guitar_r",
            }
        ):
            continue
        if preserve_guitar_attach_local and bone_name == "bone_pos_guitar":
            continue
        if not stock_hand_detail and any(token in bone_name for token in BASE_HAND_DETAIL_TOKENS):
            continue
        stock = stock_mesh_bind.get(bone_name + ".mesh")
        if stock is None:
            continue
        if bone_name not in transforms:
            transforms[bone_name] = {
                "source_name": stock.get("source_name", bone_name),
                "parent_name": parent_name,
                "local": list(stock["local"]),
                "world": list(stock["world"]),
            }
        else:
            transforms[bone_name]["parent_name"] = parent_name
            transforms[bone_name]["local"] = list(stock["local"])
            transforms[bone_name]["world"] = list(stock["world"])
        transforms[bone_name]["retarget_reason"] = (
            "Common guitarist bone uses stock GH2 bind local/world so ordinary "
            "GH2 CharClipSamples drive external Midori through the same "
            "skeleton contract as base characters."
        )
    for name, transform in transforms.items():
        if name in common or name.endswith(".mesh"):
            continue
        old_world = original_worlds.get(name)
        if old_world is None:
            continue
        parent = transforms.get(transform.get("parent_name", ""))
        transform["world"] = old_world
        if parent is not None:
            transform["local"] = multiply_matrix12_row_vector(
                old_world,
                invert_matrix12_row_vector(parent["world"]),
            )
        else:
            transform["local"] = list(old_world)
        transform["retarget_reason"] = (
            "Source-only helper keeps its imported bind world after common "
            "bones switch to stock GH2 bind; local recomputed under the new "
            "parent for ordinary external-file animation."
        )


def compensate_guitar_helper_for_main_anchor(
    transforms: dict[str, dict[str, Any]],
    main_guitar_pos_offset: list[float],
) -> bool:
    helper = transforms.get("bone_gh3_c00e3395")
    pelvis = transforms.get("bone_pelvis.mesh") or transforms.get("bone_pelvis")
    if helper is None or pelvis is None:
        return False
    forced_parent_local = quat_to_matrix12(
        GH2_STOCK_GUITAR_MAIN_QUAT_WXYZ,
        [
            GH2_STOCK_GUITAR_MAIN_POS[index] + main_guitar_pos_offset[index]
            for index in range(3)
        ],
    )
    forced_parent_world = multiply_matrix12_row_vector(
        forced_parent_local,
        pelvis["world"],
    )
    desired_helper_world = list(helper["world"])
    helper["parent_name"] = "bone_pos_guitar"
    helper["local"] = multiply_matrix12_row_vector(
        desired_helper_world,
        invert_matrix12_row_vector(forced_parent_world),
    )
    helper["retarget_reason"] = (
        "GH3 guitar-body helper local compensated against the forced GH2 "
        "guitar-main bone_pos_guitar.mesh animation anchor so the helper "
        "keeps its authored world when the main clipset overrides the parent."
    )
    refresh_child_worlds(transforms, "bone_gh3_c00e3395")
    return True


def bone_transforms_from_skeleton(
    skeleton: dict[str, Any],
    stock_rig_path: Path | None = None,
    package_name: str = "",
    stock_hand_detail: bool = False,
    stock_bind_scope: str = "all",
    preserve_guitar_attach_local: bool = False,
    compensate_guitar_helper_main_anchor: bool = False,
    main_guitar_pos_offset: list[float] | None = None,
) -> dict[str, dict[str, list[float]]]:
    skeleton_scale = skeleton_bind_scale(skeleton)
    name_by_checksum = checksum_name_map(skeleton["bones"])
    source_to_target: dict[str, str] = {}
    source_worlds: dict[str, list[float]] = {}
    source_names_by_index: dict[int, str] = {}
    for bone in skeleton["bones"]:
        source_name = resolved_bone_name(bone, name_by_checksum)
        source_names_by_index[int(bone["index"])] = source_name
        source_to_target[source_name] = target_bone_name(source_name) or source_name
        source_worlds[source_name] = source_world_bind_matrix12(
            bone,
            skeleton_scale,
        )
    parent_by_source = {
        resolved_bone_name(bone, name_by_checksum): source_names_by_index.get(
            int(parent_index)
        )
        for bone in skeleton["bones"]
        if (parent_index := bone.get("parent_index")) is not None
    }
    source_locals: dict[str, list[float]] = {}
    for source_name, world in source_worlds.items():
        parent_source = source_parent_name(source_name) or parent_by_source.get(source_name)
        parent_world = source_worlds.get(parent_source or "")
        source_locals[source_name] = (
            multiply_matrix12_row_vector(
                world,
                invert_matrix12_row_vector(parent_world),
            )
            if parent_world is not None
            else list(world)
        )

    transforms: dict[str, dict[str, Any]] = {}
    for source_name, local in source_locals.items():
        parent_source = source_parent_name(source_name) or parent_by_source.get(source_name)
        parent_name = source_to_target.get(parent_source or "", "")
        transforms[source_to_target[source_name]] = {
            "source_name": source_name,
            "parent_name": parent_name,
            "local": local,
            "world": source_worlds[source_name],
        }
    retarget_common_bones_to_gh2_parent_graph(transforms)
    apply_gh3_helper_parent_overrides(transforms)
    retarget_guitar_attach_root(transforms)
    add_gh2_attach_aliases(transforms)
    retarget_guitar_hand_proxies(transforms)
    stock_mesh_bind = load_stock_mesh_bind_transforms(stock_rig_path, package_name)
    retarget_common_bones_to_stock_bind(
        transforms,
        stock_mesh_bind,
        stock_hand_detail,
        stock_bind_scope,
        preserve_guitar_attach_local,
    )
    add_gh2_mesh_skeleton_aliases(
        transforms,
        stock_mesh_bind,
        stock_hand_detail,
        stock_bind_scope,
        preserve_guitar_attach_local,
    )
    if compensate_guitar_helper_main_anchor:
        compensate_guitar_helper_for_main_anchor(
            transforms,
            main_guitar_pos_offset or [0.0, 0.0, 0.0],
        )
    return transforms


def load_bone_transforms(
    skeleton_path: Path,
    stock_rig_path: Path | None = None,
    package_name: str = "",
    stock_hand_detail: bool = False,
    stock_bind_scope: str = "all",
    preserve_guitar_attach_local: bool = False,
    compensate_guitar_helper_main_anchor: bool = False,
    main_guitar_pos_offset: list[float] | None = None,
) -> dict[str, dict[str, list[float]]]:
    data = json.loads(skeleton_path.read_text(encoding="utf-8"))
    skeleton = data.get("skeleton", data)
    return bone_transforms_from_skeleton(
        skeleton,
        stock_rig_path,
        package_name,
        stock_hand_detail,
        stock_bind_scope,
        preserve_guitar_attach_local,
        compensate_guitar_helper_main_anchor,
        main_guitar_pos_offset,
    )


def clone_mesh_bind_transforms(transforms: dict[str, dict[str, Any]]) -> None:
    for name, transform in list(transforms.items()):
        if name.endswith(MESH_BIND_SUFFIX):
            continue
        bind_name = name + MESH_BIND_SUFFIX
        if bind_name in transforms:
            continue
        transforms[bind_name] = {
            "source_name": transform.get("source_name", name),
            "parent_name": "",
            "local": list(transform["local"]),
            "world": list(transform["world"]),
            "retarget_reason": (
                "Original mesh bind cloned before default-pose baking so "
                "Mesh28 offsets remain tied to the authored bind pose."
            ),
        }


def load_ska_clip_by_path(source_root: Path, relative_path: str) -> dict[str, Any] | None:
    manifest = json.loads((source_root / "midori_source_ir_manifest.json").read_text(encoding="utf-8"))
    clip_path = source_root / manifest["animations"]["ska_ir"]["relative_path"]
    with gzip.open(clip_path, "rt", encoding="utf-8") as handle:
        for line in handle:
            clip = json.loads(line)
            if clip.get("path") == relative_path:
                return clip
    return None


def apply_default_pose_clip(
    transforms: dict[str, dict[str, Any]],
    skeleton: dict[str, Any],
    source_root: Path,
    relative_path: str = DEFAULT_POSE_CLIP,
) -> int:
    clip = load_ska_clip_by_path(source_root, relative_path)
    if clip is None:
        raise FileNotFoundError(f"default pose clip not found in SKA IR: {relative_path}")

    name_by_checksum = checksum_name_map(skeleton["bones"])
    edit_pose_by_source = source_edit_pose_by_name(skeleton, name_by_checksum)
    clone_mesh_bind_transforms(transforms)

    posed = 0
    for bone in clip["bones"]:
        quat_keys = bone.get("quat_keys") or []
        if not quat_keys:
            continue
        source_name = resolved_bone_name(bone, name_by_checksum)
        target = target_bone_name(source_name)
        if target not in gh2_mesh_alias_parents():
            continue
        pose_target = target + ".mesh" if target + ".mesh" in transforms else target
        transform = transforms.get(pose_target or "")
        if transform is None:
            continue
        bind_rotation = matrix12_rotation(transform["local"])
        delta_rotation = source_quat_to_default_pose_delta(
            [float(value) for value in quat_keys[0]["quat_wxyz"]],
            edit_pose_by_source.get(source_name),
        )
        set_matrix12_rotation(transform["local"], mat3_mul(bind_rotation, delta_rotation))
        transform["retarget_reason"] = (
            "Visible character Trans local rotation baked from GH3 default "
            "pose clip; mesh bind offset remains on the cloned bind transform."
        )
        posed += 1

    roots = [
        name for name, transform in transforms.items()
        if not name.endswith(MESH_BIND_SUFFIX) and not transform.get("parent_name")
    ]
    for root in roots:
        refresh_child_worlds(transforms, root)
    return posed


def png_chunks(data: bytes) -> list[tuple[bytes, bytes]]:
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("texture is not a PNG")
    chunks: list[tuple[bytes, bytes]] = []
    offset = 8
    while offset + 12 <= len(data):
        size = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        payload_start = offset + 8
        payload_end = payload_start + size
        if payload_end + 4 > len(data):
            raise ValueError("truncated PNG chunk")
        chunks.append((kind, data[payload_start:payload_end]))
        offset = payload_end + 4
        if kind == b"IEND":
            return chunks
    raise ValueError("PNG has no IEND chunk")


def unfilter_scanline(filter_type: int, row: bytearray, prev: bytes, bpp: int) -> None:
    for index, value in enumerate(row):
        left = row[index - bpp] if index >= bpp else 0
        up = prev[index] if prev else 0
        up_left = prev[index - bpp] if prev and index >= bpp else 0
        if filter_type == 0:
            continue
        if filter_type == 1:
            row[index] = (value + left) & 0xFF
        elif filter_type == 2:
            row[index] = (value + up) & 0xFF
        elif filter_type == 3:
            row[index] = (value + ((left + up) // 2)) & 0xFF
        elif filter_type == 4:
            pa = abs(up - up_left)
            pb = abs(left - up_left)
            pc = abs(left + up - 2 * up_left)
            predictor = left if pa <= pb and pa <= pc else up if pb <= pc else up_left
            row[index] = (value + predictor) & 0xFF
        else:
            raise ValueError(f"unsupported PNG filter {filter_type}")


def load_png_rgba(path: Path) -> tuple[int, int, bytes]:
    chunks = png_chunks(path.read_bytes())
    ihdr = next((payload for kind, payload in chunks if kind == b"IHDR"), None)
    if ihdr is None or len(ihdr) != 13:
        raise ValueError(f"{path} has no valid IHDR")
    width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack(">IIBBBBB", ihdr)
    if bit_depth != 8 or compression != 0 or filter_method != 0 or interlace != 0:
        raise ValueError(f"{path} uses unsupported PNG layout")
    channels_by_color = {0: 1, 2: 3, 4: 2, 6: 4}
    if color_type not in channels_by_color:
        raise ValueError(f"{path} uses unsupported PNG color type {color_type}")
    channels = channels_by_color[color_type]
    row_size = width * channels
    compressed = b"".join(payload for kind, payload in chunks if kind == b"IDAT")
    raw = zlib.decompress(compressed)
    expected = (row_size + 1) * height
    if len(raw) != expected:
        raise ValueError(f"{path} decompressed to {len(raw)} bytes, expected {expected}")
    rows: list[bytes] = []
    offset = 0
    prev = bytes(row_size)
    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset:offset + row_size])
        offset += row_size
        unfilter_scanline(filter_type, row, prev, channels)
        rows.append(bytes(row))
        prev = rows[-1]
    rgba = bytearray()
    for row in rows:
        for pixel in range(width):
            source = row[pixel * channels:(pixel + 1) * channels]
            if color_type == 0:
                rgba.extend((source[0], source[0], source[0], 255))
            elif color_type == 2:
                rgba.extend((source[0], source[1], source[2], 255))
            elif color_type == 4:
                rgba.extend((source[0], source[0], source[0], source[1]))
            elif color_type == 6:
                rgba.extend(source)
    return width, height, bytes(rgba)


def texture_name(outfit: str, checksum: str) -> str:
    return f"{outfit}_{checksum.replace('0x', '')}.tex"


def load_textures(
    source_ir_root: Path, included_outfits: set[str] | None = None
) -> dict[str, dict[str, Any]]:
    manifest = json.loads((source_ir_root / "midori_source_ir_manifest.json").read_text(encoding="utf-8"))
    textures: dict[str, dict[str, Any]] = {}
    for outfit in manifest["outfits"]:
        outfit_name = outfit["name"]
        if included_outfits is not None and outfit_name not in included_outfits:
            continue
        for texture in outfit["textures"]:
            name = texture_name(outfit_name, texture["checksum"])
            path = source_ir_root / texture["relative_path"]
            width, height, rgba = load_png_rgba(path)
            if width != int(texture["width"]) or height != int(texture["height"]):
                raise ValueError(f"{path} size does not match source manifest")
            alpha_values = [rgba[index + 3] for index in range(0, len(rgba), 4)]
            has_cutout_alpha = any(alpha == 0 for alpha in alpha_values)
            hmx_rgba = bytearray()
            for index in range(0, len(rgba), 4):
                hmx_rgba.extend(rgba[index:index + 3])
                alpha = rgba[index + 3]
                if has_cutout_alpha:
                    # GH2 PS2 RndMat v27 stores alpha_cut but not alpha_threshold.
                    # Bake authored cutout textures to a hard PS2 0/128 mask so
                    # source-alpha cards do not render their anti-aliased fringes.
                    alpha = 255 if alpha >= 192 else 0
                hmx_rgba.append(min(128, (alpha + 1) // 2))
            textures[name] = {
                "width": width,
                "height": height,
                "bits_per_pixel": 32,
                "header_kind": 1,
                "encoding": 3,
                "mipmap_count": 0,
                "bytes_per_line": width * 4,
                "wii_alpha": 0,
                "data": bytes(hmx_rgba),
                "has_alpha": any(alpha < 255 for alpha in alpha_values),
                "has_cutout_alpha": has_cutout_alpha,
                "source_png": str(path),
                "checksum": texture["checksum"],
            }
    return textures


def chunk_material_flags(chunk: dict[str, Any], texture: dict[str, Any]) -> dict[str, Any]:
    is_alpha_card = (
        bool(texture.get("has_cutout_alpha"))
        and int(chunk.get("source_mesh_index", 0)) > 0
    )
    return {
        "alpha_cut": is_alpha_card,
        "alpha_write": False,
        "z_mode": 1,
        "cull": not is_alpha_card,
    }


def write_bundle(
    outfit: str,
    package_name: str,
    chunks: list[dict[str, Any]],
    bone_transforms: dict[str, dict[str, list[float]]],
    textures: dict[str, dict[str, Any]],
) -> bytes:
    out = bytearray(MAGIC)
    write_u32(out, VERSION)
    write_string(out, outfit)
    write_string(out, package_name)
    used_bones = sorted(
        bone for bone in bone_transforms
        if not omit_base_hand_overlay_transform(bone)
    )
    write_u32(out, len(used_bones))
    for bone in used_bones:
        transform = bone_transforms.get(bone)
        if transform is None:
            raise KeyError(f"missing skeleton transform for {bone}")
        write_string(out, bone)
        write_string(out, transform["source_name"])
        parent_name = transform["parent_name"]
        if (
            parent_name in gh2_mesh_alias_parents()
            and parent_name + ".mesh" in bone_transforms
        ):
            parent_name += ".mesh"
        if parent_name == PACKAGE_ROOT_PARENT:
            parent_name = package_name
        write_string(out, parent_name)
        for value in transform["local"]:
            write_f32(out, value)
        for value in transform["world"]:
            write_f32(out, value)
    used_textures = sorted({chunk["texture"] for chunk in chunks})
    write_u32(out, len(used_textures))
    for name in used_textures:
        texture = textures.get(name)
        if texture is None:
            raise KeyError(f"missing texture payload for {name}")
        write_string(out, name)
        write_u32(out, texture["width"])
        write_u32(out, texture["height"])
        write_u32(out, texture["bits_per_pixel"])
        write_u8(out, texture["header_kind"])
        write_u8(out, texture["bits_per_pixel"])
        write_u32(out, texture["encoding"])
        write_u8(out, texture["mipmap_count"])
        write_u16(out, texture["width"])
        write_u16(out, texture["height"])
        write_u16(out, texture["bytes_per_line"])
        write_u16(out, texture["wii_alpha"])
        write_bytes(out, texture["data"])
    write_u32(out, len(chunks))
    for chunk in chunks:
        texture = textures.get(chunk["texture"])
        if texture is None:
            raise KeyError(f"missing texture payload for {chunk['texture']}")
        material_flags = chunk_material_flags(chunk, texture)
        write_string(out, chunk["name"])
        write_string(out, chunk["material"])
        write_string(out, chunk["texture"])
        write_u8(out, 1 if material_flags["alpha_cut"] else 0)
        write_u8(out, 1 if material_flags["alpha_write"] else 0)
        write_u32(out, int(material_flags["z_mode"]))
        write_u8(out, 1 if material_flags["cull"] else 0)
        for value in chunk["sphere"]:
            write_f32(out, value)
        write_u32(out, len(chunk["bone_slots"]))
        for bone in chunk["bone_slots"]:
            write_string(out, bone)
            bind_name = bone + MESH_BIND_SUFFIX
            write_string(out, bind_name if bind_name in bone_transforms else bone)
        write_u32(out, len(chunk["vertices"]))
        for vertex in chunk["vertices"]:
            for value in vertex["position"]:
                write_f32(out, value)
            for value in vertex["normal"]:
                write_f32(out, value)
            for value in vertex["color_or_weights"]:
                write_f32(out, value)
            for value in vertex["uv"]:
                write_f32(out, value)
        write_u32(out, len(chunk["faces"]))
        for face in chunk["faces"]:
            for index in face:
                write_u16(out, index)
    return bytes(out)


def command_bundle(args: argparse.Namespace) -> int:
    global CONTROL_ROOT_PELVIS_PARENT
    CONTROL_ROOT_PELVIS_PARENT = bool(args.control_root_pelvis_parent)
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((args.input / "gh3_midori_model_stage_manifest.json").read_text(encoding="utf-8"))
    skeleton = Path(manifest["skeleton"])
    skeleton_data = json.loads(skeleton.read_text(encoding="utf-8"))
    skeleton_payload = skeleton_data.get("skeleton", skeleton_data)
    source_ir_root = Path(manifest["source_dir"]).parent
    included_outfits = {str(outfit["outfit"]) for outfit in manifest["outfits"]}
    textures = load_textures(source_ir_root, included_outfits)
    bundles = []
    for outfit in manifest["outfits"]:
        source = args.input / outfit["relative_path"]
        package_name = "gh3_" + outfit["outfit"]
        source_bone_transforms = load_bone_transforms(
            skeleton,
            None,
            package_name,
            False,
            args.stock_bind_scope,
            args.preserve_guitar_attach_local,
            False,
            args.main_guitar_pos_offset,
        )
        bone_transforms = load_bone_transforms(
            skeleton,
            args.gh2_stock_rig,
            package_name,
            args.stock_hand_detail_rig,
            args.stock_bind_scope,
            args.preserve_guitar_attach_local,
            args.compensate_guitar_helper_for_main_anchor,
            args.main_guitar_pos_offset,
        )
        chunks = load_chunks(source)
        bind_pose_vertex_warp = warp_chunks_to_target_bind(
            chunks,
            source_bone_transforms,
            bone_transforms,
            args.bind_pose_vertex_warp_scope,
            args.bind_pose_forearm_warp_weight,
            args.bind_pose_hand_warp_weight,
            args.bind_pose_wrist_ramp_weight,
            args.bind_pose_wrist_ramp_start,
        )
        default_pose_channel_count = 0
        if args.default_pose_clip:
            default_pose_channel_count = apply_default_pose_clip(
                bone_transforms,
                skeleton_payload,
                source_ir_root,
                args.default_pose_clip,
            )
        target = args.output / (package_name + ".meshbundle")
        excluded_chunks = []
        if args.exclude_chunk_name:
            exclude_names = set(args.exclude_chunk_name)
            kept_chunks = []
            for chunk in chunks:
                if chunk["name"] in exclude_names:
                    excluded_chunks.append(chunk["name"])
                else:
                    kept_chunks.append(chunk)
            chunks = kept_chunks
        data = write_bundle(outfit["outfit"], package_name, chunks, bone_transforms, textures)
        used_texture_names = sorted({chunk["texture"] for chunk in chunks})
        alpha_materials = sorted({
            chunk["material"]
            for chunk in chunks
            if chunk_material_flags(chunk, textures[chunk["texture"]])["alpha_cut"]
        })
        target.write_bytes(data)
        bundles.append({
            "outfit": outfit["outfit"],
            "package_name": package_name,
            "relative_path": target.relative_to(args.output).as_posix(),
            "chunk_count": len(chunks),
            "excluded_chunk_names": sorted(excluded_chunks),
            "bone_transform_count": len(bone_transforms),
            "default_pose_clip": args.default_pose_clip,
            "default_pose_channel_count": default_pose_channel_count,
            "bind_pose_vertex_warp": bind_pose_vertex_warp,
            "embedded_texture_count": len(used_texture_names),
            "embedded_texture_names": used_texture_names,
            "alpha_cut_material_count": len(alpha_materials),
            "alpha_cut_material_names": alpha_materials,
            "byte_count": len(data),
        })
    bundle_manifest = {
        "format": f"gh3_midori_model_bundle_v{VERSION}",
        "source": str(args.input),
        "bundle_count": len(bundles),
        "total_byte_count": sum(item["byte_count"] for item in bundles),
        "total_embedded_texture_count": sum(item["embedded_texture_count"] for item in bundles),
        "total_alpha_cut_material_count": sum(item["alpha_cut_material_count"] for item in bundles),
        "skeleton": str(skeleton),
        "gh2_stock_rig": str(args.gh2_stock_rig) if args.gh2_stock_rig else None,
        "stock_hand_detail_rig": args.stock_hand_detail_rig,
        "stock_bind_scope": args.stock_bind_scope,
        "control_root_pelvis_parent": args.control_root_pelvis_parent,
        "preserve_guitar_attach_local": args.preserve_guitar_attach_local,
        "compensate_guitar_helper_for_main_anchor": args.compensate_guitar_helper_for_main_anchor,
        "main_guitar_pos_offset": args.main_guitar_pos_offset,
        "default_pose_clip": args.default_pose_clip,
        "bind_pose_vertex_warp_scope": args.bind_pose_vertex_warp_scope,
        "bind_pose_forearm_warp_weight": args.bind_pose_forearm_warp_weight,
        "bind_pose_hand_warp_weight": args.bind_pose_hand_warp_weight,
        "bind_pose_wrist_ramp_weight": args.bind_pose_wrist_ramp_weight,
        "bind_pose_wrist_ramp_start": args.bind_pose_wrist_ramp_start,
        "texture_source": str(source_ir_root),
        "bundles": bundles,
        "next_step": "Feed each .meshbundle to milo_convert_tool build-character-from-meshbundle.",
    }
    manifest_path = args.output / "gh3_midori_model_bundle_manifest.json"
    manifest_path.write_text(json.dumps(bundle_manifest, indent=2) + "\n", encoding="utf-8")
    if args.print_summary:
        split = ",".join(f"{item['package_name']}:{item['chunk_count']}" for item in bundles)
        print(
            "bundles=%d chunks=%d textures=%d bytes=%d split=%s"
            % (
                len(bundles),
                sum(item["chunk_count"] for item in bundles),
                bundle_manifest["total_embedded_texture_count"],
                bundle_manifest["total_byte_count"],
                split,
            )
        )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("out/midori/model_stage"))
    parser.add_argument("--output", type=Path, default=Path("out/midori/model_bundles"))
    parser.add_argument("--gh2-stock-rig", type=Path, default=None)
    parser.add_argument(
        "--stock-hand-detail-rig",
        action="store_true",
        help="Use stock GH2 hand and finger bind aliases as well as common body aliases.",
    )
    parser.add_argument(
        "--stock-bind-scope",
        choices=STOCK_BIND_SCOPES,
        default="all",
        help="Limit which common GH2 bind aliases are borrowed from the stock rig.",
    )
    parser.add_argument(
        "--control-root-pelvis-parent",
        action="store_true",
        help="Diagnostic: parent bone_pelvis aliases under Control_Root instead of flattening them to the package root.",
    )
    parser.add_argument(
        "--preserve-guitar-attach-local",
        action="store_true",
        help="When using scoped stock binds, keep Midori's generated bone_pos_guitar(.mesh) local attach instead of transplanting the stock guitar world.",
    )
    parser.add_argument(
        "--compensate-guitar-helper-for-main-anchor",
        action="store_true",
        help="Compensate the GH3 guitar-body helper local for the forced GH2 main bone_pos_guitar.mesh animation anchor.",
    )
    parser.add_argument(
        "--main-guitar-pos-offset",
        type=lambda value: [float(part) for part in value.split(",")],
        default=[0.0, 0.0, 0.0],
        help="XYZ offset used by the forced GH2 guitar main anchor; must match ACP staging when helper compensation is enabled.",
    )
    parser.add_argument(
        "--default-pose-clip",
        default=None,
        help="Bake a one-frame GH3 default pose into visible Trans locals while preserving cloned Mesh28 bind offsets.",
    )
    parser.add_argument(
        "--bind-pose-vertex-warp-scope",
        choices=BIND_POSE_VERTEX_WARP_SCOPES,
        default="none",
        help=(
            "Warp staged vertices from the authored Midori bind into the selected "
            "GH2 stock bind through their existing skin weights before writing Mesh28."
        ),
    )
    parser.add_argument(
        "--bind-pose-forearm-warp-weight",
        type=float,
        default=1.0,
        help=(
            "Forearm correction fraction for the forearms-hands bind warp; "
            "hands and fingers remain fully corrected."
        ),
    )
    parser.add_argument(
        "--bind-pose-hand-warp-weight",
        type=float,
        default=1.0,
        help="Hand/finger correction fraction for any bind-pose vertex warp.",
    )
    parser.add_argument(
        "--bind-pose-wrist-ramp-weight",
        type=float,
        default=0.0,
        help=(
            "Blend the hand bind correction onto distal forearm weights, "
            "leaving the elbow end anchored."
        ),
    )
    parser.add_argument(
        "--bind-pose-wrist-ramp-start",
        type=float,
        default=0.6,
        help="Forearm-axis fraction where the smooth wrist correction begins.",
    )
    parser.add_argument(
        "--exclude-chunk-name",
        action="append",
        default=[],
        help="Diagnostic: omit an exact staged mesh chunk name from emitted meshbundles.",
    )
    parser.add_argument("--print-summary", action="store_true")
    parser.set_defaults(func=command_bundle)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
