#include "milo_convert.h"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <utility>

namespace gh::milo_convert {
namespace {

constexpr const char* kValueProof =
    "converter value assertions; target semantic round trip; packed-source "
    "audit";
constexpr const char* kGraphProof =
    "directory graph assertions; target reference audit; packed-source audit";

using SchemaKey = std::pair<std::string, uint32_t>;
using SchemaMap = std::map<SchemaKey, std::vector<std::string>>;
constexpr uint32_t kAnyDtbControlWord = 0xFFFFFFFFu;

void append_fields(
    std::vector<std::string>& output,
    std::initializer_list<const char*> fields,
    const char* prefix = "") {
    for (const char* field : fields)
        output.push_back(std::string(prefix) + field);
}

void append_animatable(
    std::vector<std::string>& output,
    const char* prefix = "animatable.") {
    append_fields(
        output,
        {"revision", "operations[].type", "operations[].first",
         "operations[].second", "operations[].loop",
         "operations[].integers", "objects"},
        prefix);
}

void append_drawable(
    std::vector<std::string>& output,
    const char* prefix = "drawable.") {
    append_fields(
        output,
        {"revision", "showing", "objects", "sphere", "draw_order",
         "legacy_target"},
        prefix);
}

void append_transformable(
    std::vector<std::string>& output,
    const char* prefix = "transformable.") {
    append_fields(
        output,
        {"revision", "local", "world", "children", "constraint", "target",
         "preserve_scale", "parent"},
        prefix);
}

SchemaMap build_source_schemas() {
    SchemaMap result;
    const auto add =
        [&](const char* type, uint32_t revision,
            std::initializer_list<const char*> fields,
            bool animatable = false, bool drawable = false,
            bool transformable = false,
            const char* drawable_prefix = "drawable.") {
            std::vector<std::string> schema = {"revision"};
            if (animatable) append_animatable(schema);
            if (transformable) append_transformable(schema);
            if (drawable) append_drawable(schema, drawable_prefix);
            append_fields(schema, fields);
            result.emplace(SchemaKey{type, revision}, std::move(schema));
        };

    add(
        "Cam", 9,
        {"near_plane", "far_plane", "fov", "screen_rect", "z_range",
         "target_texture"},
        false, true, true);
    add(
        "ACP", 18,
        {"class_name", "object_name", "start_beat", "end_beat",
         "beats_per_second", "flags", "play_flags", "blend_width",
         "sample_set_revision", "channel_sets[].channels",
         "channel_sets[].sample_count", "channel_sets[].compression",
         "channel_sets[].frame_size", "channel_sets[].sample_bytes",
         "trailing_bytes"});
    add(
        "CamAnim", 0,
        {"camera", "fov_keys[].value", "fov_keys[].frame", "keys_owner"},
        true);
    add(
        "Environ", 1,
        {"lights", "ambient_color", "fog_range", "fog_color",
         "fog_enabled"},
        false, true, false, "legacy_drawable.");
    add(
        "EnvAnim", 3,
        {"environment", "ambient_color_keys[].value",
         "ambient_color_keys[].frame", "keys_owner",
         "fog_color_keys[].value", "fog_color_keys[].frame",
         "fog_range_keys[].value", "fog_range_keys[].frame"},
        true);
    add(
        "Flare", 3, {"material", "sizes", "range", "steps"},
        false, true, true);
    add(
        "Font", 7,
        {"material", "cell_size", "deprecated_size", "base_kerning",
         "characters", "has_kerning_table",
         "kerning[].packed_char_pair", "kerning[].kerning"});
    add(
        "Light", 3, {"color", "range", "serialized_type"},
        false, false, true);
    add(
        "LightAnim", 1,
        {"light", "color_keys[].value", "color_keys[].frame", "keys_owner"},
        true);
    add(
        "Mat", 21,
        {"textures[].stage_blend", "textures[].tex_gen",
         "textures[].transform", "textures[].wrap", "textures[].texture",
         "blend", "color", "use_environment", "vertex_ambient",
         "vertex_dynamic", "cull", "multipass", "normalize", "z_mode",
         "alpha_cut", "alpha_write"});
    add(
        "MatAnim", 5,
        {"material", "stages[].translation_keys[].value",
         "stages[].translation_keys[].frame",
         "stages[].scale_keys[].value", "stages[].scale_keys[].frame",
         "stages[].rotation_keys[].value",
         "stages[].rotation_keys[].frame",
         "stages[].texture_keys[].object",
         "stages[].texture_keys[].frame", "keys_owner",
         "color_keys[].value", "color_keys[].frame",
         "alpha_keys[].value", "alpha_keys[].frame"},
        true);
    add(
        "Mesh", 25,
        {"material", "geometry_owner", "mutable_flags", "volume",
         "has_bsp_tree", "vertices[].position", "vertices[].normal",
         "vertices[].color_or_weights", "vertices[].uv", "faces", "patches",
         "has_bones", "bone_slots[].bone", "bone_slots[].offset",
         "strip_results[].cumulative_strip_lengths",
         "strip_results[].strip_runs"},
        false, true, true);
    add(
        "MeshAnim", 0,
        {"mesh", "point_keys[].values", "point_keys[].frame",
         "texcoord_keys[].values", "texcoord_keys[].frame",
         "color_keys[].values", "color_keys[].frame", "keys_owner"},
        true);
    add(
        "Morph", 3,
        {"poses[].mesh", "poses[].keys[].value", "poses[].keys[].frame",
         "target", "normals", "spline", "intensity"},
        true);
    add(
        "Movie", 6, {"file", "texture", "stream", "loop"}, true);
    add(
        "MultiMesh", 0, {"mesh", "transforms"}, false, true);
    add(
        "ParticleSys", 22,
        {"life", "box_extent_1", "box_extent_2", "speed", "pitch", "yaw",
         "emit_rate", "start_size", "delta_size", "start_color_low",
         "start_color_high", "end_color_low", "end_color_high",
         "bounce_enabled", "bounce_plane", "force_direction", "material",
         "type", "grow_ratio", "shrink_ratio", "mid_color_ratio",
         "mid_color_low", "mid_color_high", "max_particles",
         "bubble_period", "bubble_size", "bubble", "relative_motion",
         "emitter_mesh", "preserve_particles", "particles[].position",
         "particles[].color", "particles[].size"},
        true, true, true);
    add(
        "ParticleSysAnim", 2,
        {"particle_system", "start_color_keys[].value",
         "start_color_keys[].frame", "end_color_keys[].value",
         "end_color_keys[].frame", "emit_rate_keys[].value",
         "emit_rate_keys[].frame", "keys_owner", "speed_keys[].value",
         "speed_keys[].frame", "life_keys[].value", "life_keys[].frame",
         "start_size_keys[].value", "start_size_keys[].frame"},
        true);
    add(
        "Tex", 8,
        {"width", "height", "bits_per_pixel", "external_path",
         "mipmap_bias", "type", "use_external", "has_bitmap",
         "bitmap.header_kind", "bitmap.bits_per_pixel", "bitmap.encoding",
         "bitmap.mipmap_count", "bitmap.width", "bitmap.height",
         "bitmap.bytes_per_line", "bitmap.wii_alpha", "bitmap.reserved",
         "bitmap.data"});
    add(
        "Text", 15,
        {"font", "alignment", "text", "color", "wrap_width", "leading",
         "fixed_length", "italics", "size", "markup", "caps_mode"},
        false, true, true);
    add(
        "TransAnim", 4,
        {"target", "rotation_keys[].value", "rotation_keys[].frame",
         "translation_keys[].value", "translation_keys[].frame",
         "keys_owner", "translation_spline", "repeat_translation",
         "scale_keys[].value", "scale_keys[].frame", "scale_spline",
         "follow_path", "rotation_slerp"},
        true, true);
    add(
        "View", 7, {"children_owner", "showing_range"},
        true, true, true);
    result.emplace(
        SchemaKey{"DTB", kAnyDtbControlWord},
        std::vector<std::string>{
            "version", "nodes[].tag", "nodes[].line",
            "nodes[].integer", "nodes[].floating", "nodes[].string",
            "nodes[].children", "storage", "cipher_seed",
            "trailing_bytes"});
    result.emplace(
        SchemaKey{"VenueCamRecord", 1},
        std::vector<std::string>{
            "category", "path", "name", "start", "end", "duration",
            "singer_in", "singer_out", "offset_in", "offset_out",
            "near", "far", "fov_in", "fov_out", "crowd_region",
            "shaky", "enable_dof", "hide_crowd", "walk_ok",
            "low_excitement_ok", "real_time", "ease",
            "force_char_lod", "force_cam_facing", "eyes",
            "bad_walk_spots", "guard"});
    result.emplace(
        SchemaKey{"VenueScript", 1},
        std::vector<std::string>{
            "functions[].name", "functions[].parameters",
            "functions[].body", "handlers[].name", "handlers[].body",
            "loaded_sections[].section",
            "loaded_sections[].directory",
            "state_initializers[].variable",
            "state_initializers[].value",
            "function_calls", "foreach.variable",
            "foreach.collection", "foreach.body",
            "switch.selector", "switch.branches",
            "switch_anim", "switch_anim_rt", "anim_task",
            "animate_to", "delay_task", "random_range",
            "generic_messages"});
    return result;
}

class ContractBuilder {
  public:
    ContractBuilder(
        std::vector<SemanticFieldContract>& output,
        const char* source_type, uint32_t source_revision,
        const char* target_type, const char* target_revision,
        const char* source_revision_field = "revision",
        const char* target_revision_field = "revision")
        : output_(output),
          source_type_(source_type),
          source_revision_(source_revision),
          target_type_(target_type),
          target_revision_(target_revision) {
        translated(
            source_revision_field, target_revision_field,
            "class revision is upgraded to the native GH2 revision");
        synthesized(
            "object_fields.revision",
            "GH2 Hmx::Object base is initialized at revision 0");
        synthesized(
            "object_fields.type",
            "no GH1 type symbol exists; the native GH2 default is empty");
        synthesized(
            "object_fields.has_type_properties",
            "GH1 object has no GH2 type-property block; initialized false");
        synthesized(
            "object_fields.type_property_id",
            "inactive GH2 type-property identifier is initialized to zero");
        synthesized(
            "object_fields.type_properties",
            "inactive GH2 type-property array is initialized empty");
    }

    void retained(
        const char* source_field, const char* target_field,
        const char* rule = "value is copied without reinterpretation",
        const char* verification = kValueProof) {
        add(
            source_field, target_type_, target_revision_, target_field,
            "retained", rule, verification);
    }

    void translated(
        const char* source_field, const char* target_field,
        const char* rule, const char* verification = kValueProof) {
        add(
            source_field, target_type_, target_revision_, target_field,
            "translated", rule, verification);
    }

    void discarded(
        const char* source_field, const char* rule,
        const char* verification = kValueProof) {
        add(
            source_field, target_type_, target_revision_, "<none>",
            "intentionally_discarded", rule, verification);
    }

    void synthesized(
        const char* target_field, const char* rule,
        const char* verification = kValueProof) {
        add(
            "<synthesized>", target_type_, target_revision_, target_field,
            "synthesized", rule, verification);
    }

    void retained_fields(
        std::initializer_list<const char*> fields,
        const char* source_prefix = "", const char* target_prefix = "") {
        for (const char* field : fields) {
            const std::string source =
                std::string(source_prefix) + field;
            const std::string target =
                std::string(target_prefix) + field;
            add(
                source, target_type_, target_revision_, target, "retained",
                "value is copied without reinterpretation", kValueProof);
        }
    }

    void animatable(
        const char* source_prefix = "animatable.",
        const char* target_prefix = "animatable.") {
        const std::string source_revision =
            std::string(source_prefix) + "revision";
        const std::string target_revision =
            std::string(target_prefix) + "revision";
        add(
            source_revision, target_type_, target_revision_, target_revision,
            "translated",
            "legacy Animatable0 becomes native Animatable4", kValueProof);
        for (const char* field :
             {"operations[].type", "operations[].first",
              "operations[].second", "operations[].loop",
              "operations[].integers"}) {
            add(
                std::string(source_prefix) + field,
                "AnimFilter", "1", "scale/offset/start/end/type",
                "translated",
                "legacy operation program is reduced by retail rules and, "
                "when non-default, expanded to a native AnimFilter",
                kGraphProof);
        }
        add(
            std::string(source_prefix) + "objects",
            "Group", "12|13", "objects",
            "translated",
            "legacy animation membership is resolved by the directory View "
            "graph and expanded without flattening nested Views",
            kGraphProof);
        add(
            "<synthesized>", target_type_, target_revision_,
            std::string(target_prefix) + "frame", "synthesized",
            "native Animatable frame starts at the GH2 constructor default",
            kValueProof);
        add(
            "<synthesized>", target_type_, target_revision_,
            std::string(target_prefix) + "rate", "synthesized",
            "native Animatable rate starts at the GH2 constructor default",
            kValueProof);
    }

    void drawable(
        bool target_has_drawable = true,
        const char* source_prefix = "drawable.",
        const char* target_prefix = "drawable.") {
        const std::string target_revision =
            std::string(target_prefix) + "revision";
        const std::string target_showing =
            std::string(target_prefix) + "showing";
        const std::string target_sphere =
            std::string(target_prefix) + "sphere";
        const std::string target_order =
            std::string(target_prefix) + "draw_order";
        if (target_has_drawable) {
            add(
                std::string(source_prefix) + "revision",
                target_type_, target_revision_, target_revision,
                "translated",
                "legacy Drawable0/1 fields become native Drawable3",
                kValueProof);
            add(
                std::string(source_prefix) + "showing",
                target_type_, target_revision_, target_showing, "retained",
                "showing state is copied", kValueProof);
            add(
                std::string(source_prefix) + "sphere",
                target_type_, target_revision_, target_sphere, "retained",
                "serialized legacy sphere is copied; absent old-revision "
                "sphere retains the native default",
                kValueProof);
            add(
                std::string(source_prefix) + "draw_order",
                target_type_, target_revision_, target_order, "retained",
                "serialized revision-3+ draw order is copied; absent "
                "old-revision value retains the native default",
                kValueProof);
        } else {
            add(
                std::string(source_prefix) + "revision",
                target_type_, target_revision_, "<none>",
                "intentionally_discarded",
                "the GH2 target class no longer inherits RndDrawable",
                kValueProof);
            add(
                std::string(source_prefix) + "showing",
                target_type_, target_revision_, "<none>",
                "intentionally_discarded",
                "the GH2 target class has no drawable showing field",
                kValueProof);
            add(
                std::string(source_prefix) + "sphere",
                target_type_, target_revision_, "<none>",
                "intentionally_discarded",
                "the GH2 target class has no drawable sphere field",
                kValueProof);
            add(
                std::string(source_prefix) + "draw_order",
                target_type_, target_revision_, "<none>",
                "intentionally_discarded",
                "the GH2 target class has no drawable order field",
                kValueProof);
        }
        add(
            std::string(source_prefix) + "objects",
            "Group", "12|13", "objects",
            "translated",
            "legacy Drawable0/1 membership is recursively resolved into "
            "native Group draw membership",
            kGraphProof);
        add(
            std::string(source_prefix) + "legacy_target",
            target_type_, target_revision_, "<none>",
            "intentionally_discarded",
            "revision-4 legacy target is absent from all observed GH1 "
            "Drawable0/1 bodies and has no GH2 Drawable3 serialization",
            kValueProof);
    }

    void transformable(
        const char* source_prefix = "transformable.",
        const char* target_prefix = "transformable.") {
        const auto target = [&](const char* field) {
            return std::string(target_prefix) + field;
        };
        add(
            std::string(source_prefix) + "revision",
            target_type_, target_revision_, target("revision"),
            "translated",
            "legacy Transformable8 becomes native Transformable9",
            kValueProof);
        for (const char* field : {"local", "world", "target",
                                  "preserve_scale"}) {
            add(
                std::string(source_prefix) + field,
                target_type_, target_revision_, target(field), "retained",
                "value is copied without reinterpretation", kValueProof);
        }
        add(
            std::string(source_prefix) + "children",
            target_type_, target_revision_, target("parent"),
            "translated",
            "legacy child ownership is resolved directory-wide into native "
            "parent links before explicit-parent precedence is applied",
            kGraphProof);
        add(
            std::string(source_prefix) + "constraint",
            target_type_, target_revision_, target("constraint"),
            "translated",
            "legacy enum 2/3 becomes 3/4, removed enum 4 becomes 0, and "
            "explicit parent fixup uses GH2 old-parent constraint 2",
            kGraphProof);
        add(
            std::string(source_prefix) + "parent",
            target_type_, target_revision_, target("parent"),
            "translated",
            "explicit parent overrides the legacy child-derived parent "
            "except for the self-parent sentinel",
            kGraphProof);
    }

    void add(
        std::string source_field, std::string target_type,
        std::string target_revision, std::string target_field,
        std::string disposition, std::string rule,
        std::string verification) {
        output_.push_back(
            {source_type_, source_revision_, std::move(source_field),
             std::move(target_type), std::move(target_revision),
             std::move(target_field), std::move(disposition),
             std::move(rule), std::move(verification)});
    }

  private:
    std::vector<SemanticFieldContract>& output_;
    std::string source_type_;
    uint32_t source_revision_;
    std::string target_type_;
    std::string target_revision_;
};

std::vector<SemanticFieldContract> build_contracts() {
    std::vector<SemanticFieldContract> rows;

    {
        const auto add =
            [&](const char* source_field, const char* target_field,
                const char* disposition, const char* rule) {
                rows.push_back(
                    {"DTB", kAnyDtbControlWord, source_field, "DTB",
                     "preserved", target_field,
                     disposition, rule,
                     "DTB byte-exact archive round trip; venue-script AST "
                     "differential and deterministic reparse"});
            };
        add(
            "version", "version", "retained",
            "the Classic DTB container control word is preserved verbatim; "
            "standalone DTBs normally use 1 while song SEQ trees carry other "
            "source-authored values");
        add(
            "nodes[].tag", "nodes[].tag", "translated",
            "ordinary trees round-trip exactly; venue scripts lower authored "
            "GH1 forms to source-backed GH2 WorldDir forms");
        add(
            "nodes[].line", "nodes[].line", "translated",
            "ordinary line metadata round-trips; synthesized venue nodes use "
            "deterministic compiler metadata");
        for (const char* field :
             {"nodes[].integer", "nodes[].floating", "nodes[].string",
              "nodes[].children"}) {
            add(
                field, field, "translated",
                "ordinary node payloads round-trip exactly; venue-script "
                "payloads are lowered by typed AST rules");
        }
        add(
            "storage", "storage", "retained",
            "plain, zero-prefixed, or encrypted storage mode is preserved");
        add(
            "cipher_seed", "cipher_seed", "retained",
            "encrypted trees preserve the original cipher seed");
        add(
            "trailing_bytes", "trailing_bytes", "retained",
            "declared-root residual bytes are preserved explicitly rather "
            "than ignored");
    }
    {
        const auto add =
            [&](const char* source_field, const char* target_type,
                const char* target_field, const char* rule,
                const char* verification =
                    "all 201 packed GH1 VenueCam records convert "
                    "deterministically; native target objects reparse "
                    "byte-exactly; retail behavior is checked by read-only "
                    "GH1/GH2 camera traces") {
                rows.push_back(
                    {"VenueCamRecord", 1, source_field, target_type,
                     "native", target_field, "translated", rule,
                     verification});
            };
        add(
            "category", "WorldDir",
            "camshots[].type_properties.category",
            "the authored GH1 camera category is retained as native "
            "selection metadata");
        add(
            "path", "CamAnim",
            "camshots[].camera_animation",
            "the authored transform-animation reference resolves through "
            "the converted directory graph");
        add(
            "name", "CamShot",
            "<directory entry name>",
            "the source record name is the deterministic native camera-shot "
            "entry name");
        add(
            "start", "CamAnim", "keys[].frame",
            "the source path interval is sampled from its authored start "
            "frame");
        add(
            "end", "CamAnim", "keys[].frame",
            "the source path interval is sampled through its authored end "
            "frame, including reversed intervals");
        add(
            "duration", "CamShot", "duration",
            "the authored duration is converted from GH1 milliseconds to "
            "native shot timing");
        add(
            "singer_in", "CamShot",
            "keyframes[].screen_offset",
            "the misleadingly named GH1 ArenaSinger slot-zero head framing "
            "endpoint is retained directly in native centered screen units");
        add(
            "singer_out", "CamShot",
            "keyframes[].screen_offset",
            "the misleadingly named GH1 ArenaSinger slot-zero head framing "
            "endpoint is retained directly in native centered screen units");
        add(
            "offset_in", "CamShot",
            "keyframes[].world_offset",
            "the GH1 camera-offset endpoint is evaluated in the decoded "
            "camera basis and retained in native keys");
        add(
            "offset_out", "CamShot",
            "keyframes[].world_offset",
            "the GH1 camera-offset endpoint is evaluated in the decoded "
            "camera basis and retained in native keys");
        add(
            "near", "Cam", "near_plane",
            "the authored near clip plane is copied to the native camera");
        add(
            "far", "Cam", "far_plane",
            "the authored far clip plane is copied to the native camera");
        add(
            "fov_in", "CamShot", "keyframes[].fov",
            "the GH1 opening field of view is converted to the native "
            "camera-shot convention");
        add(
            "fov_out", "CamShot", "keyframes[].fov",
            "the GH1 closing field of view is converted to the native "
            "camera-shot convention");
        add(
            "crowd_region", "CamShot",
            "type_properties.crowd_region",
            "the source crowd-region selector is retained for native "
            "crowd/culling policy");
        add(
            "shaky", "CamShot",
            "camera_animation.additive_shaky_cam1",
            "the shared GH1 shaky-camera animation is baked additively into "
            "the native shot");
        add(
            "enable_dof", "CamShot", "use_depth_of_field",
            "the authored depth-of-field enable is retained");
        add(
            "hide_crowd", "CamShot",
            "type_properties.hide_crowd",
            "the authored crowd visibility override is retained");
        add(
            "walk_ok", "CamShot",
            "type_properties.walk_ok",
            "the authored performer-walk eligibility is retained");
        add(
            "low_excitement_ok", "CamShot",
            "type_properties.low_excitement_ok",
            "the authored low-excitement eligibility is retained");
        add(
            "real_time", "CamShot", "timing.real_time",
            "the source timing domain is retained when compiling shot keys");
        add(
            "ease", "CamShot", "keyframes[].blend_ease",
            "the GH1 ease selector determines native interpolation and "
            "adaptive subdivision");
        add(
            "force_char_lod", "CamShot",
            "type_properties.force_char_lod",
            "the authored character-LOD override is retained");
        add(
            "force_cam_facing", "CamShot",
            "type_properties.force_cam_facing",
            "the field is retained even though every explicit/default value "
            "in the packed GH1 corpus is zero");
        add(
            "eyes", "CamShot", "type_properties.eyes",
            "the GH1 eye-targeting mode is retained for the native performer "
            "camera policy");
        add(
            "bad_walk_spots", "CamShot",
            "type_properties.bad_waypoints",
            "source bad-walk waypoint references become the native "
            "bad-waypoint list");
        add(
            "guard", "CamShot", "type_properties.guard",
            "the PS2 guard-band tuple is retained as source metadata; "
            "target projection uses its native guard-band implementation");
    }
    {
        const auto add =
            [&](const char* source_field, const char* target_field,
                const char* rule) {
                rows.push_back(
                    {"VenueScript", 1, source_field, "WorldDir", "native",
                     target_field, "translated", rule,
                     "all seven packed GH1 venue scripts lower with zero "
                     "unrecognized roots; output reparses deterministically "
                     "and handler behavior is covered by runtime traces"});
            };
        add(
            "functions[].name", "inlined_functions[].identity",
            "function names define a closed call graph and are removed only "
            "after every call is resolved");
        add(
            "functions[].parameters", "inlined_calls[].bindings",
            "formal parameters are substituted with scoped source arguments");
        add(
            "functions[].body", "inlined_calls[].body",
            "function bodies are cloned into each resolved call site");
        add(
            "handlers[].name", "types[].name",
            "each Arena handler becomes a native WorldDir local type");
        add(
            "handlers[].body", "types[].handler",
            "handler bodies are lowered recursively without dropping generic "
            "messages");
        add(
            "loaded_sections[].section",
            "object_directory.subdirectories[].section",
            "the source section namespace names a deterministic native "
            "ObjectDir subdirectory link");
        add(
            "loaded_sections[].directory",
            "object_directory.subdirectories[].path",
            "the authored source RndDir is resolved, converted independently, "
            "and linked from WorldDir exactly as native GH2 lighting "
            "subdirectories are linked, replacing GH1 Arena load_section "
            "code");
        add(
            "state_initializers[].variable", "types[].properties[].name",
            "the GH1 top-level DataVariable becomes a native WorldDir type "
            "property");
        add(
            "state_initializers[].value", "types[].properties[].value",
            "the GH1 top-level initial value becomes the native type-property "
            "default");
        add(
            "function_calls", "types[].handler",
            "every source-defined function call is expanded before emission");
        add(
            "foreach.variable", "types[].handler",
            "the scoped iterator is substituted into each finite expansion");
        add(
            "foreach.collection", "types[].handler",
            "the authored finite collection determines deterministic "
            "expansion cardinality and order");
        add(
            "foreach.body", "types[].handler",
            "the loop body is cloned once per source collection element");
        add(
            "switch.selector", "types[].handler.switch.selector",
            "dynamic selectors remain native switch expressions");
        add(
            "switch.branches", "types[].handler.switch.branches",
            "source branch order and bodies are retained");
        add(
            "switch_anim", "types[].handler.animatable.animate",
            "the GH1 Arena wrapper becomes a native animatable task call");
        add(
            "switch_anim_rt", "types[].handler.animatable.animate",
            "the real-time GH1 Arena wrapper becomes a native real-time "
            "animatable task call");
        add(
            "anim_task", "types[].handler.game.anim_task",
            "the authored game animation task is retained in native call "
            "form");
        add(
            "animate_to", "types[].handler.animatable.animate_to",
            "the authored target frame and duration become a native "
            "animate-to task");
        add(
            "delay_task", "types[].handler.script_task",
            "the authored delay and nested body become a native script task");
        add(
            "random_range", "types[].handler.range",
            "stateful/random GH1 range expressions are lowered with their "
            "source evaluation semantics");
        add(
            "generic_messages", "types[].handler.messages",
            "unrecognized-by-specializer but valid DTB messages are cloned "
            "recursively rather than discarded");
        const auto synth =
            [&](const char* target_field, const char* rule) {
                rows.push_back(
                    {"VenueScript", 1, "<synthesized>",
                     "WorldDir", "11", target_field,
                     "synthesized", rule,
                     "all eight packed native GH2 main WorldDirs establish "
                     "the target root defaults; WorldDir11 source order is "
                     "covered by ihatecompvir MiloLib and byte-exact target "
                     "round trips"});
            };
        synth(
            "revision",
            "the converted main venue root uses native GH2 WorldDir revision "
            "11");
        synth(
            "legacy_value",
            "all eight native GH2 main venues author zero");
        synth(
            "legacy_float",
            "all eight native GH2 main venues author 1.0");
        synth(
            "fake_hud_filename",
            "the native one-player venue HUD preview path is "
            "../../../hud/hud_1p_nocam.milo");
        synth(
            "panel_directory.revision",
            "the embedded native GH2 PanelDir uses revision 2");
        synth(
            "panel_directory.camera",
            "the native PanelDir preview-camera link selects the sole "
            "authored Cam entry in each packed GH1 main venue directory; "
            "retail GH2 uses the same field with default.cam");
        synth(
            "panel_directory.test_event",
            "all eight native GH2 main venues use ui_enter");
        synth(
            "panel_directory.render_directory.test_event",
            "a source-authored start handler selects the native WorldDir "
            "start test event; otherwise the field remains empty");
        synth(
            "legacy_transform",
            "GH1 has no native WorldDir editor-preview camera transform, so "
            "the target receives the neutral identity transform");
    }
    {
        ContractBuilder b(
            rows, "ACP", 18, "CharClipSamples", "10");
        b.translated(
            "class_name", "<directory entry type>",
            "AnimClipSamples is validated and emitted as native "
            "CharClipSamples");
        b.retained(
            "object_name", "<directory entry name>",
            "standalone ACP object name becomes the native directory entry "
            "name");
        b.retained_fields(
            {"start_beat", "end_beat", "beats_per_second", "blend_width"});
        b.translated(
            "flags", "flags",
            "standalone ACP export marker bit 31 is removed; gameplay flag "
            "bits are retained");
        b.translated(
            "play_flags", "play_flags",
            "GH1 clip-time mode enum is mapped to the native GH2 time flags");
        b.translated(
            "sample_set_revision", "full/one",
            "nested revision 5 is validated before conversion to native "
            "CharBonesSamples10 headers");
        b.retained(
            "channel_sets[].channels", "full/one.channels",
            "set 0 becomes full and set 1 becomes one");
        b.retained(
            "channel_sets[].sample_count", "full/one.sample_count",
            "set 0 becomes full and set 1 becomes one");
        b.retained(
            "channel_sets[].compression", "full/one.compression",
            "set 0 becomes full and set 1 becomes one");
        b.translated(
            "channel_sets[].frame_size", "full/one.counts",
            "frame size is recomputed from channel suffixes and compression; "
            "category-prefix counts are synthesized in GH2 native order");
        b.retained(
            "channel_sets[].sample_bytes", "full/one.sample_bytes",
            "validated compressed sample payload is copied byte-for-byte");
        b.discarded(
            "trailing_bytes",
            "exact revision-18/5 ACP requires this field to be empty; "
            "non-empty residuals are rejected");
        b.synthesized(
            "char_clip_revision",
            "native embedded CharClip base is revision 5");
        b.synthesized(
            "range/legacy_flag/legacy_enter_event/legacy_exit_event/events",
            "GH2-only clip fields retain native empty/default values");
        b.synthesized(
            "transitions",
            "native clip transitions are compiled from the owned GH1 ACG "
            "graph, not guessed from asset names");
        b.synthesized(
            "duplicate",
            "GH2 revisions 8-12 require a discarded third header; it carries "
            "full-set compression/sample count and no channels or data");
    }
    {
        ContractBuilder b(rows, "Cam", 9, "Cam", "12");
        b.transformable();
        b.drawable(false);
        b.retained_fields(
            {"near_plane", "far_plane", "screen_rect", "z_range",
             "target_texture"});
        b.translated(
            "fov", "y_fov",
            "pre-revision-12 authored 4:3 horizontal FOV is converted to "
            "vertical FOV using 2*atan(0.75*tan(fov/2))");
    }
    {
        ContractBuilder b(rows, "CamAnim", 0, "CamAnim", "2");
        b.animatable();
        b.retained_fields(
            {"camera", "fov_keys[].value", "fov_keys[].frame",
             "keys_owner"});
    }
    {
        ContractBuilder b(rows, "Environ", 1, "Environ", "5");
        b.drawable(false, "legacy_drawable.", "drawable.");
        b.retained_fields(
            {"lights", "ambient_color", "fog_range", "fog_color",
             "fog_enabled"});
        b.synthesized(
            "animate_from_preset",
            "native GH2 environment constructor default is retained");
        b.synthesized(
            "fade_out/fade_start/fade_end",
            "GH2-only fade state uses native constructor defaults");
    }
    {
        ContractBuilder b(rows, "EnvAnim", 3, "EnvAnim", "4");
        b.animatable();
        b.retained_fields(
            {"environment", "ambient_color_keys[].value",
             "ambient_color_keys[].frame", "keys_owner",
             "fog_color_keys[].value", "fog_color_keys[].frame",
             "fog_range_keys[].value", "fog_range_keys[].frame"});
    }
    {
        ContractBuilder b(rows, "Flare", 3, "Flare", "4");
        b.transformable();
        b.drawable();
        b.retained_fields({"material", "sizes", "range", "steps"});
    }
    {
        ContractBuilder b(rows, "Font", 7, "Font", "15");
        b.retained_fields(
            {"material", "cell_size", "deprecated_size", "base_kerning",
             "has_kerning_table", "kerning[].packed_char_pair",
             "kerning[].kerning"});
        b.translated(
            "characters", "characters",
            "character ordering is retained; legacy leading 0xA0 is "
            "normalized to a space by the GH2 compatibility rule");
        b.synthesized(
            "texture_owner",
            "native texture owner is the source Font object name");
        b.synthesized(
            "monospace/packed",
            "GH1 grid fonts use native false defaults");
        b.synthesized(
            "bitmap_width/bitmap_height/texture_cell_size",
            "decoded referenced texture dimensions and source cell size "
            "produce native atlas metrics");
        b.synthesized(
            "character_info[256]",
            "per-character UV, width, and advance are measured from decoded "
            "source texture alpha using the GH1 grid");
    }
    {
        ContractBuilder b(rows, "Light", 3, "Light", "6");
        b.transformable();
        b.retained_fields({"color", "range", "serialized_type"});
        b.synthesized(
            "animate_color_from_preset/animate_position_from_preset",
            "GH2-only preset flags retain native constructor defaults");
    }
    {
        ContractBuilder b(rows, "LightAnim", 1, "LightAnim", "2");
        b.animatable();
        b.retained_fields(
            {"light", "color_keys[].value", "color_keys[].frame",
             "keys_owner"});
    }
    {
        ContractBuilder b(rows, "Mat", 21, "Mat", "27");
        b.translated(
            "textures[].stage_blend", "blend/intensify/next_pass",
            "retail revision-21 upgrade selects the root pass behavior and "
            "expands later stages to deterministic native pass materials");
        b.retained(
            "textures[].tex_gen", "tex_gen",
            "each source stage maps to its corresponding native material "
            "pass");
        b.retained(
            "textures[].transform", "texture_transform",
            "each source stage maps to its corresponding native material "
            "pass");
        b.retained(
            "textures[].wrap", "tex_wrap",
            "each source stage maps to its corresponding native material "
            "pass");
        b.retained(
            "textures[].texture", "diffuse_texture",
            "each source stage maps to its corresponding native material "
            "pass");
        b.retained_fields(
            {"blend", "color", "use_environment", "cull", "z_mode",
             "alpha_cut", "alpha_write"});
        b.translated(
            "vertex_ambient", "prelit",
            "legacy ambient-vertex lighting contributes to native prelit "
            "state using the revision upgrader rule");
        b.discarded(
            "vertex_dynamic",
            "GH2 Mat27 has no serialized vertex-dynamic field; the retail "
            "GH2 revision-21 loader reads the third legacy lighting boolean "
            "at SLUS_214.47:0x001C002C and stores no target member",
            "GH2 SLUS_214.47 revision-21 loader trace; packed value "
            "distribution; target semantic round trip");
        b.translated(
            "multipass", "color/use_environment/prelit",
            "MultipassSrc resets non-root passes to unlit white");
        b.discarded(
            "normalize",
            "retail revision-21 loader consumes this obsolete field without "
            "a target serialization",
            "GH2 SLUS_214.47:0x001C02E4 revision-21 loader trace; packed "
            "value distribution; target semantic round trip");
        b.synthesized(
            "next_pass",
            "multi-stage source material is expanded to a deterministic "
            "Mat27 linked pass chain");
        b.synthesized(
            "emissive/specular/normal-map/fur fields",
            "GH2-only material features retain native constructor defaults");
    }
    {
        ContractBuilder b(rows, "MatAnim", 5, "MatAnim", "7");
        b.animatable();
        b.retained_fields(
            {"material", "keys_owner", "color_keys[].value",
             "color_keys[].frame", "alpha_keys[].value",
             "alpha_keys[].frame"});
        b.translated(
            "stages[].translation_keys[].value", "translation_keys[].value",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].translation_keys[].frame", "translation_keys[].frame",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].scale_keys[].value", "scale_keys[].value",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].scale_keys[].frame", "scale_keys[].frame",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].rotation_keys[].value", "rotation_keys[].value",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].rotation_keys[].frame", "rotation_keys[].frame",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].texture_keys[].object", "texture_keys[].object",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
        b.translated(
            "stages[].texture_keys[].frame", "texture_keys[].frame",
            "each legacy material stage is emitted as the corresponding "
            "native MatAnim pass");
    }
    {
        ContractBuilder b(rows, "Mesh", 25, "Mesh", "28");
        b.transformable();
        b.drawable();
        b.retained_fields(
            {"material", "geometry_owner", "mutable_flags", "volume",
             "vertices[].position", "vertices[].normal",
             "vertices[].color_or_weights", "vertices[].uv", "faces",
             "has_bones", "bone_slots[].bone", "bone_slots[].offset"});
        b.translated(
            "has_bsp_tree", "bsp_nodes",
            "all packed GH1 bodies carry a null BSP; native Mesh28 receives "
            "its null BSP-root node and rejects any non-null source tree");
        b.translated(
            "patches", "group_sizes",
            "legacy byte patch sizes are the native Mesh28 group sizes");
        b.translated(
            "strip_results[].cumulative_strip_lengths",
            "group_sections[].cumulative_strip_lengths",
            "PS2 platform strip cache rows are retained per group");
        b.translated(
            "strip_results[].strip_runs",
            "group_sections[].strip_runs",
            "PS2 platform strip cache rows are retained per group");
    }
    {
        ContractBuilder b(rows, "MeshAnim", 0, "MeshAnim", "1");
        b.animatable();
        b.retained_fields(
            {"mesh", "point_keys[].values", "point_keys[].frame",
             "texcoord_keys[].values", "texcoord_keys[].frame",
             "color_keys[].values", "color_keys[].frame", "keys_owner"});
    }
    {
        ContractBuilder b(rows, "Morph", 3, "Morph", "4");
        b.animatable();
        b.retained_fields(
            {"poses[].mesh", "poses[].keys[].value",
             "poses[].keys[].frame", "target", "normals", "spline",
             "intensity"});
    }
    {
        ContractBuilder b(rows, "Movie", 6, "Movie", "8");
        b.animatable();
        b.retained_fields({"file", "texture", "stream", "loop"});
    }
    {
        ContractBuilder b(rows, "MultiMesh", 0, "MultiMesh", "1");
        b.drawable();
        b.retained_fields({"mesh", "transforms"});
    }
    {
        ContractBuilder b(rows, "ParticleSys", 22, "ParticleSys", "27");
        b.animatable();
        b.transformable();
        b.drawable();
        b.retained_fields(
            {"life", "box_extent_1", "box_extent_2", "speed", "pitch",
             "yaw", "emit_rate", "start_size", "delta_size",
             "start_color_low", "start_color_high", "end_color_low",
             "end_color_high", "force_direction", "material", "type",
             "grow_ratio", "shrink_ratio", "mid_color_ratio",
             "mid_color_low", "mid_color_high", "max_particles",
             "bubble_period", "bubble_size", "bubble", "relative_motion",
             "emitter_mesh", "preserve_particles", "particles[].position",
             "particles[].color", "particles[].size"});
        b.translated(
            "bounce_enabled", "bounce",
            "enabled finite nonzero legacy plane binds a synthesized native "
            "Trans; disabled or zero-normal planes remain unbound");
        b.translated(
            "bounce_plane", "Trans.local",
            "finite legacy plane normal and distance become a native bounce "
            "transform using the source-backed revision-26 contract");
        b.synthesized(
            "relative_parent",
            "GH1 has no relative-parent reference; native default is empty");
    }
    {
        ContractBuilder b(
            rows, "ParticleSysAnim", 2, "ParticleSysAnim", "3");
        b.animatable();
        b.retained_fields(
            {"particle_system", "start_color_keys[].value",
             "start_color_keys[].frame", "end_color_keys[].value",
             "end_color_keys[].frame", "emit_rate_keys[].value",
             "emit_rate_keys[].frame", "keys_owner",
             "speed_keys[].value", "speed_keys[].frame",
             "life_keys[].value", "life_keys[].frame",
             "start_size_keys[].value", "start_size_keys[].frame"});
    }
    {
        ContractBuilder b(rows, "Tex", 8, "Tex", "10");
        b.retained_fields(
            {"width", "height", "bits_per_pixel", "external_path",
             "mipmap_bias", "type", "use_external", "has_bitmap",
             "bitmap.header_kind", "bitmap.bits_per_pixel",
             "bitmap.encoding", "bitmap.mipmap_count", "bitmap.width",
             "bitmap.height", "bitmap.bytes_per_line", "bitmap.wii_alpha",
             "bitmap.reserved", "bitmap.data"});
    }
    {
        ContractBuilder b(rows, "Text", 15, "Text", "17");
        b.drawable();
        b.transformable();
        b.retained_fields(
            {"font", "alignment", "text", "color", "wrap_width", "leading",
             "fixed_length", "italics", "size", "markup", "caps_mode"});
    }
    {
        ContractBuilder b(rows, "TransAnim", 4, "TransAnim", "6");
        b.animatable();
        b.drawable(false);
        b.retained_fields(
            {"target", "rotation_keys[].value", "rotation_keys[].frame",
             "translation_keys[].value", "translation_keys[].frame",
             "keys_owner", "translation_spline", "repeat_translation",
             "scale_keys[].value", "scale_keys[].frame", "scale_spline",
             "follow_path", "rotation_slerp"});
    }
    {
        ContractBuilder b(rows, "View", 7, "Group", "12|13");
        b.animatable();
        b.transformable();
        b.drawable();
        b.translated(
            "children_owner", "objects/environment/draw_only",
            "owner View selects the independent legacy animation and "
            "ordered drawable list used to build native Group membership "
            "and environment scopes");
        b.discarded(
            "showing_range",
            "all packed source values must be the default (0,0); non-default "
            "ranges are rejected until a source-backed controller exists");
        b.synthesized(
            "environment",
            "a single resolved legacy Environ becomes the native Group "
            "environment; ordered changes are preserved by deterministic "
            "environment-scoped child Groups");
        b.synthesized(
            "environment_scope_groups",
            "Views with more than one ordered Environ scope emit one native "
            "child Group per scope and a draw_only traversal that preserves "
            "their source order");
        b.synthesized(
            "draw_only",
            "revision-13 companion Group is emitted only when animation "
            "membership contains drawables absent from the draw list");
        b.synthesized(
            "lod/lod_screen_size",
            "GH2-only LOD link fields retain native constructor defaults");
    }

    return rows;
}

}  // namespace

const std::vector<SemanticFieldContract>&
gh1_to_gh2_semantic_field_contracts() {
    static const std::vector<SemanticFieldContract> contracts =
        build_contracts();
    return contracts;
}

std::vector<SemanticFieldContract>
gh1_to_gh2_semantic_field_contracts_for(
    const std::string& source_type, uint32_t source_revision) {
    std::vector<SemanticFieldContract> result;
    for (const auto& row : gh1_to_gh2_semantic_field_contracts()) {
        if (row.source_type == source_type &&
            row.source_revision == source_revision)
            result.push_back(row);
    }
    if (result.empty() && source_type == "DTB") {
        for (const auto& row : gh1_to_gh2_semantic_field_contracts()) {
            if (row.source_type != "DTB" ||
                row.source_revision != kAnyDtbControlWord)
                continue;
            result.push_back(row);
            result.back().source_revision = source_revision;
        }
    }
    return result;
}

std::vector<std::string> gh1_serialized_semantic_fields_for(
    const std::string& source_type, uint32_t source_revision) {
    static const SchemaMap schemas = build_source_schemas();
    const auto found =
        schemas.find({source_type, source_revision});
    if (found != schemas.end()) return found->second;
    if (source_type == "DTB") {
        const auto fallback =
            schemas.find({source_type, kAnyDtbControlWord});
        if (fallback != schemas.end()) return fallback->second;
    }
    return {};
}

}  // namespace gh::milo_convert
