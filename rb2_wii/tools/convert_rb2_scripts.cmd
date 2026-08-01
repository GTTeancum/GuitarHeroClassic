@echo off
setlocal

set "RB2_ROOT=%~dp0.."
set "ARKHELPER=%~dp0arkhelper\arkhelper.exe"
set "RB2_HDR=%RB2_ROOT%\source_disc\DATA\files\gen\main_wii.hdr"
set "RB2_ARK_SOURCE=%RB2_ROOT%\source_ark"
set "RB2_LOGS=%RB2_ROOT%\output\batch"

if not exist "%RB2_ARK_SOURCE%" mkdir "%RB2_ARK_SOURCE%"
if not exist "%RB2_LOGS%" mkdir "%RB2_LOGS%"

rem Without --extractAll this pass emits only converted DTA scripts. dtab.exe
rem must sit beside arkhelper.exe so ArkHelper can convert its Classic DTB
rem intermediate after decoding the RBVR container.
"%ARKHELPER%" ark2dir "%RB2_HDR%" "%RB2_ARK_SOURCE%" ^
  --convertScripts --indentSize 3 --logLevel info ^
  1>"%RB2_LOGS%\arkhelper-script-convert.stdout.log" ^
  2>"%RB2_LOGS%\arkhelper-script-convert.stderr.log"

set "RB2_EXIT=%ERRORLEVEL%"
>"%RB2_LOGS%\arkhelper-script-convert.exitcode.txt" echo %RB2_EXIT%
exit /b %RB2_EXIT%
