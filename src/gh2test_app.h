// gh2test - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.
//
// Trace-360 modification: we override OnPostSetup to hide the main window
// unconditionally. The trace work runs unattended for hours and a
// focus-stealing game window would disrupt the user's other work. See
// 360_TRACE_PLAN.md "Headless execution" Hard constraint.
//
// A --headless cvar approach was tried first; the cvar value parsed as
// false regardless of the CLI value (probably a static-init ordering
// issue with our consumer-side define vs rex::cvar::Init). Simpler:
// always hide. If a visible mode is ever needed for interactive debug,
// add a --show-window cvar wired into a working channel.

#pragma once

#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/ui/window.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

class Gh2testApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Gh2testApp>(new Gh2testApp(ctx, "gh2test",
        PPCImageConfig));
  }

  // Original (unmodified upstream) hooks left commented as a reference
  // of what's available; uncomment if/when needed:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}

  void OnPostSetup() override {
#ifdef _WIN32
    auto* w = window();
    if (!w) {
      REXLOG_WARN("Gh2testApp: window() is null at OnPostSetup; can't hide");
      return;
    }
    HWND hwnd = static_cast<HWND>(w->GetNativeWindowHandle());
    if (!hwnd) {
      REXLOG_WARN("Gh2testApp: native hwnd is null at OnPostSetup; can't hide");
      return;
    }
    // The window briefly appears during SetupPresentation (which creates
    // and Open()s it before OnPostSetup runs) but gets hidden here. The
    // D3D12 swapchain still functions against a hidden window.
    ShowWindow(hwnd, SW_HIDE);
    REXLOG_INFO("Gh2testApp: window hidden for headless run (hwnd=0x{:x})",
                reinterpret_cast<uintptr_t>(hwnd));
#endif
  }
};
