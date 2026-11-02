@echo off
setlocal EnableExtensions
rem retire-defenselines-2026-11-01.bat - Rassul 2026-11-01 21:29 CT "get rid of the defense lines".
rem Moves lsDefenseLines.dll OUT of Investor/RT (nothing deleted). Undo = move it back to dllx64.
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"
set "RET=%USERPROFILE%\InvestorRT\_retired-2026-11-01"
set "LOG=C:\Dev\gex-signal-tapereader\tools\retire-plugins.log"
tasklist /FI "IMAGENAME eq investorRT.exe" 2>nul | find /I "investorRT.exe" >nul
if not errorlevel 1 ( echo Investor/RT is still open. Close it, then run this file again. & pause & exit /b 1 )
if not exist "%RET%" mkdir "%RET%"
echo ==== %date% %time% defense lines >> "%LOG%"
if exist "%DLLDIR%\lsDefenseLines.dll" ( move /Y "%DLLDIR%\lsDefenseLines.dll" "%RET%\" >nul && echo moved lsDefenseLines.dll>> "%LOG%" && echo Done: lsDefenseLines.dll moved to %RET% ) else ( echo lsDefenseLines.dll was not in dllx64 - nothing to move & echo not found lsDefenseLines.dll>> "%LOG%" )
pause
