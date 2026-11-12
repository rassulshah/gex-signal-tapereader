@echo off
setlocal EnableExtensions
rem retire-revstory-2026-11-11.bat - Rassul 2026-11-11 22:18 CT "lets go with reversal read": the indicator built tonight as lsRevStory / lsRevStoryMarks is now
rem lsRevRead / lsRevReadMarks. This moves the old-name copies OUT of Investor/RT and the old source files out of the plugin folder. Nothing is deleted:
rem the DLLs go to %USERPROFILE%\InvestorRT\_retired-2026-11-11, the sources to C:\Dev\gex-signal-tapereader\plugin\_to_delete\revstory-2026-11-11.
rem Undo = move them back (Investor/RT closed).
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"
set "RET=%USERPROFILE%\InvestorRT\_retired-2026-11-11"
set "PLUG=C:\Dev\gex-signal-tapereader\plugin"
set "OLD=%PLUG%\_to_delete\revstory-2026-11-11"
set "LOG=C:\Dev\gex-signal-tapereader\tools\retire-plugins.log"
tasklist /FI "IMAGENAME eq investorRT.exe" 2>nul | find /I "investorRT.exe" >nul
if not errorlevel 1 ( echo Investor/RT is still open. Close it, then run this file again. & pause & exit /b 1 )
if not exist "%RET%" mkdir "%RET%"
if not exist "%OLD%" mkdir "%OLD%"
echo ==== %date% %time% retire lsRevStory / lsRevStoryMarks >> "%LOG%"
for %%F in (lsRevStory.dll lsRevStoryMarks.dll) do ( if exist "%DLLDIR%\%%F" ( move /Y "%DLLDIR%\%%F" "%RET%\" >nul && echo moved %%F>> "%LOG%" && echo Done: %%F moved to %RET% ) else ( echo %%F is not in %DLLDIR% - nothing to move ) )
for %%F in (RevStory.cpp RevStoryMarks.cpp RevStoryLogic.h) do ( if exist "%PLUG%\%%F" ( move /Y "%PLUG%\%%F" "%OLD%\" >nul && echo moved %%F>> "%LOG%" ) )
if exist "%PLUG%\tests_revstory" ( move /Y "%PLUG%\tests_revstory" "%OLD%\" >nul && echo moved tests_revstory>> "%LOG%" )
set "LRA=C:\Dev\level-reversal-analytics"
if not exist "%LRA%\_to_delete\revstory-2026-11-11" mkdir "%LRA%\_to_delete\revstory-2026-11-11"
if exist "%LRA%\analytics\lra\rev_story.py" ( move /Y "%LRA%\analytics\lra\rev_story.py" "%LRA%\_to_delete\revstory-2026-11-11\" >nul && echo moved rev_story.py>> "%LOG%" )
if exist "%LRA%\analytics\test_rev_story.py" ( move /Y "%LRA%\analytics\test_rev_story.py" "%LRA%\_to_delete\revstory-2026-11-11\" >nul && echo moved test_rev_story.py>> "%LOG%" )
if not exist "%DLLDIR%\lsRevRead.dll" echo WARNING: lsRevRead.dll is not installed yet - wait 2 minutes and reopen Investor/RT.
pause
