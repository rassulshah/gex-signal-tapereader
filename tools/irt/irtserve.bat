@echo off
REM ============================================================================
REM  IRT FLEXLEVELS SERVER  --  double-click me
REM
REM  Put irtserve.bat AND irtserve.py in the lsFlexLevels folder, then
REM  double-click this file. Leave the window open while you trade.
REM
REM  Then in IRT's FlexLevels dialog:
REM     * select  Remote File   (NOT Local File)
REM     * URL     http://localhost:8000/FlexLevelsExport.csv
REM     * Check For Updates Every: 1 Minute
REM
REM  WHY: measured 2026-08-27 -- IRT does not poll a LOCAL file at all. The
REM  panel writes the CSV in place correctly; IRT simply never re-reads it.
REM  Its 1-minute poll applies to Remote File mode only.
REM
REM  NO POWERSHELL ANYWHERE -- Avast flags it (IDP.HELU.PSE88).
REM ============================================================================
setlocal
cd /d "%~dp0"

echo.
echo   IRT FlexLevels server
echo   folder: %CD%
echo.

if not exist "irtserve.py" (
  echo   ERROR: irtserve.py is not in this folder.
  echo   Put BOTH irtserve.bat and irtserve.py in the lsFlexLevels folder.
  echo.
  pause
  exit /b 1
)

if not exist "FlexLevelsExport.csv" (
  echo   NOTE: FlexLevelsExport.csv is not here yet. That is fine -- the panel
  echo         writes it on its next export. Check you picked THIS folder in
  echo         the panel's gear.
  echo.
)

REM ---- find Python. py launcher first, it is the most reliable on Windows ----
set "PY="
py -3 --version >nul 2>&1 && set "PY=py -3"
if not defined PY ( python --version >nul 2>&1 && set "PY=python" )
if not defined PY ( python3 --version >nul 2>&1 && set "PY=python3" )
if not defined PY if exist "%LOCALAPPDATA%\Programs\Python\Python312\python.exe" set "PY=%LOCALAPPDATA%\Programs\Python\Python312\python.exe"
if not defined PY if exist "%LOCALAPPDATA%\Programs\Python\Python311\python.exe" set "PY=%LOCALAPPDATA%\Programs\Python\Python311\python.exe"
if not defined PY if exist "C:\Python312\python.exe" set "PY=C:\Python312\python.exe"
if not defined PY if exist "C:\Python311\python.exe" set "PY=C:\Python311\python.exe"

if not defined PY (
  echo   ================================================================
  echo   PYTHON IS NOT INSTALLED ^(or not on PATH^).
  echo.
  echo   Install it from  https://www.python.org/downloads/
  echo   and TICK "Add python.exe to PATH" on the first screen.
  echo.
  echo   Then double-click this file again.
  echo   ================================================================
  echo.
  pause
  exit /b 1
)

echo   using: %PY%
echo.
%PY% "irtserve.py" "%CD%"
echo.
echo   server stopped.
pause
