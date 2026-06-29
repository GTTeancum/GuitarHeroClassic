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

add_executable(ghogx_render_material_uv_test material_uv_sampler_test.cpp)
target_link_libraries(ghogx_render_material_uv_test PRIVATE ghogx_render)
add_test(NAME ghogx_render_material_uv_test
         COMMAND ghogx_render_material_uv_test)

add_executable(ghogx_render_camera_matrix_test camera_matrix_test.cpp)
target_link_libraries(ghogx_render_camera_matrix_test PRIVATE ghogx_render)
add_test(NAME ghogx_render_camera_matrix_test
         COMMAND ghogx_render_camera_matrix_test)

if(MSVC)
    target_compile_options(ghogx_render_material_uv_test PRIVATE /W4)
    target_compile_options(ghogx_render_camera_matrix_test PRIVATE /W4)
else()
    target_compile_options(ghogx_render_material_uv_test PRIVATE -Wall -Wextra)
    target_compile_options(ghogx_render_camera_matrix_test PRIVATE -Wall -Wextra)
endif()
