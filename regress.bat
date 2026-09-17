@echo off
setlocal EnableDelayedExpansion
REM ===========================================================================
REM  regress.bat  --  the regression for the RTX indicators, on the operator's machine.
REM      regress.bat               all four (Gate A node tests + Gate B C++ logic tests), recorded in testing\RESULTS.md
REM      regress.bat daymodel      one: gamma | daymodel | daystats | kingtracker (several may be listed)
REM      regress.bat all -v        every suite's full output
REM  Needs: Python 3, Node.js, and Visual Studio C++ tools (found through vswhere; g++ is used instead if it is on PATH).
REM  The cloud runs the identical tools\regress.py with g++ on every build; this is the same run on the DLLs' own compiler.
REM ===========================================================================
title GEX indicator regression
set "REPO=C:\Dev\gex-signal-tapereader"
cd /d "%REPO%" || (echo repo not found at %REPO% & pause & exit /b 1)

set PY=
where python >nul 2>&1 && set PY=python
if not defined PY where py >nul 2>&1 && set PY=py -3
if not defined PY (echo PYTHON NOT FOUND - install Python 3 ^(python.org, tick "Add to PATH"^) & pause & exit /b 1)
where node >nul 2>&1 || (echo NODE NOT FOUND - install Node.js LTS ^(nodejs.org^) for the Gate A tests & pause & exit /b 1)

REM MSVC: put cl on PATH the way run-logic-tests.bat does (skipped when g++ is already there)
where g++ >nul 2>&1 && goto :run
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo Visual Studio C++ tools not found - Gate B will be skipped & goto :run)
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if defined VSINST call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul

:run
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
!PY! tools\regress.py %*
set RC=!ERRORLEVEL!
echo.
if "!RC!"=="0" (echo REGRESSION GREEN) else (echo REGRESSION: !RC! SUITE^(S^) FAILED)
pause
exit /b !RC!
