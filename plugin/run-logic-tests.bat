@echo off
REM ===========================================================================
REM  run-logic-tests.bat  --  Gate B (logic) regression for ALL the RTX plugins.
REM  Compiles each plugin\test_*_logic.cpp against its *Logic.h with the same MSVC the DLLs
REM  use (no IRT SDK needed) and runs it. Exit code = total failures.
REM      run-logic-tests.bat            all five suites
REM      run-logic-tests.bat gamma      one: gamma | contractoffset | daymodel | daystats | kingtracker
REM  The cloud runs the identical files with g++ on every build (tools/regress.py).
REM ===========================================================================
title RTX plugin logic tests
set "PLUGDIR=C:\Dev\gex-signal-tapereader\plugin"
set "ONLY=%~1"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST goto :novs
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%PLUGDIR%"
set TOTAL=0
call :one gamma          test_gammaprofile_logic
call :one contractoffset test_contractoffset_logic
call :one daymodel       test_daymodel_logic
call :one daystats       test_daystats_logic
call :one kingtracker    test_kingtracker_logic
echo.
if "%TOTAL%"=="0" (echo ALL LOGIC TESTS PASSED) else (echo %TOTAL% LOGIC TEST(S) FAILED)
pause
exit /b %TOTAL%

:one
if defined ONLY if /i not "%ONLY%"=="%~1" goto :eof
echo.
echo === %~1 (%~2.cpp) ===
cl /nologo /EHsc /W3 /Fe:%~2.exe %~2.cpp
if errorlevel 1 (echo COMPILE FAILED: %~2 & set /a TOTAL+=1 & goto :eof)
%~2.exe
set /a TOTAL+=%ERRORLEVEL%
del %~2.obj >nul 2>&1
goto :eof

:novs
echo Visual Studio C++ tools not found.
pause
exit /b 1
