# engine/src/milo_bridge/CMakeLists.txt
#
# ghogx_milo_bridge: load a PS2 MILO container's object directory into the
# runtime ObjectDir tree (structural pass). Bridges tools/milo (gh::milo) +
# tools/ark (gh::ark) into ghogx_core. milo is PUBLIC because gh::milo::Directory
# appears in the public API; ark_v3 is a private impl detail of load_object_dir.

add_library(ghogx_milo_bridge STATIC
    milo_bridge.cpp milo_bridge.h
)
# Headers included as "core/..." and "milo_bridge/..." -> root is engine/src.
target_include_directories(ghogx_milo_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ghogx_milo_bridge PUBLIC ghogx_core milo PRIVATE ark_v3)

if(MSVC)
    target_compile_options(ghogx_milo_bridge PRIVATE /W4)
else()
    target_compile_options(ghogx_milo_bridge PRIVATE -Wall -Wextra)
endif()

add_executable(ghogx_milo_bridge_test milo_bridge_test.cpp)
target_link_libraries(ghogx_milo_bridge_test PRIVATE ghogx_milo_bridge)
if(MSVC)
    target_compile_options(ghogx_milo_bridge_test PRIVATE /W4)
else()
    target_compile_options(ghogx_milo_bridge_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_milo_bridge_test COMMAND ghogx_milo_bridge_test)
