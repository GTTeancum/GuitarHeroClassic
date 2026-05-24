// miniz_export.h - Stub for miniz built as a static library.
//
// miniz's CMake build normally generates this via GenerateExportHeader. We
// build miniz inline as a static lib with no shared visibility, so all the
// export-decoration macros expand to nothing. Placed in this directory so the
// preprocessor finds it via the include search path when miniz_common.h does
// `#include "miniz_export.h"` and the submodule directory doesn't have one.

#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#define MINIZ_EXPORT
#define MINIZ_NO_EXPORT
#define MINIZ_DEPRECATED
#define MINIZ_DEPRECATED_EXPORT
#define MINIZ_DEPRECATED_NO_EXPORT

#endif
