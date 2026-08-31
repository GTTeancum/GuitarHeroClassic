#!/usr/bin/env python3
"""Verify that published Midori banks cover retail Casey's GH2 clip calls."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import importlib.util
import json
import os
import sys
from pathlib import Path
from types import ModuleType
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = ROOT / "out/midori"
DEFAULT_HDR = WORK_ROOT / "input/GEN/MAIN.HDR"
DEFAULT_ARK = WORK_ROOT / "input/GEN/MAIN_0.ARK"
DEFAULT_PACKAGE = ROOT / "DLC/community.gh3.midori"
DEFAULT_PROVENANCE = None
DEFAULT_PARSER = ROOT / "tools/re_anim_audit.py"
DEFAULT_OUTPUT = WORK_ROOT / "gh3_midori_animation_call_compatibility.json"

BANKS = {
    "main": {
        "stock": "char/rock1/anims/gen/rock1_main.milo_ps2",
        "published": "content/char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2",
    },
    "ui": {
        "stock": "char/rock1/anims/gen/rock1_ui.milo_ps2",
        "published": "content/char/gh3_midori/anims/gen/gh3_midori_ui.milo_ps2",
    },
    "fret": {
        "stock": "char/rock1/anims/gen/rock1_fret.milo_ps2",
        "published": "content/char/gh3_midori/anims/gen/gh3_midori_fret.milo_ps2",
    },
    "strum": {
        "stock": "char/rock1/anims/gen/rock1_strum.milo_ps2",
        "published": "content/char/gh3_midori/anims/gen/gh3_midori_strum.milo_ps2",
    },
}


def set_idle_priority() -> None:
    if os.name == "nt":
        ctypes.windll.kernel32.SetPriorityClass(  # type: ignore[attr-defined]
            ctypes.windll.kernel32.GetCurrentProcess(), 0x40
        )


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def relative(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return str(path.resolve())


def load_parser(path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location("midori_re_anim_audit", path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load parser: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def bank_records(
    parser: ModuleType, raw_milo: bytes
) -> tuple[
    dict[str, dict[str, Any]],
    dict[str, dict[str, Any]],
    list[str],
    list[str],
]:
    payload = parser.inflate_milo(raw_milo)
    entries = parser.parse_dir(payload)[3]
    clips: dict[str, dict[str, Any]] = {}
    groups: dict[str, dict[str, Any]] = {}
    clip_duplicates: list[str] = []
    group_duplicates: list[str] = []
    for entry in entries:
        body = payload[entry.offset : entry.offset + entry.size]
        if entry.typ == "CharClipSamples":
            if entry.name in clips:
                clip_duplicates.append(entry.name)
                continue
            clips[entry.name] = {
                "byte_count": len(body),
                "sha256": sha256_bytes(body),
            }
        elif entry.typ == "CharClipGroup":
            if entry.name in groups:
                group_duplicates.append(entry.name)
                continue
            group = parser.parse_clip_group(body)
            groups[entry.name] = {
                "byte_count": len(body),
                "sha256": sha256_bytes(body),
                "members": list(group["clips"]),
            }
    return (
        clips,
        groups,
        sorted(clip_duplicates),
        sorted(group_duplicates),
    )


def compare_clip_records(
    stock: dict[str, dict[str, Any]], published: dict[str, dict[str, Any]]
) -> dict[str, Any]:
    stock_names = set(stock)
    published_names = set(published)
    covered = stock_names & published_names
    identical = sorted(
        name for name in covered if stock[name]["sha256"] == published[name]["sha256"]
    )
    replaced = sorted(covered - set(identical))
    missing = sorted(stock_names - published_names)
    published_only = sorted(published_names - stock_names)
    return {
        "stock_clip_count": len(stock_names),
        "published_clip_count": len(published_names),
        "covered_stock_call_count": len(covered),
        "byte_identical_preserved_count": len(identical),
        "replaced_or_retargeted_count": len(replaced),
        "published_only_count": len(published_only),
        "missing_stock_call_count": len(missing),
        "byte_identical_preserved": identical,
        "replaced_or_retargeted": replaced,
        "published_only": published_only,
        "missing_stock_calls": missing,
        "exact_name_coverage": not missing,
    }


def compare_group_records(
    stock: dict[str, dict[str, Any]], published: dict[str, dict[str, Any]]
) -> dict[str, Any]:
    stock_names = set(stock)
    published_names = set(published)
    covered = stock_names & published_names
    identical = sorted(
        name for name in covered if stock[name]["sha256"] == published[name]["sha256"]
    )
    replaced = sorted(covered - set(identical))
    missing = sorted(stock_names - published_names)
    return {
        "stock_group_count": len(stock_names),
        "published_group_count": len(published_names),
        "covered_stock_group_count": len(covered),
        "byte_identical_group_count": len(identical),
        "replaced_group_count": len(replaced),
        "missing_stock_group_count": len(missing),
        "byte_identical_groups": identical,
        "replaced_groups": replaced,
        "missing_stock_groups": missing,
        "exact_group_name_coverage": not missing,
    }


def build_report(args: argparse.Namespace) -> dict[str, Any]:
    hdr = args.stock_hdr.resolve()
    ark = args.stock_ark.resolve()
    package = args.package.resolve()
    provenance_path = args.provenance.resolve() if args.provenance else None
    parser_path = args.parser.resolve()
    for path in (hdr, ark, parser_path):
        if not path.is_file():
            raise FileNotFoundError(path)
    if provenance_path is not None and not provenance_path.is_file():
        raise FileNotFoundError(provenance_path)
    parser = load_parser(parser_path)
    provenance = (
        json.loads(provenance_path.read_text(encoding="utf-8"))
        if provenance_path is not None
        else {}
    )
    casey_provenance = provenance.get("gh2_casey", {})
    provenance_files = casey_provenance.get("files", {})
    archive_entries = {
        entry.full_path.casefold(): entry for entry in parser.ark_entries(hdr)
    }

    banks: dict[str, dict[str, Any]] = {}
    provenance_checks: dict[str, bool] = {}
    if provenance_path is not None:
        provenance_checks = {
            "retail_source_declared": casey_provenance.get("source")
            == "untouched retail GH2 PS2 main.hdr/main_0.ark",
            "hdr_sha256_current": sha256_file(hdr)
            == str(casey_provenance.get("hdr", {}).get("sha256") or "").upper(),
            "ark_byte_count_current": ark.stat().st_size
            == casey_provenance.get("ark", {}).get("byte_count"),
        }

    for role, paths in BANKS.items():
        stock_path = paths["stock"]
        entry = archive_entries.get(stock_path.casefold())
        if entry is None:
            raise KeyError(f"stock bank missing from ARK: {stock_path}")
        stock_raw = parser.read_ark_entry(ark, entry)
        published_path = package / paths["published"]
        if not published_path.is_file():
            raise FileNotFoundError(published_path)
        published_raw = published_path.read_bytes()
        (
            stock_records,
            stock_groups,
            stock_duplicates,
            stock_group_duplicates,
        ) = bank_records(parser, stock_raw)
        (
            published_records,
            published_groups,
            published_duplicates,
            published_group_duplicates,
        ) = bank_records(parser, published_raw)
        comparison = compare_clip_records(stock_records, published_records)
        group_comparison = compare_group_records(stock_groups, published_groups)
        dangling_groups = {
            name: sorted(set(group["members"]) - set(published_records))
            for name, group in published_groups.items()
            if set(group["members"]) - set(published_records)
        }
        record = provenance_files.get(role)
        if isinstance(record, dict):
            provenance_checks[f"{role}_stock_milo_sha256_current"] = (
                sha256_bytes(stock_raw) == str(record.get("sha256") or "").upper()
                and len(stock_raw) == record.get("byte_count")
                and record.get("ark_path") == stock_path
                and record.get("byte_exact") is True
            )
        elif provenance_path is not None:
            provenance_checks[f"{role}_stock_milo_sha256_current"] = False
        banks[role] = {
            "stock": {
                "ark_path": stock_path,
                "byte_count": len(stock_raw),
                "sha256": sha256_bytes(stock_raw),
                "duplicate_clip_names": stock_duplicates,
                "duplicate_group_names": stock_group_duplicates,
            },
            "published": {
                "path": relative(published_path),
                "byte_count": len(published_raw),
                "sha256": sha256_bytes(published_raw),
                "duplicate_clip_names": published_duplicates,
                "duplicate_group_names": published_group_duplicates,
            },
            **comparison,
            **group_comparison,
            "dangling_published_groups": dangling_groups,
        }

    checks = {
        "source_provenance_current": all(provenance_checks.values()),
        "all_stock_calls_present": all(
            bank["exact_name_coverage"] for bank in banks.values()
        ),
        "no_stock_duplicate_clip_names": all(
            not bank["stock"]["duplicate_clip_names"] for bank in banks.values()
        ),
        "no_published_duplicate_clip_names": all(
            not bank["published"]["duplicate_clip_names"] for bank in banks.values()
        ),
        "all_stock_groups_present": all(
            bank["exact_group_name_coverage"] for bank in banks.values()
        ),
        "no_stock_duplicate_group_names": all(
            not bank["stock"]["duplicate_group_names"] for bank in banks.values()
        ),
        "no_published_duplicate_group_names": all(
            not bank["published"]["duplicate_group_names"] for bank in banks.values()
        ),
        "no_dangling_published_group_members": all(
            not bank["dangling_published_groups"] for bank in banks.values()
        ),
    }
    totals = {
        key: sum(bank[key] for bank in banks.values())
        for key in (
            "stock_clip_count",
            "published_clip_count",
            "covered_stock_call_count",
            "byte_identical_preserved_count",
            "replaced_or_retargeted_count",
            "published_only_count",
            "missing_stock_call_count",
        )
    }
    totals.update(
        {
            key: sum(bank[key] for bank in banks.values())
            for key in (
                "stock_group_count",
                "published_group_count",
                "covered_stock_group_count",
                "byte_identical_group_count",
                "replaced_group_count",
                "missing_stock_group_count",
            )
        }
    )
    return {
        "format": "gh3_midori_animation_call_compatibility_v1",
        "status": (
            "gh2_animation_call_surface_covered"
            if all(checks.values())
            else "gh2_animation_call_surface_incomplete"
        ),
        "stock_source": {
            "description": casey_provenance.get("source")
            or "user-supplied extracted GH2 PS2 GEN archive",
            "hdr": relative(hdr),
            "hdr_sha256": sha256_file(hdr),
            "ark": relative(ark),
            "ark_byte_count": ark.stat().st_size,
            "provenance": (
                relative(provenance_path) if provenance_path is not None else None
            ),
        },
        "published_package": relative(package),
        "parser": relative(parser_path),
        "provenance_checks": provenance_checks,
        "checks": checks,
        "totals": totals,
        "banks": banks,
    }


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stock-hdr", type=Path, default=DEFAULT_HDR)
    parser.add_argument("--stock-ark", type=Path, default=DEFAULT_ARK)
    parser.add_argument("--package", type=Path, default=DEFAULT_PACKAGE)
    parser.add_argument(
        "--provenance",
        type=Path,
        default=DEFAULT_PROVENANCE,
        help="Optional source-authentication JSON for the supplied GH2 GEN archive.",
    )
    parser.add_argument("--parser", type=Path, default=DEFAULT_PARSER)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--verify-only", action="store_true")
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    set_idle_priority()
    args = parse_args(argv)
    report = build_report(args)
    output = args.output.resolve()
    if args.verify_only:
        current = json.loads(output.read_text(encoding="utf-8"))
        if current != report:
            raise ValueError(f"animation-call report is stale: {output}")
    else:
        write_json(output, report)
    print(
        json.dumps(
            {
                "status": report["status"],
                "stock_calls": report["totals"]["stock_clip_count"],
                "covered": report["totals"]["covered_stock_call_count"],
                "missing": report["totals"]["missing_stock_call_count"],
                "output": str(output),
            }
        )
    )
    return 0 if report["status"] == "gh2_animation_call_surface_covered" else 1


if __name__ == "__main__":
    raise SystemExit(main())
