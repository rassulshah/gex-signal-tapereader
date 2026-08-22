# Installer template — THE INSTALLER COMMITS AND PUSHES

Regression to never repeat (2026-08-20): several v11.9–v11.15 installers were shipped WITHOUT the
commit+push step. The user ran them, the files landed in `C:\Dev\gex-signal-tapereader`, and nothing
reached GitHub — so the Tampermonkey raw URL kept serving the previous version and Tampermonkey offered
"Reinstall" instead of "Update". That symptom reads like a corrupt file and cost a round trip to diagnose.

`tools/push-data.bat` does NOT cover this. It runs `git add data` only — the daily export, never code.

Every installer .bat must therefore end with, before `pause`:

1. Git discovery identical to `tools/push-data.bat` (PATH, Program Files, LOCALAPPDATA, then the copy
   bundled inside GitHub Desktop under `%LOCALAPPDATA%\GitHubDesktop\app-*\resources\app\git\cmd\git.exe`).
2. `git add -A`
3. `git diff --cached --quiet` — errorlevel 1 means there ARE staged changes.
4. `git commit -m "<version>: <one line>"` then `git push`.
5. A distinct printed message for each outcome: pushed / nothing to commit / git not found / commit failed /
   push failed. Never a silent success, because a silent failure here is invisible until Tampermonkey
   misbehaves two steps later.

Payload rules (unchanged): base64 after `exit /b 0`, extracted with `more +<HDRLINES>` then
`certutil -f -decode` then `tar -xzf`. NO PowerShell anywhere — Avast flags it (IDP.HELU.PSE88).
`<HDRLINES>` is the header's line count and must be recomputed whenever the header changes.


## THE RAW CDN CACHES FOR FIVE MINUTES, NOT TWO (2026-08-22)

`raw.githubusercontent.com` returns `cache-control: max-age=300`. Installers have been telling the user
to "wait about 2 minutes for the GitHub raw cache to flush" — click inside the real window and
Tampermonkey fetches the PREVIOUS version, sees no version bump, and offers **Reinstall** instead of
**Update**. That looks exactly like a failed push and it is not.

A cache-busting query string does NOT reliably defeat it, and Tampermonkey requests the plain URL anyway.

**Three install failure modes now, all identical from the user's side:**

| symptom | cause | one-call check |
|---|---|---|
| GitHub serves the old version | the installer did not push | `git ls-remote` / clone and read `@version` |
| GitHub new, browser old | the page was already open | reload the tab |
| raw serves stale for ~5 min | CDN cache | read the `cache-control` / `age` headers |

Installer text must say **five minutes**.
