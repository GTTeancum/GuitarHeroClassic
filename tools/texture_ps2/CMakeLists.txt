cmake_minimum_required(VERSION 3.20)
project(tex_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ps2_texture STATIC ps2_texture.cpp ps2_texture.h)
target_include_directories(ps2_texture PUBLIC .)

add_executable(tex_tool tex_tool.cpp)
target_link_libraries(tex_tool PRIVATE ps2_texture)

add_executable(ps2_texture_test ps2_texture_test.cpp)
target_link_libraries(ps2_texture_test PRIVATE ps2_texture)
add_test(NAME ps2_texture_test COMMAND ps2_texture_test)
