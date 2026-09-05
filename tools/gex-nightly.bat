@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  GEX NIGHTLY  -- installed by setup-gex-nightly.bat  (v15.68)
REM  Every 10 minutes, hidden: when the newest day file (the one
REM  the panel's Save wrote) is NEWER than its nightly log, run
REM  tools\nightly\run.py through tools\nightly\tick.py - the log,
REM  the pattern table, the tape coverage, the refreshed sweep
REM  tables, the study registry (the Analysis tab). The "GEX sync"
REM  task pushes the results within two minutes; the panel fetches
REM  them. Nothing new = nothing happens, nothing logged.
REM  NO POWERSHELL ANYWHERE (Avast flags it: IDP.HELU.PSE88).
REM ============================================================
set REPO=C:\Dev\gex-signal-tapereader
set LOGF=%REPO%\tools\gex-nightly.log
set LOCK=%REPO%\tools\gex-nightly.lock

if not exist "%REPO%\tools\nightly\tick.py" exit /b 1
if exist "%LOCK%" (
  REM a previous run is still going - skip this tick
  exit /b 0
)
echo %DATE% %TIME% > "%LOCK%"

set PY=
where python >nul 2>&1 && set PY=python
if not defined PY where py >nul 2>&1 && set PY=py -3
if not defined PY (
  echo %DATE% %TIME% PYTHON NOT FOUND - install Python 3 and re-run setup-gex-nightly.bat >> "%LOGF%"
  del "%LOCK%" >nul 2>&1
  exit /b 1
)

cd /d "%REPO%"
REM (v15.72b) the log is UTF-8: the pattern report carries non-cp1252 characters (measured 2026-09-04 19:21 CT)
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
!PY! tools\nightly\tick.py >> "%LOGF%" 2>&1
set RC=!ERRORLEVEL!
if "!RC!"=="0" echo %DATE% %TIME% nightly ran - the GEX sync task pushes it within two minutes >> "%LOGF%"
if "!RC!"=="1" echo %DATE% %TIME% NIGHTLY FAILED - see the lines above >> "%LOGF%"
REM RC 3 = nothing to do: silent

del "%LOCK%" >nul 2>&1
exit /b 0
