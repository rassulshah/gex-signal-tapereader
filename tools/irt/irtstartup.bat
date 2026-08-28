@echo off
REM ============================================================================
REM  IRT FLEXLEVELS SERVER  --  AUTOSTART LAUNCHER
REM  This copy lives in the Windows Startup folder and runs at every login.
REM  It does NOT use %~dp0 (that would be the Startup folder) -- the data
REM  folder is hardcoded below.
REM
REM  If the folder ever moves, edit FOLDER= on the next line.
REM ============================================================================
set "FOLDER=C:\Users\rassul\InvestorRT\rtx\lsFlexLevels"

title IRT FlexLevels server
cd /d "%FOLDER%" 2>nul
if errorlevel 1 (
  echo   ERROR: folder not found: %FOLDER%
  echo   Edit FOLDER= inside this file.
  pause
  exit /b 1
)

set "PY="
py -3 --version >nul 2>&1 && set "PY=py -3"
if not defined PY ( python --version >nul 2>&1 && set "PY=python" )
if not defined PY ( python3 --version >nul 2>&1 && set "PY=python3" )
if not defined PY (
  echo   Python not found. Install from python.org and tick "Add to PATH".
  pause
  exit /b 1
)

%PY% "irtserve.py" "%FOLDER%"
pause
