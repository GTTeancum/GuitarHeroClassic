#!/usr/bin/env python3
"""Verify the lower-body RexGlue trace scaffold status.

RexGlue is useful for instrumented memory dumps, but a capture is only a leg
oracle after it reaches CharClipSamples apply rows with named lower-body
channels. This checker keeps that boundary explicit.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve_manifest_path(manifest: Path, raw_path: str) -> Path:
    path = Path(raw_path)
    if path.is_absolute():
        return path
    return (manifest.parent / path).resolve()


def check_manifest(manifest_path: Path, cross_check_summaries: bool) -> tuple[int, int]:
    manifest = load_json(manifest_path)
    require(
        manifest.get("trace_id") == "rexglue_lower_body_trace_scaffold_20260715",
        "unexpected RexGlue trace scaffold id",
    )
    require(manifest.get("trace_commit") == "580e405", "unexpected RexGlue trace commit")
    require(
        manifest.get("accepted_live_row_authority")
        == "pcsx2_rock_lower_body_mesh_rows_20260715",
        "RexGlue manifest must defer accepted live rows to PCSX2",
    )
    require(
        manifest.get("native_path") == "source_output_lower_body_bridge",
        "native path must stay on the source output lower-body bridge",
    )

    scaffold = manifest.get("scaffold", {})
    require(scaffold.get("runtime_flag") == "--trace-lower-body-memory", "missing runtime flag")
    require(scaffold.get("scripted_nav_flag") == "--trace_scripted_nav", "missing nav flag")
    require(scaffold.get("no_focus_forced") is True, "RexGlue trace must not force focus")
    hooks = set(scaffold.get("hooks", []))
    for hook in ("sub_8215DF28", "sub_8215E6A0", "821D1190", "821D1710", "CharIK_Update"):
        require(hook in hooks, f"missing RexGlue hook {hook}")

    captures = manifest.get("captures", [])
    require(len(captures) == 2, "expected two current RexGlue trace summaries")
    runtime_total = 0
    accepted_count = 0
    saw_truncated = False
    for capture in captures:
        require(capture.get("runtime_memory_events") == 192, "unexpected runtime memory count")
        require(capture.get("clip_apply_events") == 0, "current RexGlue capture must not claim clip apply rows")
        require(capture.get("lower_body_rows") == 0, "current RexGlue capture must not claim lower-body rows")
        require(capture.get("accepted_row_oracle") is False, "current RexGlue capture must be non-authoritative")
        runtime_total += int(capture.get("runtime_memory_events", 0))
        accepted_count += 1 if capture.get("accepted_row_oracle") else 0
        failures = capture.get("failures", [])
        if "trace ended with a truncated json line" in failures:
            saw_truncated = True

        if cross_check_summaries:
            summary = load_json(resolve_manifest_path(manifest_path, capture["summary"]))
            counts = summary.get("counts", {})
            require(summary.get("events") == capture.get("events"), "summary event count mismatch")
            require(
                counts.get("anim.lower_body.runtime_memory")
                == capture.get("runtime_memory_events"),
                "summary runtime memory count mismatch",
            )
            require(
                counts.get("anim.apply.weighted", 0)
                + counts.get("anim.apply.unweighted", 0)
                == capture.get("clip_apply_events"),
                "summary clip apply count mismatch",
            )
            require(
                len(summary.get("lower_body_channels", [])) == capture.get("lower_body_rows"),
                "summary lower-body row count mismatch",
            )

    require(saw_truncated, "expected the scripted RexGlue attempt to record the truncated-tail failure")
    require(accepted_count == 0, "no current RexGlue capture may be accepted as a row oracle")
    return runtime_total, accepted_count


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check the current RexGlue lower-body trace scaffold manifest."
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("tools/lower_body_rexglue_trace_manifest.json"),
    )
    parser.add_argument(
        "--cross-check-summaries",
        action="store_true",
        help="Also read the sibling trace360 summary JSON files when present.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        runtime_total, accepted_count = check_manifest(args.manifest, args.cross_check_summaries)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_rexglue_trace_manifest "
        f"runtime_memory_events={runtime_total} "
        f"accepted_row_oracles={accepted_count} "
        "accepted_live_row_authority=pcsx2_rock_lower_body_mesh_rows_20260715"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
