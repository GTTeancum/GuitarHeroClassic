// engine/src/asset/milo_image.cpp

#include "asset/milo_image.h"

#include "ark_v3.h"
#include "milo.h"
#include "milo_tex.h"
#include "ps2_texture.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace ghogx::asset {

namespace {

// Some ARK entries are reached only via the "../../system/run/" prefix the
// runtime uses; mirror the catalog tool's fallback.
std::optional<gh::ark::Entry> find_entry(const gh::ark::ArkV3Reader& ark,
                                         const std::string& path) {
  auto e = ark.find(path);
  if (!e) e = ark.find("../../system/run/" + path);
  return e;
}

bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

bool ends_with(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() &&
         s.substr(s.size() - suffix.size(), suffix.size()) == suffix;
}

std::string normalize_outfit_surface_key(std::string key);

std::string normalize_pathish(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  for (char& c : value) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return value;
}

std::vector<std::string> scan_packed_strings(const uint8_t* body, size_t size) {
  std::vector<std::string> out;
  for (size_t off = 0; off + 4 <= size; ++off) {
    uint32_t len = 0;
    std::memcpy(&len, body + off, sizeof(len));
    if (len == 0 || len > 128 || off + 4 + len > size) continue;
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
    std::string value(s, s + len);
    if (value.find_first_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ==
        std::string::npos) {
      continue;
    }
    if (out.empty() || out.back() != value) out.push_back(std::move(value));
    off += 3 + len;
  }
  return out;
}

std::string track_surface_reference_path(std::string surface_ref) {
  surface_ref = normalize_pathish(std::move(surface_ref));
  const size_t embedded = surface_ref.find("track/surfaces/");
  if (embedded != std::string::npos) surface_ref.erase(0, embedded);
  if (starts_with(surface_ref, "track/surfaces/")) return surface_ref;
  return {};
}

std::vector<std::string> track_surface_candidates_for_ref(
    const std::string& surface_ref) {
  std::vector<std::string> out;
  std::string path = track_surface_reference_path(surface_ref);
  if (!path.empty()) {
    out.push_back(path);
    if (ends_with(path, ".bmp")) {
      std::string gen_path = path;
      if (!starts_with(gen_path, "track/surfaces/gen/")) {
        gen_path.insert(std::string("track/surfaces/").size(), "gen/");
      }
      gen_path += "_ps2";
      out.push_back(gen_path);
    }
    return out;
  }

  const std::string key = normalize_outfit_surface_key(surface_ref);
  if (!key.empty()) {
    out.push_back("track/surfaces/gen/" + key + "_keep.bmp_ps2");
    // GH1 uses the same selected-character key but stores the loose bitmap
    // without GH2's `_keep` suffix.
    out.push_back("track/surfaces/gen/" + key + ".bmp_ps2");
  }
  return out;
}

std::string first_existing_track_surface(const gh::ark::ArkV3Reader& ark,
                                         const std::vector<std::string>& paths) {
  for (const auto& path : paths) {
    if (!path.empty() && find_entry(ark, path)) return path;
  }
  // Some GH1 model symbols carry a more specific suffix than their authored
  // surface key (for example a model-family qualifier). Resolve only a unique
  // prefix boundary from the packed surface inventory.
  for (const auto& requested : paths) {
    constexpr std::string_view prefix = "track/surfaces/gen/";
    constexpr std::string_view suffix = "_keep.bmp_ps2";
    if (!starts_with(requested, prefix) || !ends_with(requested, suffix))
      continue;
    const std::string key = requested.substr(
        prefix.size(), requested.size() - prefix.size() - suffix.size());
    std::vector<std::string> matches;
    for (const auto& entry : ark.entries()) {
      const std::string candidate = normalize_pathish(entry.full_path);
      if (!starts_with(candidate, prefix) ||
          !ends_with(candidate, ".bmp_ps2"))
        continue;
      const std::string stem = candidate.substr(
          prefix.size(),
          candidate.size() - prefix.size() - std::string(".bmp_ps2").size());
      if (key.size() > stem.size() && starts_with(key, stem) &&
          key[stem.size()] == '_') {
        matches.push_back(candidate);
      }
    }
    if (matches.size() == 1) return matches.front();
  }
  return {};
}

bool debug_texture_load_enabled() {
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_TEXTURE_LOAD") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
}

std::string normalize_outfit_surface_key(std::string key) {
  std::replace(key.begin(), key.end(), '\\', '/');
  const size_t slash = key.find_last_of('/');
  if (slash != std::string::npos) key.erase(0, slash + 1);
  const std::string suffix = ".milo_ps2";
  if (key.size() > suffix.size() &&
      key.compare(key.size() - suffix.size(), suffix.size(), suffix) == 0) {
    key.erase(key.size() - suffix.size());
  }
  for (char& c : key) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return key;
}

struct TextureSourceStats {
  std::unordered_set<std::string> found;
  std::unordered_set<std::string> empty;
  size_t decoded = 0;
};

TextureSourceStats load_milo_textures_from_source(
    const gh::ark::ArkV3Reader& ark, const std::string& ark_path,
    const std::string& milo_path, const std::unordered_set<std::string>& wanted,
    std::map<std::string, Image>& out) {
  TextureSourceStats stats;
  auto entry = find_entry(ark, milo_path);
  if (!entry) {
    std::fprintf(stderr, "[asset] milo not found in ARK: %s\n", milo_path.c_str());
    return stats;
  }

  auto bytes = ark.read_entry(*entry, {ark_path});
  auto hdr = gh::milo::parse_header(bytes);
  auto payload = gh::milo::inflate_payload(bytes, hdr);
  auto dir = gh::milo::parse_directory(payload);

  for (const auto& de : dir.entries) {
    if (de.type != "Tex" || wanted.find(de.name) == wanted.end()) continue;
    stats.found.insert(de.name);
    if (out.find(de.name) != out.end()) continue;
    try {
      std::vector<uint8_t> tex_bytes(payload.data() + de.offset,
                                     payload.data() + de.offset + de.size);
      auto tex = ghogx::milo::parse_tex_entry(de.name, tex_bytes);
      if (tex.bitmap.encoding != 3) continue;  // external-flagged still embeds bitmap
      Image img;
      img.rgba = gh::tex::decode_to_rgba(tex.bitmap);
      img.width = tex.bitmap.width;
      img.height = tex.bitmap.height;
      if (img.valid()) {
        out[de.name] = std::move(img);
        ++stats.decoded;
      } else if (tex.bitmap.width == 0 && tex.bitmap.height == 0 &&
                 img.rgba.empty()) {
        stats.empty.insert(de.name);
      }
    } catch (const std::exception& ex) {
      std::fprintf(stderr, "[asset]   %s decode failed: %s\n", de.name.c_str(),
                   ex.what());
    }
  }
  return stats;
}

void log_unresolved_texture_requests(
    const std::string& label, const std::vector<std::string>& entry_names,
    const std::map<std::string, Image>& out,
    const std::unordered_set<std::string>& found,
    const std::unordered_set<std::string>& empty) {
  if (!debug_texture_load_enabled() || out.size() == entry_names.size()) return;
  for (const auto& name : entry_names) {
    if (out.find(name) != out.end()) continue;
    const char* status =
        empty.find(name) != empty.end()
            ? "stock empty Tex entry"
            : (found.find(name) == found.end() ? "missing Tex entry"
                                               : "Tex entry present");
    std::fprintf(stderr, "[asset] %s: requested texture %s not decoded (%s)\n",
                 label.c_str(), name.c_str(), status);
  }
}

}  // namespace

Image load_milo_texture(const std::string& hdr_path, const std::string& ark_path,
                        const std::string& milo_path) {
  Image best;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = find_entry(ark, milo_path);
    if (!entry) {
      std::fprintf(stderr, "[asset] milo not found in ARK: %s\n", milo_path.c_str());
      return best;
    }

    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);

    long best_area = -1;
    for (const auto& de : dir.entries) {
      if (de.type != "Tex") continue;
      try {
        std::vector<uint8_t> tex_bytes(payload.data() + de.offset,
                                       payload.data() + de.offset + de.size);
        auto tex = ghogx::milo::parse_tex_entry(de.name, tex_bytes);
        // External-flagged textures still embed their bitmap on PS2; rely on
        // the decoded bitmap (encoding 3 = palettized), not the use_external flag.
        if (tex.bitmap.encoding != 3) continue;

        const long area = static_cast<long>(tex.bitmap.width) * tex.bitmap.height;
        if (area <= best_area) continue;  // keep only the largest

        best.rgba = gh::tex::decode_to_rgba(tex.bitmap);
        best.width = tex.bitmap.width;
        best.height = tex.bitmap.height;
        best_area = area;
      } catch (const std::exception& ex) {
        std::fprintf(stderr, "[asset]   tex '%s' decode failed: %s\n",
                     de.name.c_str(), ex.what());
      }
    }

    if (best.valid()) {
      std::fprintf(stderr, "[asset] %s -> %dx%d (largest of %zu entries)\n",
                   milo_path.c_str(), best.width, best.height, dir.entries.size());
    } else {
      std::fprintf(stderr, "[asset] no decodable Tex in %s\n", milo_path.c_str());
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[asset] load_milo_texture(%s): %s\n", milo_path.c_str(),
                 ex.what());
  }
  return best;
}

Image load_milo_texture_named(const std::string& hdr_path,
                              const std::string& ark_path,
                              const std::string& milo_path,
                              const std::string& entry_name) {
  Image out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = find_entry(ark, milo_path);
    if (!entry) {
      std::fprintf(stderr, "[asset] milo not found in ARK: %s\n", milo_path.c_str());
      return out;
    }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);

    for (const auto& de : dir.entries) {
      if (de.type != "Tex" || de.name != entry_name) continue;
      try {
        std::vector<uint8_t> tex_bytes(payload.data() + de.offset,
                                       payload.data() + de.offset + de.size);
        auto tex = ghogx::milo::parse_tex_entry(de.name, tex_bytes);
        if (tex.bitmap.encoding != 3) break;  // external-flagged still embeds bitmap
        out.rgba   = gh::tex::decode_to_rgba(tex.bitmap);
        out.width  = tex.bitmap.width;
        out.height = tex.bitmap.height;
        std::fprintf(stderr, "[asset] %s/%s -> %dx%d\n",
                     milo_path.c_str(), entry_name.c_str(),
                     out.width, out.height);
      } catch (const std::exception& ex) {
        std::fprintf(stderr, "[asset] %s/%s decode failed: %s\n",
                     milo_path.c_str(), entry_name.c_str(), ex.what());
      }
      break;
    }
    if (!out.valid())
      std::fprintf(stderr, "[asset] %s: Tex '%s' not found or not decodable\n",
                   milo_path.c_str(), entry_name.c_str());
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[asset] load_milo_texture_named(%s/%s): %s\n",
                 milo_path.c_str(), entry_name.c_str(), ex.what());
  }
  return out;
}

std::map<std::string, Image> load_milo_textures(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path, const std::vector<std::string>& entry_names) {
  std::map<std::string, Image> out;
  const std::unordered_set<std::string> wanted(entry_names.begin(),
                                               entry_names.end());
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    const TextureSourceStats stats =
        load_milo_textures_from_source(ark, ark_path, milo_path, wanted, out);
    std::fprintf(stderr, "[asset] %s: loaded %zu/%zu requested textures\n",
                 milo_path.c_str(), out.size(), entry_names.size());
    log_unresolved_texture_requests(milo_path, entry_names, out, stats.found,
                                    stats.empty);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[asset] load_milo_textures(%s): %s\n",
                 milo_path.c_str(), ex.what());
  }
  return out;
}

std::map<std::string, Image> load_milo_textures_from_sources(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::vector<std::string>& entry_names) {
  std::map<std::string, Image> out;
  const std::unordered_set<std::string> wanted(entry_names.begin(),
                                               entry_names.end());
  std::unordered_set<std::string> found;
  std::unordered_set<std::string> empty;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    for (const auto& milo_path : milo_paths) {
      const TextureSourceStats stats =
          load_milo_textures_from_source(ark, ark_path, milo_path, wanted, out);
      found.insert(stats.found.begin(), stats.found.end());
      empty.insert(stats.empty.begin(), stats.empty.end());
      if (out.size() == entry_names.size()) break;
    }
    const std::string label =
        milo_paths.empty() ? std::string("(no texture source)") : milo_paths.front();
    std::fprintf(stderr,
                 "[asset] %s: loaded %zu/%zu requested textures from %zu source%s\n",
                 label.c_str(), out.size(), entry_names.size(),
                 milo_paths.size(), milo_paths.size() == 1 ? "" : "s");
    log_unresolved_texture_requests(label, entry_names, out, found, empty);
  } catch (const std::exception& ex) {
    const std::string label =
        milo_paths.empty() ? std::string("(no texture source)") : milo_paths.front();
    std::fprintf(stderr, "[asset] load_milo_textures_from_sources(%s): %s\n",
                 label.c_str(), ex.what());
  }
  return out;
}

Image load_ps2_bitmap_from_ark(const std::string& hdr_path,
                               const std::string& ark_path,
                               const std::string& entry_path) {
  Image out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = find_entry(ark, entry_path);
    if (!entry) {
      std::fprintf(stderr, "[asset] bitmap not found in ARK: %s\n",
                   entry_path.c_str());
      return out;
    }

    const auto bytes = ark.read_entry(*entry, {ark_path});
    const auto bitmap = gh::tex::parse(bytes);
    out.rgba = gh::tex::decode_to_rgba(bitmap);
    out.width = bitmap.width;
    out.height = bitmap.height;
    if (out.valid()) {
      std::fprintf(stderr, "[asset] %s -> %dx%d\n", entry_path.c_str(),
                   out.width, out.height);
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[asset] load_ps2_bitmap_from_ark(%s): %s\n",
                 entry_path.c_str(), ex.what());
  }
  return out;
}

std::string endgame_photo_bitmap_path_for_outfit(std::string outfit_key) {
  outfit_key = normalize_pathish(std::move(outfit_key));
  const std::size_t slash = outfit_key.find_last_of('/');
  if (slash != std::string::npos) outfit_key.erase(0, slash + 1);
  constexpr std::string_view kMiloSuffix = ".milo_ps2";
  if (ends_with(outfit_key, kMiloSuffix))
    outfit_key.resize(outfit_key.size() - kMiloSuffix.size());
  if (outfit_key.empty()) return {};
  return "ui/image/og/gen/photo_" + outfit_key + "0_keep.bmp_ps2";
}

std::string track_surface_bitmap_path_for_outfit(std::string outfit_key) {
  const std::string normalized = normalize_outfit_surface_key(std::move(outfit_key));
  if (normalized.empty()) return {};
  return "track/surfaces/gen/" + normalized + "_keep.bmp_ps2";
}

std::string resolve_track_surface_bitmap_path(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& character_milo_path, const std::string& outfit_key) {
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);

    if (!character_milo_path.empty()) {
      auto entry = find_entry(ark, character_milo_path);
      if (entry) {
        auto bytes = ark.read_entry(*entry, {ark_path});
        auto hdr = gh::milo::parse_header(bytes);
        auto payload = gh::milo::inflate_payload(bytes, hdr);
        auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
          if (de.offset + de.size > payload.size()) continue;
          const auto strings = scan_packed_strings(
              payload.data() + de.offset, static_cast<size_t>(de.size));
          for (const auto& value : strings) {
            if (track_surface_reference_path(value).empty()) continue;
            const std::string resolved = first_existing_track_surface(
                ark, track_surface_candidates_for_ref(value));
            if (!resolved.empty()) {
              std::fprintf(stderr,
                           "[asset] character highway surface: %s -> %s "
                           "(milo reference)\n",
                           character_milo_path.c_str(), resolved.c_str());
              return resolved;
            }
          }
        }
      }
    }

    const std::string resolved = first_existing_track_surface(
        ark, track_surface_candidates_for_ref(outfit_key));
    if (!resolved.empty()) {
      std::fprintf(stderr,
                   "[asset] character highway surface: %s -> %s "
                   "(outfit key)\n",
                   outfit_key.c_str(), resolved.c_str());
      return resolved;
    }

    const std::string derived = track_surface_bitmap_path_for_outfit(outfit_key);
    if (!derived.empty()) {
      std::fprintf(stderr,
                   "[asset] character highway surface not found in ARK: %s\n",
                   derived.c_str());
    }
    return derived;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[asset] resolve_track_surface_bitmap_path(%s): %s\n",
                 outfit_key.c_str(), ex.what());
  }
  return {};
}

Image load_track_surface_bitmap(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& surface_ref, std::string* resolved_entry_path) {
  const auto candidates = track_surface_candidates_for_ref(surface_ref);
  std::string entry_path = candidates.empty() ? std::string{} : candidates.front();
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    const std::string resolved = first_existing_track_surface(ark, candidates);
    if (!resolved.empty()) entry_path = resolved;
  } catch (const std::exception&) {
    // The actual bitmap load below will log the concrete failure.
  }
  if (resolved_entry_path) *resolved_entry_path = entry_path;
  if (entry_path.empty()) return {};
  return load_ps2_bitmap_from_ark(hdr_path, ark_path, entry_path);
}

Image load_track_surface_bitmap_for_outfit(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& outfit_key, std::string* resolved_entry_path) {
  return load_track_surface_bitmap(hdr_path, ark_path, outfit_key,
                                   resolved_entry_path);
}

}  // namespace ghogx::asset
