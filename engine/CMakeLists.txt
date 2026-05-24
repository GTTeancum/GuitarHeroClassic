cmake_minimum_required(VERSION 3.20)
project(ghogx LANGUAGES C CXX)

# GuitarHeroOGX native engine.
#
# Reads PS2-format Harmonix ARKs directly via the readers under ../tools/.
# No dependency on the rexglue recompile -- the gh2test/360 binary is now
# reference material only. PC build first for dev iteration; OG Xbox toolchain
# is a follow-up target.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pull in the asset-reader static libs from tools/. Each tools/<fmt>/
# CMakeLists creates its own static library target (ark_v3, dtb,
# ps2_texture, vgs, milo). We add them as subdirectories with explicit
# binary dirs so they each get their own out-of-source build folder.
set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tools)

add_subdirectory(${TOOLS_DIR}/ark         ${CMAKE_BINARY_DIR}/_tools_ark)
add_subdirectory(${TOOLS_DIR}/dtb         ${CMAKE_BINARY_DIR}/_tools_dtb)
add_subdirectory(${TOOLS_DIR}/texture_ps2 ${CMAKE_BINARY_DIR}/_tools_tex)
add_subdirectory(${TOOLS_DIR}/vgs         ${CMAKE_BINARY_DIR}/_tools_vgs)
add_subdirectory(${TOOLS_DIR}/milo        ${CMAKE_BINARY_DIR}/_tools_milo)

add_executable(ghogx
    src/main.cpp
    src/catalog.cpp
    src/catalog.h
)

# Link the asset readers. The engine never opens raw files directly; all
# content flows through these.
target_link_libraries(ghogx PRIVATE
    ark_v3
    dtb
    ps2_texture
    vgs
    milo
)

if(MSVC)
    target_compile_options(ghogx PRIVATE /W4)
else()
    target_compile_options(ghogx PRIVATE -Wall -Wextra)
endif()
