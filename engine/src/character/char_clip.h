// engine/src/character/char_clip.h
//
// CharClipSamples decoder: loads all frames from a GH2 PS2 animation clip.

#pragma once

#include "character/char_mesh.h"

#include <array>
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

struct SourceCharBonesLayout {
  std::array<int, kSourceCharBonesTypeEnd + 1> counts = {};
  std::array<int, kSourceCharBonesTypeEnd + 1> offsets = {};
  int total_size = 0;
};

struct SourceCharBonesCompressionUpdate {
  int compression = 0;
  SourceCharBonesLayout layout;
  bool changed = false;
};

struct SourceCharBonesBone {
  std::string name;
  float weight = 1.0f;
};

struct SourceCharBonesState {
  int compression = 0;
  SourceCharBonesLayout layout;
  std::vector<SourceCharBonesBone> bones;
};

struct SourceCharBonesFindPtrResult {
  bool found = false;
  int offset = -1;
};

struct SourceCharBonesScaleAddClipStep {
  bool call_clip_scale_add = true;
  float f1 = 0.0f;
  float f2 = 0.0f;
  float f3 = 0.0f;
};

struct SourceCharBonesSamplesState {
  SourceCharBonesState bones;
  int num_samples = 0;
  int preview_sample = 0;
  int start_offset = 0;
  int raw_data_size = 0;
  std::vector<float> frames;
};

struct SourceCharBonesSampleStep {
  int start_offset = 0;
  float weight = 0.0f;
};

enum class SourceCharUtlObjectKind {
  kTransformable,
  kMesh,
  kCamera,
  kDirectory,
  kCharBone,
  kCharCollide,
  kCharCuff,
};

struct SourceCharUtlObject {
  std::string name;
  SourceCharUtlObjectKind kind = SourceCharUtlObjectKind::kTransformable;
  int mesh_bone_count = 0;
  std::string char_bone_transform;
};

struct SourceCharUtlBoneTransResult {
  std::string lookup_name;
  std::string resolved_name;
  bool via_char_bone = false;
};

struct SourceCharLookAtBounds {
  std::array<float, 3> min = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> max = {0.0f, 0.0f, 0.0f};
};

struct SourceCharLookAtEnterState {
  std::array<float, 3> smoothed_dir = {1.0e29f, 0.0f, 0.0f};
  bool reset_pivot_local = false;
};

struct SourceCharLookAtPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
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

struct SourceCharClipRefOwner {
  bool is_clip_group = false;
  std::vector<std::string> group_clips;
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
  void set_source_driver_blend_width(float blend_width);
  void set_source_play_multiple_clips(bool play_multiple_clips);
  void set_speed(float speed);
  void advance(float dt_seconds);
  void apply(Character& character, float weight = 1.0f) const;
  std::vector<ClipChannel> sampled_pose() const;
  bool sampled_pose_relative() const;
  float current_blend_weight() const;
  bool source_starved() const;
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

  float source_driver_blend_width_ = 1.0f;
  bool source_play_multiple_clips_ = false;
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

// Source-backed CharClipGroup::NumFlagDuplicates helper. `clip_index` selects
// the source clip row whose flags are compared against every other row.
int source_char_clip_group_num_flag_duplicates(
    const std::vector<uint32_t>& clip_flags,
    size_t clip_index,
    uint32_t mask);
std::vector<std::string> source_char_clip_group_sorted_names(
    std::vector<std::string> clip_names);

struct SourceCharClipDriverState {
  uint32_t play_flags = 0;
  float blend_width = 0.0f;
  float time_scale = 1.0f;
  float d_beat = 0.0f;
  float advance_beat = 0.0f;
  bool has_clip = false;
  bool has_next = false;
  int next_event = -1;
  bool play_multiple_clips = false;
};

struct SourceCharClipDriverExitDecision {
  bool recurse_next = false;
  bool execute_exit_event = false;
  bool end_sync_anim = false;
  bool delete_self = false;
  std::optional<size_t> returned_stack_head;
  std::vector<size_t> deleted_indices;
};

struct SourceCharClipDriverDeleteClipResult {
  std::optional<size_t> deleted_index;
  std::vector<size_t> remaining_indices;
};

// Source-backed CharClipDriver constructor play-flag masking.
uint32_t source_char_clip_driver_masked_play_flags(uint32_t clip_play_flags,
                                                   uint32_t mask);
uint32_t char_clip_driver_masked_play_flags(const CharClip& clip,
                                            uint32_t mask);
SourceCharClipDriverState source_char_clip_driver_construct(
    uint32_t clip_play_flags,
    bool has_clip,
    bool has_next,
    uint32_t mask,
    float blend_width,
    bool play_multiple_clips);
std::vector<size_t> source_char_clip_driver_delete_stack_order(
    size_t stack_size);
SourceCharClipDriverExitDecision source_char_clip_driver_exit_decision(
    size_t stack_size,
    bool exit_next,
    bool has_sync_anim);
SourceCharClipDriverDeleteClipResult source_char_clip_driver_delete_clip_result(
    const std::vector<bool>& clip_matches_source_order);
bool source_char_clip_driver_should_execute_event(bool symbol_null,
                                                  bool clip_has_type_def);

// Source-backed CharClip::BeatAlignString helper.
const char* source_char_clip_beat_align_string(uint32_t mask);

struct SourceCharClipFlagUpdate {
  uint32_t value = 0;
  bool dirty = false;
  bool changed = false;
};

struct SourceCharClipDefaultState {
  float frames_per_sec = 30.0f;
  uint32_t flags = 0;
  uint32_t play_flags = 0;
  float range = 0.0f;
  bool dirty = true;
  bool do_not_compress = false;
  int unk42 = -1;
  size_t beat_track_count = 1;
  float first_beat_frame = 0.0f;
  float first_beat_value = 0.0f;
};

struct SourceCharClipBeatEvent {
  std::string event;
  float beat = 0.0f;
};

struct SourceCharClipTransitionsState {
  bool has_owner = false;
  std::vector<int> node_sizes;
};

struct SourceCharClipTransitionsClearResult {
  size_t released_clips = 0;
  bool resized_zero = false;
};

struct SourceCharClipPoseMeshesSteps {
  std::string temp_meshes_name;
  bool stuff_bones = false;
  bool scale_down = false;
  float scale_down_weight = 0.0f;
  bool scale_add = false;
  float scale_add_weight = 0.0f;
  float scale_add_frame = 0.0f;
  float scale_add_blend = 0.0f;
  bool pose_meshes = false;
};

enum SourceCharDriverApplyMode {
  kSourceCharDriverApplyBlend = 0,
  kSourceCharDriverApplyAdd = 1,
  kSourceCharDriverApplyRotateTo = 2,
  kSourceCharDriverApplyBlendWeights = 3,
};

struct SourceCharDriverState {
  bool has_bones = false;
  bool has_clips = false;
  bool has_first = false;
  bool has_test_clip = false;
  bool has_default_clip = false;
  bool default_play_starved = false;
  std::string starved_handler;
  bool last_node_valid = false;
  float old_beat = 1.0e30f;
  bool realign = false;
  float beat_scale = 1.0f;
  float blend_width = 1.0f;
  std::string clip_type;
  SourceCharDriverApplyMode apply = kSourceCharDriverApplyBlend;
  bool has_internal_bones = false;
  bool play_multiple_clips = false;
};

struct SourceCharDriverSyncDecision {
  bool changed = false;
  bool clear_stack = false;
  bool reset_last_node = false;
  bool delete_internal_bones = false;
  bool allocate_internal_bones = false;
  bool clear_internal_bones = false;
  bool stuff_internal_bones = false;
  bool has_internal_bones = false;
};

struct SourceCharDriverEnterDecision {
  bool changed = false;
  bool clear_stack = false;
  bool reset_last_node = false;
  bool reset_old_beat = false;
  bool reset_beat_scale = false;
  bool play_default_clip = false;
  int default_play_flags = 1;
  float default_requested_blend_width = -1.0f;
  float default_old_beat = 1.0e30f;
  float default_start = 0.0f;
};

struct SourceCharDriverPlayGroupDecision {
  bool has_clip_dir = false;
  bool found_group = false;
  bool request_play = false;
};

struct SourceCharDriverPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharDriverMidiState {
  bool unk89 = false;
  std::string parser;
  std::string flag_parser;
  int clip_flags = 0;
  float blend_override_pct = 1.0f;
  bool has_default_clip = false;
};

struct SourceCharDriverMidiEnterDecision {
  SourceCharDriverEnterDecision driver_enter;
  bool set_unk89 = false;
  bool add_parser_sink = false;
  bool add_flag_parser_sink = false;
};

struct SourceCharDriverMidiExitDecision {
  bool call_driver_exit = false;
  bool remove_parser_sink = false;
  bool remove_flag_parser_sink = false;
};

struct SourceCharDriverMidiParserDecision {
  bool used_default_clip = false;
  bool request_play = false;
  int play_flags = 0;
  float requested_blend_width = 0.0f;
  float old_beat = 0.0f;
  float start = 0.0f;
  float assigned_blend_width = 0.0f;
};

// Source-backed CharClip constructor state.
SourceCharClipDefaultState source_char_clip_default_state();
SourceCharClipBeatEvent source_char_clip_beat_event_default();
SourceCharClipBeatEvent source_char_clip_beat_event_copy(
    const SourceCharClipBeatEvent& source);
void source_char_clip_beat_event_assign(SourceCharClipBeatEvent& dest,
                                        const SourceCharClipBeatEvent& source);
SourceCharClipBeatEvent source_char_clip_beat_event_loaded(
    const std::string& event,
    float beat);
int source_char_clip_get_context(bool has_type_def,
                                 bool has_resource_array,
                                 int resource_context);
SourceCharClipTransitionsState source_char_clip_transitions_construct(
    bool has_owner);
size_t source_char_clip_transitions_size(
    const SourceCharClipTransitionsState& transitions);
SourceCharClipTransitionsClearResult source_char_clip_transitions_clear(
    SourceCharClipTransitionsState& transitions);
std::vector<SourceCharBonesBone> source_char_clip_stuff_bones(
    const std::vector<SourceCharBonesBone>& existing_bones,
    const std::vector<SourceCharBonesBone>& listed_bones);
SourceCharClipPoseMeshesSteps source_char_clip_pose_meshes_steps(float frame);

// Source-backed CharDriver constructor, Clear, Transfer, setter, and
// SyncInternalBones state helpers.
SourceCharDriverState source_char_driver_default_state();
void source_char_driver_clear(SourceCharDriverState& state);
SourceCharDriverEnterDecision source_char_driver_enter(
    SourceCharDriverState& state);
void source_char_driver_transfer(SourceCharDriverState& state,
                                 const SourceCharDriverState& driver);
void source_char_driver_set_clips(SourceCharDriverState& state,
                                  bool has_clips);
void source_char_driver_set_bones(SourceCharDriverState& state,
                                  bool has_bones);
void source_char_driver_set_starved(SourceCharDriverState& state,
                                    const std::string& starved_handler);
void source_char_driver_set_blend_width(SourceCharDriverState& state,
                                        float blend_width);
SourceCharDriverSyncDecision source_char_driver_sync_internal_bones(
    SourceCharDriverState& state);
SourceCharDriverSyncDecision source_char_driver_set_apply(
    SourceCharDriverState& state,
    SourceCharDriverApplyMode apply);
SourceCharDriverSyncDecision source_char_driver_set_clip_type(
    SourceCharDriverState& state,
    const std::string& clip_type);
SourceCharDriverPlayGroupDecision source_char_driver_play_group_decision(
    bool has_clip_dir,
    bool found_group);
void source_char_driver_poll_deps(SourceCharDriverPollDeps& deps,
                                  const std::string& bones);
SourceCharDriverMidiState source_char_driver_midi_default_state();
SourceCharDriverMidiEnterDecision source_char_driver_midi_enter(
    SourceCharDriverState& driver_state,
    SourceCharDriverMidiState& midi_state,
    bool parser_found,
    bool flag_parser_found);
SourceCharDriverMidiExitDecision source_char_driver_midi_exit(
    bool parser_found,
    bool flag_parser_found);
void source_char_driver_midi_on_parser_flags(
    SourceCharDriverMidiState& midi_state,
    int clip_flags);
SourceCharDriverMidiParserDecision source_char_driver_midi_on_parser(
    const SourceCharDriverMidiState& midi_state,
    bool found_clip,
    bool clip_uses_real_time,
    float message_float,
    float beat_to_seconds_message_plus_current,
    float task_seconds,
    float average_beats_per_second);
SourceCharDriverMidiParserDecision source_char_driver_midi_on_parser_group(
    const SourceCharDriverMidiState& midi_state,
    bool found_group,
    bool found_group_clip,
    bool clip_uses_real_time,
    float message_float,
    float average_beats_per_second);

// Source-backed CharClip::SetFlags / SetPlayFlags dirty-state helpers.
SourceCharClipFlagUpdate source_char_clip_set_flags(uint32_t current_flags,
                                                    bool current_dirty,
                                                    uint32_t requested_flags);
SourceCharClipFlagUpdate source_char_clip_set_play_flags(
    uint32_t current_play_flags,
    bool current_dirty,
    uint32_t requested_play_flags);
bool source_char_clip_shares_groups(
    const std::vector<SourceCharClipRefOwner>& ref_owners,
    const std::string& candidate_clip_name);

// Source-backed CharDriver::Starved helper for the visible play stack state.
bool source_char_driver_starved(bool has_first, bool first_has_next,
                                uint32_t first_play_flags);

// Source-backed CharDriver::Play blend-width fallback.
float source_char_driver_resolve_blend_width(float requested_blend_width,
                                             float driver_blend_width);

// Source-backed CharDriver::Play duplicate-clip gate.
bool source_char_driver_should_start_clip(bool play_multiple_clips,
                                          bool clip_already_playing);

// Source-backed CharDriver::FirstPlaying helper. The input is in source stack
// order: mFirst, then each mNext.
std::optional<size_t> source_char_driver_first_playing_index(
    const std::vector<float>& source_stack_blend_fracs);

// Source-backed CharBones channel helpers.
int source_char_bones_type_of(const std::string& channel);
const char* source_char_bones_suffix_of(int type);
std::string source_char_bones_channel_name(const std::string& name, int type);
size_t source_char_bones_type_size(int type, int compression);
SourceCharBonesLayout source_char_bones_recompute_layout(
    const std::array<int, kSourceCharBonesTypeEnd + 1>& counts,
    int compression);
SourceCharBonesCompressionUpdate source_char_bones_set_compression(
    int current_compression,
    const SourceCharBonesLayout& current_layout,
    int requested_compression);
SourceCharBonesState source_char_bones_empty_state();
void source_char_bones_clear(SourceCharBonesState& state);
void source_char_bones_set_weights(std::vector<SourceCharBonesBone>& bones,
                                   float weight);
void source_char_bones_set_weights(SourceCharBonesState& state, float weight);
void source_char_bones_list_bones(const SourceCharBonesState& state,
                                  std::vector<SourceCharBonesBone>& bones);
int source_char_bones_find_offset(const SourceCharBonesState& state,
                                  const std::string& channel);
SourceCharBonesFindPtrResult source_char_bones_find_ptr(
    const SourceCharBonesState& state,
    const std::string& channel);
void source_char_bones_zero(std::vector<uint8_t>& start, int total_size);
SourceCharBonesScaleAddClipStep source_char_bones_scale_add_clip_step(
    float f1, float f2, float f3);

// Source-backed CharBone helpers for decoded CharClip output rows.
CharClip::OutputBone source_char_bone_copy_members(
    const CharClip::OutputBone& source);
std::optional<size_t> source_char_bone_find_weight_index(
    const CharClip::OutputBone& bone, int context_mask);
float source_char_bone_get_weight(const CharClip::OutputBone& bone,
                                  int context_mask);
void source_char_bone_clear_context(CharClip::OutputBone& bone,
                                    int context_mask);
void source_char_bone_stuff_bones(const CharClip::OutputBone& bone,
                                  int context_mask,
                                  std::vector<SourceCharBonesBone>& bones);
void source_char_bone_dir_list_bones(
    const std::vector<CharClip::OutputBone>& output_bones,
    int move_context,
    int context_mask,
    bool include_delta_facing,
    std::vector<SourceCharBonesBone>& bones);

// Source-backed CharServoBone movement helpers. These port the isolated math
// bodies only; broad CharBonesMeshes movement stays fenced to the clip stack.
void source_char_servo_bone_zero_deltas(
    std::array<float, 3>& facing_pos_delta,
    float& facing_rot_delta_radians);
void source_char_servo_bone_move_to_facing(
    milo_scene::Xfm& xfm,
    const std::array<float, 3>& facing_pos,
    float facing_rot_radians);
void source_char_servo_bone_move_to_delta_facing(
    milo_scene::Xfm& xfm,
    const std::array<float, 3>& facing_pos_delta,
    float facing_rot_delta_radians);

// Source-backed CharBonesSamples state helpers.
SourceCharBonesSamplesState source_char_bones_samples_empty_state();
void source_char_bones_samples_set(SourceCharBonesSamplesState& samples,
                                   const SourceCharBonesState& bones,
                                   int num_samples,
                                   int compression);
SourceCharBonesSamplesState source_char_bones_samples_clone(
    const SourceCharBonesSamplesState& source);
int source_char_bones_samples_allocate_size(
    const SourceCharBonesSamplesState& samples);
bool source_char_bones_samples_set_preview(
    SourceCharBonesSamplesState& samples, int requested_sample);
std::vector<SourceCharBonesSampleStep> source_char_bones_samples_split_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac);
int source_char_bones_samples_rotate_by_offset(
    const SourceCharBonesSamplesState& samples,
    int sample);
std::vector<SourceCharBonesSampleStep> source_char_bones_samples_rotate_to_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float angle,
    float frac);
std::vector<SourceCharBonesSampleStep>
source_char_bones_samples_scale_add_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac);
bool source_char_bones_samples_set_ver_known(int version);
bool source_char_bones_samples_load_version_known(int version);

// Source-backed CharUtl name/object helpers. CharUtlFindBone rewrites the
// incoming name to .cb. CharUtlFindBoneTrans checks .cb first and returns that
// CharBone row's transform before falling back to .trans, then .mesh.
std::string source_char_utl_name_with_suffix(const std::string& name,
                                             const std::string& suffix);
std::optional<SourceCharUtlObject> source_char_utl_find_bone(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects);
std::optional<SourceCharUtlBoneTransResult> source_char_utl_find_bone_trans(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects);
bool source_char_utl_is_animatable(const SourceCharUtlObject& object);

// Source-backed CharLookAt::SyncLimits helper. Angles are serialized in degrees.
SourceCharLookAtBounds source_char_lookat_sync_limits(
    float min_yaw, float max_yaw, float min_pitch, float max_pitch);
SourceCharLookAtEnterState source_char_lookat_enter(bool has_pivot);
void source_char_lookat_poll_deps(SourceCharLookAtPollDeps& deps,
                                  const std::string& source,
                                  const std::string& pivot,
                                  const std::string& dest);

// Source-backed CharWeightable::Weight helper. The owner row is used when it
// resolves; otherwise this falls back to the row's own serialized weight.
struct SourceCharWeightableState {
  std::string name;
  float weight = 1.0f;
  std::string weight_owner;
};

SourceCharWeightableState source_char_weightable_default_state(
    const std::string& name);
void source_char_weightable_set_weight(SourceCharWeightableState& state,
                                       float weight);
void source_char_weightable_set_weight_owner(SourceCharWeightableState& state,
                                             const std::string& weight_owner);
void source_char_weightable_replace(SourceCharWeightableState& state,
                                    const std::string& old_owner,
                                    const std::string& new_owner,
                                    bool new_owner_is_weightable);
void source_char_weightable_copy(SourceCharWeightableState& dest,
                                 const SourceCharWeightableState& source,
                                 bool shallow_copy,
                                 float source_owner_weight);
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

struct SourceCharWeightSetterRefOwner {
  std::string name;
  bool weight_owner_is_setter = false;
};

struct SourceCharWeightSetterPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharWeightSetterState {
  SourceCharWeightableState weightable;
  bool has_base = false;
  bool has_driver = false;
  size_t min_weight_count = 0;
  size_t max_weight_count = 0;
  uint32_t flags = 0;
  float offset = 0.0f;
  float scale = 1.0f;
  float base_weight = 0.0f;
  float beats_per_weight = 0.0f;
};

SourceCharWeightSetterState source_char_weight_setter_default_state(
    const std::string& name);
void source_char_weight_setter_set_weight(SourceCharWeightSetterState& state,
                                          float weight);

// Source-backed CharWeightSetter::PollDeps helper. Ref owners are supplied in
// source Refs() order; the helper scans them in reverse like the source body.
void source_char_weight_setter_poll_deps(
    SourceCharWeightSetterPollDeps& deps,
    const CharWeightSetter& setter,
    const std::vector<SourceCharWeightSetterRefOwner>& ref_owners);

// Source-backed CharIKRod::ComputeRod/Poll helper. Returns false when any
// source-required endpoint or destination transform is unresolved.
bool source_char_ik_rod_compute_world(const CharIKRod& rod,
                                      const Character& character,
                                      std::array<float, 16>& dest_world);

struct SourceCharIKHandMeasure {
  bool has_elbow_chain = false;
  float inv_2ab = 0.0f;
  float a2_plus_b2 = 0.0f;
  float aa_plus_bb = 0.0f;
};

// Source-backed CharIKHand::MeasureLengths / IKElbow scalar helper. The length
// inputs correspond to mHand->mLocalXfm.v and mHand->TransParent()->mLocalXfm.v.
SourceCharIKHandMeasure source_char_ik_hand_measure_lengths(
    bool has_elbow_chain,
    float hand_local_len,
    float parent_local_len);
bool source_char_ik_hand_update_measure_lengths(bool scalable,
                                                bool& hand_changed);
bool source_char_ik_hand_elbow_cosine(
    const SourceCharIKHandMeasure& measure,
    float distance_squared,
    float& out_cosine);

// Source-backed CharBoneOffset::Poll helper. Returns false when the source
// object pointer or its parent transform would be missing.
bool source_char_bone_offset_poll_world(
    const CharBoneOffset& offset,
    bool has_dest,
    bool has_parent,
    const milo_scene::Xfm& dest_local,
    const std::array<float, 16>& parent_world,
    std::array<float, 16>& dest_world);
void source_char_bone_offset_apply_to_local(const CharBoneOffset& offset,
                                            milo_scene::Xfm& dest_local);

// Source-backed CharBoneTwist::Poll helpers. Returns false when the source
// bone or target list would be missing.
float source_char_bone_twist_weight(
    const CharBoneTwist& twist,
    const std::unordered_map<std::string, float>& weights_by_name);
bool source_char_bone_twist_poll_world(
    const CharBoneTwist& twist,
    bool has_bone,
    const std::array<float, 16>& bone_world,
    const std::vector<std::array<float, 16>>& target_worlds,
    const std::unordered_map<std::string, float>& weights_by_name,
    std::array<float, 16>& out_world);

// Source-backed CharHair::FreezePoseRaw helper. Writes current runtime point
// positions back into point.unk5c in the strand root-parent local basis.
int source_char_hair_freeze_pose_raw(Character& character, CharHair& hair,
                                     SourceCharHairRuntime& state);

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
