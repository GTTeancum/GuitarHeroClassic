#pragma once

#include <rex/types.h>
#include <cstdint>

namespace band3 {

struct BinStream {
	// pointer to vf table
    rex::be<uint32_t> vfTable;
	
	// whether or not the binstream is little endian
	// when this is true, it will automatically swap data that is read back to big endian
    bool littleEndian;

	// Rand2*
    rex::be<uint32_t> mCrypto;
};

}
