#!/usr/bin/env python3
"""Apply the accepted Midori overlay to a copied GH2 ARK and verify it."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path
from typing import Any

import gh3_midori_animation_call_compatibility as animation_calls
import gh3_midori_build_casey_clone_package as package_builder
import gh3_midori_casey_retail_overlay_manifest as retail_manifest


ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = ROOT / "out/midori"
DEFAULT_ARK_TOOL = ROOT / "tools/ark/build/Release/ark_tool.exe"
DEFAULT_OUTPUT_DIR = WORK_ROOT / "retail_gate/GEN"
DEFAULT_REPORT = WORK_ROOT / "retail_gate/casey_overlay_apply.validation.json"


def proof_gate_checks(record: dict[str, Any]) -> dict[str, bool]:
    return {
        "proof_format": (
            record.get("format")
            == "gh3-midori-casey-clone-gameplay-proof-v1"
        ),
        "proof_status": record.get("status") == "pass",
        "engine": record.get("engine") == "Guitar Hero Classic ghogx_app",
        "hidden_window": record.get("hidden_window") is True,
        "clone_only": (
            record.get("iso_used") is False
            and record.get("emulator_used") is False
        ),
        "all_runtime_checks": record.get("all_checks_pass") is True,
        "artifacts_exact": record.get("artifact_hashes_exact") is True,
        "model_exact": (
            record.get("model_sha256")
            == package_builder.EXPECTED_HASHES["model"]
        ),
        "main_exact": (
            record.get("main_bank_sha256")
            == package_builder.EXPECTED_HASHES["main"]
        ),
        "human_visual_acceptance": record.get("user_acceptance") == "accepted",
    }


def accepted_clone_proof(path: Path) -> tuple[dict[str, Any], dict[str, bool]]:
    record = package_builder.clone_proof_record(path)
    checks = proof_gate_checks(record)
    if not all(checks.values()):
        failed = [name for name, passed in checks.items() if not passed]
        raise PermissionError(
            "retail apply gate is closed; clone proof failures: "
            + ", ".join(failed)
        )
    return record, checks


def build_overlay_command(
    tool: Path,
    hdr: Path,
    ark: Path,
    overlay_root: Path,
    manifest: Path,
) -> list[str]:
    return [
        str(tool),
        "overlay",
        str(hdr),
        str(ark),
        "--root",
        str(overlay_root),
        "--manifest",
        str(manifest),
    ]


def safe_output_paths(
    source_hdr: Path, source_ark: Path, output_dir: Path
) -> tuple[Path, Path]:
    source_hdr = source_hdr.resolve()
    source_ark = source_ark.resolve()
    output_dir = output_dir.resolve()
    output_hdr = output_dir / "MAIN.HDR"
    output_ark = output_dir / "MAIN_0.ARK"
    if output_hdr == source_hdr or output_ark == source_ark:
        raise ValueError("retail gate output must not alias the source archive")
    if output_dir == source_hdr.parent or output_dir == source_ark.parent:
        raise ValueError("retail gate output directory must differ from source GEN")
    return output_hdr, output_ark


def atomic_copy(source: Path, target: Path, overwrite: bool) -> None:
    if target.exists() and not overwrite:
        raise FileExistsError(f"output exists: {target}; pass --overwrite")
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_name(target.name + ".copying")
    if temporary.exists():
        temporary.unlink()
    shutil.copyfile(source, temporary)
    temporary.replace(target)


def source_gate(report_path: Path, manifest_path: Path) -> dict[str, Any]:
    if not report_path.is_file() or not manifest_path.is_file():
        raise FileNotFoundError("retail source report or manifest is missing")
    report = package_builder.read_json(report_path)
    checks = report.get("checks", {})
    manifest_sha256 = package_builder.sha256_file(manifest_path)
    if (
        report.get("status") != "pass"
        or not checks
        or not all(value is True for value in checks.values())
        or report.get("overlay", {}).get("manifest_sha256") != manifest_sha256
    ):
        raise ValueError("retail source report or manifest is not current")
    return report


def validate_patched_archive(
    hdr: Path,
    ark: Path,
    overlay_root: Path,
    parser_path: Path,
    source_report: dict[str, Any],
) -> dict[str, Any]:
    parser = animation_calls.load_parser(parser_path)
    header = parser.parse_hdr(hdr)
    entries = {
        entry.full_path.casefold(): entry for entry in parser.ark_entries(hdr)
    }
    targets = {}
    for role, archive_path in package_builder.RETAIL_OVERLAY_PATHS.items():
        entry = entries.get(archive_path.casefold())
        replacement = overlay_root / archive_path
        actual = parser.read_ark_entry(ark, entry) if entry is not None else b""
        expected_sha256 = package_builder.sha256_file(replacement)
        targets[role] = {
            "archive_path": archive_path,
            "path_exact": entry is not None and entry.full_path == archive_path,
            "byte_count": len(actual),
            "sha256": retail_manifest.sha256_bytes(actual),
            "expected_sha256": expected_sha256,
            "replacement_exact": (
                entry is not None
                and retail_manifest.sha256_bytes(actual) == expected_sha256
            ),
        }
    expected_size = source_report["overlay"]["projected_ark_byte_count"]
    part_sizes = list(header["parts"])
    checks = {
        "header_v3": header["version"] == 3,
        "single_part": len(part_sizes) == 1,
        "part_size_matches_file": (
            len(part_sizes) == 1 and part_sizes[0] == ark.stat().st_size
        ),
        "projected_size_exact": ark.stat().st_size == expected_size,
        "entry_count_preserved": (
            len(header["entries"])
            == source_report["archive"]["entry_count"]
        ),
        "all_target_paths_exact": all(
            row["path_exact"] for row in targets.values()
        ),
        "all_replacements_exact": all(
            row["replacement_exact"] for row in targets.values()
        ),
        "header_backup_present": Path(str(hdr) + ".pre-overlay.bak").is_file(),
    }
    return {
        "status": "pass" if all(checks.values()) else "fail",
        "hdr": str(hdr),
        "hdr_sha256": package_builder.sha256_file(hdr),
        "ark": str(ark),
        "ark_byte_count": ark.stat().st_size,
        "targets": targets,
        "checks": checks,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-hdr", type=Path, default=retail_manifest.DEFAULT_HDR)
    parser.add_argument("--source-ark", type=Path, default=retail_manifest.DEFAULT_ARK)
    parser.add_argument("--overlay-root", type=Path, default=retail_manifest.DEFAULT_OVERLAY)
    parser.add_argument("--manifest", type=Path, default=retail_manifest.DEFAULT_MANIFEST)
    parser.add_argument(
        "--source-report", type=Path, default=retail_manifest.DEFAULT_REPORT
    )
    parser.add_argument(
        "--clone-proof",
        type=Path,
        default=package_builder.DEFAULT_CLONE_PROOF_VALIDATION,
    )
    parser.add_argument("--ark-tool", type=Path, default=DEFAULT_ARK_TOOL)
    parser.add_argument("--parser", type=Path, default=retail_manifest.DEFAULT_PARSER)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--apply", action="store_true")
    mode.add_argument("--verify-only", action="store_true")
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def main() -> int:
    package_builder.set_low_priority()
    args = parse_args()
    source_hdr = args.source_hdr.resolve()
    source_ark = args.source_ark.resolve()
    overlay_root = args.overlay_root.resolve()
    manifest = args.manifest.resolve()
    source_report_path = args.source_report.resolve()
    clone_proof_path = args.clone_proof.resolve()
    ark_tool = args.ark_tool.resolve()
    parser_path = args.parser.resolve()
    output_hdr, output_ark = safe_output_paths(
        source_hdr, source_ark, args.output_dir
    )
    for path in (source_hdr, source_ark, ark_tool, parser_path):
        if not path.is_file():
            raise FileNotFoundError(path)
    proof, proof_checks = accepted_clone_proof(clone_proof_path)
    source_report = source_gate(source_report_path, manifest)

    overlay_stdout = ""
    if args.apply:
        atomic_copy(source_hdr, output_hdr, args.overwrite)
        atomic_copy(source_ark, output_ark, args.overwrite)
        command = build_overlay_command(
            ark_tool, output_hdr, output_ark, overlay_root, manifest
        )
        completed = subprocess.run(
            command,
            cwd=ROOT,
            env=os.environ.copy(),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=600,
            creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
            check=False,
        )
        overlay_stdout = completed.stdout
        if completed.returncode != 0:
            raise RuntimeError(
                f"ark_tool overlay failed with {completed.returncode}:\n"
                f"{completed.stdout[-4000:]}"
            )

    for path in (output_hdr, output_ark):
        if not path.is_file():
            raise FileNotFoundError(path)
    patched = validate_patched_archive(
        output_hdr, output_ark, overlay_root, parser_path, source_report
    )
    if patched["status"] != "pass":
        failed = [
            name for name, passed in patched["checks"].items() if not passed
        ]
        raise ValueError("patched retail archive failed: " + ", ".join(failed))

    report_path = args.report.resolve()
    payload = {
        "format": "gh3-midori-casey-retail-apply-v1",
        "status": "pass",
        "mode": "apply" if args.apply else "verify-only",
        "clone_proof": {
            "path": str(clone_proof_path),
            "sha256": package_builder.sha256_file(clone_proof_path),
            "user_acceptance": proof["user_acceptance"],
            "checks": proof_checks,
        },
        "source_gate": {
            "path": str(source_report_path),
            "sha256": package_builder.sha256_file(source_report_path),
            "manifest": str(manifest),
            "manifest_sha256": package_builder.sha256_file(manifest),
        },
        "patched_archive": patched,
        "overlay_stdout": overlay_stdout[-2000:],
        "execution_policy": {
            "source_archive_modified": False,
            "copied_archive_modified": args.apply,
            "iso_built": False,
            "iso_mounted": False,
            "emulator_used": False,
        },
    }
    if args.verify_only:
        current = package_builder.read_json(report_path)
        if (
            current.get("status") != "pass"
            or current.get("patched_archive") != patched
            or current.get("clone_proof", {}).get("sha256")
            != payload["clone_proof"]["sha256"]
            or current.get("source_gate", {}) != payload["source_gate"]
        ):
            raise ValueError("retail apply report or copied archive is stale")
    else:
        package_builder.write_json(report_path, payload)
    print(
        json.dumps(
            {
                "status": "pass",
                "mode": payload["mode"],
                "output_ark": str(output_ark),
                "ark_byte_count": patched["ark_byte_count"],
                "targets": len(patched["targets"]),
                "user_acceptance": proof["user_acceptance"],
                "report": str(report_path),
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
