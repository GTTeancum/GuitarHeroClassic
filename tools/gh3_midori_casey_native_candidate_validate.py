#!/usr/bin/env python3
"""Validate a promoted Casey-native Midori main animation bank."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import math
import os
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TOOL = ROOT / (
    "tools/milo_convert/out/build/win-amd64-release/Release/"
    "milo_convert_tool.exe"
)
SAMPLE_RE = re.compile(r"^(full|one)\s+sample=(\d+)\s+(\S+)\s+(.+)$")


def set_idle_priority() -> None:
    os.environ.update(
        {
            "OMP_NUM_THREADS": "1",
            "OPENBLAS_NUM_THREADS": "1",
            "MKL_NUM_THREADS": "1",
        }
    )
    if os.name == "nt":
        ctypes.windll.kernel32.SetPriorityClass(
            ctypes.windll.kernel32.GetCurrentProcess(), 0x40
        )


def run_tool(tool: Path, arguments: list[str]) -> str:
    completed = subprocess.run(
        [str(tool), *arguments],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=120,
        creationflags=getattr(subprocess, "IDLE_PRIORITY_CLASS", 0),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"milo_convert_tool failed with {completed.returncode}: "
            f"{' '.join(arguments)}\n{completed.stderr[-4000:]}"
        )
    return completed.stdout


def channel_size(channel: str) -> int:
    if channel.endswith((".pos", ".scale")):
        return 12
    if channel.endswith(".quat"):
        return 16
    if channel.endswith((".rotx", ".roty", ".rotz", ".drotx", ".droty", ".drotz")):
        return 4
    raise ValueError(f"unsupported sampled channel: {channel}")


def parse_clip_inventory(tool: Path, main_milo: Path) -> dict[str, dict[str, Any]]:
    output = run_tool(tool, ["inspect-clipset", str(main_milo), "--channels"])
    inventory: dict[str, dict[str, Any]] = {}
    current: dict[str, Any] | None = None
    for line in output.splitlines():
        if line.startswith("clip\t"):
            parts = line.split("\t")
            fields = dict(
                item.split("=", 1) for item in parts[4:] if "=" in item
            )
            current = {
                "name": parts[1],
                "clip_flags": int(fields.get("body_flags") or parts[2] or 0),
                "body_bytes": int(fields.get("body_bytes") or 0),
                "sample_bytes": int(fields.get("sample_bytes") or 0),
                "declared_sample_count": (
                    int(fields["full_samples"])
                    if "full_samples" in fields
                    else None
                ),
                "event_count": int(fields.get("events") or 0),
                "legacy_enter_event": fields.get("enter", ""),
                "legacy_exit_event": fields.get("exit", ""),
                "channels": [],
                "one_channels": [],
            }
            inventory[current["name"]] = current
        elif line.startswith("channel\t") and current is not None:
            parts = line.split("\t")
            if len(parts) >= 4 and parts[2] in ("full", "one"):
                key = "channels" if parts[2] == "full" else "one_channels"
                current[key].append(parts[3])
    for row in inventory.values():
        declared = row["declared_sample_count"]
        if declared is not None:
            row["sample_count"] = max(1, int(declared))
            continue
        stride = sum(channel_size(channel) for channel in row["channels"])
        if stride <= 0:
            row["sample_count"] = 1
        elif int(row["sample_bytes"]) % stride:
            raise ValueError(f"{row['name']}: sample bytes do not divide channel stride")
        else:
            row["sample_count"] = int(row["sample_bytes"]) // stride
    return inventory


def parse_all_sample_output(
    text: str, sample_count: int
) -> list[dict[str, list[float]]]:
    rows: list[dict[str, list[float]]] = [dict() for _ in range(sample_count)]
    one_values: dict[str, list[float]] = {}
    for line in text.splitlines():
        match = SAMPLE_RE.match(line.strip())
        if not match:
            continue
        label, raw_sample, channel, raw = match.groups()
        values = [float(part) for part in raw.split(",") if part.strip()]
        if label == "one":
            one_values[channel] = values
            continue
        sample = int(raw_sample)
        if sample < 0 or sample >= sample_count:
            raise ValueError(f"sample-clip returned out-of-range sample {sample}")
        rows[sample][channel] = values
    for row in rows:
        for channel, values in one_values.items():
            row.setdefault(channel, list(values))
    missing = [index for index, row in enumerate(rows) if not row]
    if missing and any(rows):
        raise ValueError(f"sample-clip all omitted samples: {missing[:12]}")
    return rows


def sample_clip_values(
    tool: Path, main_milo: Path, clip: str, sample_count: int
) -> list[dict[str, list[float]]]:
    output = run_tool(tool, ["sample-clip", str(main_milo), clip, "all"])
    return parse_all_sample_output(output, sample_count)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def metadata(row: dict[str, Any]) -> tuple[Any, ...]:
    return (
        int(row["clip_flags"]),
        int(row["body_bytes"]),
        int(row["sample_bytes"]),
        int(row["sample_count"]),
        int(row["event_count"]),
        str(row["legacy_enter_event"]),
        str(row["legacy_exit_event"]),
        tuple(row["channels"]),
        tuple(row["one_channels"]),
    )


def structural_metadata(row: dict[str, Any]) -> tuple[Any, ...]:
    return (
        int(row["clip_flags"]),
        int(row["event_count"]),
        str(row["legacy_enter_event"]),
        str(row["legacy_exit_event"]),
        tuple(row["channels"]),
        tuple(row["one_channels"]),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--reports", type=Path, required=True)
    parser.add_argument("--clip", action="append", required=True)
    parser.add_argument("--tool", type=Path, default=DEFAULT_TOOL)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    set_idle_priority()
    for name in ("base", "candidate", "reports", "tool"):
        path = getattr(args, name).resolve()
        if not path.exists():
            raise FileNotFoundError(path)
        setattr(args, name, path)
    args.output = args.output.resolve()
    changed = set(args.clip)
    if len(changed) != len(args.clip):
        raise ValueError("duplicate --clip declaration")

    base_header = run_tool(args.tool, ["inspect-clipset", str(args.base)]).splitlines()[0]
    candidate_header = run_tool(
        args.tool, ["inspect-clipset", str(args.candidate)]
    ).splitlines()[0]
    base = parse_clip_inventory(args.tool, args.base)
    candidate = parse_clip_inventory(args.tool, args.candidate)
    missing = sorted(changed - (set(base) & set(candidate)))
    if missing:
        raise ValueError("changed clips missing from bank: " + ", ".join(missing))

    unaffected_mismatches = sorted(
        name
        for name in set(base) - changed
        if name not in candidate or metadata(base[name]) != metadata(candidate[name])
    )
    changed_rows: list[dict[str, Any]] = []
    all_values_finite = True
    all_sample_channels_exact = True
    max_quaternion_norm_error = 0.0
    report_checks_pass = True
    report_schemas_exact = True
    base_hash = sha256(args.base)
    for name in sorted(changed):
        report_path = args.reports / f"{name}.report.json"
        if not report_path.is_file():
            raise FileNotFoundError(report_path)
        report = json.loads(report_path.read_text(encoding="utf-8"))
        report_checks_pass &= (
            report.get("status") == "pass"
            and all(report.get("checks", {}).values())
            and report.get("base_main_sha256") == base_hash
            and report.get("destination_clip") == name
        )
        report_schemas_exact &= (
            report.get("full_channels") == base[name]["channels"]
            and report.get("one_channels") == base[name]["one_channels"]
        )
        schema_exact = structural_metadata(base[name]) == structural_metadata(
            candidate[name]
        )
        samples = sample_clip_values(
            args.tool, args.candidate, name, int(candidate[name]["sample_count"])
        )
        expected_channels = set(candidate[name]["channels"]) | set(
            candidate[name]["one_channels"]
        )
        clip_channels_exact = all(set(sample) == expected_channels for sample in samples)
        clip_finite = all(
            math.isfinite(value)
            for sample in samples
            for values in sample.values()
            for value in values
        )
        clip_max_quat_error = max(
            (
                abs(math.sqrt(sum(value * value for value in values)) - 1.0)
                for sample in samples
                for channel, values in sample.items()
                if channel.endswith(".quat")
            ),
            default=0.0,
        )
        all_values_finite &= clip_finite
        all_sample_channels_exact &= clip_channels_exact
        max_quaternion_norm_error = max(
            max_quaternion_norm_error, clip_max_quat_error
        )
        changed_rows.append(
            {
                "clip": name,
                "sample_count": len(samples),
                "expected_sample_count": int(report["sample_count"]),
                "full_channel_count": len(candidate[name]["channels"]),
                "one_channel_count": len(candidate[name]["one_channels"]),
                "native_structure_exact": schema_exact,
                "sample_channels_exact": clip_channels_exact,
                "all_values_finite": clip_finite,
                "max_quaternion_norm_error": clip_max_quat_error,
                "encoder_report": str(report_path),
                "encoder_report_sha256": sha256(report_path),
            }
        )

    checks = {
        "clipset_header_exact": base_header == candidate_header,
        "clip_inventory_exact": set(base) == set(candidate),
        "unaffected_clip_metadata_exact": not unaffected_mismatches,
        "encoder_reports_authenticated_and_passing": report_checks_pass,
        "encoder_report_schemas_exact": report_schemas_exact,
        "changed_native_structures_exact": all(
            row["native_structure_exact"] for row in changed_rows
        ),
        "changed_sample_counts_exact": all(
            row["sample_count"] == row["expected_sample_count"]
            for row in changed_rows
        ),
        "changed_sample_channel_inventories_exact": all_sample_channels_exact,
        "changed_values_finite": all_values_finite,
        "changed_quaternions_normalized": max_quaternion_norm_error <= 5.0e-5,
    }
    result = {
        "format": "gh3-midori-casey-native-candidate-validation-v1",
        "status": "pass" if all(checks.values()) else "fail",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "base": str(args.base),
        "base_sha256": base_hash,
        "candidate": str(args.candidate),
        "candidate_sha256": sha256(args.candidate),
        "changed_clips": sorted(changed),
        "unaffected_clip_count": len(set(base) - changed),
        "unaffected_clip_metadata_mismatches": unaffected_mismatches,
        "changed": changed_rows,
        "max_quaternion_norm_error": max_quaternion_norm_error,
        "checks": checks,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(
        f"status={result['status']} changed={len(changed_rows)} "
        f"unaffected={result['unaffected_clip_count']} "
        f"max_quat_error={max_quaternion_norm_error:.6g} output={args.output}"
    )
    return 0 if result["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
