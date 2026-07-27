#pragma once

#include "gh1_character_manifest.h"
#include "gh1_character_package.h"
#include "milo.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace gh::milo_convert {

struct Gh2CharacterAnimationBinding {
    Gh2ClipSetRole role = Gh2ClipSetRole::Generic;
    std::string source_archetype;
    std::string relative_path;
};

struct Gh1CharacterModelBuildInput {
    Gh1CharacterSpec spec;
    gh::milo::Directory source_model;
    std::optional<gh::milo::Directory> shadow_model;
    std::optional<gh::milo::Directory> face_model;
    std::vector<Gh2CharacterAnimationBinding> animations;
};

struct Gh2CharacterModelPackage {
    std::string directory_name;
    gh::milo::Directory directory;
    std::vector<std::string> unresolved_dependencies;
    std::vector<std::string> generated_dependencies;
    size_t internal_reference_count = 0;
    size_t face_transition_count = 0;
    std::string face_controller_type;
    bool complete = false;
};

// Resolves every serialized in-directory object reference understood by the
// target render/character object schemas. Returns the number of non-null
// references and rejects duplicate names or dangling references.
size_t validate_gh2_directory_references(
    const gh::milo::Directory& directory);

size_t validate_gh2_character_model_references(
    const gh::milo::Directory& directory);

// Builds the native Character/BandCharacter model graph directly from the
// compiled GH1 character facts and the semantically converted GH1 RndDir.
// Animation bindings are accepted only when their authored archetype is the
// character's exact compiled skeleton path.
Gh2CharacterModelPackage
convert_gh1_character_to_gh2_model_package(
    const Gh1CharacterModelBuildInput& input);

}  // namespace gh::milo_convert
