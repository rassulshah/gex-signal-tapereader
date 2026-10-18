@echo off
setlocal EnableDelayedExpansion
REM ============================================================================
REM  GEX HOT BUILD  (2026-10-18)  -- hot-reload indicators: a new build loads on a chart refresh, no IRT restart
REM  Rassul 2026-10-18 15:17 CT: "is there a way for you to do the build without me having to restart every time,
REM  maybe i can just refresh the chart instead?"   Design + test steps: docs\build\HOT-RELOAD.md
REM
REM  Called by gex-build.bat every 2 minutes (hidden):   call "%REPO%\tools\gex-hot-build.bat" HotReloadDemo
REM  For plugin\<Name>\ :
REM   1. IMPLEMENTATION  <Name>Impl.cpp  (+ every .h in plugin\<Name>\ and plugin\HostSlot.h)
REM      changed -> compiled with /DHR_BUILD=<n> (n = 1, 2, 3 ... one per build) into plugin\out\hot\<Name>\
REM      -> copied under its NEW name ls<Name>_impl_<n>.dll to %USERPROFILE%\InvestorRT\hotreload\  (a new file: never
REM         in use, so this works while Investor/RT is OPEN)
REM      -> ls<Name>.manifest written as .tmp and moved over the old one (one atomic step). The shim reads it on the next
REM         refresh of the chart and switches that chart to the new build.
REM   2. SHIM  HotShim.cpp + HotReloadLogic.h -> ls<Name>.dll -> dllx64\ls<Name>.dll  - like every plugin, this copy only
REM      succeeds when Investor/RT is closed (a loaded DLL cannot be replaced). It changes rarely. Installed only after a
REM      manifest exists, so IRT never loads a shim with nothing to forward to.
REM  Old implementation copies: all but the newest 5 are deleted (a copy still in use cannot be deleted - harmless).
REM  Status: plugin\out\hot\<Name>.status.txt   Log: tools\gex-build.log   NO POWERSHELL ANYWHERE.
REM ============================================================================
set "NAME=%~1"
if "%NAME%"=="" exit /b 1
set "REPO=C:\Dev\gex-signal-tapereader"
set "PLUG=%REPO%\plugin"
set "SRC=%PLUG%\%NAME%"
set "HOUT=%PLUG%\out\hot\%NAME%"
set "LOGF=%REPO%\tools\gex-build.log"
set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"
set "HOTDIR=%USERPROFILE%\InvestorRT\hotreload"
set "LS=ls%NAME%"
set "HSTATUS=%PLUG%\out\hot\%NAME%.status.txt"
if not exist "%SRC%\%NAME%Impl.cpp" exit /b 1
if not exist "%SRC%\HotShim.cpp" exit /b 1
if not exist "%HOUT%" mkdir "%HOUT%"
if not exist "%HOTDIR%" mkdir "%HOTDIR%"
set VCOK=

call :impl
call :shim
call :status
exit /b 0

REM ---- 1. the implementation ----------------------------------------------------
:impl
set "HH="
call :hashof "%SRC%\%NAME%Impl.cpp"
set "HH=!H!"
for %%F in ("%SRC%\*.h") do ( call :hashof "%%~fF" & set "HH=!HH!-!H!" )
if exist "%PLUG%\HostSlot.h" ( call :hashof "%PLUG%\HostSlot.h" & set "HH=!HH!-!H!" )
set "IB="
if exist "%HOUT%\impl.built" set /p IB=<"%HOUT%\impl.built"
if "!IB!"=="!HH!" goto :eof
call :vc
if not defined VCOK ( call :log %NAME% hot: VISUAL STUDIO C++ TOOLS NOT FOUND - cannot build & goto :eof )
set SEQ=0
if exist "%HOUT%\impl.seq" set /p SEQ=<"%HOUT%\impl.seq"
set /a SEQ=SEQ+1
set "IMPL=%LS%_impl_!SEQ!.dll"
cd /d "%SRC%"
cl /nologo /LD /EHsc /O2 /MD %NAME%Impl.cpp /I "%SDK%\include" /I "%PLUG%" /I "%SRC%" /DHR_BUILD=!SEQ! /Fo"%HOUT%\\" /Fe:"%HOUT%\!IMPL!" /link "%LIB143%" > "%HOUT%\impl.compile.txt" 2>&1
if errorlevel 1 (
  call :log %NAME% hot: IMPLEMENTATION BUILD FAILED - see plugin\out\hot\%NAME%\impl.compile.txt
  > "%HOUT%\impl.result" echo BUILD FAILED build !SEQ! - see impl.compile.txt
  goto :eof
)
REM the build number is used up even if publishing fails below: the next build gets a new, never-used file name
> "%HOUT%\impl.seq" echo !SEQ!
copy /Y "%HOUT%\!IMPL!" "%HOTDIR%\!IMPL!" >nul 2>&1
if errorlevel 1 (
  call :log %NAME% hot: could not copy !IMPL! to %HOTDIR% - retried on the next change
  > "%HOUT%\impl.result" echo COPY FAILED build !SEQ!
  goto :eof
)
> "%HOTDIR%\%LS%.manifest.tmp" (
  echo HOTRELOAD 1
  echo NAME %LS%
  echo BUILD !SEQ!
  echo FILE !IMPL!
  echo SWAP refresh
)
move /Y "%HOTDIR%\%LS%.manifest.tmp" "%HOTDIR%\%LS%.manifest" >nul 2>&1
if errorlevel 1 (
  call :log %NAME% hot: could not publish the manifest for build !SEQ!
  > "%HOUT%\impl.result" echo MANIFEST FAILED build !SEQ!
  goto :eof
)
> "%HOUT%\impl.built" echo !HH!
call :ver
> "%HOUT%\impl.result" echo PUBLISHED build !SEQ! !VER! - refresh the chart to load it
call :log %NAME% hot: PUBLISHED %HOTDIR%\!IMPL! build !SEQ! !VER! - refresh the chart to load it (no restart)
REM keep the newest 5 copies; one still loaded by IRT refuses the delete - harmless
set /a KEEP=SEQ-5
for %%F in ("%HOTDIR%\%LS%_impl_*.dll") do (
  set "FN=%%~nF"
  set "NUM=!FN:*_impl_=!"
  set NUMV=999999999
  set /a NUMV=NUM+0 >nul 2>&1
  if !NUMV! LSS !KEEP! del "%%~fF" >nul 2>&1
)
goto :eof

REM ---- 2. the shim (installs only while IRT is closed, like every plugin) -------
:shim
set "SH="
call :hashof "%SRC%\HotShim.cpp"
set "SH=!H!"
call :hashof "%SRC%\HotReloadLogic.h"
set "SH=!SH!-!H!"
set "SB="
if exist "%HOUT%\shim.built" set /p SB=<"%HOUT%\shim.built"
if not "!SB!"=="!SH!" (
  call :vc
  if not defined VCOK ( call :log %NAME% hot: VISUAL STUDIO C++ TOOLS NOT FOUND - cannot build the shim & goto :eof )
  cd /d "%SRC%"
  cl /nologo /LD /EHsc /O2 /MD HotShim.cpp /I "%SRC%" /Fo"%HOUT%\\" /Fe:"%HOUT%\%LS%.dll" > "%HOUT%\shim.compile.txt" 2>&1
  if errorlevel 1 (
    call :log %NAME% hot: SHIM BUILD FAILED - see plugin\out\hot\%NAME%\shim.compile.txt
    goto :eof
  )
  > "%HOUT%\shim.built" echo !SH!
  set "SB=!SH!"
  call :log %NAME% hot: BUILT shim %LS%.dll
)
if not exist "%HOTDIR%\%LS%.manifest" goto :eof
set "SI="
if exist "%HOUT%\shim.installed" set /p SI=<"%HOUT%\shim.installed"
if "!SI!"=="!SB!" goto :eof
if not exist "%DLLDIR%" ( call :log %NAME% hot: extension folder not found: %DLLDIR% & goto :eof )
copy /Y "%HOUT%\%LS%.dll" "%DLLDIR%\%LS%.dll" >nul 2>&1
if errorlevel 1 (
  if not exist "%HOUT%\shim.pending" ( call :log %NAME% hot: shim PENDING - close Investor/RT once and it installs itself within 2 min & > "%HOUT%\shim.pending" echo !SB! )
  goto :eof
)
> "%HOUT%\shim.installed" echo !SB!
del "%HOUT%\shim.pending" >nul 2>&1
call :log %NAME% hot: INSTALLED shim %DLLDIR%\%LS%.dll - reopen Investor/RT once to load it
goto :eof

REM ---- :status  one file to read at a glance ---------------------------------------
:status
set "R=" & set "B=" & set "I="
if exist "%HOUT%\impl.result" set /p R=<"%HOUT%\impl.result"
if exist "%HOUT%\shim.built" set /p B=<"%HOUT%\shim.built"
if exist "%HOUT%\shim.installed" set /p I=<"%HOUT%\shim.installed"
> "%HSTATUS%" echo HOT BUILD %NAME%  %DATE% %TIME%
if defined R ( >> "%HSTATUS%" echo implementation: !R! ) else ( >> "%HSTATUS%" echo implementation: not built yet )
if "!B!"=="" ( >> "%HSTATUS%" echo shim: not built yet ) else if "!B!"=="!I!" ( >> "%HSTATUS%" echo shim: installed ) else ( >> "%HSTATUS%" echo shim: built, not installed - close Investor/RT once )
>> "%HSTATUS%" echo the shim's own report: %HOTDIR%\%LS%.status.txt
goto :eof

REM ---- :ver -> VER  the setVersion("x.y.z") literal of the implementation --------
:ver
set VER=
for /f "tokens=2 delims=()" %%v in ('findstr /c:"setVersion(" "%SRC%\%NAME%Impl.cpp"') do set VER=%%v
goto :eof

REM ---- :hashof <file> -> H  (certutil, no PowerShell) ---------------------------
:hashof
set H=
for /f "usebackq delims=" %%l in (`certutil -hashfile "%~1" SHA1 ^| findstr /v /i "hash CertUtil"`) do set H=%%l
set H=!H: =!
goto :eof

REM ---- :vc  load the x64 compiler once (skipped when the caller already has cl) ---
:vc
if defined VCOK goto :eof
where cl >nul 2>&1
if not errorlevel 1 ( set VCOK=1 & goto :eof )
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :eof
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST goto :eof
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 goto :eof
if not exist "%SDK%\include\irtsdk.h" goto :eof
set VCOK=1
goto :eof

REM ---- :log <text> ---------------------------------------------------------------
:log
echo %DATE% %TIME% %* >> "%LOGF%"
goto :eof
