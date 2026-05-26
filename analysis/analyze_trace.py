#!/usr/bin/env python3
"""Mine a trace_*.jsonl for the 1:1-port analysis.

The current trace only carries three structured event kinds (file.open,
prop.lookup, handler.lookup). This script processes one trace and emits
a report covering:

  1) Boot vs in-song boundary (large time gaps + first song asset open).
  2) Per-subsystem evidence — for each port-scope subsystem (audio,
     lighting, venue, character, crowd, camera, hud, vfx, scoring,
     input), list which paths / props / handlers in the trace map to
     that subsystem, plus an explicit "BLIND" note for subsystems with
     zero evidence (so we know which next hooks to add).
  3) Hot in-song props with returned_ptr cardinality (low ptr count
     = a single (class,key); high = key used across many classes).
  4) In-song per-second event rate to confirm sustained gameplay.

Run:
  python analysis/analyze_trace.py captures_archive/trace_surrender_30s.jsonl.gz
"""

from __future__ import annotations

import gzip
import io
import json
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path


# ---------------- subsystem keyword tables -----------------------------------
# Path-substring -> subsystem. First match wins. Lowercased.
PATH_SUBSYSTEM = [
    # audio
    ("songs/", "audio.song"),
    (".mogg", "audio.song"),
    (".mid", "audio.song"),
    (".voc", "audio.song"),
    ("crowd_v1_", "audio.crowd"),
    ("crowd", "audio.crowd"),
    ("synth_", "audio.synth"),
    # character + animation
    ("char/", "character"),
    ("anims/", "character.anim"),
    ("/skin", "character"),
    # venue / world
    ("world/", "venue"),
    ("ui/", "hud"),
    # config / data
    ("config/gen/", "config"),
    ("system/run/", "config"),
]

# Prop-name -> subsystem. Many props are ambiguous; this maps the clearest.
PROP_SUBSYSTEM = {
    "poll": "input",
    "or": "input",
    "detect": "input",
    "analog": "input",
    "ignore_buttons": "input",
    "is_missing_controller": "input",
    "is_multiple_controllers": "input",
    "ro_guitar_xbox": "input.guitar",
    "battle": "song_state.battle",
    "competitive": "song_state.battle",
    "small1": "?",          # unknown — needs investigation
    "required_songs": "song_state",
    "store": "menu.store",
    "disabled": "?",
    "skin": "character",
    "skins": "character",
    "max_width": "hud.layout",
    "highlight": "hud.layout",
    "focus_scale": "hud.layout",
    "focus": "hud.layout",
    "character": "character",
    "outfit": "character",
    "objects": "scene_graph",
    "superclasses": "type_system",
    "kick_drum": "audio.drums",
    "bass_hit": "audio.drums",
    "beat": "song_clock",
    "type": "type_system",
    "enable": "?",
    "normal": "?",
}

# Handler-name -> subsystem.
HANDLER_SUBSYSTEM = {
    "rate":         "engine.rate",
    "heap":         "engine.heap",
    "stats":        "engine.stats",
    "timers":       "engine.timers",
    "output":       "engine.output",
    "input":        "input",
    "paramedit":    "debug.paramedit",
    "synth_hud":    "audio.synth.hud",
    "time":         "song_clock",
    "char_test":    "character.test",
    "score":        "score",
    "guitar":       "input.guitar",
    "char_status":  "character.status",
    "char_history": "character.history",
}

# Subsystems we KNOW must exist for a 1:1 port; if we have no evidence
# for them in a trace we flag BLIND.
REQUIRED_SUBSYSTEMS = [
    "audio.song",
    "audio.crowd",
    "audio.drums",
    "audio.synth",
    "input",
    "input.guitar",
    "song_clock",
    "song_state",
    "score",
    "character",
    "character.anim",
    "venue",
    "hud",
    "scene_graph",
    "lighting",       # we don't yet see any
    "camera",         # we don't yet see any
    "vfx",            # we don't yet see any
    "particles",      # we don't yet see any
    "star_power",     # we don't yet see any
    "fail_meter",     # we don't yet see any
]


# ---------------- loader -----------------------------------------------------

def open_trace(path: Path) -> io.TextIOBase:
    if path.suffix == ".gz":
        return io.TextIOWrapper(gzip.open(path, "rb"), encoding="utf-8")
    return open(path, "r", encoding="utf-8")


@dataclass
class TraceData:
    file_opens: list = field(default_factory=list)  # (t, path, off, size, found)
    props:      list = field(default_factory=list)  # (t, prop, ret)
    handlers:   list = field(default_factory=list)  # (t, name, ret)
    init_t:     int  = 0
    capture_on_t: int = 0

def load(path: Path) -> TraceData:
    td = TraceData()
    with open_trace(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                ev = json.loads(line)
            except json.JSONDecodeError:
                # last line may be truncated if process was killed mid-write
                continue
            t = ev.get("t", 0)
            k = ev.get("kind", "")
            if k == "file.open":
                td.file_opens.append((t, ev.get("path", ""),
                                      ev.get("off", 0), ev.get("size", 0),
                                      ev.get("found", False)))
            elif k == "prop.lookup":
                td.props.append((t, ev.get("prop", ""), ev.get("ret", "")))
            elif k == "handler.lookup":
                td.handlers.append((t, ev.get("name", ""), ev.get("ret", "")))
            elif k == "trace.init":
                td.init_t = t
            elif k == "capture.on":
                td.capture_on_t = t
    return td


# ---------------- analysis ---------------------------------------------------

def classify_path(p: str) -> str:
    pl = p.lower()
    for sub, name in PATH_SUBSYSTEM:
        if sub in pl:
            return name
    return "?"


def find_song_start_ns(td: TraceData) -> int | None:
    """First file.open whose path includes 'songs/' (excluding ui/sel_song)."""
    for t, p, *_ in td.file_opens:
        if p.startswith("songs/") and not p.endswith(".dtb"):
            return t
    return None


def per_second_event_rate(td: TraceData, start_ns: int, end_ns: int,
                          step_ns: int = 1_000_000_000):
    """Histogram all events into [start_ns, end_ns) buckets of step_ns."""
    bins: Counter = Counter()
    for t, *_ in td.props:
        if start_ns <= t < end_ns:
            bins[(t - start_ns) // step_ns] += 1
    return bins


def detect_frame_period(td: TraceData, start_ns: int, end_ns: int):
    """Find the dominant inter-burst period within the in-song window.

    Strategy: walk prop.lookup events in time order; a 'frame boundary'
    is any gap > a threshold (5ms — much larger than the few-microsec
    gaps inside a frame burst, much smaller than 16.7ms inter-frame).
    Distribution of gap sizes should cluster around the frame period.
    """
    gaps_ns = []
    prev = None
    for t, *_ in td.props:
        if not (start_ns <= t <= end_ns):
            continue
        if prev is not None:
            g = t - prev
            if g > 500_000:  # 0.5ms = drop intra-burst noise
                gaps_ns.append(g)
        prev = t
    if not gaps_ns:
        return None
    gaps_ns.sort()
    n = len(gaps_ns)
    return {
        "n_boundaries":     n,
        "p50_ms":           gaps_ns[n//2] / 1e6,
        "p90_ms":           gaps_ns[int(n*0.9)] / 1e6,
        "p99_ms":           gaps_ns[int(n*0.99)] / 1e6,
        "min_ms":           gaps_ns[0] / 1e6,
        "max_ms":           gaps_ns[-1] / 1e6,
    }


def per_frame_signature(td: TraceData, start_ns: int, end_ns: int,
                        gap_threshold_ns: int = 5_000_000):
    """Group prop.lookup events into frames by inter-event gap.

    Returns: list of frames; each frame is a list of (offset_ns, prop) tuples.
    A 'frame' is bounded by any gap > gap_threshold_ns (default 5ms).
    """
    frames = []
    cur = []
    prev = None
    for t, prop, ret in td.props:
        if not (start_ns <= t <= end_ns):
            continue
        if prev is not None and (t - prev) > gap_threshold_ns:
            if cur:
                frames.append(cur)
            cur = []
        cur.append((t, prop))
        prev = t
    if cur:
        frames.append(cur)
    return frames


def common_burst_pattern(frames, max_frames=200, top_n=30):
    """Find the most common ordered prop sequence across frames.

    Take the first `top_n` props from each frame; count occurrences of
    each unique sequence; return the top patterns.
    """
    sigs: Counter = Counter()
    for f in frames[:max_frames]:
        seq = tuple(p for _, p in f[:top_n])
        sigs[seq] += 1
    return sigs


def per_frame_prop_histogram(frames, max_frames=500):
    """For each prop key, what fraction of frames contain at least one lookup?"""
    if not frames:
        return Counter()
    sample = frames[:max_frames]
    seen_per_frame: Counter = Counter()
    for f in sample:
        for p in {prop for _, prop in f}:
            seen_per_frame[p] += 1
    return seen_per_frame, len(sample)


def report(path: Path):
    td = load(path)
    total_events = len(td.file_opens) + len(td.props) + len(td.handlers) + 2
    last_t = max(
        (td.file_opens[-1][0] if td.file_opens else 0),
        (td.props[-1][0]      if td.props      else 0),
        (td.handlers[-1][0]   if td.handlers   else 0),
    )

    print(f"# Trace report: {path.name}\n")
    print(f"- Total events: {total_events:,}")
    print(f"  - file.open:       {len(td.file_opens):,}")
    print(f"  - prop.lookup:     {len(td.props):,}")
    print(f"  - handler.lookup:  {len(td.handlers):,}")
    print(f"- Wall time: {last_t/1e9:.2f} s")
    print()

    song_t = find_song_start_ns(td)
    if song_t is not None:
        print(f"## Song-start boundary\n")
        print(f"- First non-DTB `songs/` path opened at t={song_t/1e9:.2f}s")
        first_song = next(p for _, p, *_ in td.file_opens if p.startswith("songs/") and not p.endswith(".dtb"))
        print(f"- File: `{first_song}`")
        in_song_window = (song_t, last_t)
    else:
        print("## Song-start boundary: NOT REACHED")
        in_song_window = (0, last_t)
    print()

    # Per-subsystem evidence (paths)
    print("## Per-subsystem evidence (from paths)\n")
    sub_paths: dict[str, set] = defaultdict(set)
    for _, p, *_ in td.file_opens:
        sub_paths[classify_path(p)].add(p)
    for sub in sorted(sub_paths):
        ps = sub_paths[sub]
        print(f"- **{sub}** ({len(ps)} unique paths)")
        for p in sorted(ps)[:5]:
            print(f"    - `{p}`")
        if len(ps) > 5:
            print(f"    - ... +{len(ps)-5} more")
    print()

    # Per-subsystem evidence (props during in-song window)
    print("## Per-subsystem evidence (from in-song prop.lookup)\n")
    prop_freq: Counter = Counter()
    prop_subs: dict[str, set] = defaultdict(set)
    s0, s1 = in_song_window
    for t, prop, ret in td.props:
        if s0 <= t <= s1:
            prop_freq[prop] += 1
            sub = PROP_SUBSYSTEM.get(prop, "?")
            prop_subs[sub].add(prop)
    for sub in sorted(prop_subs):
        ps = prop_subs[sub]
        total = sum(prop_freq[p] for p in ps)
        print(f"- **{sub}** ({len(ps)} keys, {total:,} lookups)")
        for p in sorted(ps, key=lambda x: -prop_freq[x])[:5]:
            print(f"    - `{p}` ({prop_freq[p]:,})")
    print()

    # Handlers
    print("## Handlers queried\n")
    h_counts: Counter = Counter(h[1] for h in td.handlers)
    for name, n in h_counts.most_common():
        sub = HANDLER_SUBSYSTEM.get(name, "?")
        print(f"- `{name}` → {sub}  ({n})")
    print()

    # Blind subsystems
    seen_subs = set(sub_paths.keys()) | set(prop_subs.keys()) | {
        HANDLER_SUBSYSTEM.get(h[1], "?") for h in td.handlers
    }
    print("## BLIND subsystems (required for 1:1, no trace evidence)\n")
    for s in REQUIRED_SUBSYSTEMS:
        if s not in seen_subs:
            print(f"- **{s}** — no events. Need new hooks.")
    print()

    # Per-second event rate during in-song
    print("## Per-second prop.lookup rate (in-song window)\n")
    if song_t is not None:
        bins = per_second_event_rate(td, song_t, last_t)
        for k in sorted(bins):
            print(f"  t+{k:>2}s: {bins[k]:>5}")
    print()

    # Frame period detection
    print("## Frame-boundary detection\n")
    if song_t is not None:
        # Restrict to the steady-state window (after the initial loading
        # bursts, so 3s into the song -- the per-second rate shows it
        # settles by t+3s in the surrender trace).
        steady_start = song_t + 3_000_000_000
        fp = detect_frame_period(td, steady_start, last_t)
        if fp:
            print(f"- Boundaries: {fp['n_boundaries']:,}")
            print(f"- Gap p50: {fp['p50_ms']:.3f} ms  (60Hz = 16.67ms, 30Hz = 33.3ms)")
            print(f"- Gap p90: {fp['p90_ms']:.3f} ms")
            print(f"- Gap p99: {fp['p99_ms']:.3f} ms")
            print(f"- Gap min: {fp['min_ms']:.3f} ms")
            print(f"- Gap max: {fp['max_ms']:.3f} ms")
        # Common burst patterns
        frames = per_frame_signature(td, steady_start, last_t)
        print(f"\n- Frames identified (gap >5ms boundary): {len(frames):,}")
        if frames:
            sizes = sorted(len(f) for f in frames)
            print(f"- Per-frame prop.lookup count p50={sizes[len(sizes)//2]}, p90={sizes[int(len(sizes)*0.9)]}, max={sizes[-1]}")

        # Per-frame prop histogram: which props appear in nearly every frame?
        per_frame_hist, sample_n = per_frame_prop_histogram(frames)
        print(f"\n## Per-frame prop coverage (% of {sample_n} sampled frames containing each prop)\n")
        for prop, n in per_frame_hist.most_common(40):
            pct = 100.0 * n / sample_n
            if pct >= 5:
                print(f"- `{prop}`: {pct:5.1f}% ({n})")

        # Top burst patterns
        sigs = common_burst_pattern(frames, max_frames=200, top_n=8)
        print(f"\n## Most-common 8-prop opening sequences across first 200 frames\n")
        for seq, n in sigs.most_common(10):
            print(f"- {n}x: `{' -> '.join(seq)}`")
    print()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: analyze_trace.py <trace.jsonl[.gz]>")
    report(Path(sys.argv[1]))
