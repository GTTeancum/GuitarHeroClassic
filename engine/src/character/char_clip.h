// engine/src/character/char_clip.h
//
// CharClipSamples decoder: loads all frames from a GH2 PS2 animation clip.

#pragma once

#include "character/char_mesh.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ghogx::character {

enum SourceCharBonesType {
  kSourceCharBonesTypePos = 0,
  kSourceCharBonesTypeScale = 1,
  kSourceCharBonesTypeQuat = 2,
  kSourceCharBonesTypeRotX = 3,
  kSourceCharBonesTypeRotY = 4,
  kSourceCharBonesTypeRotZ = 5,
  kSourceCharBonesTypeEnd = 6,
};

// One channel value for one frame.
struct ClipChannel {
  enum Type { kPos, kScale, kQuat, kRotX, kRotY, kRotZ } type = kPos;
  std::string bone_name;  // name without suffix (e.g. "bone_R-clavicle")
  float pos[3] = {};      // kPos: X,Y,Z
  float scale[3] = {1.0f, 1.0f, 1.0f};  // kScale: local X,Y,Z scale
  float quat[4] = {};     // kQuat: X,Y,Z,W
  float angle = 0.0f;     // kRotX/kRotY/kRotZ: radians
};

// All frames of one clip, indexed [frame][channel].
struct CharClip {
  std::string name;
  std::vector<std::vector<ClipChannel>> frames;  // frames[f][ch]
  struct OutputBone {
    std::string name;    // CharBone entry name, normally bone_*.trans
    std::string parent;  // CharBone parent, normally another *.trans
    milo_scene::Xfm local;
    milo_scene::Xfm world_stored;
    uint32_t char_bone_version = 0;
    uint32_t trans_version = 0;
    uint32_t trans_constraint = 0;
    std::string trans_target;
    bool preserve_scale = false;
    int32_t position_context = 0;
    int32_t scale_context = 0;
    int32_t rotation_type = 6;  // ihatecompvir CharBones::TYPE_END.
    int32_t rotation_context = 0;
    int32_t legacy_pre_rev5_int = 0;
    bool has_legacy_pre_rev5_int = false;
    int32_t legacy_rev3_to_7_int = 0;
    bool has_legacy_rev3_to_7_int = false;
    std::string target;
    struct WeightContext {
      int32_t context = 0;
      float weight = 0.0f;
    };
    std::vector<WeightContext> weights;
    std::string trans;
    bool bake_out_as_top_level = false;
    size_t unread_bytes = 0;
  };
  // Animation MILOs carry CharBone output records beside CharClipSamples.
  // The public ihatecompvir snapshot used by this worktree does not include
  // the runtime pose publisher, so broad output publishing remains diagnostic.
  std::vector<OutputBone> output_bones;
  int fps = 30;        // authored clip playback rate
  float start_frame = 0.0f;
  float end_frame = 0.0f;
  uint32_t flags = 0;
  uint32_t default_play_flags = 0;
  float blend_width = 0.0f;
  float range = 0.0f;
  bool relative = false;
  bool loaded = false;

  float duration_seconds() const;
};

// Source-backed CharClipGroup load state. The public ihatecompvir source stores
// mClips, mWhich, and mFlags, and GetClip() advances mWhich in-place.
struct CharClipGroup {
  std::string name;
  std::string milo_path;
  std::vector<std::string> clips;
  uint32_t version = 0;
  int32_t which = 0;
  int32_t flags = 0;
  bool loaded = false;
};

struct ClipChannelLayer {
  std::vector<ClipChannel> channels;
  float weight = 1.0f;
  const std::vector<CharClip::OutputBone>* output_bones = nullptr;
  std::string debug_name;
  bool relative = false;
  bool overlay_override = false;
};

enum CharPlayFlags : uint32_t {
  kCharPlayNoDefault = 0x00000000u,
  kCharPlayNow       = 0x00000001u,
  kCharPlayNoBlend   = 0x00000002u,
  kCharPlayFirst     = 0x00000003u,
  kCharPlayLast      = 0x00000004u,
  kCharPlayDirty     = 0x00000008u,
  kCharPlayNoLoop    = 0x00000010u,
  kCharPlayLoop      = 0x00000020u,
  kCharPlayGraphLoop = 0x00000030u,
  kCharPlayNodeLoop  = 0x00000040u,
  kCharPlayRealTime  = 0x00000200u,
  kCharPlayUserTime  = 0x00000400u,
};

// Lightweight viewer-side CharDriver play-node emulation. It owns clip time,
// loop/clamp behavior, and the previous-node blend that the game runtime uses
// when a new clip is started without kCharPlayNoBlend.
class CharClipPlayer {
 public:
  void clear();
  void play(const CharClip& clip, uint32_t flags = kCharPlayLoop,
            float blend_width = -1.0f, float speed = 1.0f);
  void set_speed(float speed);
  void advance(float dt_seconds);
  void apply(Character& character, float weight = 1.0f) const;
  std::vector<ClipChannel> sampled_pose() const;
  bool sampled_pose_relative() const;
  float current_blend_weight() const;
  bool active() const { return !layers_.empty(); }
  const CharClip* current_clip() const;

 private:
  struct Layer {
    const CharClip* clip = nullptr;
    uint32_t flags = 0;
    float time_seconds = 0.0f;
    float blend_width = 0.0f;
    float blend_progress = 0.0f;
    float speed = 1.0f;
  };

  std::vector<Layer> layers_;
};

// Load all frames of a named CharClipSamples entry from the PS2 ARK.
// Returns a CharClip with frames.empty() on failure.
CharClip load_clip(const std::string& hdr_path,
                   const std::string& ark_path,
                   const std::string& milo_path,
                   const std::string& clip_name);

// Source-backed CharClipGroup::Load reader. Returns the group's serialized
// ObjPtr clip names and source mWhich/mFlags state from the first matching
// animation MILO.
CharClipGroup load_clip_group(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name);

// Source-backed CharClipGroup::GetClip index step. Mutates group.which.
std::optional<size_t> char_clip_group_get_clip_index(CharClipGroup& group);

// Source-backed CharClipDriver constructor play-flag masking.
uint32_t char_clip_driver_masked_play_flags(const CharClip& clip,
                                            uint32_t mask);

// Source-backed CharBones channel helpers.
int source_char_bones_type_of(const std::string& channel);
const char* source_char_bones_suffix_of(int type);
std::string source_char_bones_channel_name(const std::string& name, int type);
size_t source_char_bones_type_size(int type, int compression);

// Source-backed CharWeightable::Weight helper. The owner row is used when it
// resolves; otherwise this falls back to the row's own serialized weight.
float source_char_weightable_weight(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name);

// Source-backed CharWeightSetter::Poll helper for rows that do not require the
// unavailable CharDriver::EvaluateFlags body. Returns false when the row is
// driver-backed and no source evaluator is present.
bool source_char_weight_setter_poll(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name,
    float delta_beats,
    float& out_weight);

// Source-backed CharIKRod::ComputeRod/Poll helper. Returns false when any
// source-required endpoint or destination transform is unresolved.
bool source_char_ik_rod_compute_world(const CharIKRod& rod,
                                      const Character& character,
                                      std::array<float, 16>& dest_world);

// Compatibility helper for callers that only need the stored clip names.
std::vector<std::string> load_clip_group_names(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name);

// Apply one frame of a clip to the character's bone local matrices.
// frame_idx is clamped to [0, frames.size()-1].
void apply_clip_frame(const CharClip& clip, int frame_idx, Character& character);
void apply_clip_frame_weighted(const CharClip& clip, int frame_idx,
                               float weight, Character& character);

// Apply decoded character-level controllers that sit outside CharClipSamples
// (eyes/look-at, FaceFX servo targets). Call after clip poses for the frame.
struct FaceFxEyeProperties {
  float l_eye_x = 0.0f;
  float l_eye_z = 0.0f;
  float r_eye_x = 0.0f;
  float r_eye_z = 0.0f;
  bool has_l_eye_x = false;
  bool has_l_eye_z = false;
  bool has_r_eye_x = false;
  bool has_r_eye_z = false;
};

void apply_character_controllers(Character& character, float time_seconds,
                                 FaceFxEyeProperties* eye_props = nullptr);

void clear_runtime_ik_weights(Character& character);
void set_runtime_ik_weight(Character& character, const std::string& weight_prop,
                           float weight);
void clear_runtime_trans_worlds(Character& character);

// CharIKMidi bridge. GHDX/PS2 player*_fret_pos maps MIDI pitches 40..59 to
// spot_neck_fret01..20 and feeds the character's fret.ik object.
void apply_ik_midi_fret_target(Character& character,
                               const std::string& spot_name,
                               float time_seconds);

// Legacy single-frame helpers kept for --clip screenshot mode.
std::vector<ClipChannel> load_clip_pose(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& clip_name);
void apply_clip_pose(const std::vector<ClipChannel>& channels, Character& character);
void apply_clip_pose_weighted(const std::vector<ClipChannel>& channels,
                              float weight, Character& character,
                              bool relative = false);
void apply_clip_pose_sampled(const std::vector<ClipChannel>& channels,
                             float weight, Character& character,
                             bool relative = false);
std::vector<ClipChannel> blend_channel_layers(
    const std::vector<ClipChannelLayer>& layers);
void apply_clip_channel_layers(const std::vector<ClipChannelLayer>& layers,
                               Character& character, bool relative = false);

}  // namespace ghogx::character
