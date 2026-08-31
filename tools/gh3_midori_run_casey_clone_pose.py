#!/usr/bin/env python3
"""Run one bounded Midori pose proof in GuitarHeroClassic gameplay."""

from __future__ import annotations

import argparse
import ctypes
import os
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_APP = (
    ROOT
    / "engine/out/build/win-amd64-release/src/app/ghogx_app.exe"
)
DEFAULT_ARK_DIR = ROOT / "out/midori/input/GEN"
DEFAULT_PACKAGE = ROOT / "DLC/community.gh3.midori"


def set_idle_priority() -> None:
    os.environ.update(
        {
            "OMP_NUM_THREADS": "1",
            "OPENBLAS_NUM_THREADS": "1",
            "MKL_NUM_THREADS": "1",
            "NUMEXPR_NUM_THREADS": "1",
            "BLIS_NUM_THREADS": "1",
        }
    )
    if os.name == "nt":
        ctypes.windll.kernel32.SetPriorityClass(  # type: ignore[attr-defined]
            ctypes.windll.kernel32.GetCurrentProcess(), 0x40
        )


def require_file(path: Path, label: str) -> Path:
    resolved = path.resolve()
    if not resolved.is_file():
        raise FileNotFoundError(f"{label} is missing: {resolved}")
    return resolved


def clone_environment(
    package: Path,
    clip: str,
    clip_time: float,
    camera_yaw: float,
) -> dict[str, str]:
    env = os.environ.copy()
    env.update(
        {
            "GHOGX_ADDONS_DIR": str(package.parent),
            "GHOGX_ONLY_PERFORMER": "guitarist0",
            "GHOGX_DEBUG_HAND_MAP": "1",
            "GHOGX_DEBUG_HAND_POSE_ROWS": "1",
            "GHOGX_DEBUG_HAND_POSE_ROLE": "guitarist0",
            "GHOGX_DEBUG_HAND_POSE_STRIDE": "0",
            "GHOGX_DIAGNOSTIC_PERFORMER_CLIP": f"guitarist0={clip}",
            "GHOGX_DIAGNOSTIC_PERFORMER_CLIP_TIME": (
                f"guitarist0={clip_time}"
            ),
            "GHOGX_DEBUG_DIAGNOSTIC_CLIP_TIME": "1",
            "GHOGX_DIAGNOSTIC_FRONT_CAMERA_YAW_OFFSET": str(camera_yaw),
            # Preflights do not pass --screenshot, so ghogx_app would otherwise
            # expose its fixed diagnostic pose as an apparently frozen window.
            "GHOGX_HIDE_WINDOW": "1",
            "OMP_NUM_THREADS": "1",
            "OPENBLAS_NUM_THREADS": "1",
            "MKL_NUM_THREADS": "1",
            "NUMEXPR_NUM_THREADS": "1",
            "BLIS_NUM_THREADS": "1",
        }
    )
    return env


def gameplay_summary(text: str) -> dict[str, int | str] | None:
    matches = list(
        re.finditer(
            r"final gameplay summary: state=(\w+) .*?hits=(\d+) "
            r"misses=(\d+)",
            text,
        )
    )
    if not matches:
        return None
    match = matches[-1]
    return {
        "state": match.group(1),
        "hits": int(match.group(2)),
        "misses": int(match.group(3)),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, default=DEFAULT_APP)
    parser.add_argument("--ark-dir", type=Path, default=DEFAULT_ARK_DIR)
    parser.add_argument("--package", type=Path, default=DEFAULT_PACKAGE)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--screenshot", type=Path)
    parser.add_argument("--song", default="shoutatthedevil")
    parser.add_argument("--venue", default="big")
    parser.add_argument("--clip", default="stand_medium_04")
    parser.add_argument("--clip-time", type=float, default=4.766666667)
    parser.add_argument("--camera-yaw", type=float, default=-0.4)
    parser.add_argument("--song-start", type=float, default=30.0)
    parser.add_argument("--capture-frame", type=int, default=30)
    parser.add_argument("--frames", type=int, default=65)
    parser.add_argument("--fps", type=float, default=60.0)
    parser.add_argument("--timeout", type=float, default=120.0)
    args = parser.parse_args()

    set_idle_priority()
    app = require_file(args.app, "ghogx_app")
    ark_dir = args.ark_dir.resolve()
    package = args.package.resolve()
    log = args.log.resolve()
    screenshot = args.screenshot.resolve() if args.screenshot else None
    require_file(ark_dir / "MAIN.HDR", "extracted MAIN.HDR")
    require_file(ark_dir / "MAIN_0.ARK", "extracted MAIN_0.ARK")
    require_file(package / "manifest.json", "loose Midori package manifest")
    if args.capture_frame < 1 or args.frames <= args.capture_frame:
        raise ValueError("capture frame must be inside the bounded run")
    if args.fps <= 0 or args.timeout <= 0:
        raise ValueError("fps and timeout must be positive")

    command = [
        str(app),
        "--ark-dir",
        str(ark_dir),
        "--render-size",
        "1280x720",
        "--song",
        args.song,
        "--difficulty",
        "0",
        "--diagnostic-character-variant",
        "gh3_midori_1",
        "--diagnostic-venue",
        args.venue,
        "--diagnostic-front-camera",
        "guitarist0",
        "--diagnostic-proof-lighting",
        "--diagnostic-autoplay",
        "--auto-start",
        "--diagnostic-song-start",
        f"{args.song_start:.6f}",
        "--fixed-dt",
        f"{1.0 / args.fps:.9f}",
        "--frames",
        str(args.frames),
        "--mute-audio",
    ]
    if screenshot is not None:
        screenshot.parent.mkdir(parents=True, exist_ok=True)
        command.extend(
            [
                "--screenshot",
                str(screenshot),
                "--screenshot-frame",
                str(args.capture_frame),
            ]
        )
    if any(".iso" in item.lower() for item in command):
        raise ValueError("clone pose runner refuses ISO paths")

    env = clone_environment(
        package, args.clip, args.clip_time, args.camera_yaw
    )
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("wb") as handle:
        process = subprocess.Popen(
            command,
            cwd=ROOT,
            env=env,
            stdout=handle,
            stderr=subprocess.STDOUT,
            creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        )
        try:
            returncode = process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired as exc:
            process.kill()
            process.wait(timeout=10)
            raise TimeoutError(
                f"ghogx_app exceeded {args.timeout:.1f}s and was terminated: "
                f"{log}"
            ) from exc
    if returncode != 0:
        raise RuntimeError(f"ghogx_app exited {returncode}: {log}")

    text = log.read_text(encoding="utf-8", errors="replace")
    summary = gameplay_summary(text)
    checks = {
        "current_midori_model": (
            "169 meshes (169 ok / 0 fail, 45 showing), 115 bones" in text
        ),
        "requested_clip": (
            f"diagnostic performer clip override: role=guitarist0 "
            f"char=gh3_midori_1 clip={args.clip}" in text
        ),
        "requested_clip_time": (
            f"requested={args.clip} active={args.clip} "
            f"time={args.clip_time:.3f} forced={args.clip_time:.3f}" in text
        ),
        "requested_venue": f"diagnostic venue override: {args.venue}" in text,
        "playing_with_hits": (
            summary is not None
            and summary["state"] == "playing"
            and int(summary["hits"]) >= 1
            and summary["misses"] == 0
        ),
        "pose_rows": (
            text.count("[handpose] phase=postcontrollers role=guitarist0")
            >= args.capture_frame
        ),
        "hidden_window": "D3D9 window 1280x720 created (hidden)" in text,
        "bounded_exit": f"exited after {args.frames} frames" in text,
        "screenshot_state": (
            screenshot is None
            or (
                screenshot.is_file()
                and f"[ghogx] screenshot saved: {screenshot}" in text
            )
        ),
        "clone_only": ".iso" not in text.lower() and "pcsx2" not in text.lower(),
    }
    failed = [name for name, passed in checks.items() if not passed]
    print(
        "status=%s mode=%s pose_rows=%d hits=%s log=%s"
        % (
            "pass" if not failed else "fail",
            "capture" if screenshot is not None else "preflight",
            text.count("[handpose] phase=postcontrollers role=guitarist0"),
            summary["hits"] if summary is not None else "missing",
            log,
        )
    )
    if failed:
        raise ValueError(f"clone pose checks failed: {failed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
