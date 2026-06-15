from ghidra.app.decompiler import DecompInterface

targets = [
    ("CharForeTwist_update", "00175678"),
    ("CharHair_update", "00176fb8"),
    ("CharLookAt_update", "0017d658"),
    ("CharUpperTwist_update", "001823c8"),
]

args = getScriptArgs()
if len(args) < 1:
    raise Exception("usage: ExportGh2Ps2ExactFunctionDecompile.py <output_path>")
out_path = args[0]

listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()
refs = currentProgram.getReferenceManager()

decomp = DecompInterface()
decomp.openProgram(currentProgram)

def fmt_func(f):
    if f is None:
        return "<none>"
    return "%s @ %s" % (f.getName(), f.getEntryPoint())

def ensure_function(addr):
    f = fm.getFunctionAt(addr)
    if f is None:
        f = fm.getFunctionContaining(addr)
    if f is None:
        try:
            f = createFunction(addr, None)
        except:
            f = fm.getFunctionAt(addr)
    return f

def collect_callers(f, limit):
    out = []
    if f is None:
        return out
    for r in refs.getReferencesTo(f.getEntryPoint()):
        out.append((r.getFromAddress(), r.getReferenceType(),
                    fm.getFunctionContaining(r.getFromAddress())))
        if len(out) >= limit:
            break
    return out

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
            out.append((ins.getAddress(), r.getToAddress(),
                        fm.getFunctionAt(r.getToAddress())))
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
    res = decomp.decompileFunction(f, 120, monitor)
    if res is None:
        return "<decompile returned none>"
    if not res.decompileCompleted():
        return "<decompile failed: %s>" % res.getErrorMessage()
    return str(res.getDecompiledFunction().getC())

with open(str(out_path), "w") as out:
    out.write("# GH2 PS2 exact function decompile\n")
    out.write("# Program: %s\n" % currentProgram.getName())
    out.write("# Image base: %s\n\n" % currentProgram.getImageBase())
    for label, addr_s in targets:
        addr = toAddr(addr_s)
        f = ensure_function(addr)
        out.write("## %s %s\n" % (label, addr))
        out.write("FUNCTION %s\n" % fmt_func(f))
        if f is not None:
            out.write("body=%s\n" % f.getBody())
        out.write("callers:\n")
        for src, rtype, caller in collect_callers(f, 64):
            out.write("  %s %s %s\n" % (src, rtype, fmt_func(caller)))
        out.write("callees:\n")
        for src, dst, callee in collect_callees(f, 64):
            out.write("  %s -> %s %s\n" % (src, dst, fmt_func(callee)))
        out.write("disasm:\n")
        for line in disasm(f, 220):
            out.write("  %s\n" % line)
        out.write("decompile:\n")
        for line in decompile(f).splitlines():
            out.write("  %s\n" % line)
        out.write("\n")
