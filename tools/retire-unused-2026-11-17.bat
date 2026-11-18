@echo off
setlocal EnableExtensions
rem retire-unused-2026-11-17.bat - Rassul 2026-11-17 21:09 CT "aren't you going to remove what i dont use ?". These 7 indicators have written nothing
rem for 6 to 50 days (off his charts): lsGammaProfile, lsCandleAbsorb, lsZzoVolume, lsBigPrint, lsLiquidityProfile, lsBidAskStats, lsBigBidAsk.
rem Their gex-build lines are already retired (no more builds) and their nightly scorers parked. This moves the DLLs OUT of Investor/RT so they leave
rem the Add Indicator list. Nothing is deleted: the DLLs go to %USERPROFILE%\InvestorRT\_retired-2026-11-17. Undo = move them back (Investor/RT closed).
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"
set "RET=%USERPROFILE%\InvestorRT\_retired-2026-11-17"
set "LOG=C:\Dev\gex-signal-tapereader\tools\retire-plugins.log"
tasklist /FI "IMAGENAME eq investorRT.exe" 2>nul | find /I "investorRT.exe" >nul
if not errorlevel 1 ( echo Investor/RT is still open. Close it, then run this file again. & pause & exit /b 1 )
if not exist "%RET%" mkdir "%RET%"
echo ==== %date% %time% retire unused 2026-11-17 >> "%LOG%"
for %%F in (lsGammaProfile.dll lsCandleAbsorb.dll lsZzoVolume.dll lsBigPrint.dll lsLiquidityProfile.dll lsBidAskStats.dll lsBigBidAsk.dll) do (
  if exist "%DLLDIR%\%%F" ( move /Y "%DLLDIR%\%%F" "%RET%\" >nul && echo moved %%F>> "%LOG%" && echo Done: %%F moved to %RET% ) else ( echo %%F is not in %DLLDIR% - nothing to move )
)
echo.
echo Finished. Open Investor/RT again.
pause
