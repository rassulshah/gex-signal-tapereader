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
   ⚠ **AND THE TEST MUST RUN THE CODE, NOT GREP IT.** v11.86 shipped fourteen assertions that could not
   catch a wrong price; v11.70 shipped a forecast ban that passed on inserted forecast text. Third
   occurrence. `eval(ex('fn'))` with stubs costs 47ms. **Then mutate the source and confirm the
   assertions fire** — an assertion that never fails buys false confidence, which is worse than none.
3. **Smoke test** — `node tools/smoke.js`. Loads the whole script in a DOM stub, calls every debug hook,
   AND fails on anything a render catch swallowed. A section that renders empty because its own
   try/catch ate a ReferenceError looks exactly like a section with nothing to show; this is the only
   thing that tells them apart. It has caught a missing function and an out-of-scope variable already.
4. **Bump `@version` AND `GPTS_VERSION`**, plus the version pins in `test_direction_grade.js`,
   `test_pipeline_indicator.js`, `test_rules_v2.js`, `test_read_v1047.js`.
5. **Prepend a CHANGELOG entry.** Not what changed — *why it was wrong before*. The reasoning is the part
   that stops it recurring.
6a. **REGENERATE `session-state/CHAT-HISTORY.md` IN THE SAME COMMIT.**
   ⚠ **`test_chat_history.js` FAILS THE BUILD IF YOU DO NOT.** The version stamped in the history
   must equal `GPTS_VERSION`, so bumping the version without regenerating turns the suite red. This
   is deliberate: a checklist line is prose a hurried context can skip, and prose is what lost item
   18. The test also pins the wiring (config, skill, this file) so the generator cannot be quietly
   orphaned.
   `python3 tools/chat-history.py --title "<what this context was about>"`, then fill DECISIONS /
   SHIPPED / OPEN AT CLOSE by hand. Operator-mandated 2026-08-27 and it is not optional: the next
   `load gex` reads the CURRENT-CONTEXT entry in full, and it is the only record of what was *said*
   — the corrections and the rejected approaches — as opposed to what was concluded. It is generated
   from the real transcript precisely so it cannot forget; the hand-written parts are the three
   summary sections only.

6. **UPDATE `session-state/latest-resume-note.md` IN THE SAME COMMIT.** Rewrite the affected sections
   rather than appending; an accreted note buries the important parts. It must carry:
   - current version of BOTH scripts and what is verified live
   - anything LEARNED this build — especially a wrong assumption and what corrected it
   - open threads, stated precisely enough to resume without re-deriving
   - any new landmine, with the symptom that would reveal it
7. **Snapshot the note** as `session-state/YYYY-MM-DD_resume-vX.Y.md` and delete the previous snapshot.
8. **Build the delivery with `python3 tools/build-installer.py "vX.Y: one-line commit message"`.**
   ⚠ **THE PRIMARY DELIVERY IS THE PAIR `gexdropNNN.zip` + `applygexNNN.bat`** (2026-08-25: the
   self-extracting installer failed three different ways in one day — a 30MB payload hung `more`,
   certutil is an Avast target, and downloads STRIP DASHES from filenames so an exact-name check
   missed its own zip). The applier is CRLF (an LF .bat closes instantly on double-click), finds the
   zip by wildcard in three places, and unstages every delivery artifact before committing.
   `install-vX.Y.bat` is still emitted as the fallback.
   **Deliver BOTH pair files in ONE message.**
   It reads every version from the files, solves `HDRLINES` to a fixed point, round-trips its own payload
   against the working tree, and refuses to emit a stale `v11.x` header or the word PowerShell.
   ⚠ **NEVER HAND-EDIT `install.bat`.** The v11.86 one said v11.49 in its banner, v11.79 in its commit
   message, and 1.8 for a 1.13 companion. Its payload was correct, which is why it survived four builds.
9. **RUN `bash tools/release-links.sh` AND PASTE ITS OUTPUT.** Not "remember to send the link" — RUN
   THE SCRIPT. It reads both `@version` values, marks which script actually changed against origin/main,
   and prints the block ready to paste. Step 9 said "always send the link" from v11.9 onward and was
   still missed at v11.51; a reminder competes with everything else at the end of a build, a step that
   PRODUCES the message does not. If the block is absent from the delivery message, the step was skipped.
   ⚠ There are TWO scripts — the tapereader and the companion — and linking one that did NOT change makes
   Tampermonkey offer **Reinstall** instead of **Update**, which reads exactly like a failed push.
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

---

# SPEED — MEASURED 2026-08-22, NOT ASSUMED

A build was taking far longer than the change deserved. The cause was NOT where it was assumed to be.
Every number below was timed on this repo; do not re-derive them from intuition.

| thing | measured |
|---|---|
| **whole suite, 105 files, sequential** | **4.5 s** |
| whole suite, `xargs -P` | 2.3 s |
| `node tools/smoke.js` | 0.078 s |
| one targeted test | 0.047 s |
| **one subagent, any size of task** | **3–7 min fixed overhead** |

**TESTING IS FREE. SUBAGENTS ARE NOT.** The v11.49 build spent ~16 minutes inside three subagents and
about 18 seconds running tests four times. Anyone who "saves time" by running fewer tests has the cost
model exactly backwards.

## The four rules that follow from those numbers

**1. DELEGATION THRESHOLD. Delegate only when the task needs its OWN exploration, or exceeds ~15 edits
across files you have not already opened.** A subagent costs 3–7 minutes before it does anything useful.
The v11.49 `HVL`→`FLIP` rename — 35 mechanical line edits across 8 files — was delegated and took **7
minutes**; two `sed` calls inline would have taken under thirty seconds. MODEL ROUTING is about
*expensive reasoning*, not about *typing*. Renames, version bumps, packaging a file, running a suite:
do them inline.

**2. WRITE THE TEST FIRST — it is 47 milliseconds and it forces you to name the input.** v11.49 shipped a
fix for `toFri.em` vs `dte0.em` that would have surfaced in the first two minutes, because the FIRST
assertion anyone writes for an EM feature is "which EM does it read". Instead it was found after a mockup
had been built on the wrong number and thrown away. A test written first is a design review that runs.

**3. VERIFY THE DATA BEFORE DESIGNING ANYTHING.** One `__gptsDebug` call against the live page beats an
hour of reasoning about what the payload probably contains. Every mockup drawn before that call is a
mockup that may have to be discarded — and one already was.

**4. BATCH TOOL CALLS.** Four greps issued one at a time cost four round-trips and answer one question.
Combine them with `;` and read the whole answer at once.

## THE RESUME NOTE IS SPLIT — DO NOT REMERGE IT

`session-state/latest-resume-note.md` was ~250 lines fully rewritten every single build, and most of it —
failure patterns, build mechanics, standing instructions — changes maybe once a month. It is now two files:

- **`session-state/PROJECT-CONSTANTS.md`** — failure patterns, the user's stated rules, payload facts,
  build mechanics, debug surface, standing instructions. **Touch it only when one of those actually
  changes.** Most builds do not.
- **`session-state/latest-resume-note.md`** — WHERE WE ARE ONLY: current version, what is verified vs
  unverified live, what is at the top of the queue, open threads. Short. Rewritten every build.

**A load must read BOTH.** The constants file is not optional context — it is where the landmines live.
