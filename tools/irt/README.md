# THE IRT FLEXLEVELS SERVER — the four files, what each one is for, and why they exist

**Recovered into git 2026-08-28.** These were built on 2026-08-27, delivered to the operator as loose
chat attachments, and **never committed** — so a later context searched the whole repo, found
nothing, and told him the server "was never built". It was. It existed only outside git, which in
this project means it did not exist. ⚠ **Anything the operator has to run belongs HERE, in the repo,
and rides the installer.**

## Why a server at all (measured 2026-08-27, not assumed)

The panel writes `FlexLevelsExport.csv` **in place** and correctly (v14.52 — `inPlace:true`, file
identity preserved, mtime advancing every 180s). **IRT still never re-reads it.** Proof recorded at
the time: the file said `SPXW KING 100% ~ = 7717.00` with one row while IRT drew 7748.25 and 7731.50
from minutes earlier — 31 points and a missing row.

**IRT does not poll a LOCAL file.** Its *Check For Updates Every: 1 Minute* governs **Remote File**
mode only. No write strategy in the browser can fix that, which is why the file is served over HTTP.

## The four files

| file | where it goes | what it does |
|---|---|---|
| `irtserve.py` | **`C:\Users\rassul\InvestorRT\rtx\lsFlexLevels`** | the server. Binds the first free port of 8000/8765/8181/8899, IPv4 first, serves the folder with `Cache-Control: no-store` and **no `Last-Modified`/`ETag`** so a poll can never sit on a 304. Every log line prints the CSV's age and **shouts when it is older than 8 minutes**. |
| `irtserve.bat` | same folder | double-click launcher: finds Python (`py -3`, `python`, `python3`, then known install paths), then runs the server in that folder. |
| `irtstartup.bat` | **copied into the Windows Startup folder** by the setup script | the autostart launcher. ⚠ It does NOT use `%~dp0` (that would be the Startup folder) — the data folder is **hardcoded** at `FOLDER=`. |
| `setupautostart.bat` | run ONCE, **from the lsFlexLevels folder** | copies `irtstartup.bat` into `%APPDATA%\…\Startup` and launches it immediately. Undo: `Win+R` → `shell:startup` → delete `irtstartup.bat`. |

## ⚠ THE TRAPS, EACH ONE MEASURED

1. **`irtserve.py` MUST live in `lsFlexLevels`.** `irtstartup.bat` does `cd /d "%FOLDER%"` and then
   runs `irtserve.py` **by relative name** — so if the .py is sitting in Downloads, the autostart
   launcher fails at login with a Python "can't open file" error and nothing is listening.
2. ⚠⚠ **NEVER POINT IRT AT `localhost`.** On Windows `localhost` resolves to IPv6 `::1` first; the
   server binds IPv4 first, and you get `ERR_CONNECTION_REFUSED` while the server is running fine.
   **Use the URL the server PRINTS**, which is derived from the bind that actually succeeded.
   ⚠ `irtserve.bat`'s own header comment says `http://localhost:8000/...` and **that comment is
   wrong** — it contradicts `irtserve.py`'s warning. Two files, two answers, and the operator has to
   guess which: exactly the class of contradiction that sent three attachments in one delivery on
   2026-08-27. Fix the comment, do not fix the code.
3. **A 200 from this server is NOT proof the levels are fresh.** If the panel stops writing — Chrome
   drops the folder permission on **every page load** — the CSV sits on disk and the server serves
   it happily forever. That is why every log line carries the file's age.
4. **NO POWERSHELL ANYWHERE.** Avast flags it (IDP.HELU.PSE88). Same rule as the installer.

## How to tell, in ten seconds, which layer is broken

    the browser refuses 127.0.0.1:8000/8765/8181/8899   -> the SERVER is not running
    the server logs no lines after IRT's OK             -> IRT is not FETCHING (wrong URL/mode)
    the log says "CSV IS n MIN OLD"                     -> the PANEL is not writing (permission)
    the CSV has 2 rows                                  -> the band never anchored (emPiles chain)

`FlexLevelsExport.sample.csv` is a real capture, kept so the header and row shape can be diffed if
the format ever drifts. ⚠ Its two rows are the "band never anchored" case, not a healthy export.

---

## ⚠⚠ 2026-08-28 — THE SERVER IS THE PATH. `file://` READS ONCE AND NEVER POLLS.

**This section replaces an earlier one written the same afternoon that said the opposite.** Both
halves were measured; the first was measured too soon.

**What is true:** IRT's **Remote File** field ACCEPTS a `file:///C:/.../FlexLevelsExport.csv` URL and
reads it — the SPXW King redrew 7803.50 → 7726.00 the moment it was applied.
**What was WRONG:** concluding it therefore polls. It does not. Measured an hour later: the panel had
shipped v14.73, which removed `100%` from the King labels, and both charts were still drawing
`SPXW KING 100%` — labels that no longer existed in the file. **`Refresh` did not move them either.**
`file://` gives one read at apply-time and then silence.

⚠ **HOW THIS FOOLED ME, because the pattern will recur:** the one confirming observation (a level
moving) happened at the same moment as the settings change that caused the read. Cause and
confirmation were simultaneous, so a single re-read looked like a working poll. **A polling claim
needs TWO reads with no user action between them.**

### So the standing configuration is:

    irtserve.bat running (or autostarted)
    BOTH charts -> Remote File -> http://127.0.0.1:8000/FlexLevelsExport.csv, Check Every 1 Minute

Verified live 2026-08-28 15:0x CT: both charts polling, all three Kings drawing with current labels
(`SPXW KING` 7725.25 · `SPY KING` 7718.25 on ES, `QQQ KING ~` 29479.00 on NQ).

**RUN `setupautostart.bat` ONCE, from the lsFlexLevels folder**, so the server starts at every login
and the operator never touches it. (An earlier version of this file said NOT to. That was written
during the hour when `file://` was believed to poll.)

### ⚠ And a partial file is what makes the polling path dangerous — fixed in v14.74
While IRT was NOT re-reading, the panel wrote a file containing only ONE row (the QQQ King); the
SPXW and SPY rows were absent because one degraded tick dropped them. On `file://` that left
harmless orphans on the chart. **Over this polling server it would have ERASED those levels
mid-session.** v14.74 latches every King for the session day, so a blinking reader can no longer
blank a chart. **Do not run this server against a panel older than v14.74.**

