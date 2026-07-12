#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int got, int want, const char* label) {
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

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::source_char_interest_copy;
  using ghogx::character::source_char_interest_category_flags_prop_plan;
  using ghogx::character::source_char_interest_compute_score_plan;
  using ghogx::character::source_char_interest_compute_score_deterministic;
  using ghogx::character::source_char_interest_copy_plan;
  using ghogx::character::source_char_interest_defaults;
  using ghogx::character::source_char_interest_handler_plan;
  using ghogx::character::source_char_interest_is_matching_filter_flags;
  using ghogx::character::source_char_interest_is_within_view_cone;
  using ghogx::character::source_char_interest_load_plan;
  using ghogx::character::source_char_interest_load_revision_known;
  using ghogx::character::source_char_interest_prop_sync_plan;
  using ghogx::character::source_char_interest_sync_max_view_angle;

  bool ok = true;

  auto interest = source_char_interest_defaults();
  ok &= near(interest.max_view_angle, 20.0f, "default max view angle");
  ok &= near(interest.priority, 1.0f, "default priority");
  ok &= near(interest.min_look_time, 1.0f, "default min look time");
  ok &= near(interest.max_look_time, 3.0f, "default max look time");
  ok &= near(interest.refractory_period, 6.1f,
             "default refractory period");
  ok &= expect_string(interest.dart_override, "", "default dart override");
  ok &= expect_int(interest.category_flags, 0, "default category flags");
  ok &= expect_bool(interest.override_min_target_distance, false,
                    "default override min target distance");
  ok &= near(interest.min_target_distance_override, 35.0f,
             "default min target distance");
  ok &= near(interest.max_view_angle_cos,
             std::cos(20.0f * 0.017453292f),
             "default synced max view cosine");

  ok &= expect_bool(source_char_interest_load_revision_known(-1), false,
                    "revision -1 rejected");
  ok &= expect_bool(source_char_interest_load_revision_known(0), true,
                    "revision 0 accepted");
  ok &= expect_bool(source_char_interest_load_revision_known(6), true,
                    "revision 6 accepted");
  ok &= expect_bool(source_char_interest_load_revision_known(7), false,
                    "revision 7 rejected");

  const auto load_bad = source_char_interest_load_plan(7);
  ok &= expect_bool(load_bad.known_revision, false,
                    "load plan rejects revision 7");
  ok &= expect_int(static_cast<int>(load_bad.read_order.size()), 0,
                   "invalid load plan has no reads");

  const auto load_v1 = source_char_interest_load_plan(1);
  ok &= expect_bool(load_v1.known_revision, true,
                    "load v1 revision accepted");
  ok &= expect_int(static_cast<int>(load_v1.read_order.size()), 8,
                   "load v1 read count");
  ok &= expect_string(load_v1.read_order[0], "Hmx::Object",
                      "load v1 object first");
  ok &= expect_string(load_v1.read_order[1], "RndTransformable",
                      "load v1 transformable second");
  ok &= expect_string(load_v1.read_order[6], "mRefractoryPeriod",
                      "load v1 refractory period before branch");
  ok &= expect_string(load_v1.read_order[7], "mDartOverride",
                      "load v1 dart override branch");
  ok &= expect_string(load_v1.branches[0], "temp > 5",
                      "load v1 source temp branch");
  ok &= expect_bool(load_v1.sync_max_view_angle, true,
                    "load v1 syncs max view angle");

  const auto load_v3 = source_char_interest_load_plan(3);
  ok &= expect_int(static_cast<int>(load_v3.read_order.size()), 10,
                   "load v3 read count");
  ok &= expect_string(load_v3.read_order[7], "legacyObjectPtr",
                      "load v3 legacy object branch");
  ok &= expect_string(load_v3.read_order[8], "mCategoryFlags",
                      "load v3 category flags");
  ok &= expect_string(load_v3.read_order[9], "legacyCategoryFlagsByte",
                      "load v3 legacy category byte");

  const auto load_v5 = source_char_interest_load_plan(5);
  ok &= expect_int(static_cast<int>(load_v5.read_order.size()), 11,
                   "load v5 read count");
  ok &= expect_string(load_v5.read_order[7], "legacyObjectPtr",
                      "load v5 legacy object branch");
  ok &= expect_string(load_v5.read_order[8], "mCategoryFlags",
                      "load v5 category flags");
  ok &= expect_string(load_v5.read_order[9], "mOverrideMinTargetDistance",
                      "load v5 override min target flag");
  ok &= expect_string(load_v5.read_order[10], "mMinTargetDistanceOverride",
                      "load v5 min target override");

  const auto load_v6 = source_char_interest_load_plan(6);
  ok &= expect_int(static_cast<int>(load_v6.read_order.size()), 11,
                   "load v6 read count");
  ok &= expect_string(load_v6.read_order[7], "mDartOverride",
                      "load v6 dart override branch");
  ok &= expect_string(load_v6.read_order[8], "mCategoryFlags",
                      "load v6 category flags");
  ok &= expect_string(load_v6.read_order[9], "mOverrideMinTargetDistance",
                      "load v6 override min target flag");
  ok &= expect_string(load_v6.read_order[10], "mMinTargetDistanceOverride",
                      "load v6 min target override");

  ok &= near(source_char_interest_sync_max_view_angle(60.0f),
             std::cos(60.0f * 0.017453292f),
             "sync max view angle");
  ok &= expect_bool(source_char_interest_is_matching_filter_flags(0, 0x1),
                    false, "zero category does not match");
  ok &= expect_bool(source_char_interest_is_matching_filter_flags(0x4, 0x2),
                    false, "non-overlapping category does not match");
  ok &= expect_bool(source_char_interest_is_matching_filter_flags(0x6, 0x2),
                    true, "overlapping category matches");
  ok &= expect_bool(
      source_char_interest_is_within_view_cone({0.0f, 0.0f, 10.0f},
                                               {0.0f, 0.0f, 0.0f},
                                               {0.0f, 0.0f, 1.0f},
                                               source_char_interest_sync_max_view_angle(20.0f)),
      true, "view cone accepts forward interest");
  ok &= expect_bool(
      source_char_interest_is_within_view_cone({10.0f, 0.0f, 0.0f},
                                               {0.0f, 0.0f, 0.0f},
                                               {0.0f, 0.0f, 1.0f},
                                               source_char_interest_sync_max_view_angle(20.0f)),
      false, "view cone rejects side interest");

  const auto copy_plan = source_char_interest_copy_plan();
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy plan object superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "RndTransformable",
                      "copy plan transform superclass");
  ok &= expect_int(static_cast<int>(copy_plan.copied_members.size()), 9,
                   "copy plan member count");
  ok &= expect_string(copy_plan.copied_members[0], "mMaxViewAngle",
                      "copy plan first member");
  ok &= expect_string(copy_plan.copied_members.back(),
                      "mMinTargetDistanceOverride",
                      "copy plan last member");
  ok &= expect_bool(copy_plan.sync_max_view_angle, true,
                    "copy plan syncs max view angle");

  const auto prop_sync = source_char_interest_prop_sync_plan();
  ok &= expect_string(prop_sync.modify_properties[0], "max_view_angle",
                      "prop sync max view angle");
  ok &= expect_string(prop_sync.modify_actions[0], "SyncMaxViewAngle",
                      "prop sync action");
  ok &= expect_int(static_cast<int>(prop_sync.properties.size()), 7,
                   "prop sync direct count");
  ok &= expect_string(prop_sync.properties[4], "dart_ruleset_override",
                      "prop sync dart override");
  ok &= expect_string(prop_sync.custom_branches[0], "category_flags",
                      "prop sync category branch");
  ok &= expect_string(prop_sync.superclasses[0], "RndTransformable",
                      "prop sync superclass");

  const auto category_prop = source_char_interest_category_flags_prop_plan();
  ok &= expect_bool(category_prop.accepts_raw_category_flags, true,
                    "category prop accepts raw field");
  ok &= expect_bool(category_prop.accepts_symbol_bit_prefix, true,
                    "category prop accepts BIT symbol");
  ok &= expect_string(category_prop.required_symbol_prefix, "BIT_",
                      "category prop symbol prefix");
  ok &= expect_string(category_prop.operations[1],
                      "kPropGet returns mCategoryFlags & flags",
                      "category prop get operation");
  ok &= expect_string(category_prop.operations[3], "zero set clears mask",
                      "category prop clear operation");

  const auto handler_plan = source_char_interest_handler_plan();
  ok &= expect_string(handler_plan.superclasses[0], "RndTransformable",
                      "handler transform superclass");
  ok &= expect_string(handler_plan.superclasses[1], "Hmx::Object",
                      "handler object superclass");
  ok &= expect_int(handler_plan.check, 0x141, "handler check");

  const auto score_plan = source_char_interest_compute_score_plan();
  ok &= expect_string(score_plan.gates[0], "IsMatchingFilterFlags(mask)",
                      "score gate filter");
  ok &= expect_string(score_plan.score_steps[7],
                      "nonnegative score receives RandomFloat jitter",
                      "score random jitter step");
  ok &= expect_bool(score_plan.contains_random_float, true,
                    "score plan notes random");
  ok &= expect_bool(score_plan.safe_to_publish_runtime_score, false,
                    "score plan remains fenced");

  const float max_cos = source_char_interest_sync_max_view_angle(20.0f);
  auto score = source_char_interest_compute_score_deterministic(
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 10.0f}, 0.005f, 0x2,
      false, 0x6, 2.0f, max_cos, 0.125f);
  ok &= expect_bool(score.category_gate, true,
                    "score accepts matching category");
  ok &= expect_bool(score.default_category_gate, false,
                    "score does not use default category when flags match");
  ok &= near(score.distance_squared, 100.0f,
             "score distance squared");
  ok &= near(score.view_dot, 1.0f, "score view dot");
  ok &= expect_bool(score.view_dot_gate, true,
                    "score view dot gate");
  ok &= near(score.interest_dot, 1.0f, "score interest dot");
  ok &= expect_bool(score.interest_dot_gate, true,
                    "score interest dot gate");
  ok &= near(score.distance_score, 0.5f,
             "score distance contribution");
  ok &= near(score.pre_jitter_score, 1.51f,
             "score before jitter");
  ok &= expect_bool(score.applied_random_jitter, true,
                    "score applies nonnegative jitter");
  ok &= near(score.score, 3.27f, "score final priority multiply");

  score = source_char_interest_compute_score_deterministic(
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 10.0f}, 1.0f, 0x2,
      false, 0x6, 1.0f, max_cos, 0.25f);
  ok &= expect_bool(score.applied_random_jitter, false,
                    "negative score skips random jitter");
  ok &= near(score.score, -97.99f,
             "negative score still multiplies priority");

  score = source_char_interest_compute_score_deterministic(
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 10.0f}, 0.0f, 0x2,
      false, 0x0, 1.0f, max_cos, 0.0f);
  ok &= expect_bool(score.returned_reject, true,
                    "score rejects unmatched category");
  ok &= near(score.score, -1.0f, "score category reject value");

  score = source_char_interest_compute_score_deterministic(
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 10.0f}, 0.0f, 0x2,
      true, 0x0, 1.0f, max_cos, 0.2f);
  ok &= expect_bool(score.default_category_gate, true,
                    "score accepts default category fallback");
  ok &= near(score.score, 2.21f,
             "score default category fallback value");

  score = source_char_interest_compute_score_deterministic(
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 10.0f},
      std::numeric_limits<float>::quiet_NaN(), 0x2, false, 0x6,
      1.0f, max_cos, 0.0f);
  ok &= expect_bool(score.distance_score_was_nan, true,
                    "score records NaN distance fallback");
  ok &= near(score.distance_score, 0.2f,
             "score NaN distance fallback value");
  ok &= near(score.score, 1.21f, "score NaN fallback final value");

  interest.max_view_angle = 45.0f;
  interest.priority = 2.0f;
  interest.min_look_time = 0.25f;
  interest.max_look_time = 5.0f;
  interest.refractory_period = 9.0f;
  interest.dart_override = "dart.rules";
  interest.category_flags = 0x12;
  interest.override_min_target_distance = true;
  interest.min_target_distance_override = 21.0f;
  interest.max_view_angle_cos = -9.0f;

  const auto copied = source_char_interest_copy(interest);
  ok &= near(copied.max_view_angle, 45.0f, "copy max view angle");
  ok &= near(copied.priority, 2.0f, "copy priority");
  ok &= near(copied.min_look_time, 0.25f, "copy min look time");
  ok &= near(copied.max_look_time, 5.0f, "copy max look time");
  ok &= near(copied.refractory_period, 9.0f, "copy refractory period");
  ok &= expect_string(copied.dart_override, "dart.rules",
                      "copy dart override");
  ok &= expect_int(copied.category_flags, 0x12, "copy category flags");
  ok &= expect_bool(copied.override_min_target_distance, true,
                    "copy override min target distance");
  ok &= near(copied.min_target_distance_override, 21.0f,
             "copy min target distance");
  ok &= near(copied.max_view_angle_cos,
             std::cos(45.0f * 0.017453292f),
             "copy resyncs max view cosine");

  return ok ? 0 : 1;
}
