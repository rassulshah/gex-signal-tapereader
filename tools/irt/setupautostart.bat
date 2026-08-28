@echo off
REM ============================================================================
REM  ONE-TIME SETUP -- makes the IRT server start automatically at every login.
REM  Double-click this ONCE. After that you never touch it again.
REM
REM  It copies irtstartup.bat into your Windows Startup folder. Nothing else
REM  is installed, nothing runs in the background except the server itself,
REM  and you can undo it at any time (see the end of this file).
REM
REM  NO POWERSHELL -- Avast flags it (IDP.HELU.PSE88).
REM ============================================================================
setlocal
cd /d "%~dp0"

set "STARTUP=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"

echo.
echo   IRT FlexLevels -- autostart setup
echo.

if not exist "irtstartup.bat" (
  echo   ERROR: irtstartup.bat must be in this same folder.
  echo.
  pause
  exit /b 1
)
if not exist "irtserve.py" (
  echo   ERROR: irtserve.py must be in this same folder.
  echo.
  pause
  exit /b 1
)

copy /Y "irtstartup.bat" "%STARTUP%\irtstartup.bat" >nul
if errorlevel 1 (
  echo   ERROR: could not write to the Startup folder:
  echo   %STARTUP%
  echo.
  pause
  exit /b 1
)

echo   DONE. The server will now start automatically every time you log in.
echo.
echo   installed to: %STARTUP%\irtstartup.bat
echo   serving from: C:\Users\rassul\InvestorRT\rtx\lsFlexLevels
echo.
echo   Starting it now so you do not have to log out --
echo.
start "" "%STARTUP%\irtstartup.bat"
echo.
echo   TO UNDO LATER: press Win+R, type  shell:startup  and delete
echo   irtstartup.bat from the folder that opens.
echo.
pause
