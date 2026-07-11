#include "character/char_clip.h"

#include <cmath>
#include <cstdio>

namespace {

bool approx(float a, float b) {
  return std::fabs(a - b) < 1.0e-5f;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%d want=%d\n", label, got ? 1 : 0,
              want ? 1 : 0);
  return false;
}

bool expect_float(float got, float want, const char* label) {
  if (approx(got, want)) return true;
  std::printf("FAIL %s got=%.8f want=%.8f\n", label, got, want);
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharIKHandMeasure;
  using ghogx::character::source_char_ik_hand_elbow_cosine;
  using ghogx::character::source_char_ik_hand_measure_lengths;
  using ghogx::character::source_char_ik_hand_update_measure_lengths;

  bool ok = true;

  const SourceCharIKHandMeasure missing =
      source_char_ik_hand_measure_lengths(false, 4.0f, 3.0f);
  ok &= expect_bool(missing.has_elbow_chain, false, "missing chain flag");
  ok &= expect_float(missing.inv_2ab, 0.0f, "missing chain inv_2ab");
  float cosine = 12.0f;
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(missing, 25.0f, cosine),
                    false, "missing chain cosine rejected");
  ok &= expect_float(cosine, 12.0f, "missing chain leaves cosine");

  const SourceCharIKHandMeasure measure =
      source_char_ik_hand_measure_lengths(true, 4.0f, 3.0f);
  ok &= expect_bool(measure.has_elbow_chain, true, "measure chain flag");
  ok &= expect_float(measure.inv_2ab, 1.0f / 24.0f, "measure inv_2ab");
  ok &= expect_float(measure.a2_plus_b2, 25.0f, "measure a2 plus b2");
  ok &= expect_float(measure.aa_plus_bb, 7.0f, "measure reach sum");

  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 25.0f, cosine),
                    true, "cosine accepted");
  ok &= expect_float(cosine, 0.0f, "right-angle cosine");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 49.0f, cosine),
                    true, "max reach cosine accepted");
  ok &= expect_float(cosine, 1.0f, "max reach cosine");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 1.0f, cosine),
                    true, "folded cosine accepted");
  ok &= expect_float(cosine, -1.0f, "folded cosine");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 1000.0f, cosine),
                    true, "source high clamp accepted");
  ok &= expect_float(cosine, 1.0f, "source high clamp");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, -1000.0f, cosine),
                    true, "source low clamp accepted");
  ok &= expect_float(cosine, -1.0f, "source low clamp");

  bool hand_changed = true;
  ok &= expect_bool(source_char_ik_hand_update_measure_lengths(
                        false, hand_changed),
                    true, "initial hand change measures");
  ok &= expect_bool(hand_changed, false, "initial hand change clears");
  ok &= expect_bool(source_char_ik_hand_update_measure_lengths(
                        false, hand_changed),
                    false, "non-scalable stable skips measure");
  ok &= expect_bool(hand_changed, false, "stable hand remains clear");
  ok &= expect_bool(source_char_ik_hand_update_measure_lengths(
                        true, hand_changed),
                    true, "scalable hand measures every poll");
  ok &= expect_bool(hand_changed, false, "scalable measure leaves clear");

  std::printf("character_ik_hand_source_test %s\n", ok ? "OK" : "FAIL");
  return ok ? 0 : 1;
}
