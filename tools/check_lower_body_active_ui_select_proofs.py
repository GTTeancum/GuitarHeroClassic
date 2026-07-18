#!/usr/bin/env python3
"""Verify active Glam1/Metal1 UI/select flat-foot lower-body proof screenshots."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import sys

from compare_charbone_output_map import parse_output_rows, read_log_text


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
MAX_FLAT_TOE_ABS_Z = 0.75
MAX_LEFT_RIGHT_TOE_DELTA_Z = 0.75
MIN_PELVIS_TO_TOE_Z = 30.0
MIN_CHAIN_Z_DROP = 4.0

LOWER_BODY_BONES = (
    "bone_pelvis",
    "bone_L-thigh",
    "bone_L-knee",
    "bone_L-ankle",
    "bone_L-toe",
    "bone_R-thigh",
    "bone_R-knee",
    "bone_R-ankle",
    "bone_R-toe",
)


@dataclass(frozen=True)
class ProofCase:
    character: str
    frame: int
    view: str
    log_name: str
    png_name: str
    source_model: str
    source_clip: str


PROOF_CASES = (
    ProofCase(
        character="glam1",
        frame=30,
        view="front",
        log_name="glam1_ui_loop_f030_front.log",
        png_name="glam1_ui_loop_f030_front.png",
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    ProofCase(
        character="glam1",
        frame=30,
        view="side",
        log_name="glam1_ui_loop_f030_side.log",
        png_name="glam1_ui_loop_f030_side.png",
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    ProofCase(
        character="glam1",
        frame=40,
        view="front",
        log_name="glam1_ui_loop_f040_front.log",
        png_name="glam1_ui_loop_f040_front.png",
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    ProofCase(
        character="glam1",
        frame=40,
        view="side",
        log_name="glam1_ui_loop_f040_side.log",
        png_name="glam1_ui_loop_f040_side.png",
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        frame=30,
        view="front",
        log_name="metal1_ui_loop_f030_front.log",
        png_name="metal1_ui_loop_f030_front.png",
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        frame=30,
        view="side",
        log_name="metal1_ui_loop_f030_side.log",
        png_name="metal1_ui_loop_f030_side.png",
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        frame=40,
        view="front",
        log_name="metal1_ui_loop_f040_front.log",
        png_name="metal1_ui_loop_f040_front.png",
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        frame=40,
        view="side",
        log_name="metal1_ui_loop_f040_side.log",
        png_name="metal1_ui_loop_f040_side.png",
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as in_file:
        header = in_file.read(24)
    require(header.startswith(PNG_SIGNATURE), f"{path}: not a PNG")
    require(len(header) >= 24, f"{path}: truncated PNG header")
    return struct.unpack(">II", header[16:24])


def require_png(path: Path) -> None:
    require(path.is_file(), f"{path}: missing visual proof PNG")
    width, height = png_dimensions(path)
    require(
        width >= MIN_PROOF_WIDTH and height >= MIN_PROOF_HEIGHT,
        f"{path}: proof resolution too small {width}x{height}",
    )
    require(path.stat().st_size > 10_000, f"{path}: proof PNG too small")


def require_text(text: str, fragment: str, path: Path, label: str) -> None:
    require(fragment in text, f"{path}: missing {label}: {fragment}")


@dataclass(frozen=True)
class CaseMetrics:
    character: str
    max_abs_toe_z: float
    toe_delta_z: float
    min_pelvis_to_toe_z: float
    max_output_visible_gap: float


def check_case(root: Path, case: ProofCase) -> CaseMetrics:
    log_path = root / case.log_name
    png_path = root / case.png_name
    require(log_path.is_file(), f"{case.character} {case.view}: missing log {log_path}")
    require_png(png_path)

    text = read_log_text(log_path)
    require_text(
        text,
        f"[char] {case.source_model}:",
        log_path,
        "UI character source",
    )
    require_text(text, f"[char] loaded '{case.character}'", log_path, "character load")
    require_text(
        text,
        f"[clip] 'ui_loop' from {case.source_clip}",
        log_path,
        "UI loop source clip",
    )
    require_text(text, "[char] reference base enabled", log_path, "reference base")
    require_text(
        text,
        f"[char] clip-frame override enabled: {case.frame}",
        log_path,
        "frame",
    )
    require_text(text, "[ghogx] screenshot saved:", log_path, "screenshot marker")
    require("prop 'xplorer' attached" not in text, f"{log_path}: guitar prop should be absent")

    rows, visible_rows, screenshot_line = parse_output_rows(log_path, True)
    require(screenshot_line is not None, f"{log_path}: missing screenshot line")

    def visible_world(bone: str) -> tuple[float, float, float]:
        row = visible_rows.get((case.character, "lower-output", bone))
        require(row is not None, f"{log_path}: missing lower-output visible row for {bone}")
        return row.world

    max_output_visible_gap = 0.0
    for bone in LOWER_BODY_BONES:
        row = rows.get(bone)
        require(row is not None, f"{log_path}: missing output row {bone}")
        require(row.driven, f"{log_path}: output row {bone} is not driven")
        require(row.live, f"{log_path}: output row {bone} is not live")
        output_world = row.vectors.get("outPoseW")
        require(output_world is not None, f"{log_path}: output row {bone} missing outPoseW")
        visible = visible_world(bone)
        gap = tuple(visible[i] - output_world[i] for i in range(3))
        max_output_visible_gap = max(max_output_visible_gap, *(abs(x) for x in gap))

    pelvis_z = visible_world("bone_pelvis")[2]
    toe_z = {
        "L": visible_world("bone_L-toe")[2],
        "R": visible_world("bone_R-toe")[2],
    }
    for side in ("L", "R"):
        thigh_z = visible_world(f"bone_{side}-thigh")[2]
        knee_z = visible_world(f"bone_{side}-knee")[2]
        ankle_z = visible_world(f"bone_{side}-ankle")[2]
        require(
            knee_z <= thigh_z - MIN_CHAIN_Z_DROP,
            f"{log_path}: {side} knee is not below thigh enough",
        )
        require(
            ankle_z <= knee_z - MIN_CHAIN_Z_DROP,
            f"{log_path}: {side} ankle is not below knee enough",
        )
        require(
            toe_z[side] <= ankle_z,
            f"{log_path}: {side} toe is above ankle",
        )

    max_abs_toe_z = max(abs(toe_z["L"]), abs(toe_z["R"]))
    toe_delta_z = abs(toe_z["L"] - toe_z["R"])
    min_pelvis_to_toe_z = min(pelvis_z - toe_z["L"], pelvis_z - toe_z["R"])
    require(
        max_abs_toe_z <= MAX_FLAT_TOE_ABS_Z,
        f"{log_path}: toes are not near the floor enough ({max_abs_toe_z:.4f})",
    )
    require(
        toe_delta_z <= MAX_LEFT_RIGHT_TOE_DELTA_Z,
        f"{log_path}: toe height delta too large ({toe_delta_z:.4f})",
    )
    require(
        min_pelvis_to_toe_z >= MIN_PELVIS_TO_TOE_Z,
        f"{log_path}: pelvis/toe drop too small ({min_pelvis_to_toe_z:.4f})",
    )
    require(
        max_output_visible_gap <= 0.001,
        f"{log_path}: output/visible gap {max_output_visible_gap:.6f} > 0.001",
    )
    return CaseMetrics(
        character=case.character,
        max_abs_toe_z=max_abs_toe_z,
        toe_delta_z=toe_delta_z,
        min_pelvis_to_toe_z=min_pelvis_to_toe_z,
        max_output_visible_gap=max_output_visible_gap,
    )


def main() -> int:
    root = Path("engine/out/visual_proofs/lower_body_active_ui_select_20260715")
    try:
        metrics = [check_case(root, case) for case in PROOF_CASES]
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    characters = ",".join(sorted({item.character for item in metrics}))
    print(
        "PASS lower_body_active_ui_select_proofs "
        f"characters={characters} source_models=glam1_ui,metal1_ui source_clip=ui_loop "
        "frames=30,40 "
        "active_ui_select_flat_foot=true "
        f"cases={len(PROOF_CASES)} "
        f"max_abs_toe_z={max(item.max_abs_toe_z for item in metrics):.4f} "
        f"max_lr_toe_delta_z={max(item.toe_delta_z for item in metrics):.4f} "
        f"min_pelvis_to_toe_z={min(item.min_pelvis_to_toe_z for item in metrics):.4f} "
        f"max_output_visible_gap={max(item.max_output_visible_gap for item in metrics):.6f} "
        f"proof_min_resolution={MIN_PROOF_WIDTH}x{MIN_PROOF_HEIGHT}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
