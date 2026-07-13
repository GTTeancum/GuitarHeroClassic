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

bool expect_strings(const std::vector<std::string>& got,
                    const std::vector<std::string>& want,
                    const char* label) {
  if (got == want) return true;
  std::cerr << label << " got";
  for (const std::string& s : got) std::cerr << " " << s;
  std::cerr << " want";
  for (const std::string& s : want) std::cerr << " " << s;
  std::cerr << "\n";
  return false;
}

bool expect_ints(const std::vector<int32_t>& got,
                 const std::vector<int32_t>& want,
                 const char* label) {
  if (got == want) return true;
  std::cerr << label << " got";
  for (int32_t v : got) std::cerr << " " << v;
  std::cerr << " want";
  for (int32_t v : want) std::cerr << " " << v;
  std::cerr << "\n";
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

  const ghogx::character::SourceRndTexLoadPlan rev11_cached =
      ghogx::character::source_rndtex_load_plan(11, 1, true);
  ok &= expect_bool(rev11_cached.accepted_revision, true,
                    "rev11 accepted");
  ok &= expect_bool(rev11_cached.reads_object_fields, true,
                    "rev11 object fields");
  ok &= expect_bool(rev11_cached.reads_float_mip_map_k, true,
                    "rev11 float mip k");
  ok &= expect_bool(rev11_cached.reads_direct_type, true,
                    "rev11 direct type");
  ok &= expect_bool(rev11_cached.reads_post_flag, true,
                    "rev11 post flag");
  ok &= expect_bool(rev11_cached.reads_optimize_for_ps3, true,
                    "rev11 optimize gate");
  ok &= expect_bool(rev11_cached.creates_cached_loader, true,
                    "rev11 cached loader");
  ok &= expect_bool(rev11_cached.delegates_cached_payload_to_bitmap, true,
                    "rev11 cached bitmap delegate");

  const ghogx::character::SourceRndTexLoadPlan rev4_uncached =
      ghogx::character::source_rndtex_load_plan(4, 0, false);
  ok &= expect_bool(rev4_uncached.reads_object_fields, false,
                    "rev4 no object fields");
  ok &= expect_bool(rev4_uncached.reads_legacy_cubemap_mask, true,
                    "rev4 cubemap mask");
  ok &= expect_bool(rev4_uncached.reads_fixed_mip_map_k, true,
                    "rev4 fixed mip k");
  ok &= expect_bool(rev4_uncached.reads_direct_type, false,
                    "rev4 no direct type");

  const ghogx::character::SourceRndTexLoadPlan rev6_uncached =
      ghogx::character::source_rndtex_load_plan(6, 0, false);
  ok &= expect_bool(rev6_uncached.reads_legacy_type_index, true,
                    "rev6 legacy type index");
  ok &= expect_bool(rev6_uncached.reads_rendered_bool_type, false,
                    "rev6 no rendered bool");

  const ghogx::character::SourceRndTexPowerOfTwoPlan pot_zero =
      ghogx::character::source_rndtex_power_of_two_plan(16, 0);
  ok &= expect_bool(pot_zero.result, true, "power-of-two zero dimension");
  const ghogx::character::SourceRndTexPowerOfTwoPlan pot_negative =
      ghogx::character::source_rndtex_power_of_two_plan(-1, 16);
  ok &= expect_bool(pot_negative.result, false, "power-of-two negative dim");

  const ghogx::character::SourceRndTexCheckDimPlan movie_dim =
      ghogx::character::source_rndtex_check_dim_plan(24, 4, false, true);
  ok &= expect_string(movie_dim.error, "%s: dimensions not multiple of 16",
                      "movie dimension error");
  const ghogx::character::SourceRndTexCheckDimPlan file_cap =
      ghogx::character::source_rndtex_check_dim_plan(2048, 1, true, true);
  ok &= expect_string(file_cap.error, "%s: dimensions greater than 1024",
                      "file dimension cap");
  const ghogx::character::SourceRndTexCheckDimPlan power_override =
      ghogx::character::source_rndtex_check_dim_plan(1032, 1, true, true);
  ok &= expect_string(power_override.error,
                      "%s: dimensions are not power-of-2",
                      "file power-of-two override");

  const ghogx::character::SourceRndTexCheckSizePlan device_bypass =
      ghogx::character::source_rndtex_check_size_plan(7, 7, 12, 3,
                                                      0x1000, true, true);
  ok &= expect_bool(device_bypass.bypass_device_or_density, true,
                    "device texture size bypass");
  ok &= expect_string(device_bypass.error, "", "device bypass no error");
  const ghogx::character::SourceRndTexCheckSizePlan invalid_bpp =
      ghogx::character::source_rndtex_check_size_plan(64, 64, 12, 0,
                                                      1, false, true);
  ok &= expect_string(invalid_bpp.error, "%s: invalid bpp",
                      "invalid bpp error");
  const ghogx::character::SourceRndTexCheckSizePlan size_over =
      ghogx::character::source_rndtex_check_size_plan(1024, 1024, 8, 0,
                                                      1, false, true);
  ok &= expect_string(size_over.error, "%s: size over 524,272 bytes",
                      "texture size cap");
  const ghogx::character::SourceRndTexCheckSizePlan mip_error =
      ghogx::character::source_rndtex_check_size_plan(64, 64, 8, 1,
                                                      1, false, true);
  ok &= expect_string(mip_error.error, "%s: more than 0 mip levels",
                      "texture mip count error");

  const ghogx::character::SourceRndTexRenderedClampPlan rendered_clamp =
      ghogx::character::source_rndtex_rendered_clamp_plan(
          "render_target.tex", 512, 1024, 2, true);
  ok &= expect_bool(rendered_clamp.clamped, true,
                    "empty rendered texture clamps");
  ok &= expect_int(rendered_clamp.result_width, 256,
                   "empty rendered clamp width");
  ok &= expect_int(rendered_clamp.result_height, 256,
                   "empty rendered clamp height");
  const ghogx::character::SourceRndTexRenderedClampPlan movie_exception =
      ghogx::character::source_rndtex_rendered_clamp_plan(
          "movie.tex", 512, 1024, 2, true);
  ok &= expect_bool(movie_exception.clamped, false,
                    "movie texture skips rendered clamp");
  ok &= expect_int(movie_exception.result_width, 512,
                   "movie exception width");
  ok &= expect_int(movie_exception.result_height, 1024,
                   "movie exception height");

  const ghogx::character::SourceRndTexCopyPlan normal_copy =
      ghogx::character::source_rndtex_copy_plan(false, 2, 1);
  ok &= expect_bool(normal_copy.copies_mip_map_k, true,
                    "normal copy copies mipMapK");
  ok &= expect_bool(normal_copy.calls_presync_bitmap, true,
                    "normal copy presyncs bitmap");
  ok &= expect_bool(normal_copy.creates_bitmap_from_source_bpp_order, true,
                    "normal copy creates bitmap");
  ok &= expect_bool(normal_copy.calls_sync_bitmap, true,
                    "normal copy syncs bitmap");
  const ghogx::character::SourceRndTexCopyPlan max_mismatch =
      ghogx::character::source_rndtex_copy_plan(true, 2, 1);
  ok &= expect_bool(max_mismatch.copies_mip_map_k, false,
                    "copy-from-max skips mipMapK");
  ok &= expect_bool(max_mismatch.aborts_for_copy_from_max_type_mismatch, true,
                    "copy-from-max mismatched type aborts");
  ok &= expect_bool(max_mismatch.calls_presync_bitmap, false,
                    "aborted copy does not presync");

  ok &= expect_string(ghogx::character::source_rndtex_type_name(1),
                      "Regular", "texture type regular");
  ok &= expect_string(ghogx::character::source_rndtex_type_name(0x22),
                      "RenderedNoZ", "texture type rendered no z");
  ok &= expect_string(ghogx::character::source_rndtex_type_name(0x0a2),
                      "DepthVolumeMap", "texture type depth volume");
  ok &= expect_string(ghogx::character::source_rndtex_type_name(0x200),
                      "Scratch", "texture type scratch");
  ok &= expect_string(ghogx::character::source_rndtex_type_name(3),
                      "", "texture type unknown");

  const ghogx::character::SourceRndTexPrintPlan print_plan =
      ghogx::character::source_rndtex_print_plan();
  ok &= expect_strings(print_plan.fields,
                       {"width", "height", "bpp", "mipMapK", "file",
                        "type"},
                       "texture print fields");
  const ghogx::character::SourceRndTexHandlerPlan handler_plan =
      ghogx::character::source_rndtex_handler_plan();
  ok &= expect_strings(handler_plan.handlers,
                       {"set_bitmap", "set_rendered", "file_path",
                        "set_file_path", "size_kb", "tex_type", "save_bmp",
                        "Hmx::Object"},
                       "texture handler rows");
  ok &= expect_int(handler_plan.check_line, 1082, "texture handler check");
  const ghogx::character::SourceRndTexOnSetBitmapPlan file_bitmap =
      ghogx::character::source_rndtex_on_set_bitmap_plan(3);
  ok &= expect_bool(file_bitmap.uses_file_path_overload, true,
                    "set_bitmap filepath branch");
  const ghogx::character::SourceRndTexOnSetBitmapPlan explicit_bitmap =
      ghogx::character::source_rndtex_on_set_bitmap_plan(7);
  ok &= expect_bool(explicit_bitmap.uses_explicit_bitmap_overload, true,
                    "set_bitmap explicit branch");
  ok &= expect_strings(explicit_bitmap.explicit_argument_order,
                       {"width", "height", "bpp", "type", "use_mips",
                        "null_path"},
                       "set_bitmap explicit argument order");
  const ghogx::character::SourceRndTexOnSetRenderedPlan rendered_plan =
      ghogx::character::source_rndtex_on_set_rendered_plan(true, 1);
  ok &= expect_bool(rendered_plan.calls_set_bitmap, true,
                    "set_rendered calls SetBitmap");
  ok &= expect_bool(rendered_plan.use_mips, true,
                    "set_rendered use mips");
  const ghogx::character::SourceRndTexPropSyncPlan prop_sync_plan =
      ghogx::character::source_rndtex_prop_sync_plan();
  ok &= expect_strings(prop_sync_plan.get_only_props, {"width", "height", "bpp"},
                       "texture get-only props");
  ok &= expect_strings(prop_sync_plan.direct_props,
                       {"mip_map_k", "optimize_for_ps3"},
                       "texture direct props");
  ok &= expect_strings(prop_sync_plan.modify_alt_props, {"file_path"},
                       "texture modify-alt props");

  const ghogx::character::SourceRndTexPlatformBppOrderPlan wii_alpha =
      ghogx::character::source_rndtex_platform_bpp_order_plan(
          "wii", "hair.tex", 4, true);
  ok &= expect_int(wii_alpha.result_bpp, 8, "wii alpha bpp");
  ok &= expect_int(wii_alpha.result_order, 0x148, "wii alpha order");
  const ghogx::character::SourceRndTexPlatformBppOrderPlan pc_norm =
      ghogx::character::source_rndtex_platform_bpp_order_plan(
          "pc", "face_norm.tex", 8, false);
  ok &= expect_bool(pc_norm.normal_texture, true, "pc normal detection");
  ok &= expect_int(pc_norm.result_bpp, 0x18, "pc normal bpp");
  ok &= expect_int(pc_norm.result_order, 0, "pc normal order");
  const ghogx::character::SourceRndTexPlatformBppOrderPlan ps2_order =
      ghogx::character::source_rndtex_platform_bpp_order_plan(
          "ps2", "body.tex", 4, false);
  ok &= expect_bool(ps2_order.ps2_leaves_existing_values, true,
                    "ps2 leaves bpp/order values");

  const ghogx::character::SourceRndTexSetBitmapPlan back_buffer =
      ghogx::character::source_rndtex_set_bitmap_plan(
          64, 64, 16, 8, false, 640, 480, 32);
  ok &= expect_bool(back_buffer.back_buffer_uses_screen_values, true,
                    "set bitmap back buffer branch");
  ok &= expect_int(back_buffer.result_width, 640, "back buffer width");
  ok &= expect_int(back_buffer.result_height, 480, "back buffer height");
  ok &= expect_int(back_buffer.result_bpp, 32, "back buffer bpp");
  const ghogx::character::SourceRndTexSetBitmapPlan rendered_mips =
      ghogx::character::source_rndtex_set_bitmap_plan(
          128, 128, 32, 2, true, 640, 480, 32);
  ok &= expect_bool(rendered_mips.rendered_counts_mips, true,
                    "rendered set bitmap mip branch");
  ok &= expect_int(rendered_mips.rendered_mip_count, 3,
                   "rendered mip count");
  const ghogx::character::SourceRndTexSetBitmapPlan regular_bitmap =
      ghogx::character::source_rndtex_set_bitmap_plan(
          64, 64, 8, 1, false, 640, 480, 32);
  ok &= expect_bool(regular_bitmap.creates_bitmap, true,
                    "regular set bitmap creates bitmap");
  const ghogx::character::SourceRndTexSetBitmapPlan special_bitmap =
      ghogx::character::source_rndtex_set_bitmap_plan(
          64, 64, 8, 0x204, false, 640, 480, 32);
  ok &= expect_bool(special_bitmap.skips_bitmap_for_special_type, true,
                    "special set bitmap skips bitmap create");

  const ghogx::character::SourceRndTexSetBitmapFromBitmapPlan bitmap_plan =
      ghogx::character::source_rndtex_set_bitmap_from_bitmap_plan(
          64, 64, 8, 0, 0, false, "pc", "face_norm.tex", false);
  ok &= expect_bool(bitmap_plan.calls_platform_bpp_order, true,
                    "bitmap overload calls platform order");
  ok &= expect_int(bitmap_plan.create_bpp, 0x18,
                   "bitmap overload platform bpp");
  ok &= expect_int(bitmap_plan.create_order, 0,
                   "bitmap overload platform order");
  const ghogx::character::SourceRndTexSetBitmapFromBitmapPlan preserve_bitmap =
      ghogx::character::source_rndtex_set_bitmap_from_bitmap_plan(
          64, 64, 8, 0x18, 0, true, "pc", "face_norm.tex", true);
  ok &= expect_bool(preserve_bitmap.calls_platform_bpp_order, false,
                    "preserve bitmap skips platform order");
  ok &= expect_int(preserve_bitmap.create_order, 0x18,
                   "preserve bitmap order");

  const ghogx::character::SourceRndTexSetBitmapFromLoaderPlan loader_buffer =
      ghogx::character::source_rndtex_set_bitmap_from_loader_plan(
          true, true, false, false, "hair.tex", true, 128, 64, 8, 0);
  ok &= expect_bool(loader_buffer.warns_disc_build_without_keep, true,
                    "loader warns without keep suffix");
  ok &= expect_bool(loader_buffer.copies_bottom_mip, true,
                    "loader bottom mip branch");
  ok &= expect_int(loader_buffer.result_width, 128, "loader bitmap width");
  const ghogx::character::SourceRndTexSetBitmapFromLoaderPlan no_loader =
      ghogx::character::source_rndtex_set_bitmap_from_loader_plan(
          false, false, false, false, "", false, 0, 0, 0, 0);
  ok &= expect_bool(no_loader.resets_bitmap_and_dimensions, true,
                    "no loader resets bitmap");
  ok &= expect_int(no_loader.result_bpp, 0x20, "no loader reset bpp");

  const ghogx::character::SourceRndTexCopyBottomMipPlan bottom_mip =
      ghogx::character::source_rndtex_copy_bottom_mip_plan(3);
  ok &= expect_bool(bottom_mip.walks_to_last_mip, true,
                    "copy bottom mip walks chain");
  ok &= expect_int(bottom_mip.selected_mip_index, 3,
                   "copy bottom mip selected index");
  const ghogx::character::SourceRndTexLockBitmapPlan ordered_lock =
      ghogx::character::source_rndtex_lock_bitmap_plan(0x18, 8);
  ok &= expect_bool(ordered_lock.converts_ordered_bitmap_to_32bpp, true,
                    "lock bitmap ordered conversion");
  ok &= expect_int(ordered_lock.create_bpp, 0x20,
                   "lock bitmap converted bpp");
  const ghogx::character::SourceRndTexLockBitmapPlan direct_lock =
      ghogx::character::source_rndtex_lock_bitmap_plan(0, 8);
  ok &= expect_bool(direct_lock.creates_direct_bitmap_view, true,
                    "lock bitmap direct view");
  ok &= expect_int(direct_lock.create_order, 0, "lock bitmap direct order");

  const ghogx::character::SourceRndBitmapResetPlan bitmap_reset =
      ghogx::character::source_rndbitmap_reset_plan(true, true);
  ok &= expect_bool(bitmap_reset.frees_buffer_when_present, true,
                    "bitmap reset frees buffer");
  ok &= expect_bool(bitmap_reset.resets_and_frees_mip_when_present, true,
                    "bitmap reset frees mip");
  ok &= expect_int(bitmap_reset.bpp, 0x20, "bitmap reset bpp");
  ok &= expect_int(bitmap_reset.order, 1, "bitmap reset order");

  const ghogx::character::SourceRndBitmapCreatePlan create_valid =
      ghogx::character::source_rndbitmap_create_plan(
          64, 32, 0, 8, 0, false, false);
  ok &= expect_bool(create_valid.valid_dimensions, true,
                    "bitmap create dimensions");
  ok &= expect_bool(create_valid.valid_bpp, true, "bitmap create bpp");
  ok &= expect_bool(create_valid.allocates_when_no_palette_and_no_buffer, true,
                    "bitmap create allocate branch");
  const ghogx::character::SourceRndBitmapCreatePlan create_palette =
      ghogx::character::source_rndbitmap_create_plan(
          64, 32, 0, 8, 0, true, false);
  ok &= expect_bool(create_palette.frees_palette_argument_after_assignment, true,
                    "bitmap create palette branch");
  const ghogx::character::SourceRndBitmapCreatePlan create_bad =
      ghogx::character::source_rndbitmap_create_plan(
          -1, 32, 0, 12, 0, false, false);
  ok &= expect_bool(create_bad.valid_dimensions, false,
                    "bitmap create invalid dimensions");
  ok &= expect_bool(create_bad.valid_bpp, false, "bitmap create invalid bpp");

  const ghogx::character::SourceRndBitmapSetMipPlan set_mip =
      ghogx::character::source_rndbitmap_set_mip_plan(
          128, 64, 8, 0, true, 64, 32, 8, 0);
  ok &= expect_bool(set_mip.checks_half_dimensions, true,
                    "bitmap set mip checks");
  ok &= expect_bool(set_mip.accepts_mip, true, "bitmap set mip accepts");
  const ghogx::character::SourceRndBitmapSetMipPlan set_bad_mip =
      ghogx::character::source_rndbitmap_set_mip_plan(
          128, 64, 8, 0, true, 32, 32, 8, 0);
  ok &= expect_bool(set_bad_mip.accepts_mip, false,
                    "bitmap set mip rejects wrong width");

  const ghogx::character::SourceRndBitmapLoadSafelyPlan safe_dimensions =
      ghogx::character::source_rndbitmap_load_safely_plan(
          1024, 64, 8, 1024, 512, 512, 0);
  ok &= expect_bool(safe_dimensions.dimension_fallback, true,
                    "bitmap safe dimension fallback");
  ok &= expect_bool(safe_dimensions.creates_8x8_32bpp_fallback, true,
                    "bitmap safe creates fallback");
  const ghogx::character::SourceRndBitmapLoadSafelyPlan safe_row_bytes =
      ghogx::character::source_rndbitmap_load_safely_plan(
          64, 64, 8, 4, 512, 512, 0);
  ok &= expect_bool(safe_row_bytes.row_bytes_fallback, true,
                    "bitmap safe row byte fallback");
  const ghogx::character::SourceRndBitmapLoadSafelyPlan safe_ok =
      ghogx::character::source_rndbitmap_load_safely_plan(
          64, 64, 8, 64, 512, 512, 2);
  ok &= expect_bool(safe_ok.reads_palette_and_pixels, true,
                    "bitmap safe reads payload");
  ok &= expect_bool(safe_ok.builds_mip_chain, true,
                    "bitmap safe builds mips");
  ok &= expect_bool(safe_ok.result, true, "bitmap safe result");

  const ghogx::character::SourceReadChunksPlan chunks =
      ghogx::character::source_read_chunks_plan(0x9001, 0x8000);
  ok &= expect_ints(chunks.chunk_sizes, {0x8000, 0x1001},
                    "read chunks sizes");

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

  std::vector<uint8_t> render_target;
  put_u32(render_target, 11);
  put_object_fields_minimal(render_target);
  put_i32(render_target, 512);
  put_i32(render_target, 1024);
  put_i32(render_target, 32);
  put_str(render_target, "");
  put_f32(render_target, -8.0f);
  put_i32(render_target, 2);
  put_u8(render_target, 0);
  put_u8(render_target, 0);
  const ghogx::character::RndTex render_target_decoded =
      ghogx::character::decode_rnd_tex("render_target.tex", render_target);
  ok &= expect_int(render_target_decoded.width, 256,
                   "render target clamp decoded width");
  ok &= expect_int(render_target_decoded.height, 256,
                   "render target clamp decoded height");

  std::vector<uint8_t> movie_target = render_target;
  const ghogx::character::RndTex movie_target_decoded =
      ghogx::character::decode_rnd_tex("movie.tex", movie_target);
  ok &= expect_int(movie_target_decoded.width, 512,
                   "movie target keeps decoded width");
  ok &= expect_int(movie_target_decoded.height, 1024,
                   "movie target keeps decoded height");

  std::cout << "character_tex_source_test " << (ok ? "OK" : "FAIL") << "\n";
  return ok ? 0 : 1;
}
