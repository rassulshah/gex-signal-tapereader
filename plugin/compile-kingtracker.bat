@echo off
REM ===========================================================================
REM  compile-kingtracker.bat  --  DOUBLE-CLICK THIS to rebuild the lsKingTracker plugin.
REM  Finds Visual Studio, loads the x64 compiler, then calls build-kingtracker.bat.
REM  ONLY rule: close Investor/RT first (a loaded DLL can't be replaced).
REM ===========================================================================
title Build lsKingTracker
set "PLUGDIR=C:\Dev\gex-signal-tapereader\plugin"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs

set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST goto :novs

call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :novcvars

cd /d "%PLUGDIR%"
call build-kingtracker.bat
goto :end

:novs
echo [ERROR] Could not find Visual Studio's C++ tools.
echo         Open an "x64 Native Tools Command Prompt for VS", then run:
echo             cd /d "%PLUGDIR%"
echo             build-kingtracker.bat
goto :end
:novcvars
echo [ERROR] Failed to initialize the x64 compiler environment (vcvars64).
goto :end
:end
echo.
echo ---------------------------------------------------------------------------
echo Done. Reopen Investor/RT to load the new build.
echo.
pause
