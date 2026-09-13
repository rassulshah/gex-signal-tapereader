@echo off
REM ===========================================================================
REM  build.bat  —  compile GammaProfile.cpp into lsGammaProfile.dll (x64)
REM
REM  RUN THIS FROM:  "x64 Native Tools Command Prompt for VS" (Run as Administrator
REM  so the install-copy into Program Files succeeds).
REM  Then:   cd /d "C:\Dev\gex-signal-tapereader\plugin"   and   build.bat
REM
REM  NOTE: Investor/RT only loads RTX DLLs whose name starts with "ls", so the
REM  output is lsGammaProfile.dll.
REM ===========================================================================
setlocal enabledelayedexpansion

set "OUT=lsGammaProfile.dll"
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
  echo         Open "x64 Native Tools Command Prompt for VS" and run build.bat from there.
  exit /b 1
)

echo Compiling GammaProfile.cpp -^> %OUT% ...
cl /nologo /LD /EHsc /O2 /MD GammaProfile.cpp /I "%SDK%\include" /Fe:%OUT% /link "%LIB143%"
if errorlevel 1 (
  echo.
  echo [BUILD FAILED] See the compiler/linker errors above.
  echo   - LNK2038 RuntimeLibrary mismatch? change /MD to /MDd, or use the V143d lib.
  echo   - "unresolved cpp_main" at load time? rebuild adding:  /link /EXPORT:cpp_main
  exit /b 1
)

echo.
echo [BUILD OK]  %OUT% created in %cd%
echo.

echo Installing to "%DLLDIR%" ...
if not exist "%DLLDIR%" (
  echo [WARN] dllx64 folder not found at "%DLLDIR%". Copy %OUT% there manually.
  goto :done
)
REM remove the earlier (wrongly-named) build if it is present
del "%DLLDIR%\GammaProfile.dll" 2>nul
copy /Y %OUT% "%DLLDIR%\%OUT%" >nul
if errorlevel 1 (
  echo [WARN] Could not copy into Program Files ^(access denied?^).
  echo        Re-run this prompt as Administrator, OR copy %OUT% into:
  echo        %DLLDIR%
) else (
  echo [INSTALLED]  %DLLDIR%\%OUT%
)

:done
echo.
echo Next: fully quit and reopen Investor/RT, then on the EPU26 3-min chart:
echo   Add Indicator -^> RTX Extensions -^> lsGammaProfile  (between lsFootprint and lsGapRunner).
endlocal
