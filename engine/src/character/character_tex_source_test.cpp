#include "character/char_mesh.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void put_u8(std::vector<uint8_t>& b, uint8_t v) { b.push_back(v); }

void put_u16(std::vector<uint8_t>& b, uint16_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xffu));
  b.push_back(static_cast<uint8_t>((v >> 8) & 0xffu));
}

void put_u32(std::vector<uint8_t>& b, uint32_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xffu));
  b.push_back(static_cast<uint8_t>((v >> 8) & 0xffu));
  b.push_back(static_cast<uint8_t>((v >> 16) & 0xffu));
  b.push_back(static_cast<uint8_t>((v >> 24) & 0xffu));
}

void put_i32(std::vector<uint8_t>& b, int32_t v) {
  put_u32(b, static_cast<uint32_t>(v));
}

void put_f32(std::vector<uint8_t>& b, float v) {
  uint32_t raw = 0;
  static_assert(sizeof(raw) == sizeof(v), "float size");
  std::memcpy(&raw, &v, sizeof(raw));
  put_u32(b, raw);
}

void put_str(std::vector<uint8_t>& b, const std::string& s) {
  put_u32(b, static_cast<uint32_t>(s.size()));
  b.insert(b.end(), s.begin(), s.end());
}

void put_object_fields_minimal(std::vector<uint8_t>& b) {
  put_u32(b, 1);   // Hmx::Object revision 1, no alternate revision.
  put_str(b, "");  // subtype Symbol
  put_u8(b, 0);    // no root DTB tree
  put_str(b, "");  // optional note Symbol for object revision > 0
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int64_t got, int64_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_float(float got, float want, const char* label) {
  if (std::fabs(got - want) < 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

std::vector<uint8_t> make_bitmap_payload(size_t count) {
  std::vector<uint8_t> out(count);
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<uint8_t>(i & 0xffu);
  }
  return out;
}

}  // namespace

int main() {
  bool ok = true;

  std::vector<uint8_t> tex;
  put_u32(tex, (2u << 16) | 11u);  // packed RndTex rev: hmx=11, alt=2
  put_object_fields_minimal(tex);
  put_i32(tex, 16);
  put_i32(tex, 8);
  put_i32(tex, 4);
  put_str(tex, "");
  put_f32(tex, 1.25f);
  put_i32(tex, 2);  // RndTex::Rendered
  put_u8(tex, 1);   // post flag
  put_u8(tex, 1);   // optimizeForPS3

  put_u8(tex, 1);   // RndBitmap header revision
  put_u8(tex, 4);   // bpp
  put_u32(tex, 0);  // order
  put_u8(tex, 2);   // mip count
  put_u16(tex, 16);
  put_u16(tex, 8);
  put_u16(tex, 8);
  for (int i = 0; i < 0x13; ++i) put_u8(tex, 0);
  const std::vector<uint8_t> payload = make_bitmap_payload(148);
  tex.insert(tex.end(), payload.begin(), payload.end());

  const ghogx::character::RndTex decoded =
      ghogx::character::decode_rnd_tex("generated_render.tex", tex);
  ok &= expect_int(decoded.version, 11, "rev11 source revision");
  ok &= expect_int(decoded.alt_version, 2, "rev11 alt revision");
  ok &= expect_int(decoded.width, 16, "rev11 width");
  ok &= expect_int(decoded.height, 8, "rev11 height");
  ok &= expect_int(decoded.bpp, 4, "rev11 bpp");
  ok &= expect_bool(decoded.power_of_two, true, "rev11 power of two");
  ok &= expect_string(decoded.filepath, "", "rev11 empty filepath");
  ok &= expect_float(decoded.mip_map_k, 1.25f, "rev11 mip map k");
  ok &= expect_int(decoded.type, 2, "rev11 rendered type");
  ok &= expect_bool(decoded.has_post_flag, true, "rev11 post flag present");
  ok &= expect_bool(decoded.post_flag, true, "rev11 post flag value");
  ok &= expect_bool(decoded.optimize_for_ps3, true, "rev11 optimize flag");
  ok &= expect_bool(decoded.bitmap_header_decoded, true, "bitmap header");
  ok &= expect_int(decoded.bitmap_version, 1, "bitmap header revision");
  ok &= expect_int(decoded.bitmap_bpp, 4, "bitmap bpp");
  ok &= expect_int(decoded.bitmap_order, 0, "bitmap order");
  ok &= expect_int(decoded.bitmap_mip_count, 2, "bitmap mip count");
  ok &= expect_int(decoded.bitmap_width, 16, "bitmap width");
  ok &= expect_int(decoded.bitmap_height, 8, "bitmap height");
  ok &= expect_int(decoded.bitmap_row_bytes, 8, "bitmap row bytes");
  ok &= expect_size(decoded.bitmap_palette_bytes, 64, "palette bytes");
  ok &= expect_size(decoded.bitmap_base_pixel_bytes, 64, "base pixel bytes");
  ok &= expect_size(decoded.bitmap_mip_pixel_bytes, 20, "mip pixel bytes");
  ok &= expect_size(decoded.bitmap_expected_payload_bytes, 148,
                    "expected bitmap payload");
  ok &= expect_size(decoded.cached_bitmap_payload_bytes, 148,
                    "cached bitmap payload");
  ok &= expect_bool(decoded.bitmap_payload_size_matches, true,
                    "payload size match");
  ok &= expect_string(decoded.cached_bitmap_payload_prefix_hex,
                      "00:01:02:03:04:05:06:07:"
                      "08:09:0a:0b:0c:0d:0e:0f:"
                      "10:11:12:13:14:15:16:17:"
                      "18:19:1a:1b:1c:1d:1e:1f",
                      "payload prefix");

  std::vector<uint8_t> legacy;
  put_u32(legacy, 4);  // RndTex rev 4, no object fields.
  put_i32(legacy, 32);
  put_i32(legacy, 16);
  put_i32(legacy, 8);
  put_str(legacy, "hair.tex");
  put_i32(legacy, 0x10);  // legacy cubemap mask -> "_ga" suffix
  put_i32(legacy, 24);    // fixed mipMapK / 16
  const ghogx::character::RndTex legacy_decoded =
      ghogx::character::decode_rnd_tex("hair.tex", legacy);
  ok &= expect_int(legacy_decoded.version, 4, "legacy revision");
  ok &= expect_string(legacy_decoded.filepath, "hair_ga.tex",
                      "legacy suffix path");
  ok &= expect_float(legacy_decoded.mip_map_k, 1.5f, "legacy fixed mip k");
  ok &= expect_int(legacy_decoded.type, 1, "legacy default type");
  ok &= expect_bool(legacy_decoded.bitmap_header_decoded, false,
                    "legacy no cached bitmap");

  std::vector<uint8_t> old_header;
  put_u32(old_header, 11);
  put_object_fields_minimal(old_header);
  put_i32(old_header, 8);
  put_i32(old_header, 8);
  put_i32(old_header, 8);
  put_str(old_header, "");
  put_f32(old_header, -8.0f);
  put_i32(old_header, 1);
  put_u8(old_header, 0);
  put_u8(old_header, 0);
  put_u8(old_header, 0);   // RndBitmap header revision 0
  put_u8(old_header, 8);   // bpp
  put_u8(old_header, 0);   // legacy one-byte order
  put_u8(old_header, 0);   // mip count
  put_u16(old_header, 8);
  put_u16(old_header, 8);
  put_u16(old_header, 8);
  for (int i = 0; i < 6; ++i) put_u8(old_header, 0);
  const std::vector<uint8_t> old_payload = make_bitmap_payload(288);
  old_header.insert(old_header.end(), old_payload.begin(), old_payload.end());
  const ghogx::character::RndTex old_header_decoded =
      ghogx::character::decode_rnd_tex("old_header.tex", old_header);
  ok &= expect_bool(old_header_decoded.bitmap_header_decoded, true,
                    "old bitmap header decoded");
  ok &= expect_int(old_header_decoded.bitmap_version, 0,
                   "old bitmap header revision");
  ok &= expect_int(old_header_decoded.bitmap_order, 0,
                   "old bitmap one-byte order");
  ok &= expect_size(old_header_decoded.bitmap_palette_bytes, 1024,
                    "old bitmap palette bytes");
  ok &= expect_bool(old_header_decoded.bitmap_payload_size_matches, false,
                    "old bitmap intentionally short payload");

  std::cout << "character_tex_source_test " << (ok ? "OK" : "FAIL") << "\n";
  return ok ? 0 : 1;
}
