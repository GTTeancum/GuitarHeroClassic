// engine/src/ui/menu_labels_test.cpp
//
// Verifies the main-menu BandButton labels and unaligned text/layout tail fields
// directly from the stock PS2 main.milo_ps2.

#include "ui/menu_labels.h"
#include "ui/menu_font.h"

#include "asset/milo_image.h"
#include "ark_v3.h"
#include "milo.h"
#include "milo_scene/milo_scene.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int g_failures = 0;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

static std::string first_existing(const std::string& dir, std::vector<std::string> names) {
  for (auto& n : names) {
    std::string p = dir + "/" + n;
    if (fs::exists(p)) return p;
  }
  return {};
}

static bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }

int main(int argc, char** argv) {
  std::string ark_dir =
      argc > 1 ? argv[1]
               : "C:/Programming/GitHub/Guitar Hero II/Guitar Hero II PS2 (USA)/GEN";
  std::string hdr = first_existing(ark_dir, {"MAIN.HDR", "main.hdr"});
  std::string ark0 = first_existing(ark_dir, {"MAIN_0.ARK", "main_0.ark"});
  if (hdr.empty() || ark0.empty()) {
    std::printf("ghogx_menu_labels_test: SKIP (no stock ARK at %s)\n", ark_dir.c_str());
    return 0;
  }

  CHECK(ghogx::asset::endgame_photo_bitmap_path_for_outfit("rock2") ==
        "ui/image/og/gen/photo_rock20_keep.bmp_ps2");
  CHECK(ghogx::asset::endgame_photo_bitmap_path_for_outfit(
            "char/metal1/og/gen/metal1.milo_ps2") ==
        "ui/image/og/gen/photo_metal10_keep.bmp_ps2");

  auto labels = ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/main.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> by_name;
  for (auto& l : labels) by_name[l.name] = l;

  struct Expect {
    const char* name;
    const char* parent;
    const char* text;
    const char* nav;
    float width;
  };
  const Expect expects[] = {
      {"main_career.btn", "main_buttons.view", "CAREER",
       "main_quickspin.btn", 310.0f},
      {"main_quickspin.btn", "main_buttons.view", "QUICK_PLAY",
       "main_multiplayer.btn", 320.0f},
      {"main_multiplayer.btn", "main_buttons.view", "MULTIPLAYER",
       "main_tutorial.btn", 310.0f},
      {"main_tutorial.btn", "main_buttons.view", "TRAINING",
       "main_options.btn", 290.0f},
      {"main_options.btn", "main_buttons.view", "OPTIONS",
       "main_career.btn", 270.0f},
  };

  for (const auto& e : expects) {
    auto it = by_name.find(e.name);
    CHECK(it != by_name.end());
    if (it == by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandButton");
    CHECK(lbl.font == "impact");
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.text == e.text);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.button_tail.valid);
    if (lbl.button_tail.valid) {
      CHECK(lbl.button_tail.legacy_layout);
      CHECK(lbl.button_tail.all_caps == 1);
      CHECK(near(lbl.button_tail.width, e.width));
      CHECK(near(lbl.button_tail.height, 15.0f));
      CHECK(near(lbl.button_tail.leading, 1.0f));
      CHECK(lbl.button_tail.unknown_10 == 34);
      CHECK(near(lbl.button_tail.text_size, 0.5f));
      CHECK(near(lbl.button_tail.unknown_20, 1.0f));
      CHECK(lbl.button_tail.unknown_24 == 1);
      CHECK(near(lbl.button_tail.kerning, -0.05f));
      CHECK(near(lbl.button_tail.wrap_width, 30.0f));
      CHECK(near(lbl.button_tail.width_bound, 280.0f));
    }
  }

  const auto main_button_anim = ghogx::ui::extract_menu_slider_anim(
      hdr, ark0, "ui/gen/main.milo_ps2", "1");
  CHECK(main_button_anim.valid);
  if (main_button_anim.valid) {
    CHECK(main_button_anim.target.empty());
    CHECK(main_button_anim.keys_owner == "1");
    CHECK(main_button_anim.rotation_keys.size() == 1);
    CHECK(main_button_anim.translation_keys.empty());
    CHECK(main_button_anim.scale_keys.size() == 5);
    CHECK(near(main_button_anim.first_frame, 0.0f));
    CHECK(near(main_button_anim.last_frame, 4.0f));
    if (main_button_anim.scale_keys.size() == 5) {
      CHECK(near(main_button_anim.scale_keys[0].value[0], 1.0f));
      CHECK(near(main_button_anim.scale_keys[2].value[0], 1.05f));
      CHECK(near(main_button_anim.scale_keys[4].value[0], 1.1f));
      CHECK(near(main_button_anim.scale_keys[4].value[1], 1.0f));
      CHECK(near(main_button_anim.scale_keys[4].value[2], 1.1f));
    }
    std::printf("main button TransAnim: target='%s' owner='%s' frames=%.3f..%.3f "
                "rot=%zu trans=%zu scale=%zu\n",
                main_button_anim.target.c_str(),
                main_button_anim.keys_owner.c_str(),
                main_button_anim.first_frame, main_button_anim.last_frame,
                main_button_anim.rotation_keys.size(),
                main_button_anim.translation_keys.size(),
                main_button_anim.scale_keys.size());
    for (const auto& key : main_button_anim.scale_keys) {
      std::printf("  scale frame=%.3f value=(%.3f %.3f %.3f)\n", key.frame,
                  key.value[0], key.value[1], key.value[2]);
    }
  }

  // Full stock GH2 PS2 UITrigger inventory. Harmonix UITrigger rev 0 stores a
  // single event/anim pair; a null anim never blocks even when the authored
  // block_transition bit is set.
  struct TriggerExpect {
    const char* milo;
    const char* name;
    const char* anim;
    bool blocking;
  };
  const TriggerExpect trigger_expects[] = {
      {"ui/gen/chooseprof.milo_ps2", "chooseprof.trg",
       "notebook_cover.filt", true},
      {"ui/gen/endgame.milo_ps2", "receipt_on.trg", "receipt_on.filt",
       true},
      {"ui/gen/endgame_stats.milo_ps2", "receipt_on.trg", "", true},
      {"ui/gen/endgame_stats_multi.milo_ps2", "receipt_on.trg", "", true},
      {"ui/gen/manage_band.milo_ps2", "manage_band.trg",
       "notebook_cover.filt", true},
      {"ui/gen/multi_char_outfit1.milo_ps2", "bounce.trg", "bounce.tnm",
       false},
      {"ui/gen/multi_char_outfit2.milo_ps2", "bounce.trg", "bounce.tnm",
       false},
      {"ui/gen/store.milo_ps2", "UITrigger.trg", "AnimFilter.filt", true},
  };
  std::size_t trigger_count = 0;
  for (const auto& e : trigger_expects) {
    const auto triggers =
        ghogx::ui::extract_menu_ui_triggers(hdr, ark0, e.milo);
    CHECK(triggers.size() == 1);
    trigger_count += triggers.size();
    if (triggers.empty()) continue;
    const auto& trigger = triggers.front();
    CHECK(trigger.valid);
    CHECK(trigger.revision == 0);
    CHECK(trigger.component_revision == 1);
    CHECK(trigger.name == e.name);
    CHECK(trigger.event == "ui_enter");
    CHECK(trigger.anim_ref == e.anim);
    CHECK(trigger.block_transition == e.blocking);
  }
  CHECK(trigger_count == 8);
  CHECK(ghogx::ui::extract_menu_ui_triggers(
            hdr, ark0, "ui/gen/main.milo_ps2")
            .empty());

  // Every shipped screen-resource animation must decode from source order;
  // this prevents a visually quiet menu from masking a dropped animation
  // class or a parser that only happens to fit one hand-picked panel.
  {
    const auto stock_ark = gh::ark::ArkV3Reader::load(hdr);
    std::size_t ui_milos = 0;
    std::size_t trans_entries = 0;
    std::size_t trans_decoded = 0;
    std::size_t material_entries = 0;
    std::size_t material_decoded = 0;
    std::size_t panel_dirs = 0;
    std::size_t panel_dir_configs = 0;
    std::size_t env_anim_entries = 0;
    std::size_t env_anim_decoded = 0;
    std::size_t screen_mask_entries = 0;
    std::size_t screen_mask_decoded = 0;
    std::map<std::string, ghogx::milo_scene::PanelDirConfig> panel_configs;
    std::map<std::string, std::size_t> type_entries;
    std::size_t bandbutton_tails = 0;
    std::size_t legacy_bandbutton_tails = 0;
    std::size_t modern_bandbutton_tails = 0;
    std::map<std::string, std::size_t> legacy_bandbuttons_by_milo;
    std::map<std::string, std::size_t> transform_target_types;
    std::map<std::string, std::size_t> material_target_types;
    for (const auto& ark_entry : stock_ark.entries()) {
      if (ark_entry.full_path.rfind("ui/gen/", 0) != 0 ||
          ark_entry.full_path.size() < 9 ||
          ark_entry.full_path.compare(ark_entry.full_path.size() - 9, 9,
                                      ".milo_ps2") != 0)
        continue;
      ++ui_milos;
      const auto bytes = stock_ark.read_entry(ark_entry, {ark0});
      const auto header = gh::milo::parse_header(bytes);
      const auto payload = gh::milo::inflate_payload(bytes, header);
      const auto dir = gh::milo::parse_directory(payload);
      if (dir.dir_type == "PanelDir" &&
          dir.dir_entry_offset <= payload.size() &&
          dir.dir_entry_size <= payload.size() - dir.dir_entry_offset) {
        ++panel_dirs;
        const std::vector<std::uint8_t> root(
            payload.begin() + dir.dir_entry_offset,
            payload.begin() + dir.dir_entry_offset + dir.dir_entry_size);
        const auto config = ghogx::milo_scene::decode_panel_dir_config(root);
        if (config.valid) ++panel_dir_configs;
        panel_configs.emplace(ark_entry.full_path, config);
      }
      if (std::getenv("GHOGX_MENU_OBJECT_AUDIT") &&
          dir.dir_entry_offset + dir.dir_entry_size <= payload.size()) {
        std::vector<std::string> root_strings;
        const std::size_t root_end = static_cast<std::size_t>(
            dir.dir_entry_offset + dir.dir_entry_size);
        for (std::size_t pos = static_cast<std::size_t>(dir.dir_entry_offset);
             pos < root_end;) {
          while (pos < root_end && (payload[pos] < 32 || payload[pos] >= 127))
            ++pos;
          const std::size_t begin = pos;
          while (pos < root_end && payload[pos] >= 32 && payload[pos] < 127)
            ++pos;
          if (pos - begin >= 3)
            root_strings.emplace_back(
                reinterpret_cast<const char*>(payload.data() + begin),
                pos - begin);
        }
        const bool has_camera_or_environment =
            std::any_of(root_strings.begin(), root_strings.end(),
                        [](const std::string& value) {
                          return value.find(".cam") != std::string::npos ||
                                 value.find(".env") != std::string::npos;
                        });
        if (has_camera_or_environment) {
          std::printf("menu root: %s type=%s size=%llu strings=",
                      ark_entry.full_path.c_str(), dir.dir_type.c_str(),
                      static_cast<unsigned long long>(dir.dir_entry_size));
          for (const std::string& value : root_strings)
            std::printf("[%s]", value.c_str());
          std::printf("\n");
        }
      }
      std::map<std::string, std::string> type_by_name;
      for (const auto& entry : dir.entries)
        type_by_name.emplace(entry.name, entry.type);
      for (const auto& entry : dir.entries) {
        ++type_entries[entry.type];
        if (entry.type == "EnvAnim" &&
            entry.offset + entry.size <= payload.size()) {
          ++env_anim_entries;
          const std::vector<std::uint8_t> env_body(
              payload.begin() + entry.offset,
              payload.begin() + entry.offset + entry.size);
          const auto env_anim =
              ghogx::milo_scene::decode_env_anim(entry.name, env_body);
          if (env_anim.decoded) ++env_anim_decoded;
          if (ark_entry.full_path == "ui/gen/metacam.milo_ps2") {
            CHECK(env_anim.environment == "ui.env");
            CHECK(env_anim.keys_owner == "ui.enm");
            CHECK(env_anim.ambient_color_keys.size() == 2);
          }
        }
        if (entry.type == "ScreenMask" &&
            entry.offset + entry.size <= payload.size()) {
          ++screen_mask_entries;
          const std::vector<std::uint8_t> mask_body(
              payload.begin() + entry.offset,
              payload.begin() + entry.offset + entry.size);
          const auto mask =
              ghogx::milo_scene::decode_screen_mask(entry.name, mask_body);
          if (mask.decoded) ++screen_mask_decoded;
          CHECK(mask.material == "light.mat");
          CHECK(mask.color[3] == 1.0f);
          CHECK(mask.rect[0] == 0.0f && mask.rect[1] == 0.0f &&
                mask.rect[2] == 1.0f && mask.rect[3] == 1.0f);
        }
        if (std::getenv("GHOGX_MENU_OBJECT_AUDIT") &&
            (entry.type == "EnvAnim" || entry.type == "ScreenMask" ||
             entry.type == "UIPicture" || entry.type == "UIProxy" ||
             entry.type == "Spotlight" || entry.type == "Environ" ||
             entry.type == "Light" || entry.type == "UIList" ||
             entry.type == "BandSlider" || entry.type == "BandPlacer")) {
          std::printf("menu object: %s %s/%s size=%llu\n", entry.type.c_str(),
                      ark_entry.full_path.c_str(), entry.name.c_str(),
                      static_cast<unsigned long long>(entry.size));
        }
        if ((entry.type != "TransAnim" && entry.type != "MatAnim") ||
            entry.offset + entry.size > payload.size())
          continue;
        const std::vector<std::uint8_t> body(
            payload.begin() + entry.offset,
            payload.begin() + entry.offset + entry.size);
        if (entry.type == "TransAnim") {
          ++trans_entries;
          const auto decoded =
              ghogx::ui::decode_menu_trans_anim_body(body, entry.name);
          if (decoded.valid) {
            ++trans_decoded;
            const auto target = type_by_name.find(decoded.target);
            const std::string target_type =
                target == type_by_name.end()
                    ? (decoded.target.empty() ? "<null>" : "<external>")
                    : target->second;
            ++transform_target_types[target_type];
            if (std::getenv("GHOGX_MENU_OBJECT_AUDIT"))
              std::printf("menu TransAnim: %s/%s target=%s type=%s frames=%.3f..%.3f\n",
                          ark_entry.full_path.c_str(), entry.name.c_str(),
                          decoded.target.c_str(), target_type.c_str(),
                          decoded.first_frame, decoded.last_frame);
          }
        } else {
          ++material_entries;
          const auto decoded =
              ghogx::ui::decode_menu_material_anim_body(body, entry.name);
          if (decoded.valid) {
            ++material_decoded;
            const auto target = type_by_name.find(decoded.material);
            ++material_target_types[target == type_by_name.end()
                                        ? (decoded.material.empty() ? "<null>"
                                                                    : "<external>")
                                        : target->second];
          } else {
            std::fprintf(stderr, "undecoded MatAnim: %s/%s size=%llu\n",
                         ark_entry.full_path.c_str(), entry.name.c_str(),
                         static_cast<unsigned long long>(entry.size));
          }
        }
      }
      const auto milo_labels =
          ghogx::ui::extract_menu_labels(hdr, ark0, ark_entry.full_path);
      for (const auto& label : milo_labels) {
        if (label.type != "BandButton" || !label.button_tail.valid) continue;
        ++bandbutton_tails;
        if (label.button_tail.legacy_layout) {
          ++legacy_bandbutton_tails;
          ++legacy_bandbuttons_by_milo[ark_entry.full_path];
          if (std::getenv("GHOGX_MENU_OBJECT_AUDIT"))
            std::printf("legacy BandButton: %s/%s parent=%s font=%s "
                        "size=%.3f box=(%.3f %.3f) bound=%.3f "
                        "world_t=(%.3f %.3f %.3f)\n",
                        ark_entry.full_path.c_str(), label.name.c_str(),
                        label.parent.c_str(), label.font.c_str(),
                        label.button_tail.text_size,
                        label.button_tail.width, label.button_tail.height,
                        label.button_tail.width_bound, label.world[9],
                        label.world[10], label.world[11]);
        } else {
          ++modern_bandbutton_tails;
        }
      }
    }
    CHECK(panel_dirs > 100);
    CHECK(panel_dir_configs == panel_dirs);
    CHECK(env_anim_entries == 2);
    CHECK(env_anim_decoded == env_anim_entries);
    CHECK(screen_mask_entries == 7);
    CHECK(screen_mask_decoded == screen_mask_entries);
    {
      ghogx::milo_scene::Scene unlock_scene;
      CHECK(ghogx::milo_scene::load_scene(
          hdr, ark0, "ui/gen/unlockvenue1.milo_ps2", unlock_scene));
      const auto camera = std::find_if(
          unlock_scene.cams.begin(), unlock_scene.cams.end(),
          [](const ghogx::milo_scene::CamObj& value) {
            return value.name == "camera1-2.cam";
          });
      CHECK(camera != unlock_scene.cams.end());
      if (camera != unlock_scene.cams.end()) {
        CHECK(camera->parent == "unlockvenue1");
        const auto world = unlock_scene.world_matrix(*camera);
        for (float value : world) CHECK(std::isfinite(value));
        CHECK(world[15] == 1.0f);
      }
      CHECK(unlock_scene.screen_masks.size() == 1);
      if (!unlock_scene.screen_masks.empty()) {
        const auto& mask = unlock_scene.screen_masks.front();
        CHECK(mask.decoded && mask.showing);
        CHECK(!mask.use_camera_rect);
        const std::array<float, 4> full_rect{0, 0, 1, 1};
        CHECK(mask.rect == full_rect);
      }
    }
    const auto main_config = panel_configs.find("ui/gen/main.milo_ps2");
    CHECK(main_config != panel_configs.end());
    if (main_config != panel_configs.end()) {
      CHECK(main_config->second.environment == "ui.env");
      CHECK(main_config->second.camera == "meta.cam");
      CHECK(main_config->second.enter_event == "ui_enter");
    }
    const auto unlock_config =
        panel_configs.find("ui/gen/unlockvenue1.milo_ps2");
    CHECK(unlock_config != panel_configs.end());
    if (unlock_config != panel_configs.end())
      CHECK(unlock_config->second.camera == "camera1-2.cam");
    const auto character_config =
        panel_configs.find("ui/gen/sel_character.milo_ps2");
    CHECK(character_config != panel_configs.end());
    if (character_config != panel_configs.end())
      CHECK(character_config->second.camera == "meta_proxy.cam");
    std::printf("menu PanelDir corpus: decoded=%zu/%zu\n",
                panel_dir_configs, panel_dirs);
    std::printf("menu animation corpus: milos=%zu trans=%zu/%zu mat=%zu/%zu\n",
                ui_milos, trans_decoded, trans_entries, material_decoded,
                material_entries);
    std::printf("menu object corpus:");
    for (const auto& [type, count] : type_entries)
      std::printf(" %s=%zu", type.c_str(), count);
    std::printf("\n");
    std::printf("menu BandButton tail corpus: decoded=%zu legacy=%zu modern=%zu\n",
                bandbutton_tails, legacy_bandbutton_tails,
                modern_bandbutton_tails);
    for (const auto& [milo, count] : legacy_bandbuttons_by_milo)
      std::printf("  legacy BandButton tails: %s=%zu\n", milo.c_str(), count);
    CHECK(bandbutton_tails > 0);
    CHECK(legacy_bandbutton_tails > 0);
    CHECK(modern_bandbutton_tails > 0);
    std::printf("menu TransAnim target corpus:");
    for (const auto& [type, count] : transform_target_types)
      std::printf(" %s=%zu", type.c_str(), count);
    std::printf("\nmenu MatAnim target corpus:");
    for (const auto& [type, count] : material_target_types)
      std::printf(" %s=%zu", type.c_str(), count);
    std::printf("\n");
    CHECK(ui_milos == 152);
    CHECK(trans_entries == 128);
    CHECK(trans_decoded == trans_entries);
    CHECK(material_entries == 73);
    CHECK(material_decoded == material_entries);
  }

  // endgame's authored enter trigger is a timing-only transition in the
  // shipped file.  Its filter spans twenty UI frames, but receipt_on.tnm has
  // an explicitly null transform target.  RndTransAnim::Load resolves an
  // empty ObjPtr to null; it is not an implicit self/keys-owner target.
  const auto receipt_filter = ghogx::ui::extract_menu_anim_filter(
      hdr, ark0, "ui/gen/endgame.milo_ps2", "receipt_on.filt");
  CHECK(receipt_filter.valid);
  if (receipt_filter.valid) {
    CHECK(receipt_filter.trans_anim == "receipt_on.tnm");
    CHECK(near(receipt_filter.scale, 0.05f));
    CHECK(near(receipt_filter.offset, 0.0f));
    CHECK(near(receipt_filter.start, 0.0f));
    CHECK(near(receipt_filter.end, 1.0f));
    CHECK(receipt_filter.type == 0);
  }
  const auto receipt_anim = ghogx::ui::extract_menu_slider_anim(
      hdr, ark0, "ui/gen/endgame.milo_ps2", "receipt_on.tnm");
  CHECK(receipt_anim.valid);
  if (receipt_anim.valid) {
    CHECK(receipt_anim.target.empty());
    CHECK(receipt_anim.keys_owner == "receipt_on.tnm");
  }

  auto multi_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/multi.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> multi_by_name;
  for (auto& l : multi_labels) {
    std::printf("multi label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f) "
                "button_tail=(valid=%d fit=%d w=%.3f h=%.3f size=%.3f "
                "legacy=%d align=%d bound=%.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11],
                l.button_tail.valid ? 1 : 0, l.button_tail.fit_text,
                l.button_tail.width, l.button_tail.height,
                l.button_tail.text_size,
                l.button_tail.legacy_layout ? 1 : 0,
                l.button_tail.alignment,
                l.button_tail.width_bound);
    multi_by_name[l.name] = l;
  }
  struct MultiExpect {
    const char* name;
    const char* type;
    const char* text;
    const char* nav;
    float x;
    float y;
    float z;
  };
  const MultiExpect multi_expects[] = {
      {"selectmode.lbl", "BandLabel", "SELECT MULTIPLAYER MODE", "", 154.935f,
       0.0f, 147.487f},
      {"coop.btn", "BandButton", "multi_coop", "versus.btn", 155.808f, 0.0f,
       122.504f},
      {"versus.btn", "BandButton", "multi_versus", "faceoff.btn", 156.855f,
       0.0f, 92.524f},
      {"faceoff.btn", "BandButton", "multi_faceoff", "coop.btn", 157.902f,
       0.0f, 62.545f},
  };
  for (const auto& e : multi_expects) {
    auto it = multi_by_name.find(e.name);
    CHECK(it != multi_by_name.end());
    if (it == multi_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == (lbl.type == "BandButton" ? "blockletters_fill"
                                                 : "helveticablack"));
    CHECK(lbl.parent == "dl_buttons.view");
    CHECK(lbl.text == e.text);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.has_showing);
    CHECK(lbl.showing);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], e.x));
      CHECK(near(lbl.world[10], e.y));
      CHECK(near(lbl.world[11], e.z));
    }
  }
  CHECK(multi_by_name.size() == 4);
  ghogx::ui::MenuFont blockletters_font;
  CHECK(blockletters_font.load(hdr, ark0, "ui/gen/blockletters_fill.milo_ps2"));
  CHECK(blockletters_font.valid());

  auto difficulty_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/sel_difficulty.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> difficulty_by_name;
  for (auto& l : difficulty_labels) {
    std::printf("difficulty label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    difficulty_by_name[l.name] = l;
  }
  struct DifficultyExpect {
    const char* name;
    const char* parent;
    const char* text;
    const char* nav;
    float x;
    float y;
    float z;
  };
  const DifficultyExpect difficulty_expects[] = {
      {"sd_diff1.btn", "sd_diff1.grp", "easy", "sd_diff2.btn", -29.722f,
       0.0f, 158.595f},
      {"sd_diff2.btn", "sd_diff2.grp", "medium", "sd_diff3.btn", -41.243f,
       0.0f, 128.796f},
      {"sd_diff3.btn", "sd_diff3.grp", "hard", "sd_diff4.btn", -35.820f,
       0.0f, 95.701f},
      {"sd_diff4.btn", "sd_diff4.grp", "expert", "sd_diff1.btn", -61.337f,
       0.0f, 66.147f},
  };
  for (const auto& e : difficulty_expects) {
    auto it = difficulty_by_name.find(e.name);
    CHECK(it != difficulty_by_name.end());
    if (it == difficulty_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandButton");
    CHECK(lbl.font == "cutout");
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.text == e.text);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.has_showing);
    CHECK(lbl.showing);
    CHECK(lbl.has_local);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], e.x));
      CHECK(near(lbl.world[10], e.y));
      CHECK(near(lbl.world[11], e.z));
    }
  }
  if (auto it = difficulty_by_name.find("sd_select.lbl");
      it != difficulty_by_name.end()) {
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandLabel");
    CHECK(lbl.font == "helveticablackcondensed");
    CHECK(lbl.text == "sd_help");
    CHECK(lbl.text_tail.valid);
    CHECK(lbl.has_local);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], -73.0f));
      CHECK(near(lbl.world[10], 0.0f));
      CHECK(near(lbl.world[11], -83.0f));
    }
  } else {
    CHECK(false);
  }
  ghogx::ui::MenuFont cutout_font;
  CHECK(cutout_font.load(hdr, ark0, "ui/gen/cutout.milo_ps2"));
  CHECK(cutout_font.valid());

  auto guitar_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/sel_guitar.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> guitar_by_name;
  for (auto& l : guitar_labels) guitar_by_name[l.name] = l;
  if (auto it = guitar_by_name.find("sg_guitar_nm.lbl");
      it != guitar_by_name.end()) {
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandLabel");
    CHECK(lbl.font == "helveticablackcondensed");
    CHECK(lbl.parent == "sg_text_guitar.grp");
    CHECK(lbl.text == "LOG");
    CHECK(lbl.text_tail.valid);
    if (lbl.text_tail.valid) {
      CHECK(near(lbl.text_tail.width, 300.0f));
      CHECK(near(lbl.text_tail.height, 70.0f));
      CHECK(near(lbl.text_tail.leading, 0.66f));
      CHECK(lbl.text_tail.alignment == 33);
      CHECK(lbl.text_tail.all_caps == 1);
      CHECK(near(lbl.text_tail.text_size, 40.0f));
      CHECK(near(lbl.text_tail.width_bound, 340.0f));
      CHECK(lbl.text_tail.color[0] > 0.85f);
      CHECK(lbl.text_tail.color[1] > 0.85f);
      CHECK(lbl.text_tail.color[2] > 0.85f);
      CHECK(lbl.text_tail.color[3] > 0.9f);
    }
    CHECK(lbl.has_local);
    CHECK(lbl.has_world);
    if (lbl.has_local) {
      CHECK(near(lbl.local[9], -280.0f));
      CHECK(near(lbl.local[10], 0.0f));
      CHECK(near(lbl.local[11], 138.0f));
    }
    if (lbl.has_world) {
      CHECK(lbl.world[9] < -600.0f);
      CHECK(lbl.world[10] < -900.0f);
      CHECK(near(lbl.world[11], 138.0f));
    }
  } else {
    CHECK(false);
  }
  ghogx::ui::MenuFont helvetica_font;
  CHECK(helvetica_font.load(hdr, ark0, "ui/gen/helveticablackcondensed.milo_ps2"));
  CHECK(helvetica_font.valid());

  auto chooseprof_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/chooseprof.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> chooseprof_by_name;
  for (auto& l : chooseprof_labels) {
    std::printf("chooseprof label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    chooseprof_by_name[l.name] = l;
  }
  if (auto title = chooseprof_by_name.find("cp_title.lbl");
      title != chooseprof_by_name.end()) {
    CHECK(title->second.type == "BandLabel");
    CHECK(title->second.font == "blockletters_fill");
    CHECK(title->second.parent == "cp_layout.view");
    CHECK(title->second.text == "choose_band");
    CHECK(title->second.has_world);
    if (title->second.has_world) {
      CHECK(near(title->second.world[9], 37.467f));
      CHECK(near(title->second.world[10], 0.0f));
      CHECK(near(title->second.world[11], 158.610f));
    }
  } else {
    CHECK(false);
  }
  const float chooseprof_z[] = {
      118.497f, 78.821f, 38.667f, -0.476f,
      -44.105f, -83.400f, -127.210f, -168.999f,
  };
  for (int i = 0; i < 8; ++i) {
    const std::string name = "cp_band" + std::to_string(i) + ".btn";
    auto it = chooseprof_by_name.find(name);
    CHECK(it != chooseprof_by_name.end());
    if (it == chooseprof_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandButton");
    CHECK(lbl.font == "rockletters");
    CHECK(lbl.parent == "cp_band" + std::to_string(i) + ".grp");
    CHECK(lbl.text == "BAND_" + std::to_string(i + 1));
    CHECK(lbl.nav == "cp_band" + std::to_string((i + 1) % 8) + ".btn");
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[10], 0.0f));
      CHECK(near(lbl.world[11], chooseprof_z[i]));
    }
  }
  CHECK(chooseprof_by_name.size() == 9);

  ghogx::milo_scene::Scene guitar_display_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0, "ui/gen/guitar_display.milo_ps2",
                                      guitar_display_scene));
  std::map<std::string, ghogx::milo_scene::GroupObj> guitar_display_groups;
  for (const auto& group : guitar_display_scene.groups) {
    guitar_display_groups[group.name] = group;
    if (group.name == "guitar_single.view" ||
        group.name == "guitar_store.view" ||
        group.name == "guitar_axe.view" ||
        group.name == "guitar_p1.view" ||
        group.name == "guitar_p2.view") {
      std::printf("guitar display group: %s parent='%s' local=(%.3f %.3f %.3f) "
                  "world=(%.3f %.3f %.3f) children=%zu source_order=%d\n",
                  group.name.c_str(), group.parent.c_str(),
                  group.local.pos[0], group.local.pos[1], group.local.pos[2],
                  group.world_stored.pos[0], group.world_stored.pos[1],
                  group.world_stored.pos[2], group.children.size(),
                  group.source_order_decoded ? 1 : 0);
    }
  }
  struct GuitarDisplayGroupExpect {
    const char* name;
    float x;
    float y;
    float z;
    std::size_t children;
  };
  const GuitarDisplayGroupExpect guitar_display_expects[] = {
      {"guitar_single.view", 16.0f, -162.0f, -58.0f, 0},
      {"guitar_store.view", -33.0f, 0.0f, -30.0f, 2},
      {"guitar_axe.view", -11.819f, 0.0f, 27.719f, 2},
      {"guitar_p1.view", -8.5f, -150.0f, -62.0f, 2},
      {"guitar_p2.view", 20.7f, -150.0f, -62.0f, 2},
  };
  for (const auto& e : guitar_display_expects) {
    auto it = guitar_display_groups.find(e.name);
    CHECK(it != guitar_display_groups.end());
    if (it != guitar_display_groups.end()) {
      CHECK(it->second.source_order_decoded);
      CHECK(it->second.has_transform);
      CHECK(it->second.parent == "guitar_display.view");
      CHECK(near(it->second.local.pos[0], e.x));
      CHECK(near(it->second.local.pos[1], e.y));
      CHECK(near(it->second.local.pos[2], e.z));
      CHECK(near(it->second.world_stored.pos[0], e.x));
      CHECK(near(it->second.world_stored.pos[1], e.y));
      CHECK(near(it->second.world_stored.pos[2], e.z));
      CHECK(it->second.children.size() == e.children);
    }
  }
  auto guitar_display_root = guitar_display_groups.find("guitar_display.view");
  CHECK(guitar_display_root != guitar_display_groups.end());
  if (guitar_display_root != guitar_display_groups.end()) {
    CHECK(guitar_display_root->second.environment_ref == "guitar_setup.env");
  }
  const auto* guitar_setup_env =
      guitar_display_scene.find_environ("guitar_setup.env");
  CHECK(guitar_setup_env != nullptr);
  if (guitar_setup_env) {
    CHECK(guitar_setup_env->decoded);
    CHECK(guitar_setup_env->lights.size() == 3);
    CHECK(guitar_display_scene.find_light("guitar01.lit") != nullptr);
    CHECK(guitar_display_scene.find_light("guitar02.lit") != nullptr);
    CHECK(guitar_display_scene.find_light("guitar03.lit") != nullptr);
  }
  std::map<std::string, ghogx::milo_scene::BandPlacerObj> guitar_display_placers;
  for (const auto& placer : guitar_display_scene.band_placers) {
    guitar_display_placers[placer.name] = placer;
    if (placer.name == "guitar_store.placer" ||
        placer.name == "guitar_axe.placer" ||
        placer.name == "guitar_multi0.placer" ||
        placer.name == "guitar_multi1.placer") {
      std::printf("guitar display placer: %s kind='%s' parent='%s' "
                  "local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f)\n",
                  placer.name.c_str(), placer.kind.c_str(), placer.parent.c_str(),
                  placer.local.pos[0], placer.local.pos[1], placer.local.pos[2],
                  placer.world_stored.pos[0], placer.world_stored.pos[1],
                  placer.world_stored.pos[2]);
    }
  }
  struct GuitarDisplayPlacerExpect {
    const char* name;
    const char* parent;
    float lx;
    float ly;
    float lz;
    float wx;
    float wy;
    float wz;
  };
  const GuitarDisplayPlacerExpect guitar_placer_expects[] = {
      {"guitar_store.placer", "guitar_store.view", 0.0f, 0.0f, 0.0f,
       -33.0f, 0.0f, -30.0f},
      {"guitar_axe.placer", "guitar_axe.view", 6.131f, -1.763f, -8.286f,
       -16.226f, 18.145f, 9.420f},
      {"guitar_multi0.placer", "guitar_p1.view", 0.0f, -0.0f, 55.0f,
       -8.5f, -150.0f, -7.0f},
      {"guitar_multi1.placer", "guitar_p2.view", 0.0f, -0.0f, 55.0f,
       20.7f, -150.0f, -7.0f},
  };
  for (const auto& e : guitar_placer_expects) {
    auto it = guitar_display_placers.find(e.name);
    CHECK(it != guitar_display_placers.end());
    if (it == guitar_display_placers.end()) continue;
    CHECK(it->second.decoded);
    CHECK(it->second.kind == "guitar");
    CHECK(it->second.parent == e.parent);
    CHECK(near(it->second.local.pos[0], e.lx));
    CHECK(near(it->second.local.pos[1], e.ly));
    CHECK(near(it->second.local.pos[2], e.lz));
    CHECK(near(it->second.world_stored.pos[0], e.wx));
    CHECK(near(it->second.world_stored.pos[1], e.wy));
    CHECK(near(it->second.world_stored.pos[2], e.wz));
  }
  struct GuitarDisplayFilterExpect {
    const char* filter;
    const char* trans_anim;
    const char* target;
    float frame;
    float first_frame;
    float last_frame;
    std::size_t rotation_keys;
  };
  const GuitarDisplayFilterExpect guitar_filter_expects[] = {
      {"guitar_store.filt", "guitar_store.tnm", "guitar_store.placer",
       0.0f, 0.0f, 240.0f, 5},
      {"guitar_axe.filt", "guitar_axe.tnm", "guitar_axe.placer", 0.0f,
       0.0f, 100.0f, 5},
      {"guitar_p1.filt", "guitar_p1.tnm", "guitar_multi0.placer", 0.0f,
       0.0f, 240.0f, 4},
      {"guitar_p2.filt", "guitar_p2.tnm", "guitar_multi1.placer", 0.0f,
       0.0f, 240.0f, 4},
  };
  for (const auto& e : guitar_filter_expects) {
    auto filter = ghogx::ui::extract_menu_anim_filter(
        hdr, ark0, "ui/gen/guitar_display.milo_ps2", e.filter);
    std::printf("guitar display filter: %s valid=%d trans=%s frame=%.3f\n",
                e.filter, filter.valid ? 1 : 0,
                filter.trans_anim.c_str(), filter.frame);
    CHECK(filter.valid);
    if (filter.valid) {
      CHECK(filter.trans_anim == e.trans_anim);
      CHECK(near(filter.frame, e.frame));
      auto anim = ghogx::ui::extract_menu_slider_anim(
          hdr, ark0, "ui/gen/guitar_display.milo_ps2", filter.trans_anim);
      std::printf("guitar display anim: %s target=%s rot=%zu trans=%zu "
                  "first=(%.3f %.3f %.3f @ %.3f) last=(%.3f %.3f %.3f @ %.3f)\n",
                  anim.name.c_str(), anim.target.c_str(),
                  anim.rotation_keys.size(), anim.translation_keys.size(),
                  anim.first[0], anim.first[1], anim.first[2],
                  anim.first_frame, anim.last[0], anim.last[1], anim.last[2],
                  anim.last_frame);
      CHECK(anim.valid);
      CHECK(anim.target == e.target);
      CHECK(anim.rotation_keys.size() == e.rotation_keys);
      CHECK(anim.translation_keys.empty());
      CHECK(near(anim.first_frame, e.first_frame));
      CHECK(near(anim.last_frame, e.last_frame));
      CHECK(!anim.rotation_keys.empty());
      if (!anim.rotation_keys.empty()) {
        CHECK(near(anim.rotation_keys.front().frame, e.first_frame));
        CHECK(near(anim.rotation_keys.back().frame, e.last_frame));
      }
    }
  }
  struct MultiGuitarProxyExpect {
    const char* name;
    const char* parent;
    float lx;
    float ly;
    float lz;
    float wx;
    float wy;
    float wz;
  };
  const MultiGuitarProxyExpect multi_proxy_expects[] = {
      {"guitar_multi0.pxy", "mgs_guitar_p1.grp", -20.0f, -650.0f, -14.0f,
       -20.855f, -670.0f, -11.626f},
      {"guitar_multi1.pxy", "mgs_guitar_p2.grp", 20.0f, -650.0f, -14.0f,
       25.854f, -670.0f, -11.628f},
  };
  for (const auto& e : multi_proxy_expects) {
    auto proxy = ghogx::ui::extract_menu_proxy_transform(
        hdr, ark0, "ui/gen/multi_sel_guitar.milo_ps2", e.name);
    std::printf("multi guitar proxy: %s valid=%d parent=%s local=(%.3f %.3f %.3f) "
                "world=(%.3f %.3f %.3f) constraint=%u\n",
                e.name, proxy.valid ? 1 : 0, proxy.parent.c_str(),
                proxy.local[9], proxy.local[10], proxy.local[11],
                proxy.world[9], proxy.world[10], proxy.world[11],
                proxy.constraint);
    CHECK(proxy.valid);
    if (proxy.valid) {
      CHECK(proxy.name == e.name);
      CHECK(proxy.parent == e.parent);
      CHECK(near(proxy.local[9], e.lx));
      CHECK(near(proxy.local[10], e.ly));
      CHECK(near(proxy.local[11], e.lz));
      CHECK(near(proxy.world[9], e.wx));
      CHECK(near(proxy.world[10], e.wy));
      CHECK(near(proxy.world[11], e.wz));
      CHECK(proxy.constraint == 0);
    }
  }
  struct MultiGuitarFilterExpect {
    const char* name;
    const char* trans_anim;
    const char* target;
  };
  const MultiGuitarFilterExpect multi_filter_expects[] = {
      {"guitar_multi0.filt", "guitar_multi0.tnm", "guitar_multi0.pxy"},
      {"guitar_multi1.filt", "guitar_multi1.tnm", "guitar_multi1.pxy"},
  };
  for (const auto& e : multi_filter_expects) {
    auto filter = ghogx::ui::extract_menu_anim_filter(
        hdr, ark0, "ui/gen/multi_sel_guitar.milo_ps2", e.name);
    std::printf("multi guitar filter: %s valid=%d trans=%s frame=%.3f "
                "scale=%.3f start=%.3f end=%.3f type=%d\n",
                e.name, filter.valid ? 1 : 0, filter.trans_anim.c_str(),
                filter.frame, filter.scale, filter.start, filter.end,
                filter.type);
    CHECK(filter.valid);
    if (filter.valid) {
      CHECK(filter.trans_anim == e.trans_anim);
      CHECK(near(filter.frame, 500.0f));
      CHECK(near(filter.scale, 2.0f));
      CHECK(near(filter.start, 0.0f));
      CHECK(near(filter.end, 240.0f));
      CHECK(filter.type == 1);
      auto anim = ghogx::ui::extract_menu_slider_anim(
          hdr, ark0, "ui/gen/multi_sel_guitar.milo_ps2", filter.trans_anim);
      std::printf("multi guitar anim: %s target=%s rot=%zu trans=%zu "
                  "scale=%zu first_frame=%.3f last_frame=%.3f\n",
                  anim.name.c_str(), anim.target.c_str(),
                  anim.rotation_keys.size(), anim.translation_keys.size(),
                  anim.scale_keys.size(), anim.first_frame, anim.last_frame);
      CHECK(anim.valid);
      if (anim.valid) {
        CHECK(anim.target == e.target);
        CHECK(anim.rotation_keys.size() == 4);
        CHECK(anim.translation_keys.empty());
        CHECK(anim.scale_keys.size() == 1);
        CHECK(near(anim.first_frame, 0.0f));
        CHECK(near(anim.last_frame, 240.0f));
      }
    }
  }
  auto single_proxy = ghogx::ui::extract_menu_proxy_transform(
      hdr, ark0, "ui/gen/sel_guitar.milo_ps2", "guitar.pxy");
  std::printf("sel guitar proxy: valid=%d parent=%s local=(%.3f %.3f %.3f) "
              "world=(%.3f %.3f %.3f)\n",
              single_proxy.valid ? 1 : 0, single_proxy.parent.c_str(),
              single_proxy.local[9], single_proxy.local[10],
              single_proxy.local[11], single_proxy.world[9],
              single_proxy.world[10], single_proxy.world[11]);
  CHECK(single_proxy.valid);
  CHECK(single_proxy.name == "guitar.pxy");
  CHECK(single_proxy.parent == "guitar.grp");
  CHECK(near(single_proxy.local[9], 0.0f));
  CHECK(near(single_proxy.local[10], 3.0f));
  CHECK(near(single_proxy.local[11], 0.0f));
  CHECK(near(single_proxy.world[0], 0.006645f));
  CHECK(near(single_proxy.world[1], 0.871606f));
  CHECK(near(single_proxy.world[2], -0.075954f));
  CHECK(near(single_proxy.world[3], -0.871605f));
  CHECK(near(single_proxy.world[4], 0.0f));
  CHECK(near(single_proxy.world[5], -0.076256f));
  CHECK(near(single_proxy.world[6], -0.075960f));
  CHECK(near(single_proxy.world[7], 0.076250f));
  CHECK(near(single_proxy.world[8], 0.868225f));
  CHECK(near(single_proxy.world[9], -15.865f));
  CHECK(near(single_proxy.world[10], -685.0f));
  CHECK(near(single_proxy.world[11], -7.729f));
  auto single_filter = ghogx::ui::extract_menu_anim_filter(
      hdr, ark0, "ui/gen/sel_guitar.milo_ps2", "guitar_single.filt");
  std::printf("sel guitar filter: valid=%d trans=%s frame=%.3f\n",
              single_filter.valid ? 1 : 0, single_filter.trans_anim.c_str(),
              single_filter.frame);
  CHECK(single_filter.valid);
  if (single_filter.valid) {
    CHECK(single_filter.trans_anim == "guitar_single.tnm");
    CHECK(near(single_filter.frame, 120.0f));
    CHECK(near(single_filter.scale, 2.0f));
    CHECK(near(single_filter.offset, 0.0f));
    CHECK(near(single_filter.start, 0.0f));
    CHECK(near(single_filter.end, 240.0f));
    CHECK(single_filter.type == 1);
    auto single_anim = ghogx::ui::extract_menu_slider_anim(
        hdr, ark0, "ui/gen/sel_guitar.milo_ps2", single_filter.trans_anim);
    std::printf("sel guitar anim: %s target=%s rot=%zu trans=%zu "
                "first_frame=%.3f last_frame=%.3f\n",
                single_anim.name.c_str(), single_anim.target.c_str(),
                single_anim.rotation_keys.size(),
                single_anim.translation_keys.size(),
                single_anim.first_frame, single_anim.last_frame);
    CHECK(single_anim.valid);
    CHECK(single_anim.target == "guitar.pxy");
    CHECK(single_anim.rotation_keys.size() == 4);
    CHECK(single_anim.translation_keys.empty());
    CHECK(near(single_anim.first_frame, 0.0f));
    CHECK(near(single_anim.last_frame, 240.0f));
    CHECK(!single_anim.rotation_keys.empty());
    if (!single_anim.rotation_keys.empty()) {
      CHECK(near(single_anim.rotation_keys.front().frame, 0.0f));
      CHECK(near(single_anim.rotation_keys.back().frame, 240.0f));
    }
  }
  ghogx::milo_scene::Scene sel_guitar_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0,
                                      "ui/gen/sel_guitar.milo_ps2",
                                      sel_guitar_scene));
  const auto* guitar_env = sel_guitar_scene.find_environ("guitar.env");
  CHECK(guitar_env != nullptr);
  if (guitar_env) {
    CHECK(guitar_env->lights.size() == 2);
    CHECK(sel_guitar_scene.find_light("light01.lit") != nullptr);
    CHECK(sel_guitar_scene.find_light("light02.lit") != nullptr);
  }
  auto store_proxy = ghogx::ui::extract_menu_proxy_transform(
      hdr, ark0, "ui/gen/store.milo_ps2", "guitar.pxy");
  std::printf("store guitar proxy: valid=%d parent=%s local=(%.3f %.3f %.3f) "
              "world=(%.3f %.3f %.3f)\n",
              store_proxy.valid ? 1 : 0, store_proxy.parent.c_str(),
              store_proxy.local[9], store_proxy.local[10],
              store_proxy.local[11], store_proxy.world[9],
              store_proxy.world[10], store_proxy.world[11]);
  CHECK(store_proxy.valid);
  CHECK(store_proxy.name == "guitar.pxy");
  CHECK(store_proxy.parent == "guitar.grp");
  CHECK(near(store_proxy.local[9], 6.0f));
  CHECK(near(store_proxy.local[10], -85.0f));
  CHECK(near(store_proxy.local[11], -16.0f));
  CHECK(near(store_proxy.world[9], -23.650f));
  CHECK(near(store_proxy.world[10], -648.406f));
  CHECK(near(store_proxy.world[11], -18.863f));
  auto store_filter = ghogx::ui::extract_menu_anim_filter(
      hdr, ark0, "ui/gen/store.milo_ps2", "guitar_single.filt");
  std::printf("store guitar filter: valid=%d trans=%s frame=%.3f\n",
              store_filter.valid ? 1 : 0, store_filter.trans_anim.c_str(),
              store_filter.frame);
  CHECK(store_filter.valid);
  if (store_filter.valid) {
    CHECK(store_filter.trans_anim == "guitar_single.tnm");
    CHECK(near(store_filter.frame, 0.0f));
    CHECK(near(store_filter.scale, 2.0f));
    CHECK(near(store_filter.offset, 0.0f));
    CHECK(near(store_filter.start, 0.0f));
    CHECK(near(store_filter.end, 240.0f));
    CHECK(store_filter.type == 1);
    auto store_anim = ghogx::ui::extract_menu_slider_anim(
        hdr, ark0, "ui/gen/store.milo_ps2", store_filter.trans_anim);
    std::printf("store guitar anim: %s target=%s rot=%zu trans=%zu "
                "first_frame=%.3f last_frame=%.3f\n",
                store_anim.name.c_str(), store_anim.target.c_str(),
                store_anim.rotation_keys.size(), store_anim.translation_keys.size(),
                store_anim.first_frame, store_anim.last_frame);
    CHECK(store_anim.valid);
    CHECK(store_anim.target == "guitar.pxy");
    CHECK(store_anim.rotation_keys.size() == 4);
    CHECK(store_anim.translation_keys.empty());
    CHECK(near(store_anim.first_frame, 0.0f));
    CHECK(near(store_anim.last_frame, 240.0f));
    CHECK(!store_anim.rotation_keys.empty());
    if (!store_anim.rotation_keys.empty()) {
      CHECK(near(store_anim.rotation_keys.front().frame, 0.0f));
      CHECK(near(store_anim.rotation_keys.back().frame, 240.0f));
    }
  }
  ghogx::milo_scene::Scene store_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0, "ui/gen/store.milo_ps2",
                                      store_scene));
  CHECK(store_scene.find_environ("guitar.env") != nullptr);
  CHECK(store_scene.find_environ("guitar_black.env") != nullptr);
  ghogx::milo_scene::Scene lespaul_display_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0,
                                      "char/og/guitars/gen/lespaull.milo_ps2",
                                      lespaul_display_scene));
  if (const auto* mat =
          lespaul_display_scene.find_mat("guitar_lespaul_burst.mat")) {
    CHECK(mat->use_environ);
    CHECK(!mat->prelit);
  } else {
    CHECK(false);
  }

  auto video_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/video_settings.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> video_label_by_name;
  for (auto& l : video_labels) {
    std::printf("video label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0, l.world[9],
                l.world[10], l.world[11]);
    if (l.button_tail.valid) {
      std::printf(" tail=(fit=%d w=%.3f h=%.3f lead=%.3f align=%d "
                  "size=%.3f bound=%.3f kern=%.3f)",
                  l.button_tail.fit_text, l.button_tail.width,
                  l.button_tail.height, l.button_tail.leading,
                  l.button_tail.alignment, l.button_tail.text_size,
                  l.button_tail.width_bound, l.button_tail.kerning);
    }
    if (l.text_tail.valid) {
      std::printf(" text_tail=(fit=%d w=%.3f h=%.3f lead=%.3f align=%d "
                  "size=%.3f bound=%.3f caps=%d)",
                  l.text_tail.fit_text, l.text_tail.width,
                  l.text_tail.height, l.text_tail.leading,
                  l.text_tail.alignment, l.text_tail.text_size,
                  l.text_tail.width_bound,
                  static_cast<int>(l.text_tail.all_caps));
    }
    std::printf("\n");
    video_label_by_name[l.name] = l;
  }
  struct VideoLabelExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float z;
    float width;
    float height;
    float leading;
    int alignment;
    float text_size;
    float width_bound;
  };
  const VideoLabelExpect video_label_expects[] = {
      {"vs_title.lbl", "BandLabel", "clarendon", "vs_title",
       "vs_poster.view", "", -11.996f, 166.0f, 240.0f, 50.0f, 0.7f,
       65, 30.0f, 1000.0f},
      {"gs_left_p1.btn", "BandButton", "helveticablack", "LEFT_PLAYER_1",
       "vs_buttons.view", "gs_left_p2.btn", -69.979f, 112.0f, 180.0f,
       30.0f, 1.0f, 33, 30.0f, 400.0f},
      {"gs_left_p2.btn", "BandButton", "helveticablack", "LEFT_PLAYER_2",
       "vs_buttons.view", "widescreen.btn", -69.979f, 82.0f, 180.0f,
       40.0f, 1.0f, 33, 30.0f, 400.0f},
      {"widescreen.btn", "BandButton", "helveticablack", "vs_widescreen",
       "vs_buttons.view", "p_scan.btn", -69.979f, 52.0f, 180.0f,
       40.0f, 1.0f, 33, 30.0f, 400.0f},
      {"p_scan.btn", "BandButton", "helveticablack", "vs_pscan",
       "vs_buttons.view", "calibrate_lag.btn", -69.979f, 22.0f,
       180.0f, 40.0f, 1.0f, 33, 30.0f, 320.0f},
      {"calibrate_lag.btn", "BandButton", "helveticablack", "vs_lag",
       "vs_buttons.view", "gs_left_p1.btn", -6.998f, -20.0f, 140.0f,
       45.0f, 0.8f, 34, 20.0f, 150.0f},
  };
  CHECK(video_label_by_name.size() == std::size(video_label_expects));
  for (const auto& e : video_label_expects) {
    auto it = video_label_by_name.find(e.name);
    CHECK(it != video_label_by_name.end());
    if (it == video_label_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == e.font);
    CHECK(lbl.text == e.text);
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.has_showing);
    CHECK(lbl.showing);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], e.x));
      CHECK(near(lbl.world[10], 0.0f));
      CHECK(near(lbl.world[11], e.z));
    }
    if (lbl.type == "BandButton") {
      CHECK(lbl.button_tail.valid);
      if (lbl.button_tail.valid) {
        CHECK(lbl.button_tail.fit_text == 2);
        CHECK(near(lbl.button_tail.width, e.width));
        CHECK(near(lbl.button_tail.height, e.height));
        CHECK(near(lbl.button_tail.leading, e.leading));
        CHECK(lbl.button_tail.alignment == e.alignment);
        CHECK(near(lbl.button_tail.kerning, 0.0f));
        CHECK(near(lbl.button_tail.text_size, e.text_size));
        CHECK(near(lbl.button_tail.width_bound, e.width_bound));
      }
    } else {
      CHECK(lbl.text_tail.valid);
      if (lbl.text_tail.valid) {
        CHECK(lbl.text_tail.fit_text == 2);
        CHECK(near(lbl.text_tail.width, e.width));
        CHECK(near(lbl.text_tail.height, e.height));
        CHECK(near(lbl.text_tail.leading, e.leading));
        CHECK(lbl.text_tail.alignment == e.alignment);
        CHECK(near(lbl.text_tail.text_size, e.text_size));
        CHECK(near(lbl.text_tail.width_bound, e.width_bound));
      }
    }
  }
  ghogx::ui::MenuFont helvetica_black_font;
  CHECK(helvetica_black_font.load(hdr, ark0, "ui/gen/helveticablack.milo_ps2"));
  CHECK(helvetica_black_font.valid());

  ghogx::milo_scene::Scene helpbar_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0, "ui/gen/helpbar.milo_ps2",
                                      helpbar_scene));
  std::printf("helpbar scene: meshes=%zu mats=%zu groups=%zu\n",
              helpbar_scene.meshes.size(), helpbar_scene.mats.size(),
              helpbar_scene.groups.size());
  CHECK(helpbar_scene.meshes.size() == 7);
  CHECK(helpbar_scene.mats.size() == 7);
  CHECK(helpbar_scene.groups.size() == 1);
  const auto helpbar_icons = ghogx::asset::load_milo_textures(
      hdr, ark0, "ui/gen/helpbar.milo_ps2",
      {"hb_fret1.tex", "hb_fret2.tex", "hb_fret3.tex", "hb_strum.tex",
       "hb_start.tex", "help_box_mid.tex", "help_box_corner.tex"});
  for (const char* icon : {"hb_fret1.tex", "hb_fret2.tex", "hb_fret3.tex",
                           "hb_strum.tex", "hb_start.tex",
                           "help_box_mid.tex", "help_box_corner.tex"}) {
    auto it = helpbar_icons.find(icon);
    CHECK(it != helpbar_icons.end());
    if (it != helpbar_icons.end()) {
      CHECK(it->second.valid());
      if (it->second.valid()) {
        const auto& image = it->second;
        int min_row = image.height;
        int max_row = -1;
        double alpha_sum = 0.0;
        double alpha_row_sum = 0.0;
        for (int y = 0; y < image.height; ++y) {
          for (int x = 0; x < image.width; ++x) {
            const std::uint8_t alpha = image.rgba[
                (static_cast<std::size_t>(y) * image.width + x) * 4 + 3];
            if (alpha != 0) {
              min_row = std::min(min_row, y);
              max_row = std::max(max_row, y);
            }
            alpha_sum += alpha;
            alpha_row_sum += static_cast<double>(alpha) * y;
          }
        }
        const double alpha_center =
            alpha_sum > 0.0 ? alpha_row_sum / alpha_sum : 0.0;
        std::printf("helpbar texture: %s %dx%d alpha_rows=%d..%d "
                    "alpha_center=%.3f\n",
                    icon, image.width, image.height, min_row, max_row,
                    alpha_center);
      }
    }
  }
  std::map<std::string, const ghogx::milo_scene::MeshObj*> helpbar_mesh_by_name;
  for (const auto& mesh : helpbar_scene.meshes) {
    const auto world = helpbar_scene.world_matrix(mesh);
    const auto* mat = helpbar_scene.find_mat(mesh.material);
    std::printf("helpbar mesh: %s mat=%s tex=%s parent=%s showing=%d "
                "world=(%.3f %.3f %.3f) local=(%.3f %.3f %.3f) "
                "bb=[%.3f %.3f %.3f]-[%.3f %.3f %.3f]\n",
                mesh.name.c_str(), mesh.material.c_str(),
                mat ? mat->diffuse_tex.c_str() : "",
                mesh.parent.c_str(), mesh.showing ? 1 : 0, world[12],
                world[13], world[14], mesh.local.pos[0], mesh.local.pos[1],
                mesh.local.pos[2], mesh.bb_min[0], mesh.bb_min[1],
                mesh.bb_min[2], mesh.bb_max[0], mesh.bb_max[1],
                mesh.bb_max[2]);
    helpbar_mesh_by_name[mesh.name] = &mesh;
  }
  struct HelpbarMeshExpect {
    const char* name;
    const char* material;
    const char* texture;
    float x;
    float z;
  };
  const HelpbarMeshExpect helpbar_mesh_expects[] = {
      {"help_bar_starting.mesh", "", "", -294.915f, -205.0f},
      {"help_bar_strumbar.mesh", "", "", 90.699f, -205.0f},
      {"help_box_mid.mesh", "help_box_mid.mat", "help_box_mid.tex",
       -2.971f, -30.0f},
      {"help_box_left.mesh", "help_box_corner.mat", "help_box_corner.tex",
       -7.231f, -30.0f},
      {"help_box_right.mesh", "help_box_corner.mat", "help_box_corner.tex",
       18.387f, -30.0f},
  };
  for (const auto& e : helpbar_mesh_expects) {
    auto it = helpbar_mesh_by_name.find(e.name);
    CHECK(it != helpbar_mesh_by_name.end());
    if (it == helpbar_mesh_by_name.end()) continue;
    const auto& mesh = *it->second;
    const auto world = helpbar_scene.world_matrix(mesh);
    CHECK(mesh.parent == "helpbar.view");
    CHECK(mesh.material == e.material);
    if (const auto* mat = helpbar_scene.find_mat(mesh.material))
      CHECK(mat->diffuse_tex == e.texture);
    CHECK(near(world[12], e.x));
    CHECK(near(world[13], 0.0f));
    CHECK(near(world[14], e.z));
    if (mesh.name == "help_box_left.mesh") {
      CHECK(near(mesh.world_stored.rot[0][0], -0.5f));
      CHECK(near(mesh.world_stored.rot[2][2], -2.5f));
    }
    if (mesh.name == "help_box_right.mesh") {
      CHECK(near(mesh.world_stored.rot[0][0], 0.5f));
      CHECK(near(mesh.world_stored.rot[2][2], 2.5f));
    }
    if (mesh.name == "help_box_mid.mesh") {
      CHECK(near(mesh.world_stored.rot[0][0], 1.0f));
      CHECK(near(mesh.world_stored.rot[2][2], 1.0f));
    }
  }
  for (const auto& group : helpbar_scene.groups) {
    std::printf("helpbar group: %s parent=%s showing=%d world=(%.3f %.3f %.3f) "
                "children=%zu\n",
                group.name.c_str(), group.parent.c_str(),
                group.showing ? 1 : 0, group.world_stored.pos[0],
                group.world_stored.pos[1], group.world_stored.pos[2],
                group.children.size());
    CHECK(group.name == "helpbar.view");
    CHECK(group.parent == "meta.cam");
    CHECK(group.showing);
  }
  auto helpbar_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/helpbar.milo_ps2");
  CHECK(helpbar_labels.size() == 1);
  for (const auto& l : helpbar_labels) {
    std::printf("helpbar label: %s type=%s font=%s text='%s' parent='%s' "
                "showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.has_showing ? 1 : 0,
                l.showing ? 1 : 0, l.world[9], l.world[10], l.world[11]);
    CHECK(l.name == "help_bar.txt");
    CHECK(l.font == "helveticablackcondensed.font");
    CHECK(l.parent == "helpbar.view");
    CHECK(l.text == "help text");
    CHECK(l.text_tail.valid);
    CHECK(l.text_tail.alignment == 33);
    CHECK(near(l.text_tail.text_size, 18.0f));
    CHECK(near(l.text_tail.width_bound, 375.840f));
  }
  const auto helpbar_text_style = ghogx::ui::extract_menu_text_style(
      hdr, ark0, "ui/gen/helpbar.milo_ps2", "help_bar.txt");
  if (helpbar_text_style.valid) {
    std::printf("helpbar text style: %s font=%s text='%s' parent=%s "
                "align=%d wrap=%.3f leading=%.3f size=%.3f "
                "world=(%.3f %.3f %.3f) color=(%.3f %.3f %.3f %.3f)\n",
                helpbar_text_style.name.c_str(),
                helpbar_text_style.font.c_str(),
                helpbar_text_style.text.c_str(),
                helpbar_text_style.parent.c_str(),
                helpbar_text_style.alignment, helpbar_text_style.wrap_width,
                helpbar_text_style.leading, helpbar_text_style.text_size,
                helpbar_text_style.world[9], helpbar_text_style.world[10],
                helpbar_text_style.world[11],
                helpbar_text_style.color[0], helpbar_text_style.color[1],
                helpbar_text_style.color[2], helpbar_text_style.color[3]);
  }
  CHECK(helpbar_text_style.valid);
  if (helpbar_text_style.valid) {
    CHECK(helpbar_text_style.font == "helveticablackcondensed.font");
    CHECK(helpbar_text_style.parent == "helpbar.view");
    CHECK(helpbar_text_style.alignment == 33);
    CHECK(near(helpbar_text_style.wrap_width, 375.840f));
    CHECK(near(helpbar_text_style.leading, 0.750f));
    CHECK(near(helpbar_text_style.text_size, 18.0f));
    CHECK(helpbar_text_style.has_world);
    CHECK(near(helpbar_text_style.color[0], 0.9f));
    CHECK(near(helpbar_text_style.color[1], 0.9f));
    CHECK(near(helpbar_text_style.color[2], 0.9f));
    CHECK(near(helpbar_text_style.color[3], 1.0f));
  }

  const auto character_select_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/sel_character.milo_ps2");
  auto character_select_title = std::find_if(
      character_select_labels.begin(), character_select_labels.end(),
      [](const ghogx::ui::MenuLabel& label) {
        return label.name == "sc_label1.txt";
      });
  CHECK(character_select_title != character_select_labels.end());
  if (character_select_title != character_select_labels.end()) {
    CHECK(character_select_title->type == "Text");
    CHECK(character_select_title->parent == "cs_set.grp");
    CHECK(character_select_title->font.empty());
    CHECK(character_select_title->text == "select_character");
    CHECK(character_select_title->text_tail.valid);
  }

  auto audio_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/game_settings.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> audio_label_by_name;
  for (auto& l : audio_labels) audio_label_by_name[l.name] = l;
  if (auto it = audio_label_by_name.find("gs_title.lbl");
      it != audio_label_by_name.end()) {
    CHECK(it->second.type == "BandLabel");
    CHECK(it->second.font == "clarendon");
    CHECK(it->second.parent == "gs_poster.view");
    CHECK(it->second.text == "GAME_SETTINGS");
    CHECK(it->second.text_tail.valid);
    if (it->second.text_tail.valid) {
      CHECK(it->second.text_tail.text_size > 0.0f);
      CHECK(it->second.text_tail.all_caps <= 1);
    }
  } else {
    CHECK(false);
  }
  ghogx::ui::MenuFont clarendon_font;
  CHECK(clarendon_font.load(hdr, ark0, "ui/gen/clarendon.milo_ps2"));
  CHECK(clarendon_font.valid());

  auto options_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/options.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> options_by_name;
  for (auto& l : options_labels) {
    std::printf("options label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f) "
                "button_tail=(%d %.3f %.3f %.3f %d %.3f) "
                "text_tail=(%d %.3f %.3f %.3f %d %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11],
                l.button_tail.valid ? 1 : 0, l.button_tail.width,
                l.button_tail.height, l.button_tail.text_size,
                l.button_tail.alignment, l.button_tail.width_bound,
                l.text_tail.valid ? 1 : 0, l.text_tail.width,
                l.text_tail.height, l.text_tail.text_size,
                l.text_tail.alignment, l.text_tail.width_bound);
    options_by_name[l.name] = l;
  }
  struct OptionsExpect {
    const char* name;
    const char* type;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float z;
    float width;
    float height;
    float text_size;
    int alignment;
    float width_bound;
  };
  const OptionsExpect options_expects[] = {
      {"op_title.lbl", "BandLabel", "OPTIONS", "options.view", "",
       -22.000f, 37.000f, 210.000f, 55.000f, 54.000f, 68, 400.000f},
      {"op_audio.btn", "BandButton", "op_game_settings", "op_game.view",
       "video_settings.btn", 25.694f, 19.535f, 280.000f, 40.000f, 35.000f,
       36, 1000.000f},
      {"video_settings.btn", "BandButton", "op_video_settings",
       "op_video_settings.grp", "op_data.btn", -26.875f, 7.855f, 220.000f,
       40.000f, 35.000f, 36, 1000.000f},
      {"op_data.btn", "BandButton", "op_data_settings", "op_data.view",
       "memory_card.btn", 2.845f, -22.784f, 250.000f, 40.000f, 35.000f,
       36, 1000.000f},
      {"memory_card.btn", "BandButton", "op_memcard", "op_memory_card.grp",
       "op_bonus.btn", -28.666f, -50.009f, 210.000f, 40.000f, 35.000f,
       36, 1000.000f},
      {"op_bonus.btn", "BandButton", "op_bonus_material", "op_bonus.view",
       "op_credit.btn", -5.654f, -73.524f, 240.000f, 40.000f, 35.000f,
       36, 1000.000f},
      {"op_credit.btn", "BandButton", "op_credits", "op_credit.view",
       "op_audio.btn", -25.874f, -107.733f, 120.000f, 40.000f, 35.000f,
       36, 1000.000f},
  };
  for (const auto& e : options_expects) {
    auto it = options_by_name.find(e.name);
    CHECK(it != options_by_name.end());
    if (it == options_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == "helveticablack");
    CHECK(it->second.text == e.text);
    CHECK(it->second.parent == e.parent);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_showing);
    CHECK(it->second.showing);
    CHECK(it->second.has_local);
    CHECK(it->second.has_world);
    CHECK(near(it->second.world[9], e.x));
    CHECK(near(it->second.world[10], 0.0f));
    CHECK(near(it->second.world[11], e.z));
    if (it->second.type == "BandButton") {
      CHECK(it->second.button_tail.valid);
      if (it->second.button_tail.valid) {
        CHECK(near(it->second.button_tail.width, e.width));
        CHECK(near(it->second.button_tail.height, e.height));
        CHECK(near(it->second.button_tail.text_size, e.text_size));
        CHECK(it->second.button_tail.alignment == e.alignment);
        CHECK(near(it->second.button_tail.width_bound, e.width_bound));
      }
      CHECK(!it->second.text_tail.valid);
    } else if (it->second.type == "BandLabel") {
      CHECK(it->second.text_tail.valid);
      if (it->second.text_tail.valid) {
        CHECK(near(it->second.text_tail.width, e.width));
        CHECK(near(it->second.text_tail.height, e.height));
        CHECK(near(it->second.text_tail.text_size, e.text_size));
        CHECK(it->second.text_tail.alignment == e.alignment);
        CHECK(near(it->second.text_tail.width_bound, e.width_bound));
      }
      CHECK(!it->second.button_tail.valid);
    }
  }
  CHECK(options_by_name.size() == 7);

  auto mem_card_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/mem_card.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> mem_card_by_name;
  for (auto& l : mem_card_labels) {
    std::printf("mem_card label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0, l.world[9],
                l.world[10], l.world[11]);
    mem_card_by_name[l.name] = l;
  }
  CHECK(mem_card_by_name.count("save_bands.btn") == 1);
  CHECK(mem_card_by_name.count("load_bands.btn") == 1);
  CHECK(mem_card_by_name.count("autosave.btn") == 1);
  CHECK(mem_card_by_name.count("title.lbl") == 1);
  CHECK(mem_card_by_name.count("title2.lbl") == 1);
  struct MemCardLabelExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float y;
    float z;
  };
  const MemCardLabelExpect mem_card_expects[] = {
      {"save_bands.btn", "BandButton", "helveticablack", "save_bands",
       "cp_buttons.view", "load_bands.btn", 43.848f, 0.0f, 74.046f},
      {"load_bands.btn", "BandButton", "helveticablack", "load_bands",
       "cp_buttons.view", "autosave.btn", 121.875f, 0.0f, 73.041f},
      {"autosave.btn", "BandButton", "helveticablack", "autosave",
       "cp_buttons.view", "save_bands.btn", 211.837f, 0.0f, 70.314f},
      {"title.lbl", "BandLabel", "impact", "op_memcard",
       "cp_poster.view", "", 10.088f, 0.0f, 153.590f},
      {"title2.lbl", "BandLabel", "impact", "op_memcard",
       "cp_poster.view", "", 5.450f, 0.0f, 157.396f},
  };
  for (const auto& e : mem_card_expects) {
    auto it = mem_card_by_name.find(e.name);
    CHECK(it != mem_card_by_name.end());
    if (it == mem_card_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == e.font);
    CHECK(it->second.text == e.text);
    CHECK(it->second.parent == e.parent);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_showing);
    CHECK(it->second.showing);
    CHECK(it->second.has_local);
    CHECK(it->second.has_world);
    CHECK(near(it->second.world[9], e.x));
    CHECK(near(it->second.world[10], e.y));
    CHECK(near(it->second.world[11], e.z));
    if (it->second.type == "BandLabel") CHECK(it->second.text_tail.valid);
  }

  auto mem_card_checkboxes = ghogx::ui::extract_menu_checkboxes(
      hdr, ark0, "ui/gen/mem_card.milo_ps2");
  std::map<std::string, ghogx::ui::MenuCheckbox> mem_card_checkbox_by_name;
  for (auto& cb : mem_card_checkboxes) {
    std::printf("mem_card checkbox: %s type=%s resource='%s' parent='%s' "
                "checked=%d showing=%d world=(%.3f %.3f %.3f)\n",
                cb.name.c_str(), cb.type.c_str(), cb.resource.c_str(),
                cb.parent.c_str(), cb.checked ? 1 : 0,
                cb.showing ? 1 : 0, cb.world[9], cb.world[10], cb.world[11]);
    mem_card_checkbox_by_name[cb.name] = cb;
  }
  CHECK(mem_card_checkbox_by_name.count("autosave.chk") == 1);
  if (auto it = mem_card_checkbox_by_name.find("autosave.chk");
      it != mem_card_checkbox_by_name.end()) {
    CHECK(it->second.type == "CheckBox");
    CHECK(it->second.resource == "default");
    CHECK(it->second.parent == "autosave.btn");
    CHECK(it->second.checked);
    CHECK(it->second.showing);
    CHECK(it->second.has_local);
    CHECK(it->second.has_world);
    CHECK(near(it->second.world[9], 212.360f));
    CHECK(near(it->second.world[10], 0.0f));
    CHECK(near(it->second.world[11], 82.302f));
  }

  auto store_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/store.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> store_by_name;
  for (auto& l : store_labels) store_by_name[l.name] = l;
  auto unlock_shop = store_by_name.find("st_unlock_shop.lbl");
  CHECK(unlock_shop != store_by_name.end());
  if (unlock_shop != store_by_name.end()) {
    CHECK(unlock_shop->second.type == "BandLabel");
    CHECK(unlock_shop->second.parent == "st_screen2.view");
    CHECK(unlock_shop->second.text == "UNLOCK SHOP");
    CHECK(unlock_shop->second.has_showing);
    CHECK(!unlock_shop->second.showing);
    CHECK(unlock_shop->second.text_tail.valid);
    if (unlock_shop->second.text_tail.valid) {
      CHECK(unlock_shop->second.text_tail.fit_text >= 0);
      CHECK(unlock_shop->second.text_tail.fit_text <= 2);
      CHECK(unlock_shop->second.text_tail.all_caps <= 1);
      for (float c : unlock_shop->second.text_tail.color) {
        CHECK(c >= 0.0f);
        CHECK(c <= 1.0f);
      }
    }
  }
  auto item_name = store_by_name.find("st_item_name.lbl");
  CHECK(item_name != store_by_name.end());
  if (item_name != store_by_name.end()) {
    CHECK(item_name->second.type == "BandLabel");
    CHECK(item_name->second.has_showing);
    CHECK(item_name->second.showing);
  }

  auto bonus_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/bonus_material.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> bonus_by_name;
  for (auto& l : bonus_labels) {
    std::printf("bonus label: %s type=%s font=%s text='%s' parent='%s' "
                "showing=%d:%d\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.has_showing ? 1 : 0,
                l.showing ? 1 : 0);
    bonus_by_name[l.name] = l;
  }
  CHECK(bonus_by_name.count("bm_buy.lbl") == 1);
  auto bonus_buy = bonus_by_name.find("bm_buy.lbl");
  if (bonus_buy != bonus_by_name.end()) {
    CHECK(bonus_buy->second.type == "BandLabel");
    CHECK(bonus_buy->second.font == "dyingmarker");
    CHECK(bonus_buy->second.text == "BUY_BONUS");
    CHECK(bonus_buy->second.text_tail.valid);
    if (bonus_buy->second.text_tail.valid) {
      CHECK(near(bonus_buy->second.text_tail.text_size, 30.0f));
      CHECK(near(bonus_buy->second.text_tail.width, 250.0f));
      CHECK(bonus_buy->second.text_tail.alignment == 33);
    }
  }
  CHECK(bonus_by_name.count("bm_hidden.btn") == 1);
  auto bonus_hidden = bonus_by_name.find("bm_hidden.btn");
  if (bonus_hidden != bonus_by_name.end()) {
    CHECK(bonus_hidden->second.type == "BandButton");
    CHECK(bonus_hidden->second.nav == "bm_video2.btn");
    CHECK(bonus_hidden->second.text.empty());
    CHECK(!bonus_hidden->second.button_tail.valid);
    CHECK(bonus_hidden->second.has_showing);
    CHECK(bonus_hidden->second.showing);
  }

  auto training_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/training.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> training_by_name;
  for (auto& l : training_labels) {
    std::printf("training label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f) "
                "button_tail=(%d %.3f %.3f %.3f %d %.3f) "
                "text_tail=(%d %.3f %.3f %.3f %d %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11],
                l.button_tail.valid ? 1 : 0, l.button_tail.width,
                l.button_tail.height, l.button_tail.text_size,
                l.button_tail.alignment, l.button_tail.width_bound,
                l.text_tail.valid ? 1 : 0, l.text_tail.width,
                l.text_tail.height, l.text_tail.text_size,
                l.text_tail.alignment, l.text_tail.width_bound);
    training_by_name[l.name] = l;
  }
  CHECK(training_by_name.count("selectmode.lbl") == 1);
  CHECK(training_by_name.count("tutorials.btn") == 1);
  CHECK(training_by_name.count("practice.btn") == 1);
  struct TrainingExpect {
    const char* name;
    const char* type;
    const char* text;
    const char* nav;
    float x;
    float z;
    float width;
    float height;
    float text_size;
    int alignment;
    float width_bound;
  };
  const TrainingExpect training_expects[] = {
      {"selectmode.lbl", "BandLabel", "select mode", "",
       65.000f, 37.000f, 150.000f, 40.000f, 25.000f, 66, 400.000f},
      {"tutorials.btn", "BandButton", "tutorials", "practice.btn",
       65.000f, 25.000f, 150.000f, 30.000f, 30.000f, 34, 250.000f},
      {"practice.btn", "BandButton", "practice", "tutorials.btn",
       65.000f, 0.000f, 150.000f, 30.000f, 30.000f, 34, 200.000f},
  };
  for (const auto& e : training_expects) {
    auto it = training_by_name.find(e.name);
    CHECK(it != training_by_name.end());
    if (it == training_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == "clarendon");
    CHECK(it->second.parent == "cp_buttons.view");
    CHECK(it->second.text == e.text);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_showing);
    CHECK(it->second.showing);
    CHECK(it->second.has_local);
    CHECK(it->second.has_world);
    CHECK(near(it->second.world[9], e.x));
    CHECK(near(it->second.world[10], 0.0f));
    CHECK(near(it->second.world[11], e.z));
    if (it->second.type == "BandButton") {
      CHECK(it->second.button_tail.valid);
      if (it->second.button_tail.valid) {
        CHECK(near(it->second.button_tail.width, e.width));
        CHECK(near(it->second.button_tail.height, e.height));
        CHECK(near(it->second.button_tail.text_size, e.text_size));
        CHECK(it->second.button_tail.alignment == e.alignment);
        CHECK(near(it->second.button_tail.width_bound, e.width_bound));
      }
      CHECK(!it->second.text_tail.valid);
    } else if (it->second.type == "BandLabel") {
      CHECK(it->second.text_tail.valid);
      if (it->second.text_tail.valid) {
        CHECK(near(it->second.text_tail.width, e.width));
        CHECK(near(it->second.text_tail.height, e.height));
        CHECK(near(it->second.text_tail.text_size, e.text_size));
        CHECK(it->second.text_tail.alignment == e.alignment);
        CHECK(near(it->second.text_tail.width_bound, e.width_bound));
      }
      CHECK(!it->second.button_tail.valid);
    }
  }

  auto speed_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/sel_speed.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> speed_by_name;
  for (auto& l : speed_labels) {
    std::printf("speed label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    speed_by_name[l.name] = l;
  }
  struct SpeedExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float z;
  };
  const SpeedExpect speed_expects[] = {
      {"title.lbl", "BandLabel", "clarendon", "sel_speed", "speed_poster.grp",
       "", 0.651f, -20.011f},
      {"speed0.btn", "BandButton", "helveticablack", "speed_label0",
       "buttons.view", "speed1.btn", 3.242f, -72.057f},
      {"speed1.btn", "BandButton", "helveticablack", "speed_label1",
       "buttons.view", "speed2.btn", 2.719f, -102.048f},
      {"speed2.btn", "BandButton", "helveticablack", "speed_label2",
       "buttons.view", "speed3.btn", 2.195f, -132.039f},
      {"speed3.btn", "BandButton", "helveticablack", "speed_label3",
       "buttons.view", "speed0.btn", 1.671f, -162.030f},
  };
  CHECK(speed_by_name.size() == 5);
  for (const auto& e : speed_expects) {
    auto it = speed_by_name.find(e.name);
    CHECK(it != speed_by_name.end());
    if (it == speed_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == e.font);
    CHECK(it->second.text == e.text);
    CHECK(it->second.parent == e.parent);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_showing);
    CHECK(it->second.showing);
    CHECK(it->second.has_world);
    if (it->second.has_world) {
      CHECK(near(it->second.world[9], e.x));
      CHECK(near(it->second.world[10], 0.0f));
      CHECK(near(it->second.world[11], e.z));
    }
    if (it->second.type == "BandLabel") CHECK(it->second.text_tail.valid);
  }

  auto video_checkboxes = ghogx::ui::extract_menu_checkboxes(
      hdr, ark0, "ui/gen/video_settings.milo_ps2");
  CHECK(video_checkboxes.size() == 4);
  std::map<std::string, ghogx::ui::MenuCheckbox> checkbox_by_name;
  for (auto& cb : video_checkboxes) {
    std::printf("video checkbox: %s type=%s resource='%s' parent='%s' "
                "checked=%d showing=%d world=(%.3f %.3f %.3f)\n",
                cb.name.c_str(), cb.type.c_str(), cb.resource.c_str(),
                cb.parent.c_str(), cb.checked ? 1 : 0,
                cb.showing ? 1 : 0, cb.world[9], cb.world[10], cb.world[11]);
    checkbox_by_name[cb.name] = cb;
  }
  const std::pair<const char*, float> expected_video_checks[] = {
      {"lefty1.chk", 112.0f},
      {"lefty2.chk", 82.0f},
      {"widescreen.chk", 52.0f},
      {"p_scan.chk", 22.0f},
  };
  for (const auto& expected : expected_video_checks) {
    auto it = checkbox_by_name.find(expected.first);
    CHECK(it != checkbox_by_name.end());
    if (it != checkbox_by_name.end()) {
      CHECK(it->second.type == "CheckBox");
      CHECK(it->second.resource == "default");
      CHECK(it->second.parent == "vs_buttons.view");
      CHECK(it->second.checked);
      CHECK(it->second.showing);
      CHECK(it->second.has_local);
      CHECK(it->second.has_world);
      CHECK(near(it->second.world[9], -89.973f));
      CHECK(near(it->second.world[10], -1.0f));
      CHECK(near(it->second.world[11], expected.second));
    }
  }
  auto p_scan = checkbox_by_name.find("p_scan.chk");
  CHECK(p_scan != checkbox_by_name.end());
  if (p_scan != checkbox_by_name.end()) {
    CHECK(p_scan->second.type == "CheckBox");
    CHECK(p_scan->second.resource == "default");
    CHECK(p_scan->second.parent == "vs_buttons.view");
    CHECK(p_scan->second.checked);
    CHECK(p_scan->second.showing);
    CHECK(p_scan->second.has_local);
    CHECK(p_scan->second.has_world);
    CHECK(near(p_scan->second.world[11], 22.0f));
  }
  auto lefty1_label = video_label_by_name.find("gs_left_p1.btn");
  auto lefty1_check = checkbox_by_name.find("lefty1.chk");
  CHECK(lefty1_label != video_label_by_name.end());
  CHECK(lefty1_check != checkbox_by_name.end());
  if (lefty1_label != video_label_by_name.end() &&
      lefty1_check != checkbox_by_name.end() &&
      lefty1_label->second.has_world && lefty1_check->second.has_world) {
    CHECK(lefty1_check->second.world[9] < lefty1_label->second.world[9]);
    CHECK(near(lefty1_label->second.world[11],
               lefty1_check->second.world[11]));
  }

  auto pause_video_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/pause_video_settings.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> pause_video_label_by_name;
  for (auto& l : pause_video_labels) {
    std::printf("pause video label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    if (l.button_tail.valid) {
      std::printf(" tail=(fit=%d w=%.3f h=%.3f lead=%.3f align=%d "
                  "size=%.3f bound=%.3f kern=%.3f)",
                  l.button_tail.fit_text, l.button_tail.width,
                  l.button_tail.height, l.button_tail.leading,
                  l.button_tail.alignment, l.button_tail.text_size,
                  l.button_tail.width_bound, l.button_tail.kerning);
    }
    std::printf("\n");
    pause_video_label_by_name[l.name] = l;
  }
  auto pause_video_checkboxes = ghogx::ui::extract_menu_checkboxes(
      hdr, ark0, "ui/gen/pause_video_settings.milo_ps2");
  std::map<std::string, ghogx::ui::MenuCheckbox>
      pause_video_checkbox_by_name;
  for (auto& cb : pause_video_checkboxes) {
    std::printf("pause video checkbox: %s type=%s resource='%s' parent='%s' "
                "checked=%d showing=%d world=(%.3f %.3f %.3f)\n",
                cb.name.c_str(), cb.type.c_str(), cb.resource.c_str(),
                cb.parent.c_str(), cb.checked ? 1 : 0,
                cb.showing ? 1 : 0, cb.world[9], cb.world[10], cb.world[11]);
    pause_video_checkbox_by_name[cb.name] = cb;
  }
  struct PauseVideoLabelExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float y;
    float z;
    bool has_showing;
    float width;
    float height;
    int alignment;
    float width_bound;
  };
  const PauseVideoLabelExpect pause_video_label_expects[] = {
      {"gs_title.lbl", "BandLabel", "rokk", "VIDEO SETTINGS", "", "", 0.0f,
       0.0f, 105.0f, false, 0.0f, 0.0f, 0, 0.0f},
      {"gs_left_p1.btn", "BandButton", "helveticablack", "LEFT_PLAYER_1",
       "gs_buttons.view", "gs_left_p2.btn", -105.0f, 0.0f, 45.0f, true,
       230.0f, 35.0f, 33, 1000.0f},
      {"gs_left_p2.btn", "BandButton", "helveticablack", "LEFT_PLAYER_2",
       "gs_buttons.view", "widescreen.btn", -105.0f, 0.0f, 15.0f, true,
       230.0f, 35.0f, 33, 1000.0f},
      {"widescreen.btn", "BandButton", "helveticablack", "vs_widescreen",
       "gs_buttons.view", "p_scan.btn", -105.0f, 0.0f, -15.0f, true,
       230.0f, 40.0f, 33, 350.0f},
      {"p_scan.btn", "BandButton", "helveticablack", "vs_pscan",
       "gs_buttons.view", "calibrate_lag.btn", -105.0f, 0.0f, -45.0f, true,
       230.0f, 40.0f, 33, 330.0f},
      {"calibrate_lag.btn", "BandButton", "helveticablack", "vs_lag",
       "gs_poster.view", "gs_left_p1.btn", 0.0f, 0.0f, -95.0f, true,
       270.0f, 40.0f, 34, 330.0f},
  };
  for (const auto& e : pause_video_label_expects) {
    auto it = pause_video_label_by_name.find(e.name);
    CHECK(it != pause_video_label_by_name.end());
    if (it == pause_video_label_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == e.font);
    CHECK(lbl.text == e.text);
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.has_showing == e.has_showing);
    if (lbl.has_showing) CHECK(lbl.showing);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], e.x));
      CHECK(near(lbl.world[10], e.y));
      CHECK(near(lbl.world[11], e.z));
    }
    if (lbl.type == "BandButton") {
      CHECK(lbl.button_tail.valid);
      if (lbl.button_tail.valid) {
        CHECK(lbl.button_tail.fit_text == 2);
        CHECK(near(lbl.button_tail.width, e.width));
        CHECK(near(lbl.button_tail.height, e.height));
        CHECK(near(lbl.button_tail.leading, 1.0f));
        CHECK(lbl.button_tail.alignment == e.alignment);
        CHECK(near(lbl.button_tail.kerning, 0.0f));
        CHECK(near(lbl.button_tail.text_size, 30.0f));
        CHECK(near(lbl.button_tail.width_bound, e.width_bound));
      }
    } else {
      CHECK(lbl.text_tail.valid);
    }
  }
  CHECK(pause_video_label_by_name.size() == 6);
  const std::pair<const char*, float> expected_pause_video_checks[] = {
      {"lefty1.chk", 45.0f},
      {"lefty2.chk", 15.0f},
      {"widescreen.chk", -15.0f},
      {"p_scan.chk", -45.0f},
  };
  for (const auto& expected : expected_pause_video_checks) {
    auto it = pause_video_checkbox_by_name.find(expected.first);
    CHECK(it != pause_video_checkbox_by_name.end());
    if (it == pause_video_checkbox_by_name.end()) continue;
    const auto& cb = it->second;
    CHECK(cb.type == "CheckBox");
    CHECK(cb.resource == "default");
    CHECK(cb.parent == "gs_buttons.view");
    CHECK(cb.checked);
    CHECK(cb.showing);
    CHECK(cb.has_local);
    CHECK(cb.has_world);
    CHECK(near(cb.world[9], -120.0f));
    CHECK(near(cb.world[10], -1.0f));
    CHECK(near(cb.world[11], expected.second));
  }
  CHECK(pause_video_checkbox_by_name.size() == 4);
  for (const auto& row : {
           std::pair<const char*, const char*>("lefty1.chk",
                                               "gs_left_p1.btn"),
           std::pair<const char*, const char*>("lefty2.chk",
                                               "gs_left_p2.btn"),
           std::pair<const char*, const char*>("widescreen.chk",
                                               "widescreen.btn"),
           std::pair<const char*, const char*>("p_scan.chk", "p_scan.btn"),
       }) {
    auto cb = pause_video_checkbox_by_name.find(row.first);
    auto label = pause_video_label_by_name.find(row.second);
    CHECK(cb != pause_video_checkbox_by_name.end());
    CHECK(label != pause_video_label_by_name.end());
    if (cb != pause_video_checkbox_by_name.end() &&
        label != pause_video_label_by_name.end() && cb->second.has_world &&
        label->second.has_world) {
      CHECK(cb->second.world[9] < label->second.world[9]);
      CHECK(near(cb->second.world[11], label->second.world[11]));
    }
  }
  auto checkbox_textures = ghogx::asset::load_milo_textures_from_sources(
      hdr, ark0, {"ui/gen/checkbox.milo_ps2"},
      {"checkbox_on.tex", "checkbox_off.tex"});
  CHECK(checkbox_textures.count("checkbox_on.tex") == 1);
  CHECK(checkbox_textures.count("checkbox_off.tex") == 1);
  if (checkbox_textures.count("checkbox_on.tex") == 1)
    CHECK(checkbox_textures["checkbox_on.tex"].valid());
  if (checkbox_textures.count("checkbox_off.tex") == 1)
    CHECK(checkbox_textures["checkbox_off.tex"].valid());

  auto pscan_warning_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/pscan_warning.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> pscan_warning_by_name;
  for (auto& l : pscan_warning_labels) {
    std::printf("pscan warning label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    pscan_warning_by_name[l.name] = l;
  }
  struct PscanLabelExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float z;
  };
  const PscanLabelExpect pscan_warning_expects[] = {
      {"title.lbl", "BandLabel", "clarendon", "WARNING", "dl_text.grp", "",
       -0.872f, 39.983f},
      {"message.lbl", "BandLabel", "helveticablack", "pscan_warning",
       "dl_text.grp", "", -1.082f, 45.979f},
      {"pscan_ok.btn", "BandButton", "clarendon", "OK", "dl_buttons.view",
       "pscan_cancel.btn", 3.385f, -81.934f},
      {"pscan_cancel.btn", "BandButton", "clarendon", "CANCEL",
       "dl_buttons.view", "pscan_ok.btn", 4.153f, -103.919f},
  };
  CHECK(pscan_warning_by_name.size() == 4);
  for (const auto& e : pscan_warning_expects) {
    auto it = pscan_warning_by_name.find(e.name);
    CHECK(it != pscan_warning_by_name.end());
    if (it == pscan_warning_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == e.font);
    CHECK(it->second.text == e.text);
    CHECK(it->second.parent == e.parent);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_showing);
    CHECK(it->second.showing);
    CHECK(it->second.has_world);
    if (it->second.has_world) {
      CHECK(near(it->second.world[9], e.x));
      CHECK(near(it->second.world[10], 0.0f));
      CHECK(near(it->second.world[11], e.z));
    }
    if (it->second.type == "BandLabel") CHECK(it->second.text_tail.valid);
  }

  auto pscan_switch_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/pscan_switching.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> pscan_switch_by_name;
  for (auto& l : pscan_switch_labels) {
    std::printf("pscan switch label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    pscan_switch_by_name[l.name] = l;
  }
  const PscanLabelExpect pscan_switch_expects[] = {
      {"title.lbl", "BandLabel", "clarendon", "WARNING", "dl_text.grp", "",
       -0.872f, 39.983f},
      {"message.lbl", "BandLabel", "helveticablack", "pscan_switching",
       "dl_text.grp", "", -1.082f, 45.979f},
      {"pscan_yes.btn", "BandButton", "clarendon", "YES", "dl_buttons.view",
       "pscan_no.btn", 3.385f, -81.934f},
      {"pscan_no.btn", "BandButton", "clarendon", "NO", "dl_buttons.view",
       "pscan_yes.btn", 4.153f, -103.919f},
  };
  CHECK(pscan_switch_by_name.size() == 4);
  for (const auto& e : pscan_switch_expects) {
    auto it = pscan_switch_by_name.find(e.name);
    CHECK(it != pscan_switch_by_name.end());
    if (it == pscan_switch_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == e.font);
    CHECK(it->second.text == e.text);
    CHECK(it->second.parent == e.parent);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_showing);
    CHECK(it->second.showing);
    CHECK(it->second.has_world);
    if (it->second.has_world) {
      CHECK(near(it->second.world[9], e.x));
      CHECK(near(it->second.world[10], 0.0f));
      CHECK(near(it->second.world[11], e.z));
    }
    if (it->second.type == "BandLabel") CHECK(it->second.text_tail.valid);
  }

  auto lag_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/lag.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> lag_by_name;
  for (auto& l : lag_labels) {
    std::printf("lag label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    lag_by_name[l.name] = l;
  }
  const PscanLabelExpect lag_expects[] = {
      {"title.lbl", "BandLabel", "helveticablack", "lag_title", "", "",
       -20.0f, 181.0f},
      {"instructions.lbl", "BandLabel", "helveticablackcondensed",
       "lag_info_why", "", "", -60.0f, 108.0f},
      {"instructions2.lbl", "BandLabel", "helveticablackcondensed",
       "lag_info_howto", "", "", -237.0f, -48.0f},
      {"autocalibrate.btn", "BandButton", "helveticablack",
       "lag_button_calibrate", "", "reset_to_zero.btn", -237.0f, -80.0f},
      {"reset_to_zero.btn", "BandButton", "helveticablack",
       "lag_button_reset", "", "autocalibrate.btn", -237.0f, -105.0f},
      {"setting.lbl", "BandLabel", "helveticablackcondensed",
       "lag_setting", "", "", -237.0f, -132.0f},
      {"countdown.lbl", "BandLabel", "helveticablackcondensed",
       "lag_success", "", "", 182.0f, -10.0f},
  };
  CHECK(lag_by_name.size() == 7);
  for (const auto& e : lag_expects) {
    auto it = lag_by_name.find(e.name);
    CHECK(it != lag_by_name.end());
    if (it == lag_by_name.end()) continue;
    CHECK(it->second.type == e.type);
    CHECK(it->second.font == e.font);
    CHECK(it->second.text == e.text);
    CHECK(it->second.parent == e.parent);
    CHECK(it->second.nav == e.nav);
    CHECK(it->second.has_world);
    if (it->second.has_world) {
      CHECK(near(it->second.world[9], e.x));
      CHECK(std::fabs(it->second.world[10]) < 0.01f);
      CHECK(near(it->second.world[11], e.z));
    }
    if (it->second.type == "BandLabel") CHECK(it->second.text_tail.valid);
  }

  auto dialog_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/dialog.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> dialog_by_name;
  for (auto& l : dialog_labels) dialog_by_name[l.name] = l;
  struct DialogLabelExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
  };
  const DialogLabelExpect dialog_expects[] = {
      {"dl_title.lbl", "BandLabel", "clarendon", "MEMORY CARD ERROR",
       "dl_text.grp"},
      {"dl_message.lbl", "BandLabel", "helveticablack",
       "No Guitar Hero save data present on Memory Card (PS2) in MEMORY CARD "
       "slot 1. Guitar Hero uses an Autosave feature. 107kb of free space is "
       "required to save game data. Do you want to create a save file now?",
       "dl_text.grp"},
      {"dl_button1.btn", "BandButton", "clarendon", "BUTTON1",
       "dl_buttons.view"},
      {"dl_button2.btn", "BandButton", "clarendon", "BUTTON2",
       "dl_buttons.view"},
  };
  CHECK(dialog_by_name.size() == 4);
  for (const auto& e : dialog_expects) {
    auto it = dialog_by_name.find(e.name);
    CHECK(it != dialog_by_name.end());
    if (it == dialog_by_name.end()) continue;
    const auto& lbl = it->second;
    std::printf("dialog label: %s type=%s font=%s text='%s' parent='%s' "
                "showing=%d:%d world=(%.3f %.3f %.3f)",
                lbl.name.c_str(), lbl.type.c_str(), lbl.font.c_str(),
                lbl.text.c_str(), lbl.parent.c_str(),
                lbl.has_showing ? 1 : 0, lbl.showing ? 1 : 0,
                lbl.world[9], lbl.world[10], lbl.world[11]);
    if (lbl.text_tail.valid) {
      std::printf(" tail=(fit=%d w=%.3f h=%.3f lead=%.3f align=%d "
                  "size=%.3f bound=%.3f caps=%d)",
                  lbl.text_tail.fit_text, lbl.text_tail.width,
                  lbl.text_tail.height, lbl.text_tail.leading,
                  lbl.text_tail.alignment, lbl.text_tail.text_size,
                  lbl.text_tail.width_bound,
                  static_cast<int>(lbl.text_tail.all_caps));
    }
    std::printf("\n");
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == e.font);
    CHECK(lbl.text == e.text);
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.has_local);
    CHECK(lbl.has_showing);
    CHECK(lbl.showing);
    if (lbl.type == "BandLabel") CHECK(lbl.text_tail.valid);
    if (lbl.name == "dl_title.lbl" && lbl.text_tail.valid) {
      CHECK(lbl.text_tail.fit_text == 2);
      CHECK(lbl.text_tail.alignment == 66);  // RndText::kBottomCenter
      CHECK(near(lbl.text_tail.text_size, 30.0f));
      CHECK(near(lbl.text_tail.height, 30.0f));
    }
    if (lbl.name == "dl_message.lbl" && lbl.text_tail.valid) {
      CHECK(lbl.text_tail.fit_text == 2);
      CHECK(lbl.text_tail.alignment == 18);  // RndText::kTopCenter
      CHECK(near(lbl.text_tail.leading, 0.72f));
      CHECK(near(lbl.text_tail.text_size, 24.0f));
      CHECK(near(lbl.text_tail.height, 105.0f));
    }
  }

  auto song_list = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/sel_song_quickplay.milo_ps2", "ss_song.lst");
  CHECK(song_list.valid);
  if (song_list.valid) {
    std::printf("ss_song.lst: rev=%u num_display=%d min=%d max=%d "
                "circular=%d speed=%.3f num_data=%d\n",
                static_cast<unsigned>(song_list.revision),
                song_list.num_display, song_list.min_display,
                song_list.max_display, song_list.circular ? 1 : 0,
                song_list.speed, song_list.num_data);
    CHECK(song_list.revision > 0);
    CHECK(song_list.num_display > 0);
    CHECK(song_list.num_display <= 32);
    CHECK(song_list.min_display >= 0);
    CHECK(song_list.min_display < song_list.num_display);
    CHECK(song_list.max_display == -1 ||
          (song_list.max_display >= song_list.min_display &&
           song_list.max_display < song_list.num_display));
    CHECK(song_list.speed >= 0.0f);
    CHECK(song_list.has_legacy_row_metrics);
    if (song_list.has_legacy_row_metrics) {
      CHECK(near(song_list.legacy_visible_slots, 5.0f));
      CHECK(near(song_list.legacy_row_height, 40.0f));
      CHECK(near(song_list.legacy_text_height, 30.0f));

      ghogx::ui::MenuFont song_font;
      CHECK(song_font.load(hdr, ark0, "ui/gen/dyingmarker.milo_ps2"));
      CHECK(song_font.valid());
      if (song_font.valid()) {
        // UIList's legacy compact fields describe the visible list window and
        // row spacing. The rendered text size comes from list_song.milo's
        // UILabel/RndText slot object, not this 30.0 layout field.
        CHECK(song_list.legacy_text_height > song_font.cap_height());
      }
    }
  }

  auto list_text = ghogx::ui::extract_menu_text_style(
      hdr, ark0, "ui/gen/list_song.milo_ps2", "list.txt");
  CHECK(list_text.valid);
  if (list_text.valid) {
    CHECK(list_text.parent == "list_song");
    CHECK(list_text.font == "dyingmarker.font");
    CHECK(list_text.alignment == 33);
    CHECK(list_text.text == "TONIGHT I'M GONNA ROCK YOU TONIGHT");
    CHECK(near(list_text.wrap_width, 320.0f));
    CHECK(near(list_text.leading, 0.75f));
    CHECK(list_text.fixed_length == 0);
    CHECK(near(list_text.italic_strength, 0.0f));
    CHECK(near(list_text.text_size, 26.0f));
    CHECK(!list_text.markup);
    CHECK(list_text.caps_mode == 2);

    ghogx::ui::MenuFont song_font;
    CHECK(song_font.load(hdr, ark0, "ui/gen/dyingmarker.milo_ps2"));
    CHECK(song_font.valid());
    if (song_font.valid()) {
      // UIListLabel clones list_song.milo's label resource; the slot text's
      // RndText::mSize is therefore the source-backed song-row font size.
      // MiloLib names the font metrics cellSize; the authored text scale uses
      // the vertical cell height, while ss_song.lst's text_h=30 is just the slot
      // window metric.
      CHECK(near(list_text.text_size / song_font.line_height(), 26.0f / 28.0f));
      CHECK(!near(song_list.legacy_text_height / song_font.line_height(),
                  list_text.text_size / song_font.line_height()));
      CHECK(list_text.text_size < 30.0f);
    }
  }

  auto header_text = ghogx::ui::extract_menu_text_style(
      hdr, ark0, "ui/gen/list_song.milo_ps2", "header.txt");
  CHECK(header_text.valid);
  if (header_text.valid) {
    CHECK(header_text.parent == "list_song");
    CHECK(header_text.font == "dyingmarker.font");
    CHECK(header_text.alignment == 33);
    CHECK(header_text.text == "LIST TEXT");
    CHECK(near(header_text.wrap_width, 0.0f));
    CHECK(near(header_text.leading, 1.0f));
    CHECK(near(header_text.text_size, 25.0f));
    CHECK(header_text.caps_mode == 2);

    ghogx::ui::MenuFont song_font;
    CHECK(song_font.load(hdr, ark0, "ui/gen/dyingmarker.milo_ps2"));
    CHECK(song_font.valid());
    if (song_font.valid()) {
      CHECK(near(header_text.text_size / song_font.line_height(),
                 25.0f / 28.0f));
    }
  }

  auto section_list = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/practice_sel_section.milo_ps2", "sel_section.lst");
  CHECK(section_list.valid);
  if (section_list.valid) {
    std::printf("sel_section.lst: rev=%u num_display=%d min=%d max=%d "
                "circular=%d speed=%.3f num_data=%d row=%.3f text_h=%.3f "
                "world=(%.3f %.3f %.3f)\n",
                static_cast<unsigned>(section_list.revision),
                section_list.num_display, section_list.min_display,
                section_list.max_display, section_list.circular ? 1 : 0,
                section_list.speed, section_list.num_data,
                section_list.has_legacy_row_metrics
                    ? section_list.legacy_row_height
                    : 0.0f,
                section_list.has_legacy_row_metrics
                    ? section_list.legacy_text_height
                    : 0.0f,
                section_list.world[9], section_list.world[10],
                section_list.world[11]);
    CHECK(section_list.num_display == 8);
    CHECK(section_list.min_display == 0);
    CHECK(section_list.max_display == -1);
    CHECK(!section_list.circular);
    CHECK(section_list.has_world);
    CHECK(near(section_list.world[9], -82.0f));
    CHECK(near(section_list.world[10], 0.0f));
    CHECK(near(section_list.world[11], 49.0f));
    CHECK(section_list.has_legacy_row_metrics);
    if (section_list.has_legacy_row_metrics) {
      CHECK(near(section_list.legacy_visible_slots, 8.0f));
      CHECK(near(section_list.legacy_row_height, 20.0f));
      CHECK(near(section_list.legacy_text_height, 16.0f));
    }
  }

  auto section_text = ghogx::ui::extract_menu_text_style(
      hdr, ark0, "ui/gen/list_section.milo_ps2", "list.txt");
  CHECK(section_text.valid);
  if (section_text.valid) {
    std::printf("section list text: %s font=%s text='%s' parent=%s "
                "align=%d wrap=%.3f leading=%.3f size=%.3f "
                "world=(%.3f %.3f %.3f)\n",
                section_text.name.c_str(), section_text.font.c_str(),
                section_text.text.c_str(), section_text.parent.c_str(),
                section_text.alignment, section_text.wrap_width,
                section_text.leading, section_text.text_size,
                section_text.world[9], section_text.world[10],
                section_text.world[11]);
    CHECK(section_text.parent.empty());
    CHECK(section_text.font == "dyingmarker.font");
    CHECK(section_text.alignment == 33);
    CHECK(section_text.text == "LIST");
    CHECK(near(section_text.text_size, 25.0f));

    ghogx::ui::MenuFont song_font;
    CHECK(song_font.load(hdr, ark0, "ui/gen/dyingmarker.milo_ps2"));
    CHECK(song_font.valid());
    if (song_font.valid()) {
      CHECK(near(section_text.text_size / song_font.line_height(),
                 25.0f / 28.0f));
    }
  }

  auto audio_sliders = ghogx::ui::extract_menu_sliders(
      hdr, ark0, "ui/gen/game_settings.milo_ps2");
  CHECK(audio_sliders.size() == 3);
  std::map<std::string, ghogx::ui::MenuSlider> slider_by_name;
  for (auto& s : audio_sliders) {
    std::printf("audio slider: %s type=%s resource='%s' parent='%s' nav='%s' "
                "token='%s' current=%d steps=%d vertical=%d showing=%d "
                "world=(%.3f %.3f %.3f)\n",
                s.name.c_str(), s.type.c_str(), s.resource.c_str(),
                s.parent.c_str(), s.nav.c_str(), s.token.c_str(), s.current,
                s.num_steps, s.vertical ? 1 : 0, s.showing ? 1 : 0,
                s.world[9], s.world[10], s.world[11]);
    slider_by_name[s.name] = s;
  }
  struct ExpectedSlider {
    const char* name;
    const char* resource;
    const char* nav;
    const char* token;
    float x;
    float y;
    float z;
  };
  const ExpectedSlider expected_audio_sliders[] = {
      {"gs_band.sld", "aaron", "gs_guitar.sld", "gs_band", -48.0f, 0.0f,
       -28.006f},
      {"gs_guitar.sld", "aaron", "gs_sfx.sld", "gs_guitar", -48.0f,
       0.0f, -75.997f},
      {"gs_sfx.sld", "aaron", "gs_stereo.btn", "gs_sound_fx", -48.0f,
       0.0f, -123.988f},
  };
  for (const auto& expected : expected_audio_sliders) {
    auto it = slider_by_name.find(expected.name);
    CHECK(it != slider_by_name.end());
    if (it != slider_by_name.end()) {
      CHECK(it->second.type == "BandSlider");
      CHECK(it->second.resource == expected.resource);
      CHECK(it->second.parent == "gs_sliders.view");
      CHECK(it->second.current == 0);
      CHECK(it->second.num_steps == 1);
      CHECK(!it->second.vertical);
      CHECK(it->second.showing);
      CHECK(it->second.has_world);
      CHECK(it->second.nav == expected.nav);
      CHECK(it->second.token == expected.token);
      CHECK(near(it->second.world[9], expected.x));
      CHECK(near(it->second.world[10], expected.y));
      CHECK(near(it->second.world[11], expected.z));
    }
  }
  ghogx::milo_scene::Scene game_settings_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0,
                                      "ui/gen/game_settings.milo_ps2",
                                      game_settings_scene));
  const auto find_game_settings_group =
      [&game_settings_scene](const char* name)
      -> const ghogx::milo_scene::GroupObj* {
    for (const auto& group : game_settings_scene.groups) {
      if (group.name == name) return &group;
    }
    return nullptr;
  };
  const auto* game_settings_root = find_game_settings_group("game_settings.view");
  CHECK(game_settings_root != nullptr);
  if (game_settings_root) {
    CHECK(game_settings_root->source_order_decoded);
    CHECK(game_settings_root->children.size() == 9);
    if (game_settings_root->children.size() == 9) {
      CHECK(game_settings_root->children[3] == "gs_band.sld");
      CHECK(game_settings_root->children[4] == "gs_guitar.sld");
      CHECK(game_settings_root->children[5] == "gs_sfx.sld");
    }
  }
  const auto* game_settings_slider_group =
      find_game_settings_group("gs_sliders.view");
  CHECK(game_settings_slider_group != nullptr);
  if (game_settings_slider_group) {
    CHECK(near(game_settings_slider_group->local.pos[0], 102.0f));
    CHECK(near(game_settings_slider_group->local.pos[1], 0.0f));
    CHECK(near(game_settings_slider_group->local.pos[2], -60.0f));
  }
  auto pause_sliders = ghogx::ui::extract_menu_sliders(
      hdr, ark0, "ui/gen/pause_audio_settings.milo_ps2");
  CHECK(pause_sliders.size() == 3);
  std::map<std::string, ghogx::ui::MenuSlider> pause_slider_by_name;
  for (auto& s : pause_sliders) {
    std::printf("pause slider: %s type=%s resource='%s' parent='%s' nav='%s' "
                "token='%s' current=%d steps=%d vertical=%d showing=%d "
                "world=(%.3f %.3f %.3f)\n",
                s.name.c_str(), s.type.c_str(), s.resource.c_str(),
                s.parent.c_str(), s.nav.c_str(), s.token.c_str(), s.current,
                s.num_steps, s.vertical ? 1 : 0, s.showing ? 1 : 0,
                s.world[9], s.world[10], s.world[11]);
    pause_slider_by_name[s.name] = s;
  }
  const ExpectedSlider expected_pause_sliders[] = {
      {"gs_band.sld", "char", "gs_guitar.sld", "BAND", -38.0f, -2.0f,
       50.0f},
      {"gs_guitar.sld", "char", "gs_sfx.sld", "GUITAR", -38.0f, -2.0f,
       0.0f},
      {"gs_sfx.sld", "char", "gs_stereo.btn", "SOUND_FX", -38.0f,
       -2.0f, -50.0f},
  };
  for (const auto& expected : expected_pause_sliders) {
    auto it = pause_slider_by_name.find(expected.name);
    CHECK(it != pause_slider_by_name.end());
    if (it != pause_slider_by_name.end()) {
      CHECK(it->second.type == "BandSlider");
      CHECK(it->second.resource == expected.resource);
      CHECK(it->second.parent == "gs_sliders.view");
      CHECK(it->second.current == 0);
      CHECK(it->second.num_steps == 1);
      CHECK(!it->second.vertical);
      CHECK(it->second.showing);
      CHECK(it->second.has_world);
      CHECK(it->second.nav == expected.nav);
      CHECK(it->second.token == expected.token);
      CHECK(near(it->second.world[9], expected.x));
      CHECK(near(it->second.world[10], expected.y));
      CHECK(near(it->second.world[11], expected.z));
    }
  }
  auto pause_audio_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/pause_audio_settings.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> pause_audio_label_by_name;
  for (auto& l : pause_audio_labels) {
    std::printf("pause audio label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    pause_audio_label_by_name[l.name] = l;
  }
  auto pause_audio_checkboxes = ghogx::ui::extract_menu_checkboxes(
      hdr, ark0, "ui/gen/pause_audio_settings.milo_ps2");
  std::map<std::string, ghogx::ui::MenuCheckbox>
      pause_audio_checkbox_by_name;
  for (auto& cb : pause_audio_checkboxes) {
    std::printf("pause audio checkbox: %s type=%s resource='%s' parent='%s' "
                "checked=%d showing=%d world=(%.3f %.3f %.3f)\n",
                cb.name.c_str(), cb.type.c_str(), cb.resource.c_str(),
                cb.parent.c_str(), cb.checked ? 1 : 0,
                cb.showing ? 1 : 0, cb.world[9], cb.world[10], cb.world[11]);
    pause_audio_checkbox_by_name[cb.name] = cb;
  }
  struct PauseAudioLabelExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float x;
    float y;
    float z;
    bool has_showing;
  };
  const PauseAudioLabelExpect pause_audio_label_expects[] = {
      {"gs_title.lbl", "BandLabel", "rokk", "AUDIO SETTINGS", "", "", 0.0f,
       0.0f, 105.0f, false},
      {"gs_stereo.btn", "BandButton", "helveticablack", "STEREO_SOUND",
       "gs_buttons.view", "", -92.0f, 0.0f, -99.8f, true},
  };
  for (const auto& e : pause_audio_label_expects) {
    auto it = pause_audio_label_by_name.find(e.name);
    CHECK(it != pause_audio_label_by_name.end());
    if (it == pause_audio_label_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == e.font);
    CHECK(lbl.text == e.text);
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.has_showing == e.has_showing);
    if (lbl.has_showing) CHECK(lbl.showing);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], e.x));
      CHECK(near(lbl.world[10], e.y));
      CHECK(near(lbl.world[11], e.z));
    }
  }
  CHECK(pause_audio_label_by_name.size() == 2);
  auto stereo_checkbox = pause_audio_checkbox_by_name.find("stereo.chk");
  CHECK(stereo_checkbox != pause_audio_checkbox_by_name.end());
  if (stereo_checkbox != pause_audio_checkbox_by_name.end()) {
    const auto& cb = stereo_checkbox->second;
    CHECK(cb.type == "CheckBox");
    CHECK(cb.resource == "default");
    CHECK(cb.parent == "gs_buttons.view");
    CHECK(cb.checked);
    CHECK(cb.showing);
    CHECK(cb.has_local);
    CHECK(cb.has_world);
    CHECK(near(cb.world[9], -110.0f));
    CHECK(near(cb.world[10], -15.0f));
    CHECK(near(cb.world[11], -97.8f));
  }
  CHECK(pause_audio_checkbox_by_name.size() == 1);
  auto slider_anim = ghogx::ui::extract_menu_slider_anim(
      hdr, ark0, "ui/gen/slider.milo_ps2", "char_slider.tnm");
  CHECK(slider_anim.valid);
  if (slider_anim.valid) {
    CHECK(slider_anim.target == "char_slider_pod.mesh");
    CHECK(near(slider_anim.first[0], 31.0f));
    CHECK(near(slider_anim.first[1], -1.0f));
    CHECK(near(slider_anim.first[2], -5.0f));
    CHECK(near(slider_anim.first_frame, 0.0f));
    CHECK(near(slider_anim.last[0], 133.0f));
    CHECK(near(slider_anim.last[1], -1.0f));
    CHECK(near(slider_anim.last[2], -5.0f));
    CHECK(near(slider_anim.last_frame, 1.0f));
  }
  ghogx::milo_scene::Scene slider_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0, "ui/gen/slider.milo_ps2",
                                      slider_scene));
  if (!slider_scene.meshes.empty()) {
    const auto* slider_mesh = slider_scene.find_mat("aaron_slider_default.mat");
    const auto* slider_focus = slider_scene.find_mat("aaron_slider_focus.mat");
    const auto* slider_pod = slider_scene.find_mat("aaron_slider_pod.mat");
    const auto* slider_pod_focus =
        slider_scene.find_mat("aaron_slider_pod_focus.mat");
    CHECK(slider_mesh != nullptr);
    CHECK(slider_focus != nullptr);
    CHECK(slider_pod != nullptr);
    CHECK(slider_pod_focus != nullptr);
    if (slider_mesh) CHECK(slider_mesh->diffuse_tex == "slider_base.tex");
    if (slider_focus) CHECK(slider_focus->diffuse_tex == "slider_base.tex");
    if (slider_pod) CHECK(slider_pod->diffuse_tex == "slider_knob.tex");
    if (slider_pod_focus)
      CHECK(slider_pod_focus->diffuse_tex == "slider_knob.tex");
  }

  auto pause_tile_textures = ghogx::asset::load_milo_textures_from_sources(
      hdr, ark0, {"ui/gen/pause.milo_ps2", "ui/gen/pause_lose_tex.milo_ps2"},
      {"pl_tile.tex"});
  CHECK(pause_tile_textures.count("pl_tile.tex") == 1);
  if (pause_tile_textures.count("pl_tile.tex") == 1) {
    CHECK(pause_tile_textures["pl_tile.tex"].valid());
    CHECK(pause_tile_textures["pl_tile.tex"].width > 0);
    CHECK(pause_tile_textures["pl_tile.tex"].height > 0);
  }

  auto pause_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/pause.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> pause_by_name;
  for (auto& l : pause_labels) {
    std::printf("pause label: %s type=%s font=%s text='%s' parent='%s' "
                "nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    pause_by_name[l.name] = l;
  }
  struct PauseExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    const char* nav;
    float z;
  };
  const PauseExpect pause_expects[] = {
      {"pause_title.lbl", "BandLabel", "rokk", "pausetitle", "pause.view", "",
       90.0f},
      {"resume.btn", "BandButton", "helveticablack", "pause_resume",
       "pause_buttons.view", "restart.btn", 25.0f},
      {"restart.btn", "BandButton", "helveticablack", "pause_restart",
       "pause_buttons.view", "audio_options.btn", -5.0f},
      {"audio_options.btn", "BandButton", "helveticablack", "audio_settings",
       "pause_buttons.view", "video_options.btn", -35.0f},
      {"video_options.btn", "BandButton", "helveticablack", "video_settings",
       "pause_buttons.view", "quit.btn", -65.0f},
      {"quit.btn", "BandButton", "helveticablack", "pause_quit",
       "pause_buttons.view", "resume.btn", -95.0f},
  };
  for (const auto& e : pause_expects) {
    auto it = pause_by_name.find(e.name);
    CHECK(it != pause_by_name.end());
    if (it == pause_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == e.font);
    CHECK(lbl.text == e.text);
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.has_showing);
    CHECK(lbl.showing);
    CHECK(lbl.has_world);
    if (lbl.has_world) CHECK(near(lbl.world[11], e.z));
  }
  CHECK(pause_by_name.size() == 6);
  ghogx::ui::MenuFont rokk_font;
  CHECK(rokk_font.load(hdr, ark0, "ui/gen/rokk.milo_ps2"));
  CHECK(rokk_font.valid());

  auto pause_controller_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0,
                                     "ui/gen/pause_controller.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> pause_controller_by_name;
  for (auto& l : pause_controller_labels) {
    std::printf("pause controller label: %s type=%s font=%s text='%s' "
                "parent='%s' nav='%s' showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.nav.c_str(),
                l.has_showing ? 1 : 0, l.showing ? 1 : 0,
                l.world[9], l.world[10], l.world[11]);
    pause_controller_by_name[l.name] = l;
  }
  struct PauseControllerExpect {
    const char* name;
    const char* type;
    const char* font;
    const char* text;
    const char* parent;
    float z;
  };
  const PauseControllerExpect pause_controller_expects[] = {
      {"resume.btn", "BandButton", "helveticablack", "pause_resume",
       "pause_controller_buttons.view", -80.0f},
      {"pause_controller_msg.lbl", "BandLabel", "helveticablack",
       "controller_loss_msg", "pause_controller.view", 0.0f},
      {"pause_controller_title.lbl", "BandLabel", "rokk", "CONTROLLER_LOSS",
       "pause_controller.view", 90.0f},
  };
  for (const auto& e : pause_controller_expects) {
    auto it = pause_controller_by_name.find(e.name);
    CHECK(it != pause_controller_by_name.end());
    if (it == pause_controller_by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == e.type);
    CHECK(lbl.font == e.font);
    CHECK(lbl.text == e.text);
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.has_world);
    if (lbl.has_world) CHECK(near(lbl.world[11], e.z));
  }
  CHECK(pause_controller_by_name.size() == 3);
  if (auto it = pause_controller_by_name.find("pause_controller_title.lbl");
      it != pause_controller_by_name.end()) {
    CHECK(it->second.text_tail.valid);
    if (it->second.text_tail.valid) {
      const auto& t = it->second.text_tail;
      CHECK(t.fit_text == 2);
      CHECK(near(t.width, 280.0f));
      CHECK(near(t.height, 90.0f));
      CHECK(near(t.leading, 1.0f));
      CHECK(t.alignment == 34);
      CHECK(t.all_caps == 0);
      CHECK(near(t.text_size, 40.0f));
      CHECK(near(t.width_bound, 490.0f));
      CHECK(t.color[0] > 0.89f && t.color[1] < 0.01f &&
            t.color[2] < 0.01f && t.color[3] > 0.99f);
    }
  }

  struct ExpectedLoadingAnim {
    const char* name;
    const char* target;
    std::size_t rotations;
    std::size_t translations;
    float last_frame;
  };
  const ExpectedLoadingAnim expected_loading_anims[] = {
      {"wing1.tnm", "wing1.mesh", 5, 0, 20.0f},
      {"wing2.tnm", "wing2.mesh", 5, 0, 20.0f},
      {"tape.tnm", "flyingtape.grp", 9, 4, 20.0f},
      {"loading_word.tnm", "loading_word.mesh", 11, 11, 25.0f},
  };
  for (const auto& expected : expected_loading_anims) {
    auto anim = ghogx::ui::extract_menu_slider_anim(
        hdr, ark0, "ui/gen/loading.milo_ps2", expected.name);
    CHECK(anim.valid);
    if (anim.valid) {
      CHECK(anim.target == expected.target);
      CHECK(anim.rotation_keys.size() == expected.rotations);
      CHECK(anim.translation_keys.size() == expected.translations);
      CHECK(!anim.rotation_keys.empty());
      CHECK(near(anim.rotation_keys.front().frame, 0.0f));
      CHECK(near(anim.rotation_keys.back().frame, expected.last_frame));
      if (expected.translations > 0) {
        CHECK(near(anim.translation_keys.front().frame, 0.0f));
        CHECK(near(anim.translation_keys.back().frame, expected.last_frame));
      }
    }
  }
  auto loading_word_mat_anim = ghogx::ui::extract_menu_material_anim(
      hdr, ark0, "ui/gen/loading.milo_ps2", "loading_word.mnm");
  CHECK(loading_word_mat_anim.valid);
  if (loading_word_mat_anim.valid) {
    CHECK(loading_word_mat_anim.material == "loading_word.mat");
    CHECK(loading_word_mat_anim.texture_keys.size() == 3);
    if (loading_word_mat_anim.texture_keys.size() == 3) {
      CHECK(loading_word_mat_anim.texture_keys[0].texture ==
            "loading_word_gw.tex");
      CHECK(near(loading_word_mat_anim.texture_keys[0].frame, 0.0f));
      CHECK(loading_word_mat_anim.texture_keys[1].texture ==
            "loading_word2_gw.tex");
      CHECK(near(loading_word_mat_anim.texture_keys[1].frame, 5.0f));
      CHECK(loading_word_mat_anim.texture_keys[2].texture ==
            "loading_word_gw.tex");
      CHECK(near(loading_word_mat_anim.texture_keys[2].frame, 10.0f));
    }
    CHECK(near(loading_word_mat_anim.first_frame, 0.0f));
    CHECK(near(loading_word_mat_anim.last_frame, 10.0f));
  }
  auto loading_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/loading.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> loading_by_name;
  for (auto& l : loading_labels) {
    std::printf("loading label: %s type=%s font=%s text='%s' parent='%s' "
                "showing=%d:%d world=(%.3f %.3f %.3f)\n",
                l.name.c_str(), l.type.c_str(), l.font.c_str(),
                l.text.c_str(), l.parent.c_str(), l.has_showing ? 1 : 0,
                l.showing ? 1 : 0, l.world[9], l.world[10], l.world[11]);
    loading_by_name[l.name] = l;
  }
  CHECK(loading_by_name.size() == 1);
  auto loading_tip = loading_by_name.find("tip.lbl");
  CHECK(loading_tip != loading_by_name.end());
  if (loading_tip != loading_by_name.end()) {
    const auto& lbl = loading_tip->second;
    CHECK(lbl.type == "BandLabel");
    CHECK(lbl.font == "dyingmarker");
    CHECK(lbl.parent == "load_poster.view");
    CHECK(lbl.text.find("Some HDTVs have an audio/visual lag") == 0);
    CHECK(lbl.text.find("Calibrate Lag") != std::string::npos);
    CHECK(lbl.has_showing);
    CHECK(lbl.showing);
    CHECK(lbl.text_tail.valid);
    CHECK(lbl.has_world);
    if (lbl.has_world) {
      CHECK(near(lbl.world[9], -12.0f));
      CHECK(near(lbl.world[10], 0.0f));
      CHECK(near(lbl.world[11], -31.999f));
    }
  }
  ghogx::milo_scene::Scene loading_scene;
  CHECK(ghogx::milo_scene::load_scene(hdr, ark0, "ui/gen/loading.milo_ps2",
                                      loading_scene));
  const auto find_group =
      [&loading_scene](const char* name) -> const ghogx::milo_scene::GroupObj* {
    for (const auto& group : loading_scene.groups) {
      if (group.name == name) return &group;
    }
    return nullptr;
  };
  const auto* loading_root = find_group("loading.grp");
  CHECK(loading_root != nullptr);
  if (loading_root) {
    CHECK(loading_root->source_order_decoded);
    CHECK(loading_root->children.size() == 5);
    if (loading_root->children.size() == 5) {
      CHECK(loading_root->children[0] == "load_wall.mesh");
      CHECK(loading_root->children[1] == "ready.mesh");
      CHECK(loading_root->children[2] == "flyingtape2.grp");
      CHECK(loading_root->children[3] == "loading.view");
      CHECK(loading_root->children[4] == "load_blink.view");
    }
  }
  const auto* loading_view = find_group("loading.view");
  CHECK(loading_view != nullptr);
  if (loading_view) {
    CHECK(loading_view->children.size() == 2);
    if (loading_view->children.size() == 2) {
      CHECK(loading_view->children[0] == "load_poster.view");
      CHECK(loading_view->children[1] == "loading_word.grp");
    }
  }
  const auto* loading_word_grp = find_group("loading_word.grp");
  CHECK(loading_word_grp != nullptr);
  if (loading_word_grp) {
    CHECK(near(loading_word_grp->local.pos[0], -140.0f));
    CHECK(near(loading_word_grp->local.pos[1], 0.0f));
    CHECK(near(loading_word_grp->local.pos[2], -155.0f));
  }
  const auto* flying_tape_grp = find_group("flyingtape2.grp");
  CHECK(flying_tape_grp != nullptr);
  if (flying_tape_grp) {
    CHECK(near(flying_tape_grp->local.pos[0], -189.2f));
    CHECK(near(flying_tape_grp->local.pos[1], -3.0f));
    CHECK(near(flying_tape_grp->local.pos[2], -213.696f));
  }

  auto credits_list = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/credits.milo_ps2", "credits.lst");
  CHECK(credits_list.valid);
  if (credits_list.valid) {
    std::printf("credits.lst: rev=%u num_display=%d min=%d max=%d "
                "circular=%d speed=%.3f num_data=%d legacy=(%d,%d,%d,%d) legacy_row=%d "
                "row=%.3f text_h=%.3f world_t=(%.3f %.3f %.3f)\n",
                static_cast<unsigned>(credits_list.revision),
                credits_list.num_display, credits_list.min_display,
                credits_list.max_display, credits_list.circular ? 1 : 0,
                credits_list.speed, credits_list.num_data,
                credits_list.legacy_i, credits_list.legacy_j,
                credits_list.legacy_k, credits_list.legacy_x,
                credits_list.has_legacy_row_metrics ? 1 : 0,
                credits_list.legacy_row_height,
                credits_list.legacy_text_height, credits_list.world[9],
                credits_list.world[10], credits_list.world[11]);
    CHECK(credits_list.num_display > 0);
    CHECK(credits_list.num_display <= 32);
    CHECK(credits_list.revision == 2);
    CHECK(credits_list.num_display == 16);
    CHECK(credits_list.min_display == 0);
    CHECK(credits_list.max_display == -1);
    CHECK(!credits_list.circular);
    CHECK(near(credits_list.speed, 1.0f));
    CHECK(credits_list.has_legacy_row_metrics);
    CHECK(near(credits_list.legacy_visible_slots, 16.0f));
    CHECK(near(credits_list.legacy_row_height, 25.0f));
    CHECK(near(credits_list.legacy_text_height, 30.0f));
    CHECK(credits_list.has_world);
    CHECK(near(credits_list.world[9], 0.0f));
    CHECK(near(credits_list.world[10], 0.0f));
    CHECK(near(credits_list.world[11], 186.0f));
  }

  auto credit_slots =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/list_credits.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> credit_slot_by_name;
  for (auto& label : credit_slots) {
    std::printf("credit slot: %s type=%s text='%s' m=(%.3f %.3f %.3f %.3f) world=(%.3f %.3f %.3f)\n",
                label.name.c_str(), label.type.c_str(), label.text.c_str(),
                label.world[0], label.world[2], label.world[6], label.world[8],
                label.world[9], label.world[10], label.world[11]);
    credit_slot_by_name[label.name] = label;
  }
  CHECK(credit_slot_by_name.count("title.txt") == 1);
  CHECK(credit_slot_by_name.count("centername.txt") == 1);
  CHECK(credit_slot_by_name.count("center.txt") == 1);
  CHECK(credit_slot_by_name.count("name.txt") == 1);
  for (const char* name :
       {"title.txt", "centername.txt", "center.txt", "name.txt"}) {
    auto it = credit_slot_by_name.find(name);
    if (it != credit_slot_by_name.end()) {
      CHECK(it->second.has_world);
      CHECK(it->second.type == "Text");
    }
  }

  ghogx::ui::MenuFont credits_font;
  CHECK(credits_font.load(hdr, ark0, "ui/gen/clarendon.milo_ps2"));
  CHECK(credits_font.valid());
  if (credits_font.valid()) {
    CHECK(near(credits_font.cap_height(), 30.0f));
    CHECK(near(credits_font.line_height(), 36.0f));
  }
  ghogx::ui::MenuFont rockletters_font;
  CHECK(rockletters_font.load(hdr, ark0, "ui/gen/rockletters.milo_ps2"));
  CHECK(rockletters_font.valid());
  for (const char* name :
       {"title.txt", "centername.txt", "center.txt", "name.txt"}) {
    auto style = ghogx::ui::extract_menu_text_style(
        hdr, ark0, "ui/gen/list_credits.milo_ps2", name);
    CHECK(style.valid);
    if (style.valid) {
      const bool rock = std::string(name) == "center.txt";
      CHECK(style.font == (rock ? "rockletters.font" : "clarendon.font"));
      CHECK(near(style.text_size, rock ? 30.0f : 20.0f));
      const ghogx::ui::MenuFont& style_font = rock ? rockletters_font : credits_font;
      CHECK(style_font.valid());
      std::printf("credit style: %s font=%s text='%s' size=%.3f cap=%.3f "
                  "line=%.3f cap_scale=%.3f align=%d wrap=%.3f\n",
                  name, style.font.c_str(), style.text.c_str(),
                  style.text_size,
                  style_font.valid() ? style_font.cap_height() : 0.0f,
                  style_font.valid() ? style_font.line_height() : 0.0f,
                  style_font.valid() ? style.text_size / style_font.cap_height() : 0.0f,
                  style.alignment, style.wrap_width);
    }
  }

  // The retail Quickplay post-song chain is authored across these three
  // panels. Keep their exact child names and list geometry pinned so the
  // runtime cannot silently replace the canonical widgets with bespoke UI.
  const auto newspaper_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/endgame.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> newspaper_by_name;
  for (const auto& label : newspaper_labels) {
    newspaper_by_name[label.name] = label;
    std::printf("newspaper label: %s font=%s text='%s' parent='%s' "
                "local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f) "
                "world_rows=[%.6f %.6f %.6f; %.6f %.6f %.6f; "
                "%.6f %.6f %.6f] "
                "fit=%d align=%d box=(%.3f %.3f) size=%.3f wrap=%.3f\n",
                label.name.c_str(), label.font.c_str(), label.text.c_str(),
                label.parent.c_str(), label.local[9], label.local[10],
                label.local[11], label.world[9], label.world[10],
                label.world[11], label.world[0], label.world[1],
                label.world[2], label.world[3], label.world[4],
                label.world[5], label.world[6], label.world[7],
                label.world[8], label.text_tail.fit_text,
                label.text_tail.alignment, label.text_tail.width,
                label.text_tail.height, label.text_tail.text_size,
                label.text_tail.width_bound);
  }
  for (const char* name : {
           "endgame_headline.lbl", "endgame_song_data.lbl",
           "endgame_review_data.lbl", "endgame_score_data.lbl",
           "endgame_percent_data.lbl", "endgame_diff_data.lbl",
           "endgame_streak_data.lbl", "endgame_score.lbl",
           "endgame_percent.lbl", "endgame_diff.lbl"}) {
    auto it = newspaper_by_name.find(name);
    CHECK(it != newspaper_by_name.end());
    if (it != newspaper_by_name.end()) {
      CHECK(it->second.parent == "newspaper.grp");
      CHECK(it->second.has_local);
      CHECK(it->second.has_world);
      CHECK(it->second.text_tail.valid);
    }
  }
  for (const auto& [name, label] : newspaper_by_name) {
    if (label.parent != "newspaper.grp" || !label.has_world) continue;
    const auto origin =
        ghogx::ui::transform_menu_text_point(label.world, 0.0f, 0.0f);
    const auto local_x =
        ghogx::ui::transform_menu_text_point(label.world, 1.0f, 0.0f);
    const auto local_z =
        ghogx::ui::transform_menu_text_point(label.world, 0.0f, 1.0f);
    CHECK(near(origin[0], label.world[9]));
    CHECK(near(origin[1], label.world[10]));
    CHECK(near(origin[2], label.world[11]));
    CHECK(near(local_x[0] - origin[0], label.world[0]));
    CHECK(near(local_x[1] - origin[1], label.world[1]));
    CHECK(near(local_x[2] - origin[2], label.world[2]));
    CHECK(near(local_z[0] - origin[0], label.world[6]));
    CHECK(near(local_z[1] - origin[1], label.world[7]));
    CHECK(near(local_z[2] - origin[2], label.world[8]));
  }
  ghogx::milo_scene::Scene newspaper_scene;
  CHECK(ghogx::milo_scene::load_scene(
      hdr, ark0, "ui/gen/endgame.milo_ps2", newspaper_scene));
  const ghogx::milo_scene::GroupObj* newspaper_group = nullptr;
  for (const auto& group : newspaper_scene.groups) {
    if (group.name == "newspaper.grp") newspaper_group = &group;
  }
  CHECK(newspaper_group != nullptr);
  if (newspaper_group) {
    CHECK(near(newspaper_group->local.rot[0][2], 0.0383868f));
    CHECK(near(newspaper_group->local.rot[2][0], -0.0383868f));
  }
  const ghogx::milo_scene::MeshObj* newspaper_mesh = nullptr;
  for (const auto& mesh : newspaper_scene.meshes) {
    if (mesh.name == "me_newspaper.mesh") newspaper_mesh = &mesh;
  }
  CHECK(newspaper_mesh != nullptr);
  if (newspaper_mesh) {
    CHECK(newspaper_mesh->parent == "newspaper.grp");
    const auto world = newspaper_scene.world_matrix(*newspaper_mesh);
    // The shipped page and its local-X print direction rise together in world
    // Z. This catches the former row/column transpose that reversed all
    // newspaper text angles while leaving their origins approximately right.
    CHECK(world[2] > 0.0f);
    CHECK(newspaper_by_name["endgame_headline.lbl"].world[2] > 0.0f);
  }
  const auto stock_photo = ghogx::asset::load_ps2_bitmap_from_ark(
      hdr, ark0, "ui/image/og/gen/photo_rock20_keep.bmp_ps2");
  CHECK(stock_photo.valid());
  CHECK(stock_photo.width == 128);
  CHECK(stock_photo.height == 128);
  const auto fallback_photo = ghogx::asset::load_milo_texture_named(
      hdr, ark0, "ui/gen/picture_endgame.milo_ps2", "pic_photo.tex");
  CHECK(fallback_photo.valid());

  const auto result_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/endgame_stats.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> result_by_name;
  for (const auto& label : result_labels) {
    result_by_name[label.name] = label;
    std::printf("post stats label: %s type=%s font=%s text='%s' parent='%s' "
                "world=(%.3f %.3f %.3f)\n",
                label.name.c_str(), label.type.c_str(), label.font.c_str(), label.text.c_str(),
                label.parent.c_str(), label.world[9], label.world[10],
                label.world[11]);
  }
  for (const char* name : {"song_name.lbl", "player0_notes_hit.lbl",
                           "player0_sp_phrases.lbl",
                           "player0_avg_multi.lbl"})
    CHECK(result_by_name.count(name) == 1);
  const auto stats_list = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/endgame_stats.milo_ps2", "stats_sections.lst");
  CHECK(stats_list.valid);
  if (stats_list.valid) {
    CHECK(stats_list.revision == 2);
    CHECK(stats_list.has_world);
    std::printf("post stats list: display=%d row=%.3f text=%.3f "
                "world=(%.3f %.3f %.3f)\n",
                stats_list.num_display, stats_list.legacy_row_height,
                stats_list.legacy_text_height, stats_list.world[9],
                stats_list.world[10], stats_list.world[11]);
  }
  const auto stats_slots = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/list_stats.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> stats_slot_by_name;
  for (const auto& label : stats_slots) {
    stats_slot_by_name[label.name] = label;
    std::printf("post stats slot: %s type=%s font=%s text='%s' "
                "world=(%.3f %.3f %.3f)\n",
                label.name.c_str(), label.type.c_str(), label.font.c_str(),
                label.text.c_str(), label.world[9], label.world[10],
                label.world[11]);
  }
  for (const char* name : {"section.txt", "notes1.txt", "notes2.txt",
                           "notes1_best.txt", "notes2_best.txt",
                           "notes1_worst.txt", "notes2_worst.txt"})
    CHECK(stats_slot_by_name.count(name) == 1);

  const auto highscore_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/highscore.milo_ps2");
  const auto highscore_entry_style = ghogx::ui::extract_menu_text_style(
      hdr, ark0, "ui/gen/textentry.milo_ps2", "label_hand_pen.txt");
  CHECK(highscore_entry_style.valid);
  if (highscore_entry_style.valid) {
    std::printf(
        "highscore text-entry resource: parent='%s' font='%s' align=%d "
        "size=%.3f wrap=%.3f local=(%.3f %.3f %.3f) "
        "world=(%.3f %.3f %.3f)\n",
        highscore_entry_style.parent.c_str(), highscore_entry_style.font.c_str(),
        highscore_entry_style.alignment, highscore_entry_style.text_size,
        highscore_entry_style.wrap_width, highscore_entry_style.local[9],
        highscore_entry_style.local[10], highscore_entry_style.local[11],
        highscore_entry_style.world[9], highscore_entry_style.world[10],
        highscore_entry_style.world[11]);
    CHECK(highscore_entry_style.alignment == 34);
    CHECK(near(highscore_entry_style.text_size, 40.0f));
    CHECK(near(highscore_entry_style.wrap_width, 375.84f));
  }
  ghogx::milo_scene::Scene textentry_scene;
  CHECK(ghogx::milo_scene::load_scene(
      hdr, ark0, "ui/gen/textentry.milo_ps2", textentry_scene));
  for (const auto& mesh : textentry_scene.meshes) {
    const auto world = textentry_scene.world_matrix(mesh);
    const auto* mat = textentry_scene.find_mat(mesh.material);
    std::printf(
        "highscore text-entry mesh: %s mat=%s tex=%s "
        "world=(%.3f %.3f %.3f) rows=[%.3f %.3f %.3f; %.3f %.3f %.3f; "
        "%.3f %.3f %.3f] bb=[%.3f %.3f %.3f]-[%.3f %.3f %.3f]\n",
        mesh.name.c_str(), mesh.material.c_str(),
        mat ? mat->diffuse_tex.c_str() : "", world[9], world[10], world[11],
        world[0], world[1], world[2], world[4], world[5], world[6], world[8],
        world[9], world[10],
        mesh.bb_min[0], mesh.bb_min[1], mesh.bb_min[2], mesh.bb_max[0],
        mesh.bb_max[1], mesh.bb_max[2]);
    for (std::size_t vi = 0; vi < std::min<std::size_t>(4, mesh.verts.size());
         ++vi) {
      const auto& v = mesh.verts[vi];
      std::printf("  v%zu=(%.3f %.3f %.3f uv=%.3f %.3f)\n", vi, v.px,
                  v.py, v.pz, v.u, v.v);
    }
  }
  std::map<std::string, ghogx::ui::MenuLabel> highscore_by_name;
  for (const auto& label : highscore_labels) {
    highscore_by_name[label.name] = label;
    std::printf("highscore label: %s type=%s font=%s text='%s' parent='%s' "
                "world=(%.3f %.3f %.3f) tail=%d fit=%d align=%d "
                "size=%.3f box=(%.3f %.3f) wrap=%.3f "
                "local=(%.3f %.3f %.3f; norms %.3f %.3f) "
                "world_norms=(%.3f %.3f)\n",
                label.name.c_str(), label.type.c_str(), label.font.c_str(), label.text.c_str(),
                label.parent.c_str(), label.world[9], label.world[10],
                label.world[11], label.text_tail.valid ? 1 : 0,
                label.text_tail.fit_text, label.text_tail.alignment,
                label.text_tail.text_size, label.text_tail.width,
                label.text_tail.height, label.text_tail.width_bound,
                label.local[9], label.local[10], label.local[11],
                std::sqrt(label.local[0] * label.local[0] +
                          label.local[2] * label.local[2]),
                std::sqrt(label.local[6] * label.local[6] +
                          label.local[8] * label.local[8]),
                std::sqrt(label.world[0] * label.world[0] +
                          label.world[2] * label.world[2]),
                std::sqrt(label.world[6] * label.world[6] +
                          label.world[8] * label.world[8]));
  }
  CHECK(highscore_by_name.count("hs_entry1.ten") == 1);
  CHECK(highscore_by_name.count("hs_name1.lbl") == 1);
  if (highscore_by_name.count("hs_entry1.ten") &&
      highscore_by_name.count("hs_name1.lbl")) {
    // The entry component begins at the same authored name-field edge; its
    // shared center-aligned RndText spans the resource wrap from that edge.
    CHECK(near(highscore_by_name["hs_entry1.ten"].local[9], 18.0f));
    CHECK(near(highscore_by_name["hs_name1.lbl"].local[9], 26.0f));
    const auto& entry_tail =
        highscore_by_name["hs_entry1.ten"].text_entry_tail;
    CHECK(entry_tail.valid);
    CHECK(near(entry_tail.flash_time, 1.0f));
    CHECK(near(entry_tail.text_scale, 0.2f));
    CHECK(near(entry_tail.arrow_offset, -3.0f));
    CHECK(near(entry_tail.entered_color[0], 0.0f));
    CHECK(near(entry_tail.entered_color[3], 1.0f));
    CHECK(near(entry_tail.dynamic_color[0], 0.8980392f));
    CHECK(near(entry_tail.dynamic_color[3], 1.0f));
  }
  for (int i = 1; i <= 5; ++i) {
    CHECK(highscore_by_name.count("hs_num" + std::to_string(i) + ".lbl") == 1);
    CHECK(highscore_by_name.count("hs_name" + std::to_string(i) + ".lbl") == 1);
    CHECK(highscore_by_name.count("hs_score" + std::to_string(i) + ".lbl") == 1);
  }

  const auto complete_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/complete.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> complete_by_name;
  for (const auto& label : complete_labels) {
    complete_by_name[label.name] = label;
    std::printf("complete label: %s type=%s font=%s text='%s' parent='%s' "
                "world=(%.3f %.3f %.3f)\n",
                label.name.c_str(), label.type.c_str(), label.font.c_str(), label.text.c_str(),
                label.parent.c_str(), label.world[9], label.world[10],
                label.world[11]);
  }
  CHECK(complete_by_name.count("comp_selsong.btn") == 1);
  CHECK(complete_by_name.count("comp_restart.btn") == 1);
  CHECK(complete_by_name.count("comp_quit.btn") == 1);

  if (g_failures == 0) {
    std::printf("ghogx_menu_labels_test: OK (main.milo BandButton labels + "
                "unaligned tail fields decoded)\n");
  } else {
    std::printf("ghogx_menu_labels_test: %d FAILURE(S)\n", g_failures);
  }
  return g_failures == 0 ? 0 : 1;
}
