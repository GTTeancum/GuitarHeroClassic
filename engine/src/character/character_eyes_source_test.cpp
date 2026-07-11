#include "character/char_mesh.h"

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

}  // namespace

int main() {
  using ghogx::character::SourceCharEyesInterest;
  using ghogx::character::SourceCharEyesPollDeps;
  using ghogx::character::source_char_eyes_list_poll_children;
  using ghogx::character::source_char_eyes_poll_deps;

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

  return ok ? 0 : 1;
}
