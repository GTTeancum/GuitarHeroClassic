#!/usr/bin/env python3
"""Temporarily deploy analysis Midori MILOs to loose DLC, capture, then restore."""

from __future__ import annotations

import argparse
import ctypes
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import NamedTuple

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MILO_TOOL = ROOT / (
    "GuitarHeroOGX-main-ui-engine/tools/milo_convert/out/build/"
    "win-amd64-release/Release/milo_convert_tool.exe"
)

PAIRS = (
    (
        "gh3_midori_main.milo_ps2",
        "community.gh3.midori/content/char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2",
    ),
    (
        "gh3_midori_fret.milo_ps2",
        "community.gh3.midori/content/char/gh3_midori/anims/gen/gh3_midori_fret.milo_ps2",
    ),
    (
        "gh3_midori_strum.milo_ps2",
        "community.gh3.midori/content/char/gh3_midori/anims/gen/gh3_midori_strum.milo_ps2",
    ),
    (
        "gh3_midori_ui.milo_ps2",
        "community.gh3.midori/content/char/gh3_midori/anims/gen/gh3_midori_ui.milo_ps2",
    ),
    (
        "gh3_midori_1.milo_ps2",
        "community.gh3.midori/content/char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
    ),
    (
        "gh3_midori_2.milo_ps2",
        "community.gh3.midori/content/char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2",
    ),
)


def run_text(command: list[str]) -> str:
    completed = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return completed.stdout


def set_low_priority() -> None:
    try:
        ctypes.windll.kernel32.SetPriorityClass(
            ctypes.windll.kernel32.GetCurrentProcess(),
            getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        )
    except Exception:
        pass


def parse_character_parent(text: str, bone: str) -> str | None:
    pattern = re.compile(r"Trans\s+(\S+)\s+parent=([^\s]*)")
    for line in text.splitlines():
        match = pattern.search(line)
        if match and match.group(1) == bone:
            return match.group(2)
    return None


def parse_clipset_parent(text: str, bone: str) -> str | None:
    pattern = re.compile(r"bone\s+(\S+)\s+parent=([^\s]*)")
    for line in text.splitlines():
        match = pattern.search(line)
        if match and match.group(1) == bone:
            return match.group(2)
    return None


def control_root_contract_failures(
    character_texts: dict[str, str],
    clipset_text: str,
) -> list[str]:
    failures: list[str] = []
    clip_parent = parse_clipset_parent(clipset_text, "bone_pelvis.mesh")
    if clip_parent != "Control_Root":
        return failures
    for label, character_text in character_texts.items():
        character_parent = parse_character_parent(character_text, "bone_pelvis.mesh")
        if character_parent != "Control_Root":
            failures.append(
                f"{label}: bone_pelvis.mesh character parent {character_parent!r} "
                "does not match clipset parent 'Control_Root'"
            )
    return failures


def verify_control_root_contract(candidate: Path, tool: Path) -> None:
    clipset_text = run_text(
        [
            str(tool),
            "inspect-clipset",
            str(candidate / "gh3_midori_main.milo_ps2"),
            "--channels",
        ]
    )
    character_texts = {
        name: run_text(
            [
                str(tool),
                "inspect-character",
                str(candidate / name),
                "--transforms",
            ]
        )
        for name in ("gh3_midori_1.milo_ps2", "gh3_midori_2.milo_ps2")
    }
    failures = control_root_contract_failures(character_texts, clipset_text)
    if failures:
        joined = "\n  - ".join(failures)
        raise RuntimeError(
            "candidate model/clipset Control_Root contract mismatch:\n  - "
            + joined
        )


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


class DeployedFile(NamedTuple):
    dest: Path
    backup: Path
    existed: bool


def parse_extra_candidate_file(text: str) -> tuple[str, Path]:
    if "=" not in text:
        raise argparse.ArgumentTypeError(
            "--extra-candidate-file must be SOURCE_NAME=DLC_REL_PATH"
        )
    source_name, dlc_rel_text = text.split("=", 1)
    if not source_name or not dlc_rel_text:
        raise argparse.ArgumentTypeError(
            "--extra-candidate-file must be SOURCE_NAME=DLC_REL_PATH"
        )
    dlc_rel = Path(dlc_rel_text)
    if dlc_rel.is_absolute() or ".." in dlc_rel.parts:
        raise argparse.ArgumentTypeError("DLC_REL_PATH must stay inside the addons dir")
    return source_name, dlc_rel


def main() -> int:
    set_low_priority()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, default=Path("analysis/gh3_midori_gh2_milos"))
    parser.add_argument("--ark-dir", type=Path, default=Path("gh2_ps2_hybrid_assets/GEN"))
    parser.add_argument("--addons-dir", type=Path, default=Path("gh2_ps2_hybrid_assets/DLC"))
    parser.add_argument("--proof-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case-name", action="append", default=[])
    parser.add_argument("--cases-json", type=Path)
    parser.add_argument("--camera-distance", type=float)
    parser.add_argument("--char-offset-z", type=float)
    parser.add_argument("--screenshot-frame", type=int)
    parser.add_argument("--pose-mesh-dump-dir", type=Path)
    parser.add_argument("--milo-tool", type=Path, default=DEFAULT_MILO_TOOL)
    parser.add_argument(
        "--skip-control-root-contract-check",
        action="store_true",
        help="Deploy without checking that model and main clipset agree on the pelvis parent.",
    )
    parser.add_argument(
        "--extra-candidate-file",
        action="append",
        type=parse_extra_candidate_file,
        default=[],
        metavar="SOURCE_NAME=DLC_REL_PATH",
        help=(
            "Temporarily copy an additional file from --candidate into the loose "
            "DLC tree. Destination is relative to --addons-dir and is deleted "
            "again if it did not exist before capture."
        ),
    )
    parser.add_argument("--print-summary", action="store_true")
    args = parser.parse_args()

    run_kwargs = {}
    if hasattr(subprocess, "IDLE_PRIORITY_CLASS"):
        run_kwargs["creationflags"] = subprocess.IDLE_PRIORITY_CLASS

    with tempfile.TemporaryDirectory(prefix="gh3_midori_dlc_backup_") as backup_root_text:
        backup_root = Path(backup_root_text)
        deployed: list[DeployedFile] = []
        try:
            if not args.skip_control_root_contract_check:
                verify_control_root_contract(args.candidate, args.milo_tool)
            deploy_pairs = list(PAIRS) + [
                (source_name, dlc_rel.as_posix())
                for source_name, dlc_rel in args.extra_candidate_file
            ]
            for index, (source_name, dlc_rel) in enumerate(deploy_pairs):
                source = args.candidate / source_name
                dest = args.addons_dir / dlc_rel
                backup = backup_root / f"{index}_{Path(source_name).name}"
                if not source.is_file():
                    raise FileNotFoundError(source)
                if dest.is_file():
                    copy_file(dest, backup)
                    existed = True
                elif index < len(PAIRS):
                    raise FileNotFoundError(dest)
                else:
                    existed = False
                copy_file(source, dest)
                deployed.append(DeployedFile(dest=dest, backup=backup, existed=existed))

            command = [
                sys.executable,
                "-B",
                str(Path("tools/gh3_midori_pose_review.py")),
                "--ark-dir",
                str(args.ark_dir),
                "--addons-dir",
                str(args.addons_dir),
                "--proof-dir",
                str(args.proof_dir),
                "--output",
                str(args.output),
                "--capture",
                "--low-priority",
            ]
            for case_name in args.case_name:
                command.extend(["--case-name", case_name])
            if args.cases_json is not None:
                command.extend(["--cases-json", str(args.cases_json)])
            if args.camera_distance is not None:
                command.extend(["--camera-distance", str(args.camera_distance)])
            if args.char_offset_z is not None:
                command.extend(["--char-offset-z", str(args.char_offset_z)])
            if args.screenshot_frame is not None:
                command.extend(["--screenshot-frame", str(args.screenshot_frame)])
            if args.pose_mesh_dump_dir is not None:
                command.extend(["--pose-mesh-dump-dir", str(args.pose_mesh_dump_dir)])
            if args.print_summary:
                command.append("--print-summary")
            completed = subprocess.run(command, **run_kwargs)
            return int(completed.returncode)
        finally:
            for item in reversed(deployed):
                if item.existed and item.backup.is_file():
                    copy_file(item.backup, item.dest)
                elif not item.existed and item.dest.is_file():
                    item.dest.unlink()


if __name__ == "__main__":
    raise SystemExit(main())
