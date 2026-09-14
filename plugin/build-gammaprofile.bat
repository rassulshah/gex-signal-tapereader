@echo off
REM ===========================================================================
REM  build-gammaprofile.bat  --  compile GammaProfile.cpp -> lsGammaProfile.dll (x64)
REM  Run from an "x64 Native Tools Command Prompt for VS", or let
REM  compile-gammaprofile.bat set up the compiler and call this for you.
REM  Close Investor/RT first: a loaded DLL cannot be replaced.
REM ===========================================================================
setlocal
set "OUT=lsGammaProfile.dll"
set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"

if not exist "%SDK%\include\irtsdk.h" goto :nosdk
where cl >nul 2>nul
if errorlevel 1 goto :nocl

echo Compiling GammaProfile.cpp -^> %OUT% ...
cl /nologo /LD /EHsc /O2 /MD GammaProfile.cpp /I "%SDK%\include" /Fe:%OUT% /link "%LIB143%"
if errorlevel 1 goto :failed

echo.
echo [BUILD OK]  %OUT%
echo.
if not exist "%DLLDIR%" goto :nodir
del "%DLLDIR%\GammaProfile.dll" 2>nul
copy /Y %OUT% "%DLLDIR%\%OUT%" >nul
if errorlevel 1 goto :copyfail
echo [INSTALLED]  %DLLDIR%\%OUT%
goto :ok

:nosdk
echo [ERROR] Cannot find the SDK at "%SDK%". Edit the SDK path at the top of this file.
goto :ok
:nocl
echo [ERROR] cl.exe is not on PATH. Use compile-gammaprofile.bat, or an x64 Native Tools Command Prompt.
goto :ok
:failed
echo.
echo [BUILD FAILED] See the compiler/linker errors above.
goto :ok
:nodir
echo [WARN] Extension folder not found: "%DLLDIR%".  DLL built but not installed.
goto :ok
:copyfail
echo [WARN] Could not replace the DLL -- is Investor/RT still running? Close it fully and rerun.
goto :ok
:ok
endlocal
