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

  const std::string char_mesh = compact(read_file(char_dir / "char_mesh.cpp"));
  const std::string char_mesh_h = compact(read_file(char_dir / "char_mesh.h"));
  const std::string char_clip = compact(read_file(char_dir / "char_clip.cpp"));
  const std::string renderer = compact(read_file(char_dir / "char_renderer.cpp"));
  const std::string scene = compact(read_file(scene_dir / "milo_scene.cpp"));
  const std::string doc =
      read_file(char_dir / "IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md");

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

  bool ok = true;

  ok &= contains(doc, "MiloEditor/MiloLib/Assets/Rnd/RndMesh.cs",
                 "document cites RndMesh source");
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
  ok &= contains(doc, "rb3/src/system/char/CharHair.cpp",
                 "document cites CharHair runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharLookAt.cpp",
                 "document cites CharLookAt runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharEyes.cpp",
                 "document cites CharEyes runtime source");
  ok &= contains(doc, "rb3/src/system/char/CharIKHand.cpp",
                 "document cites CharIKHand runtime source");

  ok &= contains(object_cs, "publicenumNodeType:int{Int=0x00,Float=0x01",
                 "ObjectFields exposes DTB node enum");
  ok &= contains(object_cs, "type=Symbol.Read(reader);root.Read(reader);",
                 "ObjectFields reads subtype Symbol and root DTB parent");
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
  ok &= missing(renderer, "is_hair_render_mesh",
                "renderer must not derive render state from hair names");
  ok &= missing(renderer, "is_hair_mesh_name",
                "renderer must not derive render state from hair mesh names");
  ok &= missing(renderer, "is_hair_material_name",
                "renderer must not derive render state from hair material names");
  ok &= missing(renderer, "legacy_blended_hair",
                "renderer must not keep legacy hair depth fallback");
  ok &= missing(renderer, "hairRender",
                "renderer debug output must not expose removed hair-name branch");
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

  ok &= contains(rb3_char_hair_cpp, "pt.radius+=f;pt.outerRadius+=f;",
                 "RB3 CharHair source adds rev 6/7/8 float to both radii");
  ok &= contains(char_mesh,
                 "point.radius+=add_to_radius;point.outer_radius+=add_to_radius;",
                 "native CharHair decode follows rev 6/7/8 radius addition");
  ok &= contains(rb3_char_hair_cpp,
                 "if(CharHair::gRev<8){pt.sideLength=-1.0f;if(CharHair::gRev>5){"
                 "inti;bs>>i>>i;}}",
                 "RB3 CharHair source consumes two ints for old revs above 5");
  ok &= contains(char_mesh,
                 "if(hair.version<8){point.side_length=-1.0f;if(hair.version>5){"
                 "(void)r.i32();(void)r.i32();}}",
                 "native CharHair decode consumes two ints for old revs above 5");
  ok &= contains(rb3_char_lookat_cpp, "mPivot->SetWorldXfm(tf90);",
                 "RB3 CharLookAt poll writes the pivot transform");
  ok &= contains(rb3_char_lookat_cpp, "RndTransformable*srcTrans=GetSource();",
                 "RB3 CharLookAt poll resolves source through GetSource");
  ok &= contains(rb3_char_eyes_cpp, "plist.push_back((*it).mEye);",
                 "RB3 CharEyes delegates poll children to CharLookAt rows");
  ok &= contains(rb3_char_ik_hand_cpp, "voidCharIKHand::Poll(){",
                 "RB3 CharIKHand source exposes Poll");
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
