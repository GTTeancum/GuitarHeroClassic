#pragma once

#include <rex/types.h>
#include "DataNode.h"
#include "Symbol.h"

namespace band3 {

struct DataArray {
    // nodes in array
    rex::be<uint32_t> mNodes;

    // the file this array belongs to
    rex::be<uint32_t> mFile;

    // number of nodes in this array
    rex::be<short> mSize;

    // number of references
    rex::be<short> mRefs;

    // the line of the file this array is in
    rex::be<short> mLine;

    // unused in retail build
    rex::be<short> mDeprecated;
};

}
