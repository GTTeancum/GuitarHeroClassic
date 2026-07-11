#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

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

bool expect_float(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharEyesInterest;
  using ghogx::character::SourceCharEyesPollDeps;
  using ghogx::character::source_char_eyes_current_interest;
  using ghogx::character::source_char_eyes_force_blink;
  using ghogx::character::source_char_eyes_get_head;
  using ghogx::character::source_char_eyes_list_poll_children;
  using ghogx::character::source_char_eyes_poll_deps;
  using ghogx::character::source_char_eyes_set_focus_interest;

  bool ok = true;

  const auto children = source_char_eyes_list_poll_children(
      {"l-eye.lookat", "r-eye.lookat"});
  ok &= expect_size(children.size(), 2, "children count");
  ok &= expect_string(children[0], "l-eye.lookat", "left child");
  ok &= expect_string(children[1], "r-eye.lookat", "right child");

  SourceCharEyesPollDeps deps;
  source_char_eyes_poll_deps(
      deps,
      {SourceCharEyesInterest{"same.interest", true},
       SourceCharEyesInterest{"other.interest", false}},
      true,
      "head.trans",
      "target.trans",
      "head.lookat",
      "face.servo");
  ok &= expect_size(deps.changed_by.size(), 4, "poll deps changed_by");
  ok &= expect_size(deps.change.size(), 1, "poll deps change");
  ok &= expect_string(deps.changed_by[0], "same.interest",
                      "same-dir interest dependency");
  ok &= expect_string(deps.changed_by[1], "head.trans",
                      "head dependency when eyes exist");
  ok &= expect_string(deps.changed_by[2], "head.lookat",
                      "head lookat dependency");
  ok &= expect_string(deps.changed_by[3], "face.servo",
                      "face servo dependency");
  ok &= expect_string(deps.change[0], "target.trans",
                      "target change when eyes exist");

  deps = SourceCharEyesPollDeps{};
  source_char_eyes_poll_deps(
      deps,
      {SourceCharEyesInterest{"same.interest", true}},
      false,
      "head.trans",
      "target.trans",
      "",
      "");
  ok &= expect_size(deps.changed_by.size(), 1,
                    "no eyes keeps same-dir interest only");
  ok &= expect_size(deps.change.size(), 0, "no eyes has no target change");
  ok &= expect_string(deps.changed_by[0], "same.interest",
                      "no eyes interest dependency");

  ok &= expect_string(
      source_char_eyes_get_head("view.trans", "eye.parent"), "view.trans",
      "GetHead view direction priority");
  ok &= expect_string(
      source_char_eyes_get_head("", "eye.parent"), "eye.parent",
      "GetHead eye source parent fallback");
  ok &= expect_string(source_char_eyes_get_head("", ""), "",
                      "GetHead no source");

  ok &= expect_string(
      source_char_eyes_current_interest("focus.interest", "look.interest"),
      "focus.interest", "current interest focus priority");
  ok &= expect_string(
      source_char_eyes_current_interest("", "look.interest"),
      "look.interest", "current interest fallback");
  ok &= expect_string(source_char_eyes_current_interest("", ""), "",
                      "current interest empty");

  const auto rejected_focus = source_char_eyes_set_focus_interest(
      "boss.focus", 5, "minor.focus", 3);
  ok &= expect_bool(rejected_focus.accepted, false, "focus priority reject");
  ok &= expect_string(rejected_focus.focus_interest, "boss.focus",
                      "focus reject preserves current");
  ok &= expect_int(rejected_focus.focus_priority, 5,
                   "focus reject preserves priority");

  const auto accepted_focus = source_char_eyes_set_focus_interest(
      "boss.focus", 5, "major.focus", 7);
  ok &= expect_bool(accepted_focus.accepted, true, "focus priority accept");
  ok &= expect_string(accepted_focus.focus_interest, "major.focus",
                      "focus accept writes requested");
  ok &= expect_int(accepted_focus.focus_priority, 7,
                   "focus accept writes priority");

  const auto cleared_focus =
      source_char_eyes_set_focus_interest("boss.focus", 5, "", 8);
  ok &= expect_bool(cleared_focus.accepted, true, "focus clear accept");
  ok &= expect_string(cleared_focus.focus_interest, "", "focus clear value");
  ok &= expect_int(cleared_focus.focus_priority, -1,
                   "focus clear resets priority");

  const auto blink = source_char_eyes_force_blink(12.5f);
  ok &= expect_bool(blink.pending_blink, true, "force blink pending");
  ok &= expect_float(blink.blink_time, 12.5f, "force blink time");
  ok &= expect_int(blink.blink_count_delta, 1, "force blink count delta");

  return ok ? 0 : 1;
}
