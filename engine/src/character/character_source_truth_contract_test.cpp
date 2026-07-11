#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

#ifndef GHOGX_MILO_SCENE_SOURCE_DIR
#define GHOGX_MILO_SCENE_SOURCE_DIR "."
#endif

#ifndef GHOGX_IHATECOMPVIR_SOURCE_DIR
#define GHOGX_IHATECOMPVIR_SOURCE_DIR "."
#endif

#ifndef GHOGX_IHATECOMPVIR_EXTRA_DIR
#define GHOGX_IHATECOMPVIR_EXTRA_DIR "."
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string compact(std::string s) {
  s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
          }),
          s.end());
  return s;
}

bool contains(const std::string& haystack, const std::string& needle,
              const std::string& label) {
  if (haystack.find(needle) != std::string::npos) return true;
  std::cerr << "Missing source-truth contract: " << label << "\n";
  std::cerr << "Needle: " << needle << "\n";
  return false;
}

bool missing(const std::string& haystack, const std::string& needle,
             const std::string& label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Forbidden source-truth contract match: " << label << "\n";
  std::cerr << "Needle: " << needle << "\n";
  return false;
}

}  // namespace

int run_contract() {
  const std::filesystem::path char_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::filesystem::path scene_dir = GHOGX_MILO_SCENE_SOURCE_DIR;
  const std::filesystem::path source_dir = GHOGX_IHATECOMPVIR_SOURCE_DIR;
  const std::filesystem::path extra_dir = GHOGX_IHATECOMPVIR_EXTRA_DIR;
  const std::filesystem::path engine_dir = char_dir.parent_path().parent_path();

  const std::string char_mesh = compact(read_file(char_dir / "char_mesh.cpp"));
  const std::string char_mesh_h = compact(read_file(char_dir / "char_mesh.h"));
  const std::string char_clip = compact(read_file(char_dir / "char_clip.cpp"));
  const std::string char_clip_h = compact(read_file(char_dir / "char_clip.h"));
  const std::string char_clip_audit =
      compact(read_file(char_dir / "char_clip_audit.cpp"));
  const std::string clip_driver_flags_test =
      compact(read_file(char_dir / "character_clip_driver_flags_test.cpp"));
  const std::string clip_set_source_test =
      compact(read_file(char_dir / "character_clip_set_source_test.cpp"));
  const std::string clip_display_source_test =
      compact(read_file(char_dir / "character_clip_display_source_test.cpp"));
  const std::string clip_editor_source_test =
      compact(read_file(char_dir / "character_clip_editor_source_test.cpp"));
  const std::string char_bones_source_test =
      compact(read_file(char_dir / "character_char_bones_source_test.cpp"));
  const std::string char_utl_source_test =
      compact(read_file(char_dir / "character_char_utl_source_test.cpp"));
  const std::string ik_rod_source_test =
      compact(read_file(char_dir / "character_ik_rod_source_test.cpp"));
  const std::string ik_hand_source_test =
      compact(read_file(char_dir / "character_ik_hand_source_test.cpp"));
  const std::string ik_foot_source_test =
      compact(read_file(char_dir / "character_ik_foot_source_test.cpp"));
  const std::string ik_head_source_test =
      compact(read_file(char_dir / "character_ik_head_source_test.cpp"));
  const std::string ik_slider_midi_source_test = compact(
      read_file(char_dir / "character_ik_slider_midi_source_test.cpp"));
  const std::string ik_midi_source_test =
      compact(read_file(char_dir / "character_ik_midi_source_test.cpp"));
  const std::string bone_offset_source_test =
      compact(read_file(char_dir / "character_bone_offset_source_test.cpp"));
  const std::string bone_twist_source_test =
      compact(read_file(char_dir / "character_bone_twist_source_test.cpp"));
  const std::string fore_upper_twist_source_test = compact(
      read_file(char_dir / "character_fore_upper_twist_source_test.cpp"));
  const std::string lookat_source_test =
      compact(read_file(char_dir / "character_lookat_source_test.cpp"));
  const std::string weight_setter_source_test =
      compact(read_file(char_dir / "character_weight_setter_source_test.cpp"));
  const std::string mirror_source_test =
      compact(read_file(char_dir / "character_mirror_source_test.cpp"));
  const std::string char_hair_source_test =
      compact(read_file(char_dir / "character_char_hair_source_test.cpp"));
  const std::string char_collide_source_test =
      compact(read_file(char_dir / "character_char_collide_source_test.cpp"));
  const std::string face_servo_source_test =
      compact(read_file(char_dir / "character_face_servo_source_test.cpp"));
  const std::string lip_sync_source_test =
      compact(read_file(char_dir / "character_lip_sync_source_test.cpp"));
  const std::string mesh_hide_source_test =
      compact(read_file(char_dir / "character_mesh_hide_source_test.cpp"));
  const std::string trans_copy_source_test =
      compact(read_file(char_dir / "character_trans_copy_source_test.cpp"));
  const std::string poll_group_source_test =
      compact(read_file(char_dir / "character_poll_group_source_test.cpp"));
  const std::string ik_scale_source_test =
      compact(read_file(char_dir / "character_ik_scale_source_test.cpp"));
  const std::string trans_draw_source_test =
      compact(read_file(char_dir / "character_trans_draw_source_test.cpp"));
  const std::string cuff_source_test =
      compact(read_file(char_dir / "character_cuff_source_test.cpp"));
  const std::string blend_bone_source_test =
      compact(read_file(char_dir / "character_blend_bone_source_test.cpp"));
  const std::string sleeve_source_test =
      compact(read_file(char_dir / "character_sleeve_source_test.cpp"));
  const std::string mesh_cache_source_test =
      compact(read_file(char_dir / "character_mesh_cache_source_test.cpp"));
  const std::string pos_constraint_source_test = compact(
      read_file(char_dir / "character_pos_constraint_source_test.cpp"));
  const std::string waypoint_source_test =
      compact(read_file(char_dir / "character_waypoint_source_test.cpp"));
  const std::string guitar_string_source_test = compact(
      read_file(char_dir / "character_guitar_string_source_test.cpp"));
  const std::string eyes_source_test =
      compact(read_file(char_dir / "character_eyes_source_test.cpp"));
  const std::string eye_dart_ruleset_source_test = compact(
      read_file(char_dir / "character_eye_dart_ruleset_source_test.cpp"));
  const std::string interest_source_test =
      compact(read_file(char_dir / "character_interest_source_test.cpp"));
  const std::string neck_twist_source_test = compact(
      read_file(char_dir / "character_neck_twist_source_test.cpp"));
  const std::string ik_fingers_source_test = compact(
      read_file(char_dir / "character_ik_fingers_source_test.cpp"));
  const std::string character_source_test = compact(
      read_file(char_dir / "character_character_source_test.cpp"));
  const std::string character_test_source_test = compact(
      read_file(char_dir / "character_character_test_source_test.cpp"));
  const std::string mesh_decode_test =
      compact(read_file(char_dir / "character_mesh_decode_test.cpp"));
  const std::string bind_audit =
      compact(read_file(char_dir / "char_bind_audit.cpp"));
  const std::string renderer = compact(read_file(char_dir / "char_renderer.cpp"));
  const std::string cmake = compact(read_file(char_dir / "CMakeLists.txt"));
  const std::string scene = compact(read_file(scene_dir / "milo_scene.cpp"));
  const std::string doc =
      read_file(char_dir / "IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md");
  const std::string format_notes =
      read_file(char_dir / "CHARACTER_FORMAT_NOTES.md");
  const std::string format_notes_compact = compact(format_notes);
  const std::string source_readme = read_file(source_dir / "README.md");

  const std::string object_cs = compact(read_file(
      source_dir / "MiloEditor/MiloLib/Assets/Object.cs"));
  const std::string trans_cs = compact(read_file(
      source_dir / "MiloEditor/MiloLib/Assets/Rnd/RndTrans.cs"));
  const std::string drawable_cs = compact(read_file(
      source_dir / "MiloEditor/MiloLib/Assets/Rnd/RndDrawable.cs"));
  const std::string mat_cs = compact(read_file(
      source_dir / "MiloEditor/MiloLib/Assets/Rnd/RndMat.cs"));
  const std::string group_cs = compact(read_file(
      source_dir / "MiloEditor/MiloLib/Assets/Rnd/RndGroup.cs"));
  const std::string mesh_cs = compact(read_file(
      source_dir / "MiloEditor/MiloLib/Assets/Rnd/RndMesh.cs"));
  const std::string rb3_mesh_cpp = compact(read_file(
      source_dir / "rb3/src/system/rndobj/Mesh.cpp"));
  const std::string rb3_mat_cpp = compact(read_file(
      source_dir / "rb3/src/system/rndobj/Mat.cpp"));
  const std::string rb3_mat_h = compact(read_file(
      source_dir / "rb3/src/system/rndobj/Mat.h"));
  const std::string rb3_trans_cpp = compact(read_file(
      source_dir / "rb3/src/system/rndobj/Trans.cpp"));
  const std::string rb3_trans_h = compact(read_file(
      source_dir / "rb3/src/system/rndobj/Trans.h"));
  const std::string gltf_program_cs = compact(read_file(
      source_dir / "glTFMilo/Source/glTFMilo/Program.cs"));
  const std::string gltf_node_processor_cs = compact(read_file(
      source_dir / "glTFMilo/Source/glTFMilo/Core/NodeProcessor.cs"));
  const std::string rb3_char_hair_cpp = compact(read_file(
      source_dir / "rb3/src/system/char/CharHair.cpp"));
  const std::string rb3_char_lookat_cpp = compact(read_file(
      source_dir / "rb3/src/system/char/CharLookAt.cpp"));
  const std::string rb3_char_eyes_cpp = compact(read_file(
      source_dir / "rb3/src/system/char/CharEyes.cpp"));
  const std::string rb3_char_ik_hand_cpp = compact(read_file(
      source_dir / "rb3/src/system/char/CharIKHand.cpp"));
  const std::string rb3_char_upper_twist_cpp = compact(read_file(
      source_dir / "rb3/src/system/char/CharUpperTwist.cpp"));
  const std::string rb3_char_fore_twist_cpp = compact(read_file(
      source_dir / "rb3/src/system/char/CharForeTwist.cpp"));
  const std::filesystem::path rb3_latest_char_dir =
      extra_dir / "rb3-latest/src/system/char";
  const std::filesystem::path rb3_latest_rndobj_dir =
      extra_dir / "rb3-latest/src/system/rndobj";
  const std::filesystem::path rb3_latest_obj_dir =
      extra_dir / "rb3-latest/src/system/obj";
  const std::filesystem::path rb3_latest_utl_dir =
      extra_dir / "rb3-latest/src/system/utl";
  const std::filesystem::path rb2_dump_char_dir =
      extra_dir / "rb3-retail-old/doc/rb2_dump/rockband2/system/src/char";
  const std::string rb3_latest_char_hair_cpp = compact(read_file(
      rb3_latest_char_dir / "CharHair.cpp"));
  const std::string rb3_latest_char_hair_h = compact(read_file(
      rb3_latest_char_dir / "CharHair.h"));
  const std::string rb3_latest_char_lookat_h = compact(read_file(
      rb3_latest_char_dir / "CharLookAt.h"));
  const std::string rb3_latest_char_blend_bone_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBlendBone.cpp"));
  const std::string rb3_latest_char_blend_bone_h = compact(read_file(
      rb3_latest_char_dir / "CharBlendBone.h"));
  const std::string rb3_latest_char_sleeve_cpp = compact(read_file(
      rb3_latest_char_dir / "CharSleeve.cpp"));
  const std::string rb3_latest_char_sleeve_h = compact(read_file(
      rb3_latest_char_dir / "CharSleeve.h"));
  const std::string rb3_latest_char_collide_cpp = compact(read_file(
      rb3_latest_char_dir / "CharCollide.cpp"));
  const std::string rb3_latest_char_collide_h = compact(read_file(
      rb3_latest_char_dir / "CharCollide.h"));
  const std::string rb3_latest_char_cuff_cpp = compact(read_file(
      rb3_latest_char_dir / "CharCuff.cpp"));
  const std::string rb3_latest_char_cuff_h = compact(read_file(
      rb3_latest_char_dir / "CharCuff.h"));
  const std::string rb3_latest_char_ik_rod_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKRod.cpp"));
  const std::string rb3_latest_char_ik_rod_h = compact(read_file(
      rb3_latest_char_dir / "CharIKRod.h"));
  const std::string rb3_latest_char_ik_midi_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKMidi.cpp"));
  const std::string rb3_latest_char_ik_midi_h = compact(read_file(
      rb3_latest_char_dir / "CharIKMidi.h"));
  const std::string rb3_latest_char_ik_head_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKHead.cpp"));
  const std::string rb3_latest_char_ik_head_h = compact(read_file(
      rb3_latest_char_dir / "CharIKHead.h"));
  const std::string rb3_latest_char_ik_foot_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKFoot.cpp"));
  const std::string rb3_latest_char_ik_foot_h = compact(read_file(
      rb3_latest_char_dir / "CharIKFoot.h"));
  const std::string rb3_latest_char_ik_slider_midi_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKSliderMidi.cpp"));
  const std::string rb3_latest_char_ik_slider_midi_h = compact(read_file(
      rb3_latest_char_dir / "CharIKSliderMidi.h"));
  const std::string rb3_latest_char_ik_fingers_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKFingers.cpp"));
  const std::string rb3_latest_char_ik_fingers_h = compact(read_file(
      rb3_latest_char_dir / "CharIKFingers.h"));
  const std::string rb3_latest_char_neck_twist_cpp = compact(read_file(
      rb3_latest_char_dir / "CharNeckTwist.cpp"));
  const std::string rb3_latest_char_neck_twist_h = compact(read_file(
      rb3_latest_char_dir / "CharNeckTwist.h"));
  const std::string rb3_latest_char_ik_scale_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKScale.cpp"));
  const std::string rb3_latest_char_ik_scale_h = compact(read_file(
      rb3_latest_char_dir / "CharIKScale.h"));
  const std::string rb3_latest_char_servo_bone_cpp = compact(read_file(
      rb3_latest_char_dir / "CharServoBone.cpp"));
  const std::string rb3_latest_char_servo_bone_h = compact(read_file(
      rb3_latest_char_dir / "CharServoBone.h"));
  const std::string rb3_latest_char_face_servo_cpp = compact(read_file(
      rb3_latest_char_dir / "CharFaceServo.cpp"));
  const std::string rb3_latest_char_face_servo_h = compact(read_file(
      rb3_latest_char_dir / "CharFaceServo.h"));
  const std::string rb3_latest_char_lip_sync_cpp = compact(read_file(
      rb3_latest_char_dir / "CharLipSync.cpp"));
  const std::string rb3_latest_char_lip_sync_h = compact(read_file(
      rb3_latest_char_dir / "CharLipSync.h"));
  const std::string rb3_latest_char_lip_sync_driver_cpp = compact(read_file(
      rb3_latest_char_dir / "CharLipSyncDriver.cpp"));
  const std::string rb3_latest_char_lip_sync_driver_h = compact(read_file(
      rb3_latest_char_dir / "CharLipSyncDriver.h"));
  const std::string rb3_latest_char_mesh_hide_cpp = compact(read_file(
      rb3_latest_char_dir / "CharMeshHide.cpp"));
  const std::string rb3_latest_char_mesh_hide_h = compact(read_file(
      rb3_latest_char_dir / "CharMeshHide.h"));
  const std::string rb3_latest_char_mesh_cache_cpp = compact(read_file(
      rb3_latest_char_dir / "CharMeshCacheMgr.cpp"));
  const std::string rb3_latest_char_mesh_cache_h = compact(read_file(
      rb3_latest_char_dir / "CharMeshCacheMgr.h"));
  const std::string rb3_latest_char_trans_copy_cpp = compact(read_file(
      rb3_latest_char_dir / "CharTransCopy.cpp"));
  const std::string rb3_latest_char_trans_copy_h = compact(read_file(
      rb3_latest_char_dir / "CharTransCopy.h"));
  const std::string rb3_latest_char_trans_draw_cpp = compact(read_file(
      rb3_latest_char_dir / "CharTransDraw.cpp"));
  const std::string rb3_latest_char_trans_draw_h = compact(read_file(
      rb3_latest_char_dir / "CharTransDraw.h"));
  const std::string rb3_latest_char_weightable_cpp = compact(read_file(
      rb3_latest_char_dir / "CharWeightable.cpp"));
  const std::string rb3_latest_char_weightable_h = compact(read_file(
      rb3_latest_char_dir / "CharWeightable.h"));
  const std::string rb3_latest_char_mirror_cpp = compact(read_file(
      rb3_latest_char_dir / "CharMirror.cpp"));
  const std::string rb3_latest_char_mirror_h = compact(read_file(
      rb3_latest_char_dir / "CharMirror.h"));
  const std::string rb3_latest_char_driver_cpp = compact(read_file(
      rb3_latest_char_dir / "CharDriver.cpp"));
  const std::string rb3_latest_char_driver_h = compact(read_file(
      rb3_latest_char_dir / "CharDriver.h"));
  const std::string rb3_latest_char_driver_midi_cpp = compact(read_file(
      rb3_latest_char_dir / "CharDriverMidi.cpp"));
  const std::string rb3_latest_char_driver_midi_h = compact(read_file(
      rb3_latest_char_dir / "CharDriverMidi.h"));
  const std::string rb3_latest_char_clip_set_cpp = compact(read_file(
      rb3_latest_char_dir / "CharClipSet.cpp"));
  const std::string rb3_latest_char_clip_set_h = compact(read_file(
      rb3_latest_char_dir / "CharClipSet.h"));
  const std::string rb3_latest_char_clip_display_cpp = compact(read_file(
      rb3_latest_char_dir / "CharClipDisplay.cpp"));
  const std::string rb3_latest_char_clip_display_h = compact(read_file(
      rb3_latest_char_dir / "CharClipDisplay.h"));
  const std::string rb3_latest_char_task_mgr_cpp = compact(read_file(
      rb3_latest_char_dir / "CharTaskMgr.cpp"));
  const std::string rb3_latest_char_task_mgr_h = compact(read_file(
      rb3_latest_char_dir / "CharTaskMgr.h"));
  const std::string rb3_latest_clip_collide_cpp = compact(read_file(
      rb3_latest_char_dir / "ClipCollide.cpp"));
  const std::string rb3_latest_clip_collide_h = compact(read_file(
      rb3_latest_char_dir / "ClipCollide.h"));
  const std::string rb3_latest_clip_graph_gen_cpp = compact(read_file(
      rb3_latest_char_dir / "ClipGraphGen.cpp"));
  const std::string rb3_latest_clip_graph_gen_h = compact(read_file(
      rb3_latest_char_dir / "ClipGraphGen.h"));
  const std::string rb3_latest_clip_dist_map_h = compact(read_file(
      rb3_latest_char_dir / "ClipDistMap.h"));
  const std::string rb3_latest_clip_compressor_cpp = compact(read_file(
      rb3_latest_char_dir / "ClipCompressor.cpp"));
  const std::string rb3_latest_file_merger_cpp = compact(read_file(
      rb3_latest_char_dir / "FileMerger.cpp"));
  const std::string rb3_latest_file_merger_h = compact(read_file(
      rb3_latest_char_dir / "FileMerger.h"));
  const std::string rb3_latest_char_cpp = compact(read_file(
      rb3_latest_char_dir / "Char.cpp"));
  const std::string rb3_latest_char_h = compact(read_file(
      rb3_latest_char_dir / "Char.h"));
  const std::string rb3_latest_character_cpp = compact(read_file(
      rb3_latest_char_dir / "Character.cpp"));
  const std::string rb3_latest_character_h = compact(read_file(
      rb3_latest_char_dir / "Character.h"));
  const std::string rb3_latest_character_test_cpp = compact(read_file(
      rb3_latest_char_dir / "CharacterTest.cpp"));
  const std::string rb3_latest_character_test_h = compact(read_file(
      rb3_latest_char_dir / "CharacterTest.h"));
  const std::string rb3_latest_char_poll_group_cpp = compact(read_file(
      rb3_latest_char_dir / "CharPollGroup.cpp"));
  const std::string rb3_latest_char_poll_group_h = compact(read_file(
      rb3_latest_char_dir / "CharPollGroup.h"));
  const std::string rb3_latest_anim_filter_cpp = compact(read_file(
      rb3_latest_rndobj_dir / "AnimFilter.cpp"));
  const std::string rb3_latest_anim_filter_h = compact(read_file(
      rb3_latest_rndobj_dir / "AnimFilter.h"));
  const std::string rb3_latest_anim_cpp = compact(read_file(
      rb3_latest_rndobj_dir / "Anim.cpp"));
  const std::string rb3_latest_event_trigger_cpp = compact(read_file(
      rb3_latest_rndobj_dir / "EventTrigger.cpp"));
  const std::string rb3_latest_event_trigger_h = compact(read_file(
      rb3_latest_rndobj_dir / "EventTrigger.h"));
  const std::string rb3_latest_obj_vector_h = compact(read_file(
      rb3_latest_obj_dir / "ObjVector.h"));
  const std::string rb3_latest_obj_ptr_p_h = compact(read_file(
      rb3_latest_obj_dir / "ObjPtr_p.h"));
  const std::string rb3_latest_object_h = compact(read_file(
      rb3_latest_obj_dir / "Object.h"));
  const std::string rb3_latest_obj_dir_cpp = compact(read_file(
      rb3_latest_obj_dir / "Dir.cpp"));
  const std::string rb3_latest_bin_stream_h = compact(read_file(
      rb3_latest_utl_dir / "BinStream.h"));
  const std::string rb3_latest_bin_stream_cpp = compact(read_file(
      rb3_latest_utl_dir / "BinStream.cpp"));
  const std::string rb3_latest_tex_cpp = compact(read_file(
      rb3_latest_rndobj_dir / "Tex.cpp"));
  const std::string rb3_latest_tex_h = compact(read_file(
      rb3_latest_rndobj_dir / "Tex.h"));
  const std::string rb3_latest_bitmap_cpp = compact(read_file(
      rb3_latest_rndobj_dir / "Bitmap.cpp"));
  const std::string rb3_latest_rnd_dir_cpp = compact(read_file(
      rb3_latest_rndobj_dir / "Dir.cpp"));
  const std::string rb3_latest_chunk_stream_cpp = compact(read_file(
      rb3_latest_utl_dir / "ChunkStream.cpp"));
  const std::string rb3_latest_chunk_stream_h = compact(read_file(
      rb3_latest_utl_dir / "ChunkStream.h"));
  const std::string rb3_latest_file_path_h = compact(read_file(
      rb3_latest_utl_dir / "FilePath.h"));
  const std::string rb3_latest_char_weight_setter_cpp = compact(read_file(
      rb3_latest_char_dir / "CharWeightSetter.cpp"));
  const std::string rb3_latest_char_weight_setter_h = compact(read_file(
      rb3_latest_char_dir / "CharWeightSetter.h"));
  const std::string rb3_latest_char_pos_constraint_cpp = compact(read_file(
      rb3_latest_char_dir / "CharPosConstraint.cpp"));
  const std::string rb3_latest_char_pos_constraint_h = compact(read_file(
      rb3_latest_char_dir / "CharPosConstraint.h"));
  const std::string rb3_latest_waypoint_cpp = compact(read_file(
      rb3_latest_char_dir / "Waypoint.cpp"));
  const std::string rb3_latest_waypoint_h = compact(read_file(
      rb3_latest_char_dir / "Waypoint.h"));
  const std::string rb3_latest_char_guitar_string_cpp = compact(read_file(
      rb3_latest_char_dir / "CharGuitarString.cpp"));
  const std::string rb3_latest_char_guitar_string_h = compact(read_file(
      rb3_latest_char_dir / "CharGuitarString.h"));
  const std::string rb3_latest_char_eye_dart_ruleset_cpp = compact(read_file(
      rb3_latest_char_dir / "CharEyeDartRuleset.cpp"));
  const std::string rb3_latest_char_eye_dart_ruleset_h = compact(read_file(
      rb3_latest_char_dir / "CharEyeDartRuleset.h"));
  const std::string rb3_latest_char_interest_cpp = compact(read_file(
      rb3_latest_char_dir / "CharInterest.cpp"));
  const std::string rb3_latest_char_interest_h = compact(read_file(
      rb3_latest_char_dir / "CharInterest.h"));
  const std::string rb3_latest_char_bone_offset_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBoneOffset.cpp"));
  const std::string rb3_latest_char_bone_offset_h = compact(read_file(
      rb3_latest_char_dir / "CharBoneOffset.h"));
  const std::string rb3_latest_char_bone_twist_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBoneTwist.cpp"));
  const std::string rb3_latest_char_bone_twist_h = compact(read_file(
      rb3_latest_char_dir / "CharBoneTwist.h"));
  const std::string rb3_latest_char_bones_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBones.cpp"));
  const std::string rb3_latest_char_bones_h = compact(read_file(
      rb3_latest_char_dir / "CharBones.h"));
  const std::string rb3_latest_char_bones_blender_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBonesBlender.cpp"));
  const std::string rb3_latest_char_bones_blender_h = compact(read_file(
      rb3_latest_char_dir / "CharBonesBlender.h"));
  const std::string rb3_latest_char_bones_meshes_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBonesMeshes.cpp"));
  const std::string rb3_latest_char_bones_meshes_h = compact(read_file(
      rb3_latest_char_dir / "CharBonesMeshes.h"));
  const std::string rb3_latest_char_bone_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBone.cpp"));
  const std::string rb3_latest_char_bone_h = compact(read_file(
      rb3_latest_char_dir / "CharBone.h"));
  const std::string rb3_latest_char_bone_dir_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBoneDir.cpp"));
  const std::string rb3_latest_char_utl_cpp = compact(read_file(
      rb3_latest_char_dir / "CharUtl.cpp"));
  const std::string rb3_latest_char_utl_h = compact(read_file(
      rb3_latest_char_dir / "CharUtl.h"));
  const std::string rb3_latest_char_clip_h = compact(read_file(
      rb3_latest_char_dir / "CharClip.h"));
  const std::string rb3_latest_char_clip_cpp = compact(read_file(
      rb3_latest_char_dir / "CharClip.cpp"));
  const std::string rb3_latest_char_bones_samples_h = compact(read_file(
      rb3_latest_char_dir / "CharBonesSamples.h"));
  const std::string rb3_latest_char_bones_samples_cpp = compact(read_file(
      rb3_latest_char_dir / "CharBonesSamples.cpp"));
  const std::string rb3_latest_char_clip_driver_cpp = compact(read_file(
      rb3_latest_char_dir / "CharClipDriver.cpp"));
  const std::string rb3_latest_char_clip_group_cpp = compact(read_file(
      rb3_latest_char_dir / "CharClipGroup.cpp"));
  const std::string rb3_latest_char_clip_group_h = compact(read_file(
      rb3_latest_char_dir / "CharClipGroup.h"));
  const std::string rb2_char_clip_samples_cpp = compact(read_file(
      rb2_dump_char_dir / "CharClipSamples.cpp"));
  const std::string rb2_char_bones_samples_cpp = compact(read_file(
      rb2_dump_char_dir / "CharBonesSamples.cpp"));
  const std::string rb2_char_clip_driver_cpp = compact(read_file(
      rb2_dump_char_dir / "CharClipDriver.cpp"));
  const std::string rb2_char_driver_cpp = compact(read_file(
      rb2_dump_char_dir / "CharDriver.cpp"));
  const std::string rb2_char_walk_cpp = compact(read_file(
      rb2_dump_char_dir / "CharWalk.cpp"));
  const std::string rb2_outfit_loader_cpp = compact(read_file(
      rb2_dump_char_dir / "OutfitLoader.cpp"));
  const std::string rb2_char_collide_cpp = compact(read_file(
      rb2_dump_char_dir / "CharCollide.cpp"));
  const std::string rb2_char_collide_h = compact(read_file(
      rb2_dump_char_dir / "CharCollide.h"));
  const std::string rb2_dolmatch_filt = compact(read_file(
      extra_dir / "rb3-retail-old/doc/dolmatchoutput_filt.txt"));
  const std::string band3_config = compact(read_file(
      extra_dir / "band3_recomp/band3_config.toml"));
  const std::string band3_readme = read_file(
      extra_dir / "band3_recomp/README.md");
  const std::string stock_guitar_string_sweep = read_file(
      engine_dir /
      "out/source_guitarstring_20260711/guitar_sweep/guitar_sweep.csv");
  const std::string stock_guitar_string_sweep_compact =
      compact(stock_guitar_string_sweep);

  bool ok = true;

  ok &= contains(doc, "MiloEditor/MiloLib/Assets/Rnd/RndMesh.cs",
                 "document cites RndMesh source");
  ok &= contains(source_readme,
                 "This directory is a deliberately small, in-worktree "
                 "reference snapshot",
                 "snapshot README documents copied-source scope");
  ok &= contains(doc, "not a full mirror",
                 "document states copied source snapshot boundary");
  ok &= missing(doc, "re-notes",
                "document must not cite absent re-notes snapshot");
  ok &= contains(doc, "## Source Coverage Matrix",
                 "document includes source coverage matrix");
  ok &= contains(doc,
                 "| Clip sample/output publishing | `rb3-latest` `CharClip` / "
                 "`CharBones` / `CharBonesSamples` / `CharBone`, "
                 "`MiloEditor` `RndTrans.cs`, `rb3-retail-old` RB2 dump, "
                 "`band3_recomp` symbols |",
                 "coverage matrix cites current CharClip source evidence");
  ok &= contains(doc,
                 "| Clip groups | `rb3-latest` `CharClipGroup.cpp` / "
                 "`CharClipGroup.h` |",
                 "coverage matrix cites CharClipGroup source evidence");
  ok &= contains(doc,
                 "| Clip set preview/editor container | `rb3-latest` "
                 "`CharClipSet.cpp` / `CharClipSet.h` |",
                 "coverage matrix cites CharClipSet source evidence");
  ok &= contains(doc,
                 "| Clip display/task graph diagnostics | `rb3-latest` "
                 "`CharClipDisplay.cpp` / `CharClipDisplay.h`, "
                 "`CharTaskMgr.cpp` / `CharTaskMgr.h` |",
                 "coverage matrix cites CharClipDisplay and CharTaskMgr "
                 "source evidence");
  ok &= contains(doc,
                 "Native shared loader follows source `CharClipGroup::Load`: "
                 "`Hmx::Object::Load`, `mClips`, `mWhich`, and revision-gated "
                 "`mFlags`.",
                 "coverage matrix records native CharClipGroup Load slice");
  ok &= contains(doc,
                 "Guitarist active group selection now follows source "
                 "`CharClipGroup::GetClip` cycling.",
                 "coverage matrix records native CharClipGroup GetClip slice");
  ok &= contains(doc,
                 "Channel naming, compression sizing, sample interpolation "
                 "wrappers, CharBone output row fields, and partial call flow "
                 "are source-backed",
                 "coverage matrix records concrete CharBones source evidence");
  ok &= contains(doc,
                 "sample decode/evaluate and broad pose publishing remain "
                 "fenced",
                 "coverage matrix keeps incomplete clip runtime fenced");
  ok &= contains(doc,
                 "| Hair two-sided rendering | User/project visual override |",
                 "coverage matrix marks hair two-sided as project override");
  ok &= contains(doc,
                 "| Mesh hide visibility rows | `rb3-latest` `CharMeshHide.cpp` / "
                 "`CharMeshHide.h` |",
                 "coverage matrix cites CharMeshHide source");
  ok &= contains(doc,
                 "| Translucent character draw controller | `rb3-latest` "
                 "`CharTransDraw.cpp` / `CharTransDraw.h`, `Character.h` "
                 "draw-mode enum |",
                 "coverage matrix cites CharTransDraw source");
  ok &= contains(doc,
                 "| Cuff/accessory deformation rows | `rb3-latest` "
                 "`CharCuff.cpp` / `CharCuff.h` |",
                 "coverage matrix cites CharCuff source");
  ok &= contains(doc,
                 "| Blend-bone constraints | `rb3-latest` "
                 "`CharBlendBone.cpp` / `CharBlendBone.h` |",
                 "coverage matrix cites CharBlendBone source");
  ok &= contains(doc,
                 "| Sleeve secondary motion | `rb3-latest` `CharSleeve.cpp` / "
                 "`CharSleeve.h` |",
                 "coverage matrix cites CharSleeve source");
  ok &= contains(doc,
                 "| Character mesh cache | `rb3-latest` "
                 "`CharMeshCacheMgr.cpp` / `CharMeshCacheMgr.h` |",
                 "coverage matrix cites CharMeshCacheMgr source");
  ok &= contains(doc,
                 "| Transform copy controller | `rb3-latest` `CharTransCopy.cpp` / "
                 "`CharTransCopy.h` |",
                 "coverage matrix cites CharTransCopy source");
  ok &= contains(doc,
                 "| IK scale controller | `rb3-latest` `CharIKScale.cpp` / "
                 "`CharIKScale.h` |",
                 "coverage matrix cites CharIKScale source");
  ok &= contains(doc, "| Poll groups | `rb3-latest` `CharPollGroup.cpp` |",
                 "coverage matrix cites CharPollGroup source boundary");
  ok &= contains(doc,
                 "Native helper ports source `Poll`, `ListPollChildren`, and "
                 "`PollDeps` decision behavior",
                 "coverage matrix records native CharPollGroup helper");
  ok &= contains(doc,
                 "| Guitar string bend controller | `rb3-latest` "
                 "`CharGuitarString.cpp` / `CharGuitarString.h`; stock "
                 "guitar sweep |",
                 "coverage matrix cites CharGuitarString stock boundary");
  ok &= contains(doc,
                 "Native helper ports source `Poll` projection/open-string math "
                 "and `PollDeps`",
                 "coverage matrix records native CharGuitarString helper");
  ok &= contains(rb3_latest_char_guitar_string_h,
                 "ObjPtr<RndTransformable,classObjectDir>mNut;",
                 "latest CharGuitarString header exposes nut transform");
  ok &= contains(rb3_latest_char_guitar_string_cpp,
                 "voidCharGuitarString::Poll(){if(!mNut||!mBridge||!mBend||"
                 "!mTarget)return;",
                 "latest CharGuitarString Poll gates missing transforms");
  ok &= contains(rb3_latest_char_guitar_string_cpp,
                 "if(mOpen)clamped=0.0f;Interp(nutvec,bridgevec,clamped,"
                 "tf50.v);mBend->SetWorldXfm(tf50);",
                 "latest CharGuitarString Poll moves bend along string");
  ok &= contains(rb3_latest_char_guitar_string_cpp,
                 "changedBy.push_back(mNut);changedBy.push_back(mBridge);"
                 "changedBy.push_back(mTarget);change.push_back(mBend);",
                 "latest CharGuitarString PollDeps order");
  ok &= contains(char_mesh_h,
                 "structSourceCharGuitarStringPollResult{boolwrote_bend=false;",
                 "native exposes CharGuitarString poll result");
  ok &= contains(char_mesh_h,
                 "SourceCharGuitarStringPollResultsource_char_guitar_string_poll(",
                 "native exposes CharGuitarString poll helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_guitar_string_poll_deps(",
                 "native exposes CharGuitarString PollDeps helper");
  ok &= contains(char_mesh,
                 "SourceCharGuitarStringPollResultsource_char_guitar_string_poll("
                 "boolhas_nut,boolhas_bridge,boolhas_bend,boolhas_target,",
                 "native implements CharGuitarString poll helper");
  ok &= contains(char_mesh,
                 "if(!has_nut||!has_bridge||!has_bend||!has_target)"
                 "returnresult;",
                 "native CharGuitarString helper ports source gate");
  ok &= contains(char_mesh,
                 "std::clamp(source_vec_dot(tmp,tmp2)/source_vec_dot(tmp2,tmp2),"
                 "0.0f,1.0f);if(open)clamped=0.0f;",
                 "native CharGuitarString helper ports projection and open gate");
  ok &= contains(char_mesh,
                 "source_vec_add(source_vec_scale(nut_pos,1.0f-clamped),"
                 "source_vec_scale(bridge_pos,clamped));",
                 "native CharGuitarString helper ports source interpolation");
  ok &= contains(char_mesh,
                 "deps.changed_by.push_back(nut);deps.changed_by.push_back(bridge);"
                 "deps.changed_by.push_back(target);deps.change.push_back(bend);",
                 "native CharGuitarString helper ports PollDeps order");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_guitar_string_source_test",
                 "CMake builds CharGuitarString source test");
  ok &= contains(guitar_string_source_test,
                 "source_char_guitar_string_poll(",
                 "focused CharGuitarString test calls poll helper");
  ok &= contains(guitar_string_source_test,
                 "\"openstringforcesbendtonut\"",
                 "focused CharGuitarString test covers open string override");
  ok &= contains(guitar_string_source_test,
                 "source_char_guitar_string_poll_deps(deps,\"nut.trans\","
                 "\"bridge.trans\",\"target.trans\",\"bend.trans\")",
                 "focused CharGuitarString test covers PollDeps");
  ok &= contains(stock_guitar_string_sweep,
                 "\"Entry\",\"HasCharGuitarString\",\"StringHits\",\"TransSummary\"",
                 "stock guitar sweep records CharGuitarString column");
  ok &= missing(stock_guitar_string_sweep_compact, "\",\"1\",\"",
                "stock guitar sweep has no active CharGuitarString rows");
  ok &= contains(doc, "HasCharGuitarString=0",
                 "document records stock CharGuitarString absence");
  ok &= contains(doc,
                 "Native `source_char_guitar_string_*` helpers port that math",
                 "document records native CharGuitarString helper");
  ok &= contains(doc,
                 "not the active GH2 stock guitar/left-hand\n"
                 "or string-transparency route",
                 "document fences CharGuitarString from implicit fixes");
  ok &= contains(doc, "| FaceFX/lip-sync boundary | `rb3-latest` "
                 "`CharFaceServo.*`, `CharLipSync.*`, "
                 "`CharLipSyncDriver.*`; stock GH2 `FaceFxLipSyncServo` "
                 "inventory |",
                 "coverage matrix records FaceFxLipSyncServo boundary");
  ok &= contains(doc, "## Remaining Character Import Checklist",
                 "document has explicit remaining import checklist");
  ok &= contains(doc,
                 "The unresolved work is the\nconnected character animation "
                 "and controller runtime",
                 "remaining import checklist identifies the broad unresolved area");
  ok &= contains(doc,
                 "Port the missing source-backed bodies for "
                 "`CharBonesSamples::LoadHeader`,",
                 "remaining import checklist names CharBonesSamples body gap");
  ok &= contains(doc,
                 "Port the missing source-backed bodies for "
                 "`CharBones::ScaleAdd`,",
                 "remaining import checklist names CharBones pose gap");
  ok &= contains(doc,
                 "Port `CharClipDriver::Evaluate`/poll timing, blend, loop, "
                 "beat-align,",
                 "remaining import checklist names CharClipDriver runtime gap");
  ok &= contains(doc,
                 "Native `char_clip_driver_masked_play_flags` ports the "
                 "constructor mask\n    application exactly",
                 "document records concrete CharClipDriver mask slice");
  ok &= contains(doc,
                 "Native `source_char_clip_driver_construct`,",
                 "document records native CharClipDriver stack helper slice");
  ok &= contains(doc,
                 "`Exit(false)` returning the next node",
                 "document records CharClipDriver Exit(false) source behavior");
  ok &= contains(doc,
                 "Native `source_char_driver_starved` ports the concrete "
                 "source\n    `CharDriver::Starved` body",
                 "document records concrete CharDriver starved slice");
  ok &= contains(doc,
                 "Native `source_char_driver_resolve_blend_width` ports the "
                 "concrete\n    `CharDriver::Play` sentinel rule",
                 "document records concrete CharDriver blend fallback slice");
  ok &= contains(doc,
                 "Native `source_char_driver_should_start_clip` ports the "
                 "concrete\n    `CharDriver::Play` duplicate-clip gate",
                 "document records concrete CharDriver duplicate gate slice");
  ok &= contains(doc,
                 "Native `source_char_driver_first_playing_index` ports the "
                 "concrete\n    `CharDriver::FirstPlaying` stack scan",
                 "document records concrete CharDriver FirstPlaying slice");
  ok &= contains(doc,
                 "Port `CharDriver::Load`, `CharDriver::Poll`, and "
                 "`EvaluateFlags`",
                 "remaining import checklist names CharDriver runtime gap");
  ok &= contains(doc,
                 "`CharServoBone` movement and broad `CharBonesMeshes` writes "
                 "depend on the",
                 "remaining import checklist keeps CharServoBone fenced to pose stack");
  ok &= contains(doc,
                 "point\n     world-row writeback still needs the overloaded",
                 "remaining import checklist names CharHair writeback gap");
  ok &= contains(doc,
                 "The project hair rule is two-sided culling only.",
                 "remaining import checklist fences hair material behavior");
  ok &= contains(doc,
                 "The\n     focused clip audit found its real "
                 "`keyboard_main` clips under",
                 "remaining import checklist records metal_keyboard route finding");
  ok &= contains(doc, "MiloEditor/MiloLib/Assets/Rnd/RndMat.cs",
                 "document cites RndMat source");
  ok &= contains(doc, "MiloEditor/MiloLib/Assets/Rnd/RndGroup.cs",
                 "document cites RndGroup source");
  ok &= contains(doc, "glTFMilo/Source/glTFMilo/Program.cs",
                 "document cites glTFMilo skinning source");
  ok &= contains(doc, "rb3/src/system/rndobj/Mesh.cpp",
                 "document cites RB3 RndMesh runtime source");
  ok &= contains(doc, "rb3/src/system/rndobj/Mat.cpp",
                 "document cites RB3 RndMat runtime source");
  ok &= contains(doc, "rb3/src/system/rndobj/Mat.h",
                 "document cites RB3 RndMat runtime header source");
  ok &= contains(doc, "rb3/src/system/rndobj/Trans.cpp",
                 "document cites RB3 RndTransformable runtime source");
  ok &= contains(doc, "rb3/src/system/rndobj/Trans.h",
                 "document cites RB3 RndTransformable runtime header source");
  ok &= contains(doc, "rb3-latest/src/system/char/CharHair.cpp",
                 "document cites latest CharHair runtime source");
  ok &= contains(doc, "rb3-latest/src/system/char/CharClipGroup.cpp",
                 "document cites latest CharClipGroup runtime source");
  ok &= contains(doc,
                 "The older native graph/stance continuity chooser is not "
                 "source behavior and\n    is removed",
                 "document fences removed native active-group selector");
  ok &= contains(doc, "rb3-latest/src/system/char/CharIKRod.cpp",
                 "document cites latest CharIKRod runtime source");
  ok &= contains(doc, "rb3-latest/src/system/char/CharServoBone.cpp",
                 "document cites latest CharServoBone runtime source");
  ok &= contains(doc, "rb3-latest/src/system/char/CharWeightSetter.cpp",
                 "document cites latest CharWeightSetter runtime source");
  ok &= contains(doc, "rb3-latest/src/system/char/Character.cpp",
                 "document cites latest Character root loader source");
  ok &= contains(doc, "rb3-latest/src/system/rndobj/Dir.cpp",
                 "document cites latest RndDir root loader source");
  ok &= contains(doc, "rb3-latest/src/system/obj/Dir.cpp",
                 "document cites latest ObjectDir root loader source");
  ok &= contains(doc, "rb3/src/system/char/CharLookAt.cpp",
                 "document cites CharLookAt runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharEyes.cpp",
                 "document cites CharEyes runtime source");
  ok &= contains(doc,
                 "native helpers port `CharEyes` poll-child/dependency "
                 "publication plus `CharInterest` / `CharEyeDartRuleset` "
                 "data decisions only",
                 "coverage matrix records native CharEyes helper boundary");
  ok &= contains(doc, "CharInterest.cpp` / `CharInterest.h",
                 "coverage matrix cites CharInterest source");
  ok &= contains(doc, "CharEyeDartRuleset.cpp` / `CharEyeDartRuleset.h",
                 "coverage matrix cites CharEyeDartRuleset source");
  ok &= contains(doc,
                 "native helpers port `CharEyes` poll-child/dependency "
                 "publication plus `CharInterest` / `CharEyeDartRuleset` "
                 "data decisions only",
                 "coverage matrix records eye data-only boundary");
  ok &= contains(doc, "rb3/src/system/char/CharIKHand.cpp",
                 "document cites CharIKHand runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharUpperTwist.cpp",
                 "document cites CharUpperTwist runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharForeTwist.cpp",
                 "document cites CharForeTwist runtime source");
  ok &= contains(doc, "rb3-latest` `CharNeckTwist.cpp` / `CharNeckTwist.h",
                 "coverage matrix cites latest CharNeckTwist source");
  ok &= contains(doc, "ihatecompvir-extra/band3_recomp",
                 "document cites extra band3_recomp source");
  ok &= contains(band3_readme, "Early recompilation of Rock Band 3",
                 "band3_recomp README is available to source-truth contract");

  ok &= contains(doc,
                 "| Character/BandCharacter/RndDir/ObjectDir root body | "
                 "`rb3-latest` `Character.cpp`, `rndobj/Dir.cpp`, `obj/Dir.cpp` |",
                 "coverage matrix records root dir body source evidence");
  ok &= contains(doc,
                 "| Character lifecycle and directory sync flow | "
                 "`rb3-latest` `Character.cpp`, `Character.h` |",
                 "coverage matrix records Character runtime flow source");
  ok &= contains(doc,
                 "| Character subsystem init/terminate | "
                 "`rb3-latest` `Char.cpp`, `Char.h` |",
                 "coverage matrix records Char subsystem lifecycle source");
  ok &= contains(doc,
                 "| Character test harness defaults | `rb3-latest` "
                 "`CharacterTest.cpp`, `CharacterTest.h` |",
                 "coverage matrix records CharacterTest harness source");
  ok &= contains(doc, "## Character Root Body Boundary",
                 "document records root body boundary section");
  ok &= contains(doc,
                 "Do not decode or apply root `Character`, `RndDir`, or "
                 "`ObjectDir` runtime fields",
                 "document fences root dir body from guessed runtime decode");
  ok &= contains(doc,
                 "stock_character_dir_entry_inventory.log",
                 "document cites root dir entry inventory proof");
  ok &= contains(rb3_latest_char_cpp,
                 "voidCharInit(){Character::Init();CharBonesObject::Init();"
                 "CharBoneOffset::Init();PreloadSharedSubdirs(\"char\");"
                 "CharBoneDir::Init();CharUtlInit();TheDebug.AddExitCallback"
                 "(CharTerminate);}",
                 "latest Char.cpp exposes subsystem init order");
  ok &= contains(rb3_latest_char_cpp,
                 "voidCharTerminate(){TheDebug.RemoveExitCallback"
                 "(CharTerminate);Character::Terminate();"
                 "CharBoneDir::Terminate();}",
                 "latest Char.cpp exposes subsystem terminate order");
  ok &= contains(rb3_latest_char_h, "voidCharInit(),CharTerminate();",
                 "latest Char.h declares lifecycle functions");
  ok &= contains(char_mesh_h,
                 "structSourceCharLifecyclePlan{std::vector<std::string>"
                 "init_steps;",
                 "native exposes Char lifecycle source plan");
  ok &= contains(char_mesh_h,
                 "SourceCharLifecyclePlansource_char_lifecycle_plan();",
                 "native declares Char lifecycle helper");
  ok &= contains(char_mesh,
                 "SourceCharLifecyclePlansource_char_lifecycle_plan(){",
                 "native implements Char lifecycle helper");
  ok &= contains(char_mesh,
                 "\"PreloadSharedSubdirs(char)\"",
                 "native lifecycle helper records source preload step");
  ok &= contains(character_source_test,
                 "source_char_lifecycle_plan()",
                 "focused Character source test covers lifecycle helper");
  ok &= contains(doc,
                 "Native\n  `source_char_lifecycle_plan` records this order only",
                 "document records bounded Char lifecycle helper");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::PreLoad(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(0x11,0);if(gRev>1){RndDir::PreLoad(bs);",
                 "latest Character PreLoad delegates through RndDir");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::PostLoad(BinStream&bs){intrevs=PopRev(this);",
                 "latest Character PostLoad starts from pushed revision");
  ok &= contains(rb3_latest_character_cpp,
                 "RndDir::PostLoad(bs);",
                 "latest Character PostLoad delegates through RndDir");
  ok &= contains(rb3_latest_character_cpp,
                 "bs>>mLods;bs>>mShadow;",
                 "latest Character PostLoad reads lod/shadow rows");
  ok &= contains(rb3_latest_character_h,
                 "enumPollState{kCharCreated=0,kCharSyncObject=1,"
                 "kCharEntered=2,kCharPolled=3,kCharExited=4,};",
                 "Character source poll-state enum order");
  ok &= contains(rb3_latest_character_cpp,
                 "Character::Character():mLods(this),mLastLod(0),"
                 "mMinLod(0),mShadow(this,0),mTransGroup(this,0),"
                 "mDriver(0),",
                 "Character source constructor default prefix");
  ok &= contains(rb3_latest_character_cpp,
                 "mSphereBase(this,this),mBounding(),mPollState("
                 "kCharCreated),mTest(newCharacterTest(this)),mFrozen(0),"
                 "mDrawMode(kCharDrawAll),mTeleported(1),mInterestToForce(),",
                 "Character source constructor runtime defaults");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::Enter(){mPollState=kCharEntered;"
                 "mMinLod=-1;mFrozen=false;mLastLod=0;mTeleported=true;"
                 "mInterestToForce=Symbol();RndDir::Enter();}",
                 "Character source Enter state flow");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::Exit(){mPollState=kCharExited;"
                 "RndDir::Exit();}",
                 "Character source Exit state flow");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::Poll(){START_AUTO_TIMER(\"char_poll\");"
                 "if(!mFrozen){",
                 "Character source Poll frozen gate");
  ok &= contains(rb3_latest_character_cpp,
                 "RndDir::Poll();mTeleported=false;mPollState=kCharPolled;",
                 "Character source Poll state writes");
  ok &= contains(rb3_latest_character_cpp,
                 "CharServoBone*Character::BoneServo(){if(mDriver)return"
                 "dynamic_cast<CharServoBone*>(mDriver->mBones.Ptr());"
                 "elsereturn0;}",
                 "Character source BoneServo driver gate");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::Replace(Hmx::Object*from,Hmx::Object*to){"
                 "RndDir::Replace(from,to);if(from==mSphereBase){"
                 "mSphereBase=dynamic_cast<RndTransformable*>(to);"
                 "if(!mSphereBase)mSphereBase=this;}}",
                 "Character source Replace sphere-base fallback");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::SyncObjects(){mPollState=kCharSyncObject;"
                 "if(Find<RndMesh>(\"bone_pelvis.mesh\",false))"
                 "ConvertBonesToTranses(this,false);RndDir::SyncObjects();"
                 "VectorRemove(mDraws,mTransGroup);",
                 "Character source SyncObjects prefix");
  ok &= contains(rb3_latest_character_cpp,
                 "SyncShadow();CharPollableSortersorter;"
                 "sorter.Sort(mPolls);}",
                 "Character source SyncObjects sorts polls");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::AddedObject(Hmx::Object*o){"
                 "if(dynamic_cast<CharPollable*>(o)){CharDriver*driver="
                 "dynamic_cast<CharDriver*>(o);if(driver){boolstrsmatch="
                 "strcmp(driver->Name(),\"main.drv\")==0;if(strsmatch){"
                 "mDriver=driver;}}}}",
                 "Character source AddedObject main driver rule");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::RemovingObject(Hmx::Object*o){"
                 "if(o==mDriver)mDriver=0;RndDir::RemovingObject(o);}",
                 "Character source RemovingObject driver clear");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::SetSphereBase(RndTransformable*trans){"
                 "if(!trans)trans=this;Spheres18;MakeWorldSphere(s18,false);"
                 "Multiply(trans->WorldXfm(),s18.center,s18.center);"
                 "SetSphere(s18);mSphereBase=trans;}",
                 "Character source SetSphereBase flow");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::SetInterestObjects(constObjPtrList<"
                 "CharInterest,ObjectDir>&oList,ObjectDir*dir){CharEyes*eyes="
                 "GetEyes();if(eyes){eyes->ClearAllInterestObjects();"
                 "for(ObjPtrList<CharInterest,ObjectDir>::iteratorit="
                 "oList.begin();it!=oList.end();++it){if(ValidateInterest("
                 "*it,dir?dir:(*it)->Dir()))eyes->AddInterestObject(*it);}}}",
                 "Character source SetInterestObjects gate");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::ForceBlink(){CharEyes*eyes=GetEyes();"
                 "if(eyes)eyes->ForceBlink();}",
                 "Character source ForceBlink eyes gate");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::SetInterestFilterFlags(intflags){"
                 "CharEyes*eyes=GetEyes();if(eyes){eyes->"
                 "mInterestFilterFlags=flags;eyes->unk15c=true;}}",
                 "Character source SetInterestFilterFlags eyes gate");
  ok &= contains(rb3_latest_character_cpp,
                 "ShadowBone*Character::AddShadowBone("
                 "RndTransformable*trans){if(!trans)return0;else{for(inti=0;"
                 "i<mShadowBones.size();i++){if(mShadowBones[i]->mParent=="
                 "trans)returnmShadowBones[i];}mShadowBones.push_back("
                 "newShadowBone());mShadowBones.back()->mParent=trans;"
                 "returnmShadowBones.back();}}",
                 "Character source AddShadowBone flow");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::UnhookShadow(){for(inti=0;"
                 "i<mShadowBones.size();i++){}DeleteAll(mShadowBones);}",
                 "Character source UnhookShadow deletes rows");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::SyncShadow(){UnhookShadow();if(mShadow){"
                 "if(GetGfxMode()==kOldGfx){",
                 "Character source SyncShadow old gfx gate");
  ok &= contains(rb3_latest_character_cpp,
                 "if(!mesh->mBones.empty()){for(inti=0;i<mesh->mBones.size();"
                 "i++){mesh->SetBone(i,AddShadowBone(mesh->mBones[i].mBone),"
                 "false);}}else{mesh->SetTransParent(AddShadowBone(mesh),"
                 "false);}",
                 "Character source SyncShadow bone/parent hookups");
  ok &= contains(rb3_latest_character_cpp,
                 "VectorRemove(mDraws,mShadow);}}",
                 "Character source SyncShadow removes shadow draw");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::CopyBoundingSphere(Character*c){"
                 "SetSphere(c->mSphere);mBounding=c->mBounding;"
                 "if(c->mSphereBase)mSphereBase=c->mSphereBase;"
                 "elsemSphereBase=0;}",
                 "Character source CopyBoundingSphere flow");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::RepointSphereBase(ObjectDir*dir){"
                 "if(mSphereBase){RndTransformable*trans=dir->"
                 "Find<RndTransformable>(mSphereBase->Name(),false);"
                 "if(trans)mSphereBase=trans;}}",
                 "Character source RepointSphereBase flow");
  ok &= contains(rb3_latest_character_cpp,
                 "voidCharacter::PreSave(BinStream&bs){UnhookShadow();}",
                 "Character source PreSave flow");
  ok &= contains(rb3_latest_character_test_h,
                 "classCharacterTest:publicRndOverlay::Callback",
                 "CharacterTest header exposes overlay callback harness");
  ok &= contains(rb3_latest_character_test_cpp,
                 "CharacterTest::CharacterTest(classCharacter*thechar):"
                 "mMe(thechar),mDriver(thechar,0),mClip1(thechar,0),"
                 "mClip2(thechar,0),mFilterGroup(thechar,0),",
                 "CharacterTest source constructor owner pointers");
  ok &= contains(rb3_latest_character_test_cpp,
                 "mTransition(0),mCycleTransition(1),mMetronome(0),"
                 "mZeroTravel(0),mShowScreenSize(0),mShowFootExtents(0)",
                 "CharacterTest source constructor toggles");
  ok &= contains(rb3_latest_character_test_cpp,
                 "mShowDistMap=none;",
                 "CharacterTest source dist-map default");
  ok &= contains(rb3_latest_character_test_cpp,
                 "if(mOverlay->mCallback==this){mOverlay->mCallback=0;"
                 "RndOverlay*over=mOverlay;over->mShowing=0;over->mTimer."
                 "Restart();}",
                 "CharacterTest source destructor overlay cleanup");
  ok &= contains(rb3_latest_character_test_cpp,
                 "if(mDriver&&(mClip1||mClip2))mDriver->Highlight();",
                 "CharacterTest source Draw highlights driver");
  ok &= contains(rb3_latest_character_test_cpp,
                 "RndTransformable*trans=CharUtlFindBoneTrans(\"bone_head\","
                 "mMe);if(!trans)trans=mMe;",
                 "CharacterTest source Draw head fallback");
  ok &= contains(rb3_latest_character_test_cpp,
                 "ObjectDir*clipdir=mDriver?mDriver->ClipDir():0;"
                 "if(clipdir&&mClip1){",
                 "CharacterTest source Poll clip branch gate");
  ok &= contains(rb3_latest_character_test_cpp,
                 "if(!drivs)PlayNew();elseif(mClip2){CharClip*drivclip="
                 "drivs->mClip;if(drivclip!=mClip1&&drivclip!=mClip2||"
                 "(drivclip==mClip2&&unk64<drivs->mBeat))PlayNew();}"
                 "elseif(drivs->mClip!=mClip1)PlayNew();",
                 "CharacterTest source Poll PlayNew decisions");
  ok &= contains(rb3_latest_character_test_cpp,
                 "if(mZeroTravel){//someTransformoperationif(mMe->"
                 "BoneServo()){mMe->BoneServo()->mRegulate=0;}Recenter();}",
                 "CharacterTest source Poll zero-travel branch");
  ok &= contains(rb3_latest_character_test_cpp,
                 "if(!mMe->mDriver)mMe->New<CharDriver>(\"main.drv\");",
                 "CharacterTest source AddDefaults main driver");
  ok &= contains(rb3_latest_character_test_cpp,
                 "CharForeTwist*ltwist=mMe->New<CharForeTwist>("
                 "\"foreTwist_L.ik\");ltwist->SetProperty(hand,DataNode("
                 "lhand));ltwist->SetProperty(twist2,DataNode(ltwist2));"
                 "ltwist->SetProperty(offset,DataNode(90));",
                 "CharacterTest source AddDefaults left foretwist");
  ok &= contains(rb3_latest_character_test_cpp,
                 "CharForeTwist*rtwist=mMe->New<CharForeTwist>("
                 "\"foreTwist_R.ik\");rtwist->SetProperty(hand,DataNode("
                 "rhand));rtwist->SetProperty(twist2,DataNode(rtwist2));"
                 "rtwist->SetProperty(offset,DataNode(-90));",
                 "CharacterTest source AddDefaults right foretwist");
  ok &= contains(rb3_latest_character_test_cpp,
                 "CharUpperTwist*ltwist=mMe->New<CharUpperTwist>("
                 "\"upperTwist_L.ik\");ltwist->SetProperty(twist1,DataNode("
                 "lutwist1));ltwist->SetProperty(twist2,DataNode(lutwist2));"
                 "ltwist->SetProperty(upper_arm,DataNode(luarm));",
                 "CharacterTest source AddDefaults left uppertwist");
  ok &= contains(rb3_latest_character_test_cpp,
                 "voidCharacterTest::SetStartEndBeat(floatf1,floatf2,intbpm)",
                 "CharacterTest source SetStartEndBeat exists");
  ok &= contains(rb3_latest_character_test_cpp,
                 "miloObj->Handle(Message(\"set_anim_frame\",DataNode((f1*"
                 "30.0f)/(bpm/60.0f)),DataNode((f2*30.0f)/(bpm/60.0f)),"
                 "DataNode((float)bpm)),true);",
                 "CharacterTest source SetStartEndBeat frames");
  ok &= contains(rb3_latest_character_test_cpp,
                 "voidCharacterTest::SetMoveSelf(boolb){if(mMe->BoneServo()){"
                 "mMe->BoneServo()->SetMoveSelf(b);}}",
                 "CharacterTest source SetMoveSelf gate");
  ok &= contains(rb3_latest_character_test_cpp,
                 "if(gRev!=0xD)mDriver.Load(bs,false,mMe);",
                 "CharacterTest source Load driver gate");
  ok &= contains(rb3_latest_rnd_dir_cpp,
                 "voidRndDir::PreLoad(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(0xA,0);PushRev(packRevs(gAltRev,gRev),this);"
                 "ObjectDir::PreLoad(bs);}",
                 "latest RndDir PreLoad delegates through ObjectDir");
  ok &= contains(rb3_latest_rnd_dir_cpp,
                 "voidRndDir::PostLoad(BinStream&bs){ObjectDir::PostLoad(bs);",
                 "latest RndDir PostLoad starts with ObjectDir");
  ok &= contains(rb3_latest_rnd_dir_cpp,
                 "LOAD_SUPERCLASS(RndAnimatable)LOAD_SUPERCLASS(RndDrawable)",
                 "latest RndDir PostLoad reads animatable/drawable superclasses");
  ok &= contains(rb3_latest_obj_dir_cpp,
                 "voidObjectDir::PreLoad(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(0x1B,0);",
                 "latest ObjectDir PreLoad source revision gate");
  ok &= contains(rb3_latest_obj_dir_cpp,
                 "if(gRev>0x15)Hmx::Object::LoadType(bs);",
                 "latest ObjectDir PreLoad reads revision-gated object type");
  ok &= contains(rb3_latest_obj_dir_cpp,
                 "PushRev(packRevs(gAltRev,gRev),this);",
                 "latest ObjectDir PreLoad pushes packed revision");
  ok &= contains(rb3_latest_obj_dir_cpp,
                 "voidObjectDir::PostLoad(BinStream&bs){intrevs=PopRev(this);",
                 "latest ObjectDir PostLoad pops packed revision");
  ok &= contains(char_mesh_h, "int32_tdir_version=0;",
                 "native Character stores root directory version");
  ok &= contains(char_mesh_h, "uint64_tdir_entry_offset=0;",
                 "native Character stores root body offset");
  ok &= contains(char_mesh_h, "uint64_tdir_entry_size=0;",
                 "native Character stores root body size");
  ok &= contains(char_mesh_h, "std::vector<uint8_t>dir_entry_bytes;",
                 "native Character stores bounded root body bytes");
  ok &= contains(char_mesh,
                 "out.dir_version=dir.dir_version;",
                 "native load_character copies root directory version");
  ok &= contains(char_mesh,
                 "out.dir_entry_offset=dir.dir_entry_offset;",
                 "native load_character copies root body offset");
  ok &= contains(char_mesh,
                 "out.dir_entry_bytes.assign(",
                 "native load_character copies bounded root body bytes");
  ok &= contains(bind_audit,
                 "\"[dir-entry]path=%schar=%sdirType=%sdirVersion=%d",
                 "bind audit logs root dir entry inventory");
  ok &= contains(bind_audit, "source-prepost-body-fenced",
                 "bind audit marks root body as fenced source inventory");

  ok &= contains(object_cs, "publicenumNodeType:int{Int=0x00,Float=0x01",
                 "ObjectFields exposes DTB node enum");
  ok &= contains(object_cs, "uintcombinedRevision=reader.ReadUInt32();",
                 "ObjectFields reads combined low/high revision");
  ok &= contains(object_cs, "type=Symbol.Read(reader);root.Read(reader);",
                 "ObjectFields reads subtype Symbol and root DTB parent");
  ok &= contains(object_cs,
                 "hasTree=reader.ReadBoolean();if(!hasTree)return;"
                 "childCount=reader.ReadUInt16();id=reader.ReadUInt32();",
                 "ObjectFields reads root tree presence and child metadata");
  ok &= contains(object_cs, "if(revision>0){note=Symbol.Read(reader);}",
                 "ObjectFields reads revision-gated note Symbol");

  for (const char* type_case :
       {"case0x00:", "case0x01:", "case0x02:", "case0x04:",
        "case0x05:", "case0x06:", "case0x07:", "case0x08:",
        "case0x09:", "case0x10:", "case0x11:", "case0x12:",
        "case0x13:", "case0x20:", "case0x21:", "case0x22:",
        "case0x23:", "case0x24:", "case0x25:"}) {
    ok &= contains(char_mesh, type_case,
                   std::string("character DTB skip handles ") + type_case);
    ok &= contains(scene, type_case,
                   std::string("scene DTB skip handles ") + type_case);
  }
  ok &= contains(char_mesh,
                 "constuint32_tcombined_revision=r.u32();constuint16_trevision="
                 "static_cast<uint16_t>(combined_revision&0xffffu);(void)r.str();"
                 "read_dtb_parent(r);if(revision>0)(void)r.str();",
                 "character ObjectFields mirrors MiloEditor order");
  ok &= contains(scene,
                 "constuint32_tcombined_revision=r.u32();constuint16_trevision="
                 "static_cast<uint16_t>(combined_revision&0xffffu);(void)r.str();"
                 "read_dtb_parent(r);if(revision>0)(void)r.str();",
                 "scene ObjectFields mirrors MiloEditor order");

  ok &= contains(trans_cs,
                 "localXfm=localXfm.Read(reader);worldXfm=worldXfm.Read(reader);"
                 "if(revision<9)",
                 "RndTrans source local/world/legacy-child order");
  ok &= contains(char_mesh,
                 "out.local=r.matrix();out.world=r.matrix();if(ver<9)",
                 "character RndTrans local/world/legacy-child order");
  ok &= contains(scene, "out.local=r.matrix();",
                 "scene RndTrans reads local matrix");
  ok &= contains(scene, "out.world=r.matrix();",
                 "scene RndTrans reads world matrix");
  ok &= contains(scene, "if(ver<9)",
                 "scene RndTrans reads legacy child refs after matrices");
  ok &= contains(rb3_trans_h,
                 "enumConstraint{kNone=0,kLocalRotate=1,kParentWorld=2,"
                 "kLookAtTarget=3,kShadowTarget=4,kBillboardZ=5,"
                 "kBillboardXZ=6,kBillboardXYZ=7,kFastBillboardXYZ=8,"
                 "kTargetWorld=9};",
                 "RB3 RndTransformable runtime constraint enum");
  ok &= contains(rb3_trans_h,
                 "boolHasDynamicConstraint(){boolret=true;if(mConstraint<"
                 "kBillboardZ){boolret2=false;if(mConstraint>=kLookAtTarget&&"
                 "mTarget)ret2=true;if(!ret2)ret=false;}returnret;}",
                 "RB3 RndTransformable dynamic-constraint gate");
  ok &= contains(rb3_trans_cpp,
                 "if(!mParent){mWorldXfm=mLocalXfm;}elseif(mConstraint=="
                 "kParentWorld){mWorldXfm=mParent->WorldXfm();}elseif("
                 "mConstraint==kLocalRotate){Multiply(mLocalXfm.v,mParent->"
                 "WorldXfm(),mWorldXfm.v);mWorldXfm.m=mLocalXfm.m;}else{"
                 "Multiply(mLocalXfm,mParent->WorldXfm(),mWorldXfm);}if("
                 "HasDynamicConstraint())ApplyDynamicConstraint();else"
                 "UpdatedWorldXfm();",
                 "RB3 RndTransformable WorldXfm_Force composition");
  ok &= contains(rb3_trans_cpp,
                 "if(mConstraint==kTargetWorld){mWorldXfm=mTarget->WorldXfm();}",
                 "RB3 RndTransformable target-world dynamic constraint");
  ok &= contains(char_mesh,
                 "if(xfm.constraint==2){//kParentWorldreturnparent_world;}",
                 "native transform evaluator mirrors kParentWorld");
  ok &= contains(char_mesh,
                 "if(xfm.constraint==1){//kLocalRotateautoworld=local_mat;"
                 "constautopos="
                 "transform_pos(local,parent_world);world[12]=pos[0];"
                 "world[13]=pos[1];world[14]=pos[2];returnworld;}",
                 "native transform evaluator mirrors kLocalRotate");
  ok &= contains(char_mesh,
                 "autoworld=mat4_mul(local_mat,parent_world);if(xfm.constraint"
                 "==9&&!xfm.target.empty()){//kTargetWorldworld=source_world_for"
                 "(c,xfm.target,",
                 "native transform evaluator mirrors kTargetWorld replacement");
  ok &= contains(char_mesh,
                 "boolsource_dynamic_constraint_needs_runtime(uint32_tconstraint,"
                 "conststd::string&target)",
                 "native detects unsupported dynamic constraints");
  ok &= contains(char_mesh,
                 "if(constraint==9)returntarget.empty();",
                 "native treats target-world without target as unsupported");
  ok &= contains(char_mesh,
                 "returnconstraint>=3&&constraint<=8;",
                 "native treats non-target dynamic constraints as unsupported");
  ok &= contains(char_mesh,
                 "\"[source-xfm-unsupported]name=%sconstraint=%utarget=%s\"",
                 "native logs unsupported dynamic constraints");
  ok &= contains(char_mesh,
                 "runtimeWriteback=0reason=awaiting-source-dynamic-constraint-port",
                 "unsupported dynamic constraint log is source-boundary diagnostic");
  ok &= contains(doc,
                 "Other dynamic constraints log\n    `[source-xfm-unsupported]` "
                 "with `runtimeWriteback=0`",
                 "document records unsupported dynamic constraint boundary");

  ok &= contains(drawable_cs,
                 "showing=reader.ReadBoolean();if(revision<2)",
                 "RndDrawable source starts with showing flag");
  ok &= contains(drawable_cs,
                 "if(revision>2){drawOrder=reader.ReadFloat();}",
                 "RndDrawable source draw-order gate");

  ok &= contains(mat_cs,
                 "useEnviron=reader.ReadBoolean();preLit=reader.ReadBoolean();"
                 "zMode=(ZMode)reader.ReadInt32();alphaCut=reader.ReadBoolean();",
                 "RndMat source useEnviron/preLit/render-state order");
  ok &= contains(rb3_mat_cpp,
                 "mBlend(kSrc),mTexGen(kTexGenNone),mTexWrap(kRepeat),"
                 "mZMode(kNormal)",
                 "RB3 RndMat runtime defaults source blend/z/wrap state");
  ok &= contains(rb3_mat_cpp,
                 "LOAD_BITFIELD_ENUM(int,mBlend,Blend)bs>>mColor;"
                 "LOAD_BITFIELD(bool,mUseEnviron)LOAD_BITFIELD(bool,mPreLit)"
                 "LOAD_BITFIELD_ENUM(int,mZMode,ZMode)",
                 "RB3 RndMat runtime load order matches decoded render state");
  ok &= contains(rb3_mat_h,
                 "BlendGetBlend()const{returnmBlend;}ZModeGetZMode()const{"
                 "returnmZMode;}",
                 "RB3 RndMat exposes source blend and z mode getters");
  ok &= contains(scene,
                 "m.use_environ=r.u8()!=0;m.prelit=r.u8()!=0;"
                 "constint32_tz_mode=r.i32();",
                 "native Mat decode follows source useEnviron/preLit order");

  ok &= contains(group_cs,
                 "anim=newRndAnimatable().Read(reader,parent,entry);"
                 "trans=newRndTrans().Read(reader,false,parent,entry);"
                 "draw=newRndDrawable().Read(reader,false,parent,entry);",
                 "RndGroup source reads anim/trans/draw bases before objects");
  ok &= contains(group_cs,
                 "objectsCount=reader.ReadUInt32();for(inti=0;i<objectsCount;i++){"
                 "objects.Add(Symbol.Read(reader));}",
                 "RndGroup source reads explicit object Symbol list");
  ok &= contains(scene,
                 "GroupObjdecode_group(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "native exposes source-backed Group decoder");
  ok &= contains(scene,
                 "group.children.push_back(r.str());",
                 "native Group decoder reads explicit object Symbol list");
  ok &= contains(char_mesh,
                 "milo_scene::GroupObjgroup=milo_scene::decode_group(de.name,b);",
                 "character load uses source-backed Group decoder");
  ok &= missing(char_mesh, "group_child_refs",
                "character load must not scan Group strings for membership");

  ok &= contains(mesh_cs,
                 "base.Read(reader,false,parent,entry);trans=trans.Read(reader,false,parent,entry);"
                 "draw=draw.Read(reader,false,parent,entry);mat=Symbol.Read(reader);",
                 "RndMesh source superclass/read order");
  ok &= contains(mesh_cs,
                 "if(reader.ReadInt32()>0){reader.BaseStream.Position-=4;",
                 "RndMesh source bone-transform presence gate");
  ok &= contains(mesh_cs,
                 "for(inti=0;i<4;i++){boneTransforms.Add(newBoneTransform());"
                 "boneTransforms[i].name=Symbol.Read(reader);}for(inti=0;i<4;i++){"
                 "boneTransforms[i].transform=boneTransforms[i].transform.Read(reader);}",
                 "RndMesh rev<33 old-style four names then four transforms");
  ok &= contains(mesh_cs,
                 "publicclassGroupSection{publicList<int>sections=new();"
                 "publicList<ushort>vertOffsets=new();publicGroupSectionRead("
                 "EndianReaderreader,uintmeshRevision){uintsectionCount="
                 "reader.ReadUInt32();uintvertCount=reader.ReadUInt32();",
                 "RndMesh source group-section row schema");
  ok &= contains(mesh_cs,
                 "if(groupSizesCount>0&&groupSizes[0]>0&&parent.revision<25)"
                 "{for(inti=0;i<groupSizesCount;i++){GroupSectionsection="
                 "newGroupSection();groupSections.Add(section.Read(reader,"
                 "revision));}}",
                 "RndMesh source last-gen group-section gate");
  ok &= contains(char_mesh,
                 "constint32_tfirst_bone_len=r.i32();if(first_bone_len>0){"
                 "r.pos=bone_probe;",
                 "native RndMesh keeps source bone-transform presence gate");
  ok &= contains(char_mesh,
                 "for(intbi=0;bi<4;++bi){mesh.bone_palette.push_back(r.str());}"
                 "for(intbi=0;bi<4;++bi){mesh.bind.push_back(r.matrix());}",
                 "native keeps GH2 four source palette slots and four offsets");
  ok &= contains(char_mesh_h,
                 "structRndMeshGroupSection{std::vector<int32_t>sections;"
                 "std::vector<uint16_t>vert_offsets;};",
                 "native exposes RndMesh GroupSection rows");
  ok &= contains(char_mesh,
                 "if(!mesh.group_sizes.empty()&&mesh.group_sizes[0]>0&&"
                 "parent_dir_revision<25){",
                 "native follows source last-gen group-section gate");
  ok &= contains(char_mesh,
                 "group_section.sections.push_back(r.i32());",
                 "native reads signed group-section section indices");
  ok &= contains(char_mesh,
                 "group_section.vert_offsets.push_back(r.u16());",
                 "native reads group-section vertex offsets");
  ok &= contains(char_mesh,
                 "decode_skinned_mesh(de.name,b,dir.dir_version);",
                 "native passes source parent dir revision into Mesh decoder");
  ok &= missing(char_mesh, "erase(std::remove",
                "native must not trim empty source palette rows");
  ok &= missing(renderer, "is_terminal_leg_overlay_duplicate",
                "renderer must not hide meshes through invented leg duplicate rule");
  ok &= missing(renderer, "is_hidden_numbered_hair_variant",
                "renderer must not hide hair through numbered-name fallback");
  ok &= missing(renderer, "is_lod1",
                "renderer must not hide LOD meshes through name fallback");
  ok &= missing(renderer, "legacy_blended_hair",
                "renderer must not keep legacy hair depth fallback");
  ok &= missing(renderer, "hairRender",
                "renderer debug output must not expose removed hair-name branch");
  ok &= contains(renderer,
                 "boolis_hair_two_sided_surface(constSkinnedMesh*mesh,"
                 "constghogx::milo_scene::MatObj*material=nullptr)",
                 "renderer has the explicit project hair two-sided rule");
  ok &= contains(renderer,
                 "has_hair_token(mesh->name)||has_hair_token(mesh->material)",
                 "hair two-sided rule catches mesh and mesh-material tokens");
  ok &= contains(renderer,
                 "has_hair_token(material->name)||"
                 "has_hair_token(material->diffuse_tex)",
                 "hair two-sided rule catches material and texture tokens");
  ok &= contains(renderer,
                 "constDWORDmesh_cull_mode=hair_two_sided?D3DCULL_NONE:"
                 "character_cull_mode(material);",
                 "hair surfaces are marked two-sided only at cull selection");
  ok &= missing(renderer,
                "is_hair_two_sided_surface(mesh,material)){returnD3DCULL_NONE;}",
                "generic cull helper must not keep a hidden hair override");
  ok &= contains(renderer,
                 "if(hair_two_sided){dev->SetRenderState(D3DRS_CULLMODE,"
                 "D3DCULL_CCW);draw_current_mesh();dev->SetRenderState("
                 "D3DRS_CULLMODE,D3DCULL_CW);draw_current_mesh();"
                 "dev->SetRenderState(D3DRS_CULLMODE,mesh_cull_mode);}"
                 "else{draw_current_mesh();}",
                 "hair two-sided rule draws both cull sides without material-state overrides");
  ok &= contains(renderer, "hairTwoSided=%d",
                 "mesh render logs expose the hair two-sided rule");
  ok &= contains(renderer,
                 "constbooldepth_write=material_depth_write_enabled(material);",
                 "native depth write is driven by source material state");
  ok &= contains(renderer,
                 "if(std::fabs(a->draw_order-b->draw_order)>1.0e-5f){"
                 "returna->draw_order<b->draw_order;}",
                 "native draw sort uses source RndDrawable draw order without hair names");
  ok &= contains(renderer,
                 "returnis_hidden_by_character_lod_group(character,mesh);",
                 "native LOD visibility falls back only to source group membership");

  ok &= contains(gltf_program_cs,
                 "boneName.StartsWith(\"bone_hair_\",StringComparison.OrdinalIgnoreCase)",
                 "glTFMilo current hair-bone naming rule");
  ok &= contains(gltf_program_cs,
                 "varrelativeTransform=boneWorldInverse*node.WorldMatrix;"
                 "MatrixHelpers.CopyMatrix(relativeTransform,miloBoneTransform.transform,"
                 "convertCoordinates);",
                 "glTFMilo writes inverse bone world times mesh world");
  ok &= contains(rb3_mesh_cpp,
                 "Invert(t->WorldXfm(),tf48);Multiply(WorldXfm(),tf48,"
                 "mBones[i].mOffset);",
                 "RB3 runtime SetBone stores mesh world times inverse bone world");
  ok &= contains(rb3_mesh_cpp,
                 "bs>>mBones[0].mOffset>>mBones[1].mOffset>>"
                 "mBones[2].mOffset>>mBones[3].mOffset;",
                 "RB3 runtime reads GH2-era four source offsets");
  ok &= contains(doc, "vertex * storedOffset *\n    currentBoneWorld",
                 "document states native source-offset consumption order");
  ok &= contains(renderer,
                 "skin[i]=mul16(xfm16(mesh.bind[i]),curr_world);",
                 "native renderer consumes source offset then current transform");
  ok &= contains(renderer, "!character.has_transform(mesh.bone_palette[i])",
                 "native renderer skips unresolved source slots");

  ok &= contains(gltf_node_processor_cs, "CollectHairChainsSplitAtBranches",
                 "glTFMilo current hair strand splitter is visible");
  ok &= contains(gltf_node_processor_cs,
                 "strand.root=chain[0].Name;MatrixHelpers.CopyMatrix3("
                 "chain[0].LocalMatrix,strand.baseMat,convertCoordinates);"
                 "MatrixHelpers.CopyMatrix3(chain[0].LocalMatrix,strand.rootMat,"
                 "convertCoordinates);",
                 "glTFMilo CharHair strand root/base matrices");
  ok &= contains(gltf_node_processor_cs,
                 "point.bone=chainNode.Name;point.pos=ToMiloVector3(pointPosition);"
                 "point.unk5c=ToMiloVector3(resetPosition);point.sideLength=-1.0f;",
                 "glTFMilo CharHair point fields");
  ok &= contains(gltf_node_processor_cs,
                 "createaCharCollideforthehaireventhoughitisempty,"
                 "fromlookingatthedecompitseemedthattheremustbeoneorhairwon'tbesim,"
                 "couldbewrong",
                 "glTFMilo marks generated CharCollide rows as inferred");
  ok &= contains(doc,
                 "Treat those rows as exporter/format hints, not proof of GH2 runtime",
                 "document keeps glTFMilo CharCollide rows out of runtime proof");
  ok &= contains(band3_config, "CharHair__GetFPS",
                 "band3_recomp exposes CharHair GetFPS symbol");
  ok &= contains(band3_config, "CharHair__Simulate",
                 "band3_recomp exposes CharHair Simulate symbol");
  ok &= missing(band3_config, "CharHair__Hookup",
                "band3_recomp has no CharHair Hookup symbol body");
  ok &= missing(band3_config, "CharCollide__",
                "band3_recomp has no CharCollide implementation symbols");

  ok &= contains(rb3_latest_char_hair_cpp, "pt.radius+=f;pt.outerRadius+=f;",
                 "RB3 CharHair source adds rev 6/7/8 float to both radii");
  ok &= contains(char_mesh,
                 "point.radius+=add_to_radius;point.outer_radius+=add_to_radius;",
                 "native CharHair decode follows rev 6/7/8 radius addition");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev<8){pt.sideLength=-1.0f;if(CharHair::gRev>5){"
                 "inti;bs>>i>>i;}}",
                 "RB3 CharHair source consumes two ints for old revs above 5");
  ok &= contains(char_mesh,
                 "if(hair.version<8){point.side_length=-1.0f;if(hair.version>5){"
                 "(void)r.i32();(void)r.i32();}}",
                 "native CharHair decode consumes two ints for old revs above 5");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(gRev<8){mMinSlack=0.0f;mMaxSlack=0.0f;}"
                 "elsebs>>mMinSlack>>mMaxSlack;bs>>mStrands;",
                 "RB3 CharHair Load reads slack from rev 8");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(11,0);Hmx::Object::Load(bs);",
                 "RB3 CharHair Load accepts source revisions through 11");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "bs>>mStrands;bs>>mSimulate;if(gRev>10)bs>>mWind;",
                 "RB3 CharHair Load reads simulate and rev-11 wind tail");
  ok &= contains(char_mesh,
                 "if(hair.version<0||hair.version>11){"
                 "throwstd::runtime_error(\"char_mesh:CharHairrevision"
                 "outsidesourcerange\");}",
                 "native CharHair decode validates source revision range");
  ok &= contains(char_mesh,
                 "if(hair.version>=8){hair.min_slack=r.f32();"
                 "hair.max_slack=r.f32();}",
                 "native CharHair decode reads slack from rev 8");
  ok &= contains(char_mesh,
                 "hair.simulate=r.u8()!=0;if(hair.version>10)"
                 "hair.wind=r.str();",
                 "native CharHair decode reads required simulate and rev-11 wind tail");
  ok &= contains(char_mesh_h,
                 "CharHairdecode_hair(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body);",
                 "native exposes CharHair row decoder for deterministic tests");
  ok &= contains(mesh_decode_test,
                 "make_rev8_hair_without_strands()",
                 "deterministic test builds source rev-8 CharHair row");
  ok &= contains(mesh_decode_test,
                 "CHECK(approx(rev8_hair.min_slack,0.25f));",
                 "deterministic test verifies rev-8 CharHair min slack");
  ok &= contains(mesh_decode_test,
                 "CHECK(approx(rev8_hair.max_slack,0.75f));",
                 "deterministic test verifies rev-8 CharHair max slack");
  ok &= contains(mesh_decode_test,
                 "CHECK(rev11_hair.wind==\"stage.wind\");",
                 "deterministic test verifies rev-11 CharHair wind");
  ok &= contains(mesh_decode_test,
                 "CHECK(bad_version_threw);",
                 "deterministic test verifies CharHair revision range");
  ok &= contains(doc,
                 "always reads `simulate` after the\n    strand list",
                 "document records CharHair Load tail gates");
  ok &= contains(char_mesh_h, "std::stringwind;size_tunread_bytes=0;",
                 "native CharHair row records unread byte count");
  ok &= contains(char_mesh_h, "std::stringunread_tail_hex;",
                 "native CharHair row records unread byte proof");
  ok &= contains(char_mesh,
                 "hair.unread_bytes=r.n-r.pos;",
                 "native CharHair decoder records unread byte count");
  ok &= contains(char_mesh,
                 "hair.unread_tail_hex=hex_bytes(r.p+r.pos,"
                 "std::min<size_t>(hair.unread_bytes,32));",
                 "native CharHair decoder records unread tail proof");
  ok &= contains(bind_audit, "missingBonePoints=%zu",
                 "hair audit summarizes missing driven bones");
  ok &= contains(bind_audit, "missingCollisionRefs=%zu",
                 "hair audit summarizes missing collision targets");
  ok &= contains(bind_audit, "sideLengthPoints=%zu",
                 "hair audit summarizes source side-length fields");
  ok &= contains(bind_audit, "unk5cPoints=%zu",
                 "hair audit summarizes source unk5c fields");
  ok &= contains(bind_audit, "unreadBytes=%zu",
                 "hair audit summarizes unread source tails");
  ok &= contains(doc,
                 "Native hair audits now summarize each decoded `CharHair` row",
                 "document records hair digest inventory");
  ok &= contains(doc,
                 "This is diagnostic\n  inventory only; it does not publish "
                 "guessed hair physics or placement.",
                 "document fences hair digest away from guessed runtime behavior");
  ok &= contains(rb3_latest_char_hair_cpp, "pt.collides.clear();",
                 "RB3 CharHair point reader clears decoded collision list");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev<3){inti;charbuf[0x100];bs>>i;"
                 "bs.ReadString(buf,0xff);}elseif(CharHair::gRev==3){"
                 "inti;bs>>i;}",
                 "RB3 CharHair source consumes legacy inline collision fields");
  ok &= contains(char_mesh,
                 "if(hair.version<3){point.collide_type=r.u32();"
                 "point.collision=r.str();}elseif(hair.version==3){"
                 "point.collide_type=r.u32();}",
                 "native CharHair decode logs legacy inline fields only");
  ok &= contains(doc,
                 "Native may\n    log these legacy inline fields for stock GH2 "
                 "evidence, but they are not a\n    resolved runtime "
                 "`ObjPtrList<CharCollide>`",
                 "document fences legacy inline hair collision fields");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Poll(){if(mMe){if(mMe->GetPollState()=="
                 "Character::kCharSyncObject)Hookup();",
                 "RB3 CharHair poll re-hooks during character sync");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Enter(){mReset=1;RndPollable::Enter();"
                 "Hookup();}",
                 "RB3 CharHair Enter source resets and hooks up");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(mReset>0)DoReset(mReset);if(TheTaskMgr.DeltaSeconds()!="
                 "0.0f){SimulateLoops(1,GetFPS());}elseSimulateZeroTime();",
                 "RB3 CharHair poll reset/simulate flow");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairPollDecision{boolhookup=false;"
                 "boolteleported_reset=false;booldo_reset=false;"
                 "intreset_count=0;boolreturn_after_reset=false;"
                 "boolsimulate_loops=false;boolsimulate_zero_time=false;"
                 "intnext_reset=0;};",
                 "native exposes source CharHair Poll decision row");
  ok &= contains(char_mesh_h,
                 "SourceCharHairPollDecisionsource_char_hair_poll_decision("
                 "boolowner_is_character,boolcharacter_syncing,"
                 "boolcharacter_teleported,intcharacter_min_lod,"
                 "intcurrent_reset,floatdelta_seconds);",
                 "native exposes source CharHair Poll decision helper");
  ok &= contains(char_mesh,
                 "SourceCharHairPollDecisionsource_char_hair_poll_decision("
                 "boolowner_is_character,boolcharacter_syncing,"
                 "boolcharacter_teleported,intcharacter_min_lod,"
                 "intcurrent_reset,floatdelta_seconds){",
                 "native implements source CharHair Poll decision helper");
  ok &= contains(char_mesh,
                 "decision.hookup=character_syncing;if(character_teleported){"
                 "reset=1;decision.teleported_reset=true;}if("
                 "character_min_lod>0){decision.do_reset=true;"
                 "decision.reset_count=0;decision.return_after_reset=true;",
                 "native CharHair Poll helper ports character-owner branches");
  ok &= contains(char_mesh,
                 "if(reset>0){decision.do_reset=true;decision.reset_count="
                 "reset;reset=0;}if(delta_seconds!=0.0f){decision."
                 "simulate_loops=true;}else{decision.simulate_zero_time=true;}",
                 "native CharHair Poll helper ports reset and delta branches");
  ok &= contains(doc,
                 "Native `source_char_hair_poll_decision` ports this branch order",
                 "document records native CharHair Poll decision helper");
  ok &= contains(doc,
                 "`CharHair::Enter` sets `mReset = 1`",
                 "document records native CharHair Enter plan helper");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "Multiply(pt.unk5c,tf70,pt.pos);",
                 "RB3 CharHair reset seeds point position from unk5c");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "booltmpsim=mSimulate;floattmpinert=mInertia;"
                 "floattmpfric=mFriction;mSimulate=true;mInertia=0;"
                 "mFriction=0;SimulateLoops(reset,GetFPS());mSimulate=tmpsim;"
                 "mFriction=tmpfric;mInertia=tmpinert;mReset=0;",
                 "RB3 CharHair DoReset temporarily forces simulation");
  ok &= contains(char_clip,
                 "if((!force_simulate&&!hair.simulate)||hair.strands.empty())"
                 "return0;",
                 "native CharHair simulate loop has source reset force lane");
  ok &= contains(char_clip,
                 "source_char_hair_simulate_loops(character,hair,state,"
                 "std::max(reset_count,0),source_char_hair_get_fps("
                 "state.use_post_proc,0.0f),"
                 "0.0f,0.0f,true);",
                 "native CharHair DoReset forces source simulate lane");
  ok &= contains(char_clip,
                 "source_char_hair_get_fps(state.use_post_proc,0.0f),"
                 "hair.inertia,hair.friction",
                 "native CharHair Poll uses source GetFPS helper");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_char_hair_source_test",
                 "CMake builds CharHair source test");
  ok &= contains(char_hair_source_test,
                 "hair.simulate=false;",
                 "deterministic CharHair test starts with disabled simulate flag");
  ok &= contains(char_hair_source_test,
                 "apply_character_controllers(character,0.0f,nullptr);",
                 "deterministic CharHair test exercises public controller path");
  ok &= contains(char_hair_source_test,
                 "ok&=state.use_post_proc;",
                 "deterministic CharHair test proves Character owner enables postproc FPS path");
  ok &= contains(char_hair_source_test,
                 "point.pos[2]<-1.9f&&point.pos[2]>-2.1f",
                 "deterministic CharHair test proves reset forced simulation");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_poll_decision(true,true,false,0,0,0.25f)",
                 "focused CharHair source test covers Poll syncing branch");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_poll_decision(true,false,true,0,0,0.25f)",
                 "focused CharHair source test covers Poll teleport branch");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_poll_decision(true,false,false,1,3,0.25f)",
                 "focused CharHair source test covers Poll LOD branch");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_poll_decision(false,false,false,0,0,0.0f)",
                 "focused CharHair source test covers Poll zero-time branch");
  ok &= contains(doc,
                 "Native reset follows that\n    forced-simulate lane",
                 "document records native CharHair reset forced simulation");
  ok &= contains(doc,
                 "Native `source_char_hair_freeze_pose_plan` ports the call order",
                 "document records native CharHair FreezePose plan helper");
  ok &= contains(doc,
                 "`source_char_hair_freeze_pose_raw` ports the raw\n"
                 "    local-row write",
                 "document records native CharHair FreezePoseRaw helper");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "SimulateLoops(reset,GetFPS());",
                 "RB3 CharHair reset runs source simulate loops");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "Multiply(pts[j].pos,tf48,pts[j].unk5c);",
                 "RB3 CharHair FreezePoseRaw writes root-parent local point rows");
  ok &= contains(char_clip_h,
                 "intsource_char_hair_freeze_pose_raw(Character&character,"
                 "CharHair&hair,SourceCharHairRuntime&state);",
                 "native exposes CharHair FreezePoseRaw source helper");
  ok &= contains(char_clip,
                 "intsource_char_hair_freeze_pose_raw(Character&character,"
                 "CharHair&hair,SourceCharHairRuntime&state){",
                 "native CharHair FreezePoseRaw helper exists");
  ok &= contains(char_clip,
                 "conststd::array<float,16>parent_inverse=affine_inverse("
                 "parent_world);",
                 "native CharHair FreezePoseRaw inverts root parent world");
  ok &= contains(char_clip,
                 "source_transform_point(vec_from_array3(runtime_strand.points[pi].pos),"
                 "parent_inverse);strand.points[pi].unk5c[0]=local.x;",
                 "native CharHair FreezePoseRaw writes local unk5c rows");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_freeze_pose_raw(freeze_character,freeze_hair,"
                 "freeze_state);",
                 "deterministic CharHair test calls FreezePoseRaw helper");
  ok &= contains(char_hair_source_test,
                 "freeze_hair.strands[0].points[0].unk5c[2],4.0f",
                 "deterministic CharHair test proves FreezePoseRaw local write");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::FreezePose(){booltmpsim=mSimulate;Hookup();"
                 "SimulateLoops(200,60.0f);mSimulate=tmpsim;FreezePoseRaw();}",
                 "RB3 CharHair FreezePose source path hooks, simulates, restores, freezes");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::SetName(constchar*cc,ObjectDir*dir){"
                 "Hmx::Object::SetName(cc,dir);mMe=dynamic_cast<Character*>(dir);"
                 "boolpp=false;if(mMe||dynamic_cast<WorldDir*>(dir))pp=true;"
                 "mUsePostProc=pp;}",
                 "RB3 CharHair SetName source detects Character/WorldDir owners");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "floatCharHair::GetFPS(){if(mUsePostProc&&RndPostProc::Current()"
                 "&&RndPostProc::Current()->EmulateFPS()>0){floatret="
                 "RndPostProc::Current()->EmulateFPS();if(ret!=60.0f)"
                 "ret=60.0f-ret;returnret;}elsereturn60.0f;}",
                 "RB3 CharHair GetFPS source uses post-process FPS emulation");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "CharHair::CharHair():mStiffness(0.04f),mTorsion(0.1f),"
                 "mInertia(0.7f),mGravity(1.0f),mWeight(0.5f),"
                 "mFriction(0.3f),mMinSlack(0.0f),mMaxSlack(0.0f),",
                 "RB3 CharHair constructor exposes source defaults");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "mStrands(this),mReset(1),mSimulate(1),mUsePostProc(1),"
                 "mMe(this,0),mWind(this,0),mCollide(this,kObjListNoNull),"
                 "mManagedHookup(0){}",
                 "RB3 CharHair constructor exposes runtime default flags");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "BinStream&operator>>(BinStream&bs,CharHair::Point&pt){"
                 "bs>>pt.pos;bs>>pt.bone;bs>>pt.length;",
                 "RB3 CharHair Point load source reads position, bone, length");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev<3){inti;charbuf[0x100];bs>>i;"
                 "bs.ReadString(buf,0xff);}elseif(CharHair::gRev==3){",
                 "RB3 CharHair Point load source reads legacy collision rows");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev==6||CharHair::gRev==7||"
                 "CharHair::gRev==8){floatf;bs>>f;pt.radius+=f;"
                 "pt.outerRadius+=f;}",
                 "RB3 CharHair Point load source adds radius for revs 6-8");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev<8){pt.sideLength=-1.0f;"
                 "if(CharHair::gRev>5){inti;bs>>i>>i;}}else{",
                 "RB3 CharHair Point load source handles legacy side length");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev>9){bs>>pt.unk5c;}pt.collides.clear();"
                 "pt.force.Zero();pt.lastFriction.Zero();pt.lastZ.Zero();}",
                 "RB3 CharHair Point load source clears runtime point fields");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Strand::Load(BinStream&bs){bs>>mRoot;"
                 "bs>>mAngle;bs>>mPoints;bs>>mBaseMat>>mRootMat;",
                 "RB3 CharHair Strand load source reads root angle points matrices");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(CharHair::gRev>2){bs>>mHookupFlags;}elsemHookupFlags=0;}",
                 "RB3 CharHair Strand load source gates hookup flags");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(11,0);Hmx::Object::Load(bs);"
                 "bs>>mStiffness>>mTorsion>>mInertia>>mGravity>>mWeight>>"
                 "mFriction;",
                 "RB3 CharHair load source reads object and core floats");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(gRev<8){mMinSlack=0.0f;mMaxSlack=0.0f;}elsebs>>"
                 "mMinSlack>>mMaxSlack;bs>>mStrands;bs>>mSimulate;"
                 "if(gRev>10)bs>>mWind;}",
                 "RB3 CharHair load source gates slack and wind reads");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairDefaultState{floatstiffness=0.04f;"
                 "floattorsion=0.1f;floatinertia=0.7f;",
                 "native exposes source CharHair default state");
  ok &= contains(char_mesh_h,
                 "booluse_post_proc=true;boolmanaged_hookup=false;};",
                 "native exposes source CharHair default runtime flags");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairRuntime{boolinitialized=false;"
                 "booluse_post_proc=true;",
                 "native carries CharHair SetName postproc state into runtime");
  ok &= contains(char_mesh,
                 "SourceCharHairDefaultStatesource_char_hair_default_state(){"
                 "returnSourceCharHairDefaultState{};}",
                 "native CharHair default helper returns source defaults");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairPointLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;",
                 "native exposes CharHair Point load plan row");
  ok &= contains(char_mesh_h,
                 "SourceCharHairPointLoadPlansource_char_hair_point_load_plan("
                 "intrevision);",
                 "native exposes CharHair Point load plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharHairStrandLoadPlansource_char_hair_strand_load_plan("
                 "intrevision);",
                 "native exposes CharHair Strand load plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharHairLoadPlansource_char_hair_load_plan("
                 "intrevision);",
                 "native exposes CharHair load plan helper");
  ok &= contains(char_mesh,
                 "SourceCharHairPointLoadPlansource_char_hair_point_load_plan("
                 "intrevision){SourceCharHairPointLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=11;",
                 "native CharHair Point load plan ports source revision gate");
  ok &= contains(char_mesh,
                 "if(revision<3){plan.read_order.push_back("
                 "\"legacyCollideType\");plan.read_order.push_back("
                 "\"legacyCollisionName\");}elseif(revision==3){",
                 "native CharHair Point load plan ports legacy collision rows");
  ok &= contains(char_mesh,
                 "if(revision==6||revision==7||revision==8){"
                 "plan.read_order.push_back(\"addToRadius\");"
                 "plan.branches.push_back("
                 "\"addToRadiusAppliesToRadiusAndOuterRadius\");}",
                 "native CharHair Point load plan ports add-to-radius gate");
  ok &= contains(char_mesh,
                 "if(revision<8){plan.branches.push_back(\"sideLength=-1\");"
                 "if(revision>5){plan.read_order.push_back("
                 "\"legacySideLengthInt0\");",
                 "native CharHair Point load plan ports legacy side-length rows");
  ok &= contains(char_mesh,
                 "if(revision>9)plan.read_order.push_back(\"unk5c\");"
                 "plan.branches.push_back(\"clearCollides\");"
                 "plan.branches.push_back(\"zeroForce\");",
                 "native CharHair Point load plan ports runtime reset rows");
  ok &= contains(char_mesh,
                 "SourceCharHairStrandLoadPlansource_char_hair_strand_load_plan("
                 "intrevision){SourceCharHairStrandLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=11;",
                 "native CharHair Strand load plan ports source revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order={\"mRoot\",\"mAngle\",\"mPoints\","
                 "\"mBaseMat\",\"mRootMat\"};if(revision>2){",
                 "native CharHair Strand load plan ports source read order");
  ok &= contains(char_mesh,
                 "SourceCharHairLoadPlansource_char_hair_load_plan("
                 "intrevision){SourceCharHairLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=11;",
                 "native CharHair load plan ports source revision gate");
  ok &= contains(char_mesh,
                 "if(revision<8){plan.branches.push_back(\"mMinSlack=0\");"
                 "plan.branches.push_back(\"mMaxSlack=0\");}else{",
                 "native CharHair load plan ports slack revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order.push_back(\"mStrands\");"
                 "plan.read_order.push_back(\"mSimulate\");"
                 "if(revision>10)plan.read_order.push_back(\"mWind\");",
                 "native CharHair load plan ports strand/simulate/wind reads");
  ok &= contains(char_mesh_h,
                 "boolsource_char_hair_set_name_use_post_proc("
                 "boolowner_is_character,boolowner_is_world_dir);",
                 "native exposes CharHair SetName ownership helper");
  ok &= contains(char_mesh,
                 "boolsource_char_hair_set_name_use_post_proc("
                 "boolowner_is_character,boolowner_is_world_dir){"
                 "returnowner_is_character||owner_is_world_dir;}",
                 "native CharHair SetName helper follows source branch");
  ok &= contains(char_mesh,
                 "floatsource_char_hair_get_fps(booluse_post_proc,"
                 "floatemulated_fps){if(use_post_proc&&emulated_fps>0.0f){"
                 "floatret=emulated_fps;if(ret!=60.0f)ret=60.0f-ret;"
                 "returnret;}return60.0f;}",
                 "native CharHair GetFPS helper follows source branch");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairHookupPlan{boolreturned_for_managed_hookup="
                 "false;std::vector<std::string>collected_collides;"
                 "boolcalled_overloaded_hookup=false;};",
                 "native exposes CharHair Hookup plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairEnterPlan{intnext_reset=1;"
                 "boolcalled_rnd_pollable_enter=true;"
                 "SourceCharHairHookupPlanhookup;};",
                 "native exposes CharHair Enter plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairSimulateLoopsPlan{boolentered=false;"
                 "intcollide_maintenance_count=0;intsimulate_internal_calls=0;"
                 "floatfps=0.0f;};",
                 "native exposes CharHair SimulateLoops plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairFreezePosePlan{boolcalled_hookup=true;"
                 "SourceCharHairSimulateLoopsPlansimulate_loops;"
                 "boolrestored_simulate=true;",
                 "native exposes CharHair FreezePose plan");
  ok &= contains(char_mesh,
                 "SourceCharHairHookupPlansource_char_hair_hookup_plan("
                 "boolmanaged_hookup,conststd::vector<std::string>&"
                 "dir_collides){",
                 "native implements CharHair Hookup plan helper");
  ok &= contains(char_mesh,
                 "if(managed_hookup){plan.returned_for_managed_hookup=true;"
                 "returnplan;}plan.collected_collides=dir_collides;"
                 "plan.called_overloaded_hookup=true;",
                 "native CharHair Hookup plan ports managed gate and collection");
  ok &= contains(char_mesh,
                 "SourceCharHairEnterPlansource_char_hair_enter_plan(",
                 "native implements CharHair Enter plan helper");
  ok &= contains(char_mesh,
                 "plan.next_reset=1;plan.called_rnd_pollable_enter=true;",
                 "native CharHair Enter plan records reset and superclass enter");
  ok &= contains(char_mesh,
                 "SourceCharHairSimulateLoopsPlansource_char_hair_simulate_"
                 "loops_plan(boolsimulate,intstrand_count,intcollide_count,"
                 "intloop_count,floatfps){",
                 "native implements CharHair SimulateLoops plan helper");
  ok &= contains(char_mesh,
                 "if(!simulate||strand_count==0)returnplan;plan.entered=true;"
                 "plan.collide_maintenance_count=collide_count>0?collide_count:0;"
                 "plan.simulate_internal_calls=loop_count>0?loop_count:0;",
                 "native CharHair SimulateLoops plan ports source gate");
  ok &= contains(char_mesh,
                 "SourceCharHairFreezePosePlansource_char_hair_freeze_pose_plan(",
                 "native implements CharHair FreezePose plan helper");
  ok &= contains(char_mesh,
                 "source_char_hair_simulate_loops_plan(simulate,strand_count,"
                 "collide_count,200,60.0f)",
                 "native CharHair FreezePose plan ports source loop count");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_default_state();",
                 "deterministic test covers source CharHair defaults");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_set_name_use_post_proc(false,false)",
                 "deterministic test covers CharHair SetName non-owner branch");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_set_name_use_post_proc(true,false)",
                 "deterministic test covers CharHair SetName Character branch");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_set_name_use_post_proc(false,true)",
                 "deterministic test covers CharHair SetName WorldDir branch");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_get_fps(true,20.0f),40.0f",
                 "deterministic test covers source CharHair GetFPS emulation branch");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_enter_plan(false,",
                 "focused CharHair source test covers Enter plan");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_freeze_pose_plan(true,2,3)",
                 "focused CharHair source test covers FreezePose plan");
  ok &= contains(doc,
                 "Native ports the constructor constants, SetName ownership branch, and the\n"
                 "    `GetFPS` branch",
                 "document ties native CharHair defaults/GetFPS to source");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::SimulateLoops(intcount,floatf){if(mSimulate&&"
                 "mStrands.size()!=0){for(ObjPtrList<CharCollide,ObjectDir>"
                 "::iteratorit=mCollide.begin();it!=mCollide.end();++it)",
                 "RB3 CharHair SimulateLoops is gated on simulate and strands");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "for(intn=0;n<count;n++){SimulateInternal(f);}}}",
                 "RB3 CharHair SimulateLoops calls source internal simulation");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Strand::SetRoot(RndTransformable*trans){"
                 "mRoot=trans;if(!mRoot)mPoints.resize(0);else{floatlen="
                 "mPoints.size()!=0?mPoints.back().length:0;mBaseMat="
                 "mRoot->LocalXfm().m;SetAngle(mAngle);",
                 "RB3 CharHair Strand SetRoot source starts from root and cached length");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "while(true){depth++;if(it->TransChildren().empty())break;"
                 "it=it->TransChildren().front();}mPoints.resize(depth);",
                 "RB3 CharHair Strand SetRoot follows first-child chain");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "pt->length=bone->LocalXfm().v.y;pt->pos=bone->WorldXfm().v;",
                 "RB3 CharHair Strand SetRoot seeds point length and position");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(!len){if(pt)len=pt->length;elselen=5.0f;}backpt->length=len;"
                 "ScaleAdd(backpt->bone->WorldXfm().v,backpt->bone->WorldXfm().m.y,"
                 "backpt->length,backpt->pos);",
                 "RB3 CharHair Strand SetRoot terminal length and ScaleAdd fallback");
  ok &= contains(char_mesh_h,
                 "structSourceCharHairRootNode{std::stringbone;"
                 "floatlocal_y=0.0f;std::array<float,3>world_pos=",
                 "native exposes source CharHair SetRoot node rows");
  ok &= contains(char_mesh_h,
                 "voidsource_char_hair_strand_set_root(CharHairStrand&strand,"
                 "conststd::vector<SourceCharHairRootNode>&first_child_chain);",
                 "native exposes source CharHair Strand SetRoot helper");
  ok &= contains(char_mesh,
                 "strand.root=first_child_chain.empty()?\"\":first_child_chain.front().bone;"
                 "if(strand.root.empty()){strand.points.clear();return;}",
                 "native CharHair SetRoot clears points for empty roots");
  ok &= contains(char_mesh,
                 "floatlen=strand.points.empty()?0.0f:strand.points.back().length;",
                 "native CharHair SetRoot preserves previous terminal length");
  ok &= contains(char_mesh,
                 "previous_point->length=bone.local_y;previous_point->pos[0]="
                 "bone.world_pos[0];",
                 "native CharHair SetRoot seeds point length and position");
  ok &= contains(char_mesh,
                 "len=previous_point!=nullptr?previous_point->length:5.0f;",
                 "native CharHair SetRoot preserves source fallback length");
  ok &= contains(char_mesh,
                 "back_point.pos[0]=back_bone.world_pos[0]+"
                 "back_bone.world_y_axis[0]*len;",
                 "native CharHair SetRoot applies source ScaleAdd terminal point");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_strand_set_root(root_strand,chain);",
                 "deterministic test covers CharHair Strand SetRoot chain");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_strand_set_root(preserved_len_strand,"
                 "single_root);",
                 "deterministic test covers CharHair Strand SetRoot preserved length");
  ok &= contains(doc,
                 "Native ports this as\n    `source_char_hair_strand_set_root`",
                 "document ties native CharHair SetRoot helper to source");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Strand::SetAngle(floatangle){mAngle=angle;"
                 "Hmx::Matrix3m38;m38.RotateAboutX(mAngle*DEG2RAD);"
                 "Multiply(m38,mBaseMat,mRootMat);}",
                 "RB3 CharHair Strand SetAngle source formula");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "CharHair::Strand::Strand(Hmx::Object*o):mShowSpheres(0),"
                 "mShowCollide(0),mShowPose(0),mRoot(o,0),mAngle(0.0f),"
                 "mPoints(o),mHookupFlags(0){mBaseMat.Identity();"
                 "mRootMat.Identity();}",
                 "RB3 CharHair Strand constructor initializes matrix defaults");
  ok &= contains(char_mesh_h,
                 "floatbase_mat[9]={1.0f,0.0f,0.0f,0.0f,1.0f,0.0f,"
                 "0.0f,0.0f,1.0f};",
                 "native CharHairStrand base matrix default is identity");
  ok &= contains(char_mesh_h,
                 "floatroot_mat[9]={1.0f,0.0f,0.0f,0.0f,1.0f,0.0f,"
                 "0.0f,0.0f,1.0f};",
                 "native CharHairStrand root matrix default is identity");
  ok &= contains(mesh_decode_test,
                 "constghogx::character::CharHairStranddefault_strand;",
                 "deterministic test covers CharHairStrand default state");
  ok &= contains(doc,
                 "`CharHair::Strand::Strand` initializes `mBaseMat` and `mRootMat` to",
                 "document records CharHair Strand constructor matrix defaults");
  ok &= contains(char_mesh_h,
                 "std::array<float,9>source_char_hair_set_angle_root_mat("
                 "floatangle_degrees,constfloatbase_mat[9]);",
                 "native exposes CharHair SetAngle root matrix helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_hair_strand_set_angle(CharHairStrand&strand,"
                 "floatangle_degrees);",
                 "native exposes CharHair Strand SetAngle helper");
  ok &= contains(char_mesh,
                 "std::array<float,9>source_char_hair_set_angle_root_mat("
                 "floatangle_degrees,constfloatbase_mat[9]){"
                 "constexprfloatkPi=3.14159265358979323846f;",
                 "native CharHair SetAngle helper starts from source degrees");
  ok &= contains(char_mesh,
                 "out[3+col]=c*base_mat[3+col]+s*base_mat[6+col];"
                 "out[6+col]=-s*base_mat[3+col]+c*base_mat[6+col];",
                 "native CharHair SetAngle helper applies RotateAboutX times baseMat");
  ok &= contains(char_mesh,
                 "voidsource_char_hair_strand_set_angle(CharHairStrand&strand,"
                 "floatangle_degrees){strand.angle=angle_degrees;",
                 "native CharHair Strand SetAngle stores source angle");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_strand_set_angle(angle_strand,90.0f);",
                 "deterministic test covers CharHair Strand SetAngle helper");
  ok &= contains(bind_audit,
                 "source_char_hair_set_angle_root_mat(strand.angle,"
                 "strand.base_mat);",
                 "bind audit uses shared CharHair SetAngle helper");
  ok &= contains(doc,
                 "`CharHair::Strand::SetAngle` stores the angle",
                 "document records CharHair Strand SetAngle source behavior");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::SetCloth(boolb){for(inti=0;"
                 "i<mStrands.size();i++){Strand&strand=mStrands[i];"
                 "intmod=Mod(i+1,mStrands.size());",
                 "RB3 CharHair SetCloth wraps to the next strand");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "point.sideLength=b1?Distance(point.pos,"
                 "modidx.mPoints[j].pos):-1.0f;",
                 "RB3 CharHair SetCloth computes side length or disables it");
  ok &= contains(char_mesh_h,
                 "voidsource_char_hair_set_cloth(CharHair&hair,boolenabled);",
                 "native exposes source CharHair SetCloth helper");
  ok &= contains(char_mesh,
                 "voidsource_char_hair_set_cloth(CharHair&hair,boolenabled){"
                 "constsize_tstrand_count=hair.strands.size();"
                 "if(strand_count==0)return;",
                 "native CharHair SetCloth helper handles empty strand lists");
  ok &= contains(char_mesh,
                 "constCharHairStrand&next=hair.strands[(si+1)%strand_count];",
                 "native CharHair SetCloth helper wraps to the next strand");
  ok &= contains(char_mesh,
                 "point.side_length=std::sqrt(dx*dx+dy*dy+dz*dz);",
                 "native CharHair SetCloth helper computes source distance");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_set_cloth(cloth_hair,true);",
                 "deterministic test enables source CharHair SetCloth helper");
  ok &= contains(mesh_decode_test,
                 "CHECK(approx(cloth_hair.strands[0].points[0].side_length,"
                 "5.0f));",
                 "deterministic test verifies source CharHair SetCloth distance");
  ok &= contains(mesh_decode_test,
                 "source_char_hair_set_cloth(cloth_hair,false);",
                 "deterministic test disables source CharHair SetCloth helper");
  ok &= contains(doc,
                 "Native ports this exactly as `source_char_hair_set_cloth`",
                 "document ties native CharHair SetCloth helper to source");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::Hookup(){if(mManagedHookup)return;"
                 "ObjPtrList<CharCollide,ObjectDir>colList(this,kObjListNoNull);"
                 "for(ObjDirItr<CharCollide>it(Dir(),true);it!=0;++it){"
                 "colList.push_back(it);}Hookup(colList);}",
                 "RB3 CharHair default hookup gathers CharCollide rows");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "voidCharHair::SimulateLoops(intcount,floatf){if(mSimulate&&"
                 "mStrands.size()!=0){",
                 "RB3 CharHair SimulateLoops source gate");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "for(intn=0;n<count;n++){SimulateInternal(f);}",
                 "RB3 CharHair SimulateLoops source call count");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_hookup_plan(false,{\"head.collide\","
                 "\"neck.collide\"})",
                 "focused CharHair test covers Hookup collide collection");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_point_load_plan(2)",
                 "focused CharHair test covers legacy Point load plan");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_point_load_plan(6)",
                 "focused CharHair test covers rev-6 Point load plan");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_point_load_plan(8)",
                 "focused CharHair test covers rev-8 Point load plan");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_strand_load_plan(3)",
                 "focused CharHair test covers Strand hookup-flags load");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_load_plan(11)",
                 "focused CharHair test covers modern CharHair load plan");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_hookup_plan(true,{\"head.collide\","
                 "\"neck.collide\"})",
                 "focused CharHair test covers managed Hookup return");
  ok &= contains(char_hair_source_test,
                 "source_char_hair_simulate_loops_plan(true,2,3,4,30.0f)",
                 "focused CharHair test covers SimulateLoops gate");
  ok &= contains(doc,
                 "`source_char_hair_hookup_plan` ports the managed-hookup "
                 "early return",
                 "document records native CharHair Hookup plan");
  ok &= contains(doc,
                 "Native `source_char_hair_simulate_loops_plan` ports that gate",
                 "document records native CharHair SimulateLoops plan");
  ok &= contains(doc,
                 "`source_char_hair_load_plan`, `source_char_hair_strand_load_plan`, and",
                 "document records native CharHair load plan helpers");
  ok &= contains(doc,
                 "revision gates as deterministic format evidence for hair segment/controller",
                 "document ties CharHair load plans to segment/controller rows");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(thisPoint.collides.size()!=0){",
                 "RB3 CharHair runtime writes only through resolved collides");
  ok &= contains(rb3_latest_char_hair_cpp,
                 "if(thisPoint.bone)thisPoint.bone->SetWorldXfm(t100);",
                 "RB3 CharHair writes driven Trans rows only from source simulate");
  ok &= contains(rb3_latest_char_hair_h,
                 "voidHookup(ObjPtrList<CharCollide,ObjectDir>&);",
                 "latest CharHair header declares collision-list hookup");
  ok &= missing(rb3_latest_char_hair_cpp,
                "voidCharHair::Hookup(ObjPtrList<CharCollide,ObjectDir>&",
                "latest CharHair source still lacks overloaded hookup body");
  ok &= contains(rb3_latest_char_collide_h,
                 "enumShape{kPlane=0,kSphere=1,kInsideSphere=2,kCigar=3,"
                 "kInsideCigar=4,};",
                 "latest CharCollide header exposes source shape enum");
  ok &= contains(rb3_latest_char_collide_h,
                 "floatGetRadius(constVector3&v1,Vector3&vout)const{"
                 "Subtract(v1,unk1a0,vout);floatret=mCurRadius[0];",
                 "latest CharCollide header exposes inline GetRadius formula");
  ok &= contains(rb3_latest_char_collide_h,
                 "floatclamped=Clamp(mCurLength[0],mCurLength[1],unk190*"
                 "Dot(vout,unk194));",
                 "latest CharCollide GetRadius depends on cached collision fields");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "bs>>(int&)mShape;bs>>mOrigRadius[0];if(gRev>4)bs>>"
                 "mOrigLength[0];",
                 "latest CharCollide source exposes load path");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "if(gRev>1)bs>>mFlags;elsemFlags=0;if(gRev>3)bs>>"
                 "mCurRadius[0];elsemCurRadius[0]=mOrigRadius[0];",
                 "latest CharCollide source exposes flags/current-radius gates");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "bs>>unk148;bs>>mMesh;for(inti=0;i<8;i++){bs>>"
                 "unk_structs[i].unk0;bs>>unk_structs[i].vec;}bs>>mDigest;",
                 "latest CharCollide source retains mesh collision rows");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "bs>>mDigest;bs>>mMeshYBias;if(gRev<7)CopyOriginalToCur();}"
                 "else{mOrigRadius[1]=mOrigRadius[0];CopyOriginalToCur();}",
                 "latest CharCollide source exposes load CopyOriginalToCur gates");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(7,0)",
                 "latest CharCollide Load accepts source revisions through 7");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "CharCollide::CharCollide():mShape(kSphere),mFlags(0),"
                 "mMesh(this,0),mMeshYBias(0){",
                 "latest CharCollide constructor exposes source defaults");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "for(inti=0;i<2;i++){mOrigLength[i]=0;mOrigRadius[i]=0;}"
                 "CopyOriginalToCur();for(inti=0;i<8;i++){"
                 "unk_structs[i].unk0=0;unk_structs[i].vec.Zero();}"
                 "unk148.Reset();}",
                 "latest CharCollide constructor zeroes source rows");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS("
                 "RndTransformable)CREATE_COPY(CharCollide)",
                 "latest CharCollide Copy includes source superclasses");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "COPY_MEMBER(mShape)COPY_MEMBER(mFlags)memcpy(mOrigRadius,"
                 "c->mOrigRadius,8);memcpy(mOrigLength,c->mOrigLength,8);"
                 "memcpy(mCurRadius,c->mCurRadius,8);memcpy(mCurLength,"
                 "c->mCurLength,8);COPY_MEMBER(unk148)COPY_MEMBER(mMeshYBias)"
                 "COPY_MEMBER(mMesh)",
                 "latest CharCollide Copy exposes copied member list");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "intCharCollide::NumSpheres(){if(mShape==kCigar||mShape=="
                 "kInsideCigar)return2;elseif(mShape==kSphere||mShape=="
                 "kInsideSphere)return1;elsereturn0;}",
                 "latest CharCollide source exposes NumSpheres helper");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "voidCharCollide::CopyOriginalToCur(){memcpy(mCurRadius,"
                 "mOrigRadius,8);memcpy(mCurLength,mOrigLength,8);}",
                 "latest CharCollide source exposes CopyOriginalToCur helper");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "voidCharCollide::SyncShape(){f32t=mCurLength[1];"
                 "if(mCurLength[0]>t){mCurLength[0]=mCurLength[1];}"
                 "CopyOriginalToCur();}",
                 "latest CharCollide source exposes SyncShape helper");
  ok &= contains(rb2_char_collide_cpp,
                 "voidCharCollide::ComputeRadius(classCharCollide*constthis",
                 "RB2 dump names CharCollide ComputeRadius without usable body");
  ok &= contains(rb2_char_collide_cpp, "voidCharCollide::SyncRadius(){}",
                 "RB2 dump names empty CharCollide SyncRadius");
  ok &= contains(rb2_char_collide_h, "floatCharCollide::Radius(){}",
                 "RB2 dump names CharCollide Radius without usable body");
  ok &= contains(char_mesh_h,
                 "structCharCollide{std::stringname;int32_tversion=0;",
                 "native exposes decoded CharCollide rows");
  ok &= contains(char_mesh_h,
                 "structCharCollideMeshSphere{int32_tvertex=0;floatvec[3]="
                 "{0.0f,0.0f,0.0f};};",
                 "native exposes source CharCollide mesh sphere rows");
  ok &= contains(char_mesh_h,
                 "milo_scene::Xfmmesh_transform;std::array<CharCollideMeshSphere,8>"
                 "mesh_spheres;std::array<uint8_t,20>digest={};",
                 "native CharCollide stores source mesh transform and digest");
  ok &= contains(char_mesh_h,
                 "CharCollidedecode_collide(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body);",
                 "native exposes CharCollide decoder for contract coverage");
  ok &= contains(char_mesh_h,
                 "voidsource_char_collide_copy_original_to_cur(CharCollide&"
                 "collide);",
                 "native exposes CharCollide CopyOriginalToCur helper port");
  ok &= contains(char_mesh_h,
                 "voidsource_char_collide_sync_shape(CharCollide&collide);",
                 "native exposes CharCollide SyncShape helper port");
  ok &= contains(char_mesh_h,
                 "intsource_char_collide_num_spheres(constCharCollide&"
                 "collide);",
                 "native exposes CharCollide NumSpheres helper port");
  ok &= contains(char_mesh_h,
                 "structSourceCharCollideRadiusCache{std::array<float,3>"
                 "origin={0.0f,0.0f,0.0f};std::array<float,3>axis="
                 "{0.0f,1.0f,0.0f};floatlength_scale=1.0f;"
                 "floatradius_lerp_scale=1.0f;};",
                 "native exposes explicit CharCollide radius cache");
  ok &= contains(char_mesh_h,
                 "floatsource_char_collide_get_radius(constCharCollide&"
                 "collide,constSourceCharCollideRadiusCache&cache,"
                 "conststd::array<float,3>&point,std::array<float,3>&"
                 "out_delta);",
                 "native exposes bounded CharCollide GetRadius helper");
  ok &= contains(char_mesh_h,
                 "structSourceCharCollideDefaultState{int32_tshape=1;"
                 "int32_tflags=0;boolmesh_empty=true;boolmesh_y_bias=false;",
                 "native exposes CharCollide constructor default contract");
  ok &= contains(char_mesh_h,
                 "structSourceCharCollideCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>"
                 "copied_members;std::vector<std::string>"
                 "not_in_source_copy_members;};",
                 "native exposes CharCollide copy-member contract");
  ok &= contains(char_mesh_h,
                 "structSourceCharCollideLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;",
                 "native exposes CharCollide load-plan contract");
  ok &= contains(char_mesh_h,
                 "SourceCharCollideDefaultStatesource_char_collide_default_state();",
                 "native exposes CharCollide default-state helper");
  ok &= contains(char_mesh_h,
                 "SourceCharCollideLoadPlansource_char_collide_load_plan("
                 "intrevision);",
                 "native exposes CharCollide load-plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharCollideCopyPlansource_char_collide_copy_plan();",
                 "native exposes CharCollide copy-plan helper");
  ok &= contains(char_mesh,
                 "CharCollidedecode_collide(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "native CharCollide decoder exists");
  ok &= contains(char_mesh,
                 "if(collide.version<0||collide.version>7){"
                 "throwstd::runtime_error(\"char_mesh:CharColliderevision"
                 "outsidesourcerange\");}",
                 "native CharCollide decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "read_object_fields(r);constTransFieldstrans=read_rnd_trans(r,false);",
                 "native CharCollide decoder follows object then transform source order");
  ok &= contains(char_mesh,
                 "collide.shape=r.i32();collide.orig_radius[0]=r.f32();"
                 "if(collide.version>4)collide.orig_length[0]=r.f32();",
                 "native CharCollide decoder follows source radius/length gates");
  ok &= contains(char_mesh,
                 "collide.mesh_transform=r.matrix();collide.mesh=r.str();"
                 "for(inti=0;i<8;++i){collide.mesh_spheres[i].vertex=r.i32();",
                 "native CharCollide decoder stores source mesh collision rows");
  ok &= contains(char_mesh,
                 "for(uint8_t&byte:collide.digest)byte=r.u8();"
                 "collide.mesh_y_bias=r.u8()!=0;",
                 "native CharCollide decoder stores digest and mesh-y-bias");
  ok &= contains(char_mesh,
                 "if(collide.version<7)source_char_collide_copy_original_to_cur"
                 "(collide);",
                 "native CharCollide decoder uses named source copy helper");
  ok &= contains(char_mesh,
                 "voidsource_char_collide_copy_original_to_cur(CharCollide&"
                 "collide){collide.cur_radius[0]=collide.orig_radius[0];"
                 "collide.cur_radius[1]=collide.orig_radius[1];",
                 "native CharCollide CopyOriginalToCur helper is ported");
  ok &= contains(char_mesh,
                 "voidsource_char_collide_sync_shape(CharCollide&collide){"
                 "constfloatt=collide.cur_length[1];"
                 "if(collide.cur_length[0]>t){collide.cur_length[0]="
                 "collide.cur_length[1];}source_char_collide_copy_original_to_cur"
                 "(collide);}",
                 "native CharCollide SyncShape helper is ported");
  ok &= contains(char_mesh,
                 "intsource_char_collide_num_spheres(constCharCollide&collide)"
                 "{if(collide.shape==3||collide.shape==4)return2;",
                 "native CharCollide NumSpheres helper is ported");
  ok &= contains(char_mesh,
                 "floatsource_char_collide_get_radius(constCharCollide&"
                 "collide,constSourceCharCollideRadiusCache&cache,"
                 "conststd::array<float,3>&point,std::array<float,3>&"
                 "out_delta){out_delta={point[0]-cache.origin[0],",
                 "native CharCollide GetRadius helper starts from source delta");
  ok &= contains(char_mesh,
                 "SourceCharCollideDefaultStatesource_char_collide_default_state(){"
                 "return{};}",
                 "native CharCollide default-state helper follows source defaults");
  ok &= contains(char_mesh,
                 "SourceCharCollideLoadPlansource_char_collide_load_plan("
                 "intrevision){SourceCharCollideLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=7;",
                 "native CharCollide load plan records source revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order={\"LOAD_REVS\",\"Hmx::Object\","
                 "\"RndTransformable\",\"mShape\",\"mOrigRadius[0]\"};",
                 "native CharCollide load plan records source superclass order");
  ok &= contains(char_mesh,
                 "if(revision>4)plan.read_order.push_back(\"mOrigLength[0]\");"
                 "if(revision>2)plan.read_order.push_back(\"mOrigLength[1]\");",
                 "native CharCollide load plan records length gates");
  ok &= contains(char_mesh,
                 "if(revision>1){plan.read_order.push_back(\"mFlags\");}"
                 "else{plan.branches.push_back(\"mFlags=0\");}",
                 "native CharCollide load plan records flags gate");
  ok &= contains(char_mesh,
                 "if(revision>3){plan.read_order.push_back(\"mCurRadius[0]\");}"
                 "else{plan.branches.push_back("
                 "\"mCurRadius[0]=mOrigRadius[0]\");}",
                 "native CharCollide load plan records current-radius gate");
  ok &= contains(char_mesh,
                 "if(revision>5){plan.read_order.push_back(\"mOrigRadius[1]\");"
                 "plan.read_order.push_back(\"mCurRadius[1]\");",
                 "native CharCollide load plan records extended radius rows");
  ok &= contains(char_mesh,
                 "plan.read_order.push_back(\"unk_structs[8]\");"
                 "plan.mesh_sphere_rows=8;",
                 "native CharCollide load plan records eight mesh sphere rows");
  ok &= contains(char_mesh,
                 "plan.read_order.push_back(\"mDigest\");"
                 "plan.read_order.push_back(\"mMeshYBias\");"
                 "if(revision<7)plan.branches.push_back(\"CopyOriginalToCur\");",
                 "native CharCollide load plan records digest and rev-6 copy gate");
  ok &= contains(char_mesh,
                 "else{plan.branches.push_back("
                 "\"mOrigRadius[1]=mOrigRadius[0]\");"
                 "plan.branches.push_back(\"CopyOriginalToCur\");}",
                 "native CharCollide load plan records legacy radius copy gate");
  ok &= contains(char_mesh,
                 "plan.copied_superclasses={\"Hmx::Object\","
                 "\"RndTransformable\"};",
                 "native CharCollide copy plan records source superclasses");
  ok &= contains(char_mesh,
                 "plan.copied_members={\"mShape\",\"mFlags\",\"mOrigRadius\","
                 "\"mOrigLength\",\"mCurRadius\",\"mCurLength\",\"unk148\","
                 "\"mMeshYBias\",\"mMesh\"};",
                 "native CharCollide copy plan records source members");
  ok &= contains(char_mesh,
                 "plan.not_in_source_copy_members={\"mDigest\",\"unk_structs\"};",
                 "native CharCollide copy plan records members absent from source copy list");
  ok &= contains(char_mesh,
                 "std::clamp(cache.length_scale*dot_axis(),"
                 "collide.cur_length[0],collide.cur_length[1]);",
                 "native CharCollide GetRadius helper clamps cigar length");
  ok &= contains(char_mesh,
                 "radius=radius+(collide.cur_radius[1]-radius)*t;",
                 "native CharCollide GetRadius helper interpolates radius");
  ok &= contains(char_mesh,
                 "elseif(collide.shape==0){radius=dot_axis();",
                 "native CharCollide GetRadius helper handles plane branch");
  ok &= contains(char_mesh,
                 "out.collides.push_back(decode_collide(de.name,b));",
                 "character load stores decoded CharCollide rows");
  ok &= contains(char_clip,
                 "\"[chargraph]collide%s",
                 "character graph log exposes decoded CharCollide rows");
  ok &= contains(doc, "`CharCollide::Load` reads",
                 "document records CharCollide source decode order");
  ok &= contains(doc, "Native `source_char_collide_load_plan` records this exact read order",
                 "document records native CharCollide load-plan helper");
  ok &= contains(doc, "`CharCollide::Load` uses `ASSERT_REVS(7, 0)`",
                 "document records CharCollide source revision gate");
  ok &= contains(mesh_decode_test, "CHECK(bad_collide_version_threw);",
                 "mesh decode test covers invalid CharCollide revision");
  ok &= contains(mesh_decode_test, "CHECK(collide.mesh_spheres[3].vertex==30);",
                 "mesh decode test covers decoded CharCollide mesh sphere row");
  ok &= contains(mesh_decode_test, "CHECK(collide.digest[19]==20);",
                 "mesh decode test covers decoded CharCollide digest");
  ok &= contains(mesh_decode_test,
                 "source_char_collide_copy_original_to_cur(copied_collide);",
                 "mesh decode test covers CharCollide CopyOriginalToCur helper");
  ok &= contains(mesh_decode_test,
                 "source_char_collide_sync_shape(synced_collide);",
                 "mesh decode test covers CharCollide SyncShape helper");
  ok &= contains(mesh_decode_test,
                 "source_char_collide_num_spheres(collide)==2",
                 "mesh decode test covers CharCollide NumSpheres helper");
  ok &= contains(mesh_decode_test,
                 "source_char_collide_get_radius(radius_collide,"
                 "radius_cache,{4.0f,6.0f,3.0f},collide_delta)",
                 "mesh decode test covers CharCollide sphere/plane radius helper");
  ok &= contains(mesh_decode_test,
                 "source_char_collide_get_radius(radius_collide,"
                 "radius_cache,{5.0f,2.0f,0.0f},collide_delta)",
                 "mesh decode test covers CharCollide cigar radius helper");
  ok &= contains(char_collide_source_test,
                 "source_char_collide_default_state()",
                 "CharCollide source test covers constructor defaults");
  ok &= contains(char_collide_source_test,
                 "native_default.mesh_spheres.size()==static_cast<size_t>("
                 "defaults.mesh_sphere_count)",
                 "CharCollide source test covers native mesh sphere defaults");
  ok &= contains(char_collide_source_test,
                 "source_char_collide_copy_plan()",
                 "CharCollide source test covers copy-member plan");
  ok &= contains(char_collide_source_test,
                 "source_char_collide_load_plan(1)",
                 "CharCollide source test covers legacy load plan");
  ok &= contains(char_collide_source_test,
                 "source_char_collide_load_plan(6)",
                 "CharCollide source test covers rev-6 extended load plan");
  ok &= contains(char_collide_source_test,
                 "source_char_collide_load_plan(7)",
                 "CharCollide source test covers latest load plan");
  ok &= contains(cmake, "ghogx_character_char_collide_source_test",
                 "CMake registers CharCollide source test");
  ok &= contains(doc,
                 "Native GHOGX retains the internal transform, all eight mesh sphere rows",
                 "document records decoded CharCollide retained source rows");
  ok &= contains(doc, "Native GHOGX ports `CharCollide::CopyOriginalToCur`",
                 "document records CharCollide helper ports");
  ok &= contains(doc,
                 "`CharCollide::CharCollide` initializes `mShape` to `kSphere`",
                 "document records CharCollide constructor source defaults");
  ok &= contains(doc,
                 "`CharCollide::Copy` copies `Hmx::Object`, `RndTransformable`, shape, flags",
                 "document records CharCollide source copy list");
  ok &= contains(doc,
                 "The checked source copy-member list does not\n"
                 "    name `mDigest` or `unk_structs`",
                 "document fences CharCollide copy-plan omissions");
  ok &= contains(doc, "`CharCollide::SyncShape` / `CharCollide::NumSpheres`",
                 "document records CharCollide SyncShape helper port");
  ok &= contains(doc, "`CharHair::SimulateInternal` calls `CharCollide::GetRadius`",
                 "document records CharCollide collision radius dependency");
  ok &= contains(doc,
                 "ports the\n    inline formula as `source_char_collide_get_radius`",
                 "document records bounded CharCollide GetRadius formula port");
  ok &= contains(doc, "Native therefore keeps collision response disabled until the\n"
                      "    cached-field updates are sourced",
                 "document fences unsourced CharCollide collision response");
  ok &= contains(doc, "source poll/reset/sim state path",
                 "document states bounded native CharHair poll rule");
  ok &= contains(doc, "point rows unwritten until",
                 "document states bounded native CharHair writeback rule");
  ok &= contains(doc, "point collide-list population",
                 "document names missing CharHair point collision hookup boundary");
  ok &= contains(doc, "latest source includes `CharHair.h`, `CharCollide.h`",
                 "document records stronger latest hair source boundary");
  ok &= contains(doc,
                 "`Hookup(ObjPtrList<CharCollide>&)` body is still declared",
                 "document records missing CharHair hookup body boundary");
  ok &= contains(doc,
                 "The current config exposes `CharHair::GetFPS` and `CharHair::Simulate`",
                 "document records band3 CharHair symbol-only evidence");
  ok &= contains(char_clip, "runtimeWriteback=%dresolvedPointCollides=0",
                 "native CharHair path logs unresolved point-collide write count");
  ok &= contains(char_clip, "missingHookupObjPtrList=1",
                 "native CharHair path keeps missing hookup boundary explicit");
  ok &= contains(rb3_latest_char_ik_rod_h,
                 "ObjPtr<RndTransformable,ObjectDir>mLeftEnd;",
                 "latest CharIKRod source header exposes left endpoint");
  ok &= contains(rb3_latest_char_ik_rod_h,
                 "ObjPtr<RndTransformable,ObjectDir>mDest;",
                 "latest CharIKRod source header exposes destination");
  ok &= contains(rb3_latest_char_ik_rod_h, "TransformmXfm;",
                 "latest CharIKRod source header exposes stored transform");
  ok &= contains(rb3_latest_char_ik_rod_cpp,
                 "if(mDest==0||mLeftEnd==0||mRightEnd==0)returnfalse;",
                 "CharIKRod source ComputeRod refuses incomplete refs");
  ok &= contains(rb3_latest_char_ik_rod_cpp,
                 "bs>>mLeftEnd;bs>>mRightEnd;bs>>mDestPos;bs>>mSideAxis;"
                 "bs>>mVertical;bs>>mDest;bs>>mXfm;",
                 "CharIKRod source load order is mirrored");
  ok &= contains(char_mesh_h, "structCharIKRod{std::stringname;int32_tversion=0;",
                 "native CharIKRod stores source revision");
  ok &= contains(char_mesh_h, "floatxfm[4][3]={};",
                 "native CharIKRod names stored source mXfm");
  ok &= contains(char_mesh, "rod.version=r.i32();",
                 "native CharIKRod decoder stores source revision");
  ok &= contains(char_mesh,
                 "rod.left_end=r.str();rod.right_end=r.str();"
                 "rod.dest_pos=r.f32();rod.side_axis=r.str();"
                 "rod.vertical=r.u8()!=0;rod.dest=r.str();",
                 "native CharIKRod decode mirrors source load fields");
  ok &= contains(char_mesh, "rod.xfm[v][c]=r.f32();",
                 "native CharIKRod decode stores source mXfm");
  ok &= contains(char_clip_h,
                 "boolsource_char_ik_rod_compute_world(constCharIKRod&rod,"
                 "constCharacter&character,std::array<float,16>&dest_world);",
                 "native exposes source CharIKRod compute/poll helper");
  ok &= contains(char_clip,
                 "boolsource_char_ik_rod_compute_world(constCharIKRod&rod,"
                 "constCharacter&character,std::array<float,16>&dest_world){"
                 "if(rod.dest.empty()||rod.left_end.empty()||"
                 "rod.right_end.empty()){returnfalse;}",
                 "native CharIKRod helper keeps source missing-ref boundary");
  ok &= contains(char_clip,
                 "if(!character.has_transform(rod.dest))returnfalse;",
                 "native CharIKRod helper requires source destination transform");
  ok &= contains(char_clip,
                 "constVec3pos=vadd(vscale(left_pos,1.0f-t),"
                 "vscale(right_pos,t));",
                 "native CharIKRod helper interpolates endpoint position");
  ok &= contains(char_clip,
                 "rod.vertical?Vec3{0.0f,0.0f,-1.0f}:vnorm("
                 "vadd(vscale(mat_row(left_world,0),1.0f-t),",
                 "native CharIKRod helper mirrors vertical/interpolated X branch");
  ok &= contains(char_clip,
                 "if(transform_local_chain_world(character,rod.side_axis,"
                 "side_world)){z=mat_row(side_world,2);}else{z=vsub("
                 "left_pos,right_pos);}",
                 "native CharIKRod helper mirrors side-axis fallback branch");
  ok &= contains(char_clip,
                 "dest_world=mat4_mul(source_transform_row_mat4(rod.xfm),"
                 "rod_world);",
                 "native CharIKRod Poll applies stored mXfm before destination");
  ok &= contains(char_clip,
                 "character.runtime_world_overrides[rod.dest]=dest_world;",
                 "native CharIKRod Poll publishes destination world row");
  ok &= contains(char_clip, "apply_source_ik_rods(character);",
                 "native controller cadence runs source CharIKRod poll");
  ok &= contains(ik_rod_source_test,
                 "missing_dest.dest.clear();ok&=!source_char_ik_rod_compute_world"
                 "(missing_dest,character,world);",
                 "focused CharIKRod test covers source missing destination boundary");
  ok &= contains(ik_rod_source_test,
                 "character.ik_rods.push_back(make_identity_rod());"
                 "apply_character_controllers(character,0.0f,nullptr);",
                 "focused CharIKRod test covers controller writeback path");
  ok &= contains(bind_audit, "version=%dleft=%s",
                 "controller audit logs CharIKRod source revision");
  ok &= contains(bind_audit, "leftExists=%dright=%srightExists=%ddest=%s",
                 "controller audit logs CharIKRod ref existence");
  ok &= contains(bind_audit, "rod.xfm[3][0],rod.xfm[3][1],rod.xfm[3][2]",
                 "controller audit logs CharIKRod stored transform");
  ok &= contains(doc, "`CharIKRod::Load` reads revision 2 rows",
                 "document records CharIKRod source load order");
  ok &= contains(doc, "ComputeRod` returns\n    false unless `dest`, `left_end`, and `right_end` all resolve",
                 "document records CharIKRod incomplete-ref boundary");
  ok &= contains(doc,
                 "Native `source_char_ik_rod_compute_world` ports that "
                 "`ComputeRod` / `Poll`",
                 "document records native CharIKRod source poll port");
  ok &= contains(doc,
                 "Stock Grim rows with `dest=<none>` therefore remain "
                 "logged/inert",
                 "document records stock Grim missing-destination boundary");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "classCharIKHead:publicRndHighlightable,"
                 "publicCharWeightable,publicCharPollable",
                 "CharIKHead source header exposes inheritance");
  ok &= contains(rb3_latest_char_ik_head_h, "ObjVector<Point>mPoints;",
                 "CharIKHead source header exposes point rows");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "ObjPtr<RndTransformable,ObjectDir>mHead;",
                 "CharIKHead source header exposes head ref");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "ObjPtr<RndTransformable,ObjectDir>mSpine;",
                 "CharIKHead source header exposes spine ref");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "ObjPtr<RndTransformable,ObjectDir>mMouth;",
                 "CharIKHead source header exposes mouth ref");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "ObjPtr<RndTransformable,ObjectDir>mTarget;",
                 "CharIKHead source header exposes target ref");
  ok &= contains(rb3_latest_char_ik_head_h, "Vector3mHeadFilter;",
                 "CharIKHead source header exposes head filter");
  ok &= contains(rb3_latest_char_ik_head_h, "floatmTargetRadius;",
                 "CharIKHead source header exposes target radius");
  ok &= contains(rb3_latest_char_ik_head_h, "floatmHeadMat;",
                 "CharIKHead source header exposes head mat");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "ObjPtr<RndTransformable,ObjectDir>mOffset;",
                 "CharIKHead source header exposes offset ref");
  ok &= contains(rb3_latest_char_ik_head_h, "Vector3mOffsetScale;",
                 "CharIKHead source header exposes offset scale");
  ok &= contains(rb3_latest_char_ik_head_h, "boolmUpdatePoints;",
                 "CharIKHead source header exposes update-points flag");
  ok &= contains(rb3_latest_char_ik_head_h,
                 "ObjPtr<Character,ObjectDir>mMe;",
                 "CharIKHead source header exposes owning character ref");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "CharIKHead::CharIKHead():mPoints(this),mHead(this,0),"
                 "mSpine(this,0),mMouth(this,0),mTarget(this,0),"
                 "mHeadFilter(0.0f,0.0f,0.0f),mTargetRadius(0.75f),",
                 "CharIKHead source constructor exposes pointer defaults");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "mHeadMat(0.5f),mOffset(this,0),mOffsetScale(1.0f,"
                 "1.0f,1.0f),mUpdatePoints(1),mMe(this,0)",
                 "CharIKHead source constructor exposes scalar defaults");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "voidCharIKHead::SetName(constchar*name,ObjectDir*dir){"
                 "Hmx::Object::SetName(name,dir);mMe=dynamic_cast<Character*>"
                 "(dir);}",
                 "CharIKHead source SetName stores Character dir");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "changedBy.push_back(mMouth);changedBy.push_back(mHead);"
                 "changedBy.push_back(mTarget);",
                 "CharIKHead source PollDeps publishes changed-by refs");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "if(GenerationCount(mSpine,mHead)!=0){for("
                 "RndTransformable*t=mHead;t!=0&&t!=mSpine->TransParent();"
                 "t=t->TransParent()){change.push_back(t);}}"
                 "change.push_back(mOffset);",
                 "CharIKHead source PollDeps publishes chain and offset");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "voidCharIKHead::UpdatePoints(boolb){if(b||mUpdatePoints){"
                 "mUpdatePoints=false;mPoints.clear();intgencnt="
                 "GenerationCount(mSpine,mHead);",
                 "CharIKHead source UpdatePoints gates on force or dirty flag");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "mPoints.resize(gencnt+1);",
                 "CharIKHead source UpdatePoints builds generation-plus-one rows");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "pt.unk18=Length(curtrans->LocalXfm().v);",
                 "CharIKHead source UpdatePoints stores local lengths");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "mSpineLength=f1;floatf2=1.0f/f1;",
                 "CharIKHead source UpdatePoints stores spine length");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "mPoints[i].unk1c=f1*f2;f1=f1-mPoints[i].unk18;",
                 "CharIKHead source UpdatePoints stores normalized remaining length");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(3,0)",
                 "CharIKHead source load enforces revision ceiling");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "LOAD_SUPERCLASS(Hmx::Object)LOAD_SUPERCLASS(CharWeightable)"
                 "bs>>mHead;bs>>mSpine;bs>>mMouth;bs>>mTarget;",
                 "CharIKHead source load reads object, weightable, and refs");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "if(gRev>1){bs>>mTargetRadius;bs>>mHeadMat;}",
                 "CharIKHead source load gates radius and head mat");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "if(gRev>2){bs>>mOffset;bs>>mOffsetScale;}",
                 "CharIKHead source load gates offset rows");
  ok &= contains(rb3_latest_char_ik_head_cpp, "mUpdatePoints=true;",
                 "CharIKHead source load/copy marks point rows dirty");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS(CharWeightable)"
                 "CREATE_COPY(CharIKHead)",
                 "CharIKHead source copy includes object and weightable");
  ok &= contains(rb3_latest_char_ik_head_cpp,
                 "COPY_MEMBER(mHead)COPY_MEMBER(mSpine)COPY_MEMBER(mMouth)"
                 "COPY_MEMBER(mTarget)COPY_MEMBER(mTargetRadius)"
                 "COPY_MEMBER(mHeadMat)COPY_MEMBER(mOffset)"
                 "COPY_MEMBER(mOffsetScale)mUpdatePoints=true;",
                 "CharIKHead source copy mirrors member list");
  ok &= contains(char_clip_h,
                 "structSourceCharIKHeadState{SourceCharWeightableState"
                 "weightable;",
                 "native exposes CharIKHead source state");
  ok &= contains(char_clip_h,
                 "floattarget_radius=0.75f;floathead_mat=0.5f;",
                 "native stores CharIKHead source scalar defaults");
  ok &= contains(char_clip_h,
                 "std::array<float,3>offset_scale={1.0f,1.0f,1.0f};",
                 "native stores CharIKHead source offset-scale default");
  ok &= contains(char_clip_h, "boolupdate_points=true;",
                 "native stores CharIKHead source update-points default");
  ok &= contains(char_clip_h,
                 "SourceCharIKHeadStatesource_char_ik_head_default_state(",
                 "native API exposes CharIKHead defaults helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_ik_head_poll_deps(",
                 "native API exposes CharIKHead PollDeps helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKHeadUpdatePointsResult"
                 "source_char_ik_head_update_points(",
                 "native API exposes CharIKHead UpdatePoints helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKHeadLoadStepssource_char_ik_head_load_steps("
                 "int32_trevision);",
                 "native API exposes CharIKHead load gate helper");
  ok &= contains(char_clip,
                 "state.weightable=source_char_weightable_default_state(name);"
                 "returnstate;",
                 "native CharIKHead defaults helper mirrors weightable ctor");
  ok &= contains(char_clip,
                 "result.call_hmx_set_name=true;result.assigned_character="
                 "dir_is_character;state.character_dir=dir_is_character?"
                 "dir_name:std::string{};",
                 "native CharIKHead SetName helper mirrors Character cast");
  ok &= contains(char_clip,
                 "deps.changed_by.push_back(state.mouth);"
                 "deps.changed_by.push_back(state.head);"
                 "deps.changed_by.push_back(state.target);",
                 "native CharIKHead PollDeps helper mirrors changed-by refs");
  ok &= contains(char_clip,
                 "if(generation_count_nonzero){for(conststd::string&"
                 "transform:head_to_spine_parent_chain){deps.change.push_back"
                 "(transform);}}deps.change.push_back(state.offset);",
                 "native CharIKHead PollDeps helper mirrors chain and offset");
  ok &= contains(char_clip,
                 "if(!force&&!state.update_points)returnresult;"
                 "result.entered_body=true;state.update_points=false;"
                 "state.points.clear();",
                 "native CharIKHead UpdatePoints helper mirrors gate and clear");
  ok &= contains(char_clip,
                 "state.spine_length=total;result.spine_length=total;",
                 "native CharIKHead UpdatePoints helper stores spine length");
  ok &= contains(char_clip,
                 "state.points[i].normalized_remaining=remaining*inv_total;"
                 "remaining-=state.points[i].local_length;",
                 "native CharIKHead UpdatePoints helper stores normalized rows");
  ok &= contains(char_clip,
                 "steps.load_target_radius=revision>1;steps.load_head_mat="
                 "revision>1;steps.load_offset=revision>2;"
                 "steps.load_offset_scale=revision>2;",
                 "native CharIKHead load helper mirrors revision gates");
  ok &= contains(char_clip,
                 "source_char_weightable_copy(dest.weightable,"
                 "source.weightable,shallow_copy,source_owner_weight);",
                 "native CharIKHead copy helper mirrors weightable copy");
  ok &= contains(char_clip,
                 "dest.offset_scale=source.offset_scale;result.copy_offset_scale"
                 "=true;dest.update_points=true;",
                 "native CharIKHead copy helper mirrors source member tail");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_head_source_test"
                 "character_ik_head_source_test.cpp)",
                 "CMake builds focused CharIKHead source test");
  ok &= contains(ik_head_source_test,
                 "source_char_ik_head_default_state(\"ikhead.weight\")",
                 "focused CharIKHead test covers source defaults");
  ok &= contains(ik_head_source_test,
                 "source_char_ik_head_poll_deps(",
                 "focused CharIKHead test covers PollDeps helper");
  ok &= contains(ik_head_source_test,
                 "source_char_ik_head_update_points(",
                 "focused CharIKHead test covers UpdatePoints helper");
  ok &= contains(ik_head_source_test,
                 "source_char_ik_head_load_steps(3)",
                 "focused CharIKHead test covers load gates");
  ok &= contains(ik_head_source_test,
                 "source_char_ik_head_copy(dest,source,false,0.66f)",
                 "focused CharIKHead test covers copy helper");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharIKHead.cpp`",
                 "document cites CharIKHead source");
  ok &= contains(doc,
                 "Native `source_char_ik_head_*` helpers port these concrete "
                 "source behaviors",
                 "document records native CharIKHead helper boundary");
  ok &= contains(doc,
                 "does not include a\n    reviewable `CharIKHead::Poll` body",
                 "document fences absent CharIKHead Poll body");
  ok &= contains(rb3_latest_char_ik_foot_h,
                 "classCharIKFoot:publicCharIKHand",
                 "CharIKFoot source header exposes CharIKHand inheritance");
  ok &= contains(rb3_latest_char_ik_foot_h,
                 "ObjPtr<RndTransformable,ObjectDir>unk88;",
                 "CharIKFoot source header exposes helper target");
  ok &= contains(rb3_latest_char_ik_foot_h, "intunk94;",
                 "CharIKFoot source header exposes FSM state");
  ok &= contains(rb3_latest_char_ik_foot_h,
                 "ObjPtr<RndTransformable,ObjectDir>mData;",
                 "CharIKFoot source header exposes data ref");
  ok &= contains(rb3_latest_char_ik_foot_h, "intmDataIndex;",
                 "CharIKFoot source header exposes data index");
  ok &= contains(rb3_latest_char_ik_foot_h, "Vector3unka8;",
                 "CharIKFoot source header exposes planted target");
  ok &= contains(rb3_latest_char_ik_foot_h, "floatunkb4;",
                 "CharIKFoot source header exposes release distance");
  ok &= contains(rb3_latest_char_ik_foot_h,
                 "ObjPtr<Character,ObjectDir>mMe;",
                 "CharIKFoot source header exposes owning character ref");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "CharIKFoot::CharIKFoot():unk88(this,0),unk94(0),"
                 "mData(this,0),mDataIndex(0),mMe(this,0){unk88="
                 "Hmx::Object::New<RndTransformable>();"
                 "unk88->DirtyLocalXfm().Reset();}",
                 "CharIKFoot source constructor creates reset helper target");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "voidCharIKFoot::Enter(){unk94=0;unkb4=0.0f;}",
                 "CharIKFoot source Enter resets FSM state");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "voidCharIKFoot::SetName(constchar*cc,ObjectDir*dir){"
                 "Hmx::Object::SetName(cc,dir);mMe=dynamic_cast<Character*>"
                 "(dir);}",
                 "CharIKFoot source SetName stores Character dir");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "if(mMe&&mMe->Teleported())unk94=0;",
                 "CharIKFoot source FSM resets on teleport");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "floatdeltasecs=TheTaskMgr.DeltaSeconds();if(deltasecs<0.0f)"
                 "deltasecs=0.0f;",
                 "CharIKFoot source FSM clamps negative delta");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "tf.m=mFinger->WorldXfm().m;tf.v.z=mFinger->WorldXfm().v.z;",
                 "CharIKFoot source FSM copies finger matrix and z");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "floatvecat=mData->mLocalXfm.v[mDataIndex];",
                 "CharIKFoot source FSM reads data vector index");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "if(len>0.125f)v3c*=0.125f/len;Add(unka8,v3c,tf.v);return;",
                 "CharIKFoot source FSM clamps planted travel");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "unkb4=Min(-(deltasecs*25.0f-unkb4),len);",
                 "CharIKFoot source FSM decays release distance");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "if(!mFinger||!mHand||!mData)return;mTargets.clear();"
                 "mTargets.push_back(IKTarget(ObjPtr<RndTransformable,"
                 "ObjectDir>(unk88),0));DoFSM(unk88->DirtyLocalXfm());"
                 "CharIKHand::Poll();mTargets.clear();",
                 "CharIKFoot source Poll delegates through helper target");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "voidCharIKFoot::PollDeps(std::list<Hmx::Object*>&l1,"
                 "std::list<Hmx::Object*>&l2){CharIKHand::PollDeps(l1,l2);}",
                 "CharIKFoot source PollDeps delegates CharIKHand");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(6,0)LOAD_SUPERCLASS(CharIKHand)",
                 "CharIKFoot source load enforces revision ceiling");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "if(gRev<6){Symbols;bs>>s;}if(gRev<5){inti;if(gRev>1)"
                 "bs>>i;if(gRev>2)bs>>i;if(gRev>3)bs>>i;}else{"
                 "bs>>mData;bs>>mDataIndex;}",
                 "CharIKFoot source load gates legacy and data rows");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "COPY_SUPERCLASS(CharIKHand)CREATE_COPY(CharIKFoot)",
                 "CharIKFoot source copy includes CharIKHand superclass");
  ok &= contains(rb3_latest_char_ik_foot_cpp,
                 "COPY_MEMBER(mData)COPY_MEMBER(mDataIndex)",
                 "CharIKFoot source copy mirrors member list");
  ok &= contains(char_clip_h,
                 "structSourceCharIKFootState{boolhelper_target_created=true;"
                 "boolhelper_target_local_reset=true;intfsm_state=0;",
                 "native exposes CharIKFoot source state");
  ok &= contains(char_clip_h,
                 "std::array<float,3>planted_pos={0.0f,0.0f,0.0f};"
                 "floatrelease_distance=0.0f;",
                 "native stores CharIKFoot FSM state rows");
  ok &= contains(char_clip_h,
                 "SourceCharIKFootStatesource_char_ik_foot_default_state();",
                 "native API exposes CharIKFoot defaults helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKFootFsmResultsource_char_ik_foot_do_fsm(",
                 "native API exposes CharIKFoot FSM helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKFootLoadStepssource_char_ik_foot_load_steps("
                 "int32_trevision);",
                 "native API exposes CharIKFoot load helper");
  ok &= contains(char_clip,
                 "SourceCharIKFootStatesource_char_ik_foot_default_state(){"
                 "returnSourceCharIKFootState{};}",
                 "native CharIKFoot defaults helper mirrors constructor state");
  ok &= contains(char_clip,
                 "state.fsm_state=0;result.reset_fsm_state=true;"
                 "state.release_distance=0.0f;",
                 "native CharIKFoot Enter helper resets FSM state");
  ok &= contains(char_clip,
                 "result.call_hmx_set_name=true;result.assigned_character="
                 "dir_is_character;state.character_dir=dir_is_character?"
                 "dir_name:std::string{};",
                 "native CharIKFoot SetName helper mirrors Character cast");
  ok &= contains(char_clip,
                 "if(!has_finger||!has_hand||!has_data)returnplan;",
                 "native CharIKFoot Poll plan mirrors source missing-ref return");
  ok &= contains(char_clip,
                 "plan.push_helper_target=true;plan.run_do_fsm=true;"
                 "plan.call_char_ik_hand_poll=true;",
                 "native CharIKFoot Poll plan mirrors helper target delegation");
  ok &= contains(char_clip,
                 "plan.call_char_ik_hand_poll_deps=true;",
                 "native CharIKFoot PollDeps plan mirrors CharIKHand delegation");
  ok &= contains(char_clip,
                 "if(character_teleported)state.fsm_state=0;",
                 "native CharIKFoot FSM mirrors teleport reset");
  ok &= contains(char_clip,
                 "if(delta_seconds<0.0f){delta_seconds=0.0f;"
                 "result.clamped_negative_delta=true;}",
                 "native CharIKFoot FSM mirrors negative delta clamp");
  ok &= contains(char_clip,
                 "result.copied_finger_matrix=true;std::array<float,3>target="
                 "current_target_pos;target[2]=finger_world_pos[2];"
                 "state.planted_pos[2]=target[2];",
                 "native CharIKFoot FSM mirrors finger matrix/z setup");
  ok &= contains(char_clip,
                 "constfloatthreshold=state.fsm_state==1?0.6f:0.5f;",
                 "native CharIKFoot FSM mirrors planted thresholds");
  ok &= contains(char_clip,
                 "if(len>0.125f){constfloatscale=0.125f/len;",
                 "native CharIKFoot FSM mirrors planted travel clamp");
  ok &= contains(char_clip,
                 "state.release_distance=std::min(state.release_distance-"
                 "delta_seconds*25.0f,len);",
                 "native CharIKFoot FSM mirrors release decay");
  ok &= contains(char_clip,
                 "steps.known_revision=revision>=0&&revision<=steps.max_revision;",
                 "native CharIKFoot load helper mirrors revision range");
  ok &= contains(char_clip,
                 "steps.read_legacy_symbol=revision<6;",
                 "native CharIKFoot load helper mirrors legacy symbol gate");
  ok &= contains(char_clip,
                 "if(revision<5){if(revision>1)++steps.legacy_int_reads;"
                 "if(revision>2)++steps.legacy_int_reads;if(revision>3)"
                 "++steps.legacy_int_reads;}else{steps.load_data=true;"
                 "steps.load_data_index=true;}",
                 "native CharIKFoot load helper mirrors legacy/data gates");
  ok &= contains(char_clip,
                 "dest.data=source.data;result.copy_data=true;dest.data_index="
                 "source.data_index;",
                 "native CharIKFoot copy helper mirrors member copy");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_foot_source_test"
                 "character_ik_foot_source_test.cpp)",
                 "CMake builds focused CharIKFoot source test");
  ok &= contains(ik_foot_source_test,
                 "source_char_ik_foot_default_state()",
                 "focused CharIKFoot test covers source defaults");
  ok &= contains(ik_foot_source_test,
                 "source_char_ik_foot_poll_plan(true,true,true)",
                 "focused CharIKFoot test covers Poll plan");
  ok &= contains(ik_foot_source_test,
                 "source_char_ik_foot_do_fsm(",
                 "focused CharIKFoot test covers FSM helper");
  ok &= contains(ik_foot_source_test,
                 "source_char_ik_foot_load_steps(6)",
                 "focused CharIKFoot test covers load gates");
  ok &= contains(ik_foot_source_test,
                 "source_char_ik_foot_copy(dest,source)",
                 "focused CharIKFoot test covers copy helper");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharIKFoot.cpp`",
                 "document cites CharIKFoot source");
  ok &= contains(doc,
                 "Native `source_char_ik_foot_*` helpers port that "
                 "foot-specific plan and FSM",
                 "document records native CharIKFoot helper boundary");
  ok &= contains(doc,
                 "They do not add a decoded `CharIKFoot` row hookup",
                 "document fences CharIKFoot live row hookup");
  ok &= contains(rb3_latest_char_ik_midi_h,
                 "ObjPtr<RndTransformable,ObjectDir>mBone;",
                 "latest CharIKMidi source header exposes driven bone");
  ok &= contains(rb3_latest_char_ik_midi_h,
                 "ObjPtr<CharWeightable,ObjectDir>mAnimBlender;",
                 "latest CharIKMidi source header exposes anim blend owner");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(5,0)",
                 "CharIKMidi source load enforces revision ceiling");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "LOAD_SUPERCLASS(Hmx::Object)bs>>mBone;",
                 "CharIKMidi source load reads object then bone");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "if(gRev<3){ObjVector<ObjPtr<RndTransformable,ObjectDir>>"
                 "vec(this);bs>>vec;}",
                 "CharIKMidi source load gates legacy spot vector");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "if(gRev==2||gRev==3){Stringasdf;bs>>asdf;}",
                 "CharIKMidi source load gates legacy string");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "if(gRev>4){bs>>mAnimBlender;bs>>mMaxAnimBlend;}",
                 "CharIKMidi source load gates anim blend rows");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "CharIKMidi::CharIKMidi():mBone(this,0),mCurSpot(this,0),"
                 "mNewSpot(this,0),mSpotChanged(0),mAnimBlender(this,0),"
                 "mMaxAnimBlend(1.0f),mAnimFracPerBeat(0.0f),"
                 "mAnimFrac(0.0f){Enter();}",
                 "CharIKMidi source constructor exposes defaults");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "voidCharIKMidi::Enter(){mCurSpot=0;mNewSpot=0;"
                 "mSpotChanged=false;mFrac=0.0f;mFracPerBeat=0.0f;"
                 "mLocalXfm.Reset();mOldLocalXfm.Reset();"
                 "RndPollable::Enter();}",
                 "CharIKMidi source Enter resets spot interpolation state");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "voidCharIKMidi::PollDeps(std::list<Hmx::Object*>&"
                 "changedBy,std::list<Hmx::Object*>&change){"
                 "change.push_back(mBone);changedBy.push_back(mBone);"
                 "changedBy.push_back(mCurSpot);}",
                 "CharIKMidi source PollDeps publishes bone and current spot");
  ok &= contains(rb3_latest_char_ik_midi_cpp,
                 "BEGIN_COPYS(CharIKMidi)COPY_SUPERCLASS(Hmx::Object)"
                 "CREATE_COPY(CharIKMidi)BEGIN_COPYING_MEMBERS"
                 "COPY_MEMBER(mBone)COPY_MEMBER(mAnimBlender)"
                 "COPY_MEMBER(mMaxAnimBlend)",
                 "CharIKMidi source Copy member list");
  ok &= contains(char_mesh_h,
                 "structCharIKMidi{std::stringname;int32_tversion=0;"
                 "std::stringbone;",
                 "native CharIKMidi stores source revision and bone");
  ok &= contains(char_mesh_h,
                 "std::vector<std::string>legacy_spots;"
                 "std::stringlegacy_string;std::stringanim_blender;",
                 "native CharIKMidi stores source-gated optional rows");
  ok &= contains(char_mesh,
                 "midi.version=r.i32();",
                 "native CharIKMidi decoder reads source revision");
  ok &= contains(char_mesh,
                 "if(midi.version<0||midi.version>5){throwstd::runtime_error",
                 "native CharIKMidi decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "read_object_fields(r);midi.bone=r.str();",
                 "native CharIKMidi decoder mirrors source object/bone order");
  ok &= contains(char_mesh,
                 "if(midi.version<3){midi.legacy_spots=read_obj_ptr_list(r);}",
                 "native CharIKMidi decoder mirrors legacy vector gate");
  ok &= contains(char_mesh,
                 "if(midi.version==2||midi.version==3){"
                 "midi.legacy_string=r.str();}",
                 "native CharIKMidi decoder mirrors legacy string gate");
  ok &= contains(char_mesh,
                 "if(midi.version>4){midi.anim_blender=r.str();"
                 "midi.max_anim_blend=r.f32();}",
                 "native CharIKMidi decoder mirrors anim blend gate");
  ok &= contains(char_clip_h,
                 "structSourceCharIKMidiState{std::stringbone;"
                 "std::stringcur_spot;std::stringnew_spot;",
                 "native exposes CharIKMidi source state");
  ok &= contains(char_clip_h,
                 "structSourceCharIKMidiEnterResult{boolclear_cur_spot=true;"
                 "boolclear_new_spot=true;boolclear_spot_changed=true;",
                 "native exposes CharIKMidi Enter result");
  ok &= contains(char_clip_h,
                 "structSourceCharIKMidiPollDeps{std::vector<std::string>"
                 "changed_by;std::vector<std::string>change;};",
                 "native exposes CharIKMidi PollDeps result");
  ok &= contains(char_clip_h,
                 "structSourceCharIKMidiLoadSteps{boolknown_revision=false;"
                 "boolload_hmx_object=false;boolload_bone=false;",
                 "native exposes CharIKMidi load steps");
  ok &= contains(char_clip_h,
                 "structSourceCharIKMidiCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>"
                 "copied_members;};",
                 "native exposes CharIKMidi copy plan");
  ok &= contains(char_clip_h,
                 "SourceCharIKMidiStatesource_char_ik_midi_default_state();",
                 "native exposes CharIKMidi default helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKMidiEnterResultsource_char_ik_midi_enter("
                 "SourceCharIKMidiState&state);",
                 "native exposes CharIKMidi Enter helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_ik_midi_poll_deps("
                 "SourceCharIKMidiPollDeps&deps,constSourceCharIKMidiState&"
                 "state);",
                 "native exposes CharIKMidi PollDeps helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKMidiLoadStepssource_char_ik_midi_load_steps("
                 "int32_trevision);",
                 "native exposes CharIKMidi load helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKMidiCopyPlansource_char_ik_midi_copy_plan();",
                 "native exposes CharIKMidi copy helper");
  ok &= contains(char_clip,
                 "SourceCharIKMidiStatesource_char_ik_midi_default_state(){"
                 "SourceCharIKMidiStatestate;source_char_ik_midi_enter(state);"
                 "returnstate;}",
                 "native CharIKMidi default helper calls Enter like source");
  ok &= contains(char_clip,
                 "state.cur_spot.clear();state.new_spot.clear();"
                 "state.spot_changed=false;state.frac=0.0f;"
                 "state.frac_per_beat=0.0f;",
                 "native CharIKMidi Enter helper clears source spot interpolation state");
  ok &= contains(char_clip,
                 "state.local_xfm_reset=true;state.old_local_xfm_reset=true;",
                 "native CharIKMidi Enter helper resets source transforms");
  ok &= contains(char_clip,
                 "voidsource_char_ik_midi_poll_deps("
                 "SourceCharIKMidiPollDeps&deps,constSourceCharIKMidiState&"
                 "state){deps.change.push_back(state.bone);"
                 "deps.changed_by.push_back(state.bone);"
                 "deps.changed_by.push_back(state.cur_spot);}",
                 "native CharIKMidi PollDeps helper mirrors source");
  ok &= contains(char_clip,
                 "SourceCharIKMidiLoadStepssource_char_ik_midi_load_steps("
                 "int32_trevision){SourceCharIKMidiLoadStepssteps;"
                 "steps.known_revision=revision>=0&&revision<=5;",
                 "native CharIKMidi load helper enforces revision range");
  ok &= contains(char_clip,
                 "steps.load_legacy_spots=revision<3;"
                 "steps.load_legacy_string=revision==2||revision==3;"
                 "steps.load_anim_blend=revision>4;",
                 "native CharIKMidi load helper mirrors source gates");
  ok &= contains(char_clip,
                 "SourceCharIKMidiCopyPlansource_char_ik_midi_copy_plan(){"
                 "SourceCharIKMidiCopyPlanplan;plan.copied_superclasses="
                 "{\"Hmx::Object\"};plan.copied_members={\"mBone\","
                 "\"mAnimBlender\",\"mMaxAnimBlend\"};returnplan;}",
                 "native CharIKMidi copy helper mirrors source copy list");
  ok &= contains(bind_audit,
                 "\"[controller-ik-midi]char=%sname=%sversion=%d",
                 "controller audit logs CharIKMidi source revision");
  ok &= contains(bind_audit,
                 "\"boneExists=%dlegacySpots=%zulegacyString=%sanimBlender=%s",
                 "controller audit logs CharIKMidi source optional fields");
  ok &= contains(doc,
                 "`CharIKMidi::Load` accepts source revisions through 5",
                 "document records CharIKMidi source load boundary");
  ok &= contains(doc,
                 "inventory and enforces the source revision range",
                 "document records CharIKMidi revision enforcement");
  ok &= contains(doc,
                 "source_ikmidi_20260711/stock_ikmidi_controllers.stdout.log",
                 "document cites refreshed CharIKMidi proof log");
  ok &= contains(doc,
                 "all 19 stock\n    `CharIKMidi` rows are `version=4`, "
                 "target `bone_fret.mesh`, and report\n    `unreadBytes=0`",
                 "document records refreshed CharIKMidi stock proof");
  ok &= contains(doc,
                 "viewer/gameplay fret-target helper remains diagnostic",
                 "document fences CharIKMidi runtime helper");
  ok &= contains(doc,
                 "ikmidi_source_decode_audit.log",
                 "document records focused stock CharIKMidi audit");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "classCharIKSliderMidi:publicRndHighlightable,"
                 "publicCharWeightable,publicCharPollable",
                 "CharIKSliderMidi source header exposes inheritance");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "ObjPtr<RndTransformable,ObjectDir>mTarget;",
                 "CharIKSliderMidi source header exposes target ref");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "ObjPtr<RndTransformable,ObjectDir>mFirstSpot;",
                 "CharIKSliderMidi source header exposes first spot ref");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "ObjPtr<RndTransformable,ObjectDir>mSecondSpot;",
                 "CharIKSliderMidi source header exposes second spot ref");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "floatmTargetPercentage;",
                 "CharIKSliderMidi source header exposes target percentage");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "boolmPercentageChanged;",
                 "CharIKSliderMidi source header exposes percentage flag");
  ok &= contains(rb3_latest_char_ik_slider_midi_h, "boolmResetAll;",
                 "CharIKSliderMidi source header exposes reset flag");
  ok &= contains(rb3_latest_char_ik_slider_midi_h,
                 "ObjPtr<Character,ObjectDir>mMe;",
                 "CharIKSliderMidi source header exposes owning character");
  ok &= contains(rb3_latest_char_ik_slider_midi_h, "floatmTolerance;",
                 "CharIKSliderMidi source header exposes tolerance");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "CharIKSliderMidi::CharIKSliderMidi():mTarget(this,0),"
                 "mFirstSpot(this,0),mSecondSpot(this,0),"
                 "mTargetPercentage(1.0f),mPercentageChanged(0),"
                 "mResetAll(1),mMe(this,0),mTolerance(0.0f){Enter();}",
                 "CharIKSliderMidi source constructor exposes defaults");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "voidCharIKSliderMidi::Enter(){mPercentageChanged=false;"
                 "mFrac=0.0f;mFracPerBeat=0.0f;RndPollable::Enter();}",
                 "CharIKSliderMidi source Enter clears interpolation state");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "voidCharIKSliderMidi::SetName(constchar*cc,classObjectDir*"
                 "dir){Hmx::Object::SetName(cc,dir);mMe=dynamic_cast<class"
                 "Character*>(dir);}",
                 "CharIKSliderMidi source SetName stores Character dir");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "voidCharIKSliderMidi::SetupTransforms(){mResetAll=true;}",
                 "CharIKSliderMidi source SetupTransforms sets reset flag");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "change.push_back(mTarget);changedBy.push_back(mTarget);"
                 "changedBy.push_back(mFirstSpot);changedBy.push_back("
                 "mSecondSpot);",
                 "CharIKSliderMidi source PollDeps order");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "LOAD_REVS(bs);ASSERT_REVS(2,0);Hmx::Object::Load(bs);"
                 "if(gRev>1)CharWeightable::Load(bs);bs>>mTarget;"
                 "bs>>mFirstSpot;bs>>mSecondSpot;bs>>mTolerance;",
                 "CharIKSliderMidi source load order");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS(CharWeightable)"
                 "CREATE_COPY(CharIKSliderMidi)",
                 "CharIKSliderMidi source copy includes object and weightable");
  ok &= contains(rb3_latest_char_ik_slider_midi_cpp,
                 "COPY_MEMBER(mTarget)COPY_MEMBER(mFirstSpot)"
                 "COPY_MEMBER(mSecondSpot)COPY_MEMBER(mTolerance)",
                 "CharIKSliderMidi source copy mirrors member list");
  ok &= missing(rb3_latest_char_ik_slider_midi_cpp,
                "voidCharIKSliderMidi::Poll(",
                "available CharIKSliderMidi source lacks Poll body");
  ok &= missing(rb3_latest_char_ik_slider_midi_cpp,
                "voidCharIKSliderMidi::SetFraction(",
                "available CharIKSliderMidi source lacks SetFraction body");
  ok &= contains(char_clip_h,
                 "structSourceCharIKSliderMidiState{SourceCharWeightableState"
                 "weightable;",
                 "native exposes CharIKSliderMidi source state");
  ok &= contains(char_clip_h,
                 "floattarget_percentage=1.0f;",
                 "native stores CharIKSliderMidi target percentage default");
  ok &= contains(char_clip_h,
                 "boolpercentage_changed=false;boolreset_all=true;",
                 "native stores CharIKSliderMidi boolean defaults");
  ok &= contains(char_clip_h,
                 "SourceCharIKSliderMidiStatesource_char_ik_slider_midi_"
                 "default_state(",
                 "native API exposes CharIKSliderMidi defaults helper");
  ok &= contains(char_clip_h,
                 "SourceCharIKSliderMidiLoadStepssource_char_ik_slider_midi_"
                 "load_steps(",
                 "native API exposes CharIKSliderMidi load helper");
  ok &= contains(char_clip,
                 "SourceCharIKSliderMidiStatesource_char_ik_slider_midi_"
                 "default_state(conststd::string&name){SourceCharIKSliderMidi"
                 "Statestate;",
                 "native CharIKSliderMidi defaults helper exists");
  ok &= contains(char_clip,
                 "state.weightable=source_char_weightable_default_state(name);"
                 "source_char_ik_slider_midi_enter(state);returnstate;",
                 "native CharIKSliderMidi constructor helper calls Enter");
  ok &= contains(char_clip,
                 "state.percentage_changed=false;result.cleared_percentage_"
                 "changed=true;state.frac=0.0f;",
                 "native CharIKSliderMidi Enter helper clears state");
  ok &= contains(char_clip,
                 "result.call_rnd_pollable_enter=true;",
                 "native CharIKSliderMidi Enter helper records RndPollable call");
  ok &= contains(char_clip,
                 "result.call_hmx_set_name=true;result.assigned_character="
                 "dir_is_character;state.character_dir=dir_is_character?"
                 "dir_name:std::string{};",
                 "native CharIKSliderMidi SetName helper mirrors Character cast");
  ok &= contains(char_clip,
                 "state.reset_all=true;result.reset_all=true;",
                 "native CharIKSliderMidi SetupTransforms helper sets reset flag");
  ok &= contains(char_clip,
                 "deps.change.push_back(state.target);deps.changed_by.push_back"
                 "(state.target);deps.changed_by.push_back(state.first_spot);"
                 "deps.changed_by.push_back(state.second_spot);",
                 "native CharIKSliderMidi PollDeps helper mirrors source order");
  ok &= contains(char_clip,
                 "steps.known_revision=revision>=0&&revision<=steps.max_revision;"
                 "steps.load_hmx_object=true;steps.load_weightable=revision>1;",
                 "native CharIKSliderMidi load helper mirrors revision gate");
  ok &= contains(char_clip,
                 "dest.target=source.target;result.copy_target=true;"
                 "dest.first_spot=source.first_spot;",
                 "native CharIKSliderMidi copy helper mirrors member list");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_slider_midi_source_test"
                 "character_ik_slider_midi_source_test.cpp)",
                 "CMake builds focused CharIKSliderMidi source test");
  ok &= contains(ik_slider_midi_source_test,
                 "source_char_ik_slider_midi_default_state(\"slider.weight\")",
                 "focused CharIKSliderMidi test covers source defaults");
  ok &= contains(ik_slider_midi_source_test,
                 "source_char_ik_slider_midi_enter(slider)",
                 "focused CharIKSliderMidi test covers Enter helper");
  ok &= contains(ik_slider_midi_source_test,
                 "source_char_ik_slider_midi_poll_deps(deps,slider)",
                 "focused CharIKSliderMidi test covers PollDeps helper");
  ok &= contains(ik_slider_midi_source_test,
                 "source_char_ik_slider_midi_load_steps(2)",
                 "focused CharIKSliderMidi test covers load gates");
  ok &= contains(ik_slider_midi_source_test,
                 "source_char_ik_slider_midi_copy(dest,source,false,0.66f)",
                 "focused CharIKSliderMidi test covers copy helper");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharIKSliderMidi.cpp`",
                 "document cites CharIKSliderMidi source");
  ok &= contains(doc,
                 "Native `source_char_ik_slider_midi_*` helpers port these "
                 "concrete source",
                 "document records native CharIKSliderMidi helper boundary");
  ok &= contains(doc,
                 "does\n    not include reviewable `Poll` or `SetFraction` bodies",
                 "document fences CharIKSliderMidi missing Poll and SetFraction");
  ok &= contains(rb3_latest_char_ik_fingers_h,
                 "FingerDesc():unk0(0),unk8(0,0,0),unk14(0,0,0),"
                 "mFinger01(0),mFinger02(0),mFinger03(0),mFingertip(0),"
                 "unk60(0),unk64(0),unk68(1)",
                 "CharIKFingers source header exposes finger defaults");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "CharIKFingers::CharIKFingers():mHand(0,0),"
                 "mForeArm(0,0),mUpperArm(0,0),mBlendInFrames(0),"
                 "mBlendOutFrames(0),mResetHandDest(1),"
                 "mResetCurHandTrans(1),",
                 "CharIKFingers source constructor exposes hand defaults");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mFingerCurledLength(0.85f),mHandMoveForward(1.0f),"
                 "mHandPinkyRotation(-0.06f),mHandThumbRotation(0.23f),"
                 "mHandDestOffset(-0.4f),",
                 "CharIKFingers source constructor exposes finger tuning");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mIsRightHand(1),mMoveHand(0),mIsSetup(0),"
                 "mOutputTrans(this,0),mKeyboardRefBone(this,0)",
                 "CharIKFingers source constructor exposes output refs");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mFingers.resize(5,FingerDesc());",
                 "CharIKFingers source constructor creates five fingers");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mHandKeyboardOffset=Vector3(0.3f,-6.0f,0.4f);",
                 "CharIKFingers source constructor exposes keyboard offset");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mHand=dir->Find<RndTransformable>(\"bone_L-hand.mesh\",false);",
                 "CharIKFingers source SetName resolves left hand");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mFingers[kFingerMiddle].mFinger03=dir->Find"
                 "<RndTransformable>(\"bone_L-middlefinger03.mesh\",false);",
                 "CharIKFingers source SetName resolves left middle finger");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mFingers[kFingerIndex].mFingertip=dir->Find"
                 "<RndTransformable>(\"spot_R-index_tip.mesh\",false);",
                 "CharIKFingers source SetName resolves right fingertip");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mtx=Hmx::Matrix3(-0.023f,0.97899997f,0.201f,-0.228f,"
                 "0.191f,-0.95499998f,-0.972,-0.068f,0.21799999f);",
                 "CharIKFingers source SetName exposes right raw matrix");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mtx=Hmx::Matrix3(-0.067f,0.985f,0.156f,0.224f,0.167f,"
                 "-0.95999998f,-0.972f,-0.028999999f,-0.23199999f);",
                 "CharIKFingers source SetName exposes left raw matrix");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "if(!cur.mFinger01||!cur.mFinger02||!cur.mFinger03||"
                 "!cur.mFingertip){mIsSetup=false;break;}",
                 "CharIKFingers source setup completeness checks finger refs");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(5,0)LOAD_SUPERCLASS(Hmx::Object)"
                 "LOAD_SUPERCLASS(CharWeightable)",
                 "CharIKFingers source load enforces revision ceiling");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "if(gRev>1)bs>>mIsRightHand;if(gRev>2)bs>>mOutputTrans;"
                 "if(gRev>3)bs>>mKeyboardRefBone;",
                 "CharIKFingers source load gates hand side and refs");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "if(gRev>4){bs>>mHandKeyboardOffset;bs>>mHandThumbRotation;"
                 "bs>>mHandPinkyRotation;bs>>mHandMoveForward;"
                 "bs>>mHandDestOffset;}",
                 "CharIKFingers source load gates keyboard/finger tuning");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "voidCharIKFingers::SetFinger(Vector3v1,Vector3v2,"
                 "CharIKFingers::FingerNumfingerNum){MILO_ASSERT("
                 "fingerNum>=0&&fingerNum<kFingerNone,0x37);"
                 "FingerDesc&finger=mFingers[fingerNum];finger.unk8=v1;"
                 "finger.unk14=v2;finger.unk0=true;finger.unk84=true;",
                 "CharIKFingers source SetFinger exposes state writes");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "Multiply(finger.mFinger01->LocalXfm(),mCurHandTrans,tf48);",
                 "CharIKFingers source SetFinger exposes transform step");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "mBlendInFrames=5;finger.unk60=5;finger.unk64=0;}",
                 "CharIKFingers source SetFinger exposes blend counters");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "voidCharIKFingers::ReleaseFinger(FingerNumfinger){"
                 "MILO_ASSERT(finger>=0&&finger<kFingerNone,0x57);"
                 "mFingers[finger].unk0=false;mFingers[finger].unk84=true;"
                 "mFingers[finger].unk64=0;mFingers[finger].unk60=5;}",
                 "CharIKFingers source ReleaseFinger exposes state writes");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS(CharWeightable)"
                 "CREATE_COPY(CharIKFingers)BEGIN_COPYING_MEMBERS"
                 "COPY_MEMBER(mIsRightHand)COPY_MEMBER(mOutputTrans)"
                 "COPY_MEMBER(mKeyboardRefBone)COPY_MEMBER(mHandKeyboardOffset)",
                 "CharIKFingers source copy starts with expected members");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "COPY_MEMBER(mHandThumbRotation)COPY_MEMBER(mHandPinkyRotation)"
                 "COPY_MEMBER(mHandMoveForward)COPY_MEMBER(mHandDestOffset)",
                 "CharIKFingers source copy records hand tuning members");
  ok &= contains(rb3_latest_char_ik_fingers_cpp,
                 "voidCharIKFingers::Poll(){Hmx::Matrix3m;Vector3v;"
                 "mCurHandTrans.Set(m,v);}",
                 "CharIKFingers available Poll body is incomplete");
  ok &= missing(rb3_latest_char_ik_fingers_cpp,
                "voidCharIKFingers::MeasureLengths(){",
                "available CharIKFingers source lacks MeasureLengths body");
  ok &= contains(char_mesh_h,
                 "structSourceCharIKFingersState{intblend_in_frames=0;"
                 "intblend_out_frames=0;boolreset_hand_dest=true;",
                 "native exposes CharIKFingers source defaults struct");
  ok &= contains(char_mesh_h,
                 "std::array<float,3>hand_keyboard_offset={0.3f,-6.0f,0.4f};",
                 "native stores CharIKFingers keyboard offset default");
  ok &= contains(char_mesh_h,
                 "SourceCharIKFingersStatesource_char_ik_fingers_defaults();",
                 "native API exposes CharIKFingers defaults helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_ik_fingers_load_revision_known(intrevision);",
                 "native API exposes CharIKFingers revision helper");
  ok &= contains(char_mesh_h,
                 "SourceCharIKFingersSetupRefssource_char_ik_fingers_set_name_refs("
                 "boolis_right_hand);",
                 "native API exposes CharIKFingers SetName refs helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_ik_fingers_setup_complete("
                 "constSourceCharIKFingersSetupRefs&refs,"
                 "conststd::vector<std::string>&present_transforms);",
                 "native API exposes CharIKFingers setup completeness helper");
  ok &= contains(char_mesh_h,
                 "structSourceCharIKFingersSetFingerPlan{boolknown_finger=false;",
                 "native exposes CharIKFingers SetFinger plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharIKFingersLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;};",
                 "native exposes CharIKFingers load plan");
  ok &= contains(char_mesh_h,
                 "SourceCharIKFingersSetFingerPlansource_char_ik_fingers_set_finger_plan("
                 "intfinger);",
                 "native API exposes CharIKFingers SetFinger helper");
  ok &= contains(char_mesh_h,
                 "SourceCharIKFingersCopyPlansource_char_ik_fingers_copy_plan();",
                 "native API exposes CharIKFingers copy helper");
  ok &= contains(char_mesh,
                 "SourceCharIKFingersStatesource_char_ik_fingers_defaults(){"
                 "returnSourceCharIKFingersState{};}",
                 "native CharIKFingers defaults helper mirrors constructor");
  ok &= contains(char_mesh,
                 "returnrevision>=0&&revision<=5;",
                 "native CharIKFingers revision helper mirrors source range");
  ok &= contains(char_mesh,
                 "conststd::stringside=is_right_hand?\"R\":\"L\";"
                 "refs.hand=\"bone_\"+side+\"-hand.mesh\";",
                 "native CharIKFingers helper mirrors hand ref names");
  ok &= contains(char_mesh,
                 "{\"thumb\",\"index\",\"middlefinger\",\"ringfinger\",\"pinky\"}",
                 "native CharIKFingers helper mirrors finger name order");
  ok &= contains(char_mesh,
                 "refs.raw_matrix=is_right_hand?std::array<float,9>{-0.023f,"
                 "0.97899997f,0.201f,",
                 "native CharIKFingers helper carries right raw matrix");
  ok &= contains(char_mesh,
                 ":std::array<float,9>{-0.067f,0.985f,0.156f,",
                 "native CharIKFingers helper carries left raw matrix");
  ok &= contains(char_mesh,
                 "if(!present(finger.finger01)||!present(finger.finger02)||"
                 "!present(finger.finger03)||!present(finger.fingertip)){"
                 "returnfalse;}",
                 "native CharIKFingers setup helper mirrors finger-only loop");
  ok &= contains(char_mesh,
                 "SourceCharIKFingersSetFingerPlansource_char_ik_fingers_set_finger_plan("
                 "intfinger){SourceCharIKFingersSetFingerPlanplan;",
                 "native CharIKFingers SetFinger helper exists");
  ok &= contains(char_mesh,
                 "plan.assign_primary_vector=true;plan.assign_secondary_vector=true;"
                 "plan.set_active=true;plan.mark_dirty=true;"
                 "plan.multiply_finger01_by_current_hand=true;",
                 "native CharIKFingers SetFinger helper ports visible state writes");
  ok &= contains(char_mesh,
                 "SourceCharIKFingersReleaseFingerPlansource_char_ik_fingers_release_finger_plan("
                 "intfinger){SourceCharIKFingersReleaseFingerPlanplan;",
                 "native CharIKFingers ReleaseFinger helper exists");
  ok &= contains(char_mesh,
                 "plan.clear_active=true;plan.mark_dirty=true;",
                 "native CharIKFingers ReleaseFinger helper ports visible state writes");
  ok &= contains(char_mesh,
                 "SourceCharIKFingersLoadPlansource_char_ik_fingers_load_plan("
                 "intrevision){SourceCharIKFingersLoadPlanplan;",
                 "native CharIKFingers load plan helper exists");
  ok &= contains(char_mesh,
                 "if(revision>4){plan.read_order.push_back(\"mHandKeyboardOffset\");"
                 "plan.read_order.push_back(\"mHandThumbRotation\");",
                 "native CharIKFingers load plan ports revision 5 fields");
  ok &= contains(char_mesh,
                 "SourceCharIKFingersCopyPlansource_char_ik_fingers_copy_plan(){"
                 "SourceCharIKFingersCopyPlanplan;",
                 "native CharIKFingers copy plan helper exists");
  ok &= missing(char_mesh, "present(refs.hand)",
                "native CharIKFingers setup helper must not require hand ref");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_fingers_source_test"
                 "character_ik_fingers_source_test.cpp)",
                 "CMake builds focused CharIKFingers source test");
  ok &= contains(ik_fingers_source_test,
                 "source_char_ik_fingers_defaults()",
                 "focused CharIKFingers test covers source defaults");
  ok &= contains(ik_fingers_source_test,
                 "source_char_ik_fingers_set_name_refs(false)",
                 "focused CharIKFingers test covers left SetName refs");
  ok &= contains(ik_fingers_source_test,
                 "completeonlychecksfingerrefs",
                 "focused CharIKFingers test covers finger-only completeness");
  ok &= contains(ik_fingers_source_test,
                 "missingfingertipmakessetupincomplete",
                 "focused CharIKFingers test covers missing fingertip");
  ok &= contains(ik_fingers_source_test,
                 "source_char_ik_fingers_set_finger_plan(1)",
                 "focused CharIKFingers test covers SetFinger plan");
  ok &= contains(ik_fingers_source_test,
                 "source_char_ik_fingers_release_finger_plan(3)",
                 "focused CharIKFingers test covers ReleaseFinger plan");
  ok &= contains(ik_fingers_source_test,
                 "source_char_ik_fingers_load_plan(5)",
                 "focused CharIKFingers test covers revision 5 load plan");
  ok &= contains(ik_fingers_source_test,
                 "source_char_ik_fingers_copy_plan()",
                 "focused CharIKFingers test covers copy plan");
  ok &= contains(doc, "CharIKFingers.cpp",
                 "document cites CharIKFingers source");
  ok &= contains(doc,
                 "Native `source_char_ik_fingers_*` helpers\n    port these "
                 "data decisions",
                 "document records native CharIKFingers helper boundary");
  ok &= contains(doc,
                 "source_char_ik_fingers_set_finger_plan",
                 "document records CharIKFingers SetFinger helper");
  ok &= contains(doc,
                 "must\n    not be promoted into live fretting-finger behavior",
                 "document fences incomplete CharIKFingers runtime");
  ok &= contains(rb3_latest_char_servo_bone_h,
                 "classCharServoBone:publicRndHighlightable,publicCharPollable,"
                 "publicCharBonesMeshes",
                 "latest CharServoBone source header exposes inheritance");
  ok &= contains(rb3_latest_char_servo_bone_h, "SymbolmClipType;",
                 "latest CharServoBone source header exposes clip type");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "CharServoBone::CharServoBone():mPelvis(0),"
                 "mFacingRotDelta(0),mFacingPosDelta(0),mFacingRot(0),"
                 "mFacingPos(0),mMoveSelf(0),mDeltaChanged(0),"
                 "mRegulate(this,0){}",
                 "latest CharServoBone source constructor defaults");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "if(gRev>1)bs>>s;SetClipType(s);",
                 "CharServoBone source load gates clip type");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "ClearBones();CharBoneDir::StuffBones(*this,mClipType);",
                 "CharServoBone source SetClipType refills source bones");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "voidCharServoBone::Enter(){ZeroDeltas();mRegulate=0;"
                 "mDeltaChanged=false;mMoveSelf=mFacingPosDelta;}",
                 "CharServoBone source Enter resets deltas and move-self");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "voidCharServoBone::SetMoveSelf(boolb){if(mMoveSelf==b)"
                 "return;mMoveSelf=b;mDeltaChanged=true;}",
                 "CharServoBone source SetMoveSelf dirty rule");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "BEGIN_COPYS(CharServoBone)COPY_SUPERCLASS(Hmx::Object)"
                 "CREATE_COPY(CharServoBone)BEGIN_COPYING_MEMBERS"
                 "COPY_MEMBER(mMoveSelf)SetClipType(c->mClipType);",
                 "CharServoBone source Copy member list");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "mFacingPosDelta=(Vector3*)FindPtr(\"bone_facing_delta.pos\");",
                 "CharServoBone source realloc finds facing delta rows");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "voidCharServoBone::ZeroDeltas(){if(mFacingPosDelta)"
                 "mFacingPosDelta->Zero();if(!mFacingRotDelta)return;"
                 "*mFacingRotDelta=0.0f;}",
                 "CharServoBone source ZeroDeltas clears facing delta rows");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "voidCharServoBone::MoveToFacing(Transform&tf){if("
                 "*mFacingRot){RotateAboutZ(tf.m,*mFacingRot,tf.m);"
                 "RotateAboutZ(tf.v,*mFacingRot,tf.v);Normalize(tf.m,tf.m);}"
                 "tf.v+=*mFacingPos;}",
                 "CharServoBone source MoveToFacing rotates and offsets transform");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "voidCharServoBone::MoveToDeltaFacing(Transform&tf){"
                 "Vector3v18;Multiply(*mFacingPosDelta,tf.m,v18);tf.v+=v18;"
                 "if(*mFacingRotDelta){RotateAboutZ(tf.m,*mFacingRotDelta,"
                 "tf.m);Normalize(tf.m,tf.m);}}",
                 "CharServoBone source MoveToDeltaFacing applies local delta rows");
  ok &= contains(char_mesh_h,
                 "structCharServoBone{std::stringname;int32_tversion=0;"
                 "std::stringclip_type;size_tunread_bytes=0;};",
                 "native CharServoBone stores source load fields");
  ok &= contains(char_clip_h,
                 "voidsource_char_servo_bone_zero_deltas("
                 "std::array<float,3>&facing_pos_delta,"
                 "float&facing_rot_delta_radians);",
                 "native exposes bounded CharServoBone ZeroDeltas helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_servo_bone_move_to_facing("
                 "milo_scene::Xfm&xfm,conststd::array<float,3>&facing_pos,"
                 "floatfacing_rot_radians);",
                 "native exposes bounded CharServoBone MoveToFacing helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_servo_bone_move_to_delta_facing("
                 "milo_scene::Xfm&xfm,"
                 "conststd::array<float,3>&facing_pos_delta,"
                 "floatfacing_rot_delta_radians);",
                 "native exposes bounded CharServoBone MoveToDeltaFacing helper");
  ok &= contains(char_clip_h,
                 "structSourceCharServoBoneDefaultState{boolpelvis_null=true;"
                 "boolfacing_rot_delta_null=true;boolfacing_pos_delta_null=true;",
                 "native exposes CharServoBone default-state contract");
  ok &= contains(char_clip_h,
                 "structSourceCharServoBoneSetClipTypeStep{boolchanged=false;"
                 "boolassign_clip_type=false;boolclear_bones=false;"
                 "boolstuff_bones_from_dir=false;};",
                 "native exposes CharServoBone SetClipType contract");
  ok &= contains(char_clip_h,
                 "structSourceCharServoBoneEnterStep{boolzero_deltas=true;"
                 "boolclear_regulate=true;booldelta_changed=false;"
                 "boolmove_self=false;};",
                 "native exposes CharServoBone Enter contract");
  ok &= contains(char_clip_h,
                 "structSourceCharServoBoneSetMoveSelfStep{boolchanged=false;"
                 "boolmove_self=false;booldelta_changed=false;};",
                 "native exposes CharServoBone SetMoveSelf contract");
  ok &= contains(char_clip_h,
                 "structSourceCharServoBoneCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;"
                 "boolcalls_set_clip_type=true;};",
                 "native exposes CharServoBone Copy contract");
  ok &= contains(char_clip_h,
                 "SourceCharServoBoneDefaultStatesource_char_servo_bone_default_state();",
                 "native exposes CharServoBone default-state helper");
  ok &= contains(char_clip_h,
                 "SourceCharServoBoneSetClipTypeStep"
                 "source_char_servo_bone_set_clip_type_step("
                 "boolclip_type_changed);",
                 "native exposes CharServoBone SetClipType helper");
  ok &= contains(char_clip_h,
                 "SourceCharServoBoneEnterStepsource_char_servo_bone_enter("
                 "boolfacing_pos_delta_present);",
                 "native exposes CharServoBone Enter helper");
  ok &= contains(char_clip_h,
                 "SourceCharServoBoneSetMoveSelfStep"
                 "source_char_servo_bone_set_move_self(boolcurrent_move_self,"
                 "boolrequested_move_self);",
                 "native exposes CharServoBone SetMoveSelf helper");
  ok &= contains(char_clip_h,
                 "SourceCharServoBoneCopyPlansource_char_servo_bone_copy_plan();",
                 "native exposes CharServoBone Copy helper");
  ok &= contains(char_clip,
                 "SourceCharServoBoneDefaultStatesource_char_servo_bone_default_state(){"
                 "return{};}",
                 "native CharServoBone default-state helper follows source");
  ok &= contains(char_clip,
                 "SourceCharServoBoneSetClipTypeStep"
                 "source_char_servo_bone_set_clip_type_step("
                 "boolclip_type_changed){SourceCharServoBoneSetClipTypeStep"
                 "step;step.changed=clip_type_changed;if(clip_type_changed){",
                 "native CharServoBone SetClipType helper gates changed branch");
  ok &= contains(char_clip,
                 "step.assign_clip_type=true;step.clear_bones=true;"
                 "step.stuff_bones_from_dir=true;}",
                 "native CharServoBone SetClipType helper mirrors clear/stuff");
  ok &= contains(char_clip,
                 "SourceCharServoBoneEnterStep"
                 "source_char_servo_bone_enter(boolfacing_pos_delta_present){"
                 "SourceCharServoBoneEnterStepstep;step.move_self="
                 "facing_pos_delta_present;returnstep;}",
                 "native CharServoBone Enter helper mirrors move-self pointer");
  ok &= contains(char_clip,
                 "SourceCharServoBoneSetMoveSelfStep"
                 "source_char_servo_bone_set_move_self(boolcurrent_move_self,"
                 "boolrequested_move_self){SourceCharServoBoneSetMoveSelfStep"
                 "step;if(current_move_self==requested_move_self){",
                 "native CharServoBone SetMoveSelf helper gates no-op");
  ok &= contains(char_clip,
                 "step.changed=true;step.move_self=requested_move_self;"
                 "step.delta_changed=true;returnstep;}",
                 "native CharServoBone SetMoveSelf helper marks changed branch");
  ok &= contains(char_clip,
                 "SourceCharServoBoneCopyPlansource_char_servo_bone_copy_plan(){"
                 "SourceCharServoBoneCopyPlanplan;plan.copied_superclasses="
                 "{\"Hmx::Object\"};plan.copied_members={\"mMoveSelf\"};"
                 "plan.calls_set_clip_type=true;returnplan;}",
                 "native CharServoBone Copy helper mirrors source copy");
  ok &= contains(char_clip,
                 "voidsource_char_servo_bone_zero_deltas("
                 "std::array<float,3>&facing_pos_delta,"
                 "float&facing_rot_delta_radians){facing_pos_delta={0.0f,"
                 "0.0f,0.0f};facing_rot_delta_radians=0.0f;}",
                 "native CharServoBone ZeroDeltas helper follows source");
  ok &= contains(char_clip,
                 "voidsource_char_servo_bone_move_to_facing("
                 "milo_scene::Xfm&xfm,conststd::array<float,3>&facing_pos,"
                 "floatfacing_rot_radians){if(facing_rot_radians!=0.0f){"
                 "post_rotate_axis(xfm,ClipChannel::kRotZ,facing_rot_radians);"
                 "source_rotate_about_z_vec(xfm.pos,facing_rot_radians);",
                 "native CharServoBone MoveToFacing helper rotates transform");
  ok &= contains(char_clip,
                 "xfm.pos[0]+=facing_pos[0];xfm.pos[1]+=facing_pos[1];"
                 "xfm.pos[2]+=facing_pos[2];}",
                 "native CharServoBone MoveToFacing helper offsets position");
  ok &= contains(char_clip,
                 "voidsource_char_servo_bone_move_to_delta_facing("
                 "milo_scene::Xfm&xfm,"
                 "conststd::array<float,3>&facing_pos_delta,"
                 "floatfacing_rot_delta_radians){constfloatdx="
                 "facing_pos_delta[0]*xfm.rot[0][0]+",
                 "native CharServoBone MoveToDeltaFacing helper transforms delta");
  ok &= contains(char_mesh, "CharServoBonedecode_servo_bone(",
                 "native CharServoBone decoder exists");
  ok &= contains(char_mesh, "servo.version=r.i32();",
                 "native CharServoBone decoder reads revision");
  ok &= contains(char_mesh,
                 "if(servo.version<0||servo.version>2){throwstd::runtime_error",
                 "native CharServoBone decoder enforces source revision range");
  ok &= contains(char_mesh, "read_object_fields(r);//Hmx::Objectmetadata.",
                 "native CharServoBone decoder reads object fields");
  ok &= contains(char_mesh,
                 "if(servo.version>1)servo.clip_type=r.str();",
                 "native CharServoBone decoder mirrors source clip_type gate");
  ok &= contains(char_mesh, "servo.unread_bytes=r.n-r.pos;",
                 "native CharServoBone decoder records source tail bytes");
  ok &= contains(char_mesh,
                 "out.servo_bones.push_back(decode_servo_bone(de.name,b));",
                 "character load stores decoded CharServoBone rows");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_default_state()",
                 "focused CharBones test covers CharServoBone defaults");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_set_clip_type_step(true)",
                 "focused CharBones test covers CharServoBone SetClipType");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_enter(true)",
                 "focused CharBones test covers CharServoBone Enter");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_set_move_self(false,true)",
                 "focused CharBones test covers CharServoBone SetMoveSelf");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_copy_plan()",
                 "focused CharBones test covers CharServoBone Copy");
  ok &= contains(rb3_latest_char_trans_copy_h,
                 "classCharTransCopy:publicCharPollable",
                 "latest CharTransCopy header exposes source class");
  ok &= contains(rb3_latest_char_trans_copy_cpp,
                 "voidCharTransCopy::Poll(){if(!mSrc||!mDest)return;"
                 "mDest->SetLocalXfm(mSrc->mLocalXfm);}",
                 "CharTransCopy source Poll copies local transform");
  ok &= contains(rb3_latest_char_trans_copy_cpp,
                 "voidCharTransCopy::PollDeps(std::list<Hmx::Object*>&"
                 "changedBy,std::list<Hmx::Object*>&change){change.push_back"
                 "(mDest);changedBy.push_back(mSrc);}",
                 "CharTransCopy source PollDeps publishes dest/src");
  ok &= contains(rb3_latest_char_trans_copy_cpp,
                 "voidCharTransCopy::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(1,0);Hmx::Object::Load(bs);bs>>mSrc;bs>>mDest;}",
                 "CharTransCopy source Load reads src and dest");
  ok &= contains(char_mesh_h,
                 "structSourceCharTransCopyPollDeps{std::vector<std::string>"
                 "changed_by;std::vector<std::string>change;};",
                 "native exposes CharTransCopy dependency helper state");
  ok &= contains(char_mesh,
                 "boolsource_char_trans_copy_poll(constmilo_scene::Xfm*src,"
                 "milo_scene::Xfm*dest){if(src==nullptr||dest==nullptr)"
                 "returnfalse;*dest=*src;returntrue;}",
                 "native ports CharTransCopy Poll null-gated copy");
  ok &= contains(char_mesh,
                 "voidsource_char_trans_copy_poll_deps("
                 "SourceCharTransCopyPollDeps&deps,conststd::string&src,"
                 "conststd::string&dest){deps.change.push_back(dest);"
                 "deps.changed_by.push_back(src);}",
                 "native ports CharTransCopy PollDeps direction");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_trans_copy_source_test",
                 "CMake builds CharTransCopy source test");
  ok &= contains(trans_copy_source_test,
                 "source_char_trans_copy_poll(nullptr,&dest)",
                 "focused CharTransCopy test covers missing source");
  ok &= contains(trans_copy_source_test,
                 "source_char_trans_copy_poll(&src,&dest)",
                 "focused CharTransCopy test covers local transform copy");
  ok &= contains(trans_copy_source_test,
                 "source_char_trans_copy_poll_deps(deps,\"source.trans\","
                 "\"dest.trans\")",
                 "focused CharTransCopy test covers dependency direction");
  ok &= contains(doc,
                 "Native `source_char_trans_copy_poll` and",
                 "document records native CharTransCopy helper");
  ok &= contains(rb3_latest_char_poll_group_h,
                 "classCharPollGroup:publicCharPollable,publicCharWeightable",
                 "latest CharPollGroup header exposes source class");
  ok &= contains(rb3_latest_char_poll_group_cpp,
                 "voidCharPollGroup::Poll(){if(mWeightOwner->mWeight!=0.0f){"
                 "for(ObjPtrList<CharPollable,ObjectDir>::iteratorit="
                 "mPolls.begin();it!=mPolls.end();++it){(*it)->Poll();}}}",
                 "CharPollGroup source Poll gates on nonzero weight");
  ok &= contains(rb3_latest_char_poll_group_cpp,
                 "voidCharPollGroup::ListPollChildren(std::list<RndPollable*>&"
                 "l)const{ObjPtrList<CharPollable,ObjectDir>::iteratorit="
                 "mPolls.begin();ObjPtrList<CharPollable,ObjectDir>::iterator"
                 "itEnd=mPolls.end();for(;it!=itEnd;++it){l.push_back(*it);}}",
                 "CharPollGroup source ListPollChildren appends poll list");
  ok &= contains(rb3_latest_char_poll_group_cpp,
                 "voidCharPollGroup::PollDeps(std::list<Hmx::Object*>&changedBy,"
                 "std::list<Hmx::Object*>&change){if(mChangedBy||mChanges){"
                 "changedBy.push_back(mChangedBy);change.push_back(mChanges);}",
                 "CharPollGroup source PollDeps explicit override");
  ok &= contains(rb3_latest_char_poll_group_cpp,
                 "else{for(ObjPtrList<CharPollable,ObjectDir>::iteratorit="
                 "mPolls.begin();it!=mPolls.end();++it){(*it)->PollDeps"
                 "(changedBy,change);}}}",
                 "CharPollGroup source PollDeps delegates to children");
  ok &= contains(char_mesh_h,
                 "structSourceCharPollGroupChildDeps{std::stringchanged_by;"
                 "std::stringchange;};",
                 "native exposes CharPollGroup child dependency row");
  ok &= contains(char_mesh_h,
                 "structSourceCharPollGroupPollDeps{std::vector<std::string>"
                 "changed_by;std::vector<std::string>change;};",
                 "native exposes CharPollGroup dependency result row");
  ok &= contains(char_mesh,
                 "std::vector<std::string>source_char_poll_group_poll_order("
                 "floatweight,conststd::vector<std::string>&polls){"
                 "if(weight==0.0f)return{};returnpolls;}",
                 "native ports CharPollGroup Poll decision");
  ok &= contains(char_mesh,
                 "std::vector<std::string>source_char_poll_group_list_children("
                 "conststd::vector<std::string>&polls){returnpolls;}",
                 "native ports CharPollGroup ListPollChildren");
  ok &= contains(char_mesh,
                 "voidsource_char_poll_group_poll_deps("
                 "SourceCharPollGroupPollDeps&deps,conststd::vector<"
                 "SourceCharPollGroupChildDeps>&child_deps,conststd::string&"
                 "changed_by_override,conststd::string&change_override){"
                 "if(!changed_by_override.empty()||!change_override.empty()){"
                 "deps.changed_by.push_back(changed_by_override);"
                 "deps.change.push_back(change_override);return;}",
                 "native ports CharPollGroup PollDeps override branch");
  ok &= contains(char_mesh,
                 "for(constSourceCharPollGroupChildDeps&child:child_deps){"
                 "deps.changed_by.push_back(child.changed_by);"
                 "deps.change.push_back(child.change);}",
                 "native ports CharPollGroup PollDeps child branch");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_poll_group_source_test",
                 "CMake builds CharPollGroup source test");
  ok &= contains(poll_group_source_test,
                 "source_char_poll_group_poll_order(0.0f,polls)",
                 "focused CharPollGroup test covers zero weight");
  ok &= contains(poll_group_source_test,
                 "source_char_poll_group_poll_order(-0.5f,polls)",
                 "focused CharPollGroup test covers nonzero weight");
  ok &= contains(poll_group_source_test,
                 "source_char_poll_group_poll_deps(override_deps,child_deps,"
                 "\"\",\"override\")",
                 "focused CharPollGroup test covers explicit deps override");
  ok &= contains(doc,
                 "Native `source_char_poll_group_poll_order`,",
                 "document records native CharPollGroup helpers");
  ok &= contains(rb3_latest_char_ik_scale_h,
                 "classCharIKScale:publicCharWeightable,publicCharPollable",
                 "latest CharIKScale header exposes source class");
  ok &= contains(rb3_latest_char_ik_scale_cpp,
                 "CharIKScale::CharIKScale():mDest(this,0),mScale(1.0f),"
                 "mSecondaryTargets(this,kObjListNoNull),mBottomHeight(0.0f),"
                 "mTopHeight(0.0f),mAutoWeight(0)",
                 "CharIKScale source constructor defaults");
  ok &= contains(rb3_latest_char_ik_scale_cpp,
                 "voidCharIKScale::Poll(){if(mDest&&Weight()){}",
                 "CharIKScale source Poll has empty gated body");
  ok &= contains(rb3_latest_char_ik_scale_cpp,
                 "voidCharIKScale::CaptureBefore(){if(!mDest)return;"
                 "mScale=mDest->mLocalXfm.v.z;}",
                 "CharIKScale source CaptureBefore stores local z");
  ok &= contains(rb3_latest_char_ik_scale_cpp,
                 "voidCharIKScale::CaptureAfter(){if(!mDest)return;"
                 "mScale=mDest->mLocalXfm.v.z/mScale;}",
                 "CharIKScale source CaptureAfter divides by stored scale");
  ok &= contains(rb3_latest_char_ik_scale_cpp,
                 "voidCharIKScale::PollDeps(std::list<Hmx::Object*>&changedBy,"
                 "std::list<Hmx::Object*>&change){change.push_back(mDest);"
                 "for(ObjPtrList<RndTransformable,classObjectDir>::iterator"
                 "it=mSecondaryTargets.begin();it!=mSecondaryTargets.end();"
                 "++it){change.push_back(*it);}changedBy.push_back(mDest);}",
                 "CharIKScale source PollDeps order");
  ok &= contains(rb3_latest_char_ik_scale_cpp,
                 "voidCharIKScale::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(3,0);Hmx::Object::Load(bs);"
                 "CharWeightable::Load(bs);bs>>mDest;bs>>mScale;if(gRev>1)"
                 "bs>>mSecondaryTargets;if(gRev>2){bs>>mAutoWeight>>"
                 "mBottomHeight>>mTopHeight;}}",
                 "CharIKScale source Load revision gates");
  ok &= contains(char_mesh_h,
                 "structSourceCharIKScaleDefaultState{floatscale=1.0f;"
                 "floatbottom_height=0.0f;floattop_height=0.0f;"
                 "boolauto_weight=false;};",
                 "native exposes CharIKScale default state");
  ok &= contains(char_mesh,
                 "SourceCharIKScaleDefaultStatesource_char_ik_scale_default_state(){"
                 "returnSourceCharIKScaleDefaultState{};}",
                 "native ports CharIKScale defaults");
  ok &= contains(char_mesh,
                 "boolsource_char_ik_scale_poll_enters(boolhas_dest,floatweight){"
                 "returnhas_dest&&weight!=0.0f;}",
                 "native ports CharIKScale Poll gate");
  ok &= contains(char_mesh,
                 "returnhas_dest?dest_local_z:current_scale;",
                 "native ports CharIKScale CaptureBefore missing-dest gate");
  ok &= contains(char_mesh,
                 "returnhas_dest?dest_local_z/current_scale:current_scale;",
                 "native ports CharIKScale CaptureAfter source division");
  ok &= contains(char_mesh,
                 "voidsource_char_ik_scale_poll_deps(SourceCharIKScalePollDeps&"
                 "deps,conststd::string&dest,conststd::vector<std::string>&"
                 "secondary_targets){deps.change.push_back(dest);for("
                 "conststd::string&target:secondary_targets){deps.change."
                 "push_back(target);}deps.changed_by.push_back(dest);}",
                 "native ports CharIKScale PollDeps order");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_scale_source_test",
                 "CMake builds CharIKScale source test");
  ok &= contains(ik_scale_source_test,
                 "source_char_ik_scale_poll_enters(true,-0.25f)",
                 "focused CharIKScale test covers nonzero poll gate");
  ok &= contains(ik_scale_source_test,
                 "source_char_ik_scale_capture_before(true,8.0f,2.5f)",
                 "focused CharIKScale test covers CaptureBefore");
  ok &= contains(ik_scale_source_test,
                 "source_char_ik_scale_capture_after(true,12.0f,4.0f)",
                 "focused CharIKScale test covers CaptureAfter");
  ok &= contains(ik_scale_source_test,
                 "source_char_ik_scale_poll_deps(",
                 "focused CharIKScale test covers PollDeps");
  ok &= contains(doc,
                 "Native `source_char_ik_scale_*` helpers port",
                 "document records native CharIKScale helpers");
  ok &= contains(char_mesh_h,
                 "enumclassSourceCharacterPollState:int32_t{kCreated=0,"
                 "kSyncObject=1,kEntered=2,kPolled=3,kExited=4,};",
                 "native exposes source character poll-state enum order");
  ok &= contains(char_mesh,
                 "SourceCharacterStatesource_character_default_state(){"
                 "returnSourceCharacterState{};}",
                 "native ports Character constructor defaults helper");
  ok &= contains(char_mesh,
                 "voidsource_character_enter(SourceCharacterState&state){"
                 "state.poll_state=SourceCharacterPollState::kEntered;"
                 "state.min_lod=-1;state.frozen=false;state.last_lod=0;"
                 "state.teleported=true;state.interest_to_force.clear();}",
                 "native ports Character Enter state flow");
  ok &= contains(char_mesh,
                 "SourceCharacterPollResultsource_character_poll("
                 "SourceCharacterState&state){SourceCharacterPollResultresult;"
                 "if(state.frozen){result.skipped_for_frozen=true;"
                 "returnresult;}result.called_rnd_dir_poll=true;"
                 "state.teleported=false;state.poll_state="
                 "SourceCharacterPollState::kPolled;returnresult;}",
                 "native ports Character Poll frozen gate");
  ok &= contains(char_mesh,
                 "returnhas_driver&&driver_bones_is_servo;",
                 "native ports Character BoneServo driver/type gate");
  ok &= contains(char_mesh,
                 "if(is_char_pollable&&is_char_driver&&object_name=="
                 "\"main.drv\"){state.has_driver=true;"
                 "result.assigned_main_driver=true;}",
                 "native ports Character main driver assignment");
  ok &= contains(char_mesh,
                 "if(object_is_current_driver){state.has_driver=false;"
                 "result.cleared_driver=true;}result."
                 "called_rnd_dir_removing_object=true;",
                 "native ports Character RemovingObject driver clear");
  ok &= contains(char_mesh,
                 "result.called_rnd_dir_replace=true;if(from_is_sphere_base){"
                 "result.repointed_sphere_base=true;state.sphere_base_is_self="
                 "!to_is_transformable;result.fell_back_to_self="
                 "!to_is_transformable;}",
                 "native ports Character Replace sphere-base fallback");
  ok &= contains(char_mesh,
                 "state.poll_state=SourceCharacterPollState::kSyncObject;"
                 "result.converted_bones_to_transes=has_bone_pelvis_mesh;"
                 "result.called_rnd_dir_sync_objects=true;"
                 "result.removed_trans_group=true;",
                 "native ports Character SyncObjects prefix");
  ok &= contains(char_mesh,
                 "SourceCharacterSetSphereBaseResult"
                 "source_character_set_sphere_base(SourceCharacterState&state,"
                 "boolhas_transform){SourceCharacterSetSphereBaseResultresult;",
                 "native ports Character SetSphereBase helper");
  ok &= contains(char_mesh,
                 "result.defaulted_to_self=!has_transform;result."
                 "made_world_sphere=true;result.multiplied_by_trans_world="
                 "true;result.set_sphere=true;state.sphere_base_is_self="
                 "!has_transform;state.sphere_base_is_null=false;",
                 "native ports Character SetSphereBase state flow");
  ok &= contains(char_mesh,
                 "SourceCharacterSetInterestObjectsResult"
                 "source_character_set_interest_objects(boolhas_eyes,"
                 "conststd::vector<bool>&validate_results,boolhas_override_dir)",
                 "native ports Character SetInterestObjects helper");
  ok &= contains(char_mesh,
                 "if(!has_eyes)returnresult;result.cleared_all=true;"
                 "for(boolvalid:validate_results){++result.validated_count;",
                 "native ports Character SetInterestObjects eyes gate");
  ok &= contains(char_mesh,
                 "SourceCharacterAddShadowBoneResult"
                 "source_character_add_shadow_bone(int32_tcurrent_shadow_bones,"
                 "boolhas_transform,boolalready_hooked)",
                 "native ports Character AddShadowBone helper");
  ok &= contains(char_mesh,
                 "if(!has_transform){result.returned_null=true;returnresult;}"
                 "if(already_hooked){result.returned_existing=true;"
                 "returnresult;}result.created=true;++result."
                 "final_shadow_bones;",
                 "native ports Character AddShadowBone branches");
  ok &= contains(char_mesh,
                 "SourceCharacterSyncShadowResultsource_character_sync_shadow("
                 "boolhas_shadow,boolold_gfx,conststd::vector<int32_t>&"
                 "mesh_bone_counts)",
                 "native ports Character SyncShadow helper");
  ok &= contains(char_mesh,
                 "result.unhooked_shadow=true;if(!has_shadow)returnresult;"
                 "if(old_gfx){for(int32_tbone_count:mesh_bone_counts){",
                 "native ports Character SyncShadow gate");
  ok &= contains(char_mesh,
                 "SourceCharacterCopyBoundingSphereResult"
                 "source_character_copy_bounding_sphere("
                 "SourceCharacterState&state,boolsource_has_sphere_base)",
                 "native ports Character CopyBoundingSphere helper");
  ok &= contains(char_mesh,
                 "SourceCharacterRepointSphereBaseResult"
                 "source_character_repoint_sphere_base("
                 "SourceCharacterState&state,boolfound_matching_transform)",
                 "native ports Character RepointSphereBase helper");
  ok &= contains(char_mesh,
                 "SourceCharacterPreSaveResultsource_character_pre_save(){"
                 "return{true};}",
                 "native ports Character PreSave helper");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_character_source_test",
                 "CMake builds Character source test");
  ok &= contains(character_source_test,
                 "source_character_enter(state)",
                 "focused Character test covers Enter");
  ok &= contains(character_source_test,
                 "source_character_poll(state)",
                 "focused Character test covers Poll");
  ok &= contains(character_source_test,
                 "source_character_added_object(state,true,true,\"main.drv\")",
                 "focused Character test covers main driver");
  ok &= contains(character_source_test,
                 "source_character_sync_objects(state,true,3)",
                 "focused Character test covers SyncObjects");
  ok &= contains(character_source_test,
                 "source_character_set_sphere_base(state,false)",
                 "focused Character test covers SetSphereBase");
  ok &= contains(character_source_test,
                 "source_character_set_interest_objects(true,{true,false,true},"
                 "false)",
                 "focused Character test covers SetInterestObjects");
  ok &= contains(character_source_test,
                 "source_character_add_shadow_bone(2,true,false)",
                 "focused Character test covers AddShadowBone");
  ok &= contains(character_source_test,
                 "source_character_sync_shadow(true,true,{2,0,3})",
                 "focused Character test covers SyncShadow");
  ok &= contains(character_source_test,
                 "source_character_copy_bounding_sphere(state,false)",
                 "focused Character test covers CopyBoundingSphere");
  ok &= contains(character_source_test,
                 "source_character_repoint_sphere_base(state,true)",
                 "focused Character test covers RepointSphereBase");
  ok &= contains(character_source_test,
                 "source_character_pre_save().unhooked_shadow",
                 "focused Character test covers PreSave");
  ok &= contains(doc, "## Character Runtime Flow",
                 "document records Character runtime flow section");
  ok &= contains(doc,
                 "Native `source_character_*` helpers port these "
                 "source-visible runtime flows",
                 "document records native Character helpers");
  ok &= contains(char_mesh_h, "structSourceCharacterTestState{",
                 "native exposes CharacterTest default state");
  ok &= contains(char_mesh_h, "structSourceCharacterTestAddDefaultsResult{",
                 "native exposes CharacterTest AddDefaults result");
  ok &= contains(char_mesh,
                 "SourceCharacterTestStatesource_character_test_default_state(){"
                 "returnSourceCharacterTestState{};}",
                 "native ports CharacterTest constructor defaults");
  ok &= contains(char_mesh,
                 "SourceCharacterTestDestroyResultsource_character_test_destroy("
                 "booloverlay_found,booloverlay_callback_is_this){",
                 "native exposes CharacterTest destroy helper");
  ok &= contains(char_mesh,
                 "result.highlighted_driver=has_driver&&(has_clip1||has_clip2);"
                 "result.draw_transform=has_bone_head?\"bone_head\":\"self\";",
                 "native ports CharacterTest Draw decisions");
  ok &= contains(char_mesh,
                 "constboolclip_branch=input.has_driver&&input.has_clip_dir&&"
                 "input.has_clip1;",
                 "native ports CharacterTest Poll branch gate");
  ok &= contains(char_mesh,
                 "input.first_clip_is_clip2&&input.transition_beat<input."
                 "first_driver_beat;",
                 "native ports CharacterTest Poll clip2 transition gate");
  ok &= contains(char_mesh,
                 "result.reset_bone_servo_regulate=input.has_bone_servo;"
                 "result.recenter=true;",
                 "native ports CharacterTest zero-travel branch");
  ok &= contains(char_mesh,
                 "SourceCharacterTestAddDefaultsResultsource_character_test_"
                 "add_defaults(constSourceCharacterTestExisting&existing,"
                 "constSourceCharacterTestBones&bones){",
                 "native exposes CharacterTest AddDefaults helper");
  ok &= contains(char_mesh,
                 "setup.name=\"foreTwist_L.ik\";setup.hand=\"bone_L-hand\";"
                 "setup.twist2=\"bone_L-foreTwist2\";setup.has_offset=true;"
                 "setup.offset=90.0f;",
                 "native ports CharacterTest left foretwist setup");
  ok &= contains(char_mesh,
                 "setup.name=\"foreTwist_R.ik\";setup.hand=\"bone_R-hand\";"
                 "setup.twist2=\"bone_R-foreTwist2\";setup.has_offset=true;"
                 "setup.offset=-90.0f;",
                 "native ports CharacterTest right foretwist setup");
  ok &= contains(char_mesh,
                 "setup.name=\"upperTwist_L.ik\";setup.twist1="
                 "\"bone_L-upperTwist1\";setup.twist2=\"bone_L-upperTwist2\";"
                 "setup.upper_arm=\"bone_L-upperArm\";",
                 "native ports CharacterTest left uppertwist setup");
  ok &= contains(char_mesh,
                 "result.start_frame=(start_beat*30.0f)/beats_per_second;"
                 "result.end_frame=(end_beat*30.0f)/beats_per_second;",
                 "native ports CharacterTest SetStartEndBeat frames");
  ok &= contains(char_mesh,
                 "boolsource_character_test_set_move_self(boolhas_bone_servo){"
                 "returnhas_bone_servo;}",
                 "native ports CharacterTest SetMoveSelf gate");
  ok &= contains(char_mesh,
                 "result.loaded_driver=revision!=0xD;",
                 "native ports CharacterTest Load driver gate");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_character_test_source_test",
                 "CMake builds CharacterTest source test");
  ok &= contains(character_test_source_test,
                 "source_character_test_add_defaults(existing,bones)",
                 "focused CharacterTest test covers AddDefaults");
  ok &= contains(character_test_source_test,
                 "\"defaultsleftforetwistoffset\"",
                 "focused CharacterTest test covers left foretwist offset");
  ok &= contains(character_test_source_test,
                 "\"defaultsrightforetwistoffset\"",
                 "focused CharacterTest test covers right foretwist offset");
  ok &= contains(character_test_source_test,
                 "source_character_test_set_start_end_beat(true,true,true,"
                 "4.0f,8.0f,120)",
                 "focused CharacterTest test covers SetStartEndBeat");
  ok &= contains(character_test_source_test,
                 "source_character_test_load(0xD,0)",
                 "focused CharacterTest test covers Load revision gate");
  ok &= contains(doc, "## Character Test Harness Helper",
                 "document records CharacterTest helper boundary");
  ok &= contains(doc,
                 "The checked source still lacks bodies for `PlayNew`,",
                 "document fences missing CharacterTest bodies");
  ok &= contains(rb3_latest_character_h,
                 "enumDrawMode{kCharDrawNone,kCharDrawOpaque,"
                 "kCharDrawTranslucent,kCharDrawAll};",
                 "Character source draw-mode enum order");
  ok &= contains(rb3_latest_char_trans_draw_h,
                 "classCharTransDraw:publicRndDrawable",
                 "latest CharTransDraw header exposes source class");
  ok &= contains(rb3_latest_char_trans_draw_cpp,
                 "CharTransDraw::~CharTransDraw(){SetDrawModes("
                 "Character::kCharDrawAll);}",
                 "CharTransDraw source destructor restores draw all");
  ok &= contains(rb3_latest_char_trans_draw_cpp,
                 "voidCharTransDraw::SetDrawModes(Character::DrawModemode){"
                 "for(ObjPtrList<Character,ObjectDir>::iteratorit="
                 "mChars.begin();it!=mChars.end();++it){(*it)->"
                 "SetDrawMode(mode);}}",
                 "CharTransDraw source SetDrawModes order");
  ok &= contains(rb3_latest_char_trans_draw_cpp,
                 "voidCharTransDraw::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(1,0);Hmx::Object::Load(bs);"
                 "RndDrawable::Load(bs);bs>>mChars;"
                 "SetDrawModes(Character::kCharDrawOpaque);}",
                 "CharTransDraw source Load sets opaque");
  ok &= contains(rb3_latest_char_trans_draw_cpp,
                 "voidCharTransDraw::DrawShowing(){for(ObjPtrList<Character,"
                 "ObjectDir>::iteratorit=mChars.begin();it!=mChars.end();"
                 "++it){Character*theChar=*it;if(theChar->Showing()){"
                 "theChar->SetDrawMode(Character::kCharDrawTranslucent);"
                 "theChar->Draw();theChar->SetDrawMode("
                 "Character::kCharDrawOpaque);}}}",
                 "CharTransDraw source DrawShowing sequence");
  ok &= contains(char_mesh_h,
                 "enumclassSourceCharacterDrawMode:int32_t{kNone=0,"
                 "kOpaque=1,kTranslucent=2,kAll=3,};",
                 "native exposes source character draw-mode enum order");
  ok &= contains(char_mesh,
                 "std::vector<SourceCharTransDrawStep>"
                 "source_char_trans_draw_set_draw_modes("
                 "conststd::vector<std::string>&chars,"
                 "SourceCharacterDrawModemode)",
                 "native ports CharTransDraw SetDrawModes helper");
  ok &= contains(char_mesh,
                 "returnsource_char_trans_draw_set_draw_modes(chars,"
                 "SourceCharacterDrawMode::kOpaque);",
                 "native ports CharTransDraw Load opaque mode");
  ok &= contains(char_mesh,
                 "returnsource_char_trans_draw_set_draw_modes(chars,"
                 "SourceCharacterDrawMode::kAll);",
                 "native ports CharTransDraw destructor all mode");
  ok &= contains(char_mesh,
                 "if(!character.showing)continue;steps.push_back({"
                 "character.name,SourceCharacterDrawMode::kTranslucent,false});"
                 "steps.push_back({character.name,SourceCharacterDrawMode::"
                 "kTranslucent,true});steps.push_back({character.name,"
                 "SourceCharacterDrawMode::kOpaque,false});",
                 "native ports CharTransDraw DrawShowing sequence");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_trans_draw_source_test",
                 "CMake builds CharTransDraw source test");
  ok &= contains(trans_draw_source_test,
                 "source_char_trans_draw_load_modes(chars)",
                 "focused CharTransDraw test covers Load opaque");
  ok &= contains(trans_draw_source_test,
                 "source_char_trans_draw_destruct_modes(chars)",
                 "focused CharTransDraw test covers destructor all");
  ok &= contains(trans_draw_source_test,
                 "source_char_trans_draw_draw_showing(draw_chars)",
                 "focused CharTransDraw test covers DrawShowing");
  ok &= contains(doc,
                 "Native `source_char_trans_draw_*` helpers port",
                 "document records native CharTransDraw helpers");
  ok &= contains(rb3_latest_char_cuff_h,
                 "classCharCuff:publicRndTransformable",
                 "latest CharCuff header exposes source class");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "CharCuff::CharCuff():mOpenEnd(0),mIgnore(this,"
                 "kObjListNoNull),mBone(this,0),mEccentricity(1.0f),",
                 "CharCuff source constructor defaults prefix");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "mShape[0].offset=-2.9f;mShape[0].radius=1.9f;"
                 "mShape[1].offset=0.0f;mShape[1].radius=2.6f;"
                 "mShape[2].offset=2.0f;mShape[2].radius=3.5f;"
                 "mOuterRadius=mShape[1].radius+0.5f;",
                 "CharCuff source constructor shape defaults");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "floatCharCuff::Eccentricity(constVector2&v)const{"
                 "floatf1=v.y*v.y;floatf2=v.x*v.x;returnstd::sqrt((f1+f2)/"
                 "(f1*(1.0f/(mEccentricity*mEccentricity))+f2));}",
                 "CharCuff source eccentricity formula");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "ASSERT_REVS(8,0)LOAD_SUPERCLASS(Hmx::Object)"
                 "LOAD_SUPERCLASS(RndTransformable)for(inti=0;i<3;i++){"
                 "bs>>mShape[i].radius>>mShape[i].offset;}",
                 "CharCuff source load prefix and shape order");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "if(gRev>1)bs>>mOuterRadius;elsemOuterRadius=mShape[1]."
                 "radius+0.5f;if(gRev>2)bs>>mOpenEnd;elsemOpenEnd=false;"
                 "if(gRev>3)bs>>mBone;elsemBone=TransParent();",
                 "CharCuff source load early revision defaults");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "if(gRev>4)bs>>mEccentricity;elsemEccentricity=1.0f;"
                 "if(gRev>5)bs>>mCategory;elsemCategory=Symbol(\"\");"
                 "if(gRev>7)bs>>mIgnore;",
                 "CharCuff source load late revision gates");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "if(gRev<7)MILO_WARN(\"%soldCharCuff,mustconvert,seeJames\","
                 "PathName(this));",
                 "CharCuff source load warns on old rows");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS("
                 "RndTransformable)CREATE_COPY(CharCuff)",
                 "CharCuff source copy superclasses");
  ok &= contains(rb3_latest_char_cuff_cpp,
                 "memcpy(mShape,c->mShape,0x18);COPY_MEMBER(mOuterRadius)"
                 "COPY_MEMBER(mOpenEnd)COPY_MEMBER(mBone)COPY_MEMBER("
                 "mEccentricity)COPY_MEMBER(mCategory)COPY_MEMBER(mIgnore)",
                 "CharCuff source copy member list");
  ok &= contains(char_mesh_h,
                 "structSourceCharCuffState{SourceCharCuffShapeshape[3];"
                 "floatouter_radius=0.0f;boolopen_end=false;std::stringbone;",
                 "native exposes CharCuff source state");
  ok &= contains(char_mesh_h,
                 "structSourceCharCuffLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;",
                 "native exposes CharCuff source load plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharCuffCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>"
                 "copied_members;};",
                 "native exposes CharCuff source copy plan");
  ok &= contains(char_mesh_h,
                 "SourceCharCuffLoadPlansource_char_cuff_load_plan("
                 "intrevision);",
                 "native exposes CharCuff load-plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharCuffCopyPlansource_char_cuff_copy_plan();",
                 "native exposes CharCuff copy-plan helper");
  ok &= contains(char_mesh,
                 "SourceCharCuffStatesource_char_cuff_default_state(){",
                 "native ports CharCuff default helper");
  ok &= contains(char_mesh,
                 "cuff.shape[0].offset=-2.9f;cuff.shape[0].radius=1.9f;"
                 "cuff.shape[1].offset=0.0f;cuff.shape[1].radius=2.6f;"
                 "cuff.shape[2].offset=2.0f;cuff.shape[2].radius=3.5f;",
                 "native CharCuff helper sets source shape defaults");
  ok &= contains(char_mesh,
                 "returnstd::sqrt((f1+f2)/(f1*(1.0f/(eccentricity*"
                 "eccentricity))+f2));",
                 "native CharCuff helper ports eccentricity formula");
  ok &= contains(char_mesh,
                 "if(revision<=1)cuff.outer_radius=cuff.shape[1].radius+0.5f;"
                 "if(revision<=2)cuff.open_end=false;if(revision<=3)"
                 "cuff.bone=trans_parent;if(revision<=4)cuff.eccentricity=1.0f;"
                 "if(revision<=5)cuff.category.clear();if(revision<=7)"
                 "cuff.ignore.clear();",
                 "native CharCuff helper ports revision defaults");
  ok &= contains(char_mesh,
                 "SourceCharCuffLoadPlansource_char_cuff_load_plan("
                 "intrevision){SourceCharCuffLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=8;",
                 "native CharCuff load plan records source revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order={\"LOAD_REVS\",\"Hmx::Object\","
                 "\"RndTransformable\",\"mShape[0].radius\","
                 "\"mShape[0].offset\",",
                 "native CharCuff load plan records source prefix");
  ok &= contains(char_mesh,
                 "if(revision>1){plan.read_order.push_back(\"mOuterRadius\");}"
                 "else{plan.branches.push_back("
                 "\"mOuterRadius=mShape[1].radius+0.5\");}",
                 "native CharCuff load plan records outer-radius gate");
  ok &= contains(char_mesh,
                 "if(revision>3){plan.read_order.push_back(\"mBone\");}"
                 "else{plan.branches.push_back(\"mBone=TransParent\");}",
                 "native CharCuff load plan records bone parent gate");
  ok &= contains(char_mesh,
                 "if(revision>7){plan.read_order.push_back(\"mIgnore\");}"
                 "plan.warns_old_revision=revision<7;",
                 "native CharCuff load plan records ignore and warning gate");
  ok &= contains(char_mesh,
                 "SourceCharCuffCopyPlansource_char_cuff_copy_plan(){"
                 "SourceCharCuffCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\",\"RndTransformable\"};",
                 "native CharCuff copy plan records source superclasses");
  ok &= contains(char_mesh,
                 "plan.copied_members={\"mShape\",\"mOuterRadius\","
                 "\"mOpenEnd\",\"mBone\",\"mEccentricity\",\"mCategory\","
                 "\"mIgnore\"};",
                 "native CharCuff copy plan records source members");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_cuff_source_test",
                 "CMake builds CharCuff source test");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_default_state()",
                 "focused CharCuff test covers defaults");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_eccentricity(3.0f,4.0f,2.0f)",
                 "focused CharCuff test covers eccentricity formula");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_apply_revision_defaults(cuff,0,"
                 "\"parent.trans\")",
                 "focused CharCuff test covers old revision defaults");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_apply_revision_defaults(cuff,8,"
                 "\"parent.trans\")",
                 "focused CharCuff test covers rev8 preservation");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_load_plan(0)",
                 "focused CharCuff test covers legacy load plan");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_load_plan(8)",
                 "focused CharCuff test covers rev8 load plan");
  ok &= contains(cuff_source_test,
                 "source_char_cuff_copy_plan()",
                 "focused CharCuff test covers copy plan");
  ok &= contains(doc,
                 "Native `source_char_cuff_load_plan` records the source read order",
                 "document records native CharCuff load plan");
  ok &= contains(doc,
                 "Native `source_char_cuff_copy_plan` records that source copy",
                 "document records native CharCuff copy plan");
  ok &= contains(doc,
                 "Native `source_char_cuff_*` helpers port",
                 "document records native CharCuff helpers");
  ok &= contains(rb3_latest_char_blend_bone_h,
                 "classCharBlendBone:publicCharPollable",
                 "latest CharBlendBone header exposes source class");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "CharBlendBone::CharBlendBone():mTargets(this),mSrc1(this,0),"
                 "mSrc2(this,0),mTransX(0),mTransY(0),mTransZ(0),mRotation(0)",
                 "CharBlendBone source constructor defaults");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "CharBlendBone::ConstraintSystem::ConstraintSystem("
                 "Hmx::Object*o):mTarget(o,0),mWeight(0.5f){}",
                 "CharBlendBone source constraint default weight");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "BinStream&operator>>(BinStream&bs,CharBlendBone::"
                 "ConstraintSystem&cs){bs>>cs.mTarget;bs>>cs.mWeight;"
                 "returnbs;}",
                 "CharBlendBone source constraint load order");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "bs>>mTargets;bs>>mSrc1;bs>>mSrc2;bs>>mTransX;bs>>mTransY;"
                 "bs>>mTransZ;bs>>mRotation;",
                 "CharBlendBone source Load field order");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)CREATE_COPY(CharBlendBone)"
                 "BEGIN_COPYING_MEMBERSCOPY_MEMBER(mTargets)COPY_MEMBER(mSrc1)"
                 "COPY_MEMBER(mSrc2)COPY_MEMBER(mTransX)COPY_MEMBER(mTransY)"
                 "COPY_MEMBER(mTransZ)COPY_MEMBER(mRotation)END_COPYING_MEMBERS",
                 "CharBlendBone source Copy field order");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "voidCharBlendBone::PollDeps(std::list<Hmx::Object*>&"
                 "changedBy,std::list<Hmx::Object*>&change){changedBy."
                 "push_back(mSrc1);changedBy.push_back(mSrc2);for(ObjList<"
                 "ConstraintSystem>::iteratorit=mTargets.begin();it!="
                 "mTargets.end();++it){change.push_back((*it).mTarget);}}",
                 "CharBlendBone source PollDeps direction");
  ok &= contains(rb3_latest_char_blend_bone_cpp,
                 "//fn_804A4D38-poll",
                 "CharBlendBone source lacks checked Poll body");
  ok &= contains(char_mesh_h,
                 "structSourceCharBlendBoneConstraint{std::stringtarget;"
                 "floatweight=0.5f;};",
                 "native exposes CharBlendBone constraint row");
  ok &= contains(char_mesh_h,
                 "structSourceCharBlendBoneState{std::vector<"
                 "SourceCharBlendBoneConstraint>targets;std::stringsrc1;"
                 "std::stringsrc2;booltrans_x=false;booltrans_y=false;"
                 "booltrans_z=false;boolrotation=false;};",
                 "native exposes CharBlendBone state defaults");
  ok &= contains(char_mesh_h,
                 "structSourceCharBlendBoneConstraintLoadPlan{std::vector<"
                 "std::string>read_order;};",
                 "native exposes CharBlendBone constraint load plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharBlendBoneLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;};",
                 "native exposes CharBlendBone load plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharBlendBoneCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;};",
                 "native exposes CharBlendBone copy plan");
  ok &= contains(char_mesh,
                 "SourceCharBlendBoneStatesource_char_blend_bone_default_state(){"
                 "returnSourceCharBlendBoneState{};}",
                 "native ports CharBlendBone defaults");
  ok &= contains(char_mesh,
                 "SourceCharBlendBoneConstraintLoadPlansource_char_blend_bone_"
                 "constraint_load_plan(){SourceCharBlendBoneConstraintLoadPlan"
                 "plan;plan.read_order={\"mTarget\",\"mWeight\"};returnplan;}",
                 "native ports CharBlendBone constraint load order");
  ok &= contains(char_mesh,
                 "SourceCharBlendBoneLoadPlansource_char_blend_bone_load_plan("
                 "intrevision){SourceCharBlendBoneLoadPlanplan;plan."
                 "known_revision=revision>=0&&revision<=3;",
                 "native ports CharBlendBone load revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order={\"LOAD_REVS\",\"Hmx::Object\",\"mTargets\","
                 "\"mSrc1\",\"mSrc2\",\"mTransX\",\"mTransY\",\"mTransZ\","
                 "\"mRotation\"};returnplan;}",
                 "native ports CharBlendBone load order");
  ok &= contains(char_mesh,
                 "SourceCharBlendBoneCopyPlansource_char_blend_bone_copy_plan()"
                 "{SourceCharBlendBoneCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\"};plan.copied_members={\"mTargets\",\"mSrc1\","
                 "\"mSrc2\",\"mTransX\",\"mTransY\",\"mTransZ\",\"mRotation\"};"
                 "returnplan;}",
                 "native ports CharBlendBone copy order");
  ok &= contains(char_mesh,
                 "deps.changed_by.push_back(blend.src1);deps.changed_by."
                 "push_back(blend.src2);for(constSourceCharBlendBoneConstraint&"
                 "target:blend.targets){deps.change.push_back(target.target);}",
                 "native ports CharBlendBone PollDeps");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_blend_bone_source_test",
                 "CMake builds CharBlendBone source test");
  ok &= contains(blend_bone_source_test,
                 "source_char_blend_bone_default_state()",
                 "focused CharBlendBone test covers defaults");
  ok &= contains(blend_bone_source_test,
                 "constSourceCharBlendBoneConstraintconstraint;",
                 "focused CharBlendBone test covers constraint default");
  ok &= contains(blend_bone_source_test,
                 "source_char_blend_bone_constraint_load_plan()",
                 "focused CharBlendBone test covers constraint load plan");
  ok &= contains(blend_bone_source_test,
                 "source_char_blend_bone_load_plan(3)",
                 "focused CharBlendBone test covers rev3 load plan");
  ok &= contains(blend_bone_source_test,
                 "source_char_blend_bone_copy_plan()",
                 "focused CharBlendBone test covers copy plan");
  ok &= contains(blend_bone_source_test,
                 "source_char_blend_bone_poll_deps(deps,blend)",
                 "focused CharBlendBone test covers PollDeps");
  ok &= contains(doc,
                 "Native `source_char_blend_bone_load_plan` and",
                 "document records native CharBlendBone load plan");
  ok &= contains(doc,
                 "`source_char_blend_bone_copy_plan` records",
                 "document records native CharBlendBone copy plan");
  ok &= contains(doc,
                 "Native `source_char_blend_bone_*` helpers port",
                 "document records native CharBlendBone helpers");
  ok &= contains(rb3_latest_char_sleeve_h,
                 "classCharSleeve:publicRndHighlightable,publicCharPollable",
                 "latest CharSleeve header exposes source class");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "CharSleeve::CharSleeve():mSleeve(this,0),mTopSleeve(this,0),"
                 "mPos(0.0f,0.0f,0.0f),mLastPos(0.0f,0.0f,0.0f),mLastDT(0.0f),"
                 "mInertia(0.5f),mGravity(1.0f),mRange(0.0f),mNegLength(0.0f),"
                 "mPosLength(0.0f),mStiffness(0.02f),mMe(this,0)",
                 "CharSleeve source constructor defaults");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "voidCharSleeve::SetName(constchar*cc,classObjectDir*dir){"
                 "Hmx::Object::SetName(cc,dir);mMe=dynamic_cast<classCharacter*>"
                 "(dir);}",
                 "CharSleeve source owner capture");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "voidCharSleeve::Poll(){if(mSleeve&&mSleeve->TransParent()){"
                 "floatdeltasecs=TheTaskMgr.DeltaSeconds();floatdvar12="
                 "deltasecs*60.0f;floatpowed=std::pow(1.0f-mStiffness,"
                 "dvar12*dvar12);",
                 "CharSleeve source Poll gate and delta");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "if(mMe&&mMe->Teleported()){mPos=mSleeve->WorldXfm().v;",
                 "CharSleeve source teleport reset gate");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "if(mLastDT>0.0f&&deltasecs>0.0f){Vector3vc0;Subtract("
                 "mPos,mLastPos,vc0);ScaleAddEq(vb4,vc0,(mInertia*deltasecs)/"
                 "mLastDT);}",
                 "CharSleeve source inertia branch");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "vb4.z+=mGravity*deltasecs*dvar12*-3.858268f;",
                 "CharSleeve source gravity branch");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "floatlen=Length(vcc);floatinterped=Interp(len,absed,"
                 "1.0f-powed);ClampEq(interped,absed-mNegLength,"
                 "absed+mPosLength);ScaleToMagnitude(vcc,len,vcc);",
                 "CharSleeve source length/interp block");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "if(mTopSleeve){floatdotcc=Dot(vcc,sleeveparent->WorldXfm().m.x);"
                 "ScaleAddEq(vcc,sleeveparent->WorldXfm().m.x,-dotcc);",
                 "CharSleeve source top sleeve branch");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "voidCharSleeve::PollDeps(std::list<Hmx::Object*>&changedBy,"
                 "std::list<Hmx::Object*>&change){if(mSleeve){changedBy."
                 "push_back(mSleeve->mParent);change.push_back(mSleeve);"
                 "change.push_back(mTopSleeve);}}",
                 "CharSleeve source PollDeps order");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "voidCharSleeve::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(0,0);Hmx::Object::Load(bs);bs>>mSleeve;"
                 "bs>>mTopSleeve;bs>>mInertia;bs>>mGravity;bs>>mStiffness;"
                 "bs>>mRange;bs>>mNegLength;bs>>mPosLength;}",
                 "CharSleeve source Load order");
  ok &= contains(rb3_latest_char_sleeve_cpp,
                 "COPY_MEMBER(mSleeve)COPY_MEMBER(mTopSleeve)"
                 "COPY_MEMBER(mInertia)COPY_MEMBER(mGravity)"
                 "COPY_MEMBER(mStiffness)COPY_MEMBER(mRange)"
                 "COPY_MEMBER(mNegLength)COPY_MEMBER(mPosLength)",
                 "CharSleeve source Copy order");
  ok &= contains(char_mesh_h,
                 "structSourceCharSleeveState{std::array<float,3>pos="
                 "{0.0f,0.0f,0.0f};std::array<float,3>last_pos=",
                 "native exposes CharSleeve source state");
  ok &= contains(char_mesh_h,
                 "structSourceCharSleeveLoadPlan{boolrevision_supported=false;",
                 "native exposes CharSleeve load plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharSleeveCopyPlan{std::vector<std::string>"
                 "copied_superclasses;",
                 "native exposes CharSleeve copy plan");
  ok &= contains(char_mesh,
                 "SourceCharSleeveStatesource_char_sleeve_default_state(){"
                 "returnSourceCharSleeveState{};}",
                 "native ports CharSleeve defaults");
  ok &= contains(char_mesh,
                 "SourceCharSleevePollResultsource_char_sleeve_poll("
                 "SourceCharSleeveState&state,boolhas_sleeve,boolhas_parent,",
                 "native exposes CharSleeve poll helper");
  ok &= contains(char_mesh,
                 "constfloatpowed=std::pow(1.0f-state.stiffness,dvar12*dvar12);",
                 "native CharSleeve helper ports stiffness decay");
  ok &= contains(char_mesh,
                 "if(character_teleported){state.pos=source_xfm_pos(sleeve_world);",
                 "native CharSleeve helper ports teleport reset");
  ok &= contains(char_mesh,
                 "vb4[2]+=state.gravity*delta_seconds*dvar12*-3.858268f;",
                 "native CharSleeve helper ports gravity branch");
  ok &= contains(char_mesh,
                 "floatinterped=len+(absed-len)*(1.0f-powed);interped="
                 "std::clamp(interped,absed-state.neg_length,absed+"
                 "state.pos_length);(void)interped;vcc="
                 "source_vec_scale_to_magnitude(vcc,len);",
                 "native CharSleeve helper keeps source length/interp block");
  ok &= contains(char_mesh,
                 "if(has_top_sleeve){constfloatdotcc=source_vec_dot(vcc,"
                 "parent_x);SourceVec3top_delta=source_vec_add(vcc,"
                 "source_vec_scale(parent_x,-dotcc));",
                 "native CharSleeve helper ports top sleeve branch");
  ok &= contains(char_mesh,
                 "voidsource_char_sleeve_poll_deps(SourceCharSleevePollDeps&deps,"
                 "conststd::string&sleeve_parent,conststd::string&sleeve,"
                 "conststd::string&top_sleeve,boolhas_sleeve){if(!has_sleeve)"
                 "return;deps.changed_by.push_back(sleeve_parent);deps.change."
                 "push_back(sleeve);deps.change.push_back(top_sleeve);}",
                 "native CharSleeve helper ports PollDeps");
  ok &= contains(char_mesh,
                 "SourceCharSleeveLoadPlansource_char_sleeve_load_plan("
                 "int32_trevision){SourceCharSleeveLoadPlanplan;"
                 "plan.revision_supported=revision==0;",
                 "native CharSleeve helper ports load revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order={\"Hmx::Object\",\"mSleeve\","
                 "\"mTopSleeve\",\"mInertia\",\"mGravity\",\"mStiffness\","
                 "\"mRange\",\"mNegLength\",\"mPosLength\"};",
                 "native CharSleeve helper ports load order");
  ok &= contains(char_mesh,
                 "SourceCharSleeveCopyPlansource_char_sleeve_copy_plan(){"
                 "SourceCharSleeveCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\"};",
                 "native CharSleeve helper ports copy superclass");
  ok &= contains(char_mesh,
                 "plan.copied_members={\"mSleeve\",\"mTopSleeve\","
                 "\"mInertia\",\"mGravity\",\"mStiffness\",\"mRange\","
                 "\"mNegLength\",\"mPosLength\"};",
                 "native CharSleeve helper ports copy members");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_sleeve_source_test",
                 "CMake builds CharSleeve source test");
  ok &= contains(sleeve_source_test,
                 "source_char_sleeve_default_state()",
                 "focused CharSleeve test covers defaults");
  ok &= contains(sleeve_source_test,
                 "source_char_sleeve_poll(state,true,true,true,false,0.0f,"
                 "-2.0f,sleeve_world,parent)",
                 "focused CharSleeve test covers sleeve and top write");
  ok &= contains(sleeve_source_test,
                 "source_char_sleeve_poll(state,true,true,false,true,0.0f,"
                 "-2.0f,sleeve_world,parent)",
                 "focused CharSleeve test covers teleport reset");
  ok &= contains(sleeve_source_test,
                 "source_char_sleeve_poll_deps(deps,\"parent.trans\","
                 "\"sleeve.trans\",\"top.trans\",true)",
                 "focused CharSleeve test covers PollDeps");
  ok &= contains(sleeve_source_test,
                 "source_char_sleeve_load_plan(0)",
                 "focused CharSleeve test covers load plan");
  ok &= contains(sleeve_source_test,
                 "source_char_sleeve_copy_plan()",
                 "focused CharSleeve test covers copy plan");
  ok &= contains(doc,
                 "Native `source_char_sleeve_*` helpers port",
                 "document records native CharSleeve helpers");
  ok &= contains(rb3_latest_char_mesh_cache_h, "classMeshCacher{",
                 "latest CharMeshCacheMgr header exposes MeshCacher");
  ok &= contains(rb3_latest_char_mesh_cache_h,
                 "classCharMeshCacheMgr:publicSyncMeshCB",
                 "latest CharMeshCacheMgr header exposes manager class");
  ok &= contains(rb3_latest_char_mesh_cache_cpp,
                 "inlineMeshCacher::MeshCacher(RndMesh*mesh,boolb):"
                 "mMesh(mesh),unk4(0),mDisabled(b){",
                 "CharMeshCacheMgr source constructor defaults");
  ok &= contains(rb3_latest_char_mesh_cache_cpp,
                 "voidCharMeshCacheMgr::Disable(booldisable){MILO_ASSERT("
                 "mCache.empty(),0x178);mDisabled=disable;}",
                 "CharMeshCacheMgr source Disable assert");
  ok &= contains(rb3_latest_char_mesh_cache_cpp,
                 "boolCharMeshCacheMgr::HasMesh(RndMesh*mesh){for(inti=0;"
                 "i<mCache.size();i++){if(mesh==mCache[i]->mMesh)returntrue;}",
                 "CharMeshCacheMgr source HasMesh scan");
  ok &= contains(rb3_latest_char_mesh_cache_cpp,
                 "voidCharMeshCacheMgr::SyncMesh(RndMesh*mesh,intmask){intidx=0;"
                 "for(inti=0;i<mCache.size();i++){if(mCache[idx++]->mMesh=="
                 "mesh)break;}if(idx==mCache.size()){mCache.push_back(new"
                 "MeshCacher(mesh,mDisabled));}",
                 "CharMeshCacheMgr source SyncMesh scan and add");
  ok &= contains(rb3_latest_char_mesh_cache_cpp,
                 "voidCharMeshCacheMgr::StuffMeshes(ObjPtrList<RndMesh,"
                 "ObjectDir>&meshlist){for(inti=0;i<mCache.size();i++){"
                 "meshlist.push_back(mCache[i]->mMesh);}}",
                 "CharMeshCacheMgr source StuffMeshes order");
  ok &= contains(char_mesh_h, "structSourceCharMeshCacher{",
                 "native exposes CharMeshCache cacher state");
  ok &= contains(char_mesh_h, "structSourceCharMeshCacheState{",
                 "native exposes CharMeshCache state");
  ok &= contains(char_mesh,
                 "SourceCharMeshCacheStatesource_char_mesh_cache_default_"
                 "state(){returnSourceCharMeshCacheState{};}",
                 "native ports CharMeshCacheMgr defaults");
  ok &= contains(char_mesh,
                 "SourceCharMeshCacheDisableResultsource_char_mesh_cache_"
                 "disable(SourceCharMeshCacheState&state,booldisabled){",
                 "native exposes CharMeshCacheMgr Disable helper");
  ok &= contains(char_mesh,
                 "boolsource_char_mesh_cache_has_mesh(const"
                 "SourceCharMeshCacheState&state,conststd::string&mesh){",
                 "native exposes CharMeshCacheMgr HasMesh helper");
  ok &= contains(char_mesh,
                 "SourceCharMeshCacheSyncResultsource_char_mesh_cache_sync_"
                 "mesh(SourceCharMeshCacheState&state,conststd::string&mesh){",
                 "native exposes CharMeshCacheMgr SyncMesh helper");
  ok &= contains(char_mesh,
                 "if(state.cache[idx++].mesh==mesh)break;",
                 "native preserves CharMeshCacheMgr source scan index");
  ok &= contains(char_mesh,
                 "std::vector<std::string>source_char_mesh_cache_stuff_meshes("
                 "constSourceCharMeshCacheState&state){",
                 "native exposes CharMeshCacheMgr StuffMeshes helper");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_mesh_cache_source_test",
                 "CMake builds CharMeshCacheMgr source test");
  ok &= contains(mesh_cache_source_test,
                 "source_char_mesh_cache_sync_mesh(state,\"hair_front.mesh\");",
                 "focused CharMeshCacheMgr test covers SyncMesh");
  ok &= contains(mesh_cache_source_test,
                 "\"sourceloopappendswhenmatchislastentry\"",
                 "focused CharMeshCacheMgr test covers visible source loop");
  ok &= contains(mesh_cache_source_test,
                 "source_char_mesh_cache_disable(state,false);",
                 "focused CharMeshCacheMgr test covers Disable assert");
  ok &= contains(doc, "Character Mesh Cache Helper",
                 "document records CharMeshCacheMgr helper boundary");
  ok &= contains(rb3_latest_char_mesh_hide_h,
                 "classCharMeshHide:publicHmx::Object",
                 "latest CharMeshHide header exposes source class");
  ok &= contains(rb3_latest_char_mesh_hide_cpp,
                 "voidCharMeshHide::HideAll(constObjPtrList<CharMeshHide,"
                 "ObjectDir>&pList,inti){for(ObjPtrList<CharMeshHide,"
                 "ObjectDir>::iteratorit=pList.begin();it!=pList.end();++it){"
                 "i|=(*it)->mFlags;}",
                 "CharMeshHide source HideAll ORs owner flags");
  ok &= contains(rb3_latest_char_mesh_hide_cpp,
                 "voidCharMeshHide::HideDraws(intx){for(inti=0;i<mHides.size();"
                 "i++){Hide&theHide=mHides[i];if(theHide.mDraw){boolb=(x&"
                 "theHide.mFlags)==0;theHide.mShow=b&theHide.mDraw->Showing();",
                 "CharMeshHide source HideDraws gates drawable showing");
  ok &= contains(rb3_latest_char_mesh_hide_cpp,
                 "BinStream&operator>>(BinStream&bs,CharMeshHide::Hide&hide){"
                 "bs>>hide.mDraw;bs>>hide.mFlags;if(CharMeshHide::gRev>1)"
                 "bs>>hide.mShow;",
                 "CharMeshHide source Hide row load gates stored show");
  ok &= contains(rb3_latest_char_mesh_hide_cpp,
                 "voidCharMeshHide::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(2,0);Hmx::Object::Load(bs);bs>>mFlags>>mHides;}",
                 "CharMeshHide source Load reads flags and hides");
  ok &= contains(char_mesh_h,
                 "structSourceCharMeshHideRow{int32_tflags=0;"
                 "booldraw_showing=false;boolhas_draw=false;boolshow=false;};",
                 "native exposes CharMeshHide row helper state");
  ok &= contains(char_mesh_h,
                 "structSourceCharMeshHideObject{int32_tflags=0;"
                 "std::vector<SourceCharMeshHideRow>hides;};",
                 "native exposes CharMeshHide owner helper state");
  ok &= contains(char_mesh,
                 "int32_tsource_char_mesh_hide_combined_flags("
                 "conststd::vector<SourceCharMeshHideObject>&objects,",
                 "native implements CharMeshHide combined flag helper");
  ok &= contains(char_mesh,
                 "flags|=object.flags;",
                 "native CharMeshHide helper ORs owner flags");
  ok &= contains(char_mesh,
                 "voidsource_char_mesh_hide_draws(SourceCharMeshHideObject&object,"
                 "int32_tflags){for(SourceCharMeshHideRow&hide:object.hides){"
                 "if(hide.has_draw){constbooldraw_allowed=(flags&hide.flags)==0;"
                 "hide.show=draw_allowed&hide.draw_showing;",
                 "native CharMeshHide helper ports HideDraws show rule");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_mesh_hide_source_test",
                 "CMake builds CharMeshHide source test");
  ok &= contains(mesh_hide_source_test,
                 "source_char_mesh_hide_draws(single,0x1)",
                 "focused CharMeshHide test covers HideDraws");
  ok &= contains(mesh_hide_source_test,
                 "source_char_mesh_hide_all(objects,0x4)",
                 "focused CharMeshHide test covers HideAll initial flags");
  ok &= contains(mesh_hide_source_test,
                 "HideAllpreservesrowwithnodraw",
                 "focused CharMeshHide test covers no-draw row preservation");
  ok &= contains(doc,
                 "Native `source_char_mesh_hide_all` / "
                 "`source_char_mesh_hide_draws` ports",
                 "document records native CharMeshHide helper");
  ok &= contains(rb3_latest_char_face_servo_h,
                 "classCharFaceServo:publicCharPollable,publicCharBonesMeshes",
                 "latest CharFaceServo header exposes source inheritance");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "BEGIN_LOADS(CharFaceServo)LOAD_REVS(bs)ASSERT_REVS(4,0)"
                 "LOAD_SUPERCLASS(Hmx::Object)",
                 "latest CharFaceServo source load entry");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "ObjPtr<ObjectDir,ObjectDir>oDirPtr(this,0);bs>>oDirPtr;",
                 "CharFaceServo source reads clip-set ObjectDir pointer");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "if(gRev>3)bs>>sym;",
                 "CharFaceServo source gates clip type symbol");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "if(gRev!=0)bs>>mBlinkClipLeftName;if(gRev>1)"
                 "bs>>mBlinkClipRightName;if(gRev>2){"
                 "bs>>mBlinkClipLeftName2;bs>>mBlinkClipRightName2;}",
                 "CharFaceServo source reads blink clip names by revision");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "if(mBaseClip){TryScaleDown();ScaleAddIdentity();"
                 "mBaseClip->RotateBy(*this,mBaseClip->StartBeat());"
                 "PoseMeshes();}",
                 "CharFaceServo source poll applies base clip and poses meshes");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "voidCharFaceServo::ScaleAdd(CharClip*clip,floatweight,"
                 "floatf2,floatf3){if(!clip->mRelative){",
                 "CharFaceServo source exposes ScaleAdd relative gate");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "MILO_ASSERT(weight>=0,0x88);TryScaleDown();if(clip=="
                 "mBlinkClipLeft||clip==mBlinkClipLeft2){mBlinkWeightLeft+="
                 "weight;mBlinkWeightLeft=Clamp(0.0f,1.0f,mBlinkWeightLeft);}",
                 "CharFaceServo source exposes left blink accumulation");
  ok &= contains(rb3_latest_char_face_servo_cpp,
                 "elseif(clip==mBlinkClipRight||clip==mBlinkClipRight2){"
                 "mBlinkWeightRight+=weight;mBlinkWeightRight=Clamp(0.0f,"
                 "1.0f,mBlinkWeightRight);}",
                 "CharFaceServo source exposes right blink accumulation");
  ok &= contains(char_mesh_h,
                 "structSourceCharFaceServoBlinkClips{std::stringleft;"
                 "std::stringleft2;std::stringright;std::stringright2;};",
                 "native exposes source CharFaceServo blink clip names");
  ok &= contains(char_mesh_h,
                 "structSourceCharFaceServoBlinkState{floatleft=0.0f;"
                 "floatright=0.0f;boolneed_scale_down=false;};",
                 "native exposes source CharFaceServo blink state");
  ok &= contains(char_mesh_h,
                 "SourceCharFaceServoScaleAddResult"
                 "source_char_face_servo_scale_add_blink(",
                 "native exposes CharFaceServo ScaleAdd blink helper");
  ok &= contains(char_mesh,
                 "SourceCharFaceServoScaleAddResult"
                 "source_char_face_servo_scale_add_blink("
                 "SourceCharFaceServoBlinkState&state,",
                 "native implements CharFaceServo ScaleAdd blink helper");
  ok &= contains(char_mesh,
                 "if(!clip_is_relative||weight<0.0f)returnresult;",
                 "native CharFaceServo helper keeps source relative/assert boundary");
  ok &= contains(char_mesh,
                 "if(state.need_scale_down){state.left=0.0f;state.right=0.0f;"
                 "state.need_scale_down=false;result.scale_down=true;}",
                 "native CharFaceServo helper ports TryScaleDown blink reset");
  ok &= contains(char_mesh,
                 "if(left_match){state.left=std::clamp(state.left+weight,"
                 "0.0f,1.0f);result.matched_left=true;}elseif(right_match){"
                 "state.right=std::clamp(state.right+weight,0.0f,1.0f);",
                 "native CharFaceServo helper ports blink clamp branches");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_face_servo_source_test",
                 "CMake builds CharFaceServo source test");
  ok &= contains(face_servo_source_test,
                 "source_char_face_servo_scale_add_blink(state,clips,"
                 "\"blink_L2\",true,0.25f)",
                 "focused CharFaceServo test covers left2 blink branch");
  ok &= contains(face_servo_source_test,
                 "source_char_face_servo_scale_add_blink(state,clips,"
                 "\"blink_R2\",true,0.5f)",
                 "focused CharFaceServo test covers right2 clamp branch");
  ok &= contains(face_servo_source_test,
                 "SourceCharFaceServoBlinkClipsoverlapping{\"same\",\"\","
                 "\"same\",\"\"};",
                 "focused CharFaceServo test covers left-first overlap branch");
  ok &= contains(doc,
                 "Native `source_char_face_servo_scale_add_blink` ports the bounded",
                 "document records native CharFaceServo blink helper");
  ok &= contains(rb3_latest_char_lip_sync_cpp,
                 "CharLipSync::Generator::Generator():mLipSync(0),"
                 "mLastCount(0),mWeights()",
                 "CharLipSync source Generator constructor exposes defaults");
  ok &= contains(rb3_latest_char_lip_sync_cpp,
                 "CharLipSync::CharLipSync():mPropAnim(this,0),mFrames(0)",
                 "CharLipSync source constructor exposes defaults");
  ok &= contains(rb3_latest_char_lip_sync_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(1,0)LOAD_SUPERCLASS(Hmx::Object)"
                 "bs>>mVisemes;bs>>mFrames;bs>>mData;if(gRev!=0)"
                 "bs>>mPropAnim;",
                 "CharLipSync source load order and prop anim gate");
  ok &= contains(rb3_latest_char_lip_sync_h,
                 "ObjPtr<RndPropAnim,ObjectDir>mPropAnim;",
                 "CharLipSync source header exposes prop anim ref");
  ok &= contains(rb3_latest_char_lip_sync_h,
                 "std::vector<String>mVisemes;",
                 "CharLipSync source header exposes viseme rows");
  ok &= contains(rb3_latest_char_lip_sync_h, "intmFrames;",
                 "CharLipSync source header exposes frame count");
  ok &= contains(rb3_latest_char_lip_sync_h,
                 "std::vector<unsignedchar,unsignedint>mData;",
                 "CharLipSync source header exposes raw data rows");
  ok &= contains(rb3_latest_char_lip_sync_driver_h,
                 "classCharLipSyncDriver:publicRndHighlightable,"
                 "publicCharWeightable,publicCharPollable",
                 "CharLipSyncDriver source header exposes inheritance");
  ok &= contains(rb3_latest_char_lip_sync_driver_h,
                 "ObjPtr<CharLipSync,ObjectDir>mLipSync;",
                 "CharLipSyncDriver source header exposes lip sync ref");
  ok &= contains(rb3_latest_char_lip_sync_driver_h,
                 "ObjPtr<CharBonesObject,ObjectDir>mBones;",
                 "CharLipSyncDriver source header exposes bones ref");
  ok &= contains(rb3_latest_char_lip_sync_driver_cpp,
                 "CharLipSyncDriver::CharLipSyncDriver():mLipSync(this,0),"
                 "mClips(this,0),mBlinkClip(this,0),mSongOwner(this,0),"
                 "mSongOffset(0.0f),mLoop(0),mSongPlayer(0),"
                 "mBones(this,0),mTestClip(this,0),mTestWeight(1.0f),"
                 "mOverrideClip(this,0),mOverrideWeight(0.0f),",
                 "CharLipSyncDriver source constructor exposes first defaults");
  ok &= contains(rb3_latest_char_lip_sync_driver_cpp,
                 "mOverrideOptions(this,0),mApplyOverrideAdditively(0),"
                 "mAlternateDriver(this,0)",
                 "CharLipSyncDriver source constructor exposes override defaults");
  ok &= contains(rb3_latest_char_lip_sync_driver_cpp,
                 "voidCharLipSyncDriver::PollDeps(std::list<Hmx::Object*>&"
                 "changedBy,std::list<Hmx::Object*>&change){"
                 "change.push_back(mBones);}",
                 "CharLipSyncDriver source PollDeps changes bones only");
  ok &= missing(rb3_latest_char_lip_sync_driver_cpp,
                "voidCharLipSyncDriver::Poll(",
                "available CharLipSyncDriver source lacks Poll body");
  ok &= missing(rb3_latest_char_lip_sync_driver_cpp,
                "BEGIN_LOADS(CharLipSyncDriver)",
                "available CharLipSyncDriver source lacks Load body");
  ok &= contains(char_clip_h,
                 "structSourceCharLipSyncGeneratorState{boollip_sync_null=true;"
                 "intlast_count=0;",
                 "native exposes CharLipSync Generator state");
  ok &= contains(char_clip_h,
                 "structSourceCharLipSyncState{std::stringprop_anim;"
                 "std::vector<std::string>visemes;intframes=0;",
                 "native exposes CharLipSync state");
  ok &= contains(char_clip_h,
                 "structSourceCharLipSyncDriverState{SourceCharWeightableState"
                 "weightable;",
                 "native exposes CharLipSyncDriver state");
  ok &= contains(char_clip_h,
                 "SourceCharLipSyncLoadStepssource_char_lip_sync_load_steps("
                 "int32_trevision);",
                 "native API exposes CharLipSync load helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_lip_sync_driver_poll_deps(",
                 "native API exposes CharLipSyncDriver PollDeps helper");
  ok &= contains(char_clip,
                 "SourceCharLipSyncGeneratorStatesource_char_lip_sync_generator"
                 "_default_state(){returnSourceCharLipSyncGeneratorState{};}",
                 "native CharLipSync Generator defaults helper mirrors source");
  ok &= contains(char_clip,
                 "SourceCharLipSyncStatesource_char_lip_sync_default_state(){"
                 "returnSourceCharLipSyncState{};}",
                 "native CharLipSync defaults helper mirrors source");
  ok &= contains(char_clip,
                 "steps.known_revision=revision>=0&&revision<=steps.max_revision;"
                 "steps.load_hmx_object=true;steps.load_visemes=true;",
                 "native CharLipSync load helper mirrors revision gate");
  ok &= contains(char_clip,
                 "steps.load_data=true;steps.load_prop_anim=revision!=0;",
                 "native CharLipSync load helper mirrors prop anim gate");
  ok &= contains(char_clip,
                 "state.weightable=source_char_weightable_default_state(name);"
                 "returnstate;",
                 "native CharLipSyncDriver defaults helper mirrors weightable base");
  ok &= contains(char_clip,
                 "deps.change.push_back(state.bones);",
                 "native CharLipSyncDriver PollDeps helper mirrors source");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_lip_sync_source_test"
                 "character_lip_sync_source_test.cpp)",
                 "CMake builds focused CharLipSync source test");
  ok &= contains(lip_sync_source_test,
                 "source_char_lip_sync_generator_default_state()",
                 "focused CharLipSync test covers Generator defaults");
  ok &= contains(lip_sync_source_test,
                 "source_char_lip_sync_load_steps(1)",
                 "focused CharLipSync test covers load prop anim gate");
  ok &= contains(lip_sync_source_test,
                 "source_char_lip_sync_driver_default_state(\"lips.driver\")",
                 "focused CharLipSync test covers driver defaults");
  ok &= contains(lip_sync_source_test,
                 "source_char_lip_sync_driver_poll_deps(deps,driver)",
                 "focused CharLipSync test covers driver PollDeps");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharLipSync.cpp`",
                 "document cites CharLipSync source");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharLipSyncDriver.cpp`",
                 "document cites CharLipSyncDriver source");
  ok &= contains(doc,
                 "Native `source_char_lip_sync_*` helpers therefore port",
                 "document records native CharLipSync helper boundary");
  ok &= contains(doc,
                 "do\n    not promote any live GH2 mouth or viseme controller "
                 "behavior",
                 "document fences CharLipSync from GH2 mouth runtime behavior");
  ok &= missing(rb3_latest_char_face_servo_cpp, "FaceFxLipSyncServo",
                "CharFaceServo source is not a FaceFxLipSyncServo load body");
  ok &= contains(char_mesh,
                 "GH2PS2FaceFxLipSyncServocompatibility,notaCharFaceServo"
                 "sourceport",
                 "native FaceFxLipSyncServo decoder is labeled compatibility");
  ok &= contains(char_mesh,
                 "FaceFxLipSyncServo::Loadbody.Keepthislimitedtothestock"
                 "FAC/viseme",
                 "native FaceFxLipSyncServo decoder states source boundary");
  ok &= contains(bind_audit, "object_type_counts",
                 "bind audit has stock object-type inventory support");
  ok &= contains(bind_audit, "--types",
                 "bind audit exposes stock object-type inventory switch");
  ok &= contains(bind_audit,
                 "\"[controller-servo-bone]char=%sname=%sversion=%dclipType=%s"
                 "\"",
                 "controller audit logs CharServoBone source fields");
  ok &= contains(bind_audit, "unreadBytes=%zu",
                 "controller audit logs CharServoBone source tail");
  ok &= contains(doc, "`CharServoBone::Load` accepts source revisions through 2",
                 "document records CharServoBone source load");
  ok &= contains(doc, "revision is greater than 1",
                 "document records CharServoBone clip_type revision gate");
  ok &= contains(doc,
                 "enforces the source revision range, and records the row tail",
                 "document records CharServoBone tail-byte proof");
  ok &= contains(doc,
                 "source_charservobone_20260711/stock_charservobone_controllers"
                 ".stdout.log",
                 "document cites refreshed CharServoBone stock proof");
  ok &= contains(doc,
                 "all 24 stock rows are\n  `version=1`, have no `clipType`, "
                 "and report `unreadBytes=0`",
                 "document records refreshed CharServoBone zero-tail proof");
  ok &= contains(doc,
                 "Native exposes bounded source helpers for `ZeroDeltas`,\n"
                 "    `MoveToFacing`, and `MoveToDeltaFacing`",
                 "document records bounded CharServoBone movement helpers");
  ok &= contains(doc,
                 "Native GHOGX also records the checked `CharServoBone` "
                 "constructor",
                 "document records CharServoBone constructor/control-flow slice");
  ok &= contains(doc,
                 "`SetMoveSelf` only\n    marks `delta_changed` when the "
                 "requested value differs",
                 "document records CharServoBone SetMoveSelf source branch");
  ok &= contains(doc,
                 "`Copy` copies\n    `Hmx::Object`, `mMoveSelf`, and calls "
                 "`SetClipType`",
                 "document records CharServoBone Copy source branch");
  ok &= contains(doc,
                 "does not call them from the\n    live model path or port "
                 "broad `CharBonesMeshes` movement",
                 "document fences live CharServoBone movement behavior");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_zero_deltas(facing_pos_delta,"
                 "facing_rot_delta);",
                 "focused CharBones source test covers CharServoBone ZeroDeltas");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_move_to_facing(facing_xfm,{10.0f,"
                 "20.0f,30.0f},kHalfPi);",
                 "focused CharBones source test covers CharServoBone MoveToFacing");
  ok &= contains(char_bones_source_test,
                 "source_char_servo_bone_move_to_delta_facing(delta_xfm,{4.0f,"
                 "5.0f,6.0f},kHalfPi);",
                 "focused CharBones source test covers CharServoBone MoveToDeltaFacing");
  ok &= contains(rb3_latest_char_weightable_h,
                 "floatWeight(){returnmWeightOwner->mWeight;}",
                 "latest CharWeightable source exposes owner-weight lookup");
  ok &= contains(rb3_latest_char_weightable_h,
                 "voidSetWeight(floatw){mWeight=w;}",
                 "latest CharWeightable source exposes SetWeight assignment");
  ok &= contains(rb3_latest_char_weightable_h,
                 "voidSetWeightOwner(CharWeightable*o){mWeightOwner=o?o:this;}",
                 "latest CharWeightable source exposes owner fallback");
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "CharWeightable::CharWeightable():mWeight(1.0f),"
                 "mWeightOwner(this,this)",
                 "latest CharWeightable source exposes constructor defaults");
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "voidCharWeightable::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(2,0);",
                 "CharWeightable source enforces revision ceiling");
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "bs>>mWeight;if(gRev>1)bs>>mWeightOwner;",
                 "CharWeightable source load gates weight owner");
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "if(mWeightOwner==o1){mWeightOwner=dynamic_cast<"
                 "CharWeightable*>(o2);}if(mWeightOwner==0){mWeightOwner=this;}",
                 "latest CharWeightable source Replace falls back to self");
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "if(ty==kCopyShallow){SetWeightOwner(c->mWeightOwner.Ptr());}"
                 "else{SetWeightOwner(this);mWeight=c->mWeightOwner->mWeight;}",
                 "latest CharWeightable source Copy handles shallow and deep owner");
  ok &= contains(doc,
                 "| Mirror servo controller | `rb3-latest` `CharMirror.cpp` / "
                 "`CharMirror.h` |",
                 "coverage matrix records CharMirror source");
  ok &= contains(rb3_latest_char_mirror_h,
                 "classCharMirror:publicCharWeightable,publicCharPollable",
                 "latest CharMirror header exposes inheritance");
  ok &= contains(rb3_latest_char_mirror_h,
                 "ObjPtr<CharServoBone,classObjectDir>mServo;",
                 "latest CharMirror header exposes servo pointer");
  ok &= contains(rb3_latest_char_mirror_h,
                 "ObjPtr<CharServoBone,classObjectDir>mMirrorServo;",
                 "latest CharMirror header exposes mirror servo pointer");
  ok &= contains(rb3_latest_char_mirror_h,
                 "CharBonesAllocmBones;",
                 "latest CharMirror header exposes mirror bones");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "CharMirror::CharMirror():mServo(this,0),mMirrorServo(this,0),"
                 "mBones(),mOps(){}",
                 "latest CharMirror source constructor defaults");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "voidCharMirror::Poll(){floatweight=Weight();if(weight&&"
                 "mBones.TotalSize()!=0){mBones.ScaleDown(*mServo.Ptr(),"
                 "1.0f-weight);}}",
                 "latest CharMirror source Poll gate");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "voidCharMirror::SetServo(CharServoBone*bone){if(bone!="
                 "mServo){mServo=bone;SyncBones();}}",
                 "latest CharMirror source SetServo sync gate");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "voidCharMirror::SetMirrorServo(CharServoBone*bone){"
                 "if(bone!=mMirrorServo){mMirrorServo=bone;SyncBones();}}",
                 "latest CharMirror source SetMirrorServo sync gate");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "voidCharMirror::PollDeps(std::list<Hmx::Object*>&changedBy,"
                 "std::list<Hmx::Object*>&change){change.push_back(mServo);}",
                 "latest CharMirror source PollDeps");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "voidCharMirror::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(1,0);Hmx::Object::Load(bs);"
                 "CharWeightable::Load(bs);bs>>mMirrorServo;bs>>mServo;"
                 "SyncBones();}",
                 "latest CharMirror source Load order");
  ok &= contains(rb3_latest_char_mirror_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS(CharWeightable)"
                 "CREATE_COPY(CharMirror)BEGIN_COPYING_MEMBERS"
                 "SetMirrorServo(c->mMirrorServo);SetServo(c->mServo);",
                 "latest CharMirror source Copy order");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "CharWeightSetter::CharWeightSetter():mBase(this,0),"
                 "mDriver(this,0),mMinWeights(this,kObjListNoNull),"
                 "mMaxWeights(this,kObjListNoNull),mFlags(0),mOffset(0.0f),"
                 "mScale(1.0f),mBaseWeight(0.0f),mBeatsPerWeight(0.0f)",
                 "latest CharWeightSetter source exposes constructor defaults");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "voidCharWeightSetter::SetWeight(floatweight){mBaseWeight=weight;"
                 "mWeight=weight;}",
                 "latest CharWeightSetter source SetWeight writes base and inherited weight");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "voidCharWeightSetter::Poll(){if(mDriver){mBaseWeight="
                 "mScale*mDriver->EvaluateFlags(mFlags)+mOffset;}",
                 "latest CharWeightSetter source poll uses driver flags");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "elseif(mBase){mBaseWeight=mScale*mBase->Weight()+mOffset;}",
                 "latest CharWeightSetter source poll uses base weight");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "MinEq(newminweight,(*it)->Weight());",
                 "latest CharWeightSetter source poll applies min weights");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "MaxEq(newmaxweight,(*it)->Weight());",
                 "latest CharWeightSetter source poll applies max weights");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "floatsecs=TheTaskMgr.DeltaBeat()/mBeatsPerWeight;",
                 "latest CharWeightSetter source poll beat-smooths");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "BEGIN_COPYS(CharWeightSetter)COPY_SUPERCLASS(Hmx::Object)"
                 "COPY_SUPERCLASS(CharWeightable)CREATE_COPY(CharWeightSetter)"
                 "BEGIN_COPYING_MEMBERSCOPY_MEMBER(mDriver)COPY_MEMBER(mFlags)"
                 "COPY_MEMBER(mBase)COPY_MEMBER(mOffset)COPY_MEMBER(mScale)"
                 "COPY_MEMBER(mBaseWeight)COPY_MEMBER(mBeatsPerWeight)"
                 "COPY_MEMBER(mMinWeights)COPY_MEMBER(mMaxWeights)",
                 "latest CharWeightSetter source Copy member list");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "voidCharWeightSetter::PollDeps(std::list<Hmx::Object*>&"
                 "changedBy,std::list<Hmx::Object*>&change){changedBy."
                 "push_back(mDriver);changedBy.push_back(mBase);",
                 "latest CharWeightSetter source PollDeps publishes driver and base");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "for(ObjPtrList<CharWeightSetter,ObjectDir>::iteratorit="
                 "mMinWeights.begin();it!=mMinWeights.end();++it){changedBy."
                 "push_back(*it);}",
                 "latest CharWeightSetter source PollDeps publishes min weights");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "for(ObjPtrList<CharWeightSetter,ObjectDir>::iteratorit="
                 "mMaxWeights.begin();it!=mMaxWeights.end();++it){changedBy."
                 "push_back(*it);}",
                 "latest CharWeightSetter source PollDeps publishes max weights");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "CharWeightable*weightowner=dynamic_cast<CharWeightable*>("
                 "(*it)->RefOwner());if(weightowner&&weightowner->"
                 "mWeightOwner==this)change.push_back(weightowner);",
                 "latest CharWeightSetter source PollDeps publishes owned ref owners");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightableState{std::stringname;"
                 "floatweight=1.0f;std::stringweight_owner;};",
                 "native exposes source CharWeightable state");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightableLoadPlan{boolrevision_supported=false;"
                 "std::vector<std::string>read_order;};",
                 "native exposes source CharWeightable load plan");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightableCopyPlan{std::vector<std::string>"
                 "shallow_actions;std::vector<std::string>deep_actions;};",
                 "native exposes source CharWeightable copy plan");
  ok &= contains(char_clip_h,
                 "SourceCharWeightableLoadPlansource_char_weightable_load_plan("
                 "int32_trevision);",
                 "native exposes source CharWeightable load helper");
  ok &= contains(char_clip_h,
                 "SourceCharWeightableCopyPlansource_char_weightable_copy_plan();",
                 "native exposes source CharWeightable copy plan helper");
  ok &= contains(char_clip_h,
                 "SourceCharWeightableStatesource_char_weightable_default_state("
                 "conststd::string&name);",
                 "native exposes source CharWeightable constructor helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_weightable_set_weight("
                 "SourceCharWeightableState&state,floatweight);",
                 "native exposes source CharWeightable SetWeight helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_weightable_set_weight_owner("
                 "SourceCharWeightableState&state,conststd::string&weight_owner);",
                 "native exposes source CharWeightable SetWeightOwner helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_weightable_replace("
                 "SourceCharWeightableState&state,conststd::string&old_owner,"
                 "conststd::string&new_owner,boolnew_owner_is_weightable);",
                 "native exposes source CharWeightable Replace helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_weightable_copy("
                 "SourceCharWeightableState&dest,constSourceCharWeightableState&"
                 "source,boolshallow_copy,floatsource_owner_weight);",
                 "native exposes source CharWeightable Copy helper");
  ok &= contains(char_clip_h,
                 "floatsource_char_weightable_weight(constCharWeightSetter&"
                 "setter,conststd::unordered_map<std::string,float>&"
                 "weights_by_name);",
                 "native exposes source CharWeightable owner lookup helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_weight_setter_poll(constCharWeightSetter&"
                 "setter,conststd::unordered_map<std::string,float>&"
                 "weights_by_name,floatdelta_beats,float&out_weight);",
                 "native exposes source CharWeightSetter poll helper");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightSetterRefOwner{std::stringname;"
                 "boolweight_owner_is_setter=false;};",
                 "native exposes source CharWeightSetter ref-owner row");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightSetterPollDeps{std::vector<std::string>"
                 "changed_by;std::vector<std::string>change;};",
                 "native exposes source CharWeightSetter PollDeps state");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightSetterState{SourceCharWeightableStateweightable;"
                 "boolhas_base=false;boolhas_driver=false;size_tmin_weight_count=0;"
                 "size_tmax_weight_count=0;uint32_tflags=0;floatoffset=0.0f;"
                 "floatscale=1.0f;floatbase_weight=0.0f;floatbeats_per_weight=0.0f;};",
                 "native exposes source CharWeightSetter state");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightSetterLoadPlan{boolrevision_supported=false;"
                 "std::vector<std::string>read_order;std::vector<std::string>"
                 "branches;};",
                 "native exposes source CharWeightSetter load plan");
  ok &= contains(char_clip_h,
                 "structSourceCharWeightSetterCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;};",
                 "native exposes source CharWeightSetter copy plan");
  ok &= contains(char_clip_h,
                 "SourceCharWeightSetterStatesource_char_weight_setter_default_state("
                 "conststd::string&name);",
                 "native exposes source CharWeightSetter constructor helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_weight_setter_set_weight("
                 "SourceCharWeightSetterState&state,floatweight);",
                 "native exposes source CharWeightSetter SetWeight helper");
  ok &= contains(char_clip_h,
                 "SourceCharWeightSetterLoadPlansource_char_weight_setter_load_plan("
                 "int32_trevision);",
                 "native exposes source CharWeightSetter load helper");
  ok &= contains(char_clip_h,
                 "SourceCharWeightSetterCopyPlansource_char_weight_setter_copy_plan();",
                 "native exposes source CharWeightSetter copy helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_weight_setter_poll_deps("
                 "SourceCharWeightSetterPollDeps&deps,constCharWeightSetter&"
                 "setter,conststd::vector<SourceCharWeightSetterRefOwner>&"
                 "ref_owners);",
                 "native exposes source CharWeightSetter PollDeps helper");
  ok &= contains(char_clip,
                 "SourceCharWeightableStatesource_char_weightable_default_state("
                 "conststd::string&name){SourceCharWeightableStatestate;"
                 "state.name=name;state.weight=1.0f;state.weight_owner=name;"
                 "returnstate;}",
                 "native CharWeightable constructor helper ports source defaults");
  ok &= contains(char_clip,
                 "voidsource_char_weightable_set_weight("
                 "SourceCharWeightableState&state,floatweight){state.weight=weight;}",
                 "native CharWeightable SetWeight helper ports source assignment");
  ok &= contains(char_clip,
                 "voidsource_char_weightable_set_weight_owner("
                 "SourceCharWeightableState&state,conststd::string&weight_owner){"
                 "state.weight_owner=weight_owner.empty()?state.name:weight_owner;}",
                 "native CharWeightable SetWeightOwner helper ports null fallback");
  ok &= contains(char_clip,
                 "voidsource_char_weightable_replace(SourceCharWeightableState&state,"
                 "conststd::string&old_owner,conststd::string&new_owner,"
                 "boolnew_owner_is_weightable){if(state.weight_owner==old_owner){",
                 "native CharWeightable Replace helper checks old owner");
  ok &= contains(char_clip,
                 "state.weight_owner=new_owner_is_weightable?new_owner:"
                 "std::string{};}if(state.weight_owner.empty())"
                 "state.weight_owner=state.name;}",
                 "native CharWeightable Replace helper ports self fallback");
  ok &= contains(char_clip,
                 "if(shallow_copy){source_char_weightable_set_weight_owner(dest,"
                 "source.weight_owner);}else{source_char_weightable_set_weight_owner("
                 "dest,dest.name);dest.weight=source_owner_weight;}",
                 "native CharWeightable Copy helper ports shallow and deep copy");
  ok &= contains(char_clip,
                 "SourceCharWeightableLoadPlansource_char_weightable_load_plan("
                 "int32_trevision){SourceCharWeightableLoadPlanplan;"
                 "plan.revision_supported=revision>=0&&revision<=2;",
                 "native CharWeightable load helper ports revision gate");
  ok &= contains(char_clip,
                 "plan.read_order.push_back(\"mWeight\");if(revision>1)"
                 "plan.read_order.push_back(\"mWeightOwner\");",
                 "native CharWeightable load helper ports read order");
  ok &= contains(char_clip,
                 "SourceCharWeightableCopyPlansource_char_weightable_copy_plan(){"
                 "SourceCharWeightableCopyPlanplan;plan.shallow_actions={"
                 "\"SetWeightOwner(source.mWeightOwner)\"};",
                 "native CharWeightable copy plan records shallow branch");
  ok &= contains(char_clip_h,
                 "structSourceCharMirrorState{SourceCharWeightableStateweightable;"
                 "std::stringservo;std::stringmirror_servo;size_tbones_total_size=0;"
                 "size_tops_count=0;};",
                 "native exposes source CharMirror state");
  ok &= contains(char_clip_h,
                 "SourceCharMirrorPollResultsource_char_mirror_poll("
                 "constSourceCharMirrorState&state,conststd::unordered_map<"
                 "std::string,float>&weights_by_name);",
                 "native exposes source CharMirror Poll helper");
  ok &= contains(char_clip_h,
                 "SourceCharMirrorSetServoResultsource_char_mirror_set_servo("
                 "SourceCharMirrorState&state,conststd::string&servo);",
                 "native exposes source CharMirror SetServo helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_mirror_poll_deps("
                 "SourceCharMirrorPollDeps&deps,constSourceCharMirrorState&state);",
                 "native exposes source CharMirror PollDeps helper");
  ok &= contains(char_clip_h,
                 "SourceCharMirrorLoadStepssource_char_mirror_load_steps();",
                 "native exposes source CharMirror Load order helper");
  ok &= contains(char_clip_h,
                 "SourceCharMirrorCopyResultsource_char_mirror_copy("
                 "SourceCharMirrorState&dest,constSourceCharMirrorState&source,"
                 "boolshallow_copy,floatsource_owner_weight);",
                 "native exposes source CharMirror Copy helper");
  ok &= contains(char_clip,
                 "SourceCharMirrorStatesource_char_mirror_default_state("
                 "conststd::string&name){SourceCharMirrorStatestate;"
                 "state.weightable=source_char_weightable_default_state(name);"
                 "returnstate;}",
                 "native CharMirror constructor helper ports defaults");
  ok &= contains(char_clip,
                 "result.weight=source_weightable_state_weight(state.weightable,"
                 "weights_by_name);result.weight_zero=result.weight==0.0f;"
                 "result.bones_empty=state.bones_total_size==0;",
                 "native CharMirror Poll helper computes source gate");
  ok &= contains(char_clip,
                 "if(!result.weight_zero&&!result.bones_empty){result."
                 "scale_down=true;result.scale_down_weight=1.0f-result.weight;"
                 "result.servo=state.servo;}",
                 "native CharMirror Poll helper ports ScaleDown request");
  ok &= contains(char_clip,
                 "if(servo!=state.servo){state.servo=servo;result.changed=true;"
                 "result.synced_bones=true;}",
                 "native CharMirror SetServo helper ports sync gate");
  ok &= contains(char_clip,
                 "if(mirror_servo!=state.mirror_servo){state.mirror_servo="
                 "mirror_servo;result.changed=true;result.synced_bones=true;}",
                 "native CharMirror SetMirrorServo helper ports sync gate");
  ok &= contains(char_clip,
                 "voidsource_char_mirror_poll_deps("
                 "SourceCharMirrorPollDeps&deps,constSourceCharMirrorState&state){"
                 "deps.change.push_back(state.servo);}",
                 "native CharMirror PollDeps helper ports change publication");
  ok &= contains(char_clip,
                 "steps.load_hmx_object=true;steps.load_weightable=true;"
                 "steps.load_mirror_servo=true;steps.load_servo=true;"
                 "steps.sync_bones=true;",
                 "native CharMirror Load order helper ports source sequence");
  ok &= contains(char_clip,
                 "source_char_weightable_copy(dest.weightable,source.weightable,"
                 "shallow_copy,source_owner_weight);result.set_mirror_servo="
                 "source_char_mirror_set_mirror_servo(dest,source.mirror_servo);"
                 "result.set_servo=source_char_mirror_set_servo(dest,source.servo);",
                 "native CharMirror Copy helper ports source setters");
  ok &= contains(char_clip,
                 "SourceCharWeightSetterStatesource_char_weight_setter_default_state("
                 "conststd::string&name){SourceCharWeightSetterStatestate;"
                 "state.weightable=source_char_weightable_default_state(name);",
                 "native CharWeightSetter constructor helper starts from CharWeightable");
  ok &= contains(char_clip,
                 "state.flags=0;state.offset=0.0f;state.scale=1.0f;"
                 "state.base_weight=0.0f;state.beats_per_weight=0.0f;returnstate;}",
                 "native CharWeightSetter constructor helper ports source defaults");
  ok &= contains(char_clip,
                 "voidsource_char_weight_setter_set_weight("
                 "SourceCharWeightSetterState&state,floatweight){state.base_weight=weight;"
                 "state.weightable.weight=weight;}",
                 "native CharWeightSetter SetWeight helper ports source assignment");
  ok &= contains(char_clip,
                 "SourceCharWeightSetterLoadPlansource_char_weight_setter_load_plan("
                 "int32_trevision){SourceCharWeightSetterLoadPlanplan;"
                 "plan.revision_supported=revision>=0&&revision<=9;",
                 "native CharWeightSetter load helper ports revision gate");
  ok &= contains(char_clip,
                 "plan.read_order.push_back(\"Hmx::Object\");if(revision>1)"
                 "plan.read_order.push_back(\"CharWeightable\");",
                 "native CharWeightSetter load helper ports superclass gate");
  ok &= contains(char_clip,
                 "if(revision<3){plan.branches.push_back(\"mScale=1.0\");"
                 "plan.branches.push_back(\"mOffset=0.0\");}",
                 "native CharWeightSetter load helper ports old scale branch");
  ok &= contains(char_clip,
                 "elseif(revision<4){plan.read_order.push_back("
                 "\"legacyInvertBool\");",
                 "native CharWeightSetter load helper ports legacy invert branch");
  ok &= contains(char_clip,
                 "if(revision>8){plan.read_order.push_back(\"mMinWeights\");"
                 "plan.read_order.push_back(\"mMaxWeights\");}else{"
                 "if(revision>6)plan.read_order.push_back(\"legacyMinWeight\");"
                 "if(revision>7)plan.read_order.push_back(\"legacyMaxWeight\");}",
                 "native CharWeightSetter load helper ports min/max revision gates");
  ok &= contains(char_clip,
                 "SourceCharWeightSetterCopyPlansource_char_weight_setter_copy_plan(){"
                 "SourceCharWeightSetterCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\",\"CharWeightable\"};",
                 "native CharWeightSetter copy helper records superclasses");
  ok &= contains(char_clip,
                 "plan.copied_members={\"mDriver\",\"mFlags\",\"mBase\","
                 "\"mOffset\",\"mScale\",\"mBaseWeight\",\"mBeatsPerWeight\","
                 "\"mMinWeights\",\"mMaxWeights\"};",
                 "native CharWeightSetter copy helper records member list");
  ok &= contains(char_clip, "returnsetter.weight;",
                 "native CharWeightable helper falls back to row weight");
  ok &= contains(char_clip, "if(!setter.driver.empty()){returnfalse;}",
                 "native CharWeightSetter helper fences missing driver evaluator");
  ok &= contains(char_clip,
                 "base_weight=setter.scale*base->second+setter.offset;",
                 "native CharWeightSetter helper ports base scale/offset");
  ok &= contains(char_clip,
                 "base_weight=std::min(base_weight,min_weight->second);",
                 "native CharWeightSetter helper ports min weight clamp");
  ok &= contains(char_clip,
                 "base_weight=std::max(base_weight,max_weight->second);",
                 "native CharWeightSetter helper ports max weight clamp");
  ok &= contains(char_clip,
                 "constfloatstep=delta_beats/setter.beats_per_weight;",
                 "native CharWeightSetter helper ports beat smoothing");
  ok &= contains(char_clip,
                 "voidsource_char_weight_setter_poll_deps("
                 "SourceCharWeightSetterPollDeps&deps,constCharWeightSetter&"
                 "setter,conststd::vector<SourceCharWeightSetterRefOwner>&"
                 "ref_owners){deps.changed_by.push_back(setter.driver);"
                 "deps.changed_by.push_back(setter.base);",
                 "native CharWeightSetter PollDeps helper starts with driver/base");
  ok &= contains(char_clip,
                 "for(constauto&min_name:setter.min_weights){deps.changed_by."
                 "push_back(min_name);}for(constauto&max_name:"
                 "setter.max_weights){deps.changed_by.push_back(max_name);}",
                 "native CharWeightSetter PollDeps helper publishes min/max rows");
  ok &= contains(char_clip,
                 "for(autoit=ref_owners.rbegin();it!=ref_owners.rend();++it){"
                 "if(it->weight_owner_is_setter)deps.change.push_back(it->name);}",
                 "native CharWeightSetter PollDeps helper scans ref owners in reverse");
  ok &= contains(char_clip, "\"[weightsetter-source-skip]",
                 "native CharWeightSetter logs missing driver evaluator tag");
  ok &= contains(char_clip,
                 "reason=missing-source-CharDriver-EvaluateFlags",
                 "native CharWeightSetter logs missing driver evaluator reason");
  ok &= contains(char_clip,
                 "character.runtime_weight_props[setter.name]=weight;",
                 "native CharWeightSetter publishes source weight row");
  ok &= contains(char_clip,
                 "apply_source_weight_setters(character,0.0f);",
                 "native controller cadence runs CharWeightSetter before IK");
  ok &= contains(weight_setter_source_test,
                 "source_char_weightable_default_state(\"self.weight\")",
                 "focused CharWeightSetter test covers CharWeightable defaults");
  ok &= contains(weight_setter_source_test,
                 "source_char_weightable_set_weight_owner(weightable,\"\")",
                 "focused CharWeightSetter test covers CharWeightable owner fallback");
  ok &= contains(weight_setter_source_test,
                 "source_char_weightable_replace(weightable,\"new.owner\","
                 "\"not.weightable\",false)",
                 "focused CharWeightSetter test covers CharWeightable Replace fallback");
  ok &= contains(weight_setter_source_test,
                 "source_char_weightable_copy(dest,source,false,0.90f)",
                 "focused CharWeightSetter test covers CharWeightable deep copy");
  ok &= contains(weight_setter_source_test,
                 "source_char_weightable_load_plan(2)",
                 "focused CharWeightSetter test covers CharWeightable load plan");
  ok &= contains(weight_setter_source_test,
                 "source_char_weightable_copy_plan()",
                 "focused CharWeightSetter test covers CharWeightable copy plan");
  ok &= contains(weight_setter_source_test,
                 "source_char_weight_setter_default_state(\"setter.weight\")",
                 "focused CharWeightSetter test covers constructor helper");
  ok &= contains(weight_setter_source_test,
                 "source_char_weight_setter_set_weight(setter_state,0.42f)",
                 "focused CharWeightSetter test covers SetWeight helper");
  ok &= contains(weight_setter_source_test,
                 "source_char_weight_setter_load_plan(9)",
                 "focused CharWeightSetter test covers current load plan");
  ok &= contains(weight_setter_source_test,
                 "source_char_weight_setter_load_plan(8)",
                 "focused CharWeightSetter test covers legacy min/max load plan");
  ok &= contains(weight_setter_source_test,
                 "source_char_weight_setter_copy_plan()",
                 "focused CharWeightSetter test covers copy plan");
  ok &= contains(weight_setter_source_test,
                 "ok&=!source_char_weight_setter_poll(driver,weights,0.0f,out);",
                 "focused CharWeightSetter test covers driver fence");
  ok &= contains(weight_setter_source_test,
                 "apply_character_controllers(character,0.0f,nullptr);",
                 "focused CharWeightSetter test covers controller writeback");
  ok &= contains(weight_setter_source_test,
                 "source_char_weight_setter_poll_deps(",
                 "focused CharWeightSetter test covers PollDeps helper");
  ok &= contains(weight_setter_source_test,
                 "conststd::vector<std::string>want_change={\"last.change\","
                 "\"first.change\"};",
                 "focused CharWeightSetter test covers reverse ref order");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_mirror_source_test",
                 "CMake builds CharMirror source test");
  ok &= contains(mirror_source_test,
                 "source_char_mirror_default_state(\"mirror.weight\")",
                 "focused CharMirror test covers constructor defaults");
  ok &= contains(mirror_source_test,
                 "source_char_mirror_set_servo(mirror,\"bone.servo\")",
                 "focused CharMirror test covers SetServo");
  ok &= contains(mirror_source_test,
                 "source_char_mirror_poll_deps(deps,mirror)",
                 "focused CharMirror test covers PollDeps");
  ok &= contains(mirror_source_test,
                 "source_char_mirror_poll(mirror,weights)",
                 "focused CharMirror test covers Poll");
  ok &= contains(mirror_source_test,
                 "source_char_mirror_load_steps()",
                 "focused CharMirror test covers Load order");
  ok &= contains(mirror_source_test,
                 "source_char_mirror_copy(dest,mirror,false,0.80f)",
                 "focused CharMirror test covers Copy");
  ok &= contains(doc,
                 "Native `source_char_mirror_poll` ports",
                 "document records native CharMirror Poll helper");
  ok &= contains(doc,
                 "`SyncBones` rebuilding body is not present in `rb3-latest`",
                 "document fences missing CharMirror SyncBones body");
  ok &= contains(doc,
                 "Native `source_char_weight_setter_poll_deps` ports",
                 "document records native CharWeightSetter PollDeps helper");
  ok &= contains(doc,
                 "Native `source_char_weight_setter_load_plan` and",
                 "document records native CharWeightSetter load/copy plans");
  ok &= contains(doc,
                 "legacy revision 3 invert-bool branch",
                 "document records CharWeightSetter legacy invert branch");
  ok &= contains(doc,
                 "Native `source_char_weight_setter_default_state` and",
                 "document records native CharWeightSetter constructor helper");
  ok &= contains(doc,
                 "Native `source_char_weightable_load_plan` and",
                 "document records native CharWeightable load/copy plans");
  ok &= contains(doc,
                 "`SetWeight` writes both\n    `mBaseWeight` and inherited `mWeight`",
                 "document records CharWeightSetter SetWeight behavior");
  ok &= contains(doc,
                 "`CharWeightSetter::PollDeps` dependency publication",
                 "document records CharWeightSetter PollDeps source behavior");
  ok &= contains(rb3_latest_char_driver_h,
                 "ObjPtr<CharBonesObject,ObjectDir>mBones;",
                 "latest CharDriver header exposes driven bones pointer");
  ok &= contains(rb3_latest_char_driver_h,
                 "ObjPtr<ObjectDir,ObjectDir>mClips;",
                 "latest CharDriver header exposes clip directory pointer");
  ok &= contains(rb3_latest_char_driver_h,
                 "ObjPtr<Hmx::Object,ObjectDir>mDefaultClip;",
                 "latest CharDriver header exposes default clip pointer");
  ok &= contains(rb3_latest_char_driver_h,
                 "ApplyModemApply;",
                 "latest CharDriver header exposes apply mode");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "if(mDefaultClip)Play(DataNode(mDefaultClip),1,-1.0f,"
                 "1e+30f,0.0f);",
                 "latest CharDriver Enter can play the default clip");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "mFirst=newCharClipDriver(this,clip,i,f1,mFirst,f2,f3,"
                 "mPlayMultipleClips);",
                 "latest CharDriver Play builds CharClipDriver nodes");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "change.push_back(mBones);",
                 "latest CharDriver PollDeps depends on bones");
  ok &= missing(rb3_latest_char_driver_cpp, "BEGIN_LOADS(CharDriver)",
                "latest CharDriver source lacks base load body");
  ok &= missing(rb3_latest_char_driver_cpp, "voidCharDriver::Poll(",
                "latest CharDriver source lacks base Poll body");
  ok &= contains(rb2_char_driver_cpp,
                 "voidCharDriver::Load(classCharDriver*constthis/*r30*/,"
                 "classBinStream&d/*r31*/){}",
                 "RB2 dump CharDriver Load body is empty");
  ok &= contains(rb3_latest_char_driver_midi_h,
                 "SymbolmParser;",
                 "latest CharDriverMidi header exposes parser symbol");
  ok &= contains(rb3_latest_char_driver_midi_h,
                 "SymbolmFlagParser;",
                 "latest CharDriverMidi header exposes flag parser symbol");
  ok &= contains(rb3_latest_char_driver_midi_h,
                 "floatmBlendOverridePct;",
                 "latest CharDriverMidi header exposes blend override");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "CharDriverMidi::CharDriverMidi():mParser(),mFlagParser(),"
                 "mClipFlags(0),mBlendOverridePct(1.0f)",
                 "CharDriverMidi source constructor exposes defaults");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "voidCharDriverMidi::Enter(){unk89=true;CharDriver::Enter();"
                 "MsgSource*msgParser=dynamic_cast<MsgSource*>(Dir()->"
                 "FindObject(mParser.Str(),true));if(msgParser)msgParser->"
                 "AddSink(this);",
                 "CharDriverMidi source Enter sets unk89 and adds parser sink");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "MsgSource*msgFlagParser=dynamic_cast<MsgSource*>(Dir()->"
                 "FindObject(mFlagParser.Str(),true));if(msgFlagParser)"
                 "msgFlagParser->AddSink(this);}",
                 "CharDriverMidi source Enter adds flag parser sink");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "voidCharDriverMidi::Exit(){CharDriver::Exit();MsgSource*"
                 "msgParser=ObjectDir::Main()->Find<MsgSource>(mParser.Str(),"
                 "false);if(msgParser)msgParser->RemoveSink(this);",
                 "CharDriverMidi source Exit removes parser sink");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "DataNodeCharDriverMidi::OnMidiParserFlags(DataArray*da){"
                 "mClipFlags=da->Int(2);returnDataNode(0);}",
                 "CharDriverMidi source flags message stores clip flags");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "if(!unk89&&mDefaultClip)b=true;if(b)clip=dynamic_cast<"
                 "CharClip*>(mDefaultClip.Ptr());elseclip=FindClip("
                 "da->Node(2),false);",
                 "CharDriverMidi source parser chooses default clip branch");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "if(clip->mPlayFlags&0x200){floatsecs=TheTaskMgr.Seconds("
                 "TaskMgr::b);floatbts=BeatToSeconds(somefloat+"
                 "TheTaskMgr.Beat())-secs;somefloat=bts*clip->"
                 "AverageBeatsPerSecond();}",
                 "CharDriverMidi source parser ports real-time blend conversion");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "Play(clip,0,somefloat*mBlendOverridePct,-somefloat,0.0f);",
                 "CharDriverMidi source parser plays with blend override");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "mClipFlags,PathName(grp));returnDataNode(0);}else{if(clip||"
                 "clip!=FirstClip()){floatsomefloat=da->Float(3);",
                 "CharDriverMidi source group parser handles missing clip");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "Play(clip,0,-somefloat,1e+30f,0.0f)->mBlendWidth="
                 "somefloat*mBlendOverridePct;",
                 "CharDriverMidi source group parser assigns returned blend width");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(7,0)LOAD_SUPERCLASS(CharDriver)",
                 "CharDriverMidi source load begins with source superclass");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "if(gRev<7){mDefaultClip.Load(bs,false,mClips);}",
                 "CharDriverMidi source load gates default clip pointer");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "elseif(gRev>3)bs>>mParser;if(gRev>4)bs>>mFlagParser;"
                 "if(gRev>5)bs>>mBlendOverridePct;",
                 "CharDriverMidi source load gates parser fields");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "BEGIN_COPYS(CharDriverMidi)COPY_SUPERCLASS(CharDriver)"
                 "CREATE_COPY(CharDriverMidi)BEGIN_COPYING_MEMBERS"
                 "COPY_MEMBER(unk89)COPY_MEMBER(mParser)"
                 "COPY_MEMBER(mFlagParser)COPY_MEMBER(mBlendOverridePct)",
                 "CharDriverMidi source Copy member list");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "HANDLE(midi_parser,OnMidiParser)",
                 "CharDriverMidi source handles midi_parser messages");
  ok &= contains(rb3_latest_char_driver_midi_cpp,
                 "HANDLE(midi_parser_group,OnMidiParserGroup)",
                 "CharDriverMidi source handles midi_parser_group messages");
  ok &= contains(char_mesh_h,
                 "structCharDriver{std::stringname;int32_tversion=0;"
                 "int32_tweightable_version=0;",
                 "native CharDriver stores source revisions");
  ok &= contains(char_mesh_h,
                 "std::stringweight_owner;std::stringweight_prop;",
                 "native CharDriver keeps source owner plus compatibility alias");
  ok &= contains(char_mesh_h,
                 "int32_tmidi_version=0;size_tmidi_unread_bytes=0;"
                 "std::stringmidi_default_clip;"
                 "std::stringmidi_legacy_string;std::stringmidi_parser;",
                 "native CharDriver stores MIDI source revision and default clip");
  ok &= contains(char_clip_h,
                 "structSourceCharDriverMidiState{boolunk89=false;"
                 "std::stringparser;std::stringflag_parser;intclip_flags=0;"
                 "floatblend_override_pct=1.0f;boolhas_default_clip=false;};",
                 "native exposes source CharDriverMidi default state");
  ok &= contains(char_clip_h,
                 "SourceCharDriverMidiStatesource_char_driver_midi_default_state();",
                 "native exposes source CharDriverMidi constructor helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverMidiEnterDecisionsource_char_driver_midi_enter("
                 "SourceCharDriverState&driver_state,SourceCharDriverMidiState&"
                 "midi_state,boolparser_found,boolflag_parser_found);",
                 "native exposes source CharDriverMidi Enter helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverMidiExitDecisionsource_char_driver_midi_exit("
                 "boolparser_found,boolflag_parser_found);",
                 "native exposes source CharDriverMidi Exit helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_midi_on_parser_flags("
                 "SourceCharDriverMidiState&midi_state,intclip_flags);",
                 "native exposes source CharDriverMidi flags helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverMidiParserDecisionsource_char_driver_midi_on_parser("
                 "constSourceCharDriverMidiState&midi_state,boolfound_clip,"
                 "boolclip_uses_real_time,floatmessage_float,",
                 "native exposes source CharDriverMidi parser helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverMidiParserDecisionsource_char_driver_midi_on_parser_group("
                 "constSourceCharDriverMidiState&midi_state,boolfound_group,"
                 "boolfound_group_clip,boolclip_uses_real_time,",
                 "native exposes source CharDriverMidi parser group helper");
  ok &= contains(char_clip_h,
                 "structSourceCharDriverMidiCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;"
                 "std::vector<std::string>not_in_source_copy_members;};",
                 "native exposes source CharDriverMidi copy plan");
  ok &= contains(char_clip_h,
                 "SourceCharDriverMidiCopyPlansource_char_driver_midi_copy_plan();",
                 "native exposes source CharDriverMidi copy helper");
  ok &= contains(char_mesh,
                 "driver.version=r.i32();",
                 "native CharDriver decoder reads driver revision");
  ok &= contains(char_mesh,
                 "read_object_fields(r);",
                 "native CharDriver decoder reads object fields");
  ok &= contains(char_mesh,
                 "driver.weightable_version=r.i32();",
                 "native CharDriver decoder reads CharWeightable revision");
  ok &= contains(char_mesh,
                 "if(driver.weightable_version>1)driver.weight_owner=r.str();",
                 "native CharDriver decoder mirrors CharWeightable owner gate");
  ok &= contains(char_mesh,
                 "if(driver.midi_version<7&&r.pos<r.n)"
                 "driver.midi_default_clip=r.str();",
                 "native CharDriverMidi decodes source default clip pointer");
  ok &= contains(char_mesh,
                 "if(midi_version<0||midi_version>7){throwstd::runtime_error",
                 "native CharDriverMidi decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "if(driver.midi_version==2&&r.pos<r.n)"
                 "driver.midi_legacy_string=r.str();",
                 "native CharDriverMidi decodes source rev-2 legacy string");
  ok &= contains(char_mesh,
                 "if(driver.midi_version>3&&r.pos<r.n)"
                 "driver.midi_parser=r.str();",
                 "native CharDriverMidi parser decode follows source gate");
  ok &= contains(char_mesh,
                 "driver.midi_unread_bytes=r.n-r.pos;",
                 "native CharDriverMidi records remaining unread bytes");
  ok &= contains(bind_audit,
                 "\"[controller-driver]char=%sname=%sversion=%d",
                 "controller audit logs CharDriver source revision");
  ok &= contains(bind_audit,
                 "\"weightOwner=%sweightProp=%senabled=%dmidi=%d",
                 "controller audit logs CharDriver source weight owner");
  ok &= contains(bind_audit,
                 "\"midiVersion=%dmidiUnreadBytes=%zumidiDefaultClip=%s",
                 "controller audit logs CharDriverMidi source fields");
  ok &= contains(char_clip,
                 "SourceCharDriverMidiStatesource_char_driver_midi_default_state(){"
                 "returnSourceCharDriverMidiState{};}",
                 "native CharDriverMidi default helper returns source defaults");
  ok &= contains(char_clip,
                 "midi_state.unk89=true;decision.set_unk89=true;"
                 "decision.driver_enter=source_char_driver_enter(driver_state);",
                 "native CharDriverMidi Enter helper sets unk89 and enters driver");
  ok &= contains(char_clip,
                 "decision.add_parser_sink=parser_found;decision.add_flag_parser_sink="
                 "flag_parser_found;",
                 "native CharDriverMidi Enter helper records sink decisions");
  ok &= contains(char_clip,
                 "SourceCharDriverMidiExitDecisionsource_char_driver_midi_exit("
                 "boolparser_found,boolflag_parser_found){"
                 "SourceCharDriverMidiExitDecisiondecision;decision.call_driver_exit=true;",
                 "native CharDriverMidi Exit helper records base exit");
  ok &= contains(char_clip,
                 "voidsource_char_driver_midi_on_parser_flags("
                 "SourceCharDriverMidiState&midi_state,intclip_flags){"
                 "midi_state.clip_flags=clip_flags;}",
                 "native CharDriverMidi flags helper stores source flags");
  ok &= contains(char_clip,
                 "decision.used_default_clip=!midi_state.unk89&&"
                 "midi_state.has_default_clip;if(!decision.used_default_clip&&"
                 "!found_clip)returndecision;",
                 "native CharDriverMidi parser helper ports default clip gate");
  ok &= contains(char_clip,
                 "blend=(beat_to_seconds_message_plus_current-task_seconds)*"
                 "average_beats_per_second;",
                 "native CharDriverMidi parser helper ports real-time conversion");
  ok &= contains(char_clip,
                 "decision.requested_blend_width=blend*midi_state."
                 "blend_override_pct;decision.old_beat=-blend;",
                 "native CharDriverMidi parser helper ports Play arguments");
  ok &= contains(char_clip,
                 "if(!found_group)returndecision;decision.used_default_clip="
                 "!midi_state.unk89&&midi_state.has_default_clip;",
                 "native CharDriverMidi group helper ports missing group/default gate");
  ok &= contains(char_clip,
                 "decision.requested_blend_width=-blend;decision.old_beat=1.0e30f;"
                 "decision.start=0.0f;decision.assigned_blend_width=blend*"
                 "midi_state.blend_override_pct;",
                 "native CharDriverMidi group helper ports returned blend assignment");
  ok &= contains(char_clip,
                 "SourceCharDriverMidiCopyPlansource_char_driver_midi_copy_plan(){"
                 "SourceCharDriverMidiCopyPlanplan;plan.copied_superclasses="
                 "{\"CharDriver\"};plan.copied_members={\"unk89\",\"mParser\","
                 "\"mFlagParser\",\"mBlendOverridePct\"};"
                 "plan.not_in_source_copy_members={\"mClipFlags\"};returnplan;}",
                 "native CharDriverMidi copy helper mirrors source copy list");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_midi_default_state()",
                 "focused clip driver test covers CharDriverMidi defaults");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_midi_enter(driver_state,midi,true,false)",
                 "focused clip driver test covers CharDriverMidi Enter helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_midi_on_parser_flags(midi,0x1234)",
                 "focused clip driver test covers CharDriverMidi flags helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_midi_on_parser(",
                 "focused clip driver test covers CharDriverMidi parser helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_midi_on_parser_group(",
                 "focused clip driver test covers CharDriverMidi parser group helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_midi_copy_plan()",
                 "focused clip driver test covers CharDriverMidi copy plan");
  ok &= contains(doc,
                 "`CharDriverMidi::Load` reads the subclass revision",
                 "document records CharDriverMidi source load");
  ok &= contains(doc,
                 "Native `source_char_driver_midi_default_state`,",
                 "document records native CharDriverMidi source helper slice");
  ok &= contains(doc,
                 "Native `source_char_driver_midi_copy_plan` records the checked source copy",
                 "document records native CharDriverMidi copy helper slice");
  ok &= contains(doc,
                 "The source copy body does not name `mClipFlags`",
                 "document records CharDriverMidi copy omission boundary");
  ok &= contains(doc,
                 "group-message assignment of the returned node's `mBlendWidth`",
                 "document records CharDriverMidi group blend assignment");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharDriver.cpp` and "
                 "`CharDriver.h`",
                 "document cites latest CharDriver source files");
  ok &= contains(doc,
                 "Base `CharDriver::Load`/`Poll` bodies are not present in the "
                 "available source",
                 "document records missing base CharDriver Load body");
  ok &= contains(doc,
                 "25 base `CharDriver` rows across the 24 base character MILOs",
                 "document records stock base CharDriver inventory");
  ok &= contains(doc,
                 "Native GHOGX therefore decodes/logs that slot\n"
                 "    as `midiDefaultClip`",
                 "document promotes CharDriverMidi default clip pointer");
  ok &= contains(doc,
                 "and enforces the source MIDI-driver revision range",
                 "document records CharDriverMidi revision enforcement");
  ok &= contains(doc,
                 "source_chardrivermidi_20260711/stock_chardrivermidi"
                 "_controllers.stdout.log",
                 "document cites refreshed CharDriverMidi proof log");
  ok &= contains(doc,
                 "all 38 stock\n    `CharDriverMidi` rows are `midiVersion=3`, "
                 "have no default clip, and report\n    `midiUnreadBytes=0`",
                 "document records refreshed CharDriverMidi zero-tail proof");
  ok &= contains(doc,
                 "shows 38 `CharDriverMidi` rows",
                 "document records refreshed CharDriverMidi stock inventory");
  ok &= contains(doc,
                 "`midiVersion=3` with `midiDefaultClip=<none>`, `midiUnreadBytes=0`",
                 "document records GH2 CharDriverMidi default-clip proof");
  ok &= contains(rb3_latest_anim_filter_h,
                 "ObjPtr<RndAnimatable,classObjectDir>mAnim;",
                 "latest RndAnimFilter header exposes anim pointer");
  ok &= contains(rb3_latest_anim_filter_h,
                 "floatmPeriod;",
                 "latest RndAnimFilter header exposes period");
  ok &= contains(rb3_latest_anim_filter_cpp,
                 "Hmx::Object::Load(bs);RndAnimatable::Load(bs);"
                 "bs>>mAnim>>mScale>>mOffset>>mStart>>mEnd;",
                 "RndAnimFilter source load reads object, animatable, and range rows");
  ok &= contains(rb3_latest_anim_filter_cpp,
                 "if(gRev!=0){bs>>(int&)mType;bs>>mPeriod;}",
                 "RndAnimFilter source load gates type and period");
  ok &= contains(rb3_latest_anim_filter_cpp,
                 "if(gRev>1){bs>>mSnap>>mJitter;}",
                 "RndAnimFilter source load gates snap and jitter");
  ok &= contains(rb3_latest_anim_cpp,
                 "BEGIN_LOADS(RndAnimatable)LOAD_REVS(bs);ASSERT_REVS(4,0);"
                 "if(gRev>1)bs>>mFrame;",
                 "RndAnimatable source load reads frame gate");
  ok &= contains(rb3_latest_anim_cpp,
                 "if(gRev>3){bs>>(int&)mRate;}elseif(gRev>2){"
                 "unsignedcharuc;bs>>uc;mRate=(Rate)(uc==0);}",
                 "RndAnimatable source load reads rate gates");
  ok &= contains(char_mesh_h,
                 "structRndAnimFilter{std::stringname;int32_tversion=0;",
                 "native stores RndAnimFilter source fields");
  ok &= contains(char_mesh_h,
                 "int32_tanimatable_version=0;floatframe=0.0f;int32_trate=0;",
                 "native stores RndAnimatable source fields");
  ok &= contains(char_mesh,
                 "RndAnimatableFieldsread_rnd_animatable(Reader&r)",
                 "native has source-named RndAnimatable reader");
  ok &= contains(char_mesh,
                 "throwstd::runtime_error(\"char_mesh:RndAnimatablerev0object-listbranchnotdecoded\");",
                 "native fences RndAnimatable old object-list branch");
  ok &= contains(char_mesh,
                 "RndAnimFilterdecode_anim_filter(conststd::string&entry_name",
                 "native decodes RndAnimFilter rows");
  ok &= contains(char_mesh,
                 "filter.anim=r.str();filter.scale=r.f32();filter.offset=r.f32();"
                 "filter.start=r.f32();filter.end=r.f32();",
                 "native RndAnimFilter decoder mirrors source range rows");
  ok &= contains(char_mesh,
                 "elseif(de.type==\"AnimFilter\"){out.anim_filters.push_back",
                 "character load stores decoded AnimFilter rows");
  ok &= contains(bind_audit,
                 "\"[controller-anim-filter]char=%sname=%sversion=%d",
                 "controller audit logs AnimFilter source revision");
  ok &= contains(bind_audit,
                 "\"animatableVersion=%danim=%sframe=%.4frate=%dscale=%.4f",
                 "controller audit logs AnimFilter source fields");
  ok &= contains(doc,
                 "`RndAnimFilter::Load` accepts source revisions through 2",
                 "document records RndAnimFilter source load");
  ok &= contains(doc,
                 "shows one stock `AnimFilter` row, on `metal_drummer`",
                 "document records stock AnimFilter inventory");
  ok &= contains(doc,
                 "stock_character_animfilter_inventory.log",
                 "document cites refreshed AnimFilter proof log");
  ok &= contains(doc,
                 "name=crash_static.filt version=1",
                 "document records stock AnimFilter row identity");
  ok &= contains(doc,
                 "unreadBytes=0",
                 "document records stock AnimFilter fully consumed proof");
  ok &= contains(doc,
                 "`CharWalk::Load` itself has no\n  decompiled body",
                 "document fences CharWalk layout");
  ok &= contains(rb2_char_walk_cpp,
                 "voidCharWalk::Load(classCharWalk*constthis/*r29*/,"
                 "classBinStream&d/*r30*/){",
                 "RB2 dump exposes CharWalk Load symbol");
  ok &= contains(rb2_char_walk_cpp,
                 "classDebugTheDebug;//->staticintgRev;",
                 "RB2 dump CharWalk Load has no field-read body");
  ok &= contains(rb2_outfit_loader_cpp,
                 "voidOutfitLoader::Load(classOutfitLoader*constthis/*r30*/,"
                 "classBinStream&d/*r31*/){}",
                 "RB2 dump OutfitLoader Load has no serialized field body");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "BEGIN_LOADS(EventTrigger)LOAD_REVS(bs)ASSERT_REVS(0x11,0)"
                 "LOAD_SUPERCLASS(Hmx::Object)",
                 "latest EventTrigger source exposes load entry");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "if(gRev>6)bs>>mAnims>>mSounds>>mShows;",
                 "EventTrigger source load reads object lists/vectors");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "bs>>anim.mAnim>>anim.mBlend>>anim.mWait>>anim.mDelay;",
                 "EventTrigger Anim source reads first four fields");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "if(EventTrigger::gRev>9){bs>>anim.mEnable;",
                 "EventTrigger Anim source gates extended fields");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "bs>>pcall.mProxy;bs>>pcall.mCall;",
                 "EventTrigger ProxyCall source reads proxy and call");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "if(gRev>7)bs>>mProxyCalls;",
                 "EventTrigger source load reads proxy call vector");
  ok &= contains(rb3_latest_event_trigger_cpp,
                 "if(gRev>0x10)bs>>mPartLaunchers;",
                 "EventTrigger source load reads part launcher list");
  ok &= contains(rb3_latest_event_trigger_h,
                 "ObjVector<ProxyCall>mProxyCalls;",
                 "EventTrigger header exposes ObjVector boundary");
  ok &= contains(rb3_latest_event_trigger_h,
                 "ObjPtrList<Sequence,classObjectDir>mSounds;",
                 "EventTrigger header exposes ObjPtrList boundary");
  ok &= contains(rb3_latest_event_trigger_h,
                 "inlineBinStream&operator>>(BinStream&bs,"
                 "EventTrigger::HideDelay&hd)",
                 "EventTrigger header exposes custom HideDelay serialization");
  ok &= contains(rb3_latest_obj_vector_h,
                 "unsignedintlength;bs>>length;vec.resize(length);",
                 "ObjVector source reads count before element rows");
  ok &= contains(rb3_latest_obj_vector_h,
                 "for(std::vector<T1,T2>::iteratorit=vec.begin();"
                 "it!=vec.end();it++){bs>>*it;}",
                 "ObjVector source reads each element through operator");
  ok &= contains(rb3_latest_obj_ptr_p_h,
                 "bs.ReadString(buf,0x80);",
                 "ObjPtr source reads object names as bounded strings");
  ok &= contains(rb3_latest_obj_ptr_p_h,
                 "intcount;bs>>count;",
                 "ObjPtrList source reads count before row strings");
  ok &= contains(rb3_latest_bin_stream_h,
                 "template<classT1,classT2>BinStream&operator>>"
                 "(BinStream&bs,std::vector<T1,T2>&vec){"
                 "unsignedintlength;bs>>length;vec.resize(length);",
                 "BinStream source backs std::vector read shape");
  ok &= contains(rb3_latest_bin_stream_h,
                 "BinStream&operator>>(bool&b){unsignedcharuc;*this>>uc;"
                 "b=(uc!=0);",
                 "BinStream source backs one-byte bool reads");
  ok &= contains(rb3_latest_bin_stream_cpp,
                 "BinStream&BinStream::operator>>(Symbol&s){charwhy[0x200];"
                 "ReadString(why,0x200);s=Symbol(why);",
                 "BinStream source backs Symbol string rows");
  ok &= contains(rb3_latest_object_h,
                 "inlineunsignedshortgetHmxRev(intpacked){returnpacked;}",
                 "Object source backs low-half HMX revision");
  ok &= contains(rb3_latest_object_h,
                 "inlineunsignedshortgetAltRev(intpacked){return(unsignedint)"
                 "packed>>0x10;}",
                 "Object source backs high-half alt revision");
  ok &= contains(rb3_latest_tex_cpp,
                 "voidRndTex::Load(BinStream&bs){PreLoad(bs);PostLoad(bs);}",
                 "latest RndTex source exposes preload/postload split");
  ok &= contains(rb3_latest_tex_cpp,
                 "if(gRev>8)LOAD_SUPERCLASS(Hmx::Object)",
                 "latest RndTex source gates object fields");
  ok &= contains(rb3_latest_tex_cpp,
                 "bs>>mWidth>>mHeight;SetPowerOf2();bs>>mBpp;bs>>mFilepath;",
                 "latest RndTex source reads texture metadata");
  ok &= contains(rb3_latest_tex_h,
                 "enumType{Regular=1,Rendered=2,Movie=4,BackBuffer=8,"
                 "FrontBuffer=0x18,RenderedNoZ=0x22",
                 "latest RndTex source backs texture type flags");
  ok &= contains(rb3_latest_file_path_h,
                 "inlineBinStream&operator>>(BinStream&bs,FilePath&fp){"
                 "charbuf[0x100];bs.ReadString(buf,0x100);fp.SetRoot(buf);"
                 "returnbs;}",
                 "FilePath source backs bounded texture filepath string rows");
  ok &= contains(rb3_latest_tex_cpp,
                 "if(gRev<5){intcubemapmask;bs>>cubemapmask;",
                 "latest RndTex source backs legacy cubemap mask row");
  ok &= contains(rb3_latest_tex_cpp,
                 "if(gRev>7)bs>>mMipMapK;elseif(gRev>3){inti;bs>>i;"
                 "mMipMapK=i/16.0f;}",
                 "latest RndTex source backs mip-map field gates");
  ok &= contains(rb3_latest_tex_cpp,
                 "if(gRev>6){bs>>(int&)mType;}elseif(gRev>5){"
                 "Typetypes[5]={Regular,Rendered,Movie,BackBuffer,FrontBuffer};",
                 "latest RndTex source backs texture type gates");
  ok &= contains(rb3_latest_tex_cpp,
                 "if(gRev>10){boolb;bs>>b;mOptimizeForPS3=b;}",
                 "latest RndTex source backs optimize flag gate");
  ok &= contains(rb3_latest_tex_cpp,
                 "if(bs.Cached()){void*buffer=0;intsize=0;",
                 "latest RndTex source backs cached bitmap payload branch");
  ok &= contains(rb3_latest_tex_cpp,
                 "elsemBitmap.Load(bs);",
                 "latest RndTex source delegates cached payload to RndBitmap");
  ok &= contains(rb3_latest_bitmap_cpp,
                 "BinStream&RndBitmap::LoadHeader(BinStream&bs,u8&test){"
                 "u8ver,h;u8pad[0x13];bs>>ver;bs>>mBpp;",
                 "latest RndBitmap source backs cached bitmap header rows");
  ok &= contains(rb3_latest_bitmap_cpp,
                 "bs>>test;bs>>mWidth;bs>>mHeight;bs>>mRowBytes;",
                 "latest RndBitmap source backs dimensions and row bytes");
  ok &= contains(rb3_latest_bitmap_cpp,
                 "intRndBitmap::PaletteBytes()const{if(mBpp<=8){"
                 "if((mOrder&0x38)==0&&(mOrder&0x80)==0){"
                 "return(1<<mBpp)*4;}}return0;}",
                 "latest RndBitmap source backs palette-byte size");
  ok &= contains(rb3_latest_bitmap_cpp,
                 "if(mPalette)bs.Read(mPalette,PaletteBytes());"
                 "ReadChunks(bs,mPixels,mRowBytes*mHeight,0x8000);",
                 "latest RndBitmap source backs base payload read length");
  ok &= contains(rb3_latest_bitmap_cpp,
                 "working_w=working_w>>1;working_h=working_h>>1;"
                 "newMip->Create(working_w,working_h,0,mBpp,mOrder,mPalette,0,0);"
                 "ReadChunks(bs,newMip->mPixels,newMip->mRowBytes*"
                 "newMip->mHeight,0x8000);",
                 "latest RndBitmap source backs mip payload loop");
  ok &= contains(rb3_latest_bitmap_cpp,
                 "elseif(mBpp*mWidth/8!=mRowBytes)",
                 "latest RndBitmap source backs row-byte relation");
  ok &= contains(rb3_latest_chunk_stream_h,
                 "BinStream&ReadChunks(BinStream&,void*,int,int);",
                 "latest ChunkStream header exposes ReadChunks");
  ok &= contains(rb3_latest_chunk_stream_cpp,
                 "while(curr_size!=total_len){intlen_left=Min(total_len-"
                 "curr_size,max_chunk_size);",
                 "latest ChunkStream source reads chunks until total length");
  ok &= contains(rb3_latest_chunk_stream_cpp,
                 "bs.Read(&dataAsChars[curr_size],len_left);curr_size+="
                 "len_left;",
                 "latest ChunkStream source backs exact chunk byte reads");
  ok &= contains(rb2_dolmatch_filt,
                 "FixClassName__9DirLoaderF6Symbol@WorldFx@3",
                 "RB2 dump exposes only WorldFx DirLoader fixup evidence");
  ok &= contains(doc, "## Remaining Stock Type Boundary",
                 "document records remaining stock type boundary");
  ok &= contains(doc, "`CharWalk`: 19 stock rows",
                 "document records stock CharWalk row count");
  ok &= contains(doc, "`OutfitLoader`: 20 stock rows",
                 "document records stock OutfitLoader row count");
  ok &= contains(doc, "`CharPollGroup`: zero stock rows",
                 "document records stock CharPollGroup absence");
  ok &= contains(doc,
                 "source_truth_poll_inventory_20260710/"
                 "stock_character_type_inventory.log",
                 "document cites focused poll inventory proof log");
  ok &= contains(doc,
                 "finds no `CharPollGroup` rows across the 24 base character "
                 "MILOs",
                 "document records no stock CharPollGroup rows");
  ok &= contains(doc, "records 21 `FaceFxLipSyncServo` rows",
                 "document records stock FaceFxLipSyncServo count");
  ok &= contains(doc,
                 "except `metal_bass`,\n  `metal_drummer`, and `metal_keyboard`",
                 "document records stock FaceFxLipSyncServo absences");
  ok &= contains(doc,
                 "do not expose a\n    matching `FaceFxLipSyncServo::Load` body",
                 "document records missing FaceFxLipSyncServo source body");
  ok &= contains(rb3_latest_char_poll_group_cpp,
                 "if(gRev>2)CharWeightable::Load(bs);bs>>mPolls;"
                 "if(gRev>1){bs>>mChangedBy;bs>>mChanges;}",
                 "CharPollGroup source load order");
  ok &= contains(rb3_latest_char_poll_group_cpp,
                 "if(mWeightOwner->mWeight!=0.0f){for(ObjPtrList<"
                 "CharPollable,ObjectDir>::iteratorit=mPolls.begin();"
                 "it!=mPolls.end();++it){(*it)->Poll();}}",
                 "CharPollGroup source Poll iterates child poll rows by weight");
  ok &= missing(char_mesh, "decode_poll_group",
                "native must not decode absent CharPollGroup rows");
  ok &= missing(char_mesh, "out.poll_groups",
                "native must not store absent CharPollGroup rows");
  ok &= missing(char_mesh_h, "std::vector<CharPollGroup>",
                "native character model must not declare active CharPollGroup rows");
  ok &= contains(doc, "`EventTrigger`: one stock row, on `metal_drummer`",
                 "document records stock EventTrigger row count");
  ok &= contains(doc, "## Event Trigger Row Authority",
                 "document records EventTrigger source authority section");
  ok &= contains(doc,
                 "records the only stock row as `char=metal_drummer "
                 "name=game_over.trig\n  version=8`",
                 "document records focused EventTrigger stock proof");
  ok &= contains(doc, "`tailHex=00:00:00:00`",
                 "document records unresolved EventTrigger tail");
  ok &= contains(doc,
                 "It does not register events, trigger animations, play "
                 "sounds, hide/show\n  drawables, or schedule tasks.",
                 "document fences EventTrigger runtime scheduling");
  ok &= contains(doc, "`Object`: 19 stock generic object rows",
                 "document records generic Object boundary");
  ok &= contains(doc, "## Generic Object Row Authority",
                 "document records generic Object source authority section");
  ok &= contains(doc, "records 19 stock `Object` rows",
                 "document records focused generic Object stock proof");
  ok &= contains(doc, "all report `unreadBytes=0`",
                 "document records generic Object rows decode cleanly");
  ok &= contains(doc, "`Tex`: 160 stock texture rows",
                 "document records stock Tex row count");
  ok &= contains(doc, "`WorldFx`: 99 stock rows",
                 "document records stock WorldFx row count");
  ok &= contains(doc,
                 "Native now decodes and\n  logs the source-backed field "
                 "prefix using `EventTrigger::Load`",
                 "document promotes EventTrigger to passive source inventory");
  ok &= contains(doc,
                 "native texture\n  payloads are already handled by the PS2 "
                 "texture asset path",
                 "document keeps Tex rows in asset texture path");
  ok &= contains(doc, "## Rnd Texture Row Authority",
                 "document records RndTex source authority section");
  ok &= contains(doc,
                 "records 160 stock `Tex` rows with source "
                 "`RndBitmap::LoadHeader` fields",
                 "document records focused RndTex stock proof");
  ok &= contains(doc, "all 160 stock\nrows report `payloadSizeMatch=1`",
                 "document records focused RndBitmap payload proof");
  ok &= contains(doc,
                 "The inventory includes two stock mip textures\n"
                 "(`metal_keyboard_mip.tex` and `metal_singer_belt_mip.tex`)",
                 "document records focused RndBitmap mip proof");
  ok &= contains(doc,
                 "there is no\n  checked `WorldFx::Load` source body",
                 "document fences WorldFx load body absence");
  ok &= missing(char_mesh, "decode_char_walk",
                "native must not guess CharWalk decoder");
  ok &= contains(char_mesh, "EventTriggerdecode_event_trigger(",
                 "native decodes EventTrigger only through named source slice");
  ok &= contains(char_mesh,
                 "trigger.version=source_hmx_rev(packed_rev);",
                 "EventTrigger decoder uses source low-half revision");
  ok &= contains(char_mesh,
                 "trigger.alt_version=source_alt_rev(packed_rev);",
                 "EventTrigger decoder uses source high-half revision");
  ok &= contains(char_mesh,
                 "trigger.trigger_events=read_symbol_vector(r);",
                 "EventTrigger decoder reads source trigger event vector");
  ok &= contains(char_mesh,
                 "trigger.anims=read_event_trigger_anims(r,trigger.version);",
                 "EventTrigger decoder reads source Anim ObjVector shape");
  ok &= contains(char_mesh,
                 "trigger.sounds=read_obj_ptr_list(r);"
                 "trigger.shows=read_obj_ptr_list(r);",
                 "EventTrigger decoder reads source ObjPtrList names");
  ok &= contains(char_mesh,
                 "trigger.unread_tail_hex=hex_bytes(",
                 "EventTrigger decoder logs unexplained tail bytes");
  ok &= contains(char_mesh,
                 "elseif(de.type==\"EventTrigger\"){"
                 "out.event_triggers.push_back(decode_event_trigger(de.name,b));"
                 "}",
                 "native character graph stores passive EventTrigger inventory");
  ok &= contains(char_mesh_h, "std::vector<EventTrigger>event_triggers;",
                 "native header exposes passive EventTrigger inventory");
  ok &= contains(bind_audit, "eventTrigger=%zu",
                 "bind audit summary reports EventTrigger row count");
  ok &= contains(bind_audit,
                 "[controller-event-trigger]char=%sname=%sversion=%d",
                 "bind audit logs EventTrigger source rows");
  ok &= contains(bind_audit, "tailHex=%s",
                 "bind audit logs EventTrigger unresolved tail");
  ok &= contains(char_mesh_h, "structObjectRow{",
                 "native header exposes passive generic Object inventory row");
  ok &= contains(char_mesh_h, "std::vector<ObjectRow>object_rows;",
                 "native header stores passive generic Object inventory");
  ok &= contains(char_mesh, "ObjectRowdecode_object_row(",
                 "native decodes generic Object rows through named source slice");
  ok &= contains(char_mesh,
                 "constObjectFieldRowsfields=read_object_row_fields(r);",
                 "generic Object decoder uses isolated ObjectFields row reader");
  ok &= contains(char_mesh,
                 "out.object_rows.push_back(decode_object_row(de.name,b));",
                 "native character graph stores passive generic Object inventory");
  ok &= contains(bind_audit,
                 "[object-row]char=%sname=%sversion=%daltVersion=%d",
                 "bind audit logs generic Object source rows");
  ok &= contains(bind_audit, "unreadBytes=%zu",
                 "bind audit logs generic Object unread byte count");
  ok &= missing(char_mesh, "decode_outfit_loader",
                "native must not guess OutfitLoader decoder");
  ok &= missing(char_mesh, "decode_world_fx",
                "native must not guess WorldFx decoder");
  ok &= contains(char_mesh_h, "structRndTex{",
                 "native header exposes passive RndTex inventory row");
  ok &= contains(char_mesh_h, "std::vector<RndTex>tex_rows;",
                 "native header stores passive RndTex inventory");
  ok &= contains(char_mesh, "RndTexdecode_rnd_tex(",
                 "native decodes RndTex only through named source slice");
  ok &= contains(char_mesh,
                 "tex.version=source_hmx_rev(packed_rev);",
                 "RndTex decoder uses source low-half revision");
  ok &= contains(char_mesh,
                 "tex.alt_version=source_alt_rev(packed_rev);",
                 "RndTex decoder uses source high-half revision");
  ok &= contains(char_mesh,
                 "if(tex.version>8)read_object_fields(r);",
                 "RndTex decoder gates object fields like source");
  ok &= contains(char_mesh,
                 "tex.power_of_two=source_power_of_two(tex.width,tex.height);",
                 "RndTex decoder mirrors SetPowerOf2 state");
  ok &= contains(char_mesh, "tex.filepath=r.str();",
                 "RndTex decoder reads FilePath as source string payload");
  ok &= contains(char_mesh,
                 "if(tex.version<5){tex.cubemap_mask=r.i32();",
                 "RndTex decoder reads legacy cubemap mask");
  ok &= contains(char_mesh,
                 "if(tex.version>7){tex.mip_map_k=r.f32();}",
                 "RndTex decoder reads source mipMapK gate");
  ok &= contains(char_mesh,
                 "if(tex.version>6){tex.type=r.i32();}",
                 "RndTex decoder reads source type gate");
  ok &= contains(char_mesh,
                 "tex.optimize_for_ps3=r.u8()!=0;",
                 "RndTex decoder reads source PS3 optimize flag");
  ok &= contains(char_mesh,
                 "tex.cached_bitmap_bytes=r.n-r.pos;",
                 "RndTex decoder records cached bitmap boundary");
  ok &= contains(char_mesh,
                 "tex.bitmap_version=bitmap.u8();"
                 "tex.bitmap_bpp=bitmap.u8();",
                 "RndTex decoder reads source bitmap header prefix");
  ok &= contains(char_mesh,
                 "tex.bitmap_mip_count=bitmap.u8();"
                 "tex.bitmap_width=bitmap.u16();"
                 "tex.bitmap_height=bitmap.u16();"
                 "tex.bitmap_row_bytes=bitmap.u16();",
                 "RndTex decoder reads source bitmap header dimensions");
  ok &= contains(char_mesh,
                 "bitmap.skip(tex.bitmap_version!=0?0x13:6);",
                 "RndTex decoder skips source bitmap header padding");
  ok &= contains(char_mesh,
                 "size_tsource_bitmap_palette_bytes(int32_tbpp,uint32_torder)",
                 "RndTex decoder has source palette byte helper");
  ok &= contains(char_mesh,
                 "returnstatic_cast<size_t>(1u<<bpp)*4u;",
                 "RndTex decoder mirrors source palette byte size");
  ok &= contains(char_mesh,
                 "tex.bitmap_base_pixel_bytes=static_cast<size_t>"
                 "(tex.bitmap_row_bytes)*static_cast<size_t>"
                 "(tex.bitmap_height);",
                 "RndTex decoder mirrors base bitmap pixel byte size");
  ok &= contains(char_mesh,
                 "size_tsource_bitmap_row_bytes_for_width(int32_twidth,"
                 "int32_tbpp)",
                 "RndTex decoder has source mip row-byte helper");
  ok &= contains(char_mesh,
                 "returnstatic_cast<size_t>(bpp)*static_cast<size_t>(width)/8u;",
                 "RndTex decoder mirrors source row-byte relation");
  ok &= contains(char_mesh,
                 "size_tsource_bitmap_mip_pixel_bytes(int32_twidth,"
                 "int32_theight,int32_tbpp,int32_tmip_count)",
                 "RndTex decoder has source mip payload helper");
  ok &= contains(char_mesh,
                 "mip_width>>=1;mip_height>>=1;",
                 "RndTex decoder mirrors source mip dimension loop");
  ok &= contains(char_mesh,
                 "tex.bitmap_mip_pixel_bytes=source_bitmap_mip_pixel_bytes(",
                 "RndTex decoder records source mip payload bytes");
  ok &= contains(char_mesh,
                 "tex.bitmap_expected_payload_bytes=tex.bitmap_palette_bytes+"
                 "tex.bitmap_base_pixel_bytes+tex.bitmap_mip_pixel_bytes;",
                 "RndTex decoder computes source payload byte count");
  ok &= contains(char_mesh,
                 "tex.bitmap_payload_size_matches=tex.bitmap_expected_payload_bytes=="
                 "tex.cached_bitmap_payload_bytes;",
                 "RndTex decoder verifies cached bitmap payload size");
  ok &= contains(char_mesh,
                 "out.tex_rows.push_back(decode_rnd_tex(de.name,b));",
                 "native character graph stores passive RndTex inventory");
  ok &= contains(bind_audit,
                 "[tex-row]char=%sname=%sversion=%daltVersion=%d",
                 "bind audit logs RndTex source rows");
  ok &= contains(bind_audit, "cachedBitmapBytes=%zu",
                 "bind audit logs cached bitmap payload boundary");
  ok &= contains(bind_audit, "bitmapHeader=%dbitmapVer=%dbitmapBpp=%d",
                 "bind audit logs RndBitmap header fields");
  ok &= contains(bind_audit,
                 "bitmapPaletteBytes=%zubitmapBasePixelBytes=%zu",
                 "bind audit logs RndBitmap payload source sizes");
  ok &= contains(bind_audit,
                 "bitmapMipPixelBytes=%zubitmapExpectedPayloadBytes=%zu",
                 "bind audit logs RndBitmap mip payload source sizes");
  ok &= contains(bind_audit,
                 "cachedBitmapPayloadBytes=%zupayloadSizeMatch=%d",
                 "bind audit logs RndBitmap payload size validation");
  ok &= contains(bind_audit, "payloadHexPrefix=%sbitmapHeaderError=%s",
                 "bind audit logs cached bitmap payload prefix");
  ok &= missing(char_mesh, "OutfitLoader",
                "native character graph must not promote OutfitLoader yet");
  ok &= missing(char_mesh, "WorldFx",
                "native character graph must not promote WorldFx yet");
  ok &= contains(rb3_latest_char_weight_setter_h,
                 "ObjPtr<CharDriver,ObjectDir>mDriver;",
                 "latest CharWeightSetter source header exposes driver");
  ok &= contains(rb3_latest_char_weight_setter_h,
                 "intmFlags;",
                 "latest CharWeightSetter source header exposes flags");
  ok &= contains(rb3_latest_char_weight_setter_h,
                 "floatmOffset;",
                 "latest CharWeightSetter source header exposes offset");
  ok &= contains(rb3_latest_char_weight_setter_h,
                 "floatmScale;",
                 "latest CharWeightSetter source header exposes scale");
  ok &= contains(rb3_latest_char_weight_setter_h,
                 "floatmBaseWeight;",
                 "latest CharWeightSetter source header exposes base weight");
  ok &= contains(rb3_latest_char_weight_setter_h,
                 "floatmBeatsPerWeight;",
                 "latest CharWeightSetter source header exposes beat smoothing");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(9,0)",
                 "CharWeightSetter source enforces revision ceiling");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "if(gRev>1)LOAD_SUPERCLASS(CharWeightable)bs>>mDriver;"
                 "bs>>mFlags;",
                 "CharWeightSetter source load reads weightable, driver, flags");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "if(gRev<3){mScale=1.0f;mOffset=0.0f;}",
                 "CharWeightSetter source load gates default scale and offset");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "if(gRev>4){bs>>mBaseWeight;bs>>mBeatsPerWeight;}",
                 "CharWeightSetter source load gates base weight and smoothing");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "if(gRev>5)bs>>mBase;",
                 "CharWeightSetter source load gates base pointer");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "if(gRev>8){bs>>mMinWeights;bs>>mMaxWeights;}",
                 "CharWeightSetter source load gates min/max lists");
  ok &= contains(rb3_latest_char_weight_setter_cpp,
                 "mBaseWeight=mScale*mDriver->EvaluateFlags(mFlags)+mOffset;",
                 "CharWeightSetter source poll evaluates driver flags");
  ok &= contains(char_mesh_h,
                 "structCharWeightSetter{std::stringname;int32_tversion=0;"
                 "int32_tweightable_version=0;",
                 "native CharWeightSetter stores source revisions");
  ok &= contains(char_mesh_h,
                 "std::stringweight_owner;std::stringweight_prop;",
                 "native CharWeightSetter keeps source owner plus compatibility alias");
  ok &= contains(char_mesh_h,
                 "uint32_tflags=0;uint32_tmask=0;floatoffset=0.0f;"
                 "floatscale=1.0f;",
                 "native CharWeightSetter stores source flags and scalar fields");
  ok &= contains(char_mesh,
                 "setter.version=r.i32();",
                 "native CharWeightSetter decoder reads revision");
  ok &= contains(char_mesh,
                 "if(setter.version<0||setter.version>9){"
                 "throwstd::runtime_error",
                 "native CharWeightSetter decoder enforces source revision range");
  ok &= contains(char_mesh, "read_object_fields(r);//Hmx::Objectmetadata",
                 "native CharWeightSetter decoder reads object fields");
  ok &= contains(char_mesh_h,
                 "std::vector<std::string>max_weights;size_tunread_bytes=0;",
                 "native CharWeightSetter stores source tail bytes");
  ok &= contains(char_mesh,
                 "if(setter.version>1){setter.weightable_version=r.i32();",
                 "native CharWeightSetter decoder mirrors source weightable gate");
  ok &= contains(char_mesh,
                 "if(setter.weightable_version>1)setter.weight_owner=r.str();",
                 "native CharWeightSetter decoder mirrors source weight_owner gate");
  ok &= contains(char_mesh,
                 "setter.flags=r.u32();setter.mask=setter.flags;",
                 "native CharWeightSetter decoder stores source flags and alias");
  ok &= contains(char_mesh,
                 "if(setter.version<3){setter.scale=1.0f;setter.offset=0.0f;}",
                 "native CharWeightSetter decoder mirrors default scale/offset gate");
  ok &= contains(char_mesh,
                 "if(setter.version>4){setter.base_weight=r.f32();"
                 "setter.beats_per_weight=r.f32();}",
                 "native CharWeightSetter decoder mirrors base weight gate");
  ok &= contains(char_mesh,
                 "if(setter.version>8){setter.min_weights=read_obj_ptr_list(r);"
                 "setter.max_weights=read_obj_ptr_list(r);}",
                 "native CharWeightSetter decoder mirrors min/max list gate");
  ok &= contains(char_mesh, "setter.unread_bytes=r.n-r.pos;",
                 "native CharWeightSetter decoder records source tail bytes");
  ok &= contains(bind_audit,
                 "\"[controller-weight-setter]char=%sname=%sversion=%d",
                 "controller audit logs CharWeightSetter source revision");
  ok &= contains(bind_audit,
                 "\"weightOwner=%sflags=0x%08xoffset=%.4fscale=%.4f",
                 "controller audit logs CharWeightSetter source fields");
  ok &= contains(bind_audit, "maxWeights=%zuunreadBytes=%zu",
                 "controller audit logs CharWeightSetter tail bytes");
  ok &= contains(char_clip, "weightSetter%sversion=%d",
                 "character graph logs CharWeightSetter source revision");
  ok &= contains(char_clip, "beatsPerWeight=%.3funreadBytes=%zu",
                 "character graph logs CharWeightSetter tail bytes");
  ok &= contains(doc,
                 "`CharWeightSetter::Load` reads `Hmx::Object`, then `CharWeightable`",
                 "document records CharWeightSetter source load");
  ok &= contains(doc,
                 "Native GHOGX enforces that source revision range and logs\n"
                 "    the row tail byte count",
                 "document records CharWeightSetter revision and tail boundary");
  ok &= contains(doc,
                 "source_weightsetter_20260711/stock_weightsetter_controllers"
                 ".stdout.log",
                 "document cites refreshed CharWeightSetter proof log");
  ok &= contains(doc,
                 "all 38 stock\n    `CharWeightSetter` rows are `version=2`, "
                 "use `CharWeightable` revision 2,\n    and report `unreadBytes=0`",
                 "document records refreshed CharWeightSetter stock proof");
  ok &= contains(doc,
                 "Native `source_char_weightable_default_state`,",
                 "document records native CharWeightable state helper slice");
  ok &= contains(doc,
                 "non-shallow copies own themselves while\n    copying the source owner's current weight",
                 "document records CharWeightable deep copy behavior");
  ok &= contains(doc,
                 "Native `source_char_weight_setter_poll` ports the source non-driver path",
                 "document records native CharWeightSetter source poll slice");
  ok &= contains(doc,
                 "`CharDriver::EvaluateFlags` body is available",
                 "document records CharWeightSetter driver fence");
  ok &= contains(rb3_latest_char_pos_constraint_h,
                 "ObjPtr<RndTransformable,ObjectDir>mSrc;",
                 "latest CharPosConstraint header exposes source pointer");
  ok &= contains(rb3_latest_char_pos_constraint_h,
                 "ObjPtrList<RndTransformable,ObjectDir>mTargets;",
                 "latest CharPosConstraint header exposes targets list");
  ok &= contains(rb3_latest_char_pos_constraint_h, "BoxmBox;",
                 "latest CharPosConstraint header exposes Box row");
  ok &= contains(rb3_latest_char_pos_constraint_cpp,
                 "LOAD_REVS(bs);ASSERT_REVS(2,0);Hmx::Object::Load(bs);",
                 "CharPosConstraint source load accepts revisions through 2");
  ok &= contains(rb3_latest_char_pos_constraint_cpp,
                 "bs>>mTargets;bs>>mSrc;if(gRev>1){bs>>mBox;}",
                 "CharPosConstraint source load reads targets, source, box");
  ok &= contains(rb3_latest_char_pos_constraint_cpp,
                 "mBox.Set(Vector3(1.0f,1.0f,0.0f),"
                 "Vector3(-1.0f,-1.0f,1000.0f));",
                 "CharPosConstraint source load has old-revision box default");
  ok &= contains(rb3_latest_char_pos_constraint_cpp,
                 "floattmp=Clamp(mBox.mMin.x,mBox.mMax.x,"
                 "tf48.v.x-srcTrans.v.x);",
                 "CharPosConstraint source poll clamps target/source delta");
  ok &= contains(rb3_latest_char_pos_constraint_cpp,
                 "voidCharPosConstraint::PollDeps(std::list<Hmx::Object*>&"
                 "changedBy,std::list<Hmx::Object*>&change){changedBy."
                 "push_back(mSrc);for(ObjPtrList<RndTransformable,"
                 "classObjectDir>::iteratorit=mTargets.begin();it!="
                 "mTargets.end();++it){change.push_back(*it);changedBy."
                 "push_back(*it);}}",
                 "CharPosConstraint source PollDeps publishes source and targets");
  ok &= contains(rb3_latest_char_pos_constraint_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)CREATE_COPY(CharPosConstraint)"
                 "BEGIN_COPYING_MEMBERSCOPY_MEMBER(mTargets)COPY_MEMBER(mSrc)"
                 "COPY_MEMBER(mBox)",
                 "CharPosConstraint source copy member list");
  ok &= contains(char_mesh_h,
                 "structCharPosConstraint{std::stringname;int32_tversion=0;",
                 "native CharPosConstraint stores source revision");
  ok &= contains(char_mesh_h,
                 "std::vector<std::string>targets;std::stringsource;",
                 "native CharPosConstraint stores source and targets");
  ok &= contains(char_mesh_h,
                 "floatbox_min[3]={1.0f,1.0f,0.0f};"
                 "floatbox_max[3]={-1.0f,-1.0f,1000.0f};",
                 "native CharPosConstraint stores source old-revision box default");
  ok &= contains(char_mesh_h,
                 "structSourceCharPosConstraintLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;",
                 "native exposes CharPosConstraint load-plan row");
  ok &= contains(char_mesh_h,
                 "SourceCharPosConstraintLoadPlan"
                 "source_char_pos_constraint_load_plan(intrevision);",
                 "native exposes CharPosConstraint load-plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharPosConstraintCopyPlan"
                 "source_char_pos_constraint_copy_plan();",
                 "native exposes CharPosConstraint copy-plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharPosConstraintPollDepsPlan"
                 "source_char_pos_constraint_poll_deps_plan(",
                 "native exposes CharPosConstraint PollDeps helper");
  ok &= contains(char_mesh,
                 "CharPosConstraintdecode_pos_constraint("
                 "conststd::string&entry_name,conststd::vector<uint8_t>&body)",
                 "native CharPosConstraint decoder exists");
  ok &= contains(char_mesh,
                 "if(constraint.version<0||constraint.version>2){"
                 "throwstd::runtime_error(\"char_mesh:CharPosConstraint"
                 "revisionoutsidesourcerange\");}",
                 "native CharPosConstraint decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "constraint.targets=read_obj_ptr_list(r);"
                 "constraint.source=r.str();",
                 "native CharPosConstraint decoder follows source target/source order");
  ok &= contains(char_mesh,
                 "for(float&v:constraint.box_min)v=r.f32();"
                 "for(float&v:constraint.box_max)v=r.f32();",
                 "native CharPosConstraint decoder reads Box min then max");
  ok &= contains(char_mesh,
                 "out.pos_constraints.push_back(decode_pos_constraint(de.name,b));",
                 "character load stores decoded CharPosConstraint rows");
  ok &= contains(bind_audit,
                 "\"[controller-pos-constraint]char=%sname=%sversion=%d",
                 "controller audit logs CharPosConstraint source revision");
  ok &= contains(bind_audit,
                 "\"source=%ssourceExists=%dtargets=%zuboxMin=(%.4f%.4f%.4f)",
                 "controller audit logs CharPosConstraint source and box");
  ok &= contains(char_clip,
                 "\"[chargraph]posConstraint%sversion=%dsource=%s\"",
                 "character graph logs CharPosConstraint rows");
  ok &= contains(char_clip,
                 "staticvoidapply_source_pos_constraints(Character&character)",
                 "native CharPosConstraint source poll is implemented");
  ok &= contains(char_mesh_h,
                 "std::array<float,3>source_char_pos_constraint_target_position(",
                 "native exposes CharPosConstraint source helper");
  ok &= contains(char_mesh,
                 "SourceCharPosConstraintLoadPlan"
                 "source_char_pos_constraint_load_plan(intrevision){"
                 "SourceCharPosConstraintLoadPlanplan;plan.known_revision="
                 "revision>=0&&revision<=2;",
                 "native CharPosConstraint load plan records source revision gate");
  ok &= contains(char_mesh,
                 "plan.read_order={\"LOAD_REVS\",\"Hmx::Object\","
                 "\"mTargets\",\"mSrc\"};if(revision>1){"
                 "plan.read_order.push_back(\"mBox\");}",
                 "native CharPosConstraint load plan records source read order");
  ok &= contains(char_mesh,
                 "plan.branches.push_back(\"mBox.min=(1,1,0)\");"
                 "plan.branches.push_back(\"mBox.max=(-1,-1,1000)\");",
                 "native CharPosConstraint load plan records old box default");
  ok &= contains(char_mesh,
                 "SourceCharPosConstraintCopyPlansource_char_pos_constraint_"
                 "copy_plan(){SourceCharPosConstraintCopyPlanplan;"
                 "plan.copied_superclasses={\"Hmx::Object\"};",
                 "native CharPosConstraint copy plan records source superclass");
  ok &= contains(char_mesh,
                 "plan.copied_members={\"mTargets\",\"mSrc\",\"mBox\"};",
                 "native CharPosConstraint copy plan records source members");
  ok &= contains(char_mesh,
                 "SourceCharPosConstraintPollDepsPlan"
                 "source_char_pos_constraint_poll_deps_plan(",
                 "native CharPosConstraint PollDeps plan exists");
  ok &= contains(char_mesh,
                 "plan.changed_by.push_back(source);for(conststd::string&"
                 "target:targets){plan.change.push_back(target);"
                 "plan.changed_by.push_back(target);}",
                 "native CharPosConstraint PollDeps plan records source order");
  ok &= contains(char_mesh,
                 "std::array<float,3>source_char_pos_constraint_target_position("
                 "conststd::array<float,3>&source_pos,",
                 "native implements CharPosConstraint source helper");
  ok &= contains(char_mesh,
                 "constfloatdelta=std::clamp(target_pos[axis]-source_pos[axis],"
                 "box_min[axis],box_max[axis]);",
                 "native CharPosConstraint helper clamps target/source delta");
  ok &= contains(char_clip,
                 "source_char_pos_constraint_target_position("
                 "array3_from_vec(source_pos),array3_from_vec(mat_pos(target_world)),"
                 "box_min,box_max)",
                 "native controller cadence uses CharPosConstraint helper");
  ok &= contains(char_clip,
                 "character.runtime_world_overrides[target]=target_world;",
                 "native CharPosConstraint poll publishes source target world row");
  ok &= contains(char_clip,
                 "apply_source_pos_constraints(character);",
                 "native controller cadence runs CharPosConstraint poll");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_pos_constraint_source_test",
                 "CMake builds CharPosConstraint source test");
  ok &= contains(pos_constraint_source_test,
                 "source_char_pos_constraint_target_position(",
                 "focused CharPosConstraint test calls source helper");
  ok &= contains(pos_constraint_source_test,
                 "source_char_pos_constraint_load_plan(1)",
                 "focused CharPosConstraint test covers legacy load plan");
  ok &= contains(pos_constraint_source_test,
                 "source_char_pos_constraint_load_plan(2)",
                 "focused CharPosConstraint test covers modern load plan");
  ok &= contains(pos_constraint_source_test,
                 "source_char_pos_constraint_copy_plan()",
                 "focused CharPosConstraint test covers copy plan");
  ok &= contains(pos_constraint_source_test,
                 "source_char_pos_constraint_poll_deps_plan(",
                 "focused CharPosConstraint test covers PollDeps plan");
  ok &= contains(pos_constraint_source_test,
                 "decode_pos_constraint(\"bad.pos_constraint\",{3,0,0,0})",
                 "focused CharPosConstraint test covers decoder revision gate");
  ok &= contains(pos_constraint_source_test,
                 "\"old-revisionxdisabledandy/zinrange\"",
                 "focused CharPosConstraint test covers disabled old-revision axis");
  ok &= contains(pos_constraint_source_test,
                 "\"clampseachenabledaxisbytarget-sourcedelta\"",
                 "focused CharPosConstraint test covers clamp axes");
  ok &= contains(doc,
                 "`CharPosConstraint::Load` accepts source revisions through 2",
                 "document records CharPosConstraint source load");
  ok &= contains(doc,
                 "Native `source_char_pos_constraint_load_plan` records that read order",
                 "document records CharPosConstraint load plan");
  ok &= contains(doc,
                 "`source_char_pos_constraint_copy_plan` and",
                 "document records CharPosConstraint copy/PollDeps plans");
  ok &= contains(doc,
                 "Native GHOGX ports this `Poll` path directly",
                 "document records CharPosConstraint runtime writeback");
  ok &= contains(doc,
                 "`source_char_pos_constraint_target_position` helper",
                 "document records shared CharPosConstraint helper");
  ok &= contains(doc,
                 "expanded_stock_characters_controller_posconstraint_inventory.log",
                 "document cites focused stock CharPosConstraint inventory");
  ok &= contains(doc,
                 "shows five `CharPosConstraint` rows total",
                 "document records stock CharPosConstraint coverage");
  ok &= contains(doc,
                 "rows with zero decoded targets naturally produce no writes",
                 "document records zero-target CharPosConstraint boundary");
  ok &= contains(doc,
                 "Grim's `hems.pcon` names `source=grim`",
                 "document records stock Grim CharPosConstraint boundary");
  ok &= contains(rb3_latest_waypoint_h,
                 "floatmRadius;",
                 "latest Waypoint header exposes radius");
  ok &= contains(rb3_latest_waypoint_h,
                 "floatmYRadius;",
                 "latest Waypoint header exposes Y radius");
  ok &= contains(rb3_latest_waypoint_h,
                 "floatmAngRadius;",
                 "latest Waypoint header exposes angular radius");
  ok &= contains(rb3_latest_waypoint_h,
                 "floatmStrictAngDelta;",
                 "latest Waypoint header exposes strict angular delta");
  ok &= contains(rb3_latest_waypoint_h,
                 "floatmStrictRadiusDelta;",
                 "latest Waypoint header exposes strict radius delta");
  ok &= contains(rb3_latest_waypoint_h,
                 "ObjVector<ObjOwnerPtr<Waypoint,ObjectDir>>mConnections;",
                 "latest Waypoint header exposes source connections");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "Waypoint::Waypoint():mFlags(0),mRadius(12.0f),"
                 "mYRadius(0),mAngRadius(0),pad(0),mStrictAngDelta(0),"
                 "mStrictRadiusDelta(0),mConnections(this)",
                 "latest Waypoint source constructor defaults");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "ASSERT_REVS(5,0)",
                 "latest Waypoint source load accepts revisions through 5");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "bs>>mFlags;bs>>mConnections;",
                 "latest Waypoint source load reads flags and connections");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "if(gRev>1){bs>>mRadius;}elsemRadius=12;",
                 "latest Waypoint source load gates radius");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "if(gRev>2){bs>>mYRadius;bs>>mAngRadius;}",
                 "latest Waypoint source load gates shape radii");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "if(gRev>3){bs>>mStrictRadiusDelta;bs>>mStrictAngDelta;}",
                 "latest Waypoint source load gates strict deltas");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "COPY_SUPERCLASS(Hmx::Object)COPY_SUPERCLASS(RndTransformable)",
                 "latest Waypoint source copy superclasses");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "COPY_MEMBER(mFlags)COPY_MEMBER(mConnections)"
                 "COPY_MEMBER(mRadius)COPY_MEMBER(mYRadius)"
                 "COPY_MEMBER(mAngRadius)COPY_MEMBER(mStrictRadiusDelta)"
                 "COPY_MEMBER(mStrictAngDelta)",
                 "latest Waypoint source copy members");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "if(f2>0.0f){Subtract(v1,WorldXfm().v,res);",
                 "latest Waypoint ShapeDeltaBox rectangular branch");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "floatdotx=Dot(res,world.m.x);floatdoty=Dot(res,world.m.y);",
                 "latest Waypoint ShapeDeltaBox source dot products");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "Scale(world.m.x,clamped1-dotx,res);"
                 "ScaleAddEq(res,world.m.y,clamped2-doty);",
                 "latest Waypoint ShapeDeltaBox clamps world rows");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "Subtract(WorldXfm().v,v1,res);res.z=0;",
                 "latest Waypoint ShapeDeltaBox circular branch");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "if(lensq<=f1*f1)res.Zero();elseres*=1.0f-"
                 "(f1/std::sqrt(lensq));",
                 "latest Waypoint ShapeDeltaBox circular scale");
  ok &= contains(rb3_latest_waypoint_cpp,
                 "floatlimited=LimitAng(GetZAngle(WorldXfm().m)-f2);",
                 "latest Waypoint ShapeDeltaAng limited delta");
  ok &= contains(char_mesh_h,
                 "structSourceWaypointState{intflags=0;floatradius=12.0f;",
                 "native exposes Waypoint source state");
  ok &= contains(char_mesh_h,
                 "floatstrict_ang_delta=0.0f;floatstrict_radius_delta=0.0f;",
                 "native stores Waypoint strict deltas");
  ok &= contains(char_mesh_h,
                 "SourceWaypointConstrainResultsource_waypoint_constrain(",
                 "native exposes Waypoint constrain helper");
  ok &= contains(char_mesh,
                 "boolsource_waypoint_load_revision_known(intrevision){"
                 "returnrevision>=0&&revision<=5;}",
                 "native Waypoint helper mirrors source revision gate");
  ok &= contains(char_mesh,
                 "plan.copied_superclasses={\"Hmx::Object\","
                 "\"RndTransformable\"};",
                 "native Waypoint helper mirrors copy superclasses");
  ok &= contains(char_mesh,
                 "plan.copied_members={\"mFlags\",\"mConnections\","
                 "\"mRadius\",\"mYRadius\",\"mAngRadius\","
                 "\"mStrictRadiusDelta\",\"mStrictAngDelta\"};",
                 "native Waypoint helper mirrors copy members");
  ok &= contains(char_mesh,
                 "if(y_radius>0.0f){constSourceVec3from_waypoint="
                 "source_vec_sub(p,waypoint_pos);",
                 "native Waypoint helper ports rectangular branch");
  ok &= contains(char_mesh,
                 "delta=source_vec_sub(waypoint_pos,p);delta[2]=0.0f;",
                 "native Waypoint helper ports circular branch");
  ok &= contains(char_mesh,
                 "constfloatlimited=source_limit_ang(waypoint_z_angle-"
                 "subject_z_angle);constfloatclamped=std::clamp(limited,"
                 "-radius,radius);returnlimited-clamped;",
                 "native Waypoint helper ports ShapeDeltaAng");
  ok &= contains(char_mesh,
                 "if(waypoint.strict_radius_delta>0.0f){",
                 "native Waypoint constrain applies strict radius only");
  ok &= contains(char_mesh,
                 "source_rotate_about_z(result.constrained,result.angle_delta);",
                 "native Waypoint constrain applies source angle delta");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_waypoint_source_test",
                 "CMake builds Waypoint source test");
  ok &= contains(waypoint_source_test,
                 "source_waypoint_shape_delta_box(",
                 "focused Waypoint test covers shape delta box");
  ok &= contains(waypoint_source_test,
                 "source_waypoint_shape_delta_ang(",
                 "focused Waypoint test covers shape delta angle");
  ok &= contains(waypoint_source_test,
                 "source_waypoint_constrain(",
                 "focused Waypoint test covers constrain helper");
  ok &= contains(doc,
                 "## Waypoint Clip/Path Diagnostic Authorities",
                 "document records Waypoint source authority section");
  ok &= contains(doc,
                 "Native helpers are source-only deterministic diagnostics",
                 "document fences Waypoint helper from live path behavior");
  ok &= contains(rb3_latest_char_bone_offset_h,
                 "ObjPtr<RndTransformable,ObjectDir>mDest;",
                 "latest CharBoneOffset header exposes destination pointer");
  ok &= contains(rb3_latest_char_bone_offset_h,
                 "Vector3mOffset;",
                 "latest CharBoneOffset header exposes offset vector");
  ok &= contains(rb3_latest_char_bone_offset_cpp,
                 "ASSERT_REVS(1,0);Hmx::Object::Load(bs);bs>>mDest;bs>>mOffset;",
                 "CharBoneOffset source load reads object, dest, and offset");
  ok &= contains(rb3_latest_char_bone_offset_cpp,
                 "if(!mDest||!mDest->TransParent())return;",
                 "CharBoneOffset source poll returns when dest or parent is missing");
  ok &= contains(rb3_latest_char_bone_offset_cpp,
                 "Transformtf(mDest->LocalXfm());tf.v+=mOffset;TransformtRes;"
                 "Multiply(tf,mDest->TransParent()->WorldXfm(),tRes);"
                 "mDest->SetWorldXfm(tRes);",
                 "CharBoneOffset source poll offsets local then multiplies parent world");
  ok &= contains(rb3_latest_char_bone_offset_cpp,
                 "if(mDest)mDest->DirtyLocalXfm().v+=mOffset;",
                 "CharBoneOffset source ApplyToLocal offsets local row");
  ok &= contains(char_mesh_h,
                 "structCharBoneOffset{std::stringname;int32_tversion=0;"
                 "std::stringdest;floatoffset[3]={0.0f,0.0f,0.0f};",
                 "native CharBoneOffset stores source fields");
  ok &= contains(char_mesh_h,
                 "std::vector<CharBoneOffset>bone_offsets;",
                 "native Character stores decoded CharBoneOffset rows");
  ok &= contains(char_mesh,
                 "CharBoneOffsetdecode_bone_offset("
                 "conststd::string&entry_name,conststd::vector<uint8_t>&body)",
                 "native CharBoneOffset decoder exists");
  ok &= contains(char_mesh,
                 "offset.version=r.i32();if(offset.version<0||"
                 "offset.version>1)",
                 "native CharBoneOffset decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "read_object_fields(r);offset.dest=r.str();"
                 "for(float&v:offset.offset)v=r.f32();",
                 "native CharBoneOffset decoder follows source field order");
  ok &= contains(char_mesh,
                 "out.bone_offsets.push_back(decode_bone_offset(de.name,b));",
                 "character load stores decoded CharBoneOffset rows");
  ok &= contains(bind_audit,
                 "\"[controller-bone-offset]char=%sname=%sversion=%ddest=%s",
                 "controller audit logs CharBoneOffset rows");
  ok &= contains(bind_audit,
                 "boneOffset=%zuboneTwist=%zuanimFilter=%zueventTrigger=%zu\\n",
                 "controller summary logs CharBoneOffset row count");
  ok &= contains(char_clip_h,
                 "boolsource_char_bone_offset_poll_world("
                 "constCharBoneOffset&offset,boolhas_dest,boolhas_parent,",
                 "native header exposes CharBoneOffset Poll helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bone_offset_apply_to_local("
                 "constCharBoneOffset&offset,milo_scene::Xfm&dest_local);",
                 "native header exposes CharBoneOffset ApplyToLocal helper");
  ok &= contains(char_clip,
                 "if(!has_dest||!has_parent)returnfalse;",
                 "native CharBoneOffset helper mirrors source early return");
  ok &= contains(char_clip,
                 "local.pos[0]+=offset.offset[0];local.pos[1]+="
                 "offset.offset[1];local.pos[2]+=offset.offset[2];",
                 "native CharBoneOffset helper offsets local translation");
  ok &= contains(char_clip,
                 "dest_world=mat4_mul(source_xfm_to_mat4(local),parent_world);",
                 "native CharBoneOffset helper multiplies adjusted local by parent world");
  ok &= contains(char_clip,
                 "\"[chargraph]boneOffset%sversion=%ddest=%s\"",
                 "character graph logs CharBoneOffset rows");
  ok &= contains(bone_offset_source_test,
                 "source_char_bone_offset_poll_world(",
                 "focused CharBoneOffset source test covers Poll helper");
  ok &= contains(bone_offset_source_test,
                 "source_char_bone_offset_apply_to_local(offset,local);",
                 "focused CharBoneOffset source test covers ApplyToLocal helper");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharBoneOffset.cpp` and",
                 "document cites latest CharBoneOffset source");
  ok &= contains(doc,
                 "`CharBoneOffset::Poll` returns immediately",
                 "document records CharBoneOffset Poll boundary");
  ok &= contains(doc,
                 "`source_char_bone_offset_poll_world`",
                 "document records native CharBoneOffset helper port");
  ok &= contains(rb3_latest_char_bone_twist_h,
                 "ObjPtr<RndTransformable,ObjectDir>mBone;",
                 "latest CharBoneTwist header exposes driven bone pointer");
  ok &= contains(rb3_latest_char_bone_twist_h,
                 "ObjPtrList<RndTransformable,ObjectDir>mTargets;",
                 "latest CharBoneTwist header exposes target list");
  ok &= contains(rb3_latest_char_bone_twist_cpp,
                 "ASSERT_REVS(0,0);Hmx::Object::Load(bs);"
                 "CharWeightable::Load(bs);bs>>mBone;bs>>mTargets;",
                 "CharBoneTwist source load reads object, weightable, bone, and targets");
  ok &= contains(rb3_latest_char_bone_twist_cpp,
                 "if(!mBone||mTargets.size()==0)return;",
                 "CharBoneTwist source poll returns when bone or targets are missing");
  ok &= contains(rb3_latest_char_bone_twist_cpp,
                 "Vector3v58;v58.Zero();for(ObjPtrList<RndTransformable,"
                 "ObjectDir>::iteratorit=mTargets.begin();it!=mTargets.end();++it){"
                 "Vector3v64((*it)->WorldXfm().v);Add(v64,v58,v58);}",
                 "CharBoneTwist source poll averages target positions");
  ok &= contains(rb3_latest_char_bone_twist_cpp,
                 "Subtract(v58,mBone->WorldXfm().v,v70);",
                 "CharBoneTwist source poll computes target direction");
  ok &= contains(rb3_latest_char_bone_twist_cpp,
                 "Scale(tf48.m.x,Dot(tf48.m.x,v70),v7c);"
                 "Subtract(v70,v7c,v7c);Normalize(v7c,v7c);",
                 "CharBoneTwist source poll removes X-axis component");
  ok &= contains(rb3_latest_char_bone_twist_cpp,
                 "Interp(tf48.m.y,v7c,Weight(),tf48.m.y);"
                 "Normalize(tf48.m.y,tf48.m.y);Cross(tf48.m.x,tf48.m.y,tf48.m.z);"
                 "Normalize(tf48.m.z,tf48.m.z);Scale(tf48.m.z,Length(tf48.m.x),tf48.m.z);"
                 "mBone->SetWorldXfm(tf48);",
                 "CharBoneTwist source poll interpolates Y and rebuilds Z");
  ok &= contains(char_mesh_h,
                 "structCharBoneTwist{std::stringname;int32_tversion=0;"
                 "int32_tweightable_version=0;floatweight=1.0f;",
                 "native CharBoneTwist stores source weightable fields");
  ok &= contains(char_mesh_h,
                 "std::stringbone;std::vector<std::string>targets;",
                 "native CharBoneTwist stores bone and target list");
  ok &= contains(char_mesh_h,
                 "std::vector<CharBoneTwist>bone_twists;",
                 "native Character stores decoded CharBoneTwist rows");
  ok &= contains(char_mesh,
                 "CharBoneTwistdecode_bone_twist("
                 "conststd::string&entry_name,conststd::vector<uint8_t>&body)",
                 "native CharBoneTwist decoder exists");
  ok &= contains(char_mesh,
                 "twist.version=r.i32();if(twist.version!=0)",
                 "native CharBoneTwist decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "read_object_fields(r);twist.weightable_version=r.i32();",
                 "native CharBoneTwist decoder reads object and weightable revision");
  ok &= contains(char_mesh,
                 "twist.weight=r.f32();if(twist.weightable_version>1)"
                 "twist.weight_owner=r.str();twist.bone=r.str();"
                 "twist.targets=read_obj_ptr_list(r);",
                 "native CharBoneTwist decoder follows source field order");
  ok &= contains(char_mesh,
                 "out.bone_twists.push_back(decode_bone_twist(de.name,b));",
                 "character load stores decoded CharBoneTwist rows");
  ok &= contains(bind_audit,
                 "boneOffset=%zuboneTwist=%zuanimFilter=%zueventTrigger=%zu\\n",
                 "controller summary logs CharBoneTwist row count");
  ok &= contains(bind_audit,
                 "\"[controller-bone-twist]char=%sname=%sversion=%d",
                 "controller audit logs CharBoneTwist rows");
  ok &= contains(char_clip_h,
                 "floatsource_char_bone_twist_weight("
                 "constCharBoneTwist&twist,",
                 "native header exposes CharBoneTwist weight helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_bone_twist_poll_world("
                 "constCharBoneTwist&twist,boolhas_bone,",
                 "native header exposes CharBoneTwist Poll helper");
  ok &= contains(char_clip,
                 "if(!twist.weight_owner.empty()){constautoowner="
                 "weights_by_name.find(twist.weight_owner);"
                 "if(owner!=weights_by_name.end())returnowner->second;}returntwist.weight;",
                 "native CharBoneTwist weight helper mirrors source owner fallback");
  ok &= contains(char_clip,
                 "if(!has_bone||target_worlds.empty())returnfalse;",
                 "native CharBoneTwist helper mirrors source early return");
  ok &= contains(char_clip,
                 "avg=vadd(avg,mat_pos(target_world));",
                 "native CharBoneTwist helper averages target positions");
  ok &= contains(char_clip,
                 "constVec3projected_x=vscale(x,vdot(x,to_targets));"
                 "constVec3target_y=vnorm(vsub(to_targets,projected_x),old_y);",
                 "native CharBoneTwist helper removes X-axis component");
  ok &= contains(char_clip,
                 "Vec3y=vnorm(vadd(vscale(old_y,1.0f-weight),"
                 "vscale(target_y,weight)),old_y);Vec3z=vscale("
                 "vnorm(vcross(x,y),old_z),vlen(x));",
                 "native CharBoneTwist helper interpolates Y and rebuilds Z");
  ok &= contains(char_clip,
                 "\"[chargraph]boneTwist%sversion=%dbone=%stargets=%zu",
                 "character graph logs CharBoneTwist rows");
  ok &= contains(bone_twist_source_test,
                 "source_char_bone_twist_poll_world(",
                 "focused CharBoneTwist source test covers Poll helper");
  ok &= contains(bone_twist_source_test,
                 "source_char_bone_twist_weight(twist,weights)",
                 "focused CharBoneTwist source test covers weight helper");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharBoneTwist.cpp` and",
                 "document cites latest CharBoneTwist source");
  ok &= contains(doc,
                 "`CharBoneTwist::Poll` returns when the driven bone is missing",
                 "document records CharBoneTwist Poll boundary");
  ok &= contains(doc,
                 "`source_char_bone_twist_weight` and",
                 "document records native CharBoneTwist helper ports");
  ok &= contains(rb3_char_lookat_cpp, "mPivot->SetWorldXfm(tf90);",
                 "RB3 CharLookAt poll writes the pivot transform");
  ok &= contains(rb3_char_lookat_cpp, "RndTransformable*srcTrans=GetSource();",
                 "RB3 CharLookAt poll resolves source through GetSource");
  ok &= contains(rb3_latest_char_lookat_h,
                 "RndTransformable*GetSource()const{constObjPtr<"
                 "RndTransformable,ObjectDir>&ptr=mSource?mSource:mPivot;"
                 "returnptr;}",
                 "latest CharLookAt GetSource falls back to pivot");
  ok &= contains(rb3_char_lookat_cpp,
                 "voidCharLookAt::Enter(){unk6c.Set(1e+29f,0.0f,0.0f);"
                 "if(mPivot){mPivot->DirtyLocalXfm().m.Identity();}"
                 "RndPollable::Enter();}",
                 "RB3 CharLookAt Enter resets smoothed row and pivot local");
  ok &= contains(rb3_char_lookat_cpp,
                 "voidCharLookAt::PollDeps(std::list<Hmx::Object*>&changedBy,"
                 "std::list<Hmx::Object*>&change){changedBy.push_back("
                 "GetSource());changedBy.push_back(mDest);change.push_back("
                 "mPivot);}",
                 "RB3 CharLookAt PollDeps publishes source dest pivot");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(mDest&&mPivot){if(mPivot->TransParent()&&srcTrans&&"
                 "deltasecs>=0.0f){",
                 "RB3 CharLookAt Poll gates on dest pivot parent source and delta");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(mMinWeightYaw>=0.0f){",
                 "RB3 CharLookAt Poll gates yaw weighting on min weight yaw");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(charweight!=0.0f){",
                 "RB3 CharLookAt Poll skips transform branch at zero weight");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(mSourceRadius>0.0f){if(TheTaskMgr.DeltaSeconds()>0.0f){",
                 "RB3 CharLookAt Poll gates source-radius history on positive delta");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(srcTrans!=mPivot){Transformtf90(mPivot->WorldXfm());",
                 "RB3 CharLookAt Poll separates source and pivot transform path");
  ok &= contains(rb3_char_lookat_cpp,
                 "elseNormalize(ve4,ve4);",
                 "RB3 CharLookAt Poll normalizes directly when source is pivot");
  ok &= contains(rb3_char_lookat_cpp,
                 "unkb1=mBounds.Clamp(ve4);",
                 "RB3 CharLookAt Poll clamps through bounds");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(unk6c.x!=1e+29f&&mHalfTime!=0.0f){",
                 "RB3 CharLookAt Poll gates half-time smoothing");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(mTestRange){",
                 "RB3 CharLookAt Poll gates test range before show range");
  ok &= contains(rb3_char_lookat_cpp,
                 "elseif(mShowRange){",
                 "RB3 CharLookAt Poll gates show range after test range");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(mEnableJitter&&!sDisableJitter&&!disable&&deltasecs>0.0f){",
                 "RB3 CharLookAt Poll gates jitter");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(mAllowRoll){",
                 "RB3 CharLookAt Poll gates roll branch");
  ok &= contains(rb3_char_lookat_cpp,
                 "ClampEq(mMinYaw,-80.0f,80.0f);"
                 "ClampEq(mMaxYaw,-80.0f,80.0f);"
                 "ClampEq(mMinPitch,-80.0f,80.0f);"
                 "ClampEq(mMaxPitch,-80.0f,80.0f);",
                 "RB3 CharLookAt SyncLimits clamps yaw and pitch");
  ok &= contains(rb3_char_lookat_cpp,
                 "mBounds.mMin.y=cos(max_overall*DEG2RAD);"
                 "mBounds.mMax.y=1.0E+29f;",
                 "RB3 CharLookAt SyncLimits computes Y bounds");
  ok &= contains(rb3_char_lookat_cpp,
                 "mBounds.mMin.z=mBounds.mMin.y*tan(mMinYaw*DEG2RAD);"
                 "mBounds.mMax.z=mBounds.mMin.y*tan(mMaxYaw*DEG2RAD);"
                 "mBounds.mMin.x=mBounds.mMin.y*tan(mMinPitch*DEG2RAD);"
                 "mBounds.mMax.x=mBounds.mMin.y*tan(mMaxPitch*DEG2RAD);",
                 "RB3 CharLookAt SyncLimits computes yaw and pitch bounds");
  ok &= contains(rb3_char_lookat_cpp, "LOAD_REVS(bs)ASSERT_REVS(5,0)",
                 "RB3 CharLookAt source enforces revision ceiling");
  ok &= contains(rb3_char_lookat_cpp,
                 "LOAD_SUPERCLASS(Hmx::Object)LOAD_SUPERCLASS(CharWeightable)"
                 "bs>>mSource;bs>>mPivot;bs>>mDest;bs>>mHalfTime;"
                 "bs>>mMinYaw;bs>>mMaxYaw;bs>>mMinPitch;bs>>mMaxPitch;",
                 "RB3 CharLookAt source load reads object, weightable, and core rows");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(gRev>1){bs>>mMinWeightYaw;bs>>mMaxWeightYaw;"
                 "bs>>mWeightYawSpeed;}if(gRev<3)mAllowRoll=true;else"
                 "bs>>mAllowRoll;",
                 "RB3 CharLookAt source load gates yaw-weight and allow-roll rows");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(gRev<4){mEnableJitter=false;mPitchJitterLimit=0;"
                 "mYawJitterLimit=0;}else{bs>>mEnableJitter;"
                 "bs>>mPitchJitterLimit;bs>>mYawJitterLimit;}",
                 "RB3 CharLookAt source load gates jitter rows");
  ok &= contains(rb3_char_lookat_cpp,
                 "if(gRev>4)bs>>mSourceRadius;SyncLimits();",
                 "RB3 CharLookAt source load gates source radius and syncs limits");
  ok &= contains(rb3_char_lookat_cpp,
                 "BEGIN_COPYS(CharLookAt)COPY_SUPERCLASS(Hmx::Object)"
                 "COPY_SUPERCLASS(CharWeightable)CREATE_COPY(CharLookAt)"
                 "BEGIN_COPYING_MEMBERSCOPY_MEMBER(mSource)COPY_MEMBER(mPivot)"
                 "COPY_MEMBER(mDest)COPY_MEMBER(mHalfTime)",
                 "RB3 CharLookAt source copy starts with source fields");
  ok &= contains(rb3_char_lookat_cpp,
                 "COPY_MEMBER(mMinWeightYaw)COPY_MEMBER(mMaxWeightYaw)"
                 "COPY_MEMBER(mWeightYawSpeed)COPY_MEMBER(mAllowRoll)"
                 "COPY_MEMBER(mSourceRadius)COPY_MEMBER(mEnableJitter)"
                 "COPY_MEMBER(mYawJitterLimit)COPY_MEMBER(mPitchJitterLimit)"
                 "END_COPYING_MEMBERSSyncLimits();",
                 "RB3 CharLookAt source copy gates copied limits and syncs limits");
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "LOAD_REVS(bs);ASSERT_REVS(2,0);bs>>mWeight;",
                 "latest CharWeightable source enforces revision ceiling");
  ok &= contains(char_mesh,
                 "if(la.version<0||la.version>5){"
                 "throwstd::runtime_error",
                 "native CharLookAt decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "if(la.weightable_version<0||la.weightable_version>2){"
                 "throwstd::runtime_error",
                 "native CharLookAt decoder enforces CharWeightable source range");
  ok &= contains(char_mesh,
                 "la.weightable_version=r.i32();if(la.weightable_version<0||"
                 "la.weightable_version>2){throwstd::runtime_error("
                 "\"char_mesh:CharLookAtCharWeightablerevisionoutside"
                 "sourcerange\");}la.weight=r.f32();"
                 "if(la.weightable_version>1)la.weight_owner=r.str();"
                 "la.source=r.str();la.pivot=r.str();la.dest=r.str();",
                 "native CharLookAt decoder follows source Weightable/source/pivot/dest order");
  ok &= contains(char_mesh,
                 "la.half_time=r.f32();la.min_yaw=r.f32();"
                 "la.max_yaw=r.f32();la.min_pitch=r.f32();"
                 "la.max_pitch=r.f32();",
                 "native CharLookAt decoder follows source timing and limit order");
  ok &= contains(char_clip_h,
                 "structSourceCharLookAtBounds{std::array<float,3>min",
                 "native header exposes CharLookAt bounds struct");
  ok &= contains(char_clip_h,
                 "structSourceCharLookAtEnterState{std::array<float,3>"
                 "smoothed_dir={1.0e29f,0.0f,0.0f};"
                 "boolreset_pivot_local=false;};",
                 "native header exposes CharLookAt Enter state");
  ok &= contains(char_clip_h,
                 "structSourceCharLookAtPollDeps{std::vector<std::string>"
                 "changed_by;std::vector<std::string>change;};",
                 "native header exposes CharLookAt PollDeps state");
  ok &= contains(char_clip_h,
                 "structSourceCharLookAtPollPlan{boolpoll_gate_open=false;"
                 "boolcompute_dest_vector=false;boolapply_weight_yaw=false;",
                 "native header exposes CharLookAt Poll plan state");
  ok &= contains(char_clip_h,
                 "boolwrite_pivot_world_to_source=false;boolnormalize_dest_vector=false;",
                 "native header exposes CharLookAt source/pivot branch flags");
  ok &= contains(char_clip_h,
                 "boolwrite_roll_local_rotation=false;boolwrite_no_roll_axes=false;};",
                 "native header exposes CharLookAt roll branch flags");
  ok &= contains(char_clip_h,
                 "structSourceCharLookAtLoadPlan{boolrevision_supported=false;"
                 "std::vector<std::string>read_order;std::vector<std::string>"
                 "branches;boolsync_limits=false;};",
                 "native header exposes CharLookAt load plan");
  ok &= contains(char_clip_h,
                 "structSourceCharLookAtCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;"
                 "boolsync_limits=false;};",
                 "native header exposes CharLookAt copy plan");
  ok &= contains(char_clip_h,
                 "SourceCharLookAtBoundssource_char_lookat_sync_limits("
                 "floatmin_yaw,floatmax_yaw,floatmin_pitch,floatmax_pitch);",
                 "native header exposes CharLookAt SyncLimits helper");
  ok &= contains(char_clip_h,
                 "SourceCharLookAtLoadPlansource_char_lookat_load_plan("
                 "int32_trevision);",
                 "native header exposes CharLookAt load plan helper");
  ok &= contains(char_clip_h,
                 "SourceCharLookAtCopyPlansource_char_lookat_copy_plan();",
                 "native header exposes CharLookAt copy plan helper");
  ok &= contains(char_clip_h,
                 "SourceCharLookAtEnterStatesource_char_lookat_enter("
                 "boolhas_pivot);",
                 "native header exposes CharLookAt Enter helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_lookat_poll_deps("
                 "SourceCharLookAtPollDeps&deps,conststd::string&source,"
                 "conststd::string&pivot,conststd::string&dest);",
                 "native header exposes CharLookAt PollDeps helper");
  ok &= contains(char_clip_h,
                 "SourceCharLookAtPollPlansource_char_lookat_poll_plan("
                 "boolhas_resolved_source,boolhas_pivot,boolhas_dest,"
                 "boolhas_pivot_parent,floatdelta_seconds,floatweight,",
                 "native header exposes CharLookAt Poll plan helper");
  ok &= contains(char_clip,
                 "min_yaw=std::clamp(min_yaw,-80.0f,80.0f);"
                 "max_yaw=std::clamp(max_yaw,-80.0f,80.0f);"
                 "min_pitch=std::clamp(min_pitch,-80.0f,80.0f);"
                 "max_pitch=std::clamp(max_pitch,-80.0f,80.0f);",
                 "native CharLookAt SyncLimits helper clamps source limits");
  ok &= contains(char_clip,
                 "constfloatmin_y=std::cos(max_overall*kDegToRad);",
                 "native CharLookAt SyncLimits helper computes source min Y");
  ok &= contains(char_clip,
                 "bounds.min[2]=min_y*std::tan(min_yaw*kDegToRad);"
                 "bounds.max[2]=min_y*std::tan(max_yaw*kDegToRad);"
                 "bounds.min[0]=min_y*std::tan(min_pitch*kDegToRad);"
                 "bounds.max[0]=min_y*std::tan(max_pitch*kDegToRad);",
                 "native CharLookAt SyncLimits helper computes source axis bounds");
  ok &= contains(char_clip,
                 "SourceCharLookAtLoadPlansource_char_lookat_load_plan("
                 "int32_trevision){SourceCharLookAtLoadPlanplan;"
                 "plan.revision_supported=revision>=0&&revision<=5;",
                 "native CharLookAt load helper ports revision gate");
  ok &= contains(char_clip,
                 "plan.read_order={\"Hmx::Object\",\"CharWeightable\","
                 "\"mSource\",\"mPivot\",\"mDest\",\"mHalfTime\",",
                 "native CharLookAt load helper records core read order");
  ok &= contains(char_clip,
                 "if(revision>1){plan.read_order.push_back(\"mMinWeightYaw\");"
                 "plan.read_order.push_back(\"mMaxWeightYaw\");"
                 "plan.read_order.push_back(\"mWeightYawSpeed\");}",
                 "native CharLookAt load helper records yaw-weight branch");
  ok &= contains(char_clip,
                 "if(revision<3){plan.branches.push_back(\"mAllowRoll=true\");}"
                 "else{plan.read_order.push_back(\"mAllowRoll\");}",
                 "native CharLookAt load helper records allow-roll branch");
  ok &= contains(char_clip,
                 "if(revision<4){plan.branches.push_back(\"mEnableJitter=false\");"
                 "plan.branches.push_back(\"mPitchJitterLimit=0\");"
                 "plan.branches.push_back(\"mYawJitterLimit=0\");}",
                 "native CharLookAt load helper records jitter defaults");
  ok &= contains(char_clip,
                 "if(revision>4)plan.read_order.push_back(\"mSourceRadius\");"
                 "plan.sync_limits=true;",
                 "native CharLookAt load helper records source-radius and sync");
  ok &= contains(char_clip,
                 "SourceCharLookAtCopyPlansource_char_lookat_copy_plan(){"
                 "SourceCharLookAtCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\",\"CharWeightable\"};",
                 "native CharLookAt copy helper records superclasses");
  ok &= contains(char_clip,
                 "plan.copied_members={\"mSource\",\"mPivot\",\"mDest\","
                 "\"mHalfTime\",\"mMinYaw\",\"mMaxYaw\",\"mMinPitch\","
                 "\"mMaxPitch\",",
                 "native CharLookAt copy helper records member prefix");
  ok &= contains(char_clip,
                 "SourceCharLookAtEnterStatesource_char_lookat_enter("
                 "boolhas_pivot){SourceCharLookAtEnterStatestate;"
                 "state.smoothed_dir={1.0e29f,0.0f,0.0f};"
                 "state.reset_pivot_local=has_pivot;returnstate;}",
                 "native CharLookAt Enter helper ports source reset");
  ok &= contains(char_clip,
                 "voidsource_char_lookat_poll_deps("
                 "SourceCharLookAtPollDeps&deps,conststd::string&source,"
                 "conststd::string&pivot,conststd::string&dest){"
                 "deps.changed_by.push_back(source.empty()?pivot:source);"
                 "deps.changed_by.push_back(dest);deps.change.push_back(pivot);}",
                 "native CharLookAt PollDeps helper ports source publication");
  ok &= contains(char_clip,
                 "SourceCharLookAtPollPlansource_char_lookat_poll_plan(",
                 "native CharLookAt Poll plan helper is implemented");
  ok &= contains(char_clip,
                 "plan.poll_gate_open=has_dest&&has_pivot&&has_pivot_parent&&"
                 "has_resolved_source&&delta_seconds>=0.0f;",
                 "native CharLookAt Poll plan ports source gate");
  ok &= contains(char_clip,
                 "plan.apply_weight_yaw=min_weight_yaw>=0.0f;if(weight==0.0f){"
                 "plan.skip_zero_weight=true;returnplan;}",
                 "native CharLookAt Poll plan ports yaw and zero-weight gates");
  ok &= contains(char_clip,
                 "plan.update_source_radius_history=has_source_radius&&"
                 "delta_seconds>0.0f;",
                 "native CharLookAt Poll plan ports source-radius history gate");
  ok &= contains(char_clip,
                 "plan.write_pivot_world_to_source=!source_is_pivot;"
                 "plan.normalize_dest_vector=source_is_pivot;",
                 "native CharLookAt Poll plan ports source/pivot branch");
  ok &= contains(char_clip,
                 "plan.use_test_range=test_range;plan.use_show_range="
                 "!test_range&&show_range;",
                 "native CharLookAt Poll plan ports test/show range order");
  ok &= contains(char_clip,
                 "plan.apply_jitter=enable_jitter&&!static_disable_jitter&&"
                 "!cheat_disable_eye_jitter&&delta_seconds>0.0f;",
                 "native CharLookAt Poll plan ports jitter gate");
  ok &= contains(char_clip,
                 "plan.write_roll_local_rotation=allow_roll;"
                 "plan.write_no_roll_axes=!allow_roll;",
                 "native CharLookAt Poll plan ports roll branch gate");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_sync_limits(-80.0f,80.0f,-80.0f,80.0f)",
                 "focused CharLookAt source test covers default source limits");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_sync_limits(-120.0f,120.0f,-90.0f,90.0f)",
                 "focused CharLookAt source test covers clamped limits");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_sync_limits(-30.0f,45.0f,-10.0f,20.0f)",
                 "focused CharLookAt source test covers asymmetric limits");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_load_plan(2)",
                 "focused CharLookAt source test covers GH2 stock load plan");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_load_plan(5)",
                 "focused CharLookAt source test covers current load plan");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_copy_plan()",
                 "focused CharLookAt source test covers copy plan");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_enter(true)",
                 "focused CharLookAt source test covers Enter helper");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_poll_deps(deps,\"explicit.source\","
                 "\"pivot.lookat\",\"target.lookat\")",
                 "focused CharLookAt source test covers explicit source deps");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_poll_deps(fallback_deps,\"\","
                 "\"pivot.lookat\",\"target.lookat\")",
                 "focused CharLookAt source test covers pivot fallback deps");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_poll_plan(true,true,false,true",
                 "focused CharLookAt source test covers inert missing-dest Poll plan");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_poll_plan(true,true,true,true,1.0f,1.0f",
                 "focused CharLookAt source test covers roll Poll plan");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_poll_plan(true,true,true,true,0.25f,0.75f",
                 "focused CharLookAt source test covers no-roll radius Poll plan");
  ok &= contains(mesh_decode_test,
                 "ghogx::character::decode_lookat(\"l-eye.lookat\","
                 "make_lookat(2,2))",
                 "focused mesh decode test covers stock-style CharLookAt row");
  ok &= contains(mesh_decode_test,
                 "ghogx::character::decode_lookat(\"full.lookat\","
                 "make_lookat(5,1))",
                 "focused mesh decode test covers revision-gated CharLookAt tail");
  ok &= contains(mesh_decode_test,
                 "decode_lookat(\"bad.lookat\",bad_lookat)",
                 "focused mesh decode test covers CharLookAt revision rejection");
  ok &= contains(mesh_decode_test,
                 "decode_lookat(\"bad-weight.lookat\",make_lookat(2,3))",
                 "focused mesh decode test covers CharWeightable revision rejection");
  ok &= contains(doc,
                 "`Hmx::Object`, `CharWeightable`, `mSource`, `mPivot`, `mDest`",
                 "document records source CharLookAt load order");
  ok &= contains(doc,
                 "Native enforces the source `CharLookAt` revision ceiling",
                 "document records CharLookAt revision ceiling");
  ok &= contains(doc,
                 "`CharLookAt::SyncLimits` clamps yaw and pitch limits",
                 "document records CharLookAt SyncLimits helper port");
  ok &= contains(doc,
                 "Native `source_char_lookat_load_plan` and",
                 "document records CharLookAt load/copy plan helpers");
  ok &= contains(doc,
                 "`Load` accepts revisions 0-5",
                 "document records CharLookAt load revision range");
  ok &= contains(doc,
                 "Native `source_char_lookat_enter` and",
                 "document records CharLookAt Enter and PollDeps helpers");
  ok &= contains(doc,
                 "Native `source_char_lookat_poll_plan` ports the checked `Poll` gate",
                 "document records CharLookAt Poll plan helper");
  ok &= contains(doc,
                 "This remains a branch contract only; it\n    does not synthesize",
                 "document fences CharLookAt Poll plan from transform write");
  ok &= contains(doc,
                 "do\n    not claim the full `Poll` transform write",
                 "document fences full CharLookAt Poll transform write");
  ok &= contains(doc,
                 "Current stock GH2 `CharLookAt` rows observed in the base characters have\n"
                 "    `mDest=<none>`",
                 "document records stock CharLookAt inert destination boundary");
  ok &= contains(rb3_char_eyes_cpp,
                 "else{ObjPtrList<CharLookAt,ObjectDir>pList(this,"
                 "kObjListNoNull);bs>>pList;mEyes.resize(pList.size());",
                 "RB3 CharEyes old revisions read a CharLookAt list");
  ok &= contains(rb3_char_eyes_cpp,
                 "BEGIN_LOADS(CharEyes)LOAD_REVS(bs)ASSERT_REVS(0x12,0)"
                 "LOAD_SUPERCLASS(Hmx::Object)",
                 "RB3 CharEyes source load enforces revision range");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(gRev>5)LOAD_SUPERCLASS(CharWeightable)if(gRev>4)"
                 "bs>>mEyes;",
                 "RB3 CharEyes source load gates weightable and EyeDesc rows");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(gRev-3<=1U){ObjPtr<RndTransformable,ObjectDir>tPtr(this,"
                 "0);bs>>tPtr;}",
                 "RB3 CharEyes rev 3/4 consumes a trailing transformable");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(gRev-4<5U){ObjPtr<RndTransformable,ObjectDir>tPtr(this,"
                 "0);intcnt;bs>>cnt;",
                 "RB3 CharEyes source load gates legacy interest rows");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(gRev>0xE){bs>>mUpperLidTrackUp;bs>>mUpperLidTrackDown;"
                 "bs>>mLowerLidTrackUp;if(gRev<0x11){bs.ReadInt();"
                 "bs>>mLowerLidTrackDown;bs.ReadInt();}elsebs>>"
                 "mLowerLidTrackDown;}",
                 "RB3 CharEyes source load gates lid tracking rows");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(gRev>0x11)bs>>mLowerLidTrackRotate;",
                 "RB3 CharEyes source load gates lower lid rotate");
  ok &= contains(rb3_char_eyes_cpp,
                 "CharEyes::CharEyes():mEyes(this),mInterests(this),"
                 "mFaceServo(this,0),mCamWeight(this,0),unk58(0,0,0),"
                 "mDefaultFilterFlags(0),mViewDirection(this,0),"
                 "mHeadLookAt(this,0),mMaxExtrapolation(19.5f),"
                 "mMinTargetDist(35.0f),mUpperLidTrackUp(1.0f),"
                 "mUpperLidTrackDown(1.0f),mLowerLidTrackUp(0.75f),"
                 "mLowerLidTrackDown(0.75f),mLowerLidTrackRotate(0),"
                 "mInterestFilterFlags(0),unka4(0,0,0),unkb4(0),"
                 "unkc0(0),unkc4(0),unkc5(0),unkc8(this,0),unkd4(this,0),"
                 "unke0(-1),unke4(0),unke8(0),unkec(1.0F),unkf0(0),"
                 "unkf4(0),unk124(0),unk128(-1.0f),unk12c(-1),"
                 "unk13c(0),unk140(-1.0f),unk144(0),unk148(-1.0f),"
                 "unk14c(-1.0f),unk15c(0),unk15d(1){",
                 "RB3 CharEyes constructor exposes source defaults");
  ok &= contains(rb3_char_eyes_cpp,
                 "unkb8=std::cos(0.52359879f);unk9c=RndOverlay::Find("
                 "\"eye_status\",false);",
                 "RB3 CharEyes constructor computes fallback cone and overlay");
  ok &= contains(rb3_char_eyes_cpp,
                 "BEGIN_COPYS(CharEyes)COPY_SUPERCLASS(Hmx::Object)"
                 "COPY_SUPERCLASS(CharWeightable)CREATE_COPY(CharEyes)"
                 "BEGIN_COPYING_MEMBERS",
                 "RB3 CharEyes copy starts from source copy macros");
  ok &= contains(rb3_char_eyes_cpp,
                 "COPY_MEMBER(mEyes)COPY_MEMBER(mInterests)COPY_MEMBER("
                 "mFaceServo)COPY_MEMBER(unka4)COPY_MEMBER(unkb4)"
                 "COPY_MEMBER(mCamWeight)COPY_MEMBER(mDefaultFilterFlags)"
                 "COPY_MEMBER(mViewDirection)COPY_MEMBER(mHeadLookAt)"
                 "COPY_MEMBER(mMaxExtrapolation)COPY_MEMBER(mMinTargetDist)"
                 "COPY_MEMBER(mUpperLidTrackUp)COPY_MEMBER(mUpperLidTrackDown)"
                 "COPY_MEMBER(mLowerLidTrackUp)COPY_MEMBER("
                 "mLowerLidTrackDown)COPY_MEMBER(mLowerLidTrackRotate)",
                 "RB3 CharEyes copy member list is explicit");
  ok &= contains(rb3_char_eyes_cpp,
                 "CharEyes::EyeDesc::EyeDesc(Hmx::Object*o):mEye(o,0),"
                 "mUpperLid(o,0),mLowerLid(o,0),mLowerLidBlink(o,0),"
                 "mUpperLidBlink(o,0){}",
                 "RB3 CharEyes EyeDesc constructor defaults refs");
  ok &= contains(rb3_char_eyes_cpp,
                 "CharEyes::EyeDesc::EyeDesc(constCharEyes::EyeDesc&desc):"
                 "mEye(desc.mEye),mUpperLid(desc.mUpperLid),"
                 "mLowerLid(desc.mLowerLid),mLowerLidBlink("
                 "desc.mLowerLidBlink),mUpperLidBlink(desc.mUpperLidBlink){}",
                 "RB3 CharEyes EyeDesc copy constructor preserves refs");
  ok &= contains(rb3_char_eyes_cpp,
                 "CharEyes::EyeDesc&CharEyes::EyeDesc::operator=("
                 "constCharEyes::EyeDesc&desc){mEye=desc.mEye;"
                 "mUpperLid=desc.mUpperLid;mLowerLid=desc.mLowerLid;"
                 "mUpperLidBlink=desc.mUpperLidBlink;mLowerLidBlink="
                 "desc.mLowerLidBlink;return*this;}",
                 "RB3 CharEyes EyeDesc assignment preserves refs");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::CharInterestState::ResetState(){unkc=-1.0f;}",
                 "RB3 CharEyes interest state reset clears refractory timer");
  ok &= contains(rb3_char_eyes_cpp,
                 "CharEyes::CharInterestState::CharInterestState(Hmx::Object*o):"
                 "mInterest(o,0){ResetState();}",
                 "RB3 CharEyes interest state constructor resets timer");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::CharInterestState::BeginRefractoryPeriod(){"
                 "unkc=TheTaskMgr.Seconds(TaskMgr::b);}",
                 "RB3 CharEyes interest state begins refractory at task time");
  ok &= contains(rb3_char_eyes_cpp,
                 "boolCharEyes::CharInterestState::IsInRefractoryPeriod(){"
                 "if(!mInterest||unkc<0.0)returnfalse;else{floatsecs="
                 "TheTaskMgr.Seconds(TaskMgr::b)-unkc;if(secs<mInterest->"
                 "mRefractoryPeriod)returntrue;elsereturnfalse;}}",
                 "RB3 CharEyes interest state checks active refractory period");
  ok &= contains(rb3_char_eyes_cpp,
                 "floatCharEyes::CharInterestState::RefractoryTimeRemaining(){"
                 "if(!mInterest||unkc<0.0)return0.0f;else{floatsecs="
                 "TheTaskMgr.Seconds(TaskMgr::b)-unkc;if(secs<mInterest->"
                 "mRefractoryPeriod)returnmInterest->mRefractoryPeriod-secs;"
                 "elsereturn0.0f;}}",
                 "RB3 CharEyes interest state computes refractory time remaining");
  ok &= contains(rb3_char_eyes_cpp,
                 "boolCharEyes::SetFocusInterest(CharInterest*interest,inti){"
                 "if(unkd4&&unke0>i)returnfalse;CharInterest*loc_interest="
                 "interest;unkd4=interest;unke0=i;if(loc_interest!=interest)"
                 "unke4=true;if(!unkd4)unke0=-1;returntrue;}",
                 "RB3 CharEyes SetFocusInterest priority gate");
  ok &= contains(rb3_char_eyes_cpp,
                 "RndTransformable*CharEyes::GetHead(){if(mViewDirection)"
                 "returnmViewDirection;elseif(!mEyes.empty()&&mEyes[0].mEye){"
                 "RndTransformable*src=mEyes[0].mEye->GetSource();if(src)"
                 "returnsrc->TransParent();}return0;}",
                 "RB3 CharEyes GetHead view direction and eye-source fallback");
  ok &= contains(rb3_char_eyes_cpp,
                 "CharInterest*CharEyes::GetCurrentInterest(){if(unkd4)"
                 "returnunkd4;if(unkc8)returnunkc8;return0;}",
                 "RB3 CharEyes GetCurrentInterest focus fallback");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::ForceBlink(){unk13c=true;unk140="
                 "TheTaskMgr.Seconds(TaskMgr::b);unk144++;}",
                 "RB3 CharEyes ForceBlink stores task time and increments count");
  ok &= contains(rb3_char_eyes_cpp,
                 "boolCharEyes::EitherEyeClamped(){for(ObjVector<EyeDesc>::"
                 "iteratorit=mEyes.begin();it!=mEyes.end();++it){if(it->mEye&&"
                 "it->mEye->unkb1)returntrue;}returnfalse;}",
                 "RB3 CharEyes EitherEyeClamped scans present eye clamp flags");
  ok &= contains(rb3_char_eyes_cpp,
                 "DataNodeCharEyes::OnToggleForceFocus(DataArray*da){"
                 "if(unkd4)SetFocusInterest(0,0);elseSetFocusInterest("
                 "unkc8,0);returnDataNode(0);}",
                 "RB3 CharEyes force-focus handler delegates to SetFocusInterest");
  ok &= contains(rb3_char_eyes_cpp,
                 "DataNodeCharEyes::OnToggleInterestOverlay(DataArray*da){"
                 "ToggleInterestsDebugOverlay();returnDataNode(0);}",
                 "RB3 CharEyes overlay handler delegates to toggle helper");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::ToggleInterestsDebugOverlay(){RndOverlay*o="
                 "unk9c;if(!o)return;o->mShowing=o->mShowing==false;"
                 "o->mTimer.Restart();}",
                 "RB3 CharEyes overlay toggle gates missing overlay");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::Enter(){unka4.Zero();unkb4=0;unkbc=0;"
                 "unkb0=1.0f;unkc0=-1.0f;unkc4=0;unk124=0;unk128=-1.0f;"
                 "unk12c=-1;unk13c=0;unk140=-1.0f;unk144=0;unk148=-1.0f;"
                 "unk14c=-1.0f;unkc5=0;mInterestFilterFlags="
                 "mDefaultFilterFlags;unk15c=0;unke4=0;unkf4=0;",
                 "RB3 CharEyes Enter resets source state constants");
  ok &= contains(rb3_char_eyes_cpp,
                 "RndTransformable*head=GetHead();if(head){unka4=head->"
                 "WorldXfm().m.y;Normalize(unka4,unka4);}",
                 "RB3 CharEyes Enter normalizes head world Y row");
  ok &= contains(rb3_char_eyes_cpp,
                 "for(ObjVector<EyeDesc>::iteratorit=mEyes.begin();it!="
                 "mEyes.end();++it){it->mEye->Enter();}",
                 "RB3 CharEyes Enter enters each eye controller");
  ok &= contains(rb3_char_eyes_cpp,
                 "for(ObjVector<CharInterestState>::iteratorit=mInterests."
                 "begin();it!=mInterests.end();++it){it->ResetState();}"
                 "RndPollable::Enter();}",
                 "RB3 CharEyes Enter resets interests and enters pollable");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::Exit(){unkd4=0;unke0=-1;mInterests.clear();"
                 "for(ObjVector<EyeDesc>::iteratorit=mEyes.begin();it!="
                 "mEyes.end();++it){it->mEye->Exit();}RndPollable::Exit();}",
                 "RB3 CharEyes Exit clears focus interests and exits eyes");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::ClearAllInterestObjects(){mInterests.clear();}",
                 "RB3 CharEyes ClearAllInterestObjects clears interest vector");
  ok &= contains(rb3_char_eyes_cpp,
                 "voidCharEyes::AddInterestObject(CharInterest*interest){"
                 "if(interest){CharInterestStatestate(this);state.mInterest="
                 "interest;mInterests.push_back(state);}}",
                 "RB3 CharEyes AddInterestObject guards null and pushes reset state");
  ok &= contains(rb3_char_eyes_cpp, "plist.push_back((*it).mEye);",
                 "RB3 CharEyes delegates poll children to CharLookAt rows");
  ok &= contains(rb3_char_eyes_cpp,
                 "ObjectDir*dir=(*it).mInterest->Dir();if(dir==Dir()){"
                 "changedBy.push_back((*it).mInterest);}",
                 "RB3 CharEyes PollDeps gates interests by owning dir");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(!mEyes.empty()){changedBy.push_back(GetHead());"
                 "change.push_back(GetTarget());}",
                 "RB3 CharEyes PollDeps publishes head and target when eyes exist");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(mHeadLookAt)changedBy.push_back(mHeadLookAt);"
                 "if(mFaceServo)changedBy.push_back(mFaceServo);",
                 "RB3 CharEyes PollDeps publishes head lookat and face servo");
  ok &= contains(rb3_latest_char_eye_dart_ruleset_h,
                 "structEyeDartRulesetData{EyeDartRulesetData(){"
                 "ClearToDefaults();}",
                 "latest CharEyeDartRuleset header defaults in constructor");
  ok &= contains(rb3_latest_char_eye_dart_ruleset_cpp,
                 "voidCharEyeDartRuleset::EyeDartRulesetData::"
                 "ClearToDefaults(){mMinRadius=0.5f;mMaxRadius=3.0f;",
                 "latest CharEyeDartRuleset source exposes defaults");
  ok &= contains(rb3_latest_char_eye_dart_ruleset_cpp,
                 "LOAD_REVS(bs);ASSERT_REVS(1,0);Hmx::Object::Load(bs);"
                 "bs>>mData.mMinRadius>>mData.mMaxRadius",
                 "latest CharEyeDartRuleset source load accepts revision 1");
  ok &= contains(rb3_latest_char_eye_dart_ruleset_cpp,
                 "COPY_MEMBER(mData.mMinRadius)//COPY_MEMBER(mData.mMaxRadius)"
                 "mData.mMaxRadius=c->mData.mMinRadius;",
                 "latest CharEyeDartRuleset source copy has max-radius quirk");
  ok &= contains(rb3_latest_char_interest_h,
                 "floatmMaxViewAngle;",
                 "latest CharInterest header exposes timing fields");
  ok &= contains(rb3_latest_char_interest_h,
                 "floatmRefractoryPeriod;",
                 "latest CharInterest header exposes refractory field");
  ok &= contains(rb3_latest_char_interest_cpp,
                 "CharInterest::CharInterest():mMaxViewAngle(20.0f),"
                 "mPriority(1.0f),mMinLookTime(1.0f),mMaxLookTime(3.0f),"
                 "mRefractoryPeriod(6.1f)",
                 "latest CharInterest source exposes defaults");
  ok &= contains(rb3_latest_char_interest_cpp,
                 "voidCharInterest::SyncMaxViewAngle(){mMaxViewAngleCos="
                 "cos_f(mMaxViewAngle*0.017453292f);}",
                 "latest CharInterest source syncs max view angle cosine");
  ok &= contains(rb3_latest_char_interest_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(6,0)LOAD_SUPERCLASS(Hmx::Object)"
                 "LOAD_SUPERCLASS(RndTransformable)",
                 "latest CharInterest source load accepts revision 6");
  ok &= contains(rb3_latest_char_interest_cpp,
                 "boolCharInterest::IsMatchingFilterFlags(intmask){return"
                 "mCategoryFlags&mask&&mCategoryFlags!=0;}",
                 "latest CharInterest source exposes category filter logic");
  ok &= contains(rb3_latest_char_interest_cpp,
                 "floatCharInterest::ComputeScore(",
                 "latest CharInterest source exposes ComputeScore boundary");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyeDartRulesetData{floatmin_radius=0.5f;",
                 "native exposes CharEyeDartRuleset source data");
  ok &= contains(char_mesh_h,
                 "SourceCharEyeDartRulesetDatasource_char_eye_dart_ruleset_defaults();",
                 "native exposes CharEyeDartRuleset defaults helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_eye_dart_ruleset_load_revision_known(intrevision);",
                 "native exposes CharEyeDartRuleset revision helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyeDartRulesetDatasource_char_eye_dart_ruleset_copy(",
                 "native exposes CharEyeDartRuleset copy helper");
  ok &= contains(char_mesh,
                 "SourceCharEyeDartRulesetDatasource_char_eye_dart_ruleset_defaults(){"
                 "returnSourceCharEyeDartRulesetData{};}",
                 "native implements CharEyeDartRuleset defaults helper");
  ok &= contains(char_mesh,
                 "returnrevision>=0&&revision<=1;",
                 "native implements CharEyeDartRuleset source revision range");
  ok &= contains(char_mesh,
                 "dst.min_radius=src.min_radius;dst.max_radius=src.min_radius;",
                 "native preserves CharEyeDartRuleset copy max-radius quirk");
  ok &= contains(char_mesh_h,
                 "structSourceCharInterestState{floatmax_view_angle=20.0f;",
                 "native exposes CharInterest source state");
  ok &= contains(char_mesh_h,
                 "SourceCharInterestStatesource_char_interest_defaults();",
                 "native exposes CharInterest defaults helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_interest_load_revision_known(intrevision);",
                 "native exposes CharInterest revision helper");
  ok &= contains(char_mesh_h,
                 "floatsource_char_interest_sync_max_view_angle("
                 "floatmax_view_angle_degrees);",
                 "native exposes CharInterest SyncMaxViewAngle helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_interest_is_matching_filter_flags("
                 "intcategory_flags,intmask);",
                 "native exposes CharInterest filter helper");
  ok &= contains(char_mesh,
                 "returnstd::cos(max_view_angle_degrees*0.017453292f);",
                 "native ports CharInterest max view angle cosine");
  ok &= contains(char_mesh,
                 "SourceCharInterestStatesource_char_interest_defaults(){"
                 "SourceCharInterestStatestate;",
                 "native implements CharInterest defaults helper");
  ok &= contains(char_mesh,
                 "returnrevision>=0&&revision<=6;",
                 "native implements CharInterest source revision range");
  ok &= contains(char_mesh,
                 "return(category_flags&mask)!=0&&category_flags!=0;",
                 "native ports CharInterest category filter logic");
  ok &= contains(char_mesh,
                 "dst.max_view_angle_cos=source_char_interest_sync_max_view_angle"
                 "(dst.max_view_angle);",
                 "native CharInterest copy resyncs max view angle");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_eye_dart_ruleset_source_test",
                 "CMake builds CharEyeDartRuleset source test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_interest_source_test",
                 "CMake builds CharInterest source test");
  ok &= contains(eye_dart_ruleset_source_test,
                 "source_char_eye_dart_ruleset_defaults()",
                 "focused CharEyeDartRuleset source test covers defaults");
  ok &= contains(eye_dart_ruleset_source_test,
                 "source_char_eye_dart_ruleset_load_revision_known(2)",
                 "focused CharEyeDartRuleset source test covers revision reject");
  ok &= contains(eye_dart_ruleset_source_test,
                 "\"copymaxradiusfollowssourcemin-radiusassignment\"",
                 "focused CharEyeDartRuleset source test covers copy quirk");
  ok &= contains(interest_source_test,
                 "source_char_interest_defaults()",
                 "focused CharInterest source test covers defaults");
  ok &= contains(interest_source_test,
                 "source_char_interest_load_revision_known(7)",
                 "focused CharInterest source test covers revision reject");
  ok &= contains(interest_source_test,
                 "source_char_interest_is_matching_filter_flags(0x6,0x2)",
                 "focused CharInterest source test covers category matching");
  ok &= contains(interest_source_test,
                 "\"copyresyncsmaxviewcosine\"",
                 "focused CharInterest source test covers copy resync");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesInterest{std::stringinterest;boolsame_dir=false;};",
                 "native exposes CharEyes interest dependency input");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesDefaultState{size_teye_count=0;"
                 "size_tinterest_count=0;boolhas_face_servo=false;"
                 "boolhas_cam_weight=false;std::array<float,3>unk58="
                 "{0.0f,0.0f,0.0f};intdefault_filter_flags=0;"
                 "boolhas_view_direction=false;boolhas_head_lookat=false;"
                 "floatmax_extrapolation=19.5f;floatmin_target_dist=35.0f;",
                 "native exposes CharEyes default source state");
  ok &= contains(char_mesh_h,
                 "floatupper_lid_track_up=1.0f;floatupper_lid_track_down=1.0f;"
                 "floatlower_lid_track_up=0.75f;floatlower_lid_track_down=0.75f;"
                 "intlower_lid_track_rotate=0;intinterest_filter_flags=0;",
                 "native CharEyes default state exposes lid/filter defaults");
  ok &= contains(char_mesh_h,
                 "boolhas_current_interest=false;boolhas_focus_interest=false;"
                 "intfocus_priority=-1;boolunke4=false;boolunke8=false;"
                 "floatunkec=1.0f;",
                 "native CharEyes default state exposes focus defaults");
  ok &= contains(char_mesh_h,
                 "boolunk124=false;floatunk128=-1.0f;intunk12c=-1;"
                 "boolunk13c=false;floatunk140=-1.0f;intunk144=0;"
                 "floatunk148=-1.0f;floatunk14c=-1.0f;boolunk15c=false;"
                 "boolunk15d=true;std::stringoverlay_name;};",
                 "native CharEyes default state exposes timer and overlay fields");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesEyeDesc{std::stringeye;"
                 "std::stringupper_lid;std::stringlower_lid;"
                 "std::stringlower_lid_blink;std::stringupper_lid_blink;};",
                 "native exposes CharEyes EyeDesc row");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesEyeDescLoadPlan{std::vector<std::string>"
                 "read_order;};",
                 "native exposes CharEyes EyeDesc load plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesClampRow{boolhas_eye=false;"
                 "boolclamped=false;};",
                 "native exposes CharEyes clamp row");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesFocusResult{boolaccepted=false;"
                 "std::stringfocus_interest;intfocus_priority=-1;};",
                 "native exposes CharEyes focus result");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesOverlayToggleResult{boolhas_overlay=false;"
                 "boolshowing=false;booltimer_restarted=false;};",
                 "native exposes CharEyes overlay toggle result");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesForceBlinkState{boolpending_blink=false;"
                 "floatblink_time=-1.0f;intblink_count_delta=0;};",
                 "native exposes CharEyes force blink state");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesLoadPlan{boolrevision_supported=false;"
                 "std::vector<std::string>read_order;std::vector<std::string>"
                 "branches;};",
                 "native exposes CharEyes load plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;};",
                 "native exposes CharEyes copy plan");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesEnterState{std::array<float,3>unka4="
                 "{0.0f,0.0f,0.0f};intunkb4=0;intunkbc=0;floatunkb0=1.0f;"
                 "floatunkc0=-1.0f;intunkc4=0;boolunk124=false;"
                 "floatunk128=-1.0f;intunk12c=-1;boolunk13c=false;"
                 "floatunk140=-1.0f;intunk144=0;floatunk148=-1.0f;"
                 "floatunk14c=-1.0f;boolunkc5=false;intinterest_filter_flags=0;"
                 "boolunk15c=false;boolunke4=false;boolunkf4=false;"
                 "size_teye_enter_count=0;size_tinterest_reset_count=0;"
                 "boolpollable_enter=true;};",
                 "native exposes CharEyes Enter source state");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesExitState{std::stringfocus_interest;"
                 "intfocus_priority=-1;boolclear_interests=true;"
                 "size_teye_exit_count=0;boolpollable_exit=true;};",
                 "native exposes CharEyes Exit source state");
  ok &= contains(char_mesh_h,
                 "structSourceCharEyesInterestRuntime{std::stringinterest;"
                 "floatrefractory_start=-1.0f;};",
                 "native exposes CharEyes interest refractory state");
  ok &= contains(char_mesh_h,
                 "std::vector<std::string>source_char_eyes_list_poll_children(",
                 "native exposes CharEyes poll child helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_eyes_either_eye_clamped("
                 "conststd::vector<SourceCharEyesClampRow>&eyes);",
                 "native exposes CharEyes EitherEyeClamped helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesEyeDescLoadPlansource_char_eyes_eye_desc_load_plan("
                 "int32_trevision);",
                 "native exposes CharEyes EyeDesc load helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesLoadPlansource_char_eyes_load_plan(int32_trevision);",
                 "native exposes CharEyes load plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesCopyPlansource_char_eyes_copy_plan();",
                 "native exposes CharEyes copy plan helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesDefaultStatesource_char_eyes_default_state();",
                 "native exposes CharEyes default state helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesDefaultStatesource_char_eyes_copy_state("
                 "constSourceCharEyesDefaultState&source);",
                 "native exposes CharEyes copy state helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesEyeDescsource_char_eyes_eye_desc_default();",
                 "native exposes CharEyes EyeDesc default helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesEyeDescsource_char_eyes_eye_desc_copy("
                 "constSourceCharEyesEyeDesc&source);",
                 "native exposes CharEyes EyeDesc copy helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_eyes_eye_desc_assign("
                 "SourceCharEyesEyeDesc&dest,constSourceCharEyesEyeDesc&source);",
                 "native exposes CharEyes EyeDesc assignment helper");
  ok &= contains(char_mesh_h,
                 "std::stringsource_char_eyes_get_head("
                 "conststd::string&view_direction,conststd::string&"
                 "first_eye_source_parent);",
                 "native exposes CharEyes GetHead helper");
  ok &= contains(char_mesh_h,
                 "std::stringsource_char_eyes_current_interest("
                 "conststd::string&focus_interest,conststd::string&"
                 "current_interest);",
                 "native exposes CharEyes current-interest helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesFocusResultsource_char_eyes_set_focus_interest("
                 "conststd::string&current_focus,intcurrent_priority,"
                 "conststd::string&requested_interest,intrequested_priority);",
                 "native exposes CharEyes SetFocusInterest helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesFocusResultsource_char_eyes_toggle_force_focus("
                 "conststd::string&current_focus,intcurrent_priority,"
                 "conststd::string&current_interest);",
                 "native exposes CharEyes force-focus toggle helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesOverlayToggleResult"
                 "source_char_eyes_toggle_interest_overlay(boolhas_overlay,"
                 "boolcurrent_showing);",
                 "native exposes CharEyes overlay toggle helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesForceBlinkStatesource_char_eyes_force_blink("
                 "floattask_seconds);",
                 "native exposes CharEyes ForceBlink helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesEnterStatesource_char_eyes_enter_state("
                 "intdefault_filter_flags,boolhas_head,conststd::array<float,3>&"
                 "head_world_y,size_teye_count,size_tinterest_count);",
                 "native exposes CharEyes Enter state helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesExitStatesource_char_eyes_exit_state("
                 "size_teye_count);",
                 "native exposes CharEyes Exit state helper");
  ok &= contains(char_mesh_h,
                 "SourceCharEyesInterestRuntimesource_char_eyes_interest_state("
                 "conststd::string&interest);",
                 "native exposes CharEyes interest state helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_eyes_interest_reset("
                 "SourceCharEyesInterestRuntime&state);",
                 "native exposes CharEyes interest reset helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_eyes_interest_begin_refractory("
                 "SourceCharEyesInterestRuntime&state,floattask_seconds);",
                 "native exposes CharEyes interest refractory begin helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_eyes_interest_in_refractory("
                 "constSourceCharEyesInterestRuntime&state,floattask_seconds,"
                 "floatrefractory_period);",
                 "native exposes CharEyes refractory active helper");
  ok &= contains(char_mesh_h,
                 "floatsource_char_eyes_interest_refractory_remaining("
                 "constSourceCharEyesInterestRuntime&state,floattask_seconds,"
                 "floatrefractory_period);",
                 "native exposes CharEyes refractory remaining helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_eyes_clear_interest_objects("
                 "std::vector<SourceCharEyesInterestRuntime>&interests);",
                 "native exposes CharEyes clear interest helper");
  ok &= contains(char_mesh_h,
                 "boolsource_char_eyes_add_interest_object("
                 "std::vector<SourceCharEyesInterestRuntime>&interests,"
                 "conststd::string&interest);",
                 "native exposes CharEyes add interest helper");
  ok &= contains(char_mesh_h,
                 "voidsource_char_eyes_poll_deps(",
                 "native exposes CharEyes PollDeps helper");
  ok &= contains(char_mesh,
                 "std::vector<std::string>source_char_eyes_list_poll_children("
                 "conststd::vector<std::string>&eye_lookats)",
                 "native implements CharEyes poll child helper");
  ok &= contains(char_mesh,
                 "for(conststd::string&eye:eye_lookats)children.push_back(eye);",
                 "native CharEyes helper delegates poll children to lookat refs");
  ok &= contains(char_mesh,
                 "boolsource_char_eyes_either_eye_clamped("
                 "conststd::vector<SourceCharEyesClampRow>&eyes){for("
                 "constSourceCharEyesClampRow&eye:eyes){if(eye.has_eye&&"
                 "eye.clamped)returntrue;}returnfalse;}",
                 "native CharEyes EitherEyeClamped helper mirrors source scan");
  ok &= contains(char_mesh,
                 "SourceCharEyesEyeDescLoadPlansource_char_eyes_eye_desc_load_plan("
                 "int32_trevision){SourceCharEyesEyeDescLoadPlanplan;"
                 "plan.read_order={\"mEye\",\"mUpperLid\"};",
                 "native CharEyes EyeDesc load helper starts with source fields");
  ok &= contains(char_mesh,
                 "if(revision>6)plan.read_order.push_back(\"mLowerLid\");"
                 "if(revision>0xF){plan.read_order.push_back(\"mUpperLidBlink\");"
                 "plan.read_order.push_back(\"mLowerLidBlink\");}",
                 "native CharEyes EyeDesc load helper ports revision gates");
  ok &= contains(char_mesh,
                 "SourceCharEyesLoadPlansource_char_eyes_load_plan(int32_trevision){"
                 "SourceCharEyesLoadPlanplan;plan.revision_supported=revision>=0&&"
                 "revision<=0x12;",
                 "native CharEyes load helper ports revision gate");
  ok &= contains(char_mesh,
                 "if(revision>4){plan.read_order.push_back(\"mEyes\");}else{"
                 "plan.read_order.push_back(\"legacyLookAtList\");",
                 "native CharEyes load helper ports legacy eye-list gate");
  ok &= contains(char_mesh,
                 "if(revision>=4&&revision<=8){plan.read_order.push_back("
                 "\"legacyInterestTransformCount\");",
                 "native CharEyes load helper ports legacy interest branch");
  ok &= contains(char_mesh,
                 "if(revision<0x11){plan.read_order.push_back("
                 "\"legacyLowerLidTrackDownPad0\");plan.read_order.push_back("
                 "\"mLowerLidTrackDown\");",
                 "native CharEyes load helper ports lower-lid padding branch");
  ok &= contains(char_mesh,
                 "SourceCharEyesCopyPlansource_char_eyes_copy_plan(){"
                 "SourceCharEyesCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\",\"CharWeightable\"};",
                 "native CharEyes copy helper records superclasses");
  ok &= contains(char_mesh,
                 "plan.copied_members={\"mEyes\",\"mInterests\",\"mFaceServo\","
                 "\"unka4\",\"unkb4\",\"mCamWeight\",\"mDefaultFilterFlags\",",
                 "native CharEyes copy helper records copy member prefix");
  ok &= contains(char_mesh,
                 "SourceCharEyesDefaultStatesource_char_eyes_default_state(){"
                 "SourceCharEyesDefaultStatestate;state.unkb8=std::cos("
                 "0.52359879f);state.overlay_name=\"eye_status\";"
                 "returnstate;}",
                 "native CharEyes default helper ports constructor body");
  ok &= contains(char_mesh,
                 "SourceCharEyesDefaultStatesource_char_eyes_copy_state("
                 "constSourceCharEyesDefaultState&source){"
                 "SourceCharEyesDefaultStatedest=source_char_eyes_default_state();",
                 "native CharEyes copy helper starts from constructor defaults");
  ok &= contains(char_mesh,
                 "dest.eye_count=source.eye_count;dest.interest_count="
                 "source.interest_count;dest.has_face_servo=source."
                 "has_face_servo;dest.unka4=source.unka4;dest.unkb4="
                 "source.unkb4;dest.has_cam_weight=source.has_cam_weight;"
                 "dest.default_filter_flags=source.default_filter_flags;",
                 "native CharEyes copy helper ports first COPY_MEMBER fields");
  ok &= contains(char_mesh,
                 "dest.has_view_direction=source.has_view_direction;"
                 "dest.has_head_lookat=source.has_head_lookat;dest."
                 "max_extrapolation=source.max_extrapolation;dest."
                 "min_target_dist=source.min_target_dist;dest."
                 "upper_lid_track_up=source.upper_lid_track_up;dest."
                 "upper_lid_track_down=source.upper_lid_track_down;dest."
                 "lower_lid_track_up=source.lower_lid_track_up;dest."
                 "lower_lid_track_down=source.lower_lid_track_down;dest."
                 "lower_lid_track_rotate=source.lower_lid_track_rotate;"
                 "returndest;}",
                 "native CharEyes copy helper ports lid and target fields");
  ok &= contains(char_mesh,
                 "SourceCharEyesEyeDescsource_char_eyes_eye_desc_default(){"
                 "returnSourceCharEyesEyeDesc{};}",
                 "native CharEyes EyeDesc default helper ports null refs");
  ok &= contains(char_mesh,
                 "SourceCharEyesEyeDescsource_char_eyes_eye_desc_copy("
                 "constSourceCharEyesEyeDesc&source){SourceCharEyesEyeDescdesc;"
                 "desc.eye=source.eye;desc.upper_lid=source.upper_lid;"
                 "desc.lower_lid=source.lower_lid;desc.lower_lid_blink="
                 "source.lower_lid_blink;desc.upper_lid_blink="
                 "source.upper_lid_blink;returndesc;}",
                 "native CharEyes EyeDesc copy helper ports source refs");
  ok &= contains(char_mesh,
                 "voidsource_char_eyes_eye_desc_assign("
                 "SourceCharEyesEyeDesc&dest,constSourceCharEyesEyeDesc&source){"
                 "dest.eye=source.eye;dest.upper_lid=source.upper_lid;"
                 "dest.lower_lid=source.lower_lid;dest.upper_lid_blink="
                 "source.upper_lid_blink;dest.lower_lid_blink="
                 "source.lower_lid_blink;}",
                 "native CharEyes EyeDesc assignment helper ports source refs");
  ok &= contains(char_mesh,
                 "std::stringsource_char_eyes_get_head("
                 "conststd::string&view_direction,conststd::string&"
                 "first_eye_source_parent){if(!view_direction.empty())"
                 "returnview_direction;if(!first_eye_source_parent.empty())"
                 "returnfirst_eye_source_parent;return{};}",
                 "native CharEyes GetHead helper ports source fallback");
  ok &= contains(char_mesh,
                 "std::stringsource_char_eyes_current_interest("
                 "conststd::string&focus_interest,conststd::string&"
                 "current_interest){if(!focus_interest.empty())"
                 "returnfocus_interest;if(!current_interest.empty())"
                 "returncurrent_interest;return{};}",
                 "native CharEyes current-interest helper ports source fallback");
  ok &= contains(char_mesh,
                 "if(!current_focus.empty()&&current_priority>"
                 "requested_priority){returnresult;}",
                 "native CharEyes SetFocusInterest helper ports priority reject");
  ok &= contains(char_mesh,
                 "result.focus_interest=requested_interest;result.focus_priority="
                 "requested_interest.empty()?-1:requested_priority;returnresult;}",
                 "native CharEyes SetFocusInterest helper ports assignment and clear");
  ok &= contains(char_mesh,
                 "SourceCharEyesFocusResultsource_char_eyes_toggle_force_focus("
                 "conststd::string&current_focus,intcurrent_priority,"
                 "conststd::string&current_interest){if(!current_focus.empty()){"
                 "returnsource_char_eyes_set_focus_interest(current_focus,"
                 "current_priority,\"\",0);}returnsource_char_eyes_set_focus_interest("
                 "current_focus,current_priority,current_interest,0);}",
                 "native CharEyes force-focus helper delegates through source focus gate");
  ok &= contains(char_mesh,
                 "SourceCharEyesOverlayToggleResult"
                 "source_char_eyes_toggle_interest_overlay(boolhas_overlay,"
                 "boolcurrent_showing){SourceCharEyesOverlayToggleResultresult;"
                 "result.has_overlay=has_overlay;result.showing=current_showing;"
                 "if(!has_overlay)returnresult;result.showing=!current_showing;"
                 "result.timer_restarted=true;returnresult;}",
                 "native CharEyes overlay helper ports source toggle gate");
  ok &= contains(char_mesh,
                 "SourceCharEyesForceBlinkStatesource_char_eyes_force_blink("
                 "floattask_seconds){SourceCharEyesForceBlinkStatestate;"
                 "state.pending_blink=true;state.blink_time=task_seconds;"
                 "state.blink_count_delta=1;returnstate;}",
                 "native CharEyes ForceBlink helper ports source state writes");
  ok &= contains(char_mesh,
                 "SourceCharEyesEnterStatesource_char_eyes_enter_state("
                 "intdefault_filter_flags,boolhas_head,conststd::array<float,3>&"
                 "head_world_y,size_teye_count,size_tinterest_count){"
                 "SourceCharEyesEnterStatestate;state.interest_filter_flags="
                 "default_filter_flags;state.eye_enter_count=eye_count;"
                 "state.interest_reset_count=interest_count;",
                 "native CharEyes Enter helper starts from source reset state");
  ok &= contains(char_mesh,
                 "if(has_head){constfloatlen_sq=head_world_y[0]*head_world_y[0]+"
                 "head_world_y[1]*head_world_y[1]+head_world_y[2]*"
                 "head_world_y[2];if(len_sq>0.0f){constfloatinv_len=1.0f/"
                 "std::sqrt(len_sq);state.unka4={head_world_y[0]*inv_len,"
                 "head_world_y[1]*inv_len,head_world_y[2]*inv_len};}}"
                 "returnstate;}",
                 "native CharEyes Enter helper normalizes head world Y row");
  ok &= contains(char_mesh,
                 "SourceCharEyesExitStatesource_char_eyes_exit_state("
                 "size_teye_count){SourceCharEyesExitStatestate;"
                 "state.focus_interest={};state.focus_priority=-1;"
                 "state.clear_interests=true;state.eye_exit_count=eye_count;"
                 "state.pollable_exit=true;returnstate;}",
                 "native CharEyes Exit helper ports source clear behavior");
  ok &= contains(char_mesh,
                 "SourceCharEyesInterestRuntimesource_char_eyes_interest_state("
                 "conststd::string&interest){SourceCharEyesInterestRuntimestate;"
                 "state.interest=interest;state.refractory_start=-1.0f;"
                 "returnstate;}",
                 "native CharEyes interest state helper ports constructor reset");
  ok &= contains(char_mesh,
                 "voidsource_char_eyes_interest_reset("
                 "SourceCharEyesInterestRuntime&state){state.refractory_start="
                 "-1.0f;}",
                 "native CharEyes interest reset helper ports source reset");
  ok &= contains(char_mesh,
                 "voidsource_char_eyes_interest_begin_refractory("
                 "SourceCharEyesInterestRuntime&state,floattask_seconds){"
                 "state.refractory_start=task_seconds;}",
                 "native CharEyes interest begin helper ports task time write");
  ok &= contains(char_mesh,
                 "if(state.interest.empty()||state.refractory_start<0.0f)"
                 "returnfalse;returntask_seconds-state.refractory_start<"
                 "refractory_period;",
                 "native CharEyes refractory active helper ports source gates");
  ok &= contains(char_mesh,
                 "if(state.interest.empty()||state.refractory_start<0.0f)"
                 "return0.0f;constfloatelapsed=task_seconds-state."
                 "refractory_start;if(elapsed<refractory_period)return"
                 "refractory_period-elapsed;return0.0f;",
                 "native CharEyes refractory remaining helper ports source gates");
  ok &= contains(char_mesh,
                 "voidsource_char_eyes_clear_interest_objects("
                 "std::vector<SourceCharEyesInterestRuntime>&interests){"
                 "interests.clear();}",
                 "native CharEyes clear interest helper ports source clear");
  ok &= contains(char_mesh,
                 "boolsource_char_eyes_add_interest_object("
                 "std::vector<SourceCharEyesInterestRuntime>&interests,"
                 "conststd::string&interest){if(interest.empty())returnfalse;"
                 "interests.push_back(source_char_eyes_interest_state(interest));"
                 "returntrue;}",
                 "native CharEyes add interest helper ports source guard and push");
  ok &= contains(char_mesh,
                 "if(interest.same_dir)deps.changed_by.push_back(interest.interest);",
                 "native CharEyes helper gates interests by owning dir");
  ok &= contains(char_mesh,
                 "if(has_eyes){deps.changed_by.push_back(head);"
                 "deps.change.push_back(target);}",
                 "native CharEyes helper publishes head and target when eyes exist");
  ok &= contains(char_mesh,
                 "if(!head_lookat.empty())deps.changed_by.push_back(head_lookat);"
                 "if(!face_servo.empty())deps.changed_by.push_back(face_servo);",
                 "native CharEyes helper publishes optional head lookat and face servo");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_eyes_source_test",
                 "CMake builds CharEyes source test");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_list_poll_children({\"l-eye.lookat\","
                 "\"r-eye.lookat\"})",
                 "focused CharEyes source test covers poll children");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_either_eye_clamped({{false,true},"
                 "{true,false}})",
                 "focused CharEyes source test covers missing eye clamp ignore");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_either_eye_clamped({{true,false},"
                 "{true,true}})",
                 "focused CharEyes source test covers present clamped eye");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_eye_desc_load_plan(16)",
                 "focused CharEyes source test covers EyeDesc load plan");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_load_plan(4)",
                 "focused CharEyes source test covers legacy load plan");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_load_plan(18)",
                 "focused CharEyes source test covers current load plan");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_copy_plan()",
                 "focused CharEyes source test covers copy plan");
  ok &= contains(eyes_source_test,
                 "SourceCharEyesInterest{\"same.interest\",true}",
                 "focused CharEyes source test covers same-dir interest");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_default_state()",
                 "focused CharEyes source test covers constructor defaults");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_copy_state(source_defaults)",
                 "focused CharEyes source test covers copy helper");
  ok &= contains(eyes_source_test,
                 "\"copyresetsfocusruntime\"",
                 "focused CharEyes source test covers copy runtime reset");
  ok &= contains(eyes_source_test,
                 "\"copyresetsoverlayname\"",
                 "focused CharEyes source test covers copy overlay reset");
  ok &= contains(eyes_source_test,
                 "expect_float(defaults.unkb8,0.86602539f,\"defaultunkb8\")",
                 "focused CharEyes source test covers constructor cosine default");
  ok &= contains(eyes_source_test,
                 "expect_string(defaults.overlay_name,\"eye_status\"",
                 "focused CharEyes source test covers overlay lookup default");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_eye_desc_default()",
                 "focused CharEyes source test covers EyeDesc default helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_eye_desc_copy(eye_desc)",
                 "focused CharEyes source test covers EyeDesc copy helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_eye_desc_assign(assigned_eye,eye_desc)",
                 "focused CharEyes source test covers EyeDesc assignment helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_get_head(\"view.trans\",\"eye.parent\")",
                 "focused CharEyes source test covers GetHead view priority");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_current_interest(\"focus.interest\","
                 "\"look.interest\")",
                 "focused CharEyes source test covers current interest focus priority");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_set_focus_interest(\"boss.focus\",5,"
                 "\"minor.focus\",3)",
                 "focused CharEyes source test covers focus priority reject");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_set_focus_interest(\"boss.focus\",5,\"\",8)",
                 "focused CharEyes source test covers focus clear");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_toggle_force_focus(\"soft.focus\",0,"
                 "\"look.interest\")",
                 "focused CharEyes source test covers force-focus clear");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_toggle_force_focus(\"boss.focus\",5,"
                 "\"look.interest\")",
                 "focused CharEyes source test covers force-focus priority reject");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_toggle_force_focus(\"\",-1,\"look.interest\")",
                 "focused CharEyes source test covers force-focus set current interest");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_toggle_interest_overlay(true,false)",
                 "focused CharEyes source test covers overlay toggle present");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_toggle_interest_overlay(false,true)",
                 "focused CharEyes source test covers overlay toggle missing");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_force_blink(12.5f)",
                 "focused CharEyes source test covers ForceBlink helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_enter_state(0x24,true,{0.0f,3.0f,4.0f},2,3)",
                 "focused CharEyes source test covers Enter helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_enter_state(0,false,{0.0f,3.0f,4.0f},0,0)",
                 "focused CharEyes source test covers Enter no-head branch");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_exit_state(2)",
                 "focused CharEyes source test covers Exit helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_interest_state(\"stage.light\")",
                 "focused CharEyes source test covers interest state constructor");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_interest_begin_refractory(runtime,20.0f)",
                 "focused CharEyes source test covers refractory begin");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_interest_in_refractory(runtime,23.0f,6.1f)",
                 "focused CharEyes source test covers active refractory branch");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_interest_refractory_remaining(runtime,23.0f,"
                 "6.1f)",
                 "focused CharEyes source test covers refractory remaining branch");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_interest_reset(runtime)",
                 "focused CharEyes source test covers refractory reset");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_add_interest_object(interests,\"\")",
                 "focused CharEyes source test covers missing interest add guard");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_add_interest_object(interests,\"stage.light\")",
                 "focused CharEyes source test covers interest add helper");
  ok &= contains(eyes_source_test,
                 "source_char_eyes_clear_interest_objects(interests)",
                 "focused CharEyes source test covers interest clear helper");
  ok &= contains(eyes_source_test,
                 "\"noeyeshasnotargetchange\"",
                 "focused CharEyes source test covers no-eye target gate");
  ok &= contains(char_mesh,
                 "uint32_tcount=r.u32();for(uint32_ti=0;i<count&&r.pos<r.n;"
                 "++i)eyes.lookats.push_back(r.str());",
                 "native GH2 CharEyes decoder keeps old look-at list layout");
  ok &= contains(char_mesh,
                 "if(eyes.version<0||eyes.version>0x12){"
                 "throwstd::runtime_error",
                 "native GH2 CharEyes decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "if((eyes.version==3||eyes.version==4)&&r.pos<r.n){"
                 "eyes.legacy_transform=r.str();}}"
                 "eyes.unread_bytes=r.n-r.pos;",
                 "native GH2 CharEyes decoder source-gates trailing old transformable");
  ok &= contains(doc, "Rockabill2 face/attachment proof",
                 "document records current Rockabill2 eye and teeth evidence");
  ok &= contains(doc,
                 "Native `source_char_eyes_*` helpers port these graph/dependency decisions",
                 "document records native CharEyes helper boundary");
  ok &= contains(doc,
                 "plus the concrete `GetHead`, `GetCurrentInterest`, "
                 "`SetFocusInterest`, and\n    `ForceBlink` state bodies",
                 "document records native CharEyes state helper slice");
  ok &= contains(doc,
                 "Native `source_char_eyes_interest_*` helpers port the concrete",
                 "document records native CharEyes refractory helper slice");
  ok &= contains(doc,
                 "Native `source_char_eyes_eye_desc_*` and interest-list helpers",
                 "document records native CharEyes EyeDesc and interest-list helpers");
  ok &= contains(doc,
                 "Native `source_char_eyes_eye_desc_load_plan`,",
                 "document records native CharEyes load/copy plan helpers");
  ok &= contains(doc,
                 "revision 15/16 lower-lid padding branches",
                 "document records CharEyes lower-lid padding branch");
  ok &= contains(doc,
                 "Native `source_char_eyes_copy_state` ports the concrete `BEGIN_COPYS`",
                 "document records native CharEyes copy helper slice");
  ok &= contains(doc,
                 "Runtime-only fields intentionally reset instead of\n"
                 "    copying",
                 "document records native CharEyes copy runtime reset boundary");
  ok &= contains(doc,
                 "Native `source_char_eyes_enter_state` and "
                 "`source_char_eyes_exit_state`",
                 "document records native CharEyes Enter and Exit helper slice");
  ok &= contains(doc,
                 "Native `source_char_eyes_toggle_force_focus` and",
                 "document records native CharEyes handler toggle helper slice");
  ok &= contains(doc,
                 "Native `source_char_eyes_either_eye_clamped` ports the concrete",
                 "document records native CharEyes EitherEyeClamped helper");
  ok &= contains(doc,
                 "It does not invent\n    clamp state for missing eye refs",
                 "document fences native CharEyes clamp helper");
  ok &= contains(doc,
                 "Native `source_char_eyes_default_state` ports the concrete constructor",
                 "document records native CharEyes constructor default helper");
  ok &= contains(doc,
                 "Fields not initialized by the source\n    constructor remain outside this helper",
                 "document fences uninitialized CharEyes constructor fields");
  ok &= contains(doc,
                 "Native `source_char_eye_dart_ruleset_*` helpers preserve this\n"
                 "    exact data behavior",
                 "document records native CharEyeDartRuleset helper boundary");
  ok &= contains(doc,
                 "Native `source_char_interest_*`\n    helpers port these data "
                 "decisions",
                 "document records native CharInterest helper boundary");
  ok &= contains(doc,
                 "`ComputeScore` includes runtime vectors and `RandomFloat`; it stays fenced",
                 "document fences CharInterest runtime scoring");
  ok &= contains(rb3_char_ik_hand_cpp, "voidCharIKHand::Poll(){",
                 "RB3 CharIKHand source exposes Poll");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(0xC,0)",
                 "RB3 CharIKHand source enforces revision ceiling");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "RndTransformable*frontTrans=mTargets.front().mTarget;",
                 "RB3 CharIKHand source resolves target transform");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "Interp(mHand->WorldXfm().v,vec,charWeight,mWorldDst);",
                 "RB3 CharIKHand source blends world destination");
  ok &= contains(rb3_char_ik_hand_cpp, "IKElbow(parent1,parent2);",
                 "RB3 CharIKHand source drives elbow solve");
  ok &= contains(rb3_char_ik_hand_cpp, "mHand->SetWorldXfm(tf);",
                 "RB3 CharIKHand source writes hand world transform");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "PullShoulder(v100,trans2->WorldXfm(),mWorldDst,mAAPlusBB);",
                 "RB3 CharIKHand source calls PullShoulder from IKElbow");
  ok &= missing(rb3_char_ik_hand_cpp, "voidCharIKHand::PullShoulder(",
                "available RB3 CharIKHand source lacks PullShoulder body");
  ok &= contains(band3_config, "CharIKHand__PullShoulder",
                 "band3_recomp exposes CharIKHand PullShoulder symbol");
  ok &= missing(char_clip, "PullShoulder(",
                "native IKHand slice must not rederive missing PullShoulder");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "voidCharIKHand::MeasureLengths(){if(mHand){if("
                 "mHand->TransParent()){if(mHand->TransParent()->TransParent()"
                 "){floatlen=Length(mHand->mLocalXfm.v);",
                 "RB3 CharIKHand source exposes MeasureLengths chain gate");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "unk64=len*2.0f*parentlen;mInv2ab=parentlen*parentlen+"
                 "(len*len+0.0f);if(unk64!=0.0f)unk64=1.0f/unk64;"
                 "mAAPlusBB=len+parentlen;",
                 "RB3 CharIKHand source defines MeasureLengths scalar fields");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "floatloc210=unk64*(DistanceSquared(trans2->WorldXfm().v,"
                 "mWorldDst)-mInv2ab);ClampEq(loc210,-1.0f,1.0f);",
                 "RB3 CharIKHand source clamps IKElbow cosine");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "voidCharIKHand::SetHand(RndTransformable*t){mHand=t;"
                 "mHandChanged=true;}",
                 "RB3 CharIKHand source marks hand length cache dirty");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "voidCharIKHand::UpdateHand(){if(mScalable||mHandChanged){"
                 "MeasureLengths();mHandChanged=false;}}",
                 "RB3 CharIKHand source gates MeasureLengths updates");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "if(gRev>4)bs>>mFinger;elsemFinger=0;",
                 "RB3 CharIKHand source gates finger by revision");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "if(gRev<3){ObjPtr<RndTransformable,ObjectDir>tPtr(this,0);"
                 "bs>>tPtr;",
                 "RB3 CharIKHand source exposes old single-target layout");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "if(gRev>3)bs>>mMoveElbow;elsemMoveElbow=true;",
                 "RB3 CharIKHand source gates move_elbow");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "if(gRev>5)bs>>mElbowSwing;elsemElbowSwing=0.0f;",
                 "RB3 CharIKHand source gates elbow_swing");
  ok &= contains(rb3_char_ik_hand_cpp,
                 "if(gRev>0xB){bs>>mElbowCollide;bs>>mClockwise;}",
                 "RB3 CharIKHand source gates elbow collision branch");
  ok &= contains(char_mesh,
                 "hand.version=r.i32();",
                 "native CharIKHand decoder stores source revision");
  ok &= contains(char_mesh,
                 "if(hand.version<0||hand.version>0xC){"
                 "throwstd::runtime_error",
                 "native CharIKHand decoder enforces source revision range");
  ok &= contains(char_mesh_h,
                 "boolclockwise=false;size_tunread_bytes=0;",
                 "native CharIKHand stores source tail bytes");
  ok &= contains(char_mesh_h,
                 "structRuntimeIKHandMeasureState{boolhand_changed=true;"
                 "boolhas_elbow_chain=false;floatinv_2ab=0.0f;"
                 "floata2_plus_b2=0.0f;floataa_plus_bb=0.0f;};",
                 "native stores source CharIKHand length-cache state");
  ok &= contains(char_mesh_h,
                 "std::map<std::string,RuntimeIKHandMeasureState>"
                 "runtime_ik_hand_measures;",
                 "Character owns persistent source CharIKHand length caches");
  ok &= contains(char_mesh,
                 "if(hand.version>4)hand.finger=r.str();",
                 "native CharIKHand decoder follows source finger gate");
  ok &= contains(char_mesh,
                 "hand.targets.push_back({hand.target,0.0f});",
                 "native CharIKHand decoder records old single target");
  ok &= contains(char_mesh,
                 "if(hand.version>3&&r.pos<r.n)hand.move_elbow=r.u8()!=0;",
                 "native CharIKHand decoder follows source move_elbow gate");
  ok &= contains(char_mesh,
                 "if(hand.version>5&&r.pos+4<=r.n)hand.elbow_swing=r.f32();",
                 "native CharIKHand decoder follows source elbow_swing gate");
  ok &= contains(char_mesh,
                 "if(hand.version>0xB&&r.pos<r.n){hand.elbow_collide=r.str();",
                 "native CharIKHand decoder follows source elbow collision gate");
  ok &= contains(char_mesh, "hand.unread_bytes=r.n-r.pos;",
                 "native CharIKHand decoder records source tail bytes");
  ok &= contains(char_clip,
                 "\"[chargraph]ik%sversion=%dhand=%sfinger=%s\"",
                 "character graph logs source CharIKHand revision and finger");
  ok &= contains(char_clip,
                 "\"elbowSwing=%.3falwaysElbow=%dconstrainWrist=%d\"",
                 "character graph logs bounded CharIKHand optional fields");
  ok &= contains(char_clip, "clockwise=%d",
                 "character graph logs CharIKHand clockwise field");
  ok &= contains(char_clip, "unreadBytes=%zu",
                 "character graph logs CharIKHand tail bytes");
  ok &= contains(char_clip_h,
                 "structSourceCharIKHandMeasure{boolhas_elbow_chain=false;"
                 "floatinv_2ab=0.0f;floata2_plus_b2=0.0f;"
                 "floataa_plus_bb=0.0f;};",
                 "native exposes source CharIKHand MeasureLengths state");
  ok &= contains(char_clip_h,
                 "SourceCharIKHandMeasuresource_char_ik_hand_measure_lengths("
                 "boolhas_elbow_chain,floathand_local_len,"
                 "floatparent_local_len);",
                 "native API exposes source CharIKHand MeasureLengths helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_ik_hand_update_measure_lengths("
                 "boolscalable,bool&hand_changed);",
                 "native API exposes source CharIKHand UpdateHand gate helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_ik_hand_elbow_cosine("
                 "constSourceCharIKHandMeasure&measure,floatdistance_squared,"
                 "float&out_cosine);",
                 "native API exposes source CharIKHand IKElbow cosine helper");
  ok &= contains(char_clip,
                 "SourceCharIKHandMeasuresource_char_ik_hand_measure_lengths("
                 "boolhas_elbow_chain,floathand_local_len,"
                 "floatparent_local_len){SourceCharIKHandMeasureout;"
                 "if(!has_elbow_chain)returnout;out.has_elbow_chain=true;",
                 "native CharIKHand MeasureLengths helper keeps source chain gate");
  ok &= contains(char_clip,
                 "out.inv_2ab=hand_local_len*2.0f*parent_local_len;"
                 "out.a2_plus_b2=parent_local_len*parent_local_len+"
                 "hand_local_len*hand_local_len;if(out.inv_2ab!=0.0f)"
                 "out.inv_2ab=1.0f/out.inv_2ab;out.aa_plus_bb="
                 "hand_local_len+parent_local_len;",
                 "native CharIKHand MeasureLengths helper mirrors source fields");
  ok &= contains(char_clip,
                 "boolsource_char_ik_hand_update_measure_lengths("
                 "boolscalable,bool&hand_changed){if(scalable||hand_changed){"
                 "hand_changed=false;returntrue;}returnfalse;}",
                 "native CharIKHand UpdateHand helper mirrors source gate");
  ok &= contains(char_clip,
                 "out_cosine=measure.inv_2ab*(distance_squared-"
                 "measure.a2_plus_b2);out_cosine=std::clamp(out_cosine,"
                 "-1.0f,1.0f);",
                 "native CharIKHand IKElbow cosine helper mirrors source clamp");
  ok &= contains(char_clip,
                 "RuntimeIKHandMeasureState&measure_state="
                 "character.runtime_ik_hand_measures[live_key];",
                 "runtime CharIKHand slice uses persistent source length cache");
  ok &= contains(char_clip,
                 "source_char_ik_hand_update_measure_lengths("
                 "ik.scalable,measure_state.hand_changed)",
                 "runtime CharIKHand slice applies source UpdateHand gate");
  ok &= contains(char_clip,
                 "constfloatdist2=raw_dist*raw_dist;",
                 "runtime CharIKHand slice feeds source cosine from raw distance squared");
  ok &= missing(char_clip, "std::clamp(raw_dist",
                "runtime CharIKHand slice does not pre-clamp distance before source cosine");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_hand_source_test"
                 "character_ik_hand_source_test.cpp)",
                 "CMake builds focused CharIKHand source test");
  ok &= contains(ik_hand_source_test,
                 "source_char_ik_hand_measure_lengths(true,4.0f,3.0f)",
                 "focused CharIKHand source test covers MeasureLengths helper");
  ok &= contains(ik_hand_source_test,
                 "source_char_ik_hand_elbow_cosine(measure,49.0f,cosine)",
                 "focused CharIKHand source test covers IKElbow max-reach scalar");
  ok &= contains(ik_hand_source_test,
                 "source_char_ik_hand_update_measure_lengths("
                 "false,hand_changed)",
                 "focused CharIKHand source test covers UpdateHand clean gate");
  ok &= contains(doc,
                 "Native `source_char_ik_hand_measure_lengths` and\n"
                 "    `source_char_ik_hand_elbow_cosine` port the source",
                 "document records native CharIKHand MeasureLengths slice");
  ok &= contains(doc,
                 "`source_char_ik_hand_update_measure_lengths` mirrors "
                 "`SetHand` /",
                 "document records native CharIKHand UpdateHand cache slice");
  ok &= contains(doc,
                 "`CharIKHand::PullShoulder` is source-real but not yet "
                 "source-importable",
                 "document records missing CharIKHand PullShoulder body");
  ok &= contains(doc,
                 "does not include its body. Native GHOGX\n    therefore "
                 "must not rederive that shoulder offset",
                 "document fences native CharIKHand PullShoulder rederivation");
  ok &= contains(bind_audit, "boolshould_dump_controllers(intargc,char**argv)",
                 "bind audit exposes controller inventory switch");
  ok &= contains(bind_audit,
                 "\"[controller-summary]path=%schar=%sdrivers=%zu",
                 "bind audit controller summary is path-backed");
  ok &= contains(bind_audit,
                 "\"[controller-ik-hand]char=%sname=%sversion=%dunknown=%d\"",
                 "bind audit logs source CharIKHand revision fields");
  ok &= contains(bind_audit,
                 "\"elbowSwing=%.4falwaysElbow=%dconstrainWrist=%d\"",
                 "bind audit logs optional CharIKHand branch fields");
  ok &= contains(bind_audit, "clockwise=%d",
                 "bind audit logs CharIKHand clockwise field");
  ok &= contains(bind_audit, "unreadBytes=%zu",
                 "bind audit logs CharIKHand tail bytes");
  ok &= contains(bind_audit,
                 "\"char/rock1/og/gen/rock1.milo_ps2\"",
                 "bind audit default stock list includes Rock1");
  ok &= contains(bind_audit,
                 "\"char/rockabill2/og/gen/rockabill2.milo_ps2\"",
                 "bind audit default stock list includes Rockabill2");
  ok &= contains(bind_audit,
                 "\"char/grim/og/gen/grim.milo_ps2\"",
                 "bind audit default stock list includes Grim");
  ok &= contains(bind_audit,
                 "\"char/alterna2/og/gen/alterna2.milo_ps2\"",
                 "bind audit default stock list includes Alterna2");
  ok &= contains(bind_audit,
                 "\"char/glam2/og/gen/glam2.milo_ps2\"",
                 "bind audit default stock list includes Glam2");
  ok &= contains(bind_audit,
                 "\"char/punk2/og/gen/punk2.milo_ps2\"",
                 "bind audit default stock list includes Punk2");
  ok &= contains(doc,
                 "The current runtime solver is the bounded GH2 single-target slice",
                 "document fences partial CharIKHand runtime solver");
  ok &= contains(doc,
                 "expanded_stock_characters_controller_hair_inventory.log",
                 "document cites expanded stock controller inventory");
  ok &= contains(doc, "loads 24 base character MILOs",
                 "document records expanded stock character sample count");
  ok &= contains(doc, "All 38 decoded `CharIKHand` rows are source revision 2",
                 "document records stock CharIKHand revision evidence");
  ok &= contains(doc,
                 "enforces the source revision range, and\n    records the row "
                 "tail byte count",
                 "document records CharIKHand revision and tail boundary");
  ok &= contains(doc,
                 "source_ikhand_20260711/stock_ikhand_controllers.stdout.log",
                 "document cites refreshed CharIKHand proof log");
  ok &= contains(doc,
                 "all 38 stock `CharIKHand`\n  rows are `version=2`, have no "
                 "finger, and report `unreadBytes=0`",
                 "document records refreshed CharIKHand stock proof");
  ok &= contains(doc,
                 "expanded_stock_characters_controller_inventory_weightsetter.log",
                 "document cites focused stock CharWeightSetter inventory");
  ok &= contains(doc,
                 "all 38 stock `CharWeightSetter` rows are source revision 2",
                 "document records stock CharWeightSetter revision evidence");
  ok &= contains(doc,
                 "`CharWeightable` revision 2, `offset=0`, `scale=1`, `base=<none>`",
                 "document records stock CharWeightSetter source branch evidence");
  ok &= contains(doc,
                 "Nineteen rows carry\n  `flags=0x00400000`",
                 "document records stock left-weight flags");
  ok &= contains(doc,
                 "nineteen carry `flags=0x00800000`",
                 "document records stock right-weight flags");
  ok &= contains(doc, "finds zero separate `CharCollide` objects",
                 "document records stock CharCollide absence evidence");
  ok &= contains(doc,
                 "`metal_drummer` contains one revision-1 foretwist row with a missing\n"
                 "  `twist2` pointer",
                 "document records incomplete metal_drummer foretwist evidence");
  ok &= contains(doc,
                 "Only Grim exposes decoded `CharIKRod` rows in this 24-character base set",
                 "document records stock Grim CharIKRod scope");
  ok &= contains(doc,
                 "`rknee.rod` and `lknee.rod`. Both are source revision 2",
                 "document records stock Grim CharIKRod revisions");
  ok &= contains(doc,
                 "both\n  have `dest=<none>`",
                 "document records stock Grim CharIKRod missing destination");
  ok &= contains(doc,
                 "stock_character_type_inventory.log",
                 "document cites stock character type inventory");
  ok &= contains(doc,
                 "all 24 base character MILOs contain one `CharServoBone` row",
                 "document records stock CharServoBone coverage");
  ok &= contains(doc,
                 "`CharDriver target=bone.servo` is explicit source\n"
                 "  data rather than an implied name",
                 "document records driver-to-servo source data boundary");
  ok &= contains(doc,
                 "grim_charikrod_servo_inventory_after.log",
                 "document cites refreshed Grim CharServoBone proof");
  ok &= contains(doc, "version=1 clipType=<none>",
                 "document records Grim CharServoBone stock revision proof");
  ok &= contains(rb3_char_upper_twist_cpp,
                 "MakeRotQuat(twist2parentworld.m.x,twist2world.m.x,q);",
                 "RB3 CharUpperTwist source builds source-parent rotation");
  ok &= contains(rb3_char_upper_twist_cpp,
                 "Interp(v68,twist2world.m.y,0.333f,tf48.m.y);"
                 "LookAt(tf48.m);mUpperArm->SetWorldXfm(tf48);",
                 "RB3 CharUpperTwist source writes first driven twist");
  ok &= contains(rb3_char_upper_twist_cpp,
                 "Interp(v68,twist2world.m.y,0.666f,tf48.m.y);"
                 "LookAt(tf48.m);mTwist1->SetWorldXfm(tf48);",
                 "RB3 CharUpperTwist source writes second driven twist");
  ok &= contains(rb3_char_upper_twist_cpp,
                 "SYNC_PROP(upper_arm,mTwist2)SYNC_PROP(twist1,mUpperArm)"
                 "SYNC_PROP(twist2,mTwist1)",
                 "RB3 CharUpperTwist property/member crosswalk");
  ok &= contains(rb3_char_fore_twist_cpp,
                 "bs>>mOffset;bs>>mHand;bs>>mTwist2;if(gRev==2){"
                 "intdummy;bs>>dummy;}if(gRev>3)bs>>mBias;",
                 "RB3 CharForeTwist source load order");
  ok &= contains(rb3_char_fore_twist_cpp,
                 "floatangle=LimitAng(mOffset*DEG2RAD+tan2res+newbias);"
                 "floatfinalfloat=angle-newbias;",
                 "RB3 CharForeTwist source offset and bias angle path");
  ok &= contains(rb3_char_fore_twist_cpp,
                 "Interp(tf88.v,handxfm.v,twist2->mLocalXfm.v.x/"
                 "hand->mLocalXfm.v.x,tf88.v);",
                 "RB3 CharForeTwist source twist2 position interpolation");
  ok &= contains(rb3_latest_char_neck_twist_h,
                 "ObjPtr<RndTransformable,ObjectDir>mTwist;",
                 "latest CharNeckTwist header exposes twist pointer");
  ok &= contains(rb3_latest_char_neck_twist_h,
                 "ObjPtr<RndTransformable,ObjectDir>mHead;",
                 "latest CharNeckTwist header exposes head pointer");
  ok &= contains(rb3_latest_char_neck_twist_cpp,
                 "CharNeckTwist::CharNeckTwist():mTwist(this,0),"
                 "mHead(this,0)",
                 "latest CharNeckTwist source constructor exposes defaults");
  ok &= contains(rb3_latest_char_neck_twist_cpp,
                 "LOAD_REVS(bs);ASSERT_REVS(1,0);Hmx::Object::Load(bs);"
                 "bs>>mHead;bs>>mTwist;",
                 "latest CharNeckTwist source load order");
  ok &= contains(rb3_latest_char_neck_twist_cpp,
                 "changedBy.push_back(mHead);change.push_back(mTwist);",
                 "latest CharNeckTwist source PollDeps order");
  ok &= contains(rb3_latest_char_neck_twist_cpp,
                 "MakeRotQuatUnitX(tf58.m.x,q68);Multiply(tf58.m.y,q68,v78);"
                 "mTwist->DirtyLocalXfm().m.RotateAboutX("
                 "LimitAng(std::atan2(v78.z,v78.y))*0.5f);",
                 "latest CharNeckTwist source half-angle write");
  ok &= contains(char_mesh,
                 "if(t.version==2&&r.pos+4<=r.n)(void)r.i32();"
                 "if(t.version>3&&r.pos+4<=r.n)t.bias_degrees=r.f32();",
                 "native CharForeTwist decoder follows source revision fields");
  ok &= contains(char_mesh_h,
                 "structCharNeckTwist{std::stringname;int32_tversion=0;"
                 "std::stringhead;std::stringtwist;size_tunread_bytes=0;};",
                 "native stores source CharNeckTwist row fields");
  ok &= contains(char_mesh_h,
                 "std::vector<CharNeckTwist>neck_twists;",
                 "Character owns decoded CharNeckTwist rows");
  ok &= contains(char_mesh,
                 "CharNeckTwistdecode_neck_twist(conststd::string&entry_name,"
                 "conststd::vector<uint8_t>&body)",
                 "native exposes CharNeckTwist decoder");
  ok &= contains(char_mesh,
                 "if(t.version<0||t.version>1){throwstd::runtime_error",
                 "native CharNeckTwist decoder enforces source revision range");
  ok &= contains(char_mesh,
                 "read_object_fields(r);//Hmx::Objectmetadata"
                 "t.head=r.str();t.twist=r.str();"
                 "t.unread_bytes=r.n-r.pos;returnt;",
                 "native CharNeckTwist decoder follows source load order");
  ok &= contains(char_mesh,
                 "elseif(de.type==\"CharNeckTwist\"){"
                 "out.neck_twists.push_back(decode_neck_twist(de.name,b));",
                 "native character loader decodes CharNeckTwist rows");
  ok &= contains(char_mesh_h,
                 "structSourceCharNeckTwistPollDeps{"
                 "std::vector<std::string>changed_by;"
                 "std::vector<std::string>change;};",
                 "native exposes CharNeckTwist PollDeps state");
  ok &= contains(char_mesh_h,
                 "floatsource_char_neck_twist_half_limited_angle("
                 "floatrotated_y_y,floatrotated_y_z);",
                 "native API exposes CharNeckTwist half-angle helper");
  ok &= contains(char_mesh,
                 "SourceCharNeckTwistStatesource_char_neck_twist_defaults(){"
                 "returnSourceCharNeckTwistState{};}",
                 "native CharNeckTwist defaults helper mirrors constructor");
  ok &= contains(char_mesh,
                 "boolsource_char_neck_twist_load_revision_known(intrevision){"
                 "returnrevision>=0&&revision<=1;}",
                 "native CharNeckTwist revision helper mirrors source range");
  ok &= contains(char_mesh,
                 "deps.changed_by.push_back(head);deps.change.push_back(twist);",
                 "native CharNeckTwist PollDeps helper mirrors source order");
  ok &= contains(char_mesh,
                 "floatangle=std::atan2(rotated_y_z,rotated_y_y);",
                 "native CharNeckTwist angle helper starts from source atan2");
  ok &= contains(char_mesh,
                 "returnangle*0.5f;",
                 "native CharNeckTwist angle helper mirrors source half scale");
  ok &= contains(bind_audit, "neckTwist=%zu",
                 "bind audit logs CharNeckTwist row count");
  ok &= contains(bind_audit,
                 "\"[controller-neck-twist]char=%sname=%sversion=%dhead=%s\"",
                 "bind audit logs CharNeckTwist row data");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_neck_twist_source_test"
                 "character_neck_twist_source_test.cpp)",
                 "CMake builds focused CharNeckTwist source test");
  ok &= contains(neck_twist_source_test,
                 "source_char_neck_twist_load_revision_known(1)",
                 "focused CharNeckTwist test covers source revision ceiling");
  ok &= contains(neck_twist_source_test,
                 "source_char_neck_twist_poll_deps(deps,\"bone_head.mesh\","
                 "\"bone_neck.mesh\")",
                 "focused CharNeckTwist test covers PollDeps helper");
  ok &= contains(neck_twist_source_test,
                 "source_char_neck_twist_half_limited_angle(0.0f,1.0f)",
                 "focused CharNeckTwist test covers half-angle helper");
  ok &= contains(doc, "`CharNeckTwist::Load` accepts source revisions through 1",
                 "document records CharNeckTwist load boundary");
  ok &= contains(doc,
                 "`CharNeckTwist::PollDeps` publishes `head` as the changed-by row",
                 "document records CharNeckTwist dependency order");
  ok &= contains(doc,
                 "rotates the\n    twist row about local X by half of "
                 "`LimitAng(atan2(z, y))`",
                 "document records CharNeckTwist source angle behavior");
  ok &= contains(doc,
                 "source_necktwist_20260711/stock_necktwist_inventory.log",
                 "document cites refreshed CharNeckTwist inventory");
  ok &= contains(doc,
                 "every one of the 24 base\n    character MILOs reports "
                 "`neckTwist=0`",
                 "document records stock CharNeckTwist absence");
  ok &= contains(doc,
                 "not\n    evidence that Rock1/Rock2 neck posture is fixed",
                 "document fences CharNeckTwist from neck-posture signoff");
  ok &= contains(char_clip, "apply_source_upper_twists(",
                 "native standalone upper twist path is source-named");
  ok &= contains(char_clip, "apply_source_fore_twist(",
                 "native standalone fore twist path is source-named");
  ok &= contains(char_clip_h,
                 "structSourceCharForeTwistPollWorldResult{"
                 "boolapplied=false;floatsource_angle_radians=0.0f;",
                 "native API exposes CharForeTwist source poll result");
  ok &= contains(char_clip_h,
                 "boolsource_char_fore_twist_poll_world("
                 "constCharForeTwist&twist,",
                 "native API exposes CharForeTwist source Poll helper");
  ok &= contains(char_clip_h,
                 "structSourceCharUpperTwistPollWorldResult{"
                 "boolapplied=false;std::array<float,16>twist1_world={};"
                 "std::array<float,16>twist2_world={};};",
                 "native API exposes CharUpperTwist source poll result");
  ok &= contains(char_clip_h,
                 "boolsource_char_upper_twist_poll_world("
                 "boolhas_source,boolhas_twist1,boolhas_twist2,",
                 "native API exposes CharUpperTwist source Poll helper");
  ok &= contains(char_clip,
                 "quat_from_vec_to_vec(mat_row(source_parent_world,0),"
                 "mat_row(source_world,0),q);",
                 "native CharUpperTwist port follows source MakeRotQuat rows");
  ok &= contains(char_clip,
                 "out.twist1_world=make_output(twist1_current_world,0.333f);"
                 "out.twist2_world=make_output(twist2_current_world,0.666f);",
                 "native CharUpperTwist port keeps source interpolation weights");
  ok &= contains(char_clip,
                 "std::atan2(clamped2,clamped)+bias",
                 "native CharForeTwist port keeps source angle basis and bias");
  ok &= contains(char_clip,
                 "boolsource_char_fore_twist_poll_world("
                 "constCharForeTwist&twist,boolhas_hand,boolhas_twist2,",
                 "native CharForeTwist source helper body exists");
  ok &= contains(char_clip,
                 "constfloatratio=twist2_local_x/hand_local_x;",
                 "native CharForeTwist source helper keeps source ratio division");
  ok &= contains(char_clip,
                 "out.twist_parent_world=source_matrix_multiply_rotation("
                 "rot,hand_parent_world);",
                 "native CharForeTwist helper writes source twist parent world");
  ok &= contains(char_clip,
                 "out.twist2_world=source_matrix_multiply_rotation("
                 "rot,out.twist_parent_world);",
                 "native CharForeTwist helper applies source rotation again to twist2");
  ok &= contains(char_clip,
                 "boolsource_char_upper_twist_poll_world("
                 "boolhas_source,boolhas_twist1,boolhas_twist2,",
                 "native CharUpperTwist source helper body exists");
  ok &= contains(char_clip,
                 "out.twist1_world=make_output(twist1_current_world,0.333f);"
                 "out.twist2_world=make_output(twist2_current_world,0.666f);",
                 "native CharUpperTwist helper keeps source interpolation constants");
  ok &= contains(char_clip,
                 "source_char_fore_twist_poll_world(ft,true,true,true,true,"
                 "parent_world,hand_world,hand.local.pos[0],twist2.local.pos[0],",
                 "runtime CharForeTwist path calls source helper");
  ok &= contains(char_clip,
                 "source_char_upper_twist_poll_world(true,true,true,true,"
                 "upper_parent_world,upper_world,twist1_current_world,"
                 "twist2_current_world,",
                 "runtime CharUpperTwist path calls source helper");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_fore_upper_twist_source_test"
                 "character_fore_upper_twist_source_test.cpp)",
                 "CMake builds focused fore/upper twist source test");
  ok &= contains(fore_upper_twist_source_test,
                 "source_char_fore_twist_poll_world(",
                 "focused fore/upper twist test covers CharForeTwist helper");
  ok &= contains(fore_upper_twist_source_test,
                 "source_char_upper_twist_poll_world(",
                 "focused fore/upper twist test covers CharUpperTwist helper");
  ok &= contains(fore_upper_twist_source_test,
                 "fore_out.source_angle_radians,kPi*2.0f/3.0f",
                 "focused fore twist test covers source angle with bias");
  ok &= contains(fore_upper_twist_source_test,
                 "expect_upper_rows(upper_out.twist1_world,0.333f",
                 "focused upper twist test covers source first weight");
  ok &= contains(fore_upper_twist_source_test,
                 "expect_upper_rows(upper_out.twist2_world,0.666f",
                 "focused upper twist test covers source second weight");
  ok &= contains(doc,
                 "Native `source_char_upper_twist_poll_world` ports that world-row `Poll`",
                 "document records native CharUpperTwist source helper");
  ok &= contains(doc,
                 "Native `source_char_fore_twist_poll_world` ports that world-row `Poll`",
                 "document records native CharForeTwist source helper");
  ok &= contains(char_clip,
                 "apply_source_ik_hands(character);"
                 "apply_source_fore_twists(character);"
                 "apply_char_hair(character,time_seconds);"
                 "apply_source_upper_twists(character,bind_bones);",
                 "native keeps upper twists after CharHair per accepted cadence");
  ok &= contains(char_clip,
                 "for(constCharIKHand&ik:character.ik_hands)",
                 "native CharIKHand polling uses decoded source order");
  ok &= contains(doc, "## Clip Runtime Boundary",
                 "document records CharClip runtime source boundary");
  ok &= contains(doc,
                 "`CharBones::TypeOf` maps suffixes `.pos`, `.scale`, "
                 "`.quat`, `.rotx`,",
                 "document records concrete CharBones channel suffix source");
  ok &= contains(doc,
                 "Source scans every dot in the symbol",
                 "document records source CharBones TypeOf scan behavior");
  ok &= contains(doc,
                 "Native `source_char_bones_type_of`, "
                 "`source_char_bones_suffix_of`,",
                 "document records native CharBones source helper ports");
  ok &= contains(doc,
                 "`ghogx_character_char_bones_source_test`\n    covers all "
                 "six source suffixes",
                 "document records focused CharBones source helper test");
  ok &= contains(doc,
                 "Native channel classification is constrained to those six "
                 "source types.",
                 "document records source-backed native channel type fence");
  ok &= contains(doc,
                 "Rejected clip-pose reinterpretation switches for "
                 "relative/transpose/swap/\n    invert/world quaternions",
                 "document records removed clip-pose reinterpretation switches");
  ok &= contains(doc,
                 "`TypeSize` defines the per-channel byte sizes for "
                 "uncompressed vectors",
                 "document records concrete CharBones compression sizing source");
  ok &= contains(doc,
                 "`kCompressNone`,\n    `kCompressRots`, "
                 "`kCompressVects`, `kCompressQuats`, and `kCompressAll`",
                 "document records full CharBones compression enum");
  ok &= contains(doc,
                 "`kCompressQuats` and `kCompressAll` use 4-byte `ByteQuat` "
                 "rows",
                 "document records byte-quat source storage");
  ok &= contains(doc,
                 "native refuses those lists for now because the checked\n"
                 "    source snapshot and RB2 dump identify `ByteQuat` "
                 "storage but do not expose",
                 "document fences byte-quat conversion body");
  ok &= contains(doc,
                 "`GHOGX_DEBUG_CLIP=1` logs accepted source "
                 "`CharBonesSamples` list\n    compression modes",
                 "document records source clip compression inventory logging");
  ok &= contains(doc,
                 "All 24 app runs exited 0.\n    Twenty-three characters "
                 "produced accepted `CharBonesSamples` rows; all 192",
                 "document records stock clip compression inventory coverage");
  ok &= contains(doc,
                 "accepted rows used `1(kCompressRots)` and `byteQuat=0`",
                 "document records stock accepted rows avoid byte-quat");
  ok &= contains(doc,
                 "`metal_keyboard`\n    rendered and logged its source driver, "
                 "but the default viewer clip names",
                 "document records metal_keyboard clip inventory caveat");
  ok &= contains(doc, "`ghogx_character_clip_audit` expands exact `.milo_ps2`",
                 "document records focused clip audit helper");
  ok &= contains(doc,
                 "found 14 MILOs, 165 `CharClipSamples` rows, 165 accepted "
                 "rows, and zero",
                 "document records focused clip audit row counts");
  ok &= contains(doc,
                 "`keyboard_lose`, `keyboard_win`, `keyboard_active_fast`, "
                 "`keyboard_idle`,",
                 "document records keyboard-specific accepted clips");
  ok &= contains(doc,
                 "`metal_keyboard` default-viewer miss was clip-route "
                 "selection, not sample",
                 "document records metal_keyboard route not decode failure");
  ok &= contains(doc,
                 "`rock1_fret` (`finger_chord_*` and "
                 "`finger_powerchord_*`)",
                 "document records chord-named fret clip inventory");
  ok &= contains(doc,
                 "no Rock2-specific animation MILOs under the\n    "
                 "`char/rock2/` prefix",
                 "document records Rock2 animation prefix inventory");
  ok &= contains(doc,
                 "audited the 24 documented stock base character prefixes. "
                 "The helper visited",
                 "document records broad stock clip audit scope");
  ok &= contains(doc,
                 "135 MILOs; 68 MILOs contained clips; all 1,903 "
                 "`CharClipSamples` rows loaded",
                 "document records broad stock clip audit counts");
  ok &= contains(doc,
                 "source `CharDriver` selection, clip blending, or final pose "
                 "publishing",
                 "document fences broad stock clip audit from runtime signoff");
  ok &= contains(doc,
                 "`deathmetal2`, `glam2`, `goth2`, `metal2`, `punk2`, and "
                 "`rock2` had zero",
                 "document records zero-local-clip variant prefixes");
  ok &= contains(doc,
                 "`alterna2` had only fret/strum/viseme rows,\n    and "
                 "`rockabill2` had only a fret row set",
                 "document records partial local clip variant prefixes");
  ok &= contains(doc,
                 "The matching controller-route audit at\n    "
                 "`analysis/source_clip_inventory_20260711/"
                 "stock_24_base_controller_driver_routes.stdout.log`",
                 "document records controller route audit proof");
  ok &= contains(doc,
                 "decoded 63\n    driver rows across the same 24 base MILOs: "
                 "25 base `midi=0` rows and 38",
                 "document records controller route audit driver counts");
  ok &= contains(doc,
                 "`deathmetal2 -> deathmetal1`,\n    `glam2 -> glam1`, "
                 "`goth2 -> goth1`, `metal2 -> metal1`,",
                 "document records zero-local sibling animation routes");
  ok &= contains(doc,
                 "`punk2 -> punk1`, and `rock2 -> rock1`",
                 "document records punk2 and rock2 sibling animation routes");
  ok &= contains(doc,
                 "`alterna2` routes only\n    `main.drv` to `alterna1_main`",
                 "document records alterna2 partial sibling animation route");
  ok &= contains(doc,
                 "`rockabill2` routes `main.drv` and `right_hand.drv` to",
                 "document records rockabill2 partial sibling animation route");
  ok &= contains(doc,
                 "the accessible tree does not include a\n"
                 "    matching `ByteQuat` type, header, or conversion "
                 "implementation",
                 "document records upstream ByteQuat source absence");
  ok &= contains(doc,
                 "`RotateBy`, `RotateTo`, and `ScaleAddSample` select\n"
                 "    `mRawData[mTotalSize * sample]` and split weight",
                 "document records concrete CharBonesSamples interpolation source");
  ok &= contains(doc,
                 "Native `source_char_bones_samples_allocate_size`,\n"
                 "    `source_char_bones_samples_set`, "
                 "`source_char_bones_samples_clone`,",
                 "document records concrete CharBonesSamples Set/Clone slice");
  ok &= contains(doc,
                 "This does not claim the still-missing `AddBoneInternal` body\n"
                 "    or expose a native `mRawData` pointer",
                 "document fences CharBonesSamples Set/Clone pointer boundary");
  ok &= contains(doc,
                 "`source_char_bones_samples_rotate_by_offset`,\n"
                 "    `source_char_bones_samples_rotate_to_steps`, and\n"
                 "    `source_char_bones_samples_scale_add_steps` are named wrappers",
                 "document records named CharBonesSamples sample wrappers");
  ok &= contains(doc,
                 "Native `source_char_clip_beat_align_string` ports the "
                 "concrete\n    `CharClip::BeatAlignString` body",
                 "document records concrete CharClip BeatAlignString slice");
  ok &= contains(doc,
                 "Native `source_char_bones_recompute_layout`\n    now ports "
                 "the safe data-layout core of `RecomputeSizes`",
                 "document records concrete CharBones layout slice");
  ok &= contains(doc,
                 "Native `source_char_bones_set_compression` ports the source\n"
                 "    `SetCompression` guard",
                 "document records concrete CharBones SetCompression slice");
  ok &= contains(doc,
                 "Native `source_char_bones_empty_state`,\n"
                 "    `source_char_bones_clear`, "
                 "`source_char_bones_set_weights`, and",
                 "document records concrete CharBones state helper slice");
  ok &= contains(doc,
                 "`source_char_bones_list_bones` port the complete source "
                 "constructor,",
                 "document records concrete CharBones ListBones slice");
  ok &= contains(doc,
                 "helper slice also ports `FindOffset` over native source-state rows",
                 "document records concrete CharBones FindOffset slice");
  ok &= contains(doc,
                 "Native `source_char_bones_find_ptr` preserves the concrete `FindPtr`",
                 "document records concrete CharBones FindPtr decision slice");
  ok &= contains(doc,
                 "It still does not expose a live native pointer",
                 "document fences native CharBones FindPtr pointer path");
  ok &= contains(doc,
                 "Native `source_char_bones_zero` ports the concrete `Zero` byte span",
                 "document records concrete CharBones Zero slice");
  ok &= contains(doc,
                 "Native `source_char_bones_add_bones_steps` ports the visible `AddBones`",
                 "document records concrete CharBones AddBones wrapper slice");
  ok &= contains(doc,
                 "the checked source declares but does not define\n"
                 "    `AddBoneInternal`",
                 "document fences missing CharBones AddBoneInternal body");
  ok &= contains(doc,
                 "Native `source_char_bones_alloc_reallocate_step` ports the concrete",
                 "document records concrete CharBonesAlloc ReallocateInternal slice");
  ok &= contains(doc,
                 "`source_char_bones_scale_add_clip_step` records that delegation",
                 "document records concrete CharBones ScaleAdd delegation slice");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharBonesBlender.cpp` is concrete",
                 "document cites latest CharBonesBlender source");
  ok &= contains(doc,
                 "Native `source_char_bones_enter_step` ports the inline `CharBones::Enter`",
                 "document records concrete CharBones Enter slice");
  ok &= contains(doc,
                 "Native `source_char_bones_blender_poll_step` ports `Poll`",
                 "document records concrete CharBonesBlender Poll slice");
  ok &= contains(doc,
                 "Native `source_char_bones_blender_set_dest_step` ports `SetDest`",
                 "document records concrete CharBonesBlender SetDest slice");
  ok &= contains(doc,
                 "Native `source_char_bones_blender_set_clip_type_step` ports `SetClipType`",
                 "document records concrete CharBonesBlender SetClipType slice");
  ok &= contains(doc,
                 "Native `source_char_bones_blender_reallocate_step` ports",
                 "document records concrete CharBonesBlender ReallocateInternal slice");
  ok &= contains(doc,
                 "it does not claim the missing low-level\n"
                 "    `CharBones::Blend` math",
                 "document fences missing CharBones Blend math");
  ok &= contains(doc,
                 "does not include a\n"
                 "  reviewable `Evaluate` or `Poll` body",
                 "document fences missing CharClipDriver runtime evaluator bodies");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "case'p':returnTYPE_POS;case's':returnTYPE_SCALE;"
                 "case'q':returnTYPE_QUAT;case'r':unsignedcharnext=p[3];",
                 "latest CharBones source maps source channel suffixes");
  ok &= contains(char_clip_h,
                 "intsource_char_bones_type_of(conststd::string&channel);",
                 "native API exposes source CharBones type helper");
  ok &= contains(char_clip_h,
                 "std::stringsource_char_bones_channel_name("
                 "conststd::string&name,inttype);",
                 "native API exposes source CharBones channel-name helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesLayout{std::array<int,"
                 "kSourceCharBonesTypeEnd+1>counts={};std::array<int,"
                 "kSourceCharBonesTypeEnd+1>offsets={};inttotal_size=0;};",
                 "native API exposes source CharBones layout row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesCompressionUpdate{intcompression=0;"
                 "SourceCharBonesLayoutlayout;boolchanged=false;};",
                 "native API exposes source CharBones compression update row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesBone{std::stringname;floatweight=1.0f;};",
                 "native API exposes source CharBones bone row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesState{intcompression=0;"
                 "SourceCharBonesLayoutlayout;"
                 "std::vector<SourceCharBonesBone>bones;};",
                 "native API exposes source CharBones state row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesSamplesState{SourceCharBonesStatebones;"
                 "intnum_samples=0;intpreview_sample=0;intstart_offset=0;"
                 "intraw_data_size=0;std::vector<float>frames;};",
                 "native API exposes source CharBonesSamples state row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesSampleStep{intstart_offset=0;"
                 "floatweight=0.0f;};",
                 "native API exposes source CharBonesSamples sample step row");
  ok &= contains(char_clip_h,
                 "SourceCharBonesLayoutsource_char_bones_recompute_layout("
                 "conststd::array<int,kSourceCharBonesTypeEnd+1>&counts,"
                 "intcompression);",
                 "native API exposes source CharBones recompute helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesCompressionUpdatesource_char_bones_set_compression("
                 "intcurrent_compression,constSourceCharBonesLayout&"
                 "current_layout,intrequested_compression);",
                 "native API exposes source CharBones SetCompression helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesStatesource_char_bones_empty_state();",
                 "native API exposes source CharBones empty-state helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bones_clear(SourceCharBonesState&state);",
                 "native API exposes source CharBones ClearBones helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bones_set_weights("
                 "std::vector<SourceCharBonesBone>&bones,floatweight);",
                 "native API exposes source CharBones static SetWeights helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bones_set_weights(SourceCharBonesState&state,"
                 "floatweight);",
                 "native API exposes source CharBones instance SetWeights helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bones_list_bones("
                 "constSourceCharBonesState&state,"
                 "std::vector<SourceCharBonesBone>&bones);",
                 "native API exposes source CharBones ListBones helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesSamplesState"
                 "source_char_bones_samples_empty_state();",
                 "native API exposes source CharBonesSamples empty-state helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bones_samples_set("
                 "SourceCharBonesSamplesState&samples,"
                 "constSourceCharBonesState&bones,intnum_samples,"
                 "intcompression);",
                 "native API exposes source CharBonesSamples Set helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesSamplesStatesource_char_bones_samples_clone("
                 "constSourceCharBonesSamplesState&source);",
                 "native API exposes source CharBonesSamples Clone helper");
  ok &= contains(char_clip_h,
                 "intsource_char_bones_samples_allocate_size("
                 "constSourceCharBonesSamplesState&samples);",
                 "native API exposes source CharBonesSamples AllocateSize helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_bones_samples_set_preview("
                 "SourceCharBonesSamplesState&samples,intrequested_sample);",
                 "native API exposes source CharBonesSamples SetPreview helper");
  ok &= contains(char_clip_h,
                 "std::vector<SourceCharBonesSampleStep>"
                 "source_char_bones_samples_split_steps("
                 "constSourceCharBonesSamplesState&samples,intsample,"
                 "floatweight,floatfrac);",
                 "native API exposes source CharBonesSamples split-step helper");
  ok &= contains(char_clip_h,
                 "intsource_char_bones_samples_rotate_by_offset("
                 "constSourceCharBonesSamplesState&samples,intsample);",
                 "native API exposes source CharBonesSamples RotateBy helper");
  ok &= contains(char_clip_h,
                 "std::vector<SourceCharBonesSampleStep>"
                 "source_char_bones_samples_rotate_to_steps("
                 "constSourceCharBonesSamplesState&samples,intsample,"
                 "floatangle,floatfrac);",
                 "native API exposes source CharBonesSamples RotateTo helper");
  ok &= contains(char_clip_h,
                 "std::vector<SourceCharBonesSampleStep>"
                 "source_char_bones_samples_scale_add_steps("
                 "constSourceCharBonesSamplesState&samples,intsample,"
                 "floatweight,floatfrac);",
                 "native API exposes source CharBonesSamples ScaleAddSample helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_bones_samples_set_ver_known(intversion);",
                 "native API exposes source CharBonesSamples SetVer helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_bones_samples_load_version_known(intversion);",
                 "native API exposes source CharBonesSamples load-version helper");
  ok &= contains(char_clip,
                 "intsource_char_bones_type_of(conststd::string&channel)",
                 "native clip decoder ports source CharBones type helper");
  ok &= contains(char_clip,
                 "for(size_tdot=channel.find('.');dot!=std::string::npos;"
                 "dot=channel.find('.',dot+1)){",
                 "native CharBones type helper scans all suffix dots");
  ok &= contains(char_clip,
                 "constcharaxis=channel[dot+4];if(axis>='x'&&axis<='z'){"
                 "returnkSourceCharBonesTypeRotX+"
                 "(axis-'x');}",
                 "native CharBones type helper maps source rot axes");
  ok &= contains(char_clip,
                 "returnc>=0&&c<kSourceCharBonesTypeEnd;",
                 "native clip decoder rejects non-source channel categories");
  ok &= contains(char_clip,
                 "std::stringsource_char_bones_channel_name("
                 "conststd::string&name,inttype)",
                 "native clip decoder ports source CharBones ChannelName helper");
  ok &= contains(char_clip,
                 "SourceCharBonesLayoutsource_char_bones_recompute_layout("
                 "conststd::array<int,kSourceCharBonesTypeEnd+1>&counts,"
                 "intcompression)",
                 "native clip decoder ports source CharBones RecomputeSizes helper");
  ok &= contains(char_clip,
                 "layout.offsets[type+1]=layout.offsets[type]+diff*size;",
                 "native CharBones layout helper accumulates source offsets");
  ok &= contains(char_clip,
                 "layout.total_size=(layout.offsets[kSourceCharBonesTypeEnd]+"
                 "0xF)&~0xF;",
                 "native CharBones layout helper aligns source total size");
  ok &= contains(char_clip,
                 "SourceCharBonesCompressionUpdatesource_char_bones_set_compression("
                 "intcurrent_compression,constSourceCharBonesLayout&"
                 "current_layout,intrequested_compression)",
                 "native clip decoder ports source CharBones SetCompression helper");
  ok &= contains(char_clip,
                 "if(requested_compression!=current_compression){"
                 "update.compression=requested_compression;update.layout="
                 "source_char_bones_recompute_layout(current_layout.counts,"
                 "requested_compression);update.changed=true;}",
                 "native CharBones SetCompression helper mirrors source guard");
  ok &= contains(char_clip,
                 "SourceCharBonesStatesource_char_bones_empty_state(){"
                 "returnSourceCharBonesState{};}",
                 "native CharBones empty-state helper mirrors source constructor state");
  ok &= contains(char_clip,
                 "voidsource_char_bones_clear(SourceCharBonesState&state){"
                 "state.bones.clear();state.layout=SourceCharBonesLayout{};"
                 "state.compression=0;}",
                 "native CharBones ClearBones helper mirrors source state reset");
  ok &= contains(char_clip,
                 "for(SourceCharBonesBone&bone:bones){bone.weight=weight;}",
                 "native CharBones SetWeights helper writes every source bone row");
  ok &= contains(char_clip,
                 "voidsource_char_bones_set_weights(SourceCharBonesState&state,"
                 "floatweight){source_char_bones_set_weights(state.bones,weight);}",
                 "native CharBones state SetWeights helper delegates to source vector form");
  ok &= contains(char_clip,
                 "voidsource_char_bones_list_bones("
                 "constSourceCharBonesState&state,"
                 "std::vector<SourceCharBonesBone>&bones){"
                 "for(constSourceCharBonesBone&bone:state.bones){"
                 "bones.push_back(bone);}}",
                 "native CharBones ListBones helper appends source rows in order");
  ok &= contains(char_clip,
                 "SourceCharBonesSamplesState"
                 "source_char_bones_samples_empty_state(){"
                 "returnSourceCharBonesSamplesState{};}",
                 "native CharBonesSamples empty-state helper mirrors source constructor");
  ok &= contains(char_clip,
                 "voidsource_char_bones_samples_set("
                 "SourceCharBonesSamplesState&samples,"
                 "constSourceCharBonesState&bones,intnum_samples,"
                 "intcompression){SourceCharBonesSamplesStatenext;"
                 "next.bones=bones;",
                 "native CharBonesSamples Set helper starts from prepared bones");
  ok &= contains(char_clip,
                 "SourceCharBonesCompressionUpdateupdate="
                 "source_char_bones_set_compression(next.bones.compression,"
                 "next.bones.layout,compression);",
                 "native CharBonesSamples Set helper mirrors source compression guard");
  ok &= contains(char_clip,
                 "next.num_samples=num_samples;next.raw_data_size="
                 "source_char_bones_samples_allocate_size(next);"
                 "samples=next;",
                 "native CharBonesSamples Set helper mirrors sample count and allocation");
  ok &= contains(char_clip,
                 "SourceCharBonesSamplesStatesource_char_bones_samples_clone("
                 "constSourceCharBonesSamplesState&source){"
                 "SourceCharBonesSamplesStateclone;"
                 "source_char_bones_samples_set(clone,source.bones,"
                 "source.num_samples,source.bones.compression);"
                 "clone.frames=source.frames;returnclone;}",
                 "native CharBonesSamples Clone helper mirrors Set then frames copy");
  ok &= contains(char_clip,
                 "intsource_char_bones_samples_allocate_size("
                 "constSourceCharBonesSamplesState&samples){return"
                 "samples.bones.layout.total_size*samples.num_samples;}",
                 "native CharBonesSamples AllocateSize helper mirrors source multiplication");
  ok &= contains(char_clip,
                 "boolsource_char_bones_samples_set_preview("
                 "SourceCharBonesSamplesState&samples,intrequested_sample){"
                 "if(samples.num_samples<=0)returnfalse;constintlast="
                 "samples.num_samples-1;constintclamped=std::max(0,"
                 "std::min(last,requested_sample));samples.preview_sample="
                 "clamped;samples.start_offset=samples.bones.layout.total_size*"
                 "clamped;returntrue;}",
                 "native CharBonesSamples SetPreview helper mirrors source offset");
  ok &= contains(char_clip,
                 "std::vector<SourceCharBonesSampleStep>"
                 "source_char_bones_samples_split_steps("
                 "constSourceCharBonesSamplesState&samples,intsample,"
                 "floatweight,floatfrac){std::vector<SourceCharBonesSampleStep>"
                 "steps;steps.push_back({samples.bones.layout.total_size*sample,"
                 "(1.0f-frac)*weight});if(frac>0.0f){steps.push_back({"
                 "samples.bones.layout.total_size*(sample+1),frac*weight});}"
                 "returnsteps;}",
                 "native CharBonesSamples split-step helper mirrors source row offsets");
  ok &= contains(char_clip,
                 "intsource_char_bones_samples_rotate_by_offset("
                 "constSourceCharBonesSamplesState&samples,intsample){"
                 "returnsamples.bones.layout.total_size*sample;}",
                 "native CharBonesSamples RotateBy helper mirrors source row offset");
  ok &= contains(char_clip,
                 "std::vector<SourceCharBonesSampleStep>"
                 "source_char_bones_samples_rotate_to_steps("
                 "constSourceCharBonesSamplesState&samples,intsample,"
                 "floatangle,floatfrac){return"
                 "source_char_bones_samples_split_steps(samples,sample,angle,"
                 "frac);}",
                 "native CharBonesSamples RotateTo helper delegates source split");
  ok &= contains(char_clip,
                 "std::vector<SourceCharBonesSampleStep>"
                 "source_char_bones_samples_scale_add_steps("
                 "constSourceCharBonesSamplesState&samples,intsample,"
                 "floatweight,floatfrac){return"
                 "source_char_bones_samples_split_steps(samples,sample,weight,"
                 "frac);}",
                 "native CharBonesSamples ScaleAddSample helper delegates source split");
  ok &= contains(char_clip,
                 "boolsource_char_bones_samples_set_ver_known(intversion){"
                 "returnversion<13;}",
                 "native CharBonesSamples SetVer helper mirrors source legacy range");
  ok &= contains(char_clip,
                 "boolsource_char_bones_samples_load_version_known(intversion){"
                 "returnversion>12&&version<=16;}",
                 "native CharBonesSamples load-version helper mirrors source range");
  ok &= contains(char_clip,
                 "uint32_tsamples_version=0;std::memcpy(&samples_version,d,4);"
                 "if(!source_char_bones_samples_load_version_known("
                 "static_cast<int>(samples_version))){return{};}",
                 "native clip parser applies source CharBonesSamples version gate");
  ok &= contains(char_clip,
                 "SourceCharBones::ChannelNameusesthefirstdot",
                 "native suffix strip follows source first-dot rule");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_type_of(\"bone_head.rotx\")",
                 "focused CharBones source test covers rot-x suffix");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_type_of(\"bone.head.pos\")",
                 "focused CharBones source test covers later-dot suffix scan");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_channel_name(\"bone.head.pos\",",
                 "focused CharBones source test covers first-dot replacement");
  ok &= contains(char_bones_source_test,
                 "for(intcompression=0;compression<=4;++compression)",
                 "focused CharBones source test covers all compression modes");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_recompute_layout(counts,0)",
                 "focused CharBones source test covers uncompressed layout");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_recompute_layout(counts,4)",
                 "focused CharBones source test covers compressed-all layout");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_set_compression(0,layout_none,0)",
                 "focused CharBones source test covers unchanged compression");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_set_compression(0,layout_none,4)",
                 "focused CharBones source test covers changed compression");
  ok &= contains(char_bones_source_test,
                 "expect_empty_state(source_char_bones_empty_state(),",
                 "focused CharBones source test covers constructor state");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_set_weights(state,0.5f);",
                 "focused CharBones source test covers state SetWeights");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_list_bones(state,listed);",
                 "focused CharBones source test covers ListBones append behavior");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_add_bones_steps(listed)",
                 "focused CharBones source test covers AddBones wrapper");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_add_bones_steps({})",
                 "focused CharBones source test covers empty AddBones reallocation");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_find_offset(lookup_state,\"bone_b.quat\")",
                 "focused CharBones source test covers FindOffset type bucket");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_find_offset(lookup_state,\"bone_missing.pos\")",
                 "focused CharBones source test covers missing FindOffset row");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_find_ptr(lookup_state,\"bone_b.quat\")",
                 "focused CharBones source test covers FindPtr hit");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_zero(raw_bytes,lookup_state.layout.total_size)",
                 "focused CharBones source test covers Zero byte span");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_scale_add_clip_step(0.25f,12.5f,0.75f)",
                 "focused CharBones source test covers ScaleAdd delegation args");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_alloc_reallocate_step("
                 "lookup_state.layout.total_size)",
                 "focused CharBones source test covers CharBonesAlloc realloc step");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_enter_step()",
                 "focused CharBones source test covers Enter sequence");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_blender_poll_step(false,true)",
                 "focused CharBones source test covers active CharBonesBlender Poll");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_blender_set_dest_step(true,true)",
                 "focused CharBones source test covers CharBonesBlender SetDest add");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_blender_set_clip_type_step(true)",
                 "focused CharBones source test covers CharBonesBlender SetClipType");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_blender_reallocate_step(true)",
                 "focused CharBones source test covers CharBonesBlender realloc add");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_clear(state);",
                 "focused CharBones source test covers ClearBones reset");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_set_weights(bones,0.0f);",
                 "focused CharBones source test covers static SetWeights");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_empty_state()",
                 "focused CharBones source test covers CharBonesSamples constructor state");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_set(samples,prepared_bones,3,"
                 "kCompressRots);",
                 "focused CharBones source test covers CharBonesSamples Set helper");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_clone(samples)",
                 "focused CharBones source test covers CharBonesSamples Clone helper");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_allocate_size(samples)",
                 "focused CharBones source test covers CharBonesSamples AllocateSize");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_set_preview(samples,99)",
                 "focused CharBones source test covers CharBonesSamples preview clamp");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_split_steps(samples,1,1.0f,0.25f)",
                 "focused CharBones source test covers CharBonesSamples split steps");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_rotate_to_steps(samples,2,2.0f,"
                 "0.25f)",
                 "focused CharBones source test covers CharBonesSamples RotateTo helper");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_scale_add_steps(samples,0,0.5f,"
                 "0.0f)",
                 "focused CharBones source test covers CharBonesSamples ScaleAddSample helper");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_load_version_known(12)",
                 "focused CharBones source test covers low sample version reject");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_load_version_known(16)",
                 "focused CharBones source test covers high accepted sample version");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_set_ver_known(12)",
                 "focused CharBones source test covers CharBonesSamples SetVer legacy accept");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_samples_set_ver_known(13)",
                 "focused CharBones source test covers CharBonesSamples SetVer source reject");
  ok &= missing(char_clip, "GHOGX_AXIS_ROT_NO_PI",
                "old no-pi axis-rotation diagnostic removed from decoder");
  ok &= missing(char_clip, "GHOGX_FILE_ORDER_CLIP_SAMPLES",
                "old file-order sample diagnostic removed from decoder");
  ok &= missing(char_clip, ".d?x",
                "old native-only d-axis channel category removed");
  ok &= missing(char_clip, "bl.cats[bi]>=3&&bl.cats[bi]<=8",
                "native clip decoder no longer accepts non-source rot categories");
  ok &= missing(char_clip, "GHOGX_RELATIVE_FACE_QUAT",
                "old broad relative face-quat diagnostic removed");
  ok &= missing(char_clip, "GHOGX_RELATIVE_CLIP_QUAT",
                "old broad relative clip-quat diagnostic removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_FINGER_CLIPS",
                "old finger-channel drop diagnostic removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_AXIS_ROT_CHANNELS",
                "old scalar-axis channel drop diagnostic removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_THIGH_QUATS",
                "old thigh-quat drop diagnostic removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_FOOT_QUATS",
                "old foot-quat drop diagnostic removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_LEG_AXIS",
                "old leg-axis drop diagnostic removed");
  ok &= missing(char_clip, "GHOGX_RELATIVE_THIGH_QUAT",
                "old relative thigh-quat diagnostic removed");
  ok &= missing(char_clip, "GHOGX_PRE_RELATIVE_THIGH_QUAT",
                "old pre-relative thigh-quat diagnostic removed");
  ok &= missing(char_clip, "GHOGX_SWAP_THIGH_QUATS",
                "old thigh-quat swap diagnostic removed");
  ok &= missing(char_clip, "GHOGX_INVERT_THIGH_QUATS",
                "old thigh-quat invert diagnostic removed");
  ok &= missing(char_clip, "GHOGX_PRE_RELATIVE_CLIP_QUAT",
                "old pre-relative clip-quat diagnostic removed");
  ok &= missing(char_clip, "GHOGX_WORLD_CLIP_QUAT",
                "old world clip-quat diagnostic removed");
  ok &= missing(char_clip, "GHOGX_TRANSPOSE_CLIP_QUAT",
                "old transpose clip-quat diagnostic removed");
  ok &= missing(format_notes,
                "GHOGX_DISABLE_AXIS_ROT_CHANNELS=1` remains",
                "format notes must not describe removed axis-drop diagnostic as live");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "intCharBones::TypeSize(inti)const{if(i<2){if("
                 "mCompression<kCompressVects)return0xC;elsereturn6;}",
                 "latest CharBones source defines packed vector channel sizes");
  ok &= contains(rb3_latest_char_bones_h,
                 "enumCompressionType{kCompressNone,kCompressRots,"
                 "kCompressVects,kCompressQuats,kCompressAll};",
                 "latest CharBones source exposes full compression enum");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "if(i!=2){if(mCompression==kCompressNone)return4;"
                 "elsereturn2;}if(mCompression>kCompressVects)return4;"
                 "if(mCompression==kCompressNone)return0x10;return8;}",
                 "latest CharBones source defines packed rot/quat channel sizes");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::RecomputeSizes(){mPosOffset=0;for(inti=0;"
                 "i<NUM_TYPES;i++){intdiff=mCounts[i+1]-mCounts[i];"
                 "mOffsets[i+1]=mOffsets[i]+diff*TypeSize(i);}mTotalSize="
                 "mEndOffset+0xFU&0xFFFFFFF0;}",
                 "latest CharBones source defines packed layout offsets");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::SetCompression(CompressionTypety){if(ty!="
                 "mCompression){mCompression=ty;RecomputeSizes();}}",
                 "latest CharBones source defines SetCompression guard");
  ok &= contains(rb3_latest_char_bones_h,
                 "Bone():name(),weight(1.0f){}",
                 "latest CharBones source defines default bone weight");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "CharBones::CharBones():mCompression(kCompressNone),"
                 "mStart(0),mTotalSize(0){for(inti=0;i<NUM_TYPES;i++){"
                 "mCounts[i]=0;mOffsets[i]=0;}}",
                 "latest CharBones source defines default state");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "for(constchar*p=s.Str();p!=0;p++){if(*p=='.'){",
                 "latest CharBones source TypeOf scans symbol dots");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::ClearBones(){mBones.clear();for(inti=0;"
                 "i<NUM_TYPES;i++){mCounts[i]=0;mOffsets[i]=0;}mTotalSize=0;"
                 "mCompression=kCompressNone;ReallocateInternal();}",
                 "latest CharBones source defines ClearBones state reset");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::SetWeights(floatf){SetWeights(f,mBones);}",
                 "latest CharBones source defines instance SetWeights delegation");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::SetWeights(floatwt,std::vector<Bone>&bones){"
                 "for(inti=0;i<bones.size();i++){bones[i].weight=wt;}}",
                 "latest CharBones source defines static SetWeights row write");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::ListBones(std::list<Bone>&bones)const{"
                 "for(inti=0;i<mBones.size();i++){bones.push_back(mBones[i]);}}",
                 "latest CharBones source defines ListBones append loop");
  ok &= contains(rb3_latest_char_bones_h,
                 "voidAddBoneInternal(constBone&);",
                 "latest CharBones header declares AddBoneInternal");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::AddBones(conststd::vector<Bone>&vec){"
                 "for(std::vector<Bone>::const_iteratorit=vec.begin();"
                 "it!=vec.end();++it){AddBoneInternal(*it);}ReallocateInternal();}",
                 "latest CharBones source defines vector AddBones wrapper");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::AddBones(conststd::list<Bone>&bones){"
                 "for(std::list<Bone>::const_iteratorit=bones.begin();"
                 "it!=bones.end();++it){AddBoneInternal(*it);}ReallocateInternal();}",
                 "latest CharBones source defines list AddBones wrapper");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "intCharBones::FindOffset(Symbols)const{Typety=TypeOf(s);"
                 "intnextcount=mCounts[ty+1];intsize=TypeSize(ty);",
                 "latest CharBones source defines FindOffset type setup");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "intcount=mCounts[ty];intoffset=mOffsets[ty];for(inti=0;"
                 "i<nextcount-count;i++){",
                 "latest CharBones source defines FindOffset type range loop");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "elseoffset+=size;}return-1;}",
                 "latest CharBones source defines FindOffset offset advance");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "void*CharBones::FindPtr(Symbols)const{intoffset=FindOffset(s);"
                 "if(offset==-1)return0;elsereturn(void*)&mStart[offset];}",
                 "latest CharBones source defines FindPtr pointer decision");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::Zero(){memset(mStart,0,mTotalSize);}",
                 "latest CharBones source defines Zero byte span");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBonesAlloc::ReallocateInternal(){_MemFree(mStart);"
                 "mStart=(char*)_MemAlloc(mTotalSize,0);}",
                 "latest CharBonesAlloc source defines raw storage reallocation");
  ok &= contains(rb3_latest_char_bones_h,
                 "voidEnter(){Zero();SetWeights(0);}",
                 "latest CharBones header defines Enter sequence");
  ok &= contains(rb3_latest_char_bones_blender_h,
                 "classCharBonesBlender:publicCharPollable,publicCharBonesAlloc",
                 "latest CharBonesBlender header defines source inheritance");
  ok &= contains(rb3_latest_char_bones_blender_cpp,
                 "voidCharBonesBlender::Enter(){CharBones::Enter();}",
                 "latest CharBonesBlender source delegates Enter");
  ok &= contains(rb3_latest_char_bones_blender_cpp,
                 "voidCharBonesBlender::Poll(){if(mBones.empty()||!mDest)"
                 "return;Blend(*mDest);CharBones::Enter();}",
                 "latest CharBonesBlender source defines Poll call flow");
  ok &= contains(rb3_latest_char_bones_blender_cpp,
                 "voidCharBonesBlender::SetDest(CharBonesObject*obj){if(obj!="
                 "mDest){mDest=obj;if(mDest)mDest->AddBones(mBones);}}",
                 "latest CharBonesBlender source defines SetDest call flow");
  ok &= contains(rb3_latest_char_bones_blender_cpp,
                 "voidCharBonesBlender::SetClipType(Symbols){if(s!=mClipType){"
                 "mClipType=s;ClearBones();CharBoneDir::StuffBones(*this,"
                 "mClipType);}}",
                 "latest CharBonesBlender source defines SetClipType call flow");
  ok &= contains(rb3_latest_char_bones_blender_cpp,
                 "voidCharBonesBlender::ReallocateInternal(){"
                 "CharBonesAlloc::ReallocateInternal();if(mDest)"
                 "mDest->AddBones(mBones);CharBones::Enter();}",
                 "latest CharBonesBlender source defines ReallocateInternal flow");
  ok &= contains(char_clip,
                 "kSourceCompressAll=4",
                 "native clip decoder names source compression mode 4");
  ok &= contains(char_clip,
                 "compression<=kSourceCompressAll",
                 "native clip decoder accepts the source compression enum range");
  ok &= contains(char_clip,
                 "if(compression>2)return4u;if(compression==0)return16u;"
                 "return8u;",
                 "native clip decoder keeps source byte-quat size");
  ok &= contains(char_clip_h,
                 "intsource_char_bones_find_offset(constSourceCharBonesState&state,"
                 "conststd::string&channel);",
                 "native exposes CharBones FindOffset helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesFindPtrResult{boolfound=false;intoffset=-1;};",
                 "native exposes CharBones FindPtr decision row");
  ok &= contains(char_clip_h,
                 "SourceCharBonesFindPtrResultsource_char_bones_find_ptr("
                 "constSourceCharBonesState&state,conststd::string&channel);",
                 "native exposes CharBones FindPtr helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bones_zero(std::vector<uint8_t>&start,"
                 "inttotal_size);",
                 "native exposes CharBones Zero helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesScaleAddClipStepsource_char_bones_scale_add_clip_step("
                 "floatf1,floatf2,floatf3);",
                 "native exposes CharBones ScaleAdd delegation helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesAddBonesSteps{std::vector<"
                 "SourceCharBonesBone>add_bone_internal_calls;"
                 "boolreallocate_internal=false;};",
                 "native exposes CharBones AddBones wrapper row");
  ok &= contains(char_clip_h,
                 "SourceCharBonesAddBonesStepssource_char_bones_add_bones_steps("
                 "conststd::vector<SourceCharBonesBone>&bones);",
                 "native exposes CharBones AddBones wrapper helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesAllocReallocateStep{boolfree_m_start=true;"
                 "intmem_alloc_size=0;boolassign_m_start=true;};",
                 "native exposes CharBonesAlloc reallocation row");
  ok &= contains(char_clip_h,
                 "SourceCharBonesAllocReallocateStep"
                 "source_char_bones_alloc_reallocate_step(inttotal_size);",
                 "native exposes CharBonesAlloc reallocation helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesEnterStep{boolzero=true;"
                 "boolset_weights=true;floatset_weights_value=0.0f;};",
                 "native exposes CharBones Enter row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesBlenderPollStep{boolearly_out=false;"
                 "boolblend_dest=false;boolenter=false;};",
                 "native exposes CharBonesBlender Poll row");
  ok &= contains(char_clip_h,
                 "SourceCharBonesEnterStepsource_char_bones_enter_step();",
                 "native exposes CharBones Enter helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesBlenderPollStepsource_char_bones_blender_poll_step("
                 "boolbones_empty,boolhas_dest);",
                 "native exposes CharBonesBlender Poll helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesBlenderSetDestStep"
                 "source_char_bones_blender_set_dest_step("
                 "booldest_changed,boolnew_dest_exists);",
                 "native exposes CharBonesBlender SetDest helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesBlenderSetClipTypeStep"
                 "source_char_bones_blender_set_clip_type_step("
                 "boolclip_type_changed);",
                 "native exposes CharBonesBlender SetClipType helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesBlenderReallocateStep"
                 "source_char_bones_blender_reallocate_step(boolhas_dest);",
                 "native exposes CharBonesBlender ReallocateInternal helper");
  ok &= contains(char_clip,
                 "intsource_char_bones_find_offset(constSourceCharBonesState&state,"
                 "conststd::string&channel){constinttype=source_char_bones_type_of"
                 "(channel);",
                 "native CharBones FindOffset helper exists");
  ok &= contains(char_clip,
                 "constintnext_count=state.layout.counts[type+1];"
                 "constintcount=state.layout.counts[type];",
                 "native CharBones FindOffset uses source cumulative range");
  ok &= contains(char_clip,
                 "state.bones[static_cast<size_t>(bone_index)].name==channel)"
                 "{returnoffset;}",
                 "native CharBones FindOffset returns matching source row offset");
  ok &= contains(char_clip,
                 "SourceCharBonesFindPtrResultsource_char_bones_find_ptr("
                 "constSourceCharBonesState&state,conststd::string&channel){"
                 "SourceCharBonesFindPtrResultresult;result.offset="
                 "source_char_bones_find_offset(state,channel);result.found="
                 "result.offset!=-1;returnresult;}",
                 "native CharBones FindPtr helper mirrors source pointer decision");
  ok &= contains(char_clip,
                 "voidsource_char_bones_zero(std::vector<uint8_t>&start,"
                 "inttotal_size){std::fill(start.begin(),start.begin()+"
                 "total_size,uint8_t{0});}",
                 "native CharBones Zero helper mirrors source memset span");
  ok &= contains(char_clip,
                 "SourceCharBonesScaleAddClipStep"
                 "source_char_bones_scale_add_clip_step(floatf1,floatf2,floatf3){"
                 "SourceCharBonesScaleAddClipStepstep;step.f1=f1;step.f2=f2;"
                 "step.f3=f3;returnstep;}",
                 "native CharBones ScaleAdd helper preserves source arguments");
  ok &= contains(char_clip,
                 "SourceCharBonesAddBonesStepssource_char_bones_add_bones_steps("
                 "conststd::vector<SourceCharBonesBone>&bones){"
                 "SourceCharBonesAddBonesStepssteps;steps.add_bone_internal_calls="
                 "bones;steps.reallocate_internal=true;returnsteps;}",
                 "native CharBones AddBones helper mirrors source wrapper calls");
  ok &= contains(char_clip,
                 "SourceCharBonesAllocReallocateStep"
                 "source_char_bones_alloc_reallocate_step(inttotal_size){"
                 "SourceCharBonesAllocReallocateStepstep;step.mem_alloc_size="
                 "total_size;returnstep;}",
                 "native CharBonesAlloc helper mirrors source allocation size");
  ok &= contains(char_clip,
                 "SourceCharBonesEnterStepsource_char_bones_enter_step(){"
                 "returnSourceCharBonesEnterStep{};}",
                 "native CharBones Enter helper mirrors source sequence");
  ok &= contains(char_clip,
                 "if(bones_empty||!has_dest){step.early_out=true;returnstep;}"
                 "step.blend_dest=true;step.enter=true;returnstep;",
                 "native CharBonesBlender Poll helper mirrors source branch");
  ok &= contains(char_clip,
                 "if(!dest_changed)returnstep;step.changed=true;"
                 "step.assign_dest=true;step.add_bones_to_dest=new_dest_exists;",
                 "native CharBonesBlender SetDest helper mirrors source branch");
  ok &= contains(char_clip,
                 "if(!clip_type_changed)returnstep;step.changed=true;"
                 "step.assign_clip_type=true;step.clear_bones=true;"
                 "step.stuff_bones_from_dir=true;",
                 "native CharBonesBlender SetClipType helper mirrors source branch");
  ok &= contains(char_clip,
                 "SourceCharBonesBlenderReallocateStep"
                 "source_char_bones_blender_reallocate_step(boolhas_dest){"
                 "SourceCharBonesBlenderReallocateStepstep;"
                 "step.add_bones_to_dest=has_dest;returnstep;}",
                 "native CharBonesBlender ReallocateInternal helper mirrors source flow");
  ok &= contains(char_clip, "offset+=type_size;",
                 "native CharBones FindOffset advances source packed offsets");
  ok &= contains(char_clip,
                 "if(uses_source_byte_quat(out))returnfalse;",
                 "native clip decoder refuses byte-quat lists until source conversion body exists");
  ok &= contains(char_clip,
                 "source_char_bones_compression_name",
                 "native clip decoder names source compression modes for logs");
  ok &= contains(char_clip,
                 "\"[clip-source-bones]list=%zucomp=%d(%s)samples=%d\"",
                 "native clip decoder logs source CharBonesSamples list inventory");
  ok &= contains(char_clip,
                 "\"[clip-source-bones-counts]list=%zuvec=%dquat=%d\"",
                 "native clip decoder logs source CharBonesSamples channel counts");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_clip_auditchar_clip_audit.cpp)",
                 "CMake builds focused character clip audit helper");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_clip_driver_flags_test"
                 "character_clip_driver_flags_test.cpp)",
                 "CMake builds focused CharClipDriver flag-mask test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_char_bones_source_test"
                 "character_char_bones_source_test.cpp)",
                 "CMake builds focused CharBones source helper test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_char_utl_source_test"
                 "character_char_utl_source_test.cpp)",
                 "CMake builds focused CharUtl source helper test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_ik_rod_source_test"
                 "character_ik_rod_source_test.cpp)",
                 "CMake builds focused CharIKRod source test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_bone_offset_source_test"
                 "character_bone_offset_source_test.cpp)",
                 "CMake builds focused CharBoneOffset source test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_bone_twist_source_test"
                 "character_bone_twist_source_test.cpp)",
                 "CMake builds focused CharBoneTwist source test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_lookat_source_test"
                 "character_lookat_source_test.cpp)",
                 "CMake builds focused CharLookAt source test");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_weight_setter_source_test"
                 "character_weight_setter_source_test.cpp)",
                 "CMake builds focused CharWeightSetter source test");
  ok &= contains(char_clip_audit,
                 "for(constauto&entry:ark.entries()){if(!ends_with("
                 "entry.full_path,\".milo_ps2\"))continue;",
                 "clip audit expands ARK prefixes into MILO entries");
  ok &= contains(char_clip_audit,
                 "if(de.type!=\"CharClipSamples\")continue;",
                 "clip audit restricts row inventory to CharClipSamples");
  ok &= contains(char_clip_audit,
                 "ghogx::character::load_clip(hdr_path,ark_path,resolved,de.name)",
                 "clip audit reuses bounded native clip decoder by exact row name");
  ok &= contains(char_clip_audit,
                 "\"[clip-audit-milo]milo=%sresolved=%sdir=%sclips=%d\"",
                 "clip audit emits MILO-level clip counts");
  ok &= contains(char_clip_audit,
                 "\"accepted=%dframes=%zuchannels0=%zuoutputBones=%zu\\n\"",
                 "clip audit emits accepted frame/channel/output-bone evidence");
  ok &= missing(char_clip, "out.compression>3",
                "native clip decoder no longer caps source compression at mode 3");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev<9){RndTransformableRemoverremover;"
                 "remover.Load(bs);}",
                 "latest CharBone source loads legacy transform remover");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev>6)bs>>mPositionContext;else{boolb;bs>>b;"
                 "mPositionContext=b;}",
                 "latest CharBone source gates position context");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev>6)bs>>mScaleContext;elseif(gRev>1){boolb;"
                 "bs>>b;mScaleContext=b;}",
                 "latest CharBone source gates scale context");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "bs>>(int&)mRotation;if(gRev<5){inti;bs>>i;}",
                 "latest CharBone source reads rotation and legacy rev5 int");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev<2){mScaleContext=0;mRotation=(CharBones::Type)"
                 "(mRotation+1);}",
                 "latest CharBone source applies legacy rotation and scale defaults");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev<5&&mRotation>CharBones::TYPE_END){"
                 "mRotation=CharBones::TYPE_END;}",
                 "latest CharBone source clamps legacy rotation type");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev>6)bs>>mRotationContext;elsemRotationContext="
                 "mRotation!=CharBones::TYPE_END;",
                 "latest CharBone source gates rotation context");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev==6){intctx;bs>>ctx;if(mPositionContext!=0)"
                 "mPositionContext=ctx;if(mScaleContext!=0)mScaleContext=ctx;"
                 "if(mRotationContext!=0)mRotationContext=ctx;}",
                 "latest CharBone source applies revision-6 shared context");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(gRev>7)bs>>mWeights;if(gRev>8)bs>>mTrans;"
                 "if(gRev>9)bs>>mBakeOutAsTopLevel;",
                 "latest CharBone source gates weights, trans, and bake flag");
  ok &= contains(rb3_latest_char_bone_h,
                 "ObjPtr<RndTransformable,ObjectDir>mTrans;",
                 "latest CharBone header exposes source trans pointer field");
  ok &= contains(trans_cs,
                 "if(revision>6)constraint=(Constraint)reader.ReadUInt32();"
                 "if(revision>5)target=Symbol.Read(reader);if(revision>6)"
                 "preserveScale=reader.ReadBoolean();parentObj=Symbol.Read(reader);",
                 "MiloEditor RndTrans source defines embedded transform row order");
  ok &= contains(char_clip_h,
                 "uint32_tchar_bone_version=0;uint32_ttrans_version=0;"
                 "uint32_ttrans_constraint=0;",
                 "native OutputBone carries source CharBone and transform revisions");
  ok &= contains(char_clip_h,
                 "int32_tposition_context=0;int32_tscale_context=0;"
                 "int32_trotation_type=6;",
                 "native OutputBone carries source CharBone contexts");
  ok &= contains(char_clip_h,
                 "std::vector<WeightContext>weights;std::stringtrans;"
                 "boolbake_out_as_top_level=false;size_tunread_bytes=0;",
                 "native OutputBone carries source CharBone optional tail fields");
  ok &= contains(char_clip_h,
                 "structSourceCharBoneLoadPlan{boolknown_revision=false;"
                 "std::vector<std::string>read_order;std::vector<std::string>"
                 "branches;};",
                 "native API exposes source CharBone load plan row");
  ok &= contains(char_clip_h,
                 "structSourceCharBoneCopyPlan{std::vector<std::string>"
                 "copied_superclasses;std::vector<std::string>copied_members;};",
                 "native API exposes source CharBone copy plan row");
  ok &= contains(char_clip_h,
                 "SourceCharBoneLoadPlansource_char_bone_load_plan("
                 "int32_trevision);",
                 "native API exposes source CharBone load plan helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneCopyPlansource_char_bone_copy_plan();",
                 "native API exposes source CharBone copy plan helper");
  ok &= contains(char_clip_h,
                 "std::optional<size_t>source_char_bone_find_weight_index("
                 "constCharClip::OutputBone&bone,intcontext_mask);",
                 "native API exposes source CharBone FindWeight helper");
  ok &= contains(char_clip_h,
                 "floatsource_char_bone_get_weight("
                 "constCharClip::OutputBone&bone,intcontext_mask);",
                 "native API exposes source CharBone GetWeight helper");
  ok &= contains(char_clip_h,
                 "CharClip::OutputBonesource_char_bone_copy_members("
                 "constCharClip::OutputBone&source);",
                 "native API exposes source CharBone copy-member helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bone_clear_context(CharClip::OutputBone&bone,"
                 "intcontext_mask);",
                 "native API exposes source CharBone ClearContext helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bone_stuff_bones("
                 "constCharClip::OutputBone&bone,intcontext_mask,"
                 "std::vector<SourceCharBonesBone>&bones);",
                 "native API exposes source CharBone StuffBones helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneDirDefaultState"
                 "source_char_bone_dir_default_state();",
                 "native API exposes source CharBoneDir default-state helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneDirLoadPlan"
                 "source_char_bone_dir_load_plan(int32_trevision);",
                 "native API exposes source CharBoneDir load plan helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneDirCopyPlan"
                 "source_char_bone_dir_copy_plan();",
                 "native API exposes source CharBoneDir copy plan helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_bone_dir_list_bones("
                 "conststd::vector<CharClip::OutputBone>&output_bones,"
                 "intmove_context,intcontext_mask,boolinclude_delta_facing,"
                 "std::vector<SourceCharBonesBone>&bones);",
                 "native API exposes source CharBoneDir ListBones helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBoneDirClipTypeResource{std::stringclip_type;"
                 "boolhas_resource=false;std::stringresource_name;"
                 "intcontext_mask=0;boolresource_found=false;"
                 "std::stringcontext_symbol;};",
                 "native API exposes source CharBoneDir clip type resource row");
  ok &= contains(char_clip_h,
                 "structSourceCharBoneDirResourceLookupResult{"
                 "boolclip_type_found=false;boolresource_field_found=false;"
                 "boolresource_found=false;std::stringresource_name;"
                 "intcontext_mask=0;std::stringwarning;};",
                 "native API exposes source CharBoneDir resource lookup row");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_bone_dir_get_clip_types("
                 "conststd::vector<SourceCharBoneDirClipTypeResource>&clip_types);",
                 "native API exposes source CharBoneDir GetClipTypes helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneDirResourceLookupResult"
                 "source_char_bone_dir_find_resource_from_clip_type("
                 "conststd::vector<SourceCharBoneDirClipTypeResource>&"
                 "clip_types,conststd::string&clip_type);",
                 "native API exposes source CharBoneDir resource lookup helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneDirStuffBonesSymbolStep"
                 "source_char_bone_dir_stuff_bones_symbol_step("
                 "conststd::vector<SourceCharBoneDirClipTypeResource>&"
                 "clip_types,conststd::string&clip_type);",
                 "native API exposes source CharBoneDir Symbol StuffBones helper");
  ok &= contains(char_clip_h,
                 "SourceCharBoneDirContextFlagsStep"
                 "source_char_bone_dir_get_context_flags_step(",
                 "native API exposes source CharBoneDir GetContextFlags helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_bone_dir_sync_filter(",
                 "native API exposes source CharBoneDir SyncFilter helper");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesMeshesReplaceStep{boolobject_replace=true;"
                 "boolscan_meshes=false;intreplaced_index=-1;"
                 "boolassigned_dummy=false;std::vector<std::string>meshes;};",
                 "native API exposes source CharBonesMeshes Replace row");
  ok &= contains(char_clip_h,
                 "structSourceCharBonesMeshesReallocateStep{"
                 "boolchar_bones_alloc_reallocate_internal=true;"
                 "std::vector<std::string>meshes;std::vector<std::string>"
                 "missing_non_facing_bones;boolacquire_pose=false;};",
                 "native API exposes source CharBonesMeshes Reallocate row");
  ok &= contains(char_clip_h,
                 "SourceCharBonesMeshesReplaceStep"
                 "source_char_bones_meshes_replace_step("
                 "conststd::vector<std::string>&meshes,conststd::string&from,"
                 "conststd::string&to,boolto_is_transformable,"
                 "conststd::string&dummy_mesh);",
                 "native API exposes source CharBonesMeshes Replace helper");
  ok &= contains(char_clip_h,
                 "SourceCharBonesMeshesReallocateStep"
                 "source_char_bones_meshes_reallocate_step("
                 "conststd::vector<SourceCharBonesBone>&bones,"
                 "conststd::unordered_map<std::string,std::string>&"
                 "transform_lookup,conststd::string&dummy_mesh);",
                 "native API exposes source CharBonesMeshes Reallocate helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_bones_meshes_stuff_meshes("
                 "conststd::vector<std::string>&existing_objects,"
                 "conststd::vector<std::string>&meshes);",
                 "native API exposes source CharBonesMeshes StuffMeshes helper");
  ok &= contains(char_clip,
                 "out.char_bone_version=read_u32_at(body,size,pos,"
                 "\"CharBoneversion\");skip_bytes_at(body,size,pos,9,"
                 "\"CharBoneobjectfields\");if(out.char_bone_version<9)",
                 "native CharBone output decoder reads source object prefix and rev gate");
  ok &= contains(char_clip,
                 "out.trans_constraint=read_u32_at(body,size,pos,"
                 "\"RndTransformableconstraint\");",
                 "native CharBone output decoder reads embedded transform constraint");
  ok &= contains(char_clip,
                 "out.position_context=read_u8_at(body,size,pos,"
                 "\"CharBonelegacypositioncontext\")?1:0;",
                 "native CharBone output decoder follows legacy position bool gate");
  ok &= contains(char_clip,
                 "out.rotation_type=read_i32_at(body,size,pos,"
                 "\"CharBonerotationtype\");",
                 "native CharBone output decoder reads source rotation type");
  ok &= contains(char_clip,
                 "out.unread_bytes=size-pos;",
                 "native CharBone output decoder records unread byte proof");
  ok &= contains(char_clip,
                 "SourceCharBoneLoadPlansource_char_bone_load_plan("
                 "int32_trevision){SourceCharBoneLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=10;",
                 "native CharBone load plan helper ports revision gate");
  ok &= contains(char_clip,
                 "if(revision<9)plan.read_order.push_back("
                 "\"RndTransformableRemover\");",
                 "native CharBone load plan records legacy transform remover");
  ok &= contains(char_clip,
                 "plan.branches.push_back(\"mScaleContext=0\");"
                 "plan.branches.push_back(\"mRotation=mRotation+1\");",
                 "native CharBone load plan records legacy rotation defaults");
  ok &= contains(char_clip,
                 "if(revision==6){plan.read_order.push_back(\"sharedContext\");"
                 "plan.branches.push_back(\"nonzeroContextsUseSharedContext\");}",
                 "native CharBone load plan records revision-6 shared context");
  ok &= contains(char_clip,
                 "SourceCharBoneCopyPlansource_char_bone_copy_plan(){"
                 "SourceCharBoneCopyPlanplan;plan.copied_superclasses={"
                 "\"Hmx::Object\"};",
                 "native CharBone copy plan helper records superclass");
  ok &= contains(char_clip,
                 "CharClip::OutputBonesource_char_bone_copy_members("
                 "constCharClip::OutputBone&source){CharClip::OutputBonedest;"
                 "dest.rotation_context=source.rotation_context;dest."
                 "scale_context=source.scale_context;dest.position_context="
                 "source.position_context;dest.rotation_type=source."
                 "rotation_type;dest.target=source.target;dest.weights="
                 "source.weights;dest.trans=source.trans;dest."
                 "bake_out_as_top_level=source.bake_out_as_top_level;"
                 "returndest;}",
                 "native CharBone copy-member helper mirrors source COPY_MEMBER list");
  ok &= contains(char_clip,
                 "std::optional<size_t>source_char_bone_find_weight_index("
                 "constCharClip::OutputBone&bone,intcontext_mask){"
                 "for(size_ti=0;i<bone.weights.size();++i){if(("
                 "bone.weights[i].context&context_mask)!=0)returni;}"
                 "returnstd::nullopt;}",
                 "native CharBone FindWeight helper mirrors source first match");
  ok &= contains(char_clip,
                 "floatsource_char_bone_get_weight("
                 "constCharClip::OutputBone&bone,intcontext_mask){"
                 "conststd::optional<size_t>index="
                 "source_char_bone_find_weight_index(bone,context_mask);"
                 "if(index)returnbone.weights[*index].weight;return1.0f;}",
                 "native CharBone GetWeight helper mirrors source default");
  ok &= contains(char_clip,
                 "voidsource_char_bone_clear_context(CharClip::OutputBone&bone,"
                 "intcontext_mask){constintmask=~context_mask;"
                 "bone.position_context&=mask;bone.scale_context&=mask;"
                 "bone.rotation_context&=mask;}",
                 "native CharBone ClearContext helper mirrors source mask");
  ok &= contains(char_clip,
                 "voidsource_char_bone_stuff_bones("
                 "constCharClip::OutputBone&bone,intcontext_mask,"
                 "std::vector<SourceCharBonesBone>&bones){if(("
                 "bone.position_context&context_mask)!=0){bones.push_back({",
                 "native CharBone StuffBones helper mirrors source append gate");
  ok &= contains(char_clip,
                 "SourceCharBoneDirDefaultStatesource_char_bone_dir_default_state(){"
                 "returnSourceCharBoneDirDefaultState{};}",
                 "native CharBoneDir default helper preserves constructor defaults");
  ok &= contains(char_clip,
                 "SourceCharBoneDirLoadPlansource_char_bone_dir_load_plan("
                 "int32_trevision){SourceCharBoneDirLoadPlanplan;"
                 "plan.known_revision=revision>=0&&revision<=4;",
                 "native CharBoneDir load plan helper ports revision gate");
  ok &= contains(char_clip,
                 "plan.preload_order={\"LOAD_REVS\","
                 "\"PushRev(packRevs(gAltRev,gRev))\","
                 "\"ObjectDir::PreLoad\"};",
                 "native CharBoneDir load plan records PreLoad order");
  ok &= contains(char_clip,
                 "if(revision<2){plan.postload_order.push_back("
                 "\"legacyMoveContextBool\");}else{plan.postload_order."
                 "push_back(\"mMoveContext\");}if(revision<3)"
                 "plan.postload_order.push_back(\"legacyPreRev3Bool\");",
                 "native CharBoneDir load plan records legacy gates");
  ok &= contains(char_clip,
                 "plan.postload_order.push_back(\"mRecenter\");"
                 "if(revision>3)plan.postload_order.push_back("
                 "\"mBakeOutFacing\");returnplan;}",
                 "native CharBoneDir load plan records recenter and bake-out read");
  ok &= contains(char_clip,
                 "SourceCharBoneDirCopyPlansource_char_bone_dir_copy_plan(){"
                 "SourceCharBoneDirCopyPlanplan;plan.copied_superclasses={"
                 "\"ObjectDir\"};plan.copied_members={\"mMoveContext\","
                 "\"mRecenter\",\"mBakeOutFacing\"};returnplan;}",
                 "native CharBoneDir copy plan mirrors source COPY_MEMBER list");
  ok &= contains(char_clip,
                 "voidsource_char_bone_dir_list_bones("
                 "conststd::vector<CharClip::OutputBone>&output_bones,"
                 "intmove_context,intcontext_mask,boolinclude_delta_facing,"
                 "std::vector<SourceCharBonesBone>&bones){if(("
                 "move_context&context_mask)!=0){bones.push_back({"
                 "\"bone_facing.pos\",1.0f});bones.push_back({"
                 "\"bone_facing.rotz\",1.0f});",
                 "native CharBoneDir ListBones helper mirrors source facing rows");
  ok &= contains(char_clip,
                 "if(include_delta_facing){bones.push_back({"
                 "\"bone_facing_delta.pos\",1.0f});bones.push_back({"
                 "\"bone_facing_delta.rotz\",1.0f});}}for("
                 "constCharClip::OutputBone&output_bone:output_bones){"
                 "source_char_bone_stuff_bones(output_bone,context_mask,bones);}}",
                 "native CharBoneDir ListBones helper mirrors source delta and delegation");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_bone_dir_get_clip_types("
                 "conststd::vector<SourceCharBoneDirClipTypeResource>&clip_types){"
                 "std::vector<std::string>result;result.push_back(\"\");",
                 "native CharBoneDir GetClipTypes helper starts with empty symbol");
  ok &= contains(char_clip,
                 "for(constSourceCharBoneDirClipTypeResource&clip_type:clip_types){"
                 "result.push_back(clip_type.clip_type);}std::sort("
                 "result.begin(),result.end());returnresult;}",
                 "native CharBoneDir GetClipTypes helper sorts source symbols");
  ok &= contains(char_clip,
                 "if(it==clip_types.end()){result.warning=\"no_type\";"
                 "returnresult;}result.clip_type_found=true;",
                 "native CharBoneDir resource lookup mirrors missing type branch");
  ok &= contains(char_clip,
                 "if(!it->has_resource){result.warning=\"no_resource_field\";"
                 "returnresult;}result.resource_field_found=true;",
                 "native CharBoneDir resource lookup mirrors missing resource field branch");
  ok &= contains(char_clip,
                 "result.resource_name=it->resource_name;result.context_mask="
                 "it->context_mask;if(!it->resource_found){result.warning="
                 "\"no_resource\";returnresult;}result.resource_found=true;",
                 "native CharBoneDir resource lookup mirrors missing resource branch");
  ok &= contains(char_clip,
                 "if(step.lookup.resource_found){step.call_stuff_bones=true;"
                 "step.context_mask=step.lookup.context_mask;}returnstep;}",
                 "native CharBoneDir Symbol StuffBones helper mirrors context handoff");
  ok &= contains(char_clip,
                 "SourceCharBoneDirContextFlagsStep"
                 "source_char_bone_dir_get_context_flags_step(",
                 "native CharBoneDir GetContextFlags helper is implemented");
  ok &= contains(char_clip,
                 "for(size_tsource_index=0;source_index+1<clip_types.size();"
                 "++source_index)",
                 "native CharBoneDir GetContextFlags helper preserves source scan boundary");
  ok &= contains(char_clip,
                 "std::sort(step.context_flags.begin(),"
                 "step.context_flags.end());",
                 "native CharBoneDir GetContextFlags helper sorts symbols");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_bone_dir_sync_filter(",
                 "native CharBoneDir SyncFilter helper is implemented");
  ok &= contains(char_clip,
                 "bone.rotation_type!=kSourceCharBonesTypeEnd&&"
                 "(filter_context&bone.rotation_context)!=0",
                 "native CharBoneDir SyncFilter helper gates TYPE_END rotation");
  ok &= contains(char_clip,
                 "SourceCharBonesMeshesReplaceStepsource_char_bones_meshes_replace_step("
                 "conststd::vector<std::string>&meshes,conststd::string&from,"
                 "conststd::string&to,boolto_is_transformable,"
                 "conststd::string&dummy_mesh){SourceCharBonesMeshesReplaceStepstep;",
                 "native CharBonesMeshes Replace helper exists");
  ok &= contains(char_clip,
                 "if(from==dummy_mesh)returnstep;step.scan_meshes=true;",
                 "native CharBonesMeshes Replace helper mirrors dummy skip");
  ok &= contains(char_clip,
                 "if(step.meshes[i]!=from)continue;step.replaced_index="
                 "static_cast<int>(i);if(to_is_transformable){step.meshes[i]=to;}",
                 "native CharBonesMeshes Replace helper mirrors first transformable match");
  ok &= contains(char_clip,
                 "else{step.meshes[i]=dummy_mesh;step.assigned_dummy=true;}"
                 "returnstep;",
                 "native CharBonesMeshes Replace helper mirrors dummy fallback");
  ok &= contains(char_clip,
                 "SourceCharBonesMeshesReallocateStep"
                 "source_char_bones_meshes_reallocate_step("
                 "conststd::vector<SourceCharBonesBone>&bones,"
                 "conststd::unordered_map<std::string,std::string>&"
                 "transform_lookup,conststd::string&dummy_mesh){"
                 "SourceCharBonesMeshesReallocateStepstep;step.meshes.resize("
                 "bones.size());",
                 "native CharBonesMeshes Reallocate helper resizes to bone count");
  ok &= contains(char_clip,
                 "constautoit=transform_lookup.find(bones[i].name);"
                 "if(it!=transform_lookup.end()){step.meshes[i]=it->second;"
                 "continue;}",
                 "native CharBonesMeshes Reallocate helper mirrors transform lookup");
  ok &= contains(char_clip,
                 "if(bones[i].name.rfind(\"bone_facing\",0)!=0){"
                 "step.missing_non_facing_bones.push_back(bones[i].name);}"
                 "step.meshes[i]=dummy_mesh;",
                 "native CharBonesMeshes Reallocate helper mirrors dummy and facing filter");
  ok &= contains(char_clip,
                 "step.acquire_pose=!step.meshes.empty();returnstep;}",
                 "native CharBonesMeshes Reallocate helper mirrors AcquirePose gate");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_bones_meshes_stuff_meshes("
                 "conststd::vector<std::string>&existing_objects,"
                 "conststd::vector<std::string>&meshes){"
                 "std::vector<std::string>objects=existing_objects;"
                 "objects.insert(objects.end(),meshes.begin(),meshes.end());",
                 "native CharBonesMeshes StuffMeshes helper appends source order");
  ok &= contains(char_clip,
                 "\"[clip-output]%-28ssourceCharBoneversion=%u\"",
                 "native clip debug log labels source CharBone rows");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharBone.cpp` and",
                 "document cites latest CharBone source");
  ok &= contains(doc,
                 "source_char_bone_load_plan",
                 "document records native CharBone load plan helper");
  ok &= contains(doc,
                 "revision-6 shared context row",
                 "document records CharBone revision-6 branch");
  ok &= contains(doc,
                 "Native clip output rows now decode and log those fields",
                 "document records native CharBone row decode");
  ok &= contains(doc,
                 "Native `source_char_bone_find_weight_index`,",
                 "document records native CharBone helper ports");
  ok &= contains(doc,
                 "Native `source_char_bone_copy_members` ports only the concrete",
                 "document records native CharBone copy-member helper");
  ok &= contains(doc,
                 "Native decoder-only fields such as embedded legacy\n"
                 "    transform bytes",
                 "document records CharBone copy-member decoder boundary");
  ok &= contains(doc,
                 "`StuffBones`\n    appends `.pos`, `.scale`, and rotation "
                 "channels in source order",
                 "document records source CharBone StuffBones order");
  ok &= contains(doc,
                 "`CharBoneDir::ListBones` adds `bone_facing.pos`",
                 "document records source CharBoneDir ListBones source");
  ok &= contains(doc,
                 "Native `source_char_bone_dir_list_bones` ports that "
                 "list-building behavior",
                 "document records native CharBoneDir ListBones helper");
  ok &= contains(doc,
                 "`FindResourceFromClipType`, `StuffBones(CharBones&, Symbol)`, and",
                 "document records source CharBoneDir clip type resource flow");
  ok &= contains(doc,
                 "`source_char_bone_dir_find_resource_from_clip_type`,",
                 "document records native CharBoneDir resource lookup helper");
  ok &= contains(doc,
                 "missing type, missing `(resource ...)` field, missing loaded resource",
                 "document records CharBoneDir resource warning branches");
  ok &= contains(doc,
                 "`CharBoneDir::GetContextFlags` lazily rebuilds cached context symbols",
                 "document records source CharBoneDir GetContextFlags behavior");
  ok &= contains(doc,
                 "`CharBoneDir::SyncFilter` clears `mFilterBones`",
                 "document records source CharBoneDir SyncFilter behavior");
  ok &= contains(doc,
                 "`CharBoneDir` construction, load, and copy are source-visible",
                 "document records source CharBoneDir load/copy slice");
  ok &= contains(doc,
                 "`source_char_bone_dir_default_state`,",
                 "document records native CharBoneDir default helper");
  ok &= contains(doc,
                 "`source_char_bone_dir_load_plan`, and",
                 "document records native CharBoneDir load helper");
  ok &= contains(doc,
                 "These helpers do not perform runtime\n"
                 "    MILO loading",
                 "document fences CharBoneDir runtime resource loading");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharBonesMeshes.cpp` is concrete",
                 "document cites latest CharBonesMeshes source");
  ok &= contains(doc,
                 "`source_char_bones_meshes_replace_step`,",
                 "document records native CharBonesMeshes Replace helper");
  ok &= contains(doc,
                 "suppressing missing logs for\n"
                 "    `bone_facing*`",
                 "document records CharBonesMeshes facing missing filter");
  ok &= contains(doc,
                 "This still does\n"
                 "    not port broad `PoseMeshes` transform writes",
                 "document fences CharBonesMeshes PoseMeshes writes");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_copy_members(output)",
                 "focused CharBones source test covers CharBone copy-member helper");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_load_plan(6)",
                 "focused CharBones source test covers CharBone revision-6 load plan");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_load_plan(10)",
                 "focused CharBones source test covers CharBone modern load plan");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_copy_plan()",
                 "focused CharBones source test covers CharBone copy plan");
  ok &= contains(char_bones_source_test,
                 "\"CharBonecopyresetsdecoderparent\"",
                 "focused CharBones source test covers CharBone decoder reset");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_find_weight_index(output,0x2)",
                 "focused CharBones source test covers CharBone FindWeight");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_get_weight(output,0x8)",
                 "focused CharBones source test covers CharBone weight default");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_stuff_bones(output,0x4,stuffed);",
                 "focused CharBones source test covers CharBone StuffBones");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_clear_context(output,0x2);",
                 "focused CharBones source test covers CharBone ClearContext");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_default_state()",
                 "focused CharBones source test covers CharBoneDir defaults");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_load_plan(1)",
                 "focused CharBones source test covers CharBoneDir legacy load");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_load_plan(4)",
                 "focused CharBones source test covers CharBoneDir latest load");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_copy_plan()",
                 "focused CharBones source test covers CharBoneDir copy plan");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_list_bones(dir_output_bones,0x1,0x1,"
                 "true,",
                 "focused CharBones source test covers CharBoneDir facing rows");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_list_bones(dir_output_bones,0x1,0x4,"
                 "false,",
                 "focused CharBones source test covers CharBoneDir delegation without facing");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_get_clip_types(clip_resources)",
                 "focused CharBones source test covers CharBoneDir GetClipTypes");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_find_resource_from_clip_type("
                 "clip_resources,\"missing\")",
                 "focused CharBones source test covers CharBoneDir missing type");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_find_resource_from_clip_type("
                 "clip_resources,\"broken\")",
                 "focused CharBones source test covers CharBoneDir missing resource field");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_find_resource_from_clip_type("
                 "clip_resources,\"rhythm\")",
                 "focused CharBones source test covers CharBoneDir missing resource");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_stuff_bones_symbol_step(clip_resources,"
                 "\"solo\")",
                 "focused CharBones source test covers CharBoneDir Symbol StuffBones handoff");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_get_context_flags_step(",
                 "focused CharBones source test covers CharBoneDir GetContextFlags");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_sync_filter(filter_inputs,0x8)",
                 "focused CharBones source test covers CharBoneDir SyncFilter");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_meshes_replace_step({\"mesh_a\",\"mesh_b\"},",
                 "focused CharBones source test covers CharBonesMeshes Replace dummy skip");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_meshes_replace_step({\"mesh_a\",\"mesh_b\","
                 "\"mesh_b\"},",
                 "focused CharBones source test covers CharBonesMeshes Replace first match");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_meshes_reallocate_step("
                 "mesh_bones,{{\"bone_hand.pos\",\"hand_xfm\"}},\"dummy_mesh\")",
                 "focused CharBones source test covers CharBonesMeshes Reallocate");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_meshes_stuff_meshes({\"existing\"},"
                 "{\"mesh_a\",\"mesh_b\"})",
                 "focused CharBones source test covers CharBonesMeshes StuffMeshes");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "BEGIN_COPYS(CharBone)COPY_SUPERCLASS(Hmx::Object)"
                 "CREATE_COPY(CharBone)BEGIN_COPYING_MEMBERS",
                 "latest CharBone source defines copy macro start");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "COPY_MEMBER(mRotationContext)COPY_MEMBER(mScaleContext)"
                 "COPY_MEMBER(mPositionContext)COPY_MEMBER(mRotation)"
                 "COPY_MEMBER(mTarget)COPY_MEMBER(mWeights)COPY_MEMBER(mTrans)"
                 "COPY_MEMBER(mBakeOutAsTopLevel)",
                 "latest CharBone source defines copy member list");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "voidCharBone::StuffBones(std::list<CharBones::Bone>&bonelist,"
                 "inti)const{if(mPositionContext&i){bonelist.push_back("
                 "CharBones::Bone(CharBones::ChannelName(Name(),"
                 "CharBones::TYPE_POS),GetWeight(i)));}",
                 "latest CharBone source defines StuffBones position append");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "if(mScaleContext&i){bonelist.push_back(CharBones::Bone("
                 "CharBones::ChannelName(Name(),CharBones::TYPE_SCALE),"
                 "GetWeight(i)));}if(mRotation!=CharBones::TYPE_END&&"
                 "mRotationContext&i){bonelist.push_back(CharBones::Bone("
                 "CharBones::ChannelName(Name(),mRotation),GetWeight(i)));}}",
                 "latest CharBone source defines StuffBones scale and rotation append");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "voidCharBone::ClearContext(inti){intmask=~i;"
                 "mPositionContext&=mask;mScaleContext&=mask;"
                 "mRotationContext&=mask;}",
                 "latest CharBone source defines ClearContext mask");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "floatCharBone::GetWeight(inti)const{constWeightContext*ctx="
                 "FindWeight(i);if(ctx)returnctx->mWeight;elsereturn1.0f;}",
                 "latest CharBone source defines GetWeight default");
  ok &= contains(rb3_latest_char_bone_cpp,
                 "constCharBone::WeightContext*CharBone::FindWeight(inti)const{"
                 "for(std::vector<WeightContext>::const_iteratorit="
                 "mWeights.begin();it!=mWeights.end();++it){if(("
                 "*it).mContext&i)returnit;}return0;}",
                 "latest CharBone source defines FindWeight first match");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "CharBoneDir::CharBoneDir():mRecenter(this),mMoveContext(0),"
                 "mBakeOutFacing(1),mContextFlags(0),mFilterContext(0),"
                 "mFilterBones(this,kObjListNoNull){}",
                 "latest CharBoneDir source defines constructor defaults");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "voidCharBoneDir::PreLoad(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(4,0);PushRev(packRevs(gAltRev,gRev),this);"
                 "ObjectDir::PreLoad(bs);}",
                 "latest CharBoneDir source defines PreLoad revision path");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "voidCharBoneDir::PostLoad(BinStream&bs){"
                 "ObjectDir::PostLoad(bs);intrevs=PopRev(this);"
                 "gRev=getHmxRev(revs);gAltRev=getAltRev(revs);"
                 "if(gRev<2)bs.ReadBool();elsebs>>mMoveContext;"
                 "if(gRev<3)bs.ReadBool();bs>>mRecenter;if(gRev>3)"
                 "bs>>mBakeOutFacing;}",
                 "latest CharBoneDir source defines PostLoad revision gates");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "COPY_MEMBER(mMoveContext)COPY_MEMBER(mRecenter)"
                 "COPY_MEMBER(mBakeOutFacing)",
                 "latest CharBoneDir source defines copy members");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "voidCharBoneDir::ListBones(std::list<CharBones::Bone>&bones,"
                 "intmask,boolb){if(mMoveContext&mask){bones.push_back("
                 "CharBones::Bone(\"bone_facing.pos\",1.0f));"
                 "bones.push_back(CharBones::Bone(\"bone_facing.rotz\",1.0f));",
                 "latest CharBoneDir source defines facing rows");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "if(b){bones.push_back(CharBones::Bone("
                 "\"bone_facing_delta.pos\",1.0f));bones.push_back("
                 "CharBones::Bone(\"bone_facing_delta.rotz\",1.0f));}}"
                 "for(ObjDirItr<CharBone>it(this,true);it!=0;++it){"
                 "it->StuffBones(bones,mask);}}",
                 "latest CharBoneDir source defines delta rows and CharBone delegation");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "CharBoneDir*CharBoneDir::FindResourceFromClipType("
                 "Symbolcliptype){DataArray*types=sCharClipTypes->FindArray("
                 "cliptype,false);if(!types){MILO_WARN(\"CharCliphasnotype%s\","
                 "cliptype);return0;}",
                 "latest CharBoneDir source defines missing clip type branch");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "DataArray*resources=types->FindArray(\"resource\",false);"
                 "if(!resources){MILO_WARN(\"CharClip%shasno(resource...)field\","
                 "cliptype);return0;}",
                 "latest CharBoneDir source defines missing resource field branch");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "CharBoneDir*dir=FindResource(resources->Str(1));if(!dir)"
                 "MILO_WARN(\"CharClip%shasnoresource\",cliptype);returndir;",
                 "latest CharBoneDir source defines resource lookup branch");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "voidCharBoneDir::StuffBones(CharBones&bones,Symbolsym){"
                 "DataArray*found=sCharClipTypes->FindArray(sym,false);",
                 "latest CharBoneDir source defines Symbol StuffBones lookup");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "dir->StuffBones(bones,DataGetMacro(resource->Str(2))->Int(0));",
                 "latest CharBoneDir source defines Symbol StuffBones context handoff");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "DataNodeCharBoneDir::GetClipTypes(){DataArray*arr=newDataArray("
                 "sCharClipTypes->Size());arr->Node(0)=DataNode(Symbol());",
                 "latest CharBoneDir source defines GetClipTypes empty symbol");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "arr->Node(i)=DataNode(currArr->Sym(0));}arr->SortNodes();",
                 "latest CharBoneDir source defines GetClipTypes sorted symbols");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "DataNodeCharBoneDir::GetContextFlags(){if("
                 "mContextFlags.Type()==kDataInt){DataArray*cfg="
                 "SystemConfig(\"objects\",\"CharClip\",\"types\");DataArray*arr="
                 "newDataArray(cfg->Size()-1);",
                 "latest CharBoneDir source defines GetContextFlags cache rebuild");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "for(inti=1;i<arr->Size();i++){DataArray*resourceArr="
                 "cfg->Array(i)->FindArray(\"resource\",false);",
                 "latest CharBoneDir source defines GetContextFlags scan boundary");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "arr->Node(count++)=DataNode(resourceArr->Str(2));",
                 "latest CharBoneDir source defines GetContextFlags context append");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "arr->Resize(count);arr->SortNodes();mContextFlags=DataNode("
                 "arr,kDataArray);",
                 "latest CharBoneDir source defines GetContextFlags sort cache");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "voidCharBoneDir::SyncFilter(){mFilterBones.clear();"
                 "for(ObjDirItr<CharBone>it(this,true);it!=0;++it){",
                 "latest CharBoneDir source defines SyncFilter loop");
  ok &= contains(rb3_latest_char_bone_dir_cpp,
                 "mFilterContext&it->PositionContext()||mFilterContext&"
                 "it->ScaleContext()||(it->RotationType()!=CharBones::TYPE_END&&"
                 "mFilterContext&it->RotationContext())",
                 "latest CharBoneDir source defines SyncFilter gates");
  ok &= contains(rb3_latest_char_bones_meshes_h,
                 "classCharBonesMeshes:publicCharBonesAlloc",
                 "latest CharBonesMeshes header defines source inheritance");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "CharBonesMeshes::CharBonesMeshes():mMeshes(this),"
                 "mDummyMesh(Hmx::Object::New<RndTransformable>()){}",
                 "latest CharBonesMeshes source defines mesh vector and dummy construction");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "CharBonesMeshes::~CharBonesMeshes(){mMeshes.clear();"
                 "deletemDummyMesh;}",
                 "latest CharBonesMeshes source defines destructor cleanup");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "voidCharBonesMeshes::Replace(Hmx::Object*from,Hmx::Object*to){"
                 "Hmx::Object::Replace(from,to);if(from!=mDummyMesh){",
                 "latest CharBonesMeshes source defines Replace dummy skip");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "if(*it==from){*it=dynamic_cast<RndTransformable*>(to);"
                 "if(!*it)*it=mDummyMesh;return;}",
                 "latest CharBonesMeshes source defines Replace first match and dummy fallback");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "voidCharBonesMeshes::ReallocateInternal(){"
                 "CharBonesAlloc::ReallocateInternal();Stringstr;",
                 "latest CharBonesMeshes source defines Reallocate base allocation");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "mMeshes.resize(mBones.size());for(inti=0;i<mMeshes.size();i++){",
                 "latest CharBonesMeshes source defines Reallocate resize");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "mMeshes[i]=CharUtlFindBoneTrans(mBones[i].name.Str(),Dir());"
                 "if(!mMeshes[i]){if(strncmp(\"bone_facing\","
                 "mBones[i].name.Str(),0xB)){",
                 "latest CharBonesMeshes source defines transform lookup and facing filter");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "mMeshes[i]=mDummyMesh;}}if(mMeshes.empty())return;"
                 "elseAcquirePose();}",
                 "latest CharBonesMeshes source defines dummy fallback and AcquirePose gate");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "voidCharBonesMeshes::PoseMeshes(){floatangle;Hmx::Matrix3m;"
                 "m.RotateAboutY(angle);m.RotateAboutX(angle);}",
                 "latest CharBonesMeshes source exposes incomplete PoseMeshes body");
  ok &= contains(rb3_latest_char_bones_meshes_cpp,
                 "voidCharBonesMeshes::StuffMeshes(std::list<Hmx::Object*>&oList){"
                 "for(inti=0;i<mMeshes.size();i++){oList.push_back(mMeshes[i]);}}",
                 "latest CharBonesMeshes source defines StuffMeshes order");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "CharBone*CharUtlFindBone(constchar*cc,ObjectDir*dir){",
                 "latest CharUtl source exposes FindBone");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "strcpy(dst,\".cb\");returndir->Find<CharBone>(buf,false);",
                 "latest CharUtl source rewrites FindBone lookup to .cb");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "RndTransformable*CharUtlFindBoneTrans(constchar*cc,"
                 "ObjectDir*dir){",
                 "latest CharUtl source exposes FindBoneTrans");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "strcpy(dst,\".cb\");CharBone*bone=dir->Find<CharBone>"
                 "(buf,false);if(bone)returnbone->mTrans;else{",
                 "latest CharUtl source resolves CharBone trans before fallbacks");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "strcpy(dst,\".trans\");RndTransformable*trans="
                 "dir->Find<RndTransformable>(buf,false);if(trans)returntrans;"
                 "else{",
                 "latest CharUtl source tries .trans fallback before .mesh");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "strcpy(dst,\".mesh\");RndTransformable*mesh="
                 "dir->Find<RndTransformable>(buf,false);returnmesh;",
                 "latest CharUtl source tries .mesh fallback last");
  ok &= contains(rb3_latest_char_utl_h,
                 "RndTransformable*CharUtlFindBoneTrans(constchar*,ObjectDir*);",
                 "latest CharUtl header declares FindBoneTrans");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "boolCharUtlIsAnimatable(RndTransformable*trans){",
                 "latest CharUtl source exposes IsAnimatable");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "if(mesh&&mesh->NumBones()!=0)returnfalse;",
                 "latest CharUtl source rejects skinned meshes");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "if(dynamic_cast<RndCam*>(trans))returnfalse;",
                 "latest CharUtl source rejects cameras");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "if(dynamic_cast<CharCollide*>(trans))returnfalse;",
                 "latest CharUtl source rejects CharCollide transforms");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "if(dynamic_cast<CharCuff*>(trans))returnfalse;",
                 "latest CharUtl source rejects CharCuff transforms");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "if(dynamic_cast<RndDir*>(trans))returnfalse;",
                 "latest CharUtl source rejects RndDir transforms");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "returnstrncmp(trans->Name(),\"spot_\",5)!=0;",
                 "latest CharUtl source rejects spot_ transforms");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "ClipPredict::ClipPredict(CharClip*clip,constVector3&pos,"
                 "floatang):mClip(0){SetClip(clip);mPos=pos;mAng=ang;",
                 "latest CharUtl source exposes ClipPredict constructor");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "mAngChannel=clip->GetChannel(\"bone_facing.rotz\");"
                 "mPosChannel=clip->GetChannel(\"bone_facing.pos\");",
                 "latest CharUtl source binds ClipPredict facing channels");
  ok &= contains(rb3_latest_char_utl_cpp,
                 "Subtract(mLastPos,v34,v34);RotateAboutZ(v34,norm,v34);"
                 "mPos+=v34;",
                 "latest CharUtl source rotates ClipPredict position delta");
  ok &= contains(char_clip_h,
                 "enumclassSourceCharUtlObjectKind{",
                 "native header exposes CharUtl object kind model");
  ok &= contains(char_clip_h,
                 "std::stringsource_char_utl_name_with_suffix("
                 "conststd::string&name,conststd::string&suffix);",
                 "native header exposes CharUtl suffix helper");
  ok &= contains(char_clip_h,
                 "std::optional<SourceCharUtlObject>source_char_utl_find_bone(",
                 "native header exposes CharUtl FindBone helper");
  ok &= contains(char_clip_h,
                 "std::optional<SourceCharUtlBoneTransResult>"
                 "source_char_utl_find_bone_trans(",
                 "native header exposes CharUtl FindBoneTrans helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_utl_is_animatable("
                 "constSourceCharUtlObject&object);",
                 "native header exposes CharUtl IsAnimatable helper");
  ok &= contains(char_clip_h,
                 "SourceCharUtlMergeResultsource_char_utl_merge_bones("
                 "conststd::vector<SourceCharUtlMergeBone>&source_bones,",
                 "native header exposes CharUtl MergeBones helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_utl_reset_transform_names(",
                 "native header exposes CharUtl ResetTransform helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_utl_reset_hair_names(",
                 "native header exposes CharUtl ResetHair helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_utl_clip_predict("
                 "SourceCharUtlClipPredictState&state,",
                 "native header exposes CharUtl ClipPredict helper");
  ok &= contains(char_clip,
                 "std::stringsource_char_utl_name_with_suffix("
                 "conststd::string&name,conststd::string&suffix){"
                 "constsize_tdot=name.rfind('.');",
                 "native CharUtl suffix helper replaces final suffix");
  ok &= contains(char_clip,
                 "conststd::stringlookup=source_char_utl_name_with_suffix("
                 "name,\"cb\");for(constSourceCharUtlObject&object:objects){"
                 "if(object.name==lookup&&object.kind=="
                 "SourceCharUtlObjectKind::kCharBone){returnobject;}}",
                 "native CharUtl FindBone helper requires source .cb CharBone row");
  ok &= contains(char_clip,
                 "object.kind==SourceCharUtlObjectKind::kCharBone){if("
                 "object.char_bone_transform.empty())returnstd::nullopt;"
                 "returnSourceCharUtlBoneTransResult{cb_lookup,"
                 "object.char_bone_transform,true};}",
                 "native CharUtl FindBoneTrans returns CharBone transform first");
  ok &= contains(char_clip,
                 "source_char_utl_find_named_transformable(trans_lookup,objects)",
                 "native CharUtl FindBoneTrans tests .trans fallback");
  ok &= contains(char_clip,
                 "source_char_utl_find_named_transformable(mesh_lookup,objects)",
                 "native CharUtl FindBoneTrans tests .mesh fallback");
  ok &= contains(char_clip,
                 "object.kind==SourceCharUtlObjectKind::kMesh&&"
                 "object.mesh_bone_count!=0",
                 "native CharUtl IsAnimatable rejects skinned meshes");
  ok &= contains(char_clip,
                 "object.kind==SourceCharUtlObjectKind::kCamera||"
                 "object.kind==SourceCharUtlObjectKind::kCharCollide||"
                 "object.kind==SourceCharUtlObjectKind::kCharCuff||"
                 "object.kind==SourceCharUtlObjectKind::kDirectory||"
                 "object.kind==SourceCharUtlObjectKind::kCharBone",
                 "native CharUtl IsAnimatable rejects source non-animatable kinds");
  ok &= contains(char_clip,
                 "returnobject.name.rfind(\"spot_\",0)!=0;",
                 "native CharUtl IsAnimatable rejects spot_ names");
  ok &= contains(char_clip,
                 "dest.position_context=dest.scale_context|context_mask;",
                 "native CharUtl MergeBones preserves source scale branch");
  ok &= contains(char_clip,
                 "result.warnings.push_back({\"different_rotation\","
                 "source_bone.name,",
                 "native CharUtl MergeBones warns on rotation mismatch");
  ok &= contains(char_clip,
                 "if(!transform.has_parent)names.push_back(transform.name);",
                 "native CharUtl ResetTransform resets only top-level rows");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_utl_reset_hair_names("
                 "conststd::vector<std::string>&hair_names){returnhair_names;}",
                 "native CharUtl ResetHair enters each hair row");
  ok &= contains(char_clip,
                 "source_rotate_about_z_vec(delta,source_limit_ang(state.ang-"
                 "first.facing_rot));",
                 "native CharUtl ClipPredict rotates source delta");
  ok &= contains(char_clip,
                 "state.ang=source_limit_ang(state.ang+source_limit_ang("
                 "second.facing_rot-first.facing_rot));",
                 "native CharUtl ClipPredict wraps angle advance");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_name_with_suffix(\"face.bone.mesh\",\"cb\")",
                 "focused CharUtl source test covers final suffix replacement");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_find_bone(\"bone_head.mesh\",objects)",
                 "focused CharUtl source test covers .cb FindBone lookup");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_find_bone_trans(\"bone_head.mesh\",objects)",
                 "focused CharUtl source test covers CharBone transform priority");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_find_bone_trans(\"bone_spine.mesh\",objects)",
                 "focused CharUtl source test covers .trans fallback");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_find_bone_trans(\"bone_pelvis\",objects)",
                 "focused CharUtl source test covers .mesh fallback");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_is_animatable(",
                 "focused CharUtl source test covers IsAnimatable");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_merge_bones(source_bones,dest_bones,0x8)",
                 "focused CharUtl source test covers MergeBones");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_bone_saver_capture_names(transforms)",
                 "focused CharUtl source test covers BoneSaver selection");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_reset_transform_names(transforms)",
                 "focused CharUtl source test covers ResetTransform");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_reset_hair_names({\"hair_front1.hair\","
                 "\"scarf.hair\"})",
                 "focused CharUtl source test covers ResetHair");
  ok &= contains(char_utl_source_test,
                 "source_char_utl_clip_predict(predict_state,predict_first,"
                 "predict_second)",
                 "focused CharUtl source test covers ClipPredict math");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharUtl.cpp` and",
                 "document cites latest CharUtl source");
  ok &= contains(doc,
                 "`CharUtlFindBoneTrans` uses the same `.cb` lookup first",
                 "document records source CharUtl lookup order");
  ok &= contains(doc,
                 "Native `source_char_utl_name_with_suffix`,",
                 "document records native CharUtl helper ports");
  ok &= contains(doc,
                 "`CharUtlMergeBones` scans source `CharBone` rows",
                 "document records source CharUtl MergeBones behavior");
  ok &= contains(doc,
                 "scale-context branch calls `SetPositionContext(bone->ScaleContext() | i)`",
                 "document records exact source CharUtl scale branch");
  ok &= contains(doc,
                 "`CharUtlResetHair` calls `Enter()` for every `CharHair` row",
                 "document records source CharUtl ResetHair behavior");
  ok &= contains(doc,
                 "`ClipPredict` binds `bone_facing.rotz` and `bone_facing.pos`",
                 "document records source CharUtl ClipPredict channels");
  ok &= contains(rb3_latest_char_bones_cpp,
                 "voidCharBones::ScaleAdd(CharClip*clip,floatf1,floatf2,"
                 "floatf3){clip->ScaleAdd(*this,f1,f2,f3);}",
                 "latest CharBones source delegates clip pose math to CharClip");
  ok &= contains(rb3_latest_char_clip_h,
                 "CharBonesSamplesmFull;//0x64CharBonesSamplesmOne;",
                 "latest CharClip source exposes full/one sample members");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::StuffBones(CharBones&bones){std::list<"
                 "CharBones::Bone>blist;ListBones(blist);bones.AddBones(blist);}",
                 "latest CharClip source exposes StuffBones flow");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::PoseMeshes(ObjectDir*dir,floatf){CharBonesMeshes"
                 "meshes;meshes.SetName(\"tmp_viseme_bones\",dir);"
                 "StuffBones(meshes);ScaleDown(meshes,0.0f);"
                 "ScaleAdd(meshes,1.0f,f,0.0f);meshes.PoseMeshes();}",
                 "latest CharClip source exposes PoseMeshes call flow");
  ok &= missing(rb3_latest_char_clip_cpp, "voidCharClip::ScaleAdd(",
                "latest CharClip source does not expose ScaleAdd body");
  ok &= missing(rb3_latest_char_clip_cpp, "voidCharClip::Load(",
                "latest CharClip source does not expose Load body");
  ok &= contains(rb3_latest_char_bones_samples_h,
                 "voidLoadHeader(BinStream&);voidLoadData(BinStream&);"
                 "voidSetPreview(int);voidReadCounts(BinStream&,int);"
                 "voidRelativize(CharClip*);voidEvaluateChannel(void*,int,int,float);"
                 "intFracToSample(float*)const;",
                 "latest CharBonesSamples header exposes sample runtime boundary");
  ok &= contains(rb3_latest_char_bones_samples_h,
                 "intAllocateSize(){returnmTotalSize*mNumSamples;}",
                 "latest CharBonesSamples header defines AllocateSize");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "CharBonesSamples::CharBonesSamples():mNumSamples(0),"
                 "mPreviewSample(0),mRawData(0){}",
                 "latest CharBonesSamples source defines constructor state");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::Set(conststd::vector<CharBones::Bone>&"
                 "bones,inti,CharBones::CompressionTypety){ClearBones();"
                 "SetCompression(ty);mNumSamples=i;AddBones(bones);"
                 "_MemFree(mRawData);mRawData=(char*)_MemAlloc(AllocateSize(),0);"
                 "mFrames.clear();}",
                 "latest CharBonesSamples source exposes Set allocation");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::Clone(constCharBonesSamples&samp){"
                 "Set(samp.mBones,samp.mNumSamples,samp.mCompression);"
                 "memcpy(mRawData,samp.mRawData,AllocateSize());"
                 "mFrames=samp.mFrames;}",
                 "latest CharBonesSamples source exposes Clone allocation and frame copy");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::RotateBy(CharBones&bones,inti){"
                 "mStart=&mRawData[mTotalSize*i];CharBones::RotateBy(bones);}",
                 "latest CharBonesSamples source exposes RotateBy row selection");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::RotateTo(CharBones&bones,floatf1,inti,"
                 "floatf2){mStart=&mRawData[mTotalSize*i];CharBones::RotateTo"
                 "(bones,(1.0f-f2)*f1);if(f2>0.0f){mStart=&mRawData[mTotalSize*"
                 "(i+1)];CharBones::RotateTo(bones,f2*f1);}}",
                 "latest CharBonesSamples source exposes RotateTo sample split");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::ScaleAddSample(CharBones&bones,floatf1,"
                 "inti,floatf2){mStart=&mRawData[mTotalSize*i];CharBones::"
                 "ScaleAdd(bones,(1.0f-f2)*f1);if(f2>0.0f){mStart=&mRawData"
                 "[mTotalSize*(i+1)];CharBones::ScaleAdd(bones,f2*f1);}}",
                 "latest CharBonesSamples source exposes ScaleAdd sample split");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::Load(BinStream&bs){bs>>gVer;"
                 "MILO_ASSERT(gVer>12&&gVer<=VER,0x2A0);LoadHeader(bs);"
                 "LoadData(bs);}",
                 "latest CharBonesSamples source exposes Load delegation");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::SetVer(intver){MILO_ASSERT(ver<13,"
                 "0x22B);gVer=ver;}",
                 "latest CharBonesSamples source exposes legacy SetVer gate");
  ok &= contains(rb3_latest_char_bones_samples_cpp,
                 "voidCharBonesSamples::SetPreview(inti){inttmp=Clamp(0,"
                 "mNumSamples-1,i);MILO_ASSERT(mPreviewSample<32767,0x38B);"
                 "mPreviewSample=tmp;mStart=&mRawData[mTotalSize*tmp];}",
                 "latest CharBonesSamples source defines preview row offset");
  ok &= contains(doc,
                 "Native `source_char_bones_samples_allocate_size`,",
                 "document records native CharBonesSamples state helpers");
  ok &= contains(doc,
                 "`source_char_bones_samples_load_version_known` ports that "
                 "exact range",
                 "document records native CharBonesSamples version gate");
  ok &= contains(doc,
                 "`SetVer` is the separate legacy source gate and asserts "
                 "`ver < 13`",
                 "document records native CharBonesSamples SetVer boundary");
  ok &= contains(doc,
                 "the clip parser rejects out-of-range `CharBonesSamples` "
                 "entries",
                 "document records parser-side CharBonesSamples version gate");
  ok &= contains(doc,
                 "Preview stores the clamped sample and\n    selected row offset",
                 "document records source CharBonesSamples preview offset");
  ok &= contains(doc,
                 "`source_char_bones_samples_rotate_by_offset`,",
                 "document records native CharBonesSamples RotateBy wrapper");
  ok &= contains(doc,
                 "Empty sample rows remain fenced",
                 "document records native CharBonesSamples zero-sample fence");
  ok &= missing(rb3_latest_char_bones_samples_cpp,
                "voidCharBonesSamples::LoadHeader(",
                "latest CharBonesSamples source does not expose LoadHeader body");
  ok &= missing(rb3_latest_char_bones_samples_cpp,
                "voidCharBonesSamples::EvaluateChannel(",
                "latest CharBonesSamples source does not expose EvaluateChannel body");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "CharClipDriver::CharClipDriver(Hmx::Object*owner,CharClip*clip,"
                 "intmask,floatblendwidth,CharClipDriver*next,floatf2,floatf3,"
                 "boolmultclips)",
                 "latest CharClipDriver source exposes play-node construction");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "mBlendWidth(blendwidth),mTimeScale(1.0f),mDBeat(0),"
                 "mAdvanceBeat(0),mClip(owner,clip),mNext(next),"
                 "mNextEvent(-1),mPlayMultipleClips(multclips)",
                 "latest CharClipDriver source exposes constructor initialized fields");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "if(mask&0xF0U)mPlayFlags=mPlayFlags&0xffffff0f|mask&0xf0U;"
                 "if(mask&0xFU)mPlayFlags=mPlayFlags&0xfffffff0|mask&0xfU;"
                 "if(mask&0xF600U)mPlayFlags=mPlayFlags&0xffff09ff|mask&0xf600U;",
                 "latest CharClipDriver source masks blend loop and beat-align flags");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "CharClipDriver*CharClipDriver::Exit(boolb){staticSymbolexit("
                 "\"exit\");if(b&&mNext){mNext=mNext->Exit(b);}"
                 "CharClipDriver*ret=mNext;ExecuteEvent(exit);RndAnimatable*"
                 "syncanim=mClip->mSyncAnim;if(syncanim)syncanim->EndAnim();"
                 "deletethis;returnret;}",
                 "latest CharClipDriver source exposes Exit stack behavior");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "voidCharClipDriver::DeleteStack(){if(mNext)mNext->"
                 "DeleteStack();deletethis;}",
                 "latest CharClipDriver source exposes DeleteStack tail-first behavior");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "CharClipDriver*CharClipDriver::DeleteClip(Hmx::Object*obj){"
                 "if(mClip==obj)returnExit(false);elseif(mNext)mNext=mNext->"
                 "DeleteClip(obj);returnthis;}",
                 "latest CharClipDriver source exposes DeleteClip first-match behavior");
  ok &= contains(rb3_latest_char_clip_driver_cpp,
                 "voidCharClipDriver::ExecuteEvent(Symbols){if(!s.Null()){"
                 "if(mClip->TypeDef()){staticDataNode&dude(DataVariable("
                 "\"clip.dude\"));dude=DataNode(mClip.RefOwner()->Dir());",
                 "latest CharClipDriver source exposes ExecuteEvent guard");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "constchar*CharClip::BeatAlignString(intmask){switch(mask&"
                 "0xF600){case0x200:return\"RealTime\";case0x400:return"
                 "\"UserTime\";case0x1000:return\"BeatAlign1\";",
                 "latest CharClip source exposes BeatAlignString switch");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClip::CharClip():mTransitions(this),mFramesPerSec(30.0f),"
                 "mBeatTrack(),mFlags(0),mPlayFlags(0),mRange(0.0f),mDirty(1),"
                 "mDoNotCompress(0),unk42(-1),mRelative(this,0),mBeatEvents(),"
                 "mSyncAnim(this,0),mFull(),mOne(),mFacing(){mBeatTrack.resize"
                 "(1,Key<float>(0,0));mBeatTrack.front().frame=0.0f;"
                 "mBeatTrack.front().value=0.0f;}",
                 "latest CharClip source exposes constructor defaults");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClip::BeatEvent::BeatEvent():beat(0){}",
                 "latest CharClip source exposes BeatEvent default beat");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClip::BeatEvent::BeatEvent(constBeatEvent&ev):"
                 "event(ev.event),beat(ev.beat){}",
                 "latest CharClip source exposes BeatEvent copy fields");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClip::BeatEvent&CharClip::BeatEvent::operator=("
                 "constBeatEvent&ev){event=ev.event;beat=ev.beat;}",
                 "latest CharClip source exposes BeatEvent assignment fields");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::BeatEvent::Load(BinStream&bs){bs>>event;"
                 "bs>>beat;}",
                 "latest CharClip source exposes BeatEvent load order");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharBoneDir*CharClip::GetResource()const{CharBoneDir*dir=0;"
                 "constDataArray*tdef=TypeDef();if(tdef){DataArray*found="
                 "tdef->FindArray(\"resource\",false);if(found)dir="
                 "CharBoneDir::FindResource(found->Str(1));}if(!dir){"
                 "MILO_WARN(\"%shasnoresource\",PathName(this));}returndir;}",
                 "latest CharClip source exposes GetResource lookup and warning");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "intCharClip::GetContext()const{constDataArray*tdef=TypeDef();"
                 "if(tdef){DataArray*found=tdef->FindArray(\"resource\",false);"
                 "if(found){returnDataGetMacro(found->Str(2))->Int(0);}}return0;}",
                 "latest CharClip source exposes GetContext resource fallback");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClip::Transitions::Transitions(Hmx::Object*o):"
                 "mNodeStart(0),mNodeEnd(0),mOwner(o){}",
                 "latest CharClip source exposes Transitions constructor");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClip::Transitions::~Transitions(){Clear();}",
                 "latest CharClip source exposes Transitions destructor clear");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::Transitions::Clear(){for(NodeVector*it="
                 "mNodeStart;it<mNodeEnd;it=&it[it->size]){it->clip->"
                 "Release(mOwner);}Resize(0,0);}",
                 "latest CharClip source exposes Transitions clear body");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "intCharClip::Transitions::Size()const{intsize=0;"
                 "for(NodeVector*it=mNodeStart;it<mNodeEnd;it=&it[it->size]){"
                 "size++;}returnsize;}",
                 "latest CharClip source exposes Transitions size body");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::StuffBones(CharBones&bones){std::list<"
                 "CharBones::Bone>blist;ListBones(blist);bones.AddBones(blist);}",
                 "latest CharClip source exposes StuffBones call order");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::PoseMeshes(ObjectDir*dir,floatf){"
                 "CharBonesMeshesmeshes;meshes.SetName(\"tmp_viseme_bones\",dir);"
                 "StuffBones(meshes);ScaleDown(meshes,0.0f);"
                 "ScaleAdd(meshes,1.0f,f,0.0f);meshes.PoseMeshes();}",
                 "latest CharClip source exposes PoseMeshes call order");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::SetPlayFlags(inti){if(i!=mPlayFlags){"
                 "mPlayFlags=i;mDirty=true;}}",
                 "latest CharClip source exposes SetPlayFlags dirty guard");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::SetFlags(inti){if(i!=mFlags){mFlags=i;"
                 "mDirty=true;}}",
                 "latest CharClip source exposes SetFlags dirty guard");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "boolCharClip::SharesGroups(CharClip*clip){std::vector<"
                 "ObjRef*>::const_reverse_iteratorit=Refs().rbegin();",
                 "latest CharClip source exposes SharesGroups ref-owner scan");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "CharClipGroup*grp=dynamic_cast<CharClipGroup*>((*it)->"
                 "RefOwner());if(grp&&grp->HasClip(clip))returntrue;",
                 "latest CharClip source exposes SharesGroups group membership");
  ok &= contains(doc,
                 "Native `source_char_clip_set_flags` and",
                 "document records native CharClip flag dirty helpers");
  ok &= contains(doc,
                 "Native `source_char_clip_beat_event_*` helpers port the concrete",
                 "document records native CharClip BeatEvent helpers");
  ok &= contains(doc,
                 "Native `source_char_clip_get_context` ports the concrete `GetContext`",
                 "document records native CharClip GetContext helper");
  ok &= contains(doc,
                 "Native `source_char_clip_get_resource` ports the concrete `GetResource`",
                 "document records native CharClip GetResource helper");
  ok &= contains(doc,
                 "records the lookup decision only; it does not load or synthesize resources",
                 "document fences CharClip GetResource helper");
  ok &= contains(doc,
                 "Native `source_char_clip_transitions_*` helpers port the concrete",
                 "document records native CharClip Transitions helpers");
  ok &= contains(doc,
                 "does not claim `Resize`, `RemoveNodes`, or transition graph evaluation",
                 "document fences CharClip Transitions unresolved bodies");
  ok &= contains(doc,
                 "Native `source_char_clip_stuff_bones` and",
                 "document records native CharClip StuffBones helper");
  ok &= contains(doc,
                 "dataflow without claiming the still-missing pose math bodies",
                 "document fences CharClip pose math bodies");
  ok &= contains(doc,
                 "Native `source_char_clip_shares_groups` ports the complete",
                 "document records native CharClip SharesGroups helper");
  ok &= contains(doc,
                 "Native `source_char_clip_default_state` records the complete "
                 "checked\n    constructor defaults",
                 "document records native CharClip constructor default helper");
  ok &= contains(doc,
                 "unchanged values preserve the incoming\n    dirty state",
                 "document records source CharClip unchanged dirty behavior");
  ok &= contains(doc,
                 "changed values store the requested flag value and mark",
                 "document records source CharClip changed dirty behavior");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "boolCharDriver::Starved(){if(mFirst){if(mFirst->mNext)"
                 "returnfalse;if((mFirst->mPlayFlags&0xF0)==0x10)"
                 "returnfalse;}returntrue;}",
                 "latest CharDriver source exposes Starved body");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "mBlendWidth(1.0f)",
                 "latest CharDriver source constructor exposes default blend width");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "mApply(kApplyBlend),mInternalBones(0),mPlayMultipleClips(0)",
                 "latest CharDriver source constructor exposes apply/internal defaults");
  ok &= contains(rb3_latest_char_driver_h,
                 "enumApplyMode{kApplyBlend,kApplyAdd,kApplyRotateTo,"
                 "kApplyBlendWeights};",
                 "latest CharDriver header exposes source apply enum order");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::Clear(){if(mFirst)mFirst->DeleteStack();"
                 "mFirst=0;}",
                 "latest CharDriver source exposes Clear stack deletion");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::Transfer(constCharDriver&driver){Clear();"
                 "mClips=driver.mClips;mLastNode=driver.mLastNode;"
                 "mRealign=driver.mRealign;mBeatScale=driver.mBeatScale;"
                 "mBlendWidth=driver.mBlendWidth;if(driver.mFirst)mFirst="
                 "newCharClipDriver(this,*driver.mFirst);}",
                 "latest CharDriver source exposes Transfer state copy");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::SetClips(ObjectDir*dir){if(dir!=mClips){"
                 "mLastNode=DataNode((Hmx::Object*)0);mClips=dir;}}",
                 "latest CharDriver source exposes SetClips last-node reset");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::SetBones(CharBonesObject*obj){mBones=obj;}",
                 "latest CharDriver source exposes SetBones assignment");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::Enter(){Clear();mLastNode=DataNode(0);"
                 "mOldBeat=1e+30f;mBeatScale=1.0f;RndPollable::Enter();"
                 "if(mDefaultClip)Play(DataNode(mDefaultClip),1,-1.0f,"
                 "1e+30f,0.0f);}",
                 "latest CharDriver source exposes Enter reset/default playback");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "CharClipDriver*CharDriver::PlayGroup(constchar*cc,inti,"
                 "floatf1,floatf2,floatf3){if(!mClips){MILO_WARN("
                 "\"%shasnoclips\",PathName(this));return0;}",
                 "latest CharDriver source exposes PlayGroup missing-dir branch");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "CharClipGroup*grp=dynamic_cast<CharClipGroup*>("
                 "mClips->FindObject(cc,false));if(!grp){MILO_WARN("
                 "\"%scouldnotfindgroup%s\",PathName(this),cc);return0;}"
                 "elsereturnPlay(grp->GetClip(),i,f1,f2,f3);}",
                 "latest CharDriver source exposes PlayGroup group branch");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::SetStarved(Symbolstarved){"
                 "mStarvedHandler=starved;}",
                 "latest CharDriver source exposes SetStarved assignment");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::PollDeps(std::list<Hmx::Object*>&changedBy,"
                 "std::list<Hmx::Object*>&change){change.push_back(mBones);}",
                 "latest CharDriver source exposes PollDeps bones dependency");
  ok &= contains(rb3_latest_char_driver_h,
                 "floatSetBlendWidth(floatw){mBlendWidth=w;}",
                 "latest CharDriver header exposes SetBlendWidth assignment");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::SetApply(ApplyModemode){if(mode!=mApply){"
                 "mApply=mode;SyncInternalBones();}}",
                 "latest CharDriver source exposes SetApply sync gate");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::SetClipType(Symbolty){if(mClipType!=ty){"
                 "mClipType=ty;SyncInternalBones();}}",
                 "latest CharDriver source exposes SetClipType sync gate");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "voidCharDriver::SyncInternalBones(){Clear();mLastNode="
                 "DataNode((Hmx::Object*)0);if(mInternalBones&&"
                 "mClipType.Null()){deletemInternalBones;mInternalBones=0;}",
                 "latest CharDriver source exposes SyncInternalBones reset/delete");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "elseif(!mInternalBones&&mApply==kApplyBlendWeights&&"
                 "!mClipType.Null()){mInternalBones=newCharBonesAlloc();}",
                 "latest CharDriver source exposes SyncInternalBones allocation gate");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "if(mInternalBones){mInternalBones->ClearBones();"
                 "CharBoneDir::StuffBones(*mInternalBones,mClipType);}",
                 "latest CharDriver source exposes SyncInternalBones StuffBones path");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "if(f1==-1.0f)f1=mBlendWidth;",
                 "latest CharDriver source exposes Play blend sentinel");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "if(mPlayMultipleClips){for(CharClipDriver*it=mFirst;it!=0;"
                 "it=it->mNext){if(clip==it->mClip)return0;}}",
                 "latest CharDriver source exposes duplicate clip gate");
  ok &= contains(rb3_latest_char_driver_cpp,
                 "CharClipDriver*CharDriver::FirstPlaying(){CharClipDriver*d;"
                 "for(d=mFirst;d!=0&&!d->mBlendFrac;d=d->Next());returnd;}",
                 "latest CharDriver source exposes FirstPlaying scan");
  ok &= contains(char_clip_h,
                 "structSourceCharClipDriverState{uint32_tplay_flags=0;"
                 "floatblend_width=0.0f;floattime_scale=1.0f;floatd_beat=0.0f;"
                 "floatadvance_beat=0.0f;boolhas_clip=false;boolhas_next=false;"
                 "intnext_event=-1;boolplay_multiple_clips=false;};",
                 "native character API exposes source CharClipDriver constructor state");
  ok &= contains(char_clip_h,
                 "structSourceCharClipDriverExitDecision{boolrecurse_next=false;"
                 "boolexecute_exit_event=false;boolend_sync_anim=false;"
                 "booldelete_self=false;std::optional<size_t>returned_stack_head;"
                 "std::vector<size_t>deleted_indices;};",
                 "native character API exposes source CharClipDriver Exit decision");
  ok &= contains(char_clip_h,
                 "uint32_tsource_char_clip_driver_masked_play_flags("
                 "uint32_tclip_play_flags,uint32_tmask);",
                 "native character API exposes raw source CharClipDriver mask helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipDriverStatesource_char_clip_driver_construct("
                 "uint32_tclip_play_flags,boolhas_clip,boolhas_next,"
                 "uint32_tmask,floatblend_width,boolplay_multiple_clips);",
                 "native character API exposes source CharClipDriver constructor helper");
  ok &= contains(char_clip_h,
                 "std::vector<size_t>source_char_clip_driver_delete_stack_order("
                 "size_tstack_size);",
                 "native character API exposes source CharClipDriver DeleteStack helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipDriverExitDecisionsource_char_clip_driver_exit_decision("
                 "size_tstack_size,boolexit_next,boolhas_sync_anim);",
                 "native character API exposes source CharClipDriver Exit helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipDriverDeleteClipResult"
                 "source_char_clip_driver_delete_clip_result("
                 "conststd::vector<bool>&clip_matches_source_order);",
                 "native character API exposes source CharClipDriver DeleteClip helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_clip_driver_should_execute_event("
                 "boolsymbol_null,boolclip_has_type_def);",
                 "native character API exposes source CharClipDriver ExecuteEvent helper");
  ok &= contains(char_clip_h,
                 "uint32_tchar_clip_driver_masked_play_flags(constCharClip&clip,"
                 "uint32_tmask);",
                 "native character API exposes source CharClipDriver flag mask helper");
  ok &= contains(char_clip_h,
                 "constchar*source_char_clip_beat_align_string(uint32_tmask);",
                 "native character API exposes source CharClip beat-align helper");
  ok &= contains(char_clip_h,
                 "structSourceCharClipFlagUpdate{uint32_tvalue=0;"
                 "booldirty=false;boolchanged=false;};",
                 "native character API exposes source CharClip flag update row");
  ok &= contains(char_clip_h,
                 "structSourceCharClipDefaultState{floatframes_per_sec=30.0f;"
                 "uint32_tflags=0;uint32_tplay_flags=0;floatrange=0.0f;"
                 "booldirty=true;booldo_not_compress=false;intunk42=-1;"
                 "size_tbeat_track_count=1;floatfirst_beat_frame=0.0f;"
                 "floatfirst_beat_value=0.0f;};",
                 "native character API exposes source CharClip default-state row");
  ok &= contains(char_clip_h,
                 "structSourceCharClipBeatEvent{std::stringevent;"
                 "floatbeat=0.0f;};",
                 "native character API exposes source CharClip BeatEvent row");
  ok &= contains(char_clip_h,
                 "structSourceCharClipResourceLookup{boolhas_type_def=false;"
                 "boolhas_resource_array=false;std::stringresource_name;"
                 "boolfound_resource=false;boolwarn_no_resource=false;};",
                 "native character API exposes source CharClip resource lookup row");
  ok &= contains(char_clip_h,
                 "structSourceCharClipTransitionsState{boolhas_owner=false;"
                 "std::vector<int>node_sizes;};",
                 "native character API exposes source CharClip Transitions state row");
  ok &= contains(char_clip_h,
                 "structSourceCharClipTransitionsClearResult{size_treleased_clips=0;"
                 "boolresized_zero=false;};",
                 "native character API exposes source CharClip Transitions clear result");
  ok &= contains(char_clip_h,
                 "structSourceCharClipPoseMeshesSteps{std::stringtemp_meshes_name;"
                 "boolstuff_bones=false;boolscale_down=false;"
                 "floatscale_down_weight=0.0f;boolscale_add=false;",
                 "native character API exposes source CharClip PoseMeshes step row");
  ok &= contains(char_clip_h,
                 "floatscale_add_weight=0.0f;floatscale_add_frame=0.0f;"
                 "floatscale_add_blend=0.0f;boolpose_meshes=false;};",
                 "native character API exposes source CharClip PoseMeshes args");
  ok &= contains(char_clip_h,
                 "SourceCharClipDefaultStatesource_char_clip_default_state();",
                 "native character API exposes source CharClip default-state helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipBeatEventsource_char_clip_beat_event_default();",
                 "native character API exposes source CharClip BeatEvent default helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipBeatEventsource_char_clip_beat_event_copy("
                 "constSourceCharClipBeatEvent&source);",
                 "native character API exposes source CharClip BeatEvent copy helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_clip_beat_event_assign("
                 "SourceCharClipBeatEvent&dest,constSourceCharClipBeatEvent&source);",
                 "native character API exposes source CharClip BeatEvent assignment helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipBeatEventsource_char_clip_beat_event_loaded("
                 "conststd::string&event,floatbeat);",
                 "native character API exposes source CharClip BeatEvent load helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipResourceLookupsource_char_clip_get_resource("
                 "boolhas_type_def,boolhas_resource_array,conststd::string&"
                 "resource_name,boolresource_found);",
                 "native character API exposes source CharClip GetResource helper");
  ok &= contains(char_clip_h,
                 "intsource_char_clip_get_context(boolhas_type_def,"
                 "boolhas_resource_array,intresource_context);",
                 "native character API exposes source CharClip GetContext helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipTransitionsStatesource_char_clip_transitions_construct("
                 "boolhas_owner);",
                 "native character API exposes source CharClip Transitions construct helper");
  ok &= contains(char_clip_h,
                 "size_tsource_char_clip_transitions_size("
                 "constSourceCharClipTransitionsState&transitions);",
                 "native character API exposes source CharClip Transitions size helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipTransitionsClearResult"
                 "source_char_clip_transitions_clear("
                 "SourceCharClipTransitionsState&transitions);",
                 "native character API exposes source CharClip Transitions clear helper");
  ok &= contains(char_clip_h,
                 "std::vector<SourceCharBonesBone>source_char_clip_stuff_bones("
                 "conststd::vector<SourceCharBonesBone>&existing_bones,"
                 "conststd::vector<SourceCharBonesBone>&listed_bones);",
                 "native character API exposes source CharClip StuffBones helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipPoseMeshesStepssource_char_clip_pose_meshes_steps("
                 "floatframe);",
                 "native character API exposes source CharClip PoseMeshes helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipFlagUpdatesource_char_clip_set_flags("
                 "uint32_tcurrent_flags,boolcurrent_dirty,"
                 "uint32_trequested_flags);",
                 "native character API exposes source CharClip SetFlags helper");
  ok &= contains(char_clip_h,
                 "SourceCharClipFlagUpdatesource_char_clip_set_play_flags("
                 "uint32_tcurrent_play_flags,boolcurrent_dirty,"
                 "uint32_trequested_play_flags);",
                 "native character API exposes source CharClip SetPlayFlags helper");
  ok &= contains(char_clip_h,
                 "structSourceCharClipRefOwner{boolis_clip_group=false;"
                 "std::vector<std::string>group_clips;};",
                 "native character API exposes source CharClip ref-owner row");
  ok &= contains(char_clip_h,
                 "boolsource_char_clip_shares_groups(conststd::vector<"
                 "SourceCharClipRefOwner>&ref_owners,conststd::string&"
                 "candidate_clip_name);",
                 "native character API exposes source CharClip SharesGroups helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_driver_starved(boolhas_first,"
                 "boolfirst_has_next,uint32_tfirst_play_flags);",
                 "native character API exposes source CharDriver starved helper");
  ok &= contains(char_clip_h,
                 "floatsource_char_driver_resolve_blend_width("
                 "floatrequested_blend_width,floatdriver_blend_width);",
                 "native character API exposes source CharDriver blend helper");
  ok &= contains(char_clip_h,
                 "boolsource_char_driver_should_start_clip("
                 "boolplay_multiple_clips,boolclip_already_playing);",
                 "native character API exposes source CharDriver duplicate helper");
  ok &= contains(char_clip_h,
                 "std::optional<size_t>source_char_driver_first_playing_index("
                 "conststd::vector<float>&source_stack_blend_fracs);",
                 "native character API exposes source CharDriver FirstPlaying helper");
  ok &= contains(char_clip_h,
                 "enumSourceCharDriverApplyMode{kSourceCharDriverApplyBlend=0,"
                 "kSourceCharDriverApplyAdd=1,kSourceCharDriverApplyRotateTo=2,"
                 "kSourceCharDriverApplyBlendWeights=3,};",
                 "native character API exposes source CharDriver apply enum order");
  ok &= contains(char_clip_h,
                 "structSourceCharDriverState{boolhas_bones=false;"
                 "boolhas_clips=false;boolhas_first=false;boolhas_test_clip=false;"
                 "boolhas_default_clip=false;booldefault_play_starved=false;"
                 "std::stringstarved_handler;boollast_node_valid=false;"
                 "floatold_beat=1.0e30f;",
                 "native character API exposes source CharDriver state defaults");
  ok &= contains(char_clip_h,
                 "SourceCharDriverStatesource_char_driver_default_state();",
                 "native character API exposes source CharDriver default state helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_clear(SourceCharDriverState&state);",
                 "native character API exposes source CharDriver Clear helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverEnterDecisionsource_char_driver_enter("
                 "SourceCharDriverState&state);",
                 "native character API exposes source CharDriver Enter helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_transfer(SourceCharDriverState&state,"
                 "constSourceCharDriverState&driver);",
                 "native character API exposes source CharDriver Transfer helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_set_clips(SourceCharDriverState&state,"
                 "boolhas_clips);",
                 "native character API exposes source CharDriver SetClips helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_set_bones(SourceCharDriverState&state,"
                 "boolhas_bones);",
                 "native character API exposes source CharDriver SetBones helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_set_starved("
                 "SourceCharDriverState&state,conststd::string&starved_handler);",
                 "native character API exposes source CharDriver SetStarved helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_set_blend_width("
                 "SourceCharDriverState&state,floatblend_width);",
                 "native character API exposes source CharDriver SetBlendWidth helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverSyncDecisionsource_char_driver_sync_internal_bones("
                 "SourceCharDriverState&state);",
                 "native character API exposes source CharDriver SyncInternalBones helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverSyncDecisionsource_char_driver_set_apply("
                 "SourceCharDriverState&state,SourceCharDriverApplyModeapply);",
                 "native character API exposes source CharDriver SetApply helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverSyncDecisionsource_char_driver_set_clip_type("
                 "SourceCharDriverState&state,conststd::string&clip_type);",
                 "native character API exposes source CharDriver SetClipType helper");
  ok &= contains(char_clip_h,
                 "SourceCharDriverPlayGroupDecisionsource_char_driver_play_group_decision("
                 "boolhas_clip_dir,boolfound_group);",
                 "native character API exposes source CharDriver PlayGroup helper");
  ok &= contains(char_clip_h,
                 "voidsource_char_driver_poll_deps(SourceCharDriverPollDeps&deps,"
                 "conststd::string&bones);",
                 "native character API exposes source CharDriver PollDeps helper");
  ok &= contains(char_clip_h,
                 "floatsource_driver_blend_width_=1.0f;",
                 "native CharClipPlayer stores source driver blend default");
  ok &= contains(char_clip_h,
                 "boolsource_play_multiple_clips_=false;",
                 "native CharClipPlayer stores source play-multiple default");
  ok &= contains(char_clip,
                 "uint32_tsource_char_clip_driver_masked_play_flags("
                 "uint32_tclip_play_flags,uint32_tmask){uint32_tplay_flags="
                 "clip_play_flags;",
                 "native CharClipDriver raw mask helper starts from source clip flags");
  ok &= contains(char_clip,
                 "uint32_tchar_clip_driver_masked_play_flags(constCharClip&clip,"
                 "uint32_tmask){returnsource_char_clip_driver_masked_play_flags("
                 "clip.default_play_flags,mask);}",
                 "native CharClipDriver mask helper starts from stored clip flags");
  ok &= contains(char_clip,
                 "if(mask&0xF0u)play_flags=(play_flags&0xffffff0fu)|"
                 "(mask&0xF0u);if(mask&0x0Fu)play_flags=("
                 "play_flags&0xfffffff0u)|(mask&0x0Fu);if(mask&0xF600u){"
                 "play_flags=(play_flags&0xffff09ffu)|(mask&0xF600u);}",
                 "native CharClipDriver mask helper matches source bit groups");
  ok &= contains(char_clip,
                 "SourceCharClipDriverStatesource_char_clip_driver_construct("
                 "uint32_tclip_play_flags,boolhas_clip,boolhas_next,uint32_tmask,"
                 "floatblend_width,boolplay_multiple_clips){SourceCharClipDriverState"
                 "state;state.play_flags=source_char_clip_driver_masked_play_flags("
                 "clip_play_flags,mask);",
                 "native CharClipDriver constructor helper ports masked flags");
  ok &= contains(char_clip,
                 "state.blend_width=blend_width;state.time_scale=1.0f;"
                 "state.d_beat=0.0f;state.advance_beat=0.0f;"
                 "state.has_clip=has_clip;state.has_next=has_next;"
                 "state.next_event=-1;state.play_multiple_clips=play_multiple_clips;",
                 "native CharClipDriver constructor helper ports initialized fields");
  ok &= contains(char_clip,
                 "std::vector<size_t>source_char_clip_driver_delete_stack_order("
                 "size_tstack_size){std::vector<size_t>deleted;for(size_ti="
                 "stack_size;i>0;--i){deleted.push_back(i-1);}returndeleted;}",
                 "native CharClipDriver DeleteStack helper ports tail-first order");
  ok &= contains(char_clip,
                 "SourceCharClipDriverExitDecisionsource_char_clip_driver_exit_decision("
                 "size_tstack_size,boolexit_next,boolhas_sync_anim){"
                 "SourceCharClipDriverExitDecisiondecision;if(stack_size==0)"
                 "returndecision;decision.execute_exit_event=true;",
                 "native CharClipDriver Exit helper ports event/delete defaults");
  ok &= contains(char_clip,
                 "if(exit_next&&stack_size>1){decision.recurse_next=true;"
                 "decision.deleted_indices=source_char_clip_driver_delete_stack_order("
                 "stack_size);}else{decision.deleted_indices.push_back(0);"
                 "if(stack_size>1)decision.returned_stack_head=1;}",
                 "native CharClipDriver Exit helper ports recursive and self-only branches");
  ok &= contains(char_clip,
                 "SourceCharClipDriverDeleteClipResult"
                 "source_char_clip_driver_delete_clip_result(conststd::vector<bool>&"
                 "clip_matches_source_order){SourceCharClipDriverDeleteClipResultresult;",
                 "native CharClipDriver DeleteClip helper scans source stack");
  ok &= contains(char_clip,
                 "boolsource_char_clip_driver_should_execute_event("
                 "boolsymbol_null,boolclip_has_type_def){return!symbol_null&&"
                 "clip_has_type_def;}",
                 "native CharClipDriver ExecuteEvent helper ports source guard");
  ok &= contains(char_clip,
                 "constchar*source_char_clip_beat_align_string(uint32_tmask){"
                 "switch(mask&0xF600u){casekCharPlayRealTime:return"
                 "\"RealTime\";",
                 "native CharClip beat-align helper ports source switch");
  ok &= contains(char_clip,
                 "SourceCharClipFlagUpdatesource_char_clip_set_flags("
                 "uint32_tcurrent_flags,boolcurrent_dirty,"
                 "uint32_trequested_flags){SourceCharClipFlagUpdateupdate;"
                 "update.value=current_flags;update.dirty=current_dirty;"
                 "if(requested_flags!=current_flags){update.value="
                 "requested_flags;update.dirty=true;update.changed=true;}"
                 "returnupdate;}",
                 "native CharClip SetFlags helper ports source dirty guard");
  ok &= contains(char_clip,
                 "SourceCharClipDefaultStatesource_char_clip_default_state(){"
                 "returnSourceCharClipDefaultState{};}",
                 "native CharClip default-state helper returns source defaults");
  ok &= contains(char_clip,
                 "SourceCharClipBeatEventsource_char_clip_beat_event_default(){"
                 "returnSourceCharClipBeatEvent{};}",
                 "native CharClip BeatEvent default helper returns source defaults");
  ok &= contains(char_clip,
                 "SourceCharClipBeatEventsource_char_clip_beat_event_copy("
                 "constSourceCharClipBeatEvent&source){returnSourceCharClipBeatEvent{"
                 "source.event,source.beat};}",
                 "native CharClip BeatEvent copy helper ports source fields");
  ok &= contains(char_clip,
                 "voidsource_char_clip_beat_event_assign("
                 "SourceCharClipBeatEvent&dest,constSourceCharClipBeatEvent&source){"
                 "dest.event=source.event;dest.beat=source.beat;}",
                 "native CharClip BeatEvent assignment helper ports source fields");
  ok &= contains(char_clip,
                 "SourceCharClipBeatEventsource_char_clip_beat_event_loaded("
                 "conststd::string&event,floatbeat){SourceCharClipBeatEventloaded;"
                 "loaded.event=event;loaded.beat=beat;returnloaded;}",
                 "native CharClip BeatEvent load helper ports read order fields");
  ok &= contains(char_clip,
                 "SourceCharClipResourceLookupsource_char_clip_get_resource("
                 "boolhas_type_def,boolhas_resource_array,conststd::string&"
                 "resource_name,boolresource_found){SourceCharClipResourceLookup"
                 "lookup;lookup.has_type_def=has_type_def;lookup.has_resource_array="
                 "has_resource_array;if(has_type_def&&has_resource_array){"
                 "lookup.resource_name=resource_name;lookup.found_resource="
                 "resource_found;}lookup.warn_no_resource=!lookup.found_resource;"
                 "returnlookup;}",
                 "native CharClip GetResource helper ports lookup and warning");
  ok &= contains(char_clip,
                 "intsource_char_clip_get_context(boolhas_type_def,"
                 "boolhas_resource_array,intresource_context){if(has_type_def&&"
                 "has_resource_array)returnresource_context;return0;}",
                 "native CharClip GetContext helper ports source fallback");
  ok &= contains(char_clip,
                 "SourceCharClipTransitionsStatesource_char_clip_transitions_construct("
                 "boolhas_owner){SourceCharClipTransitionsStatestate;"
                 "state.has_owner=has_owner;returnstate;}",
                 "native CharClip Transitions construct helper ports owner state");
  ok &= contains(char_clip,
                 "size_tsource_char_clip_transitions_size("
                 "constSourceCharClipTransitionsState&transitions){"
                 "returntransitions.node_sizes.size();}",
                 "native CharClip Transitions size helper ports node-vector count");
  ok &= contains(char_clip,
                 "SourceCharClipTransitionsClearResult"
                 "source_char_clip_transitions_clear(SourceCharClipTransitionsState&"
                 "transitions){SourceCharClipTransitionsClearResultresult;"
                 "result.released_clips=source_char_clip_transitions_size("
                 "transitions);transitions.node_sizes.clear();"
                 "result.resized_zero=true;returnresult;}",
                 "native CharClip Transitions clear helper ports release and resize");
  ok &= contains(char_clip,
                 "std::vector<SourceCharBonesBone>source_char_clip_stuff_bones("
                 "conststd::vector<SourceCharBonesBone>&existing_bones,"
                 "conststd::vector<SourceCharBonesBone>&listed_bones){"
                 "std::vector<SourceCharBonesBone>bones=existing_bones;"
                 "bones.insert(bones.end(),listed_bones.begin(),"
                 "listed_bones.end());returnbones;}",
                 "native CharClip StuffBones helper ports append flow");
  ok &= contains(char_clip,
                 "SourceCharClipPoseMeshesStepssource_char_clip_pose_meshes_steps("
                 "floatframe){SourceCharClipPoseMeshesStepssteps;"
                 "steps.temp_meshes_name=\"tmp_viseme_bones\";"
                 "steps.stuff_bones=true;steps.scale_down=true;"
                 "steps.scale_down_weight=0.0f;steps.scale_add=true;",
                 "native CharClip PoseMeshes helper ports first call arguments");
  ok &= contains(char_clip,
                 "steps.scale_add_weight=1.0f;steps.scale_add_frame=frame;"
                 "steps.scale_add_blend=0.0f;steps.pose_meshes=true;"
                 "returnsteps;}",
                 "native CharClip PoseMeshes helper ports ScaleAdd arguments");
  ok &= contains(char_clip,
                 "SourceCharClipFlagUpdatesource_char_clip_set_play_flags("
                 "uint32_tcurrent_play_flags,boolcurrent_dirty,"
                 "uint32_trequested_play_flags){SourceCharClipFlagUpdateupdate;"
                 "update.value=current_play_flags;update.dirty=current_dirty;"
                 "if(requested_play_flags!=current_play_flags){update.value="
                 "requested_play_flags;update.dirty=true;update.changed=true;}"
                 "returnupdate;}",
                 "native CharClip SetPlayFlags helper ports source dirty guard");
  ok &= contains(char_clip,
                 "boolsource_char_clip_shares_groups(conststd::vector<"
                 "SourceCharClipRefOwner>&ref_owners,conststd::string&"
                 "candidate_clip_name){for(autoit=ref_owners.rbegin();it!="
                 "ref_owners.rend();++it){if(!it->is_clip_group)continue;",
                 "native CharClip SharesGroups helper scans ref owners in reverse");
  ok &= contains(char_clip,
                 "std::find(it->group_clips.begin(),it->group_clips.end(),"
                 "candidate_clip_name)!=it->group_clips.end()){returntrue;}",
                 "native CharClip SharesGroups helper checks group membership");
  ok &= contains(char_clip,
                 "constuint32_tplay_flags=char_clip_driver_masked_play_flags("
                 "clip,flags);",
                 "native CharClipPlayer applies source CharClipDriver flag mask");
  ok &= contains(char_clip,
                 "next.flags=play_flags;",
                 "native CharClipPlayer stores source-masked play flags");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_set_flags(0x12u,false,0x12u)",
                 "focused clip driver flags test covers unchanged SetFlags");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_set_flags(0x12u,false,0x34u)",
                 "focused clip driver flags test covers changed SetFlags");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_set_play_flags(0x20u,false,0x20u)",
                 "focused clip driver flags test covers unchanged SetPlayFlags");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_set_play_flags(0x20u,false,0x10u)",
                 "focused clip driver flags test covers changed SetPlayFlags");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_default_state()",
                 "focused clip driver flags test covers CharClip constructor defaults");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_beat_event_default()",
                 "focused clip driver flags test covers BeatEvent default");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_beat_event_loaded(\"solo_hit\",12.5f)",
                 "focused clip driver flags test covers BeatEvent load helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_beat_event_copy(loaded_event)",
                 "focused clip driver flags test covers BeatEvent copy");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_beat_event_assign(assigned_event,"
                 "loaded_event)",
                 "focused clip driver flags test covers BeatEvent assignment");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_get_resource(true,true,\"rock1_resource\",true)",
                 "focused clip driver flags test covers found GetResource helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_get_resource(false,true,\"ignored_resource\",true)",
                 "focused clip driver flags test covers missing TypeDef resource fallback");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_get_context(true,true,0x27)",
                 "focused clip driver flags test covers GetContext resource value");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_transitions_construct(true)",
                 "focused clip driver flags test covers Transitions constructor");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_transitions_size(transitions)",
                 "focused clip driver flags test covers Transitions Size");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_transitions_clear(transitions)",
                 "focused clip driver flags test covers Transitions Clear");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_stuff_bones(",
                 "focused clip driver flags test covers StuffBones append flow");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_pose_meshes_steps(14.25f)",
                 "focused clip driver flags test covers PoseMeshes step flow");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_shares_groups(",
                 "focused clip driver flags test covers CharClip SharesGroups helper");
  ok &= contains(char_clip,
                 "boolsource_char_driver_starved(boolhas_first,"
                 "boolfirst_has_next,uint32_tfirst_play_flags){if(has_first){"
                 "if(first_has_next)returnfalse;if((first_play_flags&0xF0u)"
                 "==kCharPlayNoLoop)returnfalse;}returntrue;}",
                 "native CharDriver starved helper ports source body");
  ok &= contains(char_clip,
                 "boolCharClipPlayer::source_starved()const{if(layers_.empty())"
                 "returnsource_char_driver_starved(false,false,0);",
                 "native CharClipPlayer reports source starved state");
  ok &= contains(char_clip,
                 "floatsource_char_driver_resolve_blend_width("
                 "floatrequested_blend_width,floatdriver_blend_width){return"
                 "requested_blend_width==-1.0f?driver_blend_width:"
                 "requested_blend_width;}",
                 "native CharDriver blend helper ports source sentinel");
  ok &= contains(char_clip,
                 "source_char_driver_resolve_blend_width(blend_width,"
                 "source_driver_blend_width_);",
                 "native CharClipPlayer uses source driver blend fallback");
  ok &= contains(char_clip,
                 "boolsource_char_driver_should_start_clip("
                 "boolplay_multiple_clips,boolclip_already_playing){if("
                 "play_multiple_clips&&clip_already_playing)returnfalse;"
                 "returntrue;}",
                 "native CharDriver duplicate helper ports source gate");
  ok &= contains(char_clip,
                 "if(source_play_multiple_clips_){for(constLayer&layer:"
                 "layers_){if(layer.clip==&clip){clip_already_playing=true;",
                 "native CharClipPlayer checks source duplicate clip stack");
  ok &= contains(char_clip,
                 "if(!source_char_driver_should_start_clip("
                 "source_play_multiple_clips_,clip_already_playing)){return;}",
                 "native CharClipPlayer applies source duplicate gate");
  ok &= contains(char_clip,
                 "std::optional<size_t>source_char_driver_first_playing_index("
                 "conststd::vector<float>&source_stack_blend_fracs){for("
                 "size_ti=0;i<source_stack_blend_fracs.size();++i){if("
                 "source_stack_blend_fracs[i]!=0.0f)returni;}returnstd::nullopt;}",
                 "native CharDriver FirstPlaying helper ports source scan");
  ok &= contains(char_clip,
                 "SourceCharDriverStatesource_char_driver_default_state(){"
                 "returnSourceCharDriverState{};}",
                 "native CharDriver default state helper returns source defaults");
  ok &= contains(char_clip,
                 "voidsource_char_driver_clear(SourceCharDriverState&state){"
                 "state.has_first=false;}",
                 "native CharDriver Clear helper ports source mFirst reset");
  ok &= contains(char_clip,
                 "SourceCharDriverEnterDecisionsource_char_driver_enter("
                 "SourceCharDriverState&state){SourceCharDriverEnterDecisiondecision;"
                 "decision.changed=true;decision.clear_stack=true;"
                 "decision.reset_last_node=true;decision.reset_old_beat=true;"
                 "decision.reset_beat_scale=true;source_char_driver_clear(state);",
                 "native CharDriver Enter helper ports source resets");
  ok &= contains(char_clip,
                 "state.old_beat=1.0e30f;state.beat_scale=1.0f;"
                 "if(state.has_default_clip){decision.play_default_clip=true;"
                 "state.last_node_valid=true;}returndecision;}",
                 "native CharDriver Enter helper ports default clip branch");
  ok &= contains(char_clip,
                 "voidsource_char_driver_transfer(SourceCharDriverState&state,"
                 "constSourceCharDriverState&driver){source_char_driver_clear(state);"
                 "state.has_clips=driver.has_clips;state.last_node_valid="
                 "driver.last_node_valid;state.realign=driver.realign;",
                 "native CharDriver Transfer helper ports source copied fields");
  ok &= contains(char_clip,
                 "voidsource_char_driver_set_clips(SourceCharDriverState&state,"
                 "boolhas_clips){if(has_clips!=state.has_clips){"
                 "state.last_node_valid=false;state.has_clips=has_clips;}}",
                 "native CharDriver SetClips helper ports changed-directory gate");
  ok &= contains(char_clip,
                 "voidsource_char_driver_set_bones(SourceCharDriverState&state,"
                 "boolhas_bones){state.has_bones=has_bones;}",
                 "native CharDriver SetBones helper ports source assignment");
  ok &= contains(char_clip,
                 "voidsource_char_driver_set_starved("
                 "SourceCharDriverState&state,conststd::string&starved_handler){"
                 "state.starved_handler=starved_handler;}",
                 "native CharDriver SetStarved helper ports source assignment");
  ok &= contains(char_clip,
                 "voidsource_char_driver_set_blend_width("
                 "SourceCharDriverState&state,floatblend_width){"
                 "state.blend_width=blend_width;}",
                 "native CharDriver SetBlendWidth helper ports source assignment");
  ok &= contains(char_clip,
                 "SourceCharDriverSyncDecisionsource_char_driver_sync_internal_bones("
                 "SourceCharDriverState&state){SourceCharDriverSyncDecisiondecision;"
                 "decision.changed=true;decision.clear_stack=true;"
                 "decision.reset_last_node=true;source_char_driver_clear(state);"
                 "state.last_node_valid=false;",
                 "native CharDriver SyncInternalBones helper clears stack and last node");
  ok &= contains(char_clip,
                 "if(state.has_internal_bones&&state.clip_type.empty()){"
                 "decision.delete_internal_bones=true;state.has_internal_bones=false;}"
                 "elseif(!state.has_internal_bones&&state.apply=="
                 "kSourceCharDriverApplyBlendWeights&&!state.clip_type.empty()){",
                 "native CharDriver SyncInternalBones helper ports allocation gate");
  ok &= contains(char_clip,
                 "if(state.has_internal_bones){decision.clear_internal_bones=true;"
                 "decision.stuff_internal_bones=true;}",
                 "native CharDriver SyncInternalBones helper records StuffBones path");
  ok &= contains(char_clip,
                 "SourceCharDriverSyncDecisionsource_char_driver_set_apply("
                 "SourceCharDriverState&state,SourceCharDriverApplyModeapply){"
                 "if(apply==state.apply)returnSourceCharDriverSyncDecision{};"
                 "state.apply=apply;returnsource_char_driver_sync_internal_bones(state);}",
                 "native CharDriver SetApply helper ports source sync guard");
  ok &= contains(char_clip,
                 "SourceCharDriverSyncDecisionsource_char_driver_set_clip_type("
                 "SourceCharDriverState&state,conststd::string&clip_type){"
                 "if(clip_type==state.clip_type)returnSourceCharDriverSyncDecision{};"
                 "state.clip_type=clip_type;returnsource_char_driver_sync_internal_bones(state);}",
                 "native CharDriver SetClipType helper ports source sync guard");
  ok &= contains(char_clip,
                 "SourceCharDriverPlayGroupDecisionsource_char_driver_play_group_decision("
                 "boolhas_clip_dir,boolfound_group){SourceCharDriverPlayGroupDecision"
                 "decision;decision.has_clip_dir=has_clip_dir;"
                 "decision.found_group=found_group;decision.request_play="
                 "has_clip_dir&&found_group;returndecision;}",
                 "native CharDriver PlayGroup helper ports source decision branches");
  ok &= contains(char_clip,
                 "voidsource_char_driver_poll_deps(SourceCharDriverPollDeps&deps,"
                 "conststd::string&bones){deps.change.push_back(bones);}",
                 "native CharDriver PollDeps helper ports source change list");
  ok &= missing(char_clip,
                "blend_width>=0.0f?blend_width:std::max(0.0f,clip.blend_width)",
                "native CharClipPlayer no longer falls back to clip blend width");
  ok &= contains(clip_driver_flags_test,
                 "if(mask&0xF0u)out=(out&0xffffff0fu)|(mask&0xF0u);",
                 "focused flag-mask test covers source loop-bit branch");
  ok &= contains(clip_driver_flags_test,
                 "if(mask&0xF600u)out=(out&0xffff09ffu)|(mask&0xF600u);",
                 "focused flag-mask test covers source beat-time branch");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_driver_construct(",
                 "focused flag-mask test covers CharClipDriver constructor helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_driver_delete_stack_order(3)",
                 "focused flag-mask test covers CharClipDriver DeleteStack helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_driver_exit_decision(3,true,true)",
                 "focused flag-mask test covers CharClipDriver Exit recursive helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_driver_delete_clip_result(",
                 "focused flag-mask test covers CharClipDriver DeleteClip helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_driver_should_execute_event(",
                 "focused flag-mask test covers CharClipDriver ExecuteEvent helper");
  ok &= contains(clip_driver_flags_test,
                 "expect_beat_align(0x8000u,\"BeatAlign8\","
                 "\"beatalign8\")",
                 "focused flag-mask test covers source BeatAlign8 label");
  ok &= contains(clip_driver_flags_test,
                 "expect_beat_align(0xF623u,\"NoAlign\","
                 "\"maskedunknownalign\")",
                 "focused flag-mask test covers source default beat-align label");
  ok &= contains(clip_driver_flags_test,
                 "expect_starved(false,false,0,true,\"emptystack\")",
                 "focused flag-mask test covers empty stack starved branch");
  ok &= contains(clip_driver_flags_test,
                 "expect_starved(true,true,ghogx::character::kCharPlayLoop,"
                 "false,\"stackhasnext\")",
                 "focused flag-mask test covers next clip non-starved branch");
  ok &= contains(clip_driver_flags_test,
                 "expect_starved(true,false,ghogx::character::kCharPlayNoLoop,"
                 "false,\"singleno-loopclip\")",
                 "focused flag-mask test covers no-loop non-starved branch");
  ok &= contains(clip_driver_flags_test,
                 "expect_blend(-1.0f,1.0f,1.0f,\"sourcedefaultblend\")",
                 "focused flag-mask test covers source default blend fallback");
  ok &= contains(clip_driver_flags_test,
                 "expect_blend(-0.5f,1.0f,-0.5f,"
                 "\"non-sentinelnegativeblend\")",
                 "focused flag-mask test covers exact -1 sentinel");
  ok &= contains(clip_driver_flags_test,
                 "expect_should_start(false,true,true,\"duplicatesalloweddefault\")",
                 "focused flag-mask test covers source duplicate default");
  ok &= contains(clip_driver_flags_test,
                 "expect_should_start(true,true,false,"
                 "\"duplicateclipinmultimode\")",
                 "focused flag-mask test covers source duplicate suppression");
  ok &= contains(clip_driver_flags_test,
                 "expect_first_playing({},std::nullopt,\"emptysourcestack\")",
                 "focused flag-mask test covers empty FirstPlaying stack");
  ok &= contains(clip_driver_flags_test,
                 "expect_first_playing({0.0f,0.25f,1.0f},"
                 "static_cast<size_t>(1),\"skipzeroblendnodes\")",
                 "focused flag-mask test covers FirstPlaying zero-skip scan");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_default_state()",
                 "focused flag-mask test covers CharDriver constructor defaults");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_set_clips(state,true)",
                 "focused flag-mask test covers CharDriver SetClips change");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_enter(state)",
                 "focused flag-mask test covers CharDriver Enter helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_set_starved(state,\"starved.msg\")",
                 "focused flag-mask test covers CharDriver SetStarved helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_set_blend_width(state,0.75f)",
                 "focused flag-mask test covers CharDriver SetBlendWidth helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_set_apply(state,ghogx::character::"
                 "kSourceCharDriverApplyBlendWeights)",
                 "focused flag-mask test covers CharDriver blend-weights allocation");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_set_clip_type(state,\"\")",
                 "focused flag-mask test covers CharDriver empty clip-type delete");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_transfer(dest,source)",
                 "focused flag-mask test covers CharDriver Transfer copy");
  ok &= contains(clip_driver_flags_test,
                 "expect_play_group_decision(true,true,true,\"groupfound\")",
                 "focused flag-mask test covers CharDriver PlayGroup success");
  ok &= contains(clip_driver_flags_test,
                 "source_char_driver_poll_deps(deps,\"bone.servo\")",
                 "focused flag-mask test covers CharDriver PollDeps helper");
  ok &= contains(doc,
                 "Native `SourceCharDriverState` records the checked `CharDriver`",
                 "document records native CharDriver source state");
  ok &= contains(doc,
                 "SetClips` only resets `mLastNode` when the clip directory",
                 "document records CharDriver SetClips source gate");
  ok &= contains(doc,
                 "allocated only for `kApplyBlendWeights` plus non-null clip type",
                 "document records CharDriver SyncInternalBones allocation gate");
  ok &= contains(doc,
                 "Native `source_char_driver_enter`, `source_char_driver_set_starved`",
                 "document records remaining concrete CharDriver helper slice");
  ok &= contains(doc,
                 "`PollDeps` publishes `mBones` in the change list",
                 "document records CharDriver PollDeps source direction");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "classCharClipSet:publicObjectDir,publicRndDrawable,"
                 "publicRndAnimatable",
                 "latest CharClipSet header exposes inheritance");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "FilePathmCharFilePath;",
                 "latest CharClipSet header exposes character file path");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "ObjPtr<RndDir,ObjectDir>mPreviewChar;",
                 "latest CharClipSet header exposes preview character");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "ObjPtr<CharClip,ObjectDir>mPreviewClip;",
                 "latest CharClipSet header exposes preview clip");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "intmFilterFlags;",
                 "latest CharClipSet header exposes filter flags");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "intmBpm;",
                 "latest CharClipSet header exposes bpm");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "boolmPreviewWalk;",
                 "latest CharClipSet header exposes preview walk");
  ok &= contains(rb3_latest_char_clip_set_h,
                 "ObjPtr<CharClip,ObjectDir>mStillClip;",
                 "latest CharClipSet header exposes still clip");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "CharClipSet::CharClipSet():mCharFilePath(),"
                 "mPreviewChar(this,0),mPreviewClip(this,0),"
                 "mStillClip(this,0){ResetPreviewState();mRate=k1_fpb;}",
                 "latest CharClipSet source constructor flow");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::ResetPreviewState(){deletemPreviewChar;"
                 "mPreviewClip=0;mStillClip=0;mCharFilePath.SetRoot(\"\");"
                 "mFilterFlags=0;mBpm=90;mPreviewWalk=false;}",
                 "latest CharClipSet source ResetPreviewState");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::RandomizeGroups(){for(ObjDirItr<"
                 "CharClipGroup>it(this,false);it!=0;++it){it->Randomize();}}",
                 "latest CharClipSet source RandomizeGroups");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::SortGroups(){for(ObjDirItr<CharClipGroup>"
                 "it(this,false);it!=0;++it){it->Sort();}}",
                 "latest CharClipSet source SortGroups");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::PreSave(BinStream&bs){if(mPreviewChar)"
                 "mPreviewChar->SetName(\"\",0);if(bs.Cached()){"
                 "ResetPreviewState();ResetEditorState();}}",
                 "latest CharClipSet source PreSave");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::PostSave(BinStream&bs){ObjectDir::"
                 "PostSave(bs);if(mPreviewChar){mPreviewChar->SetName("
                 "\"preview_character\",this);mPreviewChar->Enter();",
                 "latest CharClipSet source PostSave prefix");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "if(obj)obj->Handle(Message(\"update_objects\"),true);}}",
                 "latest CharClipSet source PostSave update");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::PreLoad(BinStream&bs){LOAD_REVS(bs)"
                 "ASSERT_REVS(0x18,0)MILO_ASSERT(gRev>3,0xA2);"
                 "PushRev(packRevs(gAltRev,gRev),this);ObjectDir::PreLoad(bs);}",
                 "latest CharClipSet source PreLoad");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::PostLoad(BinStream&bs){ObjectDir::"
                 "PostLoad(bs);intrevs=PopRev(this);gRev=getHmxRev(revs);"
                 "gAltRev=getAltRev(revs);if(IsProxy())return;",
                 "latest CharClipSet source PostLoad prefix");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "if(gRev<0x11){bs.ReadInt();bs.ReadInt();}"
                 "if(gRev==0xF||gRev==0x10)bs.ReadInt();",
                 "latest CharClipSet source legacy int gates");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "if(gRev<0x18){intcount=0;for(ObjDirItr<CharClip>it("
                 "this,true);it!=0;++it){count++;}for(inti=0;i<count;i++){",
                 "latest CharClipSet source legacy clip triplets");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "if(gRev<0xD)Handle(filter_clips_msg,false);"
                 "if(gRev>0x11){bs>>mCharFilePath;bs>>mPreviewClip;}"
                 "if(gRev>0x13)bs>>mFilterFlags;if(gRev>0x14)bs>>mBpm;"
                 "if(gRev>0x15)bs>>mPreviewWalk;if(gRev>0x16)bs>>mStillClip;",
                 "latest CharClipSet source modern field gates");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "COPY_MEMBER(mCharFilePath)COPY_MEMBER(mPreviewClip)"
                 "COPY_MEMBER(mFilterFlags)COPY_MEMBER(mBpm)"
                 "COPY_MEMBER(mPreviewWalk)COPY_MEMBER(mStillClip)",
                 "latest CharClipSet source Copy members");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::LoadCharacter(){MILO_ASSERT("
                 "TheLoadMgr.EditMode(),0x156);deletemPreviewChar;",
                 "latest CharClipSet source LoadCharacter prefix");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "if(mPreviewChar&&!theChar){ObjDirItr<Character>it("
                 "mPreviewChar,true);if(it)mPreviewChar=it;}",
                 "latest CharClipSet source LoadCharacter nested character");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::DrawShowing(){if(!mPreviewChar)return;"
                 "mPreviewChar->DrawShowing();}",
                 "latest CharClipSet source DrawShowing gate");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "floatCharClipSet::StartFrame(){if(mPreviewClip)return"
                 "mPreviewClip->StartBeat();elsereturn0;}",
                 "latest CharClipSet source StartFrame");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "floatCharClipSet::EndFrame(){if(mPreviewClip)return"
                 "mPreviewClip->EndBeat();elsereturn0;}",
                 "latest CharClipSet source EndFrame");
  ok &= contains(rb3_latest_char_clip_set_cpp,
                 "voidCharClipSet::SetBpm(intbpm){staticSymbolsBpm(\"bpm\");"
                 "Hmx::Object*obj=ObjectDir::Main()->FindObject(\"milo\",false);"
                 "if(obj)obj->SetProperty(sBpm,bpm);mBpm=bpm;}",
                 "latest CharClipSet source SetBpm");
  ok &= contains(char_clip_h,
                 "structSourceCharClipSetState{std::stringchar_file_root;"
                 "boolhas_preview_char=false;boolhas_preview_clip=false;"
                 "boolhas_still_clip=false;intfilter_flags=0;intbpm=90;"
                 "boolpreview_walk=false;boolrate_is_1_fpb=true;};",
                 "native exposes source CharClipSet state");
  ok &= contains(char_clip_h,
                 "SourceCharClipSetPostLoadPlansource_char_clip_set_post_load_plan("
                 "int32_trevision,boolis_proxy,int32_tclip_count,booltype_null);",
                 "native exposes source CharClipSet PostLoad plan helper");
  ok &= contains(char_clip,
                 "SourceCharClipSetStatesource_char_clip_set_default_state(){"
                 "SourceCharClipSetStatestate;source_char_clip_set_reset_preview_state("
                 "state);state.rate_is_1_fpb=true;returnstate;}",
                 "native CharClipSet constructor helper ports source flow");
  ok &= contains(char_clip,
                 "voidsource_char_clip_set_reset_preview_state("
                 "SourceCharClipSetState&state){state.char_file_root.clear();"
                 "state.has_preview_char=false;state.has_preview_clip=false;"
                 "state.has_still_clip=false;state.filter_flags=0;state.bpm=90;"
                 "state.preview_walk=false;}",
                 "native CharClipSet ResetPreviewState helper ports source fields");
  ok &= contains(char_clip,
                 "std::vector<SourceCharClipSetGroupStep>"
                 "source_char_clip_set_randomize_groups(",
                 "native exposes CharClipSet RandomizeGroups helper");
  ok &= contains(char_clip,
                 "SourceCharClipSetPreSaveResultsource_char_clip_set_pre_save("
                 "SourceCharClipSetState&state,boolcached_stream)",
                 "native exposes CharClipSet PreSave helper");
  ok &= contains(char_clip,
                 "SourceCharClipSetPostLoadPlan"
                 "source_char_clip_set_post_load_plan(int32_trevision,"
                 "boolis_proxy,int32_tclip_count,booltype_null)",
                 "native exposes CharClipSet PostLoad plan implementation");
  ok &= contains(char_clip,
                 "plan.read_char_file_path=revision>0x11;"
                 "plan.read_preview_clip=revision>0x11;"
                 "plan.read_filter_flags=revision>0x13;"
                 "plan.read_bpm=revision>0x14;plan.read_preview_walk="
                 "revision>0x15;plan.read_still_clip=revision>0x16;",
                 "native CharClipSet PostLoad helper ports modern gates");
  ok &= contains(char_clip,
                 "SourceCharClipSetLoadCharacterResult"
                 "source_char_clip_set_load_character(",
                 "native exposes CharClipSet LoadCharacter helper");
  ok &= contains(char_clip,
                 "constchar*source_char_clip_set_recenter_all_warning(){"
                 "return\"YoucanonlyrecenterclipsfromPC\";}",
                 "native CharClipSet RecenterAll helper ports warning");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_clip_set_source_test",
                 "CMake builds CharClipSet source test");
  ok &= contains(clip_set_source_test,
                 "source_char_clip_set_default_state()",
                 "focused CharClipSet test covers constructor defaults");
  ok &= contains(clip_set_source_test,
                 "source_char_clip_set_post_load_plan(4,false,3,false)",
                 "focused CharClipSet test covers old PostLoad gates");
  ok &= contains(clip_set_source_test,
                 "source_char_clip_set_load_character(state,true,true,false,true,true)",
                 "focused CharClipSet test covers LoadCharacter nested route");
  ok &= contains(clip_set_source_test,
                 "source_char_clip_set_set_bpm(state,128,true)",
                 "focused CharClipSet test covers SetBpm helper");
  ok &= contains(doc,
                 "Native `source_char_clip_set_post_load_plan` ports the full",
                 "document records native CharClipSet post-load helper");
  ok &= contains(rb3_latest_char_clip_display_h,
                 "classCharClipDisplay{public:MsgSource*FindSource("
                 "Hmx::Object*);voidSetClip(CharClip*,bool);voidSetText("
                 "constchar*);voidSetStartEnd(float,float,bool);",
                 "latest CharClipDisplay header exposes diagnostic methods");
  ok &= contains(rb3_latest_char_clip_display_cpp,
                 "voidCharClipDisplay::Init(ObjectDir*dir){sDir=dir;sEm="
                 "TheRnd->DrawString(\"\",Vector2(0,0),Hmx::Color(1.0f,"
                 "0.0f,0.0f),false).y;}",
                 "latest CharClipDisplay source Init");
  ok &= contains(rb3_latest_char_clip_display_cpp,
                 "MsgSource*CharClipDisplay::FindSource(Hmx::Object*o){"
                 "for(ObjDirItr<MsgSource>it(ObjectDir::Main(),false);"
                 "it!=0;++it){for(std::list<MsgSource::Sink>::iteratorlit="
                 "it->mSinks.begin();lit!=it->mSinks.end();++lit){if((*lit)."
                 "obj==o)returnit;}}return0;}",
                 "latest CharClipDisplay source FindSource scan");
  ok &= contains(rb3_latest_char_clip_display_cpp,
                 "voidCharClipDisplay::SetClip(CharClip*clip,boolb){unk0=clip;"
                 "SetText(clip->Name());SetStartEnd(clip->StartBeat(),"
                 "clip->EndBeat(),b);}",
                 "latest CharClipDisplay source SetClip");
  ok &= contains(rb3_latest_char_clip_display_cpp,
                 "voidCharClipDisplay::SetText(constchar*text){strcpy(unk24,"
                 "text);unk14=TheRnd->DrawString(text,Vector2(0,0),"
                 "Hmx::Color(1.0f,0.0f,0.0f),false).x+sEm;}",
                 "latest CharClipDisplay source SetText");
  ok &= contains(rb3_latest_char_clip_display_cpp,
                 "floatCharClipDisplay::LineSpacing(){returnsEm*2.0f;}",
                 "latest CharClipDisplay source LineSpacing");
  ok &= contains(rb3_latest_char_task_mgr_h,
                 "classCharTaskMgr{public:intfiller;staticboolsShowGraph;"
                 "staticvoidInit();};",
                 "latest CharTaskMgr header exposes graph flag");
  ok &= contains(rb3_latest_char_task_mgr_cpp,
                 "boolCharTaskMgr::sShowGraph=false;",
                 "latest CharTaskMgr source default graph flag");
  ok &= contains(rb3_latest_char_task_mgr_cpp,
                 "staticDataNodeOnToggleCharTaskGraph(DataArray*arr){"
                 "CharTaskMgr::sShowGraph=!CharTaskMgr::sShowGraph;"
                 "returnDataNode(CharTaskMgr::sShowGraph);}",
                 "latest CharTaskMgr source toggle callback");
  ok &= contains(rb3_latest_char_task_mgr_cpp,
                 "voidCharTaskMgr::Init(){DataRegisterFunc("
                 "\"toggle_char_task_graph\",OnToggleCharTaskGraph);}",
                 "latest CharTaskMgr source Init registration");
  ok &= contains(char_clip_h, "structSourceCharClipDisplayGlobals{",
                 "native exposes CharClipDisplay globals");
  ok &= contains(char_clip_h, "structSourceCharTaskMgrState{",
                 "native exposes CharTaskMgr state");
  ok &= contains(char_clip,
                 "voidsource_char_clip_display_init("
                 "SourceCharClipDisplayGlobals&globals,conststd::string&dir,"
                 "floatdraw_empty_y){globals.dir=dir;globals.em=draw_empty_y;}",
                 "native ports CharClipDisplay Init");
  ok &= contains(char_clip,
                 "SourceCharClipDisplayFindSourceResultsource_char_clip_"
                 "display_find_source(conststd::vector<"
                 "SourceCharClipDisplayMsgSource>&sources,conststd::string&"
                 "object){",
                 "native exposes CharClipDisplay FindSource helper");
  ok &= contains(char_clip,
                 "state.text_width_plus_em=draw_text_x+globals.em;",
                 "native ports CharClipDisplay text width plus em");
  ok &= contains(char_clip,
                 "source_char_clip_display_set_start_end(state,start_beat,"
                 "end_beat,flag);",
                 "native records CharClipDisplay SetStartEnd inputs");
  ok &= contains(char_clip,
                 "floatsource_char_clip_display_line_spacing(const"
                 "SourceCharClipDisplayGlobals&globals){returnglobals.em*2.0f;}",
                 "native ports CharClipDisplay LineSpacing");
  ok &= contains(char_clip,
                 "SourceCharTaskMgrStatesource_char_task_mgr_default_state(){"
                 "returnSourceCharTaskMgrState{};}",
                 "native ports CharTaskMgr default state");
  ok &= contains(char_clip,
                 "state.registered_toggle_char_task_graph=true;",
                 "native ports CharTaskMgr Init registration");
  ok &= contains(char_clip,
                 "state.show_graph=!state.show_graph;returnstate.show_graph;",
                 "native ports CharTaskMgr toggle callback");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_clip_display_source_test",
                 "CMake builds CharClipDisplay source test");
  ok &= contains(clip_display_source_test,
                 "source_char_clip_display_find_source(sources,\"target_obj\")",
                 "focused CharClipDisplay test covers FindSource");
  ok &= contains(clip_display_source_test,
                 "source_char_clip_display_set_clip(display,globals,"
                 "\"solo.clip\",12.0f,24.5f,true,31.0f)",
                 "focused CharClipDisplay test covers SetClip");
  ok &= contains(clip_display_source_test,
                 "source_char_task_mgr_toggle_graph(task)",
                 "focused CharTaskMgr test covers toggle");
  ok &= contains(doc, "## Clip Diagnostic Helpers",
                 "document records CharClipDisplay/CharTaskMgr boundary");
  ok &= contains(rb3_latest_clip_graph_gen_h,
                 "classClipGraphGenerator:publicHmx::Object",
                 "latest ClipGraphGenerator header exposes editor object");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "ClipGraphGenerator::ClipGraphGenerator():unk1c(0),"
                 "mDmap(0),mClipA(0),mClipB(0)",
                 "latest ClipGraphGenerator source constructor defaults");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "c1->mTransitions.RemoveNodes(c2);",
                 "latest ClipGraphGenerator GeneratePair removes old nodes");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "if(!b1&&((c1->mPlayFlags&0xF0)!=0x10))b2=false;",
                 "latest ClipGraphGenerator GeneratePair source play-flag gate");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "DataArray*transarr=unk1c->FindArray(\"on_transition\",false);",
                 "latest ClipGraphGenerator GeneratePair finds transition script");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "a_clip=DataNode(c1);b_clip=DataNode(c2);mClipA=c1;mClipB=c2;",
                 "latest ClipGraphGenerator GeneratePair publishes clip pair");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "if(dmap)dmap->SetNodes(n1,n2);",
                 "latest ClipGraphGenerator GeneratePair sets nodes when dmap exists");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "floatmax_error=1e+30f;",
                 "latest ClipGraphGenerator transition default max error");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "intbflag=mClipB->mPlayFlags>>12&15;"
                 "intaflag=mClipA->mPlayFlags>>12&15;",
                 "latest ClipGraphGenerator transition extracts play flag nibbles");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "if(beat_align<(float)aflag)beat_align=aflag;",
                 "latest ClipGraphGenerator transition raises beat alignment");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "mDmap=newClipDistMap(mClipA,mClipB,beat_align,blend_width,3,"
                 "boneweightarr);",
                 "latest ClipGraphGenerator transition constructs ClipDistMap");
  ok &= contains(rb3_latest_clip_graph_gen_cpp,
                 "mDmap->FindDists(max_facing*DEG2RAD,restrictArr);"
                 "mDmap->FindNodes(max_error,max_dist,end_dist);",
                 "latest ClipGraphGenerator transition invokes distance passes");
  ok &= contains(rb3_latest_clip_dist_map_h,
                 "ClipDistMap(CharClip*,CharClip*,float,float,int,constDataArray*);",
                 "latest ClipDistMap header exposes constructor only");
  ok &= contains(rb3_latest_clip_collide_h,
                 "classClipCollide:publicHmx::Object",
                 "latest ClipCollide header exposes editor object");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "ClipCollide::ClipCollide():mReports(),mGraph(0),"
                 "mChar(this,0),mCharPath(\"\"),mWaypoint(this,0),"
                 "mPosition(Symbol(\"front\")),mClip(this,0),mWorldLines(0),"
                 "mMoveCamera(1),mMode()",
                 "latest ClipCollide constructor defaults");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "mChar->SetProxyFile(fp,false);",
                 "latest ClipCollide SyncChar updates mismatched proxy");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "mChar->mDriver->Play(mClip,2,-1.0f,1e+30f,0.0f);",
                 "latest ClipCollide Demonstrate play call");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "ASSERT_REVS(1,0)",
                 "latest ClipCollide source load accepts revisions through 1");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "bs>>mChar;bs>>mCharPath;bs>>mWaypoint;bs>>mPosition;mClip=0;",
                 "latest ClipCollide load reads source fields and clears clip");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "constchar*directions[4]={\"front\",\"back\",\"left\",\"right\"};",
                 "latest ClipCollide TestClips direction order");
  ok &= contains(rb3_latest_clip_collide_cpp,
                 "DataArray*arr=newDataArray(listsize);"
                 "arr->Node(0)=DataNode((Hmx::Object*)0);",
                 "latest ClipCollide object list allocation plan");
  ok &= contains(rb3_latest_file_merger_cpp,
                 "FileMerger::FileMerger():mMergers(this),mAsyncLoad(0),"
                 "mLoadingLoad(0),unk44(0),unk50(0),mHeap(GetCurrentHeapNum()),"
                 "unk58(this)",
                 "latest FileMerger constructor defaults");
  ok &= contains(rb3_latest_file_merger_h,
                 "Merger(Hmx::Object*o):mProxy(0),mPreClear(0),mSubdirs(4)",
                 "latest FileMerger Merger row defaults");
  ok &= contains(rb3_latest_file_merger_h,
                 "mName=m.mName;mSelected=m.mSelected;unk10=m.unk10;"
                 "mLoaded=m.mLoaded;mDir=m.mDir;mProxy=m.mProxy;"
                 "mSubdirs=m.mSubdirs;mLoadedObjects=m.mLoadedObjects;"
                 "mLoadedSubdirs=m.mLoadedSubdirs;mPreClear=m.mPreClear;",
                 "latest FileMerger Merger copy member order");
  ok &= contains(rb3_latest_clip_compressor_cpp,
                 "voidunusedclipcompressor(){MakeString(\"%s%f%f\","
                 "\"beesechurger\",1.0f,2.0f);}",
                 "latest ClipCompressor source contains only unused function");
  ok &= contains(char_clip_h,
                 "structSourceClipGraphGeneratePairStep{"
                 "boolremove_existing_nodes=true;",
                 "native exposes ClipGraph GeneratePair plan");
  ok &= contains(char_clip_h,
                 "structSourceClipCollideState{std::stringchar_path;"
                 "std::stringposition=\"front\";",
                 "native exposes ClipCollide default state");
  ok &= contains(char_clip_h,
                 "structSourceFileMergerState{boolasync_load=false;",
                 "native exposes FileMerger default state");
  ok &= contains(char_clip,
                 "SourceClipGraphGeneratePairStep"
                 "source_clip_graph_generate_pair_step(boolhas_type_def,"
                 "boolsame_type,uint32_tclip_a_play_flags,boolhas_on_transition,"
                 "boolscript_creates_dmap)",
                 "native implements ClipGraph GeneratePair helper");
  ok &= contains(char_clip,
                 "if(!type_pair_allowed&&((clip_a_play_flags&0xF0u)!=0x10u)){"
                 "skip_transition_generation=false;}",
                 "native ClipGraph helper mirrors play-flag gate");
  ok &= contains(char_clip,
                 "plan.min_flag=std::min(plan.clip_a_flag,plan.clip_b_flag);",
                 "native ClipGraph helper mirrors transition min flag");
  ok &= contains(char_clip,
                 "SourceClipCollideStatesource_clip_collide_default_state(){"
                 "returnSourceClipCollideState{};}",
                 "native implements ClipCollide default helper");
  ok &= contains(char_clip,
                 "step.set_proxy_file=has_character&&!char_path_empty&&"
                 "!path_matches_proxy;",
                 "native ClipCollide helper mirrors proxy update gate");
  ok &= contains(char_clip,
                 "if(has_character&&has_waypoint&&has_clip){step.sync_waypoint="
                 "true;step.play_clip=true;}",
                 "native ClipCollide helper mirrors Demonstrate gate");
  ok &= contains(char_clip,
                 "plan.directions={\"front\",\"back\",\"left\",\"right\"};",
                 "native ClipCollide helper mirrors direction order");
  ok &= contains(char_clip,
                 "SourceFileMergerCopyPlansource_file_merger_merger_copy_plan()",
                 "native implements FileMerger copy-plan helper");
  ok &= contains(char_clip,
                 "evidence.observed_function=\"unusedclipcompressor\";",
                 "native records ClipCompressor absence evidence");
  ok &= contains(cmake,
                 "add_executable(ghogx_character_clip_editor_source_test",
                 "CMake builds clip editor source test");
  ok &= contains(clip_editor_source_test,
                 "source_clip_graph_generate_pair_step(true,true,0,true,true)",
                 "focused clip editor test covers ClipGraph transition branch");
  ok &= contains(clip_editor_source_test,
                 "source_clip_collide_demonstrate_step(true,true,true)",
                 "focused clip editor test covers ClipCollide Demonstrate");
  ok &= contains(clip_editor_source_test,
                 "source_file_merger_merger_copy_plan()",
                 "focused clip editor test covers FileMerger copy plan");
  ok &= contains(clip_editor_source_test,
                 "source_clip_compressor_evidence()",
                 "focused clip editor test covers ClipCompressor evidence");
  ok &= contains(doc, "## Clip Editor/Graph Diagnostic Authorities",
                 "document records clip editor graph diagnostics");
  ok &= contains(doc,
                 "no live collision, transition graph execution, compression, "
                 "or file merging behavior is promoted",
                 "document fences clip editor helpers from runtime behavior");
  ok &= contains(rb3_latest_char_clip_group_h,
                 "ObjVector<ObjOwnerPtr<CharClip,ObjectDir>>mClips;//0x8intmWhich;//0x14intmFlags;//0x18",
                 "latest CharClipGroup header exposes source storage fields");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "voidCharClipGroup::Load(BinStream&bs){LOAD_REVS(bs);"
                 "ASSERT_REVS(2,0);Hmx::Object::Load(bs);bs>>mClips;"
                 "bs>>mWhich;if(gRev>1)bs>>mFlags;elsemFlags=0;}",
                 "latest CharClipGroup source exposes Load row order");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "CharClip*CharClipGroup::GetClip(){if(mClips.empty())return0;"
                 "mWhich++;if(mWhich>=mClips.size())mWhich=0;"
                 "returnmClips[mWhich];}",
                 "latest CharClipGroup source exposes stored-order cycling");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "intCharClipGroup::NumFlagDuplicates(CharClip*clip,intx){"
                 "intflags=clip->mFlags;intcount=0;for(inti=0;i<mClips.size();"
                 "i++){if(clip!=mClips[i]){if((x&flags)==(x&mClips[i]->mFlags))"
                 "count++;}}returncount;}",
                 "latest CharClipGroup source exposes masked duplicate count");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "structAlphabetically{booloperator()(Hmx::Object*i,"
                 "Hmx::Object*j)const{returnstrcmp(i->Name(),j->Name())<0;}};",
                 "latest CharClipGroup source exposes alphabetical comparator");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "voidCharClipGroup::Sort(){std::sort(mClips.begin(),"
                 "mClips.end(),Alphabetically());}",
                 "latest CharClipGroup source exposes Sort helper");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "voidCharClipGroup::AddClip(CharClip*clip){if(!HasClip(clip))"
                 "mClips.push_back(ObjOwnerPtr<CharClip,ObjectDir>(this,clip));}",
                 "latest CharClipGroup source exposes AddClip duplicate gate");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "voidCharClipGroup::RemoveClip(CharClip*clip){",
                 "latest CharClipGroup source exposes RemoveClip body");
  ok &= contains(rb3_latest_char_clip_group_cpp,
                 "if(*it==clip){it=mClips.erase(it);}elseit++;",
                 "latest CharClipGroup source exposes RemoveClip iterator behavior");
  ok &= contains(char_clip_h,
                 "structCharClipGroup{std::stringname;std::stringmilo_path;"
                 "std::vector<std::string>clips;uint32_tversion=0;"
                 "int32_twhich=0;int32_tflags=0;boolloaded=false;};",
                 "native character API carries source CharClipGroup state");
  ok &= contains(char_clip_h,
                 "CharClipGroupload_clip_group("
                 "conststd::string&hdr_path,conststd::string&ark_path,"
                 "conststd::vector<std::string>&milo_paths,"
                 "conststd::string&group_name);",
                 "native character API exposes source-backed clip group reader");
  ok &= contains(char_clip_h,
                 "std::optional<size_t>char_clip_group_get_clip_index("
                 "CharClipGroup&group);",
                 "native character API exposes source-backed GetClip step");
  ok &= contains(char_clip_h,
                 "intsource_char_clip_group_num_flag_duplicates("
                 "conststd::vector<uint32_t>&clip_flags,size_tclip_index,"
                 "uint32_tmask);",
                 "native character API exposes source-backed NumFlagDuplicates helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_clip_group_sorted_names("
                 "std::vector<std::string>clip_names);",
                 "native character API exposes source-backed CharClipGroup sort helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_clip_group_add_clip("
                 "std::vector<std::string>clip_names,conststd::string&clip_name);",
                 "native character API exposes source-backed CharClipGroup AddClip helper");
  ok &= contains(char_clip_h,
                 "std::vector<std::string>source_char_clip_group_remove_clip("
                 "std::vector<std::string>clip_names,conststd::string&clip_name);",
                 "native character API exposes source-backed CharClipGroup RemoveClip helper");
  ok &= contains(char_clip,
                 "CharClipGroupload_clip_group(",
                 "native clip decoder implements shared clip group reader");
  ok &= contains(char_clip,
                 "(void)read_len_string(body,size,pos);//Hmx::Objectsubtypesymbol.",
                 "native clip group reader consumes Hmx Object subtype symbol");
  ok &= contains(char_clip,
                 "clips.push_back(read_len_string(body,size,pos));",
                 "native clip group reader consumes stored mClips names");
  ok &= contains(char_clip,
                 "std::memcpy(&group.which,body+pos,4);",
                 "native clip group reader consumes mWhich");
  ok &= contains(char_clip,
                 "std::memcpy(&group.flags,body+pos,4);",
                 "native clip group reader consumes revision-gated mFlags");
  ok &= contains(char_clip,
                 "std::optional<size_t>char_clip_group_get_clip_index("
                 "CharClipGroup&group){if(group.clips.empty())returnstd::nullopt;"
                 "constint32_tbefore=group.which;++group.which;",
                 "native clip group selection advances source mWhich");
  ok &= contains(char_clip,
                 "if(group.which>=static_cast<int32_t>(group.clips.size())){"
                 "group.which=0;}",
                 "native clip group selection wraps source mWhich");
  ok &= contains(char_clip,
                 "intsource_char_clip_group_num_flag_duplicates("
                 "conststd::vector<uint32_t>&clip_flags,size_tclip_index,"
                 "uint32_tmask){if(clip_index>=clip_flags.size())return0;"
                 "constuint32_tflags=clip_flags[clip_index];intcount=0;"
                 "for(size_ti=0;i<clip_flags.size();++i){if(i!=clip_index&&"
                 "(mask&flags)==(mask&clip_flags[i])){++count;}}returncount;}",
                 "native clip group duplicate helper mirrors source masked count");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_clip_group_sorted_names("
                 "std::vector<std::string>clip_names){std::sort("
                 "clip_names.begin(),clip_names.end());returnclip_names;}",
                 "native clip group sort helper mirrors source name ordering");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_clip_group_add_clip("
                 "std::vector<std::string>clip_names,conststd::string&clip_name){"
                 "if(std::find(clip_names.begin(),clip_names.end(),clip_name)=="
                 "clip_names.end()){clip_names.push_back(clip_name);}return"
                 "clip_names;}",
                 "native clip group AddClip helper mirrors source duplicate gate");
  ok &= contains(char_clip,
                 "std::vector<std::string>source_char_clip_group_remove_clip("
                 "std::vector<std::string>clip_names,conststd::string&clip_name){"
                 "for(size_ti=0;i<clip_names.size();++i){if(clip_names[i]=="
                 "clip_name){clip_names.erase(clip_names.begin()+"
                 "static_cast<std::ptrdiff_t>(i));}else{++i;}}returnclip_names;}",
                 "native clip group RemoveClip helper mirrors source iterator skip");
  ok &= contains(char_clip,
                 "\"[clip-group-source]group=%smilo=%sversion=%u\"",
                 "native clip group reader logs source row proof");
  ok &= contains(char_clip,
                 "\"[clip-group-source-select]group=%sbefore=%dafter=%d\"",
                 "native clip group selector logs source GetClip proof");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_group_num_flag_duplicates(",
                 "focused flag-mask test covers CharClipGroup duplicate helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_group_sorted_names(input)",
                 "focused flag-mask test covers CharClipGroup sort helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_group_add_clip(input,clip_name)",
                 "focused flag-mask test covers CharClipGroup AddClip helper");
  ok &= contains(clip_driver_flags_test,
                 "source_char_clip_group_remove_clip(input,clip_name)",
                 "focused flag-mask test covers CharClipGroup RemoveClip helper");
  ok &= contains(doc,
                 "Native `source_char_clip_group_num_flag_duplicates` ports",
                 "document records native CharClipGroup duplicate helper");
  ok &= contains(doc,
                 "Native `source_char_clip_group_sorted_names` ports",
                 "document records native CharClipGroup sort helper");
  ok &= contains(doc,
                 "Native `source_char_clip_group_add_clip` ports the concrete",
                 "document records native CharClipGroup AddClip helper");
  ok &= contains(doc,
                 "Native `source_char_clip_group_remove_clip` ports the visible",
                 "document records native CharClipGroup RemoveClip helper");
  ok &= contains(doc,
                 "selected clip's flags with every other clip",
                 "document records source CharClipGroup duplicate behavior");
  ok &= missing(rb3_latest_char_clip_driver_cpp,
                "floatCharClipDriver::Evaluate(",
                "latest CharClipDriver source does not expose Evaluate body");
  ok &= contains(rb2_char_clip_samples_cpp,
                 "voidCharClipSamples::ScaleAdd(",
                 "RB2 dump exposes CharClipSamples ScaleAdd runtime map");
  ok &= contains(rb2_char_clip_samples_cpp,
                 "voidCharClipSamples::Load(",
                 "RB2 dump exposes CharClipSamples Load runtime map");
  ok &= contains(rb2_char_bones_samples_cpp,
                 "voidCharBonesSamples::LoadHeader(",
                 "RB2 dump exposes CharBonesSamples LoadHeader runtime map");
  ok &= contains(rb2_char_bones_samples_cpp,
                 "voidCharBonesSamples::EvaluateChannel(",
                 "RB2 dump exposes CharBonesSamples EvaluateChannel runtime map");
  ok &= contains(rb2_char_clip_driver_cpp,
                 "floatCharClipDriver::Evaluate(",
                 "RB2 dump exposes CharClipDriver Evaluate runtime map");
  ok &= contains(rb2_char_driver_cpp,
                 "classCharClipDriver*CharDriver::Play(",
                 "RB2 dump exposes CharDriver Play runtime map");
  ok &= contains(doc,
                 "`band3_recomp` currently contributes symbol-table names",
                 "document distinguishes symbol names from runtime implementation");
  ok &= contains(band3_config,
                 "CharClip__FacingSet__Set",
                 "band3_recomp exposes CharClip FacingSet symbol only");
  ok &= contains(band3_config,
                 "CharClip__SyncProperty",
                 "band3_recomp exposes CharClip SyncProperty symbol only");
  ok &= contains(band3_config,
                 "CharBones__ScaleAddIdentity",
                 "band3_recomp exposes CharBones symbol only");
  ok &= missing(band3_config,
                "CharClipSamples",
                "band3_recomp has no CharClipSamples runtime symbol");
  ok &= missing(band3_config,
                "CharBonesSamples",
                "band3_recomp has no CharBonesSamples runtime symbol");
  ok &= contains(doc,
                 "Broad body, face, lower-body,\n  or full CharBone output publishing "
                 "remains opt-in diagnostic behavior",
                 "document keeps broad CharBone output publishing out of runtime truth");
  ok &= contains(char_clip,
                 "Decoderevidenceisboundedbyihatecompvirsource.rb3-latestexposes",
                 "clip decoder comment names current ihatecompvir source boundary");
  ok &= contains(char_clip,
                 "samplemathbodiesarestillabsentfromthecheckedpublicC++source",
                 "clip decoder comment states incomplete sample math boundary");
  ok &= contains(compact(read_file(char_dir / "char_clip.h")),
                 "broadoutputpublishingremainsdiagnostic",
                 "clip header states output publishing boundary");
  ok &= contains(char_clip,
                 "staticboolcharbone_lower_body_output_enabled()",
                 "lower-body CharBone output bridge is diagnostic opt-in");
  ok &= contains(char_clip,
                 "\"GHOGX_ENABLE_CHARBONE_LOWER_BODY_OUTPUT\"",
                 "lower-body CharBone output bridge uses explicit enable");
  ok &= contains(char_clip,
                 "constboollower_body_output=lower_body_only&&"
                 "output_map_lower_body_bone(it->first);",
                 "lower-body CharBone rows require the lower-body diagnostic opt-in");
  ok &= contains(char_clip,
                 "constboolface_output=face_output_layer&&"
                 "output_map_face_bone(it->first);",
                 "face CharBone diagnostics do not imply lower-body output");
  ok &= contains(char_clip,
                 "if(!force_selected_output&&!full_output_layer&&"
                 "!lower_body_only&&!face_output_layer){returnfalse;}",
                 "selected hand output is separate from broad output diagnostics");
  ok &= contains(format_notes,
                 "Current source-truth keeps broad lower-body\n  output opt-in only",
                 "format notes fence lower-body CharBone output as opt-in");
  ok &= contains(format_notes,
                 "There is no\n  `GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT` switch",
                 "format notes reject the old default-on disable switch");
  ok &= missing(format_notes,
                "disables that promoted lower\n  bridge",
                "format notes must not describe lower-body output as promoted");
  ok &= missing(format_notes,
                "pins the promoted\n  lower-body bridge to default-on",
                "format notes must not pin lower-body output as default-on");
  ok &= missing(char_clip, "fore_twists_applied",
                "CharIKHand path must not mark CharForeTwist rows consumed");
  ok &= missing(char_clip, "GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT",
                "lower-body CharBone output bridge must not be default-on");
  ok &= missing(char_clip, "NOTguessed",
                "clip decoder must not overstate trace notes as source truth");
  ok &= missing(char_clip, "FORMAT(pertherecomp)",
                "clip decoder must not cite non-source recomp as authority");
  ok &= missing(char_clip, "apply_ps2_ik_hand_targets",
                "old PS2-named CharIKHand runner removed");
  ok &= missing(char_clip, "ps2_ordered_ik_hands",
                "old name-based CharIKHand role ordering removed");
  ok &= missing(char_clip, "classify_ps2_ik_poll_role",
                "old name-based CharIKHand role classifier removed");
  ok &= missing(char_clip, "Ps2IkPollRole",
                "old CharIKHand role enum removed");
  ok &= missing(char_clip, "ps2_ik_hand_position_enabled",
                "old hand-position arm IK gate removed");
  ok &= missing(char_clip, "ps2_ik_hand_final_disabled",
                "old hand-final arm IK gate removed");
  ok &= missing(char_clip, "ps2_ik_hand_final_orientation_disabled",
                "old hand-final orientation gate removed");
  ok &= missing(char_clip, "ps2_ik_hand_final_position_disabled",
                "old hand-final position gate removed");
  ok &= missing(char_clip, "ps2_ik_hands_enabled",
                "old arm IK disable gate removed");
  ok &= missing(char_clip, "ps2_ik_swing_postmultiply_enabled",
                "old arm swing A/B gate removed");
  ok &= missing(char_clip, "ps2_ik_swing_transpose_enabled",
                "old arm swing transpose gate removed");
  ok &= missing(char_clip, "ps2_ik_aimed_swing_enabled",
                "old aimed arm swing gate removed");
  ok &= missing(char_clip, "GHOGX_ENABLE_PS2_IK_HAND_POS",
                "old PS2 hand-position env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_PS2_IK_HAND_FINAL",
                "old PS2 hand-final env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_PS2_IK_HAND_FINAL_ORIENTATION",
                "old PS2 hand-final orientation env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_PS2_IK_HAND_FINAL_POSITION",
                "old PS2 hand-final position env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_PS2_IK_HANDS",
                "old PS2 hand IK disable env gate removed");
  ok &= missing(char_clip, "GHOGX_PS2_IK_POSTMULTIPLY_SWING",
                "old PS2 hand IK swing env gate removed");
  ok &= missing(char_clip, "GHOGX_PS2_IK_TRANSPOSE_SWING",
                "old PS2 hand IK transpose env gate removed");
  ok &= missing(char_clip, "GHOGX_PS2_IK_AIMED_SWING",
                "old PS2 hand IK aimed env gate removed");
  ok &= missing(char_clip, "GHOGX_APPLY_HAND_POS",
                "old hand local-position env gate removed");
  ok &= missing(char_clip, "apply_ps2_fore_twist",
                "old traced foretwist helper must stay removed");
  ok &= missing(char_clip, "apply_ps2_upper_twists",
                "old traced upper-twist helper must stay removed");
  ok &= missing(char_clip, "apply_driven_twists",
                "old approximate/PS2 driven twist dispatcher must stay removed");
  ok &= missing(char_clip, "apply_source_driven_twists",
                "source foretwist runner must not carry old dispatcher name");
  ok &= missing(char_clip, "disable_driven_twists_enabled",
                "source twist controllers must not be runtime-disabled");
  ok &= missing(char_clip, "GHOGX_DISABLE_DRIVEN_TWISTS",
                "old driven twist disable env gate removed");
  ok &= missing(char_clip, "GHOGX_ENABLE_APPROX_DRIVEN_TWISTS",
                "old approximate driven twist env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_APPROX_UPPER_TWIST",
                "old approximate upper twist env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_APPROX_FORE_TWIST",
                "old approximate fore twist env gate removed");
  ok &= missing(char_clip, "GHOGX_IGNORE_APPROX_FORE_TWIST_OFFSET",
                "old approximate fore twist offset env gate removed");
  ok &= missing(char_clip, "GHOGX_APPROX_FORE_TWIST_LOCAL_HAND",
                "old approximate local-hand fore twist env gate removed");
  ok &= missing(char_clip, "ps2_twist_angle_from_local_rows",
                "old PS2 local-row twist extractor removed");
  ok &= missing(char_clip, "write_ps2_x_twist",
                "old PS2 X-twist writer removed");
  ok &= missing(char_clip, "set_rot_x_preserve_pos",
                "old local X-twist helper removed");
  ok &= missing(char_clip, "local_x_roll_delta",
                "old local roll delta helper removed");

  ok &= missing(char_mesh_h, "RuntimeHair", "legacy runtime hair state removed");
  ok &= missing(renderer, "runtime_hair_world_override",
                "legacy runtime hair skin override removed");
  ok &= missing(renderer, "hairOverride", "legacy hair override debug removed");
  ok &= missing(char_clip, "legacy_char_hair_bridge_enabled",
                "legacy CharHair bridge gate removed");
  ok &= missing(char_clip, "GHOGX_ENABLE_LEGACY_CHAR_HAIR_BRIDGE",
                "legacy CharHair env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_CHAR_HAIR",
                "disable-hair env gate removed");
  ok &= missing(char_clip, "charhair-ps2chain",
                "old native CharHair chain predictor removed");
  ok &= missing(char_clip, "hairOverride",
                "old native CharHair skin override removed");
  ok &= missing(char_clip, "GHOGX_SOURCE_CHAR_HAIR_ROOTMAT",
                "old CharHair root-matrix trial env gate removed");
  ok &= contains(char_mesh_h,
                 "sourcethenclearsPoint.collides;"
                 "//nativelogsthesefieldsbutdoesnotpromotethem",
                 "CharHair point comments keep inline collision rows out of guessed collides");
  ok &= contains(char_clip, "legacyInline=loggedOnly",
                 "CharHair source logs mark legacy inline collision rows as log-only");
  ok &= contains(char_clip,
                 "source=ihatecompvir-CharHair::Poll/DoReset/SimulateInternal",
                 "CharHair simulation log names the upstream poll/reset/sim path");
  ok &= contains(char_clip,
                 "missingHookupObjPtrList=1",
                 "CharHair simulation log keeps missing hookup boundary explicit");
  ok &= contains(format_notes,
                 "Current native `CharHair` behavior ports the checked ihatecompvir",
                 "format notes describe current CharHair source poll path");
  ok &= contains(format_notes,
                 "source poll/reset/sim boundary",
                 "format notes document the current CharHair source boundary");
  ok &= contains(format_notes,
                 "does not\n  publish solved `CharHair` transforms without a resolved source point-collide",
                 "Rock2 notes keep source CharHair writes fenced to resolved collides");
  ok &= contains(format_notes,
                 "Historical native `CharHair` shared-poller trial",
                 "old CharHair poller proof is marked historical");
  ok &= contains(format_notes,
                 "historical native follow-row bridge trial",
                 "old CharHair follow-row bridge proof is marked historical");
  ok &= contains(format_notes,
                 "historical CharHair runtime-world consumer bridge trial",
                 "old CharHair runtime-world bridge proof is marked historical");
  ok &= contains(format_notes,
                 "historical native multi-point chain trial",
                 "old CharHair multi-point chain proof is marked historical");
  ok &= contains(format_notes,
                 "historical Glam1 local-hair consumer recheck",
                 "old CharHair local-hair consumer proof is marked historical");
  ok &= contains(format_notes_compact,
                 "Currentsource-truthnolongertreatsthisasalivesharedformatrule",
                 "old CharHair world-override bridge is not current source truth");
  ok &= contains(format_notes,
                 "Historical collision mode 3 trial",
                 "old CharHair collision-mode proof is marked historical");
  ok &= contains(format_notes,
                 "Historical `GHOGX_ENABLE_CHAR_HAIR_PROBE=1` and "
                 "`GHOGX_DISABLE_CHAR_HAIR=1`",
                 "format notes mark old CharHair gates as historical evidence");
  ok &= contains(format_notes_compact,
                 "point`runtimeWriteback=0`untilthemissingsourcehookupbody",
                 "format notes document CharHair zero-writeback boundary");
  ok &= contains(format_notes,
                 "historical Glam1 wrist render-path trial",
                 "old hairRender wrist proof is marked historical");
  ok &= contains(format_notes,
                 "Historical 2026-06-15 Glam1 hair render-state route",
                 "old no-zwrite Glam1 hair route is marked historical");
  ok &= contains(format_notes,
                 "historical runtime row bridge trial",
                 "old CharHair runtime row bridge is marked historical");
  ok &= contains(format_notes_compact,
                 "side-profilearm/neckposture",
                 "Rock1/Rock2 side-profile posture remains unsigned-off");
  ok &= contains(format_notes,
                 "direct-app side-profile recheck in\n"
                 "  `analysis/rock_regression_recheck_20260710/` still reads",
                 "Rock1/Rock2 side-profile regression evidence is recorded");
  ok &= contains(format_notes,
                 "bind-pose\n  control pair in the same folder is upright",
                 "Rock1/Rock2 bind-pose control narrows issue to clip/controller stack");
  ok &= contains(format_notes,
                 "Do not sign off Rock1/Rock2 side-profile arm/neck\n"
                 "  posture until the `CharClipSamples` / `CharBonesSamples`",
                 "Rock1/Rock2 posture signoff stays fenced to source-backed clip path");
  ok &= contains(format_notes,
                 "Current source-truth no longer keeps a\n  `hairRender` branch",
                 "format notes keep hairRender branch out of current source truth");
  ok &= contains(format_notes,
                 "decoded material fields drive alpha/z/wrap state, source\n"
                 "  group/draw-order rows drive ordering, and the project override for hair is\n"
                 "  two-sided culling only",
                 "format notes fence hair override to culling only");
  ok &= contains(format_notes,
                 "Historical PS2 hand-IK A/B toggles",
                 "format notes mark old hand-IK toggles as historical only");
  ok &= contains(format_notes,
                 "it does not mean a solved hair simulation is\n  active",
                 "format notes must not claim source hair poll is full visual parity");
  ok &= missing(format_notes,
                "common native `CharHair` poller now drives",
                "format notes must not claim old common CharHair poller is active");
  ok &= missing(format_notes,
                "Native `CharHair` now runs through a shared default poller",
                "format notes must not claim old shared CharHair poller is active");
  ok &= missing(format_notes,
                "shared poller now evaluates",
                "format notes must not claim old collision-mode trial is active");
  ok &= missing(format_notes,
                "Native `CharHair` collision mode 3 is now implemented",
                "format notes must not claim old collision-mode trial is active");
  ok &= missing(format_notes,
                "`GHOGX_DISABLE_CHAR_HAIR=1` disables the poller",
                "format notes must not describe removed CharHair disable gate as current");
  ok &= missing(format_notes,
                "logs each native `CharHair` point solve",
                "format notes must not imply native CharHair points are solved");
  ok &= missing(format_notes,
                "native follow-row bridge promoted",
                "format notes must not claim old CharHair follow-row trial is promoted");
  ok &= missing(format_notes,
                "promoted CharHair runtime-world consumer bridge",
                "format notes must not claim old CharHair world bridge is promoted");
  ok &= missing(format_notes,
                "Native therefore must write follow-only `CharHair` target locals",
                "format notes must not claim PS2 trace overrides ihatecompvir CharHair boundary");
  ok &= missing(format_notes,
                "Native now mirrors that ownership",
                "format notes must not claim removed CharHair world override is live");
  ok &= missing(format_notes,
                "Native chain rows therefore submit",
                "format notes must not claim removed CharHair chain rows are live");
  ok &= missing(format_notes,
                "native now snapshots every multi-point",
                "format notes must not claim removed multi-point chain writer is live");
  ok &= missing(format_notes,
                "current shared consumer is still in the right",
                "format notes must not claim removed hairOverride consumer is current");
  ok &= missing(format_notes,
                "newer non-identity\n  CharHair row bridge",
                "format notes must not claim removed non-identity hair bridge is current");
  ok &= missing(format_notes,
                "Native now reaches the same matrix-shape",
                "format notes must not claim removed CharHair matrix writer is live");
  ok &= missing(format_notes,
                "Native now treats hair-material meshes",
                "format notes must not claim removed hairRender branch is live");
  ok &= missing(format_notes,
                "must sort/draw with hair render state",
                "format notes must not promote hairRender sorting from material name");
  ok &= missing(format_notes,
                "Glam1 wrist isolate promoted a narrow render-path correction",
                "format notes must not describe old hairRender trial as promoted");
  ok &= missing(format_notes,
                "Promoted 2026-06-15 Glam1 hair route",
                "format notes must not describe old no-zwrite hair route as promoted");
  ok &= missing(format_notes,
                "accepted renderer fix is to draw blended hair materials with depth writes\n  disabled",
                "format notes must not promote hair-name no-zwrite behavior");
  ok &= missing(format_notes,
                "native now stores a runtime world row",
                "format notes must not claim removed CharHair runtime row bridge is live");
  ok &= missing(format_notes,
                "lets hair skinning consume it",
                "format notes must not claim removed hairOverride bridge is live");
  ok &= missing(format_notes,
                "no current attached-guitar Rock arm regression was found",
                "format notes must not sign off Rock1/Rock2 arm posture from front-only proof");
  ok &= missing(char_clip, "submit_char_eyes_runtime_rows",
                "unsupported CharEyes runtime-row bridge removed");
  ok &= missing(char_clip, "source_pos=vadd(target_pos",
                "self-source look-at fallback removed");
  ok &= missing(char_clip, "set_facefx_eye_props",
                "unsupported FaceFX eye property bridge removed");
  ok &= missing(char_clip, "apply_legacy_ik_hands",
                "unsupported native two-bone arm IK bridge removed");
  ok &= missing(char_clip, "GHOGX_ENABLE_ARM_IK",
                "unsupported native arm IK env gate removed");
  ok &= missing(char_clip, "GHOGX_DISABLE_ARM_IK",
                "unsupported native arm IK disable gate removed");
  ok &= missing(char_clip, "GHOGX_ENABLE_IK_VISIBLE_STRETCH",
                "unsupported native IK stretch gate removed");
  ok &= missing(char_clip, "GHOGX_ENABLE_IK_HAND_ROT",
                "unsupported native IK rotation gate removed");

  if (!ok) {
    std::cerr
        << "Character model code must stay aligned with ihatecompvir source, "
           "not guessed bridge behavior.\n";
    return 1;
  }
  return 0;
}

int main() {
  try {
    return run_contract();
  } catch (const std::exception& ex) {
    std::cerr << "Character source-truth contract setup failed: "
              << ex.what() << "\n";
    return 2;
  }
}
