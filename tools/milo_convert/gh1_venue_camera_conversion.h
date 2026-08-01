#pragma once

#include "milo.h"
#include "milo_object.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace gh::milo_convert {

struct Gh2VenueCameraConversion {
    gh::milo::Directory main_directory;
    size_t records = 0;
    size_t keyframes = 0;
    size_t shaky_records = 0;
    size_t adaptive_subdivisions = 0;
    double maximum_position_linearization_error = 0.0;
    double maximum_rotation_linearization_error = 0.0;
    double maximum_screen_linearization_error = 0.0;
    double maximum_fov_linearization_error = 0.0;
};

// Compile every GH1 VenueCam switch_cam record into revision-20 GH2 CamShots.
// The converted campaths directory supplies the already-converted TransAnim6
// source curves; no GH1 camera DTB is required by the emitted runtime bundle.
Gh2VenueCameraConversion convert_gh1_venue_cameras_to_gh2_camshots(
    const std::vector<uint8_t>& camera_dtb,
    const gh::milo::Directory& converted_main_directory,
    const gh::milo::Directory& converted_campaths_directory,
    const std::map<std::string, gh::milo_object::TransAnim6>&
        shared_animations);

}  // namespace gh::milo_convert
