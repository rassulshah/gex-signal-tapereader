@echo off
setlocal EnableExtensions
rem retire-unused-2026-11-18.bat - Rassul 2026-11-18 09:05 CT "also tonight get rid of anything we dont use like old plugins etc".
rem Moves 10 unused indicator DLLs OUT of Investor/RT so they leave the Add Indicator list. NOTHING IS DELETED.
rem   - the 7 from 2026-11-17 (retire-unused-2026-11-17.bat was never run): lsGammaProfile, lsCandleAbsorb, lsZzoVolume, lsBigPrint,
rem     lsLiquidityProfile, lsBidAskStats, lsBigBidAsk - nothing written for 7 to 50 days, build lines already retired
rem   - lsRevStory, lsRevStoryMarks - replaced by the Reversal Read (retired 2026-11-11, never moved)
rem   - lsDefenseLines - "get rid of the defense lines" 2026-11-01, nothing written since
rem Undo = move them back from %USERPROFILE%\InvestorRT\_retired-2026-11-18 into dllx64 (with Investor/RT closed).
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"
set "RET=%USERPROFILE%\InvestorRT\_retired-2026-11-18"
set "LOG=C:\Dev\gex-signal-tapereader\tools\retire-plugins.log"
tasklist /FI "IMAGENAME eq investorRT.exe" 2>nul | find /I "investorRT.exe" >nul
if not errorlevel 1 ( echo Investor/RT is still open. Close it ^(File - Exit^), wait until it is gone from Task Manager, then run this file again. & pause & exit /b 1 )
if not exist "%RET%" mkdir "%RET%"
echo ==== %date% %time% retire unused 2026-11-18 >> "%LOG%"
for %%F in (lsGammaProfile.dll lsCandleAbsorb.dll lsZzoVolume.dll lsBigPrint.dll lsLiquidityProfile.dll lsBidAskStats.dll lsBigBidAsk.dll lsRevStory.dll lsRevStoryMarks.dll lsDefenseLines.dll) do (
  if exist "%DLLDIR%\%%F" ( move /Y "%DLLDIR%\%%F" "%RET%\" >nul && echo moved %%F>> "%LOG%" && echo Done: %%F moved to %RET% ) else ( echo %%F is not in %DLLDIR% - nothing to move )
)
echo.
echo Finished. 10 unused indicators moved out (nothing deleted). Open Investor/RT again.
pause
