@echo off
setlocal

set "RB2_ROOT=%~dp0.."
set "DOLPHIN_TOOL=%~dp0dolphin-2603a\extracted\Dolphin-x64\DolphinTool.exe"
set "RB2_IMAGE=%RB2_ROOT%\disc\Rock Band 2 (US).rvz"
set "RB2_SOURCE=%RB2_ROOT%\source_disc"
set "RB2_LOGS=%RB2_ROOT%\output\batch"

if not exist "%RB2_SOURCE%" mkdir "%RB2_SOURCE%"
if not exist "%RB2_LOGS%" mkdir "%RB2_LOGS%"

"%DOLPHIN_TOOL%" extract -i "%RB2_IMAGE%" -o "%RB2_SOURCE%" -g ^
  1>"%RB2_LOGS%\dolphin-extract.stdout.log" ^
  2>"%RB2_LOGS%\dolphin-extract.stderr.log"

set "RB2_EXIT=%ERRORLEVEL%"
>"%RB2_LOGS%\dolphin-extract.exitcode.txt" echo %RB2_EXIT%
exit /b %RB2_EXIT%
