@echo off
REM rrrun-build.bat -- builds tests_revread\rrrun.exe (the lsRevRead engine as a CLI for lra.rev_read_replay) with the MSVC the DLLs use.
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 2
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST exit /b 2
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
cl /nologo /O2 /EHsc /std:c++17 rrrun.cpp /Fe:rrrun.exe >rrrun-build.log 2>&1
exit /b %ERRORLEVEL%
