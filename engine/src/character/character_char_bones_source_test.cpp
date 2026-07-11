#include "character/char_clip.h"

#include <cstddef>
#include <iostream>
#include <string>

namespace {

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got '" << got << "' want '" << want << "'\n";
  return false;
}

}  // namespace

int main() {
  using namespace ghogx::character;

  bool ok = true;
  ok &= expect_int(source_char_bones_type_of("bone_head.pos"),
                   kSourceCharBonesTypePos, "type pos");
  ok &= expect_int(source_char_bones_type_of("bone_head.scale"),
                   kSourceCharBonesTypeScale, "type scale");
  ok &= expect_int(source_char_bones_type_of("bone_head.quat"),
                   kSourceCharBonesTypeQuat, "type quat");
  ok &= expect_int(source_char_bones_type_of("bone_head.rotx"),
                   kSourceCharBonesTypeRotX, "type rotx");
  ok &= expect_int(source_char_bones_type_of("bone_head.roty"),
                   kSourceCharBonesTypeRotY, "type roty");
  ok &= expect_int(source_char_bones_type_of("bone_head.rotz"),
                   kSourceCharBonesTypeRotZ, "type rotz");
  ok &= expect_int(source_char_bones_type_of("bone_head"),
                   kSourceCharBonesTypeEnd, "type missing suffix");

  ok &= expect_string(source_char_bones_suffix_of(kSourceCharBonesTypePos),
                      "pos", "suffix pos");
  ok &= expect_string(source_char_bones_suffix_of(kSourceCharBonesTypeRotZ),
                      "rotz", "suffix rotz");
  ok &= expect_string(source_char_bones_suffix_of(kSourceCharBonesTypeEnd),
                      "", "suffix invalid");

  ok &= expect_string(source_char_bones_channel_name("bone_head",
                                                    kSourceCharBonesTypeQuat),
                      "bone_head.quat", "channel append suffix");
  ok &= expect_string(source_char_bones_channel_name("bone_head.pos",
                                                    kSourceCharBonesTypeRotY),
                      "bone_head.roty", "channel replace suffix");
  ok &= expect_string(source_char_bones_channel_name("bone.head.pos",
                                                    kSourceCharBonesTypeRotX),
                      "bone.rotx", "channel first-dot rule");

  for (int compression = 0; compression <= 4; ++compression) {
    const size_t vec_size = compression < 2 ? 12u : 6u;
    const size_t quat_size =
        compression > 2 ? 4u : (compression == 0 ? 16u : 8u);
    const size_t angle_size = compression == 0 ? 4u : 2u;
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypePos,
                                                  compression),
                      vec_size, "type size pos");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeScale,
                                                  compression),
                      vec_size, "type size scale");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeQuat,
                                                  compression),
                      quat_size, "type size quat");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeRotX,
                                                  compression),
                      angle_size, "type size rotx");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeRotY,
                                                  compression),
                      angle_size, "type size roty");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeRotZ,
                                                  compression),
                      angle_size, "type size rotz");
  }

  ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeEnd, 0), 0,
                    "type size invalid");

  return ok ? 0 : 1;
}
