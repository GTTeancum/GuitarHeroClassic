# engine/src/render/CMakeLists.txt
#
# ghogx_render: the PC (D3D9) windowing + render-device layer. Windows-only;
# the OG-Xbox build supplies a D3D8/NV2A sibling later. Header is PIMPL so the
# engine can include it without dragging in windows.h.

add_library(ghogx_render STATIC
    window_d3d9.cpp window_d3d9.h
    scene_d3d9.cpp  scene_d3d9.h
    milo_scene_renderer.cpp milo_scene_renderer.h
)
target_include_directories(ghogx_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# milo_scene = the Trans/Mat/Mesh decoder; asset = RGBA texture loader (Image).
target_link_libraries(ghogx_render
    PUBLIC d3d9 user32 gdi32 xinput ghogx_milo_scene ghogx_asset)

if(MSVC)
    target_compile_options(ghogx_render PRIVATE /W4)
else()
    target_compile_options(ghogx_render PRIVATE -Wall -Wextra)
endif()
