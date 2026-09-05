' GEX NIGHTLY - hidden launcher (v15.68). The task runs THIS, and this runs the .bat with its window hidden
' (window style 0), so nothing pops up every ten minutes. No PowerShell: plain Windows Script Host.
Set sh = CreateObject("WScript.Shell")
sh.Run "cmd /c """"C:\Dev\gex-signal-tapereader\tools\gex-nightly.bat""""", 0, False
