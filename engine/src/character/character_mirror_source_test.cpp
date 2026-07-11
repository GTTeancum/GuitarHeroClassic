#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>

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
  using ghogx::character::SourceCharMirrorPollDeps;
  using ghogx::character::source_char_mirror_copy;
  using ghogx::character::source_char_mirror_default_state;
  using ghogx::character::source_char_mirror_load_steps;
  using ghogx::character::source_char_mirror_poll;
  using ghogx::character::source_char_mirror_poll_deps;
  using ghogx::character::source_char_mirror_set_mirror_servo;
  using ghogx::character::source_char_mirror_set_servo;
  using ghogx::character::source_char_weightable_set_weight;
  using ghogx::character::source_char_weightable_set_weight_owner;

  bool ok = true;

  auto mirror = source_char_mirror_default_state("mirror.weight");
  ok &= near(mirror.weightable.weight, 1.0f, "default inherited weight");
  ok &= expect_string(mirror.weightable.weight_owner, "mirror.weight",
                      "default weight owner");
  ok &= expect_string(mirror.servo, "", "default servo");
  ok &= expect_string(mirror.mirror_servo, "", "default mirror servo");
  ok &= expect_size(mirror.bones_total_size, 0, "default bones size");
  ok &= expect_size(mirror.ops_count, 0, "default ops count");

  auto set = source_char_mirror_set_servo(mirror, "bone.servo");
  ok &= expect_bool(set.changed, true, "SetServo changed");
  ok &= expect_bool(set.synced_bones, true, "SetServo syncs bones");
  ok &= expect_string(mirror.servo, "bone.servo", "SetServo stores servo");

  set = source_char_mirror_set_servo(mirror, "bone.servo");
  ok &= expect_bool(set.changed, false, "SetServo same pointer no-op");
  ok &= expect_bool(set.synced_bones, false, "SetServo same pointer no sync");

  set = source_char_mirror_set_mirror_servo(mirror, "bone.mirror_servo");
  ok &= expect_bool(set.changed, true, "SetMirrorServo changed");
  ok &= expect_bool(set.synced_bones, true, "SetMirrorServo syncs bones");
  ok &= expect_string(mirror.mirror_servo, "bone.mirror_servo",
                      "SetMirrorServo stores mirror servo");

  SourceCharMirrorPollDeps deps;
  source_char_mirror_poll_deps(deps, mirror);
  ok &= expect_size(deps.changed_by.size(), 0,
                    "PollDeps does not publish changedBy");
  ok &= expect_size(deps.change.size(), 1, "PollDeps publishes servo change");
  ok &= expect_string(deps.change[0], "bone.servo", "PollDeps servo target");

  std::unordered_map<std::string, float> weights;
  auto poll = source_char_mirror_poll(mirror, weights);
  ok &= near(poll.weight, 1.0f, "Poll local weight");
  ok &= expect_bool(poll.bones_empty, true, "Poll sees empty bones");
  ok &= expect_bool(poll.scale_down, false, "Poll skips empty bones");

  mirror.bones_total_size = 48;
  source_char_weightable_set_weight(mirror.weightable, 0.0f);
  poll = source_char_mirror_poll(mirror, weights);
  ok &= expect_bool(poll.weight_zero, true, "Poll sees zero weight");
  ok &= expect_bool(poll.scale_down, false, "Poll skips zero weight");

  source_char_weightable_set_weight(mirror.weightable, 0.25f);
  poll = source_char_mirror_poll(mirror, weights);
  ok &= expect_bool(poll.scale_down, true,
                    "Poll scales down nonempty bones");
  ok &= near(poll.scale_down_weight, 0.75f,
             "Poll ScaleDown uses one minus weight");
  ok &= expect_string(poll.servo, "bone.servo", "Poll ScaleDown servo");

  source_char_weightable_set_weight_owner(mirror.weightable, "owner.weight");
  weights["owner.weight"] = 0.60f;
  poll = source_char_mirror_poll(mirror, weights);
  ok &= near(poll.weight, 0.60f, "Poll uses weight owner");
  ok &= near(poll.scale_down_weight, 0.40f,
             "Poll owner ScaleDown weight");

  auto steps = source_char_mirror_load_steps();
  ok &= expect_bool(steps.max_revision == 1, true, "Load max revision");
  ok &= expect_bool(steps.load_hmx_object, true, "Load Hmx::Object");
  ok &= expect_bool(steps.load_weightable, true, "Load CharWeightable");
  ok &= expect_bool(steps.load_mirror_servo, true, "Load mirror servo");
  ok &= expect_bool(steps.load_servo, true, "Load servo");
  ok &= expect_bool(steps.sync_bones, true, "Load SyncBones");

  auto dest = source_char_mirror_default_state("dest.mirror");
  auto copy = source_char_mirror_copy(dest, mirror, false, 0.80f);
  ok &= expect_bool(copy.copy_hmx_object, true, "Copy Hmx::Object");
  ok &= expect_bool(copy.copy_weightable, true, "Copy CharWeightable");
  ok &= expect_bool(copy.set_mirror_servo.changed, true,
                    "Copy SetMirrorServo");
  ok &= expect_bool(copy.set_servo.changed, true, "Copy SetServo");
  ok &= expect_string(dest.servo, "bone.servo", "Copy servo");
  ok &= expect_string(dest.mirror_servo, "bone.mirror_servo",
                      "Copy mirror servo");
  ok &= expect_string(dest.weightable.weight_owner, "dest.mirror",
                      "Copy deep owns itself");
  ok &= near(dest.weightable.weight, 0.80f, "Copy deep owner weight");

  auto shallow = source_char_mirror_default_state("shallow.mirror");
  copy = source_char_mirror_copy(shallow, mirror, true, 0.10f);
  ok &= expect_string(shallow.weightable.weight_owner, "owner.weight",
                      "Copy shallow keeps source owner");
  ok &= near(shallow.weightable.weight, 1.0f,
             "Copy shallow keeps local default weight");

  return ok ? 0 : 1;
}
