' GEX BUILD - hidden launcher (2026-09-17; 2026-11-09 safe launcher, Rassul approved 15:15-15:22 CT).
' The task runs THIS, and this runs the .bat with its window hidden (window style 0). No PowerShell: plain Windows Script Host.
' WSH reads this whole file before running, so editing it never breaks a run in progress.
' Before starting gex-build.bat it now:
'  1) STALE LOCK: tools\gex-build.lock older than 30 min while gex-build.log has not moved for 30 min (the same rule as
'     lra.watchdog_infra) -> the lock is MOVED (never deleted) to C:\Dev\_to_delete\<date>\ and the build goes ahead.
'  2) SWAP-IN: tools\gex-build.next.bat (written by Claude instead of editing the live script) differs from gex-build.bat and
'     no build is running -> gex-build.bat is copied to gex-build.prev.bat, next is copied over gex-build.bat, and next is moved to
'     plugin\out\applied\. The live .bat is never changed while a run could be reading it.
' Each action writes a line to tools\gex-build.log and plugin\out\BUILD-EVENTS.txt (read by the watchdog's morning email).
Option Explicit
Dim fso, sh, T, OUTD, LOCKF, LOGF, BATF, NEXTF, PREVF, EVF, lockAge, logAge, stamp, d
Set fso = CreateObject("Scripting.FileSystemObject")
Set sh = CreateObject("WScript.Shell")
T = "C:\Dev\gex-signal-tapereader\tools"
OUTD = "C:\Dev\gex-signal-tapereader\plugin\out"
LOCKF = T & "\gex-build.lock" : LOGF = T & "\gex-build.log" : BATF = T & "\gex-build.bat"
NEXTF = T & "\gex-build.next.bat" : PREVF = T & "\gex-build.prev.bat" : EVF = OUTD & "\BUILD-EVENTS.txt"

Function P2(n) : P2 = Right("0" & n, 2) : End Function
Function NowTxt()
  d = Now
  NowTxt = WeekdayName(Weekday(d), True) & " " & P2(Month(d)) & "/" & P2(Day(d)) & "/" & Year(d) & " " & P2(Hour(d)) & ":" & P2(Minute(d)) & ":" & P2(Second(d)) & ".00"
End Function
Sub Note(msg)
  On Error Resume Next
  Dim f
  Set f = fso.OpenTextFile(LOGF, 8, True) : f.WriteLine NowTxt() & " " & msg : f.Close
  If Not fso.FolderExists(OUTD) Then fso.CreateFolder OUTD
  Set f = fso.OpenTextFile(EVF, 8, True) : f.WriteLine NowTxt() & "," & msg : f.Close
  On Error GoTo 0
End Sub
Function AgeMin(p)
  If fso.FileExists(p) Then AgeMin = DateDiff("n", fso.GetFile(p).DateLastModified, Now) Else AgeMin = -1
End Function
Function ReadAll(p)
  On Error Resume Next
  Dim f : ReadAll = Chr(0)
  Set f = fso.OpenTextFile(p, 1) : ReadAll = f.ReadAll : f.Close
  On Error GoTo 0
End Function
Sub EnsureDir(p)
  Dim parent : parent = fso.GetParentFolderName(p)
  If parent <> "" And Not fso.FolderExists(parent) Then EnsureDir parent
  If Not fso.FolderExists(p) Then fso.CreateFolder p
End Sub
stamp = Year(Now) & P2(Month(Now)) & P2(Day(Now)) & "-" & P2(Hour(Now)) & P2(Minute(Now)) & P2(Second(Now))

' 1) stale lock
lockAge = AgeMin(LOCKF) : logAge = AgeMin(LOGF)
If lockAge >= 30 And (logAge < 0 Or logAge >= 30) Then
  On Error Resume Next
  Dim dst : dst = "C:\Dev\_to_delete\" & Year(Now) & "-" & P2(Month(Now)) & "-" & P2(Day(Now))
  EnsureDir dst
  fso.MoveFile LOCKF, dst & "\gex-build.lock." & stamp
  If Err.Number = 0 Then Note "STALE LOCK cleared (age " & lockAge & " min, moved to " & dst & ")" Else Note "STALE LOCK could not be moved (age " & lockAge & " min): " & Err.Description
  Err.Clear
  On Error GoTo 0
End If

' 2) swap in gex-build.next.bat (only when no build is running)
If fso.FileExists(NEXTF) And Not fso.FileExists(LOCKF) Then
  If ReadAll(NEXTF) <> ReadAll(BATF) And Len(ReadAll(NEXTF)) > 1000 Then
    On Error Resume Next
    fso.CopyFile BATF, PREVF, True
    If Err.Number = 0 Then fso.CopyFile NEXTF, BATF, True
    If Err.Number = 0 Then
      EnsureDir OUTD & "\applied"
      fso.MoveFile NEXTF, OUTD & "\applied\gex-build.next." & stamp & ".bat"
      Note "SWAPPED IN gex-build.next.bat (old script kept as gex-build.prev.bat)"
    Else
      Note "SWAP-IN failed: " & Err.Description
    End If
    Err.Clear
    On Error GoTo 0
  ElseIf ReadAll(NEXTF) = ReadAll(BATF) Then
    On Error Resume Next
    EnsureDir OUTD & "\applied"
    fso.MoveFile NEXTF, OUTD & "\applied\gex-build.next." & stamp & ".same.bat"
    On Error GoTo 0
  End If
End If

sh.Run "cmd /c """"C:\Dev\gex-signal-tapereader\tools\gex-build.bat""""", 0, False
