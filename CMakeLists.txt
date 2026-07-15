# gh2test - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(gh2test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(generated/rexglue.cmake)

# Native PS2 ARK reader -- reused from the standalone engine tools so the
# recompile and the headless engine share one source of truth.
add_subdirectory(tools/ark ${CMAKE_BINARY_DIR}/_tools_ark)

# Sources
set(GH2TEST_SOURCES
    src/main.cpp
    src/trap_diag.cpp
    src/ps2_ark_hook.cpp
)

if(WIN32)
    add_executable(gh2test WIN32 ${GH2TEST_SOURCES})
else()
    add_executable(gh2test ${GH2TEST_SOURCES})
endif()

rexglue_setup_target(gh2test)
target_link_libraries(gh2test PRIVATE ark_v3)
