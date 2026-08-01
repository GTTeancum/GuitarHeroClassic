cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(tex_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows_toolchain_guard.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows_toolchain_guard.cmake")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(ps2_texture STATIC ps2_texture.cpp ps2_texture.h)
target_include_directories(ps2_texture PUBLIC .)

add_executable(tex_tool tex_tool.cpp)
target_sources(tex_tool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/src/milo_tex.cpp
)
target_include_directories(tex_tool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/src
)
target_link_libraries(tex_tool PRIVATE ps2_texture)
if(COMMAND ghogx_add_windows_import_guard)
    ghogx_add_windows_import_guard(tex_tool)
endif()

add_executable(ps2_texture_test ps2_texture_test.cpp)
target_link_libraries(ps2_texture_test PRIVATE ps2_texture)
add_test(NAME ps2_texture_test COMMAND ps2_texture_test)
if(COMMAND ghogx_add_windows_import_guard)
    ghogx_add_windows_import_guard(ps2_texture_test)
endif()
