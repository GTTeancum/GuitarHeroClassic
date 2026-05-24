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

set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tools)

add_subdirectory(${TOOLS_DIR}/ark         ${CMAKE_BINARY_DIR}/_tools_ark)
add_subdirectory(${TOOLS_DIR}/dtb         ${CMAKE_BINARY_DIR}/_tools_dtb)
add_subdirectory(${TOOLS_DIR}/texture_ps2 ${CMAKE_BINARY_DIR}/_tools_tex)
add_subdirectory(${TOOLS_DIR}/vgs         ${CMAKE_BINARY_DIR}/_tools_vgs)
add_subdirectory(${TOOLS_DIR}/milo        ${CMAKE_BINARY_DIR}/_tools_milo)

# ---- headless catalog/extraction tool -------------------------------------

add_executable(ghogx
    src/main.cpp
    src/catalog.cpp
    src/catalog.h
    src/venue_catalog.cpp
    src/venue_catalog.h
    src/character_catalog.cpp
    src/character_catalog.h
    src/milo_tex.cpp
    src/milo_tex.h
)
target_link_libraries(ghogx PRIVATE ark_v3 dtb ps2_texture vgs milo)
if(MSVC)
    target_compile_options(ghogx PRIVATE /W4)
else()
    target_compile_options(ghogx PRIVATE -Wall -Wextra)
endif()

# ---- Win32 audiovisual viewer ---------------------------------------------

if(WIN32)
    add_executable(ghogx_viewer src/viewer.cpp)
    target_link_libraries(ghogx_viewer PRIVATE
        ark_v3 ps2_texture vgs
        winmm gdi32 user32
    )
    if(MSVC)
        target_compile_options(ghogx_viewer PRIVATE /W4)
    else()
        target_compile_options(ghogx_viewer PRIVATE -Wall -Wextra)
    endif()
endif()
