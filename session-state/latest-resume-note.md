# RESUME NOTE — read this before anything else
_written 2026-08-28 · panel v14.59 · companion v1.15 · supersedes every earlier resume note_

---

## ⚠ 0 · THE CARRY-FORWARD CONTRACT — read before you rewrite this note

**This note is rewritten IN FULL every build. Anything you do not re-type is GONE — silently.** That
is how **ITEM 18** was reported three times as "never built" while its spec sat in commit `72e820e`.

1. **`session-state/LOCKED-ITEMS.md`** — agreed-but-unbuilt work. An item leaves it only by being
   built or explicitly cancelled **with a date**. Check it every build.
2. **THE LOAD CLONES FULL.** Never `--depth 1`. Before concluding anything was never built:
   `git log --all --oneline -S"<term>" -i`.
   ⚠⚠ **AND SEARCH THE RIGHT STRING.** On 2026-08-28 I searched `finance.yahoo` and reported no
   Yahoo documentation existed. A file saying *"Yahoo Finance"* cannot match that pattern. The
   operator pushed back and he was right. **`git grep -li "<bare word>" $(git rev-list --all)`.**
3. **`session-state/CHAT-HISTORY.md`** — what was *said*, in his words. Read it second.
   **REGENERATE IT ON EVERY BUILD** — `test_chat_history.js` goes red if you don't.
4. ⚠ **NEW: `design/DATA-ARCHITECTURE.md`** — who can reach what, every source, every transport, the
   ES corpus pipeline. It is in the config's canonical list and `test_futbars.js` f31–f41 fail the
   build if it is unhooked. **Data-source facts go THERE now, not in this note.**

⚠⚠ **A COMMIT IS NOT A PUSH.** The cloud gets a 403 from the git proxy by design. The installer,
run on his machine, is the only route to GitHub.

He works **one item at a time** and expects you to **discuss before building**. He has caught more
real defects than the test suite has. Treat his observations as data.

---

## 1 · THE STANDING BUSINESS REQUIREMENT (his words, never paraphrase it away)

> "I am a trader and need to know where to take trades from and where price is going, so basically I
> need to know potential support and resistance especially if it is weakening and new support and
> resistance is forming as well as where price is going."

---

## 2 · WHERE WE ARE — v14.59, and what it changed

**Shipped v14.59 + companion v1.15: the ES corpus now has a daily tap, and the HOD/LOD base rates
travel without a rebuild.** He asked for a process that obtains ES data daily, keeps ⓪a current,
is documented so `load gex` finds it, and extends to other markets.

```
companion v1.15 --GM_xmlhttpRequest--> query1.finance.yahoo.com  (ES=F NQ=F GC=F CL=F, 1m, 5d, hourly)
   -> gpts_futbars_v1 -> panel v14.59 -> `futBars` in data/YYYY-MM-DD.json -> his push
   -> tools/append-futures.py -> data/futures/<SYM>/YYYY-MM-DD.csv
   -> tools/study-hodlod.py --market ES --out data/es-1min/BASERATES.json
   -> companion couriers it back -> gpts_hodlod_base_v1 -> the panel prefers it over HODLOD_BASE
```

**Only the two Yahoo legs are new.** Everything else was an existing pipe.

⚠ **THE PANEL CANNOT FETCH YAHOO — MEASURED, not assumed:** a plain `fetch` from the Atlas page
returns `BLOCKED: Failed to fetch`. That is item 18's open question from 2026-08-16, finally
answered. `@grant none` stays load-bearing; foreign fetches go in the companion.

⚠ **VERIFIED AGAINST THE LIVE ENDPOINT BEFORE THE PARSER WAS WRITTEN:** 2674 bars over 2 days,
**152 of them null** (a null read as zero becomes a fake LOD), and the UTC trim retains **391 RTH
bars, exactly 08:30→15:00 CT** — the complete-session count `MIN_BARS=386` expects.

**Also fixed:** every ⓪a ladder hover had read **"undefined of 1169"** since v14.57 — `HODLOD_BASE`
carried no `held` and 42 assertions passed over it because none executed the hover text.

---

## 3 · WHAT IS TRUE ABOUT THE DATA (be sceptical of this project's own headline numbers)

| claim | status |
|---|---|
| HOD/LOD ladder 41/56/67/75/84 | measured over **284 sessions**, reproducible via `tools/study-hodlod.py` |
| roll destinations held 74%, drained sources broke 19/19 | measured, small n, still the strongest thing we have |
| roll arrows as a 30-min direction signal: **4/12** | arrows are STRUCTURE, not direction |
| magnet/attract 77% toward within 30m | **ONE day (n=47), contradicted below** |
| nightly scorer 2026-08-26 | "no direction factor beat the tape (trend 34%, leg 34%)" |
| deflections 2026-08-26 (n=53) | Floor 73% · Rug 73% · Ceiling 43% · **King 38%** · Gate 0% — **the King is the worst level on the board**, one day, n=8 |

⚠ **EVERY THRESHOLD IN THE STATE ENGINE IS HAND-SET**, and labelled ⚖ in source.

---

## 4 · WHAT TO DO NEXT, IN ORDER

1. **CONFIRM THE TAP RAN.** After a session with v14.59 + v1.15 installed:
   `__gptsDebug.futBars()` → per-market bar counts, errors, age. Then his push, then
   `python3 tools/append-futures.py` in the cloud. **It has never run on real couriered data** —
   the fixture proves the code, not the loop.
2. **ANSWER: WHAT IS "ND"?** He named ES/NQ/GC/**ND**/CL. ND was deliberately NOT added; a guessed
   symbol puts the wrong series in the corpus under a right-looking name. `test_futbars` f26 fails
   the build if it appears. Adding it is one row in `FUT_MARKETS`.
3. **GET `data/es-1min/EPM26-1min.csv.gz` ONTO GITHUB.** 5.1MB against a 6MB payload cap — the ONE
   tracked file the installer cannot carry. Until it lands the study cannot re-derive the
   284-session rates and the panel serves the baked-in copy. He drops it into
   `C:\Dev\gex-signal-tapereader\data\es-1min\` and his next run carries it.
4. **DIAGNOSE THE FEATURE-RECORD COLLAPSE.** 3,822 records on 08-20 against **15 on 08-27**, and
   `matrix` rows track them exactly (108→3132, 122→3822, 0, 0, 23→990, 2→8, **1→15**) against 133
   SPY snaps. **It is ONE upstream gate, not 46 features failing.** Nothing statistical is
   trustworthy until this is understood.
5. **THE LADDER WIDTH — HIS CALL, STILL OPEN.** True width 618 in a 454px body; it scrolls, nothing
   is lost. Options in `LOCKED-ITEMS.md`. Cheapest is widening the panel to ~620, one drag.
   **Do not close it by deleting a column.**
6. **ITEM 18 IS HALF BUILT.** Route and bar feed exist; Tier 1/Tier 2 (`snap.htf`, DMAs, ATR, HTF
   levels) do not. **The 2026-09-16 backfill deadline for 07-18→08-14 is untouched** — that hole is
   recoverable only at 2-minute resolution and only until then.
7. **⓪a STILL OWES:** FEATURE ENROLLMENT (it records nothing, is scored by nothing, against the
   2026-08-17 mandate); **BOP · WICK · W.END · WICK% · MUD** need HIS definitions — they are printed
   PENDING rather than invented; VWAP does not exist in the codebase and renders UNAVAILABLE.

---

## 5 · HOW TO WORK WITH HIM

1. **ONE AT A TIME.** State the one item, its fix, ask, STOP. The tell of the failure mode: three
   headings, a comparison table, and "which do you prefer" at the bottom.
2. **Do not build until he says build.** He says it plainly when he means it.
3. **RENDER EVERY MOCKUP HEADLESS FIRST**, capture `pageerror`, run the pairwise overlap audit.
4. ⚠⚠ **DELIVER EXACTLY ONE FILE.** His words, 2026-08-15 and again 2026-08-27:
   *"you are supposed to just give me an install file."* One `installvNNNN.bat`, dash-free and
   dot-free (downloads strip both) — **plus the Tampermonkey links as text.** Not a zip+applier
   pair, not "here are both", not the installer plus a render. `test_delivery.js` fails the build if
   any of the six sources a context reads stops saying it — **it caught this note dropping the quote
   during the v14.59 rewrite.**
5. ⚠ **AND TELL HIM TO CLICK THE LINK.** Tampermonkey's default update check is ONCE A DAY.
   The sequence: run the `.bat` → wait ~5 min for the CDN → **click the link** → **reload Atlas**.
   *Reinstall* means auto-update beat you to it and he HAS the build. Diagnose by reading the
   running version off the panel, never by asking.
6. **Bump BOTH version strings** (`@version`, `GPTS_VERSION`) and the four test pins
   (`test_direction_grade`, `test_pipeline_indicator`, `test_read_v1047`, `test_rules_v2`).
7. **One edit, one write, verify.** A multi-edit script that aborts writes NOTHING.
8. **Run the whole suite; 6 baseline reds are expected.** `test_chat_history` red means you have not
   regenerated the history — that is the gate working.
9. **MUTATE EVERY NEW ASSERTION INDIVIDUALLY** and confirm the guard you meant to test is what goes
   red. v14.59 found one that could not fail this way (see PROJECT-CONSTANTS L-K).
10. **He is often right about things the tests pass on.**

---

## 6 · POINTERS

- ⚠ **`design/DATA-ARCHITECTURE.md`** — who can reach what, both books, the four transports, the ES
  pipeline, the known gaps. Read on every load. `design/architecture-design.md` is SUPERSEDED on
  data sources and carries a banner saying so.
- `session-state/LOCKED-ITEMS.md` · `YAHOO-PIPELINE.md` (item 18 verbatim) · `INSIDERFINANCE.md`
  (the second book) · `SKYLIT-FEEDS.md` (the app map + the four capture rules: **RTH · READ AS
  %King · VELOCITY All · LOW NODES never Hide**) · `PROJECT-CONSTANTS.md` (landmines A–M) ·
  `DECISIONS.md` D-1..D-16 · `ISSUES-NEXT-BUILD.md` (⚠ item 5, the EM band clipping piles to ~5 ES
  points, is still the largest live defect there).
- ⚠ **NEVER EXISTED, not lost:** `skylit-docs/FINDINGS.md` (named by three live hovers),
  `BUILD-PLAN.md`, `garma/V2-PHASE-PLAN.md`, `wicks/` — verified with
  `git log --all --diff-filter=A`.
- ⚠ `origin/main` still tracks **`v10.js` at v11.48** and `install.bat`: the `git rm --cached` was a
  sandbox commit and never pushed. A fresh clone running one test file tests v11.48 and goes green.

## 7 · DOCTRINE THAT MUST NOT BE LOST

- **Two books, never averaged.** Skylit = FLOW (`|net| ≡ v`, no call/put split). IF = OI×gamma.
  Disagreement is displayed, never reconciled.
- **We store their numbers; we do not invent our own.**
- **Name both units out loud before comparing two numbers.** `kingKd` is THOUSANDS; `velocity.cur`
  and `.d15` are DOLLARS.
- **Absence of data is not a reading.** Refuse and say why; never guess a number.
- **A well-formed number is not a supported one.** Monotone ≠ evidence (L-J).
- **Gamma tells you HOW price moves, never WHICH WAY.** The read never says likely/will/should.
- **Anything unproven ships labelled unproven and scored nightly.**
