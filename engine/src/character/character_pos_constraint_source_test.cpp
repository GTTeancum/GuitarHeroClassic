#include "character/char_mesh.h"

#include <cmath>
#include <iostream>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool vec_near(const std::array<float, 3>& got,
              const std::array<float, 3>& want,
              const char* label) {
  bool ok = true;
  ok &= near(got[0], want[0], label);
  ok &= near(got[1], want[1], label);
  ok &= near(got[2], want[2], label);
  return ok;
}

}  // namespace

int main() {
  using ghogx::character::source_char_pos_constraint_target_position;

  bool ok = true;

  ok &= vec_near(
      source_char_pos_constraint_target_position(
          {10.0f, 20.0f, 30.0f},
          {14.0f, 21.0f, 41.0f},
          {1.0f, -5.0f, 0.0f},
          {-1.0f, 5.0f, 1000.0f}),
      {14.0f, 21.0f, 41.0f},
      "old-revision x disabled and y/z in range");

  ok &= vec_near(
      source_char_pos_constraint_target_position(
          {10.0f, 20.0f, 30.0f},
          {30.0f, -10.0f, 40.0f},
          {-2.0f, -3.0f, 20.0f},
          {4.0f, 6.0f, 100.0f}),
      {14.0f, 17.0f, 50.0f},
      "clamps each enabled axis by target-source delta");

  ok &= vec_near(
      source_char_pos_constraint_target_position(
          {-5.0f, 0.0f, 5.0f},
          {-9.0f, 7.0f, 1.0f},
          {-10.0f, 1.0f, -2.0f},
          {-1.0f, 3.0f, 2.0f}),
      {-9.0f, 3.0f, 3.0f},
      "preserves in-range low axis and clamps high/low axes");

  return ok ? 0 : 1;
}
