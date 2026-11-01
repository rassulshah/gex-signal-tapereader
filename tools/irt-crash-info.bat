@echo off
rem irt-crash-info.bat - READ ONLY: copies the last Windows "Application Error" / "Windows Error Reporting" events (the crash record)
rem into tools\irt-crash.txt so Claude can see which DLL crashed IRT. Nothing is changed.
set "OUT=%~dp0irt-crash.txt"
echo ==== %date% %time% > "%OUT%"
wevtutil qe Application /q:"*[System[(EventID=1000 or EventID=1001 or EventID=1026)]]" /c:6 /rd:true /f:text >> "%OUT%" 2>&1
echo.
echo Done - tell Claude "done".
pause
