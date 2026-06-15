#include "ark_v3.h"
#include "chart/midi_reader.h"
#include <cstdio>
#include <cstring>
int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: midi_diag <hdr> <ark>\n"); return 1; }
    auto ark = gh::ark::ArkV3Reader::load(argv[1]);
    auto entry = ark.find("songs/shoutatthedevil/shoutatthedevil.mid");
    if (!entry) { fprintf(stderr, "not found\n"); return 1; }
    auto bytes = ark.read_entry(*entry, {argv[2]});
    // Print first 300 bytes as hex + ascii to spot track names
    for (size_t i = 0; i < std::min(bytes.size(), (size_t)400); ++i) {
        if (i % 16 == 0) fprintf(stdout, "\n%04zx  ", i);
        fprintf(stdout, "%02x ", (unsigned)bytes[i]);
    }
    fprintf(stdout, "\n");
    // Also scan for ASCII strings >= 4 chars
    for (size_t i = 0; i+4 < bytes.size(); ) {
        if (bytes[i] >= 0x20 && bytes[i] < 0x7f) {
            size_t j = i;
            while (j < bytes.size() && bytes[j] >= 0x20 && bytes[j] < 0x7f) j++;
            if (j - i >= 4) fprintf(stdout, "@%zu: \"%.*s\"\n", i, (int)(j-i), bytes.data()+i);
            i = j+1;
        } else { i++; }
    }
    return 0;
}
