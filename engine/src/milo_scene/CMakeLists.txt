# engine/src/milo_scene/CMakeLists.txt
#
# ghogx_milo_scene: decode the 3-D render objects (Trans / Mat / Mesh) out of a
# GH2 PS2 MILO so the engine can draw real scene geometry (a venue stage, the
# note highway board + gems). Runtime-native: reads the .milo_ps2 from the ARK
# and decodes in memory. Wraps tools/ark (gh::ark) + tools/milo (gh::milo).

add_library(ghogx_milo_scene STATIC
    milo_scene.cpp milo_scene.h
)
# Headers included as "milo_scene/..." -> root is engine/src.
target_include_directories(ghogx_milo_scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ghogx_milo_scene PUBLIC milo PRIVATE ark_v3)

if(MSVC)
    target_compile_options(ghogx_milo_scene PRIVATE /W4)
else()
    target_compile_options(ghogx_milo_scene PRIVATE -Wall -Wextra)
endif()

add_executable(ghogx_milo_scene_test milo_scene_test.cpp)
target_link_libraries(ghogx_milo_scene_test PRIVATE ghogx_milo_scene)
if(MSVC)
    target_compile_options(ghogx_milo_scene_test PRIVATE /W4)
else()
    target_compile_options(ghogx_milo_scene_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_milo_scene_test COMMAND ghogx_milo_scene_test)
