@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  GEX SYNC  -- installed by setup-gex-sync.bat  (v15.62)
REM  Every 2 minutes: commit and push ANYTHING new in the repo -
REM  the day file the panel saves at the close, the nightly's log
REM  and tables that Claude writes into the folder over the
REM  desktop bridge, a Drive-inbox drop if one is there.
REM  Nothing to commit = nothing happens. Never interactive.
REM  NO POWERSHELL ANYWHERE (Avast flags it: IDP.HELU.PSE88).
REM ============================================================
set REPO=C:\Dev\gex-signal-tapereader
set LOGF=%REPO%\tools\gex-sync.log
set LOCK=%REPO%\tools\gex-sync.lock

if not exist "%REPO%\.git" exit /b 1
if exist "%LOCK%" (
  REM a previous run is still going (a slow push) - skip this tick
  exit /b 0
)
echo %DATE% %TIME% > "%LOCK%"

set GIT=
where git >nul 2>&1 && set GIT=git
if not defined GIT if exist "C:\Program Files\Git\cmd\git.exe" set GIT=C:\Program Files\Git\cmd\git.exe
if not defined GIT if exist "C:\Program Files (x86)\Git\cmd\git.exe" set GIT=C:\Program Files (x86)\Git\cmd\git.exe
if not defined GIT if exist "%LOCALAPPDATA%\Programs\Git\cmd\git.exe" set GIT=%LOCALAPPDATA%\Programs\Git\cmd\git.exe
if not defined GIT (
  for /d %%D in ("%LOCALAPPDATA%\GitHubDesktop\app-*") do if exist "%%D\resources\app\git\cmd\git.exe" set GIT=%%D\resources\app\git\cmd\git.exe
)
if not defined GIT (
  echo %DATE% %TIME% GIT NOT FOUND >> "%LOGF%"
  del "%LOCK%" >nul 2>&1
  exit /b 1
)

REM ---- optional: a Drive inbox drop (patch or tarball), if Google Drive is set up ----
set INBOX=
for %%P in ("%USERPROFILE%\My Drive\GEX-inbox" "%USERPROFILE%\Google Drive\GEX-inbox") do if not defined INBOX if exist "%%~P" set INBOX=%%~P
if defined INBOX (
  if not exist "%INBOX%\_done" mkdir "%INBOX%\_done"
  cd /d "%REPO%"
  for %%F in ("%INBOX%\gex-patch-*.patch") do (
    "!GIT!" apply --3way --whitespace=nowarn "%%F" >> "%LOGF%" 2>&1
    if errorlevel 1 (
      echo %DATE% %TIME% PATCH FAILED %%~nxF - left in inbox >> "%LOGF%"
      "!GIT!" checkout -- . >> "%LOGF%" 2>&1
    ) else (
      echo %DATE% %TIME% applied %%~nxF >> "%LOGF%"
      move /Y "%%F" "%INBOX%\_done\" >nul
    )
  )
  for %%F in ("%INBOX%\gex-drop-*.tar.gz") do (
    tar -xzf "%%F" -C "%REPO%" >> "%LOGF%" 2>&1
    if errorlevel 1 (
      echo %DATE% %TIME% EXTRACT FAILED %%~nxF >> "%LOGF%"
    ) else (
      echo %DATE% %TIME% extracted %%~nxF >> "%LOGF%"
      move /Y "%%F" "%INBOX%\_done\" >nul
    )
  )
)

REM ---- commit whatever is new (the sync's own log and lock are never committed) ----
cd /d "%REPO%"
"!GIT!" add -A -- . ":(exclude)tools/gex-sync.log" ":(exclude)tools/gex-sync.lock" ":(exclude)tools/gex-pull.log" >> "%LOGF%" 2>&1
"!GIT!" diff --cached --quiet
if errorlevel 1 (
  for /f "tokens=1-3 delims=/ " %%a in ("%DATE%") do set D=%%c-%%a-%%b
  "!GIT!" commit -q -m "gex: sync !D! %TIME:~0,5%" >> "%LOGF%" 2>&1
  echo %DATE% %TIME% committed >> "%LOGF%"
)

REM ---- push whatever is pending (a commit from this run, or one an installer left behind) ----
"!GIT!" rev-list --count @{u}..HEAD > "%TEMP%\gex-ahead.txt" 2>nul
set /p AHEAD=<"%TEMP%\gex-ahead.txt"
if "!AHEAD!"=="" set AHEAD=0
if not "!AHEAD!"=="0" (
  "!GIT!" pull --rebase -q >> "%LOGF%" 2>&1
  if errorlevel 1 (
    echo %DATE% %TIME% PULL --rebase FAILED - will retry next tick >> "%LOGF%"
    "!GIT!" rebase --abort >nul 2>&1
  ) else (
    "!GIT!" push -q >> "%LOGF%" 2>&1
    if errorlevel 1 (
      echo %DATE% %TIME% PUSH FAILED - will retry next tick >> "%LOGF%"
    ) else (
      echo %DATE% %TIME% pushed !AHEAD! commit^(s^) >> "%LOGF%"
    )
  )
)

del "%LOCK%" >nul 2>&1
exit /b 0
