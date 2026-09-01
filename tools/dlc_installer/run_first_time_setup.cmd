@echo off
setlocal
set "SETUP_SCRIPT=%~dp0first_run_setup.py"

where py >nul 2>nul
if errorlevel 1 goto try_python
py -3 -c "import sys" >nul 2>nul
if errorlevel 1 goto try_python
py -3 "%SETUP_SCRIPT%" --install-dir "%~dp0..\.." %*
exit /b %errorlevel%

:try_python
where python >nul 2>nul
if errorlevel 1 goto missing_python
python -c "import sys" >nul 2>nul
if errorlevel 1 goto missing_python
python "%SETUP_SCRIPT%" --install-dir "%~dp0..\.." %*
exit /b %errorlevel%

:missing_python
echo Python 3.9 or newer was not found. Install Python, then run this launcher again.
pause
exit /b 2
