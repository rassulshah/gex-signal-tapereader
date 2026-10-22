"""gex-build.bat P3 contract (2026-10-22): no installs on weekdays 07:00-15:15 CT. The Windows script itself is checked live through
plugin\\out\\BUILD-STATUS.txt ("INSTALL WINDOW: ..."); this checks its structure and the clock arithmetic it uses."""
import os, re, sys
s = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "gex-build.bat"), encoding="latin-1").read().replace("\r\n", "\n")
fails = 0
def check(c, m):
    global fails
    print(("ok   " if c else "FAIL ") + m); fails += (not c)
one = s[s.index("\n:one\n"):s.index("REM ---- :ver")]
check(s.index("call :window") < s.index("call :settings"), ":window runs at the start of every tick")
check(one.index("if defined HOLD (") < one.index('copy /Y "%OUT%\\%DLL%"'), "the hold is checked before the DLL copy")
check("goto :eof\n)\ncopy /Y" in one, "a held plugin leaves :one before the copy")
check('del "%OUT%\\%NAME%.held"' in one[one.index("copy /Y"):], "a successful install clears the .held marker")
check('if not defined HOLD if exist "%REPO%\\tools\\gex-hot-build.bat"' in s, "the hot-reload shim also waits outside the window")
check("INSTALL WINDOW: HELD" in s and "INSTALL WINDOW: open" in s, "BUILD-STATUS shows the window")
check("INSTALL-NOW.flag" in s, "his midday-install override exists")
w = s[s.index("\n:window\n"):s.index("\n:windowflag\n")]
check("GEQ 420" in w and "LSS 915" in w, "window = 07:00 (420) .. 15:15 (915) minutes")
check('"Sat"' in w and '"Sun"' in w, "weekends are open")
check("set /a HMIN=1!HT:~0,2!*60 + 1!HT:~3,2! - 6100" in w and "if not defined HMIN set HMIN=-1" in w and "errorlevel" not in w, "octal-safe minutes formula, no stale-errorlevel trap")
# emulate cmd's formula on every %TIME% shape ( " 7:05:12.34" with a leading space, "07:05:12.34", "15:15:00.00")
ok = True
for h in range(24):
    for m in range(60):
        t = ("%2d:%02d:12.34" % (h, m)).replace(" ", "0")              # %TIME: =0%
        v = int("1" + t[0:2]) * 60 + int("1" + t[3:5]) - 6100
        held = 420 <= v < 915
        ok &= (v == h * 60 + m) and (held == ((h, m) >= (7, 0) and (h, m) < (15, 15)))
check(ok, "formula gives the right minute and window for all 1,440 clock times")
print(f"\n{11 - fails} passed, {fails} failed"); sys.exit(1 if fails else 0)
