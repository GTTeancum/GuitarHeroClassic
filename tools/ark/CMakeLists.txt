cmake_minimum_required(VERSION 3.20)
project(ark_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ark_v3 STATIC ark_v3.cpp ark_v3.h)
target_include_directories(ark_v3 PUBLIC .)

add_executable(ark_tool ark_tool.cpp)
target_link_libraries(ark_tool PRIVATE ark_v3)
