cmake_minimum_required(VERSION 3.20)
project(vgs_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vgs STATIC vgs.cpp vgs.h)
target_include_directories(vgs PUBLIC .)

add_executable(vgs_tool vgs_tool.cpp)
target_link_libraries(vgs_tool PRIVATE vgs)
