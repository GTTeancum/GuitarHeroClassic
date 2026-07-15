// trace_hooks.cpp - REX_HOOK_RAW wrappers that feed the trace_recorder.
//
// Each hook here wraps a known-named guest function (see harmonix_symbols.h
// / recomp_symbols.md), captures the salient inputs/outputs, delegates to
// the original via __imp__, and writes a structured event to the recorder.
//
// All hooks are pass-through (delegate to __imp__) so the recompile runs
// normally. Adding hooks here doesn't change behavior, only observability.

#include "harmonix_symbols.h"
#include "trace_recorder.h"

#include "generated/gh2test_init.h"

#include <rex/hook.h>
#include <rex/logging.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if REX_PLATFORM_WIN32
#include <Windows.h>
#endif

namespace {

// Read a null-terminated guest string with a bounded scan so a stray
// pointer can't run off the end of memory.
std::string read_guest_string(uint8_t* base, uint32_t guest_addr) {
    if (!guest_addr) return {};
    const char* p = reinterpret_cast<const char*>(base + guest_addr);
    constexpr size_t kMax = 1024;
    size_t n = 0;
    while (n < kMax && p[n]) ++n;
    return std::string(p, n);
}

// Tracks which class / prop names we've already stack-sampled, so the
// host stack capture runs ONCE per unique name. Bounded; tiny.
std::mutex g_seen_mu;
std::unordered_set<std::string> g_class_stack_seen;
std::unordered_set<std::string> g_prop_stack_seen;
std::unordered_set<std::string> g_file_stack_seen;
thread_local int g_selection_dispatch_depth = 0;
std::atomic<int> g_selection_arg_window{0};

bool trace_cmdline_has_flag(const char* flag) {
#if REX_PLATFORM_WIN32
    const char* cmd = GetCommandLineA();
    return cmd && std::strstr(cmd, flag) != nullptr;
#else
    (void)flag;
    return false;
#endif
}

bool trace_scripted_nav_enabled() {
    static const bool enabled = trace_cmdline_has_flag("--trace_scripted_nav");
    return enabled;
}

uint16_t scripted_nav_buttons_ms(uint64_t elapsed_ms) {
    static constexpr uint16_t kA = 0x1000;
    static constexpr uint16_t kDown = 0x0002;
    struct Step {
        uint32_t start_ms;
        uint16_t buttons;
    };
    static constexpr Step kSteps[] = {
        {0, kA},        // controller gate/title: make the first poll an edge
        {6250, kA},     // main menu: confirm
        {10500, kDown}, // move to Quick Play
        {12200, kA},    // select Quick Play
        {17000, kA},    // default difficulty
        {22000, kA},    // default character
        {27000, kA},    // default guitar
        {32000, kA},    // default venue
        {37500, kA},    // first song
        {44500, kA},    // song confirm
    };
    static constexpr uint32_t kPressMs = 220;
    for (const Step& step : kSteps) {
        if (elapsed_ms >= step.start_ms && elapsed_ms < step.start_ms + kPressMs) {
            return step.buttons;
        }
    }
    return 0;
}

void log_periodic_call(const char* kind, std::atomic<uint32_t>& counter,
                       uint32_t this_ptr, uint32_t arg0 = 0,
                       uint32_t arg1 = 0) {
    const uint32_t n = counter.fetch_add(1, std::memory_order_relaxed);
    if (n < 16 || (n < 1024 && (n % 64u) == 0) || (n % 512u) == 0) {
        char detail[128];
        std::snprintf(detail, sizeof detail,
                      "n=%u this=0x%08X arg0=0x%08X arg1=0x%08X", n,
                      this_ptr, arg0, arg1);
        trace360::LogEvent(kind, detail);
    }
    if (n < 4) trace360::LogStackSample(kind);
}

std::string read_channel_name(uint8_t* base, uint32_t table, int index) {
    if (!table || index < 0 || index > 8192) return {};
    const uint32_t sym = REX_LOAD_U32(table + static_cast<uint32_t>(index) * 8u);
    return read_guest_string(base, sym);
}

std::string summarize_anim_table(uint8_t* base, uint32_t table_obj) {
    if (!table_obj) return "null";
    const uint32_t channels = REX_LOAD_U32(table_obj + 12);
    const uint32_t end_ptr = REX_LOAD_U32(table_obj + 16);
    const int total = (channels && end_ptr >= channels)
                          ? static_cast<int>((end_ptr - channels) / 8u)
                          : -1;
    const int c0 = static_cast<int>(REX_LOAD_U32(table_obj + 24));
    const int c1 = static_cast<int>(REX_LOAD_U32(table_obj + 28));
    const int c2 = static_cast<int>(REX_LOAD_U32(table_obj + 32));
    const int c3 = static_cast<int>(REX_LOAD_U32(table_obj + 36));
    const int c4 = static_cast<int>(REX_LOAD_U32(table_obj + 40));
    const int c5 = static_cast<int>(REX_LOAD_U32(table_obj + 44));
    const int c6 = static_cast<int>(REX_LOAD_U32(table_obj + 60));
    const int compressed = static_cast<int>(REX_LOAD_U32(table_obj + 8));
    const int stride = static_cast<int>(REX_LOAD_U32(table_obj + 104));
    char head[256];
    std::snprintf(head, sizeof head,
                  "obj=0x%08X ch=0x%08X total=%d compressed=%d stride=%d "
                  "bounds=[%d,%d,%d,%d,%d,%d,%d]",
                  table_obj, channels, total, compressed, stride, c0, c1, c2,
                  c3, c4, c5, c6);
    std::string out = head;
    out += " first={";
    if (channels && total > 0) {
        const int limit = std::min(total, 14);
        for (int i = 0; i < limit; ++i) {
            if (i) out += ",";
            std::string name = read_channel_name(base, channels, i);
            out += name.empty() ? "?" : name;
        }
        if (total > limit) out += ",...";
    }
    out += "}";
    auto append_names = [&](const char* label, int begin, int end) {
        out += " ";
        out += label;
        out += "={";
        int emitted = 0;
        if (channels && begin >= 0 && end >= begin && end <= total) {
            for (int i = begin; i < end && emitted < 6; ++i, ++emitted) {
                if (emitted) out += ",";
                std::string name = read_channel_name(base, channels, i);
                out += name.empty() ? "?" : name;
            }
            if (end - begin > emitted) out += ",...";
        }
        out += "}";
    };
    // Keep raw table-order names above as the authority. These category labels
    // are only a quick read aid while we pin the exact category-bound offsets.
    append_names("cat0", c0, c1);
    append_names("cat1", c1, c2);
    append_names("cat2", c2, c3);
    append_names("catR", c5, c6);
    return out;
}

bool looks_like_body_anim(const std::string& detail) {
    return detail.find("thigh") != std::string::npos ||
           detail.find("knee") != std::string::npos ||
           detail.find("ankle") != std::string::npos ||
           detail.find("foot") != std::string::npos;
}

float read_f32(uint8_t* base, uint32_t guest_addr) {
    PPCRegister temp{};
    temp.u32 = REX_LOAD_U32(guest_addr);
    return temp.f32;
}

std::string fmt_f(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.4f", v);
    return buf;
}

std::string fmt_ptr(uint32_t v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%08X", v);
    return buf;
}

std::string fmt_u16(uint16_t v) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "0x%04X", static_cast<unsigned>(v));
    return buf;
}

std::string fmt_vec3(float x, float y, float z) {
    return "(" + fmt_f(x) + "," + fmt_f(y) + "," + fmt_f(z) + ")";
}

std::string fmt_quat(float x, float y, float z, float w) {
    return "(" + fmt_f(x) + "," + fmt_f(y) + "," + fmt_f(z) + "," +
           fmt_f(w) + ")";
}

int find_channel_index(uint8_t* base, uint32_t table_obj,
                       const char* channel_name) {
    if (!table_obj || !channel_name) return -1;
    const uint32_t channels = REX_LOAD_U32(table_obj + 12);
    const uint32_t end_ptr = REX_LOAD_U32(table_obj + 16);
    if (!channels || end_ptr < channels) return -1;
    const int total = static_cast<int>((end_ptr - channels) / 8u);
    for (int i = 0; i < total && i < 8192; ++i) {
        if (read_channel_name(base, channels, i) == channel_name) return i;
    }
    return -1;
}

int count_category_before(uint8_t* base, uint32_t table_obj, int begin,
                          int channel_index, const char* suffix) {
    const uint32_t channels = REX_LOAD_U32(table_obj + 12);
    int count = 0;
    for (int i = begin; i < channel_index; ++i) {
        std::string name = read_channel_name(base, channels, i);
        if (!suffix || name.rfind(suffix) != std::string::npos) ++count;
    }
    return count;
}

std::string read_anim_value(uint8_t* base, uint32_t table_obj,
                            const char* channel_name) {
    const int index = find_channel_index(base, table_obj, channel_name);
    if (index < 0) return "missing";

    const std::string name(channel_name);
    const bool compressed = REX_LOAD_U32(table_obj + 8) != 0;
    const int vec_begin = static_cast<int>(REX_LOAD_U32(table_obj + 24));
    const int vec_end = static_cast<int>(REX_LOAD_U32(table_obj + 32));
    const int quat_begin = static_cast<int>(REX_LOAD_U32(table_obj + 32));
    const int quat_end = static_cast<int>(REX_LOAD_U32(table_obj + 36));
    const int rot_begin = static_cast<int>(REX_LOAD_U32(table_obj + 36));
    const int rot_end = static_cast<int>(REX_LOAD_U32(table_obj + 60));

    if ((name.ends_with(".pos") || name.ends_with(".scale")) &&
        index >= vec_begin && index < vec_end) {
        const uint32_t values = REX_LOAD_U32(table_obj + 108);
        const int ordinal = index - vec_begin;
        const uint32_t addr = values + static_cast<uint32_t>(ordinal) * 16u;
        return fmt_vec3(read_f32(base, addr + 0), read_f32(base, addr + 4),
                        read_f32(base, addr + 8));
    }

    if (name.ends_with(".quat") && index >= quat_begin && index < quat_end) {
        const uint32_t values = REX_LOAD_U32(table_obj + 116);
        const int ordinal = index - quat_begin;
        if (compressed) {
            const uint32_t addr =
                values + static_cast<uint32_t>(ordinal) * 8u;
            constexpr float k = 1.0f / 32767.0f;
            const float x = static_cast<int16_t>(REX_LOAD_U16(addr + 0)) * k;
            const float y = static_cast<int16_t>(REX_LOAD_U16(addr + 2)) * k;
            const float z = static_cast<int16_t>(REX_LOAD_U16(addr + 4)) * k;
            const float w = static_cast<int16_t>(REX_LOAD_U16(addr + 6)) * k;
            return fmt_quat(x, y, z, w);
        }
        const uint32_t addr =
            values + static_cast<uint32_t>(ordinal) * 16u;
        return fmt_quat(read_f32(base, addr + 0), read_f32(base, addr + 4),
                        read_f32(base, addr + 8), read_f32(base, addr + 12));
    }

    if ((name.ends_with(".rotx") || name.ends_with(".roty") ||
         name.ends_with(".rotz")) &&
        index >= rot_begin && index < rot_end) {
        const uint32_t values = REX_LOAD_U32(table_obj + 120);
        const int ordinal = count_category_before(base, table_obj, rot_begin,
                                                  index, ".rot");
        if (compressed) {
            const uint32_t addr =
                values + static_cast<uint32_t>(ordinal) * 2u;
            char buf[48];
            std::snprintf(buf, sizeof buf, "i16:%d",
                          static_cast<int16_t>(REX_LOAD_U16(addr)));
            return buf;
        }
        const uint32_t addr =
            values + static_cast<uint32_t>(ordinal) * 4u;
        return fmt_f(read_f32(base, addr));
    }

    return "present-unread";
}

bool lower_body_memory_trace_enabled() {
    static const bool enabled = trace_cmdline_has_flag("--trace-lower-body-memory");
    return enabled;
}

std::string raw_u32_words(uint8_t* base, uint32_t addr, int words) {
    std::string out = "[";
    for (int i = 0; i < words; ++i) {
        if (i) out += ",";
        out += fmt_ptr(REX_LOAD_U32(addr + static_cast<uint32_t>(i) * 4u));
    }
    out += "]";
    return out;
}

std::string raw_u16_words(uint8_t* base, uint32_t addr, int words) {
    std::string out = "[";
    for (int i = 0; i < words; ++i) {
        if (i) out += ",";
        out += fmt_u16(REX_LOAD_U16(addr + static_cast<uint32_t>(i) * 2u));
    }
    out += "]";
    return out;
}

std::string summarize_guest_object(uint8_t* base, uint32_t obj);

bool looks_like_guest_pointer(uint32_t addr) {
    return addr >= 0x40000000u && addr < 0x83000000u;
}

std::string describe_raw_words(uint8_t* base, const char* label,
                               uint32_t addr, int words) {
    std::string out(label);
    out += "=";
    if (!looks_like_guest_pointer(addr)) {
        out += fmt_ptr(addr);
        out += "(not_ptr)";
        return out;
    }
    out += fmt_ptr(addr);
    out += ":";
    out += raw_u32_words(base, addr, words);
    return out;
}

void log_lower_body_runtime_memory(uint8_t* base, const char* stage,
                                   const char* phase, uint32_t self,
                                   uint32_t arg0, uint32_t arg1) {
    if (!lower_body_memory_trace_enabled()) return;
    static std::atomic<uint32_t> s_runtime_dump_count{0};
    const uint32_t seq =
        s_runtime_dump_count.fetch_add(1, std::memory_order_relaxed);
    if (seq >= 192) return;

    std::string detail = "seq=" + std::to_string(seq);
    detail += " stage=";
    detail += stage;
    detail += " phase=";
    detail += phase;
    detail += " self=";
    detail += fmt_ptr(self);
    detail += " arg0=";
    detail += fmt_ptr(arg0);
    detail += " arg1=";
    detail += fmt_ptr(arg1);
    detail += " obj={";
    detail += summarize_guest_object(base, self);
    detail += "} ";
    detail += describe_raw_words(base, "self0", self, 16);
    detail += " ";
    detail += describe_raw_words(base, "self64", self + 64, 16);
    detail += " ";
    detail += describe_raw_words(base, "arg0", arg0, 12);
    detail += " ";
    detail += describe_raw_words(base, "arg1", arg1, 8);
    trace360::LogEvent("anim.lower_body.runtime_memory", detail);
}

std::string describe_anim_table_memory_header(uint8_t* base,
                                              uint32_t table_obj) {
    (void)base;
    if (!table_obj) return "null";
    const uint32_t channels = REX_LOAD_U32(table_obj + 12);
    const uint32_t end_ptr = REX_LOAD_U32(table_obj + 16);
    const int total = (channels && end_ptr >= channels)
                          ? static_cast<int>((end_ptr - channels) / 8u)
                          : -1;
    char buf[512];
    std::snprintf(
        buf, sizeof buf,
        "obj=%s ch=%s end=%s total=%d compressed=%u stride=%u "
        "bounds=[%u,%u,%u,%u,%u,%u,%u] values=[pos=%s quat=%s rot=%s]",
        fmt_ptr(table_obj).c_str(), fmt_ptr(channels).c_str(),
        fmt_ptr(end_ptr).c_str(), total, REX_LOAD_U32(table_obj + 8),
        REX_LOAD_U32(table_obj + 104), REX_LOAD_U32(table_obj + 24),
        REX_LOAD_U32(table_obj + 28), REX_LOAD_U32(table_obj + 32),
        REX_LOAD_U32(table_obj + 36), REX_LOAD_U32(table_obj + 40),
        REX_LOAD_U32(table_obj + 44), REX_LOAD_U32(table_obj + 60),
        fmt_ptr(REX_LOAD_U32(table_obj + 108)).c_str(),
        fmt_ptr(REX_LOAD_U32(table_obj + 116)).c_str(),
        fmt_ptr(REX_LOAD_U32(table_obj + 120)).c_str());
    return buf;
}

std::string describe_anim_channel_memory(uint8_t* base, uint32_t table_obj,
                                         const char* channel_name) {
    const int index = find_channel_index(base, table_obj, channel_name);
    if (index < 0) return {};

    const std::string name(channel_name);
    const bool compressed = REX_LOAD_U32(table_obj + 8) != 0;
    const int vec_begin = static_cast<int>(REX_LOAD_U32(table_obj + 24));
    const int vec_end = static_cast<int>(REX_LOAD_U32(table_obj + 32));
    const int quat_begin = static_cast<int>(REX_LOAD_U32(table_obj + 32));
    const int quat_end = static_cast<int>(REX_LOAD_U32(table_obj + 36));
    const int rot_begin = static_cast<int>(REX_LOAD_U32(table_obj + 36));
    const int rot_end = static_cast<int>(REX_LOAD_U32(table_obj + 60));

    std::string category;
    uint32_t addr = 0;
    std::string raw;
    if ((name.ends_with(".pos") || name.ends_with(".scale")) &&
        index >= vec_begin && index < vec_end) {
        category = name.ends_with(".pos") ? "pos" : "scale";
        const uint32_t values = REX_LOAD_U32(table_obj + 108);
        const int ordinal = index - vec_begin;
        addr = values + static_cast<uint32_t>(ordinal) * 16u;
        raw = raw_u32_words(base, addr, 4);
    } else if (name.ends_with(".quat") && index >= quat_begin &&
               index < quat_end) {
        category = "quat";
        const uint32_t values = REX_LOAD_U32(table_obj + 116);
        const int ordinal = index - quat_begin;
        if (compressed) {
            addr = values + static_cast<uint32_t>(ordinal) * 8u;
            raw = raw_u16_words(base, addr, 4);
        } else {
            addr = values + static_cast<uint32_t>(ordinal) * 16u;
            raw = raw_u32_words(base, addr, 4);
        }
    } else if ((name.ends_with(".rotx") || name.ends_with(".roty") ||
                name.ends_with(".rotz")) &&
               index >= rot_begin && index < rot_end) {
        category = "rot";
        const uint32_t values = REX_LOAD_U32(table_obj + 120);
        const int ordinal =
            count_category_before(base, table_obj, rot_begin, index, ".rot");
        if (compressed) {
            addr = values + static_cast<uint32_t>(ordinal) * 2u;
            raw = raw_u16_words(base, addr, 1);
        } else {
            addr = values + static_cast<uint32_t>(ordinal) * 4u;
            raw = raw_u32_words(base, addr, 1);
        }
    } else {
        category = "unread";
    }

    char head[256];
    std::snprintf(head, sizeof head,
                  "%s{idx=%d cat=%s addr=%s value=%s raw=",
                  channel_name, index, category.c_str(), fmt_ptr(addr).c_str(),
                  read_anim_value(base, table_obj, channel_name).c_str());
    std::string out = head;
    out += raw.empty() ? "[]" : raw;
    out += "}";
    return out;
}

std::string describe_lower_body_memory_rows(uint8_t* base,
                                            uint32_t table_obj) {
    static constexpr const char* kChannels[] = {
        "bone_facing.pos",     "bone_pelvis.pos",
        "bone_pelvis.quat",    "bone_L-thigh.quat",
        "bone_R-thigh.quat",   "bone_L-ankle.quat",
        "bone_R-ankle.quat",   "bone_L-foot.quat",
        "bone_R-foot.quat",    "bone_L-knee.rotz",
        "bone_R-knee.rotz",    "bone_L-toe.rotz",
        "bone_R-toe.rotz",     "bone_L-toe0.rotz",
        "bone_R-toe0.rotz",
    };
    std::string out = "{";
    bool first = true;
    for (const char* channel : kChannels) {
        std::string row = describe_anim_channel_memory(base, table_obj, channel);
        if (row.empty()) continue;
        if (!first) out += " ";
        first = false;
        out += row;
    }
    out += "}";
    return out;
}

void log_lower_body_memory_dump(uint8_t* base, const char* apply_kind,
                                const char* phase, uint32_t seq,
                                uint32_t src, uint32_t dst, float weight) {
    std::string detail = "seq=" + std::to_string(seq);
    detail += " apply=";
    detail += apply_kind;
    detail += " phase=";
    detail += phase;
    detail += " weight=";
    detail += fmt_f(weight);
    detail += " src_header=[";
    detail += describe_anim_table_memory_header(base, src);
    detail += "] dst_header=[";
    detail += describe_anim_table_memory_header(base, dst);
    detail += "] src_rows=";
    detail += describe_lower_body_memory_rows(base, src);
    detail += " dst_rows=";
    detail += describe_lower_body_memory_rows(base, dst);
    trace360::LogEvent("anim.lower_body.memory", detail);
}

std::string capture_selected_anim_values(uint8_t* base, uint32_t src,
                                         uint32_t dst) {
    static constexpr const char* kChannels[] = {
        "bone_pelvis.pos",
        "bone_L-thigh.quat",
        "bone_R-thigh.quat",
        "bone_L-ankle.quat",
        "bone_R-ankle.quat",
        "bone_L-knee.rotz",
        "bone_R-knee.rotz",
        "bone_L-upperArm.quat",
        "bone_R-upperArm.quat",
        "bone_L-clavicle.quat",
        "bone_R-clavicle.quat",
    };

    std::string out;
    for (const char* channel : kChannels) {
        const std::string s = read_anim_value(base, src, channel);
        const std::string d = read_anim_value(base, dst, channel);
        if (s == "missing" && d == "missing") continue;
        out += " ";
        out += channel;
        out += "{src=";
        out += s;
        out += " dst=";
        out += d;
        out += "}";
    }
    return out;
}

bool is_probably_guest_ptr(uint32_t p) {
    return (p >= 0x40000000u && p < 0x50000000u) ||
           (p >= 0x82000000u && p < 0x83000000u);
}

bool is_printable_ascii(const std::string& s) {
    if (s.empty() || s.size() > 96) return false;
    for (char c : s) {
        if (c < 32 || c >= 127) return false;
    }
    return true;
}

std::string describe_datanode(uint8_t* base, uint32_t node) {
    if (!node) return "null";
    const uint32_t payload = REX_LOAD_U32(node + 0);
    const uint32_t type = REX_LOAD_U32(node + 4);
    char head[80];
    std::snprintf(head, sizeof head, "type=%u payload=0x%08X", type, payload);
    std::string out = head;
    if (type == 2 || type == 17) {
        char b[40];
        std::snprintf(b, sizeof b, " int=%d", static_cast<int32_t>(payload));
        out += b;
    }
    if (is_probably_guest_ptr(payload)) {
        std::string s = read_guest_string(base, payload);
        if (is_printable_ascii(s)) {
            out += " str=";
            out += s;
        }
    }
    return out;
}

std::string summarize_dataarray(uint8_t* base, uint32_t arr) {
    if (!arr) return "arr=null";
    const uint32_t nodes = REX_LOAD_U32(arr + 0);
    const int count = static_cast<int16_t>(REX_LOAD_U16(arr + 8));
    char head[80];
    std::snprintf(head, sizeof head, "arr=0x%08X nodes=0x%08X count=%d",
                  arr, nodes, count);
    std::string out = head;
    if (!nodes || count < 0 || count > 256) return out;
    const int limit = std::min(count, 10);
    for (int i = 0; i < limit; ++i) {
        char prefix[24];
        std::snprintf(prefix, sizeof prefix, " n%d{", i);
        out += prefix;
        out += describe_datanode(base, nodes + static_cast<uint32_t>(i) * 8u);
        out += "}";
    }
    if (count > limit) out += " ...";
    return out;
}

std::string summarize_object_strings(uint8_t* base, uint32_t obj) {
    if (!obj) return "object=null";
    std::string out = "object_strings=";
    int found = 0;
    for (int w = 0; w < 32 && found < 6; ++w) {
        const uint32_t p = REX_LOAD_U32(obj + static_cast<uint32_t>(w) * 4u);
        if (!is_probably_guest_ptr(p)) continue;
        std::string s = read_guest_string(base, p);
        if (!is_printable_ascii(s)) continue;
        if (found++) out += ",";
        char prefix[16];
        std::snprintf(prefix, sizeof prefix, "w%d:", w);
        out += prefix;
        out += s;
    }
    if (!found) out += "(none)";
    return out;
}

std::string summarize_guest_object(uint8_t* base, uint32_t obj) {
    if (!obj) return "obj=null";
    std::string out = "obj=" + fmt_ptr(obj);
    const uint32_t vt = REX_LOAD_U32(obj + 0);
    out += " vt=" + fmt_ptr(vt);
    if (vt) {
        out += " slots={";
        for (int i = 0; i < 4; ++i) {
            if (i) out += ",";
            out += fmt_ptr(REX_LOAD_U32(vt + static_cast<uint32_t>(i) * 4u));
        }
        out += "}";
    }
    out += " ";
    out += summarize_object_strings(base, obj);
    return out;
}

std::string summarize_scheduler_list(uint8_t* base, uint32_t sentinel) {
    if (!sentinel) return "list=null";
    std::string out = "sentinel=" + fmt_ptr(sentinel);
    uint32_t node = REX_LOAD_U32(sentinel);
    int count = 0;
    while (node && node != sentinel && count < 8) {
        const uint32_t obj = REX_LOAD_U32(node + 8);
        out += " item" + std::to_string(count) + "{node=" + fmt_ptr(node) +
               " " + summarize_guest_object(base, obj) + "}";
        node = REX_LOAD_U32(node);
        ++count;
    }
    if (node && node != sentinel) out += " ...";
    if (!count) out += " empty";
    return out;
}

std::string summarize_driver_like(uint8_t* base, uint32_t obj) {
    (void)base;
    if (!obj) return "driver=null";
    std::string out = "this=" + fmt_ptr(obj);
    out += " clip_list=" + fmt_ptr(REX_LOAD_U32(obj + 52));
    out += " unk68=" + fmt_ptr(REX_LOAD_U32(obj + 68));
    out += " time80=" + fmt_f(read_f32(base, obj + 80));
    out += " enabled84=" + std::to_string(REX_LOAD_U8(obj + 84));
    out += " rate88=" + fmt_f(read_f32(base, obj + 88));
    return out;
}

std::string summarize_clip_node(uint8_t* base, uint32_t node) {
    if (!node) return "node=null";
    std::string out = "node=" + fmt_ptr(node);
    out += " flags0=" + fmt_ptr(REX_LOAD_U32(node + 0));
    out += " a16=" + fmt_f(read_f32(base, node + 16));
    out += " b20=" + fmt_f(read_f32(base, node + 20));
    out += " t24=" + fmt_f(read_f32(base, node + 24));
    out += " used32=" + fmt_f(read_f32(base, node + 32));
    out += " clip=" + fmt_ptr(REX_LOAD_U32(node + 36));
    out += " next=" + fmt_ptr(REX_LOAD_U32(node + 40));
    return out;
}

std::string summarize_clip_samples(uint8_t* base, uint32_t obj) {
    if (!obj) return "samples=null";
    std::string out = "this=" + fmt_ptr(obj);
    out += " range=[" + fmt_f(read_f32(base, obj + 24)) + "," +
           fmt_f(read_f32(base, obj + 28)) + "]";
    out += " n124=" + std::to_string(static_cast<int32_t>(REX_LOAD_U32(obj + 228)));
    out += " n348=" + std::to_string(static_cast<int32_t>(REX_LOAD_U32(obj + 452)));
    out += " pose124=" + fmt_ptr(obj + 124);
    out += " pose348=" + fmt_ptr(obj + 348);
    out += " pose572=" + fmt_ptr(obj + 572);
    out += " pose796=" + fmt_ptr(obj + 796);
    return out;
}

bool is_menu_anim_object(uint8_t* base, uint32_t obj) {
    if (!obj) return true;
    const uint32_t vt = REX_LOAD_U32(obj);
    // These vtables showed up as BandButton/RndText-like menu widgets in the
    // failed route traces. Logging them floods early UI navigation and makes
    // the gameplay trace harder to reach.
    if (vt == 0x8200151Cu || vt == 0x8200979Cu) return true;
    const std::string names = summarize_object_strings(base, obj);
    return names.find(".btn") != std::string::npos ||
           names.find(".lbl") != std::string::npos;
}

void log_limited_event(const char* kind, std::atomic<uint32_t>& counter,
                       const std::string& detail) {
    const uint32_t n = counter.fetch_add(1, std::memory_order_relaxed);
    if (n < 96 || (n < 2048 && (n % 128u) == 0) || (n % 1024u) == 0) {
        trace360::LogEvent(kind, "n=" + std::to_string(n) + " " + detail);
    }
    if (n < 4) trace360::LogStackSample(kind);
}

bool dataarray_mentions_selection(uint8_t* base, uint32_t arr) {
    const std::string s = summarize_dataarray(base, arr);
    return s.find("set_song_index") != std::string::npos ||
           s.find("get_song_index") != std::string::npos ||
           s.find("change_song") != std::string::npos ||
           s.find("get_song") != std::string::npos ||
           s.find("song_provider") != std::string::npos ||
           s.find("num_headers") != std::string::npos ||
           s.find("preview") != std::string::npos ||
           s.find("song_block") != std::string::npos ||
           s.find("update_song_info") != std::string::npos ||
           s.find("update_pos") != std::string::npos ||
           s.find("setlist") != std::string::npos ||
           s.find("refresh") != std::string::npos ||
           s.find("select_to_scroll") != std::string::npos ||
           s.find("ss_song.lst") != std::string::npos ||
           s.find("song2") != std::string::npos ||
           s.find("set_character") != std::string::npos ||
           s.find("change_chars") != std::string::npos ||
           s.find("set_guitar_index") != std::string::npos;
}

}  // anonymous namespace

// --- File opens ------------------------------------------------------------
//
// hmx_FileMgr_Lookup(table, path, *out_a, *out_b, *out_c, *out_d) -> u8 found
//
//   r3 = global ARK table object (we don't read it)
//   r4 = guest ptr to path string
//   r5..r8 = out-pointer slots (entry index, aux, ARK offset, ARK size)
//   returns u8 (1 = found, 0 = miss)
//
// After delegating to the original we read the offset/size that got
// written into out_c (r7) / out_d (r8) so the trace records the actual
// resolved location.

// NB: the __imp__ prefix on an extern decl must use the underlying linker
// symbol (sub_XXXX); the preprocessor doesn't expand #defines inside a
// larger identifier like __imp__hmx_X. The REX_HOOK_RAW macro itself does
// see the hmx_ alias as a standalone token and expands it correctly.
extern "C" void __imp__sub_82277878(PPCContext& ctx, uint8_t* base);  // hmx_FileMgr_Lookup
REX_HOOK_RAW(hmx_FileMgr_Lookup) {
    const uint32_t path_addr = ctx.r4.u32;
    const uint32_t out_c     = ctx.r7.u32;
    const uint32_t out_d     = ctx.r8.u32;

    __imp__sub_82277878(ctx, base);

    const bool found = (ctx.r3.u32 & 0xFF) != 0;
    const uint64_t off = (found && out_c) ? REX_LOAD_U32(out_c) : 0;
    const uint32_t sz  = (found && out_d) ? REX_LOAD_U32(out_d) : 0;
    auto path = read_guest_string(base, path_addr);
    trace360::LogFileOpen(path, off, sz, found);

    // Phase 4c addition: stack-sample the FIRST occurrence of each
    // unique file extension category, so the next capture pins the
    // asset-loader sub_ for each. Grouping by extension (not full
    // path) keeps the sample set bounded — we want one sample per
    // loader type, not per individual asset.
    if (found && !path.empty()) {
        std::string key;
        auto dot = path.find_last_of('.');
        if (dot != std::string::npos) {
            key = path.substr(dot);  // e.g. ".mid", ".milo_xbox", ".dtb"
        } else {
            key = path;  // pathological no-extension case
        }
        bool first = false;
        {
            std::lock_guard<std::mutex> lk(g_seen_mu);
            first = g_file_stack_seen.insert(key).second;
        }
        if (first) {
            trace360::LogStackSample(std::string("file_ext:") + key);
        }
    }
}

// --- Property registry lookups ---------------------------------------------
//
// hmx_PropertyTable_Find0(table, key, _) -> entry
//
//   r3 = class PropertyTable
//   r4 = key (typically a pointer to an interned string)
//   returns r3 = entry pointer (NULL on miss)
//
// The "class" identity is upstream of this call (the caller does
// hmx_ClassReg_Lookup first to get the table). We don't track the class
// here; just the key string and the returned pointer. Pairing class to
// lookup is left to offline analysis using nearby string-copies.

extern "C" void __imp__sub_82319530(PPCContext& ctx, uint8_t* base);  // hmx_PropertyTable_Find0
REX_HOOK_RAW(hmx_PropertyTable_Find0) {
    const uint32_t key_addr = ctx.r4.u32;
    __imp__sub_82319530(ctx, base);
    auto key = read_guest_string(base, key_addr);
    trace360::LogPropertyLookupA("?", key, key_addr, ctx.r3.u32);
    if (key == "set_song_index" || key == "get_song_index" ||
        key == "change_song" || key == "get_song" ||
        key == "song_provider" || key == "num_headers" ||
        key == "preview" || key == "song_block" ||
        key == "update_song_info" || key == "update_pos" ||
        key == "setlist" || key == "refresh" ||
        key == "select_to_scroll" || key == "ss_song.lst" ||
        key == "song2" || key == "set_character" ||
        key == "change_chars" || key == "set_guitar_index") {
        g_selection_arg_window.store(96, std::memory_order_relaxed);
    }

    // Phase 4b: capture host stack on first occurrence of each unique
    // prop key. Same purpose as the class.lookup variant -- gives us
    // the calling sub_ address for every named subsystem.
    if (!key.empty()) {
        bool first = false;
        {
            std::lock_guard<std::mutex> lk(g_seen_mu);
            first = g_prop_stack_seen.insert(key).second;
        }
        if (first) {
            trace360::LogStackSample(std::string("prop:") + key);
        }
    }
}

// --- Object property dispatch argument dump --------------------------------
//
// hmx_Object_HandleProperty(result_node, object, data_array, ...)
// The existing PropertyTable hook tells us which property name was looked up,
// but not the script arguments. This targeted dump logs only selection-related
// command arrays so varied song/character traces can be proven from Rexglue.

extern "C" void __imp__sub_82316428(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82316428) {
    const uint32_t result = ctx.r3.u32;
    const uint32_t object = ctx.r4.u32;
    const uint32_t arr = ctx.r5.u32;
    const bool interesting = dataarray_mentions_selection(base, arr);
    if (interesting) {
        char prefix[96];
        std::snprintf(prefix, sizeof prefix,
                      "result=0x%08X object=0x%08X ", result, object);
        std::string detail = prefix;
        detail += summarize_dataarray(base, arr);
        trace360::LogEvent("dispatch.selection.before", detail);
        trace360::LogStackSample("dispatch.selection");
    }
    if (interesting) ++g_selection_dispatch_depth;
    __imp__sub_82316428(ctx, base);
    if (interesting) --g_selection_dispatch_depth;
    if (interesting) {
        char out[128];
        std::snprintf(out, sizeof out,
                      "result=0x%08X type=%u payload=0x%08X object=0x%08X",
                      result, result ? REX_LOAD_U32(result + 4) : 0,
                      result ? REX_LOAD_U32(result + 0) : 0, object);
        trace360::LogEvent("dispatch.selection.after", out);
    }
}

extern "C" void __imp__sub_82317EF8(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_DataNode_Resolve) {
    const uint32_t in_node = ctx.r3.u32;
    __imp__sub_82317EF8(ctx, base);
    const bool in_window =
        g_selection_arg_window.load(std::memory_order_relaxed) > 0;
    if (g_selection_dispatch_depth <= 0 && !in_window) return;
    const uint32_t out_node = ctx.r3.u32;
    std::string detail = "in=0x";
    char b[256];
    std::snprintf(b, sizeof b, "%08X {%s} out=0x%08X {%s}",
                  in_node, describe_datanode(base, in_node).c_str(),
                  out_node, describe_datanode(base, out_node).c_str());
    detail += b;
    trace360::LogEvent("dispatch.selection.resolve", detail);
}

extern "C" void __imp__sub_82317FE8(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_DataNode_AsInt) {
    const uint32_t in_node = ctx.r3.u32;
    __imp__sub_82317FE8(ctx, base);
    const bool in_window =
        g_selection_arg_window.load(std::memory_order_relaxed) > 0;
    if (g_selection_dispatch_depth <= 0 && !in_window) return;
    if (in_window) g_selection_arg_window.fetch_sub(1, std::memory_order_relaxed);
    char b[256];
    std::snprintf(b, sizeof b, "in=0x%08X {%s} int=%d raw=0x%08X",
                  in_node, describe_datanode(base, in_node).c_str(),
                  static_cast<int32_t>(ctx.r3.u32), ctx.r3.u32);
    trace360::LogEvent("dispatch.selection.as_int", b);
}

extern "C" void __imp__sub_82291428(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82291428) {
    const uint32_t result = ctx.r3.u32;
    const uint32_t object = ctx.r4.u32;
    const uint32_t arr = ctx.r5.u32;
    const bool interesting = dataarray_mentions_selection(base, arr);
    if (interesting) {
        char prefix[128];
        std::snprintf(prefix, sizeof prefix,
                      "result=0x%08X object=0x%08X ", result, object);
        std::string detail = prefix;
        detail += summarize_object_strings(base, object);
        detail += " ";
        detail += summarize_dataarray(base, arr);
        trace360::LogEvent("list.handler.before", detail);
        trace360::LogStackSample("list.handler");
    }
    __imp__sub_82291428(ctx, base);
    if (interesting) {
        char out[128];
        std::snprintf(out, sizeof out,
                      "result=0x%08X type=%u payload=0x%08X object=0x%08X",
                      result, result ? REX_LOAD_U32(result + 4) : 0,
                      result ? REX_LOAD_U32(result + 0) : 0, object);
        trace360::LogEvent("list.handler.after", out);
    }
}

extern "C" void __imp__sub_82291B70(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82291B70) {
    const uint32_t result = ctx.r3.u32;
    const uint32_t object = ctx.r4.u32;
    const uint32_t arr = ctx.r5.u32;
    const bool interesting = dataarray_mentions_selection(base, arr);
    if (interesting) {
        char prefix[128];
        std::snprintf(prefix, sizeof prefix,
                      "result=0x%08X object=0x%08X ", result, object);
        std::string detail = prefix;
        detail += summarize_object_strings(base, object);
        detail += " ";
        detail += summarize_dataarray(base, arr);
        trace360::LogEvent("list.wrapper.before", detail);
        trace360::LogStackSample("list.wrapper");
    }
    __imp__sub_82291B70(ctx, base);
    if (interesting) {
        char out[128];
        std::snprintf(out, sizeof out,
                      "result=0x%08X type=%u payload=0x%08X object=0x%08X",
                      result, result ? REX_LOAD_U32(result + 4) : 0,
                      result ? REX_LOAD_U32(result + 0) : 0, object);
        trace360::LogEvent("list.wrapper.after", out);
    }
}

// --- DataHandler (named handler list) lookups ------------------------------
//
// hmx_DataHandler_Find(name) -> payload
//
//   r3 = name string ptr
//   returns r3 = payload (NULL on miss)

extern "C" void __imp__sub_821E04B8(PPCContext& ctx, uint8_t* base);  // hmx_DataHandler_Find
REX_HOOK_RAW(hmx_DataHandler_Find) {
    const uint32_t name_addr = ctx.r3.u32;
    __imp__sub_821E04B8(ctx, base);
    auto name = read_guest_string(base, name_addr);
    trace360::LogHandlerLookupA(name, name_addr, ctx.r3.u32);
}

// --- Class registry lookups (highest-leverage hook) ------------------------
//
// hmx_ClassReg_Lookup(class_symbol) -> PropertyTable*
//
//   r3 = class symbol (Sandbox Symbol = pointer to interned string)
//   returns r3 = per-class PropertyTable pointer (null on miss)
//
// Fires every time the engine asks "what's the PropertyTable for class
// X?", which happens at the head of most class method dispatches and
// every PropertyTable-Find chain. Capturing this gives us a per-frame
// list of WHICH CLASSES are active in the engine — directly surfacing
// the lighting / camera / anim / vfx subsystems that are otherwise
// invisible to FileMgr / PropertyTable / Handler hooks. Required for
// the 1:1 in-song fidelity scope (see [[port-fidelity-scope]] memory).

extern "C" void __imp__sub_82270D20(PPCContext& ctx, uint8_t* base);  // hmx_ClassReg_Lookup
REX_HOOK_RAW(hmx_ClassReg_Lookup) {
    const uint32_t sym_addr = ctx.r3.u32;
    __imp__sub_82270D20(ctx, base);
    auto name = read_guest_string(base, sym_addr);
    trace360::LogClassLookupA(name, sym_addr, ctx.r3.u32);

    // Phase 4b: on first occurrence of each class name, capture the
    // host call stack. The host stack frames in our process are the
    // recompiled C++ functions whose linker names are sub_82XXXXXX
    // — so resolved frame names give us the PPC callers that asked
    // for this class's PropertyTable. That's the "which sub_ owns
    // class X" mapping the next phase needs.
    if (!name.empty()) {
        bool first = false;
        {
            std::lock_guard<std::mutex> lk(g_seen_mu);
            first = g_class_stack_seen.insert(name).second;
        }
        if (first) {
            trace360::LogStackSample(std::string("class:") + name);
        }
    }
}

// --- CharClipSamples pose-buffer apply ------------------------------------
//
// sub_8215DF28(source_table, dest_table, weight) and sub_8215E6A0(source_table,
// dest_table) are the exact transform-buffer application primitives used by
// CharClipSamples. These are the source-of-truth hooks for whether the game is
// applying absolute-looking clip samples as additive layered pose data.

extern "C" void __imp__sub_8215DF28(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_8215DF28) {
    const uint32_t src = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const float weight = static_cast<float>(ctx.f1.f64);
    const std::string pre_detail = summarize_anim_table(base, src);
    const bool body_like = looks_like_body_anim(pre_detail);
    const std::string values_before =
        body_like ? capture_selected_anim_values(base, src, dst) : "";
    uint32_t lower_body_dump_seq = 0;
    bool lower_body_dump = false;
    if (body_like && lower_body_memory_trace_enabled()) {
        static std::atomic<uint32_t> s_lower_body_dump_count{0};
        lower_body_dump_seq =
            s_lower_body_dump_count.fetch_add(1, std::memory_order_relaxed);
        lower_body_dump = lower_body_dump_seq < 96;
        if (lower_body_dump) {
            log_lower_body_memory_dump(base, "weighted", "before",
                                       lower_body_dump_seq, src, dst, weight);
        }
    }
    __imp__sub_8215DF28(ctx, base);
    if (lower_body_dump) {
        log_lower_body_memory_dump(base, "weighted", "after",
                                   lower_body_dump_seq, src, dst, weight);
    }

    static std::atomic<uint32_t> s_count{0};
    const uint32_t n = s_count.fetch_add(1, std::memory_order_relaxed);
    if (n >= 640) {
        if ((n % 512u) == 0) {
            char pulse[96];
            std::snprintf(pulse, sizeof pulse,
                          "n=%u weight=%.4f src=0x%08X dst=0x%08X", n,
                          weight, src, dst);
            trace360::LogEvent("anim.apply.weighted.pulse", pulse);
        }
        return;
    }
    char prefix[96];
    std::snprintf(prefix, sizeof prefix, "n=%u weight=%.4f src=[", n, weight);
    std::string detail = prefix;
    detail += pre_detail;
    detail += "] dst=[";
    detail += summarize_anim_table(base, dst);
    detail += "]";
    static std::atomic<uint32_t> s_value_count{0};
    if (body_like &&
        s_value_count.fetch_add(1, std::memory_order_relaxed) < 48) {
        detail += " values_before=[";
        detail += values_before;
        detail += "] values_after=[";
        detail += capture_selected_anim_values(base, src, dst);
        detail += "]";
    }
    trace360::LogEvent("anim.apply.weighted", detail);
    static std::atomic<uint32_t> s_body_stack{0};
    if (n < 8 || (body_like &&
                  s_body_stack.fetch_add(1, std::memory_order_relaxed) < 24)) {
        trace360::LogStackSample(body_like ? "anim.apply.weighted.body"
                                           : "anim.apply.weighted");
    }
}

extern "C" void __imp__sub_8215E6A0(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_8215E6A0) {
    const uint32_t src = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const std::string pre_detail = summarize_anim_table(base, src);
    const bool body_like = looks_like_body_anim(pre_detail);
    const std::string values_before =
        body_like ? capture_selected_anim_values(base, src, dst) : "";
    uint32_t lower_body_dump_seq = 0;
    bool lower_body_dump = false;
    if (body_like && lower_body_memory_trace_enabled()) {
        static std::atomic<uint32_t> s_lower_body_dump_count{0};
        lower_body_dump_seq =
            s_lower_body_dump_count.fetch_add(1, std::memory_order_relaxed);
        lower_body_dump = lower_body_dump_seq < 96;
        if (lower_body_dump) {
            log_lower_body_memory_dump(base, "unweighted", "before",
                                       lower_body_dump_seq, src, dst, 1.0f);
        }
    }
    __imp__sub_8215E6A0(ctx, base);
    if (lower_body_dump) {
        log_lower_body_memory_dump(base, "unweighted", "after",
                                   lower_body_dump_seq, src, dst, 1.0f);
    }

    static std::atomic<uint32_t> s_count{0};
    const uint32_t n = s_count.fetch_add(1, std::memory_order_relaxed);
    if (n >= 640) {
        if ((n % 512u) == 0) {
            char pulse[80];
            std::snprintf(pulse, sizeof pulse, "n=%u src=0x%08X dst=0x%08X",
                          n, src, dst);
            trace360::LogEvent("anim.apply.unweighted.pulse", pulse);
        }
        return;
    }
    char prefix[64];
    std::snprintf(prefix, sizeof prefix, "n=%u src=[", n);
    std::string detail = prefix;
    detail += pre_detail;
    detail += "] dst=[";
    detail += summarize_anim_table(base, dst);
    detail += "]";
    static std::atomic<uint32_t> s_value_count{0};
    if (body_like &&
        s_value_count.fetch_add(1, std::memory_order_relaxed) < 48) {
        detail += " values_before=[";
        detail += values_before;
        detail += "] values_after=[";
        detail += capture_selected_anim_values(base, src, dst);
        detail += "]";
    }
    trace360::LogEvent("anim.apply.unweighted", detail);
    static std::atomic<uint32_t> s_body_stack{0};
    if (n < 8 || (body_like &&
                  s_body_stack.fetch_add(1, std::memory_order_relaxed) < 24)) {
        trace360::LogStackSample(body_like ? "anim.apply.unweighted.body"
                                           : "anim.apply.unweighted");
    }
}

// --- Character animation pipeline trace -----------------------------------
//
// These hooks sit above the final pose-buffer apply primitives. They are
// intentionally trace-only and log scheduler, driver, clip-node, sample, and
// controller ordering data before native animation code is touched again.

extern "C" void __imp__sub_821B8C98(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821B8C98) {
    const uint32_t self = ctx.r3.u32;
    static std::atomic<uint32_t> s_count{0};
    std::string detail = "this=" + fmt_ptr(self);
    if (self) {
        detail += " active_flag_m224=" +
                  std::to_string(REX_LOAD_U8(self - 224));
        detail += " list48=[" + summarize_scheduler_list(base, self + 48) + "]";
        detail += " list56=[" + summarize_scheduler_list(base, self + 56) + "]";
    }
    log_limited_event("anim.scheduler.before", s_count, detail);
    __imp__sub_821B8C98(ctx, base);
}

extern "C" void __imp__sub_8216AF70(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_8216AF70) {
    const uint32_t self = ctx.r3.u32;
    static std::atomic<uint32_t> s_count{0};
    std::string before = summarize_driver_like(base, self);
    __imp__sub_8216AF70(ctx, base);
    std::string after = summarize_driver_like(base, self);
    log_limited_event("anim.driver.update", s_count,
                      "before={" + before + "} after={" + after + "}");
}

extern "C" void __imp__sub_821A4988(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821A4988) {
    const uint32_t node = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const float weight = static_cast<float>(ctx.f1.f64);
    static std::atomic<uint32_t> s_count{0};
    std::string detail = summarize_clip_node(base, node);
    detail += " dst=" + fmt_ptr(dst);
    detail += " weight_budget=" + fmt_f(weight);
    log_limited_event("anim.clip_chain.before", s_count, detail);
    __imp__sub_821A4988(ctx, base);
}

extern "C" void __imp__sub_82162570(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82162570) {
    const uint32_t samples = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const float weight = static_cast<float>(ctx.f1.f64);
    const float t0 = static_cast<float>(ctx.f2.f64);
    const float t1 = static_cast<float>(ctx.f3.f64);
    static std::atomic<uint32_t> s_count{0};
    std::string detail = summarize_clip_samples(base, samples);
    detail += " dst=" + fmt_ptr(dst);
    detail += " weight=" + fmt_f(weight);
    detail += " t0=" + fmt_f(t0);
    detail += " t1=" + fmt_f(t1);
    log_limited_event("anim.samples.eval.before", s_count, detail);
    __imp__sub_82162570(ctx, base);
}

extern "C" void __imp__sub_82162378(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82162378) {
    const uint32_t samples = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const float weight = static_cast<float>(ctx.f1.f64);
    const float t0 = static_cast<float>(ctx.f2.f64);
    const float t1 = static_cast<float>(ctx.f3.f64);
    static std::atomic<uint32_t> s_count{0};
    std::string detail = summarize_clip_samples(base, samples);
    detail += " dst=" + fmt_ptr(dst);
    detail += " weight=" + fmt_f(weight);
    detail += " t0=" + fmt_f(t0);
    detail += " t1=" + fmt_f(t1);
    log_limited_event("anim.samples.pose796.before", s_count, detail);
    __imp__sub_82162378(ctx, base);
}

extern "C" void __imp__sub_821A1CC8(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821A1CC8) {
    const uint32_t pose = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const float weight = static_cast<float>(ctx.f1.f64);
    const uint32_t mode = ctx.r6.u32;
    static std::atomic<uint32_t> s_count{0};
    std::string detail = "pose=" + fmt_ptr(pose) + " dst=" + fmt_ptr(dst) +
                         " weight=" + fmt_f(weight) +
                         " mode_r6=" + std::to_string(mode) +
                         " src=[" + summarize_anim_table(base, pose) + "]";
    log_limited_event("anim.pose.apply_weighted_source", s_count, detail);
    __imp__sub_821A1CC8(ctx, base);
}

extern "C" void __imp__sub_821A1DB0(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821A1DB0) {
    const uint32_t pose = ctx.r3.u32;
    const uint32_t dst = ctx.r4.u32;
    const float weight = static_cast<float>(ctx.f1.f64);
    const float interp = static_cast<float>(ctx.f2.f64);
    static std::atomic<uint32_t> s_count{0};
    std::string detail = "pose=" + fmt_ptr(pose) + " dst=" + fmt_ptr(dst) +
                         " weight=" + fmt_f(weight) +
                         " interp=" + fmt_f(interp) +
                         " src=[" + summarize_anim_table(base, pose) + "]";
    log_limited_event("anim.pose.apply_interp_source", s_count, detail);
    __imp__sub_821A1DB0(ctx, base);
}

extern "C" void __imp__sub_821D1190(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821D1190) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    static std::atomic<uint32_t> s_count{0};
    const bool trace_object = !is_menu_anim_object(base, self);
    if (trace_object) {
        log_lower_body_runtime_memory(base, "821D1190", "before", self, arg0,
                                      arg1);
        log_limited_event("anim.controller.stage_821D1190", s_count,
                          "self=" + fmt_ptr(self) + " arg0=" + fmt_ptr(arg0) +
                              " arg1=" + fmt_ptr(arg1) + " " +
                              summarize_guest_object(base, self));
    }
    __imp__sub_821D1190(ctx, base);
    if (trace_object) {
        log_lower_body_runtime_memory(base, "821D1190", "after", self, arg0,
                                      arg1);
    }
}

extern "C" void __imp__sub_821D1710(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821D1710) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    static std::atomic<uint32_t> s_count{0};
    const bool trace_object = !is_menu_anim_object(base, self);
    if (trace_object) {
        log_lower_body_runtime_memory(base, "821D1710", "before", self, arg0,
                                      arg1);
        log_limited_event("anim.controller.stage_821D1710", s_count,
                          "self=" + fmt_ptr(self) + " arg0=" + fmt_ptr(arg0) +
                              " arg1=" + fmt_ptr(arg1) + " " +
                              summarize_guest_object(base, self));
    }
    __imp__sub_821D1710(ctx, base);
    if (trace_object) {
        log_lower_body_runtime_memory(base, "821D1710", "after", self, arg0,
                                      arg1);
    }
}

extern "C" void __imp__sub_8214CAC8(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_8214CAC8) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    static std::atomic<uint32_t> s_count{0};
    if (!is_menu_anim_object(base, self)) {
        log_limited_event("anim.controller.stage_8214CAC8", s_count,
                          "self=" + fmt_ptr(self) + " arg0=" + fmt_ptr(arg0) +
                              " arg1=" + fmt_ptr(arg1) + " " +
                              summarize_guest_object(base, self));
    }
    __imp__sub_8214CAC8(ctx, base);
}

extern "C" void __imp__sub_8214C610(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_8214C610) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    static std::atomic<uint32_t> s_count{0};
    if (!is_menu_anim_object(base, self)) {
        log_limited_event("anim.controller.stage_8214C610", s_count,
                          "self=" + fmt_ptr(self) + " arg0=" + fmt_ptr(arg0) +
                              " arg1=" + fmt_ptr(arg1) + " " +
                              summarize_guest_object(base, self));
    }
    __imp__sub_8214C610(ctx, base);
}

// --- Live venue / band subsystem tick probes ------------------------------
//
// These are trace-only pass-through hooks on already named vtable[15] update
// candidates. They prove sustained gameplay activity without depending on
// high-volume PropertyTable noise.

extern "C" void __imp__sub_821BCAE8(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_LightPreset_Update) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t light_out = ctx.r4.u32;
    const uint32_t force = ctx.r5.u32;
    __imp__sub_821BCAE8(ctx, base);
    static std::atomic<uint32_t> s_count{0};
    log_periodic_call("lighting.preset.update", s_count, self, light_out,
                      force);
}

extern "C" void __imp__sub_822F5A70(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_CamShot_Update) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    __imp__sub_822F5A70(ctx, base);
    static std::atomic<uint32_t> s_count{0};
    log_periodic_call("camera.camshot.update", s_count, self, arg0, arg1);
}

extern "C" void __imp__sub_822F6B58(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_CamShot_Blend) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    __imp__sub_822F6B58(ctx, base);
    static std::atomic<uint32_t> s_count{0};
    log_periodic_call("camera.camshot.blend", s_count, self, arg0, arg1);
}

extern "C" void __imp__sub_822CC848(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_WorldCrowd_Update) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    __imp__sub_822CC848(ctx, base);
    static std::atomic<uint32_t> s_count{0};
    log_periodic_call("crowd.world.update", s_count, self, arg0, arg1);
}

extern "C" void __imp__sub_8214CD88(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_CharIK_Update) {
    const uint32_t self = ctx.r3.u32;
    const uint32_t arg0 = ctx.r4.u32;
    const uint32_t arg1 = ctx.r5.u32;
    log_lower_body_runtime_memory(base, "CharIK_Update", "before", self, arg0,
                                  arg1);
    __imp__sub_8214CD88(ctx, base);
    log_lower_body_runtime_memory(base, "CharIK_Update", "after", self, arg0,
                                  arg1);
    static std::atomic<uint32_t> s_count{0};
    log_periodic_call("char.ik.update", s_count, self, arg0, arg1);
}

// --- Trace-only scripted navigation ----------------------------------------
//
// Hidden-window PostMessage input is not reliable enough for focused leg
// traces. This hook writes a small scripted XInput button sequence directly
// into the returned state, but only when --trace_scripted_nav is present.
// It is confined to the RexGlue trace build and does not touch native code.

extern "C" void __imp__sub_823B5B68(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_XamInputGetState) {
    const uint32_t user = ctx.r3.u32;
    const uint32_t state = ctx.r4.u32;
    __imp__sub_823B5B68(ctx, base);
    if (!trace_scripted_nav_enabled() || !state) return;

#if REX_PLATFORM_WIN32
    static const ULONGLONG s_start = GetTickCount64();
    const uint64_t elapsed_ms = GetTickCount64() - s_start;
    const uint16_t buttons = scripted_nav_buttons_ms(elapsed_ms);

    static std::atomic<uint32_t> s_packet{1};
    const uint32_t packet = s_packet.fetch_add(1, std::memory_order_relaxed);
    REX_STORE_U32(state + 0, packet);
    REX_STORE_U16(state + 4, buttons);
    REX_STORE_U8(state + 6, 0);
    REX_STORE_U8(state + 7, 0);
    REX_STORE_U16(state + 8, 0);
    REX_STORE_U16(state + 10, 0);
    REX_STORE_U16(state + 12, 0);
    REX_STORE_U16(state + 14, 0);
    ctx.r3.u64 = 0; // ERROR_SUCCESS: report the scripted controller connected.

    static std::atomic<uint32_t> s_poll_count{0};
    const uint32_t poll = s_poll_count.fetch_add(1, std::memory_order_relaxed);
    if (poll < 16 || (poll < 4096 && (poll % 128u) == 0) ||
        (poll % 512u) == 0) {
        char detail[192];
        std::snprintf(detail, sizeof detail,
                      "poll=%u elapsed_ms=%llu user=%u state=0x%08X "
                      "buttons=0x%04X packet=%u",
                      poll, static_cast<unsigned long long>(elapsed_ms), user,
                      state, buttons, packet);
        trace360::LogEvent("input.scripted_nav.poll", detail);
    }

    static std::atomic<uint32_t> s_last{0xFFFFFFFFu};
    if (s_last.exchange(buttons, std::memory_order_relaxed) != buttons) {
        char detail[160];
        std::snprintf(detail, sizeof detail,
                      "elapsed_ms=%llu user=%u state=0x%08X buttons=0x%04X",
                      static_cast<unsigned long long>(elapsed_ms), user, state,
                      buttons);
        trace360::LogEvent("input.scripted_nav", detail);
        REXLOG_INFO("[trace_scripted_nav] {}", detail);
    }
#endif
}

// --- Force joypad mode = ON ------------------------------------------------
//
// hmx_JoypadConfig_SetJoypadMode(this, bool enable) -> sub_8236A338
//
// Decoded body (recomp.17.cpp:77716, 47 PPC insns):
//   r3 = this (JoypadConfig*), r4 = enable (u8 bool)
//   if (enable && this+56 == NULL):
//     this+56 = Mem_Alloc(904)  // allocate Joypad object
//     sub_8227B1F8(this+56, this)    // init joypad
//     sub_8227A238(this+56, 1)       // register
//     sub_8227A220(this+56, 0xF000)  // bind input mask
//   elif (!enable && this+56 != NULL):
//     this+56->vtable[0](this+56, 1)  // destructor
//     this+56 = NULL
//
// To force joypad mode on for our headless trace-360 build (so a
// standard Xbox controller plays the game like PS2 controller mode),
// hook this function and force r4=1 on the FIRST call (which comes
// from sub_8236C4C0 reading the use_joypad config). Subsequent calls
// (if any — e.g. settings-menu toggles) pass through unmodified so
// other code paths can still observe a value change.
//
// See [[input-joypad-mode]] memory for the broader rationale.

extern "C" void __imp__sub_8236A338(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_JoypadConfig_SetJoypadMode) {
    static std::atomic<bool> g_first_set = false;
    bool expected = false;
    if (g_first_set.compare_exchange_strong(expected, true)) {
        // First call -- this is from JoypadConfig::Init at boot reading
        // the use_joypad config DataNode. Force r4=1 so joypad mode is
        // enabled regardless of the DTB value.
        const uint8_t original = static_cast<uint8_t>(ctx.r4.u32 & 0xFF);
        ctx.r4.u64 = 1;
        trace360::LogEvent("joypad.force_on",
                           original ? "was already true (no-op)"
                                    : "was false; forcing true");
    }
    __imp__sub_8236A338(ctx, base);
}

// --- GuitarPort button remap table dump ------------------------------------
//
// hmx_GuitarPort_RemapButtons(out_bitmask*, wButtons_u16, strum_up_u8, strum_dn_u8)
//
//   r3 = uint32_t* out  — function WRITES engine bitmask here; does NOT return it
//   r4 = wButtons (low 16 bits) — raw XInput XINPUT_STATE.Gamepad.wButtons
//   r5 = strum_up  (u8, nonzero sets bit 0 of *out)
//   r6 = strum_dn  (u8, nonzero sets bit 1 of *out)
//
// Body iterates XInput bits 0..15. For each bit set in wButtons, reads
// engine_bit from the 16-entry uint32 table at guest address 0x82036730
// and does: *out |= (1 << engine_bit).
// NB: PPC slw zeroes when shift_count >= 32 (bit 5 of engine_bit is set),
// so all five fret engine-bits must be < 32. The lane-index values >= 32
// in harmonix_symbols.h are from the pad-layout context, not this bitmask.
//
// We dump the full 16-entry table ONCE on first call (via REXLOG_INFO so
// it lands in the log file unconditionally, even before trace capture
// starts). From that dump we derive the correct --keybind_X= arguments.
//
// XInput wButtons bit positions 0..15 (Xbox 360 / XInput.h):
//   0 = DPAD_UP   1 = DPAD_DOWN   2 = DPAD_LEFT   3 = DPAD_RIGHT
//   4 = START     5 = BACK        6 = LSTICK       7 = RSTICK
//   8 = LB        9 = RB         10 = (reserved)  11 = (reserved)
//  12 = A        13 = B          14 = X            15 = Y

extern "C" void __imp__sub_8227B368(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(hmx_GuitarPort_RemapButtons) {
    static constexpr const char* kXInputBitName[16] = {
        "DPAD_UP",       "DPAD_DOWN",     "DPAD_LEFT",    "DPAD_RIGHT",
        "START",         "BACK",          "LSTICK",       "RSTICK",
        "LB",            "RB",            "bit10",        "bit11",
        "A",             "B",             "X",            "Y"
    };

    // One-time: dump the full 16-entry XInput->engine-bit remap table.
    static std::once_flag s_table_logged;
    std::call_once(s_table_logged, [&]() {
        REXLOG_INFO("[GuitarPort] RemapButtons 16-entry table @ guest 0x82036730:");
        for (int i = 0; i < 16; ++i) {
            uint32_t engine_bit = REX_LOAD_U32(0x82036730u + static_cast<uint32_t>(i) * 4);
            REXLOG_INFO("[GuitarPort]   [{}] {} -> engine bit {} (mask 0x{:08X})",
                        i, kXInputBitName[i], engine_bit, (1u << (engine_bit & 31u)));
        }
        REXLOG_INFO("[GuitarPort]   strum_up (r5) -> engine bit 0 (mask 0x00000001) [hardcoded]");
        REXLOG_INFO("[GuitarPort]   strum_dn (r6) -> engine bit 1 (mask 0x00000002) [hardcoded]");
    });

    // Per-change diagnostic: capture the raw XInput button word coming IN
    // and the engine bitmask written OUT, but only when the input changes
    // (edge-triggered) so we get one clean line per press/release instead
    // of 60/sec of noise. This is the decisive test for the "only G/A light
    // a fret" report: if pressing S/D/F shows their bit appearing in
    // wButtons here, the keys ARE reaching the driver and the engine bit is
    // simply not a fret; if they never appear, the key never made it in.
    const uint32_t out_ptr = ctx.r3.u32;
    uint16_t wbuttons = static_cast<uint16_t>(ctx.r4.u32 & 0xFFFF);
    uint8_t strum_up = static_cast<uint8_t>(ctx.r5.u32 & 0xFF);
    uint8_t strum_dn = static_cast<uint8_t>(ctx.r6.u32 & 0xFF);

    // Trace-build-only headless navigation bridge. Rexglue's MnK driver can
    // synthesize D-pad bits from PostMessage, but not the hardcoded guitar
    // strum bytes (r5/r6). Some GH2 list screens advance on strum instead of
    // D-pad, so this lets smoke_trace.ps1 prove varied song/character traces
    // without touching native animation or game logic.
    static const bool s_dpad_right_as_strum_dn =
        trace_cmdline_has_flag("--trace_dpad_right_strum_dn");
    if (s_dpad_right_as_strum_dn && (wbuttons & (1u << 3))) {
        ctx.r4.u64 = static_cast<uint32_t>(wbuttons & ~(1u << 3));
        ctx.r6.u64 = 1;
        wbuttons = static_cast<uint16_t>(ctx.r4.u32 & 0xFFFF);
        strum_dn = 1;
    }

    __imp__sub_8227B368(ctx, base);

    const uint32_t engine_word = out_ptr ? REX_LOAD_U32(out_ptr) : 0;

    static std::atomic<uint32_t> s_last_combo{0xFFFFFFFFu};
    const uint32_t combo = (static_cast<uint32_t>(wbuttons) << 16) |
                           (static_cast<uint32_t>(strum_up) << 8) |
                           static_cast<uint32_t>(strum_dn);
    if (combo != s_last_combo.exchange(combo) && (wbuttons || strum_up || strum_dn)) {
        std::string names;
        for (int i = 0; i < 16; ++i) {
            if (wbuttons & (1u << i)) {
                if (!names.empty()) names += '+';
                names += kXInputBitName[i];
            }
        }
        if (names.empty()) names = "(none)";
        REXLOG_INFO("[GuitarPort] IN wButtons=0x{:04X} [{}] strum_up={} strum_dn={} "
                    "=> OUT engine_word=0x{:08X}",
                    wbuttons, names, strum_up, strum_dn, engine_word);
        char detail[192];
        std::snprintf(detail, sizeof detail,
                      "wButtons=0x%04X names=%s strum_up=%u strum_dn=%u engine_word=0x%08X",
                      wbuttons, names.c_str(), strum_up, strum_dn, engine_word);
        trace360::LogEvent("input.guitar_edge", detail);
    }
}

// --- BandButton struct dump (menu text_size / alignment / world-xfm RE) -----
//
// hmx_BandButton_ColorResolve (sub_82122920) runs per button to resolve its
// per-state RGBA; r3 = the BandButton object. We dump the object's struct ONCE
// per unique pointer so we can read, at fixed struct offsets, the values that
// are NOT cleanly recoverable from the static .btn serialization: text_size,
// alignment, and the live world matrix (scale/rotation/translation of the
// rendered label). Parsed offline from the "bandbtn_struct" trace events.
extern "C" void __imp__sub_82122920(PPCContext& ctx, uint8_t* base);  // hmx_BandButton_ColorResolve
REX_HOOK_RAW(hmx_BandButton_ColorResolve) {
    const uint32_t obj = ctx.r3.u32;
    const uint32_t color_state = ctx.r4.u32;
    const uint32_t out_rgb = ctx.r5.u32;
    __imp__sub_82122920(ctx, base);
    if (obj == 0) return;
    auto FW = [&](int word) -> float {
        uint32_t w = static_cast<uint32_t>(REX_LOAD_U32(obj + word * 4u));
        float f; std::memcpy(&f, &w, 4); return f;
    };
    auto FP = [&](uint32_t addr) -> float {
        uint32_t w = static_cast<uint32_t>(REX_LOAD_U32(addr));
        float f; std::memcpy(&f, &w, 4); return f;
    };
    // Tagged 3x4 world matrix: rows at words 32/36/40/44 (3 floats + 1 tag word each).
    const float sx = std::sqrt(FW(32) * FW(32) + FW(34) * FW(34));
    const float sz = std::sqrt(FW(40) * FW(40) + FW(42) * FW(42));
    const float tilt = std::atan2(FW(34), FW(32)) * 57.29578f;
    const float tx = FW(44), ty = FW(45), tz = FW(46);
    const float text_size = FW(74);
    const uint32_t text_obj = REX_LOAD_U32(obj + 344);
    uint32_t text_vt = 0, text_fn08 = 0, text_fn16 = 0;
    if (text_obj) {
        text_vt = REX_LOAD_U32(text_obj);
        if (text_vt) {
            text_fn08 = REX_LOAD_U32(text_vt + 8);
            text_fn16 = REX_LOAD_U32(text_vt + 16);
        }
    }
    // Button NAME: scan the first 24 words for a pointer to a printable "*.btn"
    // / "*.lbl" string (the object name) so each button is identifiable; collect
    // the first 3 printable strings as fallbacks.
    std::string name; std::string strs;
    int found = 0;
    // Scan ONLY the struct header (words 0..15) — it holds object pointers, no
    // floats (the matrix/floats start at word 16+, where a value like 0x3f99999a
    // would be misread as a pointer and fault). Header pointers may be heap
    // (0x40xxxxxx, e.g. a name string) or data (0x82xxxxxx, interned) — both are
    // mapped, so reading a string at a non-string pointer just yields non-printable
    // (skipped), never a fault.
    for (int w = 0; w < 16 && found < 8; ++w) {
        uint32_t p = static_cast<uint32_t>(REX_LOAD_U32(obj + w * 4u));
        const bool heap = (p >= 0x40000000u && p < 0x50000000u);
        const bool data = (p >= 0x82000000u && p < 0x83000000u);
        if (!heap && !data) continue;
        std::string g = read_guest_string(base, p);
        if (g.size() < 2 || g.size() > 40) continue;
        bool printable = true;
        for (char c : g) if (c < 32 || c >= 127) { printable = false; break; }
        if (!printable) continue;
        if (name.empty() && (g.size() > 4) &&
            (g.compare(g.size() - 4, 4, ".btn") == 0 || g.compare(g.size() - 4, 4, ".lbl") == 0))
            name = g;
        strs += " [" + g + "]"; ++found;
    }
    // Capture the SETTLED transform: dump a button only when its translation has
    // MOVED > 0.15 since the last dump (or it is new). During a slide-in the button
    // emits a short trajectory of samples; the LAST sample per pointer is the
    // settled position. (Plain once-per-pointer dedup caught the FIRST sample =
    // mid-slide, which is why X looked staggered/unsettled.)
    static std::mutex s_mu;
    static std::unordered_map<uint32_t, std::pair<float, float>> s_last;
    {
        std::lock_guard<std::mutex> lk(s_mu);
        auto it = s_last.find(obj);
        if (it != s_last.end() &&
            std::fabs(it->second.first - tx) + std::fabs(it->second.second - tz) < 0.15f)
            return;
        s_last[obj] = {tx, tz};
    }
    char b[320];
    std::snprintf(b, sizeof b,
                  "name=%s state=%u rgb=(%.4f,%.4f,%.4f) T=(%.3f,%.3f,%.3f) scale=(%.4f,%.4f) tilt=%.2f text_size=%.4f text=0x%08X vt=0x%08X fn08=0x%08X fn16=0x%08X strs=%s",
                  name.empty() ? "?" : name.c_str(), color_state,
                  out_rgb ? FP(out_rgb) : -1.0f,
                  out_rgb ? FP(out_rgb + 4) : -1.0f,
                  out_rgb ? FP(out_rgb + 8) : -1.0f,
                  tx, ty, tz, sx, sz, tilt, text_size,
                  text_obj, text_vt, text_fn08, text_fn16, strs.c_str());
    trace360::LogEvent("bandbtn", b);
}

// RndText child update called by BandButton before resolving color:
// vtable+8 = sub_821B6F88(text, f1, f2). It stores f1 at text+8 and rebuilds
// the text's geometry arrays. Capture this instead of guessing how text_size,
// box fit, and the BandButton transform combine.
extern "C" void __imp__sub_821B6F88(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_821B6F88) {
    const uint32_t text = ctx.r3.u32;
    const float in_f1 = static_cast<float>(ctx.f1.f64);
    const float in_f2 = static_cast<float>(ctx.f2.f64);
    __imp__sub_821B6F88(ctx, base);
    if (!text) return;
    auto RF = [&](uint32_t addr) -> float {
        uint32_t w = static_cast<uint32_t>(REX_LOAD_U32(addr));
        float f; std::memcpy(&f, &w, 4); return f;
    };
    const uint32_t geo = REX_LOAD_U32(text + 76);
    if (!geo) return;
    const uint32_t a0 = REX_LOAD_U32(geo + 32), a1 = REX_LOAD_U32(geo + 36);
    const uint32_t b0 = REX_LOAD_U32(geo + 44), b1 = REX_LOAD_U32(geo + 48);
    const uint32_t c0 = REX_LOAD_U32(geo + 56), c1 = REX_LOAD_U32(geo + 60);
    const int ca = (a1 >= a0) ? static_cast<int>((a1 - a0) / 20) : -1;
    const int cb = (b1 >= b0) ? static_cast<int>((b1 - b0) / 20) : -1;
    const int cc = (c1 >= c0) ? static_cast<int>((c1 - c0) / 20) : -1;
    const float last_a = (ca > 0) ? RF(a1 - 4) : 0.0f;
    const float last_b = (cb > 0) ? RF(b1 - 4) : 0.0f;
    const float last_c = (cc > 0) ? RF(c1 - 4) : 0.0f;
    char out[256];
    std::snprintf(out, sizeof out,
                  "text=0x%08X f=(%.4f,%.4f) stored=%.4f geo=0x%08X counts=(%d,%d,%d) last=(%.4f,%.4f,%.4f)",
                  text, in_f1, in_f2, RF(text + 8), geo, ca, cb, cc,
                  last_a, last_b, last_c);
    trace360::LogEvent("rndtext_update", out);
}

// --- GPU draw capture (final menu-text geometry RE) ------------------------
//
// hmx_GPU_DrawPrimitive (sub_823C7478): r4=prim-type, r5=vtx count, r6=vtx ptr,
// r7=stride. We log each UNIQUE draw shape (type,count,stride) with the bounding
// box of its vertex positions (first 3 floats/vertex). The menu text draws show
// as transparent batches whose bbox is the rendered text extent -> exact glyph
// size + position, no struct-offset guessing. Parsed offline ("gpu_draw").
extern "C" void __imp__sub_823C7478(PPCContext& ctx, uint8_t* base);  // hmx_GPU_DrawPrimitive
REX_HOOK_RAW(hmx_GPU_DrawPrimitive) {
    const uint32_t type = ctx.r4.u32, count = ctx.r5.u32;
    const uint32_t vtx = ctx.r6.u32, stride = ctx.r7.u32;
    __imp__sub_823C7478(ctx, base);
    if (vtx == 0 || count == 0 || count > 8192 || stride < 8 || stride > 256) return;
    float mn[3] = {1e9f, 1e9f, 1e9f}, mx[3] = {-1e9f, -1e9f, -1e9f};
    for (uint32_t i = 0; i < count; ++i) {
        for (int k = 0; k < 3; ++k) {
            const uint32_t w = static_cast<uint32_t>(REX_LOAD_U32(vtx + i * stride + k * 4));
            float f;
            std::memcpy(&f, &w, 4);
            if (f == f && (f < 1e6f && f > -1e6f)) {
                if (f < mn[k]) mn[k] = f;
                if (f > mx[k]) mx[k] = f;
            }
        }
    }
    auto q16 = [](float f) -> int64_t {
        return static_cast<int64_t>(std::llround(f * 16.0f));
    };
    uint64_t sig = (static_cast<uint64_t>(type) << 56) ^
                   (static_cast<uint64_t>(count & 0xFFFFu) << 40) ^
                   (static_cast<uint64_t>(stride & 0xFFu) << 32);
    sig ^= static_cast<uint64_t>(q16(mn[0]) & 0xFFFF) << 16;
    sig ^= static_cast<uint64_t>(q16(mn[2]) & 0xFFFF);
    sig ^= static_cast<uint64_t>(q16(mx[0]) & 0xFFFF) << 48;
    sig ^= static_cast<uint64_t>(q16(mx[2]) & 0xFFFF) << 24;
    static std::mutex s_mu;
    static std::unordered_set<uint64_t> s_seen;
    {
        std::lock_guard<std::mutex> lk(s_mu);
        if (s_seen.size() > 20000 || !s_seen.insert(sig).second) return;
    }
    char b[200];
    std::snprintf(b, sizeof b,
                  "type=%u cnt=%u str=%u bbox=[%.1f %.1f %.1f]..[%.1f %.1f %.1f]",
                  type, count, stride, mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]);
    trace360::LogEvent("gpu_draw", b);
}
