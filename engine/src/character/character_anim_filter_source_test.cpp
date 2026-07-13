#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << (got ? "true" : "false")
            << " want " << (want ? "true" : "false") << "\n";
  return false;
}

bool expect_int(int got, int want, const char* label) {
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
  using ghogx::character::RndAnimFilter;
  using ghogx::character::SourceRndAnimFilterAnimInfo;
  using ghogx::character::source_rnd_anim_filter_anim_target;
  using ghogx::character::source_rnd_anim_filter_copy_plan;
  using ghogx::character::source_rnd_anim_filter_frame_bounds;
  using ghogx::character::source_rnd_anim_filter_frame_offset;
  using ghogx::character::source_rnd_anim_filter_handler_plan;
  using ghogx::character::source_rnd_anim_filter_list_anim_children;
  using ghogx::character::source_rnd_anim_filter_loop;
  using ghogx::character::source_rnd_anim_filter_prop_sync_plan;
  using ghogx::character::source_rnd_anim_filter_safe_anims;
  using ghogx::character::source_rnd_anim_filter_save_plan;
  using ghogx::character::source_rnd_anim_filter_scale;
  using ghogx::character::source_rnd_anim_filter_set_anim;

  bool ok = true;

  RndAnimFilter filter;
  ok &= expect_bool(source_rnd_anim_filter_loop(0), false,
                    "RndAnimFilter range does not loop");
  ok &= expect_bool(source_rnd_anim_filter_loop(1), true,
                    "RndAnimFilter loop loops");
  ok &= expect_bool(source_rnd_anim_filter_loop(2), true,
                    "RndAnimFilter shuttle loops");

  auto set_anim = source_rnd_anim_filter_set_anim(
      filter, "crash_static.anim", SourceRndAnimFilterAnimInfo{});
  ok &= expect_bool(set_anim.assigns_anim, true, "SetAnim assigns anim");
  ok &= expect_bool(set_anim.anim_present, false, "SetAnim no child anim");
  ok &= expect_string(filter.anim, "crash_static.anim",
                      "SetAnim stores pointer name");
  ok &= near(filter.start, 0.0f, "SetAnim missing child keeps start");
  ok &= near(filter.end, 0.0f, "SetAnim missing child keeps end");

  set_anim = source_rnd_anim_filter_set_anim(
      filter, "valid_child.anim",
      SourceRndAnimFilterAnimInfo{true, 60, 12.0f, 42.0f});
  ok &= expect_bool(set_anim.anim_present, true, "SetAnim child present");
  ok &= expect_bool(set_anim.copies_rate, true, "SetAnim copies rate");
  ok &= expect_bool(set_anim.copies_start_end, true,
                    "SetAnim copies child range");
  ok &= expect_int(filter.rate, 60, "SetAnim copied rate");
  ok &= near(filter.start, 12.0f, "SetAnim copied start");
  ok &= near(filter.end, 42.0f, "SetAnim copied end");

  auto scale = source_rnd_anim_filter_scale(10.0f, 40.0f, 3.0f, 2.0f, 30.0f);
  ok &= expect_bool(scale.period_path, true, "Scale period path");
  ok &= near(scale.scale, 0.5f, "Scale period formula");
  scale = source_rnd_anim_filter_scale(10.0f, 40.0f, 3.0f, 0.0f, 30.0f);
  ok &= expect_bool(scale.period_path, false, "Scale direct path");
  ok &= expect_bool(scale.reversed_range, false, "Scale forward range");
  ok &= near(scale.scale, 3.0f, "Scale forward value");
  scale = source_rnd_anim_filter_scale(40.0f, 10.0f, 3.0f, 0.0f, 30.0f);
  ok &= expect_bool(scale.reversed_range, true, "Scale reversed range");
  ok &= near(scale.scale, -3.0f, "Scale reversed value");

  auto offset = source_rnd_anim_filter_frame_offset(10.0f, 40.0f, 1.25f);
  ok &= expect_bool(offset.reversed_range, false, "FrameOffset forward");
  ok &= near(offset.frame_offset, 1.25f, "FrameOffset forward value");
  offset = source_rnd_anim_filter_frame_offset(40.0f, 10.0f, 1.25f);
  ok &= expect_bool(offset.reversed_range, true, "FrameOffset reversed");
  ok &= near(offset.frame_offset, 31.25f, "FrameOffset reversed value");

  filter.anim = "";
  auto bounds = source_rnd_anim_filter_frame_bounds(filter, 30.0f);
  ok &= expect_bool(bounds.has_anim, false, "Frame bounds no anim");
  ok &= near(bounds.start_frame, 0.0f, "StartFrame no anim");
  ok &= near(bounds.end_frame, 0.0f, "EndFrame no anim");

  filter.anim = "valid_child.anim";
  filter.start = 10.0f;
  filter.end = 30.0f;
  filter.offset = 1.0f;
  filter.scale = 2.0f;
  filter.period = 0.0f;
  filter.type = 0;
  bounds = source_rnd_anim_filter_frame_bounds(filter, 30.0f);
  ok &= expect_bool(bounds.has_anim, true, "Frame bounds has anim");
  ok &= near(bounds.scale, 2.0f, "Frame bounds scale");
  ok &= near(bounds.frame_offset, 1.0f, "Frame bounds offset");
  ok &= near(bounds.start_frame, 4.5f, "StartFrame source formula");
  ok &= near(bounds.end_frame, 14.5f, "EndFrame source formula");

  filter.start = 30.0f;
  filter.end = 10.0f;
  filter.type = 2;
  bounds = source_rnd_anim_filter_frame_bounds(filter, 30.0f);
  ok &= expect_bool(bounds.shuttle, true, "EndFrame shuttle path");
  ok &= near(bounds.scale, -2.0f, "Frame bounds reversed scale");
  ok &= near(bounds.frame_offset, 21.0f, "Frame bounds reversed offset");
  ok &= near(bounds.start_frame, -4.5f, "StartFrame reversed formula");
  ok &= near(bounds.end_frame, 11.0f, "EndFrame shuttle doubles");

  filter.scale = 0.0f;
  filter.start = 10.0f;
  filter.end = 30.0f;
  filter.type = 0;
  bounds = source_rnd_anim_filter_frame_bounds(filter, 30.0f);
  ok &= expect_bool(bounds.scale_was_zero, true,
                    "Frame bounds zero scale fallback");
  ok &= near(bounds.scale, 1.0f, "Frame bounds fallback scale");

  ok &= expect_string(source_rnd_anim_filter_anim_target(filter),
                      "valid_child.anim", "AnimTarget returns anim");
  auto children = source_rnd_anim_filter_list_anim_children(filter);
  ok &= expect_size(children.size(), 1, "ListAnimChildren count");
  ok &= expect_string(children[0], "valid_child.anim",
                      "ListAnimChildren anim");
  filter.anim = "";
  children = source_rnd_anim_filter_list_anim_children(filter);
  ok &= expect_size(children.size(), 0, "ListAnimChildren skips empty anim");

  auto copy = source_rnd_anim_filter_copy_plan(false);
  ok &= expect_size(copy.copied_superclasses.size(), 2,
                    "Copy superclass count");
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "Copy object superclass");
  ok &= expect_string(copy.copied_superclasses[1], "RndAnimatable",
                      "Copy animatable superclass");
  ok &= expect_size(copy.copied_members.size(), 9, "Copy member count");
  ok &= expect_string(copy.copied_members[0], "mScale",
                      "Copy first member");
  ok &= expect_string(copy.copied_members[8], "mJitter",
                      "Copy last member");
  copy = source_rnd_anim_filter_copy_plan(true);
  ok &= expect_bool(copy.copy_from_max, true, "CopyFromMax branch");
  ok &= expect_size(copy.copied_members.size(), 0,
                    "CopyFromMax skips members");

  auto safe = source_rnd_anim_filter_safe_anims(
      {{"intro.anim", false}, {"self.anim", true}, {"outro.anim", false}});
  ok &= expect_bool(safe.appends_null, true, "SafeAnims appends null");
  ok &= expect_size(safe.safe_anims.size(), 2, "SafeAnims filters count");
  ok &= expect_string(safe.safe_anims[0], "intro.anim",
                      "SafeAnims first safe");
  ok &= expect_string(safe.safe_anims[1], "outro.anim",
                      "SafeAnims second safe");

  auto handlers = source_rnd_anim_filter_handler_plan();
  ok &= expect_string(handlers.handlers[0], "safe_anims",
                      "Handlers safe_anims");
  ok &= expect_string(handlers.superclasses[0], "RndAnimatable",
                      "Handlers animatable superclass");
  ok &= expect_int(handlers.check, 0xe3, "Handlers check");

  auto props = source_rnd_anim_filter_prop_sync_plan();
  ok &= expect_string(props.set_properties[0], "anim:SetAnim",
                      "PropSync anim setter");
  ok &= expect_string(props.set_properties[1], "scale:abs",
                      "PropSync abs scale setter");
  ok &= expect_string(props.properties[0], "offset",
                      "PropSync offset");
  ok &= expect_string(props.properties.back(), "type", "PropSync type");
  ok &= expect_string(props.modify_properties[0],
                      "jitter:reset_jitter_frame",
                      "PropSync jitter reset");
  ok &= expect_string(props.superclasses[0], "RndAnimatable",
                      "PropSync animatable superclass");
  ok &= expect_int(source_rnd_anim_filter_save_plan().save_id, 0x4a,
                   "RndAnimFilter save id");

  return ok ? 0 : 1;
}
