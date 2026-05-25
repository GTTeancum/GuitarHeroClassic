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

namespace {

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
#endif  // _WIN32

REX_DEFINE_APP(gh2test, Gh2testApp::Create)
