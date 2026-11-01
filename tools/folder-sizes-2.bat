@echo off
rem folder-sizes-2.bat - READ ONLY: sizes the big LRA / GEX sub-folders (nothing is moved or deleted)
setlocal
set "SZ=%~dp0folder-sizes-2.txt"
echo ==== %date% %time% > "%SZ%"
echo Measuring folders (read only), about 1-3 minutes ...
set "L=C:\Dev\level-reversal-analytics"
for %%R in ("%L%\data\mbo\store" "%L%\data\menthorq" "%L%\data\options" "%L%\data\raw-exports" "%L%\research-artifacts" "%L%\video" "%L%\data\irt" "C:\Dev\gex-signal-tapereader\plugin" "C:\Dev\gex-signal-tapereader\data") do (
  for /d %%D in ("%%~R\*") do call :size "%%~D"
)
for %%D in ("%L%\.git" "C:\Dev\gex-signal-tapereader\.git" "C:\Dev\lra-menthorq-data\.git" "C:\Dev\lra-app-backup" "C:\Dev\_backup" "C:\Dev\mysql-data") do call :size "%%~D"
echo DONE>> "%SZ%"
echo Finished - tell Claude "done".
pause
goto :eof
:size
for /f "tokens=3" %%B in ('robocopy "%~1" "%TEMP%\_nul_" /L /S /BYTES /NJH /NFL /NDL /NC /XJ /R:0 /W:0 ^| findstr /c:"Bytes :"') do echo %%B,%~1>> "%SZ%"
goto :eof
