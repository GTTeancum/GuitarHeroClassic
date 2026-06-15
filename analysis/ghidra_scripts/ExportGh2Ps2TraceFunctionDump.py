# Exports code-level trace candidates from GH2 PS2 SLUS.
# This is static RE support only: it builds a breakpoint/decompile map for
# animation, IK, camera, lighting, crowd, and MIDI targets.

from ghidra.app.decompiler import DecompInterface

targets = [
    "CharClipSamples", "CharClipDriver", "CharClipSet", "CharClipGroup",
    "CharClipFilter", "CharDriverMidi", "CharIKHand", "CharIKFoot",
    "CharIKMidi", "CharIKRod", "CharForeTwist", "CharHair",
    "bone_facing", "bone_facing_delta", "bone_pelvis", "CamShot",
    "BandCamShot", "LightPreset", "lighting_change",
    "lighting_next_keyframe", "lighting_prev_keyframe",
    "lighting_first_keyframe", "midi_file", "midi_parser", "MidiParser",
    "MidiReceiver", "Performer", "PerformerGroup", "WorldCrowd",
    "play_group", "start_shot", "shot_ok", "check_shot",
]

args = getScriptArgs()
if len(args) < 1:
    raise Exception("usage: ExportGh2Ps2TraceFunctionDump.py <output_path>")
out_path = args[0]

listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()
refs = currentProgram.getReferenceManager()

decomp = DecompInterface()
decomp.openProgram(currentProgram)

def data_string(d):
    try:
        v = d.getValue()
        return "" if v is None else str(v)
    except:
        return ""

def func_at(addr):
    return fm.getFunctionContaining(addr)

def fmt_func(f):
    if f is None:
        return "<none>"
    return "%s @ %s" % (f.getName(), f.getEntryPoint())

def collect_callees(f, limit):
    out = []
    seen = set()
    if f is None:
        return out
    it = listing.getInstructions(f.getBody(), True)
    while it.hasNext() and len(out) < limit:
        ins = it.next()
        for r in ins.getReferencesFrom():
            if not r.getReferenceType().isCall():
                continue
            key = str(r.getToAddress())
            if key in seen:
                continue
            seen.add(key)
            out.append((ins.getAddress(), r.getToAddress(), fm.getFunctionAt(r.getToAddress())))
    return out

def collect_callers(f, limit):
    out = []
    if f is None:
        return out
    for r in refs.getReferencesTo(f.getEntryPoint()):
        out.append((r.getFromAddress(), r.getReferenceType(), func_at(r.getFromAddress())))
        if len(out) >= limit:
            break
    return out

def disasm(f, limit):
    out = []
    if f is None:
        return out
    it = listing.getInstructions(f.getBody(), True)
    while it.hasNext() and len(out) < limit:
        ins = it.next()
        out.append("%s: %s" % (ins.getAddress(), ins))
    return out

def decompile(f):
    if f is None:
        return "<none>"
    try:
        res = decomp.decompileFunction(f, 45, monitor)
        if res is None or not res.decompileCompleted():
            return "<decompile failed>"
        return str(res.getDecompiledFunction().getC())
    except Exception as e:
        return "<decompile exception: %s>" % e

direct_funcs = {}
string_hits = []

data_iter = listing.getDefinedData(True)
while data_iter.hasNext() and not monitor.isCancelled():
    d = data_iter.next()
    if not d.hasStringValue():
        continue
    s = data_string(d)
    low = s.lower()
    matched = [t for t in targets if t.lower() in low]
    if not matched:
        continue
    addr = d.getAddress()
    xrefs = list(refs.getReferencesTo(addr))
    string_hits.append((addr, s, matched, xrefs))
    for r in xrefs:
        f = func_at(r.getFromAddress())
        if f is not None:
            direct_funcs[str(f.getEntryPoint())] = f

# Add one-hop callers/callees to make breakpoint placement less brittle.
all_funcs = dict(direct_funcs)
for f in list(direct_funcs.values()):
    for _addr, _rtype, caller in collect_callers(f, 32):
        if caller is not None:
            all_funcs[str(caller.getEntryPoint())] = caller
    for _from, _to, callee in collect_callees(f, 32):
        if callee is not None:
            all_funcs[str(callee.getEntryPoint())] = callee

with open(str(out_path), "w") as out:
    out.write("# GH2 PS2 trace function dump\n")
    out.write("# Program: %s\n" % currentProgram.getName())
    out.write("# Image base: %s\n" % currentProgram.getImageBase())
    out.write("# Direct string-xref functions: %d\n" % len(direct_funcs))
    out.write("# Expanded one-hop functions: %d\n\n" % len(all_funcs))

    out.write("## Target String Hits\n")
    for addr, s, matched, xrefs in string_hits:
        out.write("\nSTRING %s [%s] %r\n" % (addr, ",".join(matched), s))
        if not xrefs:
            out.write("  XREF <none>\n")
        for r in xrefs:
            out.write("  XREF %s %s %s\n" %
                      (r.getFromAddress(), r.getReferenceType(),
                       fmt_func(func_at(r.getFromAddress()))))

    out.write("\n## Function Dumps\n")
    for key in sorted(all_funcs.keys()):
        f = all_funcs[key]
        out.write("\nFUNCTION %s\n" % fmt_func(f))
        out.write("  body=%s\n" % f.getBody())
        out.write("  source=%s\n" % ("direct-string-xref" if key in direct_funcs else "one-hop"))
        out.write("  callers:\n")
        for addr, rtype, caller in collect_callers(f, 24):
            out.write("    %s %s %s\n" % (addr, rtype, fmt_func(caller)))
        out.write("  callees:\n")
        for src, dst, callee in collect_callees(f, 32):
            out.write("    %s -> %s %s\n" % (src, dst, fmt_func(callee)))
        out.write("  disasm:\n")
        for line in disasm(f, 96):
            out.write("    %s\n" % line)
        out.write("  decompile:\n")
        dec = decompile(f)
        for line in dec.splitlines():
            out.write("    %s\n" % line)
