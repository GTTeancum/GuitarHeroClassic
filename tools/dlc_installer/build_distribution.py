#!/usr/bin/env python3
"""Assemble the trimmed player download and its single-file setup GUI."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import install_dlc


REPO = Path(__file__).resolve().parents[2]
INSTALLER = REPO / "tools/dlc_installer"
RB2_TOOLS = (
    "build_instrument_inventory.py",
    "build_instrument_finish_inventory.py",
    "convert_rb2_instruments.py",
    "build_rb2_dlc_package.py",
    "rb2_native_assets.py",
)
NATIVE_BINARIES = {
    REPO / "engine/out/build/win-amd64-release/ghogx.exe":
        "engine/out/build/win-amd64-release",
    REPO / "engine/out/build/win-amd64-release/_tools_ark/ark_tool.exe":
        "engine/out/build/win-amd64-release/_tools_ark",
    REPO / "engine/out/build/win-amd64-release/_tools_dtb/dtb_tool.exe":
        "engine/out/build/win-amd64-release/_tools_dtb",
    REPO / "engine/out/build/win-amd64-release/_tools_milo/milo_tool.exe":
        "engine/out/build/win-amd64-release/_tools_milo",
    REPO / "engine/out/build/win-amd64-release/_tools_tex/tex_tool.exe":
        "engine/out/build/win-amd64-release/_tools_tex",
    REPO.parent / "_community_re/Guitar-Hero-II-Deluxe-Unified/dependencies/windows/dtab.exe":
        "_embedded",
}


def require_file(path: Path, label: str) -> Path:
    path = path.expanduser().resolve()
    if not path.is_file():
        raise install_dlc.InstallError(f"{label} is missing: {path}")
    return path


def add_resource(command: list[str], option: str, source: Path, destination: str) -> None:
    command.extend([option, f"{source.resolve()};{destination}"])


def build_setup_exe(
    output: Path,
    dolphin_tool: Path,
    ark_helper: Path,
    superfreq: Path,
    seven_zip: Path,
) -> Path:
    command = [
        sys.executable,
        "-m",
        "PyInstaller",
        "--noconfirm",
        "--clean",
        "--onefile",
        "--windowed",
        "--name",
        "First-Time Setup",
        "--collect-all",
        "PIL",
        "--paths",
        str(INSTALLER),
        "--paths",
        str(REPO / "tools"),
        "--paths",
        str(REPO / "rb2_wii/tools"),
    ]
    add_resource(
        command, "--add-data", INSTALLER / "build_gh80_character_package.py",
        "tools/dlc_installer",
    )
    add_resource(
        command, "--add-data", REPO / "tools/build_character_variant_overlay.py",
        "tools",
    )
    for name in RB2_TOOLS:
        add_resource(
            command, "--add-data", REPO / f"rb2_wii/tools/{name}",
            "rb2_wii/tools",
        )
    add_resource(command, "--add-data", REPO / "release", "release")
    add_resource(
        command, "--add-data", REPO / "config/character_variant_labels.tsv",
        "config",
    )
    for source, destination in NATIVE_BINARIES.items():
        add_resource(command, "--add-binary", require_file(source, source.name), destination)
    add_resource(command, "--add-binary", dolphin_tool, "_embedded")
    add_resource(command, "--add-binary", ark_helper, "_embedded")
    add_resource(command, "--add-binary", superfreq, "_embedded")
    add_resource(command, "--add-binary", seven_zip, "_embedded")
    seven_zip_dll = seven_zip.with_name("7z.dll")
    if seven_zip_dll.is_file():
        add_resource(command, "--add-binary", seven_zip_dll, "_embedded")

    with tempfile.TemporaryDirectory(prefix="ghc-setup-build-") as temporary:
        temporary_root = Path(temporary)
        command.extend(
            [
                "--distpath", str(output),
                "--workpath", str(temporary_root / "work"),
                "--specpath", str(temporary_root / "spec"),
                str(INSTALLER / "first_time_setup_app.py"),
            ]
        )
        result = subprocess.run(
            command,
            text=True,
            encoding="utf-8",
            errors="replace",
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        if result.returncode:
            raise install_dlc.InstallError(
                "First-Time Setup.exe build failed:\n" + result.stdout[-6000:]
            )
    return require_file(output / "First-Time Setup.exe", "frozen setup application")


def copy_license(source: Path, destination: Path) -> None:
    require_file(source, "license")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--dolphin-tool", required=True, type=Path)
    parser.add_argument("--ark-helper", required=True, type=Path)
    parser.add_argument("--superfreq", required=True, type=Path)
    parser.add_argument("--acceptance-report", required=True, type=Path)
    parser.add_argument("--ffmpeg", required=True, type=Path)
    parser.add_argument(
        "--seven-zip", type=Path,
        default=Path(r"C:\Program Files\7-Zip\7z.exe"),
    )
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise install_dlc.InstallError(f"distribution output must be empty: {output}")
    output.mkdir(parents=True, exist_ok=True)

    dolphin_tool = require_file(args.dolphin_tool, "DolphinTool")
    ark_helper = require_file(args.ark_helper, "ArkHelper")
    superfreq = require_file(args.superfreq, "SuperFreq")
    seven_zip = require_file(args.seven_zip, "7-Zip")
    acceptance_report = require_file(
        args.acceptance_report, "four-disc acceptance report"
    )
    ffmpeg = require_file(args.ffmpeg, "FFmpeg runtime decoder")
    acceptance = install_dlc.load_manifest(acceptance_report)
    cycles = acceptance.get("cycles")
    expected_cycles = {
        "disc.gh1.songs",
        "project.gh1.converted",
        "disc.gh80s.songs",
        "disc.gh80s.characters",
        "disc.rb2_wii.instruments",
    }
    actual_cycles = {
        str(row.get("removed_and_restored"))
        for row in cycles or []
        if isinstance(row, dict)
        and row.get("base_unchanged") is True
        and row.get("packages_deterministic") is True
    }
    if (
        acceptance.get("status") != "complete"
        or actual_cycles != expected_cycles
        or acceptance.get("work_cache_removed") is not True
        or not isinstance(acceptance.get("resume_cache_evidence"), dict)
        or acceptance["resume_cache_evidence"].get("passed") is not True
    ):
        raise install_dlc.InstallError(
            "four-disc acceptance report is not a complete five-package pass"
        )
    game_exe = require_file(
        REPO / "engine/out/build/win-amd64-release/src/app/ghogx_app.exe",
        "game executable",
    )
    shutil.copy2(game_exe, output / "Guitar Hero Classic.exe")
    shutil.copy2(ffmpeg, output / "ffmpeg.exe")

    project_verification = install_dlc.validate_package(
        REPO / "release/packages/project.gh1.converted"
    )
    build_setup_exe(output, dolphin_tool, ark_helper, superfreq, seven_zip)

    licenses = output / "LICENSES"
    copy_license(REPO / "third_party/Mackiloha/LICENSE", licenses / "Mackiloha.txt")
    copy_license(seven_zip.with_name("License.txt"), licenses / "7-Zip.txt")
    copy_license(dolphin_tool.with_name("COPYING"), licenses / "Dolphin.txt")
    ffmpeg_license = ffmpeg.parent.parent / "LICENSE"
    copy_license(ffmpeg_license, licenses / "FFmpeg.txt")
    (licenses / "SuperFreq-source.txt").write_text(
        "SuperFreq executable supplied by Guitar Hero II Deluxe Unified.\n"
        "Redistribution permission/license must be confirmed before public release.\n",
        encoding="utf-8",
    )
    (output / "README-FIRST.txt").write_text(
        "Guitar Hero Classic\n\n"
        "1. Run First-Time Setup.exe.\n"
        "2. Choose your user-owned GH2 USA image (required).\n"
        "3. Optionally add USA images for GH1 (songs), GH80s (songs and "
        "characters), and/or Rock Band 2 Wii (guitars).\n"
        "4. Follow the progress bar while setup validates and installs content.\n"
        "\nThis process will take a while. Keep setup open until it finishes.\n\n"
        "After validation passes, installation starts automatically. Setup opens a "
        "dialog only when an error needs your attention.\n\n"
        "GH2 remains the byte-identical base. GH1 media contributes songs only.\n"
        "Converted GH1 characters, animations, and venues are built into setup.\n",
        encoding="utf-8",
    )

    forbidden_suffixes = {".ark", ".hdr", ".iso", ".rvz", ".wbfs"}
    forbidden = []
    rows = []
    for path in sorted(value for value in output.rglob("*") if value.is_file()):
        relative = path.relative_to(output).as_posix()
        if (
            path.suffix.casefold() in forbidden_suffixes
            or relative.casefold().startswith("tools/")
            or relative.casefold().startswith("third_party/")
            or relative.casefold().startswith("gen/")
            or relative.casefold().startswith("dlc/disc.")
        ):
            forbidden.append(relative)
        rows.append(
            {
                "path": relative,
                "size": path.stat().st_size,
                "sha256": install_dlc.sha256_file(path),
            }
        )
    if forbidden:
        raise install_dlc.InstallError(
            "distribution contains forbidden release paths: " + ", ".join(forbidden)
        )
    manifest = {
        "schema_version": 2,
        "status": "stage_complete",
        "contains_user_game_archives": False,
        "supported_source_region": "USA",
        "required_user_sources": ["gh2"],
        "optional_user_sources": ["gh1", "gh80s", "rb2_wii"],
        "gh1_disc_import_policy": "songs-only",
        "setup": {
            "entry": "First-Time Setup.exe",
            "kind": "single-file-windowed-gui",
            "progress_bar": True,
            "progress_kind": "monotonic-phase",
            "elapsed_timer": True,
            "auto_install_after_validation": True,
            "console_windows": False,
            "embedded_dependencies": True,
        },
        "bundled_project_package": {
            "id": project_verification["id"],
            "files": project_verification["files"],
            "bytes": project_verification["bytes"],
            "sha256": project_verification["sha256"],
        },
        "external_runtime_requirements": [],
        "bundled_runtime_components": ["ffmpeg.exe"],
        "four_disc_acceptance": {
            "report": acceptance_report.name,
            "sha256": install_dlc.sha256_file(acceptance_report),
            "completed_utc": acceptance.get("completed_utc"),
            "cycles": sorted(actual_cycles),
            "source_cache_reuse": True,
            "gh2_base_unchanged": True,
            "work_cache_removed": True,
        },
        "files": rows,
    }
    install_dlc.write_json(output / "distribution-manifest.json", manifest)
    print(
        "DISTRIBUTION_STAGE_COMPLETE "
        f"files={len(rows)} output={output} "
        f"gh1_package={project_verification['sha256']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
