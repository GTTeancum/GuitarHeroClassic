// engine/src/render/window_d3d9.cpp

#include "render/window_d3d9.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <xinput.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace ghogx::render {

struct Window::Impl {
  HWND hwnd = nullptr;
  IDirect3D9* d3d = nullptr;
  IDirect3DDevice9* dev = nullptr;
  bool should_close = false;
  int bb_w = 0;  // back-buffer dimensions (full-screen quad size)
  int bb_h = 0;
  IDirect3DTexture9* blit_tex = nullptr;  // lazily (re)created for blit
  int blit_w = 0;
  int blit_h = 0;

  // Input snapshots (refreshed each pump()): keyboard virtual-key down state +
  // XInput controller button bitmask, current frame and previous, for edge
  // detection in action_pressed(). key_now is written live by the window proc.
  bool key_now[256] = {};
  bool key_prev[256] = {};
  unsigned short pad_now  = 0;
  unsigned short pad_prev = 0;

  // GH2 guitar input: bits 0-4 = frets, bit 5 = strum, bit 6 = star power.
  // Built from keyboard ASDFG + Space/Shift/H and XInput LT/LB/RB/RT/A +
  // stick/Back/Y.
  uint32_t gh_now  = 0;  // current frame (raw held)
  uint32_t gh_prev = 0;  // previous frame (for edge detection)
  unsigned char lt_now = 0;  // XInput left trigger (0-255)
  unsigned char rt_now = 0;  // XInput right trigger (0-255)
};

namespace {
// Screen-space textured vertex (pre-transformed: x,y in pixels) with a diffuse
// color used to modulate (fade) the texture. FVF field order must match:
// XYZRHW, then DIFFUSE (DWORD), then TEX1.
struct ScreenVertex {
  float x, y, z, rhw;
  D3DCOLOR color;
  float u, v;
};
constexpr DWORD kScreenFVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
}  // namespace

namespace {

constexpr const char* kClassName = "GhogxWindowClass";

LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
  auto* impl = reinterpret_cast<Window::Impl*>(GetWindowLongPtr(h, GWLP_USERDATA));
  switch (msg) {
    case WM_CLOSE:
      if (impl) impl->should_close = true;
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_KEYDOWN:
      if (impl && wp < 256) impl->key_now[wp] = true;
      if (wp == VK_ESCAPE && impl) impl->should_close = true;  // hard quit
      return 0;
    case WM_KEYUP:
      if (impl && wp < 256) impl->key_now[wp] = false;
      return 0;
    default:
      return DefWindowProc(h, msg, wp, lp);
  }
}

}  // namespace

Window::Window() : impl_(std::make_unique<Impl>()) {}

Window::~Window() {
  if (impl_->blit_tex) impl_->blit_tex->Release();
  if (impl_->dev) impl_->dev->Release();
  if (impl_->d3d) impl_->d3d->Release();
  if (impl_->hwnd) DestroyWindow(impl_->hwnd);
  UnregisterClass(kClassName, GetModuleHandle(nullptr));
}

std::unique_ptr<Window> Window::create(int width, int height, const char* title) {
  std::unique_ptr<Window> win(new Window());
  Impl* impl = win->impl_.get();
  char* hide_window_env = nullptr;
  size_t hide_window_env_len = 0;
  const bool hide_window =
      _dupenv_s(&hide_window_env, &hide_window_env_len, "GHOGX_HIDE_WINDOW") ==
          0 &&
      hide_window_env && hide_window_env[0];
  if (hide_window_env) std::free(hide_window_env);

  HINSTANCE inst = GetModuleHandle(nullptr);
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = inst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = kClassName;
  RegisterClassEx(&wc);

  // Size the window so the CLIENT area is width x height.
  RECT r = {0, 0, width, height};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  impl->hwnd = CreateWindowEx(0, kClassName, title, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                              r.bottom - r.top, nullptr, nullptr, inst, nullptr);
  if (!impl->hwnd) {
    std::fprintf(stderr, "[ghogx] CreateWindowEx failed (%lu)\n", GetLastError());
    return nullptr;
  }
  SetWindowLongPtr(impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
  if (!hide_window) {
    ShowWindow(impl->hwnd, SW_SHOW);
    UpdateWindow(impl->hwnd);
  }

  impl->d3d = Direct3DCreate9(D3D_SDK_VERSION);
  if (!impl->d3d) {
    std::fprintf(stderr, "[ghogx] Direct3DCreate9 failed\n");
    return nullptr;
  }

  D3DPRESENT_PARAMETERS pp = {};
  pp.Windowed = TRUE;
  pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  pp.BackBufferFormat = D3DFMT_UNKNOWN;  // match desktop
  pp.BackBufferWidth = width;
  pp.BackBufferHeight = height;
  pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;  // vsync
  pp.hDeviceWindow = impl->hwnd;
  pp.EnableAutoDepthStencil = TRUE;     // depth buffer for 3-D rendering
  pp.AutoDepthStencilFormat = D3DFMT_D24S8;

  const char* device_path = "HAL hardware VP D24S8 vsync";
  auto try_create_device = [&](D3DDEVTYPE device_type, DWORD flags,
                               D3DFORMAT depth_format, UINT interval,
                               const char* label) {
    pp.AutoDepthStencilFormat = depth_format;
    pp.PresentationInterval = interval;
    HRESULT attempt = impl->d3d->CreateDevice(D3DADAPTER_DEFAULT, device_type,
                                              impl->hwnd, flags, &pp,
                                              &impl->dev);
    if (SUCCEEDED(attempt)) device_path = label;
    return attempt;
  };

  HRESULT hr = try_create_device(D3DDEVTYPE_HAL,
                                 D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                 D3DFMT_D24S8, D3DPRESENT_INTERVAL_ONE,
                                 device_path);
  if (FAILED(hr)) {
    // Retry with software vertex processing (some adapters / headless setups).
    hr = try_create_device(D3DDEVTYPE_HAL, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                           D3DFMT_D24S8, D3DPRESENT_INTERVAL_ONE,
                           "HAL software VP D24S8 vsync");
  }
  if (FAILED(hr)) {
    // Hidden validation windows on some drivers reject D24S8/vsync paths even
    // though the app only needs a depth buffer for screenshots.
    hr = try_create_device(D3DDEVTYPE_HAL, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                           D3DFMT_D16, D3DPRESENT_INTERVAL_IMMEDIATE,
                           "HAL software VP D16 immediate");
  }
  if (FAILED(hr)) {
    // Last-resort desktop validation path. It is slow if available, but keeps
    // trace/screenshot runs independent of a foreground emulator or app window.
    hr = try_create_device(D3DDEVTYPE_REF, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                           D3DFMT_D16, D3DPRESENT_INTERVAL_IMMEDIATE,
                           "REF software VP D16 immediate");
  }
  if (FAILED(hr)) {
    std::fprintf(stderr, "[ghogx] CreateDevice failed (hr=0x%08lX)\n",
                 static_cast<unsigned long>(hr));
    return nullptr;
  }

  impl->bb_w = width;
  impl->bb_h = height;
  std::fprintf(stderr, "[ghogx] D3D9 window %dx%d created%s via %s\n", width,
               height, hide_window ? " (hidden)" : "", device_path);
  return win;
}

void Window::pump() {
  // Snapshot last frame's input so action_pressed() can edge-detect this frame.
  std::memcpy(impl_->key_prev, impl_->key_now, sizeof(impl_->key_prev));
  impl_->pad_prev = impl_->pad_now;

  MSG msg;
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      impl_->should_close = true;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);  // updates impl_->key_now via wnd_proc
  }

  // Poll the player-1 XInput controller. Fold the left stick into the d-pad
  // bits so stick + d-pad both drive menu navigation.
  unsigned short pad = 0;
  impl_->gh_prev = impl_->gh_now;
  uint32_t gh = 0;
  XINPUT_STATE xs = {};
  if (XInputGetState(0, &xs) == ERROR_SUCCESS) {
    pad = xs.Gamepad.wButtons;
    constexpr SHORT kDead = 16000;
    if (xs.Gamepad.sThumbLY > kDead)  pad |= XINPUT_GAMEPAD_DPAD_UP;
    if (xs.Gamepad.sThumbLY < -kDead) pad |= XINPUT_GAMEPAD_DPAD_DOWN;
    if (xs.Gamepad.sThumbLX < -kDead) pad |= XINPUT_GAMEPAD_DPAD_LEFT;
    if (xs.Gamepad.sThumbLX > kDead)  pad |= XINPUT_GAMEPAD_DPAD_RIGHT;

    // GH2 guitar fret layout on a standard Xbox One controller:
    //   LT (analog) = Green, LB = Red, RB = Yellow, RT (analog) = Blue, A = Orange
    //   Left-stick ±Y = strum (both up and down count)
    impl_->lt_now = xs.Gamepad.bLeftTrigger;
    impl_->rt_now = xs.Gamepad.bRightTrigger;
    constexpr BYTE kTrigDead = 76;  // ~30% of 255
    if (xs.Gamepad.bLeftTrigger  > kTrigDead)    gh |= (1u << 0);  // Green
    if (pad & XINPUT_GAMEPAD_LEFT_SHOULDER)       gh |= (1u << 1);  // Red
    if (pad & XINPUT_GAMEPAD_RIGHT_SHOULDER)      gh |= (1u << 2);  // Yellow
    if (xs.Gamepad.bRightTrigger > kTrigDead)     gh |= (1u << 3);  // Blue
    if (pad & XINPUT_GAMEPAD_A)                   gh |= (1u << 4);  // Orange
    // Strum: left-stick vertical axis (either direction) past a generous dead zone.
    constexpr SHORT kStrumDead = 20000;
    if (xs.Gamepad.sThumbLY > kStrumDead || xs.Gamepad.sThumbLY < -kStrumDead)
      gh |= (1u << 5);
    if ((pad & XINPUT_GAMEPAD_BACK) || (pad & XINPUT_GAMEPAD_Y))
      gh |= (1u << 6);
  }
  impl_->pad_now = pad;

  // Keyboard GH2 fret mapping (A=Green S=Red D=Yellow F=Blue G=Orange Space=Strum).
  if (impl_->key_now['A'])       gh |= (1u << 0);
  if (impl_->key_now['S'])       gh |= (1u << 1);
  if (impl_->key_now['D'])       gh |= (1u << 2);
  if (impl_->key_now['F'])       gh |= (1u << 3);
  if (impl_->key_now['G'])       gh |= (1u << 4);
  if (impl_->key_now[VK_SPACE])  gh |= (1u << 5);
  if (impl_->key_now[VK_SHIFT] || impl_->key_now['H']) gh |= (1u << 6);
  impl_->gh_now = gh;
}

uint32_t Window::guitar_input_edge() const {
  return impl_->gh_now & ~impl_->gh_prev;  // bits that rose 0→1 this frame
}

uint32_t Window::guitar_input_held() const {
  return impl_->gh_now & 0x1F;  // bits 0-4 only; strum is always edge-only
}

bool Window::action_pressed(Action a) const {
  const Impl* p = impl_.get();
  auto key_edge = [p](int vk) { return p->key_now[vk] && !p->key_prev[vk]; };
  auto pad_edge = [p](unsigned short m) {
    return (p->pad_now & m) != 0 && (p->pad_prev & m) == 0;
  };
  switch (a) {
    case Action::Confirm:
      return key_edge(VK_RETURN) || key_edge(VK_SPACE) || pad_edge(XINPUT_GAMEPAD_A);
    case Action::Back:
      return key_edge(VK_BACK) || pad_edge(XINPUT_GAMEPAD_B);
    case Action::Start:
      return key_edge(VK_RETURN) || pad_edge(XINPUT_GAMEPAD_START);
    case Action::Up:
      return key_edge(VK_UP) || pad_edge(XINPUT_GAMEPAD_DPAD_UP);
    case Action::Down:
      return key_edge(VK_DOWN) || pad_edge(XINPUT_GAMEPAD_DPAD_DOWN);
    case Action::Left:
      return key_edge(VK_LEFT) || pad_edge(XINPUT_GAMEPAD_DPAD_LEFT);
    case Action::Right:
      return key_edge(VK_RIGHT) || pad_edge(XINPUT_GAMEPAD_DPAD_RIGHT);
  }
  return false;
}

bool Window::key_down(int virtual_key) const {
  if (!impl_ || virtual_key < 0 || virtual_key >= 256) return false;
  return impl_->key_now[virtual_key];
}

bool Window::should_close() const { return impl_->should_close; }

void* Window::device_ptr() const {
  return impl_ ? static_cast<void*>(impl_->dev) : nullptr;
}
int Window::bb_width()  const { return impl_ ? impl_->bb_w : 0; }
int Window::bb_height() const { return impl_ ? impl_->bb_h : 0; }

void Window::clear(float r, float g, float b) {
  if (!impl_->dev) return;
  D3DCOLOR color = D3DCOLOR_COLORVALUE(r, g, b, 1.0f);
  impl_->dev->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
}

void Window::blit_fullscreen_rgba(const unsigned char* rgba, int width, int height,
                                  float brightness) {
  IDirect3DDevice9* dev = impl_->dev;
  if (!dev || !rgba || width <= 0 || height <= 0) return;
  if (brightness < 0.0f) brightness = 0.0f;
  if (brightness > 1.0f) brightness = 1.0f;

  // (Re)create the texture if the size changed.
  if (!impl_->blit_tex || impl_->blit_w != width || impl_->blit_h != height) {
    if (impl_->blit_tex) {
      impl_->blit_tex->Release();
      impl_->blit_tex = nullptr;
    }
    if (FAILED(dev->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8,
                                  D3DPOOL_MANAGED, &impl_->blit_tex, nullptr))) {
      return;
    }
    impl_->blit_w = width;
    impl_->blit_h = height;
  }

  // Upload RGBA -> the texture's BGRA (A8R8G8B8 is B,G,R,A in memory), honoring
  // the lock pitch (rows may be padded).
  D3DLOCKED_RECT lr;
  if (SUCCEEDED(impl_->blit_tex->LockRect(0, &lr, nullptr, 0))) {
    for (int y = 0; y < height; ++y) {
      auto* dst = static_cast<unsigned char*>(lr.pBits) + y * lr.Pitch;
      const unsigned char* src = rgba + static_cast<size_t>(y) * width * 4;
      for (int x = 0; x < width; ++x) {
        dst[x * 4 + 0] = src[x * 4 + 2];  // B
        dst[x * 4 + 1] = src[x * 4 + 1];  // G
        dst[x * 4 + 2] = src[x * 4 + 0];  // R
        dst[x * 4 + 3] = src[x * 4 + 3];  // A
      }
    }
    impl_->blit_tex->UnlockRect(0);
  }

  // Fit the image inside the back buffer preserving aspect ratio (letterbox /
  // pillarbox); the prior clear() fills the bars. The -0.5 px shift aligns
  // texels to pixels.
  const float bw = static_cast<float>(impl_->bb_w);
  const float bh = static_cast<float>(impl_->bb_h);
  const float tex_aspect = static_cast<float>(width) / static_cast<float>(height);
  const float bb_aspect = bw / bh;
  float dw, dh;
  if (tex_aspect > bb_aspect) {  // image wider than window -> fit width
    dw = bw;
    dh = bw / tex_aspect;
  } else {                       // taller -> fit height
    dh = bh;
    dw = bh * tex_aspect;
  }
  const float x0 = (bw - dw) * 0.5f - 0.5f;
  const float y0 = (bh - dh) * 0.5f - 0.5f;
  const float x1 = x0 + dw;
  const float y1 = y0 + dh;
  const D3DCOLOR diffuse =
      D3DCOLOR_COLORVALUE(brightness, brightness, brightness, 1.0f);
  const ScreenVertex quad[4] = {
      {x0, y0, 0.0f, 1.0f, diffuse, 0.0f, 0.0f},
      {x1, y0, 0.0f, 1.0f, diffuse, 1.0f, 0.0f},
      {x0, y1, 0.0f, 1.0f, diffuse, 0.0f, 1.0f},
      {x1, y1, 0.0f, 1.0f, diffuse, 1.0f, 1.0f},
  };

  dev->BeginScene();
  dev->SetRenderState(D3DRS_LIGHTING, FALSE);
  dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  dev->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  // Modulate the texture by the per-vertex diffuse (the brightness fade).
  dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev->SetTexture(0, impl_->blit_tex);
  dev->SetFVF(kScreenFVF);
  dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
  dev->SetTexture(0, nullptr);
  dev->EndScene();
}

bool Window::save_screenshot(const char* path) {
  IDirect3DDevice9* dev = impl_->dev;
  if (!dev || !path) return false;

  IDirect3DSurface9* back = nullptr;
  if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back)
    return false;

  D3DSURFACE_DESC desc;
  back->GetDesc(&desc);

  // Copy the (default-pool) back buffer into a system-memory surface we can lock.
  IDirect3DSurface9* sys = nullptr;
  bool ok = false;
  if (SUCCEEDED(dev->CreateOffscreenPlainSurface(desc.Width, desc.Height,
                                                 desc.Format, D3DPOOL_SYSTEMMEM,
                                                 &sys, nullptr)) &&
      SUCCEEDED(dev->GetRenderTargetData(back, sys))) {
    D3DLOCKED_RECT lr;
    if (SUCCEEDED(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))) {
      const int w = static_cast<int>(desc.Width);
      const int h = static_cast<int>(desc.Height);
      const int row_bytes = w * 3;
      const int pad = (4 - (row_bytes % 4)) % 4;
      const int stride = row_bytes + pad;
      const uint32_t img_size = static_cast<uint32_t>(stride) * h;

      // BMP headers (14 + 40 bytes), 24-bit BGR, bottom-up.
      uint8_t fh[14] = {};
      uint8_t ih[40] = {};
      const uint32_t off = 54;
      const uint32_t file_size = off + img_size;
      fh[0] = 'B'; fh[1] = 'M';
      std::memcpy(fh + 2, &file_size, 4);
      std::memcpy(fh + 10, &off, 4);
      const uint32_t ih_size = 40;
      std::memcpy(ih + 0, &ih_size, 4);
      const int32_t iw = w, ih2 = h;
      std::memcpy(ih + 4, &iw, 4);
      std::memcpy(ih + 8, &ih2, 4);
      const uint16_t planes = 1, bpp = 24;
      std::memcpy(ih + 12, &planes, 2);
      std::memcpy(ih + 14, &bpp, 2);
      std::memcpy(ih + 20, &img_size, 4);

      if (FILE* f = std::fopen(path, "wb")) {
        std::fwrite(fh, 1, 14, f);
        std::fwrite(ih, 1, 40, f);
        std::vector<uint8_t> row(static_cast<size_t>(stride), 0);
        // Source is A8R8G8B8 / X8R8G8B8 = B,G,R,A in memory; BMP wants B,G,R.
        // Write bottom-up.
        for (int y = h - 1; y >= 0; --y) {
          const uint8_t* src = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
          for (int x = 0; x < w; ++x) {
            row[static_cast<size_t>(x) * 3 + 0] = src[x * 4 + 0];  // B
            row[static_cast<size_t>(x) * 3 + 1] = src[x * 4 + 1];  // G
            row[static_cast<size_t>(x) * 3 + 2] = src[x * 4 + 2];  // R
          }
          std::fwrite(row.data(), 1, static_cast<size_t>(stride), f);
        }
        std::fclose(f);
        ok = true;
        std::fprintf(stderr, "[ghogx] screenshot saved: %s (%dx%d)\n", path, w, h);
      }
      sys->UnlockRect();
    }
  }
  if (sys) sys->Release();
  back->Release();
  return ok;
}

void Window::present() {
  if (!impl_->dev) return;
  HRESULT hr = impl_->dev->Present(nullptr, nullptr, nullptr, nullptr);
  if (hr == D3DERR_DEVICELOST) {
    // Minimal handling for the first milestone: a lost device (e.g. alt-tab in
    // exclusive mode) is left for the reset path that lands with real
    // rendering. Windowed mode rarely loses the device.
  }
}

}  // namespace ghogx::render
