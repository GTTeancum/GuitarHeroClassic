# Dumps PS2 SLUS strings and direct xrefs relevant to venue/band tracing.
# Run via analyzeHeadless after importing/analyzing SLUS_214.47.

from ghidra.program.model.symbol import RefType

terms = [
    "anim", "clip", "bone", "char", "character", "skeleton", "ik",
    "midi", "beat", "venue", "world", "band", "perform", "guitar",
    "bass", "drum", "singer", "camera", "cam", "shot", "lighting",
    "light", "keyframe", "crowd", "spot", "poll", "update",
]

args = getScriptArgs()
if len(args) < 1:
    raise Exception("usage: ExportGh2Ps2TraceTargets.py <output_path>")
out_path = args[0]
listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()
refs = currentProgram.getReferenceManager()

def get_string_value(data):
    try:
        v = data.getValue()
        return "" if v is None else str(v)
    except:
        return ""

def containing_function(addr):
    f = fm.getFunctionContaining(addr)
    return "<none>" if f is None else "%s @ %s" % (f.getName(), f.getEntryPoint())

rows = []
data_iter = listing.getDefinedData(True)
while data_iter.hasNext() and not monitor.isCancelled():
    d = data_iter.next()
    if not d.hasStringValue():
        continue
    s = get_string_value(d)
    low = s.lower()
    matched = [t for t in terms if t in low]
    if not matched:
        continue
    addr = d.getAddress()
    rows.append("STRING %s %s %r" % (addr, ",".join(matched), s))
    for r in refs.getReferencesTo(addr):
        src = r.getFromAddress()
        rows.append("  XREF %s %s %s" % (src, r.getReferenceType(), containing_function(src)))

with open(str(out_path), "w") as f:
    f.write("# GH2 PS2 trace target strings/xrefs\n")
    f.write("# Program: %s\n" % currentProgram.getName())
    f.write("# Image base: %s\n" % currentProgram.getImageBase())
    f.write("# Rows: %d\n\n" % len(rows))
    for row in rows:
        f.write(row + "\n")
