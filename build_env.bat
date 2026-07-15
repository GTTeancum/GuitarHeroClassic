@echo off
REM Set up MSVC + Ninja environment for dependency-free GuitarHeroOGX builds.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$parts = $env:PATH -split ';' | Where-Object { $_ -and ($_ -notmatch '(?i)(\\|/)(LLVM|msys64|mingw64|mingw32|clang64)(\\|/|$)') }; [string]::Join(';', $parts)"`) do set "PATH=%%P"
set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "CC=cl"
set "CXX=cl"
set "REXSDK=C:\Programming\GitHub\Guitar Hero II\rexglue-sdk\out\install\win-amd64"
cd /d "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX"
%*
