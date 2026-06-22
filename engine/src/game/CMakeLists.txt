# engine/src/game/CMakeLists.txt
#
# ghogx_game: song gameplay system (highway renderer, audio player, state machine).
# Windows-only (D3D9 + XAudio2).

add_library(ghogx_game STATIC
    highway_renderer.cpp highway_renderer.h
    audio_player.cpp     audio_player.h
    gameplay.cpp         gameplay.h
    ../catalog.cpp       ../catalog.h
)

target_include_directories(ghogx_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ghogx_chart for the MIDI/chart types.
# ghogx_render for Window + D3D9.
# ark_v3 + vgs for asset loading inside gameplay/audio_player.
target_link_libraries(ghogx_game PUBLIC
    ghogx_chart
    ghogx_render
    ghogx_asset
    ghogx_character
    ghogx_script
    ark_v3
    dtb
    milo
    vgs
)

# XAudio2 is available on Windows 8+ SDK as a system DLL; link against the
# import library. Older SDK may need xaudio2.lib separately.
target_link_libraries(ghogx_game PRIVATE xaudio2)

if(MSVC)
    target_compile_options(ghogx_game PRIVATE /W4)
else()
    target_compile_options(ghogx_game PRIVATE -Wall -Wextra)
endif()

# Manual end-to-end smoke test for the streaming audio player (not a CTest;
# needs real ARK assets). Links only ghogx_game, so it does not pull in the
# character module.
add_executable(audio_smoke audio_smoke.cpp)
target_link_libraries(audio_smoke PRIVATE ghogx_game)

add_executable(ghogx_gameplay_venue_band_contract_test
    gameplay_venue_band_contract_test.cpp)
target_compile_definitions(ghogx_gameplay_venue_band_contract_test PRIVATE
    GHOGX_GAME_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    GHOGX_ASSET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../asset"
    GHOGX_CHART_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../chart"
    GHOGX_MILO_SCENE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../milo_scene"
    GHOGX_RENDER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../render")
add_test(NAME ghogx_gameplay_venue_band_contract_test
         COMMAND ghogx_gameplay_venue_band_contract_test)

if(MSVC)
    target_compile_options(ghogx_gameplay_venue_band_contract_test PRIVATE /W4)
else()
    target_compile_options(ghogx_gameplay_venue_band_contract_test PRIVATE
        -Wall -Wextra)
endif()
