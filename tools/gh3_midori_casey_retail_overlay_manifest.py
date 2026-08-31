#!/usr/bin/env python3
"""Authenticate Casey's retail ARK targets and write an overlay manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

import gh3_midori_animation_call_compatibility as animation_calls
import gh3_midori_build_casey_clone_package as package_builder


ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = ROOT / "out/midori"
DEFAULT_HDR = WORK_ROOT / "input/GEN/MAIN.HDR"
DEFAULT_ARK = WORK_ROOT / "input/GEN/MAIN_0.ARK"
DEFAULT_OVERLAY = WORK_ROOT / "retail_casey_overlay"
DEFAULT_MANIFEST = WORK_ROOT / "retail_casey_overlay.tsv"
DEFAULT_REPORT = WORK_ROOT / "retail_casey_overlay.source_archive.json"
DEFAULT_PARSER = ROOT / "tools/re_anim_audit.py"

STOCK_SOURCES = {
    "model": WORK_ROOT / "input/rock1.casey_template.milo_ps2",
    "main": WORK_ROOT / "input/stock_casey_banks/rock1_main.milo_ps2",
    "ui": WORK_ROOT / "input/stock_casey_banks/rock1_ui.milo_ps2",
    "fret": WORK_ROOT / "input/stock_casey_banks/rock1_fret.milo_ps2",
    "strum": WORK_ROOT / "input/stock_casey_banks/rock1_strum.milo_ps2",
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def relative(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return str(path.resolve())


def manifest_text(targets: dict[str, dict[str, Any]]) -> str:
    lines = [
        "relative_path\tsha256\tbyte_count\tstock_sha256\tstock_byte_count"
    ]
    for record in sorted(targets.values(), key=lambda row: row["archive_path"]):
        lines.append(
            "\t".join(
                (
                    record["archive_path"],
                    record["replacement"]["sha256"],
                    str(record["replacement"]["byte_count"]),
                    record["stock_archive"]["sha256"],
                    str(record["stock_archive"]["byte_count"]),
                )
            )
        )
    return "\n".join(lines) + "\n"


def build_report(args: argparse.Namespace) -> tuple[dict[str, Any], str]:
    hdr = args.stock_hdr.resolve()
    ark = args.stock_ark.resolve()
    overlay = args.overlay_root.resolve()
    parser_path = args.parser.resolve()
    for path in (hdr, ark, parser_path):
        if not path.is_file():
            raise FileNotFoundError(path)
    parser = animation_calls.load_parser(parser_path)
    header = parser.parse_hdr(hdr)
    entries = {
        entry.full_path.casefold(): entry for entry in parser.ark_entries(hdr)
    }

    targets: dict[str, dict[str, Any]] = {}
    failures: list[str] = []
    appended_bytes = 0
    for role, archive_path in package_builder.RETAIL_OVERLAY_PATHS.items():
        entry = entries.get(archive_path.casefold())
        if entry is None:
            failures.append(f"retail target missing from archive: {archive_path}")
            continue
        stock_raw = parser.read_ark_entry(ark, entry)
        stock_source = STOCK_SOURCES[role].resolve()
        replacement = (overlay / archive_path).resolve()
        if not stock_source.is_file():
            failures.append(f"stock source is missing: {stock_source}")
            continue
        if not replacement.is_file():
            failures.append(f"overlay replacement is missing: {replacement}")
            continue
        stock_source_raw = stock_source.read_bytes()
        replacement_raw = replacement.read_bytes()
        stock_sha256 = sha256_bytes(stock_raw)
        replacement_sha256 = sha256_bytes(replacement_raw)
        changes_stock = replacement_raw != stock_raw
        if changes_stock:
            appended_bytes += len(replacement_raw)
        targets[role] = {
            "archive_path": archive_path,
            "archive_path_exact": entry.full_path == archive_path,
            "stock_archive": {
                "offset": entry.offset,
                "byte_count": len(stock_raw),
                "sha256": stock_sha256,
            },
            "stock_source": {
                "path": relative(stock_source),
                "byte_count": len(stock_source_raw),
                "sha256": sha256_bytes(stock_source_raw),
                "archive_exact": stock_source_raw == stock_raw,
            },
            "replacement": {
                "path": relative(replacement),
                "byte_count": len(replacement_raw),
                "sha256": replacement_sha256,
                "expected_sha256": package_builder.EXPECTED_HASHES[role],
                "expected_exact": (
                    replacement_sha256
                    == package_builder.EXPECTED_HASHES[role]
                ),
                "changes_stock": changes_stock,
            },
        }

    manifest = manifest_text(targets)
    part_sizes = list(header["parts"])
    ark_byte_count = ark.stat().st_size
    projected_ark_byte_count = ark_byte_count + appended_bytes
    checks = {
        "archive_version_is_v3": header["version"] == 3,
        "archive_is_single_part": len(part_sizes) == 1,
        "declared_part_size_matches_ark": (
            len(part_sizes) == 1 and part_sizes[0] == ark_byte_count
        ),
        "all_five_targets_exist": len(targets) == 5,
        "archive_paths_are_exact": (
            len(targets) == 5
            and all(row["archive_path_exact"] for row in targets.values())
        ),
        "stock_sources_match_archive": (
            len(targets) == 5
            and all(
                row["stock_source"]["archive_exact"]
                for row in targets.values()
            )
        ),
        "replacements_match_expected_hashes": (
            len(targets) == 5
            and all(
                row["replacement"]["expected_exact"]
                for row in targets.values()
            )
        ),
        "model_and_main_replace_stock": (
            all(
                role in targets
                and targets[role]["replacement"]["changes_stock"]
                for role in ("model", "main")
            )
        ),
        "ui_fret_strum_reuse_stock": (
            all(
                role in targets
                and not targets[role]["replacement"]["changes_stock"]
                for role in ("ui", "fret", "strum")
            )
        ),
        "projected_ark_fits_v3_u32": projected_ark_byte_count <= 0xFFFFFFFF,
    }
    status = "pass" if not failures and all(checks.values()) else "fail"
    report = {
        "format": "gh3-midori-casey-retail-overlay-source-v1",
        "status": status,
        "archive": {
            "hdr": relative(hdr),
            "hdr_sha256": package_builder.sha256_file(hdr),
            "ark": relative(ark),
            "ark_byte_count": ark_byte_count,
            "version": header["version"],
            "flag": header["flag"],
            "part_sizes": part_sizes,
            "entry_count": len(header["entries"]),
        },
        "overlay": {
            "root": relative(overlay),
            "manifest": relative(args.manifest.resolve()),
            "manifest_sha256": sha256_bytes(manifest.encode("utf-8")),
            "target_count": len(targets),
            "changed_target_count": sum(
                row["replacement"]["changes_stock"]
                for row in targets.values()
            ),
            "reused_target_count": sum(
                not row["replacement"]["changes_stock"]
                for row in targets.values()
            ),
            "appended_byte_count": appended_bytes,
            "projected_ark_byte_count": projected_ark_byte_count,
        },
        "targets": targets,
        "checks": checks,
        "failures": failures,
        "execution_policy": {
            "read_only_archive_audit": True,
            "ark_written": False,
            "iso_built": False,
            "iso_mounted": False,
            "emulator_used": False,
        },
    }
    return report, manifest


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(text, encoding="utf-8")
    temporary.replace(path)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stock-hdr", type=Path, default=DEFAULT_HDR)
    parser.add_argument("--stock-ark", type=Path, default=DEFAULT_ARK)
    parser.add_argument("--overlay-root", type=Path, default=DEFAULT_OVERLAY)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--parser", type=Path, default=DEFAULT_PARSER)
    parser.add_argument("--verify-only", action="store_true")
    return parser.parse_args()


def main() -> int:
    package_builder.set_low_priority()
    args = parse_args()
    report, manifest = build_report(args)
    report_path = args.report.resolve()
    manifest_path = args.manifest.resolve()
    if args.verify_only:
        if not report_path.is_file() or not manifest_path.is_file():
            raise FileNotFoundError("retail overlay manifest/report is missing")
        current_report = json.loads(report_path.read_text(encoding="utf-8"))
        current_manifest = manifest_path.read_text(encoding="utf-8")
        if current_report != report or current_manifest != manifest:
            raise ValueError("retail overlay source report or manifest is stale")
    else:
        package_builder.write_json(report_path, report)
        if report["status"] == "pass":
            write_text(manifest_path, manifest)
    print(
        json.dumps(
            {
                "status": report["status"],
                "targets": report["overlay"]["target_count"],
                "changed": report["overlay"]["changed_target_count"],
                "reused": report["overlay"]["reused_target_count"],
                "projected_ark_byte_count": report["overlay"][
                    "projected_ark_byte_count"
                ],
                "manifest": str(manifest_path),
                "report": str(report_path),
            }
        )
    )
    return 0 if report["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
