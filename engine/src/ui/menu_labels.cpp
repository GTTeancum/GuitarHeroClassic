// engine/src/ui/menu_labels.cpp -- see menu_labels.h.

#include "ui/menu_labels.h"

#include "ark_v3.h"
#include "milo.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ghogx::ui {

namespace {

float rf(const std::vector<uint8_t>& d, size_t o) {
  float v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

bool finite(float f) { return f == f && std::fabs(f) < 1e30f; }

// A plausible Trans matrix at byte offset o: 12 finite floats whose three 3x3
// rows each have a sane magnitude and whose translation is in menu range.
bool looks_like_matrix(const std::vector<uint8_t>& d, size_t o) {
  if (o + 48 > d.size()) return false;
  float m[12];
  for (int i = 0; i < 12; ++i) {
    m[i] = rf(d, o + i * 4);
    if (!finite(m[i])) return false;
  }
  for (int r = 0; r < 3; ++r) {
    float mag = std::sqrt(m[r * 3] * m[r * 3] + m[r * 3 + 1] * m[r * 3 + 1] +
                          m[r * 3 + 2] * m[r * 3 + 2]);
    if (mag < 0.05f || mag > 5000.0f) return false;
  }
  for (int t = 9; t < 12; ++t)
    if (std::fabs(m[t]) > 1e5f) return false;
  return true;
}

// Find the embedded Trans and return its WORLD matrix (the second of the
// local+world pair). Scans every byte offset because the class prefix length
// (and its strings) shift the matrix off any fixed alignment.
bool find_world_matrix(const std::vector<uint8_t>& d, std::array<float, 12>& out) {
  for (size_t M = 0x10; M + 96 <= d.size(); ++M) {
    if (looks_like_matrix(d, M) && looks_like_matrix(d, M + 48)) {
      for (int i = 0; i < 12; ++i) out[i] = rf(d, M + 48 + i * 4);
      return true;
    }
  }
  // Fall back to a single matrix if no local+world pair exists.
  for (size_t M = 0x10; M + 48 <= d.size(); ++M) {
    if (looks_like_matrix(d, M)) {
      for (int i = 0; i < 12; ++i) out[i] = rf(d, M + i * 4);
      return true;
    }
  }
  return false;
}

// All length-prefixed printable-ASCII strings in the body, in order.
std::vector<std::string> embedded_strings(const std::vector<uint8_t>& d) {
  std::vector<std::string> out;
  for (size_t i = 0; i + 4 <= d.size();) {
    int32_t n;
    std::memcpy(&n, d.data() + i, 4);
    if (n >= 1 && n <= 64 && i + 4 + static_cast<size_t>(n) <= d.size()) {
      bool printable = true;
      for (int k = 0; k < n; ++k) {
        uint8_t c = d[i + 4 + k];
        if (c < 32 || c >= 127) { printable = false; break; }
      }
      if (printable) {
        out.emplace_back(reinterpret_cast<const char*>(d.data() + i + 4),
                         static_cast<size_t>(n));
        i += 4 + static_cast<size_t>(n);
        continue;
      }
    }
    ++i;
  }
  return out;
}

}  // namespace

std::vector<MenuLabel> extract_menu_labels(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path) {
  std::vector<MenuLabel> out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) return out;
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto h = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, h);
    auto dir = gh::milo::parse_directory(payload);

    for (const auto& e : dir.entries) {
      if (e.type != "BandButton" && e.type != "Text" && e.type != "BandLabel")
        continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      MenuLabel lbl;
      lbl.name = e.name;
      lbl.type = e.type;
      auto strs = embedded_strings(body);
      if (!strs.empty()) {
        lbl.text = strs.back();
        if (strs.size() >= 2) lbl.font = strs.front();
        // nav target = an embedded "*.btn" string that isn't this object's name
        // (the focus-down link, e.g. main_career.btn -> main_quickspin.btn).
        for (const auto& s : strs) {
          if (s.size() > 4 && s.compare(s.size() - 4, 4, ".btn") == 0 && s != e.name) {
            lbl.nav = s;
            break;
          }
        }
      }
      lbl.has_world = find_world_matrix(body, lbl.world);
      out.push_back(std::move(lbl));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] %s: %s\n", milo_path.c_str(), ex.what());
  }
  return out;
}

}  // namespace ghogx::ui
