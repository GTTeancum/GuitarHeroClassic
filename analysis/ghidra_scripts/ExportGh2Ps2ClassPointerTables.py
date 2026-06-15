# Dumps PS2 class metadata/pointer tables near trace-relevant class strings.
# Goal: recover likely method/vtable addresses when class names are referenced
# through registration data rather than direct code xrefs.

targets = [
    "15CharClipSamples", "11CharClipSet", "14CharDriverMidi",
    "10CharIKFoot", "10CharIKHand", "10CharIKMidi", "9CharIKRod",
    "7CamShot", "12CamShotFrame", "10WorldCrowd", "11LightPreset",
    "Q211LightPreset8Keyframe",
]

args = getScriptArgs()
if len(args) < 1:
    raise Exception("usage: ExportGh2Ps2ClassPointerTables.py <output_path>")
out_path = args[0]

listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()
mem = currentProgram.getMemory()

def data_string(d):
    try:
        v = d.getValue()
        return "" if v is None else str(v)
    except:
        return ""

def u32(addr):
    try:
        return mem.getInt(addr) & 0xffffffff
    except:
        return None

def classify(val):
    if val is None:
        return "<unreadable>"
    a = toAddr(val)
    block = mem.getBlock(a)
    if block is None:
        return ""
    f_at = fm.getFunctionAt(a)
    f_in = fm.getFunctionContaining(a)
    d = listing.getDataAt(a)
    parts = [block.getName()]
    if f_at is not None:
        parts.append("FUNC_AT %s" % f_at.getName())
    elif f_in is not None:
        parts.append("FUNC_IN %s+%s" % (f_in.getName(), a.subtract(f_in.getEntryPoint())))
    if d is not None and d.hasStringValue():
        parts.append("STRING %r" % data_string(d))
    return " ".join(parts)

hits = []
it = listing.getDefinedData(True)
while it.hasNext() and not monitor.isCancelled():
    d = it.next()
    if not d.hasStringValue():
        continue
    s = data_string(d)
    for t in targets:
        if t == s:
            hits.append((d.getAddress(), s))

with open(str(out_path), "w") as out:
    out.write("# GH2 PS2 class pointer table dump\n")
    out.write("# Program: %s\n" % currentProgram.getName())
    out.write("# Image base: %s\n\n" % currentProgram.getImageBase())
    for addr, s in hits:
        out.write("\n## %s @ %s\n" % (s, addr))
        start = addr.subtract(0x40)
        for i in range(0, 0x180, 4):
            a = start.add(i)
            val = u32(a)
            if val is None:
                out.write("%s: <unreadable>\n" % a)
            else:
                out.write("%s: %08x %s\n" % (a, val, classify(val)))
