cmake_minimum_required(VERSION 3.20)
project(milo_tool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MINIZ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/miniz)

add_library(miniz_tinfl STATIC ${MINIZ_DIR}/miniz_tinfl.c)
# CMAKE_CURRENT_SOURCE_DIR is searched before MINIZ_DIR so miniz_common.h's
# `#include "miniz_export.h"` falls through to our local stub instead of
# requiring the generated header from miniz's own CMake build.
target_include_directories(miniz_tinfl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MINIZ_DIR})
target_compile_definitions(miniz_tinfl PUBLIC
    MINIZ_NO_ARCHIVE_APIS
    MINIZ_NO_ARCHIVE_WRITING_APIS
    MINIZ_NO_TIME
    MINIZ_NO_STDIO)

add_library(milo STATIC milo.cpp milo.h)
target_include_directories(milo PUBLIC .)
target_link_libraries(milo PRIVATE miniz_tinfl)

add_executable(milo_tool milo_tool.cpp)
target_link_libraries(milo_tool PRIVATE milo)

add_executable(milo_test milo_test.cpp)
target_link_libraries(milo_test PRIVATE milo)
add_test(NAME milo_test COMMAND milo_test)
