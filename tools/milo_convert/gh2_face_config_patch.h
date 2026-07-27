#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gh::milo_convert {

struct Gh2FaceConfigPatch {
    std::vector<uint8_t> bytes;
    std::string dta;
    size_t types_added = 0;
};

struct Gh2FaceMidiParserPatch {
    std::vector<uint8_t> bytes;
    std::string dta;
    size_t parsers_added = 0;
};

struct Gh2CharacterFaceConfigPatch {
    std::vector<uint8_t> bytes;
    std::string dta;
    size_t handlers_added = 0;
};

// Adds the two generic Group type scripts consumed by converted GH1 face
// graphs to a clean GH2 rnd_objects.dtb. The patch preserves the source DTB
// storage/cipher form and does not replace any existing Group type.
Gh2FaceConfigPatch patch_gh2_rnd_objects_for_gh1_faces(
    const std::vector<uint8_t>& clean_rnd_objects);

// Adds a target-native MIDI parser for the generated GH1 SINGER FACE track.
// Pitch 108 opens the two-pose singer face and pitch 109 closes it. Both are
// zero-length events so the MIDI stream, not a runtime timer, owns timing.
Gh2FaceMidiParserPatch patch_gh2_midi_parsers_for_gh1_singer_face(
    const std::vector<uint8_t>& clean_midi_parsers);

// Adds two generic singer handlers to the target Character singer type. The
// stock singer_parser
// remains the message source; the handlers forward only to the local
// lip.servo, so stock GH2 singers and converted GH1 singers can coexist.
Gh2CharacterFaceConfigPatch
patch_gh2_char_objects_for_gh1_singer_face(
    const std::vector<uint8_t>& clean_char_objects);

// Human-readable source for the generated type rows.
std::string gh1_face_controller_types_dta();
std::string gh1_singer_face_midi_parser_dta();

}  // namespace gh::milo_convert
