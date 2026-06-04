# ghogx_character — skinned character mesh decode + bind-pose/animated renderer.
# Decodes BandCharacter PS2 MILOs (bone skeleton + skinned meshes + materials),
# provides CharRenderer for D3D9 display. Win32-only (D3D9 dependency).

if(NOT WIN32)
    return()
endif()

add_library(ghogx_character STATIC
    char_mesh.cpp
    char_mesh.h
    char_facefx.cpp
    char_facefx.h
    char_renderer.cpp
    char_renderer.h
    char_clip.cpp
    char_clip.h
)
target_include_directories(ghogx_character PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(ghogx_character PUBLIC
    ghogx_milo_scene
    ghogx_asset
    ark_v3 dtb ps2_texture milo
    d3d9
)
if(MSVC)
target_compile_options(ghogx_character PRIVATE /W4)
else()
    target_compile_options(ghogx_character PRIVATE -Wall -Wextra)
endif()
