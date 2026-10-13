@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  GEX SYNC  -- installed by setup-gex-sync.bat  (v15.62; 2026-10-13 SEC100: staged-content secrets scan before commit / push)
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
set RC=0

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
  set RC=1
  goto done
)

REM ---- optional: a Drive inbox drop (patch or tarball), if Google Drive is set up ----
set INBOX=
for %%P in ("%USERPROFILE%\My Drive\GEX-inbox" "%USERPROFILE%\Google Drive\GEX-inbox") do if not defined INBOX if exist "%%~P" set INBOX=%%~P
if defined INBOX (
  if not exist "%INBOX%\_done" mkdir "%INBOX%\_done"
  cd /d "%REPO%"
  for %%F in ("%INBOX%\gex-patch-*.patch") do if exist "%%~fF" (
    "!GIT!" apply --check --whitespace=nowarn "%%F" >> "%LOGF%" 2>&1
    if errorlevel 1 (
      echo %DATE% %TIME% PATCH REJECTED %%~nxF - left in inbox, worktree unchanged >> "%LOGF%"
      set RC=1
    ) else (
      "!GIT!" apply --3way --whitespace=nowarn "%%F" >> "%LOGF%" 2>&1
      if errorlevel 1 (
        echo %DATE% %TIME% PATCH APPLY FAILED %%~nxF - left in inbox; resolve manually, no reset was run >> "%LOGF%"
        set RC=1
      ) else (
        move /Y "%%F" "%INBOX%\_done\" >nul
        if errorlevel 1 (
          echo %DATE% %TIME% PATCH APPLIED but archive move failed %%~nxF >> "%LOGF%"
          set RC=1
        ) else (
          echo %DATE% %TIME% applied %%~nxF >> "%LOGF%"
        )
      )
    )
  )
  if exist "%INBOX%\gex-drop-*.tar.gz" (
    set PY=
    py -3.13 --version >nul 2>&1 && set PY=py -3.13
    if not defined PY py -3 --version >nul 2>&1 && set PY=py -3
    if not defined PY python --version >nul 2>&1 && set PY=python
    if not defined PY (
      echo %DATE% %TIME% TAR DROP BLOCKED - Python is required for safe-gex-inbox.py >> "%LOGF%"
      set RC=1
    ) else if not exist "%~dp0safe-gex-inbox.py" (
      echo %DATE% %TIME% TAR DROP BLOCKED - safe-gex-inbox.py is missing >> "%LOGF%"
      set RC=1
    ) else for %%F in ("%INBOX%\gex-drop-*.tar.gz") do if exist "%%~fF" (
      %PY% "%~dp0safe-gex-inbox.py" "%%~fF" "%REPO%" >> "%LOGF%" 2>&1
      if errorlevel 1 (
        echo %DATE% %TIME% TAR DROP BLOCKED %%~nxF - left in inbox >> "%LOGF%"
        set RC=1
      ) else (
        move /Y "%%F" "%INBOX%\_done\" >nul
        if errorlevel 1 (
          echo %DATE% %TIME% TAR DROP EXTRACTED but archive move failed %%~nxF >> "%LOGF%"
          set RC=1
        ) else (
          echo %DATE% %TIME% extracted %%~nxF safely >> "%LOGF%"
        )
      )
    )
  )
)

REM ---- commit whatever is new (the sync's own log and lock are never committed) ----
cd /d "%REPO%"
REM (2026-10-13) the sync log / lock are in .gitignore: naming them in an exclude pathspec made git add fail ("paths are ignored") on every tick since 03:42
"!GIT!" add -A -- . ":(exclude)tools/gex-pull.log" >> "%LOGF%" 2>&1
if errorlevel 1 (
  echo %DATE% %TIME% GIT ADD FAILED >> "%LOGF%"
  set RC=1
  goto done
)
REM ---- (2026-10-13 SEC100) scan the staged CONTENT with the LRA secrets scanner before anything is committed or pushed ----
REM  safe-staged-scan.py (LRA repo) reads the exact staged blobs with the BK100 scanner and prints COUNTS ONLY - never a path or the secret.
REM  A hit (or a scanner that cannot run) unstages everything, prints BLOCKED - secrets scanner, and skips the commit AND the push.
set STAGED_SCANNER=C:\Dev\level-reversal-analytics\tools\lra\safe-staged-scan.py
set PY=
py -3.13 --version >nul 2>&1 && set PY=py -3.13
if not defined PY py -3 --version >nul 2>&1 && set PY=py -3
if not defined PY python --version >nul 2>&1 && set PY=python
set SCANOK=1
if not exist "%STAGED_SCANNER%" set SCANOK=0
if not defined PY set SCANOK=0
if "!SCANOK!"=="1" (
  %PY% "%STAGED_SCANNER%" --git "!GIT!" --repo "%REPO%" >> "%LOGF%" 2>&1
  if errorlevel 1 set SCANOK=0
)
if "!SCANOK!"=="0" (
  "!GIT!" reset -q >nul 2>&1
  echo BLOCKED - secrets scanner
  echo %DATE% %TIME% BLOCKED - secrets scanner: staged content hit the BK100 scanner ^(or it could not run^); nothing committed, push skipped >> "%LOGF%"
  set RC=3
  goto done
)
"!GIT!" diff --cached --quiet
if errorlevel 2 (
  echo %DATE% %TIME% STAGED-DIFF CHECK FAILED >> "%LOGF%"
  set RC=1
  goto done
) else if errorlevel 1 (
  for /f "tokens=1-3 delims=/ " %%a in ("%DATE%") do set D=%%c-%%a-%%b
  "!GIT!" commit -q -m "gex: sync !D! %TIME:~0,5%" >> "%LOGF%" 2>&1
  if errorlevel 1 (
    echo %DATE% %TIME% COMMIT FAILED >> "%LOGF%"
    set RC=1
    goto done
  ) else (
    echo %DATE% %TIME% committed >> "%LOGF%"
  )
)

REM ---- push whatever is pending (a commit from this run, or one an installer left behind) ----
"!GIT!" rev-list --count @{u}..HEAD > "%TEMP%\gex-ahead.txt" 2>nul
if errorlevel 1 (
  echo %DATE% %TIME% UPSTREAM CHECK FAILED >> "%LOGF%"
  set RC=1
  goto done
)
set /p AHEAD=<"%TEMP%\gex-ahead.txt"
if "!AHEAD!"=="" set AHEAD=0
if not "!AHEAD!"=="0" (
  "!GIT!" pull --rebase -q >> "%LOGF%" 2>&1
  if errorlevel 1 (
    echo %DATE% %TIME% PULL --rebase FAILED - will retry next tick >> "%LOGF%"
    "!GIT!" rebase --abort >nul 2>&1
    set RC=1
  ) else (
    "!GIT!" push -q >> "%LOGF%" 2>&1
    if errorlevel 1 (
      echo %DATE% %TIME% PUSH FAILED - will retry next tick >> "%LOGF%"
      set RC=1
    ) else (
      echo %DATE% %TIME% pushed !AHEAD! commit^(s^) >> "%LOGF%"
    )
  )
)

:done
del "%LOCK%" >nul 2>&1
exit /b %RC%
