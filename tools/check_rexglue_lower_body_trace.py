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
)

STRONG_IN_SONG_MARKERS = (
    "anim.apply.weighted",
    "anim.apply.unweighted",
    "anim.samples.eval.before",
    "anim.samples.pose796.before",
    "anim.pose.apply_weighted_source",
    "anim.pose.apply_interp_source",
    "camera.camshot.update",
    "camera.camshot.blend",
    "crowd.world.update",
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("--require-runtime-memory", action="store_true")
    parser.add_argument("--require-clip-apply", action="store_true")
    parser.add_argument("--require-lower-body-rows", action="store_true")
    parser.add_argument("--require-in-song-route", action="store_true")
    parser.add_argument("--require-scripted-nav-polls", action="store_true")
    parser.add_argument("--require-guitar-input-edge", action="store_true")
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
    strong_in_song_events = sum(counts[kind] for kind in STRONG_IN_SONG_MARKERS)
    if args.require_in_song_route and strong_in_song_events <= 0:
        failures.append("missing strong in-song animation/camera/crowd route markers")
    if args.require_scripted_nav_polls and counts["input.scripted_nav.poll"] <= 0:
        failures.append("missing scripted nav poll heartbeat")
    if args.require_guitar_input_edge and counts["input.guitar_edge"] <= 0:
        failures.append("missing GuitarPort input edge")
    lower_body_memory = detail_text(events, "anim.lower_body.memory")
    found_channels = sorted(
        channel
        for channel in LOWER_BODY_CHANNELS
        if any(channel in detail for detail in lower_body_memory)
    )
    if args.require_lower_body_rows and not found_channels:
        failures.append("missing named lower-body channel rows")

    summary = {
        "trace": str(args.trace),
        "events": len(events),
        "invalid_lines": invalid_lines,
        "counts": counts,
        "strong_in_song_events": strong_in_song_events,
        "route_status": "in_song_route_reached" if strong_in_song_events else "route_not_reached",
        "scripted_nav_polls": counts["input.scripted_nav.poll"],
        "input_guitar_edges": counts["input.guitar_edge"],
        "lower_body_channels": found_channels,
        "first_scripted_nav_poll": (
            detail_text(events, "input.scripted_nav.poll") or [""]
        )[0][:300],
        "first_guitar_edge": (detail_text(events, "input.guitar_edge") or [""])[0][:300],
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
        f"apply={counts['anim.apply.weighted'] + counts['anim.apply.unweighted']} "
        f"rows={len(found_channels)} "
        f"insong={strong_in_song_events} "
        f"scripted_nav={counts['input.scripted_nav']} "
        f"scripted_nav_polls={counts['input.scripted_nav.poll']} "
        f"guitar_edges={counts['input.guitar_edge']} "
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
