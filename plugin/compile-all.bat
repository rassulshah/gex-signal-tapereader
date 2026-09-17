@echo off
REM ===========================================================================
REM  compile-all.bat  --  DOUBLE-CLICK THIS to rebuild ALL FOUR plugins in one go
REM  (lsGammaProfile, lsDayModel, lsDayStats, lsKingTracker). Close Investor/RT first.
REM  The hands-free alternative is setup-gex-build.bat at the repo root (run once).
REM ===========================================================================
title Build all four RTX plugins
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
call build-daymodel.bat
call build-daystats.bat
call build-kingtracker.bat
goto :end
:novs
echo [ERROR] Could not find Visual Studio's C++ tools.
:end
echo.
echo ---------------------------------------------------------------------------
echo Done. Reopen Investor/RT to load the new builds.
pause
