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
  using ghogx::character::source_char_trans_copy_poll;
  using ghogx::character::source_char_trans_copy_poll_deps;

  bool ok = true;

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
