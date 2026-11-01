@echo off
rem lra-git-cleanup-2026-11-01.bat - packs the LRA git history (git gc) and moves C:\Dev\lra-app-backup to OneDrive.
rem Nothing is deleted: the stale git lock file and the backup are MOVED.
setlocal
set "R=C:\Dev\level-reversal-analytics"
set "TD=C:\Dev\_to_delete"
set "LOG=%~dp0lra-git-cleanup.log"
set "GIT=git"
if exist "C:\Program Files\Git\cmd\git.exe" set "GIT=C:\Program Files\Git\cmd\git.exe"
echo ==== %date% %time% > "%LOG%"
tasklist /fi "imagename eq git.exe" | find /i "git.exe" >nul && (echo Git is running right now - wait a minute and run this again.& echo git running, stopped>> "%LOG%" & pause & goto :eof)
if not exist "%TD%" mkdir "%TD%"
echo 1/3  Removing the stale git lock (left over since Oct 26) ...
if exist "%R%\.git\index.lock" (move /y "%R%\.git\index.lock" "%TD%\lra-index.lock" >nul && echo moved stale .git\index.lock>> "%LOG%")
echo 2/3  Packing the git history - can take 5-15 minutes, leave it running ...
for /f "tokens=3" %%B in ('robocopy "%R%\.git" "%TEMP%\_nul_" /L /S /BYTES /NJH /NFL /NDL /NC /XJ /R:0 /W:0 ^| findstr /c:"Bytes :"') do echo git before %%B bytes>> "%LOG%"
"%GIT%" -C "%R%" gc --prune=now >> "%LOG%" 2>&1
for /f "tokens=3" %%B in ('robocopy "%R%\.git" "%TEMP%\_nul_" /L /S /BYTES /NJH /NFL /NDL /NC /XJ /R:0 /W:0 ^| findstr /c:"Bytes :"') do echo git after %%B bytes>> "%LOG%"
"%GIT%" -C "%R%" status --short | find /c /v "" >> "%LOG%"
echo 3/3  Moving C:\Dev\lra-app-backup to OneDrive ...
if not exist "%USERPROFILE%\OneDrive\LRA-Data" mkdir "%USERPROFILE%\OneDrive\LRA-Data"
if exist "C:\Dev\lra-app-backup" (move /y "C:\Dev\lra-app-backup" "%USERPROFILE%\OneDrive\LRA-Data\lra-app-backup" >nul && echo moved lra-app-backup to OneDrive>> "%LOG%")
attrib +U -P "%USERPROFILE%\OneDrive\LRA-Data\lra-app-backup\*" /S /D >nul 2>&1
echo DONE>> "%LOG%"
echo.
echo Finished - tell Claude "done".
pause
