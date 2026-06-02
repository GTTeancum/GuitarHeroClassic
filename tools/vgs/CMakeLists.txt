cmake_minimum_required(VERSION 3.20)
project(vgs_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vgs STATIC vgs.cpp vgs.h)
target_include_directories(vgs PUBLIC .)

add_executable(vgs_tool vgs_tool.cpp)
target_link_libraries(vgs_tool PRIVATE vgs)

# Streaming-vs-wholefile correctness test. Registered with CTest; the engine
# build calls enable_testing(), so `ctest` runs it alongside the engine tests.
add_executable(vgs_test vgs_test.cpp)
target_link_libraries(vgs_test PRIVATE vgs)
add_test(NAME vgs_test COMMAND vgs_test)
