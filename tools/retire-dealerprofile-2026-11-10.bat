@echo off
setlocal EnableExtensions
rem retire-dealerprofile-2026-11-10.bat - Rassul 2026-11-10 21:41 CT "retire that and only call it the limit profile".
rem Moves the old-name copy lsDealerProfile.dll OUT of Investor/RT. Nothing is deleted: it goes to %USERPROFILE%\InvestorRT\_retired-2026-11-10.
rem Undo = move it back into %USERPROFILE%\InvestorRT\dllx64 (Investor/RT closed).
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"
set "RET=%USERPROFILE%\InvestorRT\_retired-2026-11-10"
set "LOG=C:\Dev\gex-signal-tapereader\tools\retire-plugins.log"
tasklist /FI "IMAGENAME eq investorRT.exe" 2>nul | find /I "investorRT.exe" >nul
if not errorlevel 1 ( echo Investor/RT is still open. Close it, then run this file again. & pause & exit /b 1 )
if not exist "%RET%" mkdir "%RET%"
echo ==== %date% %time% retire lsDealerProfile >> "%LOG%"
if exist "%DLLDIR%\lsDealerProfile.dll" ( move /Y "%DLLDIR%\lsDealerProfile.dll" "%RET%\" >nul && echo moved lsDealerProfile.dll>> "%LOG%" && echo Done: lsDealerProfile.dll moved to %RET% ) else ( echo lsDealerProfile.dll is not in %DLLDIR% - nothing to do. )
if not exist "%DLLDIR%\lsLimitProfile.dll" echo WARNING: lsLimitProfile.dll is not installed yet - wait 2 minutes and reopen Investor/RT.
pause
