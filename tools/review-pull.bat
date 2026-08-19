@echo off
setlocal EnableDelayedExpansion
REM GEX review pull — moves nightly/weekly review files from the Drive inbox into the repo and pushes.
REM Inbox: Google Drive folder "GEX-review-inbox" (synced by Google Drive for desktop).
set REPO=C:\Dev\gex-signal-tapereader
set LOGF=%REPO%\tools\review-pull.log
set INBOX=
for %%P in ("%USERPROFILE%\My Drive\GEX-review-inbox" "%USERPROFILE%\Google Drive\GEX-review-inbox" "G:\My Drive\GEX-review-inbox" "H:\My Drive\GEX-review-inbox") do if not defined INBOX if exist "%%~P" set INBOX=%%~P
if not defined INBOX for %%D in (D E F G H I J K L M N O P Q R S T U V W X Y Z) do if not defined INBOX if exist "%%D:\My Drive\GEX-review-inbox" set INBOX=%%D:\My Drive\GEX-review-inbox
if not defined INBOX (
  echo %DATE% %TIME% inbox not found - is Google Drive for desktop running and "GEX-review-inbox" in My Drive? >> "%LOGF%"
  exit /b 1
)
if not exist "%INBOX%\_done" mkdir "%INBOX%\_done"
if not exist "%REPO%\learning\log" mkdir "%REPO%\learning\log"
if not exist "%REPO%\review" mkdir "%REPO%\review"
set MOVED=0
for %%F in ("%INBOX%\*.json") do (
  set NAME=%%~nxF
  set DEST=
  if /I "!NAME!"=="rules.json" set DEST=%REPO%\learning\rules.json
  if not defined DEST if /I "!NAME:~0,7!"=="review_" set DEST=%REPO%\review\!NAME:~7!
  if not defined DEST set DEST=%REPO%\learning\log\!NAME!
  if /I "!NAME!"=="rules.json" (
    copy /Y "%%F" "!DEST!" >nul && set /a MOVED+=1
    echo %DATE% %TIME% rules.json updated >> "%LOGF%"
  ) else if exist "!DEST!" (
    echo %DATE% %TIME% skipped !NAME! - already in repo ^(logs are append-only^) >> "%LOGF%"
  ) else (
    copy /Y "%%F" "!DEST!" >nul && set /a MOVED+=1
    echo %DATE% %TIME% placed !NAME! -^> !DEST! >> "%LOGF%"
  )
  move /Y "%%F" "%INBOX%\_done\" >nul
)
if !MOVED! EQU 0 exit /b 0
set GIT=
where git >nul 2>&1 && set GIT=git
if not defined GIT if exist "C:\Program Files\Git\cmd\git.exe" set GIT=C:\Program Files\Git\cmd\git.exe
if not defined GIT if exist "C:\Program Files (x86)\Git\cmd\git.exe" set GIT=C:\Program Files (x86)\Git\cmd\git.exe
if not defined GIT if exist "%LOCALAPPDATA%\Programs\Git\cmd\git.exe" set GIT=%LOCALAPPDATA%\Programs\Git\cmd\git.exe
if not defined GIT (
  for /d %%D in ("%LOCALAPPDATA%\GitHubDesktop\app-*") do if exist "%%D\resources\app\git\cmd\git.exe" set GIT=%%D\resources\app\git\cmd\git.exe
)
if not defined GIT ( echo %DATE% %TIME% git not found >> "%LOGF%" & exit /b 1 )
cd /d "%REPO%"
"!GIT!" add learning review
"!GIT!" diff --cached --quiet && exit /b 0
for /f "tokens=1-3 delims=/ " %%a in ("%DATE%") do set D=%%c-%%a-%%b
"!GIT!" commit -m "review: nightly/weekly files pulled from Drive inbox %D%" >> "%LOGF%" 2>&1
"!GIT!" push >> "%LOGF%" 2>&1
echo %DATE% %TIME% pushed !MOVED! file(s) >> "%LOGF%"
exit /b 0
