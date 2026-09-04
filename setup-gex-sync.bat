@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM   GEX SYNC SETUP  --  RUN ONCE  (v15.62)
REM
REM   Installs a Windows task "GEX sync" that runs
REM   tools\gex-sync.bat every 2 minutes under your login,
REM   hidden (through tools\gex-sync-hidden.vbs, no window):
REM   it commits and pushes anything new in the repo - the day
REM   file you Save at the close, the nightly's log and tables
REM   Claude writes into the folder, a Drive-inbox drop if any.
REM   After this you never run pushdata.bat again.
REM
REM   Replaces the old "GEX auto-pull" task if it exists.
REM   NO POWERSHELL -- schtasks only (Avast flags PS: IDP.HELU.PSE88)
REM ============================================================
echo.
echo   GEX SYNC SETUP
echo   ==============
echo.

set REPO=C:\Dev\gex-signal-tapereader
if not exist "%REPO%\.git" (
  echo   [X] Repo not found at %REPO%
  echo       Run the normal GEX installer first, then re-run this.
  echo.
  pause
  exit /b 1
)
if not exist "%REPO%\tools\gex-sync.bat" (
  echo   [X] %REPO%\tools\gex-sync.bat is missing - it ships with this file.
  echo.
  pause
  exit /b 1
)

echo   [1] repo found: %REPO%
echo   [2] sync script: %REPO%\tools\gex-sync.bat

schtasks /Query /TN "GEX auto-pull" >nul 2>&1
if not errorlevel 1 (
  schtasks /Delete /TN "GEX auto-pull" /F >nul 2>&1
  echo   [3] removed the old "GEX auto-pull" task ^(the sync replaces it^)
)

REM the task runs the hidden launcher (wscript, window style 0) so no cmd window pops up every two minutes
if not exist "%REPO%\tools\gex-sync-hidden.vbs" (
  echo   [X] %REPO%\tools\gex-sync-hidden.vbs is missing - it ships with this file.
  echo.
  pause
  exit /b 1
)
schtasks /Create /TN "GEX sync" /TR "wscript.exe //B //Nologo \"%REPO%\tools\gex-sync-hidden.vbs\"" /SC MINUTE /MO 2 /F >nul 2>&1
if errorlevel 1 (
  echo   [X] Could not create the scheduled task.
  echo       To schedule it by hand: Task Scheduler -^> Create Basic Task -^> "GEX sync"
  echo       -^> repeat every 2 minutes -^> action: "%REPO%\tools\gex-sync.bat"
  echo.
  pause
  exit /b 1
)
echo   [4] task "GEX sync" created: every 2 minutes, your login, window hidden
echo.
echo   Running it once now...
call "%REPO%\tools\gex-sync.bat"
echo   Log: %REPO%\tools\gex-sync.log
echo.
echo   DONE. Your end of day is now: click Save on the panel. That's all.
echo   To stop it later:  schtasks /Delete /TN "GEX sync" /F
echo.
pause
exit /b 0
