#!/usr/bin/env python3
"""Build full main-clip donors with coupled guitar and contact target channels."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "tools"))

import gh3_midori_acp_stage as stage  # noqa: E402
import gh3_midori_final_hand_target_local_solve_report as final_solve  # noqa: E402
import gh3_midori_guitar_frame_hand_bake_probe as probe  # noqa: E402
import gh3_midori_guitar_ik_contract_report as contract  # noqa: E402
import gh3_midori_model_bundle as model_bundle  # noqa: E402
import gh3_midori_source_guitar_contract_report as source_contract  # noqa: E402


DEFAULT_TOOL = (
    ROOT
    / "GuitarHeroOGX-main-ui-engine"
    / "tools"
    / "milo_convert"
    / "out"
    / "build"
    / "win-amd64-release"
    / "Release"
    / "milo_convert_tool.exe"
)
DEFAULT_CANDIDATE = ROOT / "gh2_ps2_hybrid_assets" / "DLC" / "community.gh3.midori"
DEFAULT_SOLVE = ROOT / "analysis" / "gh3_midori_r133_staticface_mainonly_coupled_guitar_solve_report.json"
DEFAULT_OUT = ROOT / "analysis" / "gh3_midori_gh2_milos" / "gh3_midori_main_staticface_fullclip_contact_r135.milo_ps2"
DEFAULT_DONOR = ROOT / "analysis" / "_midori_r135_staticface_fullclip_contact_donor.milo_ps2"
DEFAULT_ACP_DIR = ROOT / "analysis" / "_midori_r135_staticface_fullclip_contact_acp"
DEFAULT_REPORT = ROOT / "analysis" / "gh3_midori_r135_staticface_fullclip_contact_candidate_report.json"
DEFAULT_REVISION = "r135-staticface-fullclip-coupled-contact"
DEFAULT_VISIBLE_HAND_AXIS_CALIBRATION = ROOT / "analysis" / "gh3_midori_visible_hand_axis_calibration_r68.json"
DEFAULT_PER_CASE_SOURCE_BRIDGE_ROOT = (
    ROOT
    / ".codex"
    / "current-evidence"
    / "midori-review-source-bridges-fresh-targetlength-20260818-5case"
)

PATCH_CHANNELS = (
    "bone_fret_hand.mesh.pos",
    "bone_pos_guitar.mesh.pos",
    "bone_strum_hand.mesh.pos",
    "bone_pos_guitar.mesh.quat",
)
VISIBLE_ARM_PATCH_CHANNELS = (
    "bone_L-upperArm.mesh.quat",
    "bone_L-foreArm.mesh.pos",
    "bone_L-foreArm.mesh.quat",
    "bone_L-hand.mesh.pos",
    "bone_R-upperArm.mesh.quat",
    "bone_R-foreArm.mesh.pos",
    "bone_R-foreArm.mesh.quat",
    "bone_R-hand.mesh.pos",
)
VISIBLE_HAND_ROTATION_PATCH_CHANNELS = (
    "bone_L-hand.mesh.quat",
    "bone_R-hand.mesh.quat",
)
VISIBLE_CLAVICLE_PATCH_CHANNELS = (
    "bone_L-clavicle.mesh.quat",
    "bone_R-clavicle.mesh.quat",
)
SAMPLE_RE = re.compile(r"^full\s+sample=(\d+)\s+(\S+)\s+(.+)$")


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT.resolve()).as_posix()
    except ValueError:
        return str(path)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def child_creationflags() -> int:
    return getattr(subprocess, "IDLE_PRIORITY_CLASS", 0)


def set_low_priority() -> None:
    try:
        ctypes.windll.kernel32.SetPriorityClass(
            ctypes.windll.kernel32.GetCurrentProcess(),
            child_creationflags(),
        )
    except Exception:
        pass


def run_tool(tool: Path, args: list[str]) -> str:
    result = subprocess.run(
        [str(tool), *args],
        cwd=ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        creationflags=child_creationflags(),
    )
    if result.returncode != 0:
        stderr = result.stderr.strip().splitlines()[-5:]
        raise RuntimeError(
            f"milo_convert_tool failed ({result.returncode}) for {' '.join(args)}: "
            + " | ".join(stderr)
        )
    return result.stdout


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8-sig") as handle:
        payload = json.load(handle)
    if not isinstance(payload, dict):
        raise ValueError(f"{path}: expected JSON object")
    return payload


def channel_size(channel: str) -> int:
    if channel.endswith(".pos"):
        return 12
    if channel.endswith(".quat"):
        return 16
    raise ValueError(f"unsupported sampled channel: {channel}")


def parse_clip_inventory(tool: Path, main_milo: Path) -> dict[str, dict[str, Any]]:
    output = run_tool(tool, ["inspect-clipset", str(main_milo), "--channels"])
    clips: dict[str, dict[str, Any]] = {}
    current: dict[str, Any] | None = None
    for line in output.splitlines():
        if line.startswith("clip\t"):
            parts = line.split("\t")
            fields = {}
            for item in parts[4:]:
                if "=" in item:
                    key, value = item.split("=", 1)
                    fields[key] = value
            current = {
                "name": parts[1],
                "clip_flags": int(fields.get("body_flags") or parts[2] or 0),
                "body_bytes": int(fields.get("body_bytes") or 0),
                "sample_bytes": int(fields.get("sample_bytes") or 0),
                "channels": [],
            }
            clips[current["name"]] = current
        elif line.startswith("channel\t") and current is not None:
            parts = line.split("\t")
            if len(parts) >= 4 and parts[2] == "full":
                current["channels"].append(parts[3])
    for row in clips.values():
        per_sample = sum(channel_size(channel) for channel in row["channels"])
        if per_sample <= 0:
            row["sample_count"] = 1
        else:
            sample_bytes = int(row["sample_bytes"])
            if sample_bytes % per_sample != 0:
                raise ValueError(f"{row['name']}: sample bytes do not divide channel stride")
            row["sample_count"] = sample_bytes // per_sample
    return clips


def parse_sample_output(text: str) -> dict[str, list[float]]:
    values: dict[str, list[float]] = {}
    for line in text.splitlines():
        match = SAMPLE_RE.match(line.strip())
        if not match:
            continue
        _sample, channel, raw = match.groups()
        values[channel] = [float(part) for part in raw.split(",") if part.strip()]
    return values


def sample_clip_values(tool: Path, main_milo: Path, clip: str, sample_count: int) -> list[dict[str, list[float]]]:
    rows = []
    for frame in range(sample_count):
        rows.append(parse_sample_output(run_tool(tool, ["sample-clip", str(main_milo), clip, str(frame)])))
    return rows


def output_quat_xyzw(matrix: list[list[float]], hmx_quat_mode: str) -> list[float]:
    args = argparse.Namespace(hmx_quat_mode=hmx_quat_mode, clip_quat_storage_order="xyzw")
    return probe.output_clip_quat(matrix, args)


def per_case_source_bridge_path(root: Path, solve_row: dict[str, Any]) -> Path:
    case_name = str(solve_row["name"])
    candidates = [case_name]
    if case_name.startswith("midori_2_"):
        candidates.append("midori_1_" + case_name[len("midori_2_"):])
    for candidate in candidates:
        case_dir = root / candidate
        if not case_dir.is_dir():
            continue
        matches = sorted(case_dir.glob("*.pose_bridge.json"))
        if matches:
            return matches[0]
    raise ValueError(f"{solve_row['name']}: no per-case source bridge under {root}")


def bridge_record_rotation(record: dict[str, Any], field: str) -> list[list[float]]:
    matrix = record[field]
    return [
        [float(matrix[0]), float(matrix[1]), float(matrix[2])],
        [float(matrix[4]), float(matrix[5]), float(matrix[6])],
        [float(matrix[8]), float(matrix[9]), float(matrix[10])],
    ]


def source_bridge_pose_local_rotation_to_gh2(
    path: Path,
    frame: int,
    bone: str,
    basis: str,
) -> list[list[float]]:
    data = load_json(path)
    records = {
        (int(record.get("frame", -1)), str(record.get("bone"))): record
        for record in data.get("records", [])
        if isinstance(record, dict)
    }
    record = records.get((int(frame), bone))
    if record is None:
        raise ValueError(f"missing source bridge pose rotation: frame={frame} bone={bone}")
    parent_name = str(record.get("pose_parent") or "")
    if not parent_name:
        local = bridge_record_rotation(record, "pose")
    else:
        parent = records.get((int(frame), parent_name))
        if parent is None:
            raise ValueError(
                f"missing source bridge pose parent: frame={frame} bone={bone} parent={parent_name}"
            )
        local = stage.mat3_mul(
            bridge_record_rotation(record, "pose"),
            stage.mat3_transpose(bridge_record_rotation(parent, "pose")),
        )
    basis_matrix = probe.source_basis_matrix(basis)
    return stage.mat3_mul(
        stage.mat3_mul(basis_matrix, local),
        stage.mat3_transpose(basis_matrix),
    )


def source_hand_pair_bones(mode: str) -> tuple[str, str] | None:
    return {
        "source-palm-locals-per-case": ("Bone_Palm_L", "Bone_Palm_R"),
        "source-ik-helper-locals-per-case": (
            "bone_ik_hand_guitar_l",
            "bone_ik_hand_guitar_r",
        ),
    }.get(mode)


def source_coordinate_map_matrix(name: str) -> list[list[float]]:
    if name == "palm-fit-negx-y-negz":
        return [
            [-1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, 0.0, -1.0],
        ]
    if name == "direct":
        return [
            [1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, 0.0, 1.0],
        ]
    raise ValueError(f"unsupported source coordinate map: {name}")


def mapped_source_guitar_local_translation(
    records: dict[int, dict[str, list[list[float]]]],
    frame: int,
    source_bone: str,
    coordinate_map: str,
) -> list[float]:
    bones = records.get(frame)
    if bones is None:
        raise ValueError(f"missing source bridge frame {frame}")
    guitar = bones.get("bone_guitar_body")
    source = bones.get(source_bone)
    if guitar is None or source is None:
        raise ValueError(f"missing source bridge bone at frame {frame}: {source_bone}")
    local_col = source_contract.matmul(source_contract.inv_rigid_col(guitar), source)
    source_offset = source_contract.translation_col(local_col)
    bridge = source_coordinate_map_matrix(coordinate_map)
    return [
        sum(float(bridge[row][col]) * float(source_offset[col]) for col in range(3))
        for row in range(3)
    ]


def stock_target_local_translation(target: str) -> list[float]:
    matrix12 = model_bundle.GH2_STOCK_GUITAR_HAND_TARGET_LOCALS[target]
    return [float(matrix12[9]), float(matrix12[10]), float(matrix12[11])]


def row_vec_mat_mul(vec: list[float], matrix: list[list[float]]) -> list[float]:
    return [
        sum(float(vec[k]) * float(matrix[k][c]) for k in range(3))
        for c in range(3)
    ]


def fit_source_positions_to_stock_hand_targets(
    source_left: list[float],
    source_right: list[float],
    positions: dict[str, list[float]],
) -> tuple[dict[str, list[float]], dict[str, Any]]:
    target_left = stock_target_local_translation("bone_fret_hand")
    target_right = stock_target_local_translation("bone_strum_hand")
    source_delta = probe.vec_sub(source_left, source_right)
    target_delta = probe.vec_sub(target_left, target_right)
    source_length = probe.vec_length(source_delta)
    target_length = probe.vec_length(target_delta)
    if source_length < 1.0e-6:
        raise ValueError("source helper hands overlap")
    source_basis = orthonormal_basis(source_delta, [0.0, 1.0, 0.0])
    target_basis = orthonormal_basis(target_delta, [0.0, 1.0, 0.0])
    fit_rotation = stage.mat3_mul(stage.mat3_transpose(source_basis), target_basis)
    fit_scale = target_length / source_length
    fitted = {}
    for target, source_position in positions.items():
        source_relative = probe.vec_sub(source_position, source_right)
        fitted[target] = probe.vec_add(
            target_right,
            row_vec_mat_mul(probe.vec_scale(source_relative, fit_scale), fit_rotation),
        )
    return fitted, {
        "mode": "fit-stock-hand-targets",
        "source_left_ik": source_left,
        "source_right_ik": source_right,
        "target_left": target_left,
        "target_right": target_right,
        "scale": fit_scale,
        "rotation": fit_rotation,
    }


def mat3_vec_mul(matrix: list[list[float]], vector: list[float]) -> list[float]:
    return [
        sum(float(matrix[row][col]) * float(vector[col]) for col in range(3))
        for row in range(3)
    ]


def row_vec_mat_mul(vector: list[float], matrix: list[list[float]]) -> list[float]:
    return [
        sum(float(vector[row]) * float(matrix[row][col]) for row in range(3))
        for col in range(3)
    ]


def orthonormal_basis(primary: list[float], secondary: list[float]) -> list[list[float]]:
    p = probe.vec_normalize(primary)
    secondary_projected = probe.vec_sub(
        secondary,
        probe.vec_scale(p, probe.vec_dot(secondary, p)),
    )
    if probe.vec_length(secondary_projected) < 1.0e-6:
        for fallback in ([0.0, 0.0, 1.0], [0.0, 1.0, 0.0], [1.0, 0.0, 0.0]):
            secondary_projected = probe.vec_sub(
                fallback,
                probe.vec_scale(p, probe.vec_dot(fallback, p)),
            )
            if probe.vec_length(secondary_projected) >= 1.0e-6:
                break
    s = probe.vec_normalize(secondary_projected)
    return [p, s, probe.vec_normalize(probe.vec_cross(p, s))]


def solve_world_rotation_from_visible_axes(
    local_finger: list[float],
    local_palm_normal: list[float],
    desired_finger_world: list[float],
    desired_palm_normal_world: list[float],
) -> list[list[float]]:
    local_basis = orthonormal_basis(local_finger, local_palm_normal)
    desired_basis = orthonormal_basis(desired_finger_world, desired_palm_normal_world)
    return stage.mat3_mul(stage.mat3_transpose(local_basis), desired_basis)


def parse_optional_axis(raw: str | None) -> list[float] | None:
    if not raw:
        return None
    values = [float(part) for part in raw.split(",") if part.strip()]
    if len(values) != 3:
        raise ValueError(f"expected 3 comma-separated axis values, got {raw!r}")
    return values


def parse_optional_vector(raw: str | None, label: str) -> list[float] | None:
    if not raw:
        return None
    values = [float(part) for part in raw.split(",") if part.strip()]
    if len(values) != 3:
        raise ValueError(f"{label}: expected 3 comma-separated values, got {raw!r}")
    return values


def vector_arg(values: list[float]) -> str:
    return ",".join(f"{float(value):.12g}" for value in values)


def load_forearm_override_map(path: Path | None) -> dict[str, Any]:
    if path is None:
        return {}
    payload = load_json(path)
    cases = payload.get("cases", payload)
    if not isinstance(cases, dict):
        raise ValueError(f"{path}: expected object or object with 'cases'")
    return cases


def case_forearm_override_arg(
    override_map: dict[str, Any],
    case_name: str,
    side: str,
    fallback: str | None,
) -> str | None:
    entry = override_map.get(case_name) or override_map.get("*")
    if entry is None:
        return fallback
    if isinstance(entry, list):
        return vector_arg([float(value) for value in entry])
    if not isinstance(entry, dict):
        raise ValueError(f"{case_name}: expected forearm override object/list")
    side_keys = (
        ("left", "L", "left_guitar_local", "left_forearm_guitar_local")
        if side == "L"
        else ("right", "R", "right_guitar_local", "right_forearm_guitar_local")
    )
    for key in side_keys:
        value = entry.get(key)
        if value is not None:
            if isinstance(value, str):
                return value
            if isinstance(value, list):
                return vector_arg([float(item) for item in value])
            raise ValueError(f"{case_name}.{key}: expected string/list")
    return fallback


def apply_guitar_local_axis_bias(
    vector: list[float],
    axis: list[float] | None,
    degrees: float,
) -> list[float]:
    if axis is None or abs(degrees) <= 1.0e-6:
        return vector
    return row_vec_mat_mul(vector, probe.arbitrary_axis_angle_matrix(axis, degrees))


def visible_axis_calibrated_hand_local_rotation(
    transforms: dict[str, dict[str, Any]],
    side: str,
    calibration: dict[str, Any],
    grip_axis: list[float] | None,
    grip_degrees: float,
) -> list[list[float]]:
    side_calibration = calibration.get("sides", {}).get(side)
    if not side_calibration:
        raise ValueError(f"missing visible hand axis calibration for side {side}")
    local_axes = side_calibration["hand_local_visible_axes"]
    guitar_axes = side_calibration["guitar_local_visible_axes"]
    guitar_rotation = probe.matrix3_from_matrix12(transforms["bone_pos_guitar.mesh"]["world"])
    desired_finger_world = row_vec_mat_mul(
        apply_guitar_local_axis_bias(guitar_axes["finger"], grip_axis, grip_degrees),
        guitar_rotation,
    )
    desired_palm_world = row_vec_mat_mul(
        apply_guitar_local_axis_bias(guitar_axes["palm_normal"], grip_axis, grip_degrees),
        guitar_rotation,
    )
    desired_world = solve_world_rotation_from_visible_axes(
        local_axes["finger"],
        local_axes["palm_normal"],
        desired_finger_world,
        desired_palm_world,
    )
    hand_name = f"bone_{side}-hand.mesh"
    parent_name = transforms[hand_name].get("parent") or ""
    if parent_name not in transforms:
        raise ValueError(f"missing parent for {hand_name}")
    return probe.solve_local_rotation_for_world(
        transforms[parent_name]["world"],
        desired_world,
    )


def source_pose_local_offset_to_gh2(
    path: Path,
    frame: int,
    anchor_bone: str,
    child_bone: str,
    basis: str,
    scale: float,
) -> list[float]:
    anchor_pos = probe.source_bridge_translation(path, frame, anchor_bone)
    child_pos = probe.source_bridge_translation(path, frame, child_bone)
    anchor_rot = probe.source_bridge_rotation(path, frame, anchor_bone)
    source_delta_world = [
        float(child_pos[index]) - float(anchor_pos[index])
        for index in range(3)
    ]
    source_delta_local = mat3_vec_mul(stage.mat3_transpose(anchor_rot), source_delta_world)
    return probe.map_source_delta_to_gh2(source_delta_local, basis, scale)


def visible_arm_target_worlds(
    transforms: dict[str, dict[str, Any]],
    solve_row: dict[str, Any],
    visible_arm_target_mode: str,
    visible_arm_target_blend_with_current: float,
    visible_arm_target_swap_cases: set[str],
    visible_arm_grip_map: Path,
    visible_arm_source_coordinate_map: str,
    source_basis: str,
    source_scale: float,
    per_case_source_bridge_root: Path,
) -> tuple[list[float], list[float], dict[str, Any]]:
    current_left = contract.position(transforms, "bone_fret_hand.mesh")
    current_right = contract.position(transforms, "bone_strum_hand.mesh")
    if current_left is None or current_right is None:
        raise ValueError(f"{solve_row['name']}: missing hand target world")
    if visible_arm_target_mode == "current-proxies":
        return (
            current_left,
            current_right,
            {
                "mode": visible_arm_target_mode,
                "blend_with_current": visible_arm_target_blend_with_current,
            },
        )
    if visible_arm_target_mode in {"swapped-current-proxies", "mapped-current-proxies"}:
        should_swap = (
            visible_arm_target_mode == "swapped-current-proxies"
            or str(solve_row["name"]) in visible_arm_target_swap_cases
        )
        if not should_swap:
            return (
                current_left,
                current_right,
                {
                    "mode": visible_arm_target_mode,
                    "mapped_role": "current",
                    "swap_cases": sorted(visible_arm_target_swap_cases),
                    "blend_with_current": visible_arm_target_blend_with_current,
                },
            )
        return (
            current_right,
            current_left,
            {
                "mode": visible_arm_target_mode,
                "mapped_role": "swapped",
                "left_source": "bone_strum_hand.mesh",
                "right_source": "bone_fret_hand.mesh",
                "swap_cases": sorted(visible_arm_target_swap_cases),
                "blend_with_current": visible_arm_target_blend_with_current,
            },
        )
    if visible_arm_target_mode == "explicit-guitar-local-grip":
        grip_map = load_json(visible_arm_grip_map)
        default = grip_map.get("default") or {}
        case_row = (grip_map.get("cases") or {}).get(str(solve_row["name"]), {})
        role = str(case_row.get("role", default.get("role", "current")))
        offsets = default.get("offsets") or {}
        case_offsets = case_row.get("offsets") or {}

        def offset_for(name: str) -> list[float]:
            raw = case_offsets.get(name, offsets.get(name))
            if raw is None:
                raise ValueError(f"{visible_arm_grip_map}: missing {name} offset")
            values = [float(value) for value in raw]
            if len(values) != 3:
                raise ValueError(f"{visible_arm_grip_map}: {name} offset must have 3 values")
            return values

        fret_world = probe.local_offset_to_world(
            transforms["bone_pos_guitar.mesh"]["world"],
            offset_for("fret"),
        )
        strum_world = probe.local_offset_to_world(
            transforms["bone_pos_guitar.mesh"]["world"],
            offset_for("strum"),
        )
        if role == "current":
            left_world, right_world = fret_world, strum_world
        elif role == "swapped":
            left_world, right_world = strum_world, fret_world
        else:
            raise ValueError(f"unsupported grip role for {solve_row['name']}: {role}")
        blend = float(visible_arm_target_blend_with_current)
        if blend < 1.0:
            left_world = probe.vec_lerp(current_left, left_world, blend)
            right_world = probe.vec_lerp(current_right, right_world, blend)
        return (
            left_world,
            right_world,
            {
                "mode": visible_arm_target_mode,
                "grip_map": rel(visible_arm_grip_map),
                "role": role,
                "fret_guitar_local_offset": offset_for("fret"),
                "strum_guitar_local_offset": offset_for("strum"),
                "blend_with_current": blend,
                "current_left_world": current_left,
                "current_right_world": current_right,
            },
        )
    if visible_arm_target_mode == "source-palm-fit-stock-hand-targets-per-case":
        active_source_bridge = per_case_source_bridge_path(per_case_source_bridge_root, solve_row)
        active_source_frame = int(solve_row["frame"])
        records = source_contract.load_records(active_source_bridge)
        source_left_ik = mapped_source_guitar_local_translation(
            records,
            active_source_frame,
            "bone_ik_hand_guitar_l",
            visible_arm_source_coordinate_map,
        )
        source_right_ik = mapped_source_guitar_local_translation(
            records,
            active_source_frame,
            "bone_ik_hand_guitar_r",
            visible_arm_source_coordinate_map,
        )
        source_left_palm = mapped_source_guitar_local_translation(
            records,
            active_source_frame,
            "Bone_Palm_L",
            visible_arm_source_coordinate_map,
        )
        source_right_palm = mapped_source_guitar_local_translation(
            records,
            active_source_frame,
            "Bone_Palm_R",
            visible_arm_source_coordinate_map,
        )
        fitted, fit_info = fit_source_positions_to_stock_hand_targets(
            source_left_ik,
            source_right_ik,
            {
                "left_palm": source_left_palm,
                "right_palm": source_right_palm,
            },
        )
        left_offset = fitted["left_palm"]
        right_offset = fitted["right_palm"]
        left_world = probe.local_offset_to_world(
            transforms["bone_pos_guitar.mesh"]["world"],
            left_offset,
        )
        right_world = probe.local_offset_to_world(
            transforms["bone_pos_guitar.mesh"]["world"],
            right_offset,
        )
        blend = float(visible_arm_target_blend_with_current)
        if blend < 1.0:
            left_world = probe.vec_lerp(current_left, left_world, blend)
            right_world = probe.vec_lerp(current_right, right_world, blend)
        return (
            left_world,
            right_world,
            {
                "mode": visible_arm_target_mode,
                "blend_with_current": blend,
                "source_bridge": rel(active_source_bridge),
                "source_frame": active_source_frame,
                "source_coordinate_map": visible_arm_source_coordinate_map,
                "left_source_bone": "Bone_Palm_L",
                "right_source_bone": "Bone_Palm_R",
                "left_fitted_guitar_local_offset": left_offset,
                "right_fitted_guitar_local_offset": right_offset,
                "fit": fit_info,
                "current_left_world": current_left,
                "current_right_world": current_right,
            },
        )
    source_bones = source_hand_pair_bones(visible_arm_target_mode)
    if source_bones is None:
        raise ValueError(f"unsupported visible arm target mode: {visible_arm_target_mode}")
    active_source_bridge = per_case_source_bridge_path(per_case_source_bridge_root, solve_row)
    active_source_frame = int(solve_row["frame"])
    source_anchor_bone = "Bone_Chest"
    target_anchor_bone = "bone_spine3.mesh"
    if target_anchor_bone not in transforms:
        raise ValueError(f"{solve_row['name']}: missing target anchor {target_anchor_bone}")
    left_offset = source_pose_local_offset_to_gh2(
        active_source_bridge,
        active_source_frame,
        source_anchor_bone,
        source_bones[0],
        source_basis,
        source_scale,
    )
    right_offset = source_pose_local_offset_to_gh2(
        active_source_bridge,
        active_source_frame,
        source_anchor_bone,
        source_bones[1],
        source_basis,
        source_scale,
    )
    anchor_world = transforms[target_anchor_bone]["world"]
    left_world = probe.local_offset_to_world(anchor_world, left_offset)
    right_world = probe.local_offset_to_world(anchor_world, right_offset)
    blend = float(visible_arm_target_blend_with_current)
    if blend < 1.0:
        left_world = probe.vec_lerp(current_left, left_world, blend)
        right_world = probe.vec_lerp(current_right, right_world, blend)
    return (
        left_world,
        right_world,
        {
            "mode": visible_arm_target_mode,
            "blend_with_current": blend,
            "source_bridge": rel(active_source_bridge),
            "source_frame": active_source_frame,
            "source_basis": source_basis,
            "source_scale": source_scale,
            "source_anchor_bone": source_anchor_bone,
            "target_anchor_bone": target_anchor_bone,
            "left_source_bone": source_bones[0],
            "right_source_bone": source_bones[1],
            "left_anchor_local_offset": left_offset,
            "right_anchor_local_offset": right_offset,
            "current_left_world": current_left,
            "current_right_world": current_right,
        },
    )


def visible_arm_elbow_hint_world(
    transforms: dict[str, dict[str, Any]],
    solve_row: dict[str, Any],
    side: str,
    target_world: list[float],
    mode: str,
    side_offset: float,
    down_offset: float,
    source_basis: str,
    source_scale: float,
    per_case_source_bridge_root: Path,
) -> list[float] | None:
    if mode == "current":
        return None
    shoulder = contract.position(transforms, f"bone_{side}-upperArm.mesh")
    if shoulder is None:
        raise ValueError(f"missing shoulder for side {side}")
    if mode == "source-pose-per-case":
        source_bones = {
            "L": ("Bone_Bicep_L", "Bone_Forearm_L"),
            "R": ("Bone_Bicep_R", "Bone_Forearm_R"),
        }[side]
        active_source_bridge = per_case_source_bridge_path(
            per_case_source_bridge_root,
            solve_row,
        )
        source_elbow_offset = source_pose_local_offset_to_gh2(
            active_source_bridge,
            int(solve_row["frame"]),
            source_bones[0],
            source_bones[1],
            source_basis,
            source_scale,
        )
        return probe.local_offset_to_world(
            transforms[f"bone_{side}-upperArm.mesh"]["world"],
            source_elbow_offset,
        )
    if mode != "down-out":
        raise ValueError(f"unsupported visible arm elbow hint mode: {mode}")
    sign = -1.0 if side == "L" else 1.0
    return [
        (float(shoulder[0]) + float(target_world[0])) * 0.5 + sign * float(side_offset),
        (float(shoulder[1]) + float(target_world[1])) * 0.5,
        (float(shoulder[2]) + float(target_world[2])) * 0.5 - float(down_offset),
    ]


def apply_case_samples(
    tool: Path,
    candidate: Path,
    transforms: dict[str, dict[str, Any]],
    case: dict[str, Any],
    sample_quat_mode: str,
) -> None:
    main_milo = contract.candidate_milo(candidate, "gh3_midori_main.milo_ps2")
    frame = int(case["frame"])
    contract.apply_samples(
        transforms,
        contract.sample_channels(tool, main_milo, case["main_clip"], frame, ".pos"),
        contract.sample_channels(tool, main_milo, case["main_clip"], frame, ".quat"),
        sample_quat_mode=sample_quat_mode,
    )
    contract.refresh_worlds(transforms)


def target_proxy_locals_after_guitar(
    tool: Path,
    candidate: Path,
    solve_row: dict[str, Any],
    hmx_quat_mode: str,
    sample_quat_mode: str,
    viewer_prop_overrides: bool,
    solve_visible_arms: bool,
    arm_chain_reach_scale: float,
    solve_visible_hand_rotations: bool,
    solve_visible_clavicles: bool,
    visible_clavicle_blend: float,
    visible_arm_rotation_mode: str,
    visible_arm_target_mode: str,
    visible_arm_target_blend_with_current: float,
    visible_arm_target_swap_cases: set[str],
    visible_arm_grip_map: Path,
    visible_arm_source_coordinate_map: str,
    visible_arm_elbow_hint_mode: str,
    visible_arm_elbow_side_offset: float,
    visible_arm_elbow_down_offset: float,
    visible_arm_left_forearm_guitar_local: str | None,
    visible_arm_right_forearm_guitar_local: str | None,
    visible_arm_source_rotation_blend_with_aim: float,
    visible_hand_rotation_mode: str,
    visible_hand_axis_calibration: Path,
    left_hand_grip_guitar_local_axis: str | None,
    left_hand_grip_guitar_local_degrees: float,
    right_hand_grip_guitar_local_axis: str | None,
    right_hand_grip_guitar_local_degrees: float,
    source_bridge: Path,
    source_frame: int,
    source_basis: str,
    source_scale: float,
    per_case_source_bridge_root: Path,
) -> dict[str, list[float]]:
    selection = solve_row["selection"]
    outfit = "gh3_midori_2.milo_ps2" if selection == "gh3_midori_2" else "gh3_midori_1.milo_ps2"
    transforms = contract.load_transforms(
        tool,
        contract.candidate_milo(candidate, outfit, selection=selection),
    )
    apply_case_samples(tool, candidate, transforms, solve_row, sample_quat_mode)
    if viewer_prop_overrides:
        contract.apply_viewer_prop_overrides(transforms)
        contract.refresh_worlds(transforms)

    coupled = solve_row["coupled_guitar_anchor_solve"]
    guitar = transforms["bone_pos_guitar.mesh"]
    guitar["local"][9:12] = [float(value) for value in coupled["solved_guitar_local_position"]]
    guitar_rot = coupled["solved_guitar_local_rotation"]
    guitar["local"][:9] = [float(value) for row in guitar_rot for value in row]
    contract.refresh_worlds(transforms)

    arm_patch_values: dict[str, list[float]] = {}
    arm_solve: dict[str, Any] | None = None
    if solve_visible_arms:
        left_forearm_override = parse_optional_vector(
            visible_arm_left_forearm_guitar_local,
            "visible_arm_left_forearm_guitar_local",
        )
        right_forearm_override = parse_optional_vector(
            visible_arm_right_forearm_guitar_local,
            "visible_arm_right_forearm_guitar_local",
        )
        left_target_world, right_target_world, target_info = visible_arm_target_worlds(
            transforms,
            solve_row,
            visible_arm_target_mode,
            visible_arm_target_blend_with_current,
            visible_arm_target_swap_cases,
            visible_arm_grip_map,
            visible_arm_source_coordinate_map,
            source_basis,
            source_scale,
            per_case_source_bridge_root,
        )
        clavicle_solve: dict[str, Any] = {}
        if solve_visible_clavicles:
            for side, target_world in (("L", left_target_world), ("R", right_target_world)):
                clavicle_name = f"bone_{side}-clavicle.mesh"
                upper_name = f"bone_{side}-upperArm.mesh"
                clavicle_rot = probe.solve_aim_local_rotation(
                    transforms,
                    clavicle_name,
                    contract.position(transforms, upper_name),
                    target_world,
                )
                current_clavicle_rot = [
                    [float(value) for value in transforms[clavicle_name]["local"][0:3]],
                    [float(value) for value in transforms[clavicle_name]["local"][3:6]],
                    [float(value) for value in transforms[clavicle_name]["local"][6:9]],
                ]
                clavicle_rot = stage.blend_rotation_matrices(
                    current_clavicle_rot,
                    clavicle_rot,
                    visible_clavicle_blend,
                )
                transforms[clavicle_name]["local"][:9] = [
                    value for row in clavicle_rot for value in row
                ]
                arm_patch_values[f"bone_{side}-clavicle.mesh.quat"] = output_quat_xyzw(
                    clavicle_rot,
                    hmx_quat_mode,
                )
                clavicle_solve[side] = {
                    "clavicle": clavicle_name,
                    "upper": upper_name,
                    "target_world": [float(value) for value in target_world],
                    "blend": visible_clavicle_blend,
                }
            contract.refresh_worlds(transforms)
        left_chain = probe.solve_two_bone_position_chain(
            transforms,
            "L",
            left_target_world,
            arm_chain_reach_scale,
            visible_arm_elbow_hint_world(
                transforms,
                solve_row,
                "L",
                left_target_world,
                visible_arm_elbow_hint_mode,
                visible_arm_elbow_side_offset,
                visible_arm_elbow_down_offset,
                source_basis,
                source_scale,
                per_case_source_bridge_root,
            ),
        )
        right_chain = probe.solve_two_bone_position_chain(
            transforms,
            "R",
            right_target_world,
            arm_chain_reach_scale,
            visible_arm_elbow_hint_world(
                transforms,
                solve_row,
                "R",
                right_target_world,
                visible_arm_elbow_hint_mode,
                visible_arm_elbow_side_offset,
                visible_arm_elbow_down_offset,
                source_basis,
                source_scale,
                per_case_source_bridge_root,
            ),
        )
        forearm_overrides: dict[str, Any] = {}
        for side, override in (("L", left_forearm_override), ("R", right_forearm_override)):
            if override is None:
                continue
            guitar_world = transforms["bone_pos_guitar.mesh"]["world"]
            override_world = probe.local_offset_to_world(guitar_world, override)
            chain = left_chain if side == "L" else right_chain
            chain["elbow_world_before_forearm_override"] = list(chain["elbow_world"])
            chain["elbow_world"] = [float(value) for value in override_world]
            chain["forearm_guitar_local_override"] = [float(value) for value in override]
            chain["forearm_override_space"] = "guitar-local"
            forearm_overrides[side] = {
                "guitar_local": [float(value) for value in override],
                "world": [float(value) for value in override_world],
            }
        if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}:
            active_source_bridge = source_bridge
            active_source_frame = source_frame
            source_bones = {
                "left_upper": "bone_bicep_l",
                "left_fore": "bone_forearm_l",
                "right_upper": "bone_bicep_r",
                "right_fore": "bone_forearm_r",
            }
            if visible_arm_rotation_mode in {"source-per-case", "source-pose-per-case"}:
                active_source_bridge = per_case_source_bridge_path(
                    per_case_source_bridge_root,
                    solve_row,
                )
                active_source_frame = int(solve_row["frame"])
                source_bones = {
                    "left_upper": "Bone_Bicep_L",
                    "left_fore": "Bone_Forearm_L",
                    "right_upper": "Bone_Bicep_R",
                    "right_fore": "Bone_Forearm_R",
                }
            source_rotation = (
                source_bridge_pose_local_rotation_to_gh2
                if visible_arm_rotation_mode == "source-pose-per-case"
                else probe.source_bridge_local_rotation_to_gh2
            )
            left_upper_rot = source_rotation(
                active_source_bridge,
                active_source_frame,
                source_bones["left_upper"],
                source_basis,
            )
            right_upper_rot = source_rotation(
                active_source_bridge,
                active_source_frame,
                source_bones["right_upper"],
                source_basis,
            )
            source_blend = float(visible_arm_source_rotation_blend_with_aim)
            if source_blend < 1.0:
                left_upper_aim = probe.solve_aim_local_rotation(
                    transforms,
                    "bone_L-upperArm.mesh",
                    left_chain["current_elbow"],
                    left_chain["elbow_world"],
                )
                right_upper_aim = probe.solve_aim_local_rotation(
                    transforms,
                    "bone_R-upperArm.mesh",
                    right_chain["current_elbow"],
                    right_chain["elbow_world"],
                )
                left_upper_rot = stage.blend_rotation_matrices(
                    left_upper_aim,
                    left_upper_rot,
                    source_blend,
                )
                right_upper_rot = stage.blend_rotation_matrices(
                    right_upper_aim,
                    right_upper_rot,
                    source_blend,
                )
        else:
            left_upper_rot = probe.solve_aim_local_rotation(
                transforms,
                "bone_L-upperArm.mesh",
                left_chain["current_elbow"],
                left_chain["elbow_world"],
            )
            right_upper_rot = probe.solve_aim_local_rotation(
                transforms,
                "bone_R-upperArm.mesh",
                right_chain["current_elbow"],
                right_chain["elbow_world"],
            )
        transforms["bone_L-upperArm.mesh"]["local"][:9] = [
            value for row in left_upper_rot for value in row
        ]
        transforms["bone_R-upperArm.mesh"]["local"][:9] = [
            value for row in right_upper_rot for value in row
        ]
        contract.refresh_worlds(transforms)
        if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}:
            left_fore_rot = source_rotation(
                active_source_bridge,
                active_source_frame,
                source_bones["left_fore"],
                source_basis,
            )
            right_fore_rot = source_rotation(
                active_source_bridge,
                active_source_frame,
                source_bones["right_fore"],
                source_basis,
            )
            source_blend = float(visible_arm_source_rotation_blend_with_aim)
            if source_blend < 1.0:
                left_fore_aim = probe.solve_aim_local_rotation(
                    transforms,
                    "bone_L-foreArm.mesh",
                    contract.position(transforms, "bone_L-hand.mesh"),
                    left_chain["target_reachable"],
                )
                right_fore_aim = probe.solve_aim_local_rotation(
                    transforms,
                    "bone_R-foreArm.mesh",
                    contract.position(transforms, "bone_R-hand.mesh"),
                    right_chain["target_reachable"],
                )
                left_fore_rot = stage.blend_rotation_matrices(
                    left_fore_aim,
                    left_fore_rot,
                    source_blend,
                )
                right_fore_rot = stage.blend_rotation_matrices(
                    right_fore_aim,
                    right_fore_rot,
                    source_blend,
                )
        else:
            left_fore_rot = probe.solve_aim_local_rotation(
                transforms,
                "bone_L-foreArm.mesh",
                contract.position(transforms, "bone_L-hand.mesh"),
                left_chain["target_reachable"],
            )
            right_fore_rot = probe.solve_aim_local_rotation(
                transforms,
                "bone_R-foreArm.mesh",
                contract.position(transforms, "bone_R-hand.mesh"),
                right_chain["target_reachable"],
            )
        transforms["bone_L-foreArm.mesh"]["local"][:9] = [
            value for row in left_fore_rot for value in row
        ]
        transforms["bone_R-foreArm.mesh"]["local"][:9] = [
            value for row in right_fore_rot for value in row
        ]
        contract.refresh_worlds(transforms)
        left_fore_local = final_solve.local_position_for_world(
            transforms,
            "bone_L-foreArm.mesh",
            left_chain["elbow_world"],
        )
        right_fore_local = final_solve.local_position_for_world(
            transforms,
            "bone_R-foreArm.mesh",
            right_chain["elbow_world"],
        )
        transforms["bone_L-foreArm.mesh"]["local"][9:12] = left_fore_local
        transforms["bone_R-foreArm.mesh"]["local"][9:12] = right_fore_local
        contract.refresh_worlds(transforms)
        left_hand_local = final_solve.local_position_for_world(
            transforms,
            "bone_L-hand.mesh",
            left_chain["target_reachable"],
        )
        right_hand_local = final_solve.local_position_for_world(
            transforms,
            "bone_R-hand.mesh",
            right_chain["target_reachable"],
        )
        transforms["bone_L-hand.mesh"]["local"][9:12] = left_hand_local
        transforms["bone_R-hand.mesh"]["local"][9:12] = right_hand_local
        contract.refresh_worlds(transforms)
        if solve_visible_hand_rotations:
            hand_axis_calibration = (
                load_json(visible_hand_axis_calibration)
                if visible_hand_rotation_mode == "visible-axis-calibration"
                else None
            )
            for hand_name, target_name, channel_name in (
                ("bone_L-hand.mesh", "bone_fret_hand.mesh", "bone_L-hand.mesh.quat"),
                ("bone_R-hand.mesh", "bone_strum_hand.mesh", "bone_R-hand.mesh.quat"),
            ):
                parent_name = transforms[hand_name].get("parent") or ""
                if parent_name not in transforms:
                    raise ValueError(f"{solve_row['name']}: missing parent for {hand_name}")
                if visible_hand_rotation_mode == "target-proxy":
                    desired_local = final_solve.matrix_local_to_parent(
                        transforms[target_name]["world"],
                        transforms[parent_name]["world"],
                    )
                    desired_rot = [
                        [float(value) for value in desired_local[0:3]],
                        [float(value) for value in desired_local[3:6]],
                        [float(value) for value in desired_local[6:9]],
                    ]
                elif visible_hand_rotation_mode == "visible-axis-calibration":
                    side = "L" if hand_name.startswith("bone_L-") else "R"
                    grip_axis = parse_optional_axis(
                        left_hand_grip_guitar_local_axis
                        if side == "L"
                        else right_hand_grip_guitar_local_axis
                    )
                    grip_degrees = (
                        left_hand_grip_guitar_local_degrees
                        if side == "L"
                        else right_hand_grip_guitar_local_degrees
                    )
                    desired_rot = visible_axis_calibrated_hand_local_rotation(
                        transforms,
                        side,
                        hand_axis_calibration or {},
                        grip_axis,
                        grip_degrees,
                    )
                else:
                    raise ValueError(f"unsupported visible hand rotation mode: {visible_hand_rotation_mode}")
                transforms[hand_name]["local"][:9] = [
                    value for row in desired_rot for value in row
                ]
                arm_patch_values[channel_name] = output_quat_xyzw(
                    desired_rot,
                    hmx_quat_mode,
                )
            contract.refresh_worlds(transforms)
        arm_patch_values = {
            "bone_L-upperArm.mesh.quat": output_quat_xyzw(left_upper_rot, hmx_quat_mode),
            "bone_L-foreArm.mesh.pos": [float(value) for value in left_fore_local],
            "bone_L-foreArm.mesh.quat": output_quat_xyzw(left_fore_rot, hmx_quat_mode),
            "bone_L-hand.mesh.pos": [float(value) for value in left_hand_local],
            "bone_R-upperArm.mesh.quat": output_quat_xyzw(right_upper_rot, hmx_quat_mode),
            "bone_R-foreArm.mesh.pos": [float(value) for value in right_fore_local],
            "bone_R-foreArm.mesh.quat": output_quat_xyzw(right_fore_rot, hmx_quat_mode),
            "bone_R-hand.mesh.pos": [float(value) for value in right_hand_local],
            **arm_patch_values,
        }
        arm_solve = {
            "left_target_world": [float(value) for value in left_target_world],
            "right_target_world": [float(value) for value in right_target_world],
            "left_target_reachable": [float(value) for value in left_chain["target_reachable"]],
            "right_target_reachable": [float(value) for value in right_chain["target_reachable"]],
            "left_reach_clamped": bool(left_chain.get("clamped")),
            "right_reach_clamped": bool(right_chain.get("clamped")),
            "left_chain": left_chain,
            "right_chain": right_chain,
            "clavicle_solve": clavicle_solve,
            "visible_arm_rotation_mode": visible_arm_rotation_mode,
            "visible_arm_target_mode": visible_arm_target_mode,
            "visible_arm_target_info": target_info,
            "visible_arm_elbow_hint_mode": visible_arm_elbow_hint_mode,
            "visible_arm_elbow_side_offset": visible_arm_elbow_side_offset,
            "visible_arm_elbow_down_offset": visible_arm_elbow_down_offset,
            "visible_arm_forearm_overrides": forearm_overrides,
            "visible_arm_source_rotation_blend_with_aim": (
                visible_arm_source_rotation_blend_with_aim
                if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}
                else None
            ),
            "source_bridge": (
                rel(active_source_bridge)
                if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}
                else None
            ),
            "source_frame": (
                active_source_frame
                if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}
                else None
            ),
            "source_basis": (
                source_basis
                if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}
                else None
            ),
            "source_scale": (
                source_scale
                if visible_arm_rotation_mode in {"source", "source-per-case", "source-pose-per-case"}
                or visible_arm_target_mode != "current-proxies"
                else None
            ),
        }

    left_hand = contract.position(transforms, "bone_L-hand.mesh")
    right_hand = contract.position(transforms, "bone_R-hand.mesh")
    if left_hand is None or right_hand is None:
        raise ValueError(f"{solve_row['name']}: missing visible hand world")
    fret_local = final_solve.local_position_for_world(
        transforms,
        "bone_fret_hand.mesh",
        left_hand,
    )
    strum_local = final_solve.local_position_for_world(
        transforms,
        "bone_strum_hand.mesh",
        right_hand,
    )
    values = {
        "bone_pos_guitar.mesh.pos": [float(value) for value in coupled["solved_guitar_local_position"]],
        "bone_pos_guitar.mesh.quat": output_quat_xyzw(guitar_rot, hmx_quat_mode),
        "bone_fret_hand.mesh.pos": [float(value) for value in fret_local],
        "bone_strum_hand.mesh.pos": [float(value) for value in strum_local],
    }
    values.update(arm_patch_values)
    if arm_solve is not None:
        values["_arm_solve"] = arm_solve  # type: ignore[assignment]
    return values


def pack_sample(channel: str, values: list[float]) -> bytes:
    expected = 3 if channel.endswith(".pos") else 4
    if len(values) != expected:
        raise ValueError(f"{channel}: expected {expected} floats, got {len(values)}")
    return struct.pack("<" + ("3f" if expected == 3 else "4f"), *values)


def patch_signature(values: dict[str, list[float]]) -> dict[str, list[float]]:
    return {
        key: [round(float(value), 9) for value in item]
        for key, item in values.items()
        if not key.startswith("_")
    }


def build(args: argparse.Namespace) -> dict[str, Any]:
    candidate = args.candidate.resolve()
    main_milo = contract.candidate_milo(candidate, "gh3_midori_main.milo_ps2")
    solve = load_json(args.solve_report)
    solve_rows = [row for row in solve.get("cases", []) if row.get("coupled_guitar_anchor_solve")]
    if args.case_name:
        wanted = set(args.case_name)
        solve_rows = [row for row in solve_rows if row.get("name") in wanted]
        missing = sorted(wanted - {str(row.get("name")) for row in solve_rows})
        if missing:
            raise ValueError(f"solve row(s) not found: {', '.join(missing)}")
    if not solve_rows:
        raise ValueError(f"{args.solve_report}: no coupled solve rows")
    inventory = parse_clip_inventory(args.tool, main_milo)
    args.acp_dir.mkdir(parents=True, exist_ok=True)
    forearm_override_map = load_forearm_override_map(args.visible_arm_forearm_override_map)

    rows = []
    clips_to_replace = []
    clip_patch_signatures: dict[str, dict[str, Any]] = {}
    for solve_row in solve_rows:
        clip_name = solve_row["main_clip"]
        info = inventory.get(clip_name)
        if info is None:
            raise ValueError(f"main clip not found: {clip_name}")
        sample_count = int(info["sample_count"])
        contact_patch_channels = set() if args.skip_contact_patches else set(PATCH_CHANNELS)
        requested_patch_channels = set(contact_patch_channels)
        if args.solve_visible_arms:
            requested_patch_channels.update(VISIBLE_ARM_PATCH_CHANNELS)
            if args.solve_visible_clavicles:
                requested_patch_channels.update(VISIBLE_CLAVICLE_PATCH_CHANNELS)
            if args.solve_visible_hand_rotations:
                requested_patch_channels.update(VISIBLE_HAND_ROTATION_PATCH_CHANNELS)
        channels = stage.channel_order(set(info["channels"]).union(requested_patch_channels))
        sampled = sample_clip_values(args.tool, main_milo, clip_name, sample_count)
        patch_values = target_proxy_locals_after_guitar(
            args.tool,
            candidate,
            solve_row,
            args.hmx_quat_mode,
            args.sample_quat_mode,
            args.viewer_prop_overrides,
            args.solve_visible_arms,
            args.arm_chain_reach_scale,
            args.solve_visible_hand_rotations,
            args.solve_visible_clavicles,
            args.visible_clavicle_blend,
            args.visible_arm_rotation_mode,
            args.visible_arm_target_mode,
            args.visible_arm_target_blend_with_current,
            set(args.visible_arm_target_swap_case),
            args.visible_arm_grip_map,
            args.visible_arm_source_coordinate_map,
            args.visible_arm_elbow_hint_mode,
            args.visible_arm_elbow_side_offset,
            args.visible_arm_elbow_down_offset,
            case_forearm_override_arg(
                forearm_override_map,
                solve_row["name"],
                "L",
                args.visible_arm_left_forearm_guitar_local,
            ),
            case_forearm_override_arg(
                forearm_override_map,
                solve_row["name"],
                "R",
                args.visible_arm_right_forearm_guitar_local,
            ),
            args.visible_arm_source_rotation_blend_with_aim,
            args.visible_hand_rotation_mode,
            args.visible_hand_axis_calibration,
            args.left_hand_grip_guitar_local_axis,
            args.left_hand_grip_guitar_local_degrees,
            args.right_hand_grip_guitar_local_axis,
            args.right_hand_grip_guitar_local_degrees,
            args.source_bridge,
            args.source_frame,
            args.source_basis,
            args.source_scale,
            args.per_case_source_bridge_root,
        )
        arm_solve = patch_values.pop("_arm_solve", None)
        if args.skip_contact_patches:
            for channel in PATCH_CHANNELS:
                patch_values.pop(channel, None)
        signature = patch_signature(patch_values)
        previous_signature = clip_patch_signatures.get(clip_name)
        if previous_signature is not None and previous_signature["signature"] != signature:
            if args.duplicate_clip_policy == "error":
                raise ValueError(
                    f"{clip_name}: duplicate clip patch conflict between "
                    f"{previous_signature['case']} and {solve_row['name']}; "
                    "use a shared clip map or pass --duplicate-clip-policy "
                    "first/last for an explicit diagnostic overwrite"
                )
            if args.duplicate_clip_policy == "first":
                rows.append(
                    {
                        "case": solve_row["name"],
                        "clip": clip_name,
                        "duplicate_clip_skipped": True,
                        "duplicate_clip_policy": args.duplicate_clip_policy,
                        "previous_case": previous_signature["case"],
                    }
                )
                continue
        else:
            clip_patch_signatures[clip_name] = {
                "case": solve_row["name"],
                "signature": signature,
            }
        sample_bytes = bytearray()
        for frame in range(sample_count):
            for channel in channels:
                values = patch_values[channel] if channel in patch_values else sampled[frame].get(channel)
                if values is None:
                    raise ValueError(f"{clip_name}: missing sampled channel {channel} at frame {frame}")
                sample_bytes.extend(pack_sample(channel, values))
        acp = stage.acp_bytes(
            clip_name,
            channels,
            sample_count,
            bytes(sample_bytes),
            sample_count / 60.0,
            int(info["clip_flags"]),
            0,
        )
        (args.acp_dir / f"{clip_name}.acp").write_bytes(acp)
        clips_to_replace.append(clip_name)
        rows.append(
            {
                "case": solve_row["name"],
                "clip": clip_name,
                "sample_count": sample_count,
                "channel_count": len(channels),
                "added_channels": sorted(set(channels) - set(info["channels"])),
                "patched_channels": sorted(contact_patch_channels),
                "visible_arm_channels": list(VISIBLE_ARM_PATCH_CHANNELS) if args.solve_visible_arms else [],
                "visible_clavicle_channels": (
                    list(VISIBLE_CLAVICLE_PATCH_CHANNELS)
                    if args.solve_visible_arms and args.solve_visible_clavicles
                    else []
                ),
                "visible_hand_rotation_channels": (
                    list(VISIBLE_HAND_ROTATION_PATCH_CHANNELS)
                    if args.solve_visible_arms and args.solve_visible_hand_rotations
                    else []
                ),
                "arm_solve": arm_solve,
                "patch_values": patch_values,
            }
        )

    run_tool(
        args.tool,
        [
            "build-clipset-from-acp",
            str(args.acp_dir),
            "--name",
            "gh3_midori_r135_fullclip_contact_donor",
            "--role",
            "guitar-main",
            "--move-self",
            "0",
            "--out",
            str(args.donor),
        ],
    )
    replace_args = [
        "replace-clipset-clips",
        str(main_milo),
        "--donor",
        str(args.donor),
    ]
    for clip_name in sorted(set(clips_to_replace)):
        replace_args.extend(["--clip", clip_name])
    replace_args.extend(["--out", str(args.output)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    run_tool(args.tool, replace_args)

    report = {
        "format": "gh3_midori_fullclip_coupled_contact_candidate_v1",
        "status": "candidate_built",
        "revision": args.revision,
        "candidate": rel(candidate),
        "source_main_milo": rel(main_milo),
        "solve_report": rel(args.solve_report),
        "output_main_milo": {
            "path": rel(args.output),
            "size": args.output.stat().st_size,
            "sha256": sha256_file(args.output),
        },
        "donor_milo": rel(args.donor),
        "acp_dir": rel(args.acp_dir),
        "hmx_quat_mode": args.hmx_quat_mode,
        "sample_quat_mode": args.sample_quat_mode,
        "viewer_prop_overrides": args.viewer_prop_overrides,
        "solve_visible_arms": args.solve_visible_arms,
        "arm_chain_reach_scale": args.arm_chain_reach_scale,
        "solve_visible_hand_rotations": args.solve_visible_hand_rotations,
        "solve_visible_clavicles": args.solve_visible_clavicles,
        "visible_clavicle_blend": args.visible_clavicle_blend,
        "skip_contact_patches": args.skip_contact_patches,
        "visible_arm_rotation_mode": args.visible_arm_rotation_mode,
        "visible_arm_left_forearm_guitar_local": args.visible_arm_left_forearm_guitar_local,
        "visible_arm_right_forearm_guitar_local": args.visible_arm_right_forearm_guitar_local,
        "case_filter": args.case_name,
        "visible_arm_forearm_override_map": (
            rel(args.visible_arm_forearm_override_map)
            if args.visible_arm_forearm_override_map
            else None
        ),
        "source_bridge": rel(args.source_bridge),
        "per_case_source_bridge_root": rel(args.per_case_source_bridge_root),
        "source_frame": args.source_frame,
        "source_basis": args.source_basis,
        "duplicate_clip_policy": args.duplicate_clip_policy,
        "clip_count": len(rows),
        "rows": rows,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> int:
    set_low_priority()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", type=Path, default=DEFAULT_TOOL)
    parser.add_argument("--candidate", type=Path, default=DEFAULT_CANDIDATE)
    parser.add_argument("--solve-report", type=Path, default=DEFAULT_SOLVE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--donor", type=Path, default=DEFAULT_DONOR)
    parser.add_argument("--acp-dir", type=Path, default=DEFAULT_ACP_DIR)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--revision", default=DEFAULT_REVISION)
    parser.add_argument("--hmx-quat-mode", choices=("direct", "transpose"), default="transpose")
    parser.add_argument("--sample-quat-mode", choices=("direct", "hmx"), default="hmx")
    parser.add_argument("--viewer-prop-overrides", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--solve-visible-arms", action="store_true")
    parser.add_argument("--arm-chain-reach-scale", type=float, default=0.94)
    parser.add_argument("--solve-visible-hand-rotations", action="store_true")
    parser.add_argument("--solve-visible-clavicles", action="store_true")
    parser.add_argument("--visible-clavicle-blend", type=float, default=1.0)
    parser.add_argument(
        "--skip-contact-patches",
        action="store_true",
        help="Leave guitar/fret/strum contact channels unchanged; useful for arm-only diagnostics.",
    )
    parser.add_argument(
        "--visible-arm-rotation-mode",
        choices=("aim", "source", "source-per-case", "source-pose-per-case"),
        default="aim",
    )
    parser.add_argument(
        "--visible-arm-target-mode",
        choices=(
            "current-proxies",
            "source-palm-locals-per-case",
            "source-ik-helper-locals-per-case",
            "swapped-current-proxies",
            "mapped-current-proxies",
            "explicit-guitar-local-grip",
            "source-palm-fit-stock-hand-targets-per-case",
        ),
        default="current-proxies",
    )
    parser.add_argument("--visible-arm-target-blend-with-current", type=float, default=1.0)
    parser.add_argument("--visible-arm-target-swap-case", action="append", default=[])
    parser.add_argument("--visible-arm-grip-map", type=Path, default=ROOT / "analysis" / "gh3_midori_explicit_guitar_grip_map_r164.json")
    parser.add_argument(
        "--visible-arm-source-coordinate-map",
        choices=("palm-fit-negx-y-negz", "direct"),
        default="palm-fit-negx-y-negz",
    )
    parser.add_argument(
        "--visible-arm-elbow-hint-mode",
        choices=("current", "down-out", "source-pose-per-case"),
        default="current",
    )
    parser.add_argument("--visible-arm-elbow-side-offset", type=float, default=5.0)
    parser.add_argument("--visible-arm-elbow-down-offset", type=float, default=8.0)
    parser.add_argument(
        "--visible-arm-left-forearm-guitar-local",
        help="Opt-in visible left forearm/elbow target in solved guitar-local space.",
    )
    parser.add_argument(
        "--visible-arm-right-forearm-guitar-local",
        help="Opt-in visible right forearm/elbow target in solved guitar-local space.",
    )
    parser.add_argument(
        "--visible-arm-forearm-override-map",
        type=Path,
        help="Optional JSON case map overriding visible forearm guitar-local targets per diagnostic case.",
    )
    parser.add_argument("--visible-arm-source-rotation-blend-with-aim", type=float, default=1.0)
    parser.add_argument(
        "--case-name",
        action="append",
        default=[],
        help="Limit the build to one diagnostic solve row. May be repeated.",
    )
    parser.add_argument(
        "--visible-hand-rotation-mode",
        choices=("target-proxy", "visible-axis-calibration"),
        default="target-proxy",
    )
    parser.add_argument("--visible-hand-axis-calibration", type=Path, default=DEFAULT_VISIBLE_HAND_AXIS_CALIBRATION)
    parser.add_argument("--left-hand-grip-guitar-local-axis")
    parser.add_argument("--left-hand-grip-guitar-local-degrees", type=float, default=0.0)
    parser.add_argument("--right-hand-grip-guitar-local-axis")
    parser.add_argument("--right-hand-grip-guitar-local-degrees", type=float, default=0.0)
    parser.add_argument("--source-bridge", type=Path, default=probe.DEFAULT_SOURCE_BRIDGE)
    parser.add_argument("--per-case-source-bridge-root", type=Path, default=DEFAULT_PER_CASE_SOURCE_BRIDGE_ROOT)
    parser.add_argument("--source-frame", type=int, default=30)
    parser.add_argument("--source-basis", choices=("direct", "anim", "helper"), default="direct")
    parser.add_argument("--source-scale", type=float, default=probe.GH3_PS2_SKELETON_TO_GH2_SCALE)
    parser.add_argument(
        "--duplicate-clip-policy",
        choices=("error", "first", "last"),
        default="error",
        help="How to handle multiple solve rows that patch the same main clip.",
    )
    parser.add_argument("--print-summary", action="store_true")
    args = parser.parse_args()
    report = build(args)
    if args.print_summary:
        output = report["output_main_milo"]
        print(
            "status=%s clips=%d bytes=%d sha256=%s"
            % (report["status"], report["clip_count"], output["size"], output["sha256"])
        )
        for row in report["rows"]:
            print(
                "%s\tclip=%s\tchannels=%d\tadded=%s"
                % (row["case"], row["clip"], row["channel_count"], ",".join(row["added_channels"]))
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
