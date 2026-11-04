@echo off
REM (2026-11-04 10:45 CT) clears a stuck gex-build.lock: the 10:30 build run stopped without removing it, so every 2-minute build is skipped.
REM After this the next run (within 2 minutes) builds lsBookFlow 1.34.0 and the rest as usual. Safe to run while Investor/RT is open.
del "C:\Dev\gex-signal-tapereader\tools\gex-build.lock" >nul 2>&1
if exist "C:\Dev\gex-signal-tapereader\tools\gex-build.lock" (echo The lock could not be removed - tell Claude.) else (echo Build lock cleared. The next build runs within 2 minutes.)
pause
