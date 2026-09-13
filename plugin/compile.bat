@echo off
REM ===========================================================================
REM  compile.bat  —  DOUBLE-CLICK THIS to rebuild the gamma-profile plugin.
REM  No admin needed. No cmd window to open. It sets up the compiler for you.
REM  ONLY rule: close Investor/RT first (a loaded DLL can't be replaced).
REM ===========================================================================
title Build lsGammaProfile

REM locate Visual Studio (any edition/version) and load the x64 compiler env
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST (
  echo [ERROR] Could not find Visual Studio's C++ tools.
  echo         Open an "x64 Native Tools Command Prompt for VS" and run build.bat instead.
  echo.
  pause
  exit /b 1
)
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul

cd /d "C:\Dev\gex-signal-tapereader\plugin"
call build.bat

echo.
echo ---------------------------------------------------------------------------
echo Done. Reopen Investor/RT to load the new build.  (Press a key to close.)
pause >nul
