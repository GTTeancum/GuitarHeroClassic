# engine/src/asset/CMakeLists.txt
#
# ghogx_asset: load decoded RGBA images from PS2 MILO/Tex inside a PS2 ARK.
# Wraps the PS2-native readers (ark_v3 + milo + ps2_texture) and the Tex
# wrapper parser (milo_tex, shared with the catalog tool). The bridge that
# puts real PS2 game art on screen.

add_library(ghogx_asset STATIC
    milo_image.cpp milo_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../milo_tex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../milo_tex.h
)
# Headers included as "asset/..." and "milo_tex.h" -> root is engine/src.
target_include_directories(ghogx_asset PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ghogx_asset PUBLIC ark_v3 milo ps2_texture)

if(MSVC)
    target_compile_options(ghogx_asset PRIVATE /W4)
else()
    target_compile_options(ghogx_asset PRIVATE -Wall -Wextra)
endif()
