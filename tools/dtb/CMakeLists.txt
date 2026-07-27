cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(dtb_tool LANGUAGES CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows_toolchain_guard.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windows_toolchain_guard.cmake")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(dtb STATIC
    dtb.cpp dtb.h
    dtb_preprocess.cpp dtb_preprocess.h
)
target_include_directories(dtb PUBLIC .)

add_executable(dtb_tool dtb_tool.cpp)
target_link_libraries(dtb_tool PRIVATE dtb)
if(COMMAND ghogx_add_windows_import_guard)
    ghogx_add_windows_import_guard(dtb_tool)
endif()

add_executable(dtb_test dtb_test.cpp)
target_link_libraries(dtb_test PRIVATE dtb)
add_test(NAME dtb_test COMMAND dtb_test)
if(COMMAND ghogx_add_windows_import_guard)
    ghogx_add_windows_import_guard(dtb_test)
endif()
