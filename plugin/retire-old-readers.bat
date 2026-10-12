@echo off
rem retire-old-readers.bat - moves the 7 retired per-market readers (lsIRTReaderES ... lsIRTReaderEU, replaced on 2026-10-05
rem by lsFootprintReaderES ... lsFootprintReaderEU) out of Investor/RT's indicator folder so they stop showing in Add Indicator.
rem Nothing is deleted: they go to %USERPROFILE%\InvestorRT\retired-dll and can be moved back.
rem Run it with Investor/RT CLOSED (a loaded DLL cannot be moved).
setlocal
set "SRC=%USERPROFILE%\InvestorRT\dllx64"
set "DST=%USERPROFILE%\InvestorRT\retired-dll"
if not exist "%DST%" mkdir "%DST%"
set /a moved=0, failed=0
for %%M in (ES NQ CL GC HG NG EU) do (
  if exist "%SRC%\lsIRTReader%%M.dll" (
    move /Y "%SRC%\lsIRTReader%%M.dll" "%DST%\" >nul 2>&1 && (echo moved   lsIRTReader%%M.dll & set /a moved+=1) || (echo LOCKED  lsIRTReader%%M.dll - close Investor/RT and run this again & set /a failed+=1)
  ) else (
    echo gone    lsIRTReader%%M.dll
  )
)
echo.
echo %moved% moved, %failed% still in use. lsIRTReader.dll and lsFootprintReader*.dll are kept.
echo retire-old-readers %date% %time% moved %moved% failed %failed% > "%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\retire-old-readers.txt"
pause
