# Exports function neighborhoods for PS2 SLUS strings/classes relevant to
# animation, camera, lighting, and MIDI performance tracing.

targets = [
    "CharClipSamples", "CharClipDriver", "CharClipSet", "CharIKHand",
    "CharIKFoot", "CharIKMidi", "CharDriverMidi", "bone_facing",
    "bone_pelvis", "CamShot", "LightPreset", "lighting_change",
    "lighting_next_keyframe", "lighting_prev_keyframe",
    "lighting_first_keyframe", "midi_file", "midi_parser",
    "MidiParser", "MidiReceiver", "Performer", "WorldCrowd",
]

args = getScriptArgs()
if len(args) < 1:
    raise Exception("usage: ExportGh2Ps2FunctionNeighborhood.py <output_path>")
out_path = args[0]

listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()
refs = currentProgram.getReferenceManager()

def data_string(d):
    try:
        v = d.getValue()
        return "" if v is None else str(v)
    except:
        return ""

def func_at(addr):
    f = fm.getFunctionContaining(addr)
    return f

def fmt_func(f):
    if f is None:
        return "<none>"
    return "%s @ %s" % (f.getName(), f.getEntryPoint())

def callees(f, limit):
    out = []
    if f is None:
        return out
    body = f.getBody()
    it = listing.getInstructions(body, True)
    seen = set()
    while it.hasNext() and len(out) < limit:
        ins = it.next()
        for r in ins.getReferencesFrom():
            if not r.getReferenceType().isCall():
                continue
            cf = fm.getFunctionAt(r.getToAddress())
            key = str(r.getToAddress())
            if key in seen:
                continue
            seen.add(key)
            out.append("%s -> %s" % (ins.getAddress(), fmt_func(cf)))
    return out

hits = []
seen_funcs = {}
data_iter = listing.getDefinedData(True)
while data_iter.hasNext() and not monitor.isCancelled():
    d = data_iter.next()
    if not d.hasStringValue():
        continue
    s = data_string(d)
    matched = [t for t in targets if t.lower() in s.lower()]
    if not matched:
        continue
    addr = d.getAddress()
    xrefs = list(refs.getReferencesTo(addr))
    hits.append(("STRING", addr, s, matched, xrefs))
    for r in xrefs:
        f = func_at(r.getFromAddress())
        if f is not None:
            seen_funcs[str(f.getEntryPoint())] = f

with open(str(out_path), "w") as f:
    f.write("# GH2 PS2 function neighborhoods for trace target strings\n")
    f.write("# Program: %s\n" % currentProgram.getName())
    f.write("# Image base: %s\n\n" % currentProgram.getImageBase())
    for kind, addr, s, matched, xrefs in hits:
        f.write("STRING %s %s %r\n" % (addr, ",".join(matched), s))
        for r in xrefs:
            src = r.getFromAddress()
            f.write("  XREF %s %s %s\n" %
                    (src, r.getReferenceType(), fmt_func(func_at(src))))
        if not xrefs:
            f.write("  XREF <none>\n")
    f.write("\n# Functions with direct target-string xrefs\n")
    for key in sorted(seen_funcs.keys()):
        fobj = seen_funcs[key]
        f.write("\nFUNCTION %s\n" % fmt_func(fobj))
        f.write("  body=%s\n" % fobj.getBody())
        f.write("  callers:\n")
        caller_count = 0
        for r in refs.getReferencesTo(fobj.getEntryPoint()):
            caller = func_at(r.getFromAddress())
            f.write("    %s %s %s\n" %
                    (r.getFromAddress(), r.getReferenceType(), fmt_func(caller)))
            caller_count += 1
            if caller_count >= 16:
                break
        f.write("  callees:\n")
        for c in callees(fobj, 24):
            f.write("    %s\n" % c)
