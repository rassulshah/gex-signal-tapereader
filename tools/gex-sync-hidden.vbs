' GEX SYNC - hidden launcher (v15.62). The task runs THIS, and this runs the .bat with its window hidden
' (window style 0), so nothing pops up every two minutes. No PowerShell: plain Windows Script Host.
Set sh = CreateObject("WScript.Shell")
sh.Run "cmd /c """"C:\Dev\gex-signal-tapereader\tools\gex-sync.bat""""", 0, False
