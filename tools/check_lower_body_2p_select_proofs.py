#!/usr/bin/env python3
"""Verify the 2P character-select lower-body proof set."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct
import sys

from compare_charbone_output_map import parse_output_rows, read_log_text


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
MAX_FLAT_TOE_ABS_Z = 0.75
MAX_LEFT_RIGHT_TOE_DELTA_Z = 11.0
MIN_PELVIS_TO_TOE_Z = 30.0
MIN_CHAIN_Z_DROP = 10.0

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
    png_name: str
    log_name: str
    source_model: str
    source_clip: str
    placer: str
    player: int


PROOF_CASES = (
    ProofCase(
        character="glam1",
        frame=30,
        png_name="glam1_2p_animate_ui_loop_f030_front.png",
        log_name="glam1_2p_animate_ui_loop_f030_front.log",
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
        placer="char_multi0.placer",
        player=0,
    ),
    ProofCase(
        character="glam1",
        frame=40,
        png_name="glam1_2p_animate_ui_loop_f040_front.png",
        log_name="glam1_2p_animate_ui_loop_f040_front.log",
        source_model="char/glam1/og/gen/glam1_ui.milo_ps2",
        source_clip="char/glam1/anims/gen/glam1_ui.milo_ps2",
        placer="char_multi0.placer",
        player=0,
    ),
    ProofCase(
        character="metal1",
        frame=30,
        png_name="metal1_2p_animate_ui_loop_f030_front.png",
        log_name="metal1_2p_animate_ui_loop_f030_front.log",
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
        placer="char_multi1.placer",
        player=1,
    ),
    ProofCase(
        character="metal1",
        frame=40,
        png_name="metal1_2p_animate_ui_loop_f040_front.png",
        log_name="metal1_2p_animate_ui_loop_f040_front.log",
        source_model="char/metal1/og/gen/metal1_ui.milo_ps2",
        source_clip="char/metal1/anims/gen/metal1_ui.milo_ps2",
        placer="char_multi1.placer",
        player=1,
    ),
)


@dataclass(frozen=True)
class CaseMetrics:
    max_abs_toe_z: float
    toe_delta_z: float
    min_pelvis_to_toe_z: float
    max_output_visible_gap: float


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as in_file:
        header = in_file.read(24)
    require(header.startswith(PNG_SIGNATURE), f"{path}: not a PNG")
    require(len(header) >= 24, f"{path}: truncated PNG header")
    return struct.unpack(">II", header[16:24])


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_png(path: Path) -> str:
    require(path.is_file(), f"{path}: missing visual proof PNG")
    width, height = png_dimensions(path)
    require(
        width >= MIN_PROOF_WIDTH and height >= MIN_PROOF_HEIGHT,
        f"{path}: proof resolution too small {width}x{height}",
    )
    require(path.stat().st_size > 10_000, f"{path}: proof PNG too small")
    return sha256(path)


def require_text(text: str, fragment: str, path: Path, label: str) -> None:
    require(fragment in text, f"{path}: missing {label}: {fragment}")


def check_manifest(root: Path) -> dict:
    path = root / "tools/lower_body_2p_select_source_manifest.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    require(manifest["screen_script"] == "ui/gen/multiplayer.dtb", "wrong 2P screen script")
    require(manifest["screen"] == "multi_sel_character_screen", "wrong 2P screen")
    require(manifest["panel"] == "char_multi", "wrong CharsysPanel")
    require(manifest["screen_milo"] == "ui/gen/multi_sel_character.milo_ps2", "wrong 2P screen MILO")
    require(manifest["scroll_event"] == "{char_multi char_event $playerNum animate}", "wrong scroll event")
    require(
        manifest["outfit_focus_event"] == "{char_multi char_event [player_num] select}",
        "wrong select event",
    )
    require(manifest["char_objects_source"] == "char/gen/char_objects.dtb", "wrong char_objects source")
    require(
        manifest["char_objects_rule"]["multiplayer_animate"]
        == "reset_hair, then play ui_loop | kPlayLast kPlayGraphLoop",
        "wrong multiplayer character event rule",
    )
    require(
        manifest["char_objects_rule"]["select"] == "play ui_loop | kPlayLast kPlayGraphLoop",
        "wrong select character event rule",
    )
    require("ui_enter" in manifest["char_objects_rule"]["single_player_animate"], "single-player caveat missing")
    placers = manifest["placers"]
    require(set(placers) == {"char_multi0.placer", "char_multi1.placer"}, "wrong 2P placers")
    require(placers["char_multi0.placer"]["matrix0"][9] == -35.0, "P1 placer X should be -35")
    require(placers["char_multi1.placer"]["matrix0"][9] == 35.0, "P2 placer X should be +35")
    require(placers["char_multi0.placer"]["target_mesh"] == "spot_ui.mesh", "P1 target mesh missing")
    require(placers["char_multi1.placer"]["target_mesh"] == "spot_ui.mesh", "P2 target mesh missing")
    capture = manifest["proof_capture"]
    require(capture["diagnostic_option"] == "--char-2p-select-placer", "2P placer option missing")
    require(capture["diagnostic_event_option"] == "--char-2p-select-event animate", "2P animate event option missing")
    require(capture["event"] == "animate", "proof capture should use the 2P animate event")
    require("animate" in manifest["proof_event_rationale"], "2P animate event rationale missing")
    require(capture["reference_base"] == "live toe-row floor when toe bones exist", "live proof base missing")
    return manifest


def check_case(root: Path, case: ProofCase) -> tuple[str, CaseMetrics]:
    proof_root = root / "engine/out/visual_proofs/lower_body_2p_select_animate_20260715"
    log_path = proof_root / case.log_name
    png_path = proof_root / case.png_name
    require(log_path.is_file(), f"{case.character} frame {case.frame}: missing log")
    digest = require_png(png_path)

    text = read_log_text(log_path)
    require_text(text, f"[char] {case.source_model}:", log_path, "UI source model")
    require_text(text, f"[char] loaded '{case.character}'", log_path, "character load")
    require_text(text, f"[clip] 'ui_loop' from {case.source_clip}", log_path, "UI loop source clip")
    require_text(text, f"[char] clip-frame override enabled: {case.frame}", log_path, "frame override")
    require_text(text, "[char] reference base enabled", log_path, "reference base")
    require_text(text, "[char] screenshot ->", log_path, "screenshot marker")
    require_text(
        text,
        f"[2p-select] applied_placer={case.placer} player={case.player} matrix=matrix0",
        log_path,
        "applied 2P placer",
    )
    require_text(text, "source=ui/gen/multi_sel_character.milo_ps2", log_path, "2P placer source")
    require_text(text, "owner=multi_sel_character_panel target=spot_ui.mesh", log_path, "2P placer target")
    require_text(text, "[2p-select] script=ui/gen/multiplayer.dtb", log_path, "2P script evidence")
    require_text(text, "screen=multi_sel_character_screen", log_path, "2P screen evidence")
    require_text(text, "panel=char_multi event=animate", log_path, "2P char event")
    require_text(text, "clip=ui_loop skips_ui_enter=true", log_path, "2P clip rule")
    require_text(text, "char_objects=char/gen/char_objects.dtb", log_path, "2P char_objects source")
    require_text(text, "reset_hair=true", log_path, "2P animate reset-hair rule")
    require_text(text, "placers=char_multi0.placer,char_multi1.placer", log_path, "2P placers")
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
        knee_z = visible_world(f"bone_{side}-knee")[2]
        toe = toe_z[side]
        require(
            knee_z >= toe + MIN_CHAIN_Z_DROP,
            f"{log_path}: {side} knee is not above toe enough",
        )

    max_abs_toe_z = max(abs(value) for value in toe_z.values())
    toe_delta_z = abs(toe_z["L"] - toe_z["R"])
    min_pelvis_to_toe_z = min(pelvis_z - value for value in toe_z.values())
    require(max_abs_toe_z <= MAX_FLAT_TOE_ABS_Z, f"{log_path}: toe height too high")
    require(toe_delta_z <= MAX_LEFT_RIGHT_TOE_DELTA_Z, f"{log_path}: toe delta too high")
    require(
        min_pelvis_to_toe_z >= MIN_PELVIS_TO_TOE_Z,
        f"{log_path}: pelvis/toe gap too small",
    )
    require(
        max_output_visible_gap <= 0.001,
        f"{log_path}: output/visible gap {max_output_visible_gap:.6f} > 0.001",
    )
    return digest, CaseMetrics(
        max_abs_toe_z=max_abs_toe_z,
        toe_delta_z=toe_delta_z,
        min_pelvis_to_toe_z=min_pelvis_to_toe_z,
        max_output_visible_gap=max_output_visible_gap,
    )


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    try:
        check_manifest(root)
        by_character: dict[str, set[str]] = {}
        metrics: list[CaseMetrics] = []
        for case in PROOF_CASES:
            digest, case_metrics = check_case(root, case)
            by_character.setdefault(case.character, set()).add(digest)
            metrics.append(case_metrics)
        for character, digests in by_character.items():
            require(len(digests) == 2, f"{character}: frame 30/40 proof PNGs are identical")
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(
        "PASS lower_body_2p_select_proofs "
        "characters=glam1,metal1 "
        "screen=multi_sel_character_screen panel=char_multi "
        "event=animate multiplayer_clip=ui_loop skips_ui_enter=true "
        "char_objects=char/gen/char_objects.dtb "
        "placers=char_multi0.placer,char_multi1.placer "
        "applied_placers=char_multi0.placer,char_multi1.placer "
        "frames=30,40 cases=4 individual_proofs=true "
        "two_player_select=true live_reference_base=true "
        f"max_abs_toe_z={max(item.max_abs_toe_z for item in metrics):.4f} "
        f"max_lr_toe_delta_z={max(item.toe_delta_z for item in metrics):.4f} "
        f"min_pelvis_to_toe_z={min(item.min_pelvis_to_toe_z for item in metrics):.4f} "
        f"max_output_visible_gap={max(item.max_output_visible_gap for item in metrics):.6f}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
