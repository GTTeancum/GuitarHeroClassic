# engine/src/hud/CMakeLists.txt
#
# ghogx_hud: the in-song gameplay HUD (score / streak / multiplier / star-power
# meter / rock-crowd meter), rendered as a 2-D screen-space overlay built from
# the game's own hud/gen/*.milo_ps2 panels. Windows-only (D3D9).
#
# Self-contained: owns its MILO parse + texture upload. Reuses ghogx_milo_scene
# for the Mesh/Mat byte decoders and ghogx_asset for ARK->Tex->RGBA.

add_library(ghogx_hud STATIC
    hud_renderer.cpp hud_renderer.h
)

# Headers included as "hud/..." -> root is engine/src.
target_include_directories(ghogx_hud PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(ghogx_hud PUBLIC
    ghogx_milo_scene   # decode_mesh / decode_mat / Xfm
    ghogx_asset        # load_milo_textures (ARK -> MILO -> Tex -> RGBA)
    ark_v3
    milo
)

if(MSVC)
    target_compile_options(ghogx_hud PRIVATE /W4)
else()
    target_compile_options(ghogx_hud PRIVATE -Wall -Wextra)
endif()
