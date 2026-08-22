@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM   GEX AUTO-PULL SETUP  --  RUN ONCE
REM
REM   After this, Claude drops a tarball into your Google Drive
REM   folder "GEX-inbox" and your machine picks it up within two
REM   minutes: extract -> commit -> push. No more installers.
REM
REM   Installs:  C:\Dev\gex-signal-tapereader\tools\gex-pull.bat
REM   Schedules: task "GEX auto-pull", every 2 minutes, your login
REM   NO POWERSHELL -- certutil only (Avast flags PS: IDP.HELU.PSE88)
REM ============================================================
echo.
echo   GEX AUTO-PULL SETUP
echo   ===================
echo.

set REPO=C:\Dev\gex-signal-tapereader
if not exist "%REPO%\.git" (
  echo   [X] Repo not found at %REPO%
  echo       Run the normal GEX installer first, then re-run this.
  echo.
  pause
  exit /b 1
)
echo   [1/4] Repo found: %REPO%

REM ---- find Google Drive, create the inbox --------------------
set DRV=
for %%P in ("%USERPROFILE%\My Drive" "%USERPROFILE%\Google Drive" "G:\My Drive" "H:\My Drive") do if not defined DRV if exist "%%~P" set DRV=%%~P
if not defined DRV for %%D in (D E F G H I J K L M N O P Q R S T U V W X Y Z) do if not defined DRV if exist "%%D:\My Drive" set DRV=%%D:\My Drive
if not defined DRV (
  echo   [X] Google Drive for desktop not found.
  echo       Install/start Google Drive for desktop, then re-run this.
  echo.
  pause
  exit /b 1
)
echo   [2/4] Google Drive found: %DRV%
if not exist "%DRV%\GEX-inbox" mkdir "%DRV%\GEX-inbox"
if not exist "%DRV%\GEX-inbox\_done" mkdir "%DRV%\GEX-inbox\_done"

REM ---- write the puller ---------------------------------------
if not exist "%REPO%\tools" mkdir "%REPO%\tools"
set SELF=%~f0
more +83 "%SELF%" > "%TEMP%\gex-pull.b64"
certutil -f -decode "%TEMP%\gex-pull.b64" "%REPO%\tools\gex-pull.bat" >nul 2>&1
if errorlevel 1 (
  echo   [X] Could not decode the payload.
  echo.
  pause
  exit /b 1
)
del /q "%TEMP%\gex-pull.b64" >nul 2>&1
echo   [3/4] Installed tools\gex-pull.bat

REM ---- schedule it --------------------------------------------
schtasks /Create /TN "GEX auto-pull" /TR "\"%REPO%\tools\gex-pull.bat\"" /SC MINUTE /MO 2 /F >nul 2>&1
if errorlevel 1 (
  echo   [!] Could not create the scheduled task automatically.
  echo       Files are installed. To schedule it by hand, open Task
  echo       Scheduler and point a 2-minute repeating task at:
  echo       %REPO%\tools\gex-pull.bat
  echo.
  pause
  exit /b 1
)
echo   [4/4] Scheduled task "GEX auto-pull" created ^(every 2 minutes^)
echo.
echo   ============================================================
echo   DONE. Your Drive inbox is:
echo       %DRV%\GEX-inbox
echo.
echo   From now on: Claude drops a file there, your machine
echo   extracts, commits and pushes within two minutes.
echo   Watch it work:  %REPO%\tools\gex-pull.log
echo.
echo   To stop it later:  schtasks /Delete /TN "GEX auto-pull" /F
echo   ============================================================
echo.
pause
exit /b 0
QGVjaG8gb2ZmDQpzZXRsb2NhbCBFbmFibGVEZWxheWVkRXhwYW5zaW9uDQpSRU0gPT09PT09PT09
PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09DQpSRU0g
IEdFWCBBVVRPLVBVTEwgIC0tIGluc3RhbGxlZCBieSBzZXR1cC1nZXgtYXV0b3B1bGwuYmF0DQpS
RU0gIFdhdGNoZXMgdGhlIEdvb2dsZSBEcml2ZSBmb2xkZXIgIkdFWC1pbmJveCIgZm9yIGdleC1k
cm9wLSoudGFyLmd6LA0KUkVNICBleHRyYWN0cyBlYWNoIGludG8gdGhlIHJlcG8gKHRhciBwcmVz
ZXJ2ZXMgcmVsYXRpdmUgcGF0aHMpLA0KUkVNICB0aGVuIGNvbW1pdHMgYW5kIHB1c2hlcy4gUnVu
cyBvbiBhIHNjaGVkdWxlOyBuZXZlciBpbnRlcmFjdGl2ZS4NClJFTSAgTk8gUE9XRVJTSEVMTCBB
TllXSEVSRSAtLSBBdmFzdCBmbGFncyBpdCAoSURQLkhFTFUuUFNFODgpLg0KUkVNID09PT09PT09
PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PT09PQ0Kc2V0
IFJFUE89QzpcRGV2XGdleC1zaWduYWwtdGFwZXJlYWRlcg0Kc2V0IExPR0Y9JVJFUE8lXHRvb2xz
XGdleC1wdWxsLmxvZw0KDQpzZXQgSU5CT1g9DQpmb3IgJSVQIGluICgiJVVTRVJQUk9GSUxFJVxN
eSBEcml2ZVxHRVgtaW5ib3giICIlVVNFUlBST0ZJTEUlXEdvb2dsZSBEcml2ZVxHRVgtaW5ib3gi
ICJHOlxNeSBEcml2ZVxHRVgtaW5ib3giICJIOlxNeSBEcml2ZVxHRVgtaW5ib3giKSBkbyBpZiBu
b3QgZGVmaW5lZCBJTkJPWCBpZiBleGlzdCAiJSV+UCIgc2V0IElOQk9YPSUlflANCmlmIG5vdCBk
ZWZpbmVkIElOQk9YIGZvciAlJUQgaW4gKEQgRSBGIEcgSCBJIEogSyBMIE0gTiBPIFAgUSBSIFMg
VCBVIFYgVyBYIFkgWikgZG8gaWYgbm90IGRlZmluZWQgSU5CT1ggaWYgZXhpc3QgIiUlRDpcTXkg
RHJpdmVcR0VYLWluYm94IiBzZXQgSU5CT1g9JSVEOlxNeSBEcml2ZVxHRVgtaW5ib3gNCmlmIG5v
dCBkZWZpbmVkIElOQk9YICgNCiAgZWNobyAlREFURSUgJVRJTUUlIGluYm94IG5vdCBmb3VuZCAt
IGlzIEdvb2dsZSBEcml2ZSBmb3IgZGVza3RvcCBydW5uaW5nIHdpdGggIkdFWC1pbmJveCIgaW4g
TXkgRHJpdmU/ID4+ICIlTE9HRiUiDQogIGV4aXQgL2IgMQ0KKQ0KaWYgbm90IGV4aXN0ICIlUkVQ
TyVcLmdpdCIgKA0KICBlY2hvICVEQVRFJSAlVElNRSUgcmVwbyBub3QgZm91bmQgYXQgJVJFUE8l
ID4+ICIlTE9HRiUiDQogIGV4aXQgL2IgMQ0KKQ0KaWYgbm90IGV4aXN0ICIlSU5CT1glXF9kb25l
IiBta2RpciAiJUlOQk9YJVxfZG9uZSINCg0Kc2V0IEdPVD0wDQpmb3IgJSVGIGluICgiJUlOQk9Y
JVxnZXgtZHJvcC0qLnRhci5neiIpIGRvICgNCiAgZWNobyAlREFURSUgJVRJTUUlIGV4dHJhY3Rp
bmcgJSV+bnhGID4+ICIlTE9HRiUiDQogIHRhciAteHpmICIlJUYiIC1DICIlUkVQTyUiID4+ICIl
TE9HRiUiIDI+JjENCiAgaWYgZXJyb3JsZXZlbCAxICgNCiAgICBlY2hvICVEQVRFJSAlVElNRSUg
RVhUUkFDVCBGQUlMRUQgZm9yICUlfm54RiAtIGxlZnQgaW4gcGxhY2UsIE5PVCBjb21taXR0ZWQg
Pj4gIiVMT0dGJSINCiAgKSBlbHNlICgNCiAgICBzZXQgL2EgR09UKz0xDQogICAgbW92ZSAvWSAi
JSVGIiAiJUlOQk9YJVxfZG9uZVwiID5udWwNCiAgKQ0KKQ0KaWYgIUdPVCEgRVFVIDAgZXhpdCAv
YiAwDQoNCnNldCBHSVQ9DQp3aGVyZSBnaXQgPm51bCAyPiYxICYmIHNldCBHSVQ9Z2l0DQppZiBu
b3QgZGVmaW5lZCBHSVQgaWYgZXhpc3QgIkM6XFByb2dyYW0gRmlsZXNcR2l0XGNtZFxnaXQuZXhl
IiBzZXQgR0lUPUM6XFByb2dyYW0gRmlsZXNcR2l0XGNtZFxnaXQuZXhlDQppZiBub3QgZGVmaW5l
ZCBHSVQgaWYgZXhpc3QgIkM6XFByb2dyYW0gRmlsZXMgKHg4NilcR2l0XGNtZFxnaXQuZXhlIiBz
ZXQgR0lUPUM6XFByb2dyYW0gRmlsZXMgKHg4NilcR2l0XGNtZFxnaXQuZXhlDQppZiBub3QgZGVm
aW5lZCBHSVQgaWYgZXhpc3QgIiVMT0NBTEFQUERBVEElXFByb2dyYW1zXEdpdFxjbWRcZ2l0LmV4
ZSIgc2V0IEdJVD0lTE9DQUxBUFBEQVRBJVxQcm9ncmFtc1xHaXRcY21kXGdpdC5leGUNCmlmIG5v
dCBkZWZpbmVkIEdJVCAoDQogIGZvciAvZCAlJUQgaW4gKCIlTE9DQUxBUFBEQVRBJVxHaXRIdWJE
ZXNrdG9wXGFwcC0qIikgZG8gaWYgZXhpc3QgIiUlRFxyZXNvdXJjZXNcYXBwXGdpdFxjbWRcZ2l0
LmV4ZSIgc2V0IEdJVD0lJURccmVzb3VyY2VzXGFwcFxnaXRcY21kXGdpdC5leGUNCikNCmlmIG5v
dCBkZWZpbmVkIEdJVCAoIGVjaG8gJURBVEUlICVUSU1FJSBHSVQgTk9UIEZPVU5EIC0gZmlsZXMg
ZXh0cmFjdGVkIGJ1dCBOT1QgcHVzaGVkID4+ICIlTE9HRiUiICYgZXhpdCAvYiAxICkNCg0KY2Qg
L2QgIiVSRVBPJSINCiIhR0lUISIgYWRkIC1BDQoiIUdJVCEiIGRpZmYgLS1jYWNoZWQgLS1xdWll
dCAmJiAoIGVjaG8gJURBVEUlICVUSU1FJSBleHRyYWN0ZWQgIUdPVCEgZHJvcF4oc14pIGJ1dCBu
b3RoaW5nIGNoYW5nZWQgPj4gIiVMT0dGJSIgJiBleGl0IC9iIDAgKQ0Kc2V0IE1TRz0NCmlmIGV4
aXN0ICIlUkVQTyVcdG9vbHNcLmdleC1kcm9wLW1zZyIgc2V0IC9wIE1TRz08IiVSRVBPJVx0b29s
c1wuZ2V4LWRyb3AtbXNnIg0KaWYgbm90IGRlZmluZWQgTVNHIHNldCBNU0c9Z2V4OiBhdXRvLXB1
bGwgZnJvbSBEcml2ZSBpbmJveA0KIiFHSVQhIiBjb21taXQgLW0gIiFNU0chIiA+PiAiJUxPR0Yl
IiAyPiYxDQppZiBlcnJvcmxldmVsIDEgKCBlY2hvICVEQVRFJSAlVElNRSUgQ09NTUlUIEZBSUxF
RCA+PiAiJUxPR0YlIiAmIGV4aXQgL2IgMSApDQoiIUdJVCEiIHB1c2ggPj4gIiVMT0dGJSIgMj4m
MQ0KaWYgZXJyb3JsZXZlbCAxICggZWNobyAlREFURSUgJVRJTUUlIFBVU0ggRkFJTEVEIC0gY29t
bWl0dGVkIGxvY2FsbHkgb25seSA+PiAiJUxPR0YlIiAmIGV4aXQgL2IgMSApDQppZiBleGlzdCAi
JVJFUE8lXHRvb2xzXC5nZXgtZHJvcC1tc2ciIGRlbCAvcSAiJVJFUE8lXHRvb2xzXC5nZXgtZHJv
cC1tc2ciDQplY2hvICVEQVRFJSAlVElNRSUgUFVTSEVEIC0gIUdPVCEgZHJvcF4oc14pOiAhTVNH
ISA+PiAiJUxPR0YlIg0KZXhpdCAvYiAwDQo=
