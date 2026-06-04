# engine/src/chart/CMakeLists.txt
#
# ghogx_chart: MIDI SMF parser producing a Guitar Hero II note chart.
# Pure logic, no Windows dependency. Can be tested headlessly.

add_library(ghogx_chart STATIC
    midi_reader.cpp midi_reader.h
)
target_include_directories(ghogx_chart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(MSVC)
    target_compile_options(ghogx_chart PRIVATE /W4)
else()
    target_compile_options(ghogx_chart PRIVATE -Wall -Wextra)
endif()

# Unit test — hermetic, no ARK required.
add_executable(ghogx_chart_test midi_reader_test.cpp)
target_link_libraries(ghogx_chart_test PRIVATE ghogx_chart)
if(MSVC)
    target_compile_options(ghogx_chart_test PRIVATE /W4)
else()
    target_compile_options(ghogx_chart_test PRIVATE -Wall -Wextra)
endif()

add_test(NAME midi_reader_test COMMAND ghogx_chart_test)
