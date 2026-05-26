#!/usr/bin/env python3
"""Extract structural skeleton of a recompiled function.

Given a sub_82XXXXXX, find its definition across generated/*.cpp and emit:
  - Line span + size in PPC instructions (#lines roughly /4)
  - All sub_ calls it makes (with counts)
  - Struct-offset access patterns (REX_LOAD/STORE on rN +offset)
  - Branch label count (cyclomatic complexity proxy)
  - All known string-interning / DataNode / Property / Class calls
  - Whether it's a thin wrapper, a state-machine, a per-frame loop, etc.

Output is a markdown block ready to paste into recomp_symbols.md or
per-subsystem notes.

usage:
  python analysis/sub_skeleton.py sub_82316428 [sub_8236A338 ...]
"""

from __future__ import annotations

import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

GEN = Path(__file__).resolve().parent.parent / "generated"

CALL_RE   = re.compile(r"\b(sub_[0-9A-F]+)\(ctx,\s*base\)")
LOAD_RE   = re.compile(r"REX_LOAD_U(\d+)\(ctx\.(r\d+)\.u32\s*\+\s*(-?\d+)\)")
STORE_RE  = re.compile(r"REX_STORE_U(\d+)\(ctx\.(r\d+)\.u32\s*\+\s*(-?\d+)")
LABEL_RE  = re.compile(r"^\s*(loc_[0-9A-F]+):")

# Functions we've already named -- highlight calls to them.
NAMED = {
    "sub_82120208": "hmx_main_ProgramInit",
    "sub_82120B58": "hmx_main_xstart",
    "sub_82271070": "hmx_App_LoadBootAssets",
    "sub_82271618": "hmx_App_EngineInit",
    "sub_82277878": "hmx_FileMgr_Lookup",
    "sub_82354FD8": "hmx_Mem_Alloc",
    "sub_82355DA8": "hmx_String_CopyOrIntern",
    "sub_8239CD50": "hmx_memset",
    "sub_82691050": "hmx_String_HashMod",
    "sub_82357A10": "hmx_File_ctor",
    "sub_82357D40": "hmx_File_IsMissing",
    "sub_82359250": "hmx_File_Read",
    "sub_823596E8": "hmx_File_InitCrypto",
    "sub_8231C520": "hmx_FileOps_OpenAndParse",
    "sub_821E04B8": "hmx_DataHandler_Find",
    "sub_82270D20": "hmx_ClassReg_Lookup",
    "sub_82319448": "hmx_PropertyTable_Find",
    "sub_82319530": "hmx_PropertyTable_Find0",
    "sub_82316428": "hmx_Object_Update?",
    "sub_82315440": "hmx_GameState_GetProp?",
    "sub_82270D38": "hmx_ClassReg_Lookup_alt",
    "sub_82270D80": "hmx_ClassReg_Lookup_v3",
    "sub_82317FE8": "hmx_DataNode_AsInt?",
    "sub_82317EF8": "hmx_DataNode_AsVoid?",
}


def locate(sub: str):
    for f in sorted(GEN.glob("gh2test_recomp.*.cpp")):
        with f.open("r", errors="replace") as fh:
            for i, line in enumerate(fh, 1):
                if line.startswith(f"DEFINE_REX_FUNC({sub})"):
                    return f, i
    return None, None


def extract(sub: str):
    path, start = locate(sub)
    if path is None:
        return None
    with path.open("r", errors="replace") as fh:
        lines = fh.readlines()
    # find matching closing brace at column 0
    end = None
    for j in range(start, len(lines)):
        if lines[j].rstrip() == "}":
            end = j + 1
            break
    if end is None:
        return None
    body = lines[start-1:end]
    return body


def analyze(sub: str):
    path, start = locate(sub)
    body = extract(sub)
    if body is None or path is None:
        return f"### {sub}\n\n  NOT FOUND in generated/*.cpp\n"

    text = "".join(body)
    n_lines = len(body)
    # Approx PPC instruction count = number of // comment lines
    n_insns = sum(1 for l in body if l.lstrip().startswith("//"))
    labels = [m.group(1) for l in body for m in [LABEL_RE.match(l)] if m]

    calls: Counter = Counter()
    for m in CALL_RE.finditer(text):
        s = m.group(1)
        if s.lower() == sub.lower():
            continue
        calls[s] += 1

    loads: Counter = Counter()
    for m in LOAD_RE.finditer(text):
        loads[(m.group(2), int(m.group(3)), int(m.group(1)))] += 1
    stores: Counter = Counter()
    for m in STORE_RE.finditer(text):
        stores[(m.group(2), int(m.group(3)), int(m.group(1)))] += 1

    # 'this' offsets = reads/writes on r3 (the first arg) +Off
    this_reads = sorted({off for (r, off, w) in loads if r == "r3"})
    this_writes = sorted({off for (r, off, w) in stores if r == "r3"})

    out = []
    out.append(f"### {sub}  ({NAMED.get(sub, '?')})")
    out.append("")
    out.append(f"- Body: `{path.name}` lines {start}..{start + n_lines - 1}, {n_insns} PPC insns, {len(labels)} branch labels")
    out.append("")
    if calls:
        out.append("**Calls (sub_ -> count, named if known):**")
        for s, n in calls.most_common():
            name = NAMED.get(s, "")
            tag = f"  ({name})" if name else ""
            out.append(f"  - {s}{tag} × {n}")
        out.append("")
    if this_reads or this_writes:
        out.append(f"**`this` (r3) struct access offsets:**")
        if this_reads:
            out.append(f"  - reads: {', '.join(f'+{o}' for o in this_reads)}")
        if this_writes:
            out.append(f"  - writes: {', '.join(f'+{o}' for o in this_writes)}")
        out.append("")
    return "\n".join(out)


_cache: dict = {}
def start_of(sub):
    if sub not in _cache:
        path, start = locate(sub)
        _cache[sub] = start or 0
    return _cache[sub]


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: sub_skeleton.py sub_XXXX [sub_XXXX ...]")
    for s in sys.argv[1:]:
        print(analyze(s))
