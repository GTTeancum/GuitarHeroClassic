// engine/src/asset/milo_image.cpp

#include "asset/milo_image.h"

#include "ark_v3.h"
#include "milo.h"
#include "milo_tex.h"
#include "ps2_texture.h"

#include <cstdlib>
#include <cstdio>
#include <map>
#include <optional>
#include <stdexcept>
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

bool debug_texture_load_enabled() {
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_TEXTURE_LOAD") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
}

struct TextureSourceStats {
  std::unordered_set<std::string> found;
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
    const std::unordered_set<std::string>& found) {
  if (!debug_texture_load_enabled() || out.size() == entry_names.size()) return;
  for (const auto& name : entry_names) {
    if (out.find(name) != out.end()) continue;
    std::fprintf(stderr, "[asset] %s: requested texture %s not decoded (%s)\n",
                 label.c_str(), name.c_str(),
                 found.find(name) == found.end() ? "missing Tex entry"
                                                 : "Tex entry present");
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
    log_unresolved_texture_requests(milo_path, entry_names, out, stats.found);
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
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    for (const auto& milo_path : milo_paths) {
      const TextureSourceStats stats =
          load_milo_textures_from_source(ark, ark_path, milo_path, wanted, out);
      found.insert(stats.found.begin(), stats.found.end());
      if (out.size() == entry_names.size()) break;
    }
    const std::string label =
        milo_paths.empty() ? std::string("(no texture source)") : milo_paths.front();
    std::fprintf(stderr,
                 "[asset] %s: loaded %zu/%zu requested textures from %zu source%s\n",
                 label.c_str(), out.size(), entry_names.size(),
                 milo_paths.size(), milo_paths.size() == 1 ? "" : "s");
    log_unresolved_texture_requests(label, entry_names, out, found);
  } catch (const std::exception& ex) {
    const std::string label =
        milo_paths.empty() ? std::string("(no texture source)") : milo_paths.front();
    std::fprintf(stderr, "[asset] load_milo_textures_from_sources(%s): %s\n",
                 label.c_str(), ex.what());
  }
  return out;
}

}  // namespace ghogx::asset
