#pragma once

#include "milo.h"
#include "acg.h"
#include "acp.h"
#include "milo_object.h"

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

Result convert_gh1_directory_to_gh2_rnddir(
    const gh::milo::Directory& source,
    const std::string& target_directory_name);

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
