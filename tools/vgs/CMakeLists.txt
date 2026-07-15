cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(vgs_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows_toolchain_guard.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows_toolchain_guard.cmake")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(vgs STATIC vgs.cpp vgs.h)
target_include_directories(vgs PUBLIC .)

add_executable(vgs_tool vgs_tool.cpp)
target_link_libraries(vgs_tool PRIVATE vgs)
if(COMMAND ghogx_add_windows_import_guard)
    ghogx_add_windows_import_guard(vgs_tool)
endif()

# Streaming-vs-wholefile correctness test. Registered with CTest; the engine
# build calls enable_testing(), so `ctest` runs it alongside the engine tests.
add_executable(vgs_test vgs_test.cpp)
target_link_libraries(vgs_test PRIVATE vgs)
add_test(NAME vgs_test COMMAND vgs_test)
if(COMMAND ghogx_add_windows_import_guard)
    ghogx_add_windows_import_guard(vgs_test)
endif()
