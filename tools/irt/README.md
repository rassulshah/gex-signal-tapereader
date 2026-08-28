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

## ⚠⚠ 2026-08-28 — THE SERVER IS NO LONGER NEEDED. `file://` WORKS IN **REMOTE FILE** MODE.

**Measured on the operator's own chart:** paste

    file:///C:/Users/rassul/InvestorRT/rtx/lsFlexLevels/FlexLevelsExport.csv

into IRT's **Remote File** field (not Local File), *Check For Updates Every: 1 Minute*, and the levels
**re-read on the timer** — confirmed by SPXW KING redrawing from 7803.50 to 7726.00 without any HTTP
server running.

**So the 2026-08-27 conclusion was over-generalised.** What was measured then was true and remains
true: **IRT never re-reads a LOCAL FILE.** What was inferred from it — "so the file must be served
over HTTP" — was wrong. The Remote File fetcher takes a URL of any scheme it understands, and
`file://` is one of them. **One radio button separated a working pipe from a day of infrastructure.**

⚠ **THE LESSON, and it is this project's oldest one in a new costume:** the fix was built on an
untested premise about WHICH SETTING was at fault. The measurement was real; the boundary drawn
around it was not. Before building infrastructure to route around a limit, test the limit's edges.

### What this means for the four files here
They are now a **FALLBACK, not the path.** Keep them: if a future IRT version drops `file://`
support, or the folder moves onto a network share, the HTTP server still works and is proven. But
nothing needs to be running, nothing goes in Startup, and `setupautostart.bat` should NOT be run.
**To undo an autostart that was already installed:** `Win+R` → `shell:startup` → delete
`irtstartup.bat`.
