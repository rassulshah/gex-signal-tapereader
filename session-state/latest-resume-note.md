# RESUME NOTE — read this before anything else
_written 2026-08-27 · panel v14.57 · supersedes every earlier resume note_

---

## ⚠ 0 · THE CARRY-FORWARD CONTRACT — read before you rewrite this note

**This note is rewritten IN FULL every build. Anything you do not re-type is GONE — silently, with no
deletion to notice.** That is how **ITEM 18** (the Yahoo pipeline, locked 2026-08-16, fully specified)
was reported three times as "never built" while its spec sat in commit `72e820e`.

Three mechanisms prevent a repeat and none works if you skip it:

1. **`session-state/LOCKED-ITEMS.md`** — the durable ledger of agreed-but-unbuilt work. An item
   leaves it only by being **built** or **explicitly cancelled by the operator, with the date**.
   Never by omission. **Check it every build.**
2. **THE LOAD CLONES FULL.** `git clone` with no `--depth`. Before ever concluding something was
   never built: `git log --all --oneline -S"<term>" -i`.
3. **`session-state/CHAT-HISTORY.md`** — what was *said* last context, in his exact words, generated
   by `tools/chat-history.py` from the real transcript. Read the CURRENT-CONTEXT entry **second**,
   right after this note.
   ⚠⚠ **AND REGENERATE IT ON EVERY BUILD — he must never have to ask whether you did.** He asked on
   2026-08-27 (*"are you saving chat history like you are supposed to"*), which is the question this
   mechanism exists to make unnecessary. `python3 tools/chat-history.py --title '<...>'`, then fill
   DECISIONS / SHIPPED / OPEN AT CLOSE by hand. **`test_chat_history.js` FAILS THE BUILD** when the
   stamped version does not equal `GPTS_VERSION`, so a forgotten regeneration goes red rather than
   quiet. Run it LAST, after the final exchange, or the tail of the session is missing from it.

⚠⚠ **A COMMIT IS NOT A PUSH, AND THIS BIT US AGAIN.** The 2026-08-27 session committed nine times in
the sandbox; the installer squashed them into one commit named `--help` and **three files never
reached GitHub**: `design/DATA-ARCHITECTURE.md`, the **284-session ES 1-min corpus**
(`data/es-1min/EPM26-1min.csv.gz`) and both mockup PNGs. Verify a push landed by cloning, not by
trusting the .bat.

You are picking up a long-running build with a trader who works **one item at a time** and expects
you to **discuss before building**. He will say "build" when he wants code. He has caught more real
defects than the test suite has. Treat his observations as data.

---

## 1 · THE STANDING BUSINESS REQUIREMENT (his words, verbatim, never paraphrase it away)

> "I am a trader and need to know where to take trades from and where price is going, so basically I
> need to know potential support and resistance especially if it is weakening and new support and
> resistance is forming as well as where price is going."

Every feature is judged against that sentence.

---

## 2 · WHERE WE ARE RIGHT NOW

**Shipped: v14.54.** Section ② is THE LADDER — the old horizontal rail, gamma profile and percentages
rotated 90° onto one vertical price axis. v14.54 re-laid it to `mockups/mockup-ladder-v11.html`, the
spec he approved, which was previously mockup-only.

Column order, his:
`levels · rolls · price · nodes ▸ | NOW | ◂ marker · tests · delta 15m $ · state · roc`

- **The NOW column is a walled chute** (224–290). Price, the three Kings and the expected-move edges
  live in it and **nothing else may enter**. `test_ladder` asserts it and the render audit checks it.
- **%King is left-justified INSIDE its node bar**; the type rides the tip. The parked column is gone.
- **The roll lane is LEFT of the prices** — both ends of an arrow meet the price column's own edge.
- **The delta profile is Skylit's delta15 in DOLLARS**, hanging left off a zero line at x=424.
  Green builds, red drains. ⚠ **MAGNITUDE, not polarity** — a draining negative node is becoming
  *less of anything*, not more negative. Polarity is the bar's colour and only the bar's colour.
- The SPXW crown carries a **test counter**, absent at zero. The **price pill takes the tested king's
  colour** within 2 points; STRETCHED still wins.
- **REVERSIBLE:** `CFG.ladder=false` restores the old rail, hidden not deleted.

**⚠⚠ THE ONE THING STILL OPEN ON THE LADDER, AND IT IS HIS CALL.** True width went **657 → 618**
(not 632 → 588 — `LAD_ROCW` was 56 for a column needing 84, which is the 24px LOCKED-ITEMS could
never account for). **618 does not fit his 454px panel body, and nothing will**: the approved mockup
is drawn on a **544px** panel. The container scrolls rather than clips, so nothing is lost. Options
and their costs are in `LOCKED-ITEMS.md`. **Do not close it by deleting a column.**

---

## 3 · THE LEVEL LIFECYCLE — settled with him, do not re-litigate

Three ORTHOGONAL facts, three slots. He rejected merging them and he was right.

- **STATE** (the level's own condition): `BUILDING → HOLDING → TURN UP/DN → WEAKENING → SPENT`
- **MARKER** (its relationship to price): `BREAKING · DEFENDING · ATTRACTING · ◂T`
- **COUNTER** (tests taken): `2×`, absent at zero

⚠ This is why the ladder is wide, and why abbreviating the STATE words to save 16px would reopen a
decision closed at v14.49.

**Checked against Skylit Academy doctrine (FRESH → TESTED → DELIVERED → DECAYING):**
- FRESH/TESTED/DELIVERED is the TAP axis = our counter, same ~80/66/33 probabilities.
- Their DECAYING = "weakens with NO interaction" = our WEAKENING **with 0 taps**. Finer, not coarser.
- ⚠ **SPENT IS NOT THEIR "DELIVERED".** Theirs is tap-exhaustion; ours is mass. Say so in hovers.

⚠ **A TAP COMPLETES WHEN PRICE LEAVES, NOT WHEN IT ARRIVES** — from his own chart.
⚠ **ATTRACTING REQUIRES EVIDENCE, NOT POTENTIAL.** Pull = size÷distance is geometry; ATTRACTING also
requires the distance to be CLOSING. ◂T survives for pull-without-evidence.
⚠ **BREAKING = the LEVEL being abandoned while tested. NOT a claim price broke through.**

---

## 4 · WHAT THE DATA ACTUALLY SAYS (the part most likely to be forgotten)

**Be sceptical of the panel's own headline numbers. Several are one-day samples and one contradicts.**

| claim | status |
|---|---|
| roll destinations held 74%, drained sources broke 19/19 | measured, small n, still the strongest thing we have |
| roll arrows as a 30-min direction signal: **4/12** | measured — arrows are STRUCTURE, not direction |
| magnet/attract 77% toward within 30m | **ONE day (n=47), and contradicted below** |
| nightly scorer, 2026-08-26 | **"no direction factor beat the tape (trend 34%, leg 34%, magnet reached 1 in 3)"** |
| Academy taps 80/66/33 | vendor doctrine, not our measurement |

**Measured live from 53 deflections on 2026-08-26:** Floor 11/15 = **73%** · Rug 8/11 = **73%** ·
Ceiling 6/14 = 43% · **King 3/8 = 38%** · Gate 0/3 = 0%. By test number: 1st 54% · 2nd 45% · 3rd 64%
· 4th+ 50% — **no decay visible**, contrary to the tap doctrine (n per bucket 11–18, so it does not
refute it). **The King being the WORST level on the board is the opposite of what the design
assumes.** One day, n=8. Needs more days before anyone acts on it.

⚠ **EVERY THRESHOLD IN THE STATE ENGINE IS HAND-SET, NOT MEASURED**, and labelled as such in source.

---

## 5 · THE DATA PIPELINE — verified working, but the FEATURE RECORDS ARE COLLAPSING

**The export fires.** `data/2026-08-27.json` was written at 15:30 CT with 133 SPY snaps, and
`nodeEvents` is **1,332 — non-zero for the first time ever**. 2026-08-26 was recovered (160 snaps),
so v14.51's empty-day bug is closed.

⚠⚠ **BUT THE PER-BAR FEATURE RECORDS HAVE ALL BUT STOPPED, AND NOBODY HAS EXPLAINED IT:**

| day | feat records |
|---|---|
| 08-19 | 3,132 |
| 08-20 | 3,822 |
| 08-21 | **0** |
| 08-24 | **0** |
| 08-25 | 945 |
| 08-26 | **2** |
| 08-27 | **15** |

**The usable study corpus is still 08-19, 08-20, 08-25 ≈ 7,900 records ≈ 790 effective observations.**
Every scorecard, every rate and every promotion path runs on `featRecordAll` / `featEnqueue`, so this
is upstream of all of it. **Diagnose this before running any study** — a study over a corpus that
stopped recording will produce confident numbers about three days in August.

---

## 6 · HOW TO WORK WITH HIM — the rules this project learned the hard way

1. **Discuss one item at a time. Do not build until he says build.**
2. **RENDER EVERY MOCKUP HEADLESS BEFORE SENDING IT**, capture `pageerror`, and run the **pairwise
   bounding-box OVERLAP AUDIT**. In v14.54 it caught two collisions no screenshot explained — one of
   them in the fix for the other. `/tmp/ladshot.js` style, `executablePath:'/opt/pw-browsers/chromium'`.
3a. ⚠ **AND TELL HIM TO CLICK THE LINK.** Tampermonkey's default update check is ONCE A DAY, so a
   fresh push does NOT reach his browser on its own in any useful time. The sequence is: run the
   `.bat` → wait ~5 min for the CDN → **click the Tampermonkey link** → reload Atlas. If the link
   says *Reinstall* that means auto-update already beat you to it and he HAS the build — it is not a
   failure. Diagnose by reading the running version off the panel, never by asking.
   (I told him the opposite on 2026-08-27 after seeing one lucky auto-update, and he sat on a build
   with a known bug for an hour.) See the FOUR INSTALL FAILURES table in PROJECT-CONSTANTS.
3. ⚠⚠ **DELIVER EXACTLY ONE FILE.** His words, 2026-08-15 and again 2026-08-27: *"you are supposed
   to just give me an install file."* One `installvNNNN.bat` (dash-free, dot-free — downloads strip
   both), plus the Tampermonkey links as text. **Not a zip+applier pair. Not "here are both". Not
   the installer plus a render plus a screenshot.** On 2026-08-27 the builder's own banner said
   "DELIVER THESE TWO FILES (primary)" while the skill said "ship ONE", and a context followed the
   banner. All three sources now agree and `test_delivery.js` fails the build if one drifts.
   The zip+applier is the FALLBACK — send it only if he reports the .bat failed.
4. **Bump BOTH version strings** — `// @version` and `var GPTS_VERSION`. `test_ladder` asserts they
   match. Also bump the four version pins: `test_direction_grade`, `test_pipeline_indicator`,
   `test_read_v1047`, `test_rules_v2`.
5. **One edit, one write, verify.** A multi-edit script that aborts writes NOTHING.
6. **Run the whole suite; 6 baseline reds are expected** (pre-v13.8). Anything else is yours.
   `test_chat_history` going red means you have not regenerated the history yet — that is the gate
   working, not a bug.
7. **MUTATE EVERY NEW ASSERTION AND CONFIRM IT FIRES.** v14.54 mutation-tested eleven.
8. **He is often right about things the tests pass on.**

---

## 6c · THE CLOSE-OF-SESSION BOOK (v14.55) — AND WHY IT WILL LOOK BROKEN TONIGHT

After the close Skylit drops the expired chain, so the ladder becomes TOMORROW's book with every
rate of change at zero — the panel goes FLAT, not blank. v14.55 latches the last healthy live SPXW
reading during RTH and serves it once the front expiry rolls, badged in the footer with the session
and the time it froze.

⚠⚠ **THE LATCH IS WRITTEN DURING RTH ONLY AND THERE HAS NEVER BEEN ONE.** Until a full RTH session
runs with v14.55 installed, `__gptsDebug.lastBook()` reports `no latch yet` and the panel behaves
exactly as before. **That is correct, not a failure.** The first time it can engage is after the
next close. Do not "fix" it before then.

⚠ **THE RECORDER IS BLIND TO IT BY CONSTRUCTION.** Nine write paths call `recorderBlind()` =
`inReplay() || showingStaleBook()`. **A tenth write path means calling `recorderBlind()`** — never
`inReplay()` directly. `test_lastbook.js` scans for strays and goes red on one.
⚠ `pickSessionDay` was NEVER the problem and must not be loosened: it answers which day's PRICE BARS
to draw and it answers correctly.

## 6e · ⓪a DAY — HOD/LOD IS BUILT (v14.57), AND WHAT IT STILL NEEDS

Spec: **`mockuphodlodv2.html` in the REPO ROOT** — the approved design, and it was never lost;
two earlier notes said it was and both were wrong (failure pattern #4, not searching the root).

**The rates are measured, not copied from the mockup.** `tools/study-hodlod.py` over
`data/es-1min/EPM26-1min.csv.gz` — 284 complete RTH sessions — re-derives every figure and lands on
the mockup independently. ⚠ **`MIN_BARS=386` is load-bearing**: >=386 gives 284 sessions, >=391 gives
283, and one 386-bar session is the entire difference.

⚠⚠ **THE CORPUS WAS SUPPLIED TWICE.** The first copy was committed in a sandbox at `a26cdfd` and
never pushed. **The cloud cannot push; a sandbox commit is not a push.** It is 5.1MB gzipped against
a 6MB installer payload cap, so it CANNOT ride the .bat — it has to sit at
`C:\Dev\gex-signal-tapereader\data\es-1min\` on his machine and go up with his next run.
**Confirm it landed on GitHub before trusting anything downstream of it.**

**Still owed on this section, in order:**
1. **FEATURE ENROLLMENT.** ⓪a records nothing and is scored by nothing. The 2026-08-17 mandate says
   no feature ships un-enrolled — DATA, ANALYSIS, TESTING. It needs a `FEATURES` entry.
2. **BOP · WICK · W.END · WICK% · MUD** — his definitions. They exist nowhere, so they are printed as
   PENDING rather than invented. Do not guess them.
3. **VWAP** — one of the five chips, and the codebase has none. Renders UNAVAILABLE, never a ✗.

## 6d · v14.56 IS BUILT AND NOT DELIVERED

⚠ **He is running v14.55; the repo is v14.56.** The difference is one fix: the chute nudge. v14.54
moved the EM edges and the three crowns into the same column but left the nudge comparing crowns
against crowns only, so `EL 7708` and `~7716 QQQ` overlapped. Seeding the nudge list with the EM rows
fixes it. Cosmetic, not data. **Ask before shipping it on its own — he asked whether to bundle it.**

## 7 · WHAT TO DO NEXT, IN ORDER

1. **Deliver v14.56, or bundle it — his call, already asked.** Then look at the ladder. Then the width decision in `LOCKED-ITEMS.md` — most
   likely just widening the panel to ~620, which is one drag.
2. **DIAGNOSE THE FEATURE-RECORD COLLAPSE (§5).** Nothing statistical is trustworthy until this is
   understood. It is now ahead of the study in priority, because it decides what the study can say.
3. **BUILD ITEM 18 — the Yahoo pipeline.** ⚠ **Deadline 2026-09-16**: Yahoo serves intraday ≤60 days
   and the corpus gap starts 07-18. Full spec: `session-state/YAHOO-PIPELINE.md`. The browser
   fetches, `snap.htf` rides the day-file export, git carries it to the cloud. ⚠ `@grant
   GM_xmlhttpRequest` **cannot** go in the panel — it belongs in the IF companion, which already
   couriers ForexFactory at v1.14.
4. **Re-push the three files that never landed** (§0): `DATA-ARCHITECTURE.md`, the ES corpus, the
   mockup PNGs. The ES corpus is the evidence base for the whole HOD/LOD feature.
5. **Run the study** over whatever corpus survives step 2.
6. **The HOD/LOD section** — `mockups/hodlod-v2-SPEC.md` is the approved design. ⚠ It needs **IB60**
   and `sessionLevels()` computes IB30 only.

---

## 8 · POINTERS

- **`session-state/LOCKED-ITEMS.md`** — agreed work that is NOT built. Check every build.
- **`session-state/YAHOO-PIPELINE.md`** — item 18, recovered in full.
- `mockups/mockup-ladder-v11.html` — the ladder spec, now BUILT as of v14.54.
- `mockups/ladder-v1454-RENDERED.png` — the v14.54 render + overlap audit output.
- `mockups/hodlod-v2-SPEC.md` — the approved HOD/LOD section, transcribed from the screenshot.
- `session-state/SKYLIT-FEEDS.md` — the FULL APPLICATION MAP, every feed, the four capture rules
  (**RTH · READ AS %King · VELOCITY All · LOW NODES never Hide**), and the DARK POOL endpoint.
- `session-state/PROJECT-CONSTANTS.md` — the landmines and his standing rules. CQG symbology
  (ES = EPU26, NQ = ENQU26), scales, thresholds.
- `session-state/DECISIONS.md` — D-1..D-16, why the panel is the way it is.
- `session-state/INSIDERFINANCE.md` — the second book.
- `session-state/ISSUES-NEXT-BUILD.md` — the open ledger. ⚠ Item 5, the EM band clipping the piles
  to ~5 ES points, is still open and is the largest live defect in that file.
- ⚠ **MISSING AT HEAD:** `skylit-docs/FINDINGS.md`, `BUILD-PLAN.md`, `garma/V2-PHASE-PLAN.md`,
  `wicks/`, `design/DATA-ARCHITECTURE.md`, `data/es-1min/`. `FINDINGS.md` is quoted by name in three
  live hovers, so no context can currently answer "is this OPEN or CONFIRMED" for anything it covered.

## 9 · DOCTRINE THAT MUST NOT BE LOST

- **Two books, never averaged.** Skylit = FLOW (|net|≡v, no call/put split). InsiderFinance = OI×gamma.
  Disagreement is displayed, never reconciled.
- **We store their numbers; we do not invent our own.** Skylit's velocity objects ARE our rate of
  change — that makes their strike popup a free test oracle.
- **Name both units out loud before comparing two numbers.** `kingKd` is THOUSANDS; `velocity.cur`
  and `.d15` are DOLLARS — measured 2026-08-27 on SPXW 7690 (12680 vs −12,680,083).
- **Absence of data is not a reading.** Refuse and say why; never guess a number.
- **A clamped position is a false position.** Off-frame levels leave the drawing and are disclosed.
  ⚠ v14.54 extends this to LABELS: one that fits nowhere legal is DROPPED, never shoved.
- **Gamma tells you HOW price moves, never WHICH WAY.** The read never says likely/will/should.
- **Anything unproven ships labelled unproven and scored nightly.**
