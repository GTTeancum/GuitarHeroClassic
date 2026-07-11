#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

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

std::vector<std::string> finger_transforms(
    const ghogx::character::SourceCharIKFingersSetupRefs& refs) {
  std::vector<std::string> names;
  for (const auto& finger : refs.fingers) {
    names.push_back(finger.finger01);
    names.push_back(finger.finger02);
    names.push_back(finger.finger03);
    names.push_back(finger.fingertip);
  }
  return names;
}

}  // namespace

int main() {
  using ghogx::character::source_char_ik_fingers_defaults;
  using ghogx::character::source_char_ik_fingers_copy_plan;
  using ghogx::character::source_char_ik_fingers_load_plan;
  using ghogx::character::source_char_ik_fingers_load_revision_known;
  using ghogx::character::source_char_ik_fingers_release_finger_plan;
  using ghogx::character::source_char_ik_fingers_set_name_refs;
  using ghogx::character::source_char_ik_fingers_set_finger_plan;
  using ghogx::character::source_char_ik_fingers_setup_complete;

  bool ok = true;

  const auto defaults = source_char_ik_fingers_defaults();
  ok &= expect_size(defaults.finger_count, 5, "default finger count");
  ok &= expect_bool(defaults.reset_hand_dest, true, "default reset hand dest");
  ok &= expect_bool(defaults.reset_cur_hand_trans, true,
                    "default reset current hand");
  ok &= near(defaults.finger_curled_length, 0.85f,
             "default curled length");
  ok &= near(defaults.hand_keyboard_offset[0], 0.3f,
             "default keyboard offset x");
  ok &= near(defaults.hand_keyboard_offset[1], -6.0f,
             "default keyboard offset y");
  ok &= near(defaults.hand_keyboard_offset[2], 0.4f,
             "default keyboard offset z");
  ok &= near(defaults.hand_move_forward, 1.0f,
             "default hand move forward");
  ok &= near(defaults.hand_pinky_rotation, -0.06f,
             "default pinky rotation");
  ok &= near(defaults.hand_thumb_rotation, 0.23f,
             "default thumb rotation");
  ok &= near(defaults.hand_dest_offset, -0.4f,
             "default hand destination offset");
  ok &= expect_bool(defaults.is_right_hand, true, "default right hand");
  ok &= expect_bool(defaults.move_hand, false, "default move hand");
  ok &= expect_bool(defaults.is_setup, false, "default setup false");

  ok &= expect_bool(source_char_ik_fingers_load_revision_known(-1), false,
                    "revision -1 rejected");
  ok &= expect_bool(source_char_ik_fingers_load_revision_known(0), true,
                    "revision 0 accepted");
  ok &= expect_bool(source_char_ik_fingers_load_revision_known(5), true,
                    "revision 5 accepted");
  ok &= expect_bool(source_char_ik_fingers_load_revision_known(6), false,
                    "revision 6 rejected");

  const auto left = source_char_ik_fingers_set_name_refs(false);
  ok &= expect_bool(left.is_right_hand, false, "left refs side");
  ok &= expect_string(left.hand, "bone_L-hand.mesh", "left hand ref");
  ok &= expect_string(left.forearm, "bone_L-foreArm.mesh", "left forearm ref");
  ok &= expect_string(left.upperarm, "bone_L-upperArm.mesh",
                      "left upper arm ref");
  ok &= expect_string(left.fingers[0].finger01, "bone_L-thumb01.mesh",
                      "left thumb 01");
  ok &= expect_string(left.fingers[2].finger03,
                      "bone_L-middlefinger03.mesh",
                      "left middle finger 03");
  ok &= expect_string(left.fingers[3].fingertip,
                      "spot_L-ringfinger_tip.mesh",
                      "left ring fingertip");
  ok &= near(left.raw_matrix[0], -0.067f, "left raw matrix first");
  ok &= near(left.raw_matrix[8], -0.23199999f, "left raw matrix last");

  const auto right = source_char_ik_fingers_set_name_refs(true);
  ok &= expect_bool(right.is_right_hand, true, "right refs side");
  ok &= expect_string(right.hand, "bone_R-hand.mesh", "right hand ref");
  ok &= expect_string(right.fingers[4].finger02, "bone_R-pinky02.mesh",
                      "right pinky 02");
  ok &= expect_string(right.fingers[1].fingertip,
                      "spot_R-index_tip.mesh",
                      "right index fingertip");
  ok &= near(right.raw_matrix[0], -0.023f, "right raw matrix first");
  ok &= near(right.raw_matrix[8], 0.21799999f, "right raw matrix last");

  auto present = finger_transforms(left);
  ok &= expect_bool(source_char_ik_fingers_setup_complete(left, present), true,
                    "complete only checks finger refs");
  present.pop_back();
  ok &= expect_bool(source_char_ik_fingers_setup_complete(left, present), false,
                    "missing fingertip makes setup incomplete");

  const auto set_index = source_char_ik_fingers_set_finger_plan(1);
  ok &= expect_bool(set_index.known_finger, true, "set index known");
  ok &= expect_int(set_index.finger, 1, "set index finger");
  ok &= expect_bool(set_index.assign_primary_vector, true,
                    "set index primary vector");
  ok &= expect_bool(set_index.assign_secondary_vector, true,
                    "set index secondary vector");
  ok &= expect_bool(set_index.set_active, true, "set index active");
  ok &= expect_bool(set_index.mark_dirty, true, "set index dirty");
  ok &= expect_bool(set_index.multiply_finger01_by_current_hand, true,
                    "set index multiply");
  ok &= expect_int(set_index.blend_in_frames, 5, "set blend in");
  ok &= expect_int(set_index.finger_blend_in_frames, 5,
                   "set finger blend in");
  ok &= expect_int(set_index.finger_blend_out_frames, 0,
                   "set finger blend out");
  ok &= expect_bool(source_char_ik_fingers_set_finger_plan(5).known_finger,
                    false, "set rejects none finger");

  const auto release_ring = source_char_ik_fingers_release_finger_plan(3);
  ok &= expect_bool(release_ring.known_finger, true, "release ring known");
  ok &= expect_bool(release_ring.clear_active, true, "release clears active");
  ok &= expect_bool(release_ring.mark_dirty, true, "release marks dirty");
  ok &= expect_int(release_ring.finger_blend_out_frames, 0,
                   "release blend out");
  ok &= expect_int(release_ring.finger_blend_in_frames, 5,
                   "release blend in");
  ok &= expect_bool(source_char_ik_fingers_release_finger_plan(-1).known_finger,
                    false, "release rejects negative finger");

  const auto load_v1 = source_char_ik_fingers_load_plan(1);
  ok &= expect_bool(load_v1.known_revision, true, "load v1 known");
  ok &= expect_size(load_v1.read_order.size(), 2, "load v1 rows");
  ok &= expect_string(load_v1.read_order[0], "Hmx::Object",
                      "load v1 object row");
  ok &= expect_string(load_v1.read_order[1], "CharWeightable",
                      "load v1 weightable row");
  const auto load_v5 = source_char_ik_fingers_load_plan(5);
  ok &= expect_size(load_v5.read_order.size(), 10, "load v5 rows");
  ok &= expect_string(load_v5.read_order[2], "mIsRightHand",
                      "load v5 hand side row");
  ok &= expect_string(load_v5.read_order[4], "mKeyboardRefBone",
                      "load v5 keyboard ref row");
  ok &= expect_string(load_v5.read_order.back(), "mHandDestOffset",
                      "load v5 hand dest row");
  ok &= expect_bool(source_char_ik_fingers_load_plan(6).known_revision, false,
                    "load rejects revision 6");

  const auto copy = source_char_ik_fingers_copy_plan();
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_string(copy.copied_superclasses[1], "CharWeightable",
                      "copy weightable superclass");
  ok &= expect_string(copy.copied_members[0], "mIsRightHand",
                      "copy hand side first");
  ok &= expect_string(copy.copied_members.back(), "mHandDestOffset",
                      "copy hand destination last");

  return ok ? 0 : 1;
}
