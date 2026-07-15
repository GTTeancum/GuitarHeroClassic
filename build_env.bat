@echo off
REM Set up MSVC + Ninja environment for dependency-free GuitarHeroOGX builds.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "REXSDK=C:\Programming\GitHub\Guitar Hero II\rexglue-sdk\out\install\win-amd64"
cd /d "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX"
%*
