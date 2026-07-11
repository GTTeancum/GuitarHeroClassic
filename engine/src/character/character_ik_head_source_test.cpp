#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
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

}  // namespace

int main() {
  using ghogx::character::SourceCharIKHeadPollDeps;
  using ghogx::character::source_char_ik_head_copy;
  using ghogx::character::source_char_ik_head_default_state;
  using ghogx::character::source_char_ik_head_load_steps;
  using ghogx::character::source_char_ik_head_poll_deps;
  using ghogx::character::source_char_ik_head_set_name;
  using ghogx::character::source_char_ik_head_update_points;
  using ghogx::character::source_char_weightable_set_weight_owner;

  bool ok = true;

  auto head = source_char_ik_head_default_state("ikhead.weight");
  ok &= near(head.weightable.weight, 1.0f, "default inherited weight");
  ok &= expect_string(head.weightable.weight_owner, "ikhead.weight",
                      "default weight owner");
  ok &= expect_string(head.head, "", "default head");
  ok &= expect_string(head.spine, "", "default spine");
  ok &= expect_string(head.mouth, "", "default mouth");
  ok &= expect_string(head.target, "", "default target");
  ok &= near(head.head_filter[0], 0.0f, "default head filter x");
  ok &= near(head.target_radius, 0.75f, "default target radius");
  ok &= near(head.head_mat, 0.5f, "default head mat");
  ok &= expect_string(head.offset, "", "default offset");
  ok &= near(head.offset_scale[0], 1.0f, "default offset scale x");
  ok &= near(head.offset_scale[1], 1.0f, "default offset scale y");
  ok &= near(head.offset_scale[2], 1.0f, "default offset scale z");
  ok &= expect_bool(head.update_points, true, "default update points");
  ok &= expect_size(head.points.size(), 0, "default point rows");

  auto set_name = source_char_ik_head_set_name(head, "rock1", true);
  ok &= expect_bool(set_name.call_hmx_set_name, true,
                    "SetName delegates Hmx object");
  ok &= expect_bool(set_name.assigned_character, true,
                    "SetName stores character dir");
  ok &= expect_string(head.character_dir, "rock1", "SetName character dir");

  set_name = source_char_ik_head_set_name(head, "not_character", false);
  ok &= expect_bool(set_name.assigned_character, false,
                    "SetName clears non-character dir");
  ok &= expect_string(head.character_dir, "", "SetName non-character clear");

  head.head = "bone_head";
  head.spine = "bone_spine";
  head.mouth = "bone_mouth";
  head.target = "look_target";
  head.offset = "head_offset";
  SourceCharIKHeadPollDeps deps;
  source_char_ik_head_poll_deps(
      deps, head, {"bone_head", "bone_neck", "bone_spine"}, true);
  ok &= expect_size(deps.changed_by.size(), 3, "PollDeps changed_by count");
  ok &= expect_string(deps.changed_by[0], "bone_mouth",
                      "PollDeps changed_by mouth");
  ok &= expect_string(deps.changed_by[1], "bone_head",
                      "PollDeps changed_by head");
  ok &= expect_string(deps.changed_by[2], "look_target",
                      "PollDeps changed_by target");
  ok &= expect_size(deps.change.size(), 4, "PollDeps change count");
  ok &= expect_string(deps.change[0], "bone_head",
                      "PollDeps chain starts at head");
  ok &= expect_string(deps.change[2], "bone_spine",
                      "PollDeps chain reaches spine parent boundary");
  ok &= expect_string(deps.change[3], "head_offset",
                      "PollDeps always publishes offset");

  SourceCharIKHeadPollDeps no_chain_deps;
  source_char_ik_head_poll_deps(no_chain_deps, head, {"bone_head"}, false);
  ok &= expect_size(no_chain_deps.change.size(), 1,
                    "PollDeps skips chain when generation is zero");
  ok &= expect_string(no_chain_deps.change[0], "head_offset",
                      "PollDeps zero generation keeps offset");

  auto update = source_char_ik_head_update_points(
      head, false, {"bone_head", "bone_neck", "bone_spine"},
      {2.0f, 3.0f, 5.0f});
  ok &= expect_bool(update.entered_body, true, "UpdatePoints enters body");
  ok &= expect_bool(update.rebuilt_points, true, "UpdatePoints rebuilds chain");
  ok &= expect_bool(head.update_points, false, "UpdatePoints clears dirty flag");
  ok &= expect_size(update.point_count, 3, "UpdatePoints point count");
  ok &= near(update.spine_length, 10.0f, "UpdatePoints spine length");
  ok &= expect_string(head.points[0].transform, "bone_head",
                      "UpdatePoints point transform");
  ok &= near(head.points[0].local_length, 2.0f,
             "UpdatePoints first local length");
  ok &= near(head.points[0].normalized_remaining, 1.0f,
             "UpdatePoints first normalized remaining");
  ok &= near(head.points[1].normalized_remaining, 0.8f,
             "UpdatePoints second normalized remaining");
  ok &= near(head.points[2].normalized_remaining, 0.5f,
             "UpdatePoints third normalized remaining");

  update = source_char_ik_head_update_points(
      head, false, {"bone_head", "bone_neck"}, {1.0f, 1.0f});
  ok &= expect_bool(update.entered_body, false,
                    "UpdatePoints clean non-forced no-op");
  ok &= expect_size(head.points.size(), 3,
                    "UpdatePoints clean no-op preserves rows");

  update = source_char_ik_head_update_points(
      head, true, {"bone_head"}, {4.0f});
  ok &= expect_bool(update.entered_body, true,
                    "UpdatePoints forced enters body");
  ok &= expect_bool(update.rebuilt_points, false,
                    "UpdatePoints generation zero does not rebuild");
  ok &= expect_size(head.points.size(), 0,
                    "UpdatePoints generation zero clears rows");

  auto load = source_char_ik_head_load_steps(1);
  ok &= expect_bool(load.load_hmx_object, true, "Load Hmx::Object");
  ok &= expect_bool(load.load_weightable, true, "Load CharWeightable");
  ok &= expect_bool(load.load_head, true, "Load head");
  ok &= expect_bool(load.load_spine, true, "Load spine");
  ok &= expect_bool(load.load_mouth, true, "Load mouth");
  ok &= expect_bool(load.load_target, true, "Load target");
  ok &= expect_bool(load.load_target_radius, false,
                    "Load rev1 skips target radius");
  ok &= expect_bool(load.load_offset, false, "Load rev1 skips offset");
  ok &= expect_bool(load.set_update_points, true,
                    "Load sets update points");

  load = source_char_ik_head_load_steps(3);
  ok &= expect_bool(load.load_target_radius, true,
                    "Load rev3 reads target radius");
  ok &= expect_bool(load.load_head_mat, true, "Load rev3 reads head mat");
  ok &= expect_bool(load.load_offset, true, "Load rev3 reads offset");
  ok &= expect_bool(load.load_offset_scale, true,
                    "Load rev3 reads offset scale");

  auto source = source_char_ik_head_default_state("source.head");
  source.head = "source_head";
  source.spine = "source_spine";
  source.mouth = "source_mouth";
  source.target = "source_target";
  source.target_radius = 1.25f;
  source.head_mat = 0.75f;
  source.offset = "source_offset";
  source.offset_scale = {2.0f, 3.0f, 4.0f};
  source_char_weightable_set_weight_owner(source.weightable, "shared.owner");

  auto dest = source_char_ik_head_default_state("dest.head");
  auto copy = source_char_ik_head_copy(dest, source, true, 0.25f);
  ok &= expect_bool(copy.copy_hmx_object, true, "Copy Hmx::Object");
  ok &= expect_bool(copy.copy_weightable, true, "Copy CharWeightable");
  ok &= expect_bool(copy.copy_head, true, "Copy head");
  ok &= expect_bool(copy.copy_spine, true, "Copy spine");
  ok &= expect_bool(copy.copy_mouth, true, "Copy mouth");
  ok &= expect_bool(copy.copy_target, true, "Copy target");
  ok &= expect_bool(copy.copy_target_radius, true, "Copy target radius");
  ok &= expect_bool(copy.copy_head_mat, true, "Copy head mat");
  ok &= expect_bool(copy.copy_offset, true, "Copy offset");
  ok &= expect_bool(copy.copy_offset_scale, true, "Copy offset scale");
  ok &= expect_bool(copy.set_update_points, true, "Copy sets update points");
  ok &= expect_string(dest.head, "source_head", "Copy head value");
  ok &= expect_string(dest.spine, "source_spine", "Copy spine value");
  ok &= expect_string(dest.mouth, "source_mouth", "Copy mouth value");
  ok &= expect_string(dest.target, "source_target", "Copy target value");
  ok &= near(dest.target_radius, 1.25f, "Copy target radius value");
  ok &= near(dest.head_mat, 0.75f, "Copy head mat value");
  ok &= expect_string(dest.offset, "source_offset", "Copy offset value");
  ok &= near(dest.offset_scale[2], 4.0f, "Copy offset scale value");
  ok &= expect_string(dest.weightable.weight_owner, "shared.owner",
                      "Copy shallow keeps weight owner");

  dest = source_char_ik_head_default_state("dest.deep");
  copy = source_char_ik_head_copy(dest, source, false, 0.66f);
  ok &= expect_string(dest.weightable.weight_owner, "dest.deep",
                      "Copy deep owns itself");
  ok &= near(dest.weightable.weight, 0.66f, "Copy deep owner weight");

  return ok ? 0 : 1;
}
