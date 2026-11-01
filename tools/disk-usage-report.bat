@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem disk-usage-report.bat - measures how big each folder is (read only: nothing is moved or deleted) and writes
rem C:\Dev\gex-signal-tapereader\tools\disk-usage-report.txt for Claude to read. Takes a few minutes.
set "OUT=C:\Dev\gex-signal-tapereader\tools\disk-usage-report.txt"
echo DISK USAGE REPORT %date% %time%> "%OUT%"
for /f "tokens=3" %%F in ('dir C:\ /-c ^| find "bytes free"') do echo FREE_BYTES,%%F>> "%OUT%"
echo Measuring folders - please wait...
call :size "%USERPROFILE%"
for /d %%D in ("%USERPROFILE%\*") do call :size "%%D"
for /d %%D in ("%USERPROFILE%\AppData\Local\*") do call :size "%%D"
for /d %%D in ("%USERPROFILE%\AppData\Roaming\*") do call :size "%%D"
for /d %%D in ("%USERPROFILE%\Videos\*" "%USERPROFILE%\Documents\*" "%USERPROFILE%\Downloads\*" "%USERPROFILE%\Desktop\*" "%USERPROFILE%\Pictures\*" "%USERPROFILE%\Music\*") do call :size "%%D"
for /d %%D in ("C:\Dev\*") do call :size "%%D"
for /d %%D in ("C:\Program Files\*" "C:\Program Files (x86)\*" "C:\ProgramData\*") do call :size "%%D"
call :size "C:\Windows\Temp"
call :size "C:\$Recycle.Bin"
echo ---- video files over 100 MB>> "%OUT%"
for /r "%USERPROFILE%" %%F in (*.mp4 *.mov *.mkv *.avi *.wmv *.m4v *.webm) do if %%~zF GTR 104857600 echo VIDEO,%%~zF,%%~tF,%%F>> "%OUT%"
echo DONE>> "%OUT%"
echo Done. Tell Claude the report is ready.
pause
exit /b 0
:size
set "B="
for /f "tokens=1-3" %%a in ('robocopy "%~1" "%TEMP%\__none__" /L /S /NJH /BYTES /NC /NDL /NFL /XJ /R:0 /W:0 2^>nul ^| find "Bytes :"') do set "B=%%c"
if defined B echo DIR,!B!,%~1>> "%OUT%"
exit /b 0
