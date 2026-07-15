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


def check_manifest(
    manifest_path: Path, cross_check_summaries: bool
) -> tuple[int, int, int, int, int, int, int, int, int, int]:
    manifest = load_json(manifest_path)
    require(
        manifest.get("trace_id") == "rexglue_lower_body_trace_scaffold_20260715",
        "unexpected RexGlue trace scaffold id",
    )
    require(manifest.get("trace_commit") == "5fd980b", "unexpected RexGlue trace commit")
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
    require(
        scaffold.get("neighborhood_event") == "anim.lower_body.neighborhood",
        "missing lower-body neighborhood event",
    )
    require(scaffold.get("scripted_nav_flag") == "--trace_scripted_nav", "missing nav flag")
    require(
        scaffold.get("route_reachability_checker")
        == "tools/check_rexglue_lower_body_trace.py --require-in-song-route",
        "missing RexGlue route reachability checker",
    )
    require(
        scaffold.get("input_poll_checker")
        == "tools/check_rexglue_lower_body_trace.py --require-scripted-nav-polls --require-guitar-input-edge",
        "missing RexGlue input poll checker",
    )
    require(
        scaffold.get("pause_ui_preload_checker")
        == "tools/check_rexglue_lower_body_trace.py --require-pause-ui-preload-stack",
        "missing RexGlue pause-UI preload checker",
    )
    require(
        scaffold.get("neighborhood_checker")
        == "tools/check_rexglue_lower_body_trace.py --require-neighborhood",
        "missing RexGlue neighborhood checker",
    )
    require(scaffold.get("no_focus_forced") is True, "RexGlue trace must not force focus")
    hooks = set(scaffold.get("hooks", []))
    for hook in ("sub_8215DF28", "sub_8215E6A0", "821D1190", "821D1710", "CharIK_Update"):
        require(hook in hooks, f"missing RexGlue hook {hook}")

    captures = manifest.get("captures", [])
    require(len(captures) == 9, "expected nine current RexGlue trace summaries")
    runtime_total = 0
    neighborhood_total = 0
    accepted_count = 0
    pose_apply_route_total = 0
    scene_route_total = 0
    legacy_controller_gate_total = 0
    pause_ui_preload_total = 0
    scripted_nav_poll_total = 0
    input_guitar_edge_total = 0
    saw_truncated = False
    saw_clean_shutdown = False
    saw_legacy_pause_ui_preload = False
    saw_pause_ui_preload_stack = False
    saw_scripted_nav_single_poll = False
    saw_poll_heartbeat = False
    saw_guitar_edge = False
    for capture in captures:
        require(
            int(capture.get("runtime_memory_events", 0)) > 0,
            "missing runtime memory events",
        )
        require(
            int(capture.get("pose_table_neighborhood_events", 0)) == 0,
            "current RexGlue capture must not claim pose-table neighborhoods before clip apply rows",
        )
        require(capture.get("clip_apply_events") == 0, "current RexGlue capture must not claim clip apply rows")
        require(capture.get("lower_body_rows") == 0, "current RexGlue capture must not claim lower-body rows")
        require(
            int(capture.get("input_guitar_edges", 0)) in (0, 1),
            "current RexGlue capture has an unexpected GuitarPort edge count",
        )
        require(
            capture.get("route_status")
            in (
                "route_not_reached",
                "controller_gate_without_apply",
                "scene_marker_without_apply",
                "pose_apply_route_reached",
            ),
            "current RexGlue capture has an unexpected route status",
        )
        require(capture.get("accepted_row_oracle") is False, "current RexGlue capture must be non-authoritative")
        runtime_total += int(capture.get("runtime_memory_events", 0))
        neighborhood_total += int(capture.get("pose_table_neighborhood_events", 0))
        pose_apply_route_total += int(capture.get("strong_in_song_events", 0))
        scene_route_total += int(capture.get("scene_route_markers", 0))
        legacy_controller_gate_total += int(capture.get("controller_gate_events", 0))
        pause_ui_preload_total += int(capture.get("pause_ui_preload_events", 0))
        scripted_nav_poll_total += int(capture.get("scripted_nav_polls", 0))
        input_guitar_edge_total += int(capture.get("input_guitar_edges", 0))
        accepted_count += 1 if capture.get("accepted_row_oracle") else 0
        if capture.get("scripted_nav_events") == 1:
            saw_scripted_nav_single_poll = True
        if capture.get("scripted_nav_polls") == 4:
            saw_poll_heartbeat = True
        if capture.get("input_guitar_edges") == 1:
            saw_guitar_edge = True
            require(
                "engine_word=0x00000040" in capture.get("first_guitar_edge", "")
                or "engine_word=0x00000042" in capture.get("first_guitar_edge", ""),
                "RexGlue GuitarPort edge must record the remapped A edge, with optional scripted strum",
            )
        failures = capture.get("failures", [])
        if "trace ended with a truncated json line" in failures:
            saw_truncated = True
        if capture.get("invalid_lines") == 0 and capture.get("final_event") == "capture.off":
            saw_clean_shutdown = True
        if capture.get("controller_gate_events") == 1:
            require(
                capture.get("route_status") == "controller_gate_without_apply",
                "legacy pause-UI preload capture must preserve its historical route status",
            )
            require(
                capture.get("clip_apply_events") == 0
                and capture.get("lower_body_rows") == 0,
                "legacy pause-UI preload capture is not allowed to claim row authority",
            )
            require(
                "pause_controller.milo_xbox"
                in capture.get("first_controller_gate_file", ""),
                "legacy pause-UI preload capture must name pause_controller.milo_xbox",
            )
            require(
                int(capture.get("legacy_misclassified_pause_ui_preload_events", 0)) == 1,
                "legacy controller-gate label must be marked as misclassified pause-UI preload",
            )
            saw_legacy_pause_ui_preload = True

        if capture.get("pause_ui_preload_events"):
            require(
                capture.get("route_status") == "route_not_reached",
                "corrected pause-UI preload capture must not claim route reachability",
            )
            require(
                int(capture.get("pause_ui_preload_stack_samples", 0)) > 0,
                "corrected pause-UI preload capture must carry a stack sample",
            )
            require(
                "pause_controller.milo_xbox"
                in capture.get("first_pause_ui_preload_file", ""),
                "corrected pause-UI preload capture must name pause_controller.milo_xbox",
            )
            saw_pause_ui_preload_stack = True

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
                counts.get("anim.lower_body.neighborhood", 0)
                == capture.get("pose_table_neighborhood_events", 0),
                "summary lower-body neighborhood count mismatch",
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
            require(
                summary.get("strong_in_song_events")
                == capture.get("strong_in_song_events"),
                "summary pose/apply route event count mismatch",
            )
            if "scene_route_markers" in capture:
                require(
                    summary.get("scene_route_markers")
                    == capture.get("scene_route_markers"),
                    "summary scene route marker count mismatch",
                )
            if "controller_gate_events" in capture:
                require(
                    summary.get("controller_gate_events")
                    == capture.get("controller_gate_events"),
                    "summary legacy controller gate count mismatch",
                )
            if "pause_ui_preload_events" in capture:
                require(
                    summary.get("pause_ui_preload_events")
                    == capture.get("pause_ui_preload_events"),
                    "summary pause-UI preload count mismatch",
                )
                require(
                    summary.get("pause_ui_preload_stack_samples")
                    == capture.get("pause_ui_preload_stack_samples"),
                    "summary pause-UI preload stack count mismatch",
                )
            require(
                summary.get("route_status") == capture.get("route_status"),
                "summary route status mismatch",
            )
            require(
                counts.get("input.scripted_nav", 0)
                == capture.get("scripted_nav_events"),
                "summary scripted nav event count mismatch",
            )
            require(
                summary.get("scripted_nav_polls")
                == capture.get("scripted_nav_polls"),
                "summary scripted nav poll count mismatch",
            )
            require(
                summary.get("input_guitar_edges")
                == capture.get("input_guitar_edges"),
                "summary guitar input edge count mismatch",
            )
            if "invalid_lines" in capture:
                require(
                    summary.get("invalid_lines") == capture.get("invalid_lines"),
                    "summary invalid-line count mismatch",
                )

    require(saw_truncated, "expected the scripted RexGlue attempt to record the truncated-tail failure")
    require(saw_clean_shutdown, "expected a clean RexGlue shutdown trace with capture.off")
    require(saw_legacy_pause_ui_preload, "expected legacy RexGlue captures to be marked as pause-UI preload")
    require(saw_pause_ui_preload_stack, "expected a corrected pause-UI preload stack capture")
    require(saw_scripted_nav_single_poll, "expected scripted RexGlue attempt to record exactly one scripted-nav poll")
    require(saw_poll_heartbeat, "expected current RexGlue attempts to record scripted-nav poll heartbeats")
    require(saw_guitar_edge, "expected current RexGlue attempts to prove a GuitarPort input edge")
    require(pose_apply_route_total == 0, "current RexGlue captures must not claim pose/apply route markers")
    require(scene_route_total == 1, "current RexGlue captures must record the single scene marker separately")
    require(legacy_controller_gate_total == 2, "current RexGlue captures must preserve two legacy pause-UI preload labels")
    require(pause_ui_preload_total == 2, "current RexGlue captures must record two corrected pause-UI preload files")
    require(neighborhood_total == 0, "current RexGlue captures must have zero accepted pose-table neighborhoods")
    require(input_guitar_edge_total == 5, "current RexGlue captures must have exactly five proven GuitarPort input edges")
    require(accepted_count == 0, "no current RexGlue capture may be accepted as a row oracle")
    return (
        runtime_total,
        neighborhood_total,
        accepted_count,
        pose_apply_route_total,
        scene_route_total,
        legacy_controller_gate_total,
        pause_ui_preload_total,
        scripted_nav_poll_total,
        input_guitar_edge_total,
        1 if saw_clean_shutdown else 0,
    )


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
        (
            runtime_total,
            neighborhood_total,
            accepted_count,
            pose_apply_route_total,
            scene_route_total,
            legacy_controller_gate_total,
            pause_ui_preload_total,
            scripted_nav_poll_total,
            input_guitar_edge_total,
            clean_shutdown_count,
        ) = check_manifest(args.manifest, args.cross_check_summaries)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_rexglue_trace_manifest "
        f"runtime_memory_events={runtime_total} "
        f"pose_table_neighborhood_events={neighborhood_total} "
        f"pose_apply_route_events={pose_apply_route_total} "
        f"scene_route_markers={scene_route_total} "
        f"legacy_pause_ui_preload_events={legacy_controller_gate_total} "
        f"pause_ui_preload_events={pause_ui_preload_total} "
        f"scripted_nav_polls={scripted_nav_poll_total} "
        f"guitar_edges={input_guitar_edge_total} "
        f"clean_shutdown_traces={clean_shutdown_count} "
        f"accepted_row_oracles={accepted_count} "
        "apply_status=apply_rows_not_reached "
        "route_status=route_not_reached_until_pose_apply "
        "accepted_live_row_authority=pcsx2_rock_lower_body_mesh_rows_20260715"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
