# ghogx_character — skinned character mesh decode + bind-pose/animated renderer.
# Decodes BandCharacter PS2 MILOs (bone skeleton + skinned meshes + materials),
# provides CharRenderer for D3D9 display. Win32-only (D3D9 dependency).

if(NOT WIN32)
    return()
endif()

add_library(ghogx_character STATIC
    char_mesh.cpp
    char_mesh.h
    char_facefx.cpp
    char_facefx.h
    char_renderer.cpp
    char_renderer.h
    char_clip.cpp
    char_clip.h
)
target_include_directories(ghogx_character PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(ghogx_character PUBLIC
    ghogx_milo_scene
    ghogx_asset
    ark_v3 dtb ps2_texture milo
    d3d9
)

add_executable(ghogx_character_bind_audit char_bind_audit.cpp)
target_link_libraries(ghogx_character_bind_audit PRIVATE ghogx_character)

add_executable(ghogx_character_clip_audit char_clip_audit.cpp)
target_link_libraries(ghogx_character_clip_audit PRIVATE ghogx_character)

add_executable(ghogx_character_clip_driver_flags_test
    character_clip_driver_flags_test.cpp)
target_link_libraries(ghogx_character_clip_driver_flags_test PRIVATE
    ghogx_character)
add_test(NAME ghogx_character_clip_driver_flags_test
         COMMAND ghogx_character_clip_driver_flags_test)

add_executable(ghogx_character_char_bones_source_test
    character_char_bones_source_test.cpp)
target_link_libraries(ghogx_character_char_bones_source_test PRIVATE
    ghogx_character)
add_test(NAME ghogx_character_char_bones_source_test
         COMMAND ghogx_character_char_bones_source_test)

add_executable(ghogx_character_char_utl_source_test
    character_char_utl_source_test.cpp)
target_link_libraries(ghogx_character_char_utl_source_test PRIVATE
    ghogx_character)
add_test(NAME ghogx_character_char_utl_source_test
         COMMAND ghogx_character_char_utl_source_test)

add_executable(ghogx_character_ik_rod_source_test
    character_ik_rod_source_test.cpp)
target_link_libraries(ghogx_character_ik_rod_source_test PRIVATE
    ghogx_character)
add_test(NAME ghogx_character_ik_rod_source_test
         COMMAND ghogx_character_ik_rod_source_test)

add_executable(ghogx_character_weight_setter_source_test
    character_weight_setter_source_test.cpp)
target_link_libraries(ghogx_character_weight_setter_source_test PRIVATE
    ghogx_character)
add_test(NAME ghogx_character_weight_setter_source_test
         COMMAND ghogx_character_weight_setter_source_test)

add_executable(ghogx_character_char_hair_source_test
    character_char_hair_source_test.cpp)
target_link_libraries(ghogx_character_char_hair_source_test PRIVATE
    ghogx_character)
add_test(NAME ghogx_character_char_hair_source_test
         COMMAND ghogx_character_char_hair_source_test)

add_executable(ghogx_character_mesh_decode_test character_mesh_decode_test.cpp)
target_link_libraries(ghogx_character_mesh_decode_test PRIVATE ghogx_character)
add_test(NAME ghogx_character_mesh_decode_test
         COMMAND ghogx_character_mesh_decode_test)

add_executable(ghogx_character_skin_order_test character_skin_order_test.cpp)
target_link_libraries(ghogx_character_skin_order_test PRIVATE
    ghogx_character ghogx_render)
add_test(NAME ghogx_character_skin_order_test
         COMMAND ghogx_character_skin_order_test)

add_executable(ghogx_character_no_named_fix_test character_no_named_fix_test.cpp)
target_compile_definitions(ghogx_character_no_named_fix_test PRIVATE
    GHOGX_CHARACTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ghogx_character_no_named_fix_test
         COMMAND ghogx_character_no_named_fix_test)

add_executable(ghogx_character_left_hand_contract_test
    character_left_hand_contract_test.cpp)
target_compile_definitions(ghogx_character_left_hand_contract_test PRIVATE
    GHOGX_CHARACTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    GHOGX_GAME_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../game"
    GHOGX_APP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../app")
add_test(NAME ghogx_character_left_hand_contract_test
         COMMAND ghogx_character_left_hand_contract_test)

add_executable(ghogx_character_eye_bridge_contract_test
    character_eye_bridge_contract_test.cpp)
target_compile_definitions(ghogx_character_eye_bridge_contract_test PRIVATE
    GHOGX_CHARACTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ghogx_character_eye_bridge_contract_test
         COMMAND ghogx_character_eye_bridge_contract_test)

set(GHOGX_IHATECOMPVIR_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/ihatecompvir-public-milo-sources")
set(GHOGX_IHATECOMPVIR_EXTRA_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/ihatecompvir-extra")
add_executable(ghogx_character_source_truth_contract_test
    character_source_truth_contract_test.cpp)
target_compile_definitions(ghogx_character_source_truth_contract_test PRIVATE
    GHOGX_CHARACTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    GHOGX_MILO_SCENE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../milo_scene"
    GHOGX_IHATECOMPVIR_SOURCE_DIR="${GHOGX_IHATECOMPVIR_SOURCE_DIR}"
    GHOGX_IHATECOMPVIR_EXTRA_DIR="${GHOGX_IHATECOMPVIR_EXTRA_DIR}")
add_test(NAME ghogx_character_source_truth_contract_test
         COMMAND ghogx_character_source_truth_contract_test)

if(MSVC)
target_compile_options(ghogx_character PRIVATE /W4)
target_compile_options(ghogx_character_bind_audit PRIVATE /W4)
target_compile_options(ghogx_character_clip_audit PRIVATE /W4)
target_compile_options(ghogx_character_clip_driver_flags_test PRIVATE /W4)
target_compile_options(ghogx_character_char_bones_source_test PRIVATE /W4)
target_compile_options(ghogx_character_char_utl_source_test PRIVATE /W4)
target_compile_options(ghogx_character_ik_rod_source_test PRIVATE /W4)
target_compile_options(ghogx_character_weight_setter_source_test PRIVATE /W4)
target_compile_options(ghogx_character_char_hair_source_test PRIVATE /W4)
target_compile_options(ghogx_character_mesh_decode_test PRIVATE /W4)
target_compile_options(ghogx_character_skin_order_test PRIVATE /W4)
target_compile_options(ghogx_character_no_named_fix_test PRIVATE /W4)
target_compile_options(ghogx_character_left_hand_contract_test PRIVATE /W4)
target_compile_options(ghogx_character_eye_bridge_contract_test PRIVATE /W4)
target_compile_options(ghogx_character_source_truth_contract_test PRIVATE /W4)
else()
    target_compile_options(ghogx_character PRIVATE -Wall -Wextra)
    target_compile_options(ghogx_character_bind_audit PRIVATE -Wall -Wextra)
    target_compile_options(ghogx_character_clip_audit PRIVATE -Wall -Wextra)
    target_compile_options(ghogx_character_clip_driver_flags_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_char_bones_source_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_char_utl_source_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_ik_rod_source_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_weight_setter_source_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_char_hair_source_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_mesh_decode_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_skin_order_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_no_named_fix_test PRIVATE -Wall -Wextra)
    target_compile_options(ghogx_character_left_hand_contract_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_eye_bridge_contract_test PRIVATE
        -Wall -Wextra)
    target_compile_options(ghogx_character_source_truth_contract_test PRIVATE
        -Wall -Wextra)
endif()
