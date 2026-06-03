# engine/src/script/CMakeLists.txt
#
# ghogx_script: the Harmonix Sandbox DataArray script interpreter the menu UI
# runs on. Interprets directly off the tools/dtb parse tree (tags preserved).
# Pure logic -- no Windows/D3D dependency, so it stays OG-Xbox-portable.

add_library(ghogx_script STATIC
    interp.cpp     interp.h
    preprocess.cpp preprocess.h
)
# Headers included as "core/..." / "script/..." -> root is engine/src.
target_include_directories(ghogx_script PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# ghogx_core (DataNode/Object/Symbol) + dtb (gh::dtb::Node, brings its include dir).
target_link_libraries(ghogx_script PUBLIC ghogx_core dtb)

if(MSVC)
    target_compile_options(ghogx_script PRIVATE /W4)
else()
    target_compile_options(ghogx_script PRIVATE -Wall -Wextra)
endif()

add_executable(ghogx_script_test script_test.cpp)
target_link_libraries(ghogx_script_test PRIVATE ghogx_script)
if(MSVC)
    target_compile_options(ghogx_script_test PRIVATE /W4)
else()
    target_compile_options(ghogx_script_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_script_test COMMAND ghogx_script_test)
