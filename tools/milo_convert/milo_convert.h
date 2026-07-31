#pragma once

#include "milo.h"
#include "acg.h"
#include "acp.h"
#include "milo_object.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gh::milo_convert {

struct ManifestRow {
    std::string source_type;
    std::string source_name;
    std::string target_type;
    std::string target_name;
    std::string status;
    std::string detail;
};

struct Result {
    gh::milo::Directory directory;
    std::vector<ManifestRow> manifest;
    bool complete = false;
};

// Schema-level accounting for every serialized GH1 field consumed by the
// native GH2 object converter. A row may also describe a target-only field
// when source_field is "<synthesized>".
struct SemanticFieldContract {
    std::string source_type;
    uint32_t source_revision = 0;
    std::string source_field;
    std::string target_type;
    std::string target_revision;
    std::string target_field;
    std::string disposition;
    std::string rule;
    std::string verification;
};

const std::vector<SemanticFieldContract>&
gh1_to_gh2_semantic_field_contracts();

std::vector<SemanticFieldContract>
gh1_to_gh2_semantic_field_contracts_for(
    const std::string& source_type, uint32_t source_revision);

// Canonical semantic field list implied by the revision-aware GH1 body
// readers. Kept independent from the conversion table so tests and the packed
// audit can reject a superficially complete table that omits a source field.
std::vector<std::string> gh1_serialized_semantic_fields_for(
    const std::string& source_type, uint32_t source_revision);

Result convert_gh1_directory_to_gh2_rnddir(
    const gh::milo::Directory& source,
    const std::string& target_directory_name,
    const std::string& authored_draw_root = {});

gh::milo_object::CharClipSamples10
convert_gh1_acp_to_gh2_char_clip_samples10(
    const gh::acp::File& source,
    const std::vector<gh::milo_object::CharClipTransition5>&
        transitions = {});

std::vector<std::vector<gh::milo_object::CharClipTransition5>>
convert_gh1_acg_to_gh2_char_clip_transitions(
    const gh::acg::Graph& source,
    const std::vector<std::string>& clip_names);

uint32_t convert_gh1_clip_time_flags_to_gh2(uint32_t source);

std::string manifest_tsv(const Result& result);

}  // namespace gh::milo_convert
