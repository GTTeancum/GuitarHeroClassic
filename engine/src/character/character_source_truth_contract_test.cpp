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
  const std::string char_bones_source_test =
      compact(read_file(char_dir / "character_char_bones_source_test.cpp"));
  const std::string char_utl_source_test =
      compact(read_file(char_dir / "character_char_utl_source_test.cpp"));
  const std::string ik_rod_source_test =
      compact(read_file(char_dir / "character_ik_rod_source_test.cpp"));
  const std::string ik_hand_source_test =
      compact(read_file(char_dir / "character_ik_hand_source_test.cpp"));
  const std::string bone_offset_source_test =
      compact(read_file(char_dir / "character_bone_offset_source_test.cpp"));
  const std::string bone_twist_source_test =
      compact(read_file(char_dir / "character_bone_twist_source_test.cpp"));
  const std::string lookat_source_test =
      compact(read_file(char_dir / "character_lookat_source_test.cpp"));
  const std::string weight_setter_source_test =
      compact(read_file(char_dir / "character_weight_setter_source_test.cpp"));
  const std::string char_hair_source_test =
      compact(read_file(char_dir / "character_char_hair_source_test.cpp"));
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
  const std::string rb3_latest_char_collide_cpp = compact(read_file(
      rb3_latest_char_dir / "CharCollide.cpp"));
  const std::string rb3_latest_char_collide_h = compact(read_file(
      rb3_latest_char_dir / "CharCollide.h"));
  const std::string rb3_latest_char_ik_rod_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKRod.cpp"));
  const std::string rb3_latest_char_ik_rod_h = compact(read_file(
      rb3_latest_char_dir / "CharIKRod.h"));
  const std::string rb3_latest_char_ik_midi_cpp = compact(read_file(
      rb3_latest_char_dir / "CharIKMidi.cpp"));
  const std::string rb3_latest_char_ik_midi_h = compact(read_file(
      rb3_latest_char_dir / "CharIKMidi.h"));
  const std::string rb3_latest_char_servo_bone_cpp = compact(read_file(
      rb3_latest_char_dir / "CharServoBone.cpp"));
  const std::string rb3_latest_char_servo_bone_h = compact(read_file(
      rb3_latest_char_dir / "CharServoBone.h"));
  const std::string rb3_latest_char_face_servo_cpp = compact(read_file(
      rb3_latest_char_dir / "CharFaceServo.cpp"));
  const std::string rb3_latest_char_face_servo_h = compact(read_file(
      rb3_latest_char_dir / "CharFaceServo.h"));
  const std::string rb3_latest_char_weightable_cpp = compact(read_file(
      rb3_latest_char_dir / "CharWeightable.cpp"));
  const std::string rb3_latest_char_weightable_h = compact(read_file(
      rb3_latest_char_dir / "CharWeightable.h"));
  const std::string rb3_latest_char_driver_cpp = compact(read_file(
      rb3_latest_char_dir / "CharDriver.cpp"));
  const std::string rb3_latest_char_driver_h = compact(read_file(
      rb3_latest_char_dir / "CharDriver.h"));
  const std::string rb3_latest_char_driver_midi_cpp = compact(read_file(
      rb3_latest_char_dir / "CharDriverMidi.cpp"));
  const std::string rb3_latest_char_driver_midi_h = compact(read_file(
      rb3_latest_char_dir / "CharDriverMidi.h"));
  const std::string rb3_latest_character_cpp = compact(read_file(
      rb3_latest_char_dir / "Character.cpp"));
  const std::string rb3_latest_char_poll_group_cpp = compact(read_file(
      rb3_latest_char_dir / "CharPollGroup.cpp"));
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
  const std::string rb3_latest_char_guitar_string_cpp = compact(read_file(
      rb3_latest_char_dir / "CharGuitarString.cpp"));
  const std::string rb3_latest_char_guitar_string_h = compact(read_file(
      rb3_latest_char_dir / "CharGuitarString.h"));
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
  ok &= contains(doc, "| Poll groups | `rb3-latest` `CharPollGroup.cpp` |",
                 "coverage matrix cites CharPollGroup source boundary");
  ok &= contains(doc,
                 "| Guitar string bend controller | `rb3-latest` "
                 "`CharGuitarString.cpp` / `CharGuitarString.h`; stock "
                 "guitar sweep |",
                 "coverage matrix cites CharGuitarString stock boundary");
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
  ok &= contains(stock_guitar_string_sweep,
                 "\"Entry\",\"HasCharGuitarString\",\"StringHits\",\"TransSummary\"",
                 "stock guitar sweep records CharGuitarString column");
  ok &= missing(stock_guitar_string_sweep_compact, "\",\"1\",\"",
                "stock guitar sweep has no active CharGuitarString rows");
  ok &= contains(doc, "HasCharGuitarString=0",
                 "document records stock CharGuitarString absence");
  ok &= contains(doc,
                 "not the active GH2 stock guitar/left-hand\n"
                 "or string-transparency route",
                 "document fences CharGuitarString from implicit fixes");
  ok &= contains(doc, "| FaceFX lip-sync servo boundary | `rb3-latest` "
                 "`CharFaceServo.*`; stock GH2 `FaceFxLipSyncServo` inventory |",
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
  ok &= contains(doc, "rb3/src/system/char/CharIKHand.cpp",
                 "document cites CharIKHand runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharUpperTwist.cpp",
                 "document cites CharUpperTwist runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharForeTwist.cpp",
                 "document cites CharForeTwist runtime source");
  ok &= contains(doc, "ihatecompvir-extra/band3_recomp",
                 "document cites extra band3_recomp source");
  ok &= contains(band3_readme, "Early recompilation of Rock Band 3",
                 "band3_recomp README is available to source-truth contract");

  ok &= contains(doc,
                 "| Character/BandCharacter/RndDir/ObjectDir root body | "
                 "`rb3-latest` `Character.cpp`, `rndobj/Dir.cpp`, `obj/Dir.cpp` |",
                 "coverage matrix records root dir body source evidence");
  ok &= contains(doc, "## Character Root Body Boundary",
                 "document records root body boundary section");
  ok &= contains(doc,
                 "Do not decode or apply root `Character`, `RndDir`, or "
                 "`ObjectDir` runtime fields",
                 "document fences root dir body from guessed runtime decode");
  ok &= contains(doc,
                 "stock_character_dir_entry_inventory.log",
                 "document cites root dir entry inventory proof");
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
                 "if(mReset>0)DoReset(mReset);if(TheTaskMgr.DeltaSeconds()!="
                 "0.0f){SimulateLoops(1,GetFPS());}elseSimulateZeroTime();",
                 "RB3 CharHair poll reset/simulate flow");
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
  ok &= contains(doc,
                 "Native reset follows that\n    forced-simulate lane",
                 "document records native CharHair reset forced simulation");
  ok &= contains(doc,
                 "Native ports the raw local-row write as\n"
                 "    `source_char_hair_freeze_pose_raw`",
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
                 "bs>>unk148;bs>>mMesh;for(inti=0;i<8;i++){bs>>"
                 "unk_structs[i].unk0;bs>>unk_structs[i].vec;}bs>>mDigest;",
                 "latest CharCollide source retains mesh collision rows");
  ok &= contains(rb3_latest_char_collide_cpp,
                 "LOAD_REVS(bs)ASSERT_REVS(7,0)",
                 "latest CharCollide Load accepts source revisions through 7");
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
  ok &= contains(doc,
                 "Native GHOGX retains the internal transform, all eight mesh sphere rows",
                 "document records decoded CharCollide retained source rows");
  ok &= contains(doc, "Native GHOGX ports `CharCollide::CopyOriginalToCur`",
                 "document records CharCollide helper ports");
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
  ok &= contains(doc, "runs the checked source poll/reset/sim state path",
                 "document states bounded native CharHair poll rule");
  ok &= contains(doc, "point rows unwritten until",
                 "document states bounded native CharHair writeback rule");
  ok &= contains(doc, "point collide-list population",
                 "document names missing CharHair point collision hookup boundary");
  ok &= contains(doc, "latest source includes `CharHair.h`, `CharCollide.h`",
                 "document records stronger latest hair source boundary");
  ok &= contains(doc,
                 "overloaded `Hookup(ObjPtrList<CharCollide>&)` body is still\n"
                 "    declared but not implemented",
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
  ok &= contains(rb3_latest_char_servo_bone_h,
                 "classCharServoBone:publicRndHighlightable,publicCharPollable,"
                 "publicCharBonesMeshes",
                 "latest CharServoBone source header exposes inheritance");
  ok &= contains(rb3_latest_char_servo_bone_h, "SymbolmClipType;",
                 "latest CharServoBone source header exposes clip type");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "if(gRev>1)bs>>s;SetClipType(s);",
                 "CharServoBone source load gates clip type");
  ok &= contains(rb3_latest_char_servo_bone_cpp,
                 "ClearBones();CharBoneDir::StuffBones(*this,mClipType);",
                 "CharServoBone source SetClipType refills source bones");
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
  ok &= contains(rb3_latest_char_weightable_cpp,
                 "bs>>mWeight;if(gRev>1)bs>>mWeightOwner;",
                 "CharWeightable source load gates weight owner");
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
                 "ok&=!source_char_weight_setter_poll(driver,weights,0.0f,out);",
                 "focused CharWeightSetter test covers driver fence");
  ok &= contains(weight_setter_source_test,
                 "apply_character_controllers(character,0.0f,nullptr);",
                 "focused CharWeightSetter test covers controller writeback");
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
  ok &= contains(doc,
                 "`CharDriverMidi::Load` reads the subclass revision",
                 "document records CharDriverMidi source load");
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
  ok &= missing(char_mesh, "CharPollGroup",
                "native must not promote absent CharPollGroup rows");
  ok &= missing(char_mesh_h, "CharPollGroup",
                "native character model must not declare absent CharPollGroup rows");
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
  ok &= contains(char_mesh,
                 "CharPosConstraintdecode_pos_constraint("
                 "conststd::string&entry_name,conststd::vector<uint8_t>&body)",
                 "native CharPosConstraint decoder exists");
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
  ok &= contains(char_clip,
                 "delta.x=std::clamp(delta.x,constraint.box_min[0],"
                 "constraint.box_max[0]);",
                 "native CharPosConstraint poll clamps target/source x delta");
  ok &= contains(char_clip,
                 "character.runtime_world_overrides[target]=target_world;",
                 "native CharPosConstraint poll publishes source target world row");
  ok &= contains(char_clip,
                 "apply_source_pos_constraints(character);",
                 "native controller cadence runs CharPosConstraint poll");
  ok &= contains(doc,
                 "`CharPosConstraint::Load` accepts source revisions through 2",
                 "document records CharPosConstraint source load");
  ok &= contains(doc,
                 "Native GHOGX ports this `Poll` path directly",
                 "document records CharPosConstraint runtime writeback");
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
                 "SourceCharLookAtBoundssource_char_lookat_sync_limits("
                 "floatmin_yaw,floatmax_yaw,floatmin_pitch,floatmax_pitch);",
                 "native header exposes CharLookAt SyncLimits helper");
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
  ok &= contains(lookat_source_test,
                 "source_char_lookat_sync_limits(-80.0f,80.0f,-80.0f,80.0f)",
                 "focused CharLookAt source test covers default source limits");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_sync_limits(-120.0f,120.0f,-90.0f,90.0f)",
                 "focused CharLookAt source test covers clamped limits");
  ok &= contains(lookat_source_test,
                 "source_char_lookat_sync_limits(-30.0f,45.0f,-10.0f,20.0f)",
                 "focused CharLookAt source test covers asymmetric limits");
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
                 "Current stock GH2 `CharLookAt` rows observed in the base characters have\n"
                 "    `mDest=<none>`",
                 "document records stock CharLookAt inert destination boundary");
  ok &= contains(rb3_char_eyes_cpp,
                 "else{ObjPtrList<CharLookAt,ObjectDir>pList(this,"
                 "kObjListNoNull);bs>>pList;mEyes.resize(pList.size());",
                 "RB3 CharEyes old revisions read a CharLookAt list");
  ok &= contains(rb3_char_eyes_cpp,
                 "if(gRev-3<=1U){ObjPtr<RndTransformable,ObjectDir>tPtr(this,"
                 "0);bs>>tPtr;}",
                 "RB3 CharEyes rev 3/4 consumes a trailing transformable");
  ok &= contains(rb3_char_eyes_cpp, "plist.push_back((*it).mEye);",
                 "RB3 CharEyes delegates poll children to CharLookAt rows");
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
  ok &= contains(char_mesh,
                 "if(t.version==2&&r.pos+4<=r.n)(void)r.i32();"
                 "if(t.version>3&&r.pos+4<=r.n)t.bias_degrees=r.f32();",
                 "native CharForeTwist decoder follows source revision fields");
  ok &= contains(char_clip, "apply_source_upper_twists(",
                 "native standalone upper twist path is source-named");
  ok &= contains(char_clip, "apply_source_fore_twist(",
                 "native standalone fore twist path is source-named");
  ok &= contains(char_clip,
                 "quat_from_vec_to_vec(mat_row(upper_parent_world,0),"
                 "mat_row(upper_world,0),q);",
                 "native CharUpperTwist port follows source MakeRotQuat rows");
  ok &= contains(char_clip,
                 "write_output(twist1,0.333f);write_output(twist2,0.666f);",
                 "native CharUpperTwist port keeps source interpolation weights");
  ok &= contains(char_clip,
                 "std::atan2(clamped2,clamped)+bias",
                 "native CharForeTwist port keeps source angle basis and bias");
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
                 "Native `source_char_bones_samples_rotate_by_offset`,\n"
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
                 "It does not expose a native `FindPtr` pointer into live sample storage",
                 "document fences native CharBones FindPtr pointer path");
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
                 "std::vector<float>frames;};",
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
                 "source_char_bones_find_offset(lookup_state,\"bone_b.quat\")",
                 "focused CharBones source test covers FindOffset type bucket");
  ok &= contains(char_bones_source_test,
                 "source_char_bones_find_offset(lookup_state,\"bone_missing.pos\")",
                 "focused CharBones source test covers missing FindOffset row");
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
                 "if(gRev>6)bs>>mRotationContext;elsemRotationContext="
                 "mRotation!=CharBones::TYPE_END;",
                 "latest CharBone source gates rotation context");
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
                 "std::optional<size_t>source_char_bone_find_weight_index("
                 "constCharClip::OutputBone&bone,intcontext_mask);",
                 "native API exposes source CharBone FindWeight helper");
  ok &= contains(char_clip_h,
                 "floatsource_char_bone_get_weight("
                 "constCharClip::OutputBone&bone,intcontext_mask);",
                 "native API exposes source CharBone GetWeight helper");
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
                 "voidsource_char_bone_dir_list_bones("
                 "conststd::vector<CharClip::OutputBone>&output_bones,"
                 "intmove_context,intcontext_mask,boolinclude_delta_facing,"
                 "std::vector<SourceCharBonesBone>&bones);",
                 "native API exposes source CharBoneDir ListBones helper");
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
                 "\"[clip-output]%-28ssourceCharBoneversion=%u\"",
                 "native clip debug log labels source CharBone rows");
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharBone.cpp` and",
                 "document cites latest CharBone source");
  ok &= contains(doc,
                 "Native clip output rows now decode and log those fields",
                 "document records native CharBone row decode");
  ok &= contains(doc,
                 "Native `source_char_bone_find_weight_index`,",
                 "document records native CharBone helper ports");
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
                 "source_char_bone_dir_list_bones(dir_output_bones,0x1,0x1,"
                 "true,",
                 "focused CharBones source test covers CharBoneDir facing rows");
  ok &= contains(char_bones_source_test,
                 "source_char_bone_dir_list_bones(dir_output_bones,0x1,0x4,"
                 "false,",
                 "focused CharBones source test covers CharBoneDir delegation without facing");
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
  ok &= contains(doc,
                 "`rb3-latest/src/system/char/CharUtl.cpp` and",
                 "document cites latest CharUtl source");
  ok &= contains(doc,
                 "`CharUtlFindBoneTrans` uses the same `.cb` lookup first",
                 "document records source CharUtl lookup order");
  ok &= contains(doc,
                 "Native `source_char_utl_name_with_suffix`,",
                 "document records native CharUtl helper ports");
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
                 "preview stores the clamped sample and selected row offset",
                 "document records source CharBonesSamples preview offset");
  ok &= contains(doc,
                 "Native `source_char_bones_samples_rotate_by_offset`,",
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
                 "if(mask&0xF0U)mPlayFlags=mPlayFlags&0xffffff0f|mask&0xf0U;"
                 "if(mask&0xFU)mPlayFlags=mPlayFlags&0xfffffff0|mask&0xfU;"
                 "if(mask&0xF600U)mPlayFlags=mPlayFlags&0xffff09ff|mask&0xf600U;",
                 "latest CharClipDriver source masks blend loop and beat-align flags");
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
                 "voidCharClip::SetPlayFlags(inti){if(i!=mPlayFlags){"
                 "mPlayFlags=i;mDirty=true;}}",
                 "latest CharClip source exposes SetPlayFlags dirty guard");
  ok &= contains(rb3_latest_char_clip_cpp,
                 "voidCharClip::SetFlags(inti){if(i!=mFlags){mFlags=i;"
                 "mDirty=true;}}",
                 "latest CharClip source exposes SetFlags dirty guard");
  ok &= contains(doc,
                 "Native `source_char_clip_set_flags` and",
                 "document records native CharClip flag dirty helpers");
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
                 "SourceCharClipDefaultStatesource_char_clip_default_state();",
                 "native character API exposes source CharClip default-state helper");
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
                 "floatsource_driver_blend_width_=1.0f;",
                 "native CharClipPlayer stores source driver blend default");
  ok &= contains(char_clip_h,
                 "boolsource_play_multiple_clips_=false;",
                 "native CharClipPlayer stores source play-multiple default");
  ok &= contains(char_clip,
                 "uint32_tchar_clip_driver_masked_play_flags(constCharClip&clip,"
                 "uint32_tmask){uint32_tplay_flags=clip.default_play_flags;",
                 "native CharClipDriver mask helper starts from stored clip flags");
  ok &= contains(char_clip,
                 "if(mask&0xF0u)play_flags=(play_flags&0xffffff0fu)|"
                 "(mask&0xF0u);if(mask&0x0Fu)play_flags=("
                 "play_flags&0xfffffff0u)|(mask&0x0Fu);if(mask&0xF600u){"
                 "play_flags=(play_flags&0xffff09ffu)|(mask&0xF600u);}",
                 "native CharClipDriver mask helper matches source bit groups");
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
                 "SourceCharClipFlagUpdatesource_char_clip_set_play_flags("
                 "uint32_tcurrent_play_flags,boolcurrent_dirty,"
                 "uint32_trequested_play_flags){SourceCharClipFlagUpdateupdate;"
                 "update.value=current_play_flags;update.dirty=current_dirty;"
                 "if(requested_play_flags!=current_play_flags){update.value="
                 "requested_play_flags;update.dirty=true;update.changed=true;}"
                 "returnupdate;}",
                 "native CharClip SetPlayFlags helper ports source dirty guard");
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
  ok &= contains(doc,
                 "Native `source_char_clip_group_num_flag_duplicates` ports",
                 "document records native CharClipGroup duplicate helper");
  ok &= contains(doc,
                 "Native `source_char_clip_group_sorted_names` ports",
                 "document records native CharClipGroup sort helper");
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
