#!/usr/bin/env python3
"""Verify a broader 2P select lower-body source-family proof batch."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import sys

from compare_charbone_output_map import parse_output_rows, read_log_text


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
MAX_SELECT_TOE_ABS_Z = 2.0
MAX_SELECT_TOE_DELTA_Z = 1.5
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

EXPECTED_PROOF_STEMS = (
    "rock1_p1_2p_select_ui_loop_f030_side",
    "rock1_p1_2p_select_ui_loop_f040_side",
    "rock1_p2_2p_select_ui_loop_f030_side",
    "rock1_p2_2p_select_ui_loop_f040_side",
    "rock2_p1_2p_select_ui_loop_f030_side",
    "rock2_p1_2p_select_ui_loop_f040_side",
    "rock2_p2_2p_select_ui_loop_f030_side",
    "rock2_p2_2p_select_ui_loop_f040_side",
    "funk1_p1_2p_select_ui_loop_f030_side",
    "funk1_p1_2p_select_ui_loop_f040_side",
    "funk1_p2_2p_select_ui_loop_f030_side",
    "funk1_p2_2p_select_ui_loop_f040_side",
    "deathmetal1_p1_2p_select_ui_loop_f030_side",
    "deathmetal1_p1_2p_select_ui_loop_f040_side",
    "deathmetal1_p2_2p_select_ui_loop_f030_side",
    "deathmetal1_p2_2p_select_ui_loop_f040_side",
)


@dataclass(frozen=True)
class ProofCase:
    character: str
    player: int
    placer: str
    frame: int
    source_model: str
    source_clip: str

    @property
    def slot(self) -> str:
        return f"p{self.player + 1}"

    @property
    def stem(self) -> str:
        return (
            f"{self.character}_{self.slot}_2p_select_ui_loop_"
            f"f{self.frame:03d}_side"
        )


@dataclass(frozen=True)
class CaseMetrics:
    max_abs_toe_z: float
    toe_delta_z: float
    min_pelvis_to_toe_z: float
    max_output_visible_gap: float


def make_cases() -> tuple[ProofCase, ...]:
    character_sources = {
        "rock1": (
            "char/rock1/og/gen/rock1_ui.milo_ps2",
            "char/rock1/anims/gen/rock1_ui.milo_ps2",
        ),
        "rock2": (
            "char/rock2/og/gen/rock2_ui.milo_ps2",
            "char/rock1/anims/gen/rock1_ui.milo_ps2",
        ),
        "funk1": (
            "char/funk1/og/gen/funk1_ui.milo_ps2",
            "char/funk1/anims/gen/funk1_ui.milo_ps2",
        ),
        "deathmetal1": (
            "char/deathmetal1/og/gen/deathmetal1_ui.milo_ps2",
            "char/deathmetal1/anims/gen/deathmetal1_ui.milo_ps2",
        ),
    }
    placers = {
        0: "char_multi0.placer",
        1: "char_multi1.placer",
    }
    cases: list[ProofCase] = []
    for character, (source_model, source_clip) in character_sources.items():
        for player, placer in placers.items():
            for frame in (30, 40):
                cases.append(
                    ProofCase(
                        character=character,
                        player=player,
                        placer=placer,
                        frame=frame,
                        source_model=source_model,
                        source_clip=source_clip,
                    )
                )
    return tuple(cases)


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
    compact_text = "".join(text.split())
    compact_fragment = "".join(fragment.split())
    require(compact_fragment in compact_text, f"{path}: missing {label}: {fragment}")


def check_case(root: Path, case: ProofCase) -> CaseMetrics:
    log_path = root / f"{case.stem}.log"
    png_path = root / f"{case.stem}.png"
    require(log_path.is_file(), f"{case.stem}: missing log")
    require_png(png_path)

    text = read_log_text(log_path)
    require("unknown arg: --char" not in text, f"{log_path}: captured CLI tool, not app")
    require_text(text, f"[char] {case.source_model}:", log_path, "UI source model")
    require_text(text, f"[char] loaded '{case.character}'", log_path, "character load")
    require_text(
        text,
        f"[clip] 'ui_loop' from {case.source_clip}",
        log_path,
        "UI loop source clip",
    )
    require_text(
        text,
        f"[char] clip-frame override enabled: {case.frame}",
        log_path,
        "frame override",
    )
    require_text(text, "[char] reference base enabled", log_path, "reference base")
    require_text(text, "[char] screenshot ->", log_path, "screenshot marker")
    require_text(
        text,
        f"[2p-select] applied_placer={case.placer} player={case.player} matrix=matrix0",
        log_path,
        "applied 2P placer",
    )
    require_text(text, "source=ui/gen/multi_sel_character.milo_ps2", log_path, "2P source MILO")
    require_text(text, "screen=multi_sel_character_screen", log_path, "2P screen")
    require_text(text, "panel=char_multi event=select", log_path, "2P select event")
    require_text(text, "clip=ui_loop skips_ui_enter=true", log_path, "2P clip rule")
    require_text(text, "char_objects=char/gen/char_objects.dtb", log_path, "char_objects source")
    require_text(text, "reset_hair=false", log_path, "2P select reset hair rule")
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
        max_output_visible_gap = max(max_output_visible_gap, *(abs(value) for value in gap))

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
        require(toe_z[side] <= ankle_z, f"{log_path}: {side} toe is above ankle")

    max_abs_toe_z = max(abs(value) for value in toe_z.values())
    toe_delta_z = abs(toe_z["L"] - toe_z["R"])
    min_pelvis_to_toe_z = min(pelvis_z - value for value in toe_z.values())
    require(
        max_abs_toe_z <= MAX_SELECT_TOE_ABS_Z,
        f"{log_path}: select-pose toe z too far from floor ({max_abs_toe_z:.4f})",
    )
    require(
        toe_delta_z <= MAX_SELECT_TOE_DELTA_Z,
        f"{log_path}: select-pose toe delta too large ({toe_delta_z:.4f})",
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
        max_abs_toe_z=max_abs_toe_z,
        toe_delta_z=toe_delta_z,
        min_pelvis_to_toe_z=min_pelvis_to_toe_z,
        max_output_visible_gap=max_output_visible_gap,
    )


def main() -> int:
    root = Path("engine/out/visual_proofs/lower_body_2p_select_family_20260715")
    cases = make_cases()
    try:
        require(
            tuple(case.stem for case in cases) == EXPECTED_PROOF_STEMS,
            "2P select family proof stems changed",
        )
        metrics = [check_case(root, case) for case in cases]
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(
        "PASS lower_body_2p_select_family_proofs "
        "characters=rock1,rock2,funk1,deathmetal1 players=p1,p2 "
        "screen=multi_sel_character_screen panel=char_multi "
        "event=select multiplayer_clip=ui_loop skips_ui_enter=true "
        "camera_views=side frames=30,40 cases=16 "
        "individual_proofs=true both_2p_placers=true "
        "source_family_rock2_uses_rock1_ui_anim=true "
        f"max_abs_toe_z={max(item.max_abs_toe_z for item in metrics):.4f} "
        f"max_lr_toe_delta_z={max(item.toe_delta_z for item in metrics):.4f} "
        f"min_pelvis_to_toe_z={min(item.min_pelvis_to_toe_z for item in metrics):.4f} "
        f"max_output_visible_gap={max(item.max_output_visible_gap for item in metrics):.6f} "
        f"proof_min_resolution={MIN_PROOF_WIDTH}x{MIN_PROOF_HEIGHT}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
