# EVERY BUILD RUNS THIS. NO EXCEPTIONS.

**A build is not finished when the code works. It is finished when a fresh context could pick it up.**

The user should never have to say "save". If a build shipped and the resume note was not updated in the
same commit, the build is incomplete — and the next context will rediscover, at cost, whatever was left
undocumented. That has already happened repeatedly in this project.

## The checklist

1. **Code + tests in the same commit.** A deliberate change that breaks a test means the TEST is updated
   now, not later. A permanently-red baseline trains everyone to ignore red — 23 stale failures once
   camouflaged two live bugs for months.
2. **Full suite green.** `cp current/gex-signal-tapereader.user.js v10.js` first — the harness reads
   `v10.js`, not `current/`.
3. **Smoke test.** Loads the whole script in a DOM stub and calls every debug hook. The empty-book case
   is what throws.
4. **Bump `@version` AND `GPTS_VERSION`**, plus the version pins in `test_direction_grade.js`,
   `test_pipeline_indicator.js`, `test_rules_v2.js`, `test_read_v1047.js`.
5. **Prepend a CHANGELOG entry.** Not what changed — *why it was wrong before*. The reasoning is the part
   that stops it recurring.
6. **UPDATE `session-state/latest-resume-note.md` IN THE SAME COMMIT.** Rewrite the affected sections
   rather than appending; an accreted note buries the important parts. It must carry:
   - current version of BOTH scripts and what is verified live
   - anything LEARNED this build — especially a wrong assumption and what corrected it
   - open threads, stated precisely enough to resume without re-deriving
   - any new landmine, with the symptom that would reveal it
7. **Snapshot the note** as `session-state/YYYY-MM-DD_resume-vX.Y.md` and delete the previous snapshot.
8. **Build the installer, round-trip verify it**: decode its own payload, untar, `diff` against the
   working tree, confirm `@version` in both scripts, confirm `more +N` equals the `exit /b 0` line.
9. **Send the installer AND both Tampermonkey links.** Both, whenever the companion changed.
10. **Say to wait FIVE minutes**, then reload the Atlas tab.

## Why each of these exists

- **The sandbox container can reset mid-session.** It did on 2026-08-22 and the working copy reverted
  seventeen builds. Nothing was lost only because everything had been pushed. **The installer's push step
  is the only durable copy of the work.** Recover with
  `git clone https://github.com/rassulshah/gex-signal-tapereader.git`.
- **Grep before naming any new function.** Four collisions to date; `ifNum` shipped broken for nine
  releases because the later declaration silently won. `test_no_dupes.js` now fails the build on a new one.
- **NO PowerShell in the installer** — Avast flags it (IDP.HELU.PSE88). base64 + `certutil` + `tar`.
- **`raw.githubusercontent.com` caches for 300 seconds.** Click inside that window and Tampermonkey
  offers Reinstall instead of Update, which looks exactly like a failed push.
- **Installing a userscript does not affect an already-open page.** Always say "reload the tab".

## Three install failures, identical from the user's side

| symptom | cause | one-call check |
|---|---|---|
| GitHub serves the old version | the installer did not push | clone and read `@version` |
| GitHub new, browser old | the page was already open | reload the tab |
| raw stale for ~5 min | CDN cache | read the `cache-control` / `age` headers |
