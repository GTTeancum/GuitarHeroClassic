#include "ui/config_db.h"
#include "ui/menu_app.h"
#include "ui/meta_objects.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"
#include "character/char_clip.h"
#include "character/char_mesh.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace ghogx;

namespace {
int source_rank(Symbol source) {
  if (source == Symbol("gh1")) return 1;
  if (source == Symbol("gh2")) return 2;
  if (source == Symbol("gh80")) return 3;
  return 99;
}

std::string transform_base_name(std::string name) {
  for (const std::string suffix : {".trans", ".mesh"}) {
    if (name.size() >= suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
      name.resize(name.size() - suffix.size());
      break;
    }
  }
  return name;
}

bool clip_drives_transform(const character::CharClip& clip,
                           const std::string& transform) {
  for (const auto& frame : clip.frames) {
    for (const auto& channel : frame) {
      if (transform_base_name(channel.bone_name) == transform) return true;
    }
  }
  return false;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fprintf(stderr,
                 "usage: ghogx_character_variant_catalog_test "
                 "<main.hdr> <main_0.ark>\n");
    return 2;
  }
  const auto ark = gh::ark::ArkV3Reader::load(argv[1]);
  ui::ConfigDb db;
  db.load(ark, {argv[2]});
  const std::vector<Symbol> characters = db.characters();
  if (characters.empty()) {
    std::fprintf(stderr, "FAIL character catalog is empty\n");
    return 1;
  }

  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);
  ui::install_meta_singletons(mgr, db);
  Object* provider = mgr.resolve_object(Symbol("character_provider"));
  if (!provider) {
    std::fprintf(stderr, "FAIL character_provider is missing\n");
    return 1;
  }

  {
    const auto closed = ui::source_charsys_external_door_rotation(0.0f);
    const float authored_open_z = 2.52563f;
    const auto open =
        ui::source_charsys_external_door_rotation(authored_open_z);
    const auto near = [](float a, float b) {
      return std::fabs(a - b) <= 1.0e-6f;
    };
    if (!near(closed[0], 1.0f) || !near(closed[4], 0.0f) ||
        !near(closed[5], 1.0f) || !near(closed[7], -1.0f) ||
        !near(open[0], std::cos(authored_open_z)) ||
        !near(open[1], std::sin(authored_open_z))) {
      std::fprintf(
          stderr,
          "FAIL CharsysPanel external-door Euler(pi/2,0,z) bridge\n");
      return 1;
    }
  }

  std::set<const void*> selections;
  std::size_t variants = 0;
  std::size_t gh1 = 0;
  std::size_t gh2 = 0;
  std::size_t gh80 = 0;
  std::size_t direct_door_variants = 0;
  std::size_t open_pose_fallback_variants = 0;
  for (Symbol character : characters) {
    const auto rows = db.character_variants(character);
    if (rows.empty()) {
      std::fprintf(stderr, "FAIL %s has no variants\n", character.c_str());
      return 1;
    }
    int previous_rank = 0;
    DataArray count_args;
    count_args.push(DataNode::Sym(character));
    const int provider_count =
        provider->handle_property(Symbol("num_outfits"), count_args)
            .as_int()
            .value_or(-1);
    if (provider_count != static_cast<int>(rows.size())) {
      std::fprintf(stderr, "FAIL %s provider=%d catalog=%zu\n",
                   character.c_str(), provider_count, rows.size());
      return 1;
    }
    for (std::size_t index = 0; index < rows.size(); ++index) {
      const auto& row = rows[index];
      const int rank = source_rank(row.source_game);
      if (rank < previous_rank || rank == 99 || row.label.empty() ||
          !selections.insert(row.selection.id()).second) {
        std::fprintf(stderr, "FAIL invalid row %s/%s\n",
                     character.c_str(), row.selection.c_str());
        return 1;
      }
      previous_rank = rank;
      for (const std::string* path :
           {&row.model_path, &row.ui_model_path, &row.ui_anim_path,
            &row.main_anim_path, &row.strum_anim_path,
            &row.fret_anim_path, &row.highway_surface_path}) {
        if (!path->empty() && !ark.find(*path)) {
          std::fprintf(stderr, "FAIL missing %s for %s\n", path->c_str(),
                       row.selection.c_str());
          return 1;
        }
      }
      ghogx::character::Character preview_character;
      if (!ghogx::character::load_character(
              argv[1], argv[2],
              row.ui_model_path.empty() ? row.model_path
                                        : row.ui_model_path,
              preview_character)) {
        std::fprintf(stderr, "FAIL preview model %s\n",
                     row.selection.c_str());
        return 1;
      }
      const auto clips = ghogx::character::load_clip_catalog(
          argv[1], argv[2], {row.ui_anim_path});
      const auto named_loop = std::find_if(
          clips.begin(), clips.end(),
          [](const auto& clip) { return clip.name == "ui_loop"; });
      const auto authored_idle = std::find_if(
          clips.begin(), clips.end(), [](const auto& clip) {
            const std::string suffix = "_idle_ui";
            return clip.name.size() >= suffix.size() &&
                   clip.name.compare(clip.name.size() - suffix.size(),
                                     suffix.size(), suffix) == 0;
          });
      const auto selected_clip =
          named_loop != clips.end()
              ? named_loop
              : (authored_idle != clips.end() ? authored_idle
                                              : clips.begin());
      const auto ui_loop =
          selected_clip == clips.end()
              ? ghogx::character::CharClip{}
              : ghogx::character::load_clip(
                    argv[1], argv[2], selected_clip->milo_path,
                    selected_clip->name);
      if (!ui_loop.loaded) {
        std::fprintf(stderr, "FAIL ui_loop %s from %s\n",
                     row.selection.c_str(), row.ui_anim_path.c_str());
        return 1;
      }
      const bool has_authored_door_pose =
          selected_clip->name == "ui_loop" &&
          clip_drives_transform(ui_loop, "bone_door");
      direct_door_variants += has_authored_door_pose ? 1 : 0;
      open_pose_fallback_variants += has_authored_door_pose ? 0 : 1;
      DataArray get_args;
      get_args.push(DataNode::Sym(character));
      get_args.push(DataNode::Int(static_cast<int>(index)));
      const Symbol provider_outfit =
          provider->handle_property(Symbol("get_outfit"), get_args)
              .as_symbol()
              .value_or(Symbol());
      if (provider_outfit != row.selection) {
        std::fprintf(stderr, "FAIL provider order %s index=%zu\n",
                     character.c_str(), index);
        return 1;
      }
      const DataNode provider_blurb =
          provider->handle_property(Symbol("get_outfit_blurb"), get_args);
      if (row.source_game == Symbol("gh2")) {
        const std::string expected =
            std::string(character.c_str()) + "_outfit_blurb";
        if (provider_blurb.as_symbol().value_or(Symbol()) !=
            Symbol(expected.c_str())) {
          std::fprintf(stderr, "FAIL native outfit blurb %s index=%zu\n",
                       character.c_str(), index);
          return 1;
        }
      } else if (!provider_blurb.as_string().value_or("").empty()) {
        std::fprintf(stderr, "FAIL imported outfit blurb %s index=%zu\n",
                     character.c_str(), index);
        return 1;
      }
      ++variants;
      gh1 += row.source_game == Symbol("gh1") ? 1 : 0;
      gh2 += row.source_game == Symbol("gh2") ? 1 : 0;
      gh80 += row.source_game == Symbol("gh80") ? 1 : 0;
    }
    // The cyclic selector's two-row viewport is [selected, selected+1].
    // These endpoint checks cover both forward and reverse wrap.
    const std::size_t last = rows.size() - 1;
    if (rows[(last + 1) % rows.size()].selection != rows[0].selection ||
        rows[(0 + rows.size() - 1) % rows.size()].selection !=
            rows[last].selection) {
      std::fprintf(stderr, "FAIL wrap %s\n", character.c_str());
      return 1;
    }
  }
  if (direct_door_variants == 0) {
    std::fprintf(stderr,
                 "FAIL no authored ui_loop bone_door pose is available\n");
    return 1;
  }

  std::printf(
      "PASS character catalog characters=%zu variants=%zu "
      "gh1=%zu gh2=%zu gh80=%zu order=chronological wrap=both "
      "viewport=2 door_bridge=euler_pi_over_2_0_z "
      "door_direct=%zu door_open_pose_fallback=%zu\n",
      characters.size(), variants, gh1, gh2, gh80, direct_door_variants,
      open_pose_fallback_variants);
  return 0;
}
