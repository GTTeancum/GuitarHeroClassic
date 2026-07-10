#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("failed to open " + path.string());
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
              const char* label) {
  if (haystack.find(needle) != std::string::npos) return true;
  std::cerr << "Missing hair contract: " << label << "\n";
  return false;
}

bool lacks(const std::string& haystack, const std::string& needle,
           const char* label) {
  if (haystack.find(needle) == std::string::npos) return true;
  std::cerr << "Forbidden hair contract text present: " << label << "\n";
  return false;
}

std::string function_body(const std::string& source,
                          const std::string& function_name) {
  const size_t name_pos = source.find(function_name);
  if (name_pos == std::string::npos) return {};
  const size_t open = source.find('{', name_pos);
  if (open == std::string::npos) return {};
  int depth = 0;
  for (size_t i = open; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
    } else if (source[i] == '}') {
      --depth;
      if (depth == 0) return source.substr(open, i - open + 1);
    }
  }
  return {};
}

}  // namespace

int main() {
  const std::filesystem::path source_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::string char_mesh_h = read_file(source_dir / "char_mesh.h");
  const std::string char_mesh_cpp = read_file(source_dir / "char_mesh.cpp");
  const std::string char_clip = read_file(source_dir / "char_clip.cpp");
  const std::string char_renderer = read_file(source_dir / "char_renderer.cpp");
  const std::string char_bind_audit =
      read_file(source_dir / "char_bind_audit.cpp");
  const std::string character_notes =
      read_file(source_dir / "CHARACTER_FORMAT_NOTES.md");
  const std::string milo_preview_batch = read_file(
      source_dir / ".." / ".." / ".." / "analysis" /
      "ihatecompvir_milo_samples" / "MiloPreviewBatch" / "Program.cs");

  const std::string char_mesh_h_c = compact(char_mesh_h);
  const std::string char_mesh_cpp_c = compact(char_mesh_cpp);
  const std::string char_clip_c = compact(char_clip);
  const std::string char_renderer_c = compact(char_renderer);
  const std::string char_bind_audit_c = compact(char_bind_audit);
  const std::string character_notes_c = compact(character_notes);
  const std::string milo_preview_batch_c = compact(milo_preview_batch);
  const std::string apply_hair_c =
      compact(function_body(char_clip, "apply_char_hair"));

  bool ok = true;

  ok &= contains(char_mesh_h_c,
                 "floatpos_world[3]={0,0,0};",
                 "runtime hair point keeps source CharHair point position");
  ok &= contains(char_mesh_h_c,
                 "floatforce_world[3]={0,0,0};floatlast_friction_world[3]",
                 "runtime hair point keeps source CharHair force/friction state");
  ok &= contains(char_mesh_h_c,
                 "floatlast_z_world[3]={0,0,1};",
                 "runtime hair point keeps source CharHair lastZ row state");
  ok &= contains(char_mesh_h_c,
                 "bone_world_local_chain_authored(conststd::string&bone_name)"
                 "const;",
                 "characters expose authored local-chain rows");
  ok &= contains(char_mesh_cpp_c,
                 "returnsource_world_for(*this,bone_name,false,false);",
                 "authored source transform rows ignore runtime controller overrides");

  ok &= lacks(char_clip_c,
              "GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE",
              "source hair path no longer keeps PS2 single-point trial switch");
  ok &= lacks(char_clip_c,
              "GHOGX_ENABLE_SOURCE_CHAR_HAIR_SIM",
              "source hair simulation is no longer hidden behind an env switch");
  ok &= lacks(char_clip_c,
              "GHOGX_SOURCE_CHAR_HAIR_AUTHORED_INIT",
              "source authored point init is no longer hidden behind an env switch");
  ok &= lacks(char_clip_c,
              "GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_BASIS",
              "source rootMat basis is no longer hidden behind an env switch");
  ok &= lacks(char_clip_c,
              "GHOGX_SOURCE_CHAR_HAIR_ROOTMAT_LIVE_POS",
              "source rootMat live-position trial switch is removed");
  ok &= lacks(char_clip_c,
              "GHOGX_ENABLE_SOURCE_SINGLE_POINT_CHAIN",
              "source hair path no longer has a guessed one-point chain switch");
  ok &= contains(char_clip_c,
                 "autosource_root_transform=",
                 "source-lineage CharHair simulation is the default path");
  ok &= contains(apply_hair_c,
                 "return{point.pos[0],point.pos[1],point.pos[2]};",
                 "source-lineage CharHair authored point position is used directly");
  ok &= contains(apply_hair_c,
                 "set_runtime_point_pos(state,authored_point(point));",
                 "source-lineage CharHair reset uses decoded point data");
  ok &= lacks(apply_hair_c,
              "transform_local_chain_world(character,point.collision",
              "CharHair collision object must not be used as authored point parent");
  ok &= contains(apply_hair_c,
                 "strand.root_mat[r*3+k]*parent_world[k*4+c]",
                 "source-lineage CharHair rootMat basis uses decoded rootMat rows");
  ok &= contains(apply_hair_c,
                 "simulate_or_publish(hair,strand_starts,true,0.0f,0.0f,"
                 "\"DoReset\")",
                 "source-lineage CharHair performs a source DoReset pass");
  ok &= lacks(char_clip_c,
              "source-rootmat-livepos",
              "source-lineage CharHair no longer advertises the live-position rootMat trial");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_tool_samples_20260710f/`",
                 "fresh glTFMilo/MiloLib source-tool samples are recorded");
  ok &= contains(character_notes_c,
                 "native_source_tool_goes_20260710f/`",
                 "source-guided native visual goes are recorded");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_tool_samples_20260710g/`",
                 "fresh rerun glTFMilo/MiloLib source-tool samples are recorded");
  ok &= contains(character_notes_c,
                 "native_source_tool_goes_20260710g/`",
                 "fresh rerun source-guided native visual matrix is recorded");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_collision_rows_20260710v/`",
                 "glTFMilo collision-row source sample pass is recorded");
  ok &= contains(character_notes_c,
                 "native_after_authored_point_fix_20260710w/`",
                 "native authored-point correction visual matrix is recorded");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_native_goes_20260710y2/`",
                 "fresh rebuilt glTFMilo/MiloLib source previews are recorded");
  ok &= contains(character_notes_c,
                 "native_rockabill2_source_controller_goes_20260710z2/`",
                 "fresh Rockabill2 source-controller native A/B matrix is recorded");
  ok &= contains(character_notes_c,
                 "`GHOGX_DISABLE_CHAR_HAIR=1`,"
                 "`GHOGX_ENABLE_SOURCE_CHAR_HAIR_SIM=1`,",
                 "fresh Rockabill2 native matrix records the source-controller gates");
  ok &= contains(character_notes_c,
                 "DisablingallCharHairbarelychangesthepompadour/loose-card"
                 "profile",
                 "Rockabill2 static-vs-live evidence is recorded");
  ok &= contains(character_notes_c,
                 "Forcingthesourcesingle-pointchaindoespublishalive"
                 "`bone_hair.mesh`rowandvisiblydragsthepompadourcard"
                 "forward/down",
                 "Rockabill2 bad live single-point diagnostic remains rejected");
  ok &= contains(character_notes_c,
                 "collisionless`hair.hair`controllershouldnotbemadelive"
                 "bydefault",
                 "Rockabill2 collisionless one-point source conclusion is recorded");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_matstate_20260710aa/`",
                 "fresh glTFMilo/MiloLib material-state samples are recorded");
  ok &= contains(character_notes_c,
                 "native_matstate_goes_20260710ac/`",
                 "fresh native material-state A/B captures are recorded");
  ok &= contains(character_notes_c,
                 "Rockabill2's`rockabill2_head.mat`,usedby`hair.mesh`,"
                 "`hair2.mesh`,andtheteethmeshes,is`blend=3`,`zMode=1`,"
                 "`alphaCut=1`,",
                 "Rockabill2 source material alpha/cull row is recorded");
  ok &= contains(character_notes_c,
                 "`hair.mesh...alphaTest=1alphaCut=1alphaRef=96"
                 "zMode=1texWrap=1`",
                 "legacy alpha-ref-96 Rockabill2 A/B proof is recorded");
  ok &= contains(character_notes_c,
                 "`hair.mesh...alphaTest=1alphaCut=1alphaRef=0"
                 "zMode=1texWrap=1`",
                 "source alpha-ref-0 Rockabill2 A/B proof is recorded");
  ok &= contains(character_notes_c,
                 "nowuser-signed-offforRockabill2inthisslice",
                 "material-state slice records Rockabill2 user sign-off");
  ok &= contains(character_notes_c,
                 "DonotkeepiteratingonRockabill2forthecurrenthairslice",
                 "Rockabill2 sign-off prevents reopening this exact slice");
  ok &= contains(char_renderer_c,
                 "\"GHOGX_DISABLE_SOURCE_MAT_ALPHA_STATE\"",
                 "legacy alpha path remains an explicit diagnostic gate");
  ok &= contains(char_renderer_c,
                 "use_source_alpha?material->alpha_cut:true;",
                 "renderer enables alpha testing from decoded Mat alphaCut");
  ok &= contains(char_renderer_c,
                 "std::clamp(material->alpha_threshold,0,255)",
                 "renderer uses decoded Mat alphaThreshold as alpha ref");
  ok &= contains(char_renderer_c,
                 "texture_address_for_wrap(material->tex_wrap)",
                 "renderer consumes decoded Mat texWrap");
  ok &= contains(char_renderer_c,
                 "\"alphaRef=%luzMode=%utexWrap=%u\\n\"",
                 "mesh-render logs expose decoded material alpha/wrap state");
  ok &= contains(character_notes_c,
                 "`authored_hair_point_world`nowusesthedecodedCharHairpoint"
                 "`pos`directly",
                 "format notes record direct authored CharHair point interpretation");
  ok &= contains(character_notes_c,
                 "`GHOGX_ENABLE_SOURCE_SINGLE_POINT_CHAIN=1`trialmakesthe"
                 "pompadourfloatforward/downandisrejected",
                 "rejected Rockabill2 source single-point chain trial is recorded");
  ok &= contains(character_notes_c,
                 "coveringcurrent,sourceCharHairsim,authored-init,"
                 "sourcerootMatbasis,",
                 "rerun matrix documents each source-guided native attempt");
  ok &= contains(character_notes_c,
                 "rock1_hair_matrix_steps_extstack_nofocus_iso_"
                 "20260710g.json`",
                 "fresh Rock1 no-focus matrix trace is recorded");
  ok &= contains(character_notes_c,
                 "rock2_hair_matrix_steps_extstack_nofocus_iso_"
                 "20260710g.json`",
                 "fresh Rock2 no-focus matrix trace is recorded");
  ok &= contains(character_notes_c,
                 "rockabill2_hair_matrix_steps_extstack_nofocus_iso_"
                 "20260710g.json`",
                 "fresh Rockabill2 no-focus matrix trace is recorded");
  ok &= contains(character_notes_c,
                 "step_1778cc=2410,step_1778e4=2420",
                 "fresh Rock1 trace counts are recorded");
  ok &= contains(character_notes_c,
                 "step_1778cc=2223,step_1778e4=2223",
                 "fresh Rock2 trace counts are recorded");
  ok &= contains(character_notes_c,
                 "step_1778cc=1012,step_1778e4=1012",
                 "fresh Rockabill2 trace counts are recorded");
  ok &= contains(character_notes_c,
                 "`0xfae40000,0xdae40000`at`0x001778cc`and"
                 "`0xdbc40000,0x4bc4216a`at",
                 "fresh construction hook opcodes are recorded");
  ok &= contains(character_notes_c,
                 "shiftedRockabill2store-windowreading",
                 "stale shifted Rockabill2 store-window trace is fenced off");
  ok &= contains(character_notes_c,
                 "hair_group_decode_audit_20260710g/`",
                 "fresh hair group decode audit is recorded");
  ok &= contains(character_notes_c,
                 "rawrow-scale/no-normalizenativechangeisnot"
                 "source-backedorauthorized",
                 "row-scale/no-normalize hair band-aid is rejected");
  ok &= contains(character_notes_c,
                 "native_source_tool_goes_20260710h/`",
                 "source-rootMat live-position visual trial is recorded");
  ok &= contains(character_notes_c,
                 "basis=source-rootmat-livepos`and`sim=native-predict`",
                 "source-rootMat live-position logs are recorded");
  ok &= contains(character_notes_c,
                 "KeepthisoffbydefaultuntilPS2tracesprovethe"
                 "live-root-positioncontract",
                 "source-rootMat live-position trial is not promoted by default");
  ok &= contains(character_notes_c,
                 "Go4(`*_go4_source_charhair_sim_profile.png`)",
                 "source-force visual trial is documented");
  ok &= contains(character_notes_c,
                 "Go7(`*_go7_rootmat_basis_existing_predictor_profile.png`)",
                 "rootMat-basis visual trial is documented");
  ok &= contains(character_notes_c,
                 "Thegatedhooksremaindiagnosticonly.",
                 "unproven source-guided trials are not promoted by default");
  ok &= contains(character_notes_c,
                 "Rockabill2source-single-chaindiagnostic",
                 "Rockabill2 source-single-chain diagnostic is documented");
  ok &= contains(character_notes_c,
                 "`GHOGX_ENABLE_SOURCE_SINGLE_POINT_CHAIN=1`diagnostic",
                 "source single-point chain diagnostic stays explicit");
  ok &= contains(character_notes_c,
                 "`analysis/hair_source_single_chain_20260710k/`",
                 "source-single-chain visual proof folder is recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_default_restored_profile.png`",
                 "Rockabill2 restored default proof image is recorded");
  ok &= contains(character_notes_c,
                 "`[charhair-follow-ps2]`",
                 "Rockabill2 restored default log label is recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_source_single_chain_gated_profile.png`",
                 "Rockabill2 gated source-single proof image is recorded");
  ok &= contains(character_notes_c,
                 "`reason=source-single-point`",
                 "Rockabill2 gated source-single log label is recorded");
  ok &= contains(character_notes_c,
                 "Visualresultrejectspromotion",
                 "Rockabill2 source-single-chain visual rejection is recorded");
  ok &= contains(character_notes_c,
                 "Keepthisdiagnosticoffbydefault",
                 "Rockabill2 source-single-chain diagnostic remains disabled");
  ok &= contains(character_notes_c,
                 "glTFMilosource-helpercheckpoint",
                 "fresh glTFMilo source-helper checkpoint is documented");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_checkpoint_20260710l/`",
                 "fresh glTFMilo checkpoint output folder is recorded");
  ok &= contains(character_notes_c,
                 "27focused`*problem*.png`samples",
                 "fresh glTFMilo checkpoint focused PNG count is recorded");
  ok &= contains(character_notes_c,
                 "`rock1_problem_skin_bind_side.png`",
                 "fresh Rock1 glTFMilo focused sample is recorded");
  ok &= contains(character_notes_c,
                 "`rock2_problem_skin_bind_side.png`",
                 "fresh Rock2 glTFMilo focused sample is recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_problem_skin_bind_side.png`",
                 "fresh Rockabill2 glTFMilo focused sample is recorded");
  ok &= contains(character_notes_c,
                 "doesnotauthorizepromotingtherejectedsource-single-chain",
                 "fresh glTFMilo checkpoint does not authorize a native default change");
  ok &= contains(milo_preview_batch_c,
                 "staticstringMatrixSummary(HmxMatrixm)",
                 "MiloPreviewBatch logs source slot matrix rows");
  ok &= contains(milo_preview_batch_c,
                 "\"[mesh-slot]character={character}mesh=\\\"{mesh.Name}\\\""
                 "slot={slot.i}bone=\\\"{slot.b.name.value}\\\"",
                 "MiloPreviewBatch emits per-slot bind rows");
  ok &= contains(character_notes_c,
                 "glTFMilo/nativebind-rowcomparison",
                 "fresh glTFMilo/native bind-row comparison is documented");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_bindrows_20260710m/`",
                 "fresh bind-row source-tool output folder is recorded");
  ok &= contains(character_notes_c,
                 "native_bind_compare_20260710m_hairfront1.log`",
                 "fresh native Rock1/Rock2 bind-audit log is recorded");
  ok &= contains(character_notes_c,
                 "`native_bind_compare_20260710m_rockabill2_hair.log`",
                 "fresh native Rockabill2 hair bind-audit log is recorded");
  ok &= contains(character_notes_c,
                 "`native_bind_compare_20260710m_rockabill2_hair2.log`",
                 "fresh native Rockabill2 hair 2 bind-audit log is recorded");
  ok &= contains(character_notes_c,
                 "MiloLibandnativeagreetorounding",
                 "source and native active slot rows agree");
  ok &= contains(character_notes_c,
                 "nativetrimstherealpalettetothenamedactiveslots",
                 "unnamed trailing source slots are bounded off");
  ok &= contains(character_notes_c,
                 "notthestaticslotrowdecode",
                 "remaining problem is not static slot-row decode");
  ok &= contains(char_clip_c,
                 "runtime_point_last_friction(constRuntimeHairPoint&p)",
                 "source-lineage CharHair simulation keeps lastFriction state");
  ok &= contains(char_clip_c,
                 "constVec3gravity_vec{0.0f,0.0f,hair.gravity*f19*-3.858268f};",
                 "source-lineage CharHair simulation uses Harmonix gravity step");
  ok &= contains(char_clip_c,
                 "[charhair-source-sim]",
                 "CharHair debug logs prove which simulation path ran");
  ok &= lacks(char_clip_c,
              "source_ps2_single_point_chain_group",
              "source hair no longer classifies rows by guessed one-point shape");
  ok &= lacks(char_clip_c,
              "source_ps2_root_controller_group",
              "source hair no longer promotes one-point root-controller guesses");
  ok &= lacks(apply_hair_c,
              "source_ps2_root_controller",
              "root-controller special-case path is removed");
  ok &= lacks(apply_hair_c,
              "source_single_point_chain_solver",
              "single-point solver special-case path is removed");
  ok &= lacks(apply_hair_c,
              "source_collisionless_single_point",
              "collisionless one-point special-case path is removed");
  ok &= lacks(apply_hair_c,
              "source_root_controller_descriptor",
              "descriptor/root-controller special-case path is removed");
  ok &= lacks(char_clip_c,
              "charhair_source_controller_world(constCharacter&character,",
              "guessed source-controller row helper is removed");
  ok &= contains(apply_hair_c,
                 "transform_local_chain_world(character,*root_target.name,root_world)",
                 "source root transform uses the live local-chain root position");
  ok &= contains(apply_hair_c,
                 "character.bone_world_local_chain(*root_target.parent)",
                 "source root transform uses the parent local-chain world row");
  ok &= contains(apply_hair_c,
                 "v+=strand.root_mat[r*3+k]*parent_world[k*4+c];",
                 "source root transform multiplies decoded rootMat rows by the parent row");
  ok &= lacks(apply_hair_c,
              "follow_only_group",
              "follow-only one-point classification is removed from active hair path");
  ok &= lacks(apply_hair_c,
              "[charhair-static-source]",
              "static one-point source log path is removed");
  ok &= contains(apply_hair_c,
                 "source_root_transform(strand,root_target,segment_world)",
                 "segment roots resolve through source rootMat rows");
  ok &= contains(apply_hair_c,
                 "constauto&point=points[point_index];",
                 "each source point row is consumed by the CharHair loop");
  ok &= contains(apply_hair_c,
                 "set_runtime_point_pos(state,authored_point(point));",
                 "first reset anchor comes from decoded CharHair point rows");
  ok &= lacks(apply_hair_c,
              "descriptor_world",
              "descriptor/follow one-point path is removed from active hair path");
  ok &= lacks(apply_hair_c,
              "has_group_source_world[point_index+1]",
              "next-controller endpoint guessing is removed");
  ok &= contains(char_clip_c,
                 "[charhair-source]",
                 "hair debug log inventories decoded CharHair data");
  ok &= contains(char_clip_c,
                 "\"GHOGX_DEBUG_CLIP_HAIR\"",
                 "clip hair diagnostics stay explicitly gated");
  ok &= contains(char_clip_c,
                 "[clip-hair-output]",
                 "clip hair diagnostics expose decoded output targets");
  ok &= contains(char_clip_c,
                 "source=ihatecompvir-CharHair",
                 "hair source log names ihatecompvir CharHair as the evidence path");
  ok &= contains(char_clip_c,
                 "strands=%zupoints=%zu",
                 "hair source log inventories decoded strands and points");
  ok &= lacks(char_clip_c,
              "followOnlyGroups",
              "hair source log no longer categorizes guessed follow-only groups");
  ok &= lacks(char_clip_c,
              "sourceSingleChainGroups",
              "hair source log no longer categorizes guessed one-point chains");
  ok &= lacks(char_clip_c,
              "sourceRootControllerGroups",
              "hair source log no longer categorizes guessed root controllers");
  ok &= contains(apply_hair_c,
                 "log_char_hair_source_once(character,hair);",
                 "CharHair source inventory runs from the native hair poller");
  ok &= contains(apply_hair_c,
                 "character.runtime_world_overrides[*target.name]=segment_world;",
                 "CharHair submits runtime Trans rows for renderer/skinning");
  ok &= contains(apply_hair_c,
                 "Vec3axis=vsub(pos,mat_pos(segment_world));",
                 "source loop computes each segment axis from the current anchor");
  ok &= contains(apply_hair_c,
                 "pos=vadd(pos,vscale(axis,length_scale));",
                 "source loop applies the Harmonix length constraint");
  ok &= contains(apply_hair_c,
                 "Vec3roll=blend_vec(runtime_point_last_z(state),"
                 "mat_row(segment_world,2),torsion);",
                 "source loop blends persistent lastZ toward the segment roll by torsion");
  ok &= contains(apply_hair_c,
                 "Vec3row1=vnorm(axis,mat_row(segment_world,1));",
                 "source loop normalizes row1 from the point axis");
  ok &= contains(apply_hair_c,
                 "Vec3row0=vnorm(vcross(row1,roll),mat_row(segment_world,0));",
                 "source loop builds row0 from row1 x torsion-blended lastZ");
  ok &= contains(apply_hair_c,
                 "Vec3row2=vnorm(vcross(row0,row1),roll);",
                 "source loop rebuilds row2 from row0 x row1");
  ok &= contains(apply_hair_c,
                 "set_runtime_point_last_z(state,mat_row(segment_world,2));",
                 "source loop stores the submitted row2 as persistent lastZ");
  ok &= contains(apply_hair_c,
                 "set_runtime_point_world(state,segment_world);",
                 "source loop captures the submitted source segment transform");
  ok &= contains(apply_hair_c,
                 "segment_world[12]=pos.x;segment_world[13]=pos.y;"
                 "segment_world[14]=pos.z;",
                 "source loop carries each solved point position to the next segment");
  ok &= contains(apply_hair_c,
                 "constVec3source_target=vadd(mat_pos(segment_world),"
                 "vscale(mat_row(segment_world,1),length));",
                 "source loop computes the original source target for force update");
  ok &= contains(apply_hair_c,
                 "force=(%.4f%.4f%.4f)",
                 "source loop debug logs force state");
  ok &= contains(apply_hair_c,
                 "lastFriction=(%.4f%.4f%.4f)",
                 "source loop debug logs lastFriction state");
  ok &= contains(apply_hair_c,
                 "lastZ=(%.4f%.4f%.4f)",
                 "source loop debug logs lastZ state");
  ok &= contains(apply_hair_c,
                 "pass=%sadvance=%d",
                 "source loop debug logs the exact source pass");
  ok &= contains(apply_hair_c,
                 "simulate_or_publish(hair,strand_starts,true,0.0f,0.0f,\"DoReset\")",
                 "source loop performs a zero-inertia DoReset simulation pass");
  ok &= contains(apply_hair_c,
                 "simulate_or_publish(hair,strand_starts,true,hair.inertia,"
                 "hair.friction,\"SimulateInternal\")",
                 "source loop performs a source SimulateInternal pass");
  ok &= lacks(apply_hair_c,
              "root-controller-descriptor",
              "root-controller descriptor label is removed");
  ok &= lacks(apply_hair_c,
              "source-single-point",
              "single-point debug label is removed");
  ok &= lacks(apply_hair_c,
              "source-root-controller",
              "root-controller debug label is removed");
  ok &= contains(apply_hair_c,
                 "simulate_or_publish(hair,strand_starts,false,hair.inertia,"
                 "hair.friction,\"SimulateZeroTime\")",
                 "source loop publishes the source SimulateZeroTime path");
  ok &= lacks(apply_hair_c,
              "ps2_follow_hair_world",
              "follow-only hair helper is removed from active path");
  ok &= lacks(char_clip_c,
              "ps2_follow_hair_world",
              "follow-only hair helper implementation is removed");

  ok &= contains(char_renderer_c,
                 "runtime_hair_world_override(character,mesh.bone_palette[i],",
                 "CharHair palette rows are keyed by decoded controller bone name");
  ok &= contains(char_renderer_c,
                 "material&&material->blend!=0&&is_hair_render_mesh(m)",
                 "material-named hair meshes still use hair render state");
  ok &= contains(char_renderer_c,
                 "character_blend_state_for(material_blend)",
                 "character meshes use decoded Mat.blend render state");
  ok &= contains(char_renderer_c,
                 "dev->SetRenderState(D3DRS_BLENDOP,blend_state.op);",
                 "character renderer applies decoded blend operation");
  ok &= contains(char_renderer_c,
                 "dev->SetRenderState(D3DRS_SRCBLEND,blend_state.src);",
                 "character renderer applies decoded source blend");
  ok &= contains(char_renderer_c,
                 "dev->SetRenderState(D3DRS_DESTBLEND,blend_state.dest);",
                 "character renderer applies decoded destination blend");
  ok &= contains(char_renderer_c,
                 "if(has_hair_override)curr_world=hair_override;",
                 "runtime CharHair rows replace the palette current row");
  ok &= contains(char_renderer_c,
                 "return!char_env_enabled(\"GHOGX_DISABLE_SOURCE_GROUP_DRAW_ORDER\");",
                 "renderer consumes source RndGroup order by default");
  ok &= contains(char_renderer_c,
                 "constboolhas_source_bone_transforms=mesh.bind.size()>=nb;",
                 "renderer detects decoded RndMesh BoneTransform row coverage");
  ok &= contains(char_renderer_c,
                 "constchar*skin_mode=has_source_bone_transforms?"
                 "\"source-offset\":\"missing-source-offset-fallback\";",
                 "renderer consumes decoded source offsets directly whenever RndMesh bone transforms exist");
  ok &= contains(char_renderer_c,
                 "skin[i]=mul16(xfm16(mesh.bind[i]),curr_world);",
                 "source-offset skinning multiplies slot transform by current world");
  ok &= contains(char_renderer_c,
                 "std::array<float,16>curr_world=character.bone_world_local_chain"
                 "(mesh.bone_palette[i]);",
                 "skinning consumes the current Trans WorldXfm local chain");
  ok &= contains(char_renderer_c,
                 "skin[i]=mul16(xfm16(mesh.bind[i]),curr_world);",
                 "source offset path multiplies slot transform by current world");
  ok &= contains(char_renderer_c,
                 "world_mode=\"identity-source-skinned\";",
                 "skinned RndMesh output draws in world space");
  ok &= contains(char_renderer_c,
                 "\"[mesh-world-verts]mesh=%sv0=(",
                 "mesh debug diagnostics expose post-world hair card vertices");
  ok &= contains(char_bind_audit_c,
                 "\"--hair\"",
                 "bind audit exposes decoded hair row inventory");
  ok &= contains(char_bind_audit_c,
                 "source=decoded-CharHair",
                 "bind audit hair inventory names the source-backed path");
  ok &= contains(char_bind_audit_c,
                 "pre_state16=%s",
                 "bind audit exposes raw Mat state around diffuse texture refs");
  ok &= contains(char_bind_audit_c,
                 "ng_cull=%d",
                 "bind audit exposes decoded Mat.ng.cull render state");
  ok &= contains(char_mesh_h_c,
                 "floatdraw_order=0.0f;",
                 "skinned meshes preserve decoded RndDrawable.drawOrder");
  ok &= contains(char_mesh_cpp_c,
                 "mesh.draw_order=r.f32();",
                 "character mesh decoder reads Draw base drawOrder");
  ok &= contains(char_bind_audit_c,
                 "drawOrder=%.3f",
                 "bind audit exposes decoded mesh drawOrder");
  ok &= contains(char_renderer_c,
                 "a_hair&&b_hair&&std::fabs(a->draw_order-b->draw_order)"
                 ">1.0e-5f",
                 "character renderer compares decoded hair drawOrder");
  ok &= contains(char_renderer_c,
                 "returna->draw_order<b->draw_order;",
                 "character renderer draws lower hair drawOrder first");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_draworder_after_20260710r/`",
                 "fresh glTFMilo drawOrder source samples are recorded");
  ok &= contains(character_notes_c,
                 "native_render_draworder_rows_unique_20260710r.txt`",
                 "native drawOrder submission rows are recorded");
  ok &= contains(character_notes_c,
                 "sortsonlythehair-rendergroupbydecoded"
                 "`RndDrawable.drawOrder`",
                 "hair drawOrder sorting is bounded to hair render group");
  ok &= contains(character_notes_c,
                 "Rock2stillhasvisiblywronghairplacement/layering",
                 "drawOrder pass is not overclaimed as Rock2 placement fix");
  ok &= contains(character_notes_c,
                 "source_preview_gltfmilo_groups_20260710s/`",
                 "fresh source group pass is recorded");
  ok &= contains(character_notes_c,
                 "Source-exact`rock2``lod0.grp`includes"
                 "`hair-back.6.mesh`",
                 "Rock2 source group membership is recorded");
  ok &= contains(character_notes_c,
                 "DonothidetheseRock2hairmeshesasafix.",
                 "Rock2 group evidence rejects a hide-mesh band-aid");
  ok &= contains(character_notes_c,
                 "native_rock2_source_go_skinmodes_unique_20260710s.txt`",
                 "failed glTFMilo source-equation trials are recorded");
  ok &= contains(character_notes_c,
                 "../ihatecompvir-public-milo-sources/rb3/src/system/char/"
                 "CharHair.cpp`",
                 "Harmonix CharHair source pass is recorded");
  ok &= contains(character_notes_c,
                 "`SimulateInternal`,theroottransform'sworldpositionand"
                 "`RootMat*root_parent_world_rotation`",
                 "source CharHair controller update basis is recorded");
  ok &= contains(character_notes_c,
                 "continuingtowardexactCharHaircontroller/collisionhookup"
                 "andresetbehavior",
                 "source pass directs next work toward controller hookup");
  ok &= contains(character_notes_c,
                 "GH2PS2revision-2CharHairrowshavethelegacycollision"
                 "symbol/radiusfields",
                 "source pass records GH2-vs-RB3 reset gap");
  ok &= contains(char_renderer_c,
                 "material&&material->has_cull&&!material->cull",
                 "character renderer honors source Mat.ng.cull=false");
  ok &= contains(char_renderer_c,
                 "cullMode=%lu",
                 "mesh diagnostics log the resolved material cull mode");
  ok &= contains(char_renderer_c,
                 "op=%ludrawOrder=%.3f",
                 "mesh diagnostics log decoded drawOrder at submission");
  ok &= contains(char_clip_c,
                 "points=%zuangle=%.4f",
                 "CharHair source logs use the schema strand angle name");
  ok &= contains(char_bind_audit_c,
                 "angle=%.4f",
                 "bind audit reports CharHair strand angle by schema name");
  ok &= contains(char_bind_audit_c,
                 "baseMatR0=(%.4f%.4f%.4f)baseMatR1=(%.4f%.4f%.4f)",
                 "bind audit names the decoded CharHair baseMat rows");
  ok &= contains(char_bind_audit_c,
                 "rootMatR0=(%.4f%.4f%.4f)",
                 "bind audit exposes the decoded CharHair rootMat rows");
  ok &= contains(char_bind_audit_c,
                 "source_set_angle_root_mat(strand.angle,strand.base_mat)",
                 "bind audit verifies rootMat against source SetAngle rows");
  ok &= contains(char_bind_audit_c,
                 "setAngleRootErr=%.6f",
                 "bind audit logs source SetAngle/rootMat agreement");
  ok &= contains(char_bind_audit_c,
                 "[hair-collision-detail]",
                 "bind audit exposes legacy CharHair collision target rows");
  ok &= contains(char_bind_audit_c,
                 "pointDist=%.4f",
                 "bind audit logs legacy CharHair collision distance");
  ok &= contains(char_bind_audit_c,
                 "alignDist=%.4f",
                 "bind audit logs legacy CharHair collision align distance");
  ok &= contains(char_bind_audit_c,
                 "outer=%.4f",
                 "bind audit reports CharHair point outer radius by schema name");
  ok &= contains(char_bind_audit_c,
                 "side=%.4f",
                 "bind audit reports CharHair point side length by schema name");
  ok &= contains(char_mesh_h_c,
                 "floatouter_radius=0.0f;",
                 "CharHair point outer radius is decoded from source schema");
  ok &= contains(char_mesh_h_c,
                 "uint32_tcollide_type=0;std::stringcollision;floatradius=0.0f;"
                 "floatouter_radius=0.0f;",
                 "CharHair point keeps GH2 v2 legacy collision schema fields");
  ok &= contains(char_clip_c,
                 "if(point.collision.empty())returnfalse;",
                 "legacy CharHair collision only resolves decoded target rows");
  ok &= contains(char_clip_c,
                 "transform_local_chain_world(character,point.collision,"
                 "collision_world)",
                 "legacy CharHair collision resolves the decoded collision target row");
  ok &= contains(char_clip_c,
                 "switch(point.collide_type)",
                 "legacy CharHair collision dispatches on decoded collide_type");
  ok &= contains(char_clip_c,
                 "case3:{//kCollideCylinder",
                 "legacy CharHair collision preserves the documented cylinder shape id");
  ok &= contains(char_clip_c,
                 "[charhair-legacy-collision]",
                 "legacy CharHair collision logs point corrections");
  ok &= contains(char_clip_c,
                 "outer=%.4fbefore=",
                 "legacy CharHair collision logs source outerRadius separately");
  ok &= contains(apply_hair_c,
                 "noCollidesNoWriteback=1",
                 "source CharHair loop does not publish collisionless point rows");
  ok &= lacks(char_mesh_h_c,
              "flags_or_mode",
              "invented CharHair collide mode field is removed");
  ok &= lacks(char_mesh_h_c,
              "collisionobject",
              "invented CharHair collision object field is removed");
  ok &= contains(milo_preview_batch_c,
                 "[source-charcollide]",
                 "glTFMilo source sample logger emits CharCollide rows");
  ok &= contains(char_mesh_h_c,
                 "floatbase_mat[9]={};floatroot_mat[9]={};",
                 "CharHair strand matrix tail is documented as baseMat/rootMat");
  ok &= contains(character_notes_c,
                 "2026-07-06ihatecompvirin-repopublic-sourcecross-check",
                 "format notes include the in-repo ihatecompvir source pass");
  ok &= contains(character_notes_c,
                 "`../ihatecompvir-public-milo-sources/`",
                 "format notes name the local reference copy");
  ok &= contains(character_notes_c,
                 "`glTFMilo`commit`6c54acb`and`MiloEditor`commit`3ebffb1`",
                 "format notes pin the local reference commits");
  ok &= contains(character_notes_c,
                 "`grim`(`grim/core/grim/src/scene/char_hair/io.rs`),"
                 "`MiloEditor`(`MiloEditor/MiloLib/Assets/Char/CharHair.cs`),"
                 "and`re-notes/templates/milo/char_hair.bt`agree",
                 "public CharHair parsers are named as local evidence");
  ok &= contains(character_notes_c,
                 "GH2/GH2360`CharHair`version/revision2loadsglobals,"
                 "thenstrandsas`root`,`angle`,`point_count`",
                 "public source preserves the GH2 CharHair v2 strand schema");
  ok &= contains(character_notes_c,
                 "Thetemplateexplicitlydescribes`bone`asthehairbonewhose"
                 "transformisset",
                 "re-notes template confirms bone is the driven transform");
  ok &= contains(character_notes_c,
                 "simulationeventuallycalls`bone->SetWorldXfm(...)`",
                 "runtime source proves CharHair writes live Trans rows");
  ok &= contains(character_notes_c,
                 "visiblehairmeshesareconsumersofthosedrivenTransrows",
                 "notes keep CharHair separate from visible mesh consumers");
  ok &= contains(character_notes_c,
                 "`RndMesh`rev28skinningtailasafour-bonepaletteplusfour"
                 "offsets",
                 "public RndMesh source preserves the GH2-era palette tail");
  ok &= contains(character_notes_c,
                 "onlyreadsexplicitper-vertex`boneIndices`innewerrevs",
                 "public RndMesh source bounds explicit bone-index behavior");
  ok &= contains(character_notes_c,
                 "`RndMat`readsGH2-eramaterialstateinthissourceorderafter"
                 "color:`preLit`,`useEnviron`,`zMode`,`alphaCut`,"
                 "`alphaWrite`,`texGen`,`texWrap`,`texXfm`,`diffuseTex`,"
                 "`nextPass`,`intensify`,`cull`,`emissiveMultiplier`",
                 "format notes preserve the in-repo RndMat source order");
  ok &= contains(character_notes_c,
                 "2026-07-10`o`glTFMilomaterial-statebuild/testpass",
                 "format notes include the built glTFMilo material-state pass");
  ok &= contains(character_notes_c,
                 "`../ihatecompvir-public-milo-sources/glTFMilo/external/"
                 "MiloEditor/MiloLib`",
                 "format notes identify the vendored MiloLib used by the helper");
  ok &= contains(character_notes_c,
                 "flagbytes`0100`followedbyz-mode`01000000`,matching"
                 "`preLit=1/useEnviron=0`",
                 "format notes preserve the raw material flag-byte evidence");
  ok &= contains(character_notes_c,
                 "Nativenowdecodesthetwobytes",
                 "native Mat flag order follows the glTFMilo source evidence");
  ok &= contains(character_notes_c,
                 "PublicsourcedoesnotbyitselfprovethefinalRock1or"
                 "Rockabill2hair-cardconsumerequation",
                 "public source pass does not overclaim remaining hair fixes");
  ok &= contains(character_notes_c,
                 "2026-07-09ihatecompvirglTFMilo/MiloLibsamplepass",
                 "format notes include the built glTFMilo/MiloLib sample pass");
  ok &= contains(character_notes_c,
                 "stockGH2PS2`rock1`,`rock2`,`rockabill2`,`funk1`,"
                 "`grim`,`rockabill1`,and`deathmetal1`",
                 "sample pass covers the expanded stock PS2 character set");
  ok &= contains(character_notes_c,
                 "Direct`MiloglTF`executableattemptsarerecorded",
                 "format notes record direct ihatecompvir executable attempts");
  ok &= contains(character_notes_c,
                 "136-byteemptyGLBshellsbecausethecurrentMILO-to-GLTFpath"
                 "onlyexportslights",
                 "format notes bound off the empty MiloglTF mesh-export path");
  ok &= contains(character_notes_c,
                 "`gltf_tool_runs/rerun_20260709/`repeatsthisfor"
                 "`rockabill2`,`rock1`,and`funk1`",
                 "direct glTFMilo rerun remains bounded to empty GLB outputs");
  ok &= contains(character_notes_c,
                 "`RndMesh.boneTransforms`arewrittenas`inverse(jointNode."
                 "WorldMatrix)*node.WorldMatrix`",
                 "format notes preserve the glTFMilo palette-row equation");
  ok &= contains(character_notes_c,
                 "Atemporaryhelpervariantthatconsumedtheshared"
                 "`Vertex.bone0..bone3`fieldsasindicesisrejectedforGH2PS2",
                 "rev28 helper does not promote newer bone-index fields");
  ok &= contains(character_notes_c,
                 "MiloLibonlyfillsthosefieldsforrevision33andnewer,"
                 "whileeverystockGH2PS2sampleinthispasslogged"
                 "`rev=28indexedBones=0`",
                 "format notes bind explicit bone indices to newer mesh revs");
  ok &= contains(character_notes_c,
                 "renders_rebuilt_20260709",
                 "rebuilt glTFMilo/MiloLib visual sample directory is recorded");
  ok &= contains(character_notes_c,
                 "renders_rebuilt_20260709_ps2slot",
                 "corrected PS2 slot-order sample directory is recorded");
  ok &= contains(character_notes_c,
                 "afterbuildingboth`ihatecompvir-public-milo-sources/"
                 "glTFMilo/MiloGLTFUtils.sln`andthelocal`MiloPreviewBatch`",
                 "rebuilt source-reference tools are recorded");
  ok &= contains(character_notes_c,
                 "Itclassifieshairbymeshnameormaterialname",
                 "MiloLib helper material-aware hair classification is documented");
  ok &= contains(character_notes_c,
                 "Supersededintermediaterenderdirectories,includingthe"
                 "invalidindexed-bonedetour,wereremoved",
                 "superseded glTFMilo visual artifact bloat is bounded");
  ok &= contains(character_notes_c,
                 "`funk1.37.mesh`with`funk1_hair.mat`",
                 "Funk1 material-named hair sample is documented");
  ok &= contains(character_notes_c,
                 "Grim/SandTimeKeeperhood,wing,glass,hour-glass,shadow,"
                 "and`grim_head`pieces",
                 "Grim accessory sample classification is documented");
  ok &= contains(character_notes_c,
                 "Rockabill2`hair.mesh`and`hair2.mesh`asseparateRndMesh"
                 "objectswithpaletterows`[0:bone_head.mesh,1:bone_hair.mesh]`",
                 "rebuilt samples preserve Rockabill2 hair palette ownership");
  ok &= contains(character_notes_c,
                 "notchildrenembeddedintheface/bodymeshandshouldnotbe"
                 "repairedwithastaticcardoffset",
                 "Rockabill2 hair remains a palette consumer, not a hidden child");
  ok &= contains(character_notes_c,
                 "`hair.mesh`and`hair2.mesh`aretwo-slotweightedconsumersof"
                 "`bone_head.mesh`and`bone_hair.mesh`",
                 "Rockabill2 hair sample remains a weighted consumer");
  ok &= contains(character_notes_c,
                 "Byitself,thisdoesnotprovetheliveRockabill2drawconsumeror"
                 "authorizenativehairskinningchanges",
                 "glTFMilo samples are not promoted as a hair fix");
  ok &= contains(character_notes_c,
                 "donotchangenativeGH2PS2skinningtoindexed-boneconsumption",
                 "corrected glTFMilo pass rejects indexed-bone native changes");
  ok &= contains(character_notes_c,
                 "`rockabill2_hair_runtime_default.png`,"
                 "`rockabill2_hair_static_no_charhair.png`,"
                 "`rockabill2_hair_singlepoint_diag.png`,and"
                 "`rockabill2_hair_no_local_attachment.png`",
                 "native Rockabill2 hair A/B visual set is documented");
  ok &= contains(character_notes_c,
                 "Disabling`CharHair`makesthefrontcardseparateupward",
                 "static Rockabill2 hair is rejected by visual evidence");
  ok &= contains(character_notes_c,
                 "enabling`GHOGX_ENABLE_PS2_SINGLE_POINT_HAIR_STATE`pushesthe"
                 "cardsideways",
                 "single-point diagnostic remains rejected for Rockabill2");
  ok &= contains(character_notes_c,
                 "neitherstatichairnortheoldsingle-pointdiagnosticisan"
                 "evidence-backedRockabill2fix",
                 "contract prevents promoting rejected Rockabill2 diagnostics");
  ok &= contains(character_notes_c,
                 "`rockabill2_hair_base_root_audit.txt`,"
                 "`rock1_hair_base_root_audit.txt`,and"
                 "`rock2_hair_base_root_audit.txt`",
                 "base/root CharHair matrix audit outputs are documented");
  ok &= contains(character_notes_c,
                 "Rockabill2'svisible`hair.hair`hasidenticalbase/rootrows",
                 "Rockabill2 base/root matrix distinction is bounded off");
  ok &= contains(character_notes_c,
                 "Rock1'smulti-pointfront/backgroupscandifferbetween"
                 "base/rootrows",
                 "Rock1 base/root distinction remains available for tracing");
  ok &= contains(character_notes_c,
                 "boundsoffasimpleRockabill2\"userootMatinsteadofbaseMat\""
                 "fix",
                 "contract prevents a blind Rockabill2 rootMat swap");
  ok &= contains(character_notes_c,
                 "nativeRockabill2rigidfacerowcompare",
                 "format notes include the native rigid face row compare");
  ok &= contains(character_notes_c,
                 "thedecoded`meshWorld`rowmatchestheMiloLibstoredobjectrow",
                 "native compare ties meshWorld to the MiloLib object row");
  ok &= contains(character_notes_c,
                 "`hair.mesh`and`hair2.mesh`remaintwo-slotweightedcards",
                 "native compare does not convert Rockabill2 hair to a rigid fix");
  ok &= contains(character_notes_c,
                 "nativeafter-glTFMilostatic/movingA/Bproof",
                 "format notes include the native after-glTFMilo A/B proof");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "native_current_after_gltfmilo_20260710/`",
                 "after-glTFMilo native proof directory is recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_current_profile_far.png`,"
                 "`rock1_current_profile_far.png`,and"
                 "`rock2_current_profile_far.png`",
                 "moving-hair native proof images are recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_static_no_charhair_profile_far.png`and"
                 "`rock1_static_no_charhair_profile_far.png`",
                 "static no-CharHair proof images are recorded");
  ok &= contains(character_notes_c,
                 "GHOGX_DISABLE_CHAR_HAIR=1",
                 "static A/B validation records the native hair-disable gate");
  ok &= contains(character_notes_c,
                 "Rock1remainsvisiblywrongwithstatichair",
                 "static Rock1 hair is not promoted as a fix");
  ok &= contains(character_notes_c,
                 "donotpromotestatichairasaRockabill2fix",
                 "static Rockabill2 hair is not promoted as a fix");
  ok &= contains(character_notes_c,
                 "`rock1_static_alpha_diag_profile_far.log`",
                 "Rock1 texture-alpha diagnostic log is recorded");
  ok &= contains(character_notes_c,
                 "`hair-front1.mesh`,`hair-front2.mesh`,`hair-side*`,"
                 "`hair-sides*`,`hair-top_back*`,and`Hair-lower*`drawas"
                 "legacy`hairRender=1blend=3zwrite=0`",
                 "Rock1 legacy hair material render state remains recorded");
  ok &= contains(character_notes_c,
                 "`Hair-lower.2.mesh`logs`tris=48...opaque=48`",
                 "Rock1 opaque lower-hair alpha evidence is recorded");
  ok &= contains(character_notes_c,
                 "`hair-side2.mesh`logs`tris=56...a<96=0opaque=53`",
                 "Rock1 side-hair alpha evidence is recorded");
  ok &= contains(character_notes_c,
                 "cull/alpha/staticoffsetsareboundedoff",
                 "contract prevents re-promoting Rock1 alpha/cull/static offsets");
  ok &= contains(character_notes_c,
                 "sourcezMode-depth-writeA/Bevidence",
                 "format notes include source zMode depth-write A/B evidence");
  ok &= contains(character_notes_c,
                 "`analysis/ps2_trace_current/hair_consumer_20260710/"
                 "native_zmode_depth_candidate_20260710ai/`",
                 "source zMode depth-write proof directory is recorded");
  ok &= contains(character_notes_c,
                 "`rock1_zmode_depth_f120.png`,"
                 "`rock1_legacy_nozwrite_f120.png`,"
                 "`rock2_zmode_depth_f120.png`,and"
                 "`rock2_legacy_nozwrite_f120.png`",
                 "source zMode depth-write Rock1/Rock2 visual A/B is recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_zmode_depth_profile.png`and"
                 "`rockabill2_legacy_nozwrite_profile.png`",
                 "Rockabill2 signed-off zMode guard images are recorded");
  ok &= contains(character_notes_c,
                 "notaplacementfixforRock1/Rock2",
                 "source zMode depth-write is bounded away from placement fixes");
  ok &= contains(character_notes_c,
                 "`GHOGX_DISABLE_SOURCE_MAT_ZMODE_DEPTH=1`",
                 "legacy zMode depth-write fallback gate is documented");
  ok &= contains(char_renderer_c,
                 "GHOGX_DISABLE_SOURCE_MAT_ZMODE_DEPTH",
                 "renderer exposes source zMode depth-write fallback gate");
  ok &= contains(char_renderer_c,
                 "boolmaterial_depth_write_enabled(constmilo_scene::MatObj*material",
                 "renderer isolates source material depth-write decision");
  ok &= contains(char_renderer_c,
                 "case1://kZModeNormal",
                 "renderer treats source normal zMode as depth-writing");
  ok &= contains(char_renderer_c,
                 "case2://kZModeTransparent",
                 "renderer treats source transparent zMode as non-depth-writing");
  ok &= contains(char_renderer_c,
                 "D3DRS_ZWRITEENABLE,depth_write?TRUE:FALSE",
                 "renderer applies source zMode depth-write decision");
  ok &= contains(char_renderer_c,
                 "\"alphaRef=%luzMode=%utexWrap=%u\\n\"",
                 "mesh-render log keeps zMode/depth-write proof fields");
  ok &= contains(character_notes_c,
                 "glTFMilobuild/testrerunevidence",
                 "format notes include the glTFMilo build/test rerun");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "native_rerun_20260710_gltfmilo_test/`",
                 "fresh native glTFMilo rerun proof directory is recorded");
  ok &= contains(character_notes_c,
                 "`rockabill2_profile_far_rerun.png`,"
                 "`rock1_profile_far_rerun.png`,and"
                 "`rock2_profile_far_rerun.png`",
                 "fresh native rerun proof images are recorded");
  ok &= contains(character_notes_c,
                 "source-previewwireframesremainformatevidenceforrev28"
                 "slot-orderpalettesandper-slotrows",
                 "rerun notes do not promote source previews as final visual proof");
  ok &= contains(character_notes_c,
                 "extraglTFMilo/MiloLibbuild-testpass",
                 "format notes include the extra glTFMilo build-test pass");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "source_preview_gltfmilo_buildtest_20260710a/`",
                 "fresh source-preview directory is recorded");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "source_preview_gltfmilo_weightstats_20260710a/`",
                 "source weight-stat directory is recorded");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "native_gltfmilo_goes_20260710a/`",
                 "native glTFMilo go directory is recorded");
  ok &= contains(character_notes_c,
                 "Rockabill2`hair.mesh`/`hair2.mesh`,Rock2"
                 "`hair-front1.mesh`,andtheRock1/Rock2rowswithtrailing"
                 "emptypaletteentriesalllogslot2/3weightcountsandsumsaszero",
                 "empty palette slots are bounded off for sampled problem hair");
  ok &= contains(character_notes_c,
                 "Rock1current/identity/parentcapturesarepixel-identical",
                 "Rock1 local world probes are bounded off");
  ok &= contains(character_notes_c,
                 "theunresolvedpathistheliveCharHair/controllerdrawconsumer",
                 "notes keep the remaining path source-backed");
  ok &= contains(character_notes_c,
                 "freshglTFMilo/MiloLibproblem-hairsource-preview",
                 "format notes include the fresh problem-hair source preview");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "source_preview_gltfmilo_fresh_problem_hair_20260710b/`",
                 "fresh problem-hair source-preview directory is recorded");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "source_preview_gltfmilo_fresh_problem_hair_20260710c/`",
                 "rebuilt follow-up problem-hair source-preview directory is recorded");
  ok &= contains(character_notes_c,
                 "regeneratedafterrebuildingboth`MiloGLTFUtils.sln`andthelocal"
                 "`MiloPreviewBatch`helper",
                 "fresh source-preview pass records rebuilt source tools");
  ok &= contains(character_notes_c,
                 "notsufficientbythemselvestochangenativeliveCharHair"
                 "placement",
                 "fresh glTFMilo source samples are not promoted as a placement fix");
  ok &= contains(character_notes_c,
                 "glTFMilodirect-worldbuild/testpass",
                 "format notes include the glTFMilo direct-world build/test pass");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "source_preview_gltfmilo_direct_build_20260710i/`",
                 "fresh direct-build source-preview directory is recorded");
  ok &= contains(character_notes_c,
                 "Thebatchproduced60individualPNGsfor`rock1`,`rock2`,"
                 "`rockabill2`,`funk1`,and`grim`",
                 "direct-build source-preview sample count and characters are recorded");
  ok &= contains(character_notes_c,
                 "rev28slot-orderpaletteconsumers,withnorev33"
                 "indexed-bonestream",
                 "direct-build source evidence preserves GH2 PS2 palette mode");
  ok &= contains(character_notes_c,
                 "GHOGX_GLTFMILO_DIRECT_WORLD_SKIN=<mesh-or-material-"
                 "substring>",
                 "direct-world native gate is recorded as diagnostic-only");
  ok &= contains(character_notes_c,
                 "logs`mode=gltfmilo-direct-world-env`,skinswith"
                 "`slot_bind*curr_world`,anddrawswith"
                 "`world=gltfmilo-direct-world-env`",
                 "direct-world native skin/draw equation is documented");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "native_gltfmilo_direct_world_20260710i/`",
                 "fresh direct-world native proof directory is recorded");
  ok &= contains(character_notes_c,
                 "15individualPNGs:currentcalibrationshotsforRock1/Rock2/"
                 "Rockabill2plusgo10direct-world,go11direct-worldplus",
                 "direct-world native profile matrix is recorded");
  ok &= contains(character_notes_c,
                 "go12direct-worldplusthesourceCharHairsim/authored-init/"
                 "rootMat-liveposstack,andgo13direct-worldwith"
                 "`GHOGX_DISABLE_CHAR_HAIR=1`",
                 "direct-world go 12/go 13 matrix is recorded");
  ok &= contains(character_notes_c,
                 "DonotpromotetheglTFMilo-direct-world,rootMat-livepos,"
                 "source-sim,orstatic-no-CharHairgatesasanativefix",
                 "direct-world failed trials remain rejected");
  ok &= contains(character_notes_c,
                 "upstreamcontroller/sourceendpointselectionorattachment"
                 "consumption,notanotherrenderer-sidepreviewequation",
                 "direct-world pass points back to upstream evidence");
  ok &= contains(character_notes_c,
                 "focusedstatic/nativeA/Bandsource-toolproblem-cardpass",
                 "format notes include the focused static/source-tool problem-card pass");
  ok &= contains(character_notes_c,
                 "`analysis/hair_static_ab_20260710j/`",
                 "focused static/native A/B proof directory is recorded");
  ok &= contains(character_notes_c,
                 "`rock1_current_profile.png`,"
                 "`rock1_static_no_charhair_profile.png`",
                 "focused A/B proof records the Rock1 current/static pair");
  ok &= contains(character_notes_c,
                 "`rock2_current_profile.png`,"
                 "`rock2_static_no_charhair_profile.png`",
                 "focused A/B proof records the Rock2 current/static pair");
  ok &= contains(character_notes_c,
                 "`rockabill2_current_profile.png`,and"
                 "`rockabill2_static_no_charhair_profile.png`",
                 "focused A/B proof records the Rockabill2 current/static pair");
  ok &= contains(character_notes_c,
                 "Rock1`1275`CharHair/`630`ps2chain/`189`hairOverride"
                 "rows",
                 "focused A/B logs preserve Rock1 live hair row counts");
  ok &= contains(character_notes_c,
                 "Rock2`1149`/`567`/`63`,andRockabill2`449`/`189`/`63`",
                 "focused A/B logs preserve Rock2 and Rockabill2 row counts");
  ok &= contains(character_notes_c,
                 "staticrunsrecordzeroCharHair/ps2chain/hairOverride",
                 "focused A/B logs preserve disabled CharHair proof");
  ok &= contains(character_notes_c,
                 "Static/no-CharHairisnotafix",
                 "static diagnostic is explicitly rejected as a fix");
  ok &= contains(character_notes_c,
                 "`analysis/ihatecompvir_milo_samples/"
                 "source_preview_gltfmilo_focused_problem_cards_20260710j/`",
                 "focused glTFMilo problem-card source-preview directory is recorded");
  ok &= contains(character_notes_c,
                 "with`105`PNGsand`45``*problem*.png`focusedviews",
                 "focused glTFMilo source-preview image counts are recorded");
  ok &= contains(character_notes_c,
                 "Thefocusedsource"
                 "samplespreserverev28slot-orderevidenceforRock1"
                 "`hair-front1.mesh`",
                 "focused glTFMilo source-preview preserves Rock1 slot-order evidence");
  ok &= contains(character_notes_c,
                 "Rockabill2`hair.mesh`/`hair2.mesh`,Funk1"
                 "`funk1.37.mesh`,andGrimaccessorymeshes",
                 "focused glTFMilo source-preview preserves problem accessory mesh list");
  ok &= contains(character_notes_c,
                 "formatevidenceonly,notadirectplayer-viewparityreference",
                 "focused source renders are bounded as format evidence");
  ok &= contains(character_notes_c,
                 "no-focusRock1PCSX2hairwriter-callsitetrace",
                 "format notes include the no-focus Rock1 writer-callsite trace");
  ok &= contains(character_notes_c,
                 "`analysis/ps2_trace_current/hair_consumer_20260710/"
                 "rock1_hair_writer_callsite_full_nofocus.json`",
                 "Rock1 writer-callsite trace artifact is recorded");
  ok &= contains(character_notes_c,
                 "records2460callswithoutfocusforcingorinputinjection",
                 "Rock1 writer-callsite trace preserves no-focus/no-input proof");
  ok &= contains(character_notes_c,
                 "`a0`equalsthepoint's`s0+0x48`writertargetand`a1`"
                 "equals`sp`",
                 "Rock1 writer-callsite trace records the Trans writer payload");
  ok &= contains(character_notes_c,
                 "stackrow1isthenormalizedvectorfromsubmitted/rootposition"
                 "`sp+0x30`tosolvedpoint`s0+0x00`",
                 "Rock1 writer-callsite trace proves the PS2 aim-axis contract");
  ok &= contains(character_notes_c,
                 "notasauthorizationforaninventedrollfix",
                 "Rock1 writer-callsite trace does not promote an unsupported roll fix");
  ok &= contains(character_notes_c,
                 "current-ELFRock1no-focusPCSX2vector/writertraces",
                 "format notes include the current-ELF Rock1 vector/writer traces");
  ok &= contains(character_notes_c,
                 "rock1_hair_pre_writer_current_nofocus.json",
                 "current-ELF Rock1 pre-writer trace artifact is recorded");
  ok &= contains(character_notes_c,
                 "rock1_hair_store_writer_pair_current_nofocus.json",
                 "current-ELF Rock1 paired store/writer trace artifact is recorded");
  ok &= contains(character_notes_c,
                 "theloadedELFbytesforeachtracepass",
                 "current-ELF trace notes require loaded ELF address selection");
  ok &= contains(character_notes_c,
                 "records2590calls,10uniquepoint-stateobjects,`a0=="
                 "s0+0x48`,and`a1==sp`",
                 "current-ELF pre-writer trace records writer inputs");
  ok &= contains(character_notes_c,
                 "row1matchesthenormalizedvectorfromsubmitted/rootposition"
                 "`sp+0x30`tosolvedpoint`s0+0x00`withdot`0.998..1.000`",
                 "current-ELF pre-writer trace proves row1 aim");
  ok &= contains(character_notes_c,
                 "row0androw2cancarrymatchingnon-unitscalebeforethewriter"
                 "call",
                 "current-ELF paired trace records row0/row2 scale");
  ok &= contains(character_notes_c,
                 "recordedonlyonecallinthispass,sokeepitassparse/"
                 "perturbingevidence",
                 "current-ELF sparse writer-callsite run is bounded");
  ok &= contains(character_notes_c,
                 "doesnotauthorizeanativehairchangeuntilthesourcerowand"
                 "endpointselectionarematchedagainstacurrentnativecapture",
                 "current-ELF trace does not authorize a premature native fix");
  ok &= contains(character_notes_c,
                 "currentRock1nativecomparator",
                 "format notes include the current Rock1 native comparator");
  ok &= contains(character_notes_c,
                 "native`charhair-ps2chain`rowsforRock1submitunit-length"
                 "row0/1/2foreverysampledchainpoint",
                 "current native comparator records unit submitted rows");
  ok &= contains(character_notes_c,
                 "currentPS2pre-writertraceskeeprow1unitandallowrow0/2"
                 "tocarrynon-unitscale",
                 "current PS2 traces are contrasted against native unit rows");
  ok &= contains(character_notes_c,
                 "rock1_hair_pre_writer_current_s4_nofocus.json",
                 "current-ELF s4 trace artifact is recorded");
  ok &= contains(character_notes_c,
                 "twolivehairgroupsusedistinct`s4`rows",
                 "current-ELF s4 trace records group-row separation");
  ok &= contains(character_notes_c,
                 "donotexplaintheper-pointrow0/row2scale",
                 "current-ELF s4 constants are bounded off as a full scale formula");
  ok &= contains(character_notes_c,
                 "notasafehard-codedscalefactor",
                 "current native comparator rejects a hard-coded scale fix");
  ok &= contains(milo_preview_batch_c,
                 "\"skin_bind\"=>SkinBindPoint(mesh,v,storedWorld)",
                 "MiloLib visual helper renders the bind-skin equation");
  ok &= contains(milo_preview_batch_c,
                 "Transform(slot.transform,newVec3(v.x,v.y,v.z))",
                 "MiloLib visual helper consumes decoded palette slot rows");
  ok &= contains(milo_preview_batch_c,
                 "if(HasAny(name,\"hair\")||HasAny(material,\"hair\"))"
                 "return\"hair\";",
                 "MiloLib visual helper keeps material-named hair visible");
  ok &= contains(milo_preview_batch_c,
                 "varindexedBones=mesh.revision>=33;",
                 "MiloLib visual helper gates explicit bone indices by revision");
  ok &= contains(milo_preview_batch_c,
                 "varboneIndex=indexedBones?bones[i]:i;",
                 "MiloLib visual helper uses ordered palette slots for GH2 PS2");
  ok &= contains(milo_preview_batch_c,
                 "rev={mesh.Mesh.revision}indexedBones={(mesh.Mesh.revision"
                 ">=33?1:0)}",
                 "MiloLib visual helper logs mesh revision and index mode");
  ok &= contains(milo_preview_batch_c,
                 "showing={(mesh.Mesh.draw.showing?1:0)}"
                 "drawOrder={F(mesh.Mesh.draw.drawOrder)}",
                 "MiloLib visual helper logs source Draw base state");
  ok &= contains(milo_preview_batch_c,
                 "recordGroupSourceSample(stringCharacter,stringName,"
                 "RndGroupGroup);",
                 "MiloLib visual helper records source RndGroup objects");
  ok &= contains(milo_preview_batch_c,
                 "entry.objisRndGroupgroup",
                 "MiloLib visual helper consumes source RndGroup rows");
  ok &= contains(milo_preview_batch_c,
                 "\"[source-group]character={character}name=\\\""
                 "{sample.Name}\\\"\"",
                 "MiloLib visual helper logs source group rows");
  ok &= contains(milo_preview_batch_c,
                 "string.Join(\",\",group.objects.Select(o=>o.value))",
                 "MiloLib visual helper logs source group children");
  ok &= contains(milo_preview_batch_c,
                 "staticstringWeightSummary(RndMeshmesh)",
                 "MiloLib visual helper logs per-slot weight statistics");
  ok &= contains(milo_preview_batch_c,
                 "weights=[{WeightSummary(mesh.Mesh)}]",
                 "MiloLib visual helper records palette slot usage");
  ok &= contains(milo_preview_batch_c,
                 "HasAny(name,\"wing\",\"glass\",\"hour-glass\")",
                 "MiloLib visual helper samples Grim visible accessories");
  ok &= contains(milo_preview_batch_c,
                 "HasAny(material,\"wing\",\"cloth\",\"cape\",\"hood\")",
                 "MiloLib visual helper samples material-named cloth/accessories");
  ok &= contains(milo_preview_batch_c,
                 "isGrim&&(HasAny(name,\"shadow\")||HasAny(material,"
                 "\"shadow\",\"grim_head\"))",
                 "MiloLib visual helper keeps Grim-specific accessory expansion scoped");
  ok &= contains(milo_preview_batch_c,
                 "storedWorld[entry.name.value]=trans.worldXfm;",
                 "MiloLib visual helper keys controller rows by stored world");
  ok &= contains(milo_preview_batch_c,
                 "staticboolIsFocusedProblemSample(MeshSamplemesh)",
                 "MiloLib visual helper exposes focused problem-card filtering");
  ok &= contains(milo_preview_batch_c,
                 "returnnameis\"hair-front1.mesh\"or\"hair-front2.mesh\"",
                 "MiloLib visual helper focuses Rock1 front hair cards");
  ok &= contains(milo_preview_batch_c,
                 "returnnameis\"hair-front1.mesh\"or\"hair-top.mesh\"",
                 "MiloLib visual helper focuses Rock2 hair cards");
  ok &= contains(milo_preview_batch_c,
                 "returnnameis\"hair.mesh\"or\"hair2.mesh\"",
                 "MiloLib visual helper focuses Rockabill2 hair cards");
  ok &= contains(milo_preview_batch_c,
                 "returnname==\"funk1.37.mesh\"||name.Contains(\"hair\")",
                 "MiloLib visual helper keeps Funk1 hair in focused samples");
  ok &= contains(milo_preview_batch_c,
                 "Render(Path.Combine(outputDir,$\"{character}_problem_"
                 "{mode}_side.bmp\"),focused,mode,\"side_zy\","
                 "\"[render-focus]\");",
                 "MiloLib visual helper writes individual focused side views");
  ok &= contains(character_notes_c,
                 "Rock1/Rockabill2relation-rowfollow-up",
                 "format notes retain the current no-focus relation-row evidence");
  ok &= contains(character_notes_c,
                 "evidenceagainstaRock1material/cull/static-offsetfix",
                 "Rock1 relation rows are not promoted as a visual band-aid");
  ok &= contains(character_notes_c,
                 "doesnotyetprovethefinalnativecard-consumerequation",
                 "Rockabill2 remains open until the card consumer equation is traced");
  ok &= contains(character_notes_c,
                 "Rock1liveobject/paletteownerscan",
                 "format notes include the no-focus Rock1 live object scan");
  ok &= contains(character_notes_c,
                 "maps`hair-front1.mesh`string`0x00eadd44`toliveobjectrow"
                 "`0x007d5250`",
                 "Rock1 hair-front1 object identity is derived from the PS2 object directory");
  ok &= contains(character_notes_c,
                 "ownspalettepairblock`0x00f64350`",
                 "Rock1 front hair records the PS2 palette-owner block");
  ok &= contains(character_notes_c,
                 "frontsheetsarenormalroot-parentweightedcardsconsumingpalette"
                 "controllerrows",
                 "Rock1 front hair remains a weighted-card consumer problem");
  ok &= contains(character_notes_c,
                 "notaper-characterpositionaloffset",
                 "Rock1 hair cannot be converted to a manual offset fix");
  ok &= contains(character_notes_c,
                 "Rockabill2liveobject/paletteownerscan",
                 "format notes include the no-focus Rockabill2 live object scan");
  ok &= contains(character_notes_c,
                 "real`hair.mesh`string`0x00eadc0c`toliveobjectrow"
                 "`0x007d1600`",
                 "Rockabill2 hair.mesh object identity is derived from the PS2 object directory");
  ok &= contains(character_notes_c,
                 "`bone_hair.mesh`mapstoobjectrow`0x00eb2f30`,whiletheolder"
                 "relationrowaddress`0x00eb2e70`isthecontroller/liverow",
                 "Rockabill2 separates object identity from the moving bone_hair row");
  ok &= contains(character_notes_c,
                 "`hair.mesh`ownspalettepairblock`0x00f0eaa0`",
                 "Rockabill2 hair.mesh records the PS2 palette-owner block");
  ok &= contains(character_notes_c,
                 "two-controllerhead-localweighted-cardcase",
                 "Rockabill2 hair remains a head-local weighted-card consumer problem");
  ok &= contains(character_notes_c,
                 "notastaticmeshoffset",
                 "Rockabill2 hair cannot be converted to a static offset fix");
  ok &= contains(character_notes_c,
                 "Rock1/Rockabill2livepalettematrixdumps",
                 "format notes include the no-focus live palette matrix dumps");
  ok &= contains(character_notes_c,
                 "`0x00f64380`,`0x00f643c0`,`0x00f64400`,and`0x00f64440`",
                 "Rock1 live palette dump records all front1 slot-bind rows");
  ok &= contains(character_notes_c,
                 "`0x00f0ead0`and`0x00f0eb10`",
                 "Rockabill2 hair.mesh live palette dump records active slot binds");
  ok &= contains(character_notes_c,
                 "`0x00ef5890`and`0x00ef58d0`",
                 "Rockabill2 hair 2.mesh live palette dump records active slot binds");
  ok &= contains(character_notes_c,
                 "matchthenative`mesh.bind[i]`rows",
                 "live PS2 palette rows are tied to decoded native bind rows");
  ok &= contains(character_notes_c,
                 "`slot_bind*curr_world*inverse(mesh_world)`",
                 "format notes document the evidence-backed local hair equation");
  ok &= contains(char_renderer_c,
                 "skin[i]=mul16(xfm16(mesh.bind[i]),curr_world);",
                 "local hair cards now use the same MiloLib BoneTransform consumer as other meshes");
  ok &= contains(character_notes_c,
                 "Rock1nativeroot-parentdiagnosticafterthepalettematrixdump",
                 "Rock1 root-parent cards remain separately documented");
  ok &= contains(character_notes_c,
                 "Bothcardsrunas`mode=mesh-local-bind`and"
                 "`world=identity-skinned`",
                 "Rock1 current native path is not the local-attachment path");
  ok &= contains(character_notes_c,
                 "alreadyconsumesdecodedslotbindrowsplusliveCharHair"
                 "controllerrows",
                 "Rock1 native diagnostic prevents a blind transform patch");
  ok &= contains(character_notes_c,
                 "DonotpromoteaRock1transformoroffsetpatch",
                 "Rock1 remains open pending draw-consumer evidence");
  ok &= lacks(char_renderer_c,
              "reverse_skin_weight_slots_enabled",
              "renderer no longer reverses slot order for named mesh classes");
  ok &= contains(character_notes_c,
                 "Rock1PSMeshrecord-beforerenderpackettrace",
                 "Rock1 PSMesh record-before evidence is documented");
  ok &= contains(character_notes_c,
                 "`record_timing:before_original_words`",
                 "Rock1 PSMesh trace records before executing original words");
  ok &= contains(character_notes_c,
                 "records`4881`totalcalls(`26`at`0x1c87f0`,`4070`"
                 "retainedat`0x1c8c70`inthering)",
                 "Rock1 PSMesh trace call counts are preserved");
  ok &= contains(character_notes_c,
                 "twolow-memorypalette/typevaluesalsoseenintheRock1"
                 "tripletsabove",
                 "Rock1 PSMesh packet sample records palette/type values");
  ok &= contains(character_notes_c,
                 "`0x003e50c8`atpacketoffset`+0x48`and`0x003e3b70`"
                 "at`+0xec`",
                 "Rock1 PSMesh packet sample records palette/type offsets");
  ok &= contains(character_notes_c,
                 "pcsx2_rock1_psmesh_a0_snapshot_calltime_20260705",
                 "Rock1 PSMesh call-time a0 snapshot trace is documented");
  ok &= contains(character_notes_c,
                 "records`5074`total`0x001c8c70`calls,retains`1024`"
                 "ringentries,andstores`128`wordsfrom`a0`",
                 "Rock1 PSMesh call-time snapshot count and width are preserved");
  ok &= contains(character_notes_c,
                 "everyretainedcallhas`0x003e50c8`at`+0x48`and"
                 "`0x003e3b70`at`+0xec`",
                 "Rock1 PSMesh call-time snapshot proves packet row offsets");
  ok &= contains(character_notes_c,
                 "earlier`+0x1d8`observationwasapost-call"
                 "memory-neighborartifact",
                 "Rock1 PSMesh call-time snapshot rejects neighbor-packet evidence");
  ok &= contains(character_notes_c,
                 "zeromatchesfor`0x003e50c8`at`+0x1d8`",
                 "Rock1 PSMesh call-time snapshot records the rejected offset");
  ok &= contains(character_notes_c,
                 "matchthefirstfieldoftheliveRock1palettetriplets,"
                 "nottheactualmeshobjectrow`0x007d5250`",
                 "Rock1 PSMesh trace distinguishes palette type from mesh object");
  ok &= contains(character_notes_c,
                 "thepacketsnapshotdoesnotcarrythecontrollerrowpointers",
                 "Rock1 PSMesh trace is not promoted as a new transform equation");
  ok &= contains(character_notes_c,
                 "pcsx2_rockabill2_psmesh_a0_snapshot_highmem_20260705",
                 "Rockabill2 PSMesh high-memory cross-check is documented");
  ok &= contains(character_notes_c,
                 "uses`data_base=0x01e00000`sothetracebufferisnot"
                 "overwritten",
                 "Rockabill2 PSMesh cross-check records high-memory trace buffer");
  ok &= contains(character_notes_c,
                 "zerooccurrencesofRockabill2'sknownhairobject/controller"
                 "rows(`0x007d1600`,`0x007c3f00`,`0x00eb3670`,`0x00eb2e70`)",
                 "Rockabill2 PSMesh cross-check rejects this hook as hair row proof");
  ok &= contains(character_notes_c,
                 "`0x003e50c8`at`+0x48`,`0x003e3b70`at`+0xec`,and"
                 "`0x003e6e30`at`+0xf8`",
                 "Rockabill2 PSMesh cross-check records shared packet type values");
  ok &= contains(character_notes_c,
                 "notahair-carddraw-consumerproof",
                 "PSMesh hook is not promoted as a hair-card consumer proof");
  ok &= contains(character_notes_c,
                 "mesh/controllerrowsneededtoproveadifferentnative"
                 "transformequationoranystaticcardoffsetforRock1or"
                 "Rockabill2",
                 "Rock1 PSMesh boundary rejects an unsupported offset fix");
  ok &= contains(character_notes_c,
                 "Rockabill2mesh-vtableconsumercandidatetraces",
                 "Rockabill2 mesh-vtable candidate traces are documented");
  ok &= contains(character_notes_c,
                 "`mesh_vtable_1c8cb0=0x001c8cb0`",
                 "Rockabill2 mesh-vtable 0x001c8cb0 trace target is preserved");
  ok &= contains(character_notes_c,
                 "records`140`calls",
                 "Rockabill2 0x001c8cb0 trace call count is preserved");
  ok &= contains(character_notes_c,
                 "`56`sampled`a1`objectrowswiththesamelayoutmarkers",
                 "Rockabill2 0x001c8cb0 sampled object rows are documented");
  ok &= contains(character_notes_c,
                 "`+0x34=0x003e6e48`,`+0x48=0x003e50c8`,"
                 "`+0xec=0x003e3b70`,and`+0x160=0x003e6d98`",
                 "Rockabill2 0x001c8cb0 layout markers are preserved");
  ok &= contains(character_notes_c,
                 "`sp48_word`stringsamplesare`hand_L-clap.mesh`,"
                 "`hand_R-clap.mesh`,`hand_L-devil.mesh`,"
                 "`hand_R-devil.mesh`,`hand_L-fist.mesh`,"
                 "`hand_R-fist.mesh`,and`hand_R-lighter.mesh`",
                 "Rockabill2 0x001c8cb0 shows hand/showing object commands");
  ok &= contains(character_notes_c,
                 "zerodirectregisterhitsandzerosampled-wordhitsfor"
                 "Rockabill2'sknownhairobject/controller/paletterows",
                 "Rockabill2 0x001c8cb0 is not promoted as hair row proof");
  ok &= contains(character_notes_c,
                 "notevidenceforastaticoffsetoralternatetransformequation",
                 "Rockabill2 0x001c8cb0 rejects unsupported offset fixes");
  ok &= contains(character_notes_c,
                 "`mesh_vtable_1ca2d0=0x001ca2d0`",
                 "Rockabill2 mesh-vtable 0x001ca2d0 trace target is preserved");
  ok &= contains(character_notes_c,
                 "75-secondenabledwindowrecords`0`calls",
                 "Rockabill2 0x001ca2d0 zero-call trace is preserved");
  ok &= contains(character_notes_c,
                 "inactivecandidateinthiscapturedstate,notasnegativeproof"
                 "aboutthewholerenderer",
                 "Rockabill2 0x001ca2d0 negative trace remains bounded");
  ok &= contains(character_notes_c,
                 "Earlier`0x001c8830`attemptsproducedhook/writeback"
                 "instabilityoralate-livezero-callwindow",
                 "Rockabill2 0x001c8830 trace instability is not overclaimed");
  ok &= contains(character_notes_c,
                 "Rockabill2packet-tablesecond-clustertraces",
                 "Rockabill2 packet-table second-cluster traces are documented");
  ok &= contains(character_notes_c,
                 "`mesh_vtable_1c64d0=0x001c64d0`",
                 "Rockabill2 packet-table hot 0x001c64d0 target is preserved");
  ok &= contains(character_notes_c,
                 "records`1241990`totalcalls,withtheretainedringfully"
                 "occupiedby`mesh_vtable_1c64d0`",
                 "Rockabill2 0x001c64d0 hot-call evidence is preserved");
  ok &= contains(character_notes_c,
                 "`+0x34=0x003e6e48`,`+0x48=0x003e50c8`,"
                 "`+0xec=0x003e3b70`,`+0xf8=0x003e6e30`,and"
                 "`+0x160=0x003e6d98`",
                 "Rockabill2 0x001c64d0 retained object layout markers are preserved");
  ok &= contains(character_notes_c,
                 "omitsthehot`0x001c64d0`methodandaddstheneighboring"
                 "tableentries",
                 "Rockabill2 quiet-cluster trace purpose is documented");
  ok &= contains(character_notes_c,
                 "patchescleanlyandrecords`0`callsoverthesame75-second"
                 "enabledwindow",
                 "Rockabill2 quiet-cluster zero-call evidence is preserved");
  ok &= contains(character_notes_c,
                 "Rockabill2hot`0x001c64d0`a0-filtertrace",
                 "Rockabill2 filtered 0x001c64d0 trace is documented");
  ok &= contains(character_notes_c,
                 "prepatch_ps2_trace_elf_a0_filter.py",
                 "Rockabill2 filtered trace helper is named");
  ok &= contains(character_notes_c,
                 "recordsonlycallswhere`a0`equalstheknownRockabill2"
                 "`hair.mesh`objectrow`0x007d1600`or`hair2.mesh`"
                 "objectrow`0x007c3f00`",
                 "Rockabill2 filtered trace records the exact a0 filters");
  ok &= contains(character_notes_c,
                 "The75-secondenabledtracerecords`0`filteredcalls",
                 "Rockabill2 filtered 0x001c64d0 zero-call evidence is preserved");
  ok &= contains(character_notes_c,
                 "doesnotdirectlyconsumethetwoknownRockabill2haircard"
                 "objectrowsas`a0`",
                 "Rockabill2 0x001c64d0 is not promoted as the hair-card consumer");
  ok &= contains(character_notes_c,
                 "Keeplookingupstream/downstreamfortheactualhair-carddraw"
                 "orskinconsumer",
                 "Rockabill2 hair consumer remains open after 0x001c64d0");
  ok &= contains(character_notes_c,
                 "Rockabill2per-meshargument-widefilteredtraces",
                 "Rockabill2 per-mesh argument-wide filtered traces are documented");
  ok &= contains(character_notes_c,
                 "nowsupportsfilteringanyof`a0`/`a1`/`a2`/`a3`",
                 "Rockabill2 argument-wide filter helper behavior is documented");
  ok &= contains(character_notes_c,
                 "includedbroad`0x0034...`/`0x003c...`vtableentriesand"
                 "neverreachedtheRockabill2-specific`hair2.mesh`,"
                 "`bone_hair.mesh`,or`bone_head.mesh`terms",
                 "Rockabill2 broad generic-entry batch is not treated as evidence");
  ok &= contains(character_notes_c,
                 "validper-meshbatchlimitsthehooksto`0x0019ddc8`,"
                 "`0x001c8830`,`0x001c8cb0`,`0x001ca2d0`,"
                 "`0x001c7c98`,`0x0019dc78`,`0x001c63d8`,"
                 "`0x001c64d0`,`0x001c5e00`,`0x001c6298`,"
                 "`0x001b5be0`,and`0x0019e530`",
                 "Rockabill2 per-mesh filtered target set is preserved");
  ok &= contains(character_notes_c,
                 "staywithinthesafestubrangebeforethenearbynonzero"
                 "table/dataregion",
                 "Rockabill2 filtered traces preserve code-cave bounds");
  ok &= contains(character_notes_c,
                 "all-argumentfilterfor`hair.mesh`objectrow`0x007d1600`"
                 "and`hair2.mesh`objectrow`0x007c3f00`records`0`calls",
                 "Rockabill2 per-mesh object-row filter evidence is preserved");
  ok &= contains(character_notes_c,
                 "all-argumentfilterforlive`bone_head`row`0x00eb3670`"
                 "andlive`bone_hair`row`0x00eb2e70`alsorecords`0`calls",
                 "Rockabill2 per-mesh controller-row filter evidence is preserved");
  ok &= contains(character_notes_c,
                 "all-argumentfilterforhairpaletteblocks`0x00f0eaa0`"
                 "and`0x00ef5860`alsorecords`0`calls",
                 "Rockabill2 per-mesh palette-block filter evidence is preserved");
  ok &= contains(character_notes_c,
                 "widerfive-valuecontroller/palettepass"
                 "in`pcsx2_rockabill2_mesh_args_hairrows_permesh_20260705/`"
                 "didnotreachtheRockabill2-specificterms",
                 "Rockabill2 wide hair-row pass is bounded as invalid evidence");
  ok &= contains(character_notes_c,
                 "donotdirectlyreceivetheknownRockabill2haircardobjectrows,"
                 "livecontrollerrows,orpaletteblocksinargumentregisters",
                 "Rockabill2 per-mesh filtered traces are not promoted as consumer proof");
  ok &= contains(character_notes_c,
                 "realconsumerislikelyanindirectlist/table/packetpath"
                 "outsidethesedirectargumentfilters",
                 "Rockabill2 next search direction stays evidence-bounded");
  ok &= contains(character_notes_c,
                 "Rockabill2name-pointerfieldfilters",
                 "Rockabill2 name-pointer field-filter traces are documented");
  ok &= contains(character_notes_c,
                 "helpernowsupports`--field-offset`",
                 "Rockabill2 field-filter helper capability is documented");
  ok &= contains(character_notes_c,
                 "`a0+0x174`or`a1+0x174`equalsthelivenamepointersfor"
                 "Rockabill2`hair.mesh`(`0x00eadc0c`),`hair2.mesh`"
                 "(`0x00ead6f8`),or`lower-teeth.mesh`(`0x00eadd7f`)",
                 "Rockabill2 name-pointer filter values are preserved");
  ok &= contains(character_notes_c,
                 "`pcsx2_rockabill2_mesh_nameptr_a0_20260705/`and"
                 "`pcsx2_rockabill2_mesh_nameptr_a1_20260705/`patched"
                 "alltwelveprologues",
                 "Rockabill2 a0/a1 name-pointer trace directories are preserved");
  ok &= contains(character_notes_c,
                 "recorded`0`calls",
                 "Rockabill2 name-pointer traces preserve zero-call evidence");
  ok &= contains(character_notes_c,
                 "donotreceiveclonedRockabill2hair/teethrowsas`a0`or`a1`"
                 "throughtheobserved`+0x174`name-pointerlayout",
                 "Rockabill2 name-pointer traces are not overclaimed");
  ok &= contains(character_notes_c,
                 "Keeptracingtheupstreamlist/table/packetowneroradifferent"
                 "consumerbeforechangingthenativeRockabill2hair/teethskin"
                 "equation",
                 "Rockabill2 native skin equation remains blocked on consumer proof");
  ok &= contains(character_notes_c,
                 "Rockabill2palette-typeowneraudit",
                 "Rockabill2 palette-type owner audit is documented");
  ok &= contains(character_notes_c,
                 "rockabill2_palette_type_trace_owner_audit_20260705.json",
                 "Rockabill2 palette-type owner audit artifact is preserved");
  ok &= contains(character_notes_c,
                 "`palette_type_3319a8`,`palette_type_3237b0`,"
                 "and`palette_type_1d29a0`traces",
                 "Rockabill2 palette-type trace names are preserved");
  ok &= contains(character_notes_c,
                 "`hair.mesh`object`0x007d1600`,`hair2.mesh`object"
                 "`0x007c3f00`,palette-pairblocks`0x00f0eaa0`/"
                 "`0x00ef5860`,andlive`bone_head`/`bone_hair`rows"
                 "`0x00eb3670`/`0x00eb2e70`arethesource-backedhair-card"
                 "inputs",
                 "Rockabill2 positive owner rows stay separated from negative traces");
  ok &= contains(character_notes_c,
                 "`palette_type_3319a8`records`23`calls,"
                 "`palette_type_3237b0`records`7`calls,and"
                 "`palette_type_1d29a0`records`950`calls",
                 "Rockabill2 palette-type call counts are preserved");
  ok &= contains(character_notes_c,
                 "snapshotscontain`0`hitsfortheRockabill2hairobjectrows,"
                 "palette-pairblocks,livecontrollerrows,ornamepointers",
                 "Rockabill2 palette-type audit preserves zero-hit evidence");
  ok &= contains(character_notes_c,
                 "world/small1/og/textures/sign_flaming_shot_glow_decal.bmp",
                 "Rockabill2 palette-type audit records venue/effect row evidence");
  ok &= contains(character_notes_c,
                 "Treatthesehooksasbounded-offlist/effect/packetpaths,"
                 "notasthevisiblehair-cardskinningconsumer",
                 "Rockabill2 palette-type hooks are not promoted as hair-card consumer proof");
  ok &= contains(character_notes_c,
                 "upstreamowner/list/tablepatharoundrefs`0x007da158`"
                 "and`0x007d8968`",
                 "Rockabill2 next trace direction remains owner-table focused");
  ok &= contains(character_notes_c,
                 "Rockabill2owner-tableneighborhoodaudit",
                 "Rockabill2 owner-table neighborhood audit is documented");
  ok &= contains(character_notes_c,
                 "rockabill2_owner_table_neighborhood_audit_20260705.json",
                 "Rockabill2 owner-table neighborhood audit artifact is preserved");
  ok &= contains(character_notes_c,
                 "`0x00f0eaa0`startswith`0x003e50c8`,then"
                 "`hair.mesh`object`0x007d1600`pluslive`bone_head`row"
                 "`0x00eb3670`",
                 "Rockabill2 hair.mesh palette-pair first controller row is preserved");
  ok &= contains(character_notes_c,
                 "thenextentrystartsat`0x00f0eaac`with`0x003e3b70`,"
                 "repeats`0x007d1600`,andpointsatlive`bone_hair`row"
                 "`0x00eb2e70`",
                 "Rockabill2 hair.mesh palette-pair second controller row is preserved");
  ok &= contains(character_notes_c,
                 "`hair2.mesh`blockat`0x00ef5860`mirrorsthesamelayout"
                 "withobject`0x007c3f00`",
                 "Rockabill2 hair 2.mesh palette-pair layout is preserved");
  ok &= contains(character_notes_c,
                 "`0x007da158`pointsat`0x00f0eaa0`,whileneighboring"
                 "rowsinclude`0x007da160`,`0x00eb2f50`,and`0x00f0eaac`",
                 "Rockabill2 hair.mesh owner-table neighborhood is preserved");
  ok &= contains(character_notes_c,
                 "`0x007d8968`pointsat`0x00ef5860`,withneighboringrows"
                 "`0x007d8980`,`0x007da160`,and`0x00ef586c`",
                 "Rockabill2 hair 2.mesh owner-table neighborhood is preserved");
  ok &= contains(character_notes_c,
                 "aimthenextpre-calltraceattheowner/listwalkeror"
                 "object-directoryresolver",
                 "Rockabill2 next hook target stays source-backed");
  ok &= contains(character_notes_c,
                 "Donottreattheowner-tablepointeritselfasafinalskin"
                 "ningequation",
                 "Rockabill2 owner-table rows are not overclaimed as a fix");
  ok &= contains(character_notes_c,
                 "Rockabill2owner-nodestaticxrefsandsingle-hooktraceaudit",
                 "Rockabill2 owner-node single-hook trace audit is documented");
  ok &= contains(character_notes_c,
                 "rockabill2_owner_type_static_xrefs_20260705.json",
                 "Rockabill2 owner-node static xref artifact is preserved");
  ok &= contains(character_notes_c,
                 "`0x003e50c8`has38constructedrefs,`0x003e3b70`has85,"
                 "`0x003e82e8`has2,and`0x003e7eb0`has35",
                 "Rockabill2 owner-node static xref counts are preserved");
  ok &= contains(character_notes_c,
                 "rockabill2_owner_node_single_trace_audit_20260705.json",
                 "Rockabill2 owner-node single-hook audit artifact is preserved");
  ok &= contains(character_notes_c,
                 "zeroedoriginalELFstubwindowat`0x003df000`",
                 "Rockabill2 clean single-hook traces require a zeroed stub window");
  ok &= contains(character_notes_c,
                 "filtered`a0`/`a1`/`a2`/`a3`for`0x007da158`,"
                 "`0x007d8968`,`0x00f0eaa0`,and`0x00ef5860`",
                 "Rockabill2 owner-node trace filter values are preserved");
  ok &= contains(character_notes_c,
                 "recorded`0`callsfor`owner82_1c712c`,`owner7e_1bd688`,"
                 "`owner7e_1bfa0c`,and`owner7e_1c70ac`",
                 "Rockabill2 clean single-hook zero-call set is preserved");
  ok &= contains(character_notes_c,
                 "`owner82_3d3ae0`isweakevidenceonly",
                 "Rockabill2 weak owner82 trace is not overclaimed");
  ok &= contains(character_notes_c,
                 "oneusednon-slackstubbytesat`0x00370000`,andtwodidnot"
                 "reachliveRockabill2/hairterms",
                 "Rockabill2 rejected multi-hook traces stay rejected");
  ok &= contains(character_notes_c,
                 "DonotchangenativeRockabill2hair,teeth,oreyeskinning"
                 "fromthisevidence",
                 "Rockabill2 owner-node traces are not promoted as a fix");
  ok &= contains(character_notes_c,
                 "Rockabill2highowner/list-copysingle-hookaudit",
                 "Rockabill2 high owner/list-copy single-hook audit is documented");
  ok &= contains(character_notes_c,
                 "rockabill2_owner_listcopy_single_trace_audit_20260705.json",
                 "Rockabill2 high owner/list-copy audit artifact is preserved");
  ok &= contains(character_notes_c,
                 "Eachtargetusedtheverifiedclean`0x003df000`stubwindow,"
                 "patchedonlyonesite",
                 "Rockabill2 high owner/list-copy traces keep single-hook method");
  ok &= contains(character_notes_c,
                 "All15highrefsreachedtheliveterms,kept"
                 "`enable_word_after_trace=0x00000001`,andrecorded`0`"
                 "filteredcalls",
                 "Rockabill2 high owner/list-copy clean negative count is preserved");
  ok &= contains(character_notes_c,
                 "`owner7e_3b8060`,`owner7e_3b80dc`,`owner7e_3b8214`,"
                 "`owner7e_3b826c`",
                 "Rockabill2 high owner/list-copy first target group is preserved");
  ok &= contains(character_notes_c,
                 "`owner7e_3b8408`,`owner7e_3b848c`,`owner7e_3b84b0`,"
                 "`owner7e_3b8518`",
                 "Rockabill2 high owner/list-copy second target group is preserved");
  ok &= contains(character_notes_c,
                 "`owner7e_3b8960`,`owner7e_3b95a0`,`owner7e_3b97d0`,"
                 "`owner7e_3b9d6c`",
                 "Rockabill2 high owner/list-copy third target group is preserved");
  ok &= contains(character_notes_c,
                 "`owner7e_3bec88`,`owner7e_3bf020`,and`owner7e_3bf384`",
                 "Rockabill2 high owner/list-copy final target group is preserved");
  ok &= contains(character_notes_c,
                 "Thisboundsofftheobservedhighowner/list-copyrefsasdirect"
                 "runtimeconsumersoftheknownRockabill2owner/palettepointers",
                 "Rockabill2 high owner/list-copy interpretation is preserved");
  ok &= contains(character_notes_c,
                 "Itisstillnegativeevidence,notaskinningequation",
                 "Rockabill2 high owner/list-copy traces are not promoted as skinning");

  if (!ok) {
    std::cerr
        << "The hair path must stay on decoded CharHair/PS2 point-state "
           "evidence. Do not replace it with hidden meshes, per-character "
           "offsets, or default promotion of rejected single-point probes.\n";
    return 1;
  }
  return 0;
}

