@echo off
setlocal
set REPO=C:\Dev\gex-signal-tapereader
if not exist "%REPO%\.git" (
  echo ERROR: repo not found at %REPO%
  pause & exit /b 1
)
copy /Y "%~dp0v10.js" "%REPO%\v10.js" >nul
copy /Y "%~dp0current\gex-signal-tapereader.user.js" "%REPO%\current\gex-signal-tapereader.user.js" >nul
copy /Y "%~dp0changelog\CHANGELOG.md" "%REPO%\changelog\CHANGELOG.md" >nul
if not exist "%REPO%\docs" mkdir "%REPO%\docs"
copy /Y "%~dp0docs\KING-FIELD-GUIDE.html" "%REPO%\docs\KING-FIELD-GUIDE.html" >nul
for %%f in ("%~dp0test_*.js") do copy /Y "%%f" "%REPO%\" >nul
echo.
echo Done. Files installed into %REPO%
echo Next: GitHub Desktop -^> Commit -^> Push origin, wait 2 min, update Tampermonkey.
pause
