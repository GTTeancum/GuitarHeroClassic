#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

#ifndef GHOGX_IHATECOMPVIR_EXTRA_DIR
#define GHOGX_IHATECOMPVIR_EXTRA_DIR "."
#endif

namespace {

struct SourceCoverage {
  const char* source_file;
  const char* owner_target;
  const char* status;
};

constexpr SourceCoverage kCoverage[] = {
    {"Char.cpp", "ghogx_character_character_source_test",
     "ported-visible-source"},
    {"Character.cpp", "ghogx_character_character_source_test",
     "ported-visible-source"},
    {"CharacterTest.cpp", "ghogx_character_character_test_source_test",
     "diagnostic-only"},
    {"CharBlendBone.cpp", "ghogx_character_blend_bone_source_test",
     "fenced-runtime-gap"},
    {"CharBone.cpp", "ghogx_character_char_bones_source_test",
     "ported-visible-source"},
    {"CharBoneDir.cpp", "ghogx_character_char_bones_source_test",
     "fenced-runtime-gap"},
    {"CharBoneOffset.cpp", "ghogx_character_bone_offset_source_test",
     "ported-visible-source"},
    {"CharBones.cpp", "ghogx_character_char_bones_source_test",
     "fenced-runtime-gap"},
    {"CharBonesBlender.cpp", "ghogx_character_char_bones_source_test",
     "fenced-runtime-gap"},
    {"CharBonesMeshes.cpp", "ghogx_character_char_bones_source_test",
     "fenced-runtime-gap"},
    {"CharBonesSamples.cpp", "ghogx_character_char_bones_source_test",
     "fenced-runtime-gap"},
    {"CharBoneTwist.cpp", "ghogx_character_bone_twist_source_test",
     "ported-visible-source"},
    {"CharClip.cpp", "ghogx_character_clip_driver_flags_test",
     "fenced-runtime-gap"},
    {"CharClipDisplay.cpp", "ghogx_character_clip_display_source_test",
     "diagnostic-only"},
    {"CharClipDriver.cpp", "ghogx_character_clip_driver_flags_test",
     "fenced-runtime-gap"},
    {"CharClipGroup.cpp", "ghogx_character_clip_set_source_test",
     "fenced-runtime-gap"},
    {"CharClipSet.cpp", "ghogx_character_clip_set_source_test",
     "diagnostic-only"},
    {"CharCollide.cpp", "ghogx_character_char_collide_source_test",
     "ported-visible-source"},
    {"CharCuff.cpp", "ghogx_character_cuff_source_test",
     "fenced-runtime-gap"},
    {"CharDriver.cpp", "ghogx_character_clip_driver_flags_test",
     "fenced-runtime-gap"},
    {"CharDriverMidi.cpp", "ghogx_character_clip_driver_flags_test",
     "fenced-runtime-gap"},
    {"CharEyeDartRuleset.cpp",
     "ghogx_character_eye_dart_ruleset_source_test", "diagnostic-only"},
    {"CharEyes.cpp", "ghogx_character_eyes_source_test",
     "fenced-runtime-gap"},
    {"CharFaceServo.cpp", "ghogx_character_face_servo_source_test",
     "fenced-runtime-gap"},
    {"CharForeTwist.cpp", "ghogx_character_fore_upper_twist_source_test",
     "ported-visible-source"},
    {"CharGuitarString.cpp", "ghogx_character_guitar_string_source_test",
     "ported-visible-source"},
    {"CharHair.cpp", "ghogx_character_char_hair_source_test",
     "fenced-runtime-gap"},
    {"CharIKFingers.cpp", "ghogx_character_ik_fingers_source_test",
     "fenced-runtime-gap"},
    {"CharIKFoot.cpp", "ghogx_character_ik_foot_source_test",
     "fenced-runtime-gap"},
    {"CharIKHand.cpp", "ghogx_character_ik_hand_source_test",
     "ported-visible-source"},
    {"CharIKHead.cpp", "ghogx_character_ik_head_source_test",
     "fenced-runtime-gap"},
    {"CharIKMidi.cpp", "ghogx_character_ik_midi_source_test",
     "diagnostic-only"},
    {"CharIKRod.cpp", "ghogx_character_ik_rod_source_test",
     "fenced-runtime-gap"},
    {"CharIKScale.cpp", "ghogx_character_ik_scale_source_test",
     "fenced-runtime-gap"},
    {"CharIKSliderMidi.cpp", "ghogx_character_ik_slider_midi_source_test",
     "fenced-runtime-gap"},
    {"CharInterest.cpp", "ghogx_character_interest_source_test",
     "fenced-runtime-gap"},
    {"CharLipSync.cpp", "ghogx_character_lip_sync_source_test",
     "diagnostic-only"},
    {"CharLipSyncDriver.cpp", "ghogx_character_lip_sync_source_test",
     "fenced-runtime-gap"},
    {"CharLookAt.cpp", "ghogx_character_lookat_source_test",
     "fenced-runtime-gap"},
    {"CharMeshCacheMgr.cpp", "ghogx_character_mesh_cache_source_test",
     "fenced-runtime-gap"},
    {"CharMeshHide.cpp", "ghogx_character_mesh_hide_source_test",
     "ported-visible-source"},
    {"CharMirror.cpp", "ghogx_character_mirror_source_test",
     "fenced-runtime-gap"},
    {"CharNeckTwist.cpp", "ghogx_character_neck_twist_source_test",
     "fenced-runtime-gap"},
    {"CharPollGroup.cpp", "ghogx_character_poll_group_source_test",
     "ported-visible-source"},
    {"CharPosConstraint.cpp", "ghogx_character_pos_constraint_source_test",
     "ported-visible-source"},
    {"CharServoBone.cpp", "ghogx_character_char_bones_source_test",
     "fenced-runtime-gap"},
    {"CharSleeve.cpp", "ghogx_character_sleeve_source_test",
     "ported-visible-source"},
    {"CharTaskMgr.cpp", "ghogx_character_clip_display_source_test",
     "diagnostic-only"},
    {"CharTransCopy.cpp", "ghogx_character_trans_copy_source_test",
     "ported-visible-source"},
    {"CharTransDraw.cpp", "ghogx_character_trans_draw_source_test",
     "diagnostic-only"},
    {"CharUpperTwist.cpp", "ghogx_character_fore_upper_twist_source_test",
     "ported-visible-source"},
    {"CharUtl.cpp", "ghogx_character_char_utl_source_test",
     "ported-visible-source"},
    {"CharWeightable.cpp", "ghogx_character_weight_setter_source_test",
     "ported-visible-source"},
    {"CharWeightSetter.cpp", "ghogx_character_weight_setter_source_test",
     "fenced-runtime-gap"},
    {"ClipCollide.cpp", "ghogx_character_clip_editor_source_test",
     "diagnostic-only"},
    {"ClipCompressor.cpp", "ghogx_character_clip_editor_source_test",
     "absence-evidence"},
    {"ClipGraphGen.cpp", "ghogx_character_clip_editor_source_test",
     "diagnostic-only"},
    {"FileMerger.cpp", "ghogx_character_clip_editor_source_test",
     "diagnostic-only"},
    {"Waypoint.cpp", "ghogx_character_waypoint_source_test",
     "diagnostic-only"},
};

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool contains(const std::string& haystack, const std::string& needle,
              const std::string& label) {
  if (haystack.find(needle) != std::string::npos) return true;
  std::cerr << "Missing ihatecompvir inventory contract: " << label << "\n";
  std::cerr << "Needle: " << needle << "\n";
  return false;
}

bool is_known_status(const std::string& status) {
  static const std::set<std::string> kKnown = {
      "ported-visible-source", "fenced-runtime-gap", "diagnostic-only",
      "absence-evidence"};
  return kKnown.count(status) != 0;
}

}  // namespace

int main() {
  const std::filesystem::path char_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::filesystem::path extra_dir = GHOGX_IHATECOMPVIR_EXTRA_DIR;
  const std::filesystem::path source_dir =
      extra_dir / "rb3-latest" / "src" / "system" / "char";
  const std::filesystem::path doc_path =
      char_dir / "IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md";
  const std::filesystem::path cmake_path = char_dir / "CMakeLists.txt";

  const std::string doc = read_file(doc_path);
  const std::string cmake = read_file(cmake_path);

  bool ok = true;
  ok &= contains(doc, "## ihatecompvir Character Implementation Inventory",
                 "documentation has implementation inventory section");
  ok &= contains(doc, "fenced-runtime-gap",
                 "documentation names fenced runtime gap status");
  ok &= contains(doc, "absence-evidence",
                 "documentation names source absence evidence status");

  std::set<std::string> mapped_sources;
  std::set<std::string> mapped_targets;
  for (const SourceCoverage& row : kCoverage) {
    const std::string source(row.source_file);
    const std::string target(row.owner_target);
    const std::string status(row.status);

    if (!mapped_sources.insert(source).second) {
      std::cerr << "Duplicate ihatecompvir source inventory row: " << source
                << "\n";
      ok = false;
    }
    mapped_targets.insert(target);

    if (!is_known_status(status)) {
      std::cerr << "Unknown ihatecompvir source status for " << source << ": "
                << status << "\n";
      ok = false;
    }
    if (!std::filesystem::is_regular_file(source_dir / source)) {
      std::cerr << "Missing ihatecompvir source file: "
                << (source_dir / source).string() << "\n";
      ok = false;
    }
    ok &= contains(cmake, "add_executable(" + target,
                   "owner target exists for " + source);
    ok &= contains(doc, "`" + source + "`",
                   "documentation lists source " + source);
    ok &= contains(doc, "`" + target + "`",
                   "documentation lists owner target for " + source);
    ok &= contains(doc, status, "documentation lists status for " + source);
  }

  std::set<std::string> actual_sources;
  for (const auto& entry : std::filesystem::directory_iterator(source_dir)) {
    if (!entry.is_regular_file()) continue;
    if (entry.path().extension() != ".cpp") continue;
    actual_sources.insert(entry.path().filename().string());
  }

  for (const std::string& actual : actual_sources) {
    if (mapped_sources.count(actual) == 0) {
      std::cerr << "Unmapped ihatecompvir character source file: " << actual
                << "\n";
      ok = false;
    }
  }
  for (const std::string& mapped : mapped_sources) {
    if (actual_sources.count(mapped) == 0) {
      std::cerr << "Inventory maps missing ihatecompvir source file: " << mapped
                << "\n";
      ok = false;
    }
  }

  ok &= contains(doc, "`CharBones.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap`",
                 "CharBones runtime gap remains explicit");
  ok &= contains(doc, "`CharHair.cpp` | `ghogx_character_char_hair_source_test` | `fenced-runtime-gap`",
                 "CharHair runtime gap remains explicit");
  ok &= contains(doc, "`ClipCompressor.cpp` | `ghogx_character_clip_editor_source_test` | `absence-evidence`",
                 "ClipCompressor absence remains explicit");

  if (!ok) return 1;

  std::cout << "ihatecompvir character source inventory covers "
            << mapped_sources.size() << " source files across "
            << mapped_targets.size() << " owner targets\n";
  return 0;
}
