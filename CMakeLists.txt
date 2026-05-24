# gh2test - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(gh2test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(GH2TEST_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(gh2test WIN32 ${GH2TEST_SOURCES})
else()
    add_executable(gh2test ${GH2TEST_SOURCES})
endif()

rexglue_setup_target(gh2test)
