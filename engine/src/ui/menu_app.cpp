// engine/src/ui/menu_app.cpp -- see menu_app.h.

#include "ui/menu_app.h"

#include "ui/config_db.h"
#include "ui/menu_font.h"
#include "ui/menu_labels.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "asset/milo_image.h"
#include "character/char_renderer.h"
#include "milo_scene/milo_scene.h"
#include "render/milo_scene_renderer.h"
#include "render/window_d3d9.h"

#include "dtb.h"
#include "dtb_bridge/dtb_bridge.h"
#include "core/data_node.h"
#include "core/symbol.h"

#include "ark_v3.h"
#include "milo.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <functional>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ghogx::ui {

namespace {

using Action = ghogx::render::Window::Action;

std::unordered_set<std::string> compute_disabled(ScreenManager& mgr);  // fwd
float load_song_list_row_stack_origin_z(const std::string& hdr,
                                        const std::string& ark,
                                        const std::string& milo_path);  // fwd
bool menu_truthy(const DataNode& node);  // fwd
bool screen_has_panel(Object* screen, Symbol panel_name);  // fwd
bool panel_active_for_render(Object* panel);  // fwd
bool runtime_object_showing(ScreenManager& mgr, const std::string& name);  // fwd
bool menu_character_display_enabled();  // fwd
bool menu_guitar_display_enabled();  // fwd
std::vector<std::string> panel_texture_sources(
    const std::string& hdr,
    const std::string& ark,
    const std::string& panel_path,
    const std::unordered_set<std::string>& wanted_textures);  // fwd
class MenuMaterialAnimPlayer;  // fwd
std::unordered_set<std::string> menu_matanim_texture_refs(
    const std::string& hdr, const std::string& ark,
    const std::string& milo_path);  // fwd
std::vector<uint8_t> load_milo_entry_body(const std::string& hdr,
                                          const std::string& ark,
                                          const std::string& milo_path,
                                          const std::string& type,
                                          const std::string& name);  // fwd
std::vector<std::string> milo_entry_names_by_type(const std::string& hdr,
                                                  const std::string& ark,
                                                  const std::string& milo_path,
                                                  const std::string& type);  // fwd
std::vector<Symbol> screen_panel_names(Object* screen);  // fwd
std::string panel_file(Object* panel);  // fwd
std::string panel_milo_path(const std::string& file);  // fwd
std::string first_menu_ref_with_suffixes(
    const std::vector<uint8_t>& body,
    std::initializer_list<const char*> suffixes);  // fwd

uint32_t menu_read_le32_bytes(const std::vector<uint8_t>& body, size_t off) {
  if (off + 4 > body.size()) return 0;
  return static_cast<uint32_t>(body[off]) |
         (static_cast<uint32_t>(body[off + 1]) << 8) |
         (static_cast<uint32_t>(body[off + 2]) << 16) |
         (static_cast<uint32_t>(body[off + 3]) << 24);
}

size_t menu_align4(size_t v) {
  return (v + 3u) & ~size_t(3u);
}

std::string menu_milo_string_at(const std::vector<uint8_t>& body,
                                size_t& off) {
  const uint32_t len = menu_read_le32_bytes(body, off);
  off += 4;
  if (len > body.size() || off + len > body.size() || len > 256) return {};
  std::string s(reinterpret_cast<const char*>(body.data() + off), len);
  off = menu_align4(off + len);
  return s;
}

std::vector<std::string> menu_milo_string_list_at(
    const std::vector<uint8_t>& body, size_t off) {
  std::vector<std::string> out;
  if (off + 4 > body.size()) return out;
  const uint32_t count = menu_read_le32_bytes(body, off);
  off += 4;
  if (count > 64) return out;
  for (uint32_t i = 0; i < count && off + 4 <= body.size(); ++i) {
    std::string s = menu_milo_string_at(body, off);
    if (s.empty()) break;
    out.push_back(std::move(s));
  }
  return out;
}

std::string menu_first_wav_ref(const std::vector<uint8_t>& body) {
  for (size_t i = 0; i < body.size();) {
    while (i < body.size() && (body[i] < 0x20 || body[i] >= 0x7f)) ++i;
    const size_t start = i;
    while (i < body.size() && body[i] >= 0x20 && body[i] < 0x7f) ++i;
    if (i > start) {
      std::string s(reinterpret_cast<const char*>(body.data() + start),
                    i - start);
      if (s.size() > 4 && s.substr(s.size() - 4) == ".wav") return s;
    }
  }
  return {};
}

int16_t menu_clamp16(int32_t v) {
  return static_cast<int16_t>(
      std::clamp(v, int32_t(-32768), int32_t(32767)));
}

struct MenuDecodedSample {
  uint32_t sample_rate = 0;
  std::vector<int16_t> pcm_mono;
};

bool decode_menu_synth_sample(const std::vector<uint8_t>& body,
                              MenuDecodedSample& out) {
  if (body.size() < 0x50 || body[0] != 5) return false;
  const size_t path_len = body[0x0d];
  const size_t field = 0x11 + path_len + 5;
  if (field + 25 > body.size()) return false;
  const uint32_t sample_rate = menu_read_le32_bytes(body, field + 16);
  const uint32_t data_bytes = menu_read_le32_bytes(body, field + 20);
  const uint8_t channels = body[field + 24];
  const size_t data_start = field + 25;
  if (channels != 1 || sample_rate == 0 || data_bytes == 0 ||
      data_start + data_bytes > body.size() || (data_bytes % 16) != 0) {
    return false;
  }

  static constexpr int kCoef[5][2] = {
      {0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};
  int32_t h1 = 0;
  int32_t h2 = 0;
  std::vector<int16_t> pcm;
  pcm.reserve((data_bytes / 16) * 28);
  for (size_t off = data_start; off + 16 <= data_start + data_bytes;
       off += 16) {
    const uint8_t header = body[off];
    int filter = (header >> 4) & 0x0f;
    const int shift = header & 0x0f;
    if (filter < 0 || filter > 4) filter = 0;
    const int fp = kCoef[filter][0];
    const int fn = kCoef[filter][1];
    for (size_t b = off + 2; b < off + 16; ++b) {
      const uint8_t packed = body[b];
      for (int half = 0; half < 2; ++half) {
        int nibble = half == 0 ? (packed & 0x0f) : (packed >> 4);
        if (nibble >= 8) nibble -= 16;
        int32_t sample = nibble << 12;
        if (shift > 0) sample >>= shift;
        sample += ((h1 * fp + h2 * fn + 32) >> 6);
        pcm.push_back(menu_clamp16(sample));
        h2 = h1;
        h1 = sample;
      }
    }
  }
  out.sample_rate = sample_rate;
  out.pcm_mono = std::move(pcm);
  return !out.pcm_mono.empty();
}

struct MenuWavePlayback {
  HWAVEOUT handle = nullptr;
  WAVEHDR header = {};
  std::vector<int16_t> pcm;
};

void CALLBACK menu_wave_callback(HWAVEOUT handle, UINT msg, DWORD_PTR inst,
                                 DWORD_PTR, DWORD_PTR) {
  if (msg != WOM_DONE) return;
  auto* active = reinterpret_cast<MenuWavePlayback*>(inst);
  if (!active) return;
  waveOutUnprepareHeader(handle, &active->header, sizeof(active->header));
  waveOutClose(handle);
  delete active;
}

class MenuSfxPlayer {
 public:
  bool load(gh::ark::ArkV3Reader& arkr, const std::vector<std::string>& arks) {
    if (std::getenv("GHOGX_DISABLE_MENU_AUDIO")) return false;
    auto entry = arkr.find("sfx/gen/metagame_bank.milo_ps2");
    if (!entry) return false;
    auto bytes = arkr.read_entry(*entry, arks);
    auto mh = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, mh);
    auto dir = gh::milo::parse_directory(payload);

    std::unordered_map<std::string, std::vector<uint8_t>> random_groups;
    std::unordered_map<std::string, std::vector<uint8_t>> sfx_seqs;
    std::unordered_map<std::string, std::vector<uint8_t>> sfx;
    for (const auto& e : dir.entries) {
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (e.type == "RandomGroupSeq") {
        random_groups[e.name] = std::move(body);
      } else if (e.type == "SfxSeq") {
        sfx_seqs[e.name] = std::move(body);
      } else if (e.type == "Sfx") {
        sfx[e.name] = std::move(body);
      } else if (e.type == "SynthSample") {
        MenuDecodedSample sample;
        if (decode_menu_synth_sample(body, sample))
          samples_[e.name] = std::move(sample);
      }
    }

    std::unordered_map<std::string, std::string> sfx_to_sample;
    for (const auto& kv : sfx) {
      std::string sample = menu_first_wav_ref(kv.second);
      if (!sample.empty() && samples_.count(sample)) {
        sfx_to_sample[kv.first] = std::move(sample);
      }
    }

    std::unordered_map<std::string, std::string> seq_to_sample;
    for (const auto& kv : sfx_seqs) {
      size_t off = 0x20;
      std::string sfx_name = menu_milo_string_at(kv.second, off);
      auto it = sfx_to_sample.find(sfx_name);
      if (it != sfx_to_sample.end()) seq_to_sample[kv.first] = it->second;
    }

    for (const auto& kv : random_groups) {
      std::vector<std::string> refs = menu_milo_string_list_at(kv.second, 0x24);
      std::vector<std::string> sample_refs;
      for (const std::string& ref : refs) {
        auto it = seq_to_sample.find(ref);
        if (it != seq_to_sample.end()) sample_refs.push_back(it->second);
      }
      if (!sample_refs.empty()) cue_samples_[kv.first] = std::move(sample_refs);
    }
    for (const auto& kv : sfx_to_sample) cue_samples_[kv.first] = {kv.second};

    std::fprintf(stderr,
                 "[menu-audio] loaded %zu samples, %zu source-backed cues\n",
                 samples_.size(), cue_samples_.size());
    return !cue_samples_.empty();
  }

  void play(Symbol, Symbol cue) {
    if (std::getenv("GHOGX_DISABLE_MENU_AUDIO")) return;
    auto it = cue_samples_.find(cue.c_str());
    if (it == cue_samples_.end() || it->second.empty()) return;
    const std::vector<std::string>& variants = it->second;
    const std::string& sample_name = variants[next_variant_++ % variants.size()];
    auto sample_it = samples_.find(sample_name);
    if (sample_it == samples_.end()) return;
    MenuDecodedSample sample = sample_it->second;
    std::thread([sample = std::move(sample)]() {
      play_sample(sample);
    }).detach();
  }

 private:
  static void play_sample(const MenuDecodedSample& sample) {
    if (sample.pcm_mono.empty() || sample.sample_rate == 0) return;
    auto* active = new MenuWavePlayback();
    active->pcm = sample.pcm_mono;

    WAVEFORMATEX fmt = {};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = sample.sample_rate;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign =
        static_cast<WORD>(fmt.nChannels * fmt.wBitsPerSample / 8);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    MMRESULT r = waveOutOpen(
        &active->handle, WAVE_MAPPER, &fmt,
        reinterpret_cast<DWORD_PTR>(&menu_wave_callback),
        reinterpret_cast<DWORD_PTR>(active), CALLBACK_FUNCTION);
    if (r != MMSYSERR_NOERROR) {
      delete active;
      return;
    }

    active->header.lpData = reinterpret_cast<LPSTR>(active->pcm.data());
    active->header.dwBufferLength =
        static_cast<DWORD>(active->pcm.size() * sizeof(int16_t));
    if (waveOutPrepareHeader(active->handle, &active->header,
                             sizeof(active->header)) != MMSYSERR_NOERROR ||
        waveOutWrite(active->handle, &active->header,
                     sizeof(active->header)) != MMSYSERR_NOERROR) {
      waveOutUnprepareHeader(active->handle, &active->header,
                             sizeof(active->header));
      waveOutClose(active->handle);
      delete active;
    }
  }

  std::unordered_map<std::string, MenuDecodedSample> samples_;
  std::unordered_map<std::string, std::vector<std::string>> cue_samples_;
  size_t next_variant_ = 0;
};

DataArray one_arg(DataNode n) {
  DataArray a;
  a.push(std::move(n));
  return a;
}

Symbol audit_mode_for_screen(const ConfigDb& db, Symbol screen) {
  const DataArray* modes = db.table(Symbol("modes"));
  if (!modes || !screen.valid()) return Symbol();

  static constexpr std::array<const char*, 6> kScreenFields = {
      "main_screen", "continue_screen", "loading_screen",
      "win_screen", "lose_screen", "game_screen"};

  for (std::size_t i = 0; i < modes->size(); ++i) {
    auto rec = modes->at(i).as_array();
    if (!rec || rec->empty()) continue;
    auto mode = rec->at(0).as_symbol();
    if (!mode || *mode == Symbol("defaults")) continue;

    for (const char* field_name : kScreenFields) {
      const Symbol field(field_name);
      const DataNode value = ConfigDb::field(rec.get(), field);
      auto target = value.as_string();
      if (!target || target->empty()) continue;
      if (field == Symbol("continue_screen") &&
          Symbol(*target) == Symbol("main_screen")) {
        continue;
      }
      if (Symbol(*target) == screen) return *mode;
    }
  }

  return Symbol();
}

bool skip_runtime_animated_bind_mesh(const std::string& file,
                                     const std::string& mesh) {
  if (file != "sel_character.milo" && file != "multi_sel_character.milo")
    return false;
  // Character select thumbnails bind with black diffuse materials and are
  // revealed/colored by the stock MatAnim state. Drawing the bind pose in the
  // static editor produces opaque black slabs over the menu.
  return mesh.rfind("char_", 0) == 0 &&
         mesh.find("highlight") == std::string::npos;
}

float menu_rf(const std::vector<uint8_t>& d, size_t o) {
  float v = 0.0f;
  if (o + sizeof(v) <= d.size()) std::memcpy(&v, d.data() + o, sizeof(v));
  return v;
}

milo_scene::Xfm menu_xfm_at(const std::vector<uint8_t>& body, size_t offset) {
  milo_scene::Xfm x;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      x.rot[r][c] = menu_rf(body, offset + static_cast<size_t>(r * 3 + c) * 4);
  for (int c = 0; c < 3; ++c)
    x.pos[c] = menu_rf(body, offset + static_cast<size_t>(9 + c) * 4);
  return x;
}

bool load_ui_proxy_xfms(const std::string& hdr, const std::string& ark,
                        const std::string& milo_path,
                        const std::string& proxy_name,
                        milo_scene::Xfm& local,
                        milo_scene::Xfm& world,
                        std::string& parent) {
  const auto body =
      load_milo_entry_body(hdr, ark, milo_path, "UIProxy", proxy_name);
  // UIProxy bodies store the same 48-byte local/world Trans pair as labels,
  // beginning at byte 0x1b. Verified against sel_guitar/multi_sel_guitar/store.
  constexpr size_t kProxyLocalOffset = 0x1b;
  constexpr size_t kProxyWorldOffset = kProxyLocalOffset + 48;
  if (body.size() < kProxyWorldOffset + 48) return false;
  local = menu_xfm_at(body, kProxyLocalOffset);
  world = menu_xfm_at(body, kProxyWorldOffset);
  parent.clear();
  constexpr size_t kProxyParentOffset = kProxyWorldOffset + 48 + 9;
  if (kProxyParentOffset + 4 <= body.size()) {
    uint32_t len = 0;
    std::memcpy(&len, body.data() + kProxyParentOffset, sizeof(len));
    if (len <= 256 && kProxyParentOffset + 4 + len <= body.size()) {
      parent.assign(reinterpret_cast<const char*>(body.data() +
                                                  kProxyParentOffset + 4),
                    len);
    }
  }
  return true;
}

bool load_ui_proxy_world_xfm(const std::string& hdr, const std::string& ark,
                             const std::string& milo_path,
                             const std::string& proxy_name,
                             milo_scene::Xfm& out) {
  milo_scene::Xfm local;
  std::string parent;
  return load_ui_proxy_xfms(hdr, ark, milo_path, proxy_name, local, out,
                            parent);
}

std::array<float, 16> menu_xfm_to_mat4(const milo_scene::Xfm& x) {
  return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
          x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
          x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
          x.pos[0],    x.pos[1],    x.pos[2],    1.0f};
}

milo_scene::Xfm menu_mat4_to_xfm(const std::array<float, 16>& m) {
  milo_scene::Xfm x;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) x.rot[r][c] = m[r * 4 + c];
  x.pos[0] = m[12];
  x.pos[1] = m[13];
  x.pos[2] = m[14];
  return x;
}

std::array<float, 16> menu_mat4_mul(const std::array<float, 16>& a,
                                    const std::array<float, 16>& b) {
  std::array<float, 16> r{};
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) s += a[row * 4 + k] * b[k * 4 + col];
      r[row * 4 + col] = s;
    }
  return r;
}

std::array<float, 16> menu_mat4_identity() {
  return {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

bool menu_mat4_inverse_affine(const std::array<float, 16>& m,
                              std::array<float, 16>& out) {
  const float a = m[0], b = m[1], c = m[2];
  const float d = m[4], e = m[5], f = m[6];
  const float g = m[8], h = m[9], i = m[10];
  const float det = a * (e * i - f * h) -
                    b * (d * i - f * g) +
                    c * (d * h - e * g);
  if (!std::isfinite(det) || std::fabs(det) < 1.0e-7f) return false;
  const float inv_det = 1.0f / det;
  out = menu_mat4_identity();
  out[0] = (e * i - f * h) * inv_det;
  out[1] = (c * h - b * i) * inv_det;
  out[2] = (b * f - c * e) * inv_det;
  out[4] = (f * g - d * i) * inv_det;
  out[5] = (a * i - c * g) * inv_det;
  out[6] = (c * d - a * f) * inv_det;
  out[8] = (d * h - e * g) * inv_det;
  out[9] = (b * g - a * h) * inv_det;
  out[10] = (a * e - b * d) * inv_det;
  out[12] = -(m[12] * out[0] + m[13] * out[4] + m[14] * out[8]);
  out[13] = -(m[12] * out[1] + m[13] * out[5] + m[14] * out[9]);
  out[14] = -(m[12] * out[2] + m[13] * out[6] + m[14] * out[10]);
  return true;
}

std::array<float, 16> menu_scene_anchor_inverse(
    const milo_scene::Scene& scene, const char* anchor_name) {
  if (!anchor_name || !anchor_name[0]) return menu_mat4_identity();
  for (const auto& mesh : scene.meshes) {
    if (!mesh.decoded || mesh.name != anchor_name) continue;
    std::array<float, 16> inverse;
    if (menu_mat4_inverse_affine(scene.world_matrix(mesh), inverse))
      return inverse;
    break;
  }
  return menu_mat4_identity();
}

struct MenuStringHit {
  size_t offset = 0;
  size_t end = 0;
  std::string value;
};

std::vector<MenuStringHit> menu_strings_with_offsets(
    const std::vector<uint8_t>& body) {
  std::vector<MenuStringHit> out;
  for (size_t off = 0; off + 4 <= body.size(); ++off) {
    uint32_t len = 0;
    std::memcpy(&len, body.data() + off, sizeof(len));
    if (len == 0 || len > 128 || off + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + off + 4);
    bool printable = true;
    bool has_alpha = false;
    for (uint32_t i = 0; i < len; ++i) {
      const unsigned char c = static_cast<unsigned char>(s[i]);
      if (c < 0x20 || c > 0x7e) {
        printable = false;
        break;
      }
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) has_alpha = true;
    }
    if (!printable || !has_alpha) continue;
    out.push_back({off, off + 4 + len, std::string(s, s + len)});
    off += 3 + len;
  }
  return out;
}

bool menu_ref_has_suffix(std::string_view ref, std::string_view suffix) {
  return ref.size() >= suffix.size() &&
         ref.compare(ref.size() - suffix.size(), suffix.size(), suffix) == 0;
}

float menu_f32_or(const std::vector<uint8_t>& body, size_t offset,
                  float fallback) {
  if (offset + 4 > body.size()) return fallback;
  float value = 0.0f;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return std::isfinite(value) ? value : fallback;
}

uint32_t menu_u32_or(const std::vector<uint8_t>& body, size_t offset,
                     uint32_t fallback) {
  if (offset + 4 > body.size()) return fallback;
  uint32_t value = 0;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

bool menu_read_u32_advance(const std::vector<uint8_t>& body, size_t& pos,
                           uint32_t& out) {
  if (pos + 4 > body.size()) return false;
  std::memcpy(&out, body.data() + pos, sizeof(out));
  pos += 4;
  return true;
}

bool menu_read_f32_advance(const std::vector<uint8_t>& body, size_t& pos,
                           float& out) {
  if (pos + 4 > body.size()) return false;
  std::memcpy(&out, body.data() + pos, sizeof(out));
  pos += 4;
  return std::isfinite(out);
}

bool menu_read_string_advance(const std::vector<uint8_t>& body, size_t& pos,
                              std::string& out) {
  uint32_t len = 0;
  if (!menu_read_u32_advance(body, pos, len)) return false;
  if (len == 0 || len > 128 || pos + len > body.size()) return false;
  out.assign(reinterpret_cast<const char*>(body.data() + pos), len);
  pos += len;
  return true;
}

float clamp_menu_material_alpha(float value) {
  if (!std::isfinite(value)) return 1.0f;
  return std::clamp(value, 0.0f, 1.0f);
}

float clamp_menu_material_color(float value) {
  if (!std::isfinite(value)) return 1.0f;
  return std::clamp(value, 0.0f, 4.0f);
}

struct MenuMaterialAnim {
  struct FloatKey {
    float value = 0.0f;
    float frame = 0.0f;
  };
  struct Vec3Key {
    float value[3] = {0.0f, 0.0f, 0.0f};
    float frame = 0.0f;
  };
  struct ColorKey {
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float frame = 0.0f;
  };
  struct TextureKey {
    std::string texture;
    float frame = 0.0f;
  };
  std::string name;
  std::string material;
  bool has_alpha = false;
  float duration_frames = 0.0f;
  std::vector<ColorKey> color_keys;
  std::vector<FloatKey> alpha_keys;
  std::vector<Vec3Key> tex_translation_keys;
  std::vector<Vec3Key> tex_scale_keys;
  std::vector<FloatKey> tex_rotation_keys;
  std::vector<TextureKey> texture_keys;
};

bool decode_menu_matanim(const std::vector<uint8_t>& body,
                         MenuMaterialAnim& anim) {
  if (body.size() < 40 || menu_u32_or(body, 0, 0) != 7) return false;
  size_t pos = 25;
  if (!menu_read_string_advance(body, pos, anim.material) ||
      !menu_read_string_advance(body, pos, anim.name)) {
    return false;
  }

  uint32_t color_count = 0;
  if (!menu_read_u32_advance(body, pos, color_count) ||
      color_count > 4096) {
    return false;
  }
  for (uint32_t i = 0; i < color_count; ++i) {
    MenuMaterialAnim::ColorKey key;
    if (!menu_read_f32_advance(body, pos, key.color[0]) ||
        !menu_read_f32_advance(body, pos, key.color[1]) ||
        !menu_read_f32_advance(body, pos, key.color[2]) ||
        !menu_read_f32_advance(body, pos, key.color[3]) ||
        !menu_read_f32_advance(body, pos, key.frame)) {
      return false;
    }
    for (float& c : key.color) c = clamp_menu_material_color(c);
    anim.duration_frames = std::max(anim.duration_frames, key.frame);
    anim.color_keys.push_back(key);
  }

  uint32_t alpha_count = 0;
  if (!menu_read_u32_advance(body, pos, alpha_count) ||
      alpha_count > 4096) {
    return false;
  }
  for (uint32_t i = 0; i < alpha_count; ++i) {
    MenuMaterialAnim::FloatKey key;
    if (!menu_read_f32_advance(body, pos, key.value) ||
        !menu_read_f32_advance(body, pos, key.frame)) {
      return false;
    }
    key.value = clamp_menu_material_alpha(key.value);
    anim.duration_frames = std::max(anim.duration_frames, key.frame);
    anim.alpha_keys.push_back(key);
  }
  anim.has_alpha = !anim.alpha_keys.empty();

  uint32_t trans_count = 0;
  if (!menu_read_u32_advance(body, pos, trans_count) ||
      trans_count > 4096) {
    return false;
  }
  for (uint32_t i = 0; i < trans_count; ++i) {
    MenuMaterialAnim::Vec3Key key;
    if (!menu_read_f32_advance(body, pos, key.value[0]) ||
        !menu_read_f32_advance(body, pos, key.value[1]) ||
        !menu_read_f32_advance(body, pos, key.value[2]) ||
        !menu_read_f32_advance(body, pos, key.frame)) {
      return false;
    }
    anim.duration_frames = std::max(anim.duration_frames, key.frame);
    anim.tex_translation_keys.push_back(key);
  }

  uint32_t scale_count = 0;
  if (!menu_read_u32_advance(body, pos, scale_count) ||
      scale_count > 4096) {
    return false;
  }
  for (uint32_t i = 0; i < scale_count; ++i) {
    MenuMaterialAnim::Vec3Key key;
    if (!menu_read_f32_advance(body, pos, key.value[0]) ||
        !menu_read_f32_advance(body, pos, key.value[1]) ||
        !menu_read_f32_advance(body, pos, key.value[2]) ||
        !menu_read_f32_advance(body, pos, key.frame)) {
      return false;
    }
    anim.duration_frames = std::max(anim.duration_frames, key.frame);
    anim.tex_scale_keys.push_back(key);
  }

  uint32_t rot_count = 0;
  if (!menu_read_u32_advance(body, pos, rot_count) ||
      rot_count > 4096) {
    return false;
  }
  for (uint32_t i = 0; i < rot_count; ++i) {
    MenuMaterialAnim::FloatKey key;
    if (!menu_read_f32_advance(body, pos, key.value) ||
        !menu_read_f32_advance(body, pos, key.frame)) {
      return false;
    }
    anim.duration_frames = std::max(anim.duration_frames, key.frame);
    anim.tex_rotation_keys.push_back(key);
  }

  if (pos + 4 <= body.size()) {
    uint32_t texture_count = 0;
    if (!menu_read_u32_advance(body, pos, texture_count) ||
        texture_count > 4096) {
      return false;
    }
    for (uint32_t i = 0; i < texture_count; ++i) {
      MenuMaterialAnim::TextureKey key;
      if (!menu_read_string_advance(body, pos, key.texture) ||
          key.texture.rfind(".tex") == std::string::npos ||
          !menu_read_f32_advance(body, pos, key.frame)) {
        return false;
      }
      anim.duration_frames = std::max(anim.duration_frames, key.frame);
      anim.texture_keys.push_back(std::move(key));
    }
  }

  return !anim.material.empty() && !anim.name.empty() &&
         (!anim.color_keys.empty() || anim.has_alpha ||
          !anim.tex_translation_keys.empty() || !anim.tex_scale_keys.empty() ||
          !anim.tex_rotation_keys.empty() || !anim.texture_keys.empty());
}

std::map<std::string, MenuMaterialAnim> load_menu_matanims(
    const std::string& hdr, const std::string& ark,
    const std::string& milo_path) {
  std::map<std::string, MenuMaterialAnim> out;
  for (const std::string& name :
       milo_entry_names_by_type(hdr, ark, milo_path, "MatAnim")) {
    MenuMaterialAnim anim;
    if (!decode_menu_matanim(
            load_milo_entry_body(hdr, ark, milo_path, "MatAnim", name),
            anim)) {
      continue;
    }
    out[anim.name] = std::move(anim);
  }
  return out;
}

std::unordered_set<std::string> menu_matanim_texture_refs(
    const std::string& hdr, const std::string& ark,
    const std::string& milo_path) {
  std::unordered_set<std::string> refs;
  for (const auto& [_, anim] : load_menu_matanims(hdr, ark, milo_path)) {
    for (const auto& key : anim.texture_keys) {
      if (!key.texture.empty()) refs.insert(key.texture);
    }
  }
  return refs;
}

float menu_material_anim_frame_at(float duration_frames,
                                  double elapsed_seconds) {
  if (!std::isfinite(duration_frames) || duration_frames <= 0.001f)
    return 0.0f;
  float frame = static_cast<float>(std::max(0.0, elapsed_seconds) * 30.0);
  frame = std::fmod(frame, duration_frames);
  return std::clamp(frame, 0.0f, duration_frames);
}

std::array<float, 2> sample_menu_material_vec2_key(
    const std::vector<MenuMaterialAnim::Vec3Key>& keys, float frame) {
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
  for (int i = 0; i < 2; ++i)
    out[i] = a->value[i] + (b->value[i] - a->value[i]) * t;
  return out;
}

float sample_menu_material_float_key(
    const std::vector<MenuMaterialAnim::FloatKey>& keys, float frame) {
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
  return a->value + (b->value - a->value) * t;
}

std::array<float, 4> sample_menu_material_color_key(
    const std::vector<MenuMaterialAnim::ColorKey>& keys, float frame) {
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
    out[i] = clamp_menu_material_color(
        a->color[i] + (b->color[i] - a->color[i]) * t);
  }
  return out;
}

std::string sample_menu_material_texture_key(
    const std::vector<MenuMaterialAnim::TextureKey>& keys, float frame) {
  if (keys.empty()) return {};
  const auto* chosen = &keys.front();
  for (const auto& key : keys) {
    if (frame < key.frame) break;
    chosen = &key;
  }
  return chosen->texture;
}

ghogx::render::MiloSceneRenderer::MaterialTexTransformSample
sample_menu_material_tex_transform(const MenuMaterialAnim& anim, float frame) {
  ghogx::render::MiloSceneRenderer::MaterialTexTransformSample sample;
  if (!anim.tex_translation_keys.empty()) {
    sample.has_translation = true;
    sample.translation =
        sample_menu_material_vec2_key(anim.tex_translation_keys, frame);
  }
  if (!anim.tex_scale_keys.empty()) {
    sample.has_scale = true;
    sample.scale = sample_menu_material_vec2_key(anim.tex_scale_keys, frame);
  }
  if (!anim.tex_rotation_keys.empty()) {
    sample.has_rotation = true;
    sample.rotation_radians =
        sample_menu_material_float_key(anim.tex_rotation_keys, frame);
  }
  return sample;
}

class MenuMaterialAnimPlayer {
 public:
  void reset(ghogx::render::MiloSceneRenderer& renderer) {
    loops_.clear();
    one_shots_.clear();
    elapsed_seconds_ = 0.0;
    renderer.set_material_alpha_multipliers({});
    renderer.set_material_color_overrides({});
    renderer.set_material_texture_overrides({});
    renderer.set_material_tex_transform_overrides({});
  }

  void add_loop(MenuMaterialAnim anim) {
    loops_.push_back(std::move(anim));
  }

  void trigger_one_shot(MenuMaterialAnim anim) {
    OneShot active;
    active.anim = std::move(anim);
    one_shots_.push_back(std::move(active));
  }

  void apply(ghogx::render::MiloSceneRenderer& renderer) const {
    std::map<std::string, float> alphas;
    std::map<std::string, std::array<float, 4>> colors;
    std::map<std::string, std::string> textures;
    std::map<std::string,
             ghogx::render::MiloSceneRenderer::MaterialTexTransformSample>
        tex_transforms;
    for (const MenuMaterialAnim& anim : loops_) {
      const float frame =
          menu_material_anim_frame_at(anim.duration_frames, elapsed_seconds_);
      if (anim.has_alpha)
        alphas[anim.material] =
            clamp_menu_material_alpha(
                sample_menu_material_float_key(anim.alpha_keys, frame));
      if (!anim.color_keys.empty())
        colors[anim.material] =
            sample_menu_material_color_key(anim.color_keys, frame);
      if (!anim.texture_keys.empty())
        textures[anim.material] =
            sample_menu_material_texture_key(anim.texture_keys, frame);
      if (!anim.tex_translation_keys.empty() || !anim.tex_scale_keys.empty() ||
          !anim.tex_rotation_keys.empty()) {
        tex_transforms[anim.material] =
            sample_menu_material_tex_transform(anim, frame);
      }
    }
    for (const OneShot& shot : one_shots_) {
      const MenuMaterialAnim& anim = shot.anim;
      const float duration =
          std::isfinite(anim.duration_frames) ? anim.duration_frames : 0.0f;
      const float frame =
          duration <= 0.001f
              ? 0.0f
              : std::clamp(static_cast<float>(shot.elapsed_seconds * 30.0),
                           0.0f, duration);
      if (anim.has_alpha)
        alphas[anim.material] =
            clamp_menu_material_alpha(
                sample_menu_material_float_key(anim.alpha_keys, frame));
      if (!anim.color_keys.empty())
        colors[anim.material] =
            sample_menu_material_color_key(anim.color_keys, frame);
      if (!anim.texture_keys.empty())
        textures[anim.material] =
            sample_menu_material_texture_key(anim.texture_keys, frame);
      if (!anim.tex_translation_keys.empty() || !anim.tex_scale_keys.empty() ||
          !anim.tex_rotation_keys.empty()) {
        tex_transforms[anim.material] =
            sample_menu_material_tex_transform(anim, frame);
      }
    }
    renderer.set_material_alpha_multipliers(std::move(alphas));
    renderer.set_material_color_overrides(std::move(colors));
    renderer.set_material_texture_overrides(std::move(textures));
    renderer.set_material_tex_transform_overrides(std::move(tex_transforms));
  }

  void update(float dt_seconds, ghogx::render::MiloSceneRenderer& renderer) {
    if (loops_.empty() && one_shots_.empty()) return;
    elapsed_seconds_ += std::max(0.0f, dt_seconds);
    for (OneShot& shot : one_shots_) {
      shot.elapsed_seconds += std::max(0.0f, dt_seconds);
    }
    one_shots_.erase(
        std::remove_if(
            one_shots_.begin(), one_shots_.end(),
            [](const OneShot& shot) {
              const float duration =
                  std::isfinite(shot.anim.duration_frames)
                      ? shot.anim.duration_frames / 30.0f
                      : 0.0f;
              return duration > 0.001f &&
                     shot.elapsed_seconds >= duration;
            }),
        one_shots_.end());
    apply(renderer);
  }

 private:
  struct OneShot {
    MenuMaterialAnim anim;
    double elapsed_seconds = 0.0;
  };
  std::vector<MenuMaterialAnim> loops_;
  std::vector<OneShot> one_shots_;
  double elapsed_seconds_ = 0.0;
};

void menu_apply_local_translation_delta(std::array<float, 16>& world,
                                        const std::array<float, 3>& delta) {
  const float dx = delta[0] * world[0] + delta[1] * world[4] +
                   delta[2] * world[8];
  const float dy = delta[0] * world[1] + delta[1] * world[5] +
                   delta[2] * world[9];
  const float dz = delta[0] * world[2] + delta[1] * world[6] +
                   delta[2] * world[10];
  world[12] += dx;
  world[13] += dy;
  world[14] += dz;
}

std::array<float, 4> menu_normalize_quat_xyzw(std::array<float, 4> q) {
  const float len = std::sqrt(q[0] * q[0] + q[1] * q[1] +
                              q[2] * q[2] + q[3] * q[3]);
  if (!std::isfinite(len) || len <= 0.000001f)
    return {0.0f, 0.0f, 0.0f, 1.0f};
  const float inv = 1.0f / len;
  for (float& v : q) v *= inv;
  return q;
}

std::array<float, 4> menu_quat_conjugate_xyzw(std::array<float, 4> q) {
  q[0] = -q[0];
  q[1] = -q[1];
  q[2] = -q[2];
  return q;
}

std::array<float, 4> menu_quat_mul_xyzw(const std::array<float, 4>& a,
                                        const std::array<float, 4>& b) {
  const float ax = a[0], ay = a[1], az = a[2], aw = a[3];
  const float bx = b[0], by = b[1], bz = b[2], bw = b[3];
  return menu_normalize_quat_xyzw({
      aw * bx + ax * bw + ay * bz - az * by,
      aw * by - ax * bz + ay * bw + az * bx,
      aw * bz + ax * by - ay * bx + az * bw,
      aw * bw - ax * bx - ay * by - az * bz,
  });
}

void menu_quat_xyzw_to_row_rot(const std::array<float, 4>& q_in,
                               float rot[3][3]) {
  const auto q = menu_normalize_quat_xyzw(q_in);
  const float x = q[0], y = q[1], z = q[2], w = q[3];
  rot[0][0] = 1.0f - 2.0f * (y * y + z * z);
  rot[0][1] = 2.0f * (x * y + z * w);
  rot[0][2] = 2.0f * (x * z - y * w);
  rot[1][0] = 2.0f * (x * y - z * w);
  rot[1][1] = 1.0f - 2.0f * (x * x + z * z);
  rot[1][2] = 2.0f * (y * z + x * w);
  rot[2][0] = 2.0f * (x * z + y * w);
  rot[2][1] = 2.0f * (y * z - x * w);
  rot[2][2] = 1.0f - 2.0f * (x * x + y * y);
}

void menu_apply_local_rotation_delta(std::array<float, 16>& world,
                                     const std::array<float, 4>& quat_xyzw) {
  float rot[3][3];
  menu_quat_xyzw_to_row_rot(quat_xyzw, rot);
  const std::array<float, 9> basis = {world[0], world[1], world[2],
                                      world[4], world[5], world[6],
                                      world[8], world[9], world[10]};
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      world[r * 4 + c] =
          basis[r * 3 + 0] * rot[0][c] +
          basis[r * 3 + 1] * rot[1][c] +
          basis[r * 3 + 2] * rot[2][c];
    }
  }
}

void menu_apply_local_scale_delta(std::array<float, 16>& world,
                                  const std::array<float, 3>& scale) {
  for (int c = 0; c < 3; ++c) world[c] *= scale[0];
  for (int c = 0; c < 3; ++c) world[4 + c] *= scale[1];
  for (int c = 0; c < 3; ++c) world[8 + c] *= scale[2];
}

struct MenuTransAnimBlock {
  size_t offset = 0;
  std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey> keys;
  float delta = 0.0f;
  bool scale_like = false;
};

std::vector<MenuTransAnimBlock> decode_menu_transanim_vec3_blocks(
    const std::vector<uint8_t>& body) {
  using Key = ghogx::render::MiloSceneRenderer::MeshAnimKey;
  auto plausible_vec = [](float v) {
    return std::isfinite(v) && std::abs(v) < 2000.0f;
  };

  std::vector<MenuTransAnimBlock> blocks;
  for (size_t off = 0; off + 4 <= body.size(); ++off) {
    const uint32_t count = menu_u32_or(body, off, 0);
    if (count < 2 || count > 128) continue;
    const size_t start = off + 4;
    if (start + static_cast<size_t>(count) * 16 > body.size()) continue;
    MenuTransAnimBlock block;
    block.offset = off;
    block.keys.reserve(count);
    float prev_frame = -1.0f;
    bool ok = true;
    bool scale_like = true;
    for (uint32_t i = 0; i < count; ++i) {
      const size_t p = start + static_cast<size_t>(i) * 16;
      Key k;
      k.pos[0] = menu_f32_or(body, p + 0, 0.0f);
      k.pos[1] = menu_f32_or(body, p + 4, 0.0f);
      k.pos[2] = menu_f32_or(body, p + 8, 0.0f);
      k.frame = menu_f32_or(body, p + 12, -1.0f);
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
decode_menu_transanim_rotation_keys(const std::vector<uint8_t>& body) {
  using Key = ghogx::render::MiloSceneRenderer::MeshQuatAnimKey;
  std::vector<Key> best;
  float best_delta = 0.0f;
  for (size_t off = 0; off + 4 <= body.size(); ++off) {
    const uint32_t count = menu_u32_or(body, off, 0);
    if (count < 2 || count > 128) continue;
    const size_t start = off + 4;
    if (start + static_cast<size_t>(count) * 20 > body.size()) continue;
    std::vector<Key> keys;
    keys.reserve(count);
    float prev_frame = -1.0f;
    bool ok = true;
    for (uint32_t i = 0; i < count; ++i) {
      const size_t p = start + static_cast<size_t>(i) * 20;
      Key k;
      k.quat_xyzw[0] = menu_f32_or(body, p + 0, 0.0f);
      k.quat_xyzw[1] = menu_f32_or(body, p + 4, 0.0f);
      k.quat_xyzw[2] = menu_f32_or(body, p + 8, 0.0f);
      k.quat_xyzw[3] = menu_f32_or(body, p + 12, 1.0f);
      k.frame = menu_f32_or(body, p + 16, -1.0f);
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

ghogx::render::MiloSceneRenderer::MeshTransformAnim
decode_menu_transanim_transform_anim(const std::vector<uint8_t>& body) {
  ghogx::render::MiloSceneRenderer::MeshTransformAnim anim;
  const auto blocks = decode_menu_transanim_vec3_blocks(body);
  const MenuTransAnimBlock* translation = nullptr;
  const MenuTransAnimBlock* fallback_translation = nullptr;
  const MenuTransAnimBlock* scale = nullptr;
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
  anim.rotation_keys = decode_menu_transanim_rotation_keys(body);
  return anim;
}

const ghogx::render::MiloSceneRenderer::MeshAnimKey* menu_sample_vec_key(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey>& keys,
    float frame,
    const ghogx::render::MiloSceneRenderer::MeshAnimKey** next) {
  if (keys.empty()) {
    *next = nullptr;
    return nullptr;
  }
  const auto* a = &keys.front();
  const auto* b = &keys.back();
  for (size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  *next = b;
  return a;
}

std::array<float, 3> menu_sample_vec_delta(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey>& keys,
    float frame) {
  std::array<float, 3> out = {0.0f, 0.0f, 0.0f};
  const ghogx::render::MiloSceneRenderer::MeshAnimKey* b = nullptr;
  const auto* a = menu_sample_vec_key(keys, frame, &b);
  if (!a || !b) return out;
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  for (int i = 0; i < 3; ++i) {
    const float p = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
    out[i] = p - keys.front().pos[i];
  }
  return out;
}

std::array<float, 3> menu_sample_scale_ratio(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey>& keys,
    float frame) {
  std::array<float, 3> out = {1.0f, 1.0f, 1.0f};
  const ghogx::render::MiloSceneRenderer::MeshAnimKey* b = nullptr;
  const auto* a = menu_sample_vec_key(keys, frame, &b);
  if (!a || !b) return out;
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  for (int i = 0; i < 3; ++i) {
    const float p = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
    const float base = keys.front().pos[i];
    out[i] = std::fabs(base) > 0.0001f ? p / base : 1.0f;
  }
  return out;
}

std::array<float, 4> menu_slerp_quat_xyzw(std::array<float, 4> a,
                                          std::array<float, 4> b,
                                          float t) {
  a = menu_normalize_quat_xyzw(a);
  b = menu_normalize_quat_xyzw(b);
  float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
  if (dot < 0.0f) {
    for (float& v : b) v = -v;
    dot = -dot;
  }
  if (dot > 0.9995f) {
    return menu_normalize_quat_xyzw({a[0] + (b[0] - a[0]) * t,
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
  return menu_normalize_quat_xyzw({a[0] * s0 + b[0] * s1,
                                   a[1] * s0 + b[1] * s1,
                                   a[2] * s0 + b[2] * s1,
                                   a[3] * s0 + b[3] * s1});
}

std::array<float, 4> menu_sample_quat_delta(
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
  const auto cur = menu_slerp_quat_xyzw(qa, qb, t);
  const std::array<float, 4> base = {
      keys.front().quat_xyzw[0], keys.front().quat_xyzw[1],
      keys.front().quat_xyzw[2], keys.front().quat_xyzw[3]};
  return menu_quat_mul_xyzw(menu_quat_conjugate_xyzw(
                                menu_normalize_quat_xyzw(base)),
                            cur);
}

ghogx::render::MiloSceneRenderer::MeshTransformSample
menu_sample_transform_anim(
    const ghogx::render::MiloSceneRenderer::MeshTransformAnim& anim,
    float frame) {
  ghogx::render::MiloSceneRenderer::MeshTransformSample sample;
  if (anim.translation_keys.size() >= 2) {
    sample.has_translation = true;
    sample.translation = menu_sample_vec_delta(anim.translation_keys, frame);
  }
  if (anim.rotation_keys.size() >= 2) {
    sample.has_rotation = true;
    sample.rotation_xyzw = menu_sample_quat_delta(anim.rotation_keys, frame);
  }
  if (anim.scale_keys.size() >= 2) {
    sample.has_scale = true;
    sample.scale = menu_sample_scale_ratio(anim.scale_keys, frame);
  }
  return sample;
}

std::array<float, 3> menu_sample_vec_absolute(
    const std::vector<ghogx::render::MiloSceneRenderer::MeshAnimKey>& keys,
    float frame) {
  std::array<float, 3> out = {0.0f, 0.0f, 0.0f};
  const ghogx::render::MiloSceneRenderer::MeshAnimKey* b = nullptr;
  const auto* a = menu_sample_vec_key(keys, frame, &b);
  if (!a || !b) return out;
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  for (int i = 0; i < 3; ++i) out[i] = a->pos[i] + (b->pos[i] - a->pos[i]) * t;
  return out;
}

std::array<float, 4> menu_sample_quat_absolute(
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
  return menu_slerp_quat_xyzw(
      {a->quat_xyzw[0], a->quat_xyzw[1], a->quat_xyzw[2],
       a->quat_xyzw[3]},
      {b->quat_xyzw[0], b->quat_xyzw[1], b->quat_xyzw[2],
       b->quat_xyzw[3]},
      t);
}

ghogx::render::MiloSceneRenderer::MeshTransformSample
menu_sample_transform_anim_absolute(
    const ghogx::render::MiloSceneRenderer::MeshTransformAnim& anim,
    float frame) {
  ghogx::render::MiloSceneRenderer::MeshTransformSample sample;
  if (anim.translation_keys.size() >= 2) {
    sample.has_translation = true;
    sample.translation = menu_sample_vec_absolute(anim.translation_keys, frame);
  }
  if (anim.rotation_keys.size() >= 2) {
    sample.has_rotation = true;
    sample.rotation_xyzw = menu_sample_quat_absolute(anim.rotation_keys, frame);
  }
  if (anim.scale_keys.size() >= 2) {
    sample.has_scale = true;
    sample.scale = menu_sample_vec_absolute(anim.scale_keys, frame);
  }
  return sample;
}

milo_scene::Xfm menu_xfm_with_absolute_sample(
    milo_scene::Xfm base,
    const ghogx::render::MiloSceneRenderer::MeshTransformSample& sample) {
  const std::array<float, 3> base_scale = {
      std::sqrt(base.rot[0][0] * base.rot[0][0] +
                base.rot[0][1] * base.rot[0][1] +
                base.rot[0][2] * base.rot[0][2]),
      std::sqrt(base.rot[1][0] * base.rot[1][0] +
                base.rot[1][1] * base.rot[1][1] +
                base.rot[1][2] * base.rot[1][2]),
      std::sqrt(base.rot[2][0] * base.rot[2][0] +
                base.rot[2][1] * base.rot[2][1] +
                base.rot[2][2] * base.rot[2][2])};
  if (sample.has_rotation) {
    menu_quat_xyzw_to_row_rot(sample.rotation_xyzw, base.rot);
    for (int c = 0; c < 3; ++c) base.rot[0][c] *= base_scale[0];
    for (int c = 0; c < 3; ++c) base.rot[1][c] *= base_scale[1];
    for (int c = 0; c < 3; ++c) base.rot[2][c] *= base_scale[2];
  }
  if (sample.has_scale) {
    for (int c = 0; c < 3; ++c) base.rot[0][c] *= sample.scale[0];
    for (int c = 0; c < 3; ++c) base.rot[1][c] *= sample.scale[1];
    for (int c = 0; c < 3; ++c) base.rot[2][c] *= sample.scale[2];
  }
  if (sample.has_translation) {
    base.pos[0] = sample.translation[0];
    base.pos[1] = sample.translation[1];
    base.pos[2] = sample.translation[2];
  }
  return base;
}

const milo_scene::Xfm* menu_scene_local_xfm(const milo_scene::Scene& scene,
                                            const std::string& name,
                                            std::string* parent = nullptr) {
  for (const auto& group : scene.groups) {
    if (group.name == name && group.has_transform) {
      if (parent) *parent = group.parent;
      return &group.local;
    }
  }
  for (const auto& trans : scene.transes) {
    if (trans.name == name) {
      if (parent) *parent = trans.parent;
      return &trans.local;
    }
  }
  for (const auto& mesh : scene.meshes) {
    if (mesh.name == name) {
      if (parent) *parent = mesh.parent;
      return &mesh.local;
    }
  }
  if (parent) parent->clear();
  return nullptr;
}

std::array<float, 16> menu_scene_object_world_matrix(
    const milo_scene::Scene& scene, const std::string& name) {
  std::string parent;
  const milo_scene::Xfm* local = menu_scene_local_xfm(scene, name, &parent);
  if (!local) return menu_mat4_identity();
  std::array<float, 16> world = menu_xfm_to_mat4(*local);
  int guard = 0;
  while (!parent.empty() && guard++ < 64) {
    std::string next_parent;
    const milo_scene::Xfm* px =
        menu_scene_local_xfm(scene, parent, &next_parent);
    if (!px) break;
    world = menu_mat4_mul(world, menu_xfm_to_mat4(*px));
    if (next_parent == parent) break;
    parent = next_parent;
  }
  return world;
}

void menu_scene_set_local_xfm(milo_scene::Scene& scene,
                              const std::string& name,
                              const milo_scene::Xfm& xfm) {
  for (auto& group : scene.groups) {
    if (group.name == name && group.has_transform) {
      group.local = xfm;
      group.world_stored = menu_mat4_to_xfm(
          menu_scene_object_world_matrix(scene, group.name));
      return;
    }
  }
  for (auto& trans : scene.transes) {
    if (trans.name == name) {
      trans.local = xfm;
      trans.world_stored = menu_mat4_to_xfm(
          menu_scene_object_world_matrix(scene, trans.name));
      return;
    }
  }
  for (auto& mesh : scene.meshes) {
    if (mesh.name == name) {
      mesh.local = xfm;
      mesh.world_stored = menu_mat4_to_xfm(
          menu_scene_object_world_matrix(scene, mesh.name));
      return;
    }
  }
}

bool menu_scene_parent_chain_contains(const milo_scene::Scene& scene,
                                      const std::string& name,
                                      const std::string& ancestor) {
  if (name.empty() || ancestor.empty() || name == ancestor) return false;
  std::string parent;
  if (!menu_scene_local_xfm(scene, name, &parent)) return false;
  int guard = 0;
  while (!parent.empty() && guard++ < 64) {
    if (parent == ancestor) return true;
    std::string next_parent;
    if (!menu_scene_local_xfm(scene, parent, &next_parent)) break;
    if (next_parent == parent) break;
    parent = next_parent;
  }
  return false;
}

void menu_scene_refresh_descendant_worlds(milo_scene::Scene& scene,
                                          const std::string& ancestor) {
  if (ancestor.empty()) return;
  for (auto& group : scene.groups) {
    if (!group.has_transform || group.name == ancestor) continue;
    if (!menu_scene_parent_chain_contains(scene, group.name, ancestor))
      continue;
    group.world_stored =
        menu_mat4_to_xfm(menu_scene_object_world_matrix(scene, group.name));
  }
  for (auto& trans : scene.transes) {
    if (trans.name == ancestor) continue;
    if (!menu_scene_parent_chain_contains(scene, trans.name, ancestor))
      continue;
    trans.world_stored =
        menu_mat4_to_xfm(menu_scene_object_world_matrix(scene, trans.name));
  }
  for (auto& mesh : scene.meshes) {
    if (mesh.name == ancestor) continue;
    if (!menu_scene_parent_chain_contains(scene, mesh.name, ancestor))
      continue;
    mesh.world_stored =
        menu_mat4_to_xfm(menu_scene_object_world_matrix(scene, mesh.name));
  }
}

void apply_panel_static_transanim_frames(const std::string& hdr,
                                         const std::string& ark,
                                         Object* panel,
                                         const std::string& milo_path,
                                         milo_scene::Scene& scene) {
  auto* dir = dynamic_cast<ObjectDir*>(panel);
  if (!dir || milo_path.empty()) return;
  for (std::size_t i = 0; i < dir->size(); ++i) {
    Object* obj = dir->at(i);
    if (!obj) continue;
    const std::string name = obj->name().c_str();
    if (!menu_ref_has_suffix(name, ".tnm")) continue;
    const DataNode frame_node = obj->get_property(Symbol("frame"));
    const auto frame_value = frame_node.as_float();
    if (!frame_value) continue;
    const auto anim_body =
        load_milo_entry_body(hdr, ark, milo_path, "TransAnim", name);
    if (anim_body.empty()) continue;
    const std::string target =
        first_menu_ref_with_suffixes(anim_body, {".mesh", ".grp", ".view"});
    if (target.empty()) continue;
    const milo_scene::Xfm* base = menu_scene_local_xfm(scene, target);
    if (!base) continue;
    const auto anim = decode_menu_transanim_transform_anim(anim_body);
    const auto sample = menu_sample_transform_anim_absolute(anim, *frame_value);
    if (!sample.has_translation && !sample.has_rotation && !sample.has_scale)
      continue;
    menu_scene_set_local_xfm(scene, target,
                             menu_xfm_with_absolute_sample(*base, sample));
    menu_scene_refresh_descendant_worlds(scene, target);
    if (std::getenv("GHOGX_LOG_MENU_ANIMS")) {
      std::fprintf(stderr,
                   "[menu-anim] set_frame panel=%s trans=%s target=%s "
                   "frame=%.2f\n",
                   milo_path.c_str(), name.c_str(), target.c_str(),
                   *frame_value);
    }
  }
}

float setlist_runtime_frame(ScreenManager& mgr, int selected_display_row) {
  if (Object* view = mgr.resolve_object(Symbol("sel_song.view"))) {
    if (auto frame = view->get_property(Symbol("frame")).as_float())
      return *frame;
  }
  if (Object* panel = mgr.find_object(Symbol("sel_song_panel"))) {
    if (auto frame = panel->get_property(Symbol("sel_song_frame")).as_float())
      return *frame;
  }
  return static_cast<float>(selected_display_row);
}

void apply_setlist_paper_scroll(ScreenManager& mgr, const std::string& hdr,
                                const std::string& ark,
                                const std::string& milo_path,
                                milo_scene::Scene& scene) {
  if (milo_path.find("sel_song") == std::string::npos) return;
  const UiListLayout layout =
      extract_ui_list_layout(hdr, ark, milo_path, "ss_song.lst");
  if (!layout.valid || layout.row_height <= 0.0f) return;
  const milo_scene::Xfm* base = menu_scene_local_xfm(scene, "ss_setlist.view");
  if (!base) return;
  milo_scene::Xfm scrolled = *base;
  const float frame = setlist_runtime_frame(mgr, 1);
  scrolled.pos[2] += (frame - 1.0f) * layout.row_height;
  menu_scene_set_local_xfm(scene, "ss_setlist.view", scrolled);
  menu_scene_refresh_descendant_worlds(scene, "ss_setlist.view");
  if (std::getenv("GHOGX_LOG_SETLIST_SCROLL")) {
    std::fprintf(stderr,
                 "[menu] setlist paper scroll frame=%.2f row_h=%.1f dz=%.1f\n",
                 frame, layout.row_height, (frame - 1.0f) * layout.row_height);
  }
}

bool load_guitar_filter_sample(
    const std::string& hdr, const std::string& ark,
    const std::string& filter_milo, const std::string& filter_name,
    ghogx::render::MiloSceneRenderer::MeshTransformSample& sample,
    float frame_override = -1.0f,
    const char* frame_source = nullptr,
    bool absolute_sample = false) {
  const bool log = std::getenv("GHOGX_LOG_GUITAR_FILTER") != nullptr;
  auto fail = [&](const char* reason) {
    if (log) {
      std::fprintf(stderr, "[menu] guitar filter skip %s:%s (%s)\n",
                   filter_milo.c_str(), filter_name.c_str(), reason);
    }
    return false;
  };
  if (filter_milo.empty() || filter_name.empty()) return fail("empty");
  const auto filter_body =
      load_milo_entry_body(hdr, ark, filter_milo, "AnimFilter", filter_name);
  if (filter_body.empty()) return fail("no AnimFilter body");
  std::string transanim_name;
  size_t transanim_offset = 0;
  size_t transanim_end = 0;
  for (const auto& s : menu_strings_with_offsets(filter_body)) {
    if (menu_ref_has_suffix(s.value, ".tnm")) {
      transanim_name = s.value;
      transanim_offset = s.offset;
      transanim_end = s.end;
      break;
    }
  }
  if (transanim_name.empty()) return fail("no TransAnim ref");
  float frame = 0.0f;
  const char* source = frame_source ? frame_source : "serialized";
  if (std::isfinite(frame_override) && frame_override >= 0.0f) {
    frame = frame_override;
  } else {
    frame = transanim_offset >= 8
                ? menu_f32_or(filter_body, transanim_offset - 8, 0.0f)
                : 0.0f;
    const float tail_frame = menu_f32_or(filter_body, transanim_end + 12, 0.0f);
    const float offset_frame =
        menu_f32_or(filter_body, transanim_end + 20, 0.0f);
    if (!std::isfinite(frame) || frame <= 0.001f) frame = tail_frame;
    if (!std::isfinite(frame) || frame <= 0.001f) frame = offset_frame;
    if (!std::isfinite(frame)) frame = 0.0f;
  }

  const auto anim_body =
      load_milo_entry_body(hdr, ark, filter_milo, "TransAnim", transanim_name);
  if (anim_body.empty()) return fail("no TransAnim body");
  const auto anim = decode_menu_transanim_transform_anim(anim_body);
  sample = absolute_sample ? menu_sample_transform_anim_absolute(anim, frame)
                           : menu_sample_transform_anim(anim, frame);
  if (log) {
    std::fprintf(stderr,
                 "[menu] guitar filter %s:%s -> %s frame=%.2f source=%s "
                 "absolute=%d "
                 "pos=%d(%.2f %.2f %.2f) rot=%d scale=%d(%.2f %.2f %.2f)\n",
                 filter_milo.c_str(), filter_name.c_str(),
                 transanim_name.c_str(), frame, source,
                 absolute_sample ? 1 : 0,
                 sample.has_translation ? 1 : 0, sample.translation[0],
                 sample.translation[1],
                 sample.translation[2], sample.has_rotation ? 1 : 0,
                 sample.has_scale ? 1 : 0, sample.scale[0], sample.scale[1],
                 sample.scale[2]);
  }
  return sample.has_translation || sample.has_rotation || sample.has_scale;
}

bool load_guitar_display_group_world(const std::string& hdr,
                                     const std::string& ark,
                                     const std::string& group_name,
                                     milo_scene::Xfm& out) {
  if (group_name.empty()) return false;
  milo_scene::Scene display_scene;
  if (!milo_scene::load_scene(hdr, ark, "ui/gen/guitar_display.milo_ps2",
                              display_scene)) {
    return false;
  }
  for (const auto& group : display_scene.groups) {
    if (group.name == group_name) {
      out = group.world_stored;
      return true;
    }
  }
  return false;
}

bool load_guitar_display_placer_world(const std::string& hdr,
                                      const std::string& ark,
                                      const std::string& placer_name,
                                      milo_scene::Xfm& out) {
  if (placer_name.empty()) return false;
  milo_scene::Scene display_scene;
  if (!milo_scene::load_scene(hdr, ark, "ui/gen/guitar_display.milo_ps2",
                              display_scene)) {
    return false;
  }
  if (const milo_scene::BandPlacerObj* placer =
          display_scene.find_band_placer(placer_name)) {
    out = placer->world_stored;
    return true;
  }
  return false;
}

void apply_menu_transform_sample(
    std::array<float, 16>& world,
    const ghogx::render::MiloSceneRenderer::MeshTransformSample& sample) {
  if (sample.has_translation)
    menu_apply_local_translation_delta(world, sample.translation);
  if (sample.has_rotation)
    menu_apply_local_rotation_delta(world, sample.rotation_xyzw);
  if (sample.has_scale)
    menu_apply_local_scale_delta(world, sample.scale);
}

void transform_menu_point(const std::array<float, 16>& m, float& x, float& y,
                          float& z) {
  const float ox = x, oy = y, oz = z;
  x = ox * m[0] + oy * m[4] + oz * m[8] + m[12];
  y = ox * m[1] + oy * m[5] + oz * m[9] + m[13];
  z = ox * m[2] + oy * m[6] + oz * m[10] + m[14];
}

void transform_menu_normal(const std::array<float, 16>& m, float& x, float& y,
                           float& z) {
  const float ox = x, oy = y, oz = z;
  x = ox * m[0] + oy * m[4] + oz * m[8];
  y = ox * m[1] + oy * m[5] + oz * m[9];
  z = ox * m[2] + oy * m[6] + oz * m[10];
  const float len = std::sqrt(x * x + y * y + z * z);
  if (len > 0.000001f) {
    x /= len;
    y /= len;
    z /= len;
  }
}

std::string data_node_symbol_text(const DataNode& node) {
  if (auto s = node.as_symbol()) return s->c_str();
  if (auto s = node.as_string()) return std::string(s->data(), s->size());
  if (Object* o = node.as_object()) return o->name().c_str();
  return {};
}

Symbol menu_indexed_symbol(const char* stem, int index) {
  return Symbol((std::string(stem) + std::to_string(std::max(0, index))).c_str());
}

const DataArray* menu_config_record(const ConfigDb& db, const char* table,
                                    Symbol key) {
  const DataArray* t = db.table(Symbol(table));
  auto rec = t ? t->find_keyed(key) : nullptr;
  return rec.get();
}

std::string guitar_outfit_from_skin_record(const DataArray* skin_record) {
  if (!skin_record || skin_record->empty()) return {};
  DataNode outfit = ConfigDb::field(skin_record, Symbol("outfit"));
  if (auto s = outfit.as_symbol()) return s->c_str();
  if (auto s = outfit.as_string()) return std::string(s->data(), s->size());
  if (auto s = skin_record->at(0).as_symbol()) return s->c_str();
  if (auto s = skin_record->at(0).as_string()) return std::string(s->data(), s->size());
  return {};
}

std::string guitar_outfit_from_record(const DataArray* guitar_record,
                                      Symbol skin) {
  if (!guitar_record) return {};
  auto skins = guitar_record->find_keyed(Symbol("skins"));
  if (!skins) return {};
  for (std::size_t i = 1; i < skins->size(); ++i) {
    auto skin_record = skins->at(i).as_array();
    if (!skin_record || skin_record->empty()) continue;
    Symbol skin_name = skin_record->at(0).as_symbol().value_or(Symbol());
    if (!skin.valid() || skin_name == skin) {
      return guitar_outfit_from_skin_record(skin_record.get());
    }
  }
  return {};
}

std::string guitar_outfit_for(const ConfigDb& db, Symbol guitar, Symbol skin) {
  if (skin.valid()) {
    if (guitar.valid()) {
      std::string outfit =
          guitar_outfit_from_record(menu_config_record(db, "guitars", guitar),
                                    skin);
      if (!outfit.empty()) return outfit;
    }
    const DataArray* guitars = db.table(Symbol("guitars"));
    if (guitars) {
      for (std::size_t i = 0; i < guitars->size(); ++i) {
        auto record = guitars->at(i).as_array();
        std::string outfit = guitar_outfit_from_record(record.get(), skin);
        if (!outfit.empty()) return outfit;
      }
    }
  }

  if (guitar.valid()) {
    std::string outfit =
        guitar_outfit_from_record(menu_config_record(db, "guitars", guitar),
                                  Symbol());
    if (!outfit.empty()) return outfit;
  }

  return guitar_outfit_from_record(menu_config_record(db, "guitars",
                                                     Symbol("lespaul")),
                                   Symbol());
}

std::string guitar_milo_path_for(const ConfigDb& db, std::string guitar,
                                 std::string skin) {
  Symbol guitar_sym = guitar.empty() ? Symbol() : Symbol(guitar.c_str());
  Symbol skin_sym = skin.empty() ? Symbol() : Symbol(skin.c_str());
  const std::string outfit = guitar_outfit_for(db, guitar_sym, skin_sym);
  return outfit.empty() ? std::string()
                        : "char/og/guitars/gen/" + outfit + ".milo_ps2";
}

void bake_scene_to_world(milo_scene::Scene& scene,
                         const std::array<float, 16>& proxy_world,
                         const std::string& prefix,
                         const std::array<float, 16>& anchor_inverse =
                             menu_mat4_identity()) {
  for (auto& mesh : scene.meshes) {
    if (!mesh.decoded) continue;
    const std::array<float, 16> total =
        menu_mat4_mul(menu_mat4_mul(scene.world_matrix(mesh), anchor_inverse),
                      proxy_world);
    for (auto& v : mesh.verts) {
      transform_menu_point(total, v.px, v.py, v.pz);
      transform_menu_normal(total, v.nx, v.ny, v.nz);
    }
    mesh.bb_min[0] = mesh.bb_min[1] = mesh.bb_min[2] = 1.0e30f;
    mesh.bb_max[0] = mesh.bb_max[1] = mesh.bb_max[2] = -1.0e30f;
    for (const auto& v : mesh.verts) {
      mesh.bb_min[0] = std::min(mesh.bb_min[0], v.px);
      mesh.bb_min[1] = std::min(mesh.bb_min[1], v.py);
      mesh.bb_min[2] = std::min(mesh.bb_min[2], v.pz);
      mesh.bb_max[0] = std::max(mesh.bb_max[0], v.px);
      mesh.bb_max[1] = std::max(mesh.bb_max[1], v.py);
      mesh.bb_max[2] = std::max(mesh.bb_max[2], v.pz);
    }
    mesh.local = {};
    mesh.world_stored = {};
    mesh.parent.clear();
    mesh.name = prefix + mesh.name;
    if (!mesh.material.empty()) mesh.material = prefix + mesh.material;
  }
  for (auto& mat : scene.mats) mat.name = prefix + mat.name;
  for (auto& name : scene.draw_order) name = prefix + name;
}

bool load_ui_proxy_panel_world_xfm(const std::string& hdr,
                                   const std::string& ark,
                                   Object* panel,
                                   const std::string& milo_path,
                                   const std::string& proxy_name,
                                   milo_scene::Xfm& out) {
  milo_scene::Xfm proxy_local;
  milo_scene::Xfm proxy_world;
  std::string proxy_parent;
  if (!load_ui_proxy_xfms(hdr, ark, milo_path, proxy_name, proxy_local,
                          proxy_world, proxy_parent)) {
    return false;
  }
  out = proxy_world;
  if (proxy_parent.empty()) return true;

  milo_scene::Scene panel_scene;
  if (!milo_scene::load_scene(hdr, ark, milo_path, panel_scene)) return true;
  apply_panel_static_transanim_frames(hdr, ark, panel, milo_path, panel_scene);
  if (!menu_scene_local_xfm(panel_scene, proxy_parent)) return true;
  const auto parent_world = menu_scene_object_world_matrix(panel_scene, proxy_parent);
  const auto local_world =
      menu_mat4_mul(menu_xfm_to_mat4(proxy_local), parent_world);
  const auto parent_local =
      menu_mat4_mul(parent_world, menu_xfm_to_mat4(proxy_local));
  if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
    std::fprintf(stderr,
                 "[menu] proxy %s:%s parent=%s local*parent=(%.2f %.2f %.2f) "
                 "parent*local=(%.2f %.2f %.2f) stored=(%.2f %.2f %.2f)\n",
                 milo_path.c_str(), proxy_name.c_str(), proxy_parent.c_str(),
                 local_world[12], local_world[13], local_world[14],
                 parent_local[12], parent_local[13], parent_local[14],
                 proxy_world.pos[0], proxy_world.pos[1], proxy_world.pos[2]);
  }
  out = menu_mat4_to_xfm(
      local_world);
  return true;
}

void append_guitar_display_scene(const std::string& hdr, const std::string& ark,
                                 const ConfigDb& db,
                                 milo_scene::Scene& combined,
                                 std::map<std::string, asset::Image>& textures,
                                 Object* proxy_owner_panel,
                                 const std::string& guitar,
                                 const std::string& skin,
                                 const std::string& proxy_milo,
                                 const std::string& proxy_name,
                                 const std::string& filter_milo,
                                 const std::string& filter_name,
                                 const std::string& display_group,
                                 const std::string& display_placer,
                                 const std::string& prefix,
                                  float filter_frame_override = -1.0f,
                                  const char* filter_frame_source = nullptr,
                                  bool filter_absolute_sample = false) {
  const std::string guitar_path = guitar_milo_path_for(db, guitar, skin);
  if (guitar_path.empty()) return;
  if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
    std::fprintf(stderr,
                 "[menu] guitar model guitars.dtb guitar=%s skin=%s path=%s\n",
                 guitar.c_str(), skin.c_str(), guitar_path.c_str());
  }
  milo_scene::Xfm proxy;
  if (!load_ui_proxy_panel_world_xfm(hdr, ark, proxy_owner_panel, proxy_milo,
                                     proxy_name, proxy)) {
    return;
  }
  milo_scene::Scene guitar_scene;
  if (!milo_scene::load_scene(hdr, ark, guitar_path, guitar_scene)) return;

  std::unordered_set<std::string> want;
  for (const auto& mat : guitar_scene.mats)
    if (!mat.diffuse_tex.empty()) want.insert(mat.diffuse_tex);
  auto imgs = asset::load_milo_textures(
      hdr, ark, guitar_path, std::vector<std::string>(want.begin(), want.end()));
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));

  std::array<float, 16> proxy_world = menu_xfm_to_mat4(proxy);
  milo_scene::Xfm display_xfm;
  if (load_guitar_display_placer_world(hdr, ark, display_placer, display_xfm) ||
      load_guitar_display_group_world(hdr, ark, display_group, display_xfm)) {
    proxy_world = menu_xfm_to_mat4(display_xfm);
  }
  ghogx::render::MiloSceneRenderer::MeshTransformSample sample;
  if (load_guitar_filter_sample(hdr, ark, filter_milo, filter_name, sample,
                                filter_frame_override,
                                filter_frame_source,
                                filter_absolute_sample)) {
    apply_menu_transform_sample(proxy_world, sample);
  }
  bake_scene_to_world(guitar_scene, proxy_world, prefix,
                      menu_scene_anchor_inverse(guitar_scene, "guitar.mesh"));
  for (auto& mesh : guitar_scene.meshes) combined.meshes.push_back(std::move(mesh));
  for (auto& mat : guitar_scene.mats) combined.mats.push_back(std::move(mat));
  for (auto& name : guitar_scene.draw_order) combined.draw_order.push_back(std::move(name));
}

bool append_guitar_model_at_matrix(
    const std::string& hdr, const std::string& ark, const ConfigDb& db,
    milo_scene::Scene& combined,
    std::map<std::string, asset::Image>& textures, const std::string& guitar,
    const std::string& skin, const std::array<float, 16>& display_world,
    const std::string& prefix, bool anchor_to_guitar_mesh = true);

bool settled_guitar_filter_frame(const std::string& filter_name,
                                 float& frame,
                                 const char*& source) {
  if (filter_name == "guitar_single.filt") {
    // Clean native PCSX2 RAM trace after the Career guitar-select screen settled:
    // engine/out/menu_tuning/pcsx2/
    //   silo_guitar_select_runtime_attachment_trace_retry_20260704/
    //   guitar_select_logic_trace.json
    frame = 32.623825f;
    source = "pcsx2-settled-career-guitar-select";
    return true;
  }
  return false;
}

bool append_guitar_model_at_world(
    const std::string& hdr, const std::string& ark, const ConfigDb& db,
    milo_scene::Scene& combined,
    std::map<std::string, asset::Image>& textures, const std::string& guitar,
    const std::string& skin, const milo_scene::Xfm& display_xfm,
    const std::string& prefix) {
  return append_guitar_model_at_matrix(hdr, ark, db, combined, textures, guitar,
                                       skin, menu_xfm_to_mat4(display_xfm),
                                       prefix);
}

bool append_guitar_model_at_matrix(
    const std::string& hdr, const std::string& ark, const ConfigDb& db,
    milo_scene::Scene& combined,
    std::map<std::string, asset::Image>& textures, const std::string& guitar,
    const std::string& skin, const std::array<float, 16>& display_world,
    const std::string& prefix, bool anchor_to_guitar_mesh) {
  const std::string guitar_path = guitar_milo_path_for(db, guitar, skin);
  if (guitar_path.empty()) return false;
  if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
    std::fprintf(stderr,
                 "[menu] guitar model guitars.dtb guitar=%s skin=%s path=%s\n",
                 guitar.c_str(), skin.c_str(), guitar_path.c_str());
  }
  milo_scene::Scene guitar_scene;
  if (!milo_scene::load_scene(hdr, ark, guitar_path, guitar_scene)) return false;

  std::unordered_set<std::string> want;
  for (const auto& mat : guitar_scene.mats)
    if (!mat.diffuse_tex.empty()) want.insert(mat.diffuse_tex);
  auto imgs = asset::load_milo_textures(
      hdr, ark, guitar_path, std::vector<std::string>(want.begin(), want.end()));
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));

  bake_scene_to_world(
      guitar_scene, display_world, prefix,
      anchor_to_guitar_mesh ? menu_scene_anchor_inverse(guitar_scene, "guitar.mesh")
                            : menu_mat4_identity());
  milo_scene::GroupObj env_group;
  env_group.name = prefix + "guitar_display_env.grp";
  env_group.environment_ref = "guitar_setup.env";
  for (auto& mesh : guitar_scene.meshes) {
    if (mesh.name == prefix + "shadow_guitar.mesh") continue;
    env_group.children.push_back(mesh.name);
    combined.meshes.push_back(std::move(mesh));
  }
  if (!env_group.children.empty()) combined.groups.push_back(std::move(env_group));
  for (auto& mat : guitar_scene.mats) combined.mats.push_back(std::move(mat));
  for (auto& name : guitar_scene.draw_order) {
    if (name == prefix + "shadow_guitar.mesh") continue;
    combined.draw_order.push_back(std::move(name));
  }
  return true;
}

void append_runtime_guitar_displays(const std::string& hdr,
                                     const std::string& ark,
                                     const ConfigDb& db,
                                     ScreenManager& mgr, Object* screen,
                                     milo_scene::Scene& combined,
                                     std::map<std::string, asset::Image>& textures) {
  if (!menu_guitar_display_enabled()) return;
  auto visible_panel = [&](const char* name) -> Object* {
    Object* panel = mgr.find_object(Symbol(name));
    if (!screen_has_panel(screen, Symbol(name)) || !panel_active_for_render(panel))
      return nullptr;
    if (!runtime_object_showing(mgr, name)) return nullptr;
    return panel;
  };
  auto screen_milo_owning_entry =
      [&](const char* type, const std::string& entry_name,
          Object** owner_panel) -> std::string {
    if (owner_panel) *owner_panel = nullptr;
    if (!type || entry_name.empty()) return {};
    for (Symbol pn : screen_panel_names(screen)) {
      Object* candidate = mgr.find_object(pn);
      if (!panel_active_for_render(candidate)) continue;
      const std::string path = panel_milo_path(panel_file(candidate));
      if (path.empty()) continue;
      if (!load_milo_entry_body(hdr, ark, path, type, entry_name).empty()) {
        if (owner_panel) *owner_panel = candidate;
        return path;
      }
    }
    return {};
  };
  auto append_scripted_panel = [&](Object* panel, int player,
                                   const std::string& prefix) -> bool {
    if (!panel) return false;
    const std::string guitar =
        data_node_symbol_text(panel->get_property(menu_indexed_symbol("guitar", player)));
    if (guitar.empty()) return false;
    const std::string skin =
        data_node_symbol_text(panel->get_property(menu_indexed_symbol("guitar_skin", player)));
    const std::string actual_proxy = data_node_symbol_text(
        panel->get_property(menu_indexed_symbol("guitar_proxy", player)));
    const std::string actual_filter = data_node_symbol_text(
        panel->get_property(menu_indexed_symbol("guitar_filter", player)));
    if (actual_proxy.empty() || actual_filter.empty()) return false;

    Object* proxy_owner = nullptr;
    const std::string proxy_milo =
        screen_milo_owning_entry("UIProxy", actual_proxy, &proxy_owner);
    Object* filter_owner = nullptr;
    const std::string filter_milo =
        screen_milo_owning_entry("AnimFilter", actual_filter, &filter_owner);
    if (proxy_milo.empty() || filter_milo.empty()) return false;
    if (!proxy_owner) proxy_owner = filter_owner ? filter_owner : panel;

    float frame = -1.0f;
    const char* frame_source = nullptr;
    settled_guitar_filter_frame(actual_filter, frame, frame_source);
    append_guitar_display_scene(
        hdr, ark, db, combined, textures, proxy_owner, guitar, skin,
        proxy_milo, actual_proxy, filter_milo, actual_filter, "", "", prefix,
        frame, frame_source, true);
    return true;
  };
  auto append_scripted_display_panel = [&](const char* panel_name,
                                           int players) -> bool {
    Object* panel = visible_panel(panel_name);
    if (!panel) return false;
    bool appended = false;
    for (int player = 0; player < players; ++player) {
      std::string prefix = "__";
      prefix += panel_name;
      prefix += "_p";
      prefix += std::to_string(player);
      prefix += "_";
      appended = append_scripted_panel(panel, player, prefix) || appended;
    }
    return appended;
  };

  // Retail GuitarDisplayPanel draws selector guitars with the display MILO
  // display-entry data:
  //   entry = {guitar.pxy, loaded guitar dir, loaded flag, guitar_single.filt,
  //            240.0f, guitar.env}
  // Current PCSX2 code trace proves show_guitar stores the script proxy/filter,
  // initializes the filter at frame 0.0, stores a 240-frame loop length, then
  // advances it at runtime. Prefer the proxy/filter names captured from the
  // stock DTB call and find the owning screen MILO instead of hard-wiring the
  // selector/unlock MILO filenames.
  append_scripted_display_panel("guitar_display_panel", 1);
  append_scripted_display_panel("multi_guitar_display_panel", 2);
  append_scripted_display_panel("unlock_guitar_display_panel", 1);
}

bool menu_transform_anim_empty(
    const ghogx::render::MiloSceneRenderer::MeshTransformAnim& anim) {
  return anim.translation_keys.size() < 2 && anim.rotation_keys.size() < 2 &&
         anim.scale_keys.size() < 2;
}

std::vector<std::string> milo_entry_names_by_type(const std::string& hdr,
                                                  const std::string& ark,
                                                  const std::string& milo_path,
                                                  const std::string& type) {
  std::vector<std::string> names;
  try {
    auto arkr = gh::ark::ArkV3Reader::load(hdr);
    auto entry = arkr.find(milo_path);
    if (!entry) entry = arkr.find("../../system/run/" + milo_path);
    if (!entry) return names;
    auto bytes = arkr.read_entry(*entry, {ark});
    auto mh = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, mh);
    auto dir = gh::milo::parse_directory(payload);
    for (const auto& e : dir.entries) {
      if (e.type == type) names.push_back(e.name);
    }
  } catch (const std::exception&) {
  }
  return names;
}

std::string first_menu_ref_with_suffixes(const std::vector<uint8_t>& body,
                                         std::initializer_list<const char*> suffixes) {
  for (const auto& s : menu_strings_with_offsets(body)) {
    for (const char* suffix : suffixes) {
      if (menu_ref_has_suffix(s.value, suffix)) return s.value;
    }
  }
  return {};
}

void collect_animate_forever_roots(const gh::dtb::Node& node,
                                   std::vector<std::string>& roots,
                                   std::unordered_set<std::string>& seen) {
  if (node.tag == 0x11) {
    const auto& kids = gh::dtb::children(node);
    if (!kids.empty()) {
      const std::string head = gh::dtb::as_string(*kids[0]).value_or("");
      if (head == "animate_forever_30fps") {
        for (std::size_t i = 1; i < kids.size(); ++i) {
          const std::string ref = gh::dtb::as_string(*kids[i]).value_or("");
          if ((menu_ref_has_suffix(ref, ".grp") ||
               menu_ref_has_suffix(ref, ".mesh") ||
               menu_ref_has_suffix(ref, ".view")) &&
              seen.insert(ref).second) {
            roots.push_back(ref);
          }
        }
      }
    }
  }
  for (const auto& child : gh::dtb::children(node)) {
    collect_animate_forever_roots(*child, roots, seen);
  }
}

std::vector<std::string> panel_anim_loop_roots_from_dtb(
    const std::string& hdr, const std::string& ark,
    const std::string& panel_file_name) {
  std::vector<std::string> roots;
  if (panel_file_name.empty()) return roots;
  std::string dtb_name = panel_file_name;
  std::replace(dtb_name.begin(), dtb_name.end(), '\\', '/');
  const size_t slash = dtb_name.find_last_of('/');
  if (slash != std::string::npos) dtb_name.erase(0, slash + 1);
  const std::string milo_suffix = ".milo";
  if (dtb_name.size() <= milo_suffix.size() ||
      dtb_name.compare(dtb_name.size() - milo_suffix.size(),
                       milo_suffix.size(), milo_suffix) != 0) {
    return roots;
  }
  dtb_name.replace(dtb_name.size() - milo_suffix.size(),
                   milo_suffix.size(), ".dtb");
  const std::string dtb_path = "ui/gen/" + dtb_name;
  try {
    auto arkr = gh::ark::ArkV3Reader::load(hdr);
    auto entry = arkr.find(dtb_path);
    if (!entry) entry = arkr.find("../../system/run/" + dtb_path);
    if (!entry) return roots;
    auto bytes = arkr.read_entry(*entry, {ark});
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    std::unordered_set<std::string> seen;
    for (const auto& root : tree.root) {
      collect_animate_forever_roots(*root, roots, seen);
    }
  } catch (const std::exception&) {
  }
  return roots;
}

void collect_scene_ref_descendants(const milo_scene::Scene& scene,
                                   const std::string& ref,
                                   std::unordered_set<std::string>& out,
                                   std::unordered_set<std::string>& visiting) {
  if (ref.empty() || !visiting.insert(ref).second) return;
  out.insert(ref);
  for (const auto& group : scene.groups) {
    if (group.name != ref) continue;
    for (const std::string& child : group.children) {
      out.insert(child);
      if (menu_ref_has_suffix(child, ".grp") ||
          menu_ref_has_suffix(child, ".view")) {
        collect_scene_ref_descendants(scene, child, out, visiting);
      }
    }
    break;
  }
  visiting.erase(ref);
}

bool menu_material_has_active_mesh(
    const milo_scene::Scene& scene,
    const std::unordered_set<std::string>& active_refs,
    const std::string& material) {
  if (material.empty()) return false;
  for (const auto& mesh : scene.meshes) {
    if (mesh.material == material && active_refs.find(mesh.name) != active_refs.end())
      return true;
  }
  return false;
}

void install_panel_anim_loops_from_stock_data(
    const std::string& hdr, const std::string& ark,
    const std::string& panel_file_name,
    ghogx::render::MiloSceneRenderer& renderer,
    MenuMaterialAnimPlayer* material_anims) {
  const std::vector<std::string> roots =
      panel_anim_loop_roots_from_dtb(hdr, ark, panel_file_name);
  if (roots.empty()) return;
  const std::string milo_path = panel_milo_path(panel_file_name);
  milo_scene::Scene scene;
  if (!milo_scene::load_scene(hdr, ark, milo_path, scene)) return;

  std::unordered_set<std::string> active_refs;
  for (const std::string& root : roots) {
    std::unordered_set<std::string> visiting;
    collect_scene_ref_descendants(scene, root, active_refs, visiting);
  }

  const bool log = std::getenv("GHOGX_LOG_MENU_ANIMS") != nullptr;
  const std::map<std::string, MenuMaterialAnim> matanim_by_name =
      material_anims ? load_menu_matanims(hdr, ark, milo_path)
                     : std::map<std::string, MenuMaterialAnim>{};
  for (const std::string& filter_name :
       milo_entry_names_by_type(hdr, ark, milo_path, "AnimFilter")) {
    const auto filter_body =
        load_milo_entry_body(hdr, ark, milo_path, "AnimFilter", filter_name);
    const std::string transanim_name =
        first_menu_ref_with_suffixes(filter_body, {".tnm"});
    if (!transanim_name.empty()) {
      const auto anim_body =
          load_milo_entry_body(hdr, ark, milo_path, "TransAnim", transanim_name);
      if (!anim_body.empty()) {
        const std::string target =
            first_menu_ref_with_suffixes(anim_body, {".mesh", ".grp", ".view"});
        if (!target.empty() && active_refs.find(target) != active_refs.end()) {
          auto anim = decode_menu_transanim_transform_anim(anim_body);
          if (!menu_transform_anim_empty(anim)) {
            if (log) {
              std::fprintf(stderr,
                           "[menu-anim] loop panel=%s root_count=%zu filter=%s "
                           "trans=%s target=%s fps=30\n",
                           panel_file_name.c_str(), roots.size(),
                           filter_name.c_str(), transanim_name.c_str(),
                           target.c_str());
            }
            renderer.trigger_mesh_transform_anim(target, std::move(anim), 30.0f,
                                                 /*loop=*/true);
          }
        }
      }
    }

    if (!material_anims) continue;
    const std::string matanim_name =
        first_menu_ref_with_suffixes(filter_body, {".mnm"});
    if (matanim_name.empty()) continue;
    auto matanim_it = matanim_by_name.find(matanim_name);
    if (matanim_it == matanim_by_name.end()) continue;
    const MenuMaterialAnim& matanim = matanim_it->second;
    const bool active =
        active_refs.find(filter_name) != active_refs.end() ||
        menu_material_has_active_mesh(scene, active_refs, matanim.material);
    if (!active) continue;
    if (log) {
      std::fprintf(stderr,
                   "[menu-anim] mat-loop panel=%s root_count=%zu filter=%s "
                   "matanim=%s material=%s texture_keys=%zu color_keys=%zu "
                   "alpha_keys=%zu frames=%.1f fps=30\n",
                   panel_file_name.c_str(), roots.size(), filter_name.c_str(),
                   matanim.name.c_str(), matanim.material.c_str(),
                   matanim.texture_keys.size(), matanim.color_keys.size(),
                   matanim.alpha_keys.size(), matanim.duration_frames);
    }
    material_anims->add_loop(matanim);
  }
}

struct MenuUiTrigger {
  std::string name;
  std::string event;
  std::string anim_filter;
  bool block_transition = false;
};

MenuUiTrigger decode_menu_ui_trigger(const std::string& name,
                                     const std::vector<uint8_t>& body) {
  MenuUiTrigger trigger;
  trigger.name = name;
  const std::vector<MenuStringHit> strings = menu_strings_with_offsets(body);
  for (const MenuStringHit& s : strings) {
    if (trigger.event.empty()) {
      trigger.event = s.value;
      continue;
    }
    if (menu_ref_has_suffix(s.value, ".filt")) {
      trigger.anim_filter = s.value;
      break;
    }
  }
  if (!body.empty()) trigger.block_transition = body.back() != 0;
  return trigger;
}

bool trigger_panel_anim_filter_from_stock_data(
    const std::string& hdr, const std::string& ark,
    const std::string& milo_path, const std::string& panel_file_name,
    const std::string& filter_name,
    ghogx::render::MiloSceneRenderer& renderer,
    MenuMaterialAnimPlayer* material_anims) {
  const bool log = std::getenv("GHOGX_LOG_MENU_ANIMS") != nullptr;
  const auto filter_body =
      load_milo_entry_body(hdr, ark, milo_path, "AnimFilter", filter_name);
  if (filter_body.empty()) return false;
  bool fired = false;
  const std::string transanim_name =
      first_menu_ref_with_suffixes(filter_body, {".tnm"});
  if (!transanim_name.empty()) {
    const auto anim_body =
        load_milo_entry_body(hdr, ark, milo_path, "TransAnim", transanim_name);
    if (!anim_body.empty()) {
      const std::string target =
          first_menu_ref_with_suffixes(anim_body, {".mesh", ".grp", ".view"});
      if (!target.empty()) {
        auto anim = decode_menu_transanim_transform_anim(anim_body);
        if (!menu_transform_anim_empty(anim)) {
          if (log) {
            std::fprintf(stderr,
                         "[menu-trigger] fire panel=%s filter=%s trans=%s "
                         "target=%s fps=30\n",
                         panel_file_name.c_str(), filter_name.c_str(),
                         transanim_name.c_str(), target.c_str());
          }
          renderer.trigger_mesh_transform_anim(target, std::move(anim), 30.0f,
                                               /*loop=*/false);
          fired = true;
        }
      }
    }
  } else if (log) {
    std::fprintf(stderr,
                 "[menu-trigger] skip panel=%s filter=%s no TransAnim ref\n",
                 panel_file_name.c_str(), filter_name.c_str());
  }

  if (material_anims) {
    const std::string matanim_name =
        first_menu_ref_with_suffixes(filter_body, {".mnm"});
    if (!matanim_name.empty()) {
      auto matanims = load_menu_matanims(hdr, ark, milo_path);
      auto it = matanims.find(matanim_name);
      if (it != matanims.end()) {
        if (log) {
          std::fprintf(stderr,
                       "[menu-trigger] fire panel=%s filter=%s matanim=%s "
                       "material=%s frames=%.1f fps=30\n",
                       panel_file_name.c_str(), filter_name.c_str(),
                       it->second.name.c_str(), it->second.material.c_str(),
                       it->second.duration_frames);
        }
        material_anims->trigger_one_shot(std::move(it->second));
        fired = true;
      }
    } else if (log) {
      std::fprintf(stderr,
                   "[menu-trigger] skip panel=%s filter=%s no MatAnim ref\n",
                   panel_file_name.c_str(), filter_name.c_str());
    }
  }

  if (!fired && log) {
    std::fprintf(stderr,
                 "[menu-trigger] skip panel=%s filter=%s no playable anim\n",
                 panel_file_name.c_str(), filter_name.c_str());
  }
  return fired;
}

void fire_panel_ui_triggers_from_stock_data(
    const std::string& hdr, const std::string& ark,
    const std::string& panel_file_name, const std::string& event,
    ghogx::render::MiloSceneRenderer& renderer,
    MenuMaterialAnimPlayer* material_anims) {
  const std::string milo_path = panel_milo_path(panel_file_name);
  if (milo_path.empty()) return;
  const bool log = std::getenv("GHOGX_LOG_MENU_ANIMS") != nullptr;
  for (const std::string& trigger_name :
       milo_entry_names_by_type(hdr, ark, milo_path, "UITrigger")) {
    const auto trigger_body =
        load_milo_entry_body(hdr, ark, milo_path, "UITrigger", trigger_name);
    const MenuUiTrigger trigger =
        decode_menu_ui_trigger(trigger_name, trigger_body);
    if (trigger.event != event) continue;
    if (trigger.anim_filter.empty()) {
      if (log) {
        std::fprintf(stderr,
                     "[menu-trigger] skip panel=%s trigger=%s event=%s no "
                     "AnimFilter ref\n",
                     panel_file_name.c_str(), trigger.name.c_str(),
                     event.c_str());
      }
      continue;
    }
    if (log) {
      std::fprintf(stderr,
                   "[menu-trigger] event panel=%s trigger=%s event=%s "
                   "filter=%s block=%d\n",
                   panel_file_name.c_str(), trigger.name.c_str(),
                   event.c_str(), trigger.anim_filter.c_str(),
                   trigger.block_transition ? 1 : 0);
    }
    trigger_panel_anim_filter_from_stock_data(
        hdr, ark, milo_path, panel_file_name, trigger.anim_filter, renderer,
        material_anims);
  }
}

void fire_screen_ui_triggers_from_stock_data(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen, const std::string& event,
    ghogx::render::MiloSceneRenderer& renderer,
    MenuMaterialAnimPlayer* material_anims) {
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_active_for_render(panel)) continue;
    const std::string file = panel_file(panel);
    if (file.empty()) continue;
    fire_panel_ui_triggers_from_stock_data(hdr, ark, file, event, renderer,
                                           material_anims);
  }
}

bool rebuild_guitar_overlay_scene(
    const std::string& hdr, const std::string& ark, const ConfigDb& db,
    ScreenManager& mgr, Object* screen,
    ghogx::render::MiloSceneRenderer& renderer) {
  if (!menu_guitar_display_enabled()) return false;
  milo_scene::Scene display_scene;
  if (!milo_scene::load_scene(hdr, ark, "ui/gen/guitar_display.milo_ps2",
                              display_scene)) {
    return false;
  }

  milo_scene::Scene combined;
  combined.cams = display_scene.cams;
  combined.lights = display_scene.lights;
  combined.environs = display_scene.environs;
  std::map<std::string, asset::Image> textures;

  auto visible_panel = [&](const char* name) -> Object* {
    Object* panel = mgr.find_object(Symbol(name));
    if (!screen_has_panel(screen, Symbol(name)) || !panel_active_for_render(panel))
      return nullptr;
    if (!runtime_object_showing(mgr, name)) return nullptr;
    return panel;
  };
  auto screen_milo_owning_entry =
      [&](const char* type, const std::string& entry_name,
          Object** owner_panel) -> std::string {
    if (owner_panel) *owner_panel = nullptr;
    if (!type || entry_name.empty()) return {};
    for (Symbol pn : screen_panel_names(screen)) {
      Object* candidate = mgr.find_object(pn);
      if (!panel_active_for_render(candidate)) continue;
      const std::string path = panel_milo_path(panel_file(candidate));
      if (path.empty()) continue;
      if (!load_milo_entry_body(hdr, ark, path, type, entry_name).empty()) {
        if (owner_panel) *owner_panel = candidate;
        return path;
      }
    }
    return {};
  };
  auto append_from_panel = [&](Object* panel, int player,
                               const char* filter_milo,
                               const char* display_filter,
                               const char* display_group,
                               const char* display_placer,
                               const char* prefix,
                               float filter_frame_override = -1.0f,
                               const char* filter_frame_source = nullptr,
                               bool filter_absolute_sample = false,
                               bool prefer_panel_filter = true) {
    if (!panel) return false;
    const std::string guitar =
        data_node_symbol_text(panel->get_property(menu_indexed_symbol("guitar", player)));
    const std::string skin =
        data_node_symbol_text(panel->get_property(menu_indexed_symbol("guitar_skin", player)));
    const std::string actual_proxy = data_node_symbol_text(
        panel->get_property(menu_indexed_symbol("guitar_proxy", player)));
    std::string actual_filter;
    if (prefer_panel_filter) {
      actual_filter = data_node_symbol_text(
          panel->get_property(menu_indexed_symbol("guitar_filter", player)));
    }
    if (actual_filter.empty()) actual_filter = display_filter ? display_filter : "";

    Object* proxy_owner = nullptr;
    const std::string proxy_milo =
        screen_milo_owning_entry("UIProxy", actual_proxy, &proxy_owner);
    if (!proxy_milo.empty()) return false;

    if (display_filter && display_filter[0]) actual_filter = display_filter;
    milo_scene::Xfm display_xfm;
    if (!(load_guitar_display_placer_world(hdr, ark,
                                           display_placer ? display_placer : "",
                                           display_xfm) ||
          load_guitar_display_group_world(hdr, ark,
                                          display_group ? display_group : "",
                                          display_xfm))) {
      return false;
    }
    std::array<float, 16> display_world = menu_xfm_to_mat4(display_xfm);
    ghogx::render::MiloSceneRenderer::MeshTransformSample sample;
    if (load_guitar_filter_sample(hdr, ark,
                                  filter_milo ? filter_milo
                                              : "ui/gen/guitar_display.milo_ps2",
                                  actual_filter, sample,
                                  filter_frame_override,
                                  filter_frame_source,
                                  filter_absolute_sample)) {
      apply_menu_transform_sample(display_world, sample);
    }
    return append_guitar_model_at_matrix(hdr, ark, db, combined, textures,
                                         guitar, skin, display_world, prefix,
                                         /*anchor_to_guitar_mesh=*/false);
  };

  bool any = false;
  // Selector guitars are composed in the flat menu scene through their screen
  // UIProxy entries. Keep them out of the display-MILO overlay.

  // Store panels use the stock display MILO camera/lights, but the stock
  // scripts may pass screen- or item-owned proxy/filter objects to show_guitar.
  // Prefer those live objects when the owning MILO is present on the current
  // screen; fall back to the generic display-MILO placers otherwise.
  any = append_from_panel(visible_panel("store_guitar_display_panel"), 0,
                          "ui/gen/guitar_display.milo_ps2",
                          "guitar_store.filt",
                          "guitar_store.view", "guitar_store.placer",
                          "__guitar_store_",
                          -1.0f, nullptr, false,
                          true) ||
        any;
  any = append_from_panel(visible_panel("unlock_guitar_display_panel"), 0,
                          "ui/gen/guitar_display.milo_ps2",
                          "guitar_axe.filt",
                          "guitar_axe.view", "guitar_axe.placer",
                          "__guitar_unlock_",
                          -1.0f, nullptr, false,
                          true) ||
        any;
  if (!any) return false;

  std::fprintf(stderr, "[menu] guitar overlay: %zu meshes, %zu textures\n",
               combined.meshes.size(), textures.size());
  renderer.set_scene(std::move(combined), textures);
  renderer.set_text_batches({});
  renderer.set_post_text_meshes({});
  return true;
}

struct RuntimeGroupVisibility {
  std::unordered_map<std::string, std::vector<std::string>> owners;
  std::unordered_map<std::string, std::string> parents;
};

bool runtime_object_showing(ScreenManager& mgr, const std::string& name) {
  if (name.empty()) return true;
  Object* obj = mgr.resolve_object(Symbol(name.c_str()));
  if (!obj) return true;
  const DataNode showing = obj->get_property(Symbol("showing"));
  return showing.empty() || menu_truthy(showing);
}

bool runtime_group_visible(ScreenManager& mgr,
                           const RuntimeGroupVisibility& visibility,
                           const std::string& group,
                           std::unordered_set<std::string>& visiting) {
  if (group.empty()) return true;
  if (!visiting.insert(group).second) return true;
  if (!runtime_object_showing(mgr, group)) return false;
  auto it = visibility.parents.find(group);
  if (it != visibility.parents.end() && !it->second.empty() &&
      visibility.parents.find(it->second) != visibility.parents.end()) {
    if (!runtime_group_visible(mgr, visibility, it->second, visiting))
      return false;
  }
  auto owner_it = visibility.owners.find(group);
  if (owner_it != visibility.owners.end() && !owner_it->second.empty()) {
    for (const std::string& owner : owner_it->second) {
      if (runtime_group_visible(mgr, visibility, owner, visiting)) return true;
    }
    return false;
  }
  return true;
}

bool runtime_owners_showing(ScreenManager& mgr,
                            const RuntimeGroupVisibility& visibility,
                            const std::string& object_name) {
  auto it = visibility.owners.find(object_name);
  if (it == visibility.owners.end() || it->second.empty()) return true;
  for (const std::string& owner : it->second) {
    std::unordered_set<std::string> visiting;
    if (runtime_group_visible(mgr, visibility, owner, visiting)) return true;
  }
  return false;
}

void apply_runtime_scene_visibility(ScreenManager& mgr,
                                    milo_scene::Scene& scene) {
  RuntimeGroupVisibility visibility;
  for (const auto& group : scene.groups) {
    visibility.parents[group.name] = group.parent;
    for (const auto& child : group.children) {
      if (!child.empty()) visibility.owners[child].push_back(group.name);
    }
  }
  for (auto& mesh : scene.meshes) {
    if (!runtime_owners_showing(mgr, visibility, mesh.name) ||
        !runtime_object_showing(mgr, mesh.parent)) {
      mesh.showing = false;
    }
  }
  for (auto& particle : scene.particles) {
    if (!runtime_owners_showing(mgr, visibility, particle.name) ||
        !runtime_object_showing(mgr, particle.parent)) {
      particle.showing = false;
    }
  }
}

// The panel names listed in a screen's (panels ...) property.
std::vector<Symbol> screen_panel_names(Object* screen) {
  std::vector<Symbol> out;
  if (!screen) return out;
  DataNode p = screen->get_property(Symbol("panels"));
  if (auto arr = p.as_array()) {
    for (std::size_t i = 0; i < arr->size(); ++i)
      if (auto s = arr->at(i).as_symbol()) out.push_back(*s);
  } else if (auto s = p.as_symbol()) {
    out.push_back(*s);
  }
  return out;
}

std::string panel_file(Object* panel) {
  if (!panel) return {};
  DataNode f = panel->get_property(Symbol("file"));
  if (auto str = f.as_string()) return std::string(str->data(), str->size());
  if (!f.as_symbol()) f = panel->handle_property(Symbol("file"), DataArray());
  if (auto str = f.as_string()) return std::string(str->data(), str->size());
  if (auto sym = f.as_symbol()) return std::string(sym->c_str());
  return {};
}

bool panel_active_for_render(Object* panel) {
  if (!panel) return false;
  const DataNode active = panel->get_property(Symbol("active"));
  if (!active.empty() && !menu_truthy(active)) return false;
  const DataNode showing = panel->get_property(Symbol("showing"));
  return showing.empty() || menu_truthy(showing);
}

std::string panel_milo_path(const std::string& file) {
  if (file.empty()) return {};
  auto starts = [](const std::string& s, const char* prefix) {
    const std::string p(prefix);
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
  };
  auto with_ps2_suffix = [](std::string path) {
    if (path.size() >= 5 &&
        path.compare(path.size() - 5, 5, ".milo") == 0) {
      path += "_ps2";
    }
    return path;
  };
  std::vector<std::string> parts;
  std::string token;
  auto push = [&](const std::string& part) {
    if (part.empty() || part == ".") return;
    if (part == "..") {
      if (!parts.empty()) parts.pop_back();
      return;
    }
    parts.push_back(part);
  };
  std::string raw;
  if (starts(file, "../world/")) {
    raw = file.substr(3);
    const size_t slash = raw.find_last_of("/\\");
    if (slash != std::string::npos &&
        raw.find("/gen/") == std::string::npos &&
        raw.find("\\gen\\") == std::string::npos) {
      raw.insert(slash + 1, "gen/");
    }
    raw = with_ps2_suffix(raw);
  } else if (starts(file, "../hud/")) {
    raw = "hud/gen/" + file.substr(7);
    raw = with_ps2_suffix(raw);
  } else {
    raw = "ui/gen/" + with_ps2_suffix(file);
  }
  for (char ch : raw) {
    if (ch == '/' || ch == '\\') {
      push(token);
      token.clear();
    } else {
      token.push_back(ch);
    }
  }
  push(token);
  std::string out;
  for (const std::string& part : parts) {
    if (!out.empty()) out += "/";
    out += part;
  }
  return out;
}

// Append a panel's MILO (its (file) value, e.g. "main.milo" -> ui/gen/main.milo_ps2)
// into the combined scene + texture set the renderer draws.
void add_panel_milo(const std::string& hdr, const std::string& ark,
                    ScreenManager& mgr, Object* panel,
                    const std::string& file,
                    milo_scene::Scene& combined,
                    std::map<std::string, asset::Image>& textures) {
  if (file.empty()) return;
  // The helpbar panel is rebuilt from its authored tokens/textures in the overlay
  // footer; drawing its raw MILO meshes leaves the source icons at scene origin.
  if (file == "helpbar.milo") return;
  const std::string path = panel_milo_path(file);
  milo_scene::Scene s;
  if (!milo_scene::load_scene(hdr, ark, path, s)) return;
  apply_panel_static_transanim_frames(hdr, ark, panel, path, s);
  apply_setlist_paper_scroll(mgr, hdr, ark, path, s);
  apply_runtime_scene_visibility(mgr, s);

  // Collect the diffuse-texture names BEFORE moving the mats out (otherwise the
  // moved-from strings are empty and nothing loads).
  std::unordered_set<std::string> want;
  for (const auto& m : s.mats)
    if (!m.diffuse_tex.empty()) want.insert(m.diffuse_tex);
  for (const std::string& tex : menu_matanim_texture_refs(hdr, ark, path))
    want.insert(tex);

  // Panel draw order is the stock MILO's mesh surface. Decoded support meshes
  // that are not reachable from authored groups stay available to object lookups
  // but should not be imported into the rendered menu scene.
  std::unordered_set<std::string> authored_draw_meshes;
  for (const std::string& name : s.draw_order)
    authored_draw_meshes.insert(name);

  for (auto& m : s.meshes) {
    if ((!authored_draw_meshes.empty() &&
         authored_draw_meshes.find(m.name) == authored_draw_meshes.end()) ||
        skip_runtime_animated_bind_mesh(file, m.name))
      continue;
    combined.meshes.push_back(std::move(m));
  }
  for (auto& mt : s.mats) combined.mats.push_back(std::move(mt));
  for (auto& tr : s.transes) combined.transes.push_back(std::move(tr));
  for (auto& c : s.cams) combined.cams.push_back(std::move(c));
  for (auto& g : s.groups) combined.groups.push_back(std::move(g));
  for (auto& name : s.draw_order) combined.draw_order.push_back(std::move(name));

  std::vector<std::string> names(want.begin(), want.end());
  auto imgs = asset::load_milo_textures_from_sources(
      hdr, ark, panel_texture_sources(hdr, ark, path, want), names);
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));
}

std::shared_ptr<DataArray> find_keyed_recursive(
    const std::shared_ptr<DataArray>& arr, Symbol key) {
  if (!arr) return nullptr;
  if (auto direct = arr->find_keyed(key)) return direct;
  for (std::size_t i = 0; i < arr->size(); ++i) {
    auto child = arr->at(i).as_array();
    if (!child) continue;
    if (auto nested = find_keyed_recursive(child, key)) return nested;
  }
  return nullptr;
}

const std::unordered_map<std::string, std::string>& ui_list_resource_milos(
    const std::string& hdr, const std::string& ark) {
  static std::unordered_map<std::string,
                            std::unordered_map<std::string, std::string>> cache;
  const std::string key = hdr + "\n" + ark;
  auto cached = cache.find(key);
  if (cached != cache.end()) return cached->second;

  std::unordered_map<std::string, std::string> resources;
  try {
    auto arkr = gh::ark::ArkV3Reader::load(hdr);
    auto entry = arkr.find("ui/gen/ui_objects.dtb");
    if (!entry) entry = arkr.find("../../system/run/ui/gen/ui_objects.dtb");
    if (entry) {
      gh::dtb::Tree tree = gh::dtb::parse(arkr.read_entry(*entry, {ark}));
      std::shared_ptr<DataArray> root = dtb_bridge::from_tree(tree);
      for (std::size_t i = 0; root && i < root->size(); ++i) {
        auto block = root->at(i).as_array();
        if (!block || block->size() < 2) continue;
        auto head = block->at(0).as_symbol();
        if (!head || *head != Symbol("UIList")) continue;
        auto types = find_keyed_recursive(block, Symbol("types"));
        if (!types) continue;
        for (std::size_t t = 1; t < types->size(); ++t) {
          auto record = types->at(t).as_array();
          if (!record || record->size() < 2) continue;
          auto provider = record->at(0).as_string();
          if (!provider || provider->empty()) continue;
          auto resource_file = record->find_keyed(Symbol("resource_file"));
          if (!resource_file || resource_file->size() < 2) continue;
          auto file = resource_file->at(1).as_string();
          if (!file || file->empty()) continue;
          resources[std::string(*provider)] =
              panel_milo_path(std::string(*file));
        }
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[menu] ui_objects UIList resources: %s\n", ex.what());
  }

  auto [it, inserted] = cache.emplace(key, std::move(resources));
  return it->second;
}

std::string ui_list_resource_milo_path(const std::string& hdr,
                                       const std::string& ark,
                                       const std::string& provider) {
  if (provider.empty()) return {};
  const auto& resources = ui_list_resource_milos(hdr, ark);
  auto it = resources.find(provider);
  return it == resources.end() ? std::string() : it->second;
}

bool screen_ui_list_layout(const std::string& hdr, const std::string& ark,
                           ScreenManager& mgr, Object* screen,
                           const char* list_name, std::string& milo_path,
                           UiListLayout& out) {
  milo_path.clear();
  out = UiListLayout{};
  if (!screen || !list_name || !*list_name) return false;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_active_for_render(panel)) continue;
    const std::string path = panel_milo_path(panel_file(panel));
    if (path.empty()) continue;
    UiListLayout candidate = extract_ui_list_layout(hdr, ark, path, list_name);
    if (!candidate.valid) continue;
    milo_path = path;
    out = candidate;
    return true;
  }
  return false;
}

struct MenuTextureSourceIndex {
  std::unordered_map<std::string, std::vector<std::string>> sources_by_tex;
};

const MenuTextureSourceIndex& menu_texture_source_index(
    const std::string& hdr, const std::string& ark) {
  static std::unordered_map<std::string, MenuTextureSourceIndex> cache;
  const std::string key = hdr + "\n" + ark;
  auto cached = cache.find(key);
  if (cached != cache.end()) return cached->second;

  MenuTextureSourceIndex index;
  try {
    auto arkr = gh::ark::ArkV3Reader::load(hdr);
    const std::vector<std::string> arks{ark};
    for (const auto& entry : arkr.entries()) {
      if (entry.full_path.rfind("ui/gen/", 0) != 0 ||
          !menu_ref_has_suffix(entry.full_path, ".milo_ps2")) {
        continue;
      }
      try {
        const auto bytes = arkr.read_entry(entry, arks);
        const auto h = gh::milo::parse_header(bytes);
        const auto payload = gh::milo::inflate_payload(bytes, h);
        const auto dir = gh::milo::parse_directory(payload);
        for (const auto& de : dir.entries) {
          if (de.type != "Tex" || de.name.empty()) continue;
          auto& sources = index.sources_by_tex[de.name];
          if (sources.empty() || sources.back() != entry.full_path)
            sources.push_back(entry.full_path);
        }
      } catch (const std::exception&) {
      }
    }
  } catch (const std::exception&) {
  }

  if (std::getenv("GHOGX_LOG_MENU_TEXTURE_SOURCES")) {
    std::fprintf(stderr, "[menu] indexed %zu UI texture names\n",
                 index.sources_by_tex.size());
  }
  auto [it, inserted] = cache.emplace(key, std::move(index));
  (void)inserted;
  return it->second;
}

std::vector<std::string> panel_texture_sources(
    const std::string& hdr,
    const std::string& ark,
    const std::string& panel_path,
    const std::unordered_set<std::string>& wanted_textures) {
  std::vector<std::string> sources;
  std::unordered_set<std::string> seen;
  auto add_source = [&](const std::string& source) {
    if (!source.empty() && seen.insert(source).second) sources.push_back(source);
  };
  add_source(panel_path);

  const MenuTextureSourceIndex& index = menu_texture_source_index(hdr, ark);
  for (const std::string& tex : wanted_textures) {
    auto it = index.sources_by_tex.find(tex);
    if (it == index.sources_by_tex.end()) continue;
    const bool panel_has_tex =
        std::find(it->second.begin(), it->second.end(), panel_path) !=
        it->second.end();
    if (panel_has_tex) continue;
    for (const std::string& source : it->second) add_source(source);
  }

  if (std::getenv("GHOGX_LOG_MENU_TEXTURE_SOURCES") && sources.size() > 1) {
    std::fprintf(stderr, "[menu] texture sources %s", panel_path.c_str());
    for (std::size_t i = 1; i < sources.size(); ++i)
      std::fprintf(stderr, " + %s", sources[i].c_str());
    std::fprintf(stderr, "\n");
  }
  return sources;
}

bool entry_authored_draw_showing(const std::vector<uint8_t>& body,
                                 bool& showing) {
  for (const MenuStringHit& s : menu_strings_with_offsets(body)) {
    if (!menu_ref_has_suffix(s.value, ".view") &&
        !menu_ref_has_suffix(s.value, ".grp")) {
      continue;
    }
    if (s.end + 5 > body.size()) continue;
    if (menu_u32_or(body, s.end, 0) != 3) continue;
    showing = body[s.end + 4] != 0;
    return true;
  }
  return false;
}

void install_panel_milo_widgets(const gh::ark::ArkV3Reader& arkr,
                                const std::string& hdr,
                                const std::vector<std::string>& arks,
                                ScreenManager& mgr, Object* panel,
                                const std::string& file) {
  if (file.empty()) return;
  auto* dir = dynamic_cast<ObjectDir*>(panel);
  if (!dir) return;
  const std::string path = panel_milo_path(file);
  auto entry = arkr.find(path);
  if (!entry) entry = arkr.find("../../system/run/" + path);
  if (!entry) return;
  try {
    auto bytes = arkr.read_entry(*entry, arks);
    auto h = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, h);
    auto milo_dir = gh::milo::parse_directory(payload);
    std::map<std::string, MenuLabel> labels_by_name;
    for (auto& label : extract_menu_labels(hdr, arks.front(), path))
      labels_by_name[label.name] = std::move(label);
    for (const auto& e : milo_dir.entries) {
      auto label = labels_by_name.find(e.name);
      bool authored_showing = true;
      bool has_authored_showing = false;
      if (e.offset + e.size <= payload.size()) {
        std::vector<uint8_t> body(payload.begin() + e.offset,
                                  payload.begin() + e.offset + e.size);
        has_authored_showing =
            entry_authored_draw_showing(body, authored_showing);
      }
      if (Object* existing = dir->find(Symbol(e.name))) {
        if (label != labels_by_name.end() && label->second.has_showing &&
            existing->get_property(Symbol("showing")).empty()) {
          existing->set_property(
              Symbol("showing"),
              label->second.showing ? DataNode::Sym(Symbol("TRUE"))
                                    : DataNode::Sym(Symbol("FALSE")));
        } else if (has_authored_showing &&
                   existing->get_property(Symbol("showing")).empty()) {
          existing->set_property(
              Symbol("showing"),
              authored_showing ? DataNode::Sym(Symbol("TRUE"))
                               : DataNode::Sym(Symbol("FALSE")));
        }
        continue;
      }
      auto child = std::make_unique<UiObject>(Symbol(e.type.c_str()));
      child->set_name(Symbol(e.name.c_str()));
      child->set_manager(&mgr);
      if (label != labels_by_name.end() && label->second.has_showing) {
        child->set_property(
            Symbol("showing"),
            label->second.showing ? DataNode::Sym(Symbol("TRUE"))
                                  : DataNode::Sym(Symbol("FALSE")));
      } else if (has_authored_showing) {
        child->set_property(
            Symbol("showing"),
            authored_showing ? DataNode::Sym(Symbol("TRUE"))
                             : DataNode::Sym(Symbol("FALSE")));
      }
      dir->add(std::move(child));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[menu] widget install skipped %s: %s\n",
                 path.c_str(), ex.what());
  }
}

void install_milo_widget_objects(const gh::ark::ArkV3Reader& arkr,
                                 const std::string& hdr,
                                 const std::vector<std::string>& arks,
                                 ScreenManager& mgr) {
  ObjectDir& registry = mgr.registry();
  for (std::size_t i = 0; i < registry.size(); ++i) {
    Object* obj = registry.at(i);
    if (!obj) continue;
    const std::string file = panel_file(obj);
    if (file.empty()) continue;
    install_panel_milo_widgets(arkr, hdr, arks, mgr, obj, file);
  }
}

bool screen_has_panel(Object* screen, Symbol panel_name) {
  for (Symbol pn : screen_panel_names(screen)) {
    if (pn == panel_name) return true;
  }
  return false;
}

std::string song_list_milo_path(ScreenManager& mgr) {
  Object* panel = mgr.find_object(Symbol("sel_song_panel"));
  std::string file = panel_file(panel);
  if (file.empty()) file = "sel_song_quickplay.milo";
  return panel_milo_path(file);
}

bool menu_char_is_shadow(const std::string& name) {
  return name.rfind("shadow", 0) == 0;
}

bool menu_char_is_low_lod_duplicate(const std::string& name) {
  return name.find("_lod1") != std::string::npos || name.rfind("lod_", 0) == 0;
}

std::string character_milo_path_for_outfit(std::string outfit) {
  if (outfit.empty()) outfit = "rockabill1";
  return "char/" + outfit + "/og/gen/" + outfit + ".milo_ps2";
}

std::string character_display_placer_fallback(const char* panel_name,
                                              int player) {
  if (std::strcmp(panel_name, "char_single") == 0) return "char_single.placer";
  if (std::strcmp(panel_name, "char_multi") == 0)
    return "char_multi" + std::to_string(player) + ".placer";
  if (std::strcmp(panel_name, "char_store") == 0) return "char_store.placer";
  return {};
}

bool load_current_panel_placer_world(const std::string& hdr,
                                     const std::string& ark,
                                     ScreenManager& mgr, Object* screen,
                                     const std::string& placer_name,
                                     milo_scene::Xfm& out) {
  if (placer_name.empty()) return false;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_active_for_render(panel)) continue;
    const std::string path = panel_milo_path(panel_file(panel));
    if (path.empty()) continue;
    milo_scene::Scene scene;
    if (!milo_scene::load_scene(hdr, ark, path, scene)) continue;
    if (const milo_scene::BandPlacerObj* placer =
            scene.find_band_placer(placer_name)) {
      out = placer->world_stored;
      return true;
    }
  }
  return false;
}

void append_character_display_scene(
    const std::string& hdr, const std::string& ark, milo_scene::Scene& combined,
    std::map<std::string, asset::Image>& textures, const std::string& outfit,
    const milo_scene::Xfm& display_xfm, const std::string& prefix) {
  ghogx::character::Character character;
  const std::string char_path = character_milo_path_for_outfit(outfit);
  if (!ghogx::character::load_character(hdr, ark, char_path, character)) return;

  auto imgs = asset::load_milo_textures(hdr, ark, char_path, character.texture_names());
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));

  for (auto mat : character.mats) {
    mat.name = prefix + mat.name;
    combined.mats.push_back(std::move(mat));
  }

  const std::array<float, 16> display_world = menu_xfm_to_mat4(display_xfm);
  std::vector<std::array<float, 3>> posed_positions;
  std::vector<std::array<float, 3>> posed_normals;
  for (const auto& src : character.meshes) {
    if (!src.decoded || !src.showing || src.verts.empty() || src.indices.empty())
      continue;
    if (menu_char_is_shadow(src.name) || menu_char_is_low_lod_duplicate(src.name))
      continue;

    ghogx::character::skin_to_pose(src, character, posed_positions, posed_normals);
    if (posed_positions.size() != src.verts.size() ||
        posed_normals.size() != src.verts.size()) {
      continue;
    }

    std::array<float, 16> mesh_world =
        src.bone_palette.empty() ? character.mesh_world(src)
                                 : std::array<float, 16>{1, 0, 0, 0,
                                                         0, 1, 0, 0,
                                                         0, 0, 1, 0,
                                                         0, 0, 0, 1};
    const std::array<float, 16> total = menu_mat4_mul(mesh_world, display_world);

    milo_scene::MeshObj mesh;
    mesh.name = prefix + src.name;
    mesh.material = prefix + src.material;
    mesh.showing = true;
    mesh.decoded = true;
    mesh.vertex_count = static_cast<uint32_t>(src.verts.size());
    mesh.face_count = static_cast<uint32_t>(src.indices.size() / 3);
    mesh.indices = src.indices;
    mesh.verts.reserve(src.verts.size());
    mesh.bb_min[0] = mesh.bb_min[1] = mesh.bb_min[2] = 1.0e30f;
    mesh.bb_max[0] = mesh.bb_max[1] = mesh.bb_max[2] = -1.0e30f;
    for (std::size_t i = 0; i < src.verts.size(); ++i) {
      const auto& v = src.verts[i];
      milo_scene::Vertex out_v;
      out_v.px = posed_positions[i][0];
      out_v.py = posed_positions[i][1];
      out_v.pz = posed_positions[i][2];
      out_v.nx = posed_normals[i][0];
      out_v.ny = posed_normals[i][1];
      out_v.nz = posed_normals[i][2];
      transform_menu_point(total, out_v.px, out_v.py, out_v.pz);
      transform_menu_normal(total, out_v.nx, out_v.ny, out_v.nz);
      out_v.r = out_v.g = out_v.b = out_v.a = 1.0f;
      out_v.u = v.u;
      out_v.v = v.v;
      mesh.bb_min[0] = std::min(mesh.bb_min[0], out_v.px);
      mesh.bb_min[1] = std::min(mesh.bb_min[1], out_v.py);
      mesh.bb_min[2] = std::min(mesh.bb_min[2], out_v.pz);
      mesh.bb_max[0] = std::max(mesh.bb_max[0], out_v.px);
      mesh.bb_max[1] = std::max(mesh.bb_max[1], out_v.py);
      mesh.bb_max[2] = std::max(mesh.bb_max[2], out_v.pz);
      mesh.verts.push_back(out_v);
    }
    combined.draw_order.push_back(mesh.name);
    combined.meshes.push_back(std::move(mesh));
  }
}

void append_runtime_character_displays(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen, milo_scene::Scene& combined,
    std::map<std::string, asset::Image>& textures) {
  if (!menu_character_display_enabled()) return;
  auto append_from_panel = [&](const char* panel_name, int player) {
    Object* panel = mgr.find_object(Symbol(panel_name));
    if (!screen_has_panel(screen, Symbol(panel_name)) ||
        !panel_active_for_render(panel)) {
      return;
    }
    const std::string outfit = data_node_symbol_text(
        panel->get_property(menu_indexed_symbol("char", player)));
    const std::string fallback_outfit = data_node_symbol_text(
        panel->get_property(menu_indexed_symbol("char_outfit", player)));
    std::string placer_name = data_node_symbol_text(
        panel->get_property(menu_indexed_symbol("placer", player)));
    if (placer_name.empty())
      placer_name = character_display_placer_fallback(panel_name, player);
    if (!runtime_object_showing(mgr, placer_name)) return;
    milo_scene::Xfm placer;
    if (!load_current_panel_placer_world(hdr, ark, mgr, screen, placer_name,
                                         placer)) {
      return;
    }
    append_character_display_scene(
        hdr, ark, combined, textures, outfit.empty() ? fallback_outfit : outfit,
        placer, "__char_" + std::string(panel_name) + std::to_string(player) + "_");
  };

  append_from_panel("char_single", 0);
  append_from_panel("char_multi", 0);
  append_from_panel("char_multi", 1);
  append_from_panel("char_store", 0);
}

bool menu_character_display_enabled() {
  return !std::getenv("GHOGX_DISABLE_MENU_CHARACTER_DISPLAY");
}

bool menu_guitar_display_enabled() {
  return !std::getenv("GHOGX_DISABLE_MENU_GUITAR_DISPLAY");
}

bool runtime_display_state_enabled() {
  return menu_character_display_enabled() ||
         menu_guitar_display_enabled();
}

std::string runtime_display_state_key(ScreenManager& mgr, Object* screen) {
  if (!runtime_display_state_enabled()) return {};
  std::ostringstream key;
  auto add_panel = [&](const char* name, int players) {
    if (!screen_has_panel(screen, Symbol(name))) return;
    Object* panel = mgr.find_object(Symbol(name));
    if (!panel_active_for_render(panel)) return;
    key << name << "|show=" << data_node_symbol_text(panel->get_property(Symbol("showing")));
    for (int player = 0; player < players; ++player) {
      key << "|p" << player
          << ":char="
          << data_node_symbol_text(panel->get_property(menu_indexed_symbol("char", player)))
          << ":outfit="
          << data_node_symbol_text(
                 panel->get_property(menu_indexed_symbol("char_outfit", player)))
          << ":placer="
          << data_node_symbol_text(panel->get_property(menu_indexed_symbol("placer", player)))
          << ":guitar="
          << data_node_symbol_text(panel->get_property(menu_indexed_symbol("guitar", player)))
          << ":skin="
          << data_node_symbol_text(
                 panel->get_property(menu_indexed_symbol("guitar_skin", player)))
          << ":proxy="
          << data_node_symbol_text(
                 panel->get_property(menu_indexed_symbol("guitar_proxy", player)))
          << ":filter="
          << data_node_symbol_text(
                 panel->get_property(menu_indexed_symbol("guitar_filter", player)));
    }
    key << ";";
  };
  add_panel("char_single", 1);
  add_panel("char_multi", 2);
  add_panel("char_store", 1);
  add_panel("guitar_display_panel", 1);
  add_panel("multi_guitar_display_panel", 2);
  add_panel("store_guitar_display_panel", 1);
  add_panel("unlock_guitar_display_panel", 1);
  return key.str();
}

float menu_scene_origin_z(const milo_scene::Scene& scene) {
  // Quickplay setlist content is authored under ss_setlist.view. Stock PCSX2
  // settled trace and sel_song_quickplay.milo_ps2 both put it at world z=980.
  // Using that as the camera target keeps setlist_top.mesh, ss_song.lst, and the
  // help footer in the same coordinate family.
  for (const auto& group : scene.groups) {
    if (group.name == "ss_setlist.view" && group.has_transform) {
      return group.world_stored.pos[2];
    }
  }
  return 0.0f;
}

// Build the renderer's scene from the current screen's panels' MILOs.
struct MenuElementTuning {
  float x = 0.0f;
  float z = 0.0f;
  float scale = 1.0f;
};

struct MenuLayoutTuning {
  float camera_x = 0.0f;
  float camera_z = 0.0f;
  float camera_distance = 768.0f;
  float camera_fov = 0.602416f;
  float camera_yaw = 0.0f;
  float main_button_text_scale = 0.800f;
  float menu_z_scale = 1.0f;
  float menu_z_offset = 0.0f;
  float menu_center_x = 6.0f;
  // Populated HelpBarPanel anchor, fit from the settled PS2 setlist capture.
  // The MILO supplies the template geometry; runtime set_display positions the
  // populated footer slightly right/up from the raw template origin.
  float footer_x = 0.0f;
  float footer_z = -206.0f;
  float footer_scale = 0.52f;
  float setlist_parent_x = -2.0f;
  // Residual PS2 capture fit on top of the ss_song.lst MILO row coordinates.
  // The authored list gives the row window/spacing; this aligns the rendered
  // font baselines to the clean PCSX2 setlist capture.
  float setlist_parent_z = -4.2f;
  float setlist_parent_scale = 1.0f;
  float setlist_base_x = 25.0f;
  float setlist_base_z = 10.0f;
  float setlist_row_h = 40.0f;
  float setlist_text_scale = 0.950f;
  float setlist_title_x = -24.0f;
  float setlist_title_z = 200.0f;
  float setlist_title_w = 380.0f;
  float setlist_title_h = 220.0f;
  float setlist_header_x = -294.0f;
  float setlist_header_z = -24.0f;
  float setlist_header_scale = 0.900f;
  // list_song2.milo_ps2::list.txt is authored at X=-263/Z=-30. Clean PCSX2
  // capture shows non-focused song labels 3 world units right of that template,
  // while the focused blue row cancels the offset below.
  float setlist_song_x = -260.0f;
  float setlist_song_z = -19.0f;
  float setlist_selected_x = -3.0f;
  float setlist_selected_z = 0.0f;
  float setlist_selected_scale = 1.0f;
  std::map<std::string, MenuElementTuning> button_tuning;
};

constexpr const char* kMenuTuneFieldNames[] = {
    "camera_x", "camera_z", "camera_distance", "camera_fov", "camera_yaw",
    "main_button_text_scale", "menu_z_scale", "menu_z_offset",
    "menu_center_x", "footer_x", "footer_z", "footer_scale",
    "setlist_base_x", "setlist_base_z", "setlist_row_h",
    "setlist_text_scale", "setlist_title_x", "setlist_title_z",
    "setlist_title_w", "setlist_title_h", "setlist_header_x",
    "setlist_header_z", "setlist_header_scale", "setlist_song_x",
    "setlist_song_z", "setlist_selected_x", "setlist_selected_z",
    "setlist_selected_scale", "setlist_parent_x", "setlist_parent_z",
    "setlist_parent_scale"};
constexpr size_t kMenuTuneFieldCount =
    sizeof(kMenuTuneFieldNames) / sizeof(kMenuTuneFieldNames[0]);

// ui/gen/sel_song_quickplay.milo_ps2::ss_song.lst is the row-stack source of
// truth. The MILO and clean background-only PCSX2 RAM trace both decode it at
// world_z=940 with 40-unit rows; camera/text origin should use that value
// directly, not a fitted offset from ss_setlist.view's 980 root.
// Fresh background-only PCSX2 trace
// stock_menu_text_terms_20260703 confirms the live Text templates match the
// MILO values below. These constants bridge those authored Text coordinates to
// this editor's D3D glyph rasterizer; the source coordinates still come from
// ss_song.lst/list_song2.milo_ps2.
constexpr float kSetlistPcsx2ListTextXOffset = 3.0f;
constexpr float kSetlistPcsx2ListTextZLift = 11.0f;
constexpr float kSetlistPcsx2HeaderTextZLift = 5.0f;
constexpr float kSetlistListTextRendererScale = 1.00961538f;
constexpr float kSetlistHeaderTextRendererScale = 0.90000000f;

enum class MenuTuneKind {
  Camera,
  MainButtons,
  Footer,
  Button,
  SetlistGroup,
  SetlistTitle,
  SetlistRows,
  SetlistHeaders,
  SetlistSongs,
  SetlistSelected,
  SetlistSpacing,
};

struct MenuTuneTarget {
  MenuTuneKind kind = MenuTuneKind::Camera;
  std::string label;
  std::string key;
};

float* menu_tune_value(MenuLayoutTuning& t, size_t i) {
  switch (i) {
    case 0: return &t.camera_x;
    case 1: return &t.camera_z;
    case 2: return &t.camera_distance;
    case 3: return &t.camera_fov;
    case 4: return &t.camera_yaw;
    case 5: return &t.main_button_text_scale;
    case 6: return &t.menu_z_scale;
    case 7: return &t.menu_z_offset;
    case 8: return &t.menu_center_x;
    case 9: return &t.footer_x;
    case 10: return &t.footer_z;
    case 11: return &t.footer_scale;
    case 12: return &t.setlist_base_x;
    case 13: return &t.setlist_base_z;
    case 14: return &t.setlist_row_h;
    case 15: return &t.setlist_text_scale;
    case 16: return &t.setlist_title_x;
    case 17: return &t.setlist_title_z;
    case 18: return &t.setlist_title_w;
    case 19: return &t.setlist_title_h;
    case 20: return &t.setlist_header_x;
    case 21: return &t.setlist_header_z;
    case 22: return &t.setlist_header_scale;
    case 23: return &t.setlist_song_x;
    case 24: return &t.setlist_song_z;
    case 25: return &t.setlist_selected_x;
    case 26: return &t.setlist_selected_z;
    case 27: return &t.setlist_selected_scale;
    case 28: return &t.setlist_parent_x;
    case 29: return &t.setlist_parent_z;
    case 30: return &t.setlist_parent_scale;
    default: return nullptr;
  }
}

const float* menu_tune_value(const MenuLayoutTuning& t, size_t i) {
  return menu_tune_value(const_cast<MenuLayoutTuning&>(t), i);
}

bool has_suffix(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() &&
         s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool load_button_tuning_value(const std::string& name, float value,
                              MenuLayoutTuning& tuning) {
  constexpr const char* kPrefix = "button.";
  constexpr size_t kPrefixLen = 7;
  if (name.compare(0, kPrefixLen, kPrefix) != 0) return false;

  const auto set_value = [&](const char* suffix, float MenuElementTuning::*field) {
    const std::string sfx(suffix);
    if (!has_suffix(name, sfx)) return false;
    const std::string key = name.substr(kPrefixLen,
                                        name.size() - kPrefixLen - sfx.size());
    if (key.empty()) return false;
    tuning.button_tuning[key].*field = value;
    return true;
  };
  return set_value(".x", &MenuElementTuning::x) ||
         set_value(".z", &MenuElementTuning::z) ||
         set_value(".scale", &MenuElementTuning::scale);
}

const MenuElementTuning* button_tuning_for(const MenuLayoutTuning& tuning,
                                           const std::string& key) {
  auto it = tuning.button_tuning.find(key);
  return it == tuning.button_tuning.end() ? nullptr : &it->second;
}

void force_menu_camera_aspect() {
  if (std::getenv("GHOGX_CAMERA_ASPECT")) return;
  _putenv_s("GHOGX_CAMERA_ASPECT", "1.333333");
}

int menu_env_int_or(const char* name, int fallback, int min_value,
                    int max_value) {
  const char* raw = std::getenv(name);
  if (!raw || !raw[0]) return fallback;
  char* end = nullptr;
  const long value = std::strtol(raw, &end, 10);
  if (end == raw || value < min_value || value > max_value) return fallback;
  return static_cast<int>(value);
}

std::string menu_env_string(const char* name) {
  char* value = nullptr;
  size_t len = 0;
  std::string out;
  if (_dupenv_s(&value, &len, name) == 0 && value && value[0])
    out = value;
  std::free(value);
  return out;
}

std::string menu_safe_filename(std::string name) {
  for (char& c : name) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (!std::isalnum(u) && c != '_' && c != '-' && c != '.') c = '_';
  }
  return name.empty() ? std::string("screen") : name;
}

std::unordered_set<std::string> menu_env_csv_set(const char* name) {
  std::unordered_set<std::string> out;
  std::string raw = menu_env_string(name);
  std::string token;
  for (char ch : raw + ",") {
    if (ch == ',') {
      token.erase(token.begin(),
                  std::find_if(token.begin(), token.end(), [](unsigned char c) {
                    return !std::isspace(c);
                  }));
      token.erase(std::find_if(token.rbegin(), token.rend(),
                               [](unsigned char c) {
                                 return !std::isspace(c);
                               }).base(),
                  token.end());
      if (!token.empty()) out.insert(token);
      token.clear();
    } else {
      token.push_back(ch);
    }
  }
  return out;
}

std::array<float, 16> menu_identity_transform() {
  return {1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          0, 0, 0, 1};
}

std::array<float, 16> menu_translation_transform(float x, float y, float z) {
  auto m = menu_identity_transform();
  m[12] = x;
  m[13] = y;
  m[14] = z;
  return m;
}

float menu_saturate(float v) {
  return std::clamp(v, 0.0f, 1.0f);
}

float menu_smoothstep(float edge0, float edge1, float x) {
  if (edge1 <= edge0) return x >= edge1 ? 1.0f : 0.0f;
  const float t = menu_saturate((x - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

struct MenuTransitionVisual {
  float exiting_brightness = 0.0f;
  float entering_brightness = 1.0f;
  float entering_x = 0.0f;
  bool draw_exiting = false;
  bool draw_entering = true;
};

bool menu_transition_diagnostic_visuals_enabled() {
  return std::getenv("GHOGX_ENABLE_MENU_TRANSITION_DIAGNOSTIC") != nullptr &&
         std::getenv("GHOGX_DISABLE_MENU_TRANSITION_VISUALS") == nullptr;
}

MenuTransitionVisual menu_transition_visual(const ScreenManager& mgr) {
  MenuTransitionVisual v;
  if (!mgr.in_transition() || !menu_transition_diagnostic_visuals_enabled())
    return v;

  const float transition_time = std::max(0.001f, mgr.transition_time());
  const float elapsed =
      std::max(0.0f, transition_time - mgr.transition_remaining());

  // Temporary diagnostic only. These constants are intentionally not used by
  // default because the transition visual still needs fresh PCSX2 evidence; the
  // source-backed part here is only the stock config's transition_time window.
  const float exit_black_seconds = transition_time * 0.48f;
  const float enter_start_seconds = transition_time * 0.44f;
  const float enter_settled_seconds = transition_time;
  constexpr float kTraceEnterStartX = 330.0f;

  const float exit_t = menu_saturate(elapsed / exit_black_seconds);
  v.exiting_brightness = 1.0f - exit_t;
  v.draw_exiting = v.exiting_brightness > 0.001f;

  const float enter_t =
      menu_smoothstep(enter_start_seconds, enter_settled_seconds, elapsed);
  v.entering_brightness = enter_t;
  const float enter_dir = mgr.transition_is_back() ? -1.0f : 1.0f;
  v.entering_x = enter_dir * (1.0f - enter_t) * kTraceEnterStartX;
  v.draw_entering = enter_t > 0.001f || !v.draw_exiting;
  return v;
}

std::string menu_tune_target_id(const MenuTuneTarget& target) {
  return std::to_string(static_cast<int>(target.kind)) + ":" + target.key;
}

bool menu_tune_can_rotate(const MenuTuneTarget& target) {
  return target.kind == MenuTuneKind::Camera;
}

bool load_menu_tuning_file(const std::string& path, MenuLayoutTuning& tuning) {
  std::ifstream in(path);
  if (!in) return false;
  std::string line;
  while (std::getline(in, line)) {
    const size_t comment = line.find('#');
    if (comment != std::string::npos) line.resize(comment);
    std::istringstream ss(line);
    std::string name;
    float value = 0.0f;
    if (!(ss >> name >> value)) continue;
    if (load_button_tuning_value(name, value, tuning)) continue;
    for (size_t i = 0; i < kMenuTuneFieldCount; ++i) {
      if (name != kMenuTuneFieldNames[i]) continue;
      if (float* dst = menu_tune_value(tuning, i)) *dst = value;
      break;
    }
  }
  std::fprintf(stderr, "[menu-tune] loaded %s\n", path.c_str());
  return true;
}

bool save_menu_tuning_file(const std::string& path,
                           const MenuLayoutTuning& tuning) {
  if (path.empty()) return false;
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path p(path);
  if (p.has_parent_path()) fs::create_directories(p.parent_path(), ec);
  std::ofstream out(path, std::ios::trunc);
  if (!out) return false;
  out << "# GuitarHeroOGX menu layout tuning\n";
  out << "# name value\n";
  out << std::fixed << std::setprecision(6);
  for (size_t i = 0; i < kMenuTuneFieldCount; ++i) {
    if (const float* v = menu_tune_value(tuning, i))
      out << kMenuTuneFieldNames[i] << ' ' << *v << '\n';
  }
  for (const auto& kv : tuning.button_tuning) {
    out << "button." << kv.first << ".x " << kv.second.x << '\n';
    out << "button." << kv.first << ".z " << kv.second.z << '\n';
    out << "button." << kv.first << ".scale " << kv.second.scale << '\n';
  }
  return true;
}

bool nudge_menu_tuning(MenuLayoutTuning& tuning, const MenuTuneTarget& target, float dx,
                       float dy, float dw, float dh, float drot, int dz) {
  constexpr float kWorldScale = 1000.0f;
  switch (target.kind) {
    case MenuTuneKind::Camera:
      tuning.camera_x += dx * kWorldScale;
      tuning.camera_z += dy * kWorldScale;
      tuning.camera_fov = std::clamp(tuning.camera_fov + dw, 0.05f, 2.5f);
      tuning.camera_distance =
          std::max(1.0f, tuning.camera_distance + dh * kWorldScale +
                             static_cast<float>(dz) * 5.0f);
      tuning.camera_yaw += drot;
      return true;
    case MenuTuneKind::MainButtons:
      tuning.menu_center_x += dx * kWorldScale;
      tuning.menu_z_offset += dy * kWorldScale;
      tuning.main_button_text_scale =
          std::max(0.05f, tuning.main_button_text_scale + dw);
      tuning.menu_z_scale =
          std::max(0.05f, tuning.menu_z_scale + dh);
      return true;
    case MenuTuneKind::Footer:
      tuning.footer_x += dx * kWorldScale;
      tuning.footer_z += dy * kWorldScale;
      tuning.footer_scale = std::max(0.05f, tuning.footer_scale + dw + dh);
      return true;
    case MenuTuneKind::Button: {
      MenuElementTuning& e = tuning.button_tuning[target.key];
      e.x += dx * kWorldScale;
      e.z += dy * kWorldScale;
      e.scale = std::max(0.05f, e.scale + dw + dh);
      return true;
    }
    case MenuTuneKind::SetlistGroup:
      tuning.setlist_parent_x += dx * kWorldScale;
      tuning.setlist_parent_z += dy * kWorldScale;
      tuning.setlist_parent_scale =
          std::max(0.05f, tuning.setlist_parent_scale + dw + dh);
      return true;
    case MenuTuneKind::SetlistTitle:
      tuning.setlist_title_x += dx * kWorldScale;
      tuning.setlist_title_z += dy * kWorldScale;
      tuning.setlist_title_w =
          std::max(1.0f, tuning.setlist_title_w + dw * kWorldScale);
      tuning.setlist_title_h =
          std::max(1.0f, tuning.setlist_title_h + dh * kWorldScale);
      return true;
    case MenuTuneKind::SetlistRows:
      tuning.setlist_base_x += dx * kWorldScale;
      tuning.setlist_base_z += dy * kWorldScale;
      return true;
    case MenuTuneKind::SetlistHeaders:
      tuning.setlist_header_x += dx * kWorldScale;
      tuning.setlist_header_z += dy * kWorldScale;
      tuning.setlist_header_scale =
          std::max(0.05f, tuning.setlist_header_scale + dw + dh);
      return true;
    case MenuTuneKind::SetlistSongs:
      tuning.setlist_song_x += dx * kWorldScale;
      tuning.setlist_song_z += dy * kWorldScale;
      tuning.setlist_text_scale =
          std::max(0.05f, tuning.setlist_text_scale + dw + dh);
      return true;
    case MenuTuneKind::SetlistSelected:
      tuning.setlist_selected_x += dx * kWorldScale;
      tuning.setlist_selected_z += dy * kWorldScale;
      tuning.setlist_selected_scale =
          std::max(0.05f, tuning.setlist_selected_scale + dw + dh);
      return true;
    case MenuTuneKind::SetlistSpacing:
      tuning.setlist_row_h = std::max(
          1.0f, tuning.setlist_row_h + (dy + dh) * kWorldScale +
                    static_cast<float>(dz));
      return true;
    default:
      return false;
  }
}

void rebuild_scene(const std::string& hdr, const std::string& ark,
                   const ConfigDb& db, ScreenManager& mgr, Object* screen,
                   ghogx::render::MiloSceneRenderer& renderer,
                   const MenuLayoutTuning& tuning,
                   MenuMaterialAnimPlayer* material_anims) {
  milo_scene::Scene combined;
  std::map<std::string, asset::Image> textures;
  std::vector<std::string> rendered_panel_files;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_active_for_render(panel)) continue;
    const std::string file = panel_file(panel);
    if (file.empty()) continue;
    add_panel_milo(hdr, ark, mgr, panel, file, combined, textures);
    rendered_panel_files.push_back(file);
  }
  append_runtime_character_displays(hdr, ark, mgr, screen, combined, textures);
  append_runtime_guitar_displays(hdr, ark, db, mgr, screen, combined, textures);
  std::fprintf(stderr, "[menu] %s: %zu meshes, %zu textures\n",
               screen ? screen->name().c_str() : "?", combined.meshes.size(), textures.size());
  const float screen_origin_z = menu_scene_origin_z(combined);
  float source_camera_origin_z = screen_origin_z;
  if (std::fabs(screen_origin_z) > 0.0001f) {
    const float row_stack_z = load_song_list_row_stack_origin_z(
        hdr, ark, song_list_milo_path(mgr));
    if (std::fabs(row_stack_z) > 0.0001f) source_camera_origin_z = row_stack_z;
  }
  renderer.set_scene(std::move(combined), textures);
  if (material_anims) material_anims->reset(renderer);
  renderer.set_hidden_meshes(menu_env_csv_set("GHOGX_HIDE_MENU_MESHES"));
  renderer.set_post_text_meshes({"light.mesh"});
  for (const std::string& file : rendered_panel_files) {
    install_panel_anim_loops_from_stock_data(hdr, ark, file, renderer,
                                             material_anims);
  }
  if (material_anims) material_anims->apply(renderer);

  // Use the menu's REAL camera (meta.cam in ui/gen/metacam.milo_ps2), now that
  // decode_cam reads it correctly: eye (0,-768,0) along -Y, looking +Y at the X-Z
  // menu plane, fov ~0.602. This is GH2's exact framing -- the poster fills the
  // screen -- grounded in the decoded camera, no multipliers.
  ghogx::render::OrbitCamera& cam = renderer.camera();
  cam.authored = false;
  cam.result_frame = {};
  cam.screen_offset[0] = 0.0f;
  cam.screen_offset[1] = 0.0f;
  const bool setlist_projection = std::fabs(screen_origin_z) > 0.0001f;
  const bool default_camera_fov =
      std::fabs(tuning.camera_fov - 0.602416f) < 0.0001f;
  const float camera_target_z =
      (setlist_projection ? source_camera_origin_z : screen_origin_z) +
      tuning.camera_z;
  if (setlist_projection) {
    renderer.set_post_text_mesh_world_offsets(
        {{"light.mesh", {0.0f, 0.0f, camera_target_z}}});
  } else {
    renderer.set_post_text_mesh_world_offsets({});
  }
  cam.yaw = tuning.camera_yaw;
  cam.pitch = 0.0f;
  cam.target[0] = tuning.camera_x;
  cam.target[1] = 0.0f;
  cam.target[2] = camera_target_z;
  cam.distance = tuning.camera_distance;
  cam.fov = tuning.camera_fov;
  cam.near_z = 1.0f;
  cam.far_z = 5000.0f;
  milo_scene::Scene cam_scene;
  if (milo_scene::load_scene(hdr, ark, "ui/gen/metacam.milo_ps2", cam_scene)) {
    for (const auto& c : cam_scene.cams) {
      if (!c.decoded || std::strcmp(c.name.c_str(), "meta.cam") != 0) continue;
      cam.target[0] = c.local.pos[0] + tuning.camera_x;
      cam.target[2] =
          (setlist_projection ? source_camera_origin_z : screen_origin_z) +
          c.local.pos[2] + tuning.camera_z;
      cam.distance = std::max(1.0f, std::fabs(c.local.pos[1]) +
                                        tuning.camera_distance - 768.0f);
      if (c.fov > 0.05f && default_camera_fov) {
        cam.fov = c.fov;
      }
      std::fprintf(stderr,
                   "[menu] meta.cam eye=(%.1f %.1f %.1f) fov=%.6f origin_z=%.1f\n",
                   c.local.pos[0], c.local.pos[1], c.local.pos[2], cam.fov,
                   source_camera_origin_z);
      break;
    }
  }
}

// Fire the focused component's SELECT_START_MSG (Confirm). The screen's (focus)
// names the active panel; that panel's (focus) names the active component.
void do_confirm(ScreenManager& mgr) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol panel_name = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = panel_name.valid() ? mgr.find_object(panel_name) : nullptr;
  if (!panel) return;
  Symbol comp = panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  // A disabled component ignores SELECT (the original's disabled BandButton does
  // not fire its handler) — e.g. multiplayer when is_missing_multi_controller.
  if (comp.valid() && compute_disabled(mgr).count(comp.c_str())) {
    mgr.run_global_handler(Symbol("BAD_SELECT_MSG"));
    return;
  }
  mgr.set_global(Symbol("component"), DataNode::Sym(comp));
  mgr.run_global_handler(Symbol("SELECT_START_MSG"));
  panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
  if (mgr.current_screen() == screen)
    screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
}

// Back (B/circle): stock screens that care about the physical button handle
// BUTTON_DOWN_MSG with $button == kPad_Tri. If no such handler exists, fall
// back to the shared screen-back path and then authored go_back/back_screen.
void do_back(ScreenManager& mgr) {
  Object* s = mgr.current_screen();
  if (!s) return;
  const bool log_back = std::getenv("GHOGX_LOG_MENU_BACK") != nullptr;
  if (log_back) {
    std::fprintf(stderr, "[menu-back] begin current=%s\n", s->name().c_str());
  }
  if (auto* u = dynamic_cast<UiObject*>(s);
      u && u->has_handler(Symbol("BUTTON_DOWN_MSG"))) {
    if (log_back) std::fprintf(stderr, "[menu-back] scripted BUTTON_DOWN_MSG kPad_Tri\n");
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_Tri")));
    u->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    return;
  }
  mgr.run_global_handler(Symbol("SCREEN_BACK_MSG"));
  if (auto* u = dynamic_cast<UiObject*>(s); u && u->has_handler(Symbol("go_back"))) {
    if (log_back) std::fprintf(stderr, "[menu-back] scripted go_back\n");
    mgr.mark_next_goto_back();
    u->handle_property(Symbol("go_back"), DataArray());
    if (mgr.current_screen() != s) return;
  }
  Symbol back = s->get_property(Symbol("back_screen")).as_symbol().value_or(Symbol());
  if (back.valid()) {
    if (log_back) {
      std::fprintf(stderr, "[menu-back] back_screen=%s\n", back.c_str());
    }
    mgr.mark_next_goto_back();
    mgr.goto_screen(back);
    return;
  }
  const bool ok = mgr.go_back();
  if (log_back) std::fprintf(stderr, "[menu-back] history=%s\n", ok ? "ok" : "empty");
}

// Load ui/eng/gen/locale.dtb into a key->display-string map. The menu's button
// labels are locale keys (e.g. "QUICK_PLAY" -> "QUICK PLAY"); the BandButton
// embeds the key, the locale resolves the shown text. 1:1 with the stock data.
std::map<std::string, std::string> load_locale(const gh::ark::ArkV3Reader& ark,
                                               const std::vector<std::string>& arks) {
  std::map<std::string, std::string> m;
  try {
    auto e = ark.find("ui/eng/gen/locale.dtb");
    if (!e) return m;
    auto bytes = ark.read_entry(*e, arks);
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    std::shared_ptr<DataArray> root = dtb_bridge::from_tree(tree);
    if (root) {
      for (std::size_t i = 0; i < root->size(); ++i) {
        auto kv = root->at(i).as_array();
        if (!kv || kv->size() < 2) continue;
        auto key = kv->at(0).as_symbol();
        auto val = kv->at(1).as_string();
        if (key && key->valid() && val) m[key->c_str()] = std::string(*val);
      }
    }
  } catch (const std::exception&) {
  }
  std::fprintf(stderr, "[menu] locale: %zu strings\n", m.size());
  return m;
}

float load_stock_transition_time(const gh::ark::ArkV3Reader& ark,
                                 const std::vector<std::string>& arks) {
  constexpr float kFallback = 0.5f;
  try {
    auto e = ark.find("../../system/run/config/gen/default.dtb");
    if (!e) return kFallback;
    auto bytes = ark.read_entry(*e, arks);
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    std::shared_ptr<DataArray> root = dtb_bridge::from_tree(tree);
    if (!root) return kFallback;
    for (std::size_t i = 0; i < root->size(); ++i) {
      auto rec = root->at(i).as_array();
      if (!rec || rec->empty()) continue;
      if (rec->at(0).as_symbol().value_or(Symbol()) != Symbol("ui")) continue;
      for (std::size_t j = 1; j < rec->size(); ++j) {
        auto field = rec->at(j).as_array();
        if (!field || field->size() < 2) continue;
        if (field->at(0).as_symbol().value_or(Symbol()) ==
            Symbol("transition_time")) {
          return field->at(1).as_float().value_or(kFallback);
        }
      }
    }
  } catch (const std::exception&) {
  }
  return kFallback;
}

// The main-menu button strip has a separate settled PS2 tint from the generic
// UIButton/BandButton state-material family decoded from ui/gen/common.milo_ps2.

// GH2 main-menu item colours — GROUND TRUTH: the actual retail menu (reference
// frame of the real game) shows NORMAL items RED and the FOCUSED item WHITE
// (CAREER white, QUICK PLAY/MULTIPLAYER/TRAINING/.../OPTIONS red).
//   normal  = RED   (1,0,0)
//   focused = WHITE (1,1,1)
//   disabled= GREY  (held for the multiplayer-disabled case)
// NOTE: the common.milo per-state .font mats (normal.mat white / focused.mat
// yellow / selecting.mat red / disabled.mat grey) are the GENERIC arial UIButton
// widget set — NOT the main-menu BandButtons, which use this red/white scheme. I
// wrongly applied the arial mats earlier; the exact data source for red/white
// (a PanelDir type or per-button colour) is to be re-pinned, but the VALUES are
// fixed by the real menu.
// Resolved PS2 output tint: solve the editor's text overlay alpha against the
// clean PCSX2 retail main-menu frame. Normal is saturated PS2 red; focused white
// lands below the older 360-derived 0.820 grey.
constexpr uint32_t kResolvedColNormal  = 0xFF6C221Au;
constexpr uint32_t kResolvedColFocused = 0xFFC8C8C8u;
// Byte-grounded in ui/gen/common.milo_ps2: normal.mat white, focused.mat
// yellow, selecting.mat red, disabled.mat 0.4 grey. These apply to the generic
// UIButton/BandButton family outside the main-menu button strip.
constexpr uint32_t kWidgetColNormal    = 0xFFFFFFFFu;
constexpr uint32_t kWidgetColFocused   = 0xFFFFFF00u;
constexpr uint32_t kWidgetColDisabled  = 0xFF666666u;
constexpr uint32_t kWidgetColSelecting = 0xFFFF0000u;
constexpr float kFocusScale      = 1.05f;        // ui_objects_ps2.dta:10 (focus_scale 1.05)
// Base RndText text_size: static main.milo tail and live trace both show 0.5.
// The main-menu overlay still needs the projected RndText fit model decoded, so
// the main-menu BandButtons use a separate visual-fit scalar below.
constexpr float kTextScale = 0.50f;
// Current main-menu fit against the clean PCSX2 client capture. Keep separate
// from kTextScale so this is easy to delete once the RndText fit path is decoded.
constexpr float kMainButtonTextScale = 0.800f;
// Main-menu BandButton positions come from main.milo_ps2 and the live PCSX2
// trace: the stored world matrices already match the runtime button row Zs.
// Keep these as identity transforms so the renderer consumes the source values
// instead of applying a second alignment pass.
constexpr float kMenuZScale = 1.0f;
constexpr float kMenuZOffset = 0.0f;
// Shared centre axis for the menu column.
constexpr float kMenuCenterX = 6.0f;

bool uses_main_menu_button_colors(const MenuLabel& lbl) {
  return lbl.type == "BandButton" && lbl.name.rfind("main_", 0) == 0;
}

std::string resolve_menu_text(std::string text,
                              const std::map<std::string, std::string>& locale) {
  if (auto it = locale.find(text); it != locale.end()) return it->second;
  std::replace(text.begin(), text.end(), '_', ' ');
  return text;
}

std::string normalize_menu_font_key(std::string key) {
  std::replace(key.begin(), key.end(), '\\', '/');
  if (const size_t slash = key.find_last_of('/'); slash != std::string::npos)
    key.erase(0, slash + 1);
  auto lower = [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  };
  std::transform(key.begin(), key.end(), key.begin(), lower);
  for (const std::string suffix : {".milo_ps2", ".milo", ".font"}) {
    if (key.size() >= suffix.size() &&
        key.compare(key.size() - suffix.size(), suffix.size(), suffix) == 0) {
      key.resize(key.size() - suffix.size());
      break;
    }
  }
  return key;
}

struct MenuFontChoice {
  std::string key;
  const MenuFont* font = nullptr;
};

struct MenuTextTarget {
  std::string key;
  const MenuFont* font = nullptr;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> verts;
};

void add_menu_text_target(std::vector<MenuTextTarget>& targets,
                          std::string key, const MenuFont& font) {
  if (!font.valid()) return;
  key = normalize_menu_font_key(std::move(key));
  if (key.empty()) return;
  for (const auto& target : targets)
    if (target.key == key) return;
  targets.push_back({std::move(key), &font, {}});
}

MenuTextTarget* menu_text_target_for_font(
    std::vector<MenuTextTarget>& targets, const std::string& authored_font) {
  if (targets.empty()) return nullptr;
  const std::string key = normalize_menu_font_key(authored_font);
  if (key.empty()) return nullptr;
  for (auto& target : targets)
    if (target.key == key) return &target;
  return nullptr;
}

uint32_t color_from_floats(const std::array<float, 4>& rgba,
                           uint32_t fallback) {
  bool sane = true;
  for (float c : rgba) sane = sane && std::isfinite(c);
  if (!sane) return fallback;
  auto ch = [](float v) -> uint32_t {
    return static_cast<uint32_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
  };
  return (ch(rgba[3]) << 24) | (ch(rgba[0]) << 16) |
         (ch(rgba[1]) << 8) | ch(rgba[2]);
}

std::string normalize_menu_markup(std::string text) {
  for (size_t pos = 0; (pos = text.find("\\Q", pos)) != std::string::npos;)
    text.replace(pos, 2, "\"");
  for (size_t pos = 0; (pos = text.find("\\n", pos)) != std::string::npos;)
    text.replace(pos, 2, "\n");
  return text;
}

std::vector<std::string> wrap_menu_text(const MenuFont& font,
                                        const std::string& text,
                                        float max_native_width) {
  if (max_native_width <= 0.0f) return {text};
  std::vector<std::string> out;
  std::istringstream paragraphs(text);
  std::string paragraph;
  while (std::getline(paragraphs, paragraph, '\n')) {
    std::istringstream words(paragraph);
    std::string word;
    std::string line;
    while (words >> word) {
      const std::string candidate =
          line.empty() ? word : (line + " " + word);
      if (!line.empty() && font.measure(candidate) > max_native_width) {
        out.push_back(line);
        line = word;
      } else {
        line = candidate;
      }
    }
    if (!line.empty() || paragraph.empty()) out.push_back(line);
  }
  if (out.empty()) out.push_back(text);
  return out;
}

enum class MenuTextHAlign { Left, Center, Right };
enum class MenuTextVAlign { Top, Middle, Bottom };

MenuTextHAlign text_h_align(uint32_t align_flags) {
  switch (align_flags & 0x0fu) {
    case 0x01u: return MenuTextHAlign::Left;
    case 0x04u: return MenuTextHAlign::Right;
    case 0x02u:
    default: return MenuTextHAlign::Center;
  }
}

MenuTextVAlign text_v_align(uint32_t align_flags) {
  switch ((align_flags >> 4) & 0x0fu) {
    case 0x01u: return MenuTextVAlign::Top;
    case 0x04u: return MenuTextVAlign::Bottom;
    case 0x02u:
    default: return MenuTextVAlign::Middle;
  }
}

bool menu_name_matches_env_spec(const char* name, const std::string& item) {
  char* value = nullptr;
  size_t len = 0;
  const bool has = _dupenv_s(&value, &len, name) == 0 && value && value[0];
  std::string spec = has ? value : "";
  std::free(value);
  if (spec.empty()) return false;
  size_t start = 0;
  while (start <= spec.size()) {
    const size_t comma = spec.find(',', start);
    const std::string needle = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!needle.empty() && item.find(needle) != std::string::npos) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

bool is_main_menu_support_label(const MenuLabel& label) {
  return label.name == "song.text" || label.name == "venue.text" ||
         label.name == "difficulty.text" || label.name == "mm_msg.lbl";
}

uint32_t menu_read_u32(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 4 > body.size()) return 0;
  uint32_t value = 0;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

float menu_read_f32(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 4 > body.size()) return 0.0f;
  float value = 0.0f;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

bool menu_finite(float value) {
  return std::isfinite(value) && std::fabs(value) < 100000.0f;
}

bool read_menu_string(const std::vector<uint8_t>& body, size_t& offset,
                      std::string& out) {
  if (offset + 4 > body.size()) return false;
  const uint32_t len = menu_read_u32(body, offset);
  offset += 4;
  if (len > body.size() - offset || len > 128) return false;
  for (uint32_t i = 0; i < len; ++i) {
    const uint8_t c = body[offset + i];
    if (c < 0x20 || c >= 0x7f) return false;
  }
  out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
  offset += len;
  return true;
}

bool looks_like_menu_matrix(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 48 > body.size()) return false;
  float m[12];
  for (int i = 0; i < 12; ++i) {
    m[i] = menu_read_f32(body, offset + static_cast<size_t>(i) * 4);
    if (!menu_finite(m[i])) return false;
  }
  for (int r = 0; r < 3; ++r) {
    const float mag = std::sqrt(m[r * 3] * m[r * 3] +
                                m[r * 3 + 1] * m[r * 3 + 1] +
                                m[r * 3 + 2] * m[r * 3 + 2]);
    if (mag < 0.01f || mag > 100.0f) return false;
  }
  return std::fabs(m[9]) < 5000.0f && std::fabs(m[10]) < 5000.0f &&
         std::fabs(m[11]) < 5000.0f;
}

std::vector<uint8_t> load_milo_entry_body(const std::string& hdr,
                                          const std::string& ark,
                                          const std::string& milo_path,
                                          const std::string& type,
                                          const std::string& name) {
  auto arkr = gh::ark::ArkV3Reader::load(hdr);
  auto entry = arkr.find(milo_path);
  if (!entry) entry = arkr.find("../../system/run/" + milo_path);
  if (!entry) return {};
  auto bytes = arkr.read_entry(*entry, {ark});
  auto mh = gh::milo::parse_header(bytes);
  auto payload = gh::milo::inflate_payload(bytes, mh);
  auto dir = gh::milo::parse_directory(payload);
  for (const auto& e : dir.entries) {
    if (e.type != type || e.name != name || e.offset + e.size > payload.size())
      continue;
    return std::vector<uint8_t>(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
  }
  return {};
}

float load_song_list_setlist_origin_z(const std::string& hdr,
                                      const std::string& ark,
                                      const std::string& milo_path) {
  if (milo_path.empty()) return 0.0f;
  const std::vector<uint8_t> body = load_milo_entry_body(
      hdr, ark, milo_path, "Group", "ss_setlist.view");
  if (body.empty()) return 0.0f;

  for (size_t m = 0; m + 96 + 9 + 4 <= body.size(); ++m) {
    if (!looks_like_menu_matrix(body, m) ||
        !looks_like_menu_matrix(body, m + 48)) {
      continue;
    }
    size_t parent_offset = m + 96 + 9;
    std::string parent;
    if (!read_menu_string(body, parent_offset, parent) ||
        parent != "sel_song.view") {
      continue;
    }
    return menu_read_f32(body, m + 48 + 44);
  }
  return 0.0f;
}

float load_song_list_row_stack_origin_z(const std::string& hdr,
                                        const std::string& ark,
                                        const std::string& milo_path) {
  const UiListLayout layout = extract_ui_list_layout(
      hdr, ark, milo_path, "ss_song.lst");
  if (layout.valid && std::fabs(layout.world_z) > 0.0001f)
    return layout.world_z;
  return load_song_list_setlist_origin_z(hdr, ark, milo_path);
}

float screen_text_origin_z(Object* screen, ScreenManager& mgr,
                           const std::string& hdr,
                           const std::string& ark) {
  if (screen_has_panel(screen, Symbol("sel_song_panel"))) {
    const float row_stack_origin = load_song_list_row_stack_origin_z(
        hdr, ark, song_list_milo_path(mgr));
    if (std::fabs(row_stack_origin) > 0.0001f) return row_stack_origin;
  }
  return 0.0f;
}

float authored_or_tuned(float tuned_value, float old_default,
                        float authored_value) {
  return std::fabs(tuned_value - old_default) < 0.0001f ? authored_value
                                                        : tuned_value;
}

bool menu_truthy(const DataNode& node) {
  if (auto i = node.as_int()) return *i != 0;
  if (auto f = node.as_float()) return *f != 0.0f;
  if (auto s = node.as_string())
    return !s->empty() && *s != "FALSE" && *s != "false" && *s != "0";
  return false;
}

struct MenuGroupGraph {
  std::unordered_map<std::string, std::vector<std::string>> owners;
  std::unordered_map<std::string, std::string> parents;
};

MenuGroupGraph build_menu_group_graph(const milo_scene::Scene& scene) {
  MenuGroupGraph graph;
  for (const auto& group : scene.groups) {
    graph.parents[group.name] = group.parent;
    for (const auto& child : group.children) {
      if (!child.empty()) graph.owners[child].push_back(group.name);
    }
  }
  return graph;
}

MenuGroupGraph load_menu_group_graph(const std::string& hdr,
                                     const std::string& ark,
                                     const std::string& milo_path) {
  milo_scene::Scene scene;
  if (!milo_scene::load_scene(hdr, ark, milo_path, scene)) return {};
  return build_menu_group_graph(scene);
}

bool menu_object_showing(ScreenManager& mgr, const std::string& name) {
  if (name.empty()) return true;
  Object* obj = mgr.resolve_object(Symbol(name.c_str()));
  if (!obj) return true;
  const DataNode showing = obj->get_property(Symbol("showing"));
  return showing.empty() || menu_truthy(showing);
}

bool menu_group_visible(ScreenManager& mgr, const MenuGroupGraph& graph,
                        const std::string& group,
                        std::unordered_set<std::string>& visiting) {
  if (group.empty()) return true;
  if (!visiting.insert(group).second) return true;
  if (!menu_object_showing(mgr, group)) return false;
  auto it = graph.parents.find(group);
  if (it != graph.parents.end() && !it->second.empty() &&
      graph.parents.find(it->second) != graph.parents.end()) {
    if (!menu_group_visible(mgr, graph, it->second, visiting)) return false;
  }
  auto owner_it = graph.owners.find(group);
  if (owner_it != graph.owners.end() && !owner_it->second.empty()) {
    for (const std::string& owner : owner_it->second) {
      if (menu_group_visible(mgr, graph, owner, visiting)) return true;
    }
    return false;
  }
  return true;
}

bool menu_group_owners_showing(ScreenManager& mgr, const MenuGroupGraph& graph,
                               const std::string& object_name) {
  auto it = graph.owners.find(object_name);
  if (it == graph.owners.end() || it->second.empty()) return true;
  for (const std::string& owner : it->second) {
    std::unordered_set<std::string> visiting;
    if (menu_group_visible(mgr, graph, owner, visiting)) return true;
  }
  return false;
}

bool menu_group_has_owner(const MenuGroupGraph& graph,
                          const std::string& object_name) {
  auto it = graph.owners.find(object_name);
  return it != graph.owners.end() && !it->second.empty();
}

std::string menu_node_text(const DataNode& node) {
  if (auto s = node.as_string()) return std::string(s->data(), s->size());
  if (auto i = node.as_int()) return std::to_string(*i);
  if (auto f = node.as_float()) {
    std::ostringstream ss;
    ss << *f;
    return ss.str();
  }
  return {};
}

bool is_stock_placeholder_text(const std::string& text) {
  return text == "THIS SHOULD NOT BE DRAWING";
}

bool is_stock_placeholder_label(const MenuLabel& label) {
  return label.type == "BandLabel" && label.font.empty() &&
         label.parent.empty() && label.text == "default";
}

std::string log_safe_menu_text(std::string text) {
  for (char& c : text) {
    if (c == '\n') c = '|';
    else if (c == '\r' || c == '\t') c = ' ';
  }
  return text;
}

void append_text_quads(ScreenManager& mgr, const std::vector<MenuLabel>& labels,
                       std::vector<MenuTextTarget>& targets,
                       const MenuGroupGraph& group_graph,
                       const std::map<std::string, std::string>& locale,
                       const std::string& focused,
                       const std::unordered_set<std::string>& disabled,
                       const MenuLayoutTuning& tuning) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  auto emit = [&](std::vector<TV>& out, const std::vector<MenuFont::Quad>& quads,
                  const std::function<TV(float, float, float, float)>& V) {
    for (const auto& q : quads) {
      TV a = V(q.x0, q.y0, q.u0, q.v0), b = V(q.x1, q.y0, q.u1, q.v0),
         c = V(q.x1, q.y1, q.u1, q.v1), d = V(q.x0, q.y1, q.u0, q.v1);
      out.push_back(a); out.push_back(b); out.push_back(c);
      out.push_back(a); out.push_back(c); out.push_back(d);
    }
  };
  for (const auto& lbl : labels) {
    if (menu_name_matches_env_spec("GHOGX_SKIP_MENU_LABEL", lbl.name)) continue;
    if (std::getenv("GHOGX_ONLY_MENU_LABEL") &&
        !menu_name_matches_env_spec("GHOGX_ONLY_MENU_LABEL", lbl.name)) {
      continue;
    }
    const bool debug_text = !menu_env_string("GHOGX_DEBUG_MENU_TEXT").empty();
    auto debug_skip = [&](const char* reason, const std::string& detail = {}) {
      if (!debug_text) return;
      std::fprintf(stderr,
                   "[menu-text-skip] name=%s type=%s parent=%s font=%s "
                   "reason=%s text='%s' has_world=%d world=(%.1f %.1f %.1f)",
                   lbl.name.c_str(), lbl.type.c_str(), lbl.parent.c_str(),
                   lbl.font.c_str(), reason, log_safe_menu_text(lbl.text).c_str(),
                   lbl.has_world ? 1 : 0, lbl.world[9], lbl.world[10],
                   lbl.world[11]);
      if (!detail.empty()) std::fprintf(stderr, " %s", detail.c_str());
      std::fprintf(stderr, "\n");
    };
    if (is_main_menu_support_label(lbl)) continue;
    if (!lbl.has_world) {
      debug_skip("no-world");
      continue;
    }
    const std::array<float, 12>& world = lbl.world;
    const bool isBtn = (lbl.type == "BandButton");
    if (!isBtn && lbl.type != "Text" && lbl.type != "BandLabel") {
      debug_skip("unsupported-type");
      continue;
    }
    const bool off_plane_bandlabel =
        lbl.type == "BandLabel" && lbl.has_local && world[10] > -900.0f &&
        (std::fabs(world[1]) > 10.0f ||
         std::fabs(world[7]) > 10.0f);
    // Some stock labels are authored onto angled 3-D prop/poster planes. The
    // overlay text path cannot draw those perspective RndText planes yet; skip
    // them rather than collapsing the glyphs into the menu camera.
    if (off_plane_bandlabel) {
      debug_skip("off-plane-bandlabel");
      continue;
    }
    const bool uses_local_text_plane =
        !isBtn && lbl.has_local &&
        lbl.parent.find("text") != std::string::npos;
    if (!uses_local_text_plane && !isBtn && std::fabs(world[10]) > 200.0f) {
      debug_skip("far-world-y");
      continue;
    }
    if (!menu_group_owners_showing(mgr, group_graph, lbl.name)) {
      debug_skip("hidden-group-owner");
      continue;
    }
    Object* widget = mgr.resolve_object(Symbol(lbl.name.c_str()));
    if (lbl.has_showing && !lbl.showing &&
        (!widget || widget->get_property(Symbol("showing")).empty())) {
      debug_skip("hidden-authored");
      continue;
    }
    // Some stock controls are focus targets only: sel_venue.milo_ps2's
    // sv_*.btn entries have neither a serialized parent nor a MILO group owner,
    // while the visible venue names live in the banner/venue meshes. Lag/options
    // screens can store drawable buttons through group ownership only, so keep
    // those and only skip truly unowned controls.
    if (isBtn && lbl.parent.empty() &&
        !menu_group_has_owner(group_graph, lbl.name)) {
      debug_skip("button-no-parent");
      continue;
    }
    if (!lbl.parent.empty()) {
      if (Object* parent = mgr.resolve_object(Symbol(lbl.parent.c_str()))) {
        const DataNode showing = parent->get_property(Symbol("showing"));
        if (!showing.empty() && !menu_truthy(showing)) {
          debug_skip("hidden-parent");
          continue;
        }
      }
    }

    // Colour: buttons by state. Generic widgets use common.milo state mats;
    // main-menu buttons use their settled PS2 trace tint. Text/BandLabel carry
    // their authored text-tail colour when present.
    bool foc = false;
    uint32_t col = 0xFFFFFFFFu;
    std::string raw_text = lbl.text;
    bool widget_disabled = false;
    if (widget) {
      const DataNode showing = widget->get_property(Symbol("showing"));
      if (!showing.empty() && !menu_truthy(showing)) {
        debug_skip("hidden-widget");
        continue;
      }
      const DataNode runtime_text_node = widget->get_property(Symbol("text"));
      if (!runtime_text_node.empty()) raw_text = menu_node_text(runtime_text_node);
      const DataNode runtime_token_node =
          widget->get_property(Symbol("text_token"));
      if (!runtime_token_node.empty())
        raw_text = menu_node_text(runtime_token_node);
      widget_disabled = menu_truthy(widget->get_property(Symbol("disabled")));
    }
    if (raw_text.empty()) {
      debug_skip("empty-text");
      continue;
    }
    if (is_stock_placeholder_label(lbl)) {
      debug_skip("placeholder-label");
      continue;
    }
    if (is_stock_placeholder_text(raw_text)) {
      debug_skip("placeholder-text");
      continue;
    }
    if (isBtn) {
      foc = (lbl.name == focused);
      if (disabled.count(lbl.name) || widget_disabled) {
        col = kWidgetColDisabled;
      } else if (uses_main_menu_button_colors(lbl)) {
        col = foc ? kResolvedColFocused : kResolvedColNormal;
      } else {
        col = foc ? kWidgetColFocused : kWidgetColNormal;
      }
    } else if (lbl.text_tail.valid) {
      col = color_from_floats(lbl.text_tail.color, col);
    }
    std::string disp = normalize_menu_markup(resolve_menu_text(raw_text, locale));
    if (!menu_env_string("GHOGX_DEBUG_MENU_TEXT").empty()) {
      const std::string raw_log = log_safe_menu_text(raw_text);
      const std::string disp_log = log_safe_menu_text(disp);
      std::fprintf(stderr,
                   "[menu-text] name=%s type=%s parent=%s font=%s raw='%s' "
                   "disp='%s' world=(%.1f %.1f %.1f) "
                   "row0=(%.3f %.3f %.3f) row2=(%.3f %.3f %.3f)",
                   lbl.name.c_str(), lbl.type.c_str(), lbl.parent.c_str(),
                   lbl.font.c_str(), raw_log.c_str(), disp_log.c_str(),
                   world[9], world[10], world[11],
                   world[0], world[1], world[2],
                   world[6], world[7], world[8]);
      if (lbl.text_tail.valid) {
        std::fprintf(stderr,
                     " text_tail=(fit=%d width=%.1f height=%.1f "
                     "size=%.1f wrap=%.1f align=0x%08x color=%.2f,%.2f,%.2f,%.2f)",
                     lbl.text_tail.fit_text, lbl.text_tail.label_width,
                     lbl.text_tail.box_height, lbl.text_tail.text_size,
                     lbl.text_tail.width_bound, lbl.text_tail.align_flags,
                     lbl.text_tail.color[0], lbl.text_tail.color[1],
                     lbl.text_tail.color[2], lbl.text_tail.color[3]);
      }
      if (lbl.button_tail.valid) {
        std::fprintf(stderr,
                     " button_tail=(fit=%d caps=%u width=%.1f height=%.1f "
                     "scale=%.2f size=%.1f wrap=%.1f align=0x%08x)",
                     lbl.button_tail.fit_text,
                     static_cast<unsigned>(lbl.button_tail.all_caps),
                     lbl.button_tail.label_width, lbl.button_tail.box_height,
                     lbl.button_tail.scale, lbl.button_tail.text_size,
                     lbl.button_tail.width_bound, lbl.button_tail.align_flags);
      }
      std::fprintf(stderr, "\n");
    }
    MenuTextTarget* target = menu_text_target_for_font(targets, lbl.font);
    if (!target || !target->font || !target->font->valid()) {
      debug_skip("missing-font-target");
      continue;
    }
    const MenuFont& font = *target->font;
    const float capH = font.cap_height();
    float w = 0.0f;
    auto quads = font.layout(disp, &w);

    // Normalized local-X / local-Z axes (X-Z plane) for the menu's slight tilt;
    // n0/n2 are the original scale magnitudes (used to spot the bind pose).
    float r0x = world[0], r0z = world[2], r2x = world[6], r2z = world[8];
    float n0 = std::sqrt(r0x*r0x + r0z*r0z), n2 = std::sqrt(r2x*r2x + r2z*r2z);
    if (n0 > 1e-6f) { r0x /= n0; r0z /= n0; }
    if (n2 > 1e-6f) { r2x /= n2; r2z /= n2; }

    if (isBtn) {
      // BandButton glyphs always use the uniform kTextScale (the in-MILO button
      // scale is a TransAnim bind pose; it does NOT give the rendered size). X:
      // the main menu's bind-pose buttons (non-uniform box scale 0.555/1.899) use
      // the runtime-aligned left edge; other screens use the button's translation.
      const bool bindPose = n0 > 1e-3f && n2 > 1e-3f &&
                            (std::min(n0, n2) / std::max(n0, n2) < 0.6f);
      float ax = bindPose ? (world[9] + tuning.menu_center_x) : world[9];
      const float ay = world[10];
      // Main-menu bind-pose buttons: remap the bind-pose Z to the XEX-measured
      // runtime Z (affine). Other screens use their Z.
      float az = bindPose ? (tuning.menu_z_scale * world[11] +
                             tuning.menu_z_offset) : world[11];
      float authored_button_scale = kTextScale;
      if (lbl.button_tail.valid && lbl.button_tail.scale > 0.0f) {
        authored_button_scale = lbl.button_tail.scale;
      } else if (!bindPose && lbl.button_tail.valid &&
                 lbl.button_tail.text_size > 0.0f && font.cap_height() > 0.0f) {
        authored_button_scale = lbl.button_tail.text_size / font.cap_height();
      } else if (!bindPose && lbl.button_tail.valid &&
                 lbl.button_tail.box_height > 0.0f &&
                 font.line_height() > 0.0f) {
        authored_button_scale = lbl.button_tail.box_height / font.line_height();
      }
      float scl = (bindPose
                       ? authored_or_tuned(tuning.main_button_text_scale, 0.500f,
                                           authored_button_scale)
                       : authored_button_scale) *
                  (foc ? kFocusScale : 1.0f);
      float fit_x = 1.0f;
      float fit_z = 1.0f;
      if (bindPose && lbl.button_tail.valid && lbl.button_tail.label_width > 0.0f &&
          w > 0.0f) {
        const float raw_fit = std::clamp(lbl.button_tail.label_width / w, 0.5f, 2.5f);
        fit_x = std::sqrt(raw_fit);
      } else if (!bindPose && lbl.button_tail.valid &&
                 lbl.button_tail.label_width > 0.0f && w > 0.0f) {
        const float rendered_w = w * scl;
        const float rendered_h = capH * scl;
        if (rendered_w > 0.0f) {
          const float raw_fit = lbl.button_tail.label_width / rendered_w;
          if (lbl.button_tail.fit_text == 1) {
            fit_x = std::clamp(raw_fit, 0.25f, 2.5f);
            if (lbl.button_tail.box_height > 0.0f && rendered_h > 0.0f) {
              fit_z = std::clamp(lbl.button_tail.box_height / rendered_h,
                                 0.25f, 2.5f);
            }
          } else if (lbl.button_tail.fit_text == 2) {
            float just_fit = 1.0f;
            if (rendered_w > lbl.button_tail.label_width)
              just_fit = std::min(just_fit,
                                  lbl.button_tail.label_width / rendered_w);
            if (lbl.button_tail.box_height > 0.0f &&
                rendered_h > lbl.button_tail.box_height) {
              just_fit = std::min(just_fit,
                                  lbl.button_tail.box_height / rendered_h);
            }
            fit_x = fit_z = std::clamp(just_fit, 0.25f, 1.0f);
          } else if (rendered_w > lbl.button_tail.label_width) {
            fit_x = std::clamp(raw_fit, 0.25f, 1.0f);
          }
        }
      }
      if (const MenuElementTuning* bt = button_tuning_for(tuning, lbl.name)) {
        ax += bt->x;
        az += bt->z;
        scl *= bt->scale;
      }
      emit(target->verts, quads, [&](float qx, float qy, float u, float v) {
        const float lx = (qx - w * 0.5f) * scl * fit_x;
        const float lz = -(qy - capH * 0.5f) * scl * fit_z;
        TV tv{ax + lx * r0x + lz * r2x, ay, az + lx * r0z + lz * r2z, u, v, col};
        return tv;
      });
    } else {
      // Text / BandLabel: use the authored local RndText plane plus its text
      // tail size. Several stock BandLabels (notably sel_guitar) carry a stored
      // world matrix that is an off-plane parent pose, while the local transform
      // is the visible menu text anchor.
      const bool use_local_text =
          uses_local_text_plane || (lbl.has_local && world[10] < -900.0f);
      const auto& M = use_local_text ? lbl.local : world;
      const float Tx = M[9], Ty = M[10], Tz = M[11];
      const float scl =
          (lbl.text_tail.valid && lbl.text_tail.text_size > 0.0f &&
           capH > 0.0f)
              ? (lbl.text_tail.text_size / capH)
              : 1.0f;
      const float box_w =
          (lbl.text_tail.valid && lbl.text_tail.label_width > 0.0f)
              ? lbl.text_tail.label_width
              : 0.0f;
      const float box_h =
          (lbl.text_tail.valid && lbl.text_tail.box_height > 0.0f)
              ? lbl.text_tail.box_height
              : 0.0f;
      const float bound =
          box_w > 0.0f
              ? box_w
              : (lbl.text_tail.valid ? lbl.text_tail.width_bound : 0.0f);
      const bool can_wrap =
          lbl.text_tail.valid && bound > 0.0f &&
          lbl.text_tail.box_height > lbl.text_tail.text_size * 2.25f;
      const std::vector<std::string> lines =
          can_wrap ? wrap_menu_text(font, disp, bound / std::max(scl, 0.001f))
                   : std::vector<std::string>{disp};
      const uint32_t align_flags =
          lbl.text_tail.valid ? lbl.text_tail.align_flags : 0x22u;
      const MenuTextHAlign h_align = text_h_align(align_flags);
      const MenuTextVAlign v_align = text_v_align(align_flags);
      const float leading =
          (lbl.text_tail.valid && lbl.text_tail.leading > 0.0f)
              ? lbl.text_tail.leading
              : 1.0f;
      const float line_h = std::max(1.0f, font.line_height() * scl * leading);
      std::vector<std::vector<MenuFont::Quad>> line_quads;
      std::vector<float> line_widths;
      line_quads.reserve(lines.size());
      line_widths.reserve(lines.size());
      float max_line_w = 0.0f;
      for (const std::string& line : lines) {
        float line_w = 0.0f;
        line_quads.push_back(font.layout(line, &line_w));
        line_widths.push_back(line_w);
        max_line_w = std::max(max_line_w, line_w);
      }
      const float content_h =
          capH * scl +
          static_cast<float>(std::max<std::size_t>(lines.size(), 1) - 1) * line_h;
      float fit_x_object = 1.0f;
      float fit_z_object = 1.0f;
      if (lbl.text_tail.valid) {
        const float rendered_w = max_line_w * scl;
        if (lbl.text_tail.fit_text == 1) {
          if (box_w > 0.0f && rendered_w > 0.0f)
            fit_x_object = std::clamp(box_w / rendered_w, 0.25f, 2.5f);
          if (box_h > 0.0f && content_h > 0.0f)
            fit_z_object = std::clamp(box_h / content_h, 0.25f, 2.5f);
        } else if (lbl.text_tail.fit_text == 2) {
          float just_fit = 1.0f;
          if (box_w > 0.0f && rendered_w > box_w)
            just_fit = std::min(just_fit, box_w / rendered_w);
          if (box_h > 0.0f && content_h > box_h)
            just_fit = std::min(just_fit, box_h / content_h);
          fit_x_object = fit_z_object = std::clamp(just_fit, 0.01f, 1.0f);
        } else if (box_w > 0.0f && rendered_w > box_w && rendered_w > 0.0f) {
          fit_x_object = std::clamp(box_w / rendered_w, 0.25f, 1.0f);
        }
      }
      const float block_h = content_h * fit_z_object;
      const float z_origin =
          v_align == MenuTextVAlign::Top
              ? 0.0f
              : (v_align == MenuTextVAlign::Bottom ? block_h : block_h * 0.5f);
      for (size_t line_i = 0; line_i < lines.size(); ++line_i) {
        const float line_w = line_widths[line_i];
        const float anchor =
            h_align == MenuTextHAlign::Left
                ? 0.0f
                : (h_align == MenuTextHAlign::Right ? -line_w : -line_w * 0.5f);
        const float line_z = -static_cast<float>(line_i) * line_h * fit_z_object;
        emit(target->verts, line_quads[line_i],
             [&](float qx, float qy, float u, float v) {
               const float lx = (qx + anchor) * scl * fit_x_object;
               const float lz = -qy * scl * fit_z_object + z_origin + line_z;
               TV tv{Tx + M[0] * lx + M[6] * lz,
                     Ty + M[1] * lx + M[7] * lz,
                     Tz + M[2] * lx + M[8] * lz,
                     u, v, col};
               return tv;
             });
      }
    }
  }
}

void append_song_string_scaled(
    const std::string& text, const MenuFont& font, float x, float y, float z,
    float scale_x, float scale_z, uint32_t col,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out);

void append_song_string(const std::string& text, const MenuFont& font, float x, float y,
                        float z, float scale, uint32_t col,
                        std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  append_song_string_scaled(text, font, x, y, z, scale, scale, col, out);
}

void append_song_string_scaled(
                        const std::string& text, const MenuFont& font, float x,
                        float y, float z, float scale_x, float scale_z,
                        uint32_t col,
                        std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  float w = 0.0f;
  auto quads = font.layout(text, &w);
  const float capH = font.cap_height();
  for (const auto& q : quads) {
    auto V = [&](float qx, float qy, float u, float v) {
      return TV{x + qx * scale_x, y, z - (qy - capH * 0.5f) * scale_z,
                u, v, col};
    };
    TV a = V(q.x0, q.y0, q.u0, q.v0), b = V(q.x1, q.y0, q.u1, q.v0),
       c = V(q.x1, q.y1, q.u1, q.v1), d = V(q.x0, q.y1, q.u0, q.v1);
    out.push_back(a); out.push_back(b); out.push_back(c);
    out.push_back(a); out.push_back(c); out.push_back(d);
  }
}

void append_image_quad(float x, float y, float z, float w, float h, uint32_t col,
                       std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float x0 = x - w * 0.5f, x1 = x + w * 0.5f;
  const float z0 = z - h * 0.5f, z1 = z + h * 0.5f;
  TV a{x0, y, z1, 0.0f, 0.0f, col}, b{x1, y, z1, 1.0f, 0.0f, col};
  TV c{x1, y, z0, 1.0f, 1.0f, col}, d{x0, y, z0, 0.0f, 1.0f, col};
  out.push_back(a); out.push_back(b); out.push_back(c);
  out.push_back(a); out.push_back(c); out.push_back(d);
}

void append_image_quad_uv(float x, float y, float z, float w, float h,
                          float u0, float v0, float u1, float v1, uint32_t col,
                          std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float x0 = x - w * 0.5f, x1 = x + w * 0.5f;
  const float z0 = z - h * 0.5f, z1 = z + h * 0.5f;
  TV a{x0, y, z1, u0, v0, col}, b{x1, y, z1, u1, v0, col};
  TV c{x1, y, z0, u1, v1, col}, d{x0, y, z0, u0, v1, col};
  out.push_back(a); out.push_back(b); out.push_back(c);
  out.push_back(a); out.push_back(c); out.push_back(d);
}

const milo_scene::MeshObj* find_decoded_mesh(const milo_scene::Scene& scene,
                                             const char* mesh_name) {
  for (const auto& m : scene.meshes) {
    if (m.name == mesh_name && m.decoded) return &m;
  }
  return nullptr;
}

float mesh_world_pos_or(const milo_scene::Scene& scene, const char* mesh_name,
                        int axis, float fallback) {
  const auto* mesh = find_decoded_mesh(scene, mesh_name);
  if (!mesh || axis < 0 || axis > 2) return fallback;
  return mesh->world_stored.pos[axis];
}

void append_helpbar_mesh_quad_at(
    const milo_scene::Scene& scene, const char* mesh_name,
    float target_x, float target_z, uint32_t col,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const milo_scene::MeshObj* mesh = find_decoded_mesh(scene, mesh_name);
  if (!mesh || mesh->indices.empty()) return;
  const auto& world = mesh->world_stored;
  const float x_offset = target_x - world.pos[0];
  const float z_offset = target_z - world.pos[2];
  auto vertex = [&](uint16_t index) {
    const auto& v = mesh->verts[index];
    const float x = v.px * world.rot[0][0] + v.py * world.rot[1][0] +
                    v.pz * world.rot[2][0] + world.pos[0] + x_offset;
    const float y = v.px * world.rot[0][1] + v.py * world.rot[1][1] +
                    v.pz * world.rot[2][1] + world.pos[1];
    const float z = v.px * world.rot[0][2] + v.py * world.rot[1][2] +
                    v.pz * world.rot[2][2] + world.pos[2] + z_offset;
    return TV{x, y, z, v.u, v.v, col};
  };
  for (uint16_t index : mesh->indices) out.push_back(vertex(index));
}

struct HelpItem {
  std::string control;
  std::string token;
};

struct HelpbarSpacing {
  float button_spacing = 35.0f;
  float strumbar_spacing = 70.0f;
  float text_spacing = 30.0f;
};

float helpbar_prop_float(Object* helpbar, const char* key, float fallback) {
  if (!helpbar) return fallback;
  auto v = helpbar->get_property(Symbol(key)).as_float();
  if (!v || *v <= 0.0f) return fallback;
  return *v;
}

HelpbarSpacing helpbar_spacing_from_panel(Object* helpbar) {
  HelpbarSpacing out;
  out.button_spacing =
      helpbar_prop_float(helpbar, "button_spacing", out.button_spacing);
  out.strumbar_spacing =
      helpbar_prop_float(helpbar, "strumbar_spacing", out.strumbar_spacing);
  out.text_spacing =
      helpbar_prop_float(helpbar, "text_spacing", out.text_spacing);
  return out;
}

void collect_help_tokens(const DataNode& n, std::vector<HelpItem>& out) {
  auto arr = n.as_array();
  if (!arr) return;
  if (arr->size() == 2) {
    auto control = arr->at(0).as_symbol();
    auto token = arr->at(1).as_symbol();
    if (control && token) {
      const char* c = control->c_str();
      if (std::strcmp(c, "fret1") == 0 || std::strcmp(c, "fret2") == 0 ||
          std::strcmp(c, "fret3") == 0 || std::strcmp(c, "strum") == 0 ||
          std::strcmp(c, "start") == 0) {
        out.push_back({control->c_str(), token->c_str()});
        return;
      }
    }
  }
  for (std::size_t i = 0; i < arr->size(); ++i) collect_help_tokens(arr->at(i), out);
}

std::string help_label(const std::string& token,
                       const std::map<std::string, std::string>& locale) {
  if (auto it = locale.find(token); it != locale.end()) return it->second;
  return token;
}

void append_help_footer(ScreenManager& mgr, Object* screen, const MenuFont& font,
                        const MenuFont& footer_font,
                        const std::map<std::string, std::string>& locale,
                        const std::map<std::string, asset::Image>& icons,
                        const milo_scene::Scene& helpbar_scene,
                        const UiListTextTemplate& helpbar_template,
                        const MenuLayoutTuning& tuning,
                        float screen_origin_z,
                        std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out,
                        std::vector<ghogx::render::MiloSceneRenderer::TextBatch>& batches) {
  if (!screen) return;
  std::vector<HelpItem> items;
  Object* helpbar = mgr.find_object(Symbol("helpbar"));
  if (helpbar && screen_has_panel(screen, Symbol("helpbar")))
    collect_help_tokens(helpbar->get_property(Symbol("display")), items);
  if (items.empty())
    collect_help_tokens(screen->get_property(Symbol("helpbar")), items);
  if (items.empty()) return;

  const float kFooterZ = screen_origin_z + tuning.footer_z;
  constexpr float kFooterY = 0.0f;
  const float kFooterScale = tuning.footer_scale;
  const uint32_t footer_col =
      helpbar_template.valid
          ? color_from_floats(helpbar_template.color, 0xFFE6E6E6u)
          : 0xFFE6E6E6u;
  const bool use_footer_font = footer_font.valid();
  const MenuFont& text_font = use_footer_font ? footer_font : font;
  const float authored_text_size =
      (helpbar_template.valid && helpbar_template.text_size > 0.0f)
          ? helpbar_template.text_size
          : 18.0f;
  // helpbar.milo_ps2::help_bar.txt owns the footer RndText font, tint, and
  // text_size; the remaining X/Z residuals below are the runtime HelpBarPanel
  // population bridge, not template styling.
  const float footer_text_scale_z =
      use_footer_font ? (authored_text_size / text_font.cap_height()) : kFooterScale;
  const float footer_text_scale_x = footer_text_scale_z;
  const float footer_text_z =
      use_footer_font ? (kFooterZ - 2.0f) : kFooterZ;
  constexpr const char* kFretIconMesh = "help_bar_starting.mesh";
  constexpr const char* kStrumIconMesh = "help_bar_strum.mesh";
  constexpr const char* kStrumbarAnchorMesh = "help_bar_strumbar.mesh";
  const float first_icon_left =
      tuning.footer_x +
      mesh_world_pos_or(helpbar_scene, kFretIconMesh, 0, -294.9153f);
  const float strum_icon_left =
      tuning.footer_x +
      mesh_world_pos_or(helpbar_scene, kStrumbarAnchorMesh, 0, 90.6994f);
  const HelpbarSpacing spacing = helpbar_spacing_from_panel(helpbar);
  // The first and strumbar icon anchors are authored in helpbar.milo. Stock
  // set_display puts fret2/fret3 in the next populated fret slot between them.
  // HelpBarPanel spacing is authored in widget units; this scale keeps the
  // current PS2-settled world placement while making the DTB source explicit.
  constexpr float kPs2FretSlotWorldPerButtonSpacing = 135.0f / 35.0f;
  const float kRuntimeFretSlotStep =
      spacing.button_spacing * kPs2FretSlotWorldPerButtonSpacing;
  const float middle_icon_left = first_icon_left + kRuntimeFretSlotStep;
  auto x_for_control = [&](const std::string& control) {
    // Runtime helpbar population offsets the label anchors by the authored
    // HelpBarPanel spacing plus a small residual from the PS2-settled footer
    // label boxes after applying help_bar.txt's font and text size.
    constexpr float kFirstFretTextResidual = 5.9153f;
    constexpr float kMiddleFretTextResidual = 7.5823f;
    constexpr float kStrumTextResidual = 4.3006f;
    if (control == "fret1" || control == "start")
      return first_icon_left + spacing.text_spacing + kFirstFretTextResidual;
    if (control == "fret2" || control == "fret3")
      return middle_icon_left + spacing.text_spacing + kMiddleFretTextResidual;
    if (control == "strum")
      return strum_icon_left + spacing.strumbar_spacing + kStrumTextResidual;
    return tuning.footer_x;
  };
  auto tex_for_control = [](const std::string& control) -> const char* {
    if (control == "fret1") return "hb_fret1.tex";
    if (control == "fret2") return "hb_fret2.tex";
    if (control == "fret3") return "hb_fret3.tex";
    if (control == "strum") return "hb_strum.tex";
    if (control == "start") return "hb_start.tex";
    return "";
  };
  auto source_icon_left = [&](const std::string& control) {
    if (control == "fret1" || control == "start") return first_icon_left;
    if (control == "fret2" || control == "fret3") return middle_icon_left;
    if (control == "strum") return strum_icon_left;
    return tuning.footer_x;
  };
  for (const HelpItem& item : items) {
    const float x = x_for_control(item.control);
    const bool strum = item.control == "strum";
    const float icon_left = source_icon_left(item.control);
    const float icon_w = strum ? 64.0f : 32.0f;
    const float icon_h = 32.0f;
    const float icon_x = icon_left + icon_w * 0.5f;
    const std::string label = help_label(item.token, locale);
    float label_w = 0.0f;
    text_font.layout(label, &label_w);
    label_w *= footer_text_scale_x;
    const float box_left = std::min(icon_x - icon_w * 0.5f, x) - 2.0f;
    const float box_right = std::max(icon_x + icon_w * 0.5f, x + label_w) + 14.0f;
    const float box_x = (box_left + box_right) * 0.5f;
    const float box_w = box_right - box_left;
    auto mid_it = icons.find("help_box_mid.tex");
    auto cap_it = icons.find("help_box_corner.tex");
    if (mid_it != icons.end() && mid_it->second.valid() &&
        cap_it != icons.end() && cap_it->second.valid()) {
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> box_verts;
      constexpr float kCapW = 16.0f;
      constexpr float kBoxH = 40.0f;
      append_image_quad_uv(box_x - box_w * 0.5f + kCapW * 0.5f, kFooterY,
                           kFooterZ + 4.0f, kCapW, kBoxH, 0.0f, 0.0f, 1.0f, 1.0f,
                           0xFFFFFFFFu, box_verts);
      append_image_quad_uv(box_x + box_w * 0.5f - kCapW * 0.5f, kFooterY,
                           kFooterZ + 4.0f, kCapW, kBoxH, 1.0f, 0.0f, 0.0f, 1.0f,
                           0xFFFFFFFFu, box_verts);
      batches.push_back({std::move(box_verts), &cap_it->second});
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> mid_verts;
      append_image_quad(box_x, kFooterY, kFooterZ + 4.0f, box_w - kCapW * 2.0f,
                        kBoxH, 0xFFFFFFFFu, mid_verts);
      batches.push_back({std::move(mid_verts), &mid_it->second});
    }
    const char* tex = tex_for_control(item.control);
    if (auto it = icons.find(tex); it != icons.end() && it->second.valid()) {
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> icon_verts;
      if (strum) {
        append_helpbar_mesh_quad_at(helpbar_scene, kStrumIconMesh, icon_left,
                                    kFooterZ, 0xFFFFFFFFu, icon_verts);
      } else {
        append_helpbar_mesh_quad_at(helpbar_scene, kFretIconMesh, icon_left,
                                    kFooterZ, 0xFFFFFFFFu, icon_verts);
      }
      if (icon_verts.empty()) {
        append_image_quad(icon_x, kFooterY, kFooterZ + 4.0f, icon_w, icon_h,
                          0xFFFFFFFFu, icon_verts);
      }
      batches.push_back({std::move(icon_verts), &it->second});
    }
    append_song_string_scaled(label, text_font, x, kFooterY, footer_text_z,
                              footer_text_scale_x, footer_text_scale_z,
                              footer_col, out);
  }
}

struct SongListEntry {
  bool header = false;
  std::string text;
  int song_pos = -1;
};

struct CreditRow {
  std::vector<std::string> cols;
};

std::string node_string_copy(const DataNode& n) {
  if (auto s = n.as_string()) return std::string(s->data(), s->size());
  if (auto sym = n.as_symbol()) return sym->c_str();
  return {};
}

bool credit_heading_text(const std::string& text) {
  bool has_alpha = false;
  bool has_lower = false;
  for (unsigned char ch : text) {
    if (std::isalpha(ch)) {
      has_alpha = true;
      if (std::islower(ch)) has_lower = true;
    }
  }
  return has_alpha && !has_lower;
}

void append_centered_string(
    const std::string& text, const MenuFont& font, float center_x, float y,
    float z, float scale, uint32_t col,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  float w = 0.0f;
  font.layout(text, &w);
  append_song_string(text, font, center_x - w * scale * 0.5f, y, z, scale, col,
                     out);
}

void append_right_aligned_string(
    const std::string& text, const MenuFont& font, float right_x, float y,
    float z, float scale, uint32_t col,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  float w = 0.0f;
  font.layout(text, &w);
  append_song_string(text, font, right_x - w * scale, y, z, scale, col, out);
}

std::string song_title_by_key(const ConfigDb& db, Symbol key) {
  for (std::size_t i = 0; i < db.song_count(); ++i) {
    if (db.song_key(i) != key) continue;
    std::string title(db.song_field(i, Symbol("name")).as_string().value_or(""));
    return title.empty() ? std::string(key.c_str()) : title;
  }
  return std::string(key.c_str());
}

std::string normalize_setlist_header(std::string header) {
  const size_t mark = header.find('\'');
  if (mark == std::string::npos || mark == 0) return header;
  bool digits = true;
  for (size_t i = 0; i < mark; ++i) {
    digits = digits && header[i] >= '0' && header[i] <= '9';
  }
  if (digits && mark + 1 < header.size() && header[mark + 1] == ' ')
    header[mark] = '.';
  return header;
}

std::vector<SongListEntry> song_list_entries(
    const ConfigDb& db, const std::map<std::string, std::string>& locale,
    bool quickplay) {
  (void)quickplay;
  std::vector<SongListEntry> out;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  int song_pos = 0;
  if (order) {
    for (std::size_t i = 1; i < order->size(); ++i) {
      auto tier = order->at(i).as_array();
      if (!tier || tier->empty()) continue;
      Symbol tier_name = tier->at(0).as_symbol().value_or(Symbol());
      std::string header_key = std::string("song_header_") + tier_name.c_str();
      std::string header = header_key;
      if (auto it = locale.find(header_key); it != locale.end()) header = it->second;
      header = normalize_setlist_header(std::move(header));
      out.push_back({true, header, -1});
      for (std::size_t j = 1; j < tier->size(); ++j) {
        Symbol song = tier->at(j).as_symbol().value_or(Symbol());
        if (!song.valid()) continue;
        out.push_back({false, song_title_by_key(db, song), song_pos++});
      }
    }
  }
  if (!out.empty()) return out;

  for (std::size_t i = 0; i < db.song_count(); ++i) {
    std::string title(db.song_field(i, Symbol("name")).as_string().value_or(""));
    if (title.empty()) title = db.song_key(i).c_str();
    out.push_back({false, title, static_cast<int>(i)});
  }
  return out;
}

int display_row_for_song(const std::vector<SongListEntry>& entries, int selected_song) {
  for (std::size_t i = 0; i < entries.size(); ++i)
    if (!entries[i].header && entries[i].song_pos == selected_song) return static_cast<int>(i);
  return 0;
}

int song_count_in_entries(const std::vector<SongListEntry>& entries) {
  int count = 0;
  for (const SongListEntry& entry : entries) {
    if (!entry.header) count = std::max(count, entry.song_pos + 1);
  }
  return count;
}

int display_start_for_song_window(const std::vector<SongListEntry>& entries,
                                  int first_song) {
  int first_display = display_row_for_song(entries, first_song);
  while (first_display > 0 && !entries[static_cast<std::size_t>(first_display)].header)
    --first_display;
  return first_display;
}

int display_end_for_song_window(const std::vector<SongListEntry>& entries,
                                int first_display, int first_song,
                                int last_song) {
  bool saw_song_in_window = false;
  bool saw_last_song = false;
  for (int i = first_display; i < static_cast<int>(entries.size()); ++i) {
    const SongListEntry& entry = entries[static_cast<std::size_t>(i)];
    if (entry.header) {
      if (saw_last_song) return i + 1;
      continue;
    }
    if (entry.song_pos < first_song) continue;
    if (entry.song_pos > last_song) return i;
    saw_song_in_window = true;
    saw_last_song = entry.song_pos == last_song;
  }
  return saw_song_in_window ? static_cast<int>(entries.size()) : first_display;
}

void append_song_list(const std::string& hdr, const std::string& ark,
                      ScreenManager& mgr, Object* screen, const ConfigDb& db,
                      const std::map<std::string, std::string>& locale,
                      const MenuFont& font, const MenuLayoutTuning& tuning,
                      float screen_origin_z,
                      std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  Object* list = mgr.resolve_object(Symbol("ss_song.lst"));
  Object* panel = mgr.find_object(Symbol("sel_song_panel"));
  int selected = 0;
  if (list) selected = list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
  else if (panel) selected = panel->get_property(Symbol("ss_song_selected")).as_int().value_or(0);
  if (selected < 0) selected = 0;

  const std::string list_milo = song_list_milo_path(mgr);
  const UiListLayout list_layout =
      extract_ui_list_layout(hdr, ark, list_milo, "ss_song.lst");
  if (!list_layout.valid || list_layout.provider.empty()) return;
  const std::string template_milo =
      ui_list_resource_milo_path(hdr, ark, list_layout.provider);
  if (template_milo.empty()) return;
  const UiListTemplateLayout list_template =
      extract_ui_list_template_layout(hdr, ark, template_milo);
  // ss_song.lst is the authored list layout. Stock PS2 treats tier headers and
  // songs as the same row stream: one provider entry per ruled paper line.
  const int kVisibleRows =
      list_layout.valid ? std::max(1, list_layout.visible_slots) : 5;
  const float parent_scale = tuning.setlist_parent_scale;
  const float authored_x =
      list_layout.valid ? list_layout.world_x : tuning.setlist_base_x;
  const float authored_row_h =
      list_layout.valid ? list_layout.row_height : tuning.setlist_row_h;
  const float authored_base_z =
      list_layout.valid ? (list_layout.world_z + list_layout.text_height)
                        : (screen_origin_z + tuning.setlist_base_z);
  const float authored_text_scale =
      (list_layout.valid && list_layout.text_height > 0.0f &&
       font.cap_height() > 0.0f)
          ? (list_layout.text_height / font.cap_height())
          : tuning.setlist_text_scale;
  const float authored_song_text_scale =
      (list_template.list.valid && list_template.list.text_size > 0.0f &&
       font.cap_height() > 0.0f)
          ? (list_template.list.text_size / font.cap_height()) *
                kSetlistListTextRendererScale
          : authored_text_scale;
  const float authored_header_text_scale =
      (list_template.header.valid && list_template.header.text_size > 0.0f &&
       font.cap_height() > 0.0f)
          ? (list_template.header.text_size / font.cap_height()) *
                kSetlistHeaderTextRendererScale
          : authored_text_scale;
  const float kBaseX = tuning.setlist_parent_x +
                       authored_or_tuned(tuning.setlist_base_x, 25.0f,
                                         authored_x) * parent_scale;
  constexpr float kBaseY = 0.0f;
  const float kBaseZ = tuning.setlist_parent_z +
                       authored_or_tuned(tuning.setlist_base_z, 10.0f,
                                         authored_base_z) * parent_scale;
  const float kRowH = authored_or_tuned(tuning.setlist_row_h, 40.0f,
                                        authored_row_h) * parent_scale;
  const float kSongTextScale =
      authored_or_tuned(tuning.setlist_text_scale, 0.950f,
                        authored_song_text_scale) *
      parent_scale;
  const float kHeaderTextScale =
      authored_or_tuned(tuning.setlist_header_scale, 0.900f,
                        authored_header_text_scale) *
      parent_scale;
  const float authored_header_x =
      list_template.header.valid ? list_template.header.world_x
                                 : tuning.setlist_header_x;
  const float authored_header_z =
      (list_template.header.valid && list_layout.valid)
          ? (list_template.header.world_z - list_layout.text_height +
             kSetlistPcsx2HeaderTextZLift)
          : tuning.setlist_header_z;
  const float authored_song_x =
      list_template.list.valid
          ? (list_template.list.world_x + kSetlistPcsx2ListTextXOffset)
          : tuning.setlist_song_x;
  const float authored_song_z =
      list_template.list.valid
          ? (list_template.list.world_z + kSetlistPcsx2ListTextZLift)
          : tuning.setlist_song_z;

  bool quickplay = true;
  if (screen) quickplay = !menu_truthy(screen->get_property(Symbol("sel_song_career")));
  if (Object* provider = mgr.resolve_object(Symbol("song_provider"))) {
    DataNode q = provider->handle_property(Symbol("get_quickplay"), DataArray());
    if (!q.empty()) quickplay = menu_truthy(q);
  }

  static bool logged_layout = false;
  if (!logged_layout && list_layout.valid) {
    logged_layout = true;
    std::fprintf(stderr,
                 "[menu] ss_song.lst layout: milo=%s template=%s quickplay=%d "
                 "local=(%.1f %.1f) world=(%.1f %.1f) "
                 "rows=%d row_h=%.1f text_h=%.1f width=%d scale=%.3f\n",
                 list_milo.c_str(), template_milo.c_str(), quickplay ? 1 : 0,
                 list_layout.local_x, list_layout.local_z,
                 list_layout.world_x, list_layout.world_z, kVisibleRows,
                 list_layout.row_height, list_layout.text_height,
                 list_layout.width_bound, authored_text_scale);
    if (list_template.valid) {
      std::fprintf(stderr,
                   "[menu] song-list template: header=(%.1f %.1f) "
                   "list=(%.1f %.1f) size=(%.1f %.1f) "
                   "wrap=(%.1f %.1f) field14=(%.2f %.2f) flags=(0x%08x 0x%08x) "
                   "resolved=(hx %.1f hz %.1f sx %.1f sz %.1f)\n",
                   list_template.header.world_x, list_template.header.world_z,
                   list_template.list.world_x, list_template.list.world_z,
                   list_template.header.text_size,
                   list_template.list.text_size,
                   list_template.header.wrap_width,
                   list_template.list.wrap_width,
                   list_template.header.field_14,
                   list_template.list.field_14,
                   list_template.header.flags,
                   list_template.list.flags,
                   authored_header_x, authored_header_z,
                   authored_song_x, authored_song_z);
    }
  }

  std::vector<SongListEntry> entries = song_list_entries(db, locale, quickplay);
  const int total_songs = song_count_in_entries(entries);
  if (total_songs <= 0 || entries.empty()) return;
  const int selected_display =
      std::clamp(display_row_for_song(entries, selected), 0,
                 static_cast<int>(entries.size()) - 1);
  const int stock_frame =
      std::clamp(static_cast<int>(std::lround(
                     setlist_runtime_frame(mgr, selected_display))),
                 0, static_cast<int>(entries.size()) - 1);
  constexpr int kSelectedScreenRow = 1;
  const int visible_display_rows = std::max(1, kVisibleRows + 1);
  const int max_first_display =
      std::max(0, static_cast<int>(entries.size()) - visible_display_rows);
  int first_display =
      std::clamp(stock_frame - kSelectedScreenRow, 0, max_first_display);
  int last_display =
      std::min(static_cast<int>(entries.size()),
               first_display + visible_display_rows);

  for (int row = 0, ei = first_display; ei < last_display; ++row, ++ei) {
    const SongListEntry& e = entries[ei];
    float rz = kBaseZ - row * kRowH;
    if (e.header) {
      append_song_string(e.text, font,
                         kBaseX + authored_or_tuned(tuning.setlist_header_x,
                                                    -294.0f,
                                                    authored_header_x) *
                                      parent_scale,
                         kBaseY,
                         rz + authored_or_tuned(tuning.setlist_header_z,
                                                -24.0f,
                                                authored_header_z) *
                                  parent_scale,
                         kHeaderTextScale,
                         0xFFB30000u, out);
    } else {
      const bool foc = (e.song_pos == selected);
      uint32_t title_col = foc ? 0xFF0000FFu : 0xFF1A1A1Au;
      float song_x =
          kBaseX + authored_or_tuned(tuning.setlist_song_x, -260.0f,
                                     authored_song_x) *
                       parent_scale;
      float song_z =
          rz + authored_or_tuned(tuning.setlist_song_z, -19.0f,
                                 authored_song_z) *
                   parent_scale;
      float song_scale = kSongTextScale;
      if (foc) {
        song_x += tuning.setlist_selected_x * parent_scale;
        song_z += tuning.setlist_selected_z * parent_scale;
        song_scale *= tuning.setlist_selected_scale;
      }
      append_song_string(e.text, font, song_x, kBaseY, song_z, song_scale,
                         title_col, out);
    }
  }
}

uint32_t template_color(const UiListTextTemplate& t, uint32_t fallback) {
  if (!t.valid) return fallback;
  auto ch = [](float v) -> uint32_t {
    return static_cast<uint32_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
  };
  return (ch(t.color[3]) << 24) | (ch(t.color[0]) << 16) |
         (ch(t.color[1]) << 8) | ch(t.color[2]);
}

Object* provider_from_node(ScreenManager& mgr, const DataNode& node) {
  if (Object* obj = node.as_object()) return obj;
  if (auto sym = node.as_symbol()) return mgr.resolve_object(*sym);
  if (auto text = node.as_string())
    return mgr.resolve_object(Symbol(std::string(*text).c_str()));
  return nullptr;
}

void append_practice_section_list(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen, const std::map<std::string, std::string>& locale,
    const MenuFont& font,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  Object* list = mgr.resolve_object(Symbol("sel_section.lst"));
  if (!list) return;
  Object* provider = provider_from_node(mgr, list->get_property(Symbol("provider")));
  if (!provider) provider = mgr.resolve_object(Symbol("section_provider"));
  if (!provider) return;

  std::string list_milo;
  if (screen_has_panel(screen, Symbol("practice_sel_section_panel"))) {
    Object* panel = mgr.find_object(Symbol("practice_sel_section_panel"));
    list_milo = panel_milo_path(panel_file(panel));
  }
  if (list_milo.empty()) return;

  const UiListLayout layout =
      extract_ui_list_layout(hdr, ark, list_milo, "sel_section.lst");
  if (!layout.valid || layout.provider.empty()) return;
  const std::string template_milo =
      ui_list_resource_milo_path(hdr, ark, layout.provider);
  if (template_milo.empty()) return;
  const UiListTextTemplate item_template = extract_text_template_layout(
      hdr, ark, template_milo, "list.txt");
  if (!item_template.valid) return;

  const int total = std::max(0, provider->handle_property(
      Symbol("list_length"), DataArray()).as_int().value_or(0));
  if (total <= 0) return;
  int selected = list->handle_property(Symbol("selected_pos"), DataArray())
                     .as_int().value_or(0);
  selected = std::clamp(selected, 0, total - 1);
  const int slots = layout.visible_slots > 0 ? layout.visible_slots : 6;
  const int first = std::clamp(selected - slots / 2, 0,
                               std::max(0, total - slots));
  const int last = std::min(total, first + slots);
  const float row_h = layout.row_height > 0.0f ? layout.row_height : 32.0f;
  const float scale =
      item_template.valid && item_template.text_size > 0.0f &&
              font.cap_height() > 0.0f
          ? item_template.text_size / font.cap_height()
          : 0.7f;
  const float x = (layout.valid ? layout.world_x : -245.0f) +
                  (item_template.valid ? item_template.world_x : 0.0f);
  const float z0 = (layout.valid ? layout.world_z : 125.0f) +
                   (item_template.valid ? item_template.world_z : 0.0f);
  const uint32_t base_col = template_color(item_template, 0xFF1A1A1Au);

  for (int row = 0, index = first; index < last; ++row, ++index) {
    DataArray args;
    args.push(DataNode::Int(index));
    std::string key = menu_node_text(
        provider->handle_property(Symbol("get_text"), args));
    if (key.empty()) {
      key = menu_node_text(provider->handle_property(Symbol("get_symbol"), args));
    }
    if (key.empty()) continue;
    const uint32_t col = index == selected ? 0xFFB30000u : base_col;
    append_song_string(resolve_menu_text(key, locale), font, x, 0.0f,
                       z0 - row * row_h, scale, col, out);
  }

  static bool logged = false;
  if (!logged) {
    logged = true;
    std::fprintf(stderr,
                 "[menu] sel_section.lst layout: milo=%s template=%s "
                 "world=(%.1f %.1f) rows=%d row_h=%.1f "
                 "template_xz_size=(%.1f %.1f %.1f) sections=%d\n",
                 list_milo.c_str(), template_milo.c_str(), layout.world_x,
                 layout.world_z, slots, row_h,
                 item_template.world_x, item_template.world_z,
                 item_template.text_size, total);
  }
}

void append_stats_template_text(
    const std::string& text, const UiListTextTemplate& text_template,
    float base_x, float row_z, const std::map<std::string, std::string>& locale,
    bool localize_text, uint32_t color,
    std::vector<MenuTextTarget>& targets) {
  if (text.empty() || !text_template.valid) return;
  MenuTextTarget* target =
      menu_text_target_for_font(targets, text_template.font);
  if (!target || !target->font || !target->font->valid()) return;
  const MenuFont& font = *target->font;
  const float scale =
      (text_template.text_size > 0.0f && font.cap_height() > 0.0f)
          ? text_template.text_size / font.cap_height()
          : 0.7f;
  const std::string display =
      localize_text ? resolve_menu_text(text, locale) : text;
  append_song_string(display, font, base_x + text_template.world_x, 0.0f,
                     row_z + text_template.world_z, scale, color,
                     target->verts);
}

void append_stats_sections_list(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen, const std::map<std::string, std::string>& locale,
    std::vector<MenuTextTarget>& targets) {
  std::string list_milo;
  UiListLayout layout;
  if (!screen_ui_list_layout(hdr, ark, mgr, screen, "stats_sections.lst",
                             list_milo, layout) ||
      layout.provider.empty()) {
    return;
  }

  Object* list = mgr.resolve_object(Symbol("stats_sections.lst"));
  Object* provider =
      list ? provider_from_node(mgr, list->get_property(Symbol("provider")))
           : nullptr;
  if (!provider) provider = mgr.resolve_object(Symbol("stats_provider"));
  if (!provider) return;

  const std::string template_milo =
      ui_list_resource_milo_path(hdr, ark, layout.provider);
  if (template_milo.empty()) return;
  const UiListTextTemplate section_template = extract_text_template_layout(
      hdr, ark, template_milo, "section.txt");
  const UiListTextTemplate notes1_template = extract_text_template_layout(
      hdr, ark, template_milo, "notes1.txt");
  if (!section_template.valid || !notes1_template.valid) return;

  const int total = std::max(0, provider->handle_property(
      Symbol("list_length"), DataArray()).as_int().value_or(0));
  if (total <= 0) return;
  int selected = 0;
  if (list) {
    selected = list->handle_property(Symbol("selected_pos"), DataArray())
                   .as_int()
                   .value_or(0);
  }
  selected = std::clamp(selected, 0, total - 1);
  const int slots = layout.visible_slots > 0 ? layout.visible_slots : 6;
  const int first = std::clamp(selected - slots / 2, 0,
                               std::max(0, total - slots));
  const int last = std::min(total, first + slots);
  const float base_x = layout.world_x;
  const float base_z = layout.world_z;
  const float row_h = layout.row_height > 0.0f ? layout.row_height : 30.0f;

  for (int row = 0, index = first; index < last; ++row, ++index) {
    DataArray row_arg;
    row_arg.push(DataNode::Int(index));
    DataNode section_node = provider->handle_property(
        Symbol("get_section"), row_arg);
    if (section_node.empty())
      section_node = provider->handle_property(Symbol("get_symbol"), row_arg);
    std::string section = menu_node_text(section_node);

    DataNode notes_node = provider->handle_property(Symbol("get_notes1"), row_arg);
    if (notes_node.empty()) {
      DataArray text_arg;
      text_arg.push(DataNode::Int(index));
      text_arg.push(DataNode::Int(1));
      notes_node = provider->handle_property(Symbol("get_text"), text_arg);
    }
    const std::string notes = menu_node_text(notes_node);
    const float row_z = base_z - static_cast<float>(row) * row_h;
    // ui_objects.dta::LIST_STATS_COLORS normal/focused = (0.1 0.1 0.1).
    constexpr uint32_t kStatsNormalColor = 0xFF1A1A1Au;
    append_stats_template_text(section, section_template, base_x, row_z,
                               locale, true, kStatsNormalColor, targets);
    append_stats_template_text(notes, notes1_template, base_x, row_z,
                               locale, false, kStatsNormalColor, targets);
  }

  static bool logged = false;
  if (!logged) {
    logged = true;
    std::fprintf(stderr,
                 "[menu] stats_sections.lst layout: milo=%s template=%s "
                 "world=(%.1f %.1f) rows=%d row_h=%.1f provider=%s "
                 "items=%d\n",
                 list_milo.c_str(), template_milo.c_str(), layout.world_x,
                 layout.world_z, slots, row_h, layout.provider.c_str(),
                 total);
  }
}

std::vector<CreditRow> credit_rows(const ConfigDb& db) {
  std::vector<CreditRow> out;
  const DataArray* credits = db.table(Symbol("credits"));
  if (!credits) return out;
  out.reserve(credits->size());
  for (std::size_t i = 0; i < credits->size(); ++i) {
    CreditRow row;
    if (auto arr = credits->at(i).as_array()) {
      row.cols.reserve(arr->size());
      for (std::size_t c = 0; c < arr->size(); ++c)
        row.cols.push_back(node_string_copy(arr->at(c)));
    }
    out.push_back(std::move(row));
  }
  return out;
}

void append_credits_list(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen, const ConfigDb& db,
    const MenuFont& clarendon_font, const MenuFont& rockletters_font,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& clarendon_out,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& rockletters_out) {
  if (!clarendon_font.valid() || !rockletters_font.valid()) return;
  std::vector<CreditRow> rows = credit_rows(db);
  if (rows.empty()) return;

  std::string credits_milo;
  UiListLayout layout;
  if (!screen_ui_list_layout(hdr, ark, mgr, screen, "credits.lst", credits_milo,
                             layout) ||
      layout.provider.empty()) {
    return;
  }
  const std::string template_milo =
      ui_list_resource_milo_path(hdr, ark, layout.provider);
  if (template_milo.empty()) return;
  const UiListTextTemplate title_template =
      extract_text_template_layout(hdr, ark, template_milo, "title.txt");
  const UiListTextTemplate name_template =
      extract_text_template_layout(hdr, ark, template_milo, "name.txt");
  const UiListTextTemplate center_template =
      extract_text_template_layout(hdr, ark, template_milo, "center.txt");
  const UiListTextTemplate center_name_template =
      extract_text_template_layout(hdr, ark, template_milo, "centername.txt");

  const float base_x = layout.valid ? layout.world_x : 0.0f;
  const float base_z = layout.valid ? layout.world_z : 186.0f;
  const float row_h = layout.row_height > 0.0f ? layout.row_height : 30.0f;
  const float title_x = title_template.valid ? title_template.world_x : -10.0f;
  const float name_x = name_template.valid ? name_template.world_x : 10.0f;
  const float title_scale =
      (title_template.valid && title_template.text_size > 0.0f)
          ? title_template.text_size / clarendon_font.cap_height()
          : 20.0f / clarendon_font.cap_height();
  const float name_scale =
      (name_template.valid && name_template.text_size > 0.0f)
          ? name_template.text_size / clarendon_font.cap_height()
          : title_scale;
  const float center_scale =
      (center_template.valid && center_template.text_size > 0.0f)
          ? center_template.text_size / rockletters_font.cap_height()
          : 30.0f / rockletters_font.cap_height();
  const float center_name_scale =
      (center_name_template.valid && center_name_template.text_size > 0.0f)
          ? center_name_template.text_size / clarendon_font.cap_height()
          : name_scale;

  std::size_t first = 0;
  if (Object* list = mgr.resolve_object(Symbol("credits.lst"))) {
    const int max_row =
        rows.empty() ? 0 : static_cast<int>(rows.size() - 1);
    first = static_cast<std::size_t>(std::clamp(
        list->handle_property(Symbol("selected_pos"), DataArray())
            .as_int()
            .value_or(0),
        0, max_row));
  } else {
    while (first < rows.size()) {
      bool any = false;
      for (const std::string& col : rows[first].cols) any = any || !col.empty();
      if (any) break;
      ++first;
    }
  }
  const std::size_t visible_rows =
      layout.visible_slots > 0 ? static_cast<std::size_t>(layout.visible_slots)
                               : 16u;
  static bool logged = false;
  if (!logged) {
    logged = true;
    std::fprintf(stderr,
                 "[menu] credits.lst layout: milo=%s template=%s "
                 "world=(%.1f %.1f) first=%zu rows=%zu row_h=%.1f "
                 "provider=%s\n",
                 credits_milo.c_str(), template_milo.c_str(), base_x, base_z,
                 first, visible_rows, row_h, layout.provider.c_str());
  }
  constexpr uint32_t kCreditCol = 0xFFFFFFFFu;
  for (std::size_t row = 0; row < visible_rows && first + row < rows.size();
       ++row) {
    const CreditRow& cr = rows[first + row];
    const float z = base_z - static_cast<float>(row) * row_h;
    if (cr.cols.empty()) continue;
    if (cr.cols.size() == 1) {
      const std::string& text = cr.cols[0];
      if (text.empty()) continue;
      if (credit_heading_text(text)) {
        append_centered_string(text, rockletters_font, base_x, 0.0f, z,
                               center_scale, kCreditCol, rockletters_out);
      } else {
        append_centered_string(text, clarendon_font, base_x, 0.0f, z,
                               center_name_scale, kCreditCol, clarendon_out);
      }
      continue;
    }

    bool side_empty = cr.cols.size() >= 3 && cr.cols.front().empty() &&
                      cr.cols.back().empty();
    if (side_empty) {
      const std::string& text = cr.cols[1];
      if (!text.empty())
        append_centered_string(text, clarendon_font, base_x, 0.0f, z,
                               center_name_scale, kCreditCol, clarendon_out);
      continue;
    }

    const std::string role = cr.cols.size() > 0 ? cr.cols[0] : std::string();
    const std::string name = cr.cols.size() > 1 ? cr.cols[1] : std::string();
    if (!role.empty())
      append_right_aligned_string(role, clarendon_font, base_x + title_x,
                                  0.0f, z, title_scale, kCreditCol,
                                  clarendon_out);
    if (!name.empty())
      append_song_string(name, clarendon_font, base_x + name_x, 0.0f, z,
                         name_scale, kCreditCol, clarendon_out);
  }
}

// Items the original would disable: the main panel poll disables multiplayer when
// `game is_missing_multi_controller` (main.dta). We evaluate that game condition.
std::unordered_set<std::string> compute_disabled(ScreenManager& mgr) {
  std::unordered_set<std::string> d;
  // `game` is a singleton -> resolve_object (find_object only checks the screen
  // registry, so it would miss the singletons and never disable anything).
  if (Object* g = mgr.resolve_object(Symbol("game"))) {
    DataNode mm = g->handle_property(Symbol("is_missing_multi_controller"), DataArray());
    bool missing = false;
    if (auto s = mm.as_symbol()) missing = (std::strcmp(s->c_str(), "TRUE") == 0);
    if (auto i = mm.as_int()) missing = missing || (*i != 0);
    if (missing) d.insert("main_multiplayer.btn");
  }
  return d;
}

// All text-bearing objects of the current screen's panels (for focus nav).
std::vector<MenuLabel> gather_labels(const std::string& hdr, const std::string& ark,
                                     ScreenManager& mgr, Object* screen) {
  std::vector<MenuLabel> out;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_active_for_render(panel)) continue;
    std::string file = panel_file(panel);
    if (file.empty()) continue;
    auto labels = extract_menu_labels(hdr, ark, panel_milo_path(file));
    for (auto& l : labels) out.push_back(std::move(l));
  }
  return out;
}

bool screen_has_helpbar(Object* screen) {
  if (!screen) return false;
  std::vector<HelpItem> items;
  collect_help_tokens(screen->get_property(Symbol("helpbar")), items);
  return !items.empty();
}

std::string tune_label_for_button(
    const MenuLabel& lbl, const std::map<std::string, std::string>& locale,
    const std::string& parent) {
  std::string text = resolve_menu_text(lbl.text, locale);
  if (text.empty()) text = lbl.name;
  return parent.empty() ? text : parent + " > " + text;
}

void add_button_tune_targets(std::vector<MenuTuneTarget>& out,
                             const std::vector<MenuLabel>& labels,
                             const std::map<std::string, std::string>& locale,
                             const std::string& parent) {
  std::unordered_set<std::string> seen;
  for (const MenuLabel& lbl : labels) {
    if (lbl.type != "BandButton" || lbl.name.empty()) continue;
    if (!seen.insert(lbl.name).second) continue;
    out.push_back({MenuTuneKind::Button,
                   tune_label_for_button(lbl, locale, parent),
                   lbl.name});
  }
}

std::vector<MenuTuneTarget> build_menu_tune_targets(
    Object* screen, const std::vector<MenuLabel>& labels,
    const std::map<std::string, std::string>& locale) {
  std::vector<MenuTuneTarget> out;
  out.push_back({MenuTuneKind::Camera, "camera", "camera"});
  const std::string screen_name = screen ? screen->name().c_str() : "";
  const bool main_menu = screen_name == "main_screen";
  const bool song_list = screen_name == "qp_selsong_screen";

  if (main_menu) {
    out.push_back({MenuTuneKind::MainButtons, "main buttons parent",
                   "main_buttons"});
    add_button_tune_targets(out, labels, locale, "main buttons");
  } else if (song_list) {
    out.push_back({MenuTuneKind::SetlistGroup, "setlist parent",
                   "setlist"});
    out.push_back({MenuTuneKind::SetlistRows, "setlist > row stack",
                   "setlist_rows"});
    out.push_back({MenuTuneKind::SetlistHeaders, "setlist > rows > tier headers",
                   "setlist_headers"});
    out.push_back({MenuTuneKind::SetlistSongs, "setlist > rows > song titles",
                   "setlist_songs"});
    out.push_back({MenuTuneKind::SetlistSelected, "setlist > rows > selected row",
                   "setlist_selected"});
    out.push_back({MenuTuneKind::SetlistSpacing, "setlist > rows > row spacing",
                   "setlist_spacing"});
  } else {
    add_button_tune_targets(out, labels, locale, "buttons");
  }

  if (screen_has_helpbar(screen)) {
    out.push_back({MenuTuneKind::Footer, "help footer", "footer"});
  }
  return out;
}

void log_menu_tune_targets(Object* screen,
                           const std::vector<MenuTuneTarget>& targets) {
  std::fprintf(stderr, "[menu-tune] %s targets:", screen ? screen->name().c_str() : "?");
  for (const MenuTuneTarget& target : targets)
    std::fprintf(stderr, " %s;", target.label.c_str());
  std::fprintf(stderr, "\n");
}

std::string focused_component_or_stock_list(Object* panel) {
  if (!panel) return {};
  std::string focus =
      panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol()).c_str();
  if (!focus.empty()) return focus;
  auto* dir = dynamic_cast<ObjectDir*>(panel);
  if (dir && dir->find(Symbol("credits.lst"))) return "credits.lst";
  return {};
}

// Move focus down (dir>0) / up (dir<0) along the BandButton nav links, skipping
// disabled items. Sets the focused panel's (focus) property to the new component.
void focus_move(ScreenManager& mgr, const std::vector<MenuLabel>& labels,
                const std::unordered_set<std::string>& disabled, int dir,
                std::size_t song_count) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol fpn = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = fpn.valid() ? mgr.find_object(fpn) : nullptr;
  if (!panel) return;
  std::string cur = focused_component_or_stock_list(panel);
  if (cur == "ss_song.lst") {
    Symbol stored("ss_song_selected");
    Symbol frame_prop("sel_song_frame");
    auto stock_frame_for = [&](int pos) {
      int headers = 0;
      if (Object* provider = mgr.resolve_object(Symbol("song_provider"))) {
        DataArray args;
        args.push(DataNode::Int(pos));
        headers = provider->handle_property(Symbol("num_headers"), args)
                      .as_int()
                      .value_or(0);
      }
      return pos + headers;
    };
    if (Object* list = mgr.resolve_object(Symbol("ss_song.lst"))) {
      int pos = list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
      const int old_pos = pos;
      pos += dir;
      int max_pos = song_count > 0 ? static_cast<int>(song_count - 1) : 0;
      if (pos < 0) pos = 0;
      if (pos > max_pos) pos = max_pos;
      if (pos == old_pos) return;
      list->handle_property(Symbol("set_selected"), one_arg(DataNode::Int(pos)));
      panel->set_property(stored, DataNode::Int(pos));
      panel->set_property(frame_prop, DataNode::Int(stock_frame_for(pos)));
      if (Object* game = mgr.resolve_object(Symbol("game")))
        game->handle_property(Symbol("set_song_index"), one_arg(DataNode::Int(pos)));
      mgr.run_global_handler(Symbol("SCROLL_MSG"));
      panel->handle_property(Symbol("SCROLL_MSG"), DataArray());
    } else {
      int old_pos = panel->get_property(stored).as_int().value_or(0);
      int pos = old_pos + dir;
      int max_pos = song_count > 0 ? static_cast<int>(song_count - 1) : 0;
      if (pos < 0) pos = 0;
      if (pos > max_pos) pos = max_pos;
      if (pos == old_pos) return;
      panel->set_property(stored, DataNode::Int(pos));
      panel->set_property(frame_prop, DataNode::Int(stock_frame_for(pos)));
      if (Object* game = mgr.resolve_object(Symbol("game")))
        game->handle_property(Symbol("set_song_index"), one_arg(DataNode::Int(pos)));
      mgr.run_global_handler(Symbol("SCROLL_MSG"));
      panel->handle_property(Symbol("SCROLL_MSG"), DataArray());
    }
    return;
  }
  if (cur == "credits.lst") {
    Object* list = mgr.resolve_object(Symbol("credits.lst"));
    if (!list) return;
    const int lines =
        std::max(0, screen->handle_property(Symbol("num_lines"), DataArray())
                        .as_int()
                        .value_or(0));
    if (lines <= 0) return;
    int pos = list->handle_property(Symbol("selected_pos"), DataArray())
                  .as_int()
                  .value_or(0);
    const int old_pos = pos;
    pos = std::clamp(pos + dir, 0, lines - 1);
    if (pos == old_pos) return;
    list->handle_property(Symbol("set_selected"), one_arg(DataNode::Int(pos)));
    mgr.run_global_handler(Symbol("SCROLL_MSG"));
    screen->handle_property(Symbol("SCROLL_MSG"), DataArray());
    return;
  }
  for (size_t guard = 0; guard <= labels.size(); ++guard) {
    std::string next;
    if (dir > 0) {
      for (const auto& l : labels) if (l.name == cur) { next = l.nav; break; }
    } else {
      for (const auto& l : labels) if (!l.nav.empty() && l.nav == cur) { next = l.name; break; }
    }
    if (next.empty()) return;
    if (!disabled.count(next)) {
      mgr.set_global(Symbol("old_focus"), DataNode::Sym(Symbol(cur.c_str())));
      mgr.set_global(Symbol("new_focus"), DataNode::Sym(Symbol(next.c_str())));
      panel->set_property(Symbol("focus"), DataNode::Sym(Symbol(next.c_str())));
      panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
      mgr.run_global_handler(Symbol("FOCUS_MSG"));
      return;
    }
    cur = next;  // disabled -> keep moving in the same direction
  }
}

// Rebuild the renderer's text overlay from the current screen's panels.
void rebuild_text(const std::string& hdr, const std::string& ark, ScreenManager& mgr,
                  Object* screen, ghogx::render::MiloSceneRenderer& renderer,
                  const MenuFont& font, const MenuFont& song_font,
                  const MenuFont& footer_font, const MenuFont& clarendon_font,
                  const MenuFont& rockletters_font, const ConfigDb& db,
                  const std::map<std::string, std::string>& locale,
                  const MenuLayoutTuning& tuning,
                  const std::vector<MenuFontChoice>& menu_fonts) {
  // The focused component (screen.focus -> panel; panel.focus -> component) is
  // drawn in the focused colour (yellow).
  std::string focused;
  if (screen) {
    Symbol fpn = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
    if (Object* fp = fpn.valid() ? mgr.find_object(fpn) : nullptr) {
      Symbol fc = fp->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
      if (fc.valid()) focused = fc.c_str();
    }
  }
  std::unordered_set<std::string> disabled = compute_disabled(mgr);
  const float origin_z = screen_text_origin_z(screen, mgr, hdr, ark);
  std::vector<MenuTextTarget> text_targets;
  for (const MenuFontChoice& choice : menu_fonts) {
    if (choice.font) add_menu_text_target(text_targets, choice.key, *choice.font);
  }
  if (text_targets.empty()) add_menu_text_target(text_targets, "impact", font);
  std::vector<ghogx::render::MiloSceneRenderer::TextBatch> batches;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> footer_verts;
  std::vector<ghogx::render::MiloSceneRenderer::TextBatch> footer_batches;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> credit_clarendon_verts;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> credit_rockletters_verts;
  if (std::getenv("GHOGX_DISABLE_MENU_TEXT")) {
    renderer.set_post_text_mesh_text_split(0);
    renderer.set_text_batches({});
    return;
  }
  auto help_icons = asset::load_milo_textures(
      hdr, ark, "ui/gen/helpbar.milo_ps2",
      {"hb_fret1.tex", "hb_fret2.tex", "hb_fret3.tex", "hb_strum.tex", "hb_start.tex",
       "help_box_mid.tex", "help_box_corner.tex"});
  milo_scene::Scene helpbar_scene;
  milo_scene::load_scene(hdr, ark, "ui/gen/helpbar.milo_ps2", helpbar_scene);
  const UiListTextTemplate helpbar_template = extract_text_template_layout(
      hdr, ark, "ui/gen/helpbar.milo_ps2", "help_bar.txt");
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_active_for_render(panel)) continue;
    std::string file = panel_file(panel);
    if (file.empty()) continue;
    if (file == "helpbar.milo") continue;
    const MenuGroupGraph group_graph =
        load_menu_group_graph(hdr, ark, panel_milo_path(file));
    auto labels = extract_menu_labels(hdr, ark, panel_milo_path(file));
    if (!menu_env_string("GHOGX_DEBUG_MENU_GROUPS").empty()) {
      for (const auto& lbl : labels) {
        auto owners = group_graph.owners.find(lbl.name);
        std::fprintf(stderr, "[menu-groups] %s %s parent=%s owners=",
                     file.c_str(), lbl.name.c_str(), lbl.parent.c_str());
        if (owners != group_graph.owners.end()) {
          for (const auto& owner : owners->second)
            std::fprintf(stderr, "%s,", owner.c_str());
        }
        std::fprintf(stderr, "\n");
      }
    }
    append_text_quads(mgr, labels, text_targets, group_graph, locale, focused,
                      disabled, tuning);
  }
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> song_verts;
  if (screen_has_panel(screen, Symbol("sel_song_panel")) && song_font.valid())
    append_song_list(hdr, ark, mgr, screen, db, locale, song_font, tuning,
                     origin_z, song_verts);
  if (screen_has_panel(screen, Symbol("practice_sel_section_panel")) &&
      song_font.valid()) {
    append_practice_section_list(hdr, ark, mgr, screen, locale, song_font,
                                 song_verts);
  }
  if (screen_has_panel(screen, Symbol("endgame_stats_panel")))
    append_stats_sections_list(hdr, ark, mgr, screen, locale, text_targets);
  if (screen && screen->name() == Symbol("credits_screen"))
    append_credits_list(hdr, ark, mgr, screen, db, clarendon_font,
                        rockletters_font,
                        credit_clarendon_verts, credit_rockletters_verts);
  append_help_footer(mgr, screen, font, footer_font, locale, help_icons,
                     helpbar_scene, helpbar_template, tuning, origin_z, footer_verts,
                     footer_batches);
  size_t text_vert_count = 0;
  for (const auto& target : text_targets) text_vert_count += target.verts.size();
  std::fprintf(stderr, "[menu] focused component = '%s'\n", focused.c_str());
  std::fprintf(stderr,
               "[menu] text: %zu glyph-verts, song-list: %zu glyph-verts, "
               "credits: %zu/%zu glyph-verts\n",
               text_vert_count, song_verts.size(), credit_clarendon_verts.size(),
               credit_rockletters_verts.size());
  size_t pre_text_mesh_batches = 0;
  for (auto& target : text_targets) {
    if (!target.verts.empty() && target.font && target.font->valid()) {
      batches.push_back({std::move(target.verts), &target.font->atlas()});
      ++pre_text_mesh_batches;
    }
  }
  if (!song_verts.empty() && song_font.valid()) {
    batches.push_back({std::move(song_verts), &song_font.atlas()});
    ++pre_text_mesh_batches;
  }
  if (!credit_clarendon_verts.empty() && clarendon_font.valid())
    batches.push_back({std::move(credit_clarendon_verts),
                       &clarendon_font.atlas()});
  if (!credit_rockletters_verts.empty() && rockletters_font.valid())
    batches.push_back({std::move(credit_rockletters_verts),
                       &rockletters_font.atlas()});
  for (auto& batch : footer_batches) batches.push_back(std::move(batch));
  const MenuFont& footer_batch_font = footer_font.valid() ? footer_font : font;
  if (!footer_verts.empty())
    batches.push_back({std::move(footer_verts), &footer_batch_font.atlas()});
  renderer.set_post_text_mesh_text_split(pre_text_mesh_batches);
  renderer.set_text_batches(std::move(batches));
}

}  // namespace

int run_menu_mode(const std::string& hdr, const std::string& ark,
                  const MenuModeOptions& options) {
  force_menu_camera_aspect();

  // 1. Boot the menu logic engine: classes, all screens (verbatim), game-side.
  register_ui_classes();
  ScreenManager mgr;
  install_default_singletons(mgr);

  gh::ark::ArkV3Reader arkr = gh::ark::ArkV3Reader::load(hdr);
  std::vector<std::string> arks = {ark};
  mgr.set_transition_time(load_stock_transition_time(arkr, arks));
  std::fprintf(stderr, "[menu] transition_time: %.3fs\n",
               mgr.transition_time());
  int n = load_all_ui_screens(arkr, arks, mgr);
  ConfigDb db;
  db.load(arkr, arks);
  install_meta_singletons(mgr, db);
  install_milo_widget_objects(arkr, hdr, arks, mgr);
  std::fprintf(stderr, "[menu] booted: %d DTBs, %zu objects, %zu songs\n", n,
               mgr.registry().size(), db.song_count());

  // The menu bitmap font ("impact") + the locale (button labels are loc keys).
  MenuFont impact_font;
  impact_font.load(hdr, ark, "ui/gen/impact.milo_ps2");
  MenuFont song_font;
  song_font.load(hdr, ark, "ui/gen/dyingmarker.milo_ps2");
  // helpbar.milo_ps2::help_bar.txt names this RndFont for footer labels.
  MenuFont footer_font;
  footer_font.load(hdr, ark, "ui/gen/helveticablackcondensed.milo_ps2");
  MenuFont clarendon_font;
  clarendon_font.load(hdr, ark, "ui/gen/clarendon.milo_ps2");
  MenuFont rockletters_font;
  rockletters_font.load(hdr, ark, "ui/gen/rockletters.milo_ps2");
  MenuFont hand_font;
  hand_font.load(hdr, ark, "ui/gen/hand.milo_ps2");
  MenuFont cutout_font;
  cutout_font.load(hdr, ark, "ui/gen/cutout.milo_ps2");
  MenuFont gunsho_font;
  gunsho_font.load(hdr, ark, "ui/gen/gunsho.milo_ps2");
  MenuFont receipt_font;
  receipt_font.load(hdr, ark, "ui/gen/receipt.milo_ps2");
  MenuFont helvetica_black_font;
  helvetica_black_font.load(hdr, ark, "ui/gen/helveticablack.milo_ps2");
  MenuFont helvetica_thin_font;
  helvetica_thin_font.load(hdr, ark, "ui/gen/helveticathin.milo_ps2");
  MenuFont impactor_font;
  impactor_font.load(hdr, ark, "ui/gen/impactor.milo_ps2");
  MenuFont impactor2_font;
  impactor2_font.load(hdr, ark, "ui/gen/impactor2.milo_ps2");
  MenuFont impactor_mtv_font;
  impactor_mtv_font.load(hdr, ark, "ui/gen/impactor_mtv.milo_ps2");
  MenuFont rokk_font;
  rokk_font.load(hdr, ark, "ui/gen/rokk.milo_ps2");
  MenuFont tapeworm_font;
  tapeworm_font.load(hdr, ark, "ui/gen/tapeworm.milo_ps2");
  MenuFont tapewormscreen_font;
  tapewormscreen_font.load(hdr, ark, "ui/gen/tapewormscreen.milo_ps2");
  MenuFont serif_font;
  serif_font.load(hdr, ark, "ui/gen/serif.milo_ps2");
  MenuFont stars_font;
  stars_font.load(hdr, ark, "ui/gen/stars.milo_ps2");
  MenuFont blockletters_font;
  blockletters_font.load(hdr, ark, "ui/gen/blockletters.milo_ps2");
  MenuFont blockletters_fill_font;
  blockletters_fill_font.load(hdr, ark, "ui/gen/blockletters_fill.milo_ps2");
  const std::vector<MenuFontChoice> menu_fonts = {
      {"impact", &impact_font},
      {"dyingmarker", &song_font},
      {"helveticablackcondensed", &footer_font},
      {"clarendon", &clarendon_font},
      {"rockletters", &rockletters_font},
      {"hand", &hand_font},
      {"cutout", &cutout_font},
      {"gunsho", &gunsho_font},
      {"receipt", &receipt_font},
      {"helveticablack", &helvetica_black_font},
      {"helveticathin", &helvetica_thin_font},
      {"impactor", &impactor_font},
      {"impactor2", &impactor2_font},
      {"impactor_mtv", &impactor_mtv_font},
      {"rokk", &rokk_font},
      {"tapeworm", &tapeworm_font},
      {"tapewormscreen", &tapewormscreen_font},
      {"serif", &serif_font},
      {"stars", &stars_font},
      {"blockletters", &blockletters_font},
      {"blockletters_fill", &blockletters_fill_font},
  };
  std::map<std::string, std::string> locale = load_locale(arkr, arks);
  mgr.set_locale(locale);
  MenuSfxPlayer menu_sfx;
  menu_sfx.load(arkr, arks);
  mgr.set_audio_sink([&menu_sfx](Symbol source, Symbol cue) {
    menu_sfx.play(source, cue);
  });
  MenuLayoutTuning tuning;
  if (!options.tune_file.empty()) {
    load_menu_tuning_file(options.tune_file, tuning);
    std::fprintf(stderr,
                 "[menu-tune] Tab/[ ] select, arrows move, Shift+arrows scale, "
                 "Q/E yaw camera, PgUp/PgDn depth, Ctrl=fine, Space=coarse, "
                 "Ctrl+S save -> %s\n",
                 options.tune_file.c_str());
  }

  // Boot to the main menu by default; audits can jump directly to any stock
  // screen without driving through unrelated menu paths.
  const std::string start_screen = menu_env_string("GHOGX_MENU_START_SCREEN");
  mgr.goto_screen(start_screen.empty() ? Symbol("main_screen")
                                       : Symbol(start_screen.c_str()));

  // 2. Window + scene renderer. PCSX2's clean GH2 menu client capture lands at
  // 944x681 with the game content best matching a 530px viewport height. Keep
  // that measured ratio by default, while allowing explicit probe overrides.
  const int menu_w =
      menu_env_int_or("GHOGX_MENU_WINDOW_WIDTH", 960, 320, 3840);
  const int menu_h =
      menu_env_int_or("GHOGX_MENU_WINDOW_HEIGHT", 720, 240, 2160);
  auto win =
      ghogx::render::Window::create(menu_w, menu_h, "GuitarHeroOGX — menu");
  if (!win) { std::fprintf(stderr, "[menu] window/device create failed\n"); return 1; }
  ghogx::render::MiloSceneRenderer renderer(*win);
  ghogx::render::MiloSceneRenderer guitar_overlay_renderer(*win);
  ghogx::render::MiloSceneRenderer transition_from_renderer(*win);
  ghogx::render::MiloSceneRenderer transition_from_guitar_overlay_renderer(*win);
  MenuMaterialAnimPlayer menu_material_anims;
  MenuMaterialAnimPlayer transition_from_material_anims;
  bool has_guitar_overlay = false;
  bool transition_from_valid = false;
  bool transition_from_has_guitar_overlay = false;
  renderer.set_clear_color(0, 0, 0);
  transition_from_renderer.set_clear_color(0, 0, 0);
  guitar_overlay_renderer.set_clear_depth_on_overlay(true);
  guitar_overlay_renderer.set_environment_dynamic_lights(true);
  transition_from_guitar_overlay_renderer.set_clear_depth_on_overlay(true);
  transition_from_guitar_overlay_renderer.set_environment_dynamic_lights(true);
  const int viewport_w =
      menu_env_int_or("GHOGX_MENU_VIEWPORT_WIDTH", menu_w, 1, menu_w);
  const int pcsx2_fit_viewport_h =
      std::clamp((menu_h * 530 + 340) / 681, 1, menu_h);
  const int viewport_h =
      menu_env_int_or("GHOGX_MENU_VIEWPORT_HEIGHT", pcsx2_fit_viewport_h, 1,
                      menu_h);
  renderer.set_viewport((menu_w - viewport_w) / 2, (menu_h - viewport_h) / 2,
                        viewport_w, viewport_h);
  transition_from_renderer.set_viewport((menu_w - viewport_w) / 2,
                                        (menu_h - viewport_h) / 2,
                                        viewport_w, viewport_h);

  Object* shown = mgr.current_screen();
  // Run one poll tick before the first text build so panel `poll` handlers have
  // set their state (e.g. multiplayer disabled via is_missing_multi_controller).
  mgr.update(0.0f);
  rebuild_scene(hdr, ark, db, mgr, shown, renderer, tuning,
                &menu_material_anims);
  fire_screen_ui_triggers_from_stock_data(hdr, ark, mgr, shown, "ui_enter",
                                          renderer, &menu_material_anims);
  has_guitar_overlay =
      rebuild_guitar_overlay_scene(hdr, ark, db, mgr, shown,
                                   guitar_overlay_renderer);
  rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, song_font,
               footer_font, clarendon_font, rockletters_font, db, locale,
               tuning, menu_fonts);
  std::string last_runtime_display_key = runtime_display_state_key(mgr, shown);

  // Audit mode (GHOGX_MENU_DUMP=1): goto every *_screen object, rebuild its scene
  // + text, and report the mesh/texture/glyph counts. The fastest way to find
  // screens that don't render (no panels, missing MILO, empty text) — no nav
  // needed. Prints a one-line summary per screen, then exits.
  if (std::getenv("GHOGX_MENU_DUMP")) {
    ObjectDir& reg = mgr.registry();
    int ok = 0, empty = 0, total = 0;
    Object* audit_gamecfg = mgr.resolve_object(Symbol("gamecfg"));
    const DataNode audit_default_mode =
        audit_gamecfg ? audit_gamecfg->get_property(Symbol("mode")) : DataNode();
    const std::string dump_screenshot_dir =
        menu_env_string("GHOGX_MENU_DUMP_SCREENSHOTS_DIR");
    if (!dump_screenshot_dir.empty()) {
      std::error_code ec;
      std::filesystem::create_directories(dump_screenshot_dir, ec);
    }
    for (std::size_t i = 0; i < reg.size(); ++i) {
      Object* o = reg.at(i);
      if (!o) continue;
      std::string nm = o->name().c_str();
      if (nm.size() <= 7 || nm.compare(nm.size() - 7, 7, "_screen") != 0) continue;
      ++total;
      Symbol seeded_mode = audit_mode_for_screen(db, o->name());
      const bool explicit_mode = seeded_mode.valid();
      if (!seeded_mode.valid() && !audit_default_mode.empty()) {
        seeded_mode =
            audit_default_mode.as_symbol().value_or(Symbol("quickplay"));
      }
      if (audit_gamecfg && seeded_mode.valid()) {
        audit_gamecfg->set_property(Symbol("mode"), DataNode::Sym(seeded_mode));
      }
      mgr.goto_screen(o->name());
      mgr.update(0.0f);
      Object* s = mgr.current_screen();
      std::vector<Symbol> pn = screen_panel_names(s);
      std::fprintf(stderr, "[dump] %-34s panels=%zu mode=%s%s\n",
                   nm.c_str(), pn.size(),
                   seeded_mode.valid() ? seeded_mode.c_str() : "",
                   explicit_mode ? " (modes.dtb)" : "");
      rebuild_scene(hdr, ark, db, mgr, s, renderer, tuning,
                    &menu_material_anims);
      rebuild_text(hdr, ark, mgr, s, renderer, impact_font, song_font,
                   footer_font, clarendon_font, rockletters_font, db, locale,
                   tuning, menu_fonts);
      has_guitar_overlay =
          rebuild_guitar_overlay_scene(hdr, ark, db, mgr, s,
                                       guitar_overlay_renderer);
      if (has_guitar_overlay) {
        renderer.draw_scene_only();
        guitar_overlay_renderer.draw_over_scene(guitar_overlay_renderer.camera());
        renderer.draw_text_over_scene();
      } else {
        renderer.draw();
      }
      if (!dump_screenshot_dir.empty()) {
        const std::filesystem::path shot_path =
            std::filesystem::path(dump_screenshot_dir) /
            (menu_safe_filename(nm) + ".bmp");
        win->save_screenshot(shot_path.string().c_str());
      }
      win->present();
    }
    std::fprintf(stderr, "[dump] %d screens audited (ok=%d empty=%d)\n", total, ok, empty);
    return 0;
  }

  // Per-screen nav state: the focusable components (with nav links) + disabled set.
  std::vector<MenuLabel> cur_labels = gather_labels(hdr, ark, mgr, shown);
  std::unordered_set<std::string> cur_disabled = compute_disabled(mgr);
  size_t tune_index = 0;
  std::vector<MenuTuneTarget> tune_targets =
      build_menu_tune_targets(shown, cur_labels, locale);
  auto rebuild_tune_targets = [&](bool log_targets) {
    const std::string previous =
        tune_targets.empty() ? std::string() : menu_tune_target_id(tune_targets[tune_index]);
    tune_targets = build_menu_tune_targets(shown, cur_labels, locale);
    if (tune_targets.empty()) {
      tune_targets.push_back({MenuTuneKind::Camera, "camera", "camera"});
      tune_index = 0;
    } else {
      auto it = std::find_if(tune_targets.begin(), tune_targets.end(),
                             [&](const MenuTuneTarget& target) {
                               return menu_tune_target_id(target) == previous;
                             });
      if (it != tune_targets.end()) {
        tune_index = static_cast<size_t>(std::distance(tune_targets.begin(), it));
      } else if (tune_index >= tune_targets.size()) {
        tune_index = tune_targets.size() - 1;
      }
    }
    if (log_targets && !options.tune_file.empty())
      log_menu_tune_targets(shown, tune_targets);
  };
  if (!options.tune_file.empty()) log_menu_tune_targets(shown, tune_targets);
  auto focus_name = [&]() -> std::string {
    Object* s = mgr.current_screen();
    if (!s) return "";
    Symbol fpn = s->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
    Object* p = fpn.valid() ? mgr.find_object(fpn) : nullptr;
    std::string f = focused_component_or_stock_list(p);
    if (f == "ss_song.lst") {
      int pos = 0;
      if (Object* list = mgr.resolve_object(Symbol("ss_song.lst")))
        pos = list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
      else if (Object* panel = mgr.find_object(Symbol("sel_song_panel")))
        pos = panel->get_property(Symbol("ss_song_selected")).as_int().value_or(0);
      f += ":" + std::to_string(pos);
    } else if (f == "credits.lst") {
      int pos = 0;
      if (Object* list = mgr.resolve_object(Symbol("credits.lst")))
        pos = list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
      f += ":" + std::to_string(pos);
    }
    return f;
  };
  std::string last_focus = focus_name();

  // Headless auto-nav harness (GHOGX_MENU_NAV="down,confirm,back,focus:main_career.btn"
  // ...) — one action every kNavStep frames, so screenshots can reach any screen.
  std::vector<std::string> nav;
  if (const char* env = std::getenv("GHOGX_MENU_NAV")) {
    std::string e(env), tok;
    for (char ch : e + ",") { if (ch == ',') { if (!tok.empty()) nav.push_back(tok); tok.clear(); } else tok += ch; }
  }
  size_t nav_i = 0;
  const uint64_t kNavStep = 5;
  uint64_t next_nav_frame = kNavStep;

  using clock = std::chrono::steady_clock;
  auto last = clock::now();
  uint64_t frame = 0;
  int max_frames = options.max_frames;
  if (!options.screenshot_path.empty() && max_frames == 0) {
    max_frames = options.screenshot_frame + 3;
  }
  bool prev_keys[256] = {};
  auto draw_menu_renderer_pair =
      [](ghogx::render::MiloSceneRenderer& base,
         ghogx::render::MiloSceneRenderer& guitar_overlay,
         bool has_guitar, bool clear_target) {
        if (clear_target) {
          if (has_guitar) {
            base.draw_scene_only();
            guitar_overlay.draw_over_scene(guitar_overlay.camera());
            base.draw_text_over_scene();
          } else {
            base.draw();
          }
          return;
        }
        if (has_guitar) {
          base.draw_scene_only_over_scene();
          guitar_overlay.draw_over_scene(guitar_overlay.camera());
          base.draw_text_over_scene();
        } else {
          base.draw_over_scene(base.camera());
        }
      };
  auto reset_menu_renderer_transition_state =
      [](ghogx::render::MiloSceneRenderer& r) {
        r.set_global_tint(1.0f, 1.0f);
        r.set_world_transform(menu_identity_transform());
      };

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;
    auto key_edge = [&](int vk) {
      return vk >= 0 && vk < 256 && win->key_down(vk) && !prev_keys[vk];
    };
    const bool tuning_active = !options.tune_file.empty();
    const bool tune_key_left = key_edge(VK_LEFT);
    const bool tune_key_right = key_edge(VK_RIGHT);
    const bool tune_key_up = key_edge(VK_UP);
    const bool tune_key_down = key_edge(VK_DOWN);

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (dt > 0.1f) dt = 0.1f;

    // Input (live controller/keyboard) -> focus nav + the real menu scripts.
    bool menu_action_changed = false;
    const bool transition_active_before_input = mgr.in_transition();
    if (!(tuning_active && tune_key_down) && win->action_pressed(Action::Down)) {
      focus_move(mgr, cur_labels, cur_disabled, +1, db.song_count());
      menu_action_changed = true;
    }
    if (!(tuning_active && tune_key_up) && win->action_pressed(Action::Up)) {
      focus_move(mgr, cur_labels, cur_disabled, -1, db.song_count());
      menu_action_changed = true;
    }
    if (win->action_pressed(Action::Confirm)) {
      do_confirm(mgr);
      menu_action_changed = true;
    }
    if (win->action_pressed(Action::Back)) {
      do_back(mgr);
      menu_action_changed = true;
    }

    bool tuning_changed = false;
    if (!options.tune_file.empty()) {
      bool selection_changed = false;
      if (key_edge(VK_TAB) || key_edge(VK_OEM_6)) {
        if (win->key_down(VK_SHIFT)) {
          tune_index = tune_index == 0 ? tune_targets.size() - 1
                                       : tune_index - 1;
        } else {
          tune_index = (tune_index + 1) % tune_targets.size();
        }
        selection_changed = true;
      } else if (key_edge(VK_OEM_4)) {
        tune_index = tune_index == 0 ? tune_targets.size() - 1 : tune_index - 1;
        selection_changed = true;
      }

      const bool fine = win->key_down(VK_CONTROL);
      const bool coarse = win->key_down(VK_SPACE);
      const bool scale = win->key_down(VK_SHIFT);
      const float step = fine ? 0.0005f : (coarse ? 0.0200f : 0.0020f);
      const float rot_step = fine ? 0.25f : (coarse ? 10.0f : 1.0f);
      const int z_step = coarse ? 20 : 1;
      float dx = 0.0f, dy = 0.0f, dw = 0.0f, dh = 0.0f;
      float drot = 0.0f;
      int dz = 0;
      if (scale) {
        if (tune_key_left)  dw -= step;
        if (tune_key_right) dw += step;
        if (tune_key_up)    dh -= step;
        if (tune_key_down)  dh += step;
      } else {
        if (tune_key_left)  dx -= step;
        if (tune_key_right) dx += step;
        if (tune_key_up)    dy -= step;
        if (tune_key_down)  dy += step;
      }
      const MenuTuneTarget& tune_target = tune_targets[tune_index];
      if (menu_tune_can_rotate(tune_target)) {
        if (key_edge('Q')) drot -= rot_step;
        if (key_edge('E')) drot += rot_step;
      }
      if (key_edge(VK_PRIOR)) dz += z_step;
      if (key_edge(VK_NEXT)) dz -= z_step;
      if ((dx != 0.0f || dy != 0.0f || dw != 0.0f || dh != 0.0f ||
           drot != 0.0f || dz != 0) &&
          nudge_menu_tuning(tuning, tune_target, dx, dy, dw, dh, drot, dz)) {
        tuning_changed = true;
        std::fprintf(stderr, "[menu-tune] nudged %s\n", tune_target.label.c_str());
      }
      if (key_edge('S') && win->key_down(VK_CONTROL)) {
        const bool saved = save_menu_tuning_file(options.tune_file, tuning);
        std::fprintf(stderr, "[menu-tune] %s %s\n",
                     saved ? "saved" : "failed to save",
                     options.tune_file.c_str());
      }
      char title[256];
      std::snprintf(title, sizeof(title), "GuitarHeroOGX - menu tune [%zu/%zu] %s%s",
                    tune_index + 1, tune_targets.size(),
                    tune_target.label.c_str(),
                    menu_tune_can_rotate(tune_target) ? " (camera)" : "");
      win->set_title(title);
      if (selection_changed) {
        std::fprintf(stderr, "[menu-tune] selected %s\n", tune_target.label.c_str());
      }
    }

    // Scripted auto-nav (headless testing): one action per kNavStep frames.
    if (nav_i < nav.size() && frame >= next_nav_frame) {
      const std::string& a = nav[nav_i++];
      next_nav_frame = frame + kNavStep;
      if (a == "down") {
        focus_move(mgr, cur_labels, cur_disabled, +1, db.song_count());
        menu_action_changed = true;
      } else if (a == "up") {
        focus_move(mgr, cur_labels, cur_disabled, -1, db.song_count());
        menu_action_changed = true;
      }
      else if (a == "confirm") {
        do_confirm(mgr);
        menu_action_changed = true;
      } else if (a == "back") {
        do_back(mgr);
        menu_action_changed = true;
      } else if (a.rfind("wait:", 0) == 0) {
        const int wait_frames = std::max(0, std::atoi(a.c_str() + 5));
        next_nav_frame += static_cast<uint64_t>(wait_frames);
      }
      else if (a.rfind("focus:", 0) == 0) {
        Object* s = mgr.current_screen();
        Symbol fpn = s ? s->get_property(Symbol("focus")).as_symbol().value_or(Symbol()) : Symbol();
        if (Object* p = fpn.valid() ? mgr.find_object(fpn) : nullptr) {
          p->set_property(Symbol("focus"), DataNode::Sym(Symbol(a.substr(6).c_str())));
          menu_action_changed = true;
        }
      }
    }

    const bool was_in_transition = mgr.in_transition();
    const bool transition_started_this_frame =
        !transition_active_before_input && mgr.in_transition();
    mgr.update(transition_started_this_frame ? 0.0f : dt);
    const bool transition_state_changed =
        was_in_transition != mgr.in_transition();
    renderer.update(dt);
    menu_material_anims.update(dt, renderer);
    guitar_overlay_renderer.update(dt);
    transition_from_renderer.update(dt);
    transition_from_material_anims.update(dt, transition_from_renderer);
    transition_from_guitar_overlay_renderer.update(dt);
    const std::string runtime_display_key =
        runtime_display_state_key(mgr, mgr.current_screen());
    const bool runtime_display_changed =
        runtime_display_key != last_runtime_display_key;
    if (runtime_display_changed) last_runtime_display_key = runtime_display_key;

    // Reload the scene + text when the screen changed; re-render text (re-colour)
    // when only the focus moved.
    if (mgr.current_screen() != shown) {
      if (mgr.in_transition() && mgr.transition_exiting_screen() == shown) {
        rebuild_scene(hdr, ark, db, mgr, shown, transition_from_renderer,
                      tuning, &transition_from_material_anims);
        transition_from_has_guitar_overlay =
            rebuild_guitar_overlay_scene(hdr, ark, db, mgr, shown,
                                         transition_from_guitar_overlay_renderer);
        rebuild_text(hdr, ark, mgr, shown, transition_from_renderer,
                     impact_font, song_font, footer_font, clarendon_font,
                     rockletters_font, db, locale, tuning, menu_fonts);
        transition_from_valid = true;
      } else {
        transition_from_valid = false;
        transition_from_has_guitar_overlay = false;
      }
      shown = mgr.current_screen();
      cur_labels = gather_labels(hdr, ark, mgr, shown);
      cur_disabled = compute_disabled(mgr);
      rebuild_tune_targets(true);
      rebuild_scene(hdr, ark, db, mgr, shown, renderer, tuning,
                    &menu_material_anims);
      fire_screen_ui_triggers_from_stock_data(hdr, ark, mgr, shown, "ui_enter",
                                              renderer, &menu_material_anims);
      has_guitar_overlay =
          rebuild_guitar_overlay_scene(hdr, ark, db, mgr, shown,
                                       guitar_overlay_renderer);
      last_runtime_display_key = runtime_display_state_key(mgr, shown);
      rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, song_font,
                   footer_font, clarendon_font, rockletters_font, db, locale,
                   tuning, menu_fonts);
      last_focus = focus_name();
    } else if (focus_name() != last_focus || tuning_changed ||
               menu_action_changed ||
               transition_state_changed ||
               runtime_display_changed) {
      last_focus = focus_name();
      if (tuning_changed || menu_action_changed || transition_state_changed ||
          runtime_display_changed)
        rebuild_scene(hdr, ark, db, mgr, shown, renderer, tuning,
                      &menu_material_anims);
      if (transition_state_changed && !mgr.in_transition()) {
        fire_screen_ui_triggers_from_stock_data(hdr, ark, mgr, shown,
                                                "transition_complete",
                                                renderer,
                                                &menu_material_anims);
        transition_from_valid = false;
        transition_from_has_guitar_overlay = false;
      }
      if (tuning_changed || menu_action_changed || transition_state_changed ||
          runtime_display_changed)
        has_guitar_overlay =
            rebuild_guitar_overlay_scene(hdr, ark, db, mgr, shown,
                                         guitar_overlay_renderer);
      rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, song_font,
                   footer_font, clarendon_font, rockletters_font, db, locale,
                   tuning, menu_fonts);
    }

    reset_menu_renderer_transition_state(renderer);
    reset_menu_renderer_transition_state(guitar_overlay_renderer);
    reset_menu_renderer_transition_state(transition_from_renderer);
    reset_menu_renderer_transition_state(transition_from_guitar_overlay_renderer);

    if (mgr.in_transition() && menu_transition_diagnostic_visuals_enabled()) {
      const MenuTransitionVisual visual = menu_transition_visual(mgr);
      bool cleared = false;
      if (transition_from_valid && visual.draw_exiting) {
        transition_from_renderer.set_global_tint(visual.exiting_brightness,
                                                 1.0f);
        transition_from_guitar_overlay_renderer.set_global_tint(
            visual.exiting_brightness, 1.0f);
        draw_menu_renderer_pair(transition_from_renderer,
                                transition_from_guitar_overlay_renderer,
                                transition_from_has_guitar_overlay,
                                /*clear_target=*/true);
        cleared = true;
      }
      if (visual.draw_entering) {
        const auto entering_transform =
            menu_translation_transform(visual.entering_x, 0.0f, 0.0f);
        renderer.set_global_tint(visual.entering_brightness, 1.0f);
        renderer.set_world_transform(entering_transform);
        guitar_overlay_renderer.set_global_tint(visual.entering_brightness,
                                                1.0f);
        guitar_overlay_renderer.set_world_transform(entering_transform);
        draw_menu_renderer_pair(renderer, guitar_overlay_renderer,
                                has_guitar_overlay, !cleared);
        cleared = true;
      }
      if (!cleared) {
        renderer.set_global_tint(0.0f, 1.0f);
        draw_menu_renderer_pair(renderer, guitar_overlay_renderer,
                                has_guitar_overlay, /*clear_target=*/true);
      }
    } else {
      draw_menu_renderer_pair(renderer, guitar_overlay_renderer,
                              has_guitar_overlay, /*clear_target=*/true);
    }

    if (!options.screenshot_path.empty() &&
        frame == static_cast<uint64_t>(options.screenshot_frame)) {
      win->save_screenshot(options.screenshot_path.c_str());
    }
    win->present();

    ++frame;
    if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
    for (int i = 0; i < 256; ++i) prev_keys[i] = win->key_down(i);
  }
  return 0;
}

}  // namespace ghogx::ui
