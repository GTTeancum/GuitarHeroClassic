#!/usr/bin/env python3
"""Audit the current lower-body slice status without claiming full goal completion.

This checker ties together the source-backed lower-body evidence, the active
Glam1/Metal1 proof subject, stock coverage, and the still-fenced pose publisher
bodies. It should pass only when the leg slice has a documented root cause and
bounded native bridge while the wider character-model source gap remains active.
"""

from __future__ import annotations

import json
import re
import struct
import subprocess
import sys
from pathlib import Path


EXPECTED_FENCED = {
    "CharBones::ScaleAdd(CharBones&,float)",
    "CharBonesSamples::EvaluateChannel",
    "CharBonesMeshes::PoseMeshes",
    "CharClipSamples::ScaleAdd",
    "CharClipDriver::Evaluate",
}

LOWER_BODY_BONES = {
    "bone_pelvis",
    "bone_L-thigh",
    "bone_L-knee",
    "bone_L-ankle",
    "bone_L-toe",
    "bone_R-thigh",
    "bone_R-knee",
    "bone_R-ankle",
    "bone_R-toe",
}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
SCREENSHOT_MARKERS = ("screenshot saved", "screenshot ->", "saved screenshot")
CONTACT_SHEET_TOKENS = ("contact", "sheet", "montage", "overview")

ACTIVE_PROOF_ARTIFACTS = (
    (
        "glam1_direct_front",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/glam1_front.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/glam1_front.log",
        ("[char] loaded 'glam1'", "[char] reference base enabled", "t=lower-output"),
    ),
    (
        "glam1_direct_side",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/glam1_side.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/glam1_side.log",
        ("[char] loaded 'glam1'", "[char] reference base enabled", "t=lower-output"),
    ),
    (
        "metal1_direct_front",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/metal1_front.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/metal1_front.log",
        ("[char] loaded 'metal1'", "[char] reference base enabled", "t=lower-output"),
    ),
    (
        "metal1_direct_side",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/metal1_side.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_20260715/metal1_side.log",
        ("[char] loaded 'metal1'", "[char] reference base enabled", "t=lower-output"),
    ),
    (
        "glam1_ingame",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/ingame_glam1_t060_flr_near_rt01.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/ingame_glam1_t060_flr_near_rt01.log",
        (
            "[diagnostic-highway] hidden mode=over_scene",
            "[diagnostic-hud] GHOGX_HIDE_HUD active; skipping HUD draw",
            "source_publisher=fenced",
            "layers_used=0:stand_fast_03",
        ),
    ),
    (
        "glam1_ingame_viewer",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/viewer_glam1_live_stack.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/viewer_glam1_live_stack.log",
        (
            "[char] viewer stack main: prev=stand_fast_02 current=stand_fast_03",
            "[char] reference base enabled",
            "[char] midi fret target: spot_neck_fret11.mesh",
            "source_publisher=fenced",
        ),
    ),
    (
        "metal1_ingame",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/ingame_metal1_t060_flr_near_rt01.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/ingame_metal1_t060_flr_near_rt01.log",
        (
            "[diagnostic-highway] hidden mode=over_scene",
            "[diagnostic-hud] GHOGX_HIDE_HUD active; skipping HUD draw",
            "source_publisher=fenced",
            "layers_used=0:stand_fast_04",
        ),
    ),
    (
        "metal1_ingame_viewer",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/viewer_metal1_live_stack.png",
        "engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715/viewer_metal1_live_stack.log",
        (
            "[char] viewer stack main: prev=stand_fast_03 current=stand_fast_04",
            "[char] reference base enabled",
            "[char] midi fret target: spot_neck_fret11.mesh",
            "source_publisher=fenced",
        ),
    ),
)

REQUIRED_FILES = (
    "tools/check_lower_body_root_cause.py",
    "tools/check_lower_body_bridge_boundary.py",
    "tools/check_lower_body_shared_path.py",
    "tools/check_lower_body_source_row_authority.py",
    "tools/check_lower_body_stock_coverage.py",
    "tools/check_lower_body_metal1_followup_proofs.py",
    "tools/check_lower_body_glam1_metal1_ingame_proofs.py",
    "tools/check_lower_body_metal1_ui_select_proofs.py",
    "tools/check_lower_body_active_ui_select_proofs.py",
    "tools/check_lower_body_2p_select_proofs.py",
    "tools/check_lower_body_2p_select_slot_sweep.py",
    "tools/check_lower_body_2p_select_context_proofs.py",
    "tools/check_lower_body_2p_select_family_proofs.py",
    "tools/check_lower_body_2p_select_source_assets.py",
    "tools/check_lower_body_2p_select_char_events.py",
    "tools/check_lower_body_2p_select_app_placer.py",
    "tools/lower_body_2p_select_source_manifest.json",
    "tools/check_lower_body_pcsx2_row_trace.py",
    "tools/lower_body_pcsx2_row_trace_manifest.json",
    "tools/check_lower_body_rexglue_trace_manifest.py",
    "tools/lower_body_rexglue_trace_manifest.json",
    "tools/check_pose_publisher_source_gaps.py",
    "tools/arm_pose_diff_manifest.json",
    "tools/charbone_output_map_manifest.json",
    "tools/lower_body_glam1_metal1_ingame_pose_manifest.json",
    "tools/pose_publisher_source_gap_manifest.json",
    "engine/src/character/IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md",
)


def compact(text: str) -> str:
    return re.sub(r"\s+", "", text)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def load_json(path: Path) -> dict:
    try:
        payload = json.loads(read(path))
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(payload, dict):
        raise RuntimeError(f"{path}: JSON root must be an object")
    return payload


def detect_text_encoding(path: Path) -> str:
    with path.open("rb") as in_file:
        marker = in_file.read(4)
    if marker.startswith(b"\xff\xfe") or marker.startswith(b"\xfe\xff"):
        return "utf-16"
    if marker.startswith(b"\xef\xbb\xbf"):
        return "utf-8-sig"
    return "utf-8"


def read_log(path: Path) -> str:
    return path.read_text(encoding=detect_text_encoding(path), errors="replace")


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as in_file:
        header = in_file.read(24)
    require(header.startswith(PNG_SIGNATURE), f"{path}: not a PNG")
    require(len(header) >= 24, f"{path}: truncated PNG header")
    width, height = struct.unpack(">II", header[16:24])
    return width, height


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def require_contains(text: str, needle: str, label: str) -> None:
    require(needle in text, f"missing {label}: {needle}")


def require_compact_contains(text: str, needle: str, label: str) -> None:
    require_contains(compact(text), compact(needle), label)


def check_png(path: Path, label: str) -> None:
    require(path.is_file(), f"{label}: missing PNG {path}")
    lower_name = path.name.lower()
    require(
        not any(token in lower_name for token in CONTACT_SHEET_TOKENS),
        f"{label}: active proof must be an individual frame, not a contact sheet: {path}",
    )
    width, height = png_dimensions(path)
    require(
        width >= MIN_PROOF_WIDTH and height >= MIN_PROOF_HEIGHT,
        f"{label}: proof PNG too small {width}x{height}",
    )


def check_log(path: Path, label: str, required_markers: tuple[str, ...]) -> None:
    require(path.is_file(), f"{label}: missing log {path}")
    text = read_log(path)
    require(
        any(marker in text for marker in SCREENSHOT_MARKERS),
        f"{label}: missing screenshot marker in {path}",
    )
    for marker in required_markers:
        require_contains(text, marker, f"{label} log marker")


def run_checker(root: Path, script: str, label: str) -> str:
    result = subprocess.run(
        [sys.executable, str(root / script)],
        cwd=root,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    require(result.returncode == 0, f"{label} failed:\n{result.stdout}")
    return result.stdout


def check_required_files(root: Path) -> None:
    missing = [path for path in REQUIRED_FILES if not (root / path).is_file()]
    require(not missing, f"missing lower-body audit files: {missing}")


def check_root_cause(root: Path, doc: str) -> None:
    tool = read(root / "tools/check_lower_body_root_cause.py")
    require_contains(doc, 'Current lower-body root-cause summary:', "root-cause heading")
    require_contains(doc, 'standing but floating with legs\nforward', "floating-leg symptom")
    require_contains(doc, "bad_max_abs_xyz=8.310", "bad max drift metric")
    require_contains(
        doc,
        "bad_proximal_max_abs_xyz=0.0005",
        "bad proximal sanity metric",
    )
    require_contains(doc, "bad_distal_max_abs_xyz=8.310", "bad distal drift metric")
    require_contains(doc, "fixed_max_abs_xyz=0.001", "fixed max drift metric")
    require_compact_contains(
        doc,
        "not a character-specific\noffset, camera correction, animation-name rule, "
        "foot-IK guess, or broad\nbody/face/arm/hair writeback",
        "rejection of fabricated fixes",
    )
    require_contains(tool, "BAD_FORWARD_Y_MIN", "forward drift threshold")
    require_contains(tool, "BAD_TOE_Z_MIN", "toe height threshold")
    require_contains(tool, "PROXIMAL_BONES", "proximal lower-body split")
    require_contains(tool, "DISTAL_BONES", "distal lower-body split")
    require_contains(tool, "max_bad_proximal_gap", "proximal gap threshold")
    require_contains(tool, "min_bad_distal_max", "distal gap threshold")
    require_contains(
        tool,
        "proximal_rows=aligned distal_rows=drifted",
        "proximal/distal root-cause status",
    )
    require_contains(
        tool,
        "bad_rows=driven_live0 fixed_rows=driven_live1",
        "live row transition status",
    )


def check_source_bridge(root: Path, doc: str) -> None:
    boundary = read(root / "tools/check_lower_body_bridge_boundary.py")
    shared = read(root / "tools/check_lower_body_shared_path.py")
    authority = read(root / "tools/check_lower_body_source_row_authority.py")
    authority_output = run_checker(
        root,
        "tools/check_lower_body_source_row_authority.py",
        "lower-body source row authority checker",
    )
    require_contains(
        boundary,
        "FORBIDDEN_BRIDGE_TOKENS",
        "forbidden shortcut-token guard",
    )
    for marker in (
        "source_output_subset=true",
        "single_output_publisher=true",
        "no_direct_bone_write=true",
        "no_named_or_offset_shortcut=true",
    ):
        require_contains(boundary, marker, f"bridge boundary marker {marker}")
        require_contains(doc, marker, f"documented bridge boundary marker {marker}")
    for marker in (
        "viewer_shared=true",
        "gameplay_shared=true",
        "fallback_call_sites=4",
        "local_filters=hand_overlay_only",
    ):
        require_contains(shared, marker, f"shared path marker {marker}")
        require_contains(doc, marker, f"documented shared path marker {marker}")
    for marker in (
        "charbone_stuffbones=true",
        "charclip_posemeshes=true",
        "sample_split=true",
        "native_output_subset=true",
        "no_shortcut_fix=true",
        "source_dir=",
    ):
        require_contains(authority, marker, f"source-row authority marker {marker}")
        require_contains(
            authority_output,
            marker,
            f"source-row authority output marker {marker}",
        )
    require_compact_contains(
        doc,
        "rebuilds only the decoded facing/pelvis/thigh/knee/ankle/foot/toe output rows\n"
        "from the clip's authored `*.trans` output graph",
        "bounded lower-body output graph bridge",
    )


def check_visual_and_stock_coverage(root: Path, doc: str) -> None:
    stock = read(root / "tools/check_lower_body_stock_coverage.py")
    stock_output = run_checker(
        root,
        "tools/check_lower_body_stock_coverage.py",
        "lower-body stock coverage checker",
    )
    followup = read(root / "tools/check_lower_body_metal1_followup_proofs.py")
    ingame = read(root / "tools/check_lower_body_glam1_metal1_ingame_proofs.py")
    ui_select = read(root / "tools/check_lower_body_metal1_ui_select_proofs.py")
    ui_select_output = run_checker(
        root,
        "tools/check_lower_body_metal1_ui_select_proofs.py",
        "Metal1 UI/select flat-foot proof checker",
    )
    active_ui_select = read(root / "tools/check_lower_body_active_ui_select_proofs.py")
    active_ui_select_output = run_checker(
        root,
        "tools/check_lower_body_active_ui_select_proofs.py",
        "active Glam1/Metal1 UI/select proof checker",
    )
    two_player_select = read(root / "tools/check_lower_body_2p_select_proofs.py")
    two_player_select_slot_sweep = read(
        root / "tools/check_lower_body_2p_select_slot_sweep.py"
    )
    two_player_select_context = read(
        root / "tools/check_lower_body_2p_select_context_proofs.py"
    )
    two_player_select_family = read(
        root / "tools/check_lower_body_2p_select_family_proofs.py"
    )
    two_player_select_assets = read(root / "tools/check_lower_body_2p_select_source_assets.py")
    two_player_select_events = read(root / "tools/check_lower_body_2p_select_char_events.py")
    two_player_select_app = read(root / "tools/check_lower_body_2p_select_app_placer.py")
    two_player_select_manifest = read(root / "tools/lower_body_2p_select_source_manifest.json")
    two_player_select_assets_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_source_assets.py",
        "two-player character-select stock asset checker",
    )
    two_player_select_events_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_char_events.py",
        "two-player character-select stock char event checker",
    )
    two_player_select_app_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_app_placer.py",
        "two-player character-select app placer checker",
    )
    two_player_select_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_proofs.py",
        "two-player character-select proof checker",
    )
    two_player_select_slot_sweep_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_slot_sweep.py",
        "two-player character-select slot-sweep proof checker",
    )
    two_player_select_context_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_context_proofs.py",
        "two-player character-select context proof checker",
    )
    two_player_select_family_output = run_checker(
        root,
        "tools/check_lower_body_2p_select_family_proofs.py",
        "two-player character-select source-family proof checker",
    )
    pose_manifest = load_json(root / "tools/lower_body_glam1_metal1_ingame_pose_manifest.json")
    output_manifest = load_json(root / "tools/charbone_output_map_manifest.json")
    arm_manifest = load_json(root / "tools/arm_pose_diff_manifest.json")

    for label, png_path, log_path, markers in ACTIVE_PROOF_ARTIFACTS:
        check_png(root / png_path, label)
        check_log(root / log_path, label, markers)

    require_contains(stock, "PLAYABLE_INGAME_LABELS", "playable coverage table")
    require_contains(stock, "SUPPORT_VIEWER_LABELS", "support coverage table")
    for marker in (
        "PASS lower_body_stock_coverage",
        "CONTACT_SHEET_TOKENS",
        "individual_proofs=true",
        "linked_proof_pngs=true",
        "playable_leg_chain_sane=true",
        "support_distal_chain_sane=true",
        "MAX_PLAYABLE_LOWEST_TOE_Z = 1.0",
        "MIN_PLAYABLE_PELVIS_TO_LOWEST_TOE_Z = 30.0",
        "MAX_SUPPORT_TOE_Z = 4.0",
        "MIN_SUPPORT_DISTAL_Z_DROP = 10.0",
    ):
        require_contains(stock, marker, f"stock coverage marker {marker}")
    for marker in (
        "PASS lower_body_stock_coverage",
        "playable_ingame=18 support_viewer=6 stock_total=24",
        "individual_proofs=true linked_proof_pngs=true",
        "playable_leg_chain_sane=true",
        "support_distal_chain_sane=true",
    ):
        require_contains(stock_output, marker, f"stock coverage output marker {marker}")
    for marker in (
        "lower_body_glam1_metal1_20260715",
        "standing_chain_sane=true",
        "live_output_rows=true direct_source_publisher_absent=true",
    ):
        require_contains(followup, marker, f"Glam1/Metal1 follow-up marker {marker}")
    for marker in (
        'character="glam1"',
        'character="metal1"',
        'glam1_front.png',
        'glam1_side.png',
        'metal1_front.png',
        'metal1_side.png',
    ):
        require_contains(followup, marker, f"Glam1/Metal1 follow-up case marker {marker}")
    for marker in (
        "lower_body_glam1_metal1_ingame_20260715",
        "hud_hidden=true highway_hidden=true lower_body_rows=true",
        "source_publisher_fenced=true",
    ):
        require_contains(ingame, marker, f"Glam1/Metal1 in-game marker {marker}")
    for marker in (
        'character="glam1"',
        'character="metal1"',
        'ingame_glam1_t060_flr_near_rt01.png',
        'viewer_glam1_live_stack.png',
        'ingame_metal1_t060_flr_near_rt01.png',
        'viewer_metal1_live_stack.png',
    ):
        require_contains(ingame, marker, f"Glam1/Metal1 in-game case marker {marker}")
    for marker in (
        "lower_body_metal1_ui_select_20260715",
        "source_model=metal1_ui source_clip=ui_loop",
        "ui_select_flat_foot=true",
        "max_abs_toe_z=",
        "max_lr_toe_delta_z=",
    ):
        require_contains(ui_select, marker, f"Metal1 UI/select proof marker {marker}")
    for marker in (
        "PASS lower_body_metal1_ui_select_proofs",
        "character=metal1 source_model=metal1_ui source_clip=ui_loop",
        "ui_select_flat_foot=true",
        "max_abs_toe_z=0.2227",
        "max_lr_toe_delta_z=0.2643",
    ):
        require_contains(ui_select_output, marker, f"Metal1 UI/select output marker {marker}")
    for marker in (
        "lower_body_active_ui_select_20260715",
        "frames=30,40",
        "source_models=glam1_ui,metal1_ui",
        "active_ui_select_flat_foot=true",
        "glam1_ui_loop_f030_front.png",
        "glam1_ui_loop_f030_side.png",
        "glam1_ui_loop_f040_front.png",
        "glam1_ui_loop_f040_side.png",
        "metal1_ui_loop_f030_front.png",
        "metal1_ui_loop_f030_side.png",
        "metal1_ui_loop_f040_front.png",
        "metal1_ui_loop_f040_side.png",
    ):
        require_contains(active_ui_select, marker, f"active UI/select proof marker {marker}")
    for marker in (
        "PASS lower_body_active_ui_select_proofs",
        "characters=glam1,metal1",
        "source_models=glam1_ui,metal1_ui source_clip=ui_loop frames=30,40",
        "active_ui_select_flat_foot=true",
        "cases=8",
        "max_abs_toe_z=0.4526",
        "max_lr_toe_delta_z=0.5101",
    ):
        require_contains(active_ui_select_output, marker, f"active UI/select output marker {marker}")
    for marker in (
        "lower_body_2p_select_animate_20260715",
        "multi_sel_character_screen",
        "panel=char_multi",
        "event=animate",
        "skips_ui_enter=true",
        "char_objects=char/gen/char_objects.dtb",
        "reset_hair=true",
        "char_multi0.placer",
        "char_multi1.placer",
        "glam1_2p_animate_ui_loop_f030_front.png",
        "glam1_2p_animate_ui_loop_f040_front.png",
        "metal1_2p_animate_ui_loop_f030_front.png",
        "metal1_2p_animate_ui_loop_f040_front.png",
    ):
        require_contains(two_player_select, marker, f"2P select proof marker {marker}")
    for marker in (
        "\"screen_script\": \"ui/gen/multiplayer.dtb\"",
        "\"screen\": \"multi_sel_character_screen\"",
        "\"scroll_event\": \"{char_multi char_event $playerNum animate}\"",
        "\"outfit_focus_event\": \"{char_multi char_event [player_num] select}\"",
        "\"char_objects_source\": \"char/gen/char_objects.dtb\"",
        "\"single_player_animate\": \"reset_hair, then play ui_enter kPlayNoBlend, then play ui_loop | kPlayLast kPlayGraphLoop\"",
        "\"multiplayer_animate\": \"reset_hair, then play ui_loop | kPlayLast kPlayGraphLoop\"",
        "\"select\": \"play ui_loop | kPlayLast kPlayGraphLoop\"",
        "\"target_mesh\": \"spot_ui.mesh\"",
        "\"diagnostic_option\": \"--char-2p-select-placer\"",
        "\"diagnostic_event_option\": \"--char-2p-select-event animate\"",
        "\"event\": \"animate\"",
        "\"proof_event_rationale\": \"The stock 2P character-select screen displays characters through {char_multi char_event $playerNum animate}; the select event is only the outfit focus/load path. Proof captures therefore use animate and must log reset_hair=true.\"",
        "\"reference_base\": \"live toe-row floor when toe bones exist\"",
    ):
        require_contains(two_player_select_manifest, marker, f"2P select manifest marker {marker}")
    for marker in (
        "PASS lower_body_2p_select_proofs",
        "characters=glam1,metal1",
        "screen=multi_sel_character_screen panel=char_multi",
        "event=animate multiplayer_clip=ui_loop skips_ui_enter=true",
        "char_objects=char/gen/char_objects.dtb",
        "placers=char_multi0.placer,char_multi1.placer",
        "applied_placers=char_multi0.placer,char_multi1.placer",
        "frames=30,40",
        "live_reference_base=true",
        "cases=4",
        "two_player_select=true",
        "max_abs_toe_z=0.4526",
        "max_lr_toe_delta_z=0.5101",
    ):
        require_contains(two_player_select_output, marker, f"2P select output marker {marker}")
    for marker in (
        "lower_body_2p_select_slot_animate_20260715",
        "players=p1,p2",
        "side_profile=true",
        "frames=30,40",
        "both_2p_placers=true",
        "glam1_p1_2p_animate_ui_loop_f030_side",
        "glam1_p2_2p_animate_ui_loop_f030_side",
        "metal1_p1_2p_animate_ui_loop_f030_side",
        "metal1_p2_2p_animate_ui_loop_f030_side",
    ):
        require_contains(
            two_player_select_slot_sweep,
            marker,
            f"2P select slot-sweep proof marker {marker}",
        )
    for marker in (
        "PASS lower_body_2p_select_slot_sweep",
        "characters=glam1,metal1 players=p1,p2",
        "screen=multi_sel_character_screen panel=char_multi",
        "event=animate multiplayer_clip=ui_loop skips_ui_enter=true",
        "side_profile=true frames=30,40 cases=8",
        "individual_proofs=true both_2p_placers=true",
        "max_abs_toe_z=0.4526",
        "max_lr_toe_delta_z=0.5101",
    ):
        require_contains(
            two_player_select_slot_sweep_output,
            marker,
            f"2P select slot-sweep output marker {marker}",
        )
    for marker in (
        "lower_body_2p_select_context_animate_20260715",
        "camera_views=front,side",
        "frames=30,40",
        "both_2p_placers=true",
        "no_singleplayer_geometry=true",
        "glam1_p1_2p_animate_ui_loop_f030_front",
        "glam1_p1_2p_animate_ui_loop_f030_side",
        "metal1_p1_2p_animate_ui_loop_f030_front",
        "metal1_p1_2p_animate_ui_loop_f030_side",
    ):
        require_contains(
            two_player_select_context,
            marker,
            f"2P select context proof marker {marker}",
        )
    for marker in (
        "PASS lower_body_2p_select_context_proofs",
        "characters=glam1,metal1 players=p1,p2",
        "screen=multi_sel_character_screen panel=char_multi",
        "event=animate multiplayer_clip=ui_loop skips_ui_enter=true",
        "camera_views=front,side frames=30,40 cases=16",
        "individual_proofs=true both_2p_placers=true no_singleplayer_geometry=true",
        "max_abs_toe_z=0.4526",
        "max_lr_toe_delta_z=0.5101",
    ):
        require_contains(
            two_player_select_context_output,
            marker,
            f"2P select context output marker {marker}",
        )
    for marker in (
        "lower_body_2p_select_family_animate_20260715",
        "characters=rock1,rock2,funk1,deathmetal1",
        "camera_views=side",
        "frames=30,40",
        "source_family_rock2_uses_rock1_ui_anim=true",
        "rock1_p1_2p_animate_ui_loop_f030_side",
        "rock2_p1_2p_animate_ui_loop_f030_side",
        "funk1_p1_2p_animate_ui_loop_f030_side",
        "deathmetal1_p1_2p_animate_ui_loop_f030_side",
    ):
        require_contains(
            two_player_select_family,
            marker,
            f"2P select source-family proof marker {marker}",
        )
    for marker in (
        "PASS lower_body_2p_select_family_proofs",
        "characters=rock1,rock2,funk1,deathmetal1 players=p1,p2",
        "screen=multi_sel_character_screen panel=char_multi",
        "event=animate multiplayer_clip=ui_loop skips_ui_enter=true",
        "camera_views=side frames=30,40 cases=16",
        "source_family_rock2_uses_rock1_ui_anim=true",
        "max_abs_toe_z=1.8758",
        "max_lr_toe_delta_z=1.2907",
        "max_output_visible_gap=0.000500",
    ):
        require_contains(
            two_player_select_family_output,
            marker,
            f"2P select source-family output marker {marker}",
        )
    for marker in (
        "MATRIX0_OFFSET = 46",
        "MATRIX1_OFFSET = 94",
        "STRING_OFFSET = 151",
        "BandPlacer__char_multi0.placer",
        "BandPlacer__char_multi1.placer",
        "target_mesh",
    ):
        require_contains(two_player_select_assets, marker, f"2P select asset checker marker {marker}")
    for marker in (
        "PASS lower_body_2p_select_source_assets",
        "source=stock_GH2_PS2",
        "screen_milo=ui/gen/multi_sel_character.milo_ps2",
        "entries=char_multi0.placer,char_multi1.placer",
        "matrix0_offset=46 matrix1_offset=94",
        "target_group=mgs_camerafix.grp target_mesh=spot_ui.mesh",
    ):
        require_contains(two_player_select_assets_output, marker, f"2P select asset output marker {marker}")
    for marker in (
        "MULTIPLAYER_DTB = \"ui/gen/multiplayer.dtb\"",
        "CHAR_OBJECTS_DTB = \"char/gen/char_objects.dtb\"",
        "char_multichar_event",
        "animate_multiplayer=reset_hair+ui_loop",
        "select=ui_loop",
    ):
        require_contains(two_player_select_events, marker, f"2P select event checker marker {marker}")
    for marker in (
        "PASS lower_body_2p_select_char_events",
        "source=stock_GH2_PS2",
        "screen_script=ui/gen/multiplayer.dtb",
        "char_objects=char/gen/char_objects.dtb",
        "events=animate,select",
        "animate_multiplayer=reset_hair+ui_loop",
        "select=ui_loop",
        "single_player=ui_enter+ui_loop",
    ):
        require_contains(two_player_select_events_output, marker, f"2P select event output marker {marker}")
    for marker in (
        "compact_matrix16_to_source_matrix12",
        "--char-2p-select-placer",
        "--char-2p-select-event",
        "applied_placer=%s",
        "char_multi0.placer",
        "char_multi1.placer",
    ):
        require_contains(two_player_select_app, marker, f"2P select app placer marker {marker}")
    for marker in (
        "PASS lower_body_2p_select_app_placer",
        "players=0,1",
        "app_placers=char_multi0.placer,char_multi1.placer",
        "manifest_matrix=matrix0",
    ):
        require_contains(two_player_select_app_output, marker, f"2P select app placer output marker {marker}")
    cases = pose_manifest.get("cases")
    require(isinstance(cases, list) and len(cases) == 2, "Glam1/Metal1 manifest must have two cases")
    characters = {case.get("character") for case in cases if isinstance(case, dict)}
    require(characters == {"glam1", "metal1"}, f"unexpected active characters: {characters}")
    for case in cases:
        require(set(case.get("bones", [])) == LOWER_BODY_BONES, f"{case.get('label')}: wrong bones")
        require("source_publisher=fenced" in case.get("require_ingame_contains", []), f"{case.get('label')}: missing in-game source fence")
        require("source_publisher=fenced" in case.get("require_viewer_contains", []), f"{case.get('label')}: missing viewer source fence")
    arm_cases = arm_manifest.get("cases", [])
    require(isinstance(arm_cases, list), "arm manifest cases must be a list")
    output_cases = output_manifest.get("cases", [])
    require(isinstance(output_cases, list), "output manifest cases must be a list")
    require(len([c for c in arm_cases if isinstance(c, dict) and "current_lower_body" in str(c.get("label", ""))]) >= 18, "missing playable lower-body manifest coverage")
    require(len([c for c in output_cases if isinstance(c, dict) and "support_lower_body" in str(c.get("label", ""))]) >= 6, "missing support lower-body manifest coverage")
    for marker in (
        "Rockabill is no longer the active quick test subject",
        "New leg/pose proof runs should prefer Glam1 and Metal1",
        "Metal1's visible right shoulder/hand abnormality remains an open arm-follow-up",
        "Metal Drummer also has a visible arm-twist concern",
    ):
        require_contains(doc, marker, f"review-scope note {marker}")


def check_source_boundary(root: Path, doc: str) -> None:
    gaps_tool = read(root / "tools/check_pose_publisher_source_gaps.py")
    rexglue = read(root / "tools/check_lower_body_rexglue_trace_manifest.py")
    pcsx2 = read(root / "tools/check_lower_body_pcsx2_row_trace.py")
    gap_manifest = load_json(root / "tools/pose_publisher_source_gap_manifest.json")
    rexglue_manifest = load_json(root / "tools/lower_body_rexglue_trace_manifest.json")
    pcsx2_manifest = load_json(root / "tools/lower_body_pcsx2_row_trace_manifest.json")
    still_fenced = set(gap_manifest.get("still_fenced", []))
    require(still_fenced == EXPECTED_FENCED, f"unexpected fenced source bodies: {still_fenced}")
    require_contains(gaps_tool, "source-gap-manifest-five-body-fence", "source gap fence check")
    require_contains(gaps_tool, "--require-live-heads", "source gap live-head option")
    require_contains(gaps_tool, "rb3-live-head-fresh", "source gap rb3 live-head check")
    require_contains(gaps_tool, "gltfmilo-live-head-fresh", "source gap glTFMilo live-head check")
    require_contains(gaps_tool, "nested_git_toplevel", "source gap nested git guard")
    require_contains(gaps_tool, "snapshot_commit", "source gap committed snapshot support")
    require_contains(gaps_tool, "default_rb2_dump_char", "source gap committed RB2 dump fallback")
    require_contains(
        json.dumps(gap_manifest),
        "ihatecompvir-extra/rb3-retail-old/doc/rb2_dump",
        "source gap manifest uses committed RB2 dump",
    )
    captures = rexglue_manifest.get("captures")
    require(isinstance(captures, list) and captures, "RexGlue manifest needs captures")
    accepted = [capture for capture in captures if capture.get("accepted_row_oracle")]
    require(not accepted, f"RexGlue captures unexpectedly marked as row oracles: {accepted}")
    require(
        rexglue_manifest.get("runtime_corroboree") == "pcsx2_rock_lower_body_mesh_rows_20260715",
        "RexGlue manifest missing PCSX2 corroboration marker",
    )
    require_contains(rexglue, "accepted_row_oracles={accepted_count}", "RexGlue dynamic non-oracle output")
    require_contains(rexglue, "runtime_corroboree=pcsx2_rock_lower_body_mesh_rows_20260715", "PCSX2 corroboration marker")
    require(
        len(pcsx2_manifest.get("required_moving_desc_rows", [])) == 7,
        "PCSX2 manifest must pin seven moving runtime descriptor rows",
    )
    conclusion = pcsx2_manifest.get("runtime_corroboree_conclusion", {})
    require(
        isinstance(conclusion, dict)
        and conclusion.get("native_path") == "source_output_lower_body_bridge",
        "PCSX2 manifest missing lower-body bridge runtime conclusion",
    )
    require_contains(pcsx2, "runtime_transform_rows={runtime_transform_rows}", "PCSX2 dynamic runtime row output")
    require_contains(pcsx2, "native_path=source_output_lower_body_bridge", "PCSX2 lower-body conclusion")
    require_contains(doc, "broader GH2 character-model goal remains active", "active wider goal status")
    require_contains(doc, "bounded source-data bridge", "bounded source-data bridge status")


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    try:
        check_required_files(root)
        doc = read(root / "engine/src/character/IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md")
        check_root_cause(root, doc)
        check_source_bridge(root, doc)
        check_visual_and_stock_coverage(root, doc)
        check_source_boundary(root, doc)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_completion_audit "
        "root_cause=true source_bridge=true active_subjects=glam1,metal1 "
        f"proof_artifacts={len(ACTIVE_PROOF_ARTIFACTS)} "
        "individual_proofs=true "
        "proof_min_resolution=1280x720 stock_visuals=true "
        "stock_checker_passed=true stock_linked_proof_pngs=true "
        "metal1_ui_select_flat_foot=true active_ui_select_flat_foot=true "
        "two_player_select=true two_player_select_source_assets=true "
        "two_player_select_char_events=true two_player_select_app_placer=true "
        "two_player_select_slot_sweep=true two_player_select_context=true "
        "two_player_select_family=true "
        "source_boundary_active=true goal_active=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
