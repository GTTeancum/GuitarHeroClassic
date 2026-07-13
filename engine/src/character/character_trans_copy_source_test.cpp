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

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool xfm_near(const ghogx::milo_scene::Xfm& got,
              const ghogx::milo_scene::Xfm& want,
              const char* label) {
  bool ok = true;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      ok &= near(got.rot[r][c], want.rot[r][c], label);
    }
    ok &= near(got.pos[r], want.pos[r], label);
  }
  return ok;
}

ghogx::milo_scene::Xfm make_source_xfm() {
  ghogx::milo_scene::Xfm xfm;
  float value = 0.25f;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      xfm.rot[r][c] = value;
      value += 0.25f;
    }
    xfm.pos[r] = 10.0f + static_cast<float>(r);
  }
  return xfm;
}

ghogx::milo_scene::Xfm make_dest_xfm() {
  ghogx::milo_scene::Xfm xfm;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      xfm.rot[r][c] = -1.0f - static_cast<float>(r * 3 + c);
    }
    xfm.pos[r] = -10.0f - static_cast<float>(r);
  }
  return xfm;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharTransCopyPollDeps;
  using ghogx::character::source_char_trans_copy_copy_plan;
  using ghogx::character::source_char_trans_copy_handler_plan;
  using ghogx::character::source_char_trans_copy_load_plan;
  using ghogx::character::source_char_trans_copy_poll;
  using ghogx::character::source_char_trans_copy_poll_deps;
  using ghogx::character::source_char_trans_copy_prop_sync_plan;
  using ghogx::character::source_char_trans_copy_save_plan;

  bool ok = true;

  const auto load_bad = source_char_trans_copy_load_plan(2);
  ok &= expect_bool(load_bad.known_revision, false,
                    "load rejects high revision");
  ok &= expect_size(load_bad.read_order.size(), 0,
                    "bad load has no reads");
  const auto load = source_char_trans_copy_load_plan(1);
  ok &= expect_bool(load.known_revision, true,
                    "load accepts source revision");
  ok &= expect_size(load.read_order.size(), 3, "load read count");
  ok &= expect_string(load.read_order[0], "Hmx::Object",
                      "load object first");
  ok &= expect_string(load.read_order[1], "mSrc", "load source ref");
  ok &= expect_string(load.read_order[2], "mDest", "load dest ref");

  const auto save = source_char_trans_copy_save_plan();
  ok &= expect_int(save.save_id, 0x2D, "save id");

  const auto copy_plan = source_char_trans_copy_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 1,
                    "copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 2,
                    "copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mSrc",
                      "copy source ref");
  ok &= expect_string(copy_plan.copied_members[1], "mDest",
                      "copy dest ref");

  const auto handler_plan = source_char_trans_copy_handler_plan();
  ok &= expect_size(handler_plan.superclasses.size(), 2,
                    "handler superclass count");
  ok &= expect_string(handler_plan.superclasses[0], "RndPollable",
                      "handler pollable superclass");
  ok &= expect_string(handler_plan.superclasses[1], "Hmx::Object",
                      "handler object superclass");
  ok &= expect_int(handler_plan.check, 0x4C, "handler check");

  const auto prop_sync = source_char_trans_copy_prop_sync_plan();
  ok &= expect_size(prop_sync.properties.size(), 2,
                    "prop sync count");
  ok &= expect_string(prop_sync.properties[0], "src",
                      "prop sync source");
  ok &= expect_string(prop_sync.properties[1], "dest",
                      "prop sync dest");

  const auto src = make_source_xfm();
  const auto original_dest = make_dest_xfm();
  auto dest = original_dest;

  ok &= expect_bool(source_char_trans_copy_poll(nullptr, &dest), false,
                    "missing source skips poll");
  ok &= xfm_near(dest, original_dest, "missing source preserves dest");
  ok &= expect_bool(source_char_trans_copy_poll(&src, nullptr), false,
                    "missing dest skips poll");

  ok &= expect_bool(source_char_trans_copy_poll(&src, &dest), true,
                    "valid refs copy local xfm");
  ok &= xfm_near(dest, src, "dest local equals source local");

  SourceCharTransCopyPollDeps deps;
  source_char_trans_copy_poll_deps(deps, "source.trans", "dest.trans");
  ok &= expect_size(deps.change.size(), 1, "PollDeps change count");
  ok &= expect_size(deps.changed_by.size(), 1, "PollDeps changed_by count");
  ok &= expect_string(deps.change[0], "dest.trans", "PollDeps change dest");
  ok &= expect_string(deps.changed_by[0], "source.trans",
                      "PollDeps changed_by src");

  source_char_trans_copy_poll_deps(deps, "", "");
  ok &= expect_size(deps.change.size(), 2,
                    "PollDeps appends empty dest pointer slot");
  ok &= expect_size(deps.changed_by.size(), 2,
                    "PollDeps appends empty src pointer slot");
  ok &= expect_string(deps.change[1], "", "PollDeps keeps empty dest");
  ok &= expect_string(deps.changed_by[1], "", "PollDeps keeps empty src");

  return ok ? 0 : 1;
}
