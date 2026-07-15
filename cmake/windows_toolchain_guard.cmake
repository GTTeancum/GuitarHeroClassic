if(WIN32)
    set(_ghogx_windows_toolchain_text
        "${CMAKE_C_COMPILER};${CMAKE_CXX_COMPILER};${CMAKE_CXX_SIMULATE_ID}")
    string(TOLOWER "${_ghogx_windows_toolchain_text}" _ghogx_windows_toolchain_text)

    set(_ghogx_windows_uses_dependency_clang FALSE)
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND
       NOT CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
        set(_ghogx_windows_uses_dependency_clang TRUE)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
       NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        set(_ghogx_windows_uses_dependency_clang TRUE)
    endif()

    if(MSYS OR MINGW OR _ghogx_windows_toolchain_text MATCHES "msys64|mingw|clang64" OR
       _ghogx_windows_uses_dependency_clang)
        message(FATAL_ERROR
            "Dependency-free Windows proof builds must use the MSVC-compatible "
            "toolchain from build_env.bat. MSYS/MinGW or GNU-style clang "
            "toolchains can emit executables that import libc++.dll, which is "
            "not acceptable for this OG Xbox-portable codebase.")
    endif()

    unset(_ghogx_windows_uses_dependency_clang)
    unset(_ghogx_windows_toolchain_text)
endif()
