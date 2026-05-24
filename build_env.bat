@echo off
REM Set up MSVC + clang + ninja environment for GuitarHeroOGX build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\LLVM\bin;C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "REXSDK=C:\Programming\GitHub\Guitar Hero II\rexglue-sdk\out\install\win-amd64"
cd /d "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX"
%*
