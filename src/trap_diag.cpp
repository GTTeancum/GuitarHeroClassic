// trap_diag.cpp - On the first PPC tw/td trap of a run, capture a host call
// stack and resolve each frame to a symbol. Used to identify which
// rex_sub_<addr> function is asserting on the guest data, so we know where
// to inject a REX_HOOK.
//
// Activated from the (manually edited) ppc_trap path in generated/gh2test_init.h.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>

#pragma comment(lib, "dbghelp.lib")

namespace gh2test_diag {

void log_trap_callsite() {
    // Only fire the first time -- subsequent traps in the same run usually
    // come from the same spot and would just flood the log.
    static std::atomic<bool> already_logged{false};
    bool expected = false;
    if (!already_logged.compare_exchange_strong(expected, true)) return;

    static std::mutex m;
    std::lock_guard<std::mutex> lk(m);

    HANDLE proc = GetCurrentProcess();
    static std::atomic<bool> sym_init{false};
    bool sym_was_init = sym_init.exchange(true);
    if (!sym_was_init) {
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
        SymInitialize(proc, nullptr, TRUE);
    }

    constexpr USHORT kMaxFrames = 40;
    void* frames[kMaxFrames];
    USHORT n = CaptureStackBackTrace(0, kMaxFrames, frames, nullptr);

    constexpr size_t kNameBytes = sizeof(SYMBOL_INFO) + 512;
    auto* sym = static_cast<SYMBOL_INFO*>(_alloca(kNameBytes));
    std::memset(sym, 0, kNameBytes);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = 511;

    // /SUBSYSTEM:WINDOWS means stderr isn't attached to anything by default.
    // Write to a fixed sidecar file next to the exe instead.
    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    std::string log_path = exe_path;
    auto sep = log_path.find_last_of("\\/");
    if (sep != std::string::npos) log_path.resize(sep + 1);
    log_path += "trap_stack.log";

    FILE* f = std::fopen(log_path.c_str(), "w");
    if (!f) return;
    std::fprintf(f, "==== TRAP STACK (first occurrence) ====\n");
    for (USHORT i = 0; i < n; ++i) {
        DWORD64 disp = 0;
        const char* name = "<unresolved>";
        if (SymFromAddr(proc, reinterpret_cast<DWORD64>(frames[i]), &disp, sym)) {
            name = sym->Name;
        }
        std::fprintf(f, "  [%2u] %p  %s+0x%llx\n",
                     i, frames[i], name, static_cast<unsigned long long>(disp));
    }
    std::fprintf(f, "==== END TRAP STACK ====\n");
    std::fclose(f);
}

}  // namespace gh2test_diag
