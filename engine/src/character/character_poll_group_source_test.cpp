#include "character/char_mesh.h"

#include <iostream>
#include <string>
#include <vector>

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
  using ghogx::character::SourceCharPollGroupChildDeps;
  using ghogx::character::SourceCharPollGroupPollDeps;
  using ghogx::character::source_char_poll_group_list_children;
  using ghogx::character::source_char_poll_group_poll_deps;
  using ghogx::character::source_char_poll_group_poll_order;

  bool ok = true;
  const std::vector<std::string> polls = {
      "first.poll", "second.poll", "third.poll"};

  auto order = source_char_poll_group_poll_order(0.0f, polls);
  ok &= expect_size(order.size(), 0, "zero weight skips Poll children");

  order = source_char_poll_group_poll_order(-0.5f, polls);
  ok &= expect_size(order.size(), 3, "nonzero weight polls children");
  ok &= expect_string(order[0], "first.poll", "Poll preserves source order");
  ok &= expect_string(order[2], "third.poll", "Poll preserves final child");

  auto children = source_char_poll_group_list_children(polls);
  ok &= expect_size(children.size(), 3, "ListPollChildren count");
  ok &= expect_string(children[1], "second.poll",
                      "ListPollChildren preserves order");

  std::vector<SourceCharPollGroupChildDeps> child_deps = {
      {"src.a", "dst.a"}, {"src.b", "dst.b"}};
  SourceCharPollGroupPollDeps deps;
  source_char_poll_group_poll_deps(deps, child_deps, "", "");
  ok &= expect_size(deps.changed_by.size(), 2,
                    "PollDeps delegates child changed_by count");
  ok &= expect_size(deps.change.size(), 2,
                    "PollDeps delegates child change count");
  ok &= expect_string(deps.changed_by[0], "src.a",
                      "PollDeps child changed_by first");
  ok &= expect_string(deps.change[1], "dst.b", "PollDeps child change second");

  SourceCharPollGroupPollDeps override_deps;
  source_char_poll_group_poll_deps(override_deps, child_deps, "", "override");
  ok &= expect_size(override_deps.changed_by.size(), 1,
                    "PollDeps override changed_by count");
  ok &= expect_size(override_deps.change.size(), 1,
                    "PollDeps override change count");
  ok &= expect_string(override_deps.changed_by[0], "",
                      "PollDeps override keeps empty changed_by slot");
  ok &= expect_string(override_deps.change[0], "override",
                      "PollDeps override keeps change slot");

  return ok ? 0 : 1;
}
