#!/usr/bin/env python3
"""Run the destructive-to-output-only four-disc loose-DLC acceptance cycle.

The supplied source media is always read-only. The output root must be empty so
this harness can safely install, remove, and reinstall packages without
touching an existing player installation.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
import sys
import uuid
from pathlib import Path
from typing import Any

import install_dlc


EXPECTED_PACKAGES = [
    "disc.gh1.songs",
    "project.gh1.converted",
    "disc.gh80s.songs",
    "disc.gh80s.characters",
    "disc.rb2_wii.instruments",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the clean four-disc install/remove/reinstall acceptance"
    )
    parser.add_argument("--install-root", required=True, type=Path)
    parser.add_argument("--gh2", required=True, type=Path)
    parser.add_argument("--gh1", required=True, type=Path)
    parser.add_argument("--gh80s", required=True, type=Path)
    parser.add_argument("--rb2-wii", required=True, type=Path)
    parser.add_argument("--dolphin-tool", type=Path)
    parser.add_argument("--seven-zip", type=Path)
    return parser.parse_args()


def run(command: list[str], report: dict[str, Any]) -> None:
    result = subprocess.run(
        command,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    report["commands"].append(
        {
            "argv": command,
            "exit_code": result.returncode,
            "output_tail": result.stdout[-8000:],
        }
    )
    if result.returncode:
        raise install_dlc.InstallError(
            f"acceptance command failed ({result.returncode}): "
            f"{subprocess.list2cmdline(command)}\n{result.stdout[-2000:]}"
        )


def base_fingerprint(base_gen: Path) -> list[dict[str, Any]]:
    hdrs = install_dlc.find_casefold(base_gen, "MAIN.HDR")
    if len(hdrs) != 1:
        raise install_dlc.InstallError("acceptance output has no unique GH2 MAIN.HDR")
    files = [
        hdrs[0],
        *sorted(
            (
                path for path in hdrs[0].parent.glob("MAIN_*.ARK")
                if path.is_file()
            ),
            key=lambda path: int(path.stem.split("_")[-1]),
        ),
    ]
    return [
        {
            "name": path.name,
            "size": path.stat().st_size,
            "sha256": install_dlc.sha256_file(path),
        }
        for path in files
    ]


def package_fingerprints(dlc_root: Path) -> dict[str, str]:
    results = {}
    for package_id in EXPECTED_PACKAGES:
        package = dlc_root / package_id
        verification = install_dlc.validate_package(package)
        results[package_id] = verification["sha256"]
    return results


def require_gh1_content_boundary(dlc_root: Path) -> dict[str, Any]:
    songs_manifest = install_dlc.load_manifest(
        dlc_root / "disc.gh1.songs/manifest.json"
    )
    converted_manifest = install_dlc.load_manifest(
        dlc_root / "project.gh1.converted/manifest.json"
    )
    songs_files = songs_manifest.get("files")
    converted_files = converted_manifest.get("files")
    if not isinstance(songs_files, list) or not all(
        isinstance(path, str) for path in songs_files
    ):
        raise install_dlc.InstallError("disc.gh1.songs has no valid file list")
    if not isinstance(converted_files, list) or not all(
        isinstance(path, str) for path in converted_files
    ):
        raise install_dlc.InstallError("project.gh1.converted has no valid file list")

    invalid_song_files = sorted(
        path for path in songs_files
        if path != "config/dlc/gh1/songs.dtb" and not path.startswith("songs/")
    )
    if invalid_song_files:
        raise install_dlc.InstallError(
            "GH1 media-derived package contains non-song content: "
            + ", ".join(invalid_song_files[:10])
        )
    provenance = songs_manifest.get("provenance")
    if not isinstance(provenance, dict) or provenance.get("source_role") != "gh1":
        raise install_dlc.InstallError(
            "disc.gh1.songs does not identify GH1 media as its source"
        )

    converted_song_files = sorted(
        path for path in converted_files
        if path.startswith("songs/") or path == "config/dlc/gh1/songs.dtb"
    )
    if converted_song_files:
        raise install_dlc.InstallError(
            "bundled GH1 converted-content package contains songs: "
            + ", ".join(converted_song_files[:10])
        )
    if not any(path.startswith("char/") for path in converted_files):
        raise install_dlc.InstallError(
            "bundled GH1 converted-content package contains no characters"
        )
    if not any(path.startswith("world/") for path in converted_files):
        raise install_dlc.InstallError(
            "bundled GH1 converted-content package contains no venues"
        )
    overlap = sorted(set(songs_files) & set(converted_files))
    if overlap:
        raise install_dlc.InstallError(
            "GH1 source and bundled package payloads overlap: "
            + ", ".join(overlap[:10])
        )
    return {
        "disc_package": "disc.gh1.songs",
        "disc_policy": "songs-only",
        "disc_files": len(songs_files),
        "bundled_package": "project.gh1.converted",
        "bundled_files": len(converted_files),
        "bundled_contains_characters": True,
        "bundled_contains_venues": True,
        "payload_overlap": 0,
        "passed": True,
    }


def require_resume_cache_evidence(dlc_root: Path) -> dict[str, Any]:
    audits = sorted(
        (
            path for path in (dlc_root / ".install-audit").glob("*.json")
            if not path.name.startswith("manage-")
        ),
        key=lambda path: (path.stat().st_mtime_ns, path.name),
    )
    if not audits:
        raise install_dlc.InstallError("no installer audit exists for resume proof")
    audit_path = audits[-1]
    audit = install_dlc.load_manifest(audit_path)
    if audit.get("status") != "complete":
        raise install_dlc.InstallError(
            f"latest installer audit is not complete: {audit_path}"
        )
    sources = audit.get("sources")
    events = audit.get("cache_events")
    if not isinstance(sources, list) or not isinstance(events, list):
        raise install_dlc.InstallError(
            f"installer audit has no source/cache evidence: {audit_path}"
        )
    expected = {
        str(row.get("sha256"))
        for row in sources
        if isinstance(row, dict) and row.get("sha256")
    }
    reused = {
        str(row.get("source_sha256"))
        for row in events
        if isinstance(row, dict) and row.get("status") == "reused"
    }
    missing = sorted(expected - reused)
    if missing:
        raise install_dlc.InstallError(
            "reinstall did not reuse every source-hash cache: " + ", ".join(missing)
        )
    return {
        "audit": audit_path.relative_to(dlc_root.parent).as_posix(),
        "audit_sha256": install_dlc.sha256_file(audit_path),
        "source_hashes": sorted(expected),
        "reused_source_hashes": sorted(reused),
        "passed": True,
    }


def require_qualified_gh1_release() -> dict[str, Any]:
    repo = Path(__file__).resolve().parents[2]
    release_manifest_path = repo / "release/dlc-content.json"
    release_manifest = install_dlc.load_manifest(release_manifest_path)
    rows = release_manifest.get("packages")
    if not isinstance(rows, list):
        raise install_dlc.InstallError("release content manifest has no package list")
    row = next(
        (
            value for value in rows
            if isinstance(value, dict)
            and value.get("id") == "project.gh1.converted"
        ),
        None,
    )
    if row is None or row.get("release_ready") is not True:
        reason = row.get("reason", "missing release row") if row else "missing release row"
        raise install_dlc.InstallError(
            "project.gh1.converted release gate is not ready: " + str(reason)
        )
    validated = install_dlc.validate_release_ready_package(
        release_manifest_path, row
    )
    return {
        "manifest": str(release_manifest_path),
        "manifest_sha256": install_dlc.sha256_file(release_manifest_path),
        "package": str(validated["source"]),
        "package_sha256": validated["package_sha256"],
        "qualification": str(validated["qualification_path"]),
        "qualification_sha256": validated["qualification_sha256"],
        "redistribution_basis": validated["redistribution_basis"],
    }


def main() -> int:
    args = parse_args()
    install_root = args.install_root.resolve()
    if install_root.exists() and any(install_root.iterdir()):
        print(
            f"ACCEPTANCE_REFUSED: output root is not empty: {install_root}",
            file=sys.stderr,
        )
        return 2
    install_root.mkdir(parents=True, exist_ok=True)
    dlc_root = install_root / "DLC"
    base_gen = install_root / "gen"
    run_id = f"{dt.datetime.now().strftime('%Y%m%dT%H%M%S')}-{uuid.uuid4().hex[:8]}"
    report_path = install_root / "acceptance" / f"four-disc-{run_id}.json"
    report: dict[str, Any] = {
        "schema_version": 1,
        "run_id": run_id,
        "status": "running",
        "expected_packages": EXPECTED_PACKAGES,
        "commands": [],
        "cycles": [],
        "source_media_policy": "read-only user-owned media",
        "implementation": [],
    }
    try:
        for path in (
            Path(__file__).resolve(),
            Path(__file__).with_name("first_run_setup.py"),
            Path(__file__).with_name("install_dlc.py"),
            Path(__file__).with_name("manage_dlc.py"),
        ):
            report["implementation"].append(
                {
                    "path": str(path),
                    "size": path.stat().st_size,
                    "sha256": install_dlc.sha256_file(path),
                }
            )
        report["gh1_release_preflight"] = require_qualified_gh1_release()
        for path in (args.gh2, args.gh1, args.gh80s, args.rb2_wii):
            if not path.resolve().exists():
                raise install_dlc.InstallError(f"source does not exist: {path.resolve()}")
        first_run = Path(__file__).with_name("first_run_setup.py")
        first_command = [
            sys.executable,
            str(first_run),
            "--install-dir",
            str(install_root),
            "--gh2",
            str(args.gh2.resolve()),
            "--gh1",
            str(args.gh1.resolve()),
            "--gh80s",
            str(args.gh80s.resolve()),
            "--rb2-wii",
            str(args.rb2_wii.resolve()),
            "--yes",
            "--keep-work",
        ]
        if args.dolphin_tool:
            first_command.extend(["--dolphin-tool", str(args.dolphin_tool.resolve())])
        if args.seven_zip:
            first_command.extend(["--seven-zip", str(args.seven_zip.resolve())])
        run(first_command, report)

        manager = Path(__file__).with_name("manage_dlc.py")
        run(
            [sys.executable, str(manager), "--dlc-root", str(dlc_root), "verify"],
            report,
        )
        report["gh1_content_boundary"] = require_gh1_content_boundary(dlc_root)
        initial_base = base_fingerprint(base_gen)
        initial_packages = package_fingerprints(dlc_root)
        report["initial_base"] = initial_base
        report["initial_packages"] = initial_packages

        installer = Path(__file__).with_name("install_dlc.py")
        reinstall = [
            sys.executable,
            str(installer),
            "--gh2",
            str(args.gh2.resolve()),
            "--gh1",
            str(args.gh1.resolve()),
            "--gh80s",
            str(args.gh80s.resolve()),
            "--rb2-wii",
            str(args.rb2_wii.resolve()),
            "--dlc-root",
            str(dlc_root),
            "--base-gen",
            str(base_gen),
            "--keep-work",
        ]
        if args.dolphin_tool:
            reinstall.extend(["--dolphin-tool", str(args.dolphin_tool.resolve())])
        if args.seven_zip:
            reinstall.extend(["--seven-zip", str(args.seven_zip.resolve())])

        for cycle_index, package_id in enumerate(EXPECTED_PACKAGES):
            run(
                [
                    sys.executable,
                    str(manager),
                    "--dlc-root",
                    str(dlc_root),
                    "remove",
                    package_id,
                    "--yes",
                ],
                report,
            )
            run(
                [sys.executable, str(manager), "--dlc-root", str(dlc_root), "verify"],
                report,
            )
            run(reinstall, report)
            if cycle_index == 0:
                report["resume_cache_evidence"] = require_resume_cache_evidence(
                    dlc_root
                )
            current_base = base_fingerprint(base_gen)
            current_packages = package_fingerprints(dlc_root)
            if current_base != initial_base:
                raise install_dlc.InstallError(
                    f"GH2 base changed during {package_id} reinstall cycle"
                )
            if current_packages != initial_packages:
                raise install_dlc.InstallError(
                    f"package output was not deterministic after reinstalling {package_id}"
                )
            report["cycles"].append(
                {
                    "removed_and_restored": package_id,
                    "base_unchanged": True,
                    "packages_deterministic": True,
                }
            )

        run(
            [sys.executable, str(manager), "--dlc-root", str(dlc_root), "verify"],
            report,
        )
        report["installer_audits"] = [
            {
                "path": path.relative_to(install_root).as_posix(),
                "sha256": install_dlc.sha256_file(path),
            }
            for path in sorted((dlc_root / ".install-audit").glob("*.json"))
        ]
        work_root = dlc_root / ".installer-work"
        if work_root.exists():
            if work_root.resolve().parent != dlc_root.resolve():
                raise install_dlc.InstallError("installer work path escaped DLC root")
            install_dlc.remove_tree(work_root)
        report["work_cache_removed"] = not work_root.exists()
        report["status"] = "complete"
        report["completed_utc"] = install_dlc.utc_now()
        install_dlc.write_json(report_path, report)
        print(f"FOUR_DISC_ACCEPTANCE_OK report={report_path}")
        return 0
    except BaseException as error:
        report["status"] = "failed"
        report["error"] = str(error)
        report["completed_utc"] = install_dlc.utc_now()
        install_dlc.write_json(report_path, report)
        print(f"FOUR_DISC_ACCEPTANCE_FAILED report={report_path}: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
