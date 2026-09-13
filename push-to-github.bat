@echo off
REM ===========================================================================
REM  push-to-github.bat  —  DOUBLE-CLICK to commit + push this repo to GitHub.
REM  Finds git (it isn't on PATH here), shows what changed, waits for your OK,
REM  then commits and pushes. No PowerShell (Avast-safe).
REM ===========================================================================
title Push gex-signal-tapereader to GitHub
setlocal
set "REPO=C:\Dev\gex-signal-tapereader"

REM --- find git.exe (GIT-FINDER) ---
set "GIT="
for %%G in (
  "%ProgramFiles%\Git\cmd\git.exe"
  "%ProgramFiles%\Git\bin\git.exe"
  "%ProgramFiles(x86)%\Git\cmd\git.exe"
  "%ProgramFiles(x86)%\Git\bin\git.exe"
  "%LocalAppData%\Programs\Git\cmd\git.exe"
  "%LocalAppData%\Programs\Git\bin\git.exe"
) do if exist "%%~G" if not defined GIT set "GIT=%%~G"
if not defined GIT for /f "delims=" %%G in ('where git 2^>nul') do if not defined GIT set "GIT=%%G"
REM GitHub Desktop bundles git (this is the one that works on this machine):
if not defined GIT for /d %%D in ("%LocalAppData%\GitHubDesktop\app-*") do if exist "%%D\resources\app\git\cmd\git.exe" set "GIT=%%D\resources\app\git\cmd\git.exe"
if not defined GIT (
  echo [ERROR] Could not find git.exe.
  echo         Install "Git for Windows", or use GitHub Desktop / VS Code instead.
  echo.
  pause
  exit /b 1
)
echo Using git: %GIT%
echo.

cd /d "%REPO%" || ( echo [ERROR] Repo not found at %REPO% & pause & exit /b 1 )

echo === Branch ===
"%GIT%" rev-parse --abbrev-ref HEAD
echo.
echo === Staging all changes... ===
"%GIT%" add -A
echo.
echo === These changes will be committed: ===
"%GIT%" status --short
echo.
echo ---------------------------------------------------------------------------
echo Review the list above. Press any key to COMMIT + PUSH, or close to cancel.
pause >nul

"%GIT%" commit -m "IRT gamma-profile RTX plugin (lsGammaProfile) + docs - 2026-09-13"
echo.
echo === Pushing to GitHub... ===
"%GIT%" push
if errorlevel 1 (
  echo.
  echo [PUSH FAILED] See the error above. Most common fixes:
  echo   - Remote is ahead: run   "%GIT%" pull --rebase   then double-click this again.
  echo   - Sign in if a GitHub login window appears, then re-run.
) else (
  echo.
  echo [DONE] Pushed to GitHub.
)
echo.
pause
