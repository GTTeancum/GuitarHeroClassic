# engine/src/ui/CMakeLists.txt
#
# ghogx_ui_core: the menu UI runtime -- the GH2 UI object classes (GHPanel/
# GHScreen/UIComponent/...), the ScreenManager (the `ui` object + interpreter
# Host + screen-transition state machine), and the {new ...} DTB screen loader.
# Portable (core + script + dtb_bridge + ark/dtb; no Windows dependency). The
# D3D9 renderer (ghogx_ui_render) is a separate WIN32 target added in Phase 5.

add_library(ghogx_ui_core STATIC
    ui_classes.cpp      ui_classes.h
    screen_manager.cpp  screen_manager.h
    screen_loader.cpp   screen_loader.h
    config_db.cpp       config_db.h
    meta_objects.cpp    meta_objects.h
)
target_include_directories(ghogx_ui_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ghogx_ui_core PUBLIC
    ghogx_core ghogx_script ghogx_dtb_bridge ghogx_chart ark_v3 dtb milo)

if(MSVC)
    target_compile_options(ghogx_ui_core PRIVATE /W4)
else()
    target_compile_options(ghogx_ui_core PRIVATE -Wall -Wextra)
endif()

# The menu bitmap font ("impact"): parse impact.font byte-exact (charset +
# kerning) and derive glyph rects from the decoded atlas. Portable (asset decode
# is CPU-side; no D3D9), so it can be unit-tested headless.
add_library(ghogx_menu_font STATIC menu_font.cpp menu_font.h menu_labels.cpp menu_labels.h)
target_include_directories(ghogx_menu_font PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ghogx_menu_font PUBLIC ghogx_asset ark_v3 milo)
if(MSVC)
    target_compile_options(ghogx_menu_font PRIVATE /W4)
else()
    target_compile_options(ghogx_menu_font PRIVATE -Wall -Wextra)
endif()

# Windowed menu presentation. GH2 menus are 3-D MILO scenes, so this reuses the
# MiloSceneRenderer; WIN32/D3D9-only (its deps are the WIN32 render targets).
add_library(ghogx_ui_render STATIC
    menu_app.cpp menu_app.h
    pss_video_player_win32.cpp pss_video_player_win32.h)
target_include_directories(ghogx_ui_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ghogx_ui_render PUBLIC
    ghogx_ui_core ghogx_render ghogx_milo_scene ghogx_asset ghogx_menu_font
    ghogx_game ghogx_hud ark_v3 milo)
if(MSVC)
    target_compile_options(ghogx_ui_render PRIVATE /W4)
endif()

# Headless font test against the real stock ARK (skips if the ARK is absent).
add_executable(ghogx_menu_font_test menu_font_test.cpp)
target_link_libraries(ghogx_menu_font_test PRIVATE ghogx_menu_font)
if(MSVC)
    target_compile_options(ghogx_menu_font_test PRIVATE /W4)
endif()
add_test(NAME ghogx_menu_font_test COMMAND ghogx_menu_font_test)

add_executable(ghogx_menu_labels_test menu_labels_test.cpp)
target_link_libraries(ghogx_menu_labels_test PRIVATE ghogx_menu_font ghogx_milo_scene)
if(MSVC)
    target_compile_options(ghogx_menu_labels_test PRIVATE /W4)
endif()
add_test(NAME ghogx_menu_labels_test COMMAND ghogx_menu_labels_test)

# Headless boot test against the real stock ARK (skips if the ARK is absent).
add_executable(ghogx_ui_test ui_test.cpp)
target_link_libraries(ghogx_ui_test PRIVATE ghogx_ui_core)
if(MSVC)
    target_compile_options(ghogx_ui_test PRIVATE /W4)
else()
    target_compile_options(ghogx_ui_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_ui_test COMMAND ghogx_ui_test)
