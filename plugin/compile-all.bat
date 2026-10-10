@echo off
REM ===========================================================================
REM  compile-all.bat  --  DOUBLE-CLICK THIS to rebuild the original plugins in one go
REM  (lsGammaProfile, lsKingTracker). Close Investor/RT first.
REM  (MT100 2026-10-09) lsDayModel / lsDayStats retired - sources in plugin\retired\, not built.
REM  The hands-free alternative is setup-gex-build.bat at the repo root (run once).
REM ===========================================================================
title Build the original RTX plugins
set "PLUGDIR=C:\Dev\gex-signal-tapereader\plugin"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST goto :novs
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%PLUGDIR%"
call build-gammaprofile.bat
call build-kingtracker.bat
goto :end
:novs
echo [ERROR] Could not find Visual Studio's C++ tools.
:end
echo.
echo ---------------------------------------------------------------------------
echo Done. Reopen Investor/RT to load the new builds.
pause
