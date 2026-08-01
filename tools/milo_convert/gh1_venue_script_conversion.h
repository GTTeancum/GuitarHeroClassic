#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace gh::milo_convert {

struct Gh2VenueScriptConversion {
    std::vector<uint8_t> bytes;
    std::string dta;
    size_t source_roots = 0;
    size_t recognized_roots = 0;
    size_t unrecognized_roots = 0;
    std::vector<std::pair<std::string, std::string>> loaded_sections;
    std::vector<std::string> initialized_states;
    std::vector<std::string> handler_names;
    size_t source_functions = 0;
    size_t handlers = 0;
    size_t function_calls_inlined = 0;
    size_t foreach_loops_unrolled = 0;
    size_t switch_anim_calls = 0;
    size_t switch_anim_rt_calls = 0;
    size_t anim_task_calls = 0;
    size_t animate_to_calls = 0;
    size_t delay_task_calls = 0;
    size_t random_ranges_expanded = 0;
    size_t stateful_ranges_resolved = 0;
};

// Lowers a preprocessed GH1 Arena venue script into the GH2 WorldDir local
// type schema. GH1 functions are inlined, finite foreach lists are expanded,
// and Arena animation/task wrappers become native RndAnimatable::animate and
// script_task calls. The output retains the source DTB storage/cipher form.
Gh2VenueScriptConversion convert_gh1_venue_script_to_gh2_worlddir(
    const std::vector<uint8_t>& source_dtb,
    const std::string& venue);

}  // namespace gh::milo_convert
