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

#include "trace_recorder.h"

#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/ui/window.h>

// rex/audio/nop/nop_audio_system.h was tried first as a headless-audio
// approach. The NopAudioSystem::CreateDriver returns X_STATUS_NOT_IMPLEMENTED
// and the base AudioSystem layer asserts/crashes when it can't get a
// driver -- access violation at boot before XEX load. Reverted; we mute
// at the Windows audio-session level instead (see MuteOwnAudioSession()
// in OnPostSetup below). Original wiring kept commented for reference:
//   #include <rex/audio/nop/nop_audio_system.h>
//   #include <rex/runtime.h>
//   void OnPreSetup(rex::RuntimeConfig& config) override {
//     config.audio_factory = REX_AUDIO_BACKEND(rex::audio::nop::NopAudioSystem);
//   }

#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#endif

#ifdef _WIN32
namespace {

// Mute this process's audio session via the Windows Audio Session API.
// Doesn't disable the rexglue audio system (which the guest expects);
// just silences anything we'd otherwise send to the user's speakers
// so trace runs stay headless. Returns true on success.
inline bool MuteOwnAudioSession() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool we_inited_com = SUCCEEDED(hr);
    bool ok = false;
    IMMDeviceEnumerator* enumer = nullptr;
    IMMDevice* dev = nullptr;
    IAudioSessionManager2* mgr = nullptr;
    IAudioSessionEnumerator* sess_enum = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&enumer)))) goto done;
    if (FAILED(enumer->GetDefaultAudioEndpoint(eRender, eConsole, &dev))) goto done;
    if (FAILED(dev->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                             nullptr, reinterpret_cast<void**>(&mgr)))) goto done;
    if (FAILED(mgr->GetSessionEnumerator(&sess_enum))) goto done;
    {
        const DWORD my_pid = GetCurrentProcessId();
        int count = 0;
        sess_enum->GetCount(&count);
        for (int i = 0; i < count; ++i) {
            IAudioSessionControl* ctrl = nullptr;
            if (FAILED(sess_enum->GetSession(i, &ctrl))) continue;
            IAudioSessionControl2* ctrl2 = nullptr;
            DWORD pid = 0;
            if (SUCCEEDED(ctrl->QueryInterface(__uuidof(IAudioSessionControl2),
                                               reinterpret_cast<void**>(&ctrl2)))) {
                ctrl2->GetProcessId(&pid);
                ctrl2->Release();
            }
            if (pid == my_pid) {
                ISimpleAudioVolume* vol = nullptr;
                if (SUCCEEDED(ctrl->QueryInterface(__uuidof(ISimpleAudioVolume),
                                                   reinterpret_cast<void**>(&vol)))) {
                    vol->SetMute(TRUE, nullptr);
                    vol->Release();
                    ok = true;
                }
            }
            ctrl->Release();
        }
    }
done:
    if (sess_enum) sess_enum->Release();
    if (mgr)       mgr->Release();
    if (dev)       dev->Release();
    if (enumer)    enumer->Release();
    if (we_inited_com) CoUninitialize();
    return ok;
}

}  // namespace
#endif  // _WIN32

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
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}

  // (Audio is muted at the Windows-session level in OnPostSetup below;
  //  see MuteOwnAudioSession. No OnPreSetup override needed.)

  void OnPostSetup() override {
#ifdef _WIN32
    auto* w = window();
    if (!w) {
      REXLOG_WARN("Gh2testApp: window() is null at OnPostSetup; can't hide");
    } else {
      HWND hwnd = static_cast<HWND>(w->GetNativeWindowHandle());
      if (!hwnd) {
        REXLOG_WARN("Gh2testApp: native hwnd is null at OnPostSetup; can't hide");
      } else {
        // The window briefly appears during SetupPresentation (which creates
        // and Open()s it before OnPostSetup runs) but gets hidden here. The
        // D3D12 swapchain still functions against a hidden window.
        ShowWindow(hwnd, SW_HIDE);
        REXLOG_INFO("Gh2testApp: window hidden for headless run (hwnd=0x{:x})",
                    reinterpret_cast<uintptr_t>(hwnd));
      }
    }

    // Install SetCursorPos no-op hook so MnK's per-frame CenterCursor()
    // can't warp the user's cursor. See src/main.cpp for the hook impl
    // and rationale.
    extern bool gh2test_install_setcursorpos_hook();
    if (gh2test_install_setcursorpos_hook()) {
      REXLOG_INFO("Gh2testApp: user32!SetCursorPos hook installed");
    } else {
      REXLOG_WARN("Gh2testApp: SetCursorPos hook FAILED -- cursor may warp");
    }

    // Mute our Windows audio session so the song doesn't play to the
    // user's speakers. The session is created LAZILY on first audio
    // submission, which happens later than OnPostSetup. So we spawn a
    // detached worker that retries every 200ms until it succeeds (or
    // after a generous timeout). The session may also be destroyed
    // and recreated over the run, so the worker keeps trying for the
    // first ~60 s of process life as a paranoia margin.
    std::thread([]{
      for (int i = 0; i < 300; ++i) {  // 300 * 200ms = 60s
        if (MuteOwnAudioSession()) {
          REXLOG_INFO("Gh2testApp: process audio session muted (attempt {})", i + 1);
          // Keep going a bit in case session is re-created.
          for (int j = 0; j < 50; ++j) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            MuteOwnAudioSession();  // re-mute defensively
          }
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
      REXLOG_WARN("Gh2testApp: audio mute worker gave up after 60s");
    }).detach();
#endif

    // Initialize the trace recorder. Output lands in <exe_dir>/captures/
    // with a timestamped filename so multiple runs don't clobber each
    // other. Capture starts in the OFF state; flip to ON via an external
    // signal later in the boot sequence (see smoke_play.ps1 / Phase 3),
    // or by editing this to call SetCapturing(true) here for an
    // always-on capture during early bring-up.
    auto exe_dir = rex::filesystem::GetExecutableFolder();
    auto captures_dir = exe_dir / "captures";
    std::error_code ec;
    std::filesystem::create_directories(captures_dir, ec);

    auto now = std::chrono::system_clock::now();
    auto epoch_s =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::ostringstream name;
    name << "trace_" << epoch_s << ".jsonl";
    auto out_path = (captures_dir / name.str()).string();

    if (trace360::Init(out_path)) {
      // Always-on capture for now -- we'll narrow the window in Phase 3
      // when the smoke_play script drives the boot through to gameplay.
      // Until then, capturing everything lets us spot interesting
      // boot/menu events too.
      trace360::SetCapturing(true);
      REXLOG_INFO("trace360: recording to {}", out_path);
    } else {
      REXLOG_WARN("trace360: Init failed (path={})", out_path);
    }
  }

  void OnShutdown() override {
    trace360::SetCapturing(false);
    trace360::Shutdown();
  }
};
