@echo off
REM process-report.bat (2026-11-18, Rassul 08:40 CT "there are probably a lot of useless processes"): a READ-ONLY snapshot of what is running.
REM It changes nothing. Writes tools\process-report.txt for Claude to read.
set OUT=%~dp0process-report.txt
echo PROCESS REPORT %DATE% %TIME% > "%OUT%"
echo. >> "%OUT%"
echo === MEMORY / CPU PER PROCESS (tasklist /v) === >> "%OUT%"
tasklist /v /fo csv >> "%OUT%" 2>&1
echo. >> "%OUT%"
echo === CPU %% NOW PER PROCESS (wmic, may be missing on new Windows) === >> "%OUT%"
wmic path Win32_PerfFormattedData_PerfProc_Process get Name,IDProcess,PercentProcessorTime,WorkingSetPrivate /format:csv >> "%OUT%" 2>&1
echo. >> "%OUT%"
echo === STARTUP PROGRAMS === >> "%OUT%"
wmic startup get Caption,Command,Location /format:csv >> "%OUT%" 2>&1
echo. >> "%OUT%"
echo === RUNNING SERVICES === >> "%OUT%"
sc query type= service state= running >> "%OUT%" 2>&1
echo. >> "%OUT%"
echo === MEMORY TOTAL / FREE === >> "%OUT%"
systeminfo | findstr /C:"Total Physical Memory" /C:"Available Physical Memory" /C:"Virtual Memory" >> "%OUT%" 2>&1
echo. >> "%OUT%"
echo === SCHEDULED TASKS (ready / running) === >> "%OUT%"
schtasks /query /fo csv /v >> "%OUT%" 2>&1
echo Done - written to %OUT%
timeout /t 5 >nul
