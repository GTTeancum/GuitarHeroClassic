@echo off
setlocal

set "RB2_ROOT=%~dp0.."
set "ARKHELPER=%~dp0arkhelper\arkhelper.exe"
set "RB2_HDR=%RB2_ROOT%\source_disc\DATA\files\gen\main_wii.hdr"
set "RB2_ARK_SOURCE=%RB2_ROOT%\source_ark"
set "RB2_LOGS=%RB2_ROOT%\output\batch"

if not exist "%RB2_ARK_SOURCE%" mkdir "%RB2_ARK_SOURCE%"
if not exist "%RB2_LOGS%" mkdir "%RB2_LOGS%"

rem Do not request --convertScripts here. ArkHelper excludes compiled scripts
rem from --extractAll while conversion is enabled, and a missing/incompatible
rem dtab leaves those retail DTBs absent from the output.
"%ARKHELPER%" ark2dir "%RB2_HDR%" "%RB2_ARK_SOURCE%" ^
  --extractAll --logLevel info ^
  1>"%RB2_LOGS%\arkhelper-raw-extract.stdout.log" ^
  2>"%RB2_LOGS%\arkhelper-raw-extract.stderr.log"

set "RB2_EXIT=%ERRORLEVEL%"
>"%RB2_LOGS%\arkhelper-raw-extract.exitcode.txt" echo %RB2_EXIT%
exit /b %RB2_EXIT%
