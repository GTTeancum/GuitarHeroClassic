#!/usr/bin/env python3
"""Verify start-of-loop two-player character-select proof screenshots.

The stock 2P character-select screen does not name a separate 2P body clip.
It sends char_multi/animate, which uses the multiplayer branch in
char_objects.dtb: reset_hair, then ui_loop.  This checker pins the first
visible ui_loop frames so arbitrary later UI-loop frames are not treated as a
neutral lower-body standing oracle.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import sys

from compare_charbone_output_map import parse_output_rows, read_log_text


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
MAX_OUTPUT_VISIBLE_GAP = 0.001

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
    "glam1_p1_2p_animate_ui_loop_start_f000_side",
    "glam1_p1_2p_animate_ui_loop_start_f010_side",
    "metal1_p1_2p_animate_ui_loop_start_f000_side",
    "metal1_p1_2p_animate_ui_loop_start_f010_side",
)


@dataclass(frozen=True)
class ProofCase:
    character: str
    frame: int
    source_model: str
    source_clip: str

    @property
    def stem(self) -> str:
        return (
            f"{self.character}_p1_2p_animate_ui_loop_start_"
            f"f{self.frame:03d}_side"
        )


PROOF_CASES = (
    ProofCase(
        character="glam1",
        frame=0,
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    ProofCase(
        character="glam1",
        frame=10,
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        frame=0,
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        frame=10,
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
)


@dataclass(frozen=True)
class Metrics:
    max_output_visible_gap: float
    max_abs_toe_z: float
    min_pelvis_to_toe_z: float


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


def check_case(root: Path, case: ProofCase) -> Metrics:
    log_path = root / f"{case.stem}.log"
    png_path = root / f"{case.stem}.png"
    require(log_path.is_file(), f"{case.stem}: missing log")
    require_png(png_path)

    text = read_log_text(log_path)
    require("unknown arg: --char" not in text, f"{log_path}: captured wrong app")
    require_text(text, f"[char] {case.source_model}:", log_path, "UI source model")
    require_text(text, f"[char] loaded '{case.character}'", log_path, "character load")
    require_text(
        text,
        f"[clip] 'ui_loop' from {case.source_clip}",
        log_path,
        "source ui_loop clip",
    )
    require_text(
        text,
        f"[char] clip-frame override enabled: {case.frame}",
        log_path,
        "start frame",
    )
    require_text(
        text,
        "[2p-select] applied_placer=char_multi0.placer player=0 matrix=matrix0",
        log_path,
        "2P P1 placer",
    )
    require_text(text, "screen=multi_sel_character_screen", log_path, "2P screen")
    require_text(text, "panel=char_multi event=animate", log_path, "2P animate")
    require_text(text, "clip=ui_loop skips_ui_enter=true", log_path, "2P clip rule")
    require_text(text, "char_objects=char/gen/char_objects.dtb", log_path, "char_objects")
    require_text(text, "reset_hair=true", log_path, "2P reset hair rule")
    require_text(text, "[char] reference base enabled", log_path, "reference base")
    require_text(text, "[char] screenshot ->", log_path, "screenshot marker")
    require("[clip] 'ui_enter'" not in text, f"{log_path}: start proof must not play ui_enter")
    require("prop 'xplorer' attached" not in text, f"{log_path}: attached guitar prop leaked in")
    require("ui/gen/sel_character.milo" not in text, f"{log_path}: single-player geometry leaked in")

    rows, visible_rows, screenshot_line = parse_output_rows(log_path, True)
    require(screenshot_line is not None, f"{log_path}: missing screenshot line")

    max_output_visible_gap = 0.0
    visible_toe_z: list[float] = []
    pelvis_z = None
    for bone in LOWER_BODY_BONES:
        row = rows.get(bone)
        require(row is not None, f"{log_path}: missing output row {bone}")
        require(row.driven, f"{log_path}: output row {bone} is not driven")
        require(row.live, f"{log_path}: output row {bone} is not live")
        output_world = row.vectors.get("outPoseW")
        require(output_world is not None, f"{log_path}: {bone} missing outPoseW")
        visible = visible_rows.get((case.character, "lower-output", bone))
        require(visible is not None, f"{log_path}: missing visible lower-output row {bone}")
        gap = tuple(visible.world[i] - output_world[i] for i in range(3))
        max_output_visible_gap = max(max_output_visible_gap, *(abs(value) for value in gap))
        if bone == "bone_pelvis":
            pelvis_z = visible.world[2]
        if bone.endswith("-toe"):
            visible_toe_z.append(visible.world[2])

    require(
        max_output_visible_gap <= MAX_OUTPUT_VISIBLE_GAP,
        f"{log_path}: output/visible gap {max_output_visible_gap:.6f}",
    )
    require(pelvis_z is not None and len(visible_toe_z) == 2, f"{log_path}: missing pelvis/toe rows")
    return Metrics(
        max_output_visible_gap=max_output_visible_gap,
        max_abs_toe_z=max(abs(value) for value in visible_toe_z),
        min_pelvis_to_toe_z=min(pelvis_z - value for value in visible_toe_z),
    )


def main() -> int:
    root = Path("engine/out/visual_proofs/lower_body_2p_select_start_20260715")
    try:
        require(
            tuple(case.stem for case in PROOF_CASES) == EXPECTED_PROOF_STEMS,
            "2P select start-loop proof stems changed",
        )
        metrics = [check_case(root, case) for case in PROOF_CASES]
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(
        "PASS lower_body_2p_select_start_proofs "
        "characters=glam1,metal1 players=p1 "
        "screen=multi_sel_character_screen panel=char_multi "
        "event=animate multiplayer_clip=ui_loop skips_ui_enter=true "
        "frames=0,10 side_profile=true cases=4 "
        "single_player_ui_enter_absent=true single_player_geometry_absent=true "
        "start_loop_oracle=true neutral_standing_oracle=false "
        "attached_guitar_prop_absent=true "
        f"max_abs_toe_z={max(item.max_abs_toe_z for item in metrics):.4f} "
        f"min_pelvis_to_toe_z={min(item.min_pelvis_to_toe_z for item in metrics):.4f} "
        f"max_output_visible_gap={max(item.max_output_visible_gap for item in metrics):.6f} "
        f"proof_min_resolution={MIN_PROOF_WIDTH}x{MIN_PROOF_HEIGHT}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
