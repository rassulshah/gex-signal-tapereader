@echo off
setlocal EnableDelayedExpansion
REM ============================================================================
REM  GEX BUILD  -- installed by setup-gex-build.bat  (2026-09-17)
REM  Every 2 minutes, hidden: for each RTX plugin, if its source (the .cpp + the
REM  logic headers) changed since the last build, compile it into plugin\out\;
REM  then, if the built DLL is not the installed one, try to copy it into the
REM  Investor/RT extension folder. A loaded DLL cannot be replaced, so while IRT
REM  is open the copy fails harmlessly and is retried every tick -- close IRT
REM  and the new build installs itself within two minutes; reopen IRT to load it.
REM  Nothing changed = nothing happens. Never interactive.
REM  Log: tools\gex-build.log   Status: plugin\out\BUILD-STATUS.txt
REM  NO POWERSHELL ANYWHERE (Avast flags it: IDP.HELU.PSE88).
REM ============================================================================
set REPO=C:\Dev\gex-signal-tapereader
set PLUG=%REPO%\plugin
set OUT=%PLUG%\out
set LOGF=%REPO%\tools\gex-build.log
set LOCK=%REPO%\tools\gex-build.lock
set STATUS=%OUT%\BUILD-STATUS.txt
set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"

if not exist "%PLUG%" exit /b 1
if exist "%LOCK%" exit /b 0
echo %DATE% %TIME% > "%LOCK%"
if not exist "%OUT%" mkdir "%OUT%"

set VCOK=
set ANY=
call :one GammaProfile lsGammaProfile GammaProfileLogic.h ContractOffsetLogic.h
call :one DayModel     lsDayModel     DayModelLogic.h
call :one DayStats     lsDayStats     DayStatsLogic.h
call :one KingTracker  lsKingTracker  KingTrackerLogic.h ContractOffsetLogic.h
del "%LOCK%" >nul 2>&1
exit /b 0

REM ---- :one <Name> <lsName> <header...> ---------------------------------------
:one
set NAME=%~1
set DLL=%~2.dll
set SRCHASH=
call :hashof "%PLUG%\%NAME%.cpp"
set SRCHASH=!H!
for %%X in (%3 %4 %5) do (
  if exist "%PLUG%\%%X" ( call :hashof "%PLUG%\%%X" & set SRCHASH=!SRCHASH!-!H! )
)
set BUILT=
if exist "%OUT%\%NAME%.built" set /p BUILT=<"%OUT%\%NAME%.built"
if not "!BUILT!"=="!SRCHASH!" (
  call :vc
  if not defined VCOK ( call :log %NAME%: VISUAL STUDIO C++ TOOLS NOT FOUND - cannot build & goto :eof )
  cd /d "%PLUG%"
  cl /nologo /LD /EHsc /O2 /MD %NAME%.cpp /I "%SDK%\include" /Fo"%OUT%\\" /Fe:"%OUT%\%DLL%" /link "%LIB143%" > "%OUT%\%NAME%.compile.txt" 2>&1
  if errorlevel 1 (
    call :log %NAME%: BUILD FAILED - see plugin\out\%NAME%.compile.txt
    call :status
    goto :eof
  )
  > "%OUT%\%NAME%.built" echo !SRCHASH!
  set BUILT=!SRCHASH!
  call :ver
  call :log %NAME%: BUILT %DLL% !VER!
)
REM ---- install when the built DLL is not the installed one
set INST=
if exist "%OUT%\%NAME%.installed" set /p INST=<"%OUT%\%NAME%.installed"
if "!INST!"=="!BUILT!" goto :eof
if not exist "%OUT%\%DLL%" goto :eof
if not exist "%DLLDIR%" ( call :log %NAME%: extension folder not found: %DLLDIR% & goto :eof )
copy /Y "%OUT%\%DLL%" "%DLLDIR%\%DLL%" >nul 2>&1
if errorlevel 1 (
  REM the DLL is loaded (Investor/RT open) - retried next tick; say so once per build
  if not exist "%OUT%\%NAME%.pending" ( call :log %NAME%: PENDING - close Investor/RT and it installs itself within 2 min & > "%OUT%\%NAME%.pending" echo !BUILT! )
  call :status
  goto :eof
)
> "%OUT%\%NAME%.installed" echo !BUILT!
del "%OUT%\%NAME%.pending" >nul 2>&1
call :log %NAME%: INSTALLED %DLLDIR%\%DLL% - reopen Investor/RT to load it
call :status
goto :eof

REM ---- :ver  -> VER  the setVersion("x.y") of the plugin (outside any block: the delims hold parentheses) --
:ver
set VER=
for /f "tokens=2 delims=()" %%v in ('findstr /c:"setVersion(" "%PLUG%\%NAME%.cpp"') do set VER=%%v
goto :eof

REM ---- :hashof <file>  -> H  (certutil, no PowerShell) -------------------------
:hashof
set H=
for /f "usebackq delims=" %%l in (`certutil -hashfile "%~1" SHA1 ^| findstr /v /i "hash CertUtil"`) do set H=%%l
set H=!H: =!
goto :eof

REM ---- :vc  load the x64 compiler once -----------------------------------------
:vc
if defined VCOK goto :eof
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

REM ---- :status  one file a human (or Claude, over the bridge) can read at a glance -
:status
> "%STATUS%" echo GEX BUILD STATUS  %DATE% %TIME%
for %%N in (GammaProfile DayModel DayStats KingTracker) do (
  set B=& set I=& set P=
  if exist "%OUT%\%%N.built" set /p B=<"%OUT%\%%N.built"
  if exist "%OUT%\%%N.installed" set /p I=<"%OUT%\%%N.installed"
  if exist "%OUT%\%%N.pending" set "P=- PENDING, close Investor/RT and it installs itself" 
  if "!B!"=="" ( >> "%STATUS%" echo %%N: not built yet ) else if "!B!"=="!I!" ( >> "%STATUS%" echo %%N: installed ) else ( >> "%STATUS%" echo %%N: built, not installed !P! )
)
goto :eof
