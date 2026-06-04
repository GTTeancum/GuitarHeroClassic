# engine/src/core/CMakeLists.txt
#
# ghogx_core: the Sandbox runtime value/property primitives. No dependency on
# the tools/ asset readers or the reference binary -- pure logic, PC-first,
# and the foundation the rest of the engine (Object, ClassReg, Scheduler,
# subsystems) is built on.

add_library(ghogx_core STATIC
    symbol.cpp        symbol.h
    data_node.cpp     data_node.h
    property_table.cpp property_table.h
    class_reg.cpp     class_reg.h
    object.cpp        object.h
    object_dir.cpp    object_dir.h
    scheduler.cpp     scheduler.h
    engine.cpp        engine.h
)

# Headers are included as "core/<x>.h", so the include root is engine/src.
target_include_directories(ghogx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(MSVC)
    target_compile_options(ghogx_core PRIVATE /W4)
else()
    target_compile_options(ghogx_core PRIVATE -Wall -Wextra)
endif()

# ---- unit test -------------------------------------------------------------

# ghogx_add_test(<target> <source.cpp>)
macro(ghogx_add_test test_name test_src)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE ghogx_core)
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

ghogx_add_test(ghogx_core_test       core_test.cpp)
ghogx_add_test(ghogx_object_test     object_test.cpp)
ghogx_add_test(ghogx_object_dir_test object_dir_test.cpp)
ghogx_add_test(ghogx_scheduler_test  scheduler_test.cpp)
ghogx_add_test(ghogx_engine_test     engine_test.cpp)
