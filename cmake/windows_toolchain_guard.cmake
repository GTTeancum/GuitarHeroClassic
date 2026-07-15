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
