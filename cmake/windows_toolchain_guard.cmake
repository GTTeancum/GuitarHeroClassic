if(WIN32)
    set(_ghogx_windows_toolchain_text
        "${CMAKE_C_COMPILER};${CMAKE_CXX_COMPILER};${CMAKE_C_COMPILER_ID};"
        "${CMAKE_CXX_COMPILER_ID};${CMAKE_C_SIMULATE_ID};${CMAKE_CXX_SIMULATE_ID}")
    string(TOLOWER "${_ghogx_windows_toolchain_text}" _ghogx_windows_toolchain_text)

    set(_ghogx_windows_uses_non_msvc_compiler FALSE)
    foreach(_ghogx_lang C CXX)
        if(DEFINED CMAKE_${_ghogx_lang}_COMPILER_ID AND
           NOT CMAKE_${_ghogx_lang}_COMPILER_ID STREQUAL "" AND
           NOT CMAKE_${_ghogx_lang}_COMPILER_ID STREQUAL "MSVC")
            set(_ghogx_windows_uses_non_msvc_compiler TRUE)
        endif()
    endforeach()

    if(MSYS OR MINGW OR _ghogx_windows_toolchain_text MATCHES "msys64|mingw|clang64" OR
       _ghogx_windows_toolchain_text MATCHES "llvm|clang" OR
       _ghogx_windows_uses_non_msvc_compiler)
        message(FATAL_ERROR
            "Dependency-free Windows proof builds must use the MSVC cl/link "
            "toolchain from build_env.bat. Clang, MSYS, and MinGW toolchains "
            "can emit executables that import non-platform DLLs such as "
            "libc++.dll, which is not acceptable for this OG Xbox-portable "
            "codebase.")
    endif()

    unset(_ghogx_lang)
    unset(_ghogx_windows_uses_non_msvc_compiler)
    unset(_ghogx_windows_toolchain_text)
endif()

function(ghogx_add_windows_import_guard target_name)
    if(NOT WIN32)
        return()
    endif()
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "ghogx_add_windows_import_guard called for unknown target: ${target_name}")
    endif()

    get_property(_ghogx_guarded TARGET ${target_name} PROPERTY GHOGX_WINDOWS_IMPORT_GUARDED)
    if(_ghogx_guarded)
        return()
    endif()

    get_target_property(_ghogx_target_type ${target_name} TYPE)
    if(NOT _ghogx_target_type STREQUAL "EXECUTABLE")
        return()
    endif()

    find_program(GHOGX_POWERSHELL powershell.exe)
    if(GHOGX_POWERSHELL)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${GHOGX_POWERSHELL} -NoProfile -ExecutionPolicy Bypass
                    -File "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/check_windows_imports.ps1"
                    "$<TARGET_FILE:${target_name}>"
            COMMENT "Checking ${target_name} for forbidden runtime DLL imports")
        set_property(TARGET ${target_name} PROPERTY GHOGX_WINDOWS_IMPORT_GUARDED TRUE)
    endif()

    unset(_ghogx_target_type)
    unset(_ghogx_guarded)
endfunction()
