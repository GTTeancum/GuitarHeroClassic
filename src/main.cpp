// gh2test - ReXGlue Recompiled Project

#include "generated/gh2test_init.h"

#include "gh2test_app.h"

// Trace-360 modification: the window is hidden unconditionally in
// Gh2testApp::OnPostSetup; see gh2test_app.h for rationale.
//
// Additionally, a process-wide WH_CBT Win32 hook is installed at
// static-init time below to suppress window ACTIVATION (which is
// what causes focus stealing) during the window's brief visible
// period between Win32Window::OpenImpl's ShowWindow(SW_SHOWNORMAL)
// and our OnPostSetup hide. The window may still flicker visible
// for a frame or two, but it never grabs the foreground / steals
// the user's keyboard.

// A --headless cvar was tried first but consumer-side REXCVAR_DEFINE_BOOL
// values weren't picked up by rex::cvar::Init (probably a static-init
// ordering issue). Definition kept here as a commented reference in case
// the underlying issue is fixed later and a runtime toggle is wanted:
// #include <rex/cvar.h>
// REXCVAR_DEFINE_BOOL(headless, false, "Window",
//                     "Hide the main window after presentation setup (background-run mode)");

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <psapi.h>
#include <chrono>
#include <thread>

namespace {

// --- SetCursorPos no-op hook ------------------------------------------------
//
// The SDK's MnK input driver calls CenterCursor() every frame while
// "active", which calls SetCursorPos(pt.x, pt.y) to warp the cursor
// back to the window's client center. Even with the window hidden,
// the client rect still maps to a screen region (whatever the window
// position was), so the user's cursor gets yanked every frame and
// becomes unusable -- the "mouse arrest" symptom.
//
// We need MnK enabled so keyboard input reaches the guest as
// controller-button events (for menu navigation and cheat-code
// sequences). So we can't just disable MnK. Instead we patch
// SetCursorPos in this process to a no-op: it always returns TRUE
// without moving the cursor. MnK still thinks it captured the mouse
// (for its delta math) but the user's cursor stays put.
//
// Implementation: standard inline hook. Write a jmp at the start of
// user32.dll!SetCursorPos that lands in our replacement. We don't need
// a trampoline because we never want to call the original.

BOOL WINAPI NoopSetCursorPos(int /*X*/, int /*Y*/) { return TRUE; }

void DumpLine(const char* line) {
    FILE* f = std::fopen("hook_debug.log", "a");
    if (!f) return;
    std::fputs(line, f);
    std::fclose(f);
}
void LogBytes(const char* label, void* p) {
    auto* b = reinterpret_cast<const unsigned char*>(p);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "[gh2test] %s @%p: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        label, p, b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13]);
    DumpLine(buf);
}

// Walk a module's import directory and rewrite every IAT entry that
// matches `import_name` from `import_dll` so calls dispatch to
// `replacement` instead. Returns number of entries patched.
int PatchModuleIAT(HMODULE module, const char* import_dll,
                   const char* import_name, void* replacement) {
    auto base = reinterpret_cast<uint8_t*>(module);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

    auto& imp_dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imp_dir.Size) return 0;
    auto* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        base + imp_dir.VirtualAddress);

    int patched = 0;
    for (; desc->Name; ++desc) {
        const char* dll = reinterpret_cast<const char*>(base + desc->Name);
        if (_stricmp(dll, import_dll) != 0) continue;

        auto* iat = reinterpret_cast<uintptr_t*>(base + desc->FirstThunk);
        // OriginalFirstThunk preserves the name/ordinal info; FirstThunk
        // is the bound IAT we want to overwrite.
        auto* names = reinterpret_cast<IMAGE_THUNK_DATA*>(
            base + (desc->OriginalFirstThunk ? desc->OriginalFirstThunk
                                             : desc->FirstThunk));
        for (size_t i = 0; names[i].u1.AddressOfData; ++i) {
            if (IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal)) continue;
            auto* by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                base + names[i].u1.AddressOfData);
            if (std::strcmp(by_name->Name, import_name) != 0) continue;

            if (iat[i] == reinterpret_cast<uintptr_t>(replacement)) {
                // Already patched -- count it but don't spam the log.
                ++patched;
                continue;
            }
            DWORD old_prot = 0;
            if (VirtualProtect(&iat[i], sizeof(uintptr_t), PAGE_READWRITE,
                               &old_prot)) {
                char buf[128];
                std::snprintf(buf, sizeof(buf),
                    "[gh2test] IAT patch %s!%s: %p -> %p\n",
                    import_dll, import_name,
                    reinterpret_cast<void*>(iat[i]), replacement);
                DumpLine(buf);
                iat[i] = reinterpret_cast<uintptr_t>(replacement);
                DWORD discard = 0;
                VirtualProtect(&iat[i], sizeof(uintptr_t), old_prot, &discard);
                ++patched;
            }
        }
    }
    return patched;
}

// Patch every loaded module's IAT entry for user32!SetCursorPos.
int PatchAllModulesIAT() {
    HMODULE mods[1024];
    DWORD needed = 0;
    if (!EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed))
        return 0;
    const size_t count = needed / sizeof(HMODULE);
    int total = 0;
    for (size_t i = 0; i < count; ++i) {
        total += PatchModuleIAT(mods[i], "user32.dll", "SetCursorPos",
                                reinterpret_cast<void*>(&NoopSetCursorPos));
    }
    return total;
}

bool InstallSetCursorPosHook() {
    // The actual SetCursorPos caller is MnkInputDriver::CenterCursor()
    // inside rexruntime.dll. user32!SetCursorPos in newer Windows is a
    // forwarder thunk; the loader binds each importing module's IAT
    // directly to the underlying win32u.dll function, bypassing the
    // exported thunk. So patching user32 has no effect -- we must
    // patch each module's IAT entry directly.
    //
    // Additionally, modules can be LoadLibrary'd after our initial
    // patch (Tracy, SDL backends, plug-ins). A background thread
    // re-runs the patch every 250ms to catch newcomers. Cheap.
    int total = PatchAllModulesIAT();
    char buf[64];
    std::snprintf(buf, sizeof(buf),
        "[gh2test] SetCursorPos IAT patches applied (initial): %d\n", total);
    DumpLine(buf);

    std::thread([]{
        int prev = 0;
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            int n = PatchAllModulesIAT();
            if (n != prev) {
                char b[96];
                std::snprintf(b, sizeof(b),
                    "[gh2test] SetCursorPos IAT re-patch: %d entries\n", n);
                DumpLine(b);
                prev = n;
            }
        }
    }).detach();
    return total > 0;
}


LRESULT CALLBACK Gh2testCbtProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // HCBT_ACTIVATE = window is about to activate (become foreground).
    // Returning non-zero suppresses the activation. We only want to
    // suppress for windows in our own process so we don't break unrelated
    // things if any other UI ever exists.
    if (nCode == HCBT_ACTIVATE) {
        HWND hwnd = reinterpret_cast<HWND>(wParam);
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == GetCurrentProcessId()) {
            return 1;  // suppress activate
        }
    }
    // Pass everything else through.
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

struct Gh2testCbtInstaller {
    HHOOK hook = nullptr;
    Gh2testCbtInstaller() {
        hook = SetWindowsHookExW(WH_CBT, &Gh2testCbtProc, nullptr,
                                  GetCurrentThreadId());
        // No-op if SetWindowsHookEx fails; the worst case is the window
        // briefly activates. We can't REXLOG here -- logging isn't
        // initialized yet at static-init time.
    }
    ~Gh2testCbtInstaller() {
        if (hook) UnhookWindowsHookEx(hook);
    }
};

// Static initializer runs before WinMain, so the hook is in place before
// the SDK ever creates its window.
Gh2testCbtInstaller g_gh2test_cbt_installer;

}  // namespace

// External entry point for Gh2testApp::OnPostSetup to install the
// SetCursorPos no-op patch after logging is up. See the InstallSetCursorPosHook
// implementation above and the call site in gh2test_app.h.
bool gh2test_install_setcursorpos_hook() { return InstallSetCursorPosHook(); }

#endif  // _WIN32

REX_DEFINE_APP(gh2test, Gh2testApp::Create)
