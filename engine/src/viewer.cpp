// ghogx_viewer - Win32 audiovisual demo.
//
// Opens a window, decodes a PS2 HMXBitmap (.bmp_ps2 / .png_ps2) from a
// Harmonix ARK, blits it to the window via StretchDIBits, and plays a
// decoded VGS audio stem in the background via the Win32 WaveOut API.
// No SDL, no game-engine runtime; just the readers under tools/ plus
// stock win32/winmm/gdi.
//
// Usage:
//   ghogx_viewer --ark-dir <dir> --tex-path <p> --vgs-path <p>
//   ghogx_viewer --hdr <p> --ark <p> --tex-path <p> --vgs-path <p>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#include "ark_v3.h"
#include "ps2_texture.h"
#include "vgs.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Args {
    std::string hdr;
    std::string ark;
    std::string tex_path;
    std::string vgs_path;
    int win_w = 800;
    int win_h = 600;
};

void usage_and_exit() {
    std::fprintf(stderr,
        "ghogx_viewer\n"
        "\n"
        "Usage:\n"
        "  ghogx_viewer --ark-dir <dir> --tex-path <ark-path>\n"
        "              [--vgs-path <ark-path>] [--w N --h N]\n"
        "  ghogx_viewer --hdr <p> --ark <p> --tex-path <p>\n"
        "              [--vgs-path <p>] [--w N --h N]\n");
    std::exit(2);
}

Args parse_args(int argc, char** argv) {
    Args a;
    std::string ark_dir;
    for (int i = 1; i < argc; ++i) {
        std::string_view k = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { std::fprintf(stderr, "%s requires a value\n", name); std::exit(2); }
            return argv[++i];
        };
        if      (k == "--ark-dir")  ark_dir = need("--ark-dir");
        else if (k == "--hdr")      a.hdr = need("--hdr");
        else if (k == "--ark")      a.ark = need("--ark");
        else if (k == "--tex-path") a.tex_path = need("--tex-path");
        else if (k == "--vgs-path") a.vgs_path = need("--vgs-path");
        else if (k == "--w")        a.win_w = std::atoi(need("--w"));
        else if (k == "--h")        a.win_h = std::atoi(need("--h"));
        else if (k == "-h" || k == "--help") usage_and_exit();
        else { std::fprintf(stderr, "unknown arg: %s\n", argv[i]); usage_and_exit(); }
    }
    if (!ark_dir.empty()) {
        fs::path d = ark_dir;
        for (auto n : {"main.hdr", "MAIN.HDR"})  if (fs::exists(d / n)) { a.hdr = (d / n).string(); break; }
        for (auto n : {"main_0.ark", "MAIN_0.ARK"}) if (fs::exists(d / n)) { a.ark = (d / n).string(); break; }
    }
    if (a.hdr.empty() || a.ark.empty() || a.tex_path.empty()) usage_and_exit();
    return a;
}

std::optional<gh::ark::Entry> find_in_ark(const gh::ark::ArkV3Reader& ark,
                                          const std::string& p) {
    auto e = ark.find(p);
    if (!e) e = ark.find("../../system/run/" + p);
    return e;
}

// ---------- visuals ----------

struct Frame {
    int w = 0;
    int h = 0;
    std::vector<uint8_t> bgra;  // top-down BGRA8, what GDI wants
    std::string title;
};

Frame load_texture_from_ark(const gh::ark::ArkV3Reader& ark,
                            const std::string& ark_file,
                            const std::string& path) {
    auto e = find_in_ark(ark, path);
    if (!e) throw std::runtime_error("texture not found in ARK: " + path);
    auto bytes = ark.read_entry(*e, {ark_file});
    auto bm    = gh::tex::parse(bytes);
    auto rgba  = gh::tex::decode_to_rgba(bm);

    Frame f{};
    f.w = bm.width;
    f.h = bm.height;
    f.bgra.resize(rgba.size());
    for (size_t i = 0; i < rgba.size(); i += 4) {
        f.bgra[i + 0] = rgba[i + 2];
        f.bgra[i + 1] = rgba[i + 1];
        f.bgra[i + 2] = rgba[i + 0];
        f.bgra[i + 3] = rgba[i + 3];
    }
    f.title = "ghogx_viewer - " + path + " (" + std::to_string(bm.width) + "x"
              + std::to_string(bm.height) + ", " + std::to_string(bm.bpp) + "bpp)";
    return f;
}

// ---------- audio ----------

struct Audio {
    HWAVEOUT h = nullptr;
    WAVEHDR  hdr{};
    std::vector<int16_t> pcm;
    bool started = false;
    bool open(int channels, int sample_rate) {
        WAVEFORMATEX fmt{};
        fmt.wFormatTag      = WAVE_FORMAT_PCM;
        fmt.nChannels       = static_cast<WORD>(channels);
        fmt.nSamplesPerSec  = static_cast<DWORD>(sample_rate);
        fmt.wBitsPerSample  = 16;
        fmt.nBlockAlign     = static_cast<WORD>(channels * 2);
        fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
        MMRESULT r = waveOutOpen(&h, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
        return r == MMSYSERR_NOERROR;
    }
    bool play(std::vector<int16_t>&& samples) {
        pcm = std::move(samples);
        hdr = WAVEHDR{};
        hdr.lpData         = reinterpret_cast<LPSTR>(pcm.data());
        hdr.dwBufferLength = static_cast<DWORD>(pcm.size() * sizeof(int16_t));
        hdr.dwLoops        = 0;
        if (waveOutPrepareHeader(h, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) return false;
        if (waveOutWrite(h, &hdr, sizeof(hdr))         != MMSYSERR_NOERROR) return false;
        started = true;
        return true;
    }
    ~Audio() {
        if (h) {
            waveOutReset(h);
            if (started) waveOutUnprepareHeader(h, &hdr, sizeof(hdr));
            waveOutClose(h);
        }
    }
};

// ---------- window proc ----------

struct WindowState {
    Frame* frame = nullptr;
    std::atomic<bool> quit{false};
};

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<WindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            if (st && st->frame) {
                BITMAPINFO bi{};
                bi.bmiHeader.biSize        = sizeof(bi.bmiHeader);
                bi.bmiHeader.biWidth       = st->frame->w;
                bi.bmiHeader.biHeight      = -st->frame->h;  // negative => top-down
                bi.bmiHeader.biPlanes      = 1;
                bi.bmiHeader.biBitCount    = 32;
                bi.bmiHeader.biCompression = BI_RGB;
                SetStretchBltMode(dc, HALFTONE);
                StretchDIBits(dc,
                              0, 0, rc.right, rc.bottom,
                              0, 0, st->frame->w, st->frame->h,
                              st->frame->bgra.data(), &bi, DIB_RGB_COLORS, SRCCOPY);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE || wp == 'Q') {
                if (st) st->quit = true;
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;
        case WM_CLOSE:
            if (st) st->quit = true;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

}  // anonymous namespace

int main(int argc, char** argv) {
    Args a = parse_args(argc, argv);

    try {
        std::fprintf(stderr, "[viewer] loading ARK\n");
        auto ark = gh::ark::ArkV3Reader::load(a.hdr);
        std::fprintf(stderr, "[viewer] %zu entries (v%u)\n",
                     ark.entries().size(), ark.version());

        std::fprintf(stderr, "[viewer] decoding texture %s\n", a.tex_path.c_str());
        Frame frame = load_texture_from_ark(ark, a.ark, a.tex_path);
        std::fprintf(stderr, "[viewer] texture %dx%d ready\n", frame.w, frame.h);

        // Optional audio.
        Audio audio;
        if (!a.vgs_path.empty()) {
            std::fprintf(stderr, "[viewer] decoding VGS %s\n", a.vgs_path.c_str());
            auto e = find_in_ark(ark, a.vgs_path);
            if (!e) {
                std::fprintf(stderr, "[viewer] VGS not found in ARK: %s\n", a.vgs_path.c_str());
            } else {
                auto bytes = ark.read_entry(*e, {a.ark});
                auto h = gh::vgs::parse_header(bytes);
                auto pcm = gh::vgs::decode_pcm_s16(bytes, h);
                std::fprintf(stderr, "[viewer] audio %d ch / %d Hz / %.1f sec\n",
                             h.channels, h.sample_rate,
                             static_cast<double>(pcm.size())
                                 / (h.sample_rate * (h.channels == 0 ? 1 : h.channels)));
                if (audio.open(h.channels, h.sample_rate)) {
                    if (!audio.play(std::move(pcm))) {
                        std::fprintf(stderr, "[viewer] waveOutWrite failed\n");
                    }
                } else {
                    std::fprintf(stderr, "[viewer] waveOutOpen failed\n");
                }
            }
        }

        // ---- window ----
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = wnd_proc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = L"ghogx_viewer";
        RegisterClassExW(&wc);

        WindowState st{};
        st.frame = &frame;

        // Convert title to wide for SetWindowTextW; ASCII fast-path.
        std::wstring wtitle(frame.title.begin(), frame.title.end());

        HWND hwnd = CreateWindowExW(0, wc.lpszClassName, wtitle.c_str(),
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    a.win_w, a.win_h,
                                    nullptr, nullptr, wc.hInstance, nullptr);
        if (!hwnd) {
            std::fprintf(stderr, "[viewer] CreateWindow failed\n");
            return 1;
        }
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&st));
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg{};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[viewer] FATAL: %s\n", e.what());
        return 1;
    }
}
