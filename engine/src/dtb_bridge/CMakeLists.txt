# engine/src/dtb_bridge/CMakeLists.txt
#
# ghogx_dtb_bridge: one-way translation from the PS2 DTB parse tree (gh::dtb,
# tools/dtb) into the runtime value model (ghogx_core). Depends on both. This
# is the only place tools/ parse types meet the runtime types.

add_library(ghogx_dtb_bridge STATIC
    dtb_bridge.cpp dtb_bridge.h
)
# Headers included as "core/..." and "dtb_bridge/..." -> root is engine/src.
target_include_directories(ghogx_dtb_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# ghogx_core (runtime types) + dtb (PS2 DTB parser, brings its own include dir).
target_link_libraries(ghogx_dtb_bridge PUBLIC ghogx_core dtb)

if(MSVC)
    target_compile_options(ghogx_dtb_bridge PRIVATE /W4)
else()
    target_compile_options(ghogx_dtb_bridge PRIVATE -Wall -Wextra)
endif()

add_executable(ghogx_dtb_bridge_test dtb_bridge_test.cpp)
target_link_libraries(ghogx_dtb_bridge_test PRIVATE ghogx_dtb_bridge)
if(MSVC)
    target_compile_options(ghogx_dtb_bridge_test PRIVATE /W4)
else()
    target_compile_options(ghogx_dtb_bridge_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_dtb_bridge_test COMMAND ghogx_dtb_bridge_test)

# End-to-end foundation test (config -> factory -> scheduler -> frame loop).
add_executable(ghogx_integration_test integration_test.cpp)
target_link_libraries(ghogx_integration_test PRIVATE ghogx_dtb_bridge)
if(MSVC)
    target_compile_options(ghogx_integration_test PRIVATE /W4)
else()
    target_compile_options(ghogx_integration_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_integration_test COMMAND ghogx_integration_test)
