@echo off
REM ===========================================================================
REM  build.bat  —  compile GammaProfile.cpp into GammaProfile.dll (x64)
REM
REM  RUN THIS FROM:  "x64 Native Tools Command Prompt for VS 2022"
REM  (Start menu -> type that exact name. It puts cl.exe + the CRT on PATH.)
REM  Then:   cd /d "C:\Dev\gex-signal-tapereader\plugin"   and   build.bat
REM ===========================================================================
setlocal enabledelayedexpansion

set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=C:\Program Files\LinnSoft\InvestorRT\dllx64"

if not exist "%SDK%\include\irtsdk.h" (
  echo [ERROR] Cannot find the SDK at "%SDK%".
  echo         Edit the SDK path at the top of this file if InvestorRT is elsewhere.
  exit /b 1
)

where cl >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cl.exe is not on PATH.
  echo         Open "x64 Native Tools Command Prompt for VS 2022" and run build.bat from there.
  exit /b 1
)

echo Compiling GammaProfile.cpp ...
cl /nologo /LD /EHsc /O2 /MD GammaProfile.cpp /I "%SDK%\include" /Fe:GammaProfile.dll /link "%LIB143%"
if errorlevel 1 (
  echo.
  echo [BUILD FAILED] See the compiler/linker errors above.
  echo   - LNK2038 RuntimeLibrary mismatch? change /MD to /MDd, or use the V143d lib.
  echo   - "unresolved cpp_main" at load time? rebuild adding:  /link /EXPORT:cpp_main
  exit /b 1
)

echo.
echo [BUILD OK]  GammaProfile.dll created in %cd%
echo.

echo Installing to "%DLLDIR%" ...
if not exist "%DLLDIR%" (
  echo [WARN] dllx64 folder not found at "%DLLDIR%". Copy GammaProfile.dll there manually.
  goto :done
)
copy /Y GammaProfile.dll "%DLLDIR%\GammaProfile.dll" >nul
if errorlevel 1 (
  echo [WARN] Could not copy into Program Files ^(access denied?^).
  echo        Re-run this prompt as Administrator, OR copy GammaProfile.dll into:
  echo        %DLLDIR%
) else (
  echo [INSTALLED]  %DLLDIR%\GammaProfile.dll
)

:done
echo.
echo Next: in Investor/RT, open your EPU26 3-min chart, add indicator -^> RTX -^> GammaProfile.
endlocal
