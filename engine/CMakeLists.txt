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

enable_testing()

# ---- runtime core (Sandbox value/property primitives) ---------------------
# The native engine runtime, reimplemented fresh from the decoded spec. Pure
# logic, no tools/ or reference-binary dependency. Built + unit-tested first.
add_subdirectory(src/core)

set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tools)

add_subdirectory(${TOOLS_DIR}/ark         ${CMAKE_BINARY_DIR}/_tools_ark)
add_subdirectory(${TOOLS_DIR}/dtb         ${CMAKE_BINARY_DIR}/_tools_dtb)
add_subdirectory(${TOOLS_DIR}/texture_ps2 ${CMAKE_BINARY_DIR}/_tools_tex)
add_subdirectory(${TOOLS_DIR}/vgs         ${CMAKE_BINARY_DIR}/_tools_vgs)
add_subdirectory(${TOOLS_DIR}/milo        ${CMAKE_BINARY_DIR}/_tools_milo)

# ---- DTB -> runtime bridge (needs the dtb tool target above) --------------
add_subdirectory(src/dtb_bridge)

# ---- Sandbox DataArray script interpreter (drives the menu UI) ------------
# Pure logic (core + dtb only); no Windows dependency -> portable to OG Xbox.
add_subdirectory(src/script)

# ---- MILO -> ObjectDir bridge (needs the milo + ark tool targets above) ---
add_subdirectory(src/milo_bridge)

# ---- MILO 3-D render-object decoder (Trans/Mat/Mesh -> drawable geometry) --
add_subdirectory(src/milo_scene)

# ---- asset image loader (ARK -> MILO -> Tex -> RGBA; shares milo_tex) ------
add_subdirectory(src/asset)

# ---- MIDI chart parser (no Windows dependency; can be tested headlessly) ---
add_subdirectory(src/chart)

# ---- PC windowing/render (D3D9) + windowed app entrypoint -----------------
# (after src/asset so the app can link ghogx_asset for the splash image)
if(WIN32)
    add_subdirectory(src/render)
    # ---- Song gameplay system (highway + audio + state machine) --------------
    add_subdirectory(src/game)
    # ---- In-song gameplay HUD (score/streak/multiplier/SP/rock meter overlay) -
    add_subdirectory(src/hud)
    # ---- Menu UI: logic runtime (classes/manager/loader/game-side) + the
    #      windowed menu renderer (reuses the 3-D scene renderer) --------------
    add_subdirectory(src/ui)
    # ---- Skinned character mesh decode + D3D9 renderer -----------------------
    add_subdirectory(src/character)
    add_subdirectory(src/app)
endif()

# ---- headless catalog/extraction tool -------------------------------------

add_executable(ghogx
    src/main.cpp
    src/catalog.cpp
    src/catalog.h
    src/venue_catalog.cpp
    src/venue_catalog.h
    src/character_catalog.cpp
    src/character_catalog.h
)
# milo_tex now lives in ghogx_asset (shared with the app); link it for the
# tex-from-milo subcommand.
target_link_libraries(ghogx PRIVATE ghogx_asset ghogx_milo_bridge ghogx_milo_scene ark_v3 dtb ps2_texture vgs milo)
target_include_directories(ghogx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
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
