@echo off
REM ===========================================================================
REM  build.bat  —  compile GammaProfile.cpp -> lsGammaProfile.dll (x64)
REM
REM  EASIEST: just double-click  compile.bat  (sets up the compiler for you).
REM  Or run this from an "x64 Native Tools Command Prompt for VS".
REM  NO admin needed — it installs to your per-user InvestorRT folder.
REM  Close Investor/RT first: a loaded DLL cannot be replaced.
REM ===========================================================================
setlocal enabledelayedexpansion

set "OUT=lsGammaProfile.dll"
set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"

if not exist "%SDK%\include\irtsdk.h" (
  echo [ERROR] Cannot find the SDK at "%SDK%". Edit the SDK path at the top of this file.
  exit /b 1
)
where cl >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cl.exe is not on PATH. Double-click compile.bat, or open an
  echo         "x64 Native Tools Command Prompt for VS" and run build.bat there.
  exit /b 1
)

echo Compiling GammaProfile.cpp -^> %OUT% ...
cl /nologo /LD /EHsc /O2 /MD GammaProfile.cpp /I "%SDK%\include" /Fe:%OUT% /link "%LIB143%"
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
del "%DLLDIR%\GammaProfile.dll" 2>nul
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
