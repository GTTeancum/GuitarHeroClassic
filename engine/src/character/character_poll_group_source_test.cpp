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

}  // namespace

int main() {
  using ghogx::character::SourceCharPollGroupChildDeps;
  using ghogx::character::SourceCharPollGroupPollDeps;
  using ghogx::character::source_char_poll_group_copy_plan;
  using ghogx::character::source_char_poll_group_enter_order;
  using ghogx::character::source_char_poll_group_exit_order;
  using ghogx::character::source_char_poll_group_handler_plan;
  using ghogx::character::source_char_poll_group_list_children;
  using ghogx::character::source_char_poll_group_load_plan;
  using ghogx::character::source_char_poll_group_poll_deps;
  using ghogx::character::source_char_poll_group_poll_order;
  using ghogx::character::source_char_poll_group_prop_sync_plan;
  using ghogx::character::source_char_poll_group_save_plan;
  using ghogx::character::source_char_poll_group_sort_plan;

  bool ok = true;
  const auto bad_load = source_char_poll_group_load_plan(4);
  ok &= expect_size(bad_load.read_order.size(), 0,
                    "bad load has no reads");
  const auto load_v1 = source_char_poll_group_load_plan(1);
  ok &= expect_size(load_v1.read_order.size(), 2, "load v1 read count");
  ok &= expect_string(load_v1.read_order[0], "Hmx::Object",
                      "load v1 object");
  ok &= expect_string(load_v1.read_order[1], "mPolls",
                      "load v1 polls");
  const auto load_v3 = source_char_poll_group_load_plan(3);
  ok &= expect_size(load_v3.read_order.size(), 5, "load v3 read count");
  ok &= expect_string(load_v3.read_order[1], "CharWeightable",
                      "load v3 weightable");
  ok &= expect_string(load_v3.read_order[3], "mChangedBy",
                      "load v3 changed_by");
  ok &= expect_string(load_v3.read_order[4], "mChanges",
                      "load v3 changes");

  const auto save = source_char_poll_group_save_plan();
  ok &= expect_int(save.save_id, 0x58, "save id");

  const auto copy_plan = source_char_poll_group_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 2,
                    "copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "CharWeightable",
                      "copy weightable superclass");
  ok &= expect_size(copy_plan.copy_from_max_steps.size(), 2,
                    "copy-from-Max step count");
  ok &= expect_string(copy_plan.copy_from_max_steps[1],
                      "append missing poll refs only",
                      "copy-from-Max append gate");
  ok &= expect_string(copy_plan.copied_members[2], "mChanges",
                      "copy normal changes member");

  const auto handler_plan = source_char_poll_group_handler_plan();
  ok &= expect_string(handler_plan.action_handlers[0], "sort_polls",
                      "handler sort_polls");
  ok &= expect_string(handler_plan.superclasses[0], "Hmx::Object",
                      "handler superclass");
  ok &= expect_int(handler_plan.check, 0xA2, "handler check");

  const auto prop_sync = source_char_poll_group_prop_sync_plan();
  ok &= expect_string(prop_sync.properties[0], "polls",
                      "prop polls");
  ok &= expect_string(prop_sync.properties[2], "changes",
                      "prop changes");
  ok &= expect_string(prop_sync.superclasses[0], "CharWeightable",
                      "prop superclass");

  const auto sort_plan = source_char_poll_group_sort_plan();
  ok &= expect_size(sort_plan.steps.size(), 5, "sort step count");
  ok &= expect_string(sort_plan.steps[2], "CharPollableSorter::Sort",
                      "sort uses source sorter");
  ok &= expect_string(sort_plan.steps[4], "push sorted refs as CharPollable",
                      "sort repopulates polls");

  const std::vector<std::string> polls = {
      "first.poll", "second.poll", "third.poll"};

  auto order = source_char_poll_group_poll_order(0.0f, polls);
  ok &= expect_size(order.size(), 0, "zero weight skips Poll children");

  order = source_char_poll_group_poll_order(-0.5f, polls);
  ok &= expect_size(order.size(), 3, "nonzero weight polls children");
  ok &= expect_string(order[0], "first.poll", "Poll preserves source order");
  ok &= expect_string(order[2], "third.poll", "Poll preserves final child");

  auto enter_order = source_char_poll_group_enter_order(polls);
  ok &= expect_size(enter_order.size(), 3, "Enter visits all children");
  ok &= expect_string(enter_order[0], "first.poll",
                      "Enter preserves source order");
  ok &= expect_string(enter_order[2], "third.poll",
                      "Enter preserves final child");

  auto exit_order = source_char_poll_group_exit_order(polls);
  ok &= expect_size(exit_order.size(), 3, "Exit visits all children");
  ok &= expect_string(exit_order[0], "first.poll",
                      "Exit preserves source order");
  ok &= expect_string(exit_order[2], "third.poll",
                      "Exit preserves final child");

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
