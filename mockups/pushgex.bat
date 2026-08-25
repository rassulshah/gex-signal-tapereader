@echo off
setlocal EnableDelayedExpansion
REM Commit-and-push only - for when the files are already extracted but the push didn't happen.
set REPO=C:\Dev\gex-signal-tapereader
if not exist "%REPO%\.git" ( echo ERROR: repo not found at %REPO% & pause & exit /b 1 )
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
del /q "%REPO%\mockups\install*.bat" "%REPO%\mockups\apply*.bat" "%REPO%\mockups\gex*drop*.zip" >nul 2>&1
"!GIT!" add -A
"!GIT!" reset -q -- "mockups/*.zip" "mockups/apply*" >nul 2>&1
"!GIT!" diff --cached --quiet
if errorlevel 1 (
  "!GIT!" commit -m "v14.2: profile in the rail's frame, linear fill, %%King labels, today-expiry-only books, info icons"
  "!GIT!" push
  if errorlevel 1 ( echo PUSH FAILED - check the message above. & popd & pause & exit /b 1 )
  echo PUSHED.
) else (
  "!GIT!" push
  echo Nothing new to commit - pushed whatever was pending.
)
popd
pause
