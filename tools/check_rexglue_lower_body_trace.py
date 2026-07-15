#!/usr/bin/env python3
"""Validate a focused RexGlue lower-body trace capture.

This is intentionally narrow: it checks that a gh2test trace did more than
boot and actually emitted the leg/pose evidence needed for lower-body work.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


LOWER_BODY_CHANNELS = (
    "bone_pelvis.pos",
    "bone_L-thigh.quat",
    "bone_R-thigh.quat",
    "bone_L-ankle.quat",
    "bone_R-ankle.quat",
    "bone_L-knee.rotz",
    "bone_R-knee.rotz",
    "bone_L-toe.rotz",
    "bone_R-toe.rotz",
)

KINDS = (
    "anim.lower_body.memory",
    "anim.lower_body.neighborhood",
    "anim.lower_body.runtime_memory",
    "anim.apply.weighted",
    "anim.apply.unweighted",
    "anim.samples.eval.before",
    "anim.samples.pose796.before",
    "anim.pose.apply_weighted_source",
    "anim.pose.apply_interp_source",
    "anim.controller.stage_821D1190",
    "anim.controller.stage_821D1710",
    "char.ik.update",
    "camera.camshot.update",
    "camera.camshot.blend",
    "crowd.world.update",
    "lighting.preset.update",
    "input.scripted_nav",
    "input.scripted_nav.poll",
    "input.guitar_edge",
    "input.capabilities",
    "input.guitar_port.poll",
    "input.guitar_input.poll",
    "input.extract_state",
    "input.joypad.scan",
    "input.signin_state",
    "input.frame_tick",
    "input.xam_state",
    "route.pause_ui_preload_file",
    "route.song_select",
    "route.campaign_start",
    "route.campaign_is_song_current",
    "route.campaign_set_current_song",
    "route.campaign_preload_hud",
    "route.song_loader_start",
    "route.song_loader_process",
    "route.song_loader_trigger",
    "route.song_flow_822A0408",
    "route.scene_dispatch_82379738",
    "route.scene_queue_82378BB0",
    "route.scene_update_82377210",
)

POSE_APPLY_ROUTE_MARKERS = (
    "anim.apply.weighted",
    "anim.apply.unweighted",
    "anim.samples.eval.before",
    "anim.samples.pose796.before",
    "anim.pose.apply_weighted_source",
    "anim.pose.apply_interp_source",
)

SCENE_ROUTE_MARKERS = (
    "camera.camshot.update",
    "camera.camshot.blend",
    "crowd.world.update",
)

SONG_ROUTE_MARKERS = (
    "route.song_select",
    "route.campaign_start",
    "route.campaign_is_song_current",
    "route.campaign_set_current_song",
    "route.campaign_preload_hud",
    "route.song_loader_start",
    "route.song_loader_process",
    "route.song_loader_trigger",
    "route.song_flow_822A0408",
    "route.scene_dispatch_82379738",
    "route.scene_queue_82378BB0",
    "route.scene_update_82377210",
)


def load_events(path: Path, allow_truncated_tail: bool) -> tuple[list[dict[str, object]], int]:
    events: list[dict[str, object]] = []
    invalid_lines = 0
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    total = len(lines)
    for line_no, line in enumerate(lines, start=1):
        line = line.strip()
        if not line:
            continue
        try:
            event = json.loads(line)
        except json.JSONDecodeError as exc:
            if allow_truncated_tail and line_no == total:
                invalid_lines += 1
                continue
            raise SystemExit(f"FAIL invalid json at {path}:{line_no}: {exc}") from exc
        events.append(event)
    return events, invalid_lines


def count_kinds(events: list[dict[str, object]]) -> dict[str, int]:
    counts = {kind: 0 for kind in KINDS}
    for event in events:
        kind = str(event.get("kind", ""))
        if kind in counts:
            counts[kind] += 1
    return counts


def detail_text(events: list[dict[str, object]], kind: str) -> list[str]:
    return [str(event.get("detail", "")) for event in events if event.get("kind") == kind]


def file_open_paths(events: list[dict[str, object]]) -> list[str]:
    return [
        str(event.get("path", ""))
        for event in events
        if event.get("kind") == "file.open"
    ]


def final_event_kind(events: list[dict[str, object]]) -> str:
    for event in reversed(events):
        kind = str(event.get("kind", ""))
        if kind:
            return kind
    return ""


def count_stack_tags(events: list[dict[str, object]], prefix: str) -> int:
    return sum(
        1
        for event in events
        if event.get("kind") == "stack" and str(event.get("tag", "")).startswith(prefix)
    )


def route_status(pose_apply_events: int, scene_route_markers: int) -> str:
    if pose_apply_events:
        return "pose_apply_route_reached"
    if scene_route_markers:
        return "scene_marker_without_apply"
    return "route_not_reached"


def song_route_status(counts: dict[str, int]) -> str:
    if counts["route.scene_dispatch_82379738"]:
        return "scene_dispatch_82379738_reached"
    if counts["route.scene_queue_82378BB0"]:
        return "scene_queue_82378BB0_reached"
    if counts["route.scene_update_82377210"]:
        return "scene_update_82377210_reached"
    if counts["route.song_flow_822A0408"]:
        return "song_flow_822A0408_reached"
    if counts["route.song_loader_trigger"]:
        return "song_loader_trigger_reached"
    if counts["route.song_loader_process"]:
        return "song_loader_process_reached"
    if counts["route.song_loader_start"]:
        return "song_loader_start_reached"
    if counts["route.campaign_preload_hud"]:
        return "campaign_preload_hud_reached"
    if counts["route.campaign_set_current_song"]:
        return "campaign_set_current_song_reached"
    if counts["route.campaign_is_song_current"]:
        return "campaign_is_song_current_reached"
    if counts["route.campaign_start"]:
        return "campaign_start_reached"
    if counts["route.song_select"]:
        return "song_select_reached"
    return "song_route_not_reached"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("--require-runtime-memory", action="store_true")
    parser.add_argument("--require-clip-apply", action="store_true")
    parser.add_argument("--require-lower-body-rows", action="store_true")
    parser.add_argument("--require-neighborhood", action="store_true")
    parser.add_argument("--require-in-song-route", action="store_true")
    parser.add_argument("--require-scripted-nav-polls", action="store_true")
    parser.add_argument("--require-guitar-input-edge", action="store_true")
    parser.add_argument("--require-xam-state", action="store_true")
    parser.add_argument("--require-pause-ui-preload-stack", action="store_true")
    parser.add_argument("--require-song-route-marker", action="store_true")
    parser.add_argument("--allow-truncated-tail", action="store_true")
    parser.add_argument("--write-summary", type=Path)
    args = parser.parse_args()

    events, invalid_lines = load_events(args.trace, args.allow_truncated_tail)
    counts = count_kinds(events)
    failures: list[str] = []

    if not events:
        failures.append("trace has no events")
    if invalid_lines:
        failures.append("trace ended with a truncated json line")
    if args.require_runtime_memory and counts["anim.lower_body.runtime_memory"] <= 0:
        failures.append("missing anim.lower_body.runtime_memory")
    if args.require_clip_apply and (
        counts["anim.apply.weighted"] + counts["anim.apply.unweighted"] <= 0
    ):
        failures.append("missing CharClipSamples apply hooks")
    pose_apply_route_events = sum(counts[kind] for kind in POSE_APPLY_ROUTE_MARKERS)
    scene_route_markers = sum(counts[kind] for kind in SCENE_ROUTE_MARKERS)
    song_route_events = sum(counts[kind] for kind in SONG_ROUTE_MARKERS)
    opened_paths = file_open_paths(events)
    pause_ui_preload_events = sum(
        1
        for path in opened_paths
        if "pause_controller.milo" in path.replace("\\", "/")
        or "pract_pause.milo" in path.replace("\\", "/")
    )
    pause_ui_preload_stack_samples = count_stack_tags(
        events, "file.pause_ui_preload:"
    )
    status = route_status(pose_apply_route_events, scene_route_markers)
    song_status = song_route_status(counts)
    if args.require_in_song_route and pose_apply_route_events <= 0:
        failures.append("missing pose/apply in-song route markers")
    if args.require_song_route_marker and song_route_events <= 0:
        failures.append("missing song route markers")
    if args.require_scripted_nav_polls and counts["input.scripted_nav.poll"] <= 0:
        failures.append("missing scripted nav poll heartbeat")
    if args.require_guitar_input_edge and counts["input.guitar_edge"] <= 0:
        failures.append("missing GuitarPort input edge")
    if args.require_xam_state and counts["input.xam_state"] <= 0:
        failures.append("missing raw XamInputGetState rows")
    if args.require_pause_ui_preload_stack and pause_ui_preload_stack_samples <= 0:
        failures.append("missing pause-UI preload file stack sample")
    lower_body_memory = detail_text(events, "anim.lower_body.memory")
    found_channels = sorted(
        channel
        for channel in LOWER_BODY_CHANNELS
        if any(channel in detail for detail in lower_body_memory)
    )
    if args.require_lower_body_rows and not found_channels:
        failures.append("missing named lower-body channel rows")
    if args.require_neighborhood and counts["anim.lower_body.neighborhood"] <= 0:
        failures.append("missing lower-body pose-table neighborhood dumps")

    summary = {
        "trace": str(args.trace),
        "events": len(events),
        "invalid_lines": invalid_lines,
        "counts": counts,
        "strong_in_song_events": pose_apply_route_events,
        "pose_apply_route_events": pose_apply_route_events,
        "scene_route_markers": scene_route_markers,
        "song_route_events": song_route_events,
        "song_route_status": song_status,
        "song_route_counts": {
            kind: counts[kind] for kind in SONG_ROUTE_MARKERS
        },
        "pause_ui_preload_events": pause_ui_preload_events,
        "pause_ui_preload_file_markers": counts["route.pause_ui_preload_file"],
        "pause_ui_preload_stack_samples": pause_ui_preload_stack_samples,
        "route_status": status,
        "final_event": final_event_kind(events),
        "scripted_nav_polls": counts["input.scripted_nav.poll"],
        "input_guitar_edges": counts["input.guitar_edge"],
        "input_capabilities": counts["input.capabilities"],
        "input_guitar_port_polls": counts["input.guitar_port.poll"],
        "input_guitar_input_polls": counts["input.guitar_input.poll"],
        "input_extract_state": counts["input.extract_state"],
        "input_joypad_scans": counts["input.joypad.scan"],
        "input_signin_state": counts["input.signin_state"],
        "input_frame_ticks": counts["input.frame_tick"],
        "input_xam_states": counts["input.xam_state"],
        "lower_body_channels": found_channels,
        "first_lower_body_neighborhood": (
            detail_text(events, "anim.lower_body.neighborhood") or [""]
        )[0][:300],
        "first_scripted_nav_poll": (
            detail_text(events, "input.scripted_nav.poll") or [""]
        )[0][:300],
        "first_guitar_edge": (detail_text(events, "input.guitar_edge") or [""])[0][:300],
        "first_capabilities": (detail_text(events, "input.capabilities") or [""])[0][
            :300
        ],
        "first_guitar_port_poll": (
            detail_text(events, "input.guitar_port.poll") or [""]
        )[0][:300],
        "first_guitar_input_poll": (
            detail_text(events, "input.guitar_input.poll") or [""]
        )[0][:300],
        "first_extract_state": (detail_text(events, "input.extract_state") or [""])[
            0
        ][:300],
        "first_joypad_scan": (detail_text(events, "input.joypad.scan") or [""])[
            0
        ][:300],
        "first_signin_state": (detail_text(events, "input.signin_state") or [""])[
            0
        ][:300],
        "first_frame_tick": (detail_text(events, "input.frame_tick") or [""])[0][
            :300
        ],
        "first_xam_state": (detail_text(events, "input.xam_state") or [""])[0][
            :300
        ],
        "first_pause_ui_preload_marker": (
            detail_text(events, "route.pause_ui_preload_file") or [""]
        )[0][:300],
        "first_pause_ui_preload_file": (
            [
                path
                for path in opened_paths
                if "pause_controller.milo" in path.replace("\\", "/")
                or "pract_pause.milo" in path.replace("\\", "/")
            ]
            or [""]
        )[0][:300],
        "first_song_select": (detail_text(events, "route.song_select") or [""])[
            0
        ][:300],
        "first_campaign_start": (
            detail_text(events, "route.campaign_start") or [""]
        )[0][:300],
        "first_song_loader_start": (
            detail_text(events, "route.song_loader_start") or [""]
        )[0][:300],
        "first_song_loader_process": (
            detail_text(events, "route.song_loader_process") or [""]
        )[0][:300],
        "first_song_loader_trigger": (
            detail_text(events, "route.song_loader_trigger") or [""]
        )[0][:300],
        "first_song_flow_822A0408": (
            detail_text(events, "route.song_flow_822A0408") or [""]
        )[0][:300],
        "first_scene_dispatch_82379738": (
            detail_text(events, "route.scene_dispatch_82379738") or [""]
        )[0][:300],
        "first_scene_queue_82378BB0": (
            detail_text(events, "route.scene_queue_82378BB0") or [""]
        )[0][:300],
        "first_scene_update_82377210": (
            detail_text(events, "route.scene_update_82377210") or [""]
        )[0][:300],
        "first_runtime_memory": (detail_text(events, "anim.lower_body.runtime_memory") or [""])[
            0
        ][:300],
        "first_lower_body_rows": (lower_body_memory or [""])[0][:300],
        "result": "fail" if failures else "pass",
        "failures": failures,
    }

    if args.write_summary:
        args.write_summary.parent.mkdir(parents=True, exist_ok=True)
        args.write_summary.write_text(
            json.dumps(summary, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    print(
        "REXGLUE-LOWER-BODY "
        f"events={len(events)} "
        f"invalid={invalid_lines} "
        f"runtime={counts['anim.lower_body.runtime_memory']} "
        f"neighborhood={counts['anim.lower_body.neighborhood']} "
        f"apply={counts['anim.apply.weighted'] + counts['anim.apply.unweighted']} "
        f"rows={len(found_channels)} "
        f"pose_route={pose_apply_route_events} "
        f"scene_route={scene_route_markers} "
        f"song_route={song_route_events} "
        f"song_select={counts['route.song_select']} "
        f"campaign_start={counts['route.campaign_start']} "
        f"song_loader_start={counts['route.song_loader_start']} "
        f"song_loader_process={counts['route.song_loader_process']} "
        f"song_loader_trigger={counts['route.song_loader_trigger']} "
        f"song_flow_822A0408={counts['route.song_flow_822A0408']} "
        f"scene_dispatch_82379738={counts['route.scene_dispatch_82379738']} "
        f"scene_queue_82378BB0={counts['route.scene_queue_82378BB0']} "
        f"scene_update_82377210={counts['route.scene_update_82377210']} "
        f"pause_ui_preload={pause_ui_preload_events} "
        f"pause_ui_preload_markers={counts['route.pause_ui_preload_file']} "
        f"pause_ui_preload_stacks={pause_ui_preload_stack_samples} "
        f"scripted_nav={counts['input.scripted_nav']} "
        f"scripted_nav_polls={counts['input.scripted_nav.poll']} "
        f"guitar_edges={counts['input.guitar_edge']} "
        f"capabilities={counts['input.capabilities']} "
        f"guitar_port_polls={counts['input.guitar_port.poll']} "
        f"guitar_input_polls={counts['input.guitar_input.poll']} "
        f"extract_state={counts['input.extract_state']} "
        f"joypad_scans={counts['input.joypad.scan']} "
        f"signin_state={counts['input.signin_state']} "
        f"frame_ticks={counts['input.frame_tick']} "
        f"xam_states={counts['input.xam_state']} "
        f"route_status={status} "
        f"song_route_status={song_status} "
        f"result={summary['result']}"
    )
    for kind in KINDS:
        print(f"  {kind}: {counts[kind]}")
    if found_channels:
        print("  channels: " + ", ".join(found_channels))
    if failures:
        for failure in failures:
            print(f"FAIL {failure}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
