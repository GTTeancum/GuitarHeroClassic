# gh2test - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(gh2test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
# Original (clean baseline) sources: just src/main.cpp + src/gh2test_app.h.
# Trace-360 additions for instrumentation:
#   src/trace_recorder.{cpp,h} -- bounded jsonl-format event recorder
#   src/trace_hooks.cpp        -- REX_HOOK wrappers feeding the recorder
set(GH2TEST_SOURCES
    src/main.cpp
    src/trace_recorder.cpp
    src/trace_hooks.cpp
    src/autoplay_hook.cpp
    src/bone_capture_hook.cpp
)

if(WIN32)
    add_executable(gh2test WIN32 ${GH2TEST_SOURCES})
else()
    add_executable(gh2test ${GH2TEST_SOURCES})
endif()

rexglue_setup_target(gh2test)

# Enable autoplay / no-fail for sustained trace captures.
# This hooks GemPass::VtableDispatch (sub_8234A598) and forces r5=1 (HIT)
# on every gem before Scoring/StarPower vtable[16] callbacks fire.
# See src/autoplay_hook.cpp for implementation.
target_compile_definitions(gh2test PRIVATE AUTOPLAY_ENABLED)
