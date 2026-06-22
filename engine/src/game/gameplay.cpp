// engine/src/game/gameplay.cpp

#include "game/gameplay.h"
#include "render/window_d3d9.h"

#include "asset/milo_image.h"
#include "ark_v3.h"
#include "catalog.h"
#include "dtb.h"
#include "milo_scene/milo_scene.h"
#include "milo.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ghogx::game {

namespace {

bool debug_camera_enabled() {
#ifdef _MSC_VER
    char* value = nullptr;
    size_t len = 0;
    const bool enabled =
        _dupenv_s(&value, &len, "GHOGX_DEBUG_CAMERA") == 0 && value &&
        value[0];
    std::free(value);
    return enabled;
#else
    const char* value = std::getenv("GHOGX_DEBUG_CAMERA");
    return value && value[0];
#endif
}

const char* env_value(const char* name) {
#ifdef _MSC_VER
    static thread_local std::string value;
    char* raw = nullptr;
    size_t len = 0;
    if (_dupenv_s(&raw, &len, name) != 0 || !raw) return nullptr;
    value = raw;
    std::free(raw);
    return value.empty() ? nullptr : value.c_str();
#else
    const char* value = std::getenv(name);
    return value && value[0] ? value : nullptr;
#endif
}

bool debug_gameplay_camera_enabled() {
    return env_value("GHOGX_DEBUG_GAMEPLAY_CAMERA") != nullptr;
}

std::string_view only_draw_performer_role() {
    const char* role = env_value("GHOGX_ONLY_PERFORMER");
    return role ? std::string_view(role) : std::string_view();
}

bool debug_face_enabled_game() {
    return env_value("GHOGX_DEBUG_FACE") != nullptr;
}

bool debug_venue_filters_enabled() {
    return env_value("GHOGX_DEBUG_VENUE_FILTERS") != nullptr;
}

bool debug_performer_start_enabled() {
    return env_value("GHOGX_DEBUG_PERFORMER_START") != nullptr;
}

bool is_lower_body_clip_channel(std::string_view name) {
    return name == "bone_facing" ||
           name.find("pelvis") != std::string_view::npos ||
           name.find("-thigh") != std::string_view::npos ||
           name.find("-knee") != std::string_view::npos ||
           name.find("-ankle") != std::string_view::npos ||
           name.find("-foot") != std::string_view::npos ||
           name.find("-toe") != std::string_view::npos;
}

bool is_face_clip_channel(std::string_view name) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return lower.find("face") != std::string::npos ||
           lower.find("mouth") != std::string::npos ||
           lower.find("lip") != std::string::npos ||
           lower.find("jaw") != std::string::npos ||
           lower.find("brow") != std::string::npos ||
           lower.find("lid") != std::string::npos ||
           lower.find("eye") != std::string::npos;
}

void keep_face_channels(ghogx::character::CharClip& clip) {
    if (!clip.loaded) return;
    size_t kept = 0;
    size_t total = 0;
    for (auto& frame : clip.frames) {
        total += frame.size();
        frame.erase(std::remove_if(frame.begin(), frame.end(),
                                   [](const ghogx::character::ClipChannel& ch) {
                                       return !is_face_clip_channel(
                                           ch.bone_name);
                                   }),
                    frame.end());
        kept += frame.size();
    }
    clip.output_bones.erase(
        std::remove_if(clip.output_bones.begin(), clip.output_bones.end(),
                       [](const ghogx::character::CharClip::OutputBone& bone) {
                           return !is_face_clip_channel(bone.name);
                       }),
        clip.output_bones.end());
    std::fprintf(stderr,
                 "[world] face filtered '%s': kept %zu/%zu channels\n",
                 clip.name.c_str(), kept, total);
}

void keep_hand_overlay_channels(ghogx::character::CharClip& clip) {
    if (!clip.loaded) return;
    size_t kept = 0;
    size_t total = 0;
    for (auto& frame : clip.frames) {
        total += frame.size();
        frame.erase(std::remove_if(frame.begin(), frame.end(),
                                   [](const ghogx::character::ClipChannel& ch) {
                                       return is_lower_body_clip_channel(
                                           ch.bone_name);
                                   }),
                    frame.end());
        kept += frame.size();
    }
    clip.output_bones.erase(
        std::remove_if(clip.output_bones.begin(), clip.output_bones.end(),
                       [](const ghogx::character::CharClip::OutputBone& bone) {
                           return is_lower_body_clip_channel(bone.name);
                       }),
        clip.output_bones.end());
    std::fprintf(stderr,
                 "[world] hand-overlay filtered '%s': kept %zu/%zu channels\n",
                 clip.name.c_str(), kept, total);
}

std::optional<float> facefx_eye_register_value(
    const ghogx::character::FaceFxServoTarget& target,
    const ghogx::character::FaceFxEyeProperties& props) {
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    };
    const std::string prop = lower(target.property);
    const std::string object = lower(target.object);
    const bool left = prop.find("l-eye") != std::string::npos ||
                      object == "eye-l.mesh" || object == "l-eye.mesh";
    const bool right = prop.find("r-eye") != std::string::npos ||
                       object == "eye-r.mesh" || object == "r-eye.mesh";
    const bool x_axis = prop.find("eyex") != std::string::npos ||
                        prop.find("eye-x") != std::string::npos ||
                        target.prop_type == 0;
    const bool z_axis = prop.find("eyez") != std::string::npos ||
                        prop.find("eye-z") != std::string::npos ||
                        target.prop_type == 2;
    if (left && x_axis && props.has_l_eye_x) return props.l_eye_x;
    if (left && z_axis && props.has_l_eye_z) return -props.l_eye_z;
    if (right && x_axis && props.has_r_eye_x) return props.r_eye_x;
    if (right && z_axis && props.has_r_eye_z) return -props.r_eye_z;
    return std::nullopt;
}

std::unordered_map<std::string, float> facefx_registers_from_eye_servo(
    const ghogx::character::Character& character,
    const ghogx::character::FaceFxEyeProperties& props) {
    std::unordered_map<std::string, float> registers;
    for (const auto& servo : character.lip_sync_servos) {
        for (const auto& target : servo.targets) {
            if (target.property.empty()) continue;
            if (auto value = facefx_eye_register_value(target, props)) {
                registers[target.property] = *value;
            }
        }
    }
    return registers;
}

float env_float(const char* name, float fallback) {
    if (const char* value = env_value(name)) {
        char* end = nullptr;
        const float parsed = std::strtof(value, &end);
        if (end != value) return parsed;
    }
    return fallback;
}

float character_driver_blend_seconds() {
    return std::clamp(env_float("GHOGX_CHAR_DRIVER_BLEND_SECONDS", 0.25f),
                      0.0f, 2.0f);
}

float character_hand_driver_blend_seconds() {
    return std::clamp(env_float("GHOGX_CHAR_HAND_DRIVER_BLEND_SECONDS",
                                0.08f),
                      0.0f, 1.0f);
}

uint32_t stable_graph_hash(std::string_view text) {
    uint32_t h = 2166136261u;
    for (unsigned char c : text) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

bool graph_continuity_channel(const ghogx::character::ClipChannel& ch) {
    const std::string_view n = ch.bone_name;
    if (n.find("finger") != std::string_view::npos ||
        n.find("thumb") != std::string_view::npos ||
        n.find("pinky") != std::string_view::npos ||
        n.find("ring") != std::string_view::npos ||
        n.find("index") != std::string_view::npos ||
        n.find("brow") != std::string_view::npos ||
        n.find("eye") != std::string_view::npos ||
        n.find("lid") != std::string_view::npos ||
        n.find("lip") != std::string_view::npos ||
        n.find("jaw") != std::string_view::npos ||
        n.find("guitar") != std::string_view::npos ||
        n.find("strum") != std::string_view::npos ||
        n.find("fret") != std::string_view::npos) {
        return false;
    }
    return ch.type == ghogx::character::ClipChannel::kPos ||
           ch.type == ghogx::character::ClipChannel::kQuat ||
           ch.type == ghogx::character::ClipChannel::kRotX ||
           ch.type == ghogx::character::ClipChannel::kRotY ||
           ch.type == ghogx::character::ClipChannel::kRotZ;
}

std::string graph_channel_key(const ghogx::character::ClipChannel& ch) {
    return ch.bone_name + "#" + std::to_string(static_cast<int>(ch.type));
}

double graph_channel_distance(const ghogx::character::ClipChannel& a,
                              const ghogx::character::ClipChannel& b) {
    using ghogx::character::ClipChannel;
    if (a.type != b.type) return 0.0;
    switch (a.type) {
        case ClipChannel::kPos: {
            const double dx = static_cast<double>(a.pos[0] - b.pos[0]);
            const double dy = static_cast<double>(a.pos[1] - b.pos[1]);
            const double dz = static_cast<double>(a.pos[2] - b.pos[2]);
            return (dx * dx + dy * dy + dz * dz) * 0.05;
        }
        case ClipChannel::kQuat: {
            double dot = static_cast<double>(a.quat[0] * b.quat[0] +
                                             a.quat[1] * b.quat[1] +
                                             a.quat[2] * b.quat[2] +
                                             a.quat[3] * b.quat[3]);
            dot = std::clamp(std::abs(dot), 0.0, 1.0);
            return (1.0 - dot) * 8.0;
        }
        case ClipChannel::kRotX:
        case ClipChannel::kRotY:
        case ClipChannel::kRotZ: {
            double d = static_cast<double>(a.angle - b.angle);
            while (d > 3.14159265358979323846) d -= 6.28318530717958647692;
            while (d < -3.14159265358979323846) d += 6.28318530717958647692;
            return d * d;
        }
        default:
            return 0.0;
    }
}

double graph_channel_weight(const ghogx::character::ClipChannel& ch) {
    const std::string_view n = ch.bone_name;
    if (n == "bone_facing" || n == "bone_pelvis" ||
        n.find("-thigh") != std::string_view::npos ||
        n.find("-knee") != std::string_view::npos ||
        n.find("-ankle") != std::string_view::npos ||
        n.find("-foot") != std::string_view::npos ||
        n.find("-toe") != std::string_view::npos) {
        return 6.0;
    }
    if (n.find("spine") != std::string_view::npos ||
        n.find("clavicle") != std::string_view::npos ||
        n.find("upperArm") != std::string_view::npos ||
        n.find("foreArm") != std::string_view::npos ||
        n.find("-hand") != std::string_view::npos) {
        return 1.5;
    }
    return 1.0;
}

bool debug_graph_selection_enabled() {
#ifdef _MSC_VER
    char* value = nullptr;
    size_t len = 0;
    const bool enabled =
        _dupenv_s(&value, &len, "GHOGX_DEBUG_GRAPH_SELECTION") == 0 &&
        value && value[0];
    std::free(value);
    return enabled;
#else
    const char* value = std::getenv("GHOGX_DEBUG_GRAPH_SELECTION");
    return value && value[0];
#endif
}

size_t choose_graph_continuity_clip(
    const std::vector<ghogx::character::CharClip>& clips,
    const std::vector<ghogx::character::ClipChannel>& reference,
    size_t current_index, uint32_t bar, std::string_view salt,
    bool avoid_current) {
    if (clips.empty()) return 0;
    if (reference.empty()) {
        return static_cast<size_t>(
            (stable_graph_hash(salt) + bar) % static_cast<uint32_t>(clips.size()));
    }

    std::unordered_map<std::string, const ghogx::character::ClipChannel*> ref;
    ref.reserve(reference.size());
    for (const auto& ch : reference) {
        if (graph_continuity_channel(ch)) ref[graph_channel_key(ch)] = &ch;
    }
    if (ref.empty()) {
        return static_cast<size_t>(
            (stable_graph_hash(salt) + bar) % static_cast<uint32_t>(clips.size()));
    }

    size_t best = current_index % clips.size();
    double best_score = std::numeric_limits<double>::infinity();
    const bool debug_graph = debug_graph_selection_enabled();
    for (size_t i = 0; i < clips.size(); ++i) {
        const auto& clip = clips[i];
        if (!clip.loaded || clip.frames.empty()) continue;
        double score = 0.0;
        double matched_weight = 0.0;
        for (const auto& ch : clip.frames.front()) {
            if (!graph_continuity_channel(ch)) continue;
            auto it = ref.find(graph_channel_key(ch));
            if (it == ref.end()) continue;
            const double weight = graph_channel_weight(ch);
            score += graph_channel_distance(*it->second, ch) * weight;
            matched_weight += weight;
        }
        if (matched_weight <= 0.0) continue;
        score /= matched_weight;
        if (avoid_current && clips.size() > 1 && i == current_index) {
            score += 0.25;
        }
        score += static_cast<double>(
                     (stable_graph_hash(clip.name) + bar) & 0xffu) *
                 1.0e-6;
        if (debug_graph) {
            std::fprintf(stderr,
                         "[graph-select] salt=%.*s bar=%u clip=%s index=%zu "
                         "score=%.6f matched_weight=%.2f avoid=%d current=%d\n",
                         static_cast<int>(salt.size()), salt.data(), bar,
                         clip.name.c_str(), i, score, matched_weight,
                         avoid_current ? 1 : 0,
                         i == current_index ? 1 : 0);
        }
        if (score < best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

std::optional<float> lower_body_stance_width(
    const ghogx::character::Character& character) {
    auto sample = [&](std::initializer_list<const char*> names)
        -> std::optional<std::array<float, 3>> {
        std::array<float, 3> sum{0.0f, 0.0f, 0.0f};
        int count = 0;
        for (const char* name : names) {
            for (const auto& bone : character.bones) {
                if (bone.name != name) continue;
                const auto world = character.bone_world_local_chain(bone.name);
                sum[0] += world[12];
                sum[1] += world[13];
                sum[2] += world[14];
                ++count;
                break;
            }
        }
        if (count == 0) return std::nullopt;
        const float inv = 1.0f / static_cast<float>(count);
        return std::array<float, 3>{sum[0] * inv, sum[1] * inv, sum[2] * inv};
    };
    const auto left = sample({"bone_L-ankle.mesh", "bone_L-toe.mesh"});
    const auto right = sample({"bone_R-ankle.mesh", "bone_R-toe.mesh"});
    if (!left || !right) return std::nullopt;
    const float dx = (*left)[0] - (*right)[0];
    const float dy = (*left)[1] - (*right)[1];
    return std::sqrt(dx * dx + dy * dy);
}

std::string clip_family_name(std::string name) {
    const size_t last = name.rfind('_');
    if (last == std::string::npos) return name;
    bool numeric = last + 1 < name.size();
    for (size_t i = last + 1; i < name.size(); ++i) {
        numeric = numeric && std::isdigit(static_cast<unsigned char>(name[i]));
    }
    if (!numeric) return name;
    return name.substr(0, last);
}

size_t choose_stance_continuity_clip(
    const std::vector<ghogx::character::CharClip>& clips,
    const std::vector<ghogx::character::ClipChannel>& reference,
    const ghogx::character::Character& base_character, size_t fallback_index,
    size_t current_index, uint32_t bar, std::string_view salt,
    bool avoid_current) {
    if (clips.empty() || reference.empty()) return fallback_index;

    ghogx::character::Character ref_character = base_character;
    ghogx::character::apply_clip_pose_sampled(reference, 1.0f, ref_character);
    const auto ref_width = lower_body_stance_width(ref_character);
    if (!ref_width) return fallback_index;

    const bool debug_graph = debug_graph_selection_enabled();
    const std::string family =
        fallback_index < clips.size()
            ? clip_family_name(clips[fallback_index].name)
            : std::string{};
    const bool constrain_family =
        !family.empty() &&
        std::count_if(clips.begin(), clips.end(), [&](const auto& clip) {
            return clip_family_name(clip.name) == family;
        }) > 1;
    size_t best = fallback_index % clips.size();
    float best_score = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < clips.size(); ++i) {
        const auto& clip = clips[i];
        if (!clip.loaded || clip.frames.empty()) continue;
        if (constrain_family && clip_family_name(clip.name) != family) continue;
        ghogx::character::Character candidate = base_character;
        ghogx::character::apply_clip_pose_sampled(clip.frames.front(), 1.0f,
                                                  candidate, clip.relative);
        const auto width = lower_body_stance_width(candidate);
        if (!width) continue;
        float score = std::fabs(*width - *ref_width);
        if (avoid_current && clips.size() > 1 && i == current_index) {
            score += 0.5f;
        }
        score += static_cast<float>(
                     (stable_graph_hash(clip.name) + bar) & 0xffu) *
                 1.0e-5f;
        if (debug_graph) {
            std::fprintf(stderr,
                         "[stance-select] salt=%.*s bar=%u clip=%s index=%zu "
                         "width=%.3f ref=%.3f score=%.6f fallback=%d\n",
                         static_cast<int>(salt.size()), salt.data(), bar,
                         clip.name.c_str(), i, *width, *ref_width, score,
                         i == fallback_index ? 1 : 0);
        }
        if (score < best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

std::optional<Gameplay::QuickplayRig> resolve_quickplay_rig(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& shortname) {
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find("config/gen/songs.dtb");
        if (!entry) return std::nullopt;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto songs = ghogx::catalog::extract_songs(gh::dtb::parse(bytes));
        for (const auto& song : songs) {
            if (song.shortname != shortname || !song.quickplay) continue;
            std::vector<std::string> band = song.band;
            if (band.empty()) {
                if (auto gh2 = ark.find("config/gen/gh2.dtb")) {
                    auto gh2_bytes = ark.read_entry(*gh2, {ark_path});
                    auto gh2_tree = gh::dtb::parse(gh2_bytes);
                    if (auto def = gh::dtb::find_keyed(gh2_tree, "default_band")) {
                        const auto& kids = gh::dtb::children(*def);
                        for (size_t i = 1; i < kids.size(); ++i) {
                            if (auto s = gh::dtb::as_string(*kids[i]))
                                band.push_back(*s);
                        }
                    }
                }
            }
            return Gameplay::QuickplayRig{song.quickplay->character_outfit,
                                          song.quickplay->guitar,
                                          song.quickplay->venue,
                                          std::move(band)};
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] config/gen/songs.dtb: %s\n", ex.what());
    }
    return std::nullopt;
}

std::string guitar_milo_for_quickplay(const std::string& symbol) {
    // Source: config/gen/songs.dtb quickplay guitar symbols, resolved to the
    // actual PS2 ARK entries under char/og/guitars/gen.
    static const std::unordered_map<std::string, std::string> kMap = {
        {"flying_v", "flyingv_v2"},
        {"sg", "guitar_sg"},
        {"lespaul", "lespaull"},
        {"xplorer", "xplorer"},
    };
    auto it = kMap.find(symbol);
    const std::string stem = (it == kMap.end()) ? symbol : it->second;
    return "char/og/guitars/gen/" + stem + ".milo_ps2";
}

std::optional<std::string> first_bass_guitar_milo(
    const std::string& hdr_path, const std::string& ark_path) {
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find("config/gen/guitars.dtb");
        if (!entry) return std::nullopt;
        auto tree = gh::dtb::parse(ark.read_entry(*entry, {ark_path}));
        for (const auto& root : tree.root) {
            if (!root || !gh::dtb::is_array(*root)) continue;
            auto type = gh::dtb::find_keyed(*root, "type");
            if (!type) continue;
            const auto& type_kids = gh::dtb::children(*type);
            if (type_kids.size() < 2) continue;
            if (gh::dtb::as_string(*type_kids[1]).value_or("") != "bass")
                continue;
            auto skins = gh::dtb::find_keyed(*root, "skins");
            if (!skins) continue;
            for (const auto& skin : gh::dtb::children(*skins)) {
                if (!skin || !gh::dtb::is_array(*skin)) continue;
                const auto& kids = gh::dtb::children(*skin);
                if (kids.empty()) continue;
                if (auto outfit = gh::dtb::as_string(*kids[0])) {
                    return "char/og/guitars/gen/" + *outfit + ".milo_ps2";
                }
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] config/gen/guitars.dtb: %s\n", ex.what());
    }
    return std::nullopt;
}

std::vector<std::string> scan_milo_strings(const uint8_t* body, size_t size) {
    std::vector<std::string> out;
    for (size_t i = 0; i + 4 <= size; ++i) {
        uint32_t len = 0;
        std::memcpy(&len, body + i, sizeof(len));
        if (len == 0 || len > 64 || i + 4 + len > size) continue;
        bool printable = true;
        for (uint32_t j = 0; j < len; ++j) {
            const uint8_t c = body[i + 4 + j];
            if (c < 0x20 || c > 0x7e) {
                printable = false;
                break;
            }
        }
        if (!printable) continue;
        std::string s(reinterpret_cast<const char*>(body + i + 4), len);
        if (s.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ==
            std::string::npos)
            continue;
        if (out.empty() || out.back() != s) out.push_back(std::move(s));
        i += 3 + len;
    }
    return out;
}

struct PackedStringHit {
    size_t offset = 0;
    size_t end = 0;
    std::string value;
};

std::vector<PackedStringHit> packed_strings_with_offsets(const uint8_t* body,
                                                         size_t size) {
    std::vector<PackedStringHit> out;
    for (size_t off = 0; off + 4 <= size; ++off) {
        uint32_t len = 0;
        std::memcpy(&len, body + off, sizeof(len));
        if (len == 0 || len > 96 || off + 4 + len > size) continue;
        const char* s = reinterpret_cast<const char*>(body + off + 4);
        bool printable = true;
        for (uint32_t i = 0; i < len; ++i) {
            const unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 32 || c > 126) {
                printable = false;
                break;
            }
        }
        if (!printable) continue;
        out.push_back({off, off + 4 + len, std::string(s, s + len)});
    }
    return out;
}

uint32_t read_u32_at_unchecked(const uint8_t* body, size_t off) {
    uint32_t v = 0;
    std::memcpy(&v, body + off, sizeof(v));
    return v;
}

float read_f32_at_unchecked(const uint8_t* body, size_t off) {
    float v = 0.0f;
    std::memcpy(&v, body + off, sizeof(v));
    return v;
}

std::string canonical_milo_ref(std::string s) {
    static constexpr std::string_view suffixes[] = {
        ".mesh", ".filt", ".grp", ".tnm", ".mnm", ".enm", ".lnm",
        ".msnm", ".meshanim", ".part", ".panim", ".trig"};
    for (const auto suffix : suffixes) {
        const size_t pos = s.find(suffix);
        if (pos != std::string::npos) {
            s.resize(pos + suffix.size());
            return s;
        }
    }
    return s;
}

std::string next_string_after(const std::vector<std::string>& strings,
                              std::string_view key) {
    for (size_t i = 0; i + 1 < strings.size(); ++i) {
        if (strings[i] == key) return strings[i + 1];
    }
    return {};
}

bool bool_string_after(const std::vector<std::string>& strings,
                       std::string_view key, bool fallback) {
    const std::string value = next_string_after(strings, key);
    if (value == "TRUE") return true;
    if (value == "FALSE") return false;
    return fallback;
}

std::optional<bool> milo_bool_property(const uint8_t* body, size_t size,
                                       std::string_view key) {
    if (key.empty() || key.size() > 64) return std::nullopt;
    for (size_t i = 0; i + 4 + key.size() + 8 <= size; ++i) {
        uint32_t len = 0;
        std::memcpy(&len, body + i, sizeof(len));
        if (len != key.size()) continue;
        if (std::memcmp(body + i + 4, key.data(), key.size()) != 0) continue;
        const size_t value_off = i + 4 + key.size();
        uint32_t tag = 0;
        uint32_t value = 0;
        std::memcpy(&tag, body + value_off, sizeof(tag));
        std::memcpy(&value, body + value_off + 4, sizeof(value));
        if (tag == 0 && value <= 1) return value != 0;
    }
    return std::nullopt;
}

bool camshot_bool_property(const uint8_t* body, size_t size,
                           const std::vector<std::string>& strings,
                           std::string_view key, bool fallback) {
    if (auto value = milo_bool_property(body, size, key)) return *value;
    return bool_string_after(strings, key, fallback);
}

bool is_performer_entity(std::string_view s) {
    return s == "guitarist0" || s == "guitarist1" || s == "bassist" ||
           s == "singer" || s == "drummer" || s == "keyboard";
}

bool is_target_subpart(std::string_view s) {
    return s.rfind("bone_", 0) == 0 || s.rfind("spot_", 0) == 0 ||
           s.find(".mesh") != std::string_view::npos;
}

void infer_camshot_target(const std::vector<std::string>& strings,
                          std::string_view shot_name,
                          Gameplay::CameraKey& key) {
    auto entity_from_shot = [](std::string_view name) -> const char* {
        if (name.find("bass") != std::string_view::npos) return "bassist";
        if (name.find("singer") != std::string_view::npos) return "singer";
        if (name.find("drum") != std::string_view::npos) return "drummer";
        if (name.find("key") != std::string_view::npos) return "keyboard";
        return nullptr;
    };
    const char* hinted_entity = entity_from_shot(shot_name);
    for (size_t i = 0; i < strings.size(); ++i) {
        if (!is_performer_entity(strings[i])) continue;
        key.target_entity = hinted_entity ? hinted_entity : strings[i];
        if (i + 1 < strings.size() && is_target_subpart(strings[i + 1])) {
            key.target_subpart = strings[i + 1];
        }
        return;
    }
    for (const auto& s : strings) {
        if (!is_target_subpart(s)) continue;
        key.target_subpart = s;
        break;
    }
    if (!key.target_subpart.empty()) {
        key.target_entity = hinted_entity ? hinted_entity : "guitarist0";
    }
}

std::string camera_target_id(std::string_view entity, std::string_view subpart) {
    std::string id(entity);
    if (!subpart.empty()) {
        id += ':';
        id += subpart;
    }
    return id;
}

std::string strip_mesh_suffix(std::string name) {
    constexpr std::string_view suffix = ".mesh";
    if (name.size() > suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
        name.resize(name.size() - suffix.size());
    }
    return name;
}

bool debug_hand_pose_rows_enabled() {
    return env_value("GHOGX_DEBUG_HAND_POSE_ROWS") != nullptr;
}

bool debug_left_hand_contact_enabled() {
    return env_value("GHOGX_DEBUG_LEFT_HAND_CONTACT") != nullptr;
}

bool hand_pose_bone_matches(std::string candidate, std::string wanted) {
    return strip_mesh_suffix(std::move(candidate)) ==
           strip_mesh_suffix(std::move(wanted));
}

const milo_scene::TransObj* find_hand_pose_bone(
    const ghogx::character::Character& character, const char* wanted) {
    for (const auto& bone : character.bones) {
        if (hand_pose_bone_matches(bone.name, wanted)) return &bone;
    }
    return nullptr;
}

std::array<float, 3> mat4_pos_game(const std::array<float, 16>& m) {
    return {m[12], m[13], m[14]};
}

std::array<float, 3> mat4_basis_delta_game(const std::array<float, 16>& basis,
                                           const std::array<float, 3>& point) {
    const float dx = point[0] - basis[12];
    const float dy = point[1] - basis[13];
    const float dz = point[2] - basis[14];
    return {
        basis[0] * dx + basis[1] * dy + basis[2] * dz,
        basis[4] * dx + basis[5] * dy + basis[6] * dz,
        basis[8] * dx + basis[9] * dy + basis[10] * dz,
    };
}

std::array<float, 16> mat4_affine_inverse_game(
    const std::array<float, 16>& m) {
    const float a = m[0], b = m[1], c = m[2];
    const float d = m[4], e = m[5], f = m[6];
    const float g = m[8], h = m[9], i = m[10];
    const float det =
        a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    const float id = (std::fabs(det) > 1e-12f) ? 1.0f / det : 0.0f;
    std::array<float, 16> r{};
    r[0] = (e * i - f * h) * id;
    r[1] = (c * h - b * i) * id;
    r[2] = (b * f - c * e) * id;
    r[4] = (f * g - d * i) * id;
    r[5] = (a * i - c * g) * id;
    r[6] = (c * d - a * f) * id;
    r[8] = (d * h - e * g) * id;
    r[9] = (b * g - a * h) * id;
    r[10] = (a * e - b * d) * id;
    const float tx = m[12], ty = m[13], tz = m[14];
    r[12] = -(tx * r[0] + ty * r[4] + tz * r[8]);
    r[13] = -(tx * r[1] + ty * r[5] + tz * r[9]);
    r[14] = -(tx * r[2] + ty * r[6] + tz * r[10]);
    r[15] = 1.0f;
    return r;
}

std::array<float, 3> mat4_xform_point_game(
    const std::array<float, 16>& m, const std::array<float, 3>& p) {
    return {
        p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12],
        p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
        p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14],
    };
}

void dump_left_hand_prop_contact_rows(
    const ghogx::character::Character& character, std::string_view role,
    std::string_view phase, double song_time, uint32_t tick, uint32_t mask,
    const std::array<float, 16>* performer_world,
    const std::array<float, 16>* guitar_strings_world) {
    if (!debug_left_hand_contact_enabled() || !performer_world ||
        !guitar_strings_world) {
        return;
    }
    if (const char* only = env_value("GHOGX_DEBUG_LEFT_HAND_CONTACT_ROLE")) {
        if (role != only) return;
    }

    static constexpr const char* kPoints[] = {
        "bone_L-hand",
        "bone_L-thumb01",
        "bone_L-thumb02",
        "bone_L-thumb03",
        "bone_L-index01",
        "bone_L-index02",
        "bone_L-index03",
        "bone_L-middlefinger01",
        "bone_L-middlefinger02",
        "bone_L-middlefinger03",
        "bone_L-ringfinger01",
        "bone_L-ringfinger02",
        "bone_L-ringfinger03",
        "bone_L-pinky01",
        "bone_L-pinky02",
        "bone_L-pinky03",
    };

    const auto inv_strings = mat4_affine_inverse_game(*guitar_strings_world);
    std::fprintf(stderr,
                 "[hand-prop-ref] phase=%.*s role=%.*s t=%.3f tick=%u "
                 "mask=0x%02x ref=guitar_strings.mesh "
                 "world=(%.5f %.5f %.5f) "
                 "r0=(%.5f %.5f %.5f) r1=(%.5f %.5f %.5f) "
                 "r2=(%.5f %.5f %.5f)\n",
                 static_cast<int>(phase.size()), phase.data(),
                 static_cast<int>(role.size()), role.data(), song_time, tick,
                 mask & 0x1fu, (*guitar_strings_world)[12],
                 (*guitar_strings_world)[13], (*guitar_strings_world)[14],
                 (*guitar_strings_world)[0], (*guitar_strings_world)[1],
                 (*guitar_strings_world)[2], (*guitar_strings_world)[4],
                 (*guitar_strings_world)[5], (*guitar_strings_world)[6],
                 (*guitar_strings_world)[8], (*guitar_strings_world)[9],
                 (*guitar_strings_world)[10]);

    for (const char* point_name : kPoints) {
        const auto* point = find_hand_pose_bone(character, point_name);
        if (!point) continue;
        const auto point_world = character.bone_world_local_chain(point->name);
        const auto point_pos = mat4_pos_game(point_world);
        const auto staged = mat4_xform_point_game(*performer_world, point_pos);
        const auto local = mat4_xform_point_game(inv_strings, staged);
        std::fprintf(stderr,
                     "[hand-prop-contact] phase=%.*s role=%.*s t=%.3f "
                     "tick=%u mask=0x%02x ref=guitar_strings.mesh "
                     "point=%s exact=%s local=(%.5f %.5f %.5f) "
                     "world=(%.5f %.5f %.5f)\n",
                     static_cast<int>(phase.size()), phase.data(),
                     static_cast<int>(role.size()), role.data(), song_time,
                     tick, mask & 0x1fu, point_name, point->name.c_str(),
                     local[0], local[1], local[2], staged[0], staged[1],
                     staged[2]);
    }
}

void dump_left_hand_contact_rows(
    const ghogx::character::Character& character, std::string_view role,
    std::string_view phase, double song_time, uint32_t tick, uint32_t mask) {
    if (!debug_left_hand_contact_enabled()) return;
    if (const char* only = env_value("GHOGX_DEBUG_LEFT_HAND_CONTACT_ROLE")) {
        if (role != only) return;
    }

    static constexpr const char* kRefs[] = {
        "bone_fret_hand",
        "bone_fret",
        "bone_pos_guitar",
        "spot_neck_fret01",
        "spot_neck_fret05",
        "spot_neck_fret10",
        "spot_neck_fret15",
        "spot_neck_fret20",
    };
    static constexpr const char* kPoints[] = {
        "bone_L-hand",
        "bone_L-thumb01",
        "bone_L-thumb02",
        "bone_L-thumb03",
        "bone_L-index01",
        "bone_L-index02",
        "bone_L-index03",
        "bone_L-middlefinger01",
        "bone_L-ringfinger01",
        "bone_L-pinky01",
    };

    for (const char* ref_name : kRefs) {
        const auto* ref = find_hand_pose_bone(character, ref_name);
        if (!ref) continue;
        const auto ref_world = character.bone_world_local_chain(ref->name);
        const auto ref_pos = mat4_pos_game(ref_world);
        std::fprintf(stderr,
                     "[hand-contact-ref] phase=%.*s role=%.*s t=%.3f "
                     "tick=%u mask=0x%02x ref=%s exact=%s "
                     "world=(%.5f %.5f %.5f) "
                     "r0=(%.5f %.5f %.5f) r1=(%.5f %.5f %.5f) "
                     "r2=(%.5f %.5f %.5f)\n",
                     static_cast<int>(phase.size()), phase.data(),
                     static_cast<int>(role.size()), role.data(), song_time,
                     tick, mask & 0x1fu, ref_name, ref->name.c_str(),
                     ref_pos[0], ref_pos[1], ref_pos[2], ref_world[0],
                     ref_world[1], ref_world[2], ref_world[4], ref_world[5],
                     ref_world[6], ref_world[8], ref_world[9],
                     ref_world[10]);

        for (const char* point_name : kPoints) {
            const auto* point = find_hand_pose_bone(character, point_name);
            if (!point) continue;
            const auto point_world = character.bone_world_local_chain(point->name);
            const auto point_pos = mat4_pos_game(point_world);
            const auto d = mat4_basis_delta_game(ref_world, point_pos);
            std::fprintf(stderr,
                         "[hand-contact] phase=%.*s role=%.*s t=%.3f "
                         "tick=%u mask=0x%02x ref=%s point=%s exact=%s "
                         "localDelta=(%.5f %.5f %.5f) "
                         "world=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(),
                         static_cast<int>(role.size()), role.data(),
                         song_time, tick, mask & 0x1fu, ref_name, point_name,
                         point->name.c_str(), d[0], d[1], d[2], point_pos[0],
                         point_pos[1], point_pos[2]);
        }
    }
}

void dump_hand_pose_rows(const ghogx::character::Character& character,
                         std::string_view role, std::string_view phase,
                         double song_time, uint32_t tick, uint32_t mask,
                         const std::vector<std::string>& strum_clips,
                         const std::vector<std::string>& fret_clips,
                         const std::array<float, 16>* performer_world,
                         const std::array<float, 16>* guitar_strings_world) {
    const bool dump_rows = debug_hand_pose_rows_enabled();
    const bool dump_contact = debug_left_hand_contact_enabled();
    if (!dump_rows && !dump_contact) return;
    if (const char* only = env_value("GHOGX_DEBUG_HAND_POSE_ROLE")) {
        if (role != only) return;
    }

    const float stride =
        std::max(0.0f, env_float("GHOGX_DEBUG_HAND_POSE_STRIDE", 0.0f));
    if (stride > 0.0f) {
        static std::unordered_map<std::string, double> next_log_time;
        std::string key(role);
        key += ':';
        key += phase;
        double& next_time = next_log_time[key];
        if (song_time + 1e-5 < next_time) return;
        next_time = song_time + static_cast<double>(stride);
    }

    if (dump_contact) {
        dump_left_hand_contact_rows(character, role, phase, song_time, tick,
                                    mask);
        dump_left_hand_prop_contact_rows(character, role, phase, song_time,
                                         tick, mask, performer_world,
                                         guitar_strings_world);
    }
    if (!dump_rows) return;

    auto print_clip_list = [](const std::vector<std::string>& names) {
        if (names.empty()) {
            std::fprintf(stderr, "-");
            return;
        }
        for (size_t i = 0; i < names.size(); ++i) {
            std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                         names[i].c_str());
        }
    };

    std::fprintf(stderr,
                 "[handpose] phase=%.*s role=%.*s t=%.3f tick=%u mask=0x%02x "
                 "strum=",
                 static_cast<int>(phase.size()), phase.data(),
                 static_cast<int>(role.size()), role.data(), song_time, tick,
                 mask & 0x1fu);
    print_clip_list(strum_clips);
    std::fprintf(stderr, " fret=");
    print_clip_list(fret_clips);
    std::fprintf(stderr, "\n");

    static constexpr const char* kBones[] = {
        "bone_pos_guitar",
        "bone_fret",
        "bone_L-upperArm",
        "bone_L-foreArm",
        "bone_L-hand",
        "bone_fret_hand",
        "bone_L-index01",
        "bone_L-index02",
        "bone_L-index03",
        "bone_L-middlefinger01",
        "bone_L-middlefinger02",
        "bone_L-middlefinger03",
        "bone_L-ringfinger01",
        "bone_L-ringfinger02",
        "bone_L-ringfinger03",
        "bone_L-pinky01",
        "bone_L-pinky02",
        "bone_L-pinky03",
        "bone_L-thumb01",
        "bone_L-thumb02",
        "bone_L-thumb03",
        "bone_strum_hand",
        "bone_R-index01",
        "bone_R-index02",
        "bone_R-index03",
        "bone_R-middlefinger01",
        "bone_R-middlefinger02",
        "bone_R-middlefinger03",
        "bone_R-ringfinger01",
        "bone_R-ringfinger02",
        "bone_R-ringfinger03",
        "bone_R-pinky01",
        "bone_R-pinky02",
        "bone_R-pinky03",
        "bone_R-thumb01",
        "bone_R-thumb02",
        "bone_R-thumb03",
    };

    for (const char* wanted : kBones) {
        const auto* bone = find_hand_pose_bone(character, wanted);
        if (!bone) continue;
        const auto world = character.bone_world_local_chain(bone->name);
        const auto& local = bone->local;
        std::fprintf(stderr,
                     "[hpos] ph=%.*s b=%s l=(%.5f %.5f %.5f)\n",
                     static_cast<int>(phase.size()), phase.data(), wanted,
                     local.pos[0], local.pos[1], local.pos[2]);
        std::fprintf(stderr,
                     "[hpos] ph=%.*s b=%s w=(%.5f %.5f %.5f)\n",
                     static_cast<int>(phase.size()), phase.data(), wanted,
                     world[12], world[13], world[14]);
        const bool relation_bone =
            std::strcmp(wanted, "bone_pos_guitar") == 0 ||
            std::strcmp(wanted, "bone_fret") == 0 ||
            std::strcmp(wanted, "bone_fret_hand") == 0 ||
            std::strcmp(wanted, "bone_L-hand") == 0 ||
            std::strcmp(wanted, "bone_L-thumb01") == 0 ||
            std::strcmp(wanted, "bone_L-thumb02") == 0 ||
            std::strcmp(wanted, "bone_L-thumb03") == 0 ||
            std::strcmp(wanted, "bone_L-index01") == 0 ||
            std::strcmp(wanted, "bone_L-index02") == 0 ||
            std::strcmp(wanted, "bone_L-index03") == 0 ||
            std::strcmp(wanted, "bone_L-middlefinger01") == 0 ||
            std::strcmp(wanted, "bone_L-middlefinger02") == 0 ||
            std::strcmp(wanted, "bone_L-middlefinger03") == 0 ||
            std::strcmp(wanted, "bone_L-ringfinger01") == 0 ||
            std::strcmp(wanted, "bone_L-ringfinger02") == 0 ||
            std::strcmp(wanted, "bone_L-ringfinger03") == 0 ||
            std::strcmp(wanted, "bone_L-pinky01") == 0 ||
            std::strcmp(wanted, "bone_L-pinky02") == 0 ||
            std::strcmp(wanted, "bone_L-pinky03") == 0;
        if (relation_bone) {
            std::fprintf(stderr,
                         "[hrow] ph=%.*s b=%s r0=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(), wanted,
                         local.rot[0][0], local.rot[0][1], local.rot[0][2]);
            std::fprintf(stderr,
                         "[hrow] ph=%.*s b=%s r1=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(), wanted,
                         local.rot[1][0], local.rot[1][1], local.rot[1][2]);
            std::fprintf(stderr,
                         "[hrow] ph=%.*s b=%s r2=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(), wanted,
                         local.rot[2][0], local.rot[2][1], local.rot[2][2]);
            std::fprintf(stderr,
                         "[hwrow] ph=%.*s b=%s r0=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(), wanted,
                         world[0], world[1], world[2]);
            std::fprintf(stderr,
                         "[hwrow] ph=%.*s b=%s r1=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(), wanted,
                         world[4], world[5], world[6]);
            std::fprintf(stderr,
                         "[hwrow] ph=%.*s b=%s r2=(%.5f %.5f %.5f)\n",
                         static_cast<int>(phase.size()), phase.data(), wanted,
                         world[8], world[9], world[10]);
        }
        std::fprintf(
            stderr,
            "[handpose] phase=%.*s role=%.*s bone=%s exact=%s "
            "localPos=(%.5f %.5f %.5f) world=(%.5f %.5f %.5f) "
            "r0=(%.5f %.5f %.5f) r1=(%.5f %.5f %.5f) "
            "r2=(%.5f %.5f %.5f)\n",
            static_cast<int>(phase.size()), phase.data(),
            static_cast<int>(role.size()), role.data(), wanted,
            bone->name.c_str(), local.pos[0], local.pos[1], local.pos[2],
            world[12], world[13], world[14], local.rot[0][0],
            local.rot[0][1], local.rot[0][2], local.rot[1][0],
            local.rot[1][1], local.rot[1][2], local.rot[2][0],
            local.rot[2][1], local.rot[2][2]);
    }
}

std::array<float, 16> mat4_mul_game(const std::array<float, 16>& a,
                                    const std::array<float, 16>& b) {
    std::array<float, 16> r{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a[i * 4 + k] * b[k * 4 + j];
            r[i * 4 + j] = s;
        }
    }
    return r;
}

struct VenueCameraPolicy {
    std::string intro_distance;
    std::string intro_facing;
    std::map<std::string, std::pair<int, int>> duration_bars = {
        {"kExcitementOkay", {2, 4}},
    };
};

VenueCameraPolicy load_venue_camera_policy(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& venue) {
    VenueCameraPolicy p;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        const std::string dtb_path =
            "world/" + venue + "/gen/" + venue + ".dtb";
        auto entry = ark.find(dtb_path);
        if (!entry) return p;
        auto tree = gh::dtb::parse(ark.read_entry(*entry, {ark_path}));
        if (auto world_dir = gh::dtb::find_keyed(tree, "WorldDir")) {
            if (auto types = gh::dtb::find_keyed(*world_dir, "types")) {
                const gh::dtb::Node* venue_node = nullptr;
                for (const auto& child : gh::dtb::children(*types)) {
                    if (!child || !gh::dtb::is_array(*child)) continue;
                    const auto& kids = gh::dtb::children(*child);
                    if (kids.empty()) continue;
                    if (gh::dtb::as_string(*kids[0]).value_or("") == venue) {
                        venue_node = child.get();
                        break;
                    }
                }
                if (!venue_node) venue_node = types.get();
                if (auto d = gh::dtb::find_keyed(*venue_node, "intro_camera_distance")) {
                    const auto& kids = gh::dtb::children(*d);
                    if (kids.size() > 1)
                        p.intro_distance =
                            gh::dtb::as_string(*kids[1]).value_or("");
                }
                if (auto f = gh::dtb::find_keyed(*venue_node, "intro_camera_facing")) {
                    const auto& kids = gh::dtb::children(*f);
                    if (kids.size() > 1)
                        p.intro_facing =
                            gh::dtb::as_string(*kids[1]).value_or("");
                }
                if (auto durs = gh::dtb::find_keyed(*venue_node, "camera_durations")) {
                    const auto& kids = gh::dtb::children(*durs);
                    if (kids.size() > 1 && kids[1] &&
                        gh::dtb::is_array(*kids[1])) {
                        for (const auto& row : gh::dtb::children(*kids[1])) {
                            if (!row || !gh::dtb::is_array(*row)) continue;
                            const auto& vals = gh::dtb::children(*row);
                            if (vals.size() < 3) continue;
                            const std::string key =
                                gh::dtb::as_string(*vals[0]).value_or("");
                            if (key.empty()) continue;
                            auto lo = gh::dtb::as_int(*vals[1]);
                            auto hi = gh::dtb::as_int(*vals[2]);
                            if (!lo || !hi) continue;
                            p.duration_bars[key] = {*lo, *hi};
                        }
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] venue camera policy: %s\n", ex.what());
    }
    return p;
}

std::string select_intro_camera_anim(const std::string& hdr_path,
                                     const std::string& ark_path,
                                     const std::string& venue) {
    const VenueCameraPolicy policy =
        load_venue_camera_policy(hdr_path, ark_path, venue);
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        const std::string milo_path =
            "world/" + venue + "/gen/" + venue + ".milo_ps2";
        auto entry = ark.find(milo_path);
        if (!entry) return "Intro.tnm";
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        struct Candidate {
            std::string shot;
            std::string anim;
            int score = 0;
        };
        std::vector<Candidate> candidates;
        for (const auto& de : dir.entries) {
            if (de.type != "CamShot" || de.offset + de.size > payload.size())
                continue;
            const uint8_t* body = payload.data() + de.offset;
            auto strings = scan_milo_strings(body, static_cast<size_t>(de.size));
            bool is_intro = false;
            for (const auto& s : strings) {
                if (s == "INTRO") is_intro = true;
            }
            if (!is_intro) continue;
            Candidate c;
            c.shot = de.name;
            c.anim = {};
            for (const auto& s : strings) {
                if (s.size() > 4 && s.rfind(".tnm") == s.size() - 4) {
                    c.anim = s;
                    break;
                }
            }
            if (c.anim.empty()) continue;
            const std::string distance = next_string_after(strings, "distance");
            const std::string facing = next_string_after(strings, "facing");
            if (!policy.intro_distance.empty() &&
                distance == policy.intro_distance) {
                c.score += 2;
            }
            if (!policy.intro_facing.empty() && facing == policy.intro_facing) {
                c.score += 2;
            }
            if (c.shot.rfind("intro", 0) == 0) c.score += 1;
            candidates.push_back(std::move(c));
        }
        if (!candidates.empty()) {
            std::stable_sort(candidates.begin(), candidates.end(),
                             [](const Candidate& a, const Candidate& b) {
                                 return a.score > b.score;
                             });
            std::fprintf(stderr,
                         "[world] intro CamShot %s -> %s (score=%d, policy distance=%s facing=%s)\n",
                         candidates.front().shot.c_str(),
                         candidates.front().anim.c_str(), candidates.front().score,
                         policy.intro_distance.c_str(),
                         policy.intro_facing.c_str());
            return candidates.front().anim;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] intro camera select: %s\n", ex.what());
    }
    return "Intro.tnm";
}

std::vector<std::string> texture_names_for_scene(
    const ghogx::milo_scene::Scene& scene) {
    std::unordered_set<std::string> unique;
    for (const auto& mat : scene.mats)
        if (!mat.diffuse_tex.empty()) unique.insert(mat.diffuse_tex);
    return std::vector<std::string>(unique.begin(), unique.end());
}

std::unordered_set<std::string> mesh_names_in_groups(
    const ghogx::milo_scene::Scene& scene,
    std::initializer_list<const char*> group_names) {
    std::unordered_set<std::string> wanted_groups(group_names.begin(),
                                                  group_names.end());
    std::unordered_set<std::string> hidden;
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& group : scene.groups) {
            if (wanted_groups.find(group.name) == wanted_groups.end()) {
                continue;
            }
            for (const auto& child : group.children) {
                if (child.size() > 5 &&
                    child.rfind(".mesh") == child.size() - 5) {
                    hidden.insert(child);
                } else if (child.size() > 4 &&
                           child.rfind(".grp") == child.size() - 4 &&
                           wanted_groups.insert(child).second) {
                    changed = true;
                }
            }
        }
    }
    return hidden;
}

std::unordered_set<std::string> mesh_names_with_materials(
    const ghogx::milo_scene::Scene& scene,
    std::initializer_list<const char*> material_names) {
    std::unordered_set<std::string> wanted(material_names.begin(),
                                           material_names.end());
    std::unordered_set<std::string> hidden;
    for (const auto& mesh : scene.meshes) {
        if (wanted.find(mesh.material) != wanted.end()) {
            hidden.insert(mesh.name);
        }
    }
    return hidden;
}

std::string strip_trigger_suffix(std::string name) {
    constexpr std::string_view suffix = ".trig";
    if (name.size() >= suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
        name.resize(name.size() - suffix.size());
    }
    return name;
}

bool is_event_payload_label(std::string_view text) {
    return !text.empty() && text.find('.') == std::string_view::npos;
}

void push_unique_ref(std::vector<std::string>& values, const std::string& ref) {
    if (ref.empty()) return;
    if (std::find(values.begin(), values.end(), ref) == values.end())
        values.push_back(ref);
}

std::vector<std::string> event_trigger_route_keys(
    const std::string& trigger_name, std::string_view payload_label) {
    std::vector<std::string> keys;
    if (is_event_payload_label(payload_label))
        keys.emplace_back(payload_label);
    const std::string object_key = strip_trigger_suffix(trigger_name);
    push_unique_ref(keys, object_key);
    return keys;
}

void merge_venue_group_visibility(Gameplay::VenueGroupVisibility& dst,
                                  const Gameplay::VenueGroupVisibility& src) {
    for (const auto& mesh : src.show_meshes)
        push_unique_ref(dst.show_meshes, mesh);
    for (const auto& mesh : src.hide_meshes)
        push_unique_ref(dst.hide_meshes, mesh);
}

float clamp_material_alpha(float alpha) {
    if (!std::isfinite(alpha)) return 1.0f;
    return std::clamp(alpha, 0.0f, 1.0f);
}

bool read_u32_advance(const uint8_t* body, size_t size, size_t& pos,
                      uint32_t& out) {
    if (pos + 4 > size) return false;
    std::memcpy(&out, body + pos, sizeof(out));
    pos += 4;
    return true;
}

bool read_f32_advance(const uint8_t* body, size_t size, size_t& pos,
                      float& out) {
    if (pos + 4 > size) return false;
    std::memcpy(&out, body + pos, sizeof(out));
    pos += 4;
    return std::isfinite(out);
}

bool skip_key_rows(size_t row_size, uint32_t count, size_t size, size_t& pos) {
    if (count > 4096) return false;
    const size_t bytes = row_size * static_cast<size_t>(count);
    if (pos + bytes > size) return false;
    pos += bytes;
    return true;
}

float material_anim_tex_value_to_uv(float value) {
    if (!std::isfinite(value)) return 0.0f;
    return value;
}

std::map<std::string, Gameplay::VenueMaterialAnim> load_venue_mat_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, Gameplay::VenueMaterialAnim> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
            if (de.type != "MatAnim" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            if (size < 40) continue;
            uint32_t version = 0;
            std::memcpy(&version, body, sizeof(version));
            if (version != 7) continue;
            size_t pos = 25;
            auto read_string = [&]() -> std::optional<std::string> {
                if (pos + 4 > size) return std::nullopt;
                uint32_t len = 0;
                std::memcpy(&len, body + pos, sizeof(len));
                pos += 4;
                if (len == 0 || len > 96 || pos + len > size)
                    return std::nullopt;
                std::string s(reinterpret_cast<const char*>(body + pos), len);
                pos += len;
                return s;
            };
            auto material = read_string();
            auto anim_name = read_string();
            if (!material || !anim_name) continue;
            Gameplay::VenueMaterialAnim anim;
            anim.name = *anim_name;
            anim.material = *material;
            uint32_t color_count = 0;
            if (!read_u32_advance(body, size, pos, color_count) ||
                !skip_key_rows(20, color_count, size, pos)) {
                continue;
            }
            uint32_t alpha_count = 0;
            if (!read_u32_advance(body, size, pos, alpha_count) ||
                alpha_count > 4096) {
                continue;
            }
            for (uint32_t i = 0; i < alpha_count; ++i) {
                Gameplay::VenueMaterialAnim::FloatKey key;
                if (!read_f32_advance(body, size, pos, key.value) ||
                    !read_f32_advance(body, size, pos, key.frame)) {
                    anim.alpha_keys.clear();
                    break;
                }
                key.value = clamp_material_alpha(key.value);
                anim.alpha_keys.push_back(key);
                anim.duration_frames = std::max(anim.duration_frames, key.frame);
            }
            if (alpha_count != anim.alpha_keys.size()) continue;
            anim.has_alpha = !anim.alpha_keys.empty();
            if (anim.has_alpha) {
                anim.start_alpha = anim.alpha_keys.front().value;
                anim.end_alpha = anim.alpha_keys.back().value;
            }
            uint32_t trans_count = 0;
            if (!read_u32_advance(body, size, pos, trans_count) ||
                trans_count > 4096) {
                continue;
            }
            for (uint32_t i = 0; i < trans_count; ++i) {
                Gameplay::VenueMaterialAnim::Vec3Key key;
                if (!read_f32_advance(body, size, pos, key.value[0]) ||
                    !read_f32_advance(body, size, pos, key.value[1]) ||
                    !read_f32_advance(body, size, pos, key.value[2]) ||
                    !read_f32_advance(body, size, pos, key.frame)) {
                    anim.tex_translation_keys.clear();
                    break;
                }
                anim.tex_translation_keys.push_back(key);
                anim.duration_frames = std::max(anim.duration_frames, key.frame);
            }
            if (trans_count != anim.tex_translation_keys.size()) continue;
            uint32_t scale_count = 0;
            if (!read_u32_advance(body, size, pos, scale_count) ||
                scale_count > 4096) {
                continue;
            }
            for (uint32_t i = 0; i < scale_count; ++i) {
                Gameplay::VenueMaterialAnim::Vec3Key key;
                if (!read_f32_advance(body, size, pos, key.value[0]) ||
                    !read_f32_advance(body, size, pos, key.value[1]) ||
                    !read_f32_advance(body, size, pos, key.value[2]) ||
                    !read_f32_advance(body, size, pos, key.frame)) {
                    anim.tex_scale_keys.clear();
                    break;
                }
                anim.tex_scale_keys.push_back(key);
                anim.duration_frames = std::max(anim.duration_frames, key.frame);
            }
            if (scale_count != anim.tex_scale_keys.size()) continue;
            uint32_t rot_count = 0;
            if (!read_u32_advance(body, size, pos, rot_count) ||
                rot_count > 4096) {
                continue;
            }
            for (uint32_t i = 0; i < rot_count; ++i) {
                Gameplay::VenueMaterialAnim::FloatKey key;
                if (!read_f32_advance(body, size, pos, key.value) ||
                    !read_f32_advance(body, size, pos, key.frame)) {
                    anim.tex_rotation_keys.clear();
                    break;
                }
                anim.tex_rotation_keys.push_back(key);
                anim.duration_frames = std::max(anim.duration_frames, key.frame);
            }
            if (rot_count != anim.tex_rotation_keys.size()) continue;
            if (!anim.has_alpha && anim.tex_translation_keys.empty() &&
                anim.tex_scale_keys.empty() && anim.tex_rotation_keys.empty()) {
                continue;
            }
            out[anim.name] = anim;
            std::fprintf(stderr,
                         "[world] MatAnim %s -> %s alpha %.3f -> %.3f alpha_keys=%zu tex_trans_keys=%zu tex_scale_keys=%zu tex_rot_keys=%zu frames=%.1f\n",
                         anim.name.c_str(), anim.material.c_str(),
                         anim.start_alpha, anim.end_alpha,
                         anim.alpha_keys.size(),
                         anim.tex_translation_keys.size(),
                         anim.tex_scale_keys.size(),
                         anim.tex_rotation_keys.size(),
                         anim.duration_frames);
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] MatAnim load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, Gameplay::VenueEnvironmentAnim> load_venue_env_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, Gameplay::VenueEnvironmentAnim> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
            if (de.type != "EnvAnim" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            if (size < 40) continue;
            uint32_t version = 0;
            std::memcpy(&version, body, sizeof(version));
            if (version != 4) continue;
            size_t pos = 25;
            auto read_string = [&]() -> std::optional<std::string> {
                if (pos + 4 > size) return std::nullopt;
                uint32_t len = 0;
                std::memcpy(&len, body + pos, sizeof(len));
                pos += 4;
                if (len == 0 || len > 96 || pos + len > size)
                    return std::nullopt;
                std::string s(reinterpret_cast<const char*>(body + pos), len);
                pos += len;
                return s;
            };
            auto environment = read_string();
            if (!environment || environment->rfind(".env") == std::string::npos)
                continue;
            uint32_t color_count = 0;
            if (!read_u32_advance(body, size, pos, color_count) ||
                color_count > 4096) {
                continue;
            }
            Gameplay::VenueEnvironmentAnim anim;
            anim.name = canonical_milo_ref(de.name);
            anim.environment = canonical_milo_ref(*environment);
            for (uint32_t i = 0; i < color_count; ++i) {
                Gameplay::VenueEnvironmentAnim::ColorKey key;
                bool ok = true;
                for (int c = 0; c < 4; ++c) {
                    ok = ok && read_f32_advance(body, size, pos, key.color[c]);
                    key.color[c] = std::clamp(key.color[c], 0.0f, 1.0f);
                }
                ok = ok && read_f32_advance(body, size, pos, key.frame);
                if (!ok || key.frame < 0.0f || key.frame > 100000.0f) {
                    anim.color_keys.clear();
                    break;
                }
                anim.color_keys.push_back(key);
                anim.duration_frames = std::max(anim.duration_frames, key.frame);
            }
            if (color_count != anim.color_keys.size() ||
                anim.color_keys.empty()) {
                continue;
            }
            out[anim.name] = anim;
            std::fprintf(stderr,
                         "[world] EnvAnim %s -> %s color_keys=%zu frames=%.1f\n",
                         anim.name.c_str(), anim.environment.c_str(),
                         anim.color_keys.size(), anim.duration_frames);
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] EnvAnim load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, Gameplay::VenueLightAnim> load_venue_light_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, Gameplay::VenueLightAnim> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
            if (de.type != "LightAnim" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            if (size < 40) continue;
            uint32_t version = 0;
            std::memcpy(&version, body, sizeof(version));
            if (version != 2) continue;
            size_t pos = 25;
            auto read_string = [&]() -> std::optional<std::string> {
                if (pos + 4 > size) return std::nullopt;
                uint32_t len = 0;
                std::memcpy(&len, body + pos, sizeof(len));
                pos += 4;
                if (len == 0 || len > 96 || pos + len > size)
                    return std::nullopt;
                std::string s(reinterpret_cast<const char*>(body + pos), len);
                pos += len;
                return s;
            };
            auto light = read_string();
            if (!light || light->rfind(".lit") == std::string::npos)
                continue;
            uint32_t color_count = 0;
            if (!read_u32_advance(body, size, pos, color_count) ||
                color_count > 4096) {
                continue;
            }
            Gameplay::VenueLightAnim anim;
            anim.name = canonical_milo_ref(de.name);
            anim.light = canonical_milo_ref(*light);
            for (uint32_t i = 0; i < color_count; ++i) {
                Gameplay::VenueLightAnim::ColorKey key;
                bool ok = true;
                for (int c = 0; c < 4; ++c) {
                    ok = ok && read_f32_advance(body, size, pos, key.color[c]);
                    key.color[c] = std::clamp(key.color[c], 0.0f, 4.0f);
                }
                ok = ok && read_f32_advance(body, size, pos, key.frame);
                if (!ok || key.frame < 0.0f || key.frame > 10000.0f) {
                    anim.color_keys.clear();
                    break;
                }
                anim.color_keys.push_back(key);
                anim.duration_frames = std::max(anim.duration_frames, key.frame);
            }
            if (color_count != anim.color_keys.size()) continue;
            if (anim.color_keys.empty()) {
                const auto strings = scan_milo_strings(body, size);
                for (const auto& s : strings) {
                    const auto ref = canonical_milo_ref(s);
                    if (ref != anim.name && ref.size() > 4 &&
                        ref.rfind(".lnm") == ref.size() - 4) {
                        anim.keys_owner = ref;
                        break;
                    }
                }
            }
            out[anim.name] = anim;
        }
        for (auto& [name, anim] : out) {
            if (!anim.color_keys.empty() || anim.keys_owner.empty()) continue;
            const auto owner = out.find(anim.keys_owner);
            if (owner == out.end() || owner->second.color_keys.empty()) continue;
            anim.color_keys = owner->second.color_keys;
            anim.duration_frames = owner->second.duration_frames;
        }
        for (const auto& [name, anim] : out) {
            if (anim.color_keys.empty()) continue;
            std::fprintf(stderr,
                         "[world] LightAnim %s -> %s color_keys=%zu frames=%.1f%s%s\n",
                         anim.name.c_str(), anim.light.c_str(),
                         anim.color_keys.size(), anim.duration_frames,
                         anim.keys_owner.empty() ? "" : " keys_owner=",
                         anim.keys_owner.c_str());
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] LightAnim load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, std::vector<std::string>> load_venue_event_light_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, std::vector<std::string>> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        std::map<std::string, std::vector<std::string>> filter_refs;
        std::map<std::string, std::vector<std::string>> group_refs;
        for (const auto& de : dir.entries) {
            if (de.offset + de.size > payload.size()) continue;
            if (de.type != "AnimFilter" && de.type != "Group") continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            auto& refs = de.type == "AnimFilter"
                             ? filter_refs[canonical_milo_ref(de.name)]
                             : group_refs[canonical_milo_ref(de.name)];
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                const bool object_ref =
                    (ref.size() > 4 && ref.rfind(".lnm") == ref.size() - 4) ||
                    (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5) ||
                    (ref.size() > 4 && ref.rfind(".grp") == ref.size() - 4);
                if (object_ref) push_unique_ref(refs, ref);
            }
        }
        auto collect_ref =
            [&](auto&& self, const std::string& ref,
                std::vector<std::string>& routes,
                std::unordered_set<std::string>& seen) -> void {
            if (!seen.insert(ref).second) return;
            if (ref.size() > 4 && ref.rfind(".lnm") == ref.size() - 4) {
                push_unique_ref(routes, ref);
                return;
            }
            const auto& ref_map =
                (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5)
                    ? filter_refs
                    : group_refs;
            const auto refs_it = ref_map.find(ref);
            if (refs_it == ref_map.end()) return;
            for (const auto& child : refs_it->second)
                self(self, child, routes, seen);
        };
        for (const auto& de : dir.entries) {
            if (de.type != "EventTrigger" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            if (strings.empty()) continue;
            const std::string_view event_label =
                is_event_payload_label(strings.front()) ? std::string_view(strings.front())
                                                        : std::string_view{};
            std::vector<std::string> light_anims;
            std::unordered_set<std::string> seen;
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                const bool object_ref =
                    (ref.size() > 4 && ref.rfind(".lnm") == ref.size() - 4) ||
                    (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5) ||
                    (ref.size() > 4 && ref.rfind(".grp") == ref.size() - 4);
                if (object_ref) collect_ref(collect_ref, ref, light_anims, seen);
            }
            if (light_anims.empty()) continue;
            for (const auto& key : event_trigger_route_keys(de.name, event_label)) {
                auto& routed = out[key];
                for (const auto& anim : light_anims) push_unique_ref(routed, anim);
            }
            if (debug_venue_filters_enabled()) {
                std::fprintf(stderr, "[world] EventTrigger %s LightAnims=",
                             de.name.c_str());
                for (const auto& anim : light_anims)
                    std::fprintf(stderr, "%s ", anim.c_str());
                std::fprintf(stderr, "\n");
            }
        }
        if (!out.empty()) {
            std::fprintf(stderr,
                         "[world] venue LightAnim routes loaded %s: %zu events\n",
                         milo_path.c_str(), out.size());
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] EventTrigger LightAnim load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, std::vector<std::string>> load_venue_event_env_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, std::vector<std::string>> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        std::map<std::string, std::vector<std::string>> filter_env_anims;
        std::map<std::string, std::vector<std::string>> filter_group_refs;
        for (const auto& de : dir.entries) {
            if (de.type != "AnimFilter" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            const auto filter_key = canonical_milo_ref(de.name);
            auto& routed = filter_env_anims[filter_key];
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if (ref.size() > 4 && ref.rfind(".enm") == ref.size() - 4) {
                    push_unique_ref(routed, ref);
                } else if (ref.size() > 4 &&
                           ref.rfind(".grp") == ref.size() - 4) {
                    push_unique_ref(filter_group_refs[filter_key], ref);
                }
            }
        }

        std::map<std::string, std::vector<std::string>> group_refs;
        for (const auto& de : dir.entries) {
            if (de.type != "Group" || de.offset + de.size > payload.size())
                continue;
            auto& refs = group_refs[canonical_milo_ref(de.name)];
            const auto strings = scan_milo_strings(
                payload.data() + de.offset, static_cast<size_t>(de.size));
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if ((ref.size() > 4 && ref.rfind(".enm") == ref.size() - 4) ||
                    (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5) ||
                    (ref.size() > 4 && ref.rfind(".grp") == ref.size() - 4)) {
                    push_unique_ref(refs, ref);
                }
            }
        }

        std::map<std::string, std::vector<std::string>> group_env_anims;
        auto collect_group_env_anims =
            [&](auto&& self, const std::string& group,
                std::vector<std::string>& out,
                std::unordered_set<std::string>& seen) -> void {
            if (!seen.insert(group).second) return;
            if (const auto cached = group_env_anims.find(group);
                cached != group_env_anims.end()) {
                for (const auto& anim : cached->second) push_unique_ref(out, anim);
                return;
            }
            std::vector<std::string> resolved;
            const auto refs_it = group_refs.find(group);
            if (refs_it != group_refs.end()) {
                for (const auto& ref : refs_it->second) {
                    if (ref.size() > 4 && ref.rfind(".enm") == ref.size() - 4) {
                        push_unique_ref(resolved, ref);
                    } else if (ref.size() > 5 &&
                               ref.rfind(".filt") == ref.size() - 5) {
                        const auto filter_it = filter_env_anims.find(ref);
                        if (filter_it == filter_env_anims.end()) continue;
                        for (const auto& anim : filter_it->second)
                            push_unique_ref(resolved, anim);
                    } else if (ref.size() > 4 &&
                               ref.rfind(".grp") == ref.size() - 4) {
                        self(self, ref, resolved, seen);
                    }
                }
            }
            group_env_anims[group] = resolved;
            for (const auto& anim : resolved) push_unique_ref(out, anim);
        };
        for (const auto& [group, refs] : group_refs) {
            (void)refs;
            std::vector<std::string> resolved;
            std::unordered_set<std::string> seen;
            collect_group_env_anims(collect_group_env_anims, group, resolved,
                                    seen);
        }
        for (const auto& [filter, groups] : filter_group_refs) {
            auto& routed = filter_env_anims[filter];
            for (const auto& group : groups) {
                const auto group_it = group_env_anims.find(group);
                if (group_it == group_env_anims.end()) continue;
                for (const auto& anim : group_it->second)
                    push_unique_ref(routed, anim);
            }
        }

        for (const auto& de : dir.entries) {
            if (de.type != "EventTrigger" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            if (strings.empty()) continue;
            const std::string_view event_label =
                is_event_payload_label(strings.front()) ? std::string_view(strings.front())
                                                        : std::string_view{};
            std::vector<std::string> env_anims;
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if (ref.size() > 4 && ref.rfind(".enm") == ref.size() - 4) {
                    push_unique_ref(env_anims, ref);
                } else if (ref.size() > 5 &&
                           ref.rfind(".filt") == ref.size() - 5) {
                    const auto filter_it = filter_env_anims.find(ref);
                    if (filter_it == filter_env_anims.end()) continue;
                    for (const auto& anim : filter_it->second)
                        push_unique_ref(env_anims, anim);
                } else if (ref.size() > 4 &&
                           ref.rfind(".grp") == ref.size() - 4) {
                    const auto group_it = group_env_anims.find(ref);
                    if (group_it == group_env_anims.end()) continue;
                    for (const auto& anim : group_it->second)
                        push_unique_ref(env_anims, anim);
                }
            }
            if (!env_anims.empty()) {
                for (const auto& key : event_trigger_route_keys(de.name, event_label)) {
                    auto& routed = out[key];
                    for (const auto& anim : env_anims)
                        push_unique_ref(routed, anim);
                }
                if (debug_venue_filters_enabled()) {
                    std::fprintf(stderr, "[world] EventTrigger %s EnvAnims=",
                                 de.name.c_str());
                    for (const auto& anim : env_anims) {
                        std::fprintf(stderr, "%s ", anim.c_str());
                    }
                    std::fprintf(stderr, "\n");
                }
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] EventTrigger EnvAnim load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

Gameplay::VenueParticleRoute decode_particle_anim_route(
    const std::string& entry_name, const uint8_t* body, size_t size) {
    Gameplay::VenueParticleRoute route;
    route.anim = canonical_milo_ref(entry_name);
    if (size < 32 || read_u32_at_unchecked(body, 0) != 3) return route;

    const auto strings = packed_strings_with_offsets(body, size);
    const PackedStringHit* particle_string = nullptr;
    const PackedStringHit* self_string = nullptr;
    const PackedStringHit* owner_string = nullptr;
    for (const auto& hit : strings) {
        const auto ref = canonical_milo_ref(hit.value);
        if (!particle_string && ref.size() > 5 &&
            ref.rfind(".part") == ref.size() - 5) {
            particle_string = &hit;
        }
        if (ref == route.anim) self_string = &hit;
    }
    for (const auto& hit : strings) {
        const auto ref = canonical_milo_ref(hit.value);
        if (ref != route.anim && ref.size() > 6 &&
            ref.rfind(".panim") == ref.size() - 6) {
            owner_string = &hit;
            break;
        }
    }
    if (particle_string)
        route.particle = canonical_milo_ref(particle_string->value);
    if (owner_string) route.keys_owner = canonical_milo_ref(owner_string->value);
    if (!particle_string) return route;

    const size_t limit = self_string ? self_string->offset : size;
    const size_t count_off = particle_string->end + 8;
    if (count_off + 4 > limit) return route;
    const uint32_t key_count = read_u32_at_unchecked(body, count_off);
    if (key_count == 0 || key_count > 512) return route;
    const uint64_t key_bytes = static_cast<uint64_t>(key_count) * 12ull;
    if (key_bytes > static_cast<uint64_t>(limit - (count_off + 4)))
        return route;

    size_t pos = count_off + 4;
    route.emission_keys.reserve(key_count);
    for (uint32_t i = 0; i < key_count; ++i) {
        Gameplay::VenueParticleRoute::EmissionKey key;
        key.min_value = read_f32_at_unchecked(body, pos);
        key.max_value = read_f32_at_unchecked(body, pos + 4);
        key.frame = read_f32_at_unchecked(body, pos + 8);
        pos += 12;
        if (!std::isfinite(key.min_value) || !std::isfinite(key.max_value) ||
            !std::isfinite(key.frame) || key.frame < 0.0f ||
            key.frame > 100000.0f) {
            route.emission_keys.clear();
            route.duration_frames = 0.0f;
            return route;
        }
        route.duration_frames = std::max(route.duration_frames, key.frame);
        route.emission_keys.push_back(key);
    }
    std::sort(route.emission_keys.begin(), route.emission_keys.end(),
              [](const auto& a, const auto& b) {
                  return a.frame < b.frame;
              });
    return route;
}

std::map<std::string, std::vector<Gameplay::VenueParticleRoute>>
load_venue_event_particles(const std::string& hdr_path,
                           const std::string& ark_path,
                           const std::string& milo_path) {
    std::map<std::string, std::vector<Gameplay::VenueParticleRoute>> out;
    auto push_route =
        [](std::vector<Gameplay::VenueParticleRoute>& routes,
           Gameplay::VenueParticleRoute route) {
            if (route.particle.empty()) return;
            for (auto& existing : routes) {
                if (existing.particle != route.particle) continue;
                existing.duration_frames =
                    std::max(existing.duration_frames, route.duration_frames);
                if (existing.emission_keys.size() < route.emission_keys.size()) {
                    existing.anim = route.anim;
                    existing.keys_owner = route.keys_owner;
                    existing.emission_keys = route.emission_keys;
                }
                return;
            }
            routes.push_back(std::move(route));
        };
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        std::unordered_set<std::string> particle_names;
        std::map<std::string, Gameplay::VenueParticleRoute> panim_routes;
        std::map<std::string, std::vector<std::string>> filter_refs;
        std::map<std::string, std::vector<std::string>> group_refs;
        for (const auto& de : dir.entries) {
            if (de.offset + de.size > payload.size()) continue;
            const auto* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            if (de.type == "ParticleSys") {
                particle_names.insert(canonical_milo_ref(de.name));
            } else if (de.type == "ParticleSysAnim") {
                auto route = decode_particle_anim_route(de.name, body, size);
                if (!route.anim.empty() && !route.particle.empty())
                    panim_routes[route.anim] = std::move(route);
            } else if (de.type == "AnimFilter" || de.type == "Group") {
                auto& refs = de.type == "AnimFilter"
                                 ? filter_refs[canonical_milo_ref(de.name)]
                                 : group_refs[canonical_milo_ref(de.name)];
                const auto strings = scan_milo_strings(body, size);
                for (const auto& s : strings) {
                    const auto ref = canonical_milo_ref(s);
                    const bool object_ref =
                        (ref.size() > 5 &&
                         ref.rfind(".part") == ref.size() - 5) ||
                        (ref.size() > 6 &&
                         ref.rfind(".panim") == ref.size() - 6) ||
                        (ref.size() > 5 &&
                         ref.rfind(".filt") == ref.size() - 5) ||
                        (ref.size() > 4 &&
                         ref.rfind(".grp") == ref.size() - 4);
                    if (object_ref) push_unique_ref(refs, ref);
                }
            }
        }
        for (auto& [name, route] : panim_routes) {
            if (!route.emission_keys.empty() || route.keys_owner.empty())
                continue;
            const auto owner = panim_routes.find(route.keys_owner);
            if (owner == panim_routes.end() ||
                owner->second.emission_keys.empty())
                continue;
            route.emission_keys = owner->second.emission_keys;
            route.duration_frames = owner->second.duration_frames;
        }

        auto collect_ref =
            [&](auto&& self, const std::string& ref,
                std::vector<Gameplay::VenueParticleRoute>& routes,
                std::unordered_set<std::string>& seen) -> void {
            if (!seen.insert(ref).second) return;
            if (ref.size() > 5 && ref.rfind(".part") == ref.size() - 5) {
                if (particle_names.find(ref) != particle_names.end()) {
                    Gameplay::VenueParticleRoute route;
                    route.particle = ref;
                    push_route(routes, std::move(route));
                }
                return;
            }
            if (ref.size() > 6 && ref.rfind(".panim") == ref.size() - 6) {
                const auto it = panim_routes.find(ref);
                if (it != panim_routes.end()) push_route(routes, it->second);
                return;
            }
            const auto& ref_map =
                (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5)
                    ? filter_refs
                    : group_refs;
            const auto refs_it = ref_map.find(ref);
            if (refs_it == ref_map.end()) return;
            for (const auto& child : refs_it->second)
                self(self, child, routes, seen);
        };

        for (const auto& de : dir.entries) {
            if (de.type != "EventTrigger" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            if (strings.empty()) continue;
            const std::string_view event_label =
                is_event_payload_label(strings.front()) ? std::string_view(strings.front())
                                                        : std::string_view{};
            std::vector<Gameplay::VenueParticleRoute> routes;
            std::unordered_set<std::string> seen;
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                const bool object_ref =
                    (ref.size() > 5 && ref.rfind(".part") == ref.size() - 5) ||
                    (ref.size() > 6 && ref.rfind(".panim") == ref.size() - 6) ||
                    (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5) ||
                    (ref.size() > 4 && ref.rfind(".grp") == ref.size() - 4);
                if (object_ref) collect_ref(collect_ref, ref, routes, seen);
            }
            if (routes.empty()) continue;
            for (const auto& key : event_trigger_route_keys(de.name, event_label)) {
                auto& routed = out[key];
                for (const auto& route : routes) push_route(routed, route);
            }
            if (debug_venue_filters_enabled()) {
                std::fprintf(stderr, "[world] EventTrigger %s Particles=",
                             de.name.c_str());
                for (const auto& route : routes) {
                    std::fprintf(stderr, "%s via %s keys=%zu frames=%.1f ",
                                 route.particle.c_str(), route.anim.c_str(),
                                 route.emission_keys.size(),
                                 route.duration_frames);
                }
                std::fprintf(stderr, "\n");
            }
        }
        if (!out.empty()) {
            std::fprintf(stderr,
                         "[world] venue ParticleSys routes loaded %s: %zu events\n",
                         milo_path.c_str(), out.size());
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] EventTrigger ParticleSys load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, std::vector<std::string>> load_venue_event_mat_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, std::vector<std::string>> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        std::map<std::string, std::vector<std::string>> filter_mat_anims;
        std::map<std::string, std::vector<std::string>> filter_group_refs;
        for (const auto& de : dir.entries) {
            if (de.type != "AnimFilter" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            const auto filter_key = canonical_milo_ref(de.name);
            auto& routed = filter_mat_anims[filter_key];
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if (ref.size() > 4 && ref.rfind(".mnm") == ref.size() - 4) {
                    push_unique_ref(routed, ref);
                } else if (ref.size() > 4 &&
                           ref.rfind(".grp") == ref.size() - 4) {
                    push_unique_ref(filter_group_refs[filter_key], ref);
                }
            }
        }

        std::map<std::string, std::vector<std::string>> group_refs;
        for (const auto& de : dir.entries) {
            if (de.type != "Group" || de.offset + de.size > payload.size())
                continue;
            auto& refs = group_refs[canonical_milo_ref(de.name)];
            const auto strings = scan_milo_strings(
                payload.data() + de.offset, static_cast<size_t>(de.size));
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if ((ref.size() > 4 && ref.rfind(".mnm") == ref.size() - 4) ||
                    (ref.size() > 5 && ref.rfind(".filt") == ref.size() - 5) ||
                    (ref.size() > 4 && ref.rfind(".grp") == ref.size() - 4)) {
                    push_unique_ref(refs, ref);
                }
            }
        }

        std::map<std::string, std::vector<std::string>> group_mat_anims;
        auto collect_group_mat_anims =
            [&](auto&& self, const std::string& group,
                std::vector<std::string>& out,
                std::unordered_set<std::string>& seen) -> void {
            if (!seen.insert(group).second) return;
            if (const auto cached = group_mat_anims.find(group);
                cached != group_mat_anims.end()) {
                for (const auto& anim : cached->second) push_unique_ref(out, anim);
                return;
            }
            std::vector<std::string> resolved;
            const auto refs_it = group_refs.find(group);
            if (refs_it != group_refs.end()) {
                for (const auto& ref : refs_it->second) {
                    if (ref.size() > 4 && ref.rfind(".mnm") == ref.size() - 4) {
                        push_unique_ref(resolved, ref);
                    } else if (ref.size() > 5 &&
                               ref.rfind(".filt") == ref.size() - 5) {
                        const auto filter_it = filter_mat_anims.find(ref);
                        if (filter_it == filter_mat_anims.end()) continue;
                        for (const auto& anim : filter_it->second)
                            push_unique_ref(resolved, anim);
                    } else if (ref.size() > 4 &&
                               ref.rfind(".grp") == ref.size() - 4) {
                        self(self, ref, resolved, seen);
                    }
                }
            }
            group_mat_anims[group] = resolved;
            for (const auto& anim : resolved) push_unique_ref(out, anim);
        };
        for (const auto& [group, refs] : group_refs) {
            (void)refs;
            std::vector<std::string> resolved;
            std::unordered_set<std::string> seen;
            collect_group_mat_anims(collect_group_mat_anims, group, resolved,
                                    seen);
        }
        for (const auto& [filter, groups] : filter_group_refs) {
            auto& routed = filter_mat_anims[filter];
            for (const auto& group : groups) {
                const auto group_it = group_mat_anims.find(group);
                if (group_it == group_mat_anims.end()) continue;
                for (const auto& anim : group_it->second)
                    push_unique_ref(routed, anim);
            }
        }

        for (const auto& de : dir.entries) {
            if (de.type != "EventTrigger" || de.offset + de.size > payload.size())
                continue;
            const auto* body = payload.data() + de.offset;
            const auto strings =
                scan_milo_strings(body, static_cast<size_t>(de.size));
            if (strings.empty()) continue;
            const std::string_view event_label =
                is_event_payload_label(strings.front()) ? std::string_view(strings.front())
                                                        : std::string_view{};
            std::vector<std::string> mat_anims;
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if (ref.size() > 4 && ref.rfind(".mnm") == ref.size() - 4) {
                    push_unique_ref(mat_anims, ref);
                } else if (ref.size() > 5 &&
                           ref.rfind(".filt") == ref.size() - 5) {
                    const auto filter_it = filter_mat_anims.find(ref);
                    if (filter_it == filter_mat_anims.end()) continue;
                    for (const auto& anim : filter_it->second)
                        push_unique_ref(mat_anims, anim);
                } else if (ref.size() > 4 &&
                           ref.rfind(".grp") == ref.size() - 4) {
                    const auto group_it = group_mat_anims.find(ref);
                    if (group_it == group_mat_anims.end()) continue;
                    for (const auto& anim : group_it->second)
                        push_unique_ref(mat_anims, anim);
                }
            }
            if (!mat_anims.empty()) {
                for (const auto& key : event_trigger_route_keys(de.name, event_label)) {
                    auto& routed = out[key];
                    for (const auto& anim : mat_anims)
                        push_unique_ref(routed, anim);
                }
                if (debug_venue_filters_enabled()) {
                    std::fprintf(stderr, "[world] EventTrigger %s MatAnims=",
                                 de.name.c_str());
                    for (const auto& anim : mat_anims) {
                        std::fprintf(stderr, "%s ", anim.c_str());
                    }
                    std::fprintf(stderr, "\n");
                }
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] EventTrigger MatAnim load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, std::vector<std::string>> load_venue_event_filters(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
    std::map<std::string, std::vector<std::string>> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
            if (de.type != "EventTrigger" || de.offset + de.size > payload.size())
                continue;
            const auto strings = scan_milo_strings(
                payload.data() + de.offset, static_cast<size_t>(de.size));
            if (strings.empty()) continue;
            std::vector<std::string> filters;
            for (size_t i = 1; i < strings.size(); ++i) {
                if (strings[i].rfind(".filt") == std::string::npos) continue;
                const auto ref = canonical_milo_ref(strings[i]);
                if (std::find(filters.begin(), filters.end(), ref) ==
                    filters.end()) {
                    filters.push_back(ref);
                }
            }
            if (!filters.empty()) {
                const std::string_view event_label =
                    is_event_payload_label(strings.front())
                        ? std::string_view(strings.front())
                        : std::string_view{};
                for (const auto& key :
                     event_trigger_route_keys(de.name, event_label)) {
                    auto& routed = out[key];
                    for (const auto& filter : filters)
                        push_unique_ref(routed, filter);
                }
            }
        }
        if (!out.empty()) {
            std::fprintf(stderr, "[world] EventTrigger filters loaded %s: %zu events\n",
                         milo_path.c_str(), out.size());
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] EventTrigger filter load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, std::vector<std::string>> load_venue_filter_mesh_targets(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path,
    const ghogx::milo_scene::Scene& scene) {
    std::map<std::string, std::vector<std::string>> out;
    std::map<std::string, std::vector<std::string>> group_children;
    for (const auto& group : scene.groups) group_children[group.name] = group.children;
    std::map<std::string, std::string> trans_anim_targets;

    auto add_group_meshes = [&](auto&& self, const std::string& group_name,
                                std::vector<std::string>& meshes,
                                std::unordered_set<std::string>& seen_groups) -> void {
        if (!seen_groups.insert(group_name).second) return;
        const auto group_it = group_children.find(group_name);
        if (group_it == group_children.end()) return;
        for (const auto& child : group_it->second) {
            const auto ref = canonical_milo_ref(child);
            if (ref.rfind(".mesh") != std::string::npos) {
                if (std::find(meshes.begin(), meshes.end(), ref) == meshes.end())
                    meshes.push_back(ref);
            } else if (ref.rfind(".grp") != std::string::npos) {
                self(self, ref, meshes, seen_groups);
            } else if (ref.rfind(".tnm") != std::string::npos) {
                const auto anim_it = trans_anim_targets.find(ref);
                if (anim_it != trans_anim_targets.end() &&
                    std::find(meshes.begin(), meshes.end(), anim_it->second) ==
                        meshes.end()) {
                    meshes.push_back(anim_it->second);
                }
            }
        }
    };

    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
            if (de.type != "Group" || de.offset + de.size > payload.size())
                continue;
            const auto strings = scan_milo_strings(
                payload.data() + de.offset, static_cast<size_t>(de.size));
            auto& children = group_children[de.name];
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                const bool object_ref =
                    ref.rfind(".mesh") != std::string::npos ||
                    ref.rfind(".grp") != std::string::npos ||
                    ref.rfind(".tnm") != std::string::npos;
                if (!object_ref) continue;
                if (std::find(children.begin(), children.end(), ref) ==
                    children.end()) {
                    children.push_back(ref);
                }
            }
        }
        for (const auto& de : dir.entries) {
            if (de.type != "TransAnim" || de.offset + de.size > payload.size())
                continue;
            const auto strings = scan_milo_strings(
                payload.data() + de.offset, static_cast<size_t>(de.size));
            for (const auto& s : strings) {
                if (s.rfind(".mesh") != std::string::npos) {
                    trans_anim_targets[de.name] = canonical_milo_ref(s);
                    break;
                }
            }
        }
        for (const auto& de : dir.entries) {
            if (de.type != "AnimFilter" || de.offset + de.size > payload.size())
                continue;
            const auto strings = scan_milo_strings(
                payload.data() + de.offset, static_cast<size_t>(de.size));
            std::vector<std::string> meshes;
            for (const auto& s : strings) {
                const auto ref = canonical_milo_ref(s);
                if (ref.rfind(".mesh") != std::string::npos) {
                    if (std::find(meshes.begin(), meshes.end(), ref) == meshes.end())
                        meshes.push_back(ref);
                } else if (ref.rfind(".grp") != std::string::npos) {
                    std::unordered_set<std::string> seen_groups;
                    add_group_meshes(add_group_meshes, ref, meshes, seen_groups);
                } else if (ref.rfind(".tnm") != std::string::npos) {
                    const auto anim_it = trans_anim_targets.find(ref);
                    if (anim_it != trans_anim_targets.end() &&
                        std::find(meshes.begin(), meshes.end(), anim_it->second) ==
                            meshes.end()) {
                        meshes.push_back(anim_it->second);
                    }
                }
            }
            if (!meshes.empty()) out[de.name] = std::move(meshes);
        }
        if (!out.empty()) {
            size_t targets = 0;
            for (const auto& [_, meshes] : out) targets += meshes.size();
            std::fprintf(stderr,
                         "[world] AnimFilter mesh targets loaded %s: %zu filters %zu meshes\n",
                         milo_path.c_str(), out.size(), targets);
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] AnimFilter mesh load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::string normalize_milo_path_game(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    const std::string system_prefix = "../../system/run/";
    if (path.rfind(system_prefix, 0) == 0) path.erase(0, system_prefix.size());
    std::vector<std::string> parts;
    size_t i = 0;
    while (i <= path.size()) {
        size_t j = path.find('/', i);
        if (j == std::string::npos) j = path.size();
        std::string part = path.substr(i, j - i);
        if (part.empty() || part == ".") {
            // skip
        } else if (part == "..") {
            if (!parts.empty()) parts.pop_back();
        } else {
            parts.push_back(std::move(part));
        }
        if (j == path.size()) break;
        i = j + 1;
    }
    std::string out;
    for (size_t k = 0; k < parts.size(); ++k) {
        if (k) out += '/';
        out += parts[k];
    }
    return out;
}

std::string replace_suffix_game(std::string path, std::string_view from,
                                std::string_view to) {
    if (path.size() >= from.size() &&
        path.compare(path.size() - from.size(), from.size(), from) == 0) {
        path.resize(path.size() - from.size());
        path += to;
    }
    return path;
}

std::string insert_gen_dir_for_anim_game(std::string path) {
    path = normalize_milo_path_game(std::move(path));
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return path;
    const std::string dir = path.substr(0, slash);
    if (dir.size() >= 4 && dir.compare(dir.size() - 4, 4, "/gen") == 0)
        return path;
    return dir + "/gen" + path.substr(slash);
}

std::vector<std::string> driver_milo_candidates_game(
    const std::string& character_milo, const std::string& driver_milo) {
    std::vector<std::string> out;
    auto add = [&](std::string p) {
        if (p.empty()) return;
        p = normalize_milo_path_game(std::move(p));
        if (std::find(out.begin(), out.end(), p) == out.end())
            out.push_back(std::move(p));
    };

    std::string char_dir = normalize_milo_path_game(character_milo);
    const size_t slash = char_dir.find_last_of('/');
    char_dir = slash == std::string::npos ? std::string()
                                          : char_dir.substr(0, slash);

    std::string rel = driver_milo;
    std::replace(rel.begin(), rel.end(), '\\', '/');
    const std::string resolved = normalize_milo_path_game(
        (!char_dir.empty() && rel.rfind("/", 0) != 0 &&
         rel.find(':') == std::string::npos)
            ? char_dir + "/" + rel
            : rel);
    add(resolved);
    add(replace_suffix_game(resolved, ".milo", ".milo_ps2"));
    add(insert_gen_dir_for_anim_game(resolved));
    add(insert_gen_dir_for_anim_game(
        replace_suffix_game(resolved, ".milo", ".milo_ps2")));
    return out;
}

std::vector<std::string> facefx_viseme_milo_candidates_game(
    const std::string& character_milo,
    const std::vector<ghogx::character::FaceFxLipSyncServo>& servos) {
    std::vector<std::string> out;
    auto add = [&](std::string p) {
        if (p.empty()) return;
        p = normalize_milo_path_game(std::move(p));
        if (std::find(out.begin(), out.end(), p) == out.end())
            out.push_back(std::move(p));
    };

    std::string char_dir = normalize_milo_path_game(character_milo);
    const size_t slash = char_dir.find_last_of('/');
    char_dir = slash == std::string::npos ? std::string()
                                          : char_dir.substr(0, slash);

    for (const auto& servo : servos) {
        if (servo.viseme_milo.empty()) continue;
        std::string rel = servo.viseme_milo;
        std::replace(rel.begin(), rel.end(), '\\', '/');
        const std::string resolved = normalize_milo_path_game(
            (!char_dir.empty() && rel.rfind("/", 0) != 0 &&
             rel.find(':') == std::string::npos)
                ? char_dir + "/" + rel
                : rel);
        add(resolved);
        add(replace_suffix_game(resolved, ".milo", ".milo_ps2"));
        add(insert_gen_dir_for_anim_game(resolved));
        add(insert_gen_dir_for_anim_game(
            replace_suffix_game(resolved, ".milo", ".milo_ps2")));
    }
    return out;
}

bool is_lighting_category(std::string_view s) {
    static const std::unordered_set<std::string_view> kCategories = {
        "INTRO",
        "INTRO_ENCORE",
        "WIN",
        "WIN_ENCORE",
        "WIN_GAME",
        "LOSE",
        "VERSE",
        "VERSE_ENCORE",
        "CHORUS",
        "CHORUS_ENCORE",
        "VERSECHORUS",
        "VERSECHORUS_ENCORE",
        "SOLO",
        "SOLO_ENCORE",
        "VERSECHORUSSOLO",
        "VERSECHORUSSOLO_ENCORE",
    };
    return kCategories.find(s) != kCategories.end();
}

bool is_lighting_adjective(std::string_view s) {
    static const std::unordered_set<std::string_view> kAdjectives = {
        "blackout", "strobe", "flare", "color1", "color2", "sweep",
    };
    return kAdjectives.find(s) != kAdjectives.end();
}

bool is_lighting_object_ref(std::string_view s) {
    return s.rfind(".mesh") != std::string::npos ||
           s.rfind(".spot") != std::string::npos ||
           s.rfind(".env") != std::string::npos ||
           s.rfind(".mat") != std::string::npos ||
           s.rfind(".lit") != std::string::npos ||
           s == "guitarist0" || s == "bone_pelvis.mesh";
}

std::string lower_ascii(std::string_view s) {
    std::string out(s);
    for (char& c : out) {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

std::optional<std::string> strip_spotlight_target_mesh_suffix(
    std::string_view target) {
    constexpr size_t kSuffixLength = 12;
    if (target.size() <= kSuffixLength) return std::nullopt;
    const std::string suffix =
        lower_ascii(target.substr(target.size() - kSuffixLength));
    if (suffix != "_target.mesh" && suffix != ".target.mesh")
        return std::nullopt;
    return std::string(target.substr(0, target.size() - kSuffixLength));
}

bool is_spotlight_target_mesh(std::string_view target) {
    return strip_spotlight_target_mesh_suffix(target).has_value();
}

bool is_lighting_keyframe_label(std::string_view s) {
    if (s.size() < 2 || s.size() > 48) return false;
    if (is_lighting_category(s) || is_lighting_adjective(s) ||
        is_lighting_object_ref(s)) {
        return false;
    }
    bool has_alpha = false;
    for (const char c : s) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
                        c == '-';
        if (!ok) return false;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            has_alpha = true;
    }
    return has_alpha;
}

float read_f32_at(const uint8_t* body, size_t size, size_t off, float fallback) {
    if (off + 4 > size) return fallback;
    float v = fallback;
    std::memcpy(&v, body + off, sizeof(v));
    if (!std::isfinite(v)) return fallback;
    return v;
}

bool plausible_lighting_frame_count(float frames) {
    // Accepted dumps show LightPreset timing as authored frame counts. Packed
    // string/table bytes can also sit after a keyframe label; reject those
    // instead of letting them become multi-century fades.
    constexpr float kMaxObservedLightPresetFrames = 920.0f;
    constexpr float kConservativeSlack = 2.0f;
    return std::isfinite(frames) && frames >= 0.0f &&
           frames <= kMaxObservedLightPresetFrames * kConservativeSlack;
}

float read_light_preset_timing_f32(const uint8_t* body, size_t size,
                                   size_t off) {
    const float v = read_f32_at(body, size, off, 0.0f);
    return plausible_lighting_frame_count(v) ? v : 0.0f;
}

void add_unique_lighting_ref(std::vector<std::string>& refs,
                             std::string value) {
    if (std::find(refs.begin(), refs.end(), value) == refs.end())
        refs.push_back(std::move(value));
}

void collect_lighting_object_refs(const std::vector<std::string>& strings,
                                  std::vector<std::string>& spot_refs,
                                  std::vector<std::string>& env_refs,
                                  std::vector<std::string>& lit_refs) {
    for (auto s : strings) {
        if (s.rfind(".spot") != std::string::npos) {
            add_unique_lighting_ref(spot_refs, std::move(s));
        } else if (s.rfind(".env") != std::string::npos) {
            add_unique_lighting_ref(env_refs, std::move(s));
        } else if (s.rfind(".lit") != std::string::npos) {
            add_unique_lighting_ref(lit_refs, std::move(s));
        }
    }
}

std::vector<std::string> extract_lighting_keyframe_labels(
    const uint8_t* body, size_t size, uint32_t count,
    std::vector<size_t>* label_offsets) {
    constexpr size_t kLightPresetHeaderBytes = 0x1C;
    struct Candidate {
        size_t off = 0;
        size_t len = 0;
        std::string label;
    };
    std::vector<Candidate> candidates;
    for (size_t i = kLightPresetHeaderBytes; i + 4 <= size; ++i) {
        uint32_t len = 0;
        std::memcpy(&len, body + i, sizeof(len));
        if (len == 0 || len > 64 || i + 4 + len > size) continue;
        std::string s(reinterpret_cast<const char*>(body + i + 4), len);
        if (!is_lighting_keyframe_label(s)) continue;
        candidates.push_back({i, len, std::move(s)});
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const Candidate& a, const Candidate& b) {
                         return a.off < b.off;
                     });
    std::vector<Candidate> filtered;
    for (const auto& c : candidates) {
        bool shadowed = false;
        for (const auto& other : candidates) {
            if (&c == &other) continue;
            const bool overlaps =
                c.off < other.off + 4 + other.len &&
                other.off < c.off + 4 + c.len;
            const bool same_span = std::abs(static_cast<long long>(c.off) -
                                            static_cast<long long>(other.off)) <= 8;
            if ((overlaps || same_span) && other.label.size() > c.label.size() &&
                (other.label.find(c.label) != std::string::npos ||
                 c.label.find(other.label) != std::string::npos ||
                 same_span)) {
                shadowed = true;
                break;
            }
        }
        if (!shadowed) filtered.push_back(c);
    }
    std::vector<std::string> labels;
    if (label_offsets) label_offsets->clear();
    for (const auto& c : filtered) {
        if (labels.empty() || labels.back() != c.label) {
            labels.push_back(c.label);
            if (label_offsets) label_offsets->push_back(c.off);
        }
    }
    if (labels.size() > count) {
        labels.resize(count);
        if (label_offsets) label_offsets->resize(count);
    }
    return labels;
}

std::vector<Gameplay::LightingPreset::Keyframe> extract_lighting_keyframes(
    const uint8_t* body, size_t size, const std::vector<std::string>& labels,
    const std::vector<size_t>& label_offsets) {
    std::vector<Gameplay::LightingPreset::Keyframe> out;
    size_t record_start = 0;
    for (size_t i = 0; i < labels.size() && i < label_offsets.size(); ++i) {
        const size_t label_off = label_offsets[i];
        const size_t label_end = std::min(size, label_off + 4 + labels[i].size());
        if (label_off >= size || record_start >= label_end) continue;
        Gameplay::LightingPreset::Keyframe k;
        k.name = labels[i];
        k.record_start = record_start;
        k.record_end = label_end;
        k.label_offset = label_off;
        // LightPreset keyframe editor schema stores description, duration,
        // then fade_out, but packed target/object data can follow labels in
        // single-frame records. Keep only plausible authored frame counts.
        k.duration = read_light_preset_timing_f32(body, size, label_end);
        k.fade_out = read_light_preset_timing_f32(body, size, label_end + 4);

        for (size_t pos = record_start; pos + 4 <= label_off; ++pos) {
            uint32_t len = 0;
            std::memcpy(&len, body + pos, sizeof(len));
            if (len == 0 || len > 96 || pos + 4 + len > label_off) continue;
            std::string s(reinterpret_cast<const char*>(body + pos + 4), len);
            const auto add_unique = [](std::vector<std::string>& refs,
                                       std::string value) {
                if (std::find(refs.begin(), refs.end(), value) == refs.end())
                    refs.push_back(std::move(value));
            };
            if (s.rfind(".mesh") != std::string::npos) {
                const std::string target = s;
                add_unique(k.mesh_targets, std::move(s));
                if (is_spotlight_target_mesh(target) &&
                    pos + 4 + len + 41 <= label_off) {
                    const size_t payload = pos + 4 + len;
                    Gameplay::LightingPreset::TargetState state;
                    state.target = target;
                    // LightPreset target rows are packed: string, nine bytes of
                    // row metadata, then unaligned floats. The first float is the
                    // light amount; the final vec3 is RGB.
                    state.intensity = std::clamp(
                        read_f32_at(body, size, payload + 9, 0.0f), 0.0f, 4.0f);
                    state.color[0] = std::clamp(
                        read_f32_at(body, size, payload + 29, 1.0f), 0.0f, 4.0f);
                    state.color[1] = std::clamp(
                        read_f32_at(body, size, payload + 33, 1.0f), 0.0f, 4.0f);
                    state.color[2] = std::clamp(
                        read_f32_at(body, size, payload + 37, 1.0f), 0.0f, 4.0f);
                    k.target_states.push_back(std::move(state));
                }
            } else if (s.rfind(".spot") != std::string::npos) {
                add_unique_lighting_ref(k.spot_refs, std::move(s));
            } else if (s.rfind(".env") != std::string::npos) {
                add_unique_lighting_ref(k.env_refs, std::move(s));
            } else if (s.rfind(".lit") != std::string::npos) {
                add_unique_lighting_ref(k.lit_refs, std::move(s));
            }
        }
        out.push_back(std::move(k));
        record_start = label_end;
    }
    return out;
}

std::vector<Gameplay::LightingPreset> load_lighting_presets(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& venue) {
    std::vector<Gameplay::LightingPreset> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        const std::string milo_path =
            "world/" + venue + "/og/gen/" + venue + "_lighting.milo_ps2";
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
            if (de.type != "LightPreset" ||
                de.offset + de.size > payload.size() || de.size < 0x1C) {
                continue;
            }
            const uint8_t* body = payload.data() + de.offset;
            Gameplay::LightingPreset p;
            p.name = de.name;
            p.keyframe_count = body[0x19];
            if (de.size >= 10) {
                std::memcpy(&p.min_excitement,
                            body + static_cast<size_t>(de.size) - 10,
                            sizeof(p.min_excitement));
                std::memcpy(&p.max_excitement,
                            body + static_cast<size_t>(de.size) - 6,
                            sizeof(p.max_excitement));
            }
            auto strings = scan_milo_strings(body, static_cast<size_t>(de.size));
            for (const auto& s : strings) {
                if (is_lighting_category(s)) p.category = s;
                if (is_lighting_adjective(s)) p.adjective = s;
            }
            collect_lighting_object_refs(strings, p.spot_refs, p.env_refs,
                                         p.lit_refs);
            p.keyframe_names = extract_lighting_keyframe_labels(
                body, static_cast<size_t>(de.size), p.keyframe_count,
                &p.keyframe_label_offsets);
            p.keyframes = extract_lighting_keyframes(
                body, static_cast<size_t>(de.size), p.keyframe_names,
                p.keyframe_label_offsets);
            out.push_back(std::move(p));
        }
        std::fprintf(stderr, "[world] lighting presets decoded: %zu\n",
                     out.size());
        for (const auto& p : out) {
            std::fprintf(stderr,
                         "[world]   LightPreset %s category=%s adjective=%s keyframes=%u excitement=%u..%u preset_refs=%zu/%zu/%zu",
                         p.name.c_str(), p.category.c_str(),
                         p.adjective.c_str(), p.keyframe_count,
                         p.min_excitement, p.max_excitement,
                         p.spot_refs.size(), p.env_refs.size(),
                         p.lit_refs.size());
            if (!p.keyframe_names.empty()) {
                std::string labels;
                for (size_t i = 0; i < p.keyframe_names.size(); ++i) {
                    if (i) labels += ",";
                    labels += p.keyframe_names[i];
                }
                std::fprintf(stderr, " labels=%s", labels.c_str());
            }
            if (!p.keyframe_label_offsets.empty()) {
                std::fprintf(stderr, " label_offsets=");
                for (size_t i = 0; i < p.keyframe_label_offsets.size(); ++i) {
                    std::fprintf(stderr, "%s0x%zx", i ? "," : "",
                                 p.keyframe_label_offsets[i]);
                }
            }
            std::fprintf(stderr, "\n");
            for (const auto& k : p.keyframes) {
                std::fprintf(stderr,
                             "[world]     keyframe '%s' span=0x%zx..0x%zx duration=%.3f fade=%.3f targets=%zu spots=%zu env=%zu lit=%zu\n",
                             k.name.c_str(), k.record_start, k.record_end,
                             k.duration, k.fade_out,
                             k.mesh_targets.size(), k.spot_refs.size(),
                             k.env_refs.size(), k.lit_refs.size());
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] lighting preset decode: %s\n", ex.what());
    }
    return out;
}

void log_lighting_light_object_coverage(
    const ghogx::milo_scene::Scene& lighting_scene,
    const std::vector<Gameplay::LightingPreset>& presets,
    const std::map<std::string, ghogx::milo_scene::LightObj>& venue_lights,
    const std::map<std::string, ghogx::milo_scene::EnvironObj>& venue_environs) {
    std::map<std::string, const ghogx::milo_scene::LightObj*> lights_by_name;
    size_t failed_lights = 0;
    for (const auto& light : lighting_scene.lights) {
        if (!light.decoded) {
            ++failed_lights;
            std::fprintf(stderr,
                         "[world] lighting Light object decode failed: %s %s\n",
                         light.name.c_str(), light.error.c_str());
            continue;
        }
        lights_by_name[light.name] = &light;
        std::fprintf(
            stderr,
            "[world] lighting Light object decoded: %s type=%d anim_color=%d anim_pos=%d pos=(%.3f %.3f %.3f) color=(%.3f %.3f %.3f %.3f) range=%.3f\n",
            light.name.c_str(), light.type,
            light.animate_color_from_preset ? 1 : 0,
            light.animate_position_from_preset ? 1 : 0,
            light.world_stored.pos[0], light.world_stored.pos[1],
            light.world_stored.pos[2],
            light.color[0], light.color[1], light.color[2], light.color[3],
            light.range);
    }
    std::map<std::string, const ghogx::milo_scene::EnvironObj*> environs_by_name;
    size_t failed_environs = 0;
    for (const auto& env : lighting_scene.environs) {
        if (!env.decoded) {
            ++failed_environs;
            std::fprintf(stderr,
                         "[world] lighting Environ object decode failed: %s %s\n",
                         env.name.c_str(), env.error.c_str());
            continue;
        }
        environs_by_name[env.name] = &env;
        std::fprintf(
            stderr,
            "[world] lighting Environ object decoded: %s lights=%zu color_a=(%.3f %.3f %.3f %.3f) ranges=(%.3f %.3f %.3f) color_b=(%.3f %.3f %.3f %.3f)\n",
            env.name.c_str(), env.lights.size(), env.color_a[0], env.color_a[1],
            env.color_a[2], env.color_a[3], env.range_a, env.range_b,
            env.range, env.color_b[0], env.color_b[1], env.color_b[2],
            env.color_b[3]);
    }

    std::map<std::string, bool> lit_refs;
    std::map<std::string, bool> env_refs;
    auto add_lit_ref = [&](const std::string& ref) {
        if (!ref.empty()) lit_refs[ref] = true;
    };
    auto add_env_ref = [&](const std::string& ref) {
        if (!ref.empty()) env_refs[ref] = true;
    };
    for (const auto& preset : presets) {
        for (const auto& ref : preset.lit_refs) add_lit_ref(ref);
        for (const auto& ref : preset.env_refs) add_env_ref(ref);
        for (const auto& keyframe : preset.keyframes) {
            for (const auto& ref : keyframe.lit_refs) add_lit_ref(ref);
            for (const auto& ref : keyframe.env_refs) add_env_ref(ref);
        }
    }

    size_t matched_refs = 0;
    size_t matched_venue_refs = 0;
    size_t unmatched_refs = 0;
    size_t unmatched_logged = 0;
    for (const auto& [ref, unused] : lit_refs) {
        (void)unused;
        if (lights_by_name.find(ref) != lights_by_name.end()) {
            ++matched_refs;
            continue;
        }
        if (venue_lights.find(ref) != venue_lights.end()) {
            ++matched_venue_refs;
            continue;
        }
        ++unmatched_refs;
        if (unmatched_logged < 24) {
            std::fprintf(
                stderr,
                "[world] lighting preset .lit ref has no decoded Light object: %s\n",
                ref.c_str());
            ++unmatched_logged;
        }
    }
    std::fprintf(
        stderr,
        "[world] lighting Light object coverage: decoded=%zu failed=%zu venue_decoded=%zu preset_lit_refs=%zu matched_lighting=%zu matched_venue=%zu unmatched=%zu\n",
        lights_by_name.size(), failed_lights, venue_lights.size(),
        lit_refs.size(), matched_refs, matched_venue_refs, unmatched_refs);

    size_t matched_env_refs = 0;
    size_t matched_venue_env_refs = 0;
    size_t unmatched_env_refs = 0;
    size_t unmatched_env_logged = 0;
    for (const auto& [ref, unused] : env_refs) {
        (void)unused;
        if (environs_by_name.find(ref) != environs_by_name.end()) {
            ++matched_env_refs;
            continue;
        }
        if (venue_environs.find(ref) != venue_environs.end()) {
            ++matched_venue_env_refs;
            continue;
        }
        ++unmatched_env_refs;
        if (unmatched_env_logged < 24) {
            std::fprintf(
                stderr,
                "[world] lighting preset .env ref has no decoded Environ object: %s\n",
                ref.c_str());
            ++unmatched_env_logged;
        }
    }
    std::fprintf(
        stderr,
        "[world] lighting Environ object coverage: decoded=%zu failed=%zu venue_decoded=%zu preset_env_refs=%zu matched_lighting=%zu matched_venue=%zu unmatched=%zu\n",
        environs_by_name.size(), failed_environs, venue_environs.size(),
        env_refs.size(), matched_env_refs, matched_venue_env_refs,
        unmatched_env_refs);
}

struct LightingRequest {
    std::string category = "INTRO";
    std::string adjective;
};

uint32_t venue_excitement_level(std::string_view venue_event) {
    if (venue_event.find("peak") != std::string_view::npos) return 4;
    if (venue_event.find("great") != std::string_view::npos) return 3;
    if (venue_event.find("bad") != std::string_view::npos) return 1;
    if (venue_event.find("boot") != std::string_view::npos) return 0;
    return 2;
}

bool venue_excitement_is_high(std::string_view venue_event) {
    return venue_excitement_level(venue_event) >= 3;
}

LightingRequest lighting_request_at(const ghogx::chart::Chart& chart,
                                    double song_time,
                                    double intro_seconds) {
    LightingRequest req;
    if (intro_seconds > 0.0 && song_time >= intro_seconds) req.category = "VERSE";
    for (const auto& ev : chart.text_events) {
        const double t = chart.tick_to_sec(ev.tick);
        if (t > song_time) break;
        if (ev.text == "[verse]") {
            req.category = "VERSE";
        } else if (ev.text == "[chorus]") {
            req.category = "CHORUS";
        } else if (ev.text == "[solo]") {
            req.category = "SOLO";
        } else if (ev.text.rfind("[lighting", 0) == 0) {
            req.adjective.clear();
            const size_t open = ev.text.find('(');
            const size_t close =
                ev.text.find(')', open == std::string::npos ? 0 : open);
            if (open != std::string::npos && close != std::string::npos &&
                close > open + 1) {
                req.adjective = ev.text.substr(open + 1, close - open - 1);
            }
        }
    }
    return req;
}

const Gameplay::LightingPreset* choose_lighting_preset(
    const std::vector<Gameplay::LightingPreset>& presets,
    const LightingRequest& request,
    uint32_t excitement_level) {
    const std::string_view primary = request.category;
    const std::string_view fallback =
        (request.category == "INTRO") ? std::string_view{} : "VERSECHORUSSOLO";
    const Gameplay::LightingPreset* best = nullptr;
    const Gameplay::LightingPreset* unadjectived = nullptr;
    for (const auto& p : presets) {
        if (excitement_level < p.min_excitement ||
            excitement_level > p.max_excitement) {
            continue;
        }
        if (p.category != primary) continue;
        if (!request.adjective.empty() && p.adjective == request.adjective) {
            return &p;
        }
        if (!unadjectived && p.adjective.empty()) unadjectived = &p;
        if (!best) best = &p;
    }
    if (!fallback.empty()) {
        const Gameplay::LightingPreset* fallback_unadjectived = nullptr;
        for (const auto& p : presets) {
            if (excitement_level < p.min_excitement ||
                excitement_level > p.max_excitement) {
                continue;
            }
            if (p.category != fallback) continue;
            if (!request.adjective.empty() && p.adjective == request.adjective) {
                return &p;
            }
            if (!fallback_unadjectived && p.adjective.empty()) {
                fallback_unadjectived = &p;
            }
            if (!best) best = &p;
        }
        if (fallback_unadjectived) return fallback_unadjectived;
    }
    if (unadjectived) return unadjectived;
    return best;
}

size_t lighting_keyframe_index_at(const Gameplay::LightingPreset& preset,
                                  const ghogx::chart::Chart& chart,
                                  double song_time,
                                  double preset_start_time) {
    if (preset.keyframes.empty()) return 0;
    if (preset.keyframes.size() == 1) return 0;
    constexpr double kFramesPerSecond = 30.0;
    double total_duration = 0.0;
    for (const auto& keyframe : preset.keyframes) {
        if (std::isfinite(keyframe.duration) && keyframe.duration > 0.0f)
            total_duration += keyframe.duration / kFramesPerSecond;
        if (std::isfinite(keyframe.fade_out) && keyframe.fade_out > 0.0f) {
            total_duration += keyframe.fade_out / kFramesPerSecond;
        }
    }
    if (total_duration > 0.0) {
        double local = std::max(0.0, song_time - preset_start_time);
        local = std::fmod(local, total_duration);
        double cursor = 0.0;
        for (size_t i = 0; i < preset.keyframes.size(); ++i) {
            const double duration =
                (std::isfinite(preset.keyframes[i].duration) &&
                 preset.keyframes[i].duration > 0.0f)
                    ? preset.keyframes[i].duration / kFramesPerSecond
                    : 0.0;
            cursor += duration;
            if (local < cursor) return i;
            const double fade =
                (std::isfinite(preset.keyframes[i].fade_out) &&
                 preset.keyframes[i].fade_out > 0.0f)
                    ? preset.keyframes[i].fade_out / kFramesPerSecond
                    : 0.0;
            cursor += fade;
            if (local < cursor) return (i + 1) % preset.keyframes.size();
        }
        return preset.keyframes.size() - 1;
    }
    if (chart.ticks_per_beat == 0) return 0;
    const uint32_t tick = chart.sec_to_tick(song_time);
    const uint32_t start_tick = chart.sec_to_tick(preset_start_time);
    const uint32_t elapsed = tick >= start_tick ? tick - start_tick : 0;
    const uint32_t beat = elapsed / chart.ticks_per_beat;
    return static_cast<size_t>(beat % preset.keyframes.size());
}

constexpr double kLightingFramesPerSecond = 30.0;

double authored_frames_to_seconds(float frames) {
    return std::isfinite(frames) && frames > 0.0f
               ? static_cast<double>(frames) / kLightingFramesPerSecond
               : 0.0;
}

double lighting_frames_to_seconds(float frames) {
    return authored_frames_to_seconds(frames);
}

using SpotlightState = ghogx::render::MiloSceneRenderer::SpotlightState;

std::vector<SpotlightState> sorted_unique_spotlight_states(
    std::vector<SpotlightState> spots) {
    std::sort(spots.begin(), spots.end(), [](const auto& a, const auto& b) {
        return a.name < b.name;
    });
    spots.erase(std::unique(spots.begin(), spots.end(),
                            [](const auto& a, const auto& b) {
                                return a.name == b.name;
                            }),
                spots.end());
    return spots;
}

bool nearly_same(float a, float b) {
    return std::fabs(a - b) <= 0.0001f;
}

bool same_spotlight_states(const std::vector<SpotlightState>& a,
                           const std::vector<SpotlightState>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].name != b[i].name ||
            a[i].target_mesh != b[i].target_mesh ||
            !nearly_same(a[i].r, b[i].r) ||
            !nearly_same(a[i].g, b[i].g) ||
            !nearly_same(a[i].b, b[i].b) ||
            !nearly_same(a[i].intensity, b[i].intensity)) {
            return false;
        }
    }
    return true;
}

std::map<std::string, SpotlightState> spotlight_state_map(
    const std::vector<SpotlightState>& spots) {
    std::map<std::string, SpotlightState> out;
    for (const auto& spot : spots) out[spot.name] = spot;
    return out;
}

float mix_lighting(float a, float b, double t) {
    return static_cast<float>(static_cast<double>(a) +
                              (static_cast<double>(b) - a) * t);
}

std::string performer_event_track_for_role(std::string_view role) {
    if (role == "guitarist0") return "PART GUITAR";
    if (role == "bassist") return "BAND BASS";
    if (role == "drummer") return "BAND DRUMS";
    if (role == "singer") return "BAND SINGER";
    if (role == "keyboard") return "BAND KEYS";
    return {};
}

struct BandRoleNames {
    std::string singer;
    std::string bass;
    std::string drummer;
    std::string keyboard;
};

BandRoleNames classify_band_roles(const std::vector<std::string>& band) {
    BandRoleNames roles;
    for (const auto& member : band) {
        if (member.find("keyboard") != std::string::npos) {
            if (roles.keyboard.empty()) roles.keyboard = member;
        } else if (member.find("drum") != std::string::npos) {
            if (roles.drummer.empty()) roles.drummer = member;
        } else if (member.find("bass") != std::string::npos) {
            if (roles.bass.empty()) roles.bass = member;
        } else if (member.find("singer") != std::string::npos) {
            if (roles.singer.empty()) roles.singer = member;
        }
    }

    // GH2 default_band order is singer/bass/drummer. Keep this as a fallback
    // for mods or future assets whose names do not encode the role.
    if (roles.singer.empty() && band.size() > 0 &&
        band[0].find("bass") == std::string::npos &&
        band[0].find("drum") == std::string::npos &&
        band[0].find("keyboard") == std::string::npos) {
        roles.singer = band[0];
    }
    if (roles.bass.empty() && band.size() > 1 &&
        band[1].find("drum") == std::string::npos &&
        band[1].find("keyboard") == std::string::npos) {
        roles.bass = band[1];
    }
    if (roles.drummer.empty() && band.size() > 2 &&
        band[2].find("keyboard") == std::string::npos) {
        roles.drummer = band[2];
    }
    return roles;
}

struct PerformerMidiState {
    bool playing = false;
    bool wail = false;
    bool solo = false;
    bool allbeat = false;
    bool half_time = false;
    bool no_snare = false;
    std::string hand_map;
    std::string marker;
};

PerformerMidiState performer_midi_state_at(const ghogx::chart::Chart& chart,
                                           std::string_view track,
                                           double song_time) {
    PerformerMidiState state;
    for (const auto& ev : chart.performer_events) {
        if (ev.track != track) continue;
        const double t = chart.tick_to_sec(ev.tick);
        if (t > song_time) break;
        state.marker = ev.text;
        if (ev.text == "[play]") {
            state.playing = true;
        } else if (ev.text == "[idle]") {
            state.playing = false;
        } else if (ev.text == "[nobeat]") {
            state.playing = true;
            state.no_snare = true;
            state.allbeat = false;
            state.half_time = false;
        } else if (ev.text == "[wail_on]") {
            state.wail = true;
        } else if (ev.text == "[wail_off]") {
            state.wail = false;
        } else if (ev.text == "[solo_on]") {
            state.solo = true;
        } else if (ev.text == "[solo_off]") {
            state.solo = false;
        } else if (ev.text == "[allbeat]") {
            state.playing = true;
            state.allbeat = true;
            state.half_time = false;
            state.no_snare = false;
        } else if (ev.text == "[half_time]") {
            state.playing = true;
            state.half_time = true;
            state.allbeat = false;
            state.no_snare = false;
        } else if (ev.text.rfind("[map ", 0) == 0 && ev.text.size() > 6) {
            state.hand_map = ev.text.substr(5, ev.text.size() - 6);
        }
    }
    return state;
}

std::string infer_spotlight_from_target(std::string_view target) {
    const auto base = strip_spotlight_target_mesh_suffix(target);
    if (!base) {
        return {};
    }
    std::string name(*base);
    name += ".spot";
    return name;
}

std::array<float, 16> xfm_to_mat4(const ghogx::milo_scene::Xfm& x) {
    return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
            x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
            x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
            x.pos[0], x.pos[1], x.pos[2], 1.0f};
}

std::optional<ghogx::milo_scene::Xfm> find_start_xfm(
    const ghogx::milo_scene::Scene& chars, std::string_view name,
    std::initializer_list<uint32_t> flags) {
    for (uint32_t flag : flags) {
        if (flag == 0) continue;
        for (const auto& w : chars.waypoints) {
            if (w.decoded && (w.flags & flag)) return w.local;
        }
    }
    if (!name.empty()) {
        for (const auto& w : chars.waypoints) {
            if (w.decoded && w.name == name) return w.local;
        }
    }
    return std::nullopt;
}

bool load_clip_first(ghogx::character::CharClip& out,
                     const std::string& hdr_path, const std::string& ark_path,
                     const std::string& milo_path,
                     std::initializer_list<const char*> names) {
    for (const char* name : names) {
        out = ghogx::character::load_clip(hdr_path, ark_path, milo_path, name);
        if (out.loaded) return true;
    }
    return false;
}

bool load_clip_first(ghogx::character::CharClip& out,
                     const std::string& hdr_path, const std::string& ark_path,
                     const std::string& milo_path,
                     const std::vector<std::string>& names) {
    for (const auto& name : names) {
        out = ghogx::character::load_clip(hdr_path, ark_path, milo_path, name);
        if (out.loaded) return true;
    }
    return false;
}

bool load_clip_first_from_milos(
    ghogx::character::CharClip& out, const std::string& hdr_path,
    const std::string& ark_path, const std::vector<std::string>& milos,
    const std::vector<std::string>& names) {
    for (const auto& milo_path : milos) {
        if (load_clip_first(out, hdr_path, ark_path, milo_path, names))
            return true;
    }
    return false;
}

std::vector<std::string> load_char_clip_group(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_candidates,
    const std::string& group_name) {
    auto read_u8 = [](const std::vector<uint8_t>& b, size_t& p) -> uint8_t {
        if (p >= b.size()) return 0;
        return b[p++];
    };
    auto read_i32 = [](const std::vector<uint8_t>& b, size_t& p) -> int32_t {
        if (p + 4 > b.size()) {
            p = b.size();
            return 0;
        }
        int32_t v = 0;
        std::memcpy(&v, b.data() + p, sizeof(v));
        p += 4;
        return v;
    };
    auto read_u32 = [](const std::vector<uint8_t>& b, size_t& p) -> uint32_t {
        if (p + 4 > b.size()) {
            p = b.size();
            return 0;
        }
        uint32_t v = 0;
        std::memcpy(&v, b.data() + p, sizeof(v));
        p += 4;
        return v;
    };
    auto read_string = [&](const std::vector<uint8_t>& b,
                           size_t& p) -> std::string {
        const uint32_t n = read_u32(b, p);
        if (n > b.size() - p) {
            p = b.size();
            return {};
        }
        std::string s(reinterpret_cast<const char*>(b.data() + p), n);
        p += n;
        return s;
    };

    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        for (const auto& milo_path : milo_candidates) {
            auto entry = ark.find(milo_path);
            if (!entry) continue;
            auto bytes = ark.read_entry(*entry, {ark_path});
            auto hdr = gh::milo::parse_header(bytes);
            auto payload = gh::milo::inflate_payload(bytes, hdr);
            auto dir = gh::milo::parse_directory(payload);
            for (const auto& de : dir.entries) {
                if (de.type != "CharClipGroup" || de.name != group_name ||
                    de.offset + de.size > payload.size()) {
                    continue;
                }
                std::vector<uint8_t> body(payload.begin() + de.offset,
                                          payload.begin() + de.offset + de.size);
                size_t p = 0;
                (void)read_i32(body, p);     // version
                (void)read_u32(body, p);     // Hmx::Object revision
                (void)read_string(body, p);  // object symbol
                (void)read_u8(body, p);      // object terminator
                const uint32_t count = read_u32(body, p);
                std::vector<std::string> clips;
                clips.reserve(count);
                for (uint32_t i = 0; i < count && p < body.size(); ++i) {
                    std::string name = read_string(body, p);
                    if (!name.empty()) clips.push_back(std::move(name));
                }
                if (!clips.empty()) {
                    std::fprintf(stderr,
                                 "[world] CharClipGroup %s from %s: %zu clips\n",
                                 group_name.c_str(), milo_path.c_str(),
                                 clips.size());
                    return clips;
                }
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] CharClipGroup %s: %s\n",
                     group_name.c_str(), ex.what());
    }
    return {};
}

std::vector<Gameplay::CameraKey> load_camera_position_keys(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& venue, const std::string& anim_name) {
    std::vector<Gameplay::CameraKey> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        const std::string milo_path =
            "world/" + venue + "/gen/" + venue + ".milo_ps2";
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        const gh::milo::Entry* anim = nullptr;
        for (const auto& de : dir.entries) {
            if (de.type == "TransAnim" && de.name == anim_name) {
                anim = &de;
                break;
            }
        }
        if (!anim || anim->offset + anim->size > payload.size()) return out;
        const uint8_t* body = payload.data() + anim->offset;
        const size_t size = static_cast<size_t>(anim->size);
        auto f32_at = [&](size_t off) {
            float v = 0.0f;
            std::memcpy(&v, body + off, sizeof(v));
            return v;
        };
        auto plausible = [](float v) {
            return std::isfinite(v) && std::abs(v) < 2500.0f;
        };

        size_t best_off = SIZE_MAX;
        int best_len = 0;
        for (size_t off = 0; off + 16 <= size; ++off) {
            float prev_frame = -1.0f;
            int len = 0;
            for (size_t p = off; p + 16 <= size; p += 16) {
                const float x = f32_at(p + 0);
                const float y = f32_at(p + 4);
                const float z = f32_at(p + 8);
                const float frame = f32_at(p + 12);
                if (!plausible(x) || !plausible(y) || !plausible(z) ||
                    !std::isfinite(frame) || frame < prev_frame ||
                    frame > 2000.0f) {
                    break;
                }
                if (std::abs(x) < 10.0f && std::abs(y) < 10.0f &&
                    std::abs(z) < 10.0f) {
                    break;
                }
                prev_frame = frame;
                ++len;
            }
            if (len > best_len) {
                best_len = len;
                best_off = off;
            }
        }
        if (best_len < 4 || best_off == SIZE_MAX) return out;
        out.reserve(static_cast<size_t>(best_len));
        for (int i = 0; i < best_len; ++i) {
            const size_t p = best_off + static_cast<size_t>(i) * 16;
            Gameplay::CameraKey k;
            k.eye[0] = f32_at(p + 0);
            k.eye[1] = f32_at(p + 4);
            k.eye[2] = f32_at(p + 8);
            k.frame = f32_at(p + 12);
            out.push_back(k);
        }
        std::vector<Gameplay::CameraKey> rot_keys;
        for (size_t off = 0; off + 20 <= size; ++off) {
            float prev_frame = -1.0f;
            std::vector<Gameplay::CameraKey> cand;
            for (size_t p = off; p + 20 <= size; p += 20) {
                const float x = f32_at(p + 0);
                const float y = f32_at(p + 4);
                const float z = f32_at(p + 8);
                const float w = f32_at(p + 12);
                const float frame = f32_at(p + 16);
                const float n = std::sqrt(x * x + y * y + z * z + w * w);
                if (!std::isfinite(n) || n < 0.95f || n > 1.05f ||
                    frame < prev_frame || frame > 2000.0f) {
                    break;
                }
                Gameplay::CameraKey k;
                k.frame = frame;
                k.quat[0] = x;
                k.quat[1] = y;
                k.quat[2] = z;
                k.quat[3] = w;
                k.has_quat = true;
                cand.push_back(k);
                prev_frame = frame;
            }
            if (cand.size() > rot_keys.size()) rot_keys = std::move(cand);
        }
        if (!rot_keys.empty()) {
            size_t ri = 0;
            for (auto& pos : out) {
                while (ri + 1 < rot_keys.size() &&
                       rot_keys[ri + 1].frame <= pos.frame) {
                    ++ri;
                }
                pos.has_quat = true;
                for (int i = 0; i < 4; ++i) pos.quat[i] = rot_keys[ri].quat[i];
            }
            std::fprintf(stderr, "[world] camera anim %s: %zu rot keys\n",
                         anim_name.c_str(), rot_keys.size());
        }
        std::fprintf(stderr, "[world] camera anim %s: %zu keys at body+0x%zX\n",
                     anim_name.c_str(), out.size(), best_off);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] camera anim %s: %s\n", anim_name.c_str(),
                     ex.what());
    }
    return out;
}

std::optional<std::string> first_mesh_target_in_transanim(const uint8_t* body,
                                                          size_t size) {
    for (size_t off = 0; off + 9 <= size; ++off) {
        uint32_t len = 0;
        std::memcpy(&len, body + off, sizeof(len));
        if (len == 0 || len > 64 || off + 4 + len > size) continue;
        const char* s = reinterpret_cast<const char*>(body + off + 4);
        bool printable = true;
        for (uint32_t i = 0; i < len; ++i) {
            const unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 32 || c > 126) {
                printable = false;
                break;
            }
        }
        if (!printable) continue;
        std::string name(s, s + len);
        if (name.size() > 5 &&
            name.rfind(".mesh") == name.size() - 5) {
            return name;
        }
    }
    return std::nullopt;
}

Gameplay::VenueMeshAnim decode_venue_mesh_anim(
    const std::string& entry_name, const uint8_t* body, size_t size) {
    Gameplay::VenueMeshAnim anim;
    anim.name = canonical_milo_ref(entry_name);
    if (size < 32 || read_u32_at_unchecked(body, 0) != 1) return anim;

    const auto strings = packed_strings_with_offsets(body, size);
    const PackedStringHit* mesh_string = nullptr;
    const PackedStringHit* self_string = nullptr;
    const PackedStringHit* owner_string = nullptr;
    for (const auto& hit : strings) {
        const auto ref = canonical_milo_ref(hit.value);
        if (!mesh_string && ref.size() > 5 &&
            ref.rfind(".mesh") == ref.size() - 5) {
            mesh_string = &hit;
        }
        if (ref == anim.name) self_string = &hit;
    }
    for (const auto& hit : strings) {
        const auto ref = canonical_milo_ref(hit.value);
        if (ref != anim.name && ref.size() > 5 &&
            ref.rfind(".msnm") == ref.size() - 5) {
            owner_string = &hit;
            break;
        }
    }
    if (mesh_string) anim.mesh = canonical_milo_ref(mesh_string->value);
    if (owner_string) anim.keys_owner = canonical_milo_ref(owner_string->value);

    const size_t limit = self_string ? self_string->offset : size;
    auto valid_count_pair = [&](size_t off, uint32_t& frames,
                                uint32_t& verts) -> bool {
        if (off + 8 > limit) return false;
        frames = read_u32_at_unchecked(body, off);
        verts = read_u32_at_unchecked(body, off + 4);
        if (frames == 0 || frames > 4096 || verts == 0 || verts > 20000)
            return false;
        const uint64_t samples = static_cast<uint64_t>(frames) * verts;
        if (samples > 2000000ull) return false;
        const uint64_t bytes = samples * 12ull;
        if (bytes > static_cast<uint64_t>(limit - (off + 8))) return false;
        return true;
    };

    uint32_t frames = 0;
    uint32_t verts = 0;
    size_t count_off = std::string::npos;
    if (mesh_string && valid_count_pair(mesh_string->end, frames, verts)) {
        count_off = mesh_string->end;
    } else {
        for (size_t off = 13; off + 8 <= std::min<size_t>(limit, 160); ++off) {
            if (valid_count_pair(off, frames, verts)) {
                count_off = off;
                break;
            }
        }
    }
    if (count_off == std::string::npos) return anim;

    anim.frame_count = frames;
    anim.vertex_count = verts;
    if (self_string && self_string->offset >= 12) {
        const float duration = read_f32_at_unchecked(body, self_string->offset - 12);
        if (std::isfinite(duration) && duration > 0.001f &&
            duration < 100000.0f) {
            anim.duration_frames = duration;
        }
    }
    if (anim.duration_frames <= 0.001f)
        anim.duration_frames = frames > 1 ? 100.0f : 0.0f;

    const size_t data = count_off + 8;
    anim.frames.resize(frames);
    size_t pos = data;
    bool ok = true;
    for (uint32_t f = 0; f < frames && ok; ++f) {
        auto& frame = anim.frames[f];
        frame.positions.resize(verts);
        for (uint32_t v = 0; v < verts; ++v) {
            std::array<float, 3> p{};
            for (int c = 0; c < 3; ++c) {
                if (pos + 4 > size) {
                    ok = false;
                    break;
                }
                p[c] = read_f32_at_unchecked(body, pos);
                pos += 4;
                if (!std::isfinite(p[c]) || std::fabs(p[c]) > 1000000.0f) {
                    ok = false;
                    break;
                }
            }
            if (!ok) break;
            frame.positions[v] = p;
        }
    }
    if (!ok) {
        anim.frames.clear();
        anim.frame_count = 0;
        anim.vertex_count = 0;
    }
    return anim;
}

struct TransAnimVec3Block {
    size_t offset = 0;
    std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey> keys;
    float delta = 0.0f;
    bool scale_like = false;
};

std::vector<TransAnimVec3Block> decode_transanim_vec3_blocks(
    const uint8_t* body, size_t size) {
    using Key = ghogx::render::MiloSceneRenderer::MeshAnimKey;
    auto u32_at = [&](size_t off) {
        uint32_t v = 0;
        std::memcpy(&v, body + off, sizeof(v));
        return v;
    };
    auto f32_at = [&](size_t off) {
        float v = 0.0f;
        std::memcpy(&v, body + off, sizeof(v));
        return v;
    };
    auto plausible_vec = [](float v) {
        return std::isfinite(v) && std::abs(v) < 2000.0f;
    };

    std::vector<TransAnimVec3Block> blocks;
    for (size_t off = 0; off + 4 <= size; ++off) {
        const uint32_t count = u32_at(off);
        if (count < 2 || count > 128) continue;
        const size_t start = off + 4;
        if (start + static_cast<size_t>(count) * 16 > size) continue;
        TransAnimVec3Block block;
        block.offset = off;
        block.keys.reserve(count);
        float prev_frame = -1.0f;
        bool ok = true;
        bool scale_like = true;
        for (uint32_t i = 0; i < count; ++i) {
            const size_t p = start + static_cast<size_t>(i) * 16;
            Key k;
            k.pos[0] = f32_at(p + 0);
            k.pos[1] = f32_at(p + 4);
            k.pos[2] = f32_at(p + 8);
            k.frame = f32_at(p + 12);
            if (!plausible_vec(k.pos[0]) || !plausible_vec(k.pos[1]) ||
                !plausible_vec(k.pos[2]) || !std::isfinite(k.frame) ||
                k.frame < prev_frame || k.frame > 1000.0f) {
                ok = false;
                break;
            }
            for (float v : k.pos) {
                if (v <= 0.001f || v > 20.0f) scale_like = false;
            }
            prev_frame = k.frame;
            block.keys.push_back(k);
        }
        if (!ok) continue;
        for (float v : block.keys.front().pos) {
            if (v < 0.05f || v > 5.0f) scale_like = false;
        }
        float delta = 0.0f;
        for (const auto& k : block.keys) {
            const float dx = k.pos[0] - block.keys.front().pos[0];
            const float dy = k.pos[1] - block.keys.front().pos[1];
            const float dz = k.pos[2] - block.keys.front().pos[2];
            delta = std::max(delta, std::sqrt(dx * dx + dy * dy + dz * dz));
        }
        if (delta <= 0.001f) continue;
        block.delta = delta;
        block.scale_like = scale_like;
        blocks.push_back(std::move(block));
    }
    return blocks;
}

std::vector<ghogx::render::MiloSceneRenderer::MeshQuatAnimKey>
decode_transanim_rotation_keys(const uint8_t* body, size_t size) {
    using Key = ghogx::render::MiloSceneRenderer::MeshQuatAnimKey;
    auto u32_at = [&](size_t off) {
        uint32_t v = 0;
        std::memcpy(&v, body + off, sizeof(v));
        return v;
    };
    auto f32_at = [&](size_t off) {
        float v = 0.0f;
        std::memcpy(&v, body + off, sizeof(v));
        return v;
    };

    std::vector<Key> best;
    float best_delta = 0.0f;
    for (size_t off = 0; off + 4 <= size; ++off) {
        const uint32_t count = u32_at(off);
        if (count < 2 || count > 128) continue;
        const size_t start = off + 4;
        if (start + static_cast<size_t>(count) * 20 > size) continue;
        std::vector<Key> keys;
        keys.reserve(count);
        float prev_frame = -1.0f;
        bool ok = true;
        for (uint32_t i = 0; i < count; ++i) {
            const size_t p = start + static_cast<size_t>(i) * 20;
            Key k;
            k.quat_xyzw[0] = f32_at(p + 0);
            k.quat_xyzw[1] = f32_at(p + 4);
            k.quat_xyzw[2] = f32_at(p + 8);
            k.quat_xyzw[3] = f32_at(p + 12);
            k.frame = f32_at(p + 16);
            const float norm =
                std::sqrt(k.quat_xyzw[0] * k.quat_xyzw[0] +
                          k.quat_xyzw[1] * k.quat_xyzw[1] +
                          k.quat_xyzw[2] * k.quat_xyzw[2] +
                          k.quat_xyzw[3] * k.quat_xyzw[3]);
            if (!std::isfinite(norm) || norm < 0.5f || norm > 1.5f ||
                !std::isfinite(k.frame) || k.frame < prev_frame ||
                k.frame > 1000.0f) {
                ok = false;
                break;
            }
            prev_frame = k.frame;
            keys.push_back(k);
        }
        if (!ok) continue;
        float delta = 0.0f;
        const auto& first = keys.front();
        for (const auto& k : keys) {
            const float dot = std::abs(first.quat_xyzw[0] * k.quat_xyzw[0] +
                                       first.quat_xyzw[1] * k.quat_xyzw[1] +
                                       first.quat_xyzw[2] * k.quat_xyzw[2] +
                                       first.quat_xyzw[3] * k.quat_xyzw[3]);
            delta = std::max(delta, 1.0f - std::min(dot, 1.0f));
        }
        if (delta > best_delta && delta > 0.000001f) {
            best_delta = delta;
            best = std::move(keys);
        }
    }
    return best;
}

ghogx::render::MiloSceneRenderer::MeshTransformAnim decode_transanim_transform_anim(
    const uint8_t* body, size_t size) {
    ghogx::render::MiloSceneRenderer::MeshTransformAnim anim;
    const auto blocks = decode_transanim_vec3_blocks(body, size);
    const TransAnimVec3Block* translation = nullptr;
    const TransAnimVec3Block* fallback_translation = nullptr;
    const TransAnimVec3Block* scale = nullptr;
    for (const auto& block : blocks) {
        if (!fallback_translation || block.delta > fallback_translation->delta)
            fallback_translation = &block;
        if (!block.scale_like &&
            (!translation || block.delta > translation->delta)) {
            translation = &block;
        }
    }
    if (!translation) translation = fallback_translation;
    for (const auto& block : blocks) {
        if (&block == translation || !block.scale_like) continue;
        if (!scale || block.delta > scale->delta) scale = &block;
    }
    if (translation) anim.translation_keys = translation->keys;
    if (scale) anim.scale_keys = scale->keys;
    anim.rotation_keys = decode_transanim_rotation_keys(body, size);
    return anim;
}

bool mesh_transform_anim_empty(
    const ghogx::render::MiloSceneRenderer::MeshTransformAnim& anim) {
    return anim.translation_keys.empty() && anim.rotation_keys.empty() &&
           anim.scale_keys.empty();
}

std::vector<std::string> ascii_strings_in_object(const uint8_t* body,
                                                 size_t size) {
    std::vector<std::string> out;
    for (size_t off = 0; off + 4 <= size; ++off) {
        uint32_t len = 0;
        std::memcpy(&len, body + off, sizeof(len));
        if (len == 0 || len > 64 || off + 4 + len > size) continue;
        const char* s = reinterpret_cast<const char*>(body + off + 4);
        bool printable = true;
        for (uint32_t i = 0; i < len; ++i) {
            const unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 32 || c > 126) {
                printable = false;
                break;
            }
        }
        if (printable) out.emplace_back(s, s + len);
    }
    return out;
}

std::optional<size_t> packed_string_end(const uint8_t* body, size_t size,
                                        std::string_view value) {
    if (value.empty() || value.size() > 256) return std::nullopt;
    for (size_t off = 0; off + 4 + value.size() <= size; ++off) {
        uint32_t len = 0;
        std::memcpy(&len, body + off, sizeof(len));
        if (len != value.size()) continue;
        const char* s = reinterpret_cast<const char*>(body + off + 4);
        if (std::memcmp(s, value.data(), value.size()) == 0) {
            return off + 4 + value.size();
        }
    }
    return std::nullopt;
}

float read_f32_or(const uint8_t* body, size_t size, size_t off, float fallback) {
    if (off + 4 > size) return fallback;
    float v = fallback;
    std::memcpy(&v, body + off, sizeof(v));
    return std::isfinite(v) ? v : fallback;
}

int read_i32_or(const uint8_t* body, size_t size, size_t off, int fallback) {
    if (off + 4 > size) return fallback;
    int32_t v = fallback;
    std::memcpy(&v, body + off, sizeof(v));
    return v;
}

bool read_u32_cursor(const uint8_t* body, size_t size, size_t& cursor,
                     uint32_t& out) {
    if (cursor + 4 > size) return false;
    std::memcpy(&out, body + cursor, sizeof(out));
    cursor += 4;
    return true;
}

bool read_packed_string_cursor(const uint8_t* body, size_t size,
                               size_t& cursor, std::string& out) {
    uint32_t len = 0;
    if (!read_u32_cursor(body, size, cursor, len)) return false;
    if (len > 256 || cursor + len > size) return false;
    out.assign(reinterpret_cast<const char*>(body + cursor),
               reinterpret_cast<const char*>(body + cursor + len));
    cursor += len;
    return true;
}

std::array<float, 3> sample_translation_offset(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey>& keys,
    float frame) {
    std::array<float, 3> out = {0.0f, 0.0f, 0.0f};
    if (keys.empty()) return out;
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = std::max(b->frame - a->frame, 0.001f);
    const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    for (int i = 0; i < 3; ++i) {
        const float p = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
        out[i] = p - keys.front().pos[i];
    }
    return out;
}

std::array<float, 3> sample_scale_ratio(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey>& keys,
    float frame) {
    std::array<float, 3> out = {1.0f, 1.0f, 1.0f};
    if (keys.empty()) return out;
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = std::max(b->frame - a->frame, 0.001f);
    const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    for (int i = 0; i < 3; ++i) {
        const float p = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
        const float base = keys.front().pos[i];
        out[i] = std::fabs(base) > 0.0001f ? p / base : 1.0f;
    }
    return out;
}

std::array<float, 4> normalize_quat_xyzw(std::array<float, 4> q) {
    const float len = std::sqrt(q[0] * q[0] + q[1] * q[1] +
                                q[2] * q[2] + q[3] * q[3]);
    if (!std::isfinite(len) || len <= 0.000001f)
        return {0.0f, 0.0f, 0.0f, 1.0f};
    const float inv = 1.0f / len;
    for (float& v : q) v *= inv;
    return q;
}

std::array<float, 4> quat_conjugate_xyzw(std::array<float, 4> q) {
    q[0] = -q[0];
    q[1] = -q[1];
    q[2] = -q[2];
    return q;
}

std::array<float, 4> quat_mul_xyzw(const std::array<float, 4>& a,
                                   const std::array<float, 4>& b) {
    const float ax = a[0], ay = a[1], az = a[2], aw = a[3];
    const float bx = b[0], by = b[1], bz = b[2], bw = b[3];
    return normalize_quat_xyzw({
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    });
}

std::array<float, 4> slerp_quat_xyzw(std::array<float, 4> a,
                                     std::array<float, 4> b, float t) {
    a = normalize_quat_xyzw(a);
    b = normalize_quat_xyzw(b);
    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (dot < 0.0f) {
        for (float& v : b) v = -v;
        dot = -dot;
    }
    if (dot > 0.9995f) {
        return normalize_quat_xyzw({a[0] + (b[0] - a[0]) * t,
                                    a[1] + (b[1] - a[1]) * t,
                                    a[2] + (b[2] - a[2]) * t,
                                    a[3] + (b[3] - a[3]) * t});
    }
    dot = std::clamp(dot, -1.0f, 1.0f);
    const float theta0 = std::acos(dot);
    const float theta = theta0 * t;
    const float sin_theta = std::sin(theta);
    const float sin_theta0 = std::sin(theta0);
    const float s0 = std::cos(theta) - dot * sin_theta / sin_theta0;
    const float s1 = sin_theta / sin_theta0;
    return normalize_quat_xyzw({a[0] * s0 + b[0] * s1,
                                a[1] * s0 + b[1] * s1,
                                a[2] * s0 + b[2] * s1,
                                a[3] * s0 + b[3] * s1});
}

std::array<float, 4> sample_rotation_delta(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshQuatAnimKey>& keys,
    float frame) {
    if (keys.empty()) return {0.0f, 0.0f, 0.0f, 1.0f};
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = std::max(b->frame - a->frame, 0.001f);
    const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    const std::array<float, 4> qa = {a->quat_xyzw[0], a->quat_xyzw[1],
                                     a->quat_xyzw[2], a->quat_xyzw[3]};
    const std::array<float, 4> qb = {b->quat_xyzw[0], b->quat_xyzw[1],
                                     b->quat_xyzw[2], b->quat_xyzw[3]};
    const auto cur = slerp_quat_xyzw(qa, qb, t);
    const std::array<float, 4> base = {
        keys.front().quat_xyzw[0], keys.front().quat_xyzw[1],
        keys.front().quat_xyzw[2], keys.front().quat_xyzw[3]};
    return quat_mul_xyzw(quat_conjugate_xyzw(normalize_quat_xyzw(base)), cur);
}

ghogx::render::MiloSceneRenderer::MeshTransformSample sample_mesh_transform(
    const ghogx::render::MiloSceneRenderer::MeshTransformAnim& anim,
    float frame) {
    ghogx::render::MiloSceneRenderer::MeshTransformSample sample;
    if (anim.translation_keys.size() >= 2) {
        sample.has_translation = true;
        sample.translation = sample_translation_offset(anim.translation_keys, frame);
    }
    if (anim.rotation_keys.size() >= 2) {
        sample.has_rotation = true;
        sample.rotation_xyzw = sample_rotation_delta(anim.rotation_keys, frame);
    }
    if (anim.scale_keys.size() >= 2) {
        sample.has_scale = true;
        sample.scale = sample_scale_ratio(anim.scale_keys, frame);
    }
    return sample;
}

std::vector<std::array<float, 3>> sample_mesh_anim_positions(
    const Gameplay::VenueMeshAnim& anim, float frame) {
    if (anim.frames.empty() || anim.vertex_count == 0) return {};
    if (anim.frames.size() == 1 || anim.duration_frames <= 0.001f)
        return anim.frames.front().positions;
    const float clamped_frame =
        std::clamp(frame, 0.0f, std::max(anim.duration_frames, 0.0f));
    const float key_pos =
        (clamped_frame / std::max(anim.duration_frames, 0.001f)) *
        static_cast<float>(anim.frames.size() - 1);
    const size_t a = static_cast<size_t>(
        std::clamp(std::floor(key_pos), 0.0f,
                   static_cast<float>(anim.frames.size() - 1)));
    const size_t b = std::min(a + 1, anim.frames.size() - 1);
    const float t = std::clamp(key_pos - static_cast<float>(a), 0.0f, 1.0f);
    const auto& fa = anim.frames[a].positions;
    const auto& fb = anim.frames[b].positions;
    if (fa.size() != fb.size()) return {};
    std::vector<std::array<float, 3>> out;
    out.resize(fa.size());
    for (size_t i = 0; i < fa.size(); ++i) {
        for (int c = 0; c < 3; ++c)
            out[i][c] = fa[i][c] + (fb[i][c] - fa[i][c]) * t;
    }
    return out;
}

std::array<float, 2> sample_material_vec2_key(
    const std::vector<Gameplay::VenueMaterialAnim::Vec3Key>& keys,
    float frame) {
    std::array<float, 2> out = {0.0f, 0.0f};
    if (keys.empty()) return out;
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = b->frame - a->frame;
    const float t =
        span <= 0.0001f ? 0.0f : std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    for (int i = 0; i < 2; ++i) {
        const float value = a->value[i] + (b->value[i] - a->value[i]) * t;
        out[i] = value;
    }
    return out;
}

float sample_material_float_key(
    const std::vector<Gameplay::VenueMaterialAnim::FloatKey>& keys,
    float frame) {
    if (keys.empty()) return 0.0f;
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = b->frame - a->frame;
    const float t =
        span <= 0.0001f ? 0.0f : std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    return a->value + (b->value - a->value) * t;
}

ghogx::render::MiloSceneRenderer::MaterialTexTransformSample
sample_material_tex_transform(const Gameplay::ActiveVenueMaterialAnim& anim,
                              float frame) {
    ghogx::render::MiloSceneRenderer::MaterialTexTransformSample sample;
    if (!anim.tex_translation_keys.empty()) {
        const auto uv = sample_material_vec2_key(anim.tex_translation_keys, frame);
        sample.has_translation = true;
        sample.translation[0] = material_anim_tex_value_to_uv(uv[0]);
        sample.translation[1] = material_anim_tex_value_to_uv(uv[1]);
    }
    if (!anim.tex_scale_keys.empty()) {
        sample.has_scale = true;
        sample.scale = sample_material_vec2_key(anim.tex_scale_keys, frame);
    }
    if (!anim.tex_rotation_keys.empty()) {
        sample.has_rotation = true;
        sample.rotation_radians =
            sample_material_float_key(anim.tex_rotation_keys, frame);
    }
    return sample;
}

ghogx::render::MiloSceneRenderer::MaterialTexTransformSample
sample_material_tex_transform(const Gameplay::VenueMaterialAnim& anim,
                              float frame) {
    Gameplay::ActiveVenueMaterialAnim active;
    active.tex_translation_keys = anim.tex_translation_keys;
    active.tex_scale_keys = anim.tex_scale_keys;
    active.tex_rotation_keys = anim.tex_rotation_keys;
    return sample_material_tex_transform(active, frame);
}

std::array<float, 4> sample_environment_color_key(
    const std::vector<Gameplay::VenueEnvironmentAnim::ColorKey>& keys,
    float frame) {
    if (keys.empty()) return {1.0f, 1.0f, 1.0f, 1.0f};
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = b->frame - a->frame;
    const float t =
        span <= 0.0001f ? 0.0f : std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    std::array<float, 4> out{};
    for (int i = 0; i < 4; ++i) {
        out[i] = std::clamp(a->color[i] + (b->color[i] - a->color[i]) * t,
                            0.0f, 1.0f);
    }
    return out;
}

std::array<float, 4> sample_light_color_key(
    const std::vector<Gameplay::VenueLightAnim::ColorKey>& keys,
    float frame) {
    if (keys.empty()) return {1.0f, 1.0f, 1.0f, 1.0f};
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = b->frame - a->frame;
    const float t =
        span <= 0.0001f ? 0.0f : std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    std::array<float, 4> out{};
    for (int i = 0; i < 4; ++i) {
        const float hi = i == 3 ? 1.0f : 4.0f;
        out[i] = std::clamp(a->color[i] + (b->color[i] - a->color[i]) * t,
                            0.0f, hi);
    }
    return out;
}

float particle_emission_key_value(
    const Gameplay::VenueParticleRoute::EmissionKey& key) {
    return std::max(0.0f, (key.min_value + key.max_value) * 0.5f);
}

float sample_particle_emission(
    const std::vector<Gameplay::VenueParticleRoute::EmissionKey>& keys,
    float frame) {
    if (keys.empty()) return 1.0f;
    const auto* a = &keys.front();
    const auto* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    const float span = b->frame - a->frame;
    const float t =
        span <= 0.0001f ? 0.0f : std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
    const float va = particle_emission_key_value(*a);
    const float vb = particle_emission_key_value(*b);
    return std::clamp(va + (vb - va) * t, 0.0f, 8.0f);
}

float material_anim_frame_at(float duration_frames, double elapsed_seconds,
                             bool persistent) {
    if (!std::isfinite(duration_frames) || duration_frames <= 0.001f)
        return 0.0f;
    float frame = static_cast<float>(std::max(0.0, elapsed_seconds) * 30.0);
    if (persistent) {
        frame = std::fmod(frame, duration_frames);
    } else {
        frame = std::min(frame, duration_frames);
    }
    return std::clamp(frame, 0.0f, duration_frames);
}

float venue_filter_frame_at(const Gameplay::VenueAnimFilter& filter,
                            double elapsed_seconds, bool persistent) {
    const float start = filter.start_frame;
    const float end = std::max(filter.end_frame, start);
    const float span = end - start;
    const double authored_offset = std::isfinite(filter.offset_frame)
                                       ? static_cast<double>(filter.offset_frame)
                                       : 0.0;
    if (!std::isfinite(span) || span <= 0.001f) {
        return std::max(0.0f,
                        start + static_cast<float>(authored_offset));
    }

    const double scale =
        std::isfinite(filter.scale) && std::fabs(filter.scale) > 0.001f
            ? std::fabs(static_cast<double>(filter.scale))
            : 1.0;
    double local_frame = 0.0;
    if (std::isfinite(filter.period) && filter.period > 0.001f) {
        local_frame =
            (std::max(0.0, elapsed_seconds) / filter.period) *
            static_cast<double>(span);
    } else {
        local_frame = std::max(0.0, elapsed_seconds) * 30.0 * scale;
    }
    local_frame += authored_offset;

    auto positive_mod = [](double value, double modulus) {
        if (modulus <= 0.001) return 0.0;
        double out = std::fmod(value, modulus);
        if (out < 0.0) out += modulus;
        return out;
    };

    float frame = start;
    switch (filter.type) {
        case 1:  // kAnimLoop
            frame = start + static_cast<float>(
                              positive_mod(local_frame, span));
            break;
        case 2: {  // kAnimShuttle
            const double phase =
                positive_mod(local_frame, static_cast<double>(span) * 2.0);
            const double shuttle =
                phase <= span ? phase : (static_cast<double>(span) * 2.0 - phase);
            frame = start + static_cast<float>(shuttle);
            break;
        }
        case 0:  // kAnimRange
        default:
            (void)persistent;
            frame = start + static_cast<float>(local_frame);
            break;
    }
    return std::clamp(frame, start, end);
}

double venue_filter_duration_seconds(const Gameplay::VenueAnimFilter& filter) {
    const float start = filter.start_frame;
    const float end = std::max(filter.end_frame, start);
    const float span = end - start;
    if (!std::isfinite(span) || span <= 0.001f) return 0.0;
    if (std::isfinite(filter.period) && filter.period > 0.001f)
        return static_cast<double>(filter.period);
    const float scale =
        std::isfinite(filter.scale) && std::fabs(filter.scale) > 0.001f
            ? std::fabs(filter.scale)
            : 1.0f;
    const double cycle =
        filter.type == 2 ? static_cast<double>(span) * 2.0
                         : static_cast<double>(span);
    return cycle / (30.0 * static_cast<double>(scale));
}

std::map<std::string, Gameplay::VenueGroupVisibility>
load_venue_group_visibility(const std::string& hdr_path,
                            const std::string& ark_path,
                            const std::string& milo_path,
                            const ghogx::milo_scene::Scene& scene) {
    std::map<std::string, Gameplay::VenueGroupVisibility> out;
    std::map<std::string, std::vector<std::string>> group_children;
    for (const auto& group : scene.groups) group_children[group.name] = group.children;

    auto add_group_meshes = [&](auto&& self, const std::string& group_name,
                                std::vector<std::string>& meshes,
                                std::unordered_set<std::string>& seen_groups) -> void {
        if (!seen_groups.insert(group_name).second) return;
        const auto group_it = group_children.find(group_name);
        if (group_it == group_children.end()) return;
        for (const auto& child : group_it->second) {
            const auto ref = canonical_milo_ref(child);
            if (ref.rfind(".mesh") != std::string::npos) {
                if (std::find(meshes.begin(), meshes.end(), ref) == meshes.end())
                    meshes.push_back(ref);
            } else if (ref.rfind(".grp") != std::string::npos) {
                self(self, ref, meshes, seen_groups);
            }
        }
    };

    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        for (const auto& de : dir.entries) {
            if (de.type == "Group" && de.offset + de.size <= payload.size()) {
                const uint8_t* body = payload.data() + de.offset;
                const size_t size = static_cast<size_t>(de.size);
                auto& children = group_children[de.name];
                for (const auto& s : ascii_strings_in_object(body, size)) {
                    const auto ref = canonical_milo_ref(s);
                    if (ref.rfind(".mesh") == std::string::npos &&
                        ref.rfind(".grp") == std::string::npos) {
                        continue;
                    }
                    if (std::find(children.begin(), children.end(), ref) ==
                        children.end()) {
                        children.push_back(ref);
                    }
                }
            }
        }

        for (const auto& de : dir.entries) {
            if (de.type != "EventTrigger" || de.offset + de.size > payload.size())
                continue;
            const uint8_t* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            auto strings = scan_milo_strings(body, size);
            if (strings.empty()) continue;
            const std::string event_label = strings.front();
            auto event_end = packed_string_end(body, size, event_label);
            if (!event_end) continue;
            size_t cursor = *event_end;
            uint32_t filter_count = 0;
            if (!read_u32_cursor(body, size, cursor, filter_count) ||
                filter_count > 64) {
                continue;
            }
            bool ok = true;
            for (uint32_t i = 0; i < filter_count; ++i) {
                std::string ignored;
                if (!read_packed_string_cursor(body, size, cursor, ignored)) {
                    ok = false;
                    break;
                }
                // EventTrigger object refs include a nul terminator plus two
                // zero words before the next ref/count field.
                if (cursor + 9 > size) {
                    ok = false;
                    break;
                }
                cursor += 9;
            }
            if (!ok || cursor + 4 > size) continue;
            uint32_t unused = 0;
            read_u32_cursor(body, size, cursor, unused);

            auto read_group_mesh_list =
                [&](std::vector<std::string>& meshes) -> bool {
                uint32_t count = 0;
                if (!read_u32_cursor(body, size, cursor, count) || count > 64)
                    return false;
                for (uint32_t i = 0; i < count; ++i) {
                    std::string group_name;
                    if (!read_packed_string_cursor(body, size, cursor,
                                                   group_name)) {
                        return false;
                    }
                    std::unordered_set<std::string> seen_groups;
                    add_group_meshes(add_group_meshes,
                                     canonical_milo_ref(group_name), meshes,
                                     seen_groups);
                }
                return true;
            };

            Gameplay::VenueGroupVisibility visibility;
            if (!read_group_mesh_list(visibility.show_meshes)) continue;
            if (!read_group_mesh_list(visibility.hide_meshes)) continue;
            if (visibility.show_meshes.empty() && visibility.hide_meshes.empty())
                continue;
            for (const auto& key :
                 event_trigger_route_keys(de.name, event_label)) {
                merge_venue_group_visibility(out[key], visibility);
            }
            if (debug_venue_filters_enabled()) {
                std::fprintf(
                    stderr,
                    "[world] venue EventTrigger visibility %s show=%zu hide=%zu label=%s routes=",
                    de.name.c_str(), visibility.show_meshes.size(),
                    visibility.hide_meshes.size(), event_label.c_str());
                const auto route_keys =
                    event_trigger_route_keys(de.name, event_label);
                for (size_t i = 0; i < route_keys.size(); ++i) {
                    std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                                 route_keys[i].c_str());
                }
                std::fprintf(stderr, "\n");
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] venue visibility load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::map<std::string, std::vector<Gameplay::VenueAnimFilter>>
load_venue_anim_filters(const std::string& hdr_path,
                        const std::string& ark_path,
                        const std::string& milo_path,
                        const ghogx::milo_scene::Scene& scene) {
    std::map<std::string, std::vector<Gameplay::VenueAnimFilter>> out;
    std::map<std::string, std::vector<std::string>> group_children;
    for (const auto& group : scene.groups) group_children[group.name] = group.children;

    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        std::map<std::string, std::string> transanim_mesh;
        std::map<std::string,
                 ghogx::render::MiloSceneRenderer::MeshTransformAnim>
            transanim_anims;
        std::map<std::string, Gameplay::VenueMeshAnim> meshanim_anims;
        std::map<std::string, std::vector<std::string>> event_filters;

        for (const auto& de : dir.entries) {
            if (de.offset + de.size > payload.size()) continue;
            const uint8_t* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            if (de.type == "Group") {
                auto& children = group_children[de.name];
                for (const auto& s : ascii_strings_in_object(body, size)) {
                    const auto ref = canonical_milo_ref(s);
                    const bool object_ref =
                        ref.rfind(".mesh") != std::string::npos ||
                        ref.rfind(".grp") != std::string::npos ||
                        ref.rfind(".tnm") != std::string::npos ||
                        ref.rfind(".msnm") != std::string::npos ||
                        ref.rfind(".meshanim") != std::string::npos;
                    if (!object_ref) continue;
                    if (std::find(children.begin(), children.end(), ref) ==
                        children.end()) {
                        children.push_back(ref);
                    }
                }
            } else if (de.type == "TransAnim") {
                auto target = first_mesh_target_in_transanim(body, size);
                if (!target) continue;
                auto anim = decode_transanim_transform_anim(body, size);
                if (mesh_transform_anim_empty(anim)) continue;
                transanim_mesh[de.name] = canonical_milo_ref(*target);
                if (debug_venue_filters_enabled()) {
                    std::fprintf(
                        stderr,
                        "[world] venue TransAnim %s -> %s pos=%zu rot=%zu scale=%zu\n",
                        de.name.c_str(), target->c_str(),
                        anim.translation_keys.size(), anim.rotation_keys.size(),
                        anim.scale_keys.size());
                }
                transanim_anims[de.name] = std::move(anim);
            } else if (de.type == "MeshAnim") {
                auto anim = decode_venue_mesh_anim(de.name, body, size);
                if (!anim.name.empty()) {
                    meshanim_anims[anim.name] = std::move(anim);
                }
            } else if (de.type == "EventTrigger") {
                const auto strings = scan_milo_strings(body, size);
                const std::string_view event_label =
                    !strings.empty() && is_event_payload_label(strings.front())
                        ? std::string_view(strings.front())
                        : std::string_view{};
                for (const auto& s : strings) {
                    if (s.rfind(".filt") == std::string::npos) continue;
                    for (const auto& key :
                         event_trigger_route_keys(de.name, event_label)) {
                        push_unique_ref(event_filters[key],
                                        canonical_milo_ref(s));
                    }
                }
            }
        }
        for (auto& [name, anim] : meshanim_anims) {
            if (!anim.frames.empty() || anim.keys_owner.empty()) continue;
            const auto owner = meshanim_anims.find(anim.keys_owner);
            if (owner == meshanim_anims.end() || owner->second.frames.empty())
                continue;
            anim.frames = owner->second.frames;
            anim.frame_count = owner->second.frame_count;
            anim.vertex_count = owner->second.vertex_count;
            anim.duration_frames = owner->second.duration_frames;
        }
        if (debug_venue_filters_enabled()) {
            for (const auto& [name, anim] : meshanim_anims) {
                if (anim.frames.empty()) continue;
                std::fprintf(
                    stderr,
                    "[world] venue MeshAnim %s -> %s frames=%u verts=%u duration=%.1f%s%s\n",
                    anim.name.c_str(), anim.mesh.c_str(), anim.frame_count,
                    anim.vertex_count, anim.duration_frames,
                    anim.keys_owner.empty() ? "" : " keys_owner=",
                    anim.keys_owner.c_str());
            }
        }

        std::map<std::string, Gameplay::VenueAnimFilter> filters_by_name;
        for (const auto& de : dir.entries) {
            if (de.type != "AnimFilter" || de.offset + de.size > payload.size())
                continue;
            const uint8_t* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            std::string target_ref;
            std::string target_raw;
            for (const auto& s : scan_milo_strings(body, size)) {
                const auto ref = canonical_milo_ref(s);
                if (ref.rfind(".grp") != std::string::npos ||
                    ref.rfind(".tnm") != std::string::npos ||
                    ref.rfind(".msnm") != std::string::npos ||
                    ref.rfind(".meshanim") != std::string::npos ||
                    ref.rfind(".mesh") != std::string::npos) {
                    target_ref = ref;
                    target_raw = s;
                    break;
                }
            }
            if (target_ref.empty()) continue;

            Gameplay::VenueAnimFilter filter;
            filter.name = de.name;
            if (auto end = packed_string_end(body, size, target_raw)) {
                // PS2 AnimFilter records store scale/period before the frame
                // window, then the ANIM_ENUM type and authored frame offset.
                const size_t timing_off = *end;
                filter.scale = read_f32_or(body, size, timing_off, 1.0f);
                filter.period = read_f32_or(body, size, timing_off + 4, 0.0f);
                filter.start_frame =
                    read_f32_or(body, size, timing_off + 8, 0.0f);
                filter.end_frame = read_f32_or(body, size, timing_off + 12,
                                               filter.start_frame);
                filter.type = read_i32_or(body, size, timing_off + 16, 0);
                filter.offset_frame =
                    read_f32_or(body, size, timing_off + 20, 0.0f);
                if (filter.type < 0 || filter.type > 2) filter.type = 0;
                if (!std::isfinite(filter.offset_frame) ||
                    std::fabs(filter.offset_frame) > 100000.0f) {
                    filter.offset_frame = 0.0f;
                }
                if (!std::isfinite(filter.start_frame) ||
                    filter.start_frame < 0.0f ||
                    filter.start_frame > 100000.0f) {
                    filter.start_frame = 0.0f;
                }
                if (!std::isfinite(filter.end_frame) ||
                    filter.end_frame < 0.0f ||
                    filter.end_frame > 100000.0f) {
                    filter.end_frame = filter.start_frame;
                }
            }

            auto add_transanim = [&](const std::string& tnm) {
                const auto mesh_it = transanim_mesh.find(tnm);
                const auto anim_it = transanim_anims.find(tnm);
                if (mesh_it == transanim_mesh.end() ||
                    anim_it == transanim_anims.end()) {
                    return;
                }
                Gameplay::VenueAnimFilterTarget target;
                target.mesh = mesh_it->second;
                target.anim = anim_it->second;
                filter.targets.push_back(std::move(target));
            };
            auto add_meshanim = [&](const std::string& msnm) {
                const auto anim_it = meshanim_anims.find(msnm);
                if (anim_it == meshanim_anims.end()) return;
                const auto& anim = anim_it->second;
                if (anim.mesh.empty() || anim.frames.empty()) return;
                Gameplay::VenueAnimFilterMeshTarget target;
                target.mesh = anim.mesh;
                target.anim = anim;
                filter.mesh_anim_targets.push_back(std::move(target));
            };
            auto collect = [&](auto&& self, const std::string& ref,
                               std::unordered_set<std::string>& seen) -> void {
                if (!seen.insert(ref).second) return;
                if (ref.rfind(".tnm") != std::string::npos) {
                    add_transanim(ref);
                    return;
                }
                if (ref.rfind(".msnm") != std::string::npos ||
                    ref.rfind(".meshanim") != std::string::npos) {
                    add_meshanim(ref);
                    return;
                }
                if (ref.rfind(".grp") != std::string::npos) {
                    const auto group_it = group_children.find(ref);
                    if (group_it == group_children.end()) return;
                    for (const auto& child : group_it->second) {
                        self(self, canonical_milo_ref(child), seen);
                    }
                }
            };
            std::unordered_set<std::string> seen;
            collect(collect, target_ref, seen);
            if (!filter.targets.empty() || !filter.mesh_anim_targets.empty()) {
                filters_by_name[de.name] = std::move(filter);
            }
        }

        size_t routed = 0;
        for (const auto& [event, filter_names] : event_filters) {
            for (const auto& filter_name : filter_names) {
                const auto filter_it = filters_by_name.find(filter_name);
                if (filter_it == filters_by_name.end()) continue;
                out[event].push_back(filter_it->second);
                ++routed;
            }
        }
        if (!out.empty()) {
            std::fprintf(stderr,
                         "[world] venue AnimFilter transforms loaded %s: %zu events %zu filters\n",
                         milo_path.c_str(), out.size(), routed);
            if (debug_venue_filters_enabled()) {
                for (const auto& [event, filters] : out) {
                    size_t mesh_anim_targets = 0;
                    for (const auto& filter : filters)
                        mesh_anim_targets += filter.mesh_anim_targets.size();
                    std::fprintf(stderr,
                                 "[world] venue AnimFilter event key: %s filters=%zu mesh_anims=%zu\n",
                                 event.c_str(), filters.size(),
                                 mesh_anim_targets);
                }
            }
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] venue AnimFilter load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

struct DrumAnimData {
    std::map<std::string,
             ghogx::render::MiloSceneRenderer::MeshTransformAnim>
        mesh_transform_anims;
    std::map<std::string, std::vector<std::string>> event_mesh_targets;
};

DrumAnimData load_drum_anim_data(const std::string& hdr_path,
                                 const std::string& ark_path,
                                 const std::string& milo_path) {
    DrumAnimData out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        std::map<std::string, std::string> transanim_mesh;
        std::map<std::string, std::vector<std::string>> group_children;
        std::map<std::string, std::vector<std::string>> filter_children;
        std::map<std::string, std::vector<std::string>> trigger_children;
        for (const auto& de : dir.entries) {
            if (de.offset + de.size > payload.size()) {
                continue;
            }
            const uint8_t* body = payload.data() + de.offset;
            const size_t size = static_cast<size_t>(de.size);
            if (de.type == "TransAnim") {
                auto target = first_mesh_target_in_transanim(body, size);
                if (!target) continue;
                transanim_mesh[de.name] = *target;
                auto anim = decode_transanim_transform_anim(body, size);
                if (mesh_transform_anim_empty(anim)) continue;
                std::fprintf(
                    stderr,
                    "[world] drum TransAnim %s -> %s pos=%zu rot=%zu scale=%zu\n",
                    de.name.c_str(), target->c_str(),
                    anim.translation_keys.size(), anim.rotation_keys.size(),
                    anim.scale_keys.size());
                out.mesh_transform_anims[*target] = std::move(anim);
            } else if (de.type == "Group") {
                group_children[de.name] = ascii_strings_in_object(body, size);
            } else if (de.type == "AnimFilter") {
                filter_children[de.name] = ascii_strings_in_object(body, size);
            } else if (de.type == "EventTrigger") {
                trigger_children[de.name] = ascii_strings_in_object(body, size);
            }
        }
        auto mesh_targets_for_child = [&](const std::string& child) {
            std::vector<std::string> targets;
            auto add = [&](const std::string& mesh) {
                if (std::find(targets.begin(), targets.end(), mesh) ==
                    targets.end()) {
                    targets.push_back(mesh);
                }
            };
            if (child.size() > 5 &&
                child.rfind(".mesh") == child.size() - 5) {
                add(child);
            } else if (auto it = transanim_mesh.find(child);
                       it != transanim_mesh.end()) {
                add(it->second);
            } else if (auto group = group_children.find(child);
                       group != group_children.end()) {
                for (const auto& group_child : group->second) {
                    if (group_child.size() > 5 &&
                        group_child.rfind(".mesh") == group_child.size() - 5) {
                        add(group_child);
                    } else if (auto t = transanim_mesh.find(group_child);
                               t != transanim_mesh.end()) {
                        add(t->second);
                    }
                }
            }
            return targets;
        };
        for (const auto& [trigger_name, strings] : trigger_children) {
            std::string event;
            std::vector<std::string> targets;
            for (const auto& s : strings) {
                if (s.size() > 5 && s.rfind(".filt") == s.size() - 5) {
                    auto filter = filter_children.find(s);
                    if (filter == filter_children.end()) continue;
                    for (const auto& child : filter->second) {
                        auto child_targets = mesh_targets_for_child(child);
                        targets.insert(targets.end(), child_targets.begin(),
                                       child_targets.end());
                    }
                } else if (event.empty()) {
                    event = s;
                }
            }
            if (event.empty() || targets.empty()) continue;
            std::sort(targets.begin(), targets.end());
            targets.erase(std::unique(targets.begin(), targets.end()),
                          targets.end());
            out.event_mesh_targets[event] = targets;
            std::fprintf(stderr, "[world] drum trigger %s event=%s targets=",
                         trigger_name.c_str(), event.c_str());
            for (const auto& target : targets)
                std::fprintf(stderr, "%s%s", target.c_str(),
                             &target == &targets.back() ? "" : ",");
            std::fprintf(stderr, "\n");
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] drum anim data load %s: %s\n",
                     milo_path.c_str(), ex.what());
    }
    return out;
}

std::vector<std::pair<Gameplay::CameraKey, size_t>> decode_camshot_poses(
    const uint8_t* body, size_t size) {
    auto f32_at = [&](size_t off) {
        float v = 0.0f;
        std::memcpy(&v, body + off, sizeof(v));
        return v;
    };
    auto finite = [](float v) {
        return std::isfinite(v) && std::abs(v) < 4000.0f;
    };
    auto row_norm = [](const float* r) {
        return std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    };
    auto dot = [](const float* a, const float* b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };

    struct Candidate {
        Gameplay::CameraKey key;
        size_t off = 0;
        float score = 0.0f;
    };
    std::vector<Candidate> candidates;
    for (size_t off = 0; off + 48 <= size; ++off) {
        float r[3][3] = {};
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                r[row][col] = f32_at(off + static_cast<size_t>(row * 12 + col * 4));
                if (!finite(r[row][col])) goto next_offset;
            }
        }
        float pos[3] = {f32_at(off + 36), f32_at(off + 40), f32_at(off + 44)};
        for (float v : pos)
            if (!finite(v)) goto next_offset;

        {
            const float n0 = row_norm(r[0]);
            const float n1 = row_norm(r[1]);
            const float n2 = row_norm(r[2]);
            if (n0 < 0.80f || n0 > 1.20f || n1 < 0.80f || n1 > 1.20f ||
                n2 < 0.80f || n2 > 1.20f) {
                goto next_offset;
            }
            if (std::abs(dot(r[0], r[1])) > 0.35f ||
                std::abs(dot(r[0], r[2])) > 0.35f ||
                std::abs(dot(r[1], r[2])) > 0.35f) {
                goto next_offset;
            }
            const float pos_mag =
                std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
            if (pos_mag < 30.0f || pos_mag > 2500.0f) goto next_offset;

            const float identity =
                std::abs(r[0][0] - 1.0f) + std::abs(r[1][1] - 1.0f) +
                std::abs(r[2][2] - 1.0f) + std::abs(r[0][1]) +
                std::abs(r[0][2]) + std::abs(r[1][0]) +
                std::abs(r[1][2]) + std::abs(r[2][0]) +
                std::abs(r[2][1]);
            Candidate c;
            c.off = off;
            c.score = 10.0f - std::abs(n0 - 1.0f) - std::abs(n1 - 1.0f) -
                      std::abs(n2 - 1.0f);
            if (identity < 0.25f) c.score -= 4.0f;
            c.key.frame = 0.0f;
            c.key.eye[0] = pos[0];
            c.key.eye[1] = pos[1];
            c.key.eye[2] = pos[2];
            c.key.has_quat = false;
            for (int i = 0; i < 3; ++i) {
                c.key.forward[i] = r[1][i];
                c.key.up[i] = r[2][i];
            }
            c.key.has_basis = true;
            if (off >= 4) {
                const float fov = f32_at(off - 4);
                if (std::isfinite(fov) && fov > 0.05f && fov < 2.5f) {
                    c.key.fov = fov;
                    c.key.has_fov = true;
                }
            }
            if (off + 56 <= size) {
                const float sx = f32_at(off + 48);
                const float sy = f32_at(off + 52);
                if (std::isfinite(sx) && std::isfinite(sy) &&
                    std::abs(sx) < 4.0f && std::abs(sy) < 4.0f) {
                    c.key.screen_offset[0] = sx;
                    c.key.screen_offset[1] = sy;
                    c.key.has_screen_offset =
                        std::abs(sx) > 0.0001f || std::abs(sy) > 0.0001f;
                }
            }
            candidates.push_back(c);
        }
    next_offset:
        continue;
    }
    if (candidates.empty()) return {};
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const Candidate& a, const Candidate& b) {
                         return a.off < b.off;
                     });
    std::vector<std::pair<Gameplay::CameraKey, size_t>> out;
    for (const auto& c : candidates) {
        bool duplicate = false;
        for (const auto& prev : out) {
            const auto& key = prev.first;
            const float dx = key.eye[0] - c.key.eye[0];
            const float dy = key.eye[1] - c.key.eye[1];
            const float dz = key.eye[2] - c.key.eye[2];
            const size_t off_delta =
                prev.second > c.off ? prev.second - c.off
                                    : c.off - prev.second;
            if (std::sqrt(dx * dx + dy * dy + dz * dz) < 1.0f &&
                off_delta < 0x40) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) out.push_back({c.key, c.off});
    }
    return out;
}

std::optional<Gameplay::CameraKey> decode_static_camshot_pose(
    const uint8_t* body, size_t size, size_t* selected_off = nullptr) {
    auto poses = decode_camshot_poses(body, size);
    if (poses.empty()) return std::nullopt;
    if (selected_off) *selected_off = poses.front().second;
    return poses.front().first;
}

std::vector<Gameplay::CameraKey> load_regular_camera_keys(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& venue, const VenueCameraPolicy& policy) {
    std::vector<Gameplay::CameraKey> out;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        const std::string milo_path =
            "world/" + venue + "/gen/" + venue + ".milo_ps2";
        auto entry = ark.find(milo_path);
        if (!entry) return out;
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);

        struct Candidate {
            std::string shot;
            std::string distance;
            std::string facing;
            Gameplay::CameraKey key;
            size_t off = 0;
            int score = 0;
        };
        std::vector<Candidate> candidates;
        for (const auto& de : dir.entries) {
            if (de.type != "CamShot" || de.offset + de.size > payload.size())
                continue;
            const uint8_t* body = payload.data() + de.offset;
            auto strings = scan_milo_strings(body, static_cast<size_t>(de.size));
            bool normal_category = false;
            for (const auto& s : strings) {
                if (s == "flr_near_lft" || s == "flr_near_rt" ||
                    s == "flr_far_lft" || s == "flr_far_rt" ||
                    s == "band_POV" || s == "balcony_lft" ||
                    s == "balcony_rt" || s == "SOLO_NEAR" ||
                    s == "SOLO_FAR") {
                    normal_category = true;
                }
                if (s == "INTRO" || s == "INTRO_FAST" ||
                    s == "INTRO_ENCORE" || s == "LIGHTER") {
                    normal_category = false;
                    break;
                }
            }
            if (!normal_category) continue;

            const std::string special = next_string_after(strings, "special");
            if (special == "TRUE") continue;
            const std::string solo = next_string_after(strings, "solo");
            if (!solo.empty() && solo != "ok" && solo != "never") continue;
            auto decoded_poses =
                decode_camshot_poses(body, static_cast<size_t>(de.size));
            if (decoded_poses.empty()) continue;
            if (debug_camera_enabled()) {
                for (const auto& pose : decoded_poses) {
                    const auto& key = pose.first;
                    std::fprintf(
                        stderr,
                        "[camera-candidate] shot=%s off=0x%zX eye=(%.2f %.2f %.2f) forward=(%.3f %.3f %.3f) up=(%.3f %.3f %.3f) fov=%s%.3f\n",
                        de.name.c_str(), pose.second, key.eye[0], key.eye[1],
                        key.eye[2], key.forward[0], key.forward[1],
                        key.forward[2], key.up[0], key.up[1], key.up[2],
                        key.has_fov ? "" : "none/",
                        key.has_fov ? key.fov : 0.0f);
                    if (key.has_screen_offset) {
                        std::fprintf(stderr,
                                     "[camera-candidate] shot=%s off=0x%zX screen_offset=(%.6f %.6f)\n",
                                     de.name.c_str(), pose.second,
                                     key.screen_offset[0],
                                     key.screen_offset[1]);
                    }
                }
            }
            size_t pose_off = decoded_poses.front().second;

            Candidate c;
            c.shot = de.name;
            c.distance = next_string_after(strings, "distance");
            c.facing = next_string_after(strings, "facing");
            c.key = decoded_poses.front().first;
            c.key.distance = c.distance;
            c.key.facing = c.facing;
            c.key.solo = next_string_after(strings, "solo");
            c.key.special = camshot_bool_property(
                body, static_cast<size_t>(de.size), strings, "special", false);
            c.key.walk_ok = camshot_bool_property(
                body, static_cast<size_t>(de.size), strings, "walk_ok", true);
            c.key.starpower_ok =
                camshot_bool_property(body, static_cast<size_t>(de.size),
                                      strings, "starpower_ok", false);
            c.key.low_excitement_ok =
                camshot_bool_property(body, static_cast<size_t>(de.size),
                                      strings, "low_excitement_ok", true);
            infer_camshot_target(strings, c.shot, c.key);
            for (auto& decoded_pose : decoded_poses) {
                Gameplay::CameraKey pos = decoded_pose.first;
                pos.name = c.shot;
                pos.distance = c.key.distance;
                pos.facing = c.key.facing;
                pos.solo = c.key.solo;
                pos.special = c.key.special;
                pos.walk_ok = c.key.walk_ok;
                pos.starpower_ok = c.key.starpower_ok;
                pos.low_excitement_ok = c.key.low_excitement_ok;
                pos.target_entity = c.key.target_entity;
                pos.target_subpart = c.key.target_subpart;
                c.key.positions.push_back(std::move(pos));
            }
            c.off = pose_off;
            if (policy.intro_facing == "left") {
                if (c.facing == "right") c.score += 4;
                if (c.facing == "null" || c.facing.empty()) c.score += 2;
                if (c.facing == "left") c.score -= 4;
            } else if (policy.intro_facing == "right") {
                if (c.facing == "left") c.score += 4;
                if (c.facing == "null" || c.facing.empty()) c.score += 2;
                if (c.facing == "right") c.score -= 4;
            }
            if (policy.intro_distance == "far" ||
                policy.intro_distance == "behind") {
                if (c.distance == "near" || c.distance == "closeup" ||
                    c.distance == "null" || c.distance.empty()) {
                    c.score += 2;
                }
                if (c.distance == policy.intro_distance) c.score -= 2;
            }
            if (c.shot.rfind("flr_", 0) == 0) c.score += 1;
            candidates.push_back(std::move(c));
        }
        if (candidates.empty()) return out;
        std::stable_sort(candidates.begin(), candidates.end(),
                         [](const Candidate& a, const Candidate& b) {
                             if (a.score != b.score) return a.score > b.score;
                             return a.shot < b.shot;
                         });
        out.reserve(candidates.size());
        for (const auto& c : candidates) {
            Gameplay::CameraKey key = c.key;
            key.name = c.shot;
            key.frame = 0.0f;
            out.push_back(key);
            std::fprintf(stderr,
                         "[world] regular CamShot %s distance=%s facing=%s target=%s:%s poses=%zu pose body+0x%zX score=%d special=%d walk_ok=%d low_excitement_ok=%d starpower_ok=%d\n",
                         c.shot.c_str(), c.distance.c_str(), c.facing.c_str(),
                         key.target_entity.c_str(), key.target_subpart.c_str(),
                         key.positions.size(), c.off, c.score,
                         key.special ? 1 : 0, key.walk_ok ? 1 : 0,
                         key.low_excitement_ok ? 1 : 0,
                         key.starpower_ok ? 1 : 0);
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] regular camera select: %s\n", ex.what());
    }
    return out;
}

double intro_camera_duration_seconds(const ghogx::chart::Chart& chart) {
    // world_objects_worldbase.dta intro_start_msg sets camera_bars_left to 6.
    // The downbeat handler decrements it once per bar before regular camera
    // selection resumes, so convert six 4/4 bars through the MIDI tempo map.
    if (chart.ticks_per_beat == 0) return 0.0;
    const uint32_t intro_ticks = chart.ticks_per_beat * 4u * 6u;
    return chart.tick_to_sec(intro_ticks);
}

size_t authored_section_slot_at(const ghogx::chart::Chart& chart,
                                double song_time) {
    size_t slot = 0;
    for (const auto& ev : chart.text_events) {
        const double t = chart.tick_to_sec(ev.tick);
        if (t > song_time) break;
        if (ev.text.rfind("[section ", 0) == 0) ++slot;
    }
    return slot;
}

const Gameplay::CameraKey* choose_regular_camera_key(
    const std::vector<Gameplay::CameraKey>& keys,
    const ghogx::chart::Chart& chart,
    double song_time) {
    if (keys.empty()) return nullptr;
    const size_t slot = authored_section_slot_at(chart, song_time);
    return &keys[slot % keys.size()];
}

const Gameplay::CameraKey* choose_regular_camera_key_by_counter(
    const std::vector<Gameplay::CameraKey>& keys, size_t counter) {
    if (keys.empty()) return nullptr;
    return &keys[counter % keys.size()];
}

bool string_in(std::string_view value,
               std::initializer_list<std::string_view> allowed) {
    for (std::string_view s : allowed) {
        if (value == s) return true;
    }
    return false;
}

bool regular_camera_filter_ok(const Gameplay::CameraKey& key,
                              const Gameplay::CameraKey* previous,
                              bool low_excitement,
                              bool walking,
                              bool starpower) {
    // Community world_objects_worldbase.dta::pick_regular_camera_shot:
    // alternate away from previous facing, avoid repeating far/behind
    // distance, require solo ok/never and special FALSE, and only apply
    // low/walk/starpower filters when the script pushes those predicates.
    if (key.special) return false;
    if (!string_in(key.solo, {"", "ok", "never"})) return false;
    if (low_excitement && !key.low_excitement_ok) return false;
    if (walking && !key.walk_ok) return false;
    if (starpower && !key.starpower_ok) return false;

    if (previous) {
        if (previous->facing == "left" &&
            !string_in(key.facing, {"right", "null", ""})) {
            return false;
        }
        if (previous->facing == "right" &&
            !string_in(key.facing, {"left", "null", ""})) {
            return false;
        }
        if (previous->distance == "far" || previous->distance == "behind") {
            if (!string_in(key.distance, {"null", "near", "closeup", ""})) {
                return false;
            }
        }
    }
    return true;
}

const Gameplay::CameraKey* choose_regular_camera_key_scripted(
    const std::vector<Gameplay::CameraKey>& keys,
    const Gameplay::CameraKey* previous,
    size_t counter,
    bool low_excitement,
    bool walking,
    bool starpower) {
    if (keys.empty()) return nullptr;
    std::vector<const Gameplay::CameraKey*> filtered;
    for (const auto& key : keys) {
        if (&key == previous) continue;
        if (regular_camera_filter_ok(key, previous, low_excitement, walking,
                                     starpower)) {
            filtered.push_back(&key);
        }
    }
    if (filtered.empty()) {
        for (const auto& key : keys) {
            if (&key != previous) filtered.push_back(&key);
        }
    }
    if (filtered.empty()) return &keys[counter % keys.size()];
    return filtered[counter % filtered.size()];
}

uint32_t camera_bar_at(const ghogx::chart::Chart& chart, double song_time) {
    if (chart.ticks_per_beat == 0) return 0;
    const uint32_t ticks_per_bar = chart.ticks_per_beat * 4u;
    if (ticks_per_bar == 0) return 0;
    return chart.sec_to_tick(song_time) / ticks_per_bar;
}

int deterministic_camera_duration_bars(int min_bars, int max_bars,
                                       size_t counter) {
    if (min_bars <= 0) min_bars = 1;
    if (max_bars < min_bars) max_bars = min_bars;
    const int span = max_bars - min_bars + 1;
    return min_bars + static_cast<int>(counter % static_cast<size_t>(span));
}

std::string camera_excitement_duration_key(std::string_view venue_event) {
    if (venue_event.find("peak") != std::string_view::npos)
        return "kExcitementPeak";
    if (venue_event.find("great") != std::string_view::npos)
        return "kExcitementGreat";
    if (venue_event.find("bad") != std::string_view::npos)
        return "kExcitementBad";
    if (venue_event.find("boot") != std::string_view::npos)
        return "kExcitementBoot";
    return "kExcitementOkay";
}

std::pair<std::string, std::pair<int, int>> camera_duration_range_for_event(
    const std::map<std::string, std::pair<int, int>>& duration_bars,
    std::string_view venue_event) {
    std::string key = camera_excitement_duration_key(venue_event);
    auto it = duration_bars.find(key);
    if (it != duration_bars.end()) return {key, it->second};
    it = duration_bars.find("kExcitementOkay");
    if (it != duration_bars.end()) return {"kExcitementOkay", it->second};
    return {"kExcitementOkay", {2, 4}};
}

const Gameplay::CameraKey* find_camera_key_by_name(
    const std::vector<Gameplay::CameraKey>& keys, std::string_view name) {
    for (const auto& key : keys) {
        if (key.name == name) return &key;
    }
    return nullptr;
}

Gameplay::CameraKey camera_position_for(const Gameplay::CameraKey& shot,
                                        size_t index) {
    if (shot.positions.empty()) return shot;
    Gameplay::CameraKey out =
        shot.positions[index % shot.positions.size()];
    out.positions = shot.positions;
    return out;
}

std::vector<Gameplay::CameraKey> regular_camera_sweep_keys(
    const Gameplay::CameraKey& current,
    const Gameplay::CameraKey* previous,
    double song_time,
    double start_time) {
    constexpr double kSweepSeconds = 1.25;
    std::vector<Gameplay::CameraKey> keys;
    if (!previous || song_time < start_time ||
        song_time >= start_time + kSweepSeconds) {
        keys.push_back(current);
        return keys;
    }
    Gameplay::CameraKey a = *previous;
    Gameplay::CameraKey b = current;
    a.frame = static_cast<float>(start_time * 30.0);
    b.frame = static_cast<float>((start_time + kSweepSeconds) * 30.0);
    keys.push_back(a);
    keys.push_back(b);
    return keys;
}

std::array<float, 3> camera_authored_at_for_key(
    const Gameplay::CameraKey& key,
    const std::unordered_map<std::string, std::array<float, 3>>& targets,
    const float eye[3]) {
    if (!key.target_entity.empty()) {
        auto it = targets.find(
            camera_target_id(key.target_entity, key.target_subpart));
        if (it == targets.end() && !key.target_subpart.empty()) {
            it = targets.find(camera_target_id(key.target_entity, {}));
        }
        if (it != targets.end()) return it->second;
    }
    if (key.has_quat) {
        float q[4] = {key.quat[0], key.quat[1], key.quat[2], key.quat[3]};
        const float n =
            std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (n > 0.0001f) {
            for (float& v : q) v /= n;
        }
        const float x = q[0], y = q[1], z = q[2], w = q[3];
        return {eye[0] + 2.0f * (x * y - z * w) * 100.0f,
                eye[1] + (1.0f - 2.0f * (x * x + z * z)) * 100.0f,
                eye[2] + 2.0f * (y * z + x * w) * 100.0f};
    }
    if (key.has_basis) {
        return {eye[0] + key.forward[0] * 100.0f,
                eye[1] + key.forward[1] * 100.0f,
                eye[2] + key.forward[2] * 100.0f};
    }
    return {0.0f, -80.0f, -330.0f};
}

std::optional<std::array<float, 3>> camera_target_for_key(
    const Gameplay::CameraKey& key,
    const std::unordered_map<std::string, std::array<float, 3>>& targets) {
    if (key.target_entity.empty()) return std::nullopt;
    auto it = targets.find(camera_target_id(key.target_entity,
                                            key.target_subpart));
    if (it == targets.end() && !key.target_subpart.empty()) {
        it = targets.find(camera_target_id(key.target_entity, {}));
    }
    if (it == targets.end()) return std::nullopt;
    return it->second;
}

std::array<float, 3> camera_authored_eye_for_key(
    const Gameplay::CameraKey& key,
    const std::unordered_map<std::string, std::array<float, 3>>& targets) {
    std::array<float, 3> eye = {key.eye[0], key.eye[1], key.eye[2]};
    const bool body_bone_source =
        key.target_subpart.rfind("bone_", 0) == 0;
    if (body_bone_source) {
        const auto target = camera_target_for_key(key, targets);
        if (!target) return eye;
        // Accepted PS2 CamShot traces split the moving path frame from the
        // final result frame. The result-frame translation is the path-frame
        // camera offset resolved through the live body-bone source transform.
        // Prop/spot targets stay aim-only until their camera-source transform
        // branch is mapped. Guitar/bass prop target positions are now
        // validated for focus targets, but not for path-frame eye offsets.
        eye[0] += (*target)[0];
        eye[1] += (*target)[1];
        eye[2] += (*target)[2];
    }
    return eye;
}

void camera_authored_up_for_key(const Gameplay::CameraKey& key, float out[3]) {
    if (key.has_quat) {
        float q[4] = {key.quat[0], key.quat[1], key.quat[2], key.quat[3]};
        const float n =
            std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (n > 0.0001f) {
            for (float& v : q) v /= n;
        }
        const float x = q[0], y = q[1], z = q[2], w = q[3];
        out[0] = 2.0f * (x * z + y * w);
        out[1] = 2.0f * (y * z - x * w);
        out[2] = 1.0f - 2.0f * (x * x + y * y);
        return;
    }
    if (key.has_basis) {
        out[0] = key.up[0];
        out[1] = key.up[1];
        out[2] = key.up[2];
        return;
    }
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 1.0f;
}

void apply_camera_keys(
    ghogx::render::OrbitCamera& cam,
    const std::vector<Gameplay::CameraKey>& keys,
    double song_time,
    const std::unordered_map<std::string, std::array<float, 3>>& targets = {}) {
    if (keys.empty()) return;
    const float frame = static_cast<float>(song_time * 30.0);
    const Gameplay::CameraKey* a = &keys.front();
    const Gameplay::CameraKey* b = &keys.back();
    for (size_t i = 1; i < keys.size(); ++i) {
        if (keys[i].frame >= frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
        }
    }
    float t = 0.0f;
    if (b->frame > a->frame) t = (frame - a->frame) / (b->frame - a->frame);
    t = std::clamp(t, 0.0f, 1.0f);
    cam.authored = true;
    float eye_a[3] = {};
    float eye_b[3] = {};
    const auto authored_eye_a = camera_authored_eye_for_key(*a, targets);
    const auto authored_eye_b = camera_authored_eye_for_key(*b, targets);
    for (int i = 0; i < 3; ++i) {
        eye_a[i] = authored_eye_a[i];
        eye_b[i] = authored_eye_b[i];
        cam.authored_eye[i] = eye_a[i] + (eye_b[i] - eye_a[i]) * t;
    }
    const auto at_a = camera_authored_at_for_key(*a, targets, eye_a);
    const auto at_b = camera_authored_at_for_key(*b, targets, eye_b);
    for (int i = 0; i < 3; ++i)
        cam.authored_at[i] = at_a[i] + (at_b[i] - at_a[i]) * t;
    float up_a[3] = {};
    float up_b[3] = {};
    camera_authored_up_for_key(*a, up_a);
    camera_authored_up_for_key(*b, up_b);
    for (int i = 0; i < 3; ++i)
        cam.authored_up[i] = up_a[i] + (up_b[i] - up_a[i]) * t;
    if (a->has_fov || b->has_fov) {
        const float fov_a = a->has_fov ? a->fov : (b->has_fov ? b->fov : cam.fov);
        const float fov_b = b->has_fov ? b->fov : fov_a;
        cam.fov = fov_a + (fov_b - fov_a) * t;
    }
    const float sx_a = a->has_screen_offset ? a->screen_offset[0] : 0.0f;
    const float sy_a = a->has_screen_offset ? a->screen_offset[1] : 0.0f;
    const float sx_b = b->has_screen_offset ? b->screen_offset[0] : 0.0f;
    const float sy_b = b->has_screen_offset ? b->screen_offset[1] : 0.0f;
    cam.screen_offset[0] = sx_a + (sx_b - sx_a) * t;
    cam.screen_offset[1] = sy_a + (sy_b - sy_a) * t;
    cam.near_z = 1.0f;
    cam.far_z = 6000.0f;
    if (debug_camera_enabled()) {
        std::fprintf(
            stderr,
            "[camera] frame=%.2f t=%.3f a=%s(%.2f) b=%s(%.2f) "
            "eye=(%.2f %.2f %.2f) at=(%.2f %.2f %.2f) "
            "up=(%.3f %.3f %.3f) fov=%.3f screen_offset=(%.6f %.6f) targets=%zu\n",
            frame, t, a->name.c_str(), a->frame, b->name.c_str(), b->frame,
            cam.authored_eye[0], cam.authored_eye[1], cam.authored_eye[2],
            cam.authored_at[0], cam.authored_at[1], cam.authored_at[2],
            cam.authored_up[0], cam.authored_up[1], cam.authored_up[2],
            cam.fov, cam.screen_offset[0], cam.screen_offset[1],
            targets.size());
    }
}

struct NoteCue {
    bool active = false;
    uint32_t tick = 0;
    uint32_t mask = 0;
    double length = 0.0;
};

struct FretPositionState {
    bool active = false;
    uint32_t tick = UINT32_MAX;
    int spot_index = 0;
    std::string spot_name;
};

// Player difficulty owns scoring/highway only. Performer hand scheduling uses
// the highest authored note lane for the song so character animation does not
// change when the player selects Easy/Medium/Hard/Expert.
size_t performer_chart_lane_index(
    const std::vector<ghogx::chart::Note> (&lanes)[4]) {
    for (int i = 3; i >= 0; --i) {
        if (!lanes[static_cast<size_t>(i)].empty())
            return static_cast<size_t>(i);
    }
    return 0;
}

const std::vector<ghogx::chart::Note>& performer_chart_notes(
    const std::vector<ghogx::chart::Note> (&lanes)[4]) {
    return lanes[performer_chart_lane_index(lanes)];
}

size_t performer_hand_cue_lane_index(
    const std::vector<ghogx::chart::HandGemCue> (&lanes)[4]) {
    for (int i = 3; i >= 0; --i) {
        if (!lanes[static_cast<size_t>(i)].empty())
            return static_cast<size_t>(i);
    }
    return 0;
}

const std::vector<ghogx::chart::HandGemCue>& performer_hand_cues(
    const std::vector<ghogx::chart::HandGemCue> (&lanes)[4]) {
    return lanes[performer_hand_cue_lane_index(lanes)];
}

NoteCue current_note_cue(double song_time, const ghogx::chart::Chart& chart,
                         const std::vector<ghogx::chart::Note>& notes) {
    NoteCue cue;
    for (const auto& note : notes) {
        const double dt = chart.tick_to_sec(note.tick_on) - song_time;
        if (dt > 0.12) break;
        if (std::abs(dt) <= 0.08) {
            if (!cue.active || note.tick_on < cue.tick) {
                cue.active = true;
                cue.tick = note.tick_on;
                cue.mask = 0;
                cue.length = 0.0;
            }
            if (cue.active && note.tick_on == cue.tick) {
                cue.mask |= 1u << std::clamp(note.lane, 0, 4);
                const double on = chart.tick_to_sec(note.tick_on);
                const double off = std::max(on, chart.tick_to_sec(note.tick_off));
                cue.length = std::max(cue.length, off - on);
            }
        }
    }
    return cue;
}

NoteCue performer_animation_note_cue(
    double song_time, const ghogx::chart::Chart& chart,
    const std::vector<ghogx::chart::Note>& notes) {
    NoteCue cue;
    enum class Rank { kNone = 3, kSustain = 0, kUpcoming = 1, kRelease = 2 };
    Rank best_rank = Rank::kNone;
    double best_delta = 1.0e9;
    constexpr double kLookaheadSec = 0.35;
    constexpr double kReleaseSec = 0.25;

    for (const auto& note : notes) {
        const double on = chart.tick_to_sec(note.tick_on);
        const double off = std::max(on, chart.tick_to_sec(note.tick_off));
        if (off + kReleaseSec < song_time) continue;
        if (on - kLookaheadSec > song_time && cue.active) break;

        Rank rank = Rank::kNone;
        double delta = 0.0;
        if (song_time >= on && song_time <= off) {
            rank = Rank::kSustain;
            delta = 0.0;
        } else if (song_time < on && on - song_time <= kLookaheadSec) {
            rank = Rank::kUpcoming;
            delta = on - song_time;
        } else if (song_time > off && song_time - off <= kReleaseSec) {
            rank = Rank::kRelease;
            delta = song_time - off;
        } else {
            continue;
        }

        if (!cue.active || rank < best_rank ||
            (rank == best_rank && delta < best_delta) ||
            (rank == best_rank && delta == best_delta &&
             note.tick_on < cue.tick)) {
            cue.active = true;
            cue.tick = note.tick_on;
            cue.mask = 0;
            cue.length = 0.0;
            best_rank = rank;
            best_delta = delta;
        }
        if (cue.active && note.tick_on == cue.tick) {
            cue.mask |= 1u << std::clamp(note.lane, 0, 4);
            cue.length = std::max(cue.length, off - on);
        }
    }
    return cue;
}

NoteCue current_fret_hand_cue(
    double song_time, const ghogx::chart::Chart& chart,
    const std::vector<ghogx::chart::HandGemCue>& cues) {
    NoteCue cue;
    if (cues.empty()) return cue;
    constexpr double kMaxGapSec = 0.24;
    const uint32_t now_tick = chart.sec_to_tick(song_time);
    const ghogx::chart::HandGemCue* chosen = nullptr;
    for (const auto& candidate : cues) {
        if (candidate.tick > now_tick) break;
        chosen = &candidate;
    }
    if (!chosen) return cue;
    const double on = chart.tick_to_sec(chosen->tick);
    const double off = std::max(on, chart.tick_to_sec(chosen->tick_off));
    if (song_time > off + kMaxGapSec) return cue;
    cue.active = true;
    cue.tick = chosen->tick;
    cue.mask = chosen->mask & 0x1fu;
    cue.length = chosen->length;
    return cue;
}

FretPositionState current_fret_position_state(
    double song_time, const ghogx::chart::Chart& chart,
    const std::vector<ghogx::chart::FretPositionCue>& cues) {
    FretPositionState state;
    if (cues.empty()) return state;
    const uint32_t now_tick = chart.sec_to_tick(song_time);
    const ghogx::chart::FretPositionCue* chosen = nullptr;
    for (const auto& cue : cues) {
        if (cue.tick > now_tick) break;
        if (cue.tick <= now_tick) {
            chosen = &cue;
        }
    }
    if (!chosen) return state;
    state.active = true;
    state.tick = chosen->tick;
    state.spot_index = chosen->spot_index;
    char name[32] = {};
    std::snprintf(name, sizeof(name), "spot_neck_fret%02d.mesh",
                  std::clamp(chosen->spot_index, 1, 20));
    state.spot_name = name;
    return state;
}

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() &&
           text.substr(0, prefix.size()) == prefix;
}

void push_unique(std::vector<std::string>& out, std::string value) {
    if (value.empty()) return;
    if (std::find(out.begin(), out.end(), value) == out.end())
        out.push_back(std::move(value));
}

std::optional<std::string> dtb_atom_text(const gh::dtb::Node& node) {
    if (auto s = gh::dtb::as_string(node)) return *s;
    if (auto i = gh::dtb::as_int(node)) return std::to_string(*i);
    return std::nullopt;
}

void collect_clip_names_with_prefix(const gh::dtb::Node& node,
                                    std::string_view prefix,
                                    std::vector<std::string>& out) {
    if (auto text = dtb_atom_text(node)) {
        if (starts_with(*text, prefix)) push_unique(out, *text);
        return;
    }
    if (!gh::dtb::is_array(node)) return;
    for (const auto& child : gh::dtb::children(node)) {
        if (child) collect_clip_names_with_prefix(*child, prefix, out);
    }
}

std::optional<double> find_length_threshold(const gh::dtb::Node& node) {
    if (auto f = gh::dtb::as_float(node)) return static_cast<double>(*f);
    if (auto i = gh::dtb::as_int(node)) return static_cast<double>(*i);
    if (!gh::dtb::is_array(node)) return std::nullopt;
    std::optional<double> found;
    for (const auto& child : gh::dtb::children(node)) {
        if (!child) continue;
        if (auto value = find_length_threshold(*child)) found = value;
    }
    return found;
}

Gameplay::HandClipChoice parse_prefixed_clip_choice(
    const gh::dtb::Node& node, std::string_view prefix) {
    Gameplay::HandClipChoice choice;
    if (gh::dtb::is_array(node)) {
        const auto& kids = gh::dtb::children(node);
        if (kids.size() >= 4) {
            const std::string head =
                kids[0] ? gh::dtb::as_string(*kids[0]).value_or("") : "";
            if (head == "if_else") {
                if (kids[1]) {
                    if (auto threshold = find_length_threshold(*kids[1]))
                        choice.length_threshold = *threshold;
                }
                collect_clip_names_with_prefix(*kids[2], prefix,
                                               choice.long_names);
                collect_clip_names_with_prefix(*kids[3], prefix,
                                               choice.short_names);
                if (choice.short_names.empty())
                    choice.short_names = choice.long_names;
                if (choice.long_names.empty())
                    choice.long_names = choice.short_names;
                return choice;
            }
        }
    }

    collect_clip_names_with_prefix(node, prefix, choice.short_names);
    choice.long_names = choice.short_names;
    return choice;
}

Gameplay::HandClipChoice parse_hand_clip_choice(const gh::dtb::Node& node) {
    return parse_prefixed_clip_choice(node, "finger_");
}

std::vector<int> parse_hand_chord_key(const gh::dtb::Node& node) {
    std::vector<int> keys;
    if (!gh::dtb::is_array(node)) return keys;
    for (const auto& child : gh::dtb::children(node)) {
        if (!child) continue;
        if (auto i = gh::dtb::as_int(*child)) keys.push_back(*i);
    }
    return keys;
}

void parse_hand_map_node(const gh::dtb::Node& node,
                         std::map<std::string, Gameplay::FretHandMap>& maps) {
    if (!gh::dtb::is_array(node)) return;
    const auto& kids = gh::dtb::children(node);
    if (kids.empty()) return;
    const std::string name =
        kids[0] ? gh::dtb::as_string(*kids[0]).value_or("") : "";
    if (!starts_with(name, "HandMap_")) return;

    Gameplay::FretHandMap map;
    map.name = name;
    if (auto events = gh::dtb::find_keyed(node, "events")) {
        const auto& event_rows = gh::dtb::children(*events);
        for (size_t i = 1; i < event_rows.size(); ++i) {
            const auto& row = event_rows[i];
            if (!row || !gh::dtb::is_array(*row)) continue;
            const auto& row_kids = gh::dtb::children(*row);
            if (row_kids.size() < 2 || !row_kids[0] || !row_kids[1])
                continue;
            const auto choice = parse_hand_clip_choice(*row_kids[1]);
            if (choice.short_names.empty() && choice.long_names.empty())
                continue;
            if (auto key = gh::dtb::as_int(*row_kids[0])) {
                if (*key >= 1 && *key <= 5)
                    map.single[(size_t)(*key - 1)] = choice;
            } else if (gh::dtb::is_array(*row_kids[0])) {
                Gameplay::HandChordRule rule;
                rule.keys = parse_hand_chord_key(*row_kids[0]);
                rule.choice = choice;
                map.chords.push_back(std::move(rule));
            }
        }
    }
    maps[map.name] = std::move(map);
}

void parse_strum_map_node(
    const gh::dtb::Node& node,
    std::map<std::string, Gameplay::StrumHandMap>& maps) {
    if (!gh::dtb::is_array(node)) return;
    const auto& kids = gh::dtb::children(node);
    if (kids.empty()) return;
    const std::string name =
        kids[0] ? gh::dtb::as_string(*kids[0]).value_or("") : "";
    if (!starts_with(name, "StrumMap_")) return;

    Gameplay::StrumHandMap map;
    map.name = name;
    if (auto events = gh::dtb::find_keyed(node, "events")) {
        const auto& event_rows = gh::dtb::children(*events);
        for (size_t i = 1; i < event_rows.size(); ++i) {
            const auto& row = event_rows[i];
            if (!row || !gh::dtb::is_array(*row)) continue;
            const auto& row_kids = gh::dtb::children(*row);
            if (row_kids.size() < 2 || !row_kids[0] || !row_kids[1])
                continue;
            auto choice = parse_prefixed_clip_choice(*row_kids[1], "strum_");
            if (choice.short_names.empty() && choice.long_names.empty())
                continue;
            const std::string key =
                gh::dtb::as_string(*row_kids[0]).value_or("");
            if (key == "default") {
                map.fallback = std::move(choice);
            } else if (gh::dtb::is_array(*row_kids[0]) &&
                       gh::dtb::children(*row_kids[0]).empty()) {
                map.regular = std::move(choice);
            }
        }
    }
    if (map.regular.short_names.empty() && map.regular.long_names.empty())
        map.regular = map.fallback;
    if (map.fallback.short_names.empty() && map.fallback.long_names.empty())
        map.fallback = map.regular;
    if (!map.regular.short_names.empty() || !map.regular.long_names.empty())
        maps[map.name] = std::move(map);
}

void collect_hand_maps_recursive(
    const gh::dtb::Node& node,
    std::map<std::string, Gameplay::FretHandMap>& maps) {
    parse_hand_map_node(node, maps);
    if (!gh::dtb::is_array(node)) return;
    for (const auto& child : gh::dtb::children(node)) {
        if (child) collect_hand_maps_recursive(*child, maps);
    }
}

void collect_strum_maps_recursive(
    const gh::dtb::Node& node,
    std::map<std::string, Gameplay::StrumHandMap>& maps) {
    parse_strum_map_node(node, maps);
    if (!gh::dtb::is_array(node)) return;
    for (const auto& child : gh::dtb::children(node)) {
        if (child) collect_strum_maps_recursive(*child, maps);
    }
}

std::map<std::string, Gameplay::FretHandMap> load_fret_hand_maps(
    const std::string& hdr_path, const std::string& ark_path) {
    std::map<std::string, Gameplay::FretHandMap> maps;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find("config/gen/midi_parsers.dtb");
        if (!entry) return maps;
        auto tree = gh::dtb::parse(ark.read_entry(*entry, {ark_path}));
        for (const auto& root : tree.root) {
            if (root) collect_hand_maps_recursive(*root, maps);
        }
        std::fprintf(stderr, "[world] loaded %zu HandMap fret mappings\n",
                     maps.size());
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] config/gen/midi_parsers.dtb: %s\n",
                     ex.what());
    }
    return maps;
}

std::map<std::string, Gameplay::StrumHandMap> load_strum_hand_maps(
    const std::string& hdr_path, const std::string& ark_path) {
    std::map<std::string, Gameplay::StrumHandMap> maps;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find("config/gen/midi_parsers.dtb");
        if (!entry) return maps;
        auto tree = gh::dtb::parse(ark.read_entry(*entry, {ark_path}));
        for (const auto& root : tree.root) {
            if (root) collect_strum_maps_recursive(*root, maps);
        }
        std::fprintf(stderr, "[world] loaded %zu StrumMap mappings\n",
                     maps.size());
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[world] config/gen/midi_parsers.dtb: %s\n",
                     ex.what());
    }
    return maps;
}

std::vector<std::string> names_for_hand_choice(
    const Gameplay::HandClipChoice& choice, double note_length) {
    const std::vector<std::string>& preferred =
        note_length > choice.length_threshold ? choice.long_names
                                              : choice.short_names;
    if (!preferred.empty()) return preferred;
    return note_length > choice.length_threshold ? choice.short_names
                                                 : choice.long_names;
}

std::vector<std::string> scheduled_hand_choice_names(
    const std::vector<std::string>& candidates, size_t child_index) {
    if (candidates.size() <= 1) return candidates;
    return {candidates[child_index % candidates.size()]};
}

struct FretClipSelection {
    std::vector<std::string> choices;
    std::vector<std::string> selected;
};

FretClipSelection make_fret_clip_selection(std::vector<std::string> choices,
                                           bool schedule_children,
                                           size_t child_index) {
    FretClipSelection selection;
    selection.choices = std::move(choices);
    selection.selected = schedule_children
                             ? scheduled_hand_choice_names(selection.choices,
                                                           child_index)
                             : selection.choices;
    return selection;
}

int first_lane_from_mask(uint32_t note_mask) {
    for (int lane = 0; lane < 5; ++lane) {
        if ((note_mask & (1u << lane)) != 0) return lane;
    }
    return -1;
}

std::string player_fret_hit_event(int lane) {
    const int fret = std::clamp(lane, 0, 4) + 1;
    return "hit_p0_fret" + std::to_string(fret);
}

std::vector<int> event_keys_from_mask(uint32_t note_mask) {
    std::vector<int> keys;
    for (int lane = 0; lane < 5; ++lane) {
        if ((note_mask & (1u << lane)) != 0) keys.push_back(lane + 1);
    }
    return keys;
}

bool chord_rule_exact_matches(const Gameplay::HandChordRule& rule,
                              const std::vector<int>& chord_keys) {
    return rule.keys.size() > 1 && rule.keys == chord_keys;
}

bool chord_rule_root_bucket_matches(const Gameplay::HandChordRule& rule,
                                    int first_event_key) {
    return rule.keys.size() == 1 && rule.keys.front() == first_event_key;
}

std::vector<std::string> names_for_chord_rule(
    const Gameplay::HandChordRule& rule, double note_length) {
    return names_for_hand_choice(rule.choice, note_length);
}

FretClipSelection fret_clip_selection_for_note(
    const std::map<std::string, Gameplay::FretHandMap>& maps,
    const std::string& hand_map_name, uint32_t note_mask, double note_length,
    size_t child_index) {
    auto it = maps.find(hand_map_name);
    if (it == maps.end()) it = maps.find("HandMap_Default");
    if (it == maps.end() || (note_mask & 0x1fu) == 0)
        return make_fret_clip_selection({"finger_open"}, false, child_index);

    const Gameplay::FretHandMap& map = it->second;
    const int first_lane = first_lane_from_mask(note_mask);
    if (first_lane < 0)
        return make_fret_clip_selection({"finger_open"}, false, child_index);
    const int event_key = first_lane + 1;
    const bool chord = (note_mask & (note_mask - 1u)) != 0;
    if (!chord) {
        auto names = names_for_hand_choice(map.single[(size_t)first_lane],
                                           note_length);
        if (!names.empty())
            return make_fret_clip_selection(std::move(names), true,
                                            child_index);
    } else {
        const std::vector<int> chord_keys = event_keys_from_mask(note_mask);
        const Gameplay::HandChordRule* empty_fallback = nullptr;
        for (const auto& rule : map.chords) {
            if (rule.keys.empty()) {
                if (!empty_fallback) empty_fallback = &rule;
                continue;
            }
            if (!chord_rule_exact_matches(rule, chord_keys)) continue;
            auto names = names_for_chord_rule(rule, note_length);
            if (!names.empty())
                return make_fret_clip_selection(std::move(names), true,
                                                child_index);
        }
        for (const auto& rule : map.chords) {
            if (!chord_rule_root_bucket_matches(rule, event_key)) continue;
            auto names = names_for_chord_rule(rule, note_length);
            if (!names.empty())
                return make_fret_clip_selection(std::move(names), true,
                                                child_index);
        }
        if (empty_fallback) {
            auto names = names_for_chord_rule(*empty_fallback, note_length);
            if (!names.empty())
                return make_fret_clip_selection(std::move(names), true,
                                                child_index);
        }
    }
    return make_fret_clip_selection(
        chord ? std::vector<std::string>{"finger_powerchord_1",
                                         "finger_chord_bar"}
              : std::vector<std::string>{"finger_hold_index"},
        false, child_index);
}

FretClipSelection strum_clip_selection_for_note(
    const std::map<std::string, Gameplay::StrumHandMap>& maps,
    const std::string& strum_map_name, double note_length,
    size_t child_index) {
    auto it = maps.find(strum_map_name);
    if (it == maps.end()) it = maps.find("StrumMap_Default");
    if (it == maps.end())
        return make_fret_clip_selection({"strum_short_01"}, false,
                                        child_index);
    const Gameplay::HandClipChoice& choice =
        note_length > 0.0 ? it->second.regular : it->second.fallback;
    auto names = names_for_hand_choice(choice, note_length);
    if (!names.empty())
        return make_fret_clip_selection(std::move(names), true, child_index);
    return make_fret_clip_selection({"strum_short_01"}, false, child_index);
}

std::vector<std::string> all_strum_hand_clip_names(
    const std::map<std::string, Gameplay::StrumHandMap>& maps) {
    std::vector<std::string> out;
    push_unique(out, "strum_open");
    auto add_choice = [&](const Gameplay::HandClipChoice& choice) {
        for (const auto& name : choice.short_names) push_unique(out, name);
        for (const auto& name : choice.long_names) push_unique(out, name);
    };
    for (const auto& entry : maps) {
        add_choice(entry.second.regular);
        add_choice(entry.second.fallback);
    }
    for (const char* fallback :
         {"strum_short_01", "strum_short_02", "strum_short_03",
          "strum_short_04", "strum_long_01", "strum_long_02",
          "strum_long_03", "strum_long_04", "strum_pick_01",
          "strum_pick_02"}) {
        push_unique(out, fallback);
    }
    return out;
}

std::vector<std::string> all_fret_hand_clip_names(
    const std::map<std::string, Gameplay::FretHandMap>& maps) {
    std::vector<std::string> out;
    auto add_choice = [&](const Gameplay::HandClipChoice& choice) {
        for (const auto& name : choice.short_names) push_unique(out, name);
        for (const auto& name : choice.long_names) push_unique(out, name);
    };
    push_unique(out, "finger_open");
    for (const auto& entry : maps) {
        const auto& map = entry.second;
        for (const auto& choice : map.single) add_choice(choice);
        for (const auto& rule : map.chords) add_choice(rule.choice);
    }
    for (const char* fallback :
         {"finger_hold_index", "finger_hold_middle", "finger_hold_ring",
          "finger_hold_pinky", "finger_hold_pinky_hi", "finger_powerchord_1",
          "finger_powerchord_2", "finger_chord_bar"}) {
        push_unique(out, fallback);
    }
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// load_song
// ---------------------------------------------------------------------------

bool Gameplay::load_song(const std::string& hdr_path, const std::string& ark_path,
                          const std::string& shortname, int difficulty) {
    chart_loaded_ = false;
    song_time_    = 0.0;
    next_note_idx_= 0;
    score_        = 0;
    streak_       = 0;
    multiplier_   = 1;
    hit_flash_mask_  = 0;
    miss_flash_mask_ = 0;
    prev_fret_mask_  = 0;
    diagnostic_autoplay_last_note_tick_ = UINT32_MAX;
    for (auto& consumed : note_consumed_) consumed.clear();
    difficulty_   = std::clamp(difficulty, 0, 3);
    hdr_path_     = hdr_path;
    ark_path_     = ark_path;
    world_.reset();
    lighting_.reset();
    drum_kit_.reset();
    drum_mesh_transform_anims_.clear();
    drum_event_mesh_targets_.clear();
    fret_hand_maps_.clear();
    performers_.clear();
    highway_.reset();
    world_init_attempted_ = false;
    quickplay_rig_.reset();
    facefx_animation_.reset();
    camera_keys_.clear();
    regular_camera_keys_.clear();
    active_regular_camera_.clear();
    previous_regular_camera_.clear();
    active_regular_camera_start_ = 0.0;
    active_camera_position_start_ = 0.0;
    active_camera_position_index_ = 0;
    previous_camera_position_index_ = 0;
    intro_camera_seconds_ = 0.0;
    camera_duration_bars_.clear();
    camera_duration_bars_["kExcitementOkay"] = {2, 4};
    camera_bars_left_ = 0;
    last_camera_bar_ = UINT32_MAX;
    last_forced_camera_event_tick_ = UINT32_MAX;
    camera_shot_counter_ = 0;
    intro_end_dispatched_ = false;
    should_resend_excitement_ = false;
    lighting_presets_.clear();
    lighting_spotlights_.clear();
    active_lighting_preset_.clear();
    active_lighting_keyframe_.clear();
    active_lighting_keyframe_index_ = SIZE_MAX;
    active_lighting_preset_start_ = 0.0;
    active_lighting_spot_targets_.clear();
    lighting_transition_from_.clear();
    lighting_transition_to_.clear();
    lighting_transition_start_ = 0.0;
    lighting_transition_duration_ = 0.0;
    lighting_transition_active_ = false;
    next_lighting_cue_idx_ = 0;
    ignored_last_light_change_ = false;
    lighting_mat_anims_.clear();
    lighting_event_mat_anims_.clear();
    lighting_material_alpha_.clear();
    lighting_material_tex_transforms_.clear();
    active_lighting_material_anims_.clear();
    venue_mat_anims_.clear();
    venue_event_mat_anims_.clear();
    venue_env_anims_.clear();
    venue_event_env_anims_.clear();
    venue_light_anims_.clear();
    venue_event_light_anims_.clear();
    venue_light_colors_.clear();
    active_venue_light_anims_.clear();
    venue_event_particle_systems_.clear();
    venue_active_particle_systems_.clear();
    venue_particle_intensities_.clear();
    active_venue_particles_.clear();
    last_venue_particle_debug_time_ = -1.0;
    venue_event_filters_.clear();
    venue_filter_mesh_targets_.clear();
    venue_event_anim_filters_.clear();
    venue_lights_.clear();
    venue_environs_.clear();
    active_venue_anim_filters_.clear();
    last_venue_filter_debug_time_ = -1.0;
    venue_event_group_visibility_.clear();
    pending_transient_venue_events_.clear();
    venue_material_meshes_.clear();
    venue_material_alpha_.clear();
    venue_material_tex_transforms_.clear();
    active_venue_material_anims_.clear();
    venue_environment_colors_.clear();
    active_venue_environment_anims_.clear();
    venue_mesh_translation_offsets_.clear();
    venue_mesh_transform_offsets_.clear();
    venue_mesh_position_overrides_.clear();
    venue_base_hidden_meshes_.clear();
    venue_runtime_hidden_meshes_.clear();
    active_venue_event_.clear();
    last_anim_time_ = -1.0;
    last_band_note_tick_ = UINT32_MAX;
    next_drum_cue_idx_ = 0;
    next_bass_cue_idx_ = 0;
    next_venue_cue_idx_ = 0;

    if (hdr_path.empty() || ark_path.empty()) {
        std::fprintf(stderr, "[gameplay] no ARK paths; cannot load song\n");
        return false;
    }

    // --- MIDI chart ---
    const std::string mid_path = "songs/" + shortname + "/" + shortname + ".mid";
    std::fprintf(stderr, "[gameplay] loading chart: %s\n", mid_path.c_str());

    std::vector<uint8_t> mid_bytes;
    try {
        auto ark = gh::ark::ArkV3Reader::load(hdr_path);
        auto entry = ark.find(mid_path);
        if (!entry) {
            std::fprintf(stderr, "[gameplay] MIDI not found in ARK: %s\n", mid_path.c_str());
            return false;
        }
        mid_bytes = ark.read_entry(*entry, {ark_path});
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[gameplay] ARK error: %s\n", ex.what());
        return false;
    }

    chart_ = ghogx::chart::parse_midi(mid_bytes);
    for (int d = 0; d < 4; ++d) {
        note_consumed_[d].assign(chart_.notes[d].size(), 0);
    }
    chart_loaded_ = true;

    std::fprintf(stderr, "[gameplay] chart loaded: diff=%d notes=%zu dur=%.1fs\n",
                 difficulty_,
                 chart_.notes[difficulty_].size(),
                 chart_.duration_sec());
    const size_t guitar_perf_lane = performer_chart_lane_index(chart_.notes);
    const size_t bass_perf_lane = performer_chart_lane_index(chart_.bass_notes);
    std::fprintf(
        stderr,
        "[gameplay] performer note source: guitar_lane=%zu notes=%zu bass_lane=%zu notes=%zu player_diff=%d\n",
        guitar_perf_lane, chart_.notes[guitar_perf_lane].size(),
        bass_perf_lane, chart_.bass_notes[bass_perf_lane].size(), difficulty_);
    fret_hand_maps_ = load_fret_hand_maps(hdr_path, ark_path);
    strum_hand_maps_ = load_strum_hand_maps(hdr_path, ark_path);

    // --- Audio ---
    const std::string vgs_path = "songs/" + shortname + "/" + shortname + ".vgs";
    audio_.load_vgs(hdr_path, ark_path, vgs_path);  // non-fatal on failure

    const std::string voc_path = "songs/" + shortname + "/" + shortname + ".voc";
    facefx_animation_ =
        ghogx::character::load_facefx_animation(hdr_path, ark_path, voc_path);

    quickplay_rig_ = resolve_quickplay_rig(hdr_path, ark_path, shortname);
    if (quickplay_rig_) {
        std::fprintf(stderr, "[world] quickplay rig: character=%s guitar=%s venue=%s band=",
                     quickplay_rig_->character_outfit.c_str(),
                     quickplay_rig_->guitar.c_str(), quickplay_rig_->venue.c_str());
        for (const auto& b : quickplay_rig_->band) std::fprintf(stderr, "%s ", b.c_str());
        std::fprintf(stderr, "\n");
    } else {
        std::fprintf(stderr, "[world] no quickplay rig for song '%s'\n", shortname.c_str());
    }

    return true;
}

bool Gameplay::apply_venue_event_visibility(const std::string& event_name,
                                            bool log) {
    const auto visibility_it = venue_event_group_visibility_.find(event_name);
    if (visibility_it == venue_event_group_visibility_.end()) return false;
    for (const auto& mesh : visibility_it->second.show_meshes) {
        venue_runtime_hidden_meshes_.erase(mesh);
    }
    for (const auto& mesh : visibility_it->second.hide_meshes) {
        venue_runtime_hidden_meshes_.insert(mesh);
    }
    if (log) {
        std::fprintf(
            stderr,
            "[world] venue event %s: trigger visibility show=%zu hide=%zu\n",
            event_name.c_str(), visibility_it->second.show_meshes.size(),
            visibility_it->second.hide_meshes.size());
    }
    return true;
}

std::unordered_set<std::string> Gameplay::composed_venue_hidden_meshes() const {
    std::unordered_set<std::string> hidden = venue_runtime_hidden_meshes_;
    for (const auto& [material, alpha] : venue_material_alpha_) {
        if (alpha > 0.001f) continue;
        const auto mesh_it = venue_material_meshes_.find(material);
        if (mesh_it == venue_material_meshes_.end()) continue;
        for (const auto& mesh : mesh_it->second) hidden.insert(mesh);
    }
    return hidden;
}

void Gameplay::apply_venue_event(const std::string& event_name,
                                 bool persistent) {
    if (event_name.empty()) return;
    if (persistent && !world_) {
        active_venue_event_ = event_name;
        if (debug_venue_filters_enabled()) {
            std::fprintf(stderr,
                         "[world] venue event %s: latched until venue load\n",
                         event_name.c_str());
        }
        return;
    }
    if (!persistent && !world_) {
        push_unique_ref(pending_transient_venue_events_, event_name);
        if (debug_venue_filters_enabled()) {
            std::fprintf(stderr,
                         "[world] venue event %s: queued until venue load\n",
                         event_name.c_str());
        }
        return;
    }
    if (persistent) {
        if (active_venue_event_ == event_name && world_) return;
        active_venue_event_ = event_name;
        active_venue_anim_filters_.erase(
            std::remove_if(active_venue_anim_filters_.begin(),
                           active_venue_anim_filters_.end(),
                           [](const ActiveVenueAnimFilter& active) {
                               return active.persistent;
                           }),
            active_venue_anim_filters_.end());
        active_venue_material_anims_.erase(
            std::remove_if(active_venue_material_anims_.begin(),
                           active_venue_material_anims_.end(),
                           [](const ActiveVenueMaterialAnim& active) {
                               return active.persistent;
                           }),
            active_venue_material_anims_.end());
        active_venue_environment_anims_.erase(
            std::remove_if(active_venue_environment_anims_.begin(),
                           active_venue_environment_anims_.end(),
                           [](const ActiveVenueEnvironmentAnim& active) {
                               return active.persistent;
                           }),
            active_venue_environment_anims_.end());
        active_venue_light_anims_.erase(
            std::remove_if(active_venue_light_anims_.begin(),
                           active_venue_light_anims_.end(),
                           [](const ActiveVenueLightAnim& active) {
                               return active.persistent;
                           }),
            active_venue_light_anims_.end());
        active_venue_particles_.erase(
            std::remove_if(active_venue_particles_.begin(),
                           active_venue_particles_.end(),
                           [](const ActiveVenueParticleSystem& active) {
                               return active.persistent;
                           }),
            active_venue_particles_.end());
    }
    std::vector<std::pair<std::string, float>> material_changes;
    bool material_tex_changed = false;
    bool environment_color_changed = false;
    bool light_color_changed = false;
    if (persistent) {
        venue_mesh_translation_offsets_.clear();
        environment_color_changed = !venue_environment_colors_.empty();
        venue_environment_colors_.clear();
        light_color_changed = !venue_light_colors_.empty();
        venue_light_colors_.clear();
    }
    auto event_it = venue_event_mat_anims_.find(event_name);
    if (event_it != venue_event_mat_anims_.end()) {
        for (const auto& anim_name : event_it->second) {
            auto anim_it = venue_mat_anims_.find(anim_name);
            if (anim_it == venue_mat_anims_.end()) continue;
            const auto& anim = anim_it->second;
            const bool has_tex_transform =
                !anim.tex_translation_keys.empty() ||
                !anim.tex_scale_keys.empty() ||
                !anim.tex_rotation_keys.empty();
            if (!anim.has_alpha && !has_tex_transform) continue;
            active_venue_material_anims_.erase(
                std::remove_if(active_venue_material_anims_.begin(),
                               active_venue_material_anims_.end(),
                               [&](const ActiveVenueMaterialAnim& active) {
                                   return active.material == anim.material;
                               }),
                active_venue_material_anims_.end());
            const auto current_alpha = venue_material_alpha_.find(anim.material);
            ActiveVenueMaterialAnim active_anim;
            active_anim.name = anim.name;
            active_anim.material = anim.material;
            active_anim.has_alpha = anim.has_alpha;
            active_anim.start_alpha =
                anim.has_alpha
                    ? (current_alpha == venue_material_alpha_.end()
                           ? anim.start_alpha
                           : current_alpha->second)
                    : 1.0f;
            active_anim.end_alpha = anim.end_alpha;
            active_anim.start_time = song_time_;
            active_anim.duration_seconds =
                authored_frames_to_seconds(anim.duration_frames);
            active_anim.duration_frames = anim.duration_frames;
            active_anim.tex_translation_keys = anim.tex_translation_keys;
            active_anim.tex_scale_keys = anim.tex_scale_keys;
            active_anim.tex_rotation_keys = anim.tex_rotation_keys;
            active_anim.persistent = persistent;
            if (anim.has_alpha) {
                venue_material_alpha_[anim.material] = active_anim.start_alpha;
                material_changes.push_back({anim.material, anim.end_alpha});
            }
            if (has_tex_transform) {
                venue_material_tex_transforms_[anim.material] =
                    sample_material_tex_transform(anim, 0.0f);
                material_tex_changed = true;
            }
            std::fprintf(stderr,
                         "[world] venue event %s: MatAnim %s -> %s alpha %.3f -> %.3f tex_trans_keys=%zu tex_scale_keys=%zu tex_rot_keys=%zu seconds=%.3f %s\n",
                         event_name.c_str(), anim_name.c_str(),
                         anim.material.c_str(), active_anim.start_alpha,
                         active_anim.end_alpha,
                         active_anim.tex_translation_keys.size(),
                         active_anim.tex_scale_keys.size(),
                         active_anim.tex_rotation_keys.size(),
                         active_anim.duration_seconds,
                         persistent ? "persistent" : "transient");
            if (active_anim.duration_seconds <= 0.0001) {
                if (anim.has_alpha)
                    venue_material_alpha_[anim.material] = anim.end_alpha;
            } else {
                active_venue_material_anims_.push_back(std::move(active_anim));
            }
        }
    }
    if (const auto env_event_it = venue_event_env_anims_.find(event_name);
        env_event_it != venue_event_env_anims_.end()) {
        for (const auto& anim_name : env_event_it->second) {
            const auto anim_it = venue_env_anims_.find(anim_name);
            if (anim_it == venue_env_anims_.end()) continue;
            const auto& anim = anim_it->second;
            if (anim.color_keys.empty()) continue;
            active_venue_environment_anims_.erase(
                std::remove_if(active_venue_environment_anims_.begin(),
                               active_venue_environment_anims_.end(),
                               [&](const ActiveVenueEnvironmentAnim& active) {
                                   return active.environment == anim.environment;
                               }),
                active_venue_environment_anims_.end());
            ActiveVenueEnvironmentAnim active_anim;
            active_anim.name = anim.name;
            active_anim.environment = anim.environment;
            active_anim.start_time = song_time_;
            active_anim.duration_frames = anim.duration_frames;
            active_anim.color_keys = anim.color_keys;
            active_anim.persistent = persistent;
            venue_environment_colors_[anim.environment] =
                sample_environment_color_key(active_anim.color_keys, 0.0f);
            environment_color_changed = true;
            std::fprintf(
                stderr,
                "[world] venue event %s: EnvAnim %s -> %s color_keys=%zu frames=%.1f %s\n",
                event_name.c_str(), anim.name.c_str(),
                anim.environment.c_str(), anim.color_keys.size(),
                anim.duration_frames, persistent ? "persistent" : "transient");
            if (anim.duration_frames > 0.001f) {
                active_venue_environment_anims_.push_back(std::move(active_anim));
            }
        }
    }
    if (const auto light_event_it =
            venue_event_light_anims_.find(event_name);
        light_event_it != venue_event_light_anims_.end()) {
        for (const auto& anim_name : light_event_it->second) {
            const auto anim_it = venue_light_anims_.find(anim_name);
            if (anim_it == venue_light_anims_.end()) continue;
            const auto& anim = anim_it->second;
            if (anim.color_keys.empty()) continue;
            active_venue_light_anims_.erase(
                std::remove_if(active_venue_light_anims_.begin(),
                               active_venue_light_anims_.end(),
                               [&](const ActiveVenueLightAnim& active) {
                                   return active.light == anim.light;
                               }),
                active_venue_light_anims_.end());
            ActiveVenueLightAnim active_anim;
            active_anim.name = anim.name;
            active_anim.light = anim.light;
            active_anim.start_time = song_time_;
            active_anim.duration_frames = anim.duration_frames;
            active_anim.color_keys = anim.color_keys;
            active_anim.persistent = persistent;
            venue_light_colors_[anim.light] =
                sample_light_color_key(active_anim.color_keys, 0.0f);
            light_color_changed = true;
            std::fprintf(
                stderr,
                "[world] venue event %s: LightAnim %s -> %s color_keys=%zu frames=%.1f %s\n",
                event_name.c_str(), anim.name.c_str(), anim.light.c_str(),
                anim.color_keys.size(), anim.duration_frames,
                persistent ? "persistent" : "transient");
            if (anim.duration_frames > 0.001f) {
                active_venue_light_anims_.push_back(std::move(active_anim));
            }
        }
    }
    if (const auto particle_it =
            venue_event_particle_systems_.find(event_name);
        particle_it != venue_event_particle_systems_.end()) {
        for (const auto& route : particle_it->second) {
            active_venue_particles_.erase(
                std::remove_if(active_venue_particles_.begin(),
                               active_venue_particles_.end(),
                               [&](const ActiveVenueParticleSystem& active) {
                                   return active.particle == route.particle &&
                                          active.persistent == persistent;
                               }),
                active_venue_particles_.end());
            ActiveVenueParticleSystem active;
            active.particle = route.particle;
            active.start_time = song_time_;
            active.duration_seconds = authored_frames_to_seconds(
                route.duration_frames > 0.001f ? route.duration_frames : 30.0f);
            active.duration_frames = route.duration_frames;
            active.emission_keys = route.emission_keys;
            active.persistent = persistent;
            active_venue_particles_.push_back(std::move(active));
            std::fprintf(
                stderr,
                "[world] venue event %s: ParticleSys %s via %s keys=%zu frames=%.1f seconds=%.3f %s\n",
                event_name.c_str(), route.particle.c_str(),
                route.anim.c_str(), route.emission_keys.size(),
                route.duration_frames,
                active_venue_particles_.back().duration_seconds,
                persistent ? "persistent" : "transient");
        }
    }
    for (const auto& [material, alpha] : material_changes) {
        const auto mesh_it = venue_material_meshes_.find(material);
        if (mesh_it == venue_material_meshes_.end()) continue;
        std::fprintf(stderr, "[world] venue event %s: material %s %s %zu meshes\n",
                     event_name.c_str(), material.c_str(),
                     alpha <= 0.001f ? "hide" : "show",
                     mesh_it->second.size());
    }
    const bool current_visibility_applied =
        apply_venue_event_visibility(event_name, true);
    if (!current_visibility_applied && debug_venue_filters_enabled()) {
        std::fprintf(stderr,
                     "[world] venue event %s: no trigger visibility route\n",
                     event_name.c_str());
    }
    if (const auto filter_event_it =
            venue_event_anim_filters_.find(event_name);
        filter_event_it != venue_event_anim_filters_.end()) {
        ActiveVenueAnimFilter active_filter;
        active_filter.event_name = event_name;
        active_filter.filters = filter_event_it->second;
        active_filter.start_time = song_time_;
        active_filter.persistent = persistent;
        active_venue_anim_filters_.push_back(std::move(active_filter));
        for (const auto& filter : filter_event_it->second) {
            std::fprintf(
                stderr,
                "[world] venue event %s: AnimFilter %s frame %.2f..%.2f targets=%zu mesh_anims=%zu scale=%.3f period=%.3f offset=%.3f type=%d %s\n",
                event_name.c_str(), filter.name.c_str(), filter.start_frame,
                filter.end_frame, filter.targets.size(),
                filter.mesh_anim_targets.size(), filter.scale, filter.period,
                filter.offset_frame, filter.type,
                persistent ? "persistent" : "transient");
        }
    } else if (debug_venue_filters_enabled()) {
        std::fprintf(stderr,
                     "[world] venue event %s: no decoded AnimFilter transforms\n",
                     event_name.c_str());
    }
    if (world_) {
        world_->set_material_alpha_multipliers(venue_material_alpha_);
        if (material_tex_changed)
            world_->set_material_tex_transform_overrides(
                venue_material_tex_transforms_);
        if (environment_color_changed)
            world_->set_environment_color_overrides(venue_environment_colors_);
        if (light_color_changed)
            world_->set_light_color_overrides(venue_light_colors_);
        world_->set_hidden_meshes(composed_venue_hidden_meshes());
    }
    update_active_venue_light_anims();
    update_active_venue_particles();
    update_active_venue_anim_filters();
    apply_lighting_event(event_name);
}

void Gameplay::resend_active_venue_event() {
    if (active_venue_event_.empty()) return;
    const std::string active = active_venue_event_;
    active_venue_event_.clear();
    std::fprintf(stderr, "[world] resend_excitement: %s\n", active.c_str());
    apply_venue_event(active, true);
}

void Gameplay::clear_runtime_venue_animation_state() {
    lighting_material_alpha_.clear();
    lighting_material_tex_transforms_.clear();
    active_lighting_material_anims_.clear();

    venue_material_alpha_.clear();
    venue_material_tex_transforms_.clear();
    active_venue_material_anims_.clear();
    venue_environment_colors_.clear();
    active_venue_environment_anims_.clear();
    venue_light_colors_.clear();
    active_venue_light_anims_.clear();
    venue_active_particle_systems_.clear();
    venue_particle_intensities_.clear();
    active_venue_particles_.clear();
    last_venue_particle_debug_time_ = -1.0;
    active_venue_anim_filters_.clear();
    last_venue_filter_debug_time_ = -1.0;
    venue_mesh_translation_offsets_.clear();
    venue_mesh_transform_offsets_.clear();
    venue_mesh_position_overrides_.clear();
    pending_transient_venue_events_.clear();
    active_venue_event_.clear();

    if (world_) {
        venue_runtime_hidden_meshes_ = venue_base_hidden_meshes_;
        apply_venue_event_visibility("start", false);
        world_->set_material_alpha_multipliers(venue_material_alpha_);
        world_->set_material_tex_transform_overrides(
            venue_material_tex_transforms_);
        world_->set_environment_color_overrides(venue_environment_colors_);
        world_->set_light_color_overrides(venue_light_colors_);
        world_->set_active_particle_systems(venue_active_particle_systems_);
        world_->set_particle_intensities(venue_particle_intensities_);
        world_->set_mesh_transform_offsets(venue_mesh_transform_offsets_);
        world_->set_mesh_position_overrides(venue_mesh_position_overrides_);
        world_->set_hidden_meshes(composed_venue_hidden_meshes());
    }

    if (lighting_) {
        lighting_->set_material_alpha_multipliers(lighting_material_alpha_);
        lighting_->set_material_tex_transform_overrides(
            lighting_material_tex_transforms_);
        apply_lighting_event("start");
    }
}

void Gameplay::update_active_venue_material_anims() {
    if (!world_) return;
    bool alpha_changed = false;
    bool tex_changed = false;
    for (auto it = active_venue_material_anims_.begin();
         it != active_venue_material_anims_.end();) {
        const double duration = std::max(0.0, it->duration_seconds);
        const double t =
            duration <= 0.0001
                ? 1.0
                : std::clamp((song_time_ - it->start_time) / duration,
                             0.0, 1.0);
        if (it->has_alpha) {
            const float alpha = static_cast<float>(
                static_cast<double>(it->start_alpha) +
                (static_cast<double>(it->end_alpha) - it->start_alpha) * t);
            venue_material_alpha_[it->material] = clamp_material_alpha(alpha);
            alpha_changed = true;
        }
        const bool has_tex_transform =
            !it->tex_translation_keys.empty() ||
            !it->tex_scale_keys.empty() ||
            !it->tex_rotation_keys.empty();
        if (has_tex_transform) {
            const float frame = material_anim_frame_at(
                it->duration_frames, song_time_ - it->start_time,
                it->persistent);
            venue_material_tex_transforms_[it->material] =
                sample_material_tex_transform(*it, frame);
            tex_changed = true;
        }
        const bool keep_persistent_tex =
            it->persistent && has_tex_transform && duration > 0.0001;
        if (t >= 1.0 && !keep_persistent_tex) {
            it = active_venue_material_anims_.erase(it);
        } else {
            ++it;
        }
    }
    if (alpha_changed) {
        world_->set_material_alpha_multipliers(venue_material_alpha_);
        world_->set_hidden_meshes(composed_venue_hidden_meshes());
    }
    if (tex_changed)
        world_->set_material_tex_transform_overrides(
            venue_material_tex_transforms_);
}

void Gameplay::update_active_venue_environment_anims() {
    if (!world_) return;
    if (active_venue_environment_anims_.empty()) return;

    bool changed = false;
    for (auto it = active_venue_environment_anims_.begin();
         it != active_venue_environment_anims_.end();) {
        const float frame = material_anim_frame_at(
            it->duration_frames, song_time_ - it->start_time,
            it->persistent);
        venue_environment_colors_[it->environment] =
            sample_environment_color_key(it->color_keys, frame);
        changed = true;
        if (!it->persistent && frame >= it->duration_frames - 0.001f) {
            it = active_venue_environment_anims_.erase(it);
        } else {
            ++it;
        }
    }
    if (changed)
        world_->set_environment_color_overrides(venue_environment_colors_);
}

void Gameplay::update_active_venue_light_anims() {
    if (!world_) return;
    if (active_venue_light_anims_.empty()) return;

    bool changed = false;
    for (auto it = active_venue_light_anims_.begin();
         it != active_venue_light_anims_.end();) {
        const float frame = material_anim_frame_at(
            it->duration_frames, song_time_ - it->start_time, it->persistent);
        venue_light_colors_[it->light] =
            sample_light_color_key(it->color_keys, frame);
        changed = true;
        if (!it->persistent && frame >= it->duration_frames - 0.001f) {
            it = active_venue_light_anims_.erase(it);
        } else {
            ++it;
        }
    }
    if (changed) world_->set_light_color_overrides(venue_light_colors_);
}

void Gameplay::update_active_venue_particles() {
    if (!world_) return;
    std::unordered_set<std::string> active_particles;
    std::map<std::string, float> particle_intensities;
    const bool debug_sample =
        debug_venue_filters_enabled() &&
        (last_venue_particle_debug_time_ < 0.0 ||
         song_time_ - last_venue_particle_debug_time_ >= 0.5);
    for (auto it = active_venue_particles_.begin();
         it != active_venue_particles_.end();) {
        const double elapsed = std::max(0.0, song_time_ - it->start_time);
        if (!it->persistent && elapsed > it->duration_seconds) {
            it = active_venue_particles_.erase(it);
            continue;
        }
        const float frame =
            it->duration_frames > 0.001f
                ? material_anim_frame_at(it->duration_frames, elapsed,
                                         it->persistent)
                : static_cast<float>(elapsed * kLightingFramesPerSecond);
        const float intensity = sample_particle_emission(it->emission_keys, frame);
        if (intensity > 0.001f) {
            active_particles.insert(it->particle);
            auto& slot = particle_intensities[it->particle];
            slot = std::max(slot, intensity);
            if (debug_sample) {
                std::fprintf(
                    stderr,
                    "[world] venue ParticleSys sample %s frame=%.2f intensity=%.3f keys=%zu persistent=%d\n",
                    it->particle.c_str(), frame, intensity,
                    it->emission_keys.size(), it->persistent ? 1 : 0);
            }
        }
        ++it;
    }
    if (debug_sample) last_venue_particle_debug_time_ = song_time_;
    if (active_particles != venue_active_particle_systems_ ||
        particle_intensities != venue_particle_intensities_) {
        venue_active_particle_systems_ = std::move(active_particles);
        venue_particle_intensities_ = std::move(particle_intensities);
        world_->set_active_particle_systems(venue_active_particle_systems_);
        world_->set_particle_intensities(venue_particle_intensities_);
    }
}

void Gameplay::update_active_venue_anim_filters() {
    if (!world_) return;
    if (active_venue_anim_filters_.empty()) {
        if (!venue_mesh_transform_offsets_.empty() ||
            !venue_mesh_position_overrides_.empty()) {
            venue_mesh_translation_offsets_.clear();
            venue_mesh_transform_offsets_.clear();
            venue_mesh_position_overrides_.clear();
            world_->set_mesh_transform_offsets(venue_mesh_transform_offsets_);
            world_->set_mesh_position_overrides(venue_mesh_position_overrides_);
        }
        return;
    }

    venue_mesh_translation_offsets_.clear();
    venue_mesh_transform_offsets_.clear();
    venue_mesh_position_overrides_.clear();
    const bool debug_sample =
        debug_venue_filters_enabled() &&
        (last_venue_filter_debug_time_ < 0.0 ||
         song_time_ - last_venue_filter_debug_time_ >= 0.5);
    if (debug_sample) last_venue_filter_debug_time_ = song_time_;
    for (auto it = active_venue_anim_filters_.begin();
         it != active_venue_anim_filters_.end();) {
        const double elapsed = std::max(0.0, song_time_ - it->start_time);
        double duration = 0.0;
        for (const auto& filter : it->filters) {
            duration =
                std::max(duration, venue_filter_duration_seconds(filter));
        }
        if (!it->persistent && duration > 0.0 && elapsed > duration) {
            it = active_venue_anim_filters_.erase(it);
            continue;
        }

        for (const auto& filter : it->filters) {
            const float frame =
                venue_filter_frame_at(filter, elapsed, it->persistent);
            for (const auto& target : filter.targets) {
                const auto sample = sample_mesh_transform(target.anim, frame);
                venue_mesh_transform_offsets_[target.mesh] = sample;
                if (sample.has_translation)
                    venue_mesh_translation_offsets_[target.mesh] =
                        sample.translation;
                if (debug_sample) {
                    std::fprintf(
                        stderr,
                        "[world] venue AnimFilter sample event=%s filter=%s mesh=%s frame=%.2f pos=%d rot=%d scale=%d offset=(%.3f %.3f %.3f) persistent=%d\n",
                        it->event_name.c_str(), filter.name.c_str(),
                        target.mesh.c_str(), frame,
                        sample.has_translation ? 1 : 0,
                        sample.has_rotation ? 1 : 0,
                        sample.has_scale ? 1 : 0, sample.translation[0],
                        sample.translation[1], sample.translation[2],
                        it->persistent ? 1 : 0);
                }
            }
            for (const auto& target : filter.mesh_anim_targets) {
                auto positions = sample_mesh_anim_positions(target.anim, frame);
                if (!positions.empty())
                    venue_mesh_position_overrides_[target.mesh] =
                        std::move(positions);
                if (debug_sample) {
                    std::fprintf(
                        stderr,
                        "[world] venue MeshAnim sample event=%s filter=%s mesh=%s anim=%s frame=%.2f verts=%u persistent=%d\n",
                        it->event_name.c_str(), filter.name.c_str(),
                        target.mesh.c_str(), target.anim.name.c_str(), frame,
                        target.anim.vertex_count, it->persistent ? 1 : 0);
                }
            }
        }
        ++it;
    }
    world_->set_mesh_transform_offsets(venue_mesh_transform_offsets_);
    world_->set_mesh_position_overrides(venue_mesh_position_overrides_);
}

void Gameplay::apply_lighting_event(const std::string& event_name) {
    if (event_name.empty() || !lighting_) return;
    auto event_it = lighting_event_mat_anims_.find(event_name);
    if (event_it == lighting_event_mat_anims_.end()) return;

    bool alpha_changed = false;
    bool tex_changed = false;
    bool venue_alpha_changed = false;
    bool venue_tex_changed = false;
    for (const auto& anim_name : event_it->second) {
        auto anim_it = lighting_mat_anims_.find(anim_name);
        if (anim_it == lighting_mat_anims_.end()) {
            auto venue_anim_it = venue_mat_anims_.find(anim_name);
            if (venue_anim_it != venue_mat_anims_.end() && world_) {
                const auto& anim = venue_anim_it->second;
                const bool has_tex_transform =
                    !anim.tex_translation_keys.empty() ||
                    !anim.tex_scale_keys.empty() ||
                    !anim.tex_rotation_keys.empty();
                if (!anim.has_alpha && !has_tex_transform) continue;
                active_venue_material_anims_.erase(
                    std::remove_if(active_venue_material_anims_.begin(),
                                   active_venue_material_anims_.end(),
                                   [&](const ActiveVenueMaterialAnim& active) {
                                       return active.material == anim.material;
                                   }),
                    active_venue_material_anims_.end());
                const auto current_alpha =
                    venue_material_alpha_.find(anim.material);
                ActiveVenueMaterialAnim active_anim;
                active_anim.name = anim.name;
                active_anim.material = anim.material;
                active_anim.has_alpha = anim.has_alpha;
                active_anim.start_alpha =
                    anim.has_alpha
                        ? (current_alpha == venue_material_alpha_.end()
                               ? anim.start_alpha
                               : current_alpha->second)
                        : 1.0f;
                active_anim.end_alpha = anim.end_alpha;
                active_anim.start_time = song_time_;
                active_anim.duration_seconds =
                    authored_frames_to_seconds(anim.duration_frames);
                active_anim.duration_frames = anim.duration_frames;
                active_anim.tex_translation_keys = anim.tex_translation_keys;
                active_anim.tex_scale_keys = anim.tex_scale_keys;
                active_anim.tex_rotation_keys = anim.tex_rotation_keys;
                active_anim.persistent = true;
                if (anim.has_alpha) {
                    venue_material_alpha_[anim.material] =
                        active_anim.start_alpha;
                    venue_alpha_changed = true;
                }
                if (has_tex_transform) {
                    venue_material_tex_transforms_[anim.material] =
                        sample_material_tex_transform(anim, 0.0f);
                    venue_tex_changed = true;
                }
                std::fprintf(
                    stderr,
                    "[world] lighting event %s: venue MatAnim %s -> %s alpha %.3f -> %.3f tex_trans_keys=%zu tex_scale_keys=%zu tex_rot_keys=%zu seconds=%.3f\n",
                    event_name.c_str(), anim_name.c_str(),
                    anim.material.c_str(), active_anim.start_alpha,
                    active_anim.end_alpha,
                    active_anim.tex_translation_keys.size(),
                    active_anim.tex_scale_keys.size(),
                    active_anim.tex_rotation_keys.size(),
                    active_anim.duration_seconds);
                if (active_anim.duration_seconds <= 0.0001) {
                    if (anim.has_alpha)
                        venue_material_alpha_[anim.material] = anim.end_alpha;
                } else {
                    active_venue_material_anims_.push_back(
                        std::move(active_anim));
                }
                continue;
            }
            if (debug_venue_filters_enabled()) {
                std::fprintf(stderr,
                             "[world] lighting event %s: MatAnim %s route has unsupported channel shape\n",
                             event_name.c_str(), anim_name.c_str());
            }
            continue;
        }
        const auto& anim = anim_it->second;
        const bool has_tex_transform =
            !anim.tex_translation_keys.empty() ||
            !anim.tex_scale_keys.empty() ||
            !anim.tex_rotation_keys.empty();
        if (!anim.has_alpha && !has_tex_transform) {
            if (debug_venue_filters_enabled()) {
                std::fprintf(stderr,
                             "[world] lighting event %s: MatAnim %s has no supported material channels\n",
                             event_name.c_str(), anim_name.c_str());
            }
            continue;
        }
        active_lighting_material_anims_.erase(
            std::remove_if(active_lighting_material_anims_.begin(),
                           active_lighting_material_anims_.end(),
                           [&](const ActiveVenueMaterialAnim& active) {
                               return active.material == anim.material;
                           }),
            active_lighting_material_anims_.end());
        const auto current_alpha = lighting_material_alpha_.find(anim.material);
        ActiveVenueMaterialAnim active_anim;
        active_anim.name = anim.name;
        active_anim.material = anim.material;
        active_anim.has_alpha = anim.has_alpha;
        active_anim.start_alpha =
            anim.has_alpha
                ? (current_alpha == lighting_material_alpha_.end()
                       ? anim.start_alpha
                       : current_alpha->second)
                : 1.0f;
        active_anim.end_alpha = anim.end_alpha;
        active_anim.start_time = song_time_;
        active_anim.duration_seconds =
            authored_frames_to_seconds(anim.duration_frames);
        active_anim.duration_frames = anim.duration_frames;
        active_anim.tex_translation_keys = anim.tex_translation_keys;
        active_anim.tex_scale_keys = anim.tex_scale_keys;
        active_anim.tex_rotation_keys = anim.tex_rotation_keys;
        active_anim.persistent = true;
        if (anim.has_alpha)
            lighting_material_alpha_[anim.material] = active_anim.start_alpha;
        if (has_tex_transform) {
            lighting_material_tex_transforms_[anim.material] =
                sample_material_tex_transform(anim, 0.0f);
            tex_changed = true;
        }
        std::fprintf(stderr,
                     "[world] lighting event %s: MatAnim %s -> %s alpha %.3f -> %.3f tex_trans_keys=%zu tex_scale_keys=%zu tex_rot_keys=%zu seconds=%.3f\n",
                     event_name.c_str(), anim_name.c_str(), anim.material.c_str(),
                     active_anim.start_alpha, active_anim.end_alpha,
                     active_anim.tex_translation_keys.size(),
                     active_anim.tex_scale_keys.size(),
                     active_anim.tex_rotation_keys.size(),
                     active_anim.duration_seconds);
        if (active_anim.duration_seconds <= 0.0001) {
            if (anim.has_alpha)
                lighting_material_alpha_[anim.material] = anim.end_alpha;
        } else {
            active_lighting_material_anims_.push_back(std::move(active_anim));
        }
        if (anim.has_alpha) alpha_changed = true;
    }
    if (alpha_changed)
        lighting_->set_material_alpha_multipliers(lighting_material_alpha_);
    if (tex_changed)
        lighting_->set_material_tex_transform_overrides(
            lighting_material_tex_transforms_);
    if (venue_alpha_changed && world_)
        world_->set_material_alpha_multipliers(venue_material_alpha_);
    if (venue_tex_changed && world_)
        world_->set_material_tex_transform_overrides(
            venue_material_tex_transforms_);
}

void Gameplay::update_active_lighting_material_anims() {
    if (!lighting_) return;
    bool alpha_changed = false;
    bool tex_changed = false;
    for (auto it = active_lighting_material_anims_.begin();
         it != active_lighting_material_anims_.end();) {
        const double duration = std::max(0.0, it->duration_seconds);
        const double t =
            duration <= 0.0001
                ? 1.0
                : std::clamp((song_time_ - it->start_time) / duration,
                             0.0, 1.0);
        if (it->has_alpha) {
            const float alpha = static_cast<float>(
                static_cast<double>(it->start_alpha) +
                (static_cast<double>(it->end_alpha) - it->start_alpha) * t);
            lighting_material_alpha_[it->material] = clamp_material_alpha(alpha);
            alpha_changed = true;
        }
        const bool has_tex_transform =
            !it->tex_translation_keys.empty() ||
            !it->tex_scale_keys.empty() ||
            !it->tex_rotation_keys.empty();
        if (has_tex_transform) {
            const float frame = material_anim_frame_at(
                it->duration_frames, song_time_ - it->start_time,
                it->persistent);
            lighting_material_tex_transforms_[it->material] =
                sample_material_tex_transform(*it, frame);
            tex_changed = true;
        }
        const bool keep_persistent_tex =
            it->persistent && has_tex_transform && duration > 0.0001;
        if (t >= 1.0 && !keep_persistent_tex) {
            it = active_lighting_material_anims_.erase(it);
        } else {
            ++it;
        }
    }
    if (alpha_changed)
        lighting_->set_material_alpha_multipliers(lighting_material_alpha_);
    if (tex_changed)
        lighting_->set_material_tex_transform_overrides(
            lighting_material_tex_transforms_);
}

void Gameplay::set_lighting_spot_targets(
    std::vector<ghogx::render::MiloSceneRenderer::SpotlightState> targets,
    double fade_seconds) {
    targets = sorted_unique_spotlight_states(std::move(targets));
    if (same_spotlight_states(active_lighting_spot_targets_, targets)) return;

    std::vector<SpotlightState> current =
        lighting_transition_active_ ? interpolated_lighting_spots()
                                    : active_lighting_spot_targets_;
    current = sorted_unique_spotlight_states(std::move(current));

    const bool cold_start = current.empty() &&
                            active_lighting_spot_targets_.empty() &&
                            !lighting_transition_active_;
    const double duration =
        cold_start ? 0.0 : std::max(0.0, fade_seconds);
    active_lighting_spot_targets_ = targets;
    if (duration <= 0.0001) {
        lighting_transition_from_.clear();
        lighting_transition_to_.clear();
        lighting_transition_active_ = false;
        lighting_transition_duration_ = 0.0;
        return;
    }

    const auto current_by_name = spotlight_state_map(current);
    const auto target_by_name = spotlight_state_map(active_lighting_spot_targets_);
    std::map<std::string, bool> names;
    for (const auto& kv : current_by_name) names[kv.first] = true;
    for (const auto& kv : target_by_name) names[kv.first] = true;

    lighting_transition_from_.clear();
    lighting_transition_to_.clear();
    for (const auto& kv : names) {
        const auto cur_it = current_by_name.find(kv.first);
        const auto target_it = target_by_name.find(kv.first);
        if (cur_it != current_by_name.end() &&
            target_it != target_by_name.end()) {
            lighting_transition_from_.push_back(cur_it->second);
            lighting_transition_to_.push_back(target_it->second);
        } else if (target_it != target_by_name.end()) {
            SpotlightState from = target_it->second;
            from.intensity = 0.0f;
            lighting_transition_from_.push_back(std::move(from));
            lighting_transition_to_.push_back(target_it->second);
        } else if (cur_it != current_by_name.end()) {
            SpotlightState to = cur_it->second;
            to.intensity = 0.0f;
            lighting_transition_from_.push_back(cur_it->second);
            lighting_transition_to_.push_back(std::move(to));
        }
    }
    lighting_transition_start_ = song_time_;
    lighting_transition_duration_ = duration;
    lighting_transition_active_ = !lighting_transition_from_.empty();
    if (debug_venue_filters_enabled()) {
        std::fprintf(stderr,
                     "[world] lighting transition target: from=%llu to=%llu final=%llu fade_seconds=%.3f t=%.3f\n",
                     static_cast<unsigned long long>(
                         lighting_transition_from_.size()),
                     static_cast<unsigned long long>(
                         lighting_transition_to_.size()),
                     static_cast<unsigned long long>(
                         active_lighting_spot_targets_.size()),
                     lighting_transition_duration_, song_time_);
    }
}

std::vector<ghogx::render::MiloSceneRenderer::SpotlightState>
Gameplay::interpolated_lighting_spots() const {
    if (!lighting_transition_active_ ||
        lighting_transition_duration_ <= 0.0001 ||
        lighting_transition_from_.size() != lighting_transition_to_.size()) {
        return active_lighting_spot_targets_;
    }
    const double t = std::clamp(
        (song_time_ - lighting_transition_start_) /
            lighting_transition_duration_,
        0.0, 1.0);
    std::vector<SpotlightState> out;
    out.reserve(lighting_transition_to_.size());
    for (size_t i = 0; i < lighting_transition_to_.size(); ++i) {
        const auto& from = lighting_transition_from_[i];
        const auto& to = lighting_transition_to_[i];
        SpotlightState spot = to;
        if (spot.target_mesh.empty()) spot.target_mesh = from.target_mesh;
        spot.r = mix_lighting(from.r, to.r, t);
        spot.g = mix_lighting(from.g, to.g, t);
        spot.b = mix_lighting(from.b, to.b, t);
        spot.intensity = mix_lighting(from.intensity, to.intensity, t);
        if (spot.intensity > 0.001f || t < 1.0) out.push_back(std::move(spot));
    }
    return sorted_unique_spotlight_states(std::move(out));
}

void Gameplay::update_lighting_spotlight_renderer() {
    if (!lighting_) return;
    if (lighting_transition_active_ &&
        song_time_ >= lighting_transition_start_ + lighting_transition_duration_) {
        lighting_transition_active_ = false;
        lighting_transition_from_.clear();
        lighting_transition_to_.clear();
        lighting_->set_active_spotlights(active_lighting_spot_targets_);
        return;
    }
    lighting_->set_active_spotlights(interpolated_lighting_spots());
}

// ---------------------------------------------------------------------------
// tick
// ---------------------------------------------------------------------------

bool Gameplay::is_finished() const {
    if (!chart_loaded_) return false;
    return song_time_ >= chart_.duration_sec() + 2.0;  // 2s grace after last note
}

void Gameplay::seek_for_diagnostic_capture(double seconds) {
    if (!chart_loaded_) return;
    song_time_ = std::clamp(seconds, 0.0, std::max(0.0, chart_.duration_sec()));
    last_anim_time_ = song_time_;
    diagnostic_autoplay_last_note_tick_ = UINT32_MAX;
    for (int d = 0; d < 4; ++d) {
        note_consumed_[d].assign(chart_.notes[d].size(), 0);
    }

    const auto& player_notes = chart_.notes[std::clamp(difficulty_, 0, 3)];
    next_note_idx_ = 0;
    while (next_note_idx_ < player_notes.size() &&
           chart_.tick_to_sec(player_notes[next_note_idx_].tick_on) <
               song_time_ - kHitWindowSec) {
        ++next_note_idx_;
    }
    auto& consumed = note_consumed_[std::clamp(difficulty_, 0, 3)];
    for (size_t i = 0; i < next_note_idx_ && i < consumed.size(); ++i) {
        consumed[i] = 1;
    }

    auto skip_cues_before = [&](const auto& cues, size_t& index) {
        index = 0;
        while (index < cues.size() &&
               chart_.tick_to_sec(cues[index].tick) < song_time_) {
            ++index;
        }
    };
    skip_cues_before(chart_.drum_cues, next_drum_cue_idx_);
    skip_cues_before(chart_.bass_cues, next_bass_cue_idx_);
    skip_cues_before(chart_.venue_cues, next_venue_cue_idx_);
    skip_cues_before(chart_.lighting_cues, next_lighting_cue_idx_);
    last_camera_bar_ = UINT32_MAX;
    camera_bars_left_ = 0;
    last_forced_camera_event_tick_ = UINT32_MAX;
    intro_end_dispatched_ = false;
    should_resend_excitement_ = false;
    active_lighting_preset_.clear();
    active_lighting_keyframe_.clear();
    active_lighting_keyframe_index_ = SIZE_MAX;
    active_lighting_preset_start_ = song_time_;
    active_lighting_spot_targets_.clear();
    lighting_transition_from_.clear();
    lighting_transition_to_.clear();
    lighting_transition_start_ = song_time_;
    lighting_transition_duration_ = 0.0;
    lighting_transition_active_ = false;
    clear_runtime_venue_animation_state();
    ignored_last_light_change_ = false;
    std::fprintf(stderr,
                 "[gameplay] diagnostic seek: %.3fs player_note_idx=%zu drum_idx=%zu bass_idx=%zu venue_idx=%zu lighting_idx=%zu\n",
                 song_time_, next_note_idx_, next_drum_cue_idx_,
                 next_bass_cue_idx_, next_venue_cue_idx_,
                 next_lighting_cue_idx_);
}

uint32_t Gameplay::diagnostic_autoplay_fret_mask(
    const std::vector<ghogx::chart::Note>& notes) {
    const auto& consumed = note_consumed_[std::clamp(difficulty_, 0, 3)];
    uint32_t tick = UINT32_MAX;
    for (size_t i = next_note_idx_; i < notes.size(); ++i) {
        if (i < consumed.size() && consumed[i]) continue;
        const auto& note = notes[i];
        const double note_sec = chart_.tick_to_sec(note.tick_on);
        if (note_sec < song_time_ - kHitWindowSec) continue;
        if (note_sec > song_time_ + kHitWindowSec) break;
        tick = note.tick_on;
        break;
    }
    if (tick == UINT32_MAX) return 0;

    uint32_t mask = 0;
    for (size_t i = next_note_idx_; i < notes.size(); ++i) {
        if (i < consumed.size() && consumed[i]) continue;
        const auto& note = notes[i];
        if (note.tick_on < tick) continue;
        if (note.tick_on > tick) break;
        mask |= 1u << std::clamp(note.lane, 0, 4);
    }
    if (tick != diagnostic_autoplay_last_note_tick_) {
        mask |= 1u << 5;
        diagnostic_autoplay_last_note_tick_ = tick;
        if (debug_venue_filters_enabled()) {
            std::fprintf(
                stderr,
                "[gameplay] diagnostic autoplay tick=%u mask=0x%02x t=%.3f\n",
                tick, mask & 0x1fu, song_time_);
        }
    }
    return mask;
}

void Gameplay::tick(float dt, uint32_t fret_mask) {
    if (!chart_loaded_) return;

    // On the first tick, start the audio.
    const bool first_tick = (song_time_ == 0.0 && dt > 0.0f);
    if (first_tick) {
        if (!deterministic_clock_) audio_.play();
        std::fprintf(stderr, "[gameplay] song started\n");
    }

    // Master clock: the audio playback position when playing (so note timing
    // stays locked to the sound), else wall-clock accumulation.
    if (!deterministic_clock_ && audio_.is_playing())
        song_time_ = audio_.position_sec();
    else
        song_time_ += static_cast<double>(dt);

    while (next_drum_cue_idx_ < chart_.drum_cues.size()) {
        const auto& cue = chart_.drum_cues[next_drum_cue_idx_];
        const double cue_sec = chart_.tick_to_sec(cue.tick);
        if (cue_sec > song_time_) break;
        std::fprintf(stderr,
                     "[world] drummer cue: %s pitch=%d tick=%u t=%.3f\n",
                     cue.event.c_str(), cue.pitch, cue.tick, song_time_);
        apply_venue_event(cue.event, false);
        if (drum_kit_) {
            auto trigger_drum_anim = [&](const char* mesh_name) {
                auto it = drum_mesh_transform_anims_.find(mesh_name);
                if (it == drum_mesh_transform_anims_.end()) return false;
                drum_kit_->trigger_mesh_transform_anim(mesh_name, it->second,
                                                       30.0f);
                return true;
            };
            bool handled = false;
            if (auto routed = drum_event_mesh_targets_.find(cue.event);
                routed != drum_event_mesh_targets_.end()) {
                for (const auto& mesh_name : routed->second) {
                    handled = trigger_drum_anim(mesh_name.c_str()) || handled;
                }
            }
            if (handled) {
                // Routed through EventTrigger -> AnimFilter -> TransAnim.
            } else if (cue.event == "kick_drum") {
                if (!trigger_drum_anim("kick.mesh"))
                    drum_kit_->trigger_mesh_pulse("kick.mesh", 6.0f);
            } else if (cue.event == "hit_snare") {
                if (!trigger_drum_anim("snare.mesh"))
                    drum_kit_->trigger_mesh_pulse("snare.mesh", 5.0f);
            } else if (cue.event == "hit_hihat") {
                if (!trigger_drum_anim("hat.mesh"))
                    drum_kit_->trigger_mesh_pulse("hat.mesh", 4.0f);
            } else if (cue.event == "crash_symbal") {
                if (!trigger_drum_anim("crash.mesh"))
                    drum_kit_->trigger_mesh_pulse("crash.mesh", 5.0f);
            }
        }
        ++next_drum_cue_idx_;
    }

    while (next_bass_cue_idx_ < chart_.bass_cues.size()) {
        const auto& cue = chart_.bass_cues[next_bass_cue_idx_];
        const double cue_sec = chart_.tick_to_sec(cue.tick);
        if (cue_sec > song_time_) break;
        std::fprintf(stderr,
                     "[world] bass cue: %s pitch=%d tick=%u t=%.3f\n",
                     cue.event.c_str(), cue.pitch, cue.tick, song_time_);
        apply_venue_event(cue.event, false);
        ++next_bass_cue_idx_;
    }

    while (next_venue_cue_idx_ < chart_.venue_cues.size()) {
        const auto& cue = chart_.venue_cues[next_venue_cue_idx_];
        const double cue_sec = chart_.tick_to_sec(cue.tick);
        if (cue_sec > song_time_) break;
        std::fprintf(stderr,
                     "[world] venue cue: %s pitch=%d tick=%u t=%.3f\n",
                     cue.event.c_str(), cue.pitch, cue.tick, song_time_);
        apply_venue_event(cue.event, false);
        ++next_venue_cue_idx_;
    }

    // Decay per-lane hit flames (~0.22 s lifetime).
    for (int i = 0; i < 5; ++i)
        lane_flash_[i] = std::max(0.0f, lane_flash_[i] - dt * 4.5f);

    // Clear per-frame feedback.
    hit_flash_mask_  = 0;
    miss_flash_mask_ = 0;
    std::memset(lane_hit_, 0, sizeof(lane_hit_));

    const auto& notes = chart_.notes[difficulty_];
    auto& consumed = note_consumed_[difficulty_];
    if (consumed.size() != notes.size()) consumed.assign(notes.size(), 0);
    if (diagnostic_autoplay_) {
        fret_mask = diagnostic_autoplay_fret_mask(notes);
    }

    const bool strummed =
        ((fret_mask & (1u << 5)) != 0) &&
        ((prev_fret_mask_ & (1u << 5)) == 0);  // rising edge on strum bit

    // Advance next_note_idx_ past notes that are permanently missed or hit.
    while (next_note_idx_ < notes.size()) {
        if (next_note_idx_ < consumed.size() && consumed[next_note_idx_]) {
            ++next_note_idx_;
            continue;
        }
        const auto& n = notes[next_note_idx_];
        const double note_sec = chart_.tick_to_sec(n.tick_on);
        if (note_sec < song_time_ - kHitWindowSec) {
            // Note passed without being hit.
            if (!lane_hit_[n.lane]) {
                miss_flash_mask_ |= (1u << n.lane);
                streak_ = 0;
                multiplier_ = 1;
                std::fprintf(stderr, "[gameplay] miss lane=%d streak reset\n", n.lane);
            }
            if (next_note_idx_ < consumed.size()) consumed[next_note_idx_] = 1;
            ++next_note_idx_;
        } else {
            break;
        }
    }

    // Check upcoming notes for hits.
    for (size_t i = next_note_idx_; i < notes.size(); ++i) {
        if (i < consumed.size() && consumed[i]) continue;
        const auto& n = notes[i];
        const double note_sec = chart_.tick_to_sec(n.tick_on);

        // Past the lookahead window — stop processing.
        if (note_sec > song_time_ + kHitWindowSec) break;

        // Already hit this lane this frame.
        if (lane_hit_[n.lane]) continue;

        // Within hit window: note_sec ∈ [song_time - kHitWindowSec, song_time + kHitWindowSec].
        if (std::abs(note_sec - song_time_) > kHitWindowSec) continue;

        const bool lane_pressed = (fret_mask >> n.lane) & 1;
        const bool is_hopo_candidate = n.is_hopo && (streak_ > 0);

        bool can_hit = false;
        if (is_hopo_candidate && lane_pressed) {
            // HOPO: just pressing (not strumming) counts.
            // Make sure this is a new press (edge).
            const bool was_pressed = (prev_fret_mask_ >> n.lane) & 1;
            can_hit = lane_pressed && !was_pressed;
        }
        if (!can_hit && strummed && lane_pressed) {
            can_hit = true;
        }

        if (can_hit) {
            lane_hit_[n.lane] = true;
            hit_flash_mask_ |= (1u << n.lane);
            lane_flash_[n.lane] = 1.0f;  // light the strikeline flame
            ++streak_;
            // Multiplier: 1→2 at 10, 2→3 at 20, 3→4 at 30, cap at 4.
            if      (streak_ >= 30) multiplier_ = 4;
            else if (streak_ >= 20) multiplier_ = 3;
            else if (streak_ >= 10) multiplier_ = 2;
            else                    multiplier_ = 1;

            const int pts = 50 * multiplier_;
            score_ += pts;

            std::fprintf(stderr,
                "[gameplay] HIT lane=%d tick=%u pts=%d streak=%d mult=%d score=%d\n",
                n.lane, n.tick_on, pts, streak_, multiplier_, score_);
            if (i < consumed.size()) consumed[i] = 1;
            apply_venue_event(player_fret_hit_event(n.lane), false);
        }
    }
    while (next_note_idx_ < notes.size() &&
           next_note_idx_ < consumed.size() &&
           consumed[next_note_idx_]) {
        ++next_note_idx_;
    }

    if (miss_flash_mask_ != 0) {
        apply_venue_event("excitement_bad");
    } else if (streak_ >= 10) {
        apply_venue_event("excitement_great");
    } else if (active_venue_event_.empty()) {
        apply_venue_event("excitement_okay");
    }
    update_active_venue_material_anims();
    update_active_venue_environment_anims();
    update_active_venue_light_anims();
    update_active_venue_particles();
    update_active_venue_anim_filters();
    update_active_lighting_material_anims();

    prev_fret_mask_ = fret_mask;

    // Print score summary once per second.
    static double last_print = 0.0;
    if (song_time_ - last_print >= 1.0) {
        last_print = song_time_;
        std::fprintf(stderr, "[gameplay] t=%.1f score=%d streak=%d mult=%d\n",
                     song_time_, score_, streak_, multiplier_);
    }
}

// ---------------------------------------------------------------------------
// draw
// ---------------------------------------------------------------------------

void Gameplay::draw(ghogx::render::Window& win) {
    if (!chart_loaded_) return;
    if (!world_init_attempted_) {
        world_init_attempted_ = true;
        if (quickplay_rig_) {
            const std::string venue_geom =
                "world/" + quickplay_rig_->venue + "/og/gen/" +
                quickplay_rig_->venue + "_geom.milo_ps2";
            ghogx::milo_scene::Scene venue_scene;
            if (ghogx::milo_scene::load_scene(hdr_path_, ark_path_, venue_geom,
                                              venue_scene)) {
                auto venue_textures = ghogx::asset::load_milo_textures(
                    hdr_path_, ark_path_, venue_geom,
                    texture_names_for_scene(venue_scene));
                auto hidden_venue_meshes = mesh_names_in_groups(
                    venue_scene, {"coplight_red.grp",
                                  "coplight_blue.grp"});
                auto venue_mat_anims =
                    load_venue_mat_anims(hdr_path_, ark_path_, venue_geom);
                venue_mat_anims_ = std::move(venue_mat_anims);
                venue_event_mat_anims_ =
                    load_venue_event_mat_anims(hdr_path_, ark_path_,
                                               venue_geom);
                venue_env_anims_ =
                    load_venue_env_anims(hdr_path_, ark_path_, venue_geom);
                venue_event_env_anims_ =
                    load_venue_event_env_anims(hdr_path_, ark_path_,
                                               venue_geom);
                venue_light_anims_ =
                    load_venue_light_anims(hdr_path_, ark_path_, venue_geom);
                venue_event_light_anims_ =
                    load_venue_event_light_anims(hdr_path_, ark_path_,
                                                 venue_geom);
                venue_event_particle_systems_ =
                    load_venue_event_particles(hdr_path_, ark_path_,
                                               venue_geom);
                venue_event_filters_ =
                    load_venue_event_filters(hdr_path_, ark_path_, venue_geom);
                venue_filter_mesh_targets_ =
                    load_venue_filter_mesh_targets(hdr_path_, ark_path_,
                                                   venue_geom, venue_scene);
                venue_event_anim_filters_ =
                    load_venue_anim_filters(hdr_path_, ark_path_, venue_geom,
                                            venue_scene);
                venue_event_group_visibility_ =
                    load_venue_group_visibility(hdr_path_, ark_path_,
                                                venue_geom, venue_scene);
                venue_material_meshes_.clear();
                for (const auto& mesh : venue_scene.meshes) {
                    if (mesh.material.empty()) continue;
                    auto& meshes = venue_material_meshes_[mesh.material];
                    if (std::find(meshes.begin(), meshes.end(), mesh.name) ==
                        meshes.end()) {
                        meshes.push_back(mesh.name);
                    }
                }
                venue_lights_.clear();
                for (const auto& light : venue_scene.lights) {
                    if (light.decoded) venue_lights_[light.name] = light;
                }
                venue_environs_.clear();
                for (const auto& env : venue_scene.environs) {
                    if (env.decoded) venue_environs_[env.name] = env;
                }
                venue_base_hidden_meshes_ = std::move(hidden_venue_meshes);
                venue_runtime_hidden_meshes_ = venue_base_hidden_meshes_;
                apply_venue_event_visibility("start", true);
                world_ = std::make_unique<ghogx::render::MiloSceneRenderer>(win);
                world_->set_scene(std::move(venue_scene), venue_textures);
                world_->set_active_particle_systems({});
                world_->set_particle_intensities({});
                world_->set_hidden_meshes(composed_venue_hidden_meshes());
                if (active_venue_event_.empty()) {
                    apply_venue_event("excitement_bad");
                } else {
                    const std::string active = active_venue_event_;
                    active_venue_event_.clear();
                    apply_venue_event(active);
                }
                if (!pending_transient_venue_events_.empty()) {
                    auto pending = std::move(pending_transient_venue_events_);
                    pending_transient_venue_events_.clear();
                    for (const auto& event : pending)
                        apply_venue_event(event, false);
                }
                std::fprintf(stderr, "[world] venue loaded: %s\n", venue_geom.c_str());
                const VenueCameraPolicy camera_policy =
                    load_venue_camera_policy(hdr_path_, ark_path_,
                                             quickplay_rig_->venue);
                camera_duration_bars_ = camera_policy.duration_bars;
                camera_bars_left_ = 6;
                last_camera_bar_ = UINT32_MAX;
                camera_keys_ = load_camera_position_keys(
                    hdr_path_, ark_path_, quickplay_rig_->venue,
                    select_intro_camera_anim(hdr_path_, ark_path_,
                                             quickplay_rig_->venue));
                regular_camera_keys_ = load_regular_camera_keys(
                    hdr_path_, ark_path_, quickplay_rig_->venue, camera_policy);
                intro_camera_seconds_ = intro_camera_duration_seconds(chart_);
                std::fprintf(stderr,
                             "[world] intro camera window: %.3fs (6 bars)\n",
                             intro_camera_seconds_);
            }
            const std::string lighting_milo =
                "world/" + quickplay_rig_->venue + "/og/gen/" +
                quickplay_rig_->venue + "_lighting.milo_ps2";
            ghogx::milo_scene::Scene lighting_scene;
            if (ghogx::milo_scene::load_scene(hdr_path_, ark_path_,
                                              lighting_milo, lighting_scene)) {
                lighting_spotlights_.clear();
                lighting_spotlights_.reserve(lighting_scene.spotlights.size());
                for (const auto& spot : lighting_scene.spotlights) {
                    lighting_spotlights_.push_back(
                        {spot.name, spot.target, spot.material, spot.group});
                }
                lighting_mat_anims_ =
                    load_venue_mat_anims(hdr_path_, ark_path_, lighting_milo);
                lighting_event_mat_anims_ =
                    load_venue_event_mat_anims(hdr_path_, ark_path_,
                                               lighting_milo);
                lighting_presets_ = load_lighting_presets(
                    hdr_path_, ark_path_, quickplay_rig_->venue);
                log_lighting_light_object_coverage(lighting_scene,
                                                   lighting_presets_,
                                                   venue_lights_,
                                                   venue_environs_);
                auto lighting_textures = ghogx::asset::load_milo_textures(
                    hdr_path_, ark_path_, lighting_milo,
                    texture_names_for_scene(lighting_scene));
                lighting_ =
                    std::make_unique<ghogx::render::MiloSceneRenderer>(win);
                lighting_->set_scene(std::move(lighting_scene),
                                     lighting_textures);
                lighting_->set_additive_blend(true);
                apply_lighting_event("start");
                std::fprintf(stderr, "[world] lighting overlay loaded: %s\n",
                              lighting_milo.c_str());
            }

            ghogx::milo_scene::Scene chars_scene;
            const std::string chars_milo =
                "world/" + quickplay_rig_->venue + "/gen/" +
                quickplay_rig_->venue + "_chars.milo_ps2";
            ghogx::milo_scene::load_scene(hdr_path_, ark_path_, chars_milo,
                                          chars_scene);
            performers_.reserve(4);

            auto add_performer = [&](std::string role, std::string character_name,
                                     const std::string& model_name,
                                     const std::string& anim_stem,
                                     std::string_view waypoint_name,
                                     uint32_t start_flag,
                                     std::initializer_list<const char*> idle_names,
                                     std::initializer_list<const char*> intro_names,
                                     std::initializer_list<const char*> active_names,
                                     const std::string& prop_milo = std::string(),
                                     const std::string& prop_attach_bone =
                                         "bone_pos_guitar.mesh") {
                const std::string char_milo =
                    "char/" + model_name + "/og/gen/" + model_name + ".milo_ps2";
                ghogx::character::Character character;
                if (!ghogx::character::load_character(hdr_path_, ark_path_,
                                                      char_milo, character)) {
                    std::fprintf(stderr, "[world] performer failed: %s\n",
                                 char_milo.c_str());
                    return;
                }
                const auto character_drivers = character.drivers;
                const auto facefx_servos = character.lip_sync_servos;
                auto facefx_graph = ghogx::character::load_facefx_graph(
                    hdr_path_, ark_path_, char_milo, character);
                auto load_driver_clip_first =
                    [&](ghogx::character::CharClip& out,
                        const std::string& driver_name,
                        std::initializer_list<const char*> clip_names) {
                        for (const auto& driver : character_drivers) {
                            if (driver.name != driver_name ||
                                driver.clip_milo.empty())
                                continue;
                            for (const auto& candidate :
                                 driver_milo_candidates_game(
                                     char_milo, driver.clip_milo)) {
                                if (load_clip_first(out, hdr_path_, ark_path_,
                                                    candidate, clip_names)) {
                                    return true;
                                }
                            }
                        }
                        return false;
                    };
                auto load_driver_clip_names =
                    [&](ghogx::character::CharClip& out,
                        const std::string& driver_name,
                        const std::vector<std::string>& clip_names) {
                        for (const auto& driver : character_drivers) {
                            if (driver.name != driver_name ||
                                driver.clip_milo.empty())
                                continue;
                            for (const auto& candidate :
                                 driver_milo_candidates_game(
                                     char_milo, driver.clip_milo)) {
                                if (load_clip_first(out, hdr_path_, ark_path_,
                                                    candidate, clip_names)) {
                                    return true;
                                }
                            }
                        }
                        return false;
                    };
                auto driver_milos_for = [&](const std::string& driver_name) {
                    std::vector<std::string> milos;
                    for (const auto& driver : character_drivers) {
                        if (driver.name != driver_name || driver.clip_milo.empty())
                            continue;
                        for (const auto& candidate :
                             driver_milo_candidates_game(char_milo,
                                                         driver.clip_milo)) {
                            if (std::find(milos.begin(), milos.end(),
                                          candidate) == milos.end()) {
                                milos.push_back(candidate);
                            }
                        }
                    }
                    return milos;
                };
                auto textures = ghogx::asset::load_milo_textures(
                    hdr_path_, ark_path_, char_milo, character.texture_names());

                Performer perf;
                perf.role = std::move(role);
                perf.character_name = std::move(character_name);
                perf.event_track = performer_event_track_for_role(perf.role);
                perf.renderer =
                    std::make_unique<ghogx::character::CharRenderer>(win);
                perf.renderer->set_character(std::move(character), textures);
                perf.facefx_graph = std::move(facefx_graph);

                const auto start =
                    perf.role == "guitarist0"
                        ? find_start_xfm(chars_scene, waypoint_name,
                                         {start_flag, 1u})
                        : find_start_xfm(chars_scene, waypoint_name,
                                         {start_flag});
                if (start) {
                    perf.world_transform = xfm_to_mat4(*start);
                    perf.renderer->set_world_transform(perf.world_transform);
                    std::fprintf(stderr,
                                 "[world] %s start xfm flags=%u pos=(%.1f %.1f %.1f)\n",
                                 perf.role.c_str(), start_flag, start->pos[0],
                                 start->pos[1], start->pos[2]);
                    if (debug_performer_start_enabled()) {
                        std::fprintf(
                            stderr,
                            "[world-start-row] %s row0 %.5f %.5f %.5f %.5f\n",
                            perf.role.c_str(), perf.world_transform[0],
                            perf.world_transform[1], perf.world_transform[2],
                            perf.world_transform[3]);
                        std::fprintf(
                            stderr,
                            "[world-start-row] %s row1 %.5f %.5f %.5f %.5f\n",
                            perf.role.c_str(), perf.world_transform[4],
                            perf.world_transform[5], perf.world_transform[6],
                            perf.world_transform[7]);
                        std::fprintf(
                            stderr,
                            "[world-start-row] %s row2 %.5f %.5f %.5f %.5f\n",
                            perf.role.c_str(), perf.world_transform[8],
                            perf.world_transform[9], perf.world_transform[10],
                            perf.world_transform[11]);
                        std::fprintf(
                            stderr,
                            "[world-start-row] %s row3 %.5f %.5f %.5f %.5f\n",
                            perf.role.c_str(), perf.world_transform[12],
                            perf.world_transform[13], perf.world_transform[14],
                            perf.world_transform[15]);
                    }
                }

                if (!prop_milo.empty()) {
                    ghogx::milo_scene::Scene prop_scene;
                    if (ghogx::milo_scene::load_scene(hdr_path_, ark_path_,
                                                      prop_milo, prop_scene)) {
                        auto prop_textures = ghogx::asset::load_milo_textures(
                            hdr_path_, ark_path_, prop_milo,
                            texture_names_for_scene(prop_scene));
                        perf.renderer->set_attached_prop(
                            std::move(prop_scene), prop_textures,
                            prop_attach_bone);
                    }
                }

                const std::string anim_milo =
                    "char/" + model_name + "/anims/gen/" + anim_stem +
                    "_main.milo_ps2";
                std::vector<std::string> main_anim_milos =
                    driver_milos_for("main.drv");
                if (std::find(main_anim_milos.begin(), main_anim_milos.end(),
                              anim_milo) == main_anim_milos.end()) {
                    main_anim_milos.push_back(anim_milo);
                }
                std::vector<std::string> active_group_names;
                if (perf.role == "guitarist0") {
                    active_group_names = load_char_clip_group(
                        hdr_path_, ark_path_, main_anim_milos, "normal");
                }
                if (!load_driver_clip_first(perf.idle_clip, "main.drv",
                                            idle_names)) {
                    load_clip_first(perf.idle_clip, hdr_path_, ark_path_,
                                    anim_milo, idle_names);
                }
                if (!load_driver_clip_first(perf.intro_clip, "main.drv",
                                            intro_names)) {
                    load_clip_first(perf.intro_clip, hdr_path_, ark_path_,
                                    anim_milo, intro_names);
                }
                if (!load_driver_clip_first(perf.active_clip, "main.drv",
                                            active_names)) {
                    load_clip_first(perf.active_clip, hdr_path_, ark_path_,
                                    anim_milo, active_names);
                }
                std::vector<std::string> face_milos =
                    facefx_viseme_milo_candidates_game(char_milo,
                                                       facefx_servos);
                const std::string fallback_face_milo =
                    "char/" + model_name + "/anims/gen/" + anim_stem +
                    "_viseme.milo_ps2";
                if (std::find(face_milos.begin(), face_milos.end(),
                              fallback_face_milo) == face_milos.end()) {
                    face_milos.push_back(fallback_face_milo);
                }
                load_clip_first_from_milos(perf.face_base_clip, hdr_path_,
                                           ark_path_, face_milos,
                                           std::vector<std::string>{"neutral"});
                keep_face_channels(perf.face_base_clip);
                if (!active_group_names.empty()) {
                    for (const auto& clip_name : active_group_names) {
                        ghogx::character::CharClip clip;
                        if (load_clip_first_from_milos(
                                clip, hdr_path_, ark_path_, main_anim_milos,
                                std::vector<std::string>{clip_name})) {
                            perf.active_group_clips.push_back(std::move(clip));
                        }
                    }
                    if (!perf.active_group_clips.empty()) {
                        const std::vector<ghogx::character::ClipChannel>
                            idle_reference =
                                perf.idle_clip.frames.empty()
                                    ? std::vector<ghogx::character::ClipChannel>{}
                                    : perf.idle_clip.frames.front();
                        perf.active_group_index = choose_graph_continuity_clip(
                            perf.active_group_clips, idle_reference, 0, 0,
                            perf.character_name, false);
                        perf.active_group_index = choose_stance_continuity_clip(
                            perf.active_group_clips, idle_reference,
                            perf.renderer->character(), perf.active_group_index,
                            0, 0, perf.character_name, false);
                        perf.active_clip =
                            perf.active_group_clips[perf.active_group_index];
                        std::fprintf(
                            stderr,
                            "[world] CharClipGroup normal selected entry: role=%s char=%s clip=%s index=%zu\n",
                            perf.role.c_str(), perf.character_name.c_str(),
                            perf.active_clip.name.c_str(),
                            perf.active_group_index);
                    }
                }
                if (perf.role == "drummer") {
                    if (!load_driver_clip_first(
                            perf.active_allbeat_clip, "main.drv",
                            {"drummer_active_medium_allbeat"})) {
                        load_clip_first(perf.active_allbeat_clip, hdr_path_,
                                        ark_path_, anim_milo,
                                        {"drummer_active_medium_allbeat"});
                    }
                    if (!load_driver_clip_first(
                            perf.active_half_clip, "main.drv",
                            {"drummer_active_medium_half"})) {
                        load_clip_first(perf.active_half_clip, hdr_path_,
                                        ark_path_, anim_milo,
                                        {"drummer_active_medium_half"});
                    }
                    if (!load_driver_clip_first(
                            perf.active_nosnare_clip, "main.drv",
                            {"drummer_active_medium_nosnare"})) {
                        load_clip_first(perf.active_nosnare_clip, hdr_path_,
                                        ark_path_, anim_milo,
                                        {"drummer_active_medium_nosnare"});
                    }
                }
                const auto& hand_character = perf.renderer->character();
                const bool graph_has_midi_hand_driver =
                    !hand_character.ik_hands.empty() &&
                    !hand_character.ik_midis.empty();
                const bool graph_has_hand_clip_drivers =
                    std::any_of(character_drivers.begin(),
                                character_drivers.end(),
                                [](const auto& driver) {
                                    return (driver.name == "left_hand.drv" ||
                                            driver.name == "right_hand.drv") &&
                                           !driver.clip_milo.empty();
                                });
                if (graph_has_midi_hand_driver &&
                    graph_has_hand_clip_drivers) {
                    const std::string strum_milo =
                        "char/" + model_name + "/anims/gen/" + anim_stem +
                        "_strum.milo_ps2";
                    const std::string fret_milo =
                        "char/" + model_name + "/anims/gen/" + anim_stem +
                        "_fret.milo_ps2";
                    if (!load_driver_clip_first(
                            perf.strum_open_clip, "right_hand.drv",
                            {"strum_open"})) {
                        load_clip_first(perf.strum_open_clip, hdr_path_,
                                        ark_path_, strum_milo,
                                        {"strum_open"});
                    }
                    keep_hand_overlay_channels(perf.strum_open_clip);
                    if (!load_driver_clip_first(
                            perf.strum_clip, "right_hand.drv",
                            {"strum_short_01", "strum_long_01",
                             "strum_pick_01"})) {
                        load_clip_first(perf.strum_clip, hdr_path_, ark_path_,
                                        strum_milo,
                                        {"strum_short_01", "strum_long_01",
                                         "strum_pick_01"});
                    }
                    keep_hand_overlay_channels(perf.strum_clip);
                    for (const auto& clip_name :
                         all_strum_hand_clip_names(strum_hand_maps_)) {
                        if (clip_name == "strum_open") continue;
                        ghogx::character::CharClip named_clip;
                        const std::vector<std::string> names{clip_name};
                        if (!load_driver_clip_names(named_clip,
                                                    "right_hand.drv",
                                                    names)) {
                            load_clip_first(named_clip, hdr_path_, ark_path_,
                                            strum_milo, names);
                        }
                        if (named_clip.loaded) {
                            keep_hand_overlay_channels(named_clip);
                            perf.strum_named_clips[clip_name] =
                                std::move(named_clip);
                        }
                    }
                    static const char* kLaneFretClips[5] = {
                        "finger_hold_index", "finger_hold_middle",
                        "finger_hold_ring", "finger_hold_pinky",
                        "finger_hold_pinky_hi"};
                    for (const char* clip_name : kLaneFretClips) {
                        ghogx::character::CharClip lane_clip;
                        if (!load_driver_clip_first(lane_clip, "left_hand.drv",
                                                    {clip_name})) {
                            load_clip_first(lane_clip, hdr_path_, ark_path_,
                                            fret_milo, {clip_name});
                        }
                        keep_hand_overlay_channels(lane_clip);
                        perf.fret_lane_clips.push_back(std::move(lane_clip));
                    }
                    if (!load_driver_clip_first(
                            perf.fret_open_clip, "left_hand.drv",
                            {"finger_open"})) {
                        load_clip_first(perf.fret_open_clip, hdr_path_,
                                        ark_path_, fret_milo,
                                        {"finger_open"});
                    }
                    keep_hand_overlay_channels(perf.fret_open_clip);
                    if (!load_driver_clip_first(
                            perf.fret_clip, "left_hand.drv",
                            {"finger_powerchord_1", "finger_chord_bar",
                             "finger_open"})) {
                        load_clip_first(perf.fret_clip, hdr_path_, ark_path_,
                                        fret_milo, {"finger_powerchord_1",
                                                    "finger_chord_bar",
                                                    "finger_open"});
                    }
                    keep_hand_overlay_channels(perf.fret_clip);
                    for (const auto& clip_name :
                         all_fret_hand_clip_names(fret_hand_maps_)) {
                        ghogx::character::CharClip named_clip;
                        const std::vector<std::string> names{clip_name};
                        if (!load_driver_clip_names(named_clip, "left_hand.drv",
                                                    names)) {
                            load_clip_first(named_clip, hdr_path_, ark_path_,
                                            fret_milo, names);
                        }
                        if (named_clip.loaded) {
                            keep_hand_overlay_channels(named_clip);
                            perf.fret_named_clips[clip_name] =
                                std::move(named_clip);
                        }
                    }
                    const bool right_hand_loaded =
                        perf.strum_open_clip.loaded || perf.strum_clip.loaded ||
                        !perf.strum_named_clips.empty();
                    const bool left_hand_loaded =
                        perf.fret_open_clip.loaded || perf.fret_clip.loaded ||
                        !perf.fret_named_clips.empty();
                    perf.hand_driver_available =
                        graph_has_midi_hand_driver && right_hand_loaded &&
                        left_hand_loaded;
                    std::fprintf(stderr,
                                 "[world] performer hand map clips: role=%s handDriver=%d loaded=%zu maps=%zu ikHands=%zu ikMidis=%zu\n",
                                 perf.role.c_str(),
                                 perf.hand_driver_available ? 1 : 0,
                                 perf.fret_named_clips.size() +
                                     perf.strum_named_clips.size(),
                                 fret_hand_maps_.size(),
                                 hand_character.ik_hands.size(),
                                 hand_character.ik_midis.size());
                } else if (perf.role == "guitarist0" ||
                           perf.role == "bassist") {
                    std::fprintf(stderr,
                                 "[world] performer hand map skipped: role=%s handDriver=0 handGraph=%d handClips=%d ikHands=%zu ikMidis=%zu\n",
                                 perf.role.c_str(),
                                 graph_has_midi_hand_driver ? 1 : 0,
                                 graph_has_hand_clip_drivers ? 1 : 0,
                                 hand_character.ik_hands.size(),
                                 hand_character.ik_midis.size());
                }
                std::fprintf(stderr,
                             "[world] performer loaded: role=%s track=%s char=%s model=%s\n",
                             perf.role.c_str(), perf.event_track.c_str(),
                             perf.character_name.c_str(),
                             char_milo.c_str());
                performers_.push_back(std::move(perf));
                Performer& stored = performers_.back();
                if (stored.idle_clip.loaded) {
                    stored.idle_player.play(
                        stored.idle_clip, ghogx::character::kCharPlayLoop |
                                              ghogx::character::kCharPlayNoBlend);
                }
                if (stored.intro_clip.loaded) {
                    stored.intro_player.play(
                        stored.intro_clip, ghogx::character::kCharPlayNoLoop |
                                               ghogx::character::kCharPlayNoBlend);
                }
                if (stored.active_clip.loaded) {
                    const uint32_t flags =
                        stored.active_group_clips.empty()
                            ? (ghogx::character::kCharPlayLoop |
                               ghogx::character::kCharPlayNoBlend)
                            : ghogx::character::kCharPlayNoLoop;
                    stored.active_group_started = song_time_;
                    stored.active_group_last_bar = camera_bar_at(chart_, song_time_);
                    stored.active_player.play(
                        stored.active_clip, flags,
                        stored.active_group_clips.empty()
                            ? -1.0f
                            : character_driver_blend_seconds());
                }
                if (stored.face_base_clip.loaded) {
                    stored.face_base_player.play(
                        stored.face_base_clip,
                        ghogx::character::kCharPlayLoop |
                            ghogx::character::kCharPlayNoBlend);
                }
                if (stored.hand_driver_available &&
                    stored.strum_open_clip.loaded) {
                    stored.strum_player.play(
                        stored.strum_open_clip,
                        ghogx::character::kCharPlayLoop |
                            ghogx::character::kCharPlayNoBlend);
                }
                if (stored.hand_driver_available &&
                    stored.fret_open_clip.loaded) {
                    stored.fret_player.play(
                        stored.fret_open_clip,
                        ghogx::character::kCharPlayLoop |
                            ghogx::character::kCharPlayNoBlend);
                }
            };

            add_performer("guitarist0", quickplay_rig_->character_outfit,
                          quickplay_rig_->character_outfit,
                          quickplay_rig_->character_outfit,
                          "start_guitarist0.way", 1u,
                          {"idle_medium_01", "stand_medium_01"},
                          {"intro_01", "intro_03", "intro_04"},
                          {"stand_medium_01", "stand_medium_02",
                           "stand_medium_03", "stand_medium_04",
                           "stand_fast_01", "stand_fast_02",
                           "stand_fast_03", "stand_fast_04"},
                          guitar_milo_for_quickplay(quickplay_rig_->guitar));

            const BandRoleNames band_roles =
                classify_band_roles(quickplay_rig_->band);
            const std::string& singer = band_roles.singer;
            const std::string& bass = band_roles.bass;
            const std::string& drummer = band_roles.drummer;
            const std::string& keyboard = band_roles.keyboard;
            if (!singer.empty()) {
                add_performer("singer", singer, singer, "singer",
                              "singer_start.way", 4u,
                              {"singer_idle_medium_01", "singer_idle_medium_02"},
                              {"singer_intro"},
                              {"singer_active_medium_01",
                               "singer_active_medium_02"});
            }
            if (!bass.empty()) {
                const std::string bass_prop =
                    first_bass_guitar_milo(hdr_path_, ark_path_).value_or("");
                add_performer("bassist", bass, bass, "bass",
                              "bassist_start.way", 16u,
                              {"bassist_idle_medium_01",
                               "bassist_idle_medium_02"},
                              {"bassist_intro"},
                              {"bassist_active_medium_01",
                               "bassist_active_medium_02"},
                              bass_prop, "bone_pos_gutbass.mesh");
            }
            if (!drummer.empty()) {
                add_performer("drummer", drummer, drummer, "drummer",
                              "drummer_start.way", 32u, {"drummer_idle"},
                              {},
                              {"drummer_active_medium_normal",
                               "drummer_active_medium_allbeat"});
                if (auto start = find_start_xfm(chars_scene,
                                                "drummer_start.way", {32u})) {
                    const std::string drums_milo =
                        "char/og/drums/gen/dw_" + quickplay_rig_->venue +
                        "_drums.milo_ps2";
                    ghogx::milo_scene::Scene drums_scene;
                    if (ghogx::milo_scene::load_scene(hdr_path_, ark_path_,
                                                      drums_milo, drums_scene)) {
                        auto drum_textures = ghogx::asset::load_milo_textures(
                            hdr_path_, ark_path_, drums_milo,
                            texture_names_for_scene(drums_scene));
                        drum_kit_ =
                            std::make_unique<ghogx::render::MiloSceneRenderer>(
                                win);
                        drum_kit_->set_scene(std::move(drums_scene),
                                             drum_textures);
                        drum_kit_->set_world_transform(xfm_to_mat4(*start));
                        auto drum_anim_data =
                            load_drum_anim_data(hdr_path_, ark_path_,
                                                drums_milo);
                        drum_mesh_transform_anims_ =
                            std::move(drum_anim_data.mesh_transform_anims);
                        drum_event_mesh_targets_ =
                            std::move(drum_anim_data.event_mesh_targets);
                        std::fprintf(stderr,
                                     "[world] drums loaded: %s pos=(%.1f %.1f %.1f)\n",
                                     drums_milo.c_str(), start->pos[0],
                                     start->pos[1], start->pos[2]);
                    }
                }
            }
            if (!keyboard.empty()) {
                add_performer("keyboard", keyboard, keyboard, "keyboard",
                              "start_singer.way", 4u, {"keyboard_idle"}, {},
                              {"keyboard_active_medium",
                               "keyboard_active_fast"});
            }
        }
    }

    if (world_) {
        const double dt = (last_anim_time_ < 0.0)
                              ? 0.0
                              : std::max(0.0, song_time_ - last_anim_time_);
        last_anim_time_ = song_time_;
        const auto& performer_guitar_notes = performer_chart_notes(chart_.notes);
        const auto& performer_bass_notes =
            performer_chart_notes(chart_.bass_notes);
        const auto& performer_guitar_hand_cues =
            performer_hand_cues(chart_.fret_hand_cues);
        const auto& performer_bass_hand_cues =
            performer_hand_cues(chart_.bass_fret_hand_cues);
        const NoteCue note_cue =
            current_note_cue(song_time_, chart_, performer_guitar_notes);
        const bool intro_active =
            intro_camera_seconds_ > 0.0 && song_time_ < intro_camera_seconds_;
        if (drum_kit_) {
            drum_kit_->update(static_cast<float>(dt));
        }
        for (auto& perf : performers_) {
            if (!perf.renderer) continue;
            PerformerMidiState midi_state =
                performer_midi_state_at(chart_, perf.event_track, song_time_);
            if (perf.role == "keyboard" && midi_state.marker.empty()) {
                midi_state.playing = true;
            }
            if (perf.last_midi_marker != midi_state.marker) {
                perf.last_midi_marker = midi_state.marker;
                perf.midi_playing = midi_state.playing;
                std::fprintf(stderr,
                             "[world] performer midi: role=%s track=%s marker=%s playing=%d wail=%d solo=%d allbeat=%d halftime=%d nosnare=%d handmap=%s t=%.3f\n",
                             perf.role.c_str(), perf.event_track.c_str(),
                             midi_state.marker.c_str(), midi_state.playing ? 1 : 0,
                             midi_state.wail ? 1 : 0, midi_state.solo ? 1 : 0,
                             midi_state.allbeat ? 1 : 0,
                             midi_state.half_time ? 1 : 0,
                             midi_state.no_snare ? 1 : 0,
                             midi_state.hand_map.c_str(), song_time_);
            }
            const bool performer_playing = midi_state.playing;
            auto& character = perf.renderer->character();
            const ghogx::character::CharClip* desired_active = &perf.active_clip;
            std::string desired_mode = "normal";
            if (perf.role == "drummer") {
                if (midi_state.allbeat && perf.active_allbeat_clip.loaded) {
                    desired_active = &perf.active_allbeat_clip;
                    desired_mode = "allbeat";
                } else if (midi_state.half_time && perf.active_half_clip.loaded) {
                    desired_active = &perf.active_half_clip;
                    desired_mode = "half";
                } else if (midi_state.no_snare &&
                           perf.active_nosnare_clip.loaded) {
                    desired_active = &perf.active_nosnare_clip;
                    desired_mode = "nosnare";
                }
            }
            if (desired_active->loaded && perf.active_clip_mode != desired_mode) {
                perf.active_clip_mode = desired_mode;
                if (!perf.active_group_clips.empty() &&
                    desired_mode == "normal") {
                    const auto reference = perf.active_player.active()
                                               ? perf.active_player.sampled_pose()
                                               : perf.idle_player.sampled_pose();
                    perf.active_group_index = choose_graph_continuity_clip(
                        perf.active_group_clips, reference,
                        perf.active_group_index, camera_bar_at(chart_, song_time_),
                        perf.character_name, false);
                    perf.active_group_index = choose_stance_continuity_clip(
                        perf.active_group_clips, reference, character,
                        perf.active_group_index, perf.active_group_index,
                        camera_bar_at(chart_, song_time_), perf.character_name,
                        false);
                    perf.active_group_started = song_time_;
                    perf.active_group_last_bar = camera_bar_at(chart_, song_time_);
                    perf.active_player.play(
                        perf.active_group_clips[perf.active_group_index],
                        ghogx::character::kCharPlayNoLoop,
                        character_driver_blend_seconds());
                    desired_active = &perf.active_group_clips[perf.active_group_index];
                } else {
                    perf.active_player.play(
                        *desired_active,
                        ghogx::character::kCharPlayLoop |
                            ghogx::character::kCharPlayNoBlend);
                }
                std::fprintf(stderr,
                             "[world] performer active clip: role=%s mode=%s clip=%s t=%.3f\n",
                             perf.role.c_str(), desired_mode.c_str(),
                             desired_active->name.c_str(), song_time_);
            }
            perf.renderer->update(static_cast<float>(dt));
            perf.idle_player.advance(static_cast<float>(dt));
            perf.intro_player.advance(static_cast<float>(dt));
            perf.active_player.advance(static_cast<float>(dt));
            perf.face_base_player.advance(static_cast<float>(dt));
            perf.strum_open_player.advance(static_cast<float>(dt));
            perf.strum_player.advance(static_cast<float>(dt));
            perf.fret_open_player.advance(static_cast<float>(dt));
            perf.fret_player.advance(static_cast<float>(dt));
            for (auto& player : perf.fret_extra_players) {
                player.advance(static_cast<float>(dt));
            }
            if (!intro_active && performer_playing &&
                !perf.active_group_clips.empty() &&
                perf.active_clip_mode == "normal") {
                const uint32_t bar = camera_bar_at(chart_, song_time_);
                if (perf.active_group_last_bar == UINT32_MAX) {
                    perf.active_group_last_bar = bar;
                }
                if (bar != perf.active_group_last_bar) {
                    const auto reference = perf.active_player.sampled_pose();
                    perf.active_group_index = choose_graph_continuity_clip(
                        perf.active_group_clips, reference,
                        perf.active_group_index, bar, perf.character_name, true);
                    perf.active_group_index = choose_stance_continuity_clip(
                        perf.active_group_clips, reference, character,
                        perf.active_group_index, perf.active_group_index, bar,
                        perf.character_name, true);
                    perf.active_group_started = song_time_;
                    perf.active_group_last_bar = bar;
                    const auto& next =
                        perf.active_group_clips[perf.active_group_index];
                    perf.active_player.play(
                        next, ghogx::character::kCharPlayNoLoop,
                        character_driver_blend_seconds());
                    std::fprintf(stderr,
                                 "[world] performer group clip: role=%s group=normal clip=%s t=%.3f\n",
                                 perf.role.c_str(), next.name.c_str(),
                                 song_time_);
                }
            }
            const NoteCue perf_note_cue =
                (perf.role == "bassist")
                    ? current_note_cue(song_time_, chart_,
                                       performer_bass_notes)
                    : note_cue;
            const bool use_fret_hand_parser =
                (perf.role == "bassist")
                    ? !performer_bass_hand_cues.empty()
                    : !performer_guitar_hand_cues.empty();
            const NoteCue perf_anim_note_cue =
                (perf.role == "bassist")
                    ? (use_fret_hand_parser
                           ? current_fret_hand_cue(
                                 song_time_, chart_, performer_bass_hand_cues)
                           : performer_animation_note_cue(
                                 song_time_, chart_, performer_bass_notes))
                    : (use_fret_hand_parser
                           ? current_fret_hand_cue(
                                 song_time_, chart_, performer_guitar_hand_cues)
                           : performer_animation_note_cue(
                                 song_time_, chart_, performer_guitar_notes));
            const FretPositionState perf_fret_pos =
                (perf.role == "bassist")
                    ? current_fret_position_state(
                          song_time_, chart_, chart_.bass_fret_positions)
                    : current_fret_position_state(
                          song_time_, chart_, chart_.fret_positions);
            const bool hand_driver_active =
                !intro_active && perf.hand_driver_available;
            auto trigger_strum_clip = [&](const NoteCue& cue) {
                if (!cue.active || cue.tick == perf.last_note_tick) return;
                const ghogx::character::CharClip* next_clip = nullptr;
                std::vector<std::string> requested_strum_names;
                std::vector<std::string> selected_strum_names;
                const auto strum_selection = strum_clip_selection_for_note(
                    strum_hand_maps_, "StrumMap_Default", cue.length,
                    perf.strum_hand_scheduler_child_index);
                requested_strum_names = strum_selection.choices;
                for (const auto& clip_name : strum_selection.selected) {
                    auto named = perf.strum_named_clips.find(clip_name);
                    if (named != perf.strum_named_clips.end() &&
                        named->second.loaded) {
                        next_clip = &named->second;
                        selected_strum_names.push_back(clip_name);
                        break;
                    }
                }
                if (!next_clip && perf.strum_clip.loaded) {
                    next_clip = &perf.strum_clip;
                    selected_strum_names.push_back(perf.strum_clip.name);
                }
                if (!next_clip) return;
                if (env_value("GHOGX_DEBUG_HAND_MAP") != nullptr) {
                    std::fprintf(
                        stderr,
                        "[strummap] role=%s map=StrumMap_Default tick=%u len=%.3f choices=",
                        perf.role.c_str(), cue.tick, cue.length);
                    for (size_t i = 0; i < requested_strum_names.size(); ++i) {
                        std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                                     requested_strum_names[i].c_str());
                    }
                    std::fprintf(stderr, " selected=");
                    for (size_t i = 0; i < selected_strum_names.size(); ++i) {
                        std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                                     selected_strum_names[i].c_str());
                    }
                    std::fprintf(stderr, "\n");
                }
                perf.last_note_tick = cue.tick;
                perf.last_strum_started = song_time_;
                perf.last_strum_duration = next_clip->duration_seconds();
                perf.active_strum_clip_names = selected_strum_names;
                perf.strum_player.play(
                    *next_clip, ghogx::character::kCharPlayNoLoop,
                    character_hand_driver_blend_seconds());
                ++perf.strum_hand_scheduler_child_index;
            };
            if (!intro_active && performer_playing && hand_driver_active &&
                (perf.role == "guitarist0" || perf.role == "bassist")) {
                trigger_strum_clip(perf_note_cue);
            }
            const double strum_duration = perf.last_strum_duration;
            const bool strum_overlay_live =
                strum_duration > 0.0 &&
                song_time_ - perf.last_strum_started <= strum_duration;
            if (hand_driver_active && !strum_overlay_live &&
                perf.strum_open_clip.loaded &&
                perf.strum_player.current_clip() != &perf.strum_open_clip) {
                perf.strum_player.play(
                    perf.strum_open_clip, ghogx::character::kCharPlayLoop,
                    character_hand_driver_blend_seconds());
                perf.active_strum_clip_names = {"strum_open"};
            }
            std::vector<ghogx::character::ClipChannelLayer> pose_layers;
            bool pose_relative = false;
            bool pose_relative_set = false;
            auto add_player_layer =
                [&](const ghogx::character::CharClipPlayer& player,
                    float weight, bool overlay_override = false) {
                    if (!player.active()) return;
                    auto channels = player.sampled_pose();
                    if (channels.empty()) return;
                    const bool relative = player.sampled_pose_relative();
                    if (!pose_relative_set) {
                        pose_relative = relative;
                        pose_relative_set = true;
                    } else if (pose_relative != relative) {
                        pose_relative = false;
                    }
                    const auto* clip = player.current_clip();
                    pose_layers.push_back(
                        ghogx::character::ClipChannelLayer{
                            std::move(channels), weight,
                            clip ? &clip->output_bones : nullptr,
                            clip ? clip->name : std::string{},
                            relative,
                            overlay_override});
                };

            if (hand_driver_active) {
                const uint32_t desired_mask =
                    perf_anim_note_cue.active ? (perf_anim_note_cue.mask & 0x1fu)
                                              : 0u;
                const uint32_t desired_tick = perf_anim_note_cue.active
                                                  ? perf_anim_note_cue.tick
                                                  : UINT32_MAX;
                const bool same_fret_note_event =
                    desired_mask != 0 &&
                    desired_mask == perf.last_anim_note_mask &&
                    desired_tick == perf.last_anim_note_tick;
                std::vector<const ghogx::character::CharClip*> next_fret_clips;
                std::vector<std::string> requested_fret_names;
                std::vector<std::string> next_fret_names;
                auto push_fret_clip =
                    [&](std::string label,
                        const ghogx::character::CharClip* clip) {
                        if (!clip || !clip->loaded) return;
                        if (label.empty()) label = clip->name;
                        next_fret_clips.push_back(clip);
                        next_fret_names.push_back(std::move(label));
                    };

                if (same_fret_note_event) {
                    // The PS2 hand driver schedules one child for the MIDI note
                    // event. Keep that choice stable until a new tick/mask arrives;
                    // otherwise list-valued HandMap entries rotate on the frame
                    // after the event and replay the wrong child.
                    requested_fret_names = perf.active_fret_clip_names;
                    next_fret_names = perf.active_fret_clip_names;
                } else if (desired_mask == 0) {
                    requested_fret_names.push_back("finger_open");
                    push_fret_clip("finger_open",
                                   perf.fret_open_clip.loaded
                                       ? &perf.fret_open_clip
                                       : nullptr);
                } else {
                    const auto fret_selection = fret_clip_selection_for_note(
                        fret_hand_maps_, midi_state.hand_map, desired_mask,
                        perf_anim_note_cue.length,
                        perf.fret_hand_scheduler_child_index);
                    requested_fret_names = fret_selection.choices;
                    for (const auto& clip_name : fret_selection.selected) {
                        auto named = perf.fret_named_clips.find(clip_name);
                        if (named != perf.fret_named_clips.end() &&
                            named->second.loaded) {
                            push_fret_clip(clip_name, &named->second);
                        }
                    }
                    if (next_fret_clips.empty() &&
                        (desired_mask & (desired_mask - 1u)) != 0 &&
                        perf.fret_clip.loaded) {
                        push_fret_clip(perf.fret_clip.name, &perf.fret_clip);
                    }
                    if (next_fret_clips.empty()) {
                        for (size_t lane = 0; lane < perf.fret_lane_clips.size();
                             ++lane) {
                            if ((desired_mask & (1u << lane)) == 0) continue;
                            if (perf.fret_lane_clips[lane].loaded) {
                                push_fret_clip(perf.fret_lane_clips[lane].name,
                                               &perf.fret_lane_clips[lane]);
                            }
                            break;
                        }
                    }
                }

                const bool new_fret_note_event =
                    desired_mask != 0 &&
                    desired_tick != perf.last_anim_note_tick;
                if (desired_mask != perf.last_anim_note_mask ||
                    desired_tick != perf.last_anim_note_tick ||
                    next_fret_names != perf.active_fret_clip_names) {
                    if (env_value("GHOGX_DEBUG_HAND_MAP") != nullptr) {
                        std::fprintf(
                            stderr,
                            "[handmap] role=%s source=%s map=%s mask=0x%02x tick=%u len=%.3f choices=",
                            perf.role.c_str(),
                            use_fret_hand_parser ? "player_fret"
                                                 : "note_fallback",
                            midi_state.hand_map.c_str(),
                            desired_mask & 0x1fu, desired_tick,
                            perf_anim_note_cue.length);
                        for (size_t i = 0; i < requested_fret_names.size();
                             ++i) {
                            std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                                         requested_fret_names[i].c_str());
                        }
                        std::fprintf(stderr, " selected=");
                        for (size_t i = 0; i < next_fret_names.size(); ++i) {
                            std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                                         next_fret_names[i].c_str());
                        }
                        std::fprintf(stderr, " players=%zu\n",
                                     next_fret_clips.size());
                    }
                    perf.last_anim_note_mask = desired_mask;
                    perf.last_anim_note_tick = desired_tick;
                    perf.active_fret_clip_names = next_fret_names;
                    if (next_fret_clips.empty()) {
                        perf.fret_player.clear();
                        for (auto& player : perf.fret_extra_players)
                            player.clear();
                        perf.fret_extra_players.clear();
                    } else {
                        perf.fret_player.play(
                            *next_fret_clips.front(),
                            ghogx::character::kCharPlayLoop,
                            character_hand_driver_blend_seconds());
                        const size_t extra_count = next_fret_clips.size() - 1;
                        if (perf.fret_extra_players.size() > extra_count) {
                            for (size_t i = extra_count;
                                 i < perf.fret_extra_players.size(); ++i) {
                                perf.fret_extra_players[i].clear();
                            }
                            perf.fret_extra_players.resize(extra_count);
                        } else if (perf.fret_extra_players.size() <
                                   extra_count) {
                            perf.fret_extra_players.resize(extra_count);
                        }
                        for (size_t i = 0; i < extra_count; ++i) {
                            perf.fret_extra_players[i].play(
                                *next_fret_clips[i + 1],
                                ghogx::character::kCharPlayLoop,
                                character_hand_driver_blend_seconds());
                        }
                    }
                    if (new_fret_note_event) {
                        ++perf.fret_hand_scheduler_child_index;
                    }
                }
            }

            if (intro_active && perf.intro_player.active()) {
                add_player_layer(perf.intro_player, 1.0f);
            } else if (!intro_active && performer_playing &&
                       perf.active_player.active()) {
                add_player_layer(perf.active_player, 1.0f);
            } else if (perf.idle_player.active()) {
                add_player_layer(perf.idle_player, 1.0f);
            }
            add_player_layer(perf.face_base_player, 1.0f);
            if (hand_driver_active) {
                add_player_layer(perf.strum_player, 1.0f, true);
                add_player_layer(perf.fret_player, 1.0f, true);
                for (const auto& player : perf.fret_extra_players) {
                    add_player_layer(player, 1.0f, true);
                }
            }
            ghogx::character::clear_runtime_trans_worlds(character);
            if (!pose_layers.empty()) {
                ghogx::character::apply_clip_channel_layers(
                    pose_layers, character, pose_relative);
            }
            if (hand_driver_active) {
                const uint32_t debug_hand_mask =
                    perf_anim_note_cue.active
                        ? (perf_anim_note_cue.mask & 0x1fu)
                        : 0u;
                const uint32_t debug_hand_tick =
                    perf_anim_note_cue.active ? perf_anim_note_cue.tick
                                              : UINT32_MAX;
                const auto guitar_strings_world =
                    perf.renderer
                        ? perf.renderer->attached_prop_world(
                              "guitar_strings.mesh")
                        : std::nullopt;
                dump_hand_pose_rows(
                    character, perf.role, "postclip", song_time_,
                    debug_hand_tick, debug_hand_mask,
                    perf.active_strum_clip_names, perf.active_fret_clip_names,
                    &perf.world_transform,
                    guitar_strings_world ? &*guitar_strings_world : nullptr);
            }
            ghogx::character::clear_runtime_ik_weights(character);
            if (hand_driver_active) {
                // Accepted GH2DXu/GHDX autoplay sampling keeps
                // left.weight/right.weight at ~1.0 while the hand-driver
                // scheduler blends clips. The scheduler blends finger rows; it
                // does not ease IK on/off at each fretting event.
                bool fret_active = perf.fret_player.active();
                for (const auto& player : perf.fret_extra_players) {
                    fret_active = fret_active || player.active();
                }
                float left_weight = fret_active ? 1.0f : 0.0f;
                float right_weight =
                    perf.strum_player.active() ? 1.0f : 0.0f;
                if (env_value("GHOGX_LEFT_WEIGHT") != nullptr) {
                    left_weight = std::clamp(
                        env_float("GHOGX_LEFT_WEIGHT", left_weight), 0.0f,
                        1.0f);
                }
                if (env_value("GHOGX_RIGHT_WEIGHT") != nullptr) {
                    right_weight = std::clamp(
                        env_float("GHOGX_RIGHT_WEIGHT", right_weight), 0.0f,
                        1.0f);
                }
                ghogx::character::set_runtime_ik_weight(
                    character, "left.weight", left_weight);
                ghogx::character::set_runtime_ik_weight(
                    character, "right.weight", right_weight);
            }
            if (hand_driver_active && perf_fret_pos.active) {
                if (env_value("GHOGX_DEBUG_HAND_MAP") != nullptr) {
                    std::fprintf(stderr,
                                 "[fretpos] role=%s tick=%u spot=%s index=%d\n",
                                 perf.role.c_str(), perf_fret_pos.tick,
                                 perf_fret_pos.spot_name.c_str(),
                                 perf_fret_pos.spot_index);
                }
                ghogx::character::apply_ik_midi_fret_target(
                    character, perf_fret_pos.spot_name,
                    static_cast<float>(song_time_));
            }
            ghogx::character::FaceFxEyeProperties eye_props;
            ghogx::character::apply_character_controllers(
                character, static_cast<float>(song_time_), &eye_props);
            if (hand_driver_active) {
                const uint32_t debug_hand_mask =
                    perf_anim_note_cue.active
                        ? (perf_anim_note_cue.mask & 0x1fu)
                        : 0u;
                const uint32_t debug_hand_tick =
                    perf_anim_note_cue.active ? perf_anim_note_cue.tick
                                              : UINT32_MAX;
                const auto guitar_strings_world =
                    perf.renderer
                        ? perf.renderer->attached_prop_world(
                              "guitar_strings.mesh")
                        : std::nullopt;
                dump_hand_pose_rows(
                    character, perf.role, "postcontrollers", song_time_,
                    debug_hand_tick, debug_hand_mask,
                    perf.active_strum_clip_names, perf.active_fret_clip_names,
                    &perf.world_transform,
                    guitar_strings_world ? &*guitar_strings_world : nullptr);
            }
            if (perf.facefx_graph) {
                auto registers =
                    facefx_animation_
                        ? ghogx::character::sample_facefx_animation(
                              *facefx_animation_,
                              static_cast<float>(song_time_))
                        : std::unordered_map<std::string, float>{};
                for (const auto& [name, value] :
                     facefx_registers_from_eye_servo(character, eye_props)) {
                    registers[name] = value;
                }
                if (!registers.empty()) {
                    const float eyez =
                        ghogx::character::evaluate_facefx_node(
                            *perf.facefx_graph, "EyeZCombiner", registers);
                    const bool applied =
                        ghogx::character::apply_facefx_animation_frame(
                            *perf.facefx_graph, registers, character);
                    if (debug_face_enabled_game()) {
                        std::fprintf(
                            stderr,
                            "[facefx] role=%s EyeZCombiner=%.4f "
                            "graph=%s regs=%zu\n",
                            perf.role.c_str(), eyez, applied ? "applied" : "idle",
                            registers.size());
                    }
                }
            }
        }

        std::unordered_map<std::string, std::array<float, 3>> camera_targets;
        for (auto& perf : performers_) {
            if (!perf.renderer) continue;
            auto& character = perf.renderer->character();
            auto add_target = [&](std::string_view subpart,
                                  const std::array<float, 16>& local_world) {
                const auto world =
                    mat4_mul_game(local_world, perf.world_transform);
                camera_targets[camera_target_id(perf.role, subpart)] =
                    {world[12], world[13], world[14]};
            };
            for (const auto& bone : character.bones) {
                // Camera focus targets must live in the same authored basis as
                // the skinned performer vertices before applying the stage
                // placement transform.
                const auto local_world =
                    character.bone_world_local_chain(bone.name);
                add_target(bone.name, local_world);
                add_target(bone.name + ".mesh", local_world);
            }
            for (const auto& mesh : character.meshes) {
                const auto local_world = character.mesh_world(mesh);
                add_target(mesh.name, local_world);
                add_target(strip_mesh_suffix(mesh.name), local_world);
            }
            auto add_prop_camera_targets = [&](const std::vector<CameraKey>& keys) {
                for (const auto& key : keys) {
                    if (key.target_entity != perf.role ||
                        key.target_subpart.empty()) {
                        continue;
                    }
                    auto prop_world =
                        perf.renderer->attached_prop_world(key.target_subpart);
                    if (!prop_world) continue;
                    camera_targets[camera_target_id(perf.role,
                                                    key.target_subpart)] =
                        {(*prop_world)[12], (*prop_world)[13],
                         (*prop_world)[14]};
                    camera_targets[camera_target_id(
                        perf.role, strip_mesh_suffix(key.target_subpart))] =
                        {(*prop_world)[12], (*prop_world)[13],
                         (*prop_world)[14]};
                }
            };
            add_prop_camera_targets(camera_keys_);
            add_prop_camera_targets(regular_camera_keys_);
            auto spine = camera_targets.find(
                camera_target_id(perf.role, "bone_spine1.mesh"));
            if (spine == camera_targets.end()) {
                spine = camera_targets.find(
                    camera_target_id(perf.role, "bone_spine1"));
            }
            if (spine != camera_targets.end()) {
                camera_targets[camera_target_id(perf.role, {})] = spine->second;
            }
        }

        if (!intro_end_dispatched_ && intro_camera_seconds_ > 0.0 &&
            song_time_ >= intro_camera_seconds_) {
            intro_end_dispatched_ = true;
            should_resend_excitement_ = true;
            apply_venue_event("intro_end", false);
        }

        const bool in_intro_camera_window =
            intro_camera_seconds_ > 0.0 && song_time_ < intro_camera_seconds_;
        if (!in_intro_camera_window && !regular_camera_keys_.empty()) {
            const uint32_t bar = camera_bar_at(chart_, song_time_);
            if (last_camera_bar_ == UINT32_MAX) {
                last_camera_bar_ = bar;
                camera_bars_left_ = 0;
            } else if (bar != last_camera_bar_) {
                const uint32_t bars_elapsed = bar - last_camera_bar_;
                last_camera_bar_ = bar;
                if (camera_bars_left_ > 0) {
                    camera_bars_left_ =
                        std::max(0, camera_bars_left_ -
                                        static_cast<int>(bars_elapsed));
                }
            }

            bool force_camera = false;
            for (const auto& ev : chart_.text_events) {
                const double t = chart_.tick_to_sec(ev.tick);
                if (t < song_time_ - std::max(0.001, dt * 1.5)) continue;
                if (t > song_time_) break;
                if (ev.text == "[sync_wag]" || ev.text == "[sync_head_bang]" ||
                    ev.text == "[band_jump]") {
                    if (ev.tick == last_forced_camera_event_tick_) continue;
                    last_forced_camera_event_tick_ = ev.tick;
                    force_camera = true;
                    camera_bars_left_ = 4;
                    break;
                }
            }

            if (force_camera || camera_bars_left_ <= 0 ||
                active_regular_camera_.empty()) {
                auto duration =
                    camera_duration_range_for_event(camera_duration_bars_,
                                                    active_venue_event_);
                if (force_camera) {
                    duration = {"force", {4, 4}};
                } else {
                    camera_bars_left_ = deterministic_camera_duration_bars(
                        duration.second.first, duration.second.second,
                        camera_shot_counter_);
                }
                ++camera_shot_counter_;
                const CameraKey* current_key =
                    find_camera_key_by_name(regular_camera_keys_,
                                            active_regular_camera_);
                const bool low_excitement =
                    active_venue_event_.find("bad") != std::string::npos ||
                    active_venue_event_.find("boot") != std::string::npos;
                constexpr bool kGuitaristWalking = false;
                constexpr bool kGuitaristStarpower = false;
                if (const auto* key = choose_regular_camera_key_scripted(
                        regular_camera_keys_, current_key,
                        camera_shot_counter_, low_excitement,
                        kGuitaristWalking, kGuitaristStarpower)) {
                    const bool shot_changed =
                        active_regular_camera_ != key->name;
                    if (shot_changed) {
                        previous_regular_camera_ = active_regular_camera_;
                        previous_camera_position_index_ =
                            active_camera_position_index_;
                        active_regular_camera_ = key->name;
                        active_regular_camera_start_ = song_time_;
                        active_camera_position_start_ = song_time_;
                        active_camera_position_index_ = 0;
                        std::fprintf(
                            stderr,
                            "[world] regular camera sweep: %s -> %s bars_left=%d duration=%s[%d,%d] forced=%d bar=%u t=%.3f\n",
                            previous_regular_camera_.c_str(),
                            key->name.c_str(), camera_bars_left_,
                            duration.first.c_str(), duration.second.first,
                            duration.second.second, force_camera ? 1 : 0, bar,
                            song_time_);
                    }
                    if (should_resend_excitement_) {
                        should_resend_excitement_ = false;
                        resend_active_venue_event();
                    }
                }
            }
            if (const auto* key =
                    find_camera_key_by_name(regular_camera_keys_,
                                            active_regular_camera_)) {
                constexpr double kPostSwitchSeconds = 2.06;
                if (key->positions.size() > 1 &&
                    song_time_ >=
                        active_camera_position_start_ + kPostSwitchSeconds) {
                    previous_regular_camera_ = active_regular_camera_;
                    previous_camera_position_index_ =
                        active_camera_position_index_;
                    active_camera_position_index_ =
                        (active_camera_position_index_ + 1) %
                        key->positions.size();
                    active_camera_position_start_ = song_time_;
                    std::fprintf(stderr,
                                 "[world] post_switch_cam: %s pos=%zu/%zu t=%.3f\n",
                                 key->name.c_str(),
                                 active_camera_position_index_,
                                 key->positions.size(), song_time_);
                }
                const CameraKey current_position =
                    camera_position_for(*key, active_camera_position_index_);
                const CameraKey* previous_shot =
                    find_camera_key_by_name(regular_camera_keys_,
                                            previous_regular_camera_);
                std::optional<CameraKey> previous_position;
                if (previous_shot) {
                    previous_position =
                        camera_position_for(*previous_shot,
                                            previous_camera_position_index_);
                }
                std::vector<CameraKey> selected_camera =
                    regular_camera_sweep_keys(
                        current_position,
                        previous_position ? &*previous_position : nullptr,
                        song_time_, active_camera_position_start_);
                apply_camera_keys(world_->camera(), selected_camera, song_time_,
                                  camera_targets);
            }
        } else if (in_intro_camera_window && !camera_keys_.empty()) {
            apply_camera_keys(world_->camera(), camera_keys_, song_time_,
                              camera_targets);
        }
        if (debug_gameplay_camera_enabled()) {
            const char* target_env =
                env_value("GHOGX_DEBUG_GAMEPLAY_CAMERA_TARGET");
            const std::string target_id =
                target_env && target_env[0] ? target_env
                                            : "guitarist0:bone_spine1.mesh";
            auto target = camera_targets.find(target_id);
            if (target == camera_targets.end()) {
                target = camera_targets.find("guitarist0:bone_spine1");
            }
            if (target != camera_targets.end()) {
                auto& cam = world_->camera();
                cam.authored = false;
                cam.target[0] = target->second[0] +
                    env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_TARGET_X", 0.0f);
                cam.target[1] = target->second[1] +
                    env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_TARGET_Y", 0.0f);
                cam.target[2] = target->second[2] +
                    env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_TARGET_Z", 6.0f);
                cam.yaw = env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_YAW", 0.0f);
                cam.pitch =
                    env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_PITCH", 0.18f);
                cam.distance =
                    env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_DIST", 85.0f);
                cam.fov = env_float("GHOGX_DEBUG_GAMEPLAY_CAMERA_FOV", 0.55f);
            }
        }
        world_->draw();
        if (lighting_) {
            const LightingRequest lighting_request =
                lighting_request_at(chart_, song_time_, intro_camera_seconds_);
            const uint32_t lighting_excitement =
                venue_excitement_level(active_venue_event_);
            if (const auto* preset =
                    choose_lighting_preset(lighting_presets_, lighting_request,
                                           lighting_excitement)) {
                const bool preset_changed =
                    active_lighting_preset_ != preset->name;
                if (preset_changed) {
                    active_lighting_preset_ = preset->name;
                    active_lighting_keyframe_.clear();
                    active_lighting_keyframe_index_ = SIZE_MAX;
                    active_lighting_preset_start_ = song_time_;
                    std::fprintf(stderr,
                                 "[world] lighting preset active: %s category=%s adjective=%s request=%s/%s excitement=%u keyframes=%u t=%.3f\n",
                                 preset->name.c_str(),
                                 preset->category.c_str(),
                                 preset->adjective.c_str(),
                                 lighting_request.category.c_str(),
                                 lighting_request.adjective.c_str(),
                                 lighting_excitement, preset->keyframe_count,
                                 song_time_);
                }
                if (!preset->keyframes.empty()) {
                    if (preset_changed ||
                        active_lighting_keyframe_index_ == SIZE_MAX ||
                        active_lighting_keyframe_index_ >=
                            preset->keyframes.size()) {
                        active_lighting_keyframe_index_ = 0;
                    }
                    const size_t previous_lighting_keyframe_index =
                        active_lighting_keyframe_index_;
                    while (next_lighting_cue_idx_ < chart_.lighting_cues.size()) {
                        const auto& cue =
                            chart_.lighting_cues[next_lighting_cue_idx_];
                        const double cue_sec = chart_.tick_to_sec(cue.tick);
                        if (cue_sec > song_time_) break;
                        const bool high_excitement =
                            venue_excitement_is_high(active_venue_event_);
                        const bool apply_keyframe =
                            high_excitement || ignored_last_light_change_;
                        std::fprintf(
                            stderr,
                            "[world] lighting cue: %s pitch=%d tick=%u apply=%d ignored_last=%d excitement=%s t=%.3f preset=%s\n",
                            cue.event.c_str(), cue.pitch, cue.tick,
                            apply_keyframe ? 1 : 0,
                            ignored_last_light_change_ ? 1 : 0,
                            active_venue_event_.c_str(), song_time_,
                            preset->name.c_str());
                        if (apply_keyframe) {
                            ignored_last_light_change_ = false;
                            if (cue.event == "first") {
                                active_lighting_keyframe_index_ = 0;
                            } else if (cue.event == "next") {
                                active_lighting_keyframe_index_ =
                                    (active_lighting_keyframe_index_ + 1) %
                                    preset->keyframes.size();
                            } else if (cue.event == "prev") {
                                active_lighting_keyframe_index_ =
                                    (active_lighting_keyframe_index_ +
                                     preset->keyframes.size() - 1) %
                                    preset->keyframes.size();
                            }
                            active_lighting_keyframe_.clear();
                        } else {
                            ignored_last_light_change_ = true;
                        }
                        ++next_lighting_cue_idx_;
                    }
                    const size_t keyframe_index =
                        chart_.lighting_cues.empty()
                            ? lighting_keyframe_index_at(
                                  *preset, chart_, song_time_,
                                  active_lighting_preset_start_)
                            : active_lighting_keyframe_index_;
                    const auto& keyframe =
                        preset->keyframes[std::min(keyframe_index,
                                                   preset->keyframes.size() - 1)];
                    if (active_lighting_keyframe_ != keyframe.name ||
                        active_lighting_keyframe_index_ != keyframe_index ||
                        preset_changed) {
                        float transition_fade_frames = keyframe.fade_out;
                        if (!preset_changed &&
                            previous_lighting_keyframe_index != SIZE_MAX &&
                            previous_lighting_keyframe_index <
                                preset->keyframes.size() &&
                            previous_lighting_keyframe_index != keyframe_index) {
                            const float previous_fade =
                                preset->keyframes[previous_lighting_keyframe_index]
                                    .fade_out;
                            if (std::isfinite(previous_fade) &&
                                previous_fade > 0.0f) {
                                transition_fade_frames = previous_fade;
                            }
                        }
                        active_lighting_keyframe_ = keyframe.name;
                        active_lighting_keyframe_index_ = keyframe_index;
                        std::map<std::string, const LightingSpotlight*> spots_by_name;
                        std::map<std::string, std::vector<const LightingSpotlight*>> spots_by_target;
                        for (const auto& spot : lighting_spotlights_) {
                            spots_by_name[spot.name] = &spot;
                            if (!spot.target.empty())
                                spots_by_target[spot.target].push_back(&spot);
                        }
                        std::map<std::string,
                                 const LightingPreset::TargetState*> states_by_target;
                        for (const auto& state : keyframe.target_states) {
                            states_by_target[state.target] = &state;
                        }
                        auto preset_has_spot = [&](const std::string& name) {
                            return preset->spot_refs.empty() ||
                                   std::find(preset->spot_refs.begin(),
                                             preset->spot_refs.end(), name) !=
                                       preset->spot_refs.end();
                        };
                        size_t inferred_spots = 0;
                        std::vector<ghogx::render::MiloSceneRenderer::SpotlightState>
                            active_spots;
                        auto push_spot = [&](const LightingSpotlight& spot,
                                             const LightingPreset::TargetState* state) {
                            ghogx::render::MiloSceneRenderer::SpotlightState out;
                            out.name = spot.name;
                            out.target_mesh = spot.target;
                            if (state) {
                                out.target_mesh = state->target;
                                out.r = state->color[0];
                                out.g = state->color[1];
                                out.b = state->color[2];
                                out.intensity = state->intensity;
                            }
                            active_spots.push_back(std::move(out));
                        };
                        size_t mesh_target_spots = 0;
                        for (const auto& target : keyframe.mesh_targets) {
                            const auto target_it = spots_by_target.find(target);
                            if (target_it == spots_by_target.end()) continue;
                            const auto state_it = states_by_target.find(target);
                            for (const LightingSpotlight* spot : target_it->second) {
                                if (!spot) continue;
                                if (!preset_has_spot(spot->name)) continue;
                                ++mesh_target_spots;
                                push_spot(*spot,
                                          state_it == states_by_target.end()
                                              ? nullptr
                                              : state_it->second);
                            }
                        }
                        size_t direct_spots = 0;
                        for (const auto& spot_ref : keyframe.spot_refs) {
                            const auto spot_it = spots_by_name.find(spot_ref);
                            if (spot_it == spots_by_name.end()) continue;
                            if (!preset_has_spot(spot_it->second->name))
                                continue;
                            ++direct_spots;
                            const auto state_it =
                                states_by_target.find(spot_it->second->target);
                            push_spot(*spot_it->second,
                                      state_it == states_by_target.end()
                                          ? nullptr
                                          : state_it->second);
                        }
                        for (const auto& state : keyframe.target_states) {
                            if (const auto target_it =
                                    spots_by_target.find(state.target);
                                target_it != spots_by_target.end()) {
                                for (const LightingSpotlight* spot : target_it->second) {
                                    if (!spot) continue;
                                    if (!preset_has_spot(spot->name)) continue;
                                    ++inferred_spots;
                                    push_spot(*spot, &state);
                                }
                                continue;
                            }
                            const std::string spot_name =
                                infer_spotlight_from_target(state.target);
                            const auto spot_it = spots_by_name.find(spot_name);
                            if (spot_it != spots_by_name.end()) {
                                if (!preset_has_spot(spot_it->second->name))
                                    continue;
                                ++inferred_spots;
                                push_spot(*spot_it->second, &state);
                            }
                        }
                        active_spots =
                            sorted_unique_spotlight_states(std::move(active_spots));
                        const size_t active_spot_count = active_spots.size();
                        const double transition_fade_seconds =
                            lighting_frames_to_seconds(transition_fade_frames);
                        set_lighting_spot_targets(std::move(active_spots),
                                                  transition_fade_seconds);
                        std::fprintf(
                            stderr,
                            "[world] lighting keyframe active: %s[%llu] '%s' span=0x%llx..0x%llx targets=%llu target_states=%llu static_targeted_spots=%llu direct_spots=%llu inferred_spots=%llu active_spots=%llu duration_frames=%.3f fade_frames=%.3f fade_seconds=%.3f t=%.3f\n",
                            preset->name.c_str(),
                            static_cast<unsigned long long>(keyframe_index),
                            keyframe.name.c_str(),
                            static_cast<unsigned long long>(
                                keyframe.record_start),
                            static_cast<unsigned long long>(
                                keyframe.record_end),
                            static_cast<unsigned long long>(
                                keyframe.mesh_targets.size()),
                            static_cast<unsigned long long>(
                                keyframe.target_states.size()),
                            static_cast<unsigned long long>(mesh_target_spots),
                            static_cast<unsigned long long>(direct_spots),
                            static_cast<unsigned long long>(inferred_spots),
                            static_cast<unsigned long long>(active_spot_count),
                            keyframe.duration, transition_fade_frames,
                            transition_fade_seconds, song_time_);
                    }
                }
            }
            update_lighting_spotlight_renderer();
            lighting_->draw_over_scene(world_->camera());
        }
        if (drum_kit_) {
            drum_kit_->draw_over_scene(world_->camera());
        }
        const std::string_view only_role = only_draw_performer_role();
        for (auto& perf : performers_) {
            if (!only_role.empty() && perf.role != only_role) continue;
            if (!perf.renderer) continue;
            perf.renderer->draw_over_scene(world_->camera());
        }
        return;
    }

    if (!highway_) {
        highway_ = std::make_unique<HighwayRenderer>(win);
        // Load the GH2 track texture set natively from the ARK (once).
        highway_->load_textures(hdr_path_, ark_path_);
    }
    // song_time_ is the audio-synced master clock (set in tick()).
    highway_->draw(song_time_, chart_, difficulty_,
                   prev_fret_mask_ & 0x1F /* held frets */, lane_flash_, 1.5f);
}

}  // namespace ghogx::game
