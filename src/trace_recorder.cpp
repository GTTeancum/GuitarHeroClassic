// trace_recorder.cpp - see header for purpose.

#include "trace_recorder.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace trace360 {

namespace {

std::atomic<bool> g_capturing{false};
std::atomic<bool> g_initialized{false};
std::mutex        g_mu;
FILE*             g_fp = nullptr;
std::chrono::steady_clock::time_point g_t0;

uint64_t monotonic_ns() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<nanoseconds>(steady_clock::now() - g_t0).count());
}

// JSON-escape a string view into the output buffer. Conservative: only
// escapes the characters JSON requires. Strings are short (paths, names),
// so this isn't worth a fancier library.
void json_escape(std::string& out, std::string_view s) {
    out.reserve(out.size() + s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
}

// Write one jsonl line. Assumes g_mu held.
void write_line_locked(const std::string& line) {
    if (!g_fp) return;
    std::fwrite(line.data(), 1, line.size(), g_fp);
    std::fputc('\n', g_fp);
}

// Common prefix: opening brace, timestamp, kind. Caller appends remaining
// fields and the closing brace.
void start_event(std::string& out, std::string_view kind) {
    out.clear();
    char head[64];
    std::snprintf(head, sizeof(head), "{\"t\":%llu,\"kind\":\"",
                  static_cast<unsigned long long>(monotonic_ns()));
    out += head;
    json_escape(out, kind);
    out += "\"";
}

void end_event(std::string& out) {
    out += "}";
}

}  // anonymous namespace

bool Init(const std::string& output_path) {
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) {
        return true;  // already initialized
    }
    g_t0 = std::chrono::steady_clock::now();
    g_fp = std::fopen(output_path.c_str(), "wb");
    if (!g_fp) {
        g_initialized = false;
        return false;
    }
    // Header event so the file is never empty even if nothing else is captured.
    std::string line;
    start_event(line, "trace.init");
    line += ",\"path\":\"";
    json_escape(line, output_path);
    line += "\"";
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
    std::fflush(g_fp);
    return true;
}

void Shutdown() {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_fp) {
        std::fflush(g_fp);
        std::fclose(g_fp);
        g_fp = nullptr;
    }
    g_initialized = false;
    g_capturing = false;
}

void SetCapturing(bool on) {
    bool prev = g_capturing.exchange(on);
    if (prev == on) return;
    std::string line;
    start_event(line, on ? "capture.on" : "capture.off");
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
    if (g_fp) std::fflush(g_fp);
}

bool IsCapturing() {
    return g_capturing.load(std::memory_order_relaxed);
}

void Flush() {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_fp) std::fflush(g_fp);
}

void LogFileOpen(std::string_view path, uint64_t offset, uint32_t size, bool found) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, "file.open");
    line += ",\"path\":\"";
    json_escape(line, path);
    char tail[96];
    std::snprintf(tail, sizeof(tail),
                  "\",\"off\":%llu,\"size\":%u,\"found\":%s",
                  static_cast<unsigned long long>(offset), size,
                  found ? "true" : "false");
    line += tail;
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogPropertyLookup(std::string_view cls, std::string_view prop,
                       uint32_t returned_ptr) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, "prop.lookup");
    line += ",\"class\":\"";
    json_escape(line, cls);
    line += "\",\"prop\":\"";
    json_escape(line, prop);
    char tail[48];
    std::snprintf(tail, sizeof(tail), "\",\"ret\":\"0x%08x\"", returned_ptr);
    line += tail;
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogHandlerLookup(std::string_view name, uint32_t returned_ptr) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, "handler.lookup");
    line += ",\"name\":\"";
    json_escape(line, name);
    char tail[48];
    std::snprintf(tail, sizeof(tail), "\",\"ret\":\"0x%08x\"", returned_ptr);
    line += tail;
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogClassLookup(std::string_view class_name, uint32_t returned_ptr) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, "class.lookup");
    line += ",\"class\":\"";
    json_escape(line, class_name);
    char tail[48];
    std::snprintf(tail, sizeof(tail), "\",\"ret\":\"0x%08x\"", returned_ptr);
    line += tail;
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogAudioSubmit(uint32_t stream_id, uint32_t buffer_len) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, "audio.submit");
    char tail[64];
    std::snprintf(tail, sizeof(tail), ",\"stream\":%u,\"len\":%u",
                  stream_id, buffer_len);
    line += tail;
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogFrame(uint64_t frame_index) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, "frame");
    char tail[48];
    std::snprintf(tail, sizeof(tail), ",\"n\":%llu",
                  static_cast<unsigned long long>(frame_index));
    line += tail;
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogEvent(std::string_view kind, std::string_view detail) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
    std::string line;
    start_event(line, kind);
    line += ",\"detail\":\"";
    json_escape(line, detail);
    line += "\"";
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
}

void LogStackSample(std::string_view tag) {
    if (!g_capturing.load(std::memory_order_relaxed)) return;
#ifdef _WIN32
    static std::once_flag sym_init_once;
    std::call_once(sym_init_once, []() {
        SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
    });
    constexpr USHORT kMaxFrames = 32;
    void* frames[kMaxFrames];
    USHORT n = CaptureStackBackTrace(0, kMaxFrames, frames, nullptr);

    constexpr size_t kNameBytes = sizeof(SYMBOL_INFO) + 256;
    auto* sym = static_cast<SYMBOL_INFO*>(_alloca(kNameBytes));
    std::memset(sym, 0, kNameBytes);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = 255;

    std::string line;
    start_event(line, "stack");
    line += ",\"tag\":\"";
    json_escape(line, tag);
    line += "\",\"frames\":[";
    HANDLE proc = GetCurrentProcess();
    for (USHORT i = 0; i < n; ++i) {
        DWORD64 disp = 0;
        const char* name = "?";
        if (SymFromAddr(proc, reinterpret_cast<DWORD64>(frames[i]), &disp, sym)) {
            name = sym->Name;
        }
        if (i) line += ",";
        line += "\"";
        json_escape(line, name);
        line += "\"";
    }
    line += "]";
    end_event(line);
    std::lock_guard<std::mutex> lk(g_mu);
    write_line_locked(line);
#else
    (void)tag;
#endif
}

}  // namespace trace360
