#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
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
  using ghogx::character::source_char_interest_defaults;
  using ghogx::character::source_char_interest_is_matching_filter_flags;
  using ghogx::character::source_char_interest_load_plan;
  using ghogx::character::source_char_interest_load_revision_known;
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
