// engine/src/character/char_clip_audit.cpp
//
// Deterministic CharClipSamples inventory helper. This is a read-only data
// tool for source-truth work; it does not render or apply poses.

#include "ark_v3.h"
#include "character/char_clip.h"
#include "milo.h"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

namespace {

bool ends_with(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() &&
         s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::string> expand_milo_args(const gh::ark::ArkV3Reader& ark,
                                          const std::vector<std::string>& args) {
  std::vector<std::string> out;
  for (const std::string& arg : args) {
    if (ends_with(arg, ".milo_ps2") || ends_with(arg, ".acp")) {
      out.push_back(arg);
      continue;
    }
    for (const auto& entry : ark.entries()) {
      if (!ends_with(entry.full_path, ".milo_ps2") &&
          !ends_with(entry.full_path, ".acp")) {
        continue;
      }
      if (entry.full_path.rfind(arg, 0) != 0) continue;
      out.push_back(entry.full_path);
    }
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::string resolve_milo_path(const gh::ark::ArkV3Reader& ark,
                              const std::string& milo_path) {
  if (ark.find(milo_path)) return milo_path;
  const std::string system_run = "../../system/run/" + milo_path;
  if (ark.find(system_run)) return system_run;
  return milo_path;
}

bool audit_one(const gh::ark::ArkV3Reader& ark, const std::string& hdr_path,
               const std::string& ark_path, const std::string& milo_path) {
  const std::string resolved = resolve_milo_path(ark, milo_path);
  auto entry = ark.find(resolved);
  if (!entry) {
    std::printf("[clip-audit] milo=%s status=missing\n", milo_path.c_str());
    return false;
  }

  try {
    if (ends_with(resolved, ".acp")) {
      const auto clip =
          ghogx::character::load_acp_clip(hdr_path, ark_path, resolved);
      std::printf(
          "[clip-audit-acp] path=%s clip=%s accepted=%d frames=%zu "
          "channels0=%zu flags=0x%08x playFlags=0x%08x events=%zu "
          "enter=%s exit=%s\n",
          resolved.c_str(), clip.name.c_str(), clip.loaded ? 1 : 0,
          clip.frames.size(),
          clip.frames.empty() ? 0 : clip.frames.front().size(), clip.flags,
          clip.default_play_flags, clip.beat_events.size(),
          clip.legacy_enter_event.empty() ? "<none>"
                                          : clip.legacy_enter_event.c_str(),
          clip.legacy_exit_event.empty() ? "<none>"
                                         : clip.legacy_exit_event.c_str());
      if (!clip.frames.empty()) {
        for (const auto& channel : clip.frames.front()) {
          const char* type =
              channel.type == ghogx::character::ClipChannel::kPos
                  ? "pos"
                  : channel.type == ghogx::character::ClipChannel::kScale
                        ? "scale"
                        : channel.type ==
                                  ghogx::character::ClipChannel::kQuat
                              ? "quat"
                              : channel.type ==
                                        ghogx::character::ClipChannel::kRotX
                                    ? "rotx"
                                    : channel.type ==
                                              ghogx::character::ClipChannel::
                                                  kRotY
                                          ? "roty"
                                          : channel.type ==
                                                    ghogx::character::
                                                        ClipChannel::kRotZ
                                                ? "rotz"
                                                : channel.type ==
                                                          ghogx::character::
                                                              ClipChannel::
                                                                  kDeltaX
                                                      ? "dx"
                                                      : channel.type ==
                                                                ghogx::
                                                                    character::
                                                                        ClipChannel::
                                                                            kDeltaY
                                                            ? "dy"
                                                            : "dz";
          if (channel.type == ghogx::character::ClipChannel::kQuat) {
            std::printf(
                "[clip-audit-channel] type=%s target=%s weight=%.6f "
                "value=(%.8f %.8f %.8f %.8f)\n",
                type, channel.bone_name.c_str(), channel.source_weight,
                channel.quat[0], channel.quat[1], channel.quat[2],
                channel.quat[3]);
          } else if (channel.type ==
                         ghogx::character::ClipChannel::kPos ||
                     channel.type ==
                         ghogx::character::ClipChannel::kScale) {
            const float* value =
                channel.type == ghogx::character::ClipChannel::kPos
                    ? channel.pos
                    : channel.scale;
            std::printf(
                "[clip-audit-channel] type=%s target=%s weight=%.6f "
                "value=(%.8f %.8f %.8f)\n",
                type, channel.bone_name.c_str(), channel.source_weight,
                value[0], value[1], value[2]);
          } else {
            std::printf(
                "[clip-audit-channel] type=%s target=%s weight=%.6f "
                "value=%.8f\n",
                type, channel.bone_name.c_str(), channel.source_weight,
                channel.angle);
          }
        }
      }
      return clip.loaded;
    }

    const auto bytes = ark.read_entry(*entry, {ark_path});
    const auto header = gh::milo::parse_header(bytes);
    const auto payload = gh::milo::inflate_payload(bytes, header);
    const auto dir = gh::milo::parse_directory(payload);

    int clip_count = 0;
    int output_bone_count = 0;
    for (const auto& de : dir.entries) {
      if (de.type == "CharClipSamples") ++clip_count;
      if (de.type == "CharBone") ++output_bone_count;
    }

    std::printf("[clip-audit-milo] milo=%s resolved=%s dir=%s clips=%d "
                "charBones=%d entries=%zu\n",
                milo_path.c_str(), resolved.c_str(), dir.dir_name.c_str(),
                clip_count, output_bone_count, dir.entries.size());

    for (const auto& de : dir.entries) {
      if (de.type != "CharClipSamples") continue;
      const auto clip =
          ghogx::character::load_clip(hdr_path, ark_path, resolved, de.name);
      const bool accepted = clip.loaded && !clip.frames.empty();
      const size_t channels =
          accepted && !clip.frames.empty() ? clip.frames[0].size() : 0;
      const int extended_typed = clip.raw_channel_counts.scale +
                                 clip.raw_channel_counts.rotx +
                                 clip.raw_channel_counts.roty +
                                 clip.raw_channel_counts.dx +
                                 clip.raw_channel_counts.dy +
                                 clip.raw_channel_counts.dz;
      std::printf("[clip-audit] milo=%s clip=%s bodyBytes=%llu "
                  "accepted=%d frames=%zu channels0=%zu outputBones=%zu "
                  "rawPos=%d rawScale=%d rawQuat=%d rawRotX=%d rawRotY=%d "
                  "rawRotZ=%d rawDX=%d rawDY=%d rawDZ=%d extendedTyped=%d "
                  "startBeat=%.6f endBeat=%.6f beatsPerSecond=%.6f "
                  "flags=0x%08x playFlags=0x%08x blend=%.6f "
                  "transitions=%zu events=%zu enter=%s exit=%s\n",
                  resolved.c_str(), de.name.c_str(),
                  static_cast<unsigned long long>(de.size),
                  accepted ? 1 : 0, clip.frames.size(), channels,
                  clip.output_bones.size(), clip.raw_channel_counts.pos,
                  clip.raw_channel_counts.scale, clip.raw_channel_counts.quat,
                  clip.raw_channel_counts.rotx, clip.raw_channel_counts.roty,
                  clip.raw_channel_counts.rotz, clip.raw_channel_counts.dx,
                  clip.raw_channel_counts.dy, clip.raw_channel_counts.dz,
                  extended_typed, clip.start_beat, clip.end_beat,
                  clip.beats_per_second, clip.flags,
                  clip.default_play_flags, clip.blend_width,
                  clip.transitions.size(), clip.beat_events.size(),
                  clip.legacy_enter_event.empty()
                      ? "<none>"
                      : clip.legacy_enter_event.c_str(),
                  clip.legacy_exit_event.empty()
                      ? "<none>"
                      : clip.legacy_exit_event.c_str());
    }
    return true;
  } catch (const std::exception& ex) {
    std::printf("[clip-audit] milo=%s status=error error=\"%s\"\n",
                milo_path.c_str(), ex.what());
    return false;
  }
}

void usage() {
  std::fprintf(stderr,
               "usage: ghogx_character_clip_audit <main.hdr> <main_0.ark> "
               "<milo-acp-or-prefix> [...]\n");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    usage();
    return 2;
  }

  const std::string hdr_path = argv[1];
  const std::string ark_path = argv[2];
  std::vector<std::string> args;
  for (int i = 3; i < argc; ++i) args.emplace_back(argv[i]);

  try {
    const auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    const auto milos = expand_milo_args(ark, args);
    if (milos.empty()) {
      std::fprintf(stderr, "[clip-audit] no MILOs matched\n");
      return 1;
    }

    int ok = 0;
    for (const auto& milo : milos) {
      if (audit_one(ark, hdr_path, ark_path, milo)) ++ok;
    }
    return ok == static_cast<int>(milos.size()) ? 0 : 1;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[clip-audit] setup failed: %s\n", ex.what());
    return 2;
  }
}
