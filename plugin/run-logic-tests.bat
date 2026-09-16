@echo off
REM ===========================================================================
REM  run-logic-tests.bat  --  Gate B (logic) regression for lsGammaProfile.
REM  Compiles plugin\test_gammaprofile_logic.cpp against GammaProfileLogic.h with
REM  the same MSVC the DLL uses (no IRT SDK needed) and runs it. Exit code = failures.
REM  The cloud runs the identical file with g++ on every build.
REM ===========================================================================
title lsGammaProfile logic tests
set "PLUGDIR=C:\Dev\gex-signal-tapereader\plugin"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST goto :novs
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%PLUGDIR%"
cl /nologo /EHsc /W3 /Fe:test_gammaprofile_logic.exe test_gammaprofile_logic.cpp
if errorlevel 1 goto :fail
test_gammaprofile_logic.exe
set RC=%ERRORLEVEL%
del test_gammaprofile_logic.obj >nul 2>&1
echo.
if "%RC%"=="0" (echo ALL LOGIC TESTS PASSED) else (echo %RC% LOGIC TEST(S) FAILED)
pause
exit /b %RC%
:novs
echo Visual Studio C++ tools not found.
pause
exit /b 1
:fail
echo COMPILE FAILED
pause
exit /b 1
