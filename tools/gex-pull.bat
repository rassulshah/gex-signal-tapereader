@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  GEX AUTO-PULL  -- installed by setup-gex-autopull.bat
REM  Watches the Google Drive folder "GEX-inbox" for gex-drop-*.tar.gz,
REM  extracts each into the repo (tar preserves relative paths),
REM  then commits and pushes. Runs on a schedule; never interactive.
REM  NO POWERSHELL ANYWHERE -- Avast flags it (IDP.HELU.PSE88).
REM ============================================================
set REPO=C:\Dev\gex-signal-tapereader
set LOGF=%REPO%\tools\gex-pull.log

set INBOX=
for %%P in ("%USERPROFILE%\My Drive\GEX-inbox" "%USERPROFILE%\Google Drive\GEX-inbox" "G:\My Drive\GEX-inbox" "H:\My Drive\GEX-inbox") do if not defined INBOX if exist "%%~P" set INBOX=%%~P
if not defined INBOX for %%D in (D E F G H I J K L M N O P Q R S T U V W X Y Z) do if not defined INBOX if exist "%%D:\My Drive\GEX-inbox" set INBOX=%%D:\My Drive\GEX-inbox
if not defined INBOX (
  echo %DATE% %TIME% inbox not found - is Google Drive for desktop running with "GEX-inbox" in My Drive? >> "%LOGF%"
  exit /b 1
)
if not exist "%REPO%\.git" (
  echo %DATE% %TIME% repo not found at %REPO% >> "%LOGF%"
  exit /b 1
)
if not exist "%INBOX%\_done" mkdir "%INBOX%\_done"

REM ---- find git FIRST: patches need it, tarballs need it only to commit ----
set GIT=
where git >nul 2>&1 && set GIT=git
if not defined GIT if exist "C:\Program Files\Git\cmd\git.exe" set GIT=C:\Program Files\Git\cmd\git.exe
if not defined GIT if exist "C:\Program Files (x86)\Git\cmd\git.exe" set GIT=C:\Program Files (x86)\Git\cmd\git.exe
if not defined GIT if exist "%LOCALAPPDATA%\Programs\Git\cmd\git.exe" set GIT=%LOCALAPPDATA%\Programs\Git\cmd\git.exe
if not defined GIT (
  for /d %%D in ("%LOCALAPPDATA%\GitHubDesktop\app-*") do if exist "%%D\resources\app\git\cmd\git.exe" set GIT=%%D\resources\app\git\cmd\git.exe
)

set GOT=0

REM ---- PATCHES FIRST. Kilobytes instead of megabytes, and they carry DELETES and RENAMES,
REM ---- which a tarball never can (a tarball only adds or overwrites, so a rename would
REM ---- silently leave the old file behind). --3way verifies a clean apply; on failure the
REM ---- tree is restored and NOTHING is committed.
for %%F in ("%INBOX%\gex-patch-*.patch") do (
  if not defined GIT (
    echo %DATE% %TIME% GIT NOT FOUND - cannot apply %%~nxF, left in inbox >> "%LOGF%"
  ) else (
    cd /d "%REPO%"
    "!GIT!" apply --3way --whitespace=nowarn "%%F" >> "%LOGF%" 2>&1
    if errorlevel 1 (
      echo %DATE% %TIME% PATCH FAILED to apply: %%~nxF - left in inbox, NOTHING committed >> "%LOGF%"
      "!GIT!" checkout -- . >> "%LOGF%" 2>&1
    ) else (
      echo %DATE% %TIME% applied patch %%~nxF >> "%LOGF%"
      set /a GOT+=1
      move /Y "%%F" "%INBOX%\_done\" >nul
    )
  )
)

REM ---- TARBALLS: new files, binaries, or a full replacement ----
for %%F in ("%INBOX%\gex-drop-*.tar.gz") do (
  echo %DATE% %TIME% extracting %%~nxF >> "%LOGF%"
  tar -xzf "%%F" -C "%REPO%" >> "%LOGF%" 2>&1
  if errorlevel 1 (
    echo %DATE% %TIME% EXTRACT FAILED for %%~nxF - left in place, NOT committed >> "%LOGF%"
  ) else (
    set /a GOT+=1
    move /Y "%%F" "%INBOX%\_done\" >nul
  )
)
if !GOT! EQU 0 exit /b 0

if not defined GIT ( echo %DATE% %TIME% GIT NOT FOUND - files extracted but NOT pushed >> "%LOGF%" & exit /b 1 )

cd /d "%REPO%"
"!GIT!" add -A
"!GIT!" diff --cached --quiet && ( echo %DATE% %TIME% extracted !GOT! drop^(s^) but nothing changed >> "%LOGF%" & exit /b 0 )
set MSG=
if exist "%REPO%\tools\.gex-drop-msg" set /p MSG=<"%REPO%\tools\.gex-drop-msg"
if not defined MSG set MSG=gex: auto-pull from Drive inbox
"!GIT!" commit -m "!MSG!" >> "%LOGF%" 2>&1
if errorlevel 1 ( echo %DATE% %TIME% COMMIT FAILED >> "%LOGF%" & exit /b 1 )
"!GIT!" push >> "%LOGF%" 2>&1
if errorlevel 1 ( echo %DATE% %TIME% PUSH FAILED - committed locally only >> "%LOGF%" & exit /b 1 )
if exist "%REPO%\tools\.gex-drop-msg" del /q "%REPO%\tools\.gex-drop-msg"
echo %DATE% %TIME% PUSHED - !GOT! drop^(s^): !MSG! >> "%LOGF%"
exit /b 0
