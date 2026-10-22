@echo off
REM ============================================================
REM  GEX-GIT-TIDY.bat  (2026-10-22 git cleanup, docs in level-reversal-analytics\docs\GIT-CLEANUP-PLAN-2026-10-22.md)
REM  NO POWERSHELL. Stops git tracking logs/staged-scan-findings.log (the secrets scanner's local report, committed
REM  ~550 times on 10-21 / 10-22) and removes the empty junk file plugin\--------------------------------------- .
REM  The file logs\staged-scan-findings.log stays on this PC. The next "GEX sync" tick commits and pushes the change.
REM  Safe to run twice. History is NOT rewritten.
REM ============================================================
title GEX git tidy
setlocal
cd /d "%~dp0"
set "GIT=git"
where git >nul 2>&1 || set "GIT=C:\Program Files\Git\cmd\git.exe"
set "LOCK=%~dp0tools\gex-sync.lock"
set /a N=0
:wait
if not exist "%LOCK%" goto go
set /a N+=1
if %N% GEQ 180 goto stuck
echo A GEX sync is running - waiting for it to finish...
timeout /t 5 /nobreak >nul
goto wait
:go
REM take the sync's own lock so it skips its tick while this runs
echo %DATE% %TIME% GEX-GIT-TIDY> "%LOCK%"
"%GIT%" rm --cached --ignore-unmatch --quiet -- logs/staged-scan-findings.log
if exist "plugin\---------------------------------------" del "plugin\---------------------------------------"
del "%LOCK%" >nul 2>&1
echo.
echo  Done. The next GEX sync (within 2 minutes) commits and pushes this. Nothing else changed.
pause
exit /b 0
:stuck
echo.
echo  The sync lock tools\gex-sync.lock has been there for 15 minutes.
echo  If no sync is running, delete that file and run GEX-GIT-TIDY.bat again.
pause
exit /b 1
