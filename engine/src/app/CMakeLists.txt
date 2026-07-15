# engine/src/app/CMakeLists.txt
#
# ghogx_app: the windowed engine entrypoint (PC dev build). Drives the Engine
# loop against a real D3D9 window. Windows-only for now.

add_executable(ghogx_app app_main.cpp)
target_link_libraries(ghogx_app PRIVATE
    ghogx_core
    ghogx_render
    ghogx_asset
    ghogx_game        # highway renderer + audio + gameplay state machine
    ghogx_hud         # in-song HUD overlay (score/streak/mult/SP/rock meter)
    ghogx_character   # skinned character mesh decode + D3D9 renderer
    ghogx_ui_render   # windowed menu mode (--menu): boots the menu engine + draws
)

if(MSVC)
    target_compile_options(ghogx_app PRIVATE /W4)
else()
    target_compile_options(ghogx_app PRIVATE -Wall -Wextra)
endif()
ghogx_add_windows_import_guard(ghogx_app)

if(WIN32)
    find_program(GHOGX_POWERSHELL powershell.exe)
    if(GHOGX_POWERSHELL)
        add_test(NAME ghogx_app_imports
                 COMMAND ${GHOGX_POWERSHELL} -NoProfile -ExecutionPolicy Bypass
                         -File "${CMAKE_SOURCE_DIR}/../tools/check_windows_imports.ps1"
                         $<TARGET_FILE:ghogx_app>)
    endif()
endif()
