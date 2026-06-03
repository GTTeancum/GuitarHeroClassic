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
    ghogx_core ghogx_script ghogx_dtb_bridge ark_v3 dtb)

if(MSVC)
    target_compile_options(ghogx_ui_core PRIVATE /W4)
else()
    target_compile_options(ghogx_ui_core PRIVATE -Wall -Wextra)
endif()

# Headless boot test against the real stock ARK (skips if the ARK is absent).
add_executable(ghogx_ui_test ui_test.cpp)
target_link_libraries(ghogx_ui_test PRIVATE ghogx_ui_core)
if(MSVC)
    target_compile_options(ghogx_ui_test PRIVATE /W4)
else()
    target_compile_options(ghogx_ui_test PRIVATE -Wall -Wextra)
endif()
add_test(NAME ghogx_ui_test COMMAND ghogx_ui_test)
