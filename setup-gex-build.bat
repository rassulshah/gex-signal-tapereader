@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM   GEX BUILD SETUP  --  RUN ONCE  (2026-09-17)
REM
REM   Installs a Windows task "GEX build" that runs
REM   tools\gex-build.bat every 2 minutes under your login,
REM   hidden (tools\gex-build-hidden.vbs, no window): when a
REM   plugin's source changes (Claude writes the .cpp/.h into
REM   this folder over the desktop bridge), it compiles the DLL
REM   into plugin\out\ and installs it into Investor/RT's
REM   extension folder as soon as IRT is closed. After this your
REM   whole install step for a plugin build is: restart IRT.
REM   The compile-*.bat files still work for a manual build.
REM   NO POWERSHELL -- schtasks only (Avast flags PS: IDP.HELU.PSE88)
REM ============================================================
echo.
echo   GEX BUILD SETUP
echo   ===============
echo.
set REPO=C:\Dev\gex-signal-tapereader
if not exist "%REPO%\plugin" ( echo   [X] Repo not found at %REPO% & pause & exit /b 1 )
if not exist "%REPO%\tools\gex-build.bat" ( echo   [X] tools\gex-build.bat is missing & pause & exit /b 1 )
if not exist "%REPO%\tools\gex-build-hidden.vbs" ( echo   [X] tools\gex-build-hidden.vbs is missing & pause & exit /b 1 )
echo   [1] repo found: %REPO%
schtasks /Create /TN "GEX build" /TR "wscript.exe //B //Nologo \"%REPO%\tools\gex-build-hidden.vbs\"" /SC MINUTE /MO 2 /F >nul 2>&1
if errorlevel 1 (
  echo   [X] Could not create the scheduled task.
  echo       By hand: Task Scheduler -^> Create Basic Task -^> "GEX build" -^> repeat every 2 minutes
  echo       -^> action: wscript.exe "%REPO%\tools\gex-build-hidden.vbs"
  pause
  exit /b 1
)
echo   [2] task "GEX build" created: every 2 minutes, your login, window hidden
echo.
echo   Running it once now (the first run builds all four plugins - about a minute)...
call "%REPO%\tools\gex-build.bat"
echo.
if exist "%REPO%\plugin\out\BUILD-STATUS.txt" type "%REPO%\plugin\out\BUILD-STATUS.txt"
echo.
echo   Log: %REPO%\tools\gex-build.log
echo   DONE. From now on: a plugin build lands, compiles itself, and installs when Investor/RT
echo   is closed. Your only step is to restart Investor/RT when the log says INSTALLED.
echo   To stop it later:  schtasks /Delete /TN "GEX build" /F
echo.
pause
exit /b 0
