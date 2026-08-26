@echo off
setlocal EnableDelayedExpansion
REM pushdata.bat - move day-file JSONs from Downloads into the repo data folder and push them,
REM so the nightly LLM review can read them. Safe to run any time; does nothing if nothing to do.
set REPO=C:\Dev\gex-signal-tapereader
set DATA=%REPO%\data
set DL=%USERPROFILE%\Downloads
if not exist "%REPO%\.git" ( echo ERROR: repo not found at %REPO% & pause & exit /b 1 )
if not exist "%DATA%" mkdir "%DATA%"

echo.
echo === 1. day files sitting in Downloads ===
set MOVED=0
for %%F in ("%DL%\2026-??-??.json") do (
  echo   moving %%~nxF
  move /Y "%%F" "%DATA%\%%~nxF" >nul
  set /a MOVED+=1
)
if !MOVED!==0 ( echo   none found in Downloads - fine if the panel wrote straight to the data folder. )

echo.
echo === 2. day files now in the repo ===
dir /b "%DATA%\2026-*.json" 2>nul

echo.
echo === 3. commit and push ===
set GIT=
where git >nul 2>&1 && set GIT=git
if not defined GIT if exist "C:\Program Files\Git\cmd\git.exe" set GIT=C:\Program Files\Git\cmd\git.exe
if not defined GIT if exist "C:\Program Files (x86)\Git\cmd\git.exe" set GIT=C:\Program Files (x86)\Git\cmd\git.exe
if not defined GIT if exist "%LOCALAPPDATA%\Programs\Git\cmd\git.exe" set GIT=%LOCALAPPDATA%\Programs\Git\cmd\git.exe
if not defined GIT (
  for /d %%D in ("%LOCALAPPDATA%\GitHubDesktop\app-*") do if exist "%%D\resources\app\git\cmd\git.exe" set GIT=%%D\resources\app\git\cmd\git.exe
)
if not defined GIT ( echo ERROR: git not found - push via GitHub Desktop instead. & pause & exit /b 1 )
pushd "%REPO%"
"!GIT!" add data
"!GIT!" diff --cached --quiet
if errorlevel 1 (
  "!GIT!" commit -m "data: day files for the nightly LLM review"
  "!GIT!" push
  if errorlevel 1 ( echo PUSH FAILED - check the message above. & popd & pause & exit /b 1 )
  echo PUSHED - the review can now read them.
) else (
  echo Nothing new under data\ - already committed. Pushing anything pending...
  "!GIT!" push
)
popd
echo.
pause
