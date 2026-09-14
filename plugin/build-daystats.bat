@echo off
REM ===========================================================================
REM  build-daystats.bat  —  compile DayStats.cpp -> lsDayStats.dll (x64)  [Day-model STATS strip]
REM
REM  EASIEST: double-click  compile-daystats.bat  (sets up the compiler for you).
REM  Or run this from an "x64 Native Tools Command Prompt for VS".
REM  NO admin needed -- installs to your per-user InvestorRT folder.
REM  Close Investor/RT first: a loaded DLL cannot be replaced.
REM  SEPARATE plugin -- it does not touch lsGammaProfile / lsDayModel / lsDayStats.
REM ===========================================================================
setlocal enabledelayedexpansion

set "OUT=lsDayStats.dll"
set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"

if not exist "%SDK%\include\irtsdk.h" (
  echo [ERROR] Cannot find the SDK at "%SDK%". Edit the SDK path at the top of this file.
  exit /b 1
)
where cl >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cl.exe is not on PATH. Double-click compile-daystats.bat, or open an
  echo         "x64 Native Tools Command Prompt for VS" and run build-daystats.bat there.
  exit /b 1
)

echo Compiling DayStats.cpp -^> %OUT% ...
cl /nologo /LD /EHsc /O2 /MD DayStats.cpp /I "%SDK%\include" /Fe:%OUT% /link "%LIB143%"
if errorlevel 1 (
  echo.
  echo [BUILD FAILED] See the compiler/linker errors above.
  exit /b 1
)
echo.
echo [BUILD OK]  %OUT%
echo.

echo Installing to "%DLLDIR%" ...
if not exist "%DLLDIR%" (
  echo [WARN] Extension folder not found: "%DLLDIR%".
  goto :done
)
del "%DLLDIR%\DayStats.dll" 2>nul
copy /Y %OUT% "%DLLDIR%\%OUT%" >nul
if errorlevel 1 (
  echo [WARN] Could not replace the DLL -- is Investor/RT still running?
  echo        Close it completely ^(all windows + the system-tray icon; check Task Manager^),
  echo        then run this again.
) else (
  echo [INSTALLED]  %DLLDIR%\%OUT%
)

:done
echo.
echo Next: reopen Investor/RT -- it loads the new build on startup.
endlocal
