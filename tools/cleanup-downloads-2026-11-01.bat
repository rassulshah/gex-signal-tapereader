@echo off
rem cleanup-downloads-2026-11-01.bat - MOVES old installers / duplicates out of Downloads into Downloads\_to_delete
rem (nothing is deleted). Then sizes the LRA and GEX folders (read-only) for Claude to review.
setlocal
set "DL=%USERPROFILE%\Downloads"
set "TD=%DL%\_to_delete"
set "LOG=%~dp0cleanup-downloads.log"
if not exist "%TD%" mkdir "%TD%"
echo ==== %date% %time% > "%LOG%"
echo 1/2  Moving old installers and duplicates to Downloads\_to_delete ...
for %%F in (
 "FuturesPulse.zip" "Anaconda3-2024.02-1-Windows-x86_64.exe" "Anaconda3-2024.10-1-Windows-x86_64.exe" "Anaconda3-2025.12-2-Windows-x86_64.exe"
 "pycharm-2025.3.3.exe" "GenClaw-win-x64-0.1.701 (1).zip" "GenClaw-win-x64-0.1.701.zip" "GenClaw-win-x64-0.1.723.zip" "Genmail-win-x64-1.0.898.exe"
 "GitHubDesktopSetup-x64.exe" "GoogleDriveSetup.exe" "TradeStation 9.5 Setup.exe" "TradeStation Setup.exe" "daytradrInstall5001f.exe"
 "xlstat.exe" "bookmap_install-7.3.0_b45.exe" "OBS-Studio-29.1.3-Full-Installer-x64.exe" "RStudio-2021.09.0+351.exe"
 "MovaviVideoSuiteSetupC_W8723zv_.exe" "basic-miktex-24.1-x64.exe" "install-tl-windows.exe" "WhatsAppSetup.exe" "LockDownBrowser-2-0-9-04.exe"
 "OnVUE-23.7.50.exe" "OnVUE-23.8.39.exe" "NinjaTrader.Install.msi" "VSCodeUserSetup-x64-1.60.0.exe" "VSCodeUserSetup-x64-1.75.1.exe"
 "VSCodeUserSetup-x64-1.90.1.exe" "AWSCLIV2.msi" "python-3.13.7-amd64.exe" "win-mg2500-1_1-mcd.exe" "mysql-installer-web-community-8.0.38.0.msi"
 "irt_win_1426.exe" "irt_win_1512 (1).exe" "irt_win_15125.exe" "irt_win_15128.exe" "irt_win_152b9.exe" "irt_win_161b10.exe"
 "irt_win_161c1 (1).exe" "irt_win_161c1.exe" "irt_win_161c2 (1).exe" "irt_win_161c2 (2).exe" "irt_win_161c2.exe" "rtraderpro.msi"
 "avast_one_setup_online (1).exe" "avast_one_setup_online (2).exe" "avast_one_setup_online.exe" "avastsupport (1).exe" "avastsupport.exe"
 "Support-LogMeInRescue.exe" "ChatGPT Installer.exe" "ChromeSetup.exe" "Claude Setup (1).exe" "Claude Setup.exe" "GOG_Galaxy_2.0.exe"
 "GOG_Galaxy_Heroes_of_Might_and_Magic_3_Complete.exe" "GoogleCloudSDKInstaller.exe" "VisualStudioSetup.exe"
 "datasets.zip" "Display Pilot for Windows_V2.2.08.0_Windows.zip" "dea_fut_xls_2024 (1).zip" "IRT_MBO_Purpose_Enhancements_Source (1).zip"
 "GC  LTF_GCEZ26_TimeAndSales.csv" "futurespulse-repo.tar.gz" "futurespulse-update.tar_1.gz"
 "irt-crash-info.bat" "irt-crash-info.txt" "irt-finish.bat" "irt-hold-round2.bat" "irt-hold-round3.bat" "irt-restore-all.bat"
 "irt-round4.bat" "irt-round5.bat" "irt-round6.bat" "irt-round7.bat" "irt-round8.bat" "irt-round9.bat"
) do if exist "%DL%\%%~F" (move /y "%DL%\%%~F" "%TD%\" >nul && echo moved %%~F>> "%LOG%")
if exist "%DL%\Telegram Desktop\zoom_0.mp4" (move /y "%DL%\Telegram Desktop\zoom_0.mp4" "%TD%\" >nul && echo moved Telegram zoom_0.mp4>> "%LOG%")
for %%D in ("Display Pilot for Windows_V2.2.08.0_Windows" "TableauAutoUpdate" "New folder" "Movavi Sync") do if exist "%DL%\%%~D" (move /y "%DL%\%%~D" "%TD%\" >nul && echo moved folder %%~D>> "%LOG%")
echo 2/2  Sizing the LRA and GEX folders (read only) ...
set "SZ=%~dp0folder-sizes.txt"
echo ==== %date% %time% > "%SZ%"
for %%R in ("C:\Dev\level-reversal-analytics" "C:\Dev\level-reversal-analytics\data" "C:\Dev\level-reversal-analytics\data\mbo" "C:\Dev\level-reversal-analytics\data\databento" "C:\Dev\level-reversal-analytics\research-artifacts" "C:\Dev\gex-signal-tapereader" "C:\Dev\gex-signal-tapereader\plugin" "%USERPROFILE%\InvestorRT") do (
  for /d %%D in ("%%~R\*") do (
    for /f "tokens=3" %%B in ('robocopy "%%~D" "%TEMP%\_nul_" /L /S /BYTES /NJH /NFL /NDL /NC /XJ /R:0 /W:0 ^| findstr /c:"Bytes :"') do echo %%B,%%~D>> "%SZ%"
  )
)
echo DONE>> "%SZ%"
echo.
echo Finished. Open Downloads\_to_delete, check it, then select all and press Shift+Delete.
pause
