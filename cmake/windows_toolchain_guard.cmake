if(WIN32)
    set(_ghogx_windows_toolchain_text
        "${CMAKE_C_COMPILER};${CMAKE_CXX_COMPILER};${CMAKE_CXX_SIMULATE_ID}")
    string(TOLOWER "${_ghogx_windows_toolchain_text}" _ghogx_windows_toolchain_text)

    if(MSYS OR MINGW OR _ghogx_windows_toolchain_text MATCHES "msys64|mingw|clang64")
        message(FATAL_ERROR
            "Dependency-free Windows proof builds must use the MSVC-compatible "
            "toolchain from build_env.bat. The MSYS/MinGW clang toolchain can "
            "emit executables that import libc++.dll, which is not acceptable "
            "for this OG Xbox-portable codebase.")
    endif()

    unset(_ghogx_windows_toolchain_text)
endif()
