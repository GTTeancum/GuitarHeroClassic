#!/usr/bin/env python3
"""Capture and verify a broader GH3 Midori native pose review set.

This is a representative visual-framing gate for the ark-external Midori
package. It uses the existing native `ghogx_app --char` viewer and generated
MILO banks; it does not replace PS2/emulator validation or prove every frame of
every source clip.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
from pathlib import Path
from typing import Any

from gh3_midori_render_proof import analyze_bmp, read_log


APP = Path("GuitarHeroOGX-main-ui-engine/engine/out/build/win-amd64-release/src/app/ghogx_app.exe")
ARK_DIR = Path("gh2_ps2_hybrid_assets/gen")
MAIN_MILO = "char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2"
STRUM_MILO = "char/gh3_midori/anims/gen/gh3_midori_strum.milo_ps2"
FRET_MILO = "char/gh3_midori/anims/gen/gh3_midori_fret.milo_ps2"

CASES: tuple[dict[str, Any], ...] = (
    {
        "name": "midori_1_medium_idle_f060",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "gh3_guit_mido_a_med_idle01",
        "frame": 60,
        "category": "main_idle",
        "expected_layers": 1,
    },
    {
        "name": "midori_1_attack_left_f030",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "gh3_guit_mido_a_attackl",
        "frame": 30,
        "category": "main_attack",
        "expected_layers": 1,
    },
    {
        "name": "midori_1_fast_jump_f040",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "gh3_guit_mido_a_fst_jump01",
        "frame": 40,
        "category": "main_jump",
        "expected_layers": 1,
    },
    {
        "name": "midori_1_fast_solo_f090",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "gh3_guit_mido_a_fst_solo01",
        "frame": 90,
        "category": "main_solo",
        "expected_layers": 1,
    },
    {
        "name": "midori_1_transition_out_f050",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "gh3_guit_midori_tran_atoout",
        "frame": 50,
        "category": "main_transition",
        "expected_layers": 1,
        "camera_distance": 150.0,
    },
    {
        "name": "midori_1_accessory_acc01_f030",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "gh3_guitarist_midori_acc01",
        "frame": 30,
        "category": "accessory_fallback",
        "expected_layers": 1,
        "camera_distance": 150.0,
    },
    {
        "name": "midori_1_hand_overlay_f010",
        "selection": "gh3_midori_1",
        "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
        "clip": "stand_medium_01",
        "strum_clip": "gh3_hnd_guit_strum_mido_norm_m01_d",
        "fret_clip": "gh3_hnd_guit_chord_mid_bar3_d",
        "frame": 10,
        "category": "main_strum_fret_overlay",
        "expected_layers": 3,
    },
    {
        "name": "midori_2_medium_idle_f060",
        "selection": "gh3_midori_2",
        "model": "char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2",
        "clip": "gh3_guit_mido_a_med_idle01",
        "frame": 60,
        "category": "second_outfit_idle",
        "expected_layers": 1,
    },
    {
        "name": "midori_2_attack_left_f030",
        "selection": "gh3_midori_2",
        "model": "char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2",
        "clip": "gh3_guit_mido_a_attackl",
        "frame": 30,
        "category": "second_outfit_attack",
        "expected_layers": 1,
    },
)


def rel(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def clip_arg(milo: str, clip: str) -> str:
    return f"{milo}:{clip}"


def clean_capture_dir(proof_dir: Path) -> None:
    proof_dir.mkdir(parents=True, exist_ok=True)
    for path in proof_dir.iterdir():
        if path.is_file() and path.suffix.lower() in {".bmp", ".log", ".json"}:
            path.unlink()


def capture_case(case: dict[str, Any], args: argparse.Namespace, root: Path, proof_dir: Path) -> None:
    bmp = proof_dir / f"{case['name']}.bmp"
    log = proof_dir / f"{case['name']}.log"
    frame = int(case["frame"])
    screenshot_frame = int(args.screenshot_frame) if args.screenshot_frame is not None else frame
    main_milo = str(case.get("main_milo") or MAIN_MILO)
    strum_milo = str(case.get("strum_milo") or STRUM_MILO)
    fret_milo = str(case.get("fret_milo") or FRET_MILO)
    face_milo = str(case.get("face_milo") or main_milo)
    command = [
        str((root / args.app).resolve()),
        "--ark-dir",
        str((root / args.ark_dir).resolve()),
        "--render-size",
        "1280x720",
        "--char",
        case["model"],
        "--clip",
        clip_arg(main_milo, case["clip"]),
        "--clip-frame",
        str(frame),
        "--cam-dist",
        str(max(float(args.camera_distance), float(case.get("camera_distance", args.camera_distance)))),
        "--char-offset",
        "0",
        "0",
        str(args.char_offset_z),
        "--screenshot",
        str(bmp),
        "--screenshot-frame",
        str(screenshot_frame),
        "--frames",
        str(screenshot_frame + 3),
        "--mute-audio",
    ]
    if case.get("strum_clip"):
        command.extend(["--strum-clip", clip_arg(strum_milo, case["strum_clip"])])
    if case.get("fret_clip"):
        command.extend(["--fret-clip", clip_arg(fret_milo, case["fret_clip"])])
    if case.get("face_clip"):
        command.extend(["--face-clip", clip_arg(face_milo, case["face_clip"])])
    if args.pose_mesh_dump_dir:
        dump_dir = root / args.pose_mesh_dump_dir
        dump_dir.mkdir(parents=True, exist_ok=True)
        command.extend(["--char-pose-mesh-dump", str((dump_dir / f"{case['name']}.jsonl").resolve())])
    env = os.environ.copy()
    env["GHOGX_DEBUG_POSE_PUBLISHER"] = "1"
    if args.addons_dir:
        env["GHOGX_ADDONS_DIR"] = str((root / args.addons_dir).resolve())
    if case.get("strum_clip"):
        env["GHOGX_RIGHT_WEIGHT"] = "1"
    if case.get("fret_clip"):
        env["GHOGX_LEFT_WEIGHT"] = "1"
    with log.open("wb") as handle:
        creationflags = (
            subprocess.IDLE_PRIORITY_CLASS
            if args.low_priority and os.name == "nt"
            else 0
        )
        completed = subprocess.run(
            command,
            cwd=root,
            env=env,
            stdout=handle,
            stderr=subprocess.STDOUT,
            creationflags=creationflags,
        )
    if completed.returncode != 0:
        with log.open("ab") as handle:
            handle.write(f"\n[pose-review] command failed: {completed.returncode}\n".encode("utf-8"))


def framing_sane(image: dict[str, Any]) -> bool:
    bounds = image.get("visible_bounds") or {}
    width = int(image.get("width", 0))
    height = int(image.get("height", 0))
    x = int(bounds.get("x", -1))
    y = int(bounds.get("y", -1))
    w = int(bounds.get("width", 0))
    h = int(bounds.get("height", 0))
    ratio = float(image.get("visible_pixel_ratio", 0.0))
    if width != 1280 or height != 720:
        return False
    return (
        100 <= w <= 900
        and 230 <= h <= 700
        and x >= 10
        and y >= 10
        and x + w <= width - 20
        and y + h <= height - 10
        and 0.008 <= ratio <= 0.16
    )


def sampled_bmp_difference_ratio(path: Path, reference: Path) -> float | None:
    if not path.is_file() or not reference.is_file():
        return None
    data = path.read_bytes()
    ref = reference.read_bytes()
    limit = min(len(data), len(ref))
    if limit <= 54:
        return None
    step = max(1, (limit - 54) // 200000)
    total = 0
    changed = 0
    for offset in range(54, limit, step):
        total += 1
        if abs(data[offset] - ref[offset]) > 8:
            changed += 1
    return changed / float(total) if total else None


def reference_idle_name(case: dict[str, Any]) -> str | None:
    if case["name"].endswith("medium_idle_f060"):
        return None
    if case["selection"] == "gh3_midori_2":
        return "midori_2_medium_idle_f060"
    return "midori_1_medium_idle_f060"


def verify_case(case: dict[str, Any], root: Path, proof_dir: Path) -> dict[str, Any]:
    bmp = proof_dir / f"{case['name']}.bmp"
    log = proof_dir / f"{case['name']}.log"
    errors: list[str] = []
    image: dict[str, Any] = {}
    log_text = ""
    if bmp.is_file():
        image = analyze_bmp(bmp)
    else:
        errors.append("missing_bmp")
    if log.is_file():
        log_text = read_log(log)
    else:
        errors.append("missing_log")

    expected_clips = [case["clip"]]
    if case.get("strum_clip"):
        expected_clips.append(case["strum_clip"])
    if case.get("fret_clip"):
        expected_clips.append(case["fret_clip"])
    if case.get("face_clip"):
        expected_clips.append(case["face_clip"])
    reference_name = reference_idle_name(case)
    diff_ratio = (
        sampled_bmp_difference_ratio(bmp, proof_dir / f"{reference_name}.bmp")
        if reference_name
        else None
    )
    checks = {
        "bmp_present": bmp.is_file(),
        "log_present": log.is_file(),
        "dimensions_1280x720": image.get("width") == 1280 and image.get("height") == 720,
        "uncompressed_24_bit": image.get("bit_count") == 24,
        "nonblank_visible_pixels": image.get("visible_pixel_count", 0) > 1000,
        "sampled_color_diversity": image.get("sampled_unique_color_count", 0) > 64,
        "framing_has_margins": framing_sane(image),
        "source_dlc_mounted": "[configdb] DLC catalog:" in log_text,
        "model_loaded": f"[char] loaded '{case['selection']}'" in log_text,
        "textures_uploaded": "[char3d] uploaded 1/1 textures" in log_text,
        "expected_clips_loaded": all(f"[clip] '{clip}' from " in log_text for clip in expected_clips),
        "pose_publisher_layer_count": f"layers={case['expected_layers']}" in log_text,
        "visible_difference_from_idle": (
            diff_ratio is None
            if reference_name is None
            else diff_ratio is not None
            and diff_ratio >= float(case.get("min_idle_diff_ratio", 0.001))
        ),
        "screenshot_written": "[char] screenshot ->" in log_text,
        "no_command_failure": "[pose-review] command failed" not in log_text,
    }
    for name, passed in checks.items():
        if not passed:
            errors.append(name)
    return {
        "name": case["name"],
        "category": case["category"],
        "selection": case["selection"],
        "model": case["model"],
        "main_milo": case.get("main_milo") or MAIN_MILO,
        "main_clip": case["clip"],
        "strum_milo": case.get("strum_milo") or STRUM_MILO if case.get("strum_clip") else None,
        "strum_clip": case.get("strum_clip"),
        "fret_milo": case.get("fret_milo") or FRET_MILO if case.get("fret_clip") else None,
        "fret_clip": case.get("fret_clip"),
        "face_milo": case.get("face_milo") or case.get("main_milo") or MAIN_MILO if case.get("face_clip") else None,
        "face_clip": case.get("face_clip"),
        "frame": case["frame"],
        "idle_reference": reference_name,
        "sampled_idle_difference_ratio": diff_ratio,
        "screenshot": rel(bmp, root),
        "log": rel(log, root),
        "image": image,
        "checks": checks,
        "passed": not errors,
        "errors": errors,
    }


def run(args: argparse.Namespace) -> int:
    root = args.root.resolve()
    proof_dir = (root / args.proof_dir).resolve()
    selected_cases = list(CASES)
    if args.cases_json:
        cases_path = (root / args.cases_json).resolve()
        payload = json.loads(cases_path.read_text(encoding="utf-8"))
        loaded = payload.get("cases") if isinstance(payload, dict) else payload
        if not isinstance(loaded, list):
            raise SystemExit(f"{cases_path}: expected a case list or object with 'cases'")
        selected_cases = list(loaded)
    if args.case_name:
        wanted = set(args.case_name)
        selected_cases = [case for case in selected_cases if case["name"] in wanted]
        missing = sorted(wanted - {case["name"] for case in selected_cases})
        if missing:
            raise SystemExit(f"unknown case name(s): {', '.join(missing)}")
    if args.capture:
        clean_capture_dir(proof_dir)
        for case in selected_cases:
            capture_case(case, args, root, proof_dir)

    proofs = [verify_case(case, root, proof_dir) for case in selected_cases]
    failures = [
        f"{proof['name']}:{error}"
        for proof in proofs
        for error in proof.get("errors", [])
    ]
    result = {
        "format": "gh3_midori_pose_review_proof_v1",
        "status": "native_viewer_representative_pose_framing_review_passed" if not failures else "failed",
        "proof_count": len(proofs),
        "failure_count": len(failures),
        "failures": failures,
        "camera_distance": args.camera_distance,
        "char_offset_z": args.char_offset_z,
        "screenshot_frame": args.screenshot_frame,
        "app": str(args.app),
        "ark_dir": str(args.ark_dir),
        "addons_dir": str(args.addons_dir) if args.addons_dir else None,
        "proofs": proofs,
        "scope": (
            "Representative native viewer review for both Midori outfits and "
            "main idle/attack/jump/solo/transition/accessory plus hand-overlay "
            "animation paths. This checks load, pose-publisher layer count, "
            "nonblank texture-rich screenshots, and 1280x720 framing margins."
        ),
        "remaining_runtime_gaps": [
            "This is representative automated pose/framing evidence, not exhaustive visual review of all 280 source clips.",
            "PS2/emulator runtime loading from the ark-external package is not proven.",
        ],
    }
    output = (root / args.output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    if args.print_summary:
        bounds = [
            proof.get("image", {}).get("visible_bounds", {})
            for proof in proofs
            if proof.get("image")
        ]
        min_margin = "n/a"
        if bounds:
            margins = [
                min(
                    int(b.get("x", 0)),
                    int(b.get("y", 0)),
                    1280 - int(b.get("x", 0)) - int(b.get("width", 0)),
                    720 - int(b.get("y", 0)) - int(b.get("height", 0)),
                )
                for b in bounds
            ]
            min_margin = str(min(margins))
        print(
            "status=%s proofs=%d failures=%d min_margin=%s"
            % (result["status"], result["proof_count"], result["failure_count"], min_margin)
        )
    return 0 if not failures else 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--app", type=Path, default=APP)
    parser.add_argument("--ark-dir", type=Path, default=ARK_DIR)
    parser.add_argument(
        "--proof-dir",
        type=Path,
        default=Path("analysis/gh3_midori_pose_review_proofs"),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("analysis/gh3_midori_pose_review_proofs/pose_review_proof_manifest.json"),
    )
    parser.add_argument(
        "--camera-distance",
        type=float,
        default=130.0,
        help="Fixed native-viewer distance shared with the stock Glam scale proof.",
    )
    parser.add_argument(
        "--char-offset-z",
        type=float,
        default=0.0,
        help="Review-only vertical character offset for screenshot framing.",
    )
    parser.add_argument("--addons-dir", type=Path)
    parser.add_argument(
        "--pose-mesh-dump-dir",
        type=Path,
        help="Write runtime posed mesh bounds JSONL files for captured cases.",
    )
    parser.add_argument(
        "--screenshot-frame",
        type=int,
        default=None,
        help="Render frame used for screenshots; clip-frame still selects the reviewed animation frame.",
    )
    parser.add_argument("--capture", action="store_true")
    parser.add_argument(
        "--case-name",
        action="append",
        default=[],
        help="Limit capture and verification to one named pose-review case. May be repeated.",
    )
    parser.add_argument(
        "--cases-json",
        type=Path,
        help="JSON list of pose-review case objects, or object with a 'cases' list.",
    )
    parser.add_argument(
        "--low-priority",
        action="store_true",
        help="Launch native viewer captures at below-normal process priority on Windows.",
    )
    parser.add_argument("--print-summary", action="store_true")
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
