@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM   GEX NIGHTLY SETUP  --  RUN ONCE  (v15.68)
REM
REM   Installs a Windows task "GEX nightly" that runs
REM   tools\gex-nightly.bat every 10 minutes under your login,
REM   hidden (through tools\gex-nightly-hidden.vbs, no window):
REM   when the day file you Save at the close is newer than its
REM   nightly log, it runs the nightly (tools\nightly\run.py) -
REM   the log, the pattern table, the study registry the Analysis
REM   tab renders - and the "GEX sync" task pushes the results.
REM   Your end of day stays ONE click: Save.
REM
REM   Needs Python 3 on the PATH (python --version).
REM   NO POWERSHELL -- schtasks only (Avast flags PS: IDP.HELU.PSE88)
REM ============================================================
echo.
echo   GEX NIGHTLY SETUP
echo   =================
echo.

set REPO=C:\Dev\gex-signal-tapereader
if not exist "%REPO%\.git" (
  echo   [X] Repo not found at %REPO%
  echo       Run the normal GEX installer first, then re-run this.
  echo.
  pause
  exit /b 1
)
for %%F in ("tools\gex-nightly.bat" "tools\gex-nightly-hidden.vbs" "tools\nightly\tick.py" "tools\nightly\run.py") do (
  if not exist "%REPO%\%%~F" (
    echo   [X] %REPO%\%%~F is missing - it ships with the installer. Run installv1568.bat first.
    echo.
    pause
    exit /b 1
  )
)
echo   [1] repo found: %REPO%

set PY=
where python >nul 2>&1 && set PY=python
if not defined PY where py >nul 2>&1 && set PY=py -3
if not defined PY (
  echo   [X] Python 3 was not found on the PATH ^(python --version^). Install it, then re-run this.
  echo.
  pause
  exit /b 1
)
for /f "delims=" %%V in ('!PY! --version 2^>^&1') do set PYV=%%V
echo   [2] python: !PYV!

schtasks /Query /TN "GEX sync" >nul 2>&1
if errorlevel 1 (
  echo   [!] the "GEX sync" task is not installed - the nightly's results would never be pushed.
  echo       Run setup-gex-sync.bat first, then re-run this.
  echo.
  pause
  exit /b 1
)
echo   [3] the "GEX sync" task is present ^(it pushes what the nightly writes^)

schtasks /Create /TN "GEX nightly" /TR "wscript.exe //B //Nologo \"%REPO%\tools\gex-nightly-hidden.vbs\"" /SC MINUTE /MO 10 /F >nul 2>&1
if errorlevel 1 (
  echo   [X] Could not create the scheduled task.
  echo       To schedule it by hand: Task Scheduler -^> Create Basic Task -^> "GEX nightly"
  echo       -^> repeat every 10 minutes -^> action: "%REPO%\tools\gex-nightly.bat"
  echo.
  pause
  exit /b 1
)
echo   [4] task "GEX nightly" created: every 10 minutes, your login, window hidden
echo.
echo   Checking what it would do right now...
cd /d "%REPO%"
!PY! tools\nightly\tick.py --check
echo   Log: %REPO%\tools\gex-nightly.log  ^(only written when it runs or fails^)
echo.
echo   DONE. Your end of day is still: click Save on the panel. Within ~10 minutes the
echo   nightly runs here, within ~2 more the sync pushes it, and the panel picks it up
echo   on its next 10-minute check or reload.
echo   To stop it later:  schtasks /Delete /TN "GEX nightly" /F
echo.
pause
exit /b 0
