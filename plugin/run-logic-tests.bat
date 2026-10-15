@echo off
REM ===========================================================================
REM  run-logic-tests.bat  --  Gate B (logic) regression for ALL the RTX plugins.
REM  Compiles each plugin\test_*_logic.cpp against its *Logic.h with the same MSVC the DLLs
REM  use (no IRT SDK needed) and runs it. Exit code = total failures.
REM      run-logic-tests.bat            all four suites
REM      run-logic-tests.bat gamma      one: gamma | contractoffset | kingtracker | dealer | deltarules | deltacore | deltaedge | deltarules285
REM  (MT100 2026-10-09) daymodel / daystats retired: their tests live in plugin\retired\ with the sources
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
call :one kingtracker    test_kingtracker_logic
call :one dealer         test_dealer_logic
REM (lsDeltaProfile 2.8.4, 2026-10-14 night) the SDK-free Delta Profile suites: the A rules (pivot / halfway / edge) and the core
call :one deltarules     tests_delta\test_rules284_core
call :one deltacore      tests_delta\core_tests
call :one deltaedge      tests_delta\test_audit_257
REM (lsDeltaProfile 2.8.5, 2026-10-14 night) one bubble per candle per side + the one-time pre-2.8.4 re-check (SDK-free half)
call :one deltarules285  tests_delta\test_rules285_core
REM The mock-SDK Delta Profile suites (tests_delta\test_rules285, test_rules284, test_dp_bubble_halfway, adapter_tests, test_rule262, test_record_254,
REM test_contract263, test_exh270, test_offscreen271, test_columns280, test_sync260, test_session_noon) need POSIX + the mock irtsdk.h:
REM the cloud runs them with g++ -fsanitize=address,undefined from tests_delta (see the Build line at the top of each file).
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
