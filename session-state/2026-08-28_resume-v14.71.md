# RESUME NOTE — read this before anything else
_written 2026-08-28 · panel v14.71 · companion v1.15 · supersedes every earlier resume note_

---

## ⚠ 0 · THE CARRY-FORWARD CONTRACT, AND HOW IT JUST FAILED

**This note is rewritten IN FULL every build. Anything not re-typed is GONE, silently.**

⚠⚠ **IT WENT SEVEN BUILDS STALE IN THE SESSION THAT WROTE THIS.** v14.60 through v14.66 shipped —
the entire LOD/HOD feature — while this file still said v14.59 and contained zero mentions of
`lodhod`, `HLTAB` or `posr`. `CHAT-HISTORY.md`, `CHANGELOG.md` and `FINDINGS.md` were updated every
build; this one was not, and **the operator had to ask.** A fresh context would have rebuilt the
model from scratch.
**THE RULE: this note is updated in the SAME COMMIT as the build, exactly like the chat history.**
There is no test enforcing it — `test_chat_history.js` guards the history only — so it depends on
the checklist being followed. Consider adding one.

1. **`session-state/LOCKED-ITEMS.md`** — agreed-but-unbuilt work. Check every build.
2. **THE LOAD CLONES FULL** (never `--depth 1`), and **searches the right string**: use
   `git grep -li "<bare word>" $(git rev-list --all)`. Twice this session a narrow pattern produced
   a confident "it does not exist" that was wrong.
3. **`session-state/CHAT-HISTORY.md`** — what was *said*. Read second. Regenerated every build.
4. **`design/DATA-ARCHITECTURE.md`** — who can reach what.
5. **`skylit-docs/FINDINGS.md`** — F-1..F-8, our measurements. **It exists now** (created 2026-08-28
   after never having existed in any commit).

⚠⚠ **A COMMIT IS NOT A PUSH.** The cloud gets a 403 from the git proxy — verified again this
session. `installvNNNN.bat`, run on his machine, is the only route to GitHub.

He works **one item at a time** and expects you to **discuss before building**. He has caught more
real defects than the test suite has. When he pushes back, he is usually right.

---

## 0b · ⚠⚠ THE SAVE-DONE RULE (operator-mandated 2026-08-28)

> "after you give me a build, i want a confirmation something like a checkmark save done for future
> context. this tells me that you have updated the chat history and any relevant files that a future
> context would need to proceed if this context was closed."

**Every build ends with an explicit `✅ SAVE DONE` line** naming what was updated. It is not a
courtesy — it is the receipt that the handoff chain is current.

⚠ **`test_savedone.js` ENFORCES IT.** It fails the build when `latest-resume-note.md` does not
declare the current panel version, when CHAT-HISTORY or the CHANGELOG are not stamped, when a ledger
is missing, or when this convention stops being written down. **The resume note went SEVEN builds
stale on 2026-08-28 and then FOUR more the same day, while CHAT-HISTORY stayed perfect — because a
test guarded one and nothing guarded the other.** A rule enforced by a test is followed; a rule
enforced by a checklist is followed until it is busy.

---

## 1 · THE STANDING BUSINESS REQUIREMENT (his words, never paraphrase it away)

> "I am a trader and need to know where to take trades from and where price is going, so basically I
> need to know potential support and resistance especially if it is weakening and new support and
> resistance is forming as well as where price is going."

And for ⓪a specifically, 2026-08-28:

> "i just want to know with high accuracy whether the hod or lod have occurred ... I want to know it
> as early as possible."

---

## 2 · WHERE WE ARE — v14.66

**⓪a answers the LOD/HOD question with a measured table.** The face reads:

    LOD IN — 88%   (travelled 91% off it · n=2396) · toward HOD  typically ~13:33 (11:51–14:47)

Three states: **IN** (cell ≥70%), **STANDING**, **NOT IN** (cell ≤20%).

**THE ENGINE IS AN 8×9 LOOKUP TABLE, NOT A MODEL, AND THAT WAS A DELIBERATE DOWNGRADE.**
A 5-feature logistic was built first and measured against a plain two-axis lookup:
`AUC 0.8795 vs 0.8787, IDENTICAL Brier 0.1321`. The regression was ceremony. The table ships because
every cell carries its own **n**, it can be read and argued with, and drift shows as a cell that
stops matching. ⚠ A third axis (`stood`) made it **worse**. See **FINDINGS F-4**.

**The two axes:** `posr` (how far price has travelled off the extreme ÷ running session range) and
minutes since the open. `posr` alone scores **AUC 0.829** — better than the whole clock baseline.

**Measured decision rates** (first crossing, one row per session, FIRST-printed extreme):
- **IN at ≥70%: 94% correct, median 09:55, n=284**, far side still ahead on 97%
- **NOT IN at ≤20%: 72%, median 09:45, n=85** vs a ~57% base — real, early, thin

⚠ **DO NOT QUOTE 76% FOR THE IN CALL OR 93% FOR THE NOT-IN CALL.** Both came from a sample that
pooled BOTH sides at every bar, which also asks "is the SECOND extreme in" — a different, harder
question the panel never puts. **Measure the question the face actually asks.** (F-8)

---

## 3 · WHAT WAS MEASURED AND REJECTED — do not re-add these

| candidate | AUC alone | verdict |
|---|---|---|
| `posr` distance travelled | **0.829** | **the model** |
| `stood` | 0.818 | in the clock baseline |
| RSI level | 0.798 | redundant with posr |
| 50-SMA | 0.713 | proxy for posr |
| open reclaimed | 0.709 | proxy |
| IB30 / IB60 | 0.684 / 0.655 | **crude switches approximating posr** |
| sweep + reclaim | 0.559 | **48% standalone — BELOW its own base rate** |
| momentum divergence | 0.545 | **−0.0004 AUC. His hypothesis, answered no** |
| NQ cross-market divergence | — | **−0.0014 AUC. Tested on 163 overlapping sessions** |
| 60-minute breakout | 0.501 | a coin flip |

⚠ **THE CHIP ROW WAS DELETED FOR THIS REASON** (v14.65). Showing IB60/OPEN/POS beside the table was
**the same evidence twice, dressed as independent agreement** — POS was literally `posr` thresholded
at 0.5. VWAP does not exist in this codebase.
⚠ **BOTH DIVERGENCES ONLY LOOK ANTI-PREDICTIVE.** They can fire only at the instant a fresh extreme
prints — which is exactly `posr≈0`, a state the table already reads. (F-3)

**THE TABLE TRANSFERS TO NQ** (F-7): ES table on NQ data **AUC 0.8877** vs NQ's own table 0.8853.
64 comparable cells, mean gap 4.5 points. It measures session structure, not something about ES —
so **one table serves both markets**.

**RED/GREEN DAYS: TESTED AND REFUSED** (F-6). Against the correct baseline — "is price above the
open", already 83% — nothing moves accuracy at any hour. **Do not build it.**

---

## 4 · THE WICK FAMILY — his definitions, confirmed on the tape (v14.60–14.62)

- **TOOK** open → first extremity · **W.END** the first bar to **CLOSE** back through the session
  open (⚠ close, not touch) · **BOP** = "Back to Open", extremity → W.END · **WICK** = TOOK + BOP ·
  **WICK%** = |open − extremity| ÷ range (**a PRICE ratio**) · **MUD** = "MarkUp/MarkDown", W.END →
  the second extremity.
- Verified against his own panel on 2026-08-27: Wick% 26 vs 26, W.End 8:42 vs 8:42 **exactly**.
- ⚠ **EVERY E-ROW FIELD IS A TRIMMED MEAN** (Tukey 1.5×IQR), not a median — *"i thought they were
  all averages."* One row, one statistic. `s1`–`s7` pin it. p25/p75 stay true percentiles.
- ⚠ Zero wick **prints 0** and leaves the medians; never reclaiming the open is **null, not 0**.

---

## 4c · WHAT SHIPPED 2026-08-28 AFTER THE STORAGE FIND (v14.68 → v14.70)

- **v14.68** `__gptsDebug.storage()` — total, cap, headroom, top keys and a REAL 40 KB write probe.
  ⚠ The bounded-write FIX is **not** built: it is parked at
  `session-state/pending/v14.68-bounded-writes.patch` because he installs during live sessions.
- **v14.69** ⓪a's static half renders **without candles**. The old `if(!D.ok) return <one line>` hid
  the base rates, the ladder, the provenance and the table whenever there were no bars — i.e. every
  pre-open minute. The A row still refuses, verbatim.
- **v14.70** THE TABLE NOW COVERS THE WHOLE SESSION (**F-11**). It had skipped the first 60 minutes
  because the study required IB60 to exist first; IB60 was measured worthless and dropped, and the
  exclusion stayed behind. Re-derived from minute 5: **72/72 cells**, and AUC on the SAME late rows
  is IDENTICAL (0.8787) — strictly additive.
  ⚠ **THE NOT-IN CALL LIVED IN THE MISSING CELLS: 72% n=85 @09:45 → 85% n=230 @08:40.**
  ⚠ **DO NOT QUOTE the older figures** (94/09:55 IN, 72/09:45 NOT-IN) — they belong to the 64-cell
  table. Current: **IN 92% n=284 @09:35 · NOT IN 85% n=230 @08:40**.

⚠ **VERIFIED WORKING LIVE 2026-08-28:** the IRT export (`how:file`, `inPlace:true`, `err:null`, both
Kings on EPU26) and the Yahoo corpus tap (ES/NQ/GC/CL ~1,900 bars each). ⚠ The IRT export carries
only TWO rows because `emPiles` has no band anchor — same F-10 chain; expect it to fill once bars
anchor the band.

---

## 4b · ⚠⚠ THE ONE THING THAT EXPLAINED FIVE BUGS — localStorage WAS FULL (2026-08-28)

    localStorage      10,240 KB = exactly Chrome's 10 MB cap
    gpts_recorder_v7   5,957 KB  (ONE day)
    gpts_nodeevents_v1 3,228 KB
    40 KB write probe  QuotaExceededError

**Every `setItem` was failing behind `catch(e){}`.** That is the feature-record collapse (15 records
across 1 bar against 133 snapshots), the corpus tap "never running", the base-rate courier never
arriving, and IF levels 8.5 hours stale. **One fault, five symptoms**, chased for two sessions.

⚠ Cleared by hand; everything came alive within 12 seconds. **It refills at ~6 MB/day — the fix is
NOT built.** See FINDINGS F-10 and LOCKED-ITEMS.

⚠ **TWO LESSONS WORTH MORE THAN THE FIX:**
- **A diagnostic created its own evidence.** Deleting two keys to test "is the companion alive"
  freed exactly enough room for the tiny calendar object to write — which looked like proof the
  script ran and only Yahoo was broken. **Check the quota before concluding from a storage probe.**
- **The v14.67 instrument was aimed at the wrong layer.** `FEATH` counters asked registry-vs-dedupe.
  Neither: the records were built correctly every bar and discarded at the final `setItem`.
  **Measure the cheapest thing first.**

## 5 · WHAT TO DO NEXT, IN ORDER

1. ⚠⚠ **HE MUST CLICK BOTH TAMPERMONKEY LINKS.** He has **never** updated the companion — it is
   still v1.14 on his machine, so the Yahoo corpus tap has **never run once**. Panel v14.66 AND
   companion v1.15. Diagnose by reading the running version off the panel, never by asking.
   ⚠ `release-links.sh` marks changed-vs-**origin**; what matters is changed-vs-**his browser**.
2. **CONFIRM THE TAP RAN.** `__gptsDebug.futBars()`, then his push, then
   `python3 tools/append-futures.py`. `data/futures/` is still empty and no day file carries
   `futBars`.
3. ⚠ **THE HIGHEST-VALUE UNBUILT IDEA: condition the table on the GAMMA side.** Does price sitting
   on the King, or a low printing at a put wall, change the probability? **That is what would make
   this feature belong to THIS panel rather than being generic price structure any charting package
   could compute.** BLOCKED — see 4.
4. **DIAGNOSE THE FEATURE-RECORD COLLAPSE. This is the top priority, not housekeeping.**
   3,822 records on 08-20 against **15** on 08-27; `matrix` rows track exactly (108→3132, 122→3822,
   0, 0, 23→990, 2→8, 1→15) against 133 SPY snaps. **One upstream gate, not 46 features failing.**
   It blocks item 3 and the forward test of the table.
5. **The ladder width** — 618px in a 454px body, still his call. `LOCKED-ITEMS.md`.
6. **ITEM 18 is HALF built** — the Yahoo route and bar feed exist; Tier 1/2 (`snap.htf`, DMAs, ATR,
   HTF levels) do not. ⚠ **The 2026-09-16 backfill deadline for 07-18→08-14 is untouched.**
7. Untested and cheap: **overnight/globex context** — the corpus has ETH bars we filter out.

---

## 6 · HOW TO WORK WITH HIM

1. **ONE AT A TIME.** State one item, its fix, ask, STOP. The tell of the failure: three headings,
   a table, and "which do you prefer" at the bottom.
2. **Do not build until he says build.** He says it plainly.
3. **TEST BEFORE YOU BUILD.** On 2026-08-28 "test everything then one build" changed the build twice
   and caught two wrong numbers I had already told him.
4. **RENDER EVERY MOCKUP HEADLESS FIRST** with the pairwise overlap audit.
5. ⚠⚠ **DELIVER EXACTLY ONE FILE.** His words: *"you are supposed to just give me an install file."*
   One `installvNNNN.bat` (dash-free, dot-free) **plus the Tampermonkey links as text**.
6. ⚠ **AND TELL HIM TO CLICK THE LINKS** — Tampermonkey's default check is once a day.
7. **Bump BOTH version strings** and the four test pins (`test_direction_grade`,
   `test_pipeline_indicator`, `test_read_v1047`, `test_rules_v2`). Rule count pin lives in
   `test_rules_v2` (**74** now).
8. **One edit, one write, verify.** A multi-edit script that aborts writes NOTHING.
9. **Run the whole suite; 6 baseline reds are expected.**
10. **MUTATE EVERY NEW ASSERTION INDIVIDUALLY.** Twice this session a guard could not fail.
11. **VERIFY THE INSTALLER BY DECODING IT.** It has silently dropped `design/`, `skylit-docs/` and
    `tools/fixtures/` — each caught only by decoding the `.bat` before sending.
12. **He is often right about things the tests pass on.**

---

## 7 · POINTERS

- **`skylit-docs/FINDINGS.md`** — F-1 the rule study · F-2 the model · F-3 divergences dead ·
  **F-4 the model does not earn its complexity, ship the table** · F-5 there is a trade left when it
  fires · F-6 red/green refused · F-7 it transfers to NQ · F-8 measure the question the face asks.
- `design/DATA-ARCHITECTURE.md` · `session-state/LOCKED-ITEMS.md` · `INSIDERFINANCE.md` ·
  `SKYLIT-FEEDS.md` (RTH · READ AS %King · VELOCITY All · LOW NODES never Hide) ·
  `PROJECT-CONSTANTS.md` · `DECISIONS.md` D-1..D-16 · `ISSUES-NEXT-BUILD.md`.
- Studies, all reproducible: `tools/study-lodhod.py` · `model-lodhod.py` · `sanity-lodhod.py` ·
  `study-travel.py` · `study-redgreen.py` · `study-transfer.py` · `study-notin.py` ·
  `study-hodlod.py` · `append-futures.py`.
- **Corpora are ON GitHub** — `data/es-1min/ES TestingData.txt` (284 sessions) and
  `NQ TestingData.txt` (188). ⚠ The tooling once looked for `EPM26-1min.csv.gz` and reported the
  corpus missing; **he supplied it twice because of that error.**

## 8 · DOCTRINE THAT MUST NOT BE LOST

- **Two books, never averaged.** Skylit = FLOW. InsiderFinance = OI×gamma.
- **Name both units out loud before comparing two numbers.** `kingKd` thousands, `velocity` dollars.
- **Absence of data is not a reading.** Thin cells refuse; they do not guess.
- **A well-formed number is not a supported one.** Monotone ≠ evidence.
- **Measure against the right baseline** — the clock for LOD/HOD, the open's sign for red/green.
  Against 50% everything looks brilliant.
- **Gamma tells you HOW price moves, never WHICH WAY.**
- **Anything unproven ships labelled unproven and scored nightly.**
