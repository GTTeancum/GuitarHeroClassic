cmake_minimum_required(VERSION 3.20)
project(dtb_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dtb STATIC dtb.cpp dtb.h)
target_include_directories(dtb PUBLIC .)

add_executable(dtb_tool dtb_tool.cpp)
target_link_libraries(dtb_tool PRIVATE dtb)
