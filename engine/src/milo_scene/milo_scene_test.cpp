// engine/src/milo_scene/milo_scene_test.cpp
//
// Hermetic unit tests for the MILO render-object decoders. We hand-build the
// exact GH2 PS2 byte layouts (no ARK / no I/O) so the decoder's field offsets
// are pinned by an in-repo oracle. Byte layouts mirror real entries decoded
// from track/gen/track.milo_ps2 (green_gem.mesh, gem.mat, track_fade.trans).

#include "milo_scene/milo_scene.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

using namespace ghogx::milo_scene;
namespace fs = std::filesystem;

// Assert that works regardless of NDEBUG (unit tests must check unconditionally).
#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::printf("  [FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
      std::exit(1);                                                          \
    }                                                                        \
  } while (0)

void put_u32(std::vector<uint8_t>& b, uint32_t v) {
  for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
void put_f32(std::vector<uint8_t>& b, float f) {
  uint32_t v;
  std::memcpy(&v, &f, 4);
  put_u32(b, v);
}
void put_u16(std::vector<uint8_t>& b, uint16_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xff));
  b.push_back(static_cast<uint8_t>(v >> 8));
}
void put_str(std::vector<uint8_t>& b, const std::string& s) {
  put_u32(b, static_cast<uint32_t>(s.size()));
  for (char c : s) b.push_back(static_cast<uint8_t>(c));
}
void put_zeros(std::vector<uint8_t>& b, size_t n) {
  for (size_t i = 0; i < n; ++i) b.push_back(0);
}
// Identity rotation + given translation, as a Harmonix 3x4 matrix.
void put_matrix(std::vector<uint8_t>& b, float tx, float ty, float tz) {
  put_f32(b, 1); put_f32(b, 0); put_f32(b, 0);
  put_f32(b, 0); put_f32(b, 1); put_f32(b, 0);
  put_f32(b, 0); put_f32(b, 0); put_f32(b, 1);
  put_f32(b, tx); put_f32(b, ty); put_f32(b, tz);
}

bool approx(float a, float b) { return std::fabs(a - b) < 1e-4f; }

std::string first_existing(const std::string& dir,
                           std::initializer_list<const char*> names) {
  for (const char* n : names) {
    fs::path p = fs::path(dir) / n;
    if (fs::exists(p)) return p.string();
  }
  return {};
}

void test_trans() {
  std::vector<uint8_t> b;
  put_u32(b, 9);                 // version
  put_zeros(b, 9);               // base metadata
  put_matrix(b, 0, 120.0f, 0);   // local matrix (ty=120, like track_fade.trans)
  put_matrix(b, 0, 120.0f, 0);   // world matrix (identical)
  put_zeros(b, 9);               // constraint/flags
  put_str(b, "track_surface5.view");

  TransObj t = decode_trans("track_fade.trans", b);
  CHECK(t.name == "track_fade.trans");
  CHECK(t.parent == "track_surface5.view");
  CHECK(approx(t.local.pos[1], 120.0f));
  CHECK(approx(t.local.rot[0][0], 1.0f) && approx(t.local.rot[2][2], 1.0f));
  std::printf("  [ok] Trans: parent=%s pos.y=%.1f\n", t.parent.c_str(),
              t.local.pos[1]);
}

void test_mat() {
  std::vector<uint8_t> b;
  put_u32(b, 27);                // version 0x1b
  put_zeros(b, 9);               // base metadata
  put_u32(b, 4);                 // kBlendSrcAlphaAdd
  put_f32(b, 1); put_f32(b, 1); put_f32(b, 1); put_f32(b, 1);  // colour RGBA
  b.push_back(1);                // use_environ
  b.push_back(0);                // prelit
  put_u32(b, 1);                 // kZModeNormal
  b.push_back(1);                // alpha_cut
  b.push_back(0);                // alpha_write
  put_u32(b, 0);                 // kTexGenNone
  put_u32(b, 1);                 // kTexWrapRepeat
  const float tex_xfm[3][3] = {
      {1.06f, 0.0f, 0.0f},
      {0.0f, 1.22f, 0.0f},
      {0.0f, 0.0f, 1.84f},
  };
  for (const auto& row : tex_xfm)
    for (float value : row)
      put_f32(b, value);
  put_f32(b, 0.0f);
  put_f32(b, -0.14f);
  put_f32(b, 0.0f);
  put_str(b, "gem.tex");         // diffuse texture
  put_str(b, "");                // next pass
  b.push_back(0);                // intensify
  b.push_back(1);                // cull

  MatObj m = decode_mat("gem.mat", b);
  CHECK(m.decoded);
  CHECK(m.diffuse_tex == "gem.tex");
  CHECK(approx(m.color[0], 1.0f));
  CHECK(m.blend == 4);
  CHECK(m.use_environ);
  CHECK(!m.prelit);
  CHECK(m.z_mode == 1);
  CHECK(m.alpha_cut);
  CHECK(!m.alpha_write);
  CHECK(m.tex_wrap == 1);
  CHECK(!m.intensify);
  CHECK(m.cull);
  CHECK(approx(m.tex_xfm[0][0], 1.06f));
  CHECK(approx(m.tex_xfm[1][1], 1.22f));
  CHECK(approx(m.tex_xfm[2][0], 0.0f));
  CHECK(approx(m.tex_xfm[2][1], 0.0f));
  CHECK(approx(m.tex_xfm[2][2], 1.0f));
  std::printf("  [ok] Mat: tex=%s blend=%u color=(%.0f,%.0f,%.0f,%.0f)\n",
              m.diffuse_tex.c_str(), static_cast<unsigned>(m.blend),
              m.color[0], m.color[1], m.color[2], m.color[3]);
}

void test_light() {
  std::vector<uint8_t> b;
  put_u32(b, 6);                 // Light version
  put_zeros(b, 9);               // Hmx::Object metadata
  put_u32(b, 9);                 // RndTransformable revision
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 0);                 // constraint
  put_str(b, "");                // target
  b.push_back(0);                // preserve_scale
  put_str(b, "stage_root.grp");  // parent
  put_f32(b, 0.1f); put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 1.0f);
  put_f32(b, 500.0f);
  put_u32(b, 1);                 // kLightDirectional
  b.push_back(1);                // animate_color_from_preset
  b.push_back(0);                // animate_position_from_preset

  LightObj light = decode_light("stage_light_02.lit", b);
  CHECK(light.decoded);
  CHECK(light.source_order_decoded);
  CHECK(light.parent == "stage_root.grp");
  CHECK(approx(light.world_stored.pos[0], 40.0f));
  CHECK(approx(light.color[2], 0.3f));
  CHECK(approx(light.range, 500.0f));
  CHECK(light.type == 1);
  CHECK(light.animate_color_from_preset);
  CHECK(!light.animate_position_from_preset);
  CHECK(light.animate_range_from_preset);
  std::printf("  [ok] Light: type=%d color=(%.1f,%.1f,%.1f) range=%.1f\n",
              light.type, light.color[0], light.color[1], light.color[2],
              light.range);
}

void test_environ_with_lights() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 2);                 // dynamic light ref count
  put_str(b, "stage_light_02.lit");
  put_str(b, "stage_light_03.lit");
  put_f32(b, 0.25f); put_f32(b, 0.5f); put_f32(b, 0.75f); put_f32(b, 1.0f);
  put_f32(b, 0.0f);
  put_f32(b, 1.0f);
  put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  b.push_back(0);                 // fog_enable
  b.push_back(1);                 // animate_from_preset
  b.push_back(1);                 // fade_out
  put_f32(b, 120.0f);             // fade_start
  put_f32(b, 1000.0f);            // fade_end

  EnvironObj env = decode_environ("stage.env", b);
  CHECK(env.decoded);
  CHECK(env.source_order_decoded);
  CHECK(env.revision == 5);
  CHECK(env.lights.size() == 2);
  CHECK(env.lights[0] == "stage_light_02.lit");
  CHECK(approx(env.color_a[0], 0.25f));
  CHECK(approx(env.color_a[2], 0.75f));
  CHECK(!env.fog_enabled);
  CHECK(env.animate_from_preset);
  CHECK(env.fade_out);
  CHECK(approx(env.fade_start, 120.0f));
  CHECK(approx(env.fade_end, 1000.0f));
  CHECK(approx(env.range, 1000.0f));
  std::printf("  [ok] Environ: lights=%zu ambient=(%.2f,%.2f,%.2f)\n",
              env.lights.size(), env.color_a[0], env.color_a[1],
              env.color_a[2]);
}

void test_environ_with_extensionless_light() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 1);                 // dynamic light ref count
  put_str(b, "curtain");         // GH2 PS2 Big uses Light__curtain
  put_f32(b, 0.30f); put_f32(b, 0.30f); put_f32(b, 0.30f); put_f32(b, 1.0f);
  put_f32(b, 250.0f);
  put_f32(b, 1.0f);
  put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  b.push_back(0);                 // fog_enable
  b.push_back(1);                 // animate_from_preset
  b.push_back(0);                 // fade_out
  put_f32(b, 300.0f);             // fade_start
  put_f32(b, 1000.0f);            // fade_end

  EnvironObj env = decode_environ("curtain_light", b);
  CHECK(env.decoded);
  CHECK(env.source_order_decoded);
  CHECK(env.lights.size() == 1);
  CHECK(env.lights[0] == "curtain");
  CHECK(approx(env.color_a[0], 0.30f));
  CHECK(approx(env.range_a, 250.0f));
  CHECK(approx(env.fog_start, 250.0f));
  CHECK(env.animate_from_preset);
  CHECK(!env.fade_out);
  CHECK(approx(env.fade_start, 300.0f));
  CHECK(approx(env.fade_end, 1000.0f));
  CHECK(approx(env.range, 1000.0f));
  std::printf("  [ok] Environ extensionless light: %s -> %s\n",
              env.name.c_str(), env.lights[0].c_str());
}

void test_environ_with_fog() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 0);                 // dynamic light ref count
  put_f32(b, 0.07f); put_f32(b, 0.04f); put_f32(b, 0.14f); put_f32(b, 1.0f);
  put_f32(b, 0.0f);              // fog_start
  put_f32(b, 3000.0f);           // fog_end
  put_f32(b, 0.5f); put_f32(b, 0.0f); put_f32(b, 0.5f); put_f32(b, 1.0f);
  b.push_back(1);                 // fog_enable
  b.push_back(0);                 // animate_from_preset
  b.push_back(1);                 // fade_out
  put_f32(b, 10.0f);              // fade_start
  put_f32(b, 1000.0f);            // fade_end

  EnvironObj env = decode_environ("op_Art_projection.env", b);
  CHECK(env.decoded);
  CHECK(env.source_order_decoded);
  CHECK(env.fog_enabled);
  CHECK(!env.animate_from_preset);
  CHECK(approx(env.fog_start, 0.0f));
  CHECK(approx(env.fog_end, 3000.0f));
  CHECK(approx(env.fog_color[0], 0.5f));
  CHECK(approx(env.fog_color[2], 0.5f));
  CHECK(env.fade_out);
  CHECK(approx(env.fade_start, 10.0f));
  std::printf("  [ok] Environ fog: %s start=%.0f end=%.0f\n",
              env.name.c_str(), env.fog_start, env.fog_end);
}

void test_spotlight_source_order_rev20() {
  std::vector<uint8_t> b;
  put_u32(b, 20);                // GH2 PS2 Spotlight revision.
  put_zeros(b, 9);               // Hmx::Object metadata.
  put_u32(b, 3);                 // RndDrawable revision.
  b.push_back(1);                // showing.
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_f32(b, 2.0f);              // draw order.
  put_u32(b, 9);                 // RndTransformable revision.
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 0);                 // constraint.
  put_str(b, "");                // trans target.
  b.push_back(0);                // preserve_scale.
  put_str(b, "lighting_root.grp");
  put_f32(b, 1.5f);              // spot_scale.
  put_f32(b, 2.5f);              // spot_height.
  put_u32(b, 3);                 // pre-rev23 ObjVector<BeamDef> count.
  b.push_back(1);                // BeamDef::mIsCone.
  put_f32(b, 100.0f);            // length.
  put_f32(b, 10.0f);             // bottom_radius.
  put_f32(b, 4.0f);              // top_radius.
  put_f32(b, 0.2f);              // top_side_border.
  put_f32(b, 0.3f);              // bottom_side_border.
  put_f32(b, 0.4f);              // bottom_border.
  put_str(b, "beam.mat");
  put_f32(b, 0.75f);             // beam offset.
  put_f32(b, 1.0f); put_f32(b, 2.0f);  // target offset.
  b.push_back(0);                // extra beam consumed, but not selected.
  put_f32(b, 200.0f);
  put_f32(b, 20.0f);
  put_f32(b, 8.0f);
  put_f32(b, 0.2f);
  put_f32(b, 0.3f);
  put_f32(b, 0.4f);
  put_str(b, "beam_second.mat");
  put_f32(b, 0.80f);
  put_f32(b, 3.0f); put_f32(b, 4.0f);
  b.push_back(0);                // extra beam consumed, but not selected.
  put_f32(b, 300.0f);
  put_f32(b, 30.0f);
  put_f32(b, 12.0f);
  put_f32(b, 0.2f);
  put_f32(b, 0.3f);
  put_f32(b, 0.4f);
  put_str(b, "beam_third.mat");
  put_f32(b, 0.85f);
  put_f32(b, 5.0f); put_f32(b, 6.0f);
  put_str(b, "lightcan.grp");
  put_str(b, "bone_pelvis.mesh");
  put_f32(b, 3.25f);             // light_can_offset.
  put_f32(b, 0.1f); put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 0.4f);
  put_f32(b, 0.8f);              // intensity.
  put_str(b, "spot_circle.mat"); // disc material.
  put_f32(b, 0.6f);              // damping_constant.
  put_str(b, "legacy_symbol");
  put_str(b, "flare.mat");
  put_f32(b, 5.0f); put_f32(b, 6.0f);  // flare sizes.
  put_f32(b, 7.0f); put_f32(b, 8.0f);  // flare range.
  put_u32(b, 9);                 // flare steps.
  put_f32(b, 1.25f);             // flare offset.
  b.push_back(1);                // flare enabled.
  b.push_back(0);                // flare visibility test.
  put_f32(b, 11.0f);             // lens size.
  put_f32(b, 12.0f);             // lens offset.
  put_str(b, "lens.mat");
  put_u32(b, 2);                 // additional objects.
  put_str(b, "SPOT_circle.mesh");
  put_str(b, "beam_instance.mesh");
  b.push_back(1);                // target shadow.
  b.push_back(0);                // animate_color_from_preset.

  SpotlightObj spot = decode_spotlight("SHADOW_solo.spot", b);
  CHECK(spot.decoded);
  CHECK(spot.source_order_decoded);
  CHECK(spot.revision == 20);
  CHECK(spot.draw_revision == 3);
  CHECK(spot.trans_revision == 9);
  CHECK(spot.parent == "lighting_root.grp");
  CHECK(spot.group == "lightcan.grp");
  CHECK(spot.target == "bone_pelvis.mesh");
  CHECK(spot.material == "beam.mat");
  CHECK(spot.disc_material == "spot_circle.mat");
  CHECK(spot.circle_material == "spot_circle.mat");
  CHECK(spot.flare_material == "flare.mat");
  CHECK(spot.lens_material == "lens.mat");
  CHECK(spot.circle_mesh == "SPOT_circle.mesh");
  CHECK(spot.instance_meshes.size() == 2);
  CHECK(spot.instance_meshes[1] == "beam_instance.mesh");
  CHECK(spot.has_default_state);
  CHECK(approx(spot.default_color[1], 0.2f));
  CHECK(approx(spot.default_intensity, 0.8f));
  CHECK(approx(spot.world_stored.pos[0], 40.0f));
  CHECK(approx(spot.beam_length, 100.0f));
  CHECK(approx(spot.spot_scale, 1.5f));
  CHECK(approx(spot.light_can_offset, 3.25f));
  CHECK(spot.flare_steps == 9);
  CHECK(spot.flare_enabled);
  CHECK(!spot.flare_visibility_test);
  CHECK(spot.target_shadow);
  CHECK(!spot.animate_color_from_preset);
  CHECK(!spot.animate_orientation_from_preset);
  std::printf("  [ok] Spotlight: source rev=%u target=%s group=%s\n",
              spot.revision, spot.target.c_str(), spot.group.c_str());
}

void test_cam_projection_fields() {
  std::vector<uint8_t> b;
  put_u32(b, 12);                // Cam revision from GH2 PS2 metacam.
  put_zeros(b, 9);               // object/base metadata.
  put_u32(b, 9);                 // embedded Trans revision.
  put_matrix(b, 0.0f, -768.0f, 0.0f);
  put_matrix(b, 0.0f, -768.0f, 0.0f);
  put_u32(b, 0);                 // constraint.
  put_str(b, "");                // target.
  b.push_back(0);                // preserve_scale.
  put_str(b, "meta.cam");        // parent, as in meta_proxy.cam.
  put_f32(b, 50.0f);             // near.
  put_f32(b, 1000.0f);           // far.
  put_f32(b, 0.6024157f);        // vertical fov, stored as radians.
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_str(b, "");

  CamObj cam = decode_cam("meta_proxy.cam", b);
  CHECK(cam.decoded);
  CHECK(cam.parent == "meta.cam");
  CHECK(approx(cam.local.pos[1], -768.0f));
  CHECK(approx(cam.world_stored.pos[1], -768.0f));
  CHECK(approx(cam.near_plane, 50.0f));
  CHECK(approx(cam.far_plane, 1000.0f));
  CHECK(approx(cam.fov, 0.6024157f));
  CHECK(approx(cam.screen_rect[2], 1.0f));
  CHECK(approx(cam.screen_rect[3], 1.0f));
  CHECK(approx(cam.z_range[0], 0.0f));
  CHECK(approx(cam.z_range[1], 1.0f));
  std::printf(
      "  [ok] Cam: parent=%s near=%.0f far=%.0f fov=%.6f z=(%.0f,%.0f)\n",
      cam.parent.c_str(), cam.near_plane, cam.far_plane, cam.fov,
      cam.z_range[0], cam.z_range[1]);
}

void test_group_transform() {
  std::vector<uint8_t> b;
  put_u32(b, 13);                // GH2-era Group version with drawOnly.
  put_zeros(b, 9);               // Hmx::Object metadata.
  put_u32(b, 4);                 // RndAnimatable revision.
  put_f32(b, 0.0f);              // frame.
  put_u32(b, 0);                 // rate.
  put_u32(b, 9);                 // RndTransformable revision.
  put_matrix(b, 25.0f, 0.0f, -40.0f);
  put_matrix(b, 25.0f, 0.0f, 940.0f);
  put_zeros(b, 9);               // constraint, empty target, preserve_scale.
  put_str(b, "ss_setlist.view");
  put_u32(b, 3);                  // RndDrawable revision.
  b.push_back(1);                 // showing.
  put_zeros(b, 16);               // sphere.
  put_f32(b, 2.0f);               // draw order.
  put_u32(b, 3);                  // RndGroup objects.
  put_str(b, "paper.mesh");
  put_str(b, "title.lbl");
  put_str(b, "child.view");
  put_str(b, "lighting.env");
  put_str(b, "paper.mesh");       // drawOnly.
  put_str(b, "");                 // LOD.
  put_f32(b, 0.0f);               // LOD screen size.

  GroupObj group = decode_group("ss_songlist.view", b);
  CHECK(group.name == "ss_songlist.view");
  CHECK(group.decoded);
  CHECK(group.source_order_decoded);
  CHECK(group.has_transform);
  CHECK(group.parent == "ss_setlist.view");
  CHECK(group.showing);
  CHECK(approx(group.draw_order, 2.0f));
  CHECK(approx(group.local.pos[0], 25.0f));
  CHECK(approx(group.local.pos[2], -40.0f));
  CHECK(approx(group.world_stored.pos[2], 940.0f));
  CHECK(group.children.size() == 3);
  CHECK(group.children[0] == "paper.mesh");
  CHECK(group.children[1] == "title.lbl");
  CHECK(group.children[2] == "child.view");
  CHECK(group.environment_ref == "lighting.env");
  CHECK(group.draw_only == "paper.mesh");
  std::printf("  [ok] Group: parent=%s local.z=%.0f world.z=%.0f\n",
              group.parent.c_str(), group.local.pos[2],
              group.world_stored.pos[2]);
}

void test_group_draw_order_matches_rnddir_roots() {
  Scene scene;
  auto mesh = [](const char* name) {
    MeshObj out;
    out.name = name;
    out.decoded = true;
    out.showing = true;
    return out;
  };
  MeshObj root_mesh = mesh("root.mesh");
  root_mesh.draw_order = 0.5f;
  root_mesh.dir_index = 10;
  scene.meshes.push_back(root_mesh);
  scene.meshes.push_back(mesh("translucent.mesh"));
  scene.meshes.push_back(mesh("opaque.mesh"));
  scene.meshes.push_back(mesh("hidden_child.mesh"));

  GroupObj translucent;
  translucent.name = "translucent.grp";
  translucent.showing = true;
  translucent.draw_order = 1.0f;
  translucent.children.push_back("translucent.mesh");
  scene.groups.push_back(translucent);

  GroupObj hidden_parent;
  hidden_parent.name = "hidden_parent.grp";
  hidden_parent.showing = false;
  hidden_parent.draw_order = -1.0f;
  hidden_parent.children.push_back("hidden_child.grp");
  scene.groups.push_back(hidden_parent);

  GroupObj opaque;
  opaque.name = "opaque.grp";
  opaque.showing = true;
  opaque.draw_order = 0.0f;
  opaque.dir_index = 20;
  opaque.children.push_back("opaque.mesh");
  scene.groups.push_back(opaque);

  GroupObj hidden_child;
  hidden_child.name = "hidden_child.grp";
  hidden_child.showing = true;
  hidden_child.draw_order = -1.0f;
  hidden_child.children.push_back("hidden_child.mesh");
  scene.groups.push_back(hidden_child);

  rebuild_group_authored_draw_order_for_test(scene);
  CHECK(scene.draw_order.size() == 3);
  CHECK(scene.draw_order[0] == "opaque.mesh");
  CHECK(scene.draw_order[1] == "root.mesh");
  CHECK(scene.draw_order[2] == "translucent.mesh");
  CHECK(std::find(scene.draw_order.begin(), scene.draw_order.end(),
                  "hidden_child.mesh") == scene.draw_order.end());
  CHECK(std::find(scene.grouped_meshes.begin(), scene.grouped_meshes.end(),
                  "hidden_child.mesh") != scene.grouped_meshes.end());
  std::printf(
      "  [ok] Group draw roots: opaque before translucent, hidden child suppressed\n");
}

void test_band_placer() {
  std::vector<uint8_t> b;
  put_u32(b, 2);                 // BandPlacer version.
  put_zeros(b, 8);               // object/base header before the kind string.
  put_str(b, "char");
  b.push_back(0);                // PS2 BandPlacer kind strings are nul-padded.
  put_u32(b, 3);
  put_u32(b, 1);
  put_zeros(b, 0x2a - b.size());
  put_u32(b, 9);                 // embedded transform marker.
  put_matrix(b, -35.0f, -30.0f, -47.5f);
  put_matrix(b, -35.0f, -636.5f, -47.5f);
  put_zeros(b, 9);
  put_str(b, "mgs_camerafix.grp");
  put_str(b, "spot_ui.mesh");

  BandPlacerObj placer = decode_band_placer("char_multi0.placer", b);
  CHECK(placer.decoded);
  CHECK(placer.kind == "char");
  CHECK(placer.parent == "mgs_camerafix.grp");
  CHECK(approx(placer.local.pos[0], -35.0f));
  CHECK(approx(placer.world_stored.pos[1], -636.5f));
  CHECK(approx(placer.world_stored.pos[2], -47.5f));

  Scene sc;
  sc.band_placers.push_back(placer);
  CHECK(sc.find_band_placer("char_multi0.placer") != nullptr);
  CHECK(sc.find_band_placer("missing.placer") == nullptr);
  std::printf("  [ok] BandPlacer: kind=%s parent=%s world=(%.1f %.1f %.1f)\n",
              placer.kind.c_str(), placer.parent.c_str(),
              placer.world_stored.pos[0], placer.world_stored.pos[1],
              placer.world_stored.pos[2]);
}

void test_real_menu_band_placers() {
  const std::string ark_dir =
      "C:/Programming/GitHub/Guitar Hero II/gh2_ps2_hybrid_assets/gen";
  const std::string hdr = first_existing(ark_dir, {"main.hdr", "MAIN.HDR"});
  const std::string ark = first_existing(ark_dir, {"main_0.ark", "MAIN_0.ARK"});
  if (hdr.empty() || ark.empty()) {
    std::printf("  [skip] real menu BandPlacers (no PS2 archive)\n");
    return;
  }

  Scene single;
  CHECK(load_scene(hdr, ark, "ui/gen/sel_character.milo_ps2", single));
  const BandPlacerObj* single_placer =
      single.find_band_placer("char_single.placer");
  CHECK(single_placer != nullptr);
  CHECK(single_placer && single_placer->parent == "sel_character.view");

  Scene multi;
  CHECK(load_scene(hdr, ark, "ui/gen/multi_sel_character.milo_ps2", multi));
  CHECK(multi.find_band_placer("char_multi0.placer") != nullptr);
  CHECK(multi.find_band_placer("char_multi1.placer") != nullptr);

  Scene store;
  CHECK(load_scene(hdr, ark, "ui/gen/store.milo_ps2", store));
  CHECK(store.find_band_placer("char_store.placer") != nullptr);
  std::printf("  [ok] real menu BandPlacers: char_single, char_multi0/1, char_store\n");
}

void test_mesh() {
  std::vector<uint8_t> b;
  put_u32(b, 28);                // mesh version 0x1c
  // Trans base.
  put_u32(b, 9);                 // trans version
  put_zeros(b, 9);
  put_matrix(b, 1.0f, 2.0f, 3.0f);  // local (translation 1,2,3)
  put_matrix(b, 1.0f, 2.0f, 3.0f);  // world
  put_zeros(b, 9);
  put_str(b, "track.view");      // trans parent
  // Draw base.
  put_u32(b, 3);                 // draw version
  const size_t draw_showing_offset = b.size();
  b.push_back(1);                // showing
  put_zeros(b, 20);              // sphere + draw-order
  // Mesh fields.
  put_str(b, "gem.mat");         // material
  put_str(b, "tri.mesh");        // geometry owner
  put_u32(b, 0x1F);              // RndMesh::mMutable
  put_u32(b, 1);                 // volume
  b.push_back(0);                // null BSP-tree owner
  put_u32(b, 3);                 // vertex_count = 3
  // 3 vertices (pos / normal / colour / uv), forming a unit triangle.
  const float P[3][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  for (int i = 0; i < 3; ++i) {
    put_f32(b, P[i][0]); put_f32(b, P[i][1]); put_f32(b, P[i][2]);  // pos
    put_f32(b, 0); put_f32(b, 0); put_f32(b, 1);                    // normal
    put_f32(b, 1); put_f32(b, 1); put_f32(b, 1); put_f32(b, 1);     // colour
    put_f32(b, P[i][0]); put_f32(b, P[i][1]);                       // uv
  }
  put_u32(b, 1);                 // face_count = 1
  put_u16(b, 0); put_u16(b, 1); put_u16(b, 2);  // the triangle
  put_zeros(b, 8);               // trailing group data

  MeshObj m = decode_mesh("tri.mesh", b);
  if (!m.decoded) std::printf("  [FAIL] mesh error: %s\n", m.error.c_str());
  CHECK(m.decoded);
  CHECK(m.vertex_count == 3);
  CHECK(m.face_count == 1);
  CHECK(m.indices.size() == 3 && m.indices[2] == 2);
  CHECK(m.material == "gem.mat");
  CHECK(m.mutable_flags == 0x1F);
  CHECK(m.parent == "track.view");
  CHECK(m.showing);
  CHECK(approx(m.local.pos[0], 1.0f) && approx(m.local.pos[2], 3.0f));
  // bbox of the unit triangle.
  CHECK(approx(m.bb_min[0], 0.0f) && approx(m.bb_max[0], 1.0f));
  CHECK(approx(m.bb_max[1], 1.0f));
  std::printf("  [ok] Mesh: vtx=%u face=%u mat=%s parent=%s bbox=[%.0f,%.0f,%.0f]-[%.0f,%.0f,%.0f]\n",
              m.vertex_count, m.face_count, m.material.c_str(),
              m.parent.c_str(), m.bb_min[0], m.bb_min[1], m.bb_min[2],
              m.bb_max[0], m.bb_max[1], m.bb_max[2]);

  // World-matrix composition: a mesh under a Trans that translates by (10,0,0).
  Scene sc;
  TransObj parent;
  parent.name = "track.view";
  parent.local.pos[0] = 10.0f;
  sc.transes.push_back(parent);
  sc.meshes.push_back(m);
  auto w = sc.world_matrix(sc.meshes[0]);
  // local pos (1,2,3) composed with parent translate (10,0,0) -> (11,2,3).
  CHECK(approx(w[12], 11.0f));
  CHECK(approx(w[13], 2.0f));
  CHECK(approx(w[14], 3.0f));
  std::printf("  [ok] world compose: translation=(%.0f,%.0f,%.0f)\n", w[12],
              w[13], w[14]);

  Scene group_sc;
  GroupObj group;
  group.name = "track.view";
  group.has_transform = true;
  group.local.pos[0] = 10.0f;
  group.world_stored = group.local;
  group_sc.groups.push_back(group);
  group_sc.meshes.push_back(m);
  auto gw = group_sc.world_matrix(group_sc.meshes[0]);
  CHECK(approx(gw[12], 11.0f));
  CHECK(approx(gw[13], 2.0f));
  CHECK(approx(gw[14], 3.0f));
  std::printf("  [ok] group world compose: translation=(%.0f,%.0f,%.0f)\n",
              gw[12], gw[13], gw[14]);

  MeshObj authored = m;
  authored.world_stored.pos[0] = 100.0f;
  authored.world_stored.pos[1] = 200.0f;
  authored.world_stored.pos[2] = 300.0f;
  Scene authored_sc;
  authored_sc.groups.push_back(group);
  authored_sc.meshes.push_back(authored);
  auto aw = authored_sc.world_matrix(authored_sc.meshes[0]);
  CHECK(approx(aw[12], 100.0f));
  CHECK(approx(aw[13], 200.0f));
  CHECK(approx(aw[14], 300.0f));
  std::printf("  [ok] authored world wins: translation=(%.0f,%.0f,%.0f)\n",
              aw[12], aw[13], aw[14]);

  std::vector<uint8_t> hidden = b;
  hidden[draw_showing_offset] = 0;
  MeshObj h = decode_mesh("hidden.mesh", hidden);
  CHECK(h.decoded);
  CHECK(!h.showing);
}

void test_world_crowd_gh2_matrix_stride() {
  std::vector<uint8_t> b;
  put_u32(b, 6);                 // WorldCrowd revision used by GH2 PS2 chars.
  put_u32(b, 3);                 // RndDrawable revision.
  b.push_back(1);                // showing
  put_f32(b, 0); put_f32(b, 0); put_f32(b, 0); put_f32(b, 0);
  put_f32(b, 0);                 // draw order
  put_str(b, "Crowd_area.mesh"); // placement mesh
  put_u32(b, 2);                 // total placements
  b.push_back(0);                // pre-rev8 flag
  put_u32(b, 1);                 // actor count
  put_str(b, "crowd_male01");
  put_f32(b, 95.0f);             // height
  put_f32(b, 1.0f);              // density
  put_f32(b, 10.0f);             // radius
  put_u32(b, 2);                 // GH2 matrix-only placement count
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 1234);              // modifyStamp
  b.push_back(0);                // show3DOnly

  WorldCrowdObj crowd = decode_world_crowd("crowd", b);
  CHECK(crowd.decoded);
  CHECK(crowd.area_mesh == "Crowd_area.mesh");
  CHECK(crowd.total_placements == 2);
  CHECK(crowd.actors.size() == 1);
  CHECK(crowd.placement_sets.size() == 1);
  CHECK(crowd.placement_sets[0].placements.size() == 2);
  CHECK(approx(crowd.placement_sets[0].placements[0].pos[0], 10.0f));
  CHECK(approx(crowd.placement_sets[0].placements[0].pos[1], 20.0f));
  CHECK(approx(crowd.placement_sets[0].placements[0].pos[2], 30.0f));
  CHECK(approx(crowd.placement_sets[0].placements[1].pos[0], 40.0f));
  CHECK(approx(crowd.placement_sets[0].placements[1].pos[1], 50.0f));
  CHECK(approx(crowd.placement_sets[0].placements[1].pos[2], 60.0f));
  std::printf("  [ok] WorldCrowd rev6: placements=%u matrix stride\n",
              crowd.total_placements);
}

}  // namespace

int main() {
  std::printf("milo_scene_test\n");
  test_trans();
  test_mat();
  test_light();
  test_environ_with_lights();
  test_environ_with_extensionless_light();
  test_environ_with_fog();
  test_spotlight_source_order_rev20();
  test_cam_projection_fields();
  test_group_transform();
  test_group_draw_order_matches_rnddir_roots();
  test_band_placer();
  test_real_menu_band_placers();
  test_mesh();
  test_world_crowd_gh2_matrix_stride();
  std::printf("ALL PASS\n");
  return 0;
}
