@echo off
REM zdrun-build.bat -- builds tests_zd\zdrun.exe (the lsZoneDefense engine as a CLI for lra.zone_defense) with the MSVC the DLLs use.
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 2
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST exit /b 2
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
cl /nologo /O2 /EHsc /std:c++17 /I.. zdrun.cpp /Fe:zdrun.exe >zdrun-build.log 2>&1
exit /b %ERRORLEVEL%
