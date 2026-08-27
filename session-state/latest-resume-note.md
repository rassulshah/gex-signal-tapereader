# RESUME NOTE — read this before anything else
_written 2026-08-27 · panel v14.51 · supersedes every earlier resume note_

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

**Shipped and live: v14.51.** The panel's section ② is now **THE LADDER** — the old horizontal rail,
gamma profile and percentages section rotated 90° onto one vertical price axis.

**Why the rotation happened** (he caught it): the horizontal rail drew 15 posts and could label only
9, because two neighbouring strikes sit ~23px apart while a two-line label is ~22px wide. Vertically
the same strikes are ~15px apart and a label is ~9px TALL. The crowding does not get managed, it
stops existing.

**Column order is his, and each fact has exactly one home** (7692 used to appear three times):
`levels · rolls · price · nodes ▸ | NOW | ◂ delta · state · tests · roc`

- **The NOW column is a walled chute.** Price, the three Kings, and the expected-move edges live in
  it and NOTHING else may enter. That is what makes "price gets overlapped" structurally impossible
  rather than merely unlikely, and `test_ladder` asserts the arithmetic.
- Nodes reach INWARD toward price; bar length = %King; the type rides the bar; ACC/BRK are NOT drawn
  because the bar's colour already says polarity (purple = negative/accelerator, yellow = positive/brake).
- The delta profile is **DOLLARS, not percent** — the ROC column beside it is already percentages, and
  dollars let a roll be checked by eye (source −$4M, destination +$4M).
- Roll arrows: circle at the SOURCE, out, along, back in past the origin's x, then the head. The
  landing is a SEPARATE SOLID sub-path — a dashed stroke ends wherever the pattern falls and left the
  head floating.
- **REVERSIBLE:** `CFG.ladder=false` restores the old rail, which is hidden not deleted for one release.

**⚠ THE LADDER DISPLAY IS PARTLY MOCKUP-ONLY.** `mockups/mockup-ladder-v11.html` is the AGREED spec
and is ahead of the code. Still to build from it: delta profile in dollars, rolls moved left of the
prices, kings in the chute with test counters, %King left-justified on the bar, EH/EL as pills in the
chute. **Render every mockup headless before sending it** (see §6).

---

## 3 · THE LEVEL LIFECYCLE — settled with him, do not re-litigate

Three ORTHOGONAL facts, three slots. He rejected merging them and he was right.

- **STATE** (the level's own condition): `BUILDING → HOLDING → TURN UP/DN → WEAKENING → SPENT`
- **MARKER** (its relationship to price): `BREAKING · DEFENDING · ATTRACTING · ◂T`
- **COUNTER** (tests taken): `2×`, absent at zero

**Checked against Skylit Academy doctrine (FRESH → TESTED → DELIVERED → DECAYING):**
- FRESH/TESTED/DELIVERED is the TAP axis = our counter, same ~80/66/33 probabilities.
- Their DECAYING = "weakens with NO interaction" = our WEAKENING **with 0 taps**. Finer, not coarser.
- ⚠ **SPENT IS NOT THEIR "DELIVERED".** Theirs is tap-exhaustion; ours is mass. Say so in hovers.
- Their HALO fires when multi-window rates AGREE; TURN fires when they DISAGREE.

**⚠ A TAP COMPLETES WHEN PRICE LEAVES, NOT WHEN IT ARRIVES.** From his own chart: a level held its
first test then failed its second, and the badge must read **1×** *during* that second test, because
one prior test is the fact you decide on. Counting on contact would report a test you are inside.

**⚠ ATTRACTING REQUIRES EVIDENCE, NOT POTENTIAL.** Pull = size÷distance is geometry; a node can hold
top pull all session while price walks away. ATTRACTING additionally requires the distance to be
CLOSING. ◂T survives for pull-without-evidence.

**⚠ BREAKING = the LEVEL being abandoned while tested. NOT a claim price broke through.**

---

## 4 · WHAT THE DATA ACTUALLY SAYS (this is the part most likely to be forgotten)

**Be sceptical of the panel's own headline numbers. Several are one-day samples and one contradicts.**

| claim | status |
|---|---|
| roll destinations held 74%, drained sources broke 19/19 | measured, small n, still the strongest thing we have |
| roll arrows as a 30-min direction signal: **4/12** | measured — arrows are STRUCTURE, not direction |
| magnet/attract 77% toward within 30m | **ONE day (n=47), and contradicted below** |
| nightly scorer, 2026-08-26 | **"no direction factor beat the tape (trend 34%, leg 34%, magnet reached 1 in 3)"** |
| Academy taps 80/66/33 | vendor doctrine, not our measurement |

**Measured live from 53 deflections on 2026-08-26 (pulled from localStorage, never exported):**

| level kind | deflection continued |
|---|---|
| Floor | 11/15 = **73%** |
| Rug | 8/11 = **73%** |
| Ceiling | 6/14 = 43% |
| **King** | **3/8 = 38%** |
| Gate | 0/3 = 0% |

By test number: 1st 54% · 2nd 45% · 3rd 64% · 4th+ 50% — **no decay visible**, contrary to the tap
doctrine. n per bucket is 11–18, so this does not refute it; it does mean test-count was not what
separated that day's reversals. **The King being the WORST level on the board (38%) is the opposite of
what the whole design assumes.** One day, n=8. It needs more days before anyone acts on it.

**⚠ EVERY THRESHOLD IN THE STATE ENGINE IS HAND-SET, NOT MEASURED**, and labelled as such in source.

---

## 5 · THE DATA PIPELINE, AND THE BUG THAT WAS LOSING SESSIONS

**v14.51 fixed this — verify it is working before trusting any study.** `buildDayExport` used
`dateKey || TODAY`, so an export run before the open wrote an EMPTY file stamped with that date, and
nothing overwrote it afterwards. `data/2026-08-26.json` in the repo is such a blank while the real
182 snapshots and 59 deflections sat unexported in localStorage. **An empty file is indistinguishable
from "nothing happened".** Now: no explicit date → export the most recent day WITH DATA; and an empty
day is REFUSED loudly by both export paths.

**Corpus:** `data/2026-08-17 … 08-26.json`, ~950 recorded bars over 7 sessions, per-bar feature
vectors. `recorderLoad()` keeps 10 rolling days in localStorage; anything not exported is lost.

**FIRST JOB NEXT SESSION:** confirm the export fires, recover 2026-08-26 from localStorage
(`__gptsDebug.saveDayToFile('2026-08-26')`), then run the study in §7.

---

## 6 · HOW TO WORK WITH HIM — the rules this session learned the hard way

1. **Discuss one item at a time. Do not build until he says build.**
2. **RENDER EVERY MOCKUP HEADLESS BEFORE SENDING IT.** Nine mockups went out unrendered; one had a
   JS syntax error and displayed an empty frame. Use `/tmp/shot.js`-style Playwright
   (`executablePath:'/opt/pw-browsers/chromium'`), capture `pageerror`, and run an **overlap audit**
   (pairwise getBoundingClientRect intersection) — it has caught four collisions no screenshot
   explained.
3. **Deliver ONE install file per build, dash-free name `installvNNNN.bat`, plus the Tampermonkey
   links, EVERY time.** Downloads strip dashes AND dots.
4. **Bump BOTH version strings.** `// @version` in the header AND `var GPTS_VERSION`. They drifted for
   six releases and the footer lied about every install while I kept telling him to check it.
   `test_ladder` now asserts they match.
5. **Multi-edit Python scripts that `sys.exit()` on a failed assert WRITE NOTHING** — earlier
   successful edits in the same script are silently lost. One edit, one write, verify. Also: the file
   contains LITERAL `—`, `÷`, `◂` characters, not `\u` escapes; match them literally.
6. **Run the whole suite; 6 baseline reds are expected** (pre-v13.8). Anything else is yours.
7. **He is often right about things the tests pass on.** "There are more flags than strikes", "the
   gray outline is too symmetrical", "current price is in two columns" were all real defects.

---

## 6b · OPEN, AWAITING LIVE VERIFICATION (2026-08-27)

**The IRT FlexLevels CSV write (v14.52).** The operator reported the local file only showed lines
after a manual refresh. Diagnosed as `createWritable()` defaulting to an ATOMIC REPLACE in Chromium —
a swap file renamed over the original, so the file IDENTITY changed on every export and IRT, which
opens the file once and polls it, was left holding an orphan. Fixed by writing in place
(`keepExistingData:true`, write at position 0, truncate to byte length).
**⚠ THIS IS A HYPOTHESIS UNTIL HE WATCHES IT TOMORROW.** Ask him. If the lines still need a refresh,
the fallback is a local HTTP server (`python -m http.server 8000` in that folder, then IRT's "Remote
File" → `http://localhost:8000/FlexLevelsExport.csv`, ideally with `Cache-Control: no-store`).
**GitHub raw is NOT an option** — CDN-cached about 5 minutes against a 1-minute poll, and it would
need a push per export. `__gptsDebug.irt()` reports `IRT_LAST.inPlace` so you can see which write
path actually ran.

## 7 · WHAT TO DO NEXT, IN ORDER

1. **Verify the export fix and recover 2026-08-26.** Nothing below is trustworthy until the pipeline is.
2. **Run the study over the 7 day-files** — this was agreed as the highest-value next step, ahead of
   any new feature. Questions: does FORMING/BUILDING hold more than HOLDING? is 19/19 real at n>19?
   is the King really a 38% level? does the magnet reach at any distance? Report rates with n.
3. **Absorption detector** — a level tested *while gaining mass* is being defended with real money. We
   measure both halves (tap counter, 15m delta) and never combine them. Test it on the 7 days first.
4. **Sweep-and-reclaim (Wicks W2/W7)** — the trigger layer we do not have, and the one thing gamma
   structurally cannot provide.
5. Finish the ladder display from `mockups/mockup-ladder-v11.html` (§2).
6. Then `BUILD-PLAN.md` order: Garma Phase C + Wicks W7 together, W1/W2, Phase D, W3/W5.

---

## 8 · POINTERS

- `BUILD-PLAN.md` — the master sequence across BOTH analysts, with the 46-rule Wicks overlap audit.
- `garma/V2-PHASE-PLAN.md` — Garma V2 (59 rules) gap audit.
- `garma/claude_package_v2/`, `wicks/` — the two analyst packages.
- `session-state/SKYLIT-FEEDS.md` — the FULL APPLICATION MAP, every feed, the four capture rules, and
  the **DARK POOL endpoint** (`/fs/api/dark-pool/top-prints?ticker=SPY&top_n=3&lookback_days=45`).
- `session-state/PROJECT-CONSTANTS.md` — CQG symbology (ES = EPU26, NQ = ENQU26), scales, thresholds.
- `session-state/ISSUES-NEXT-BUILD.md` — the open ledger.
- `mockups/mockup-ladder-v11.html` — the agreed ladder spec, ahead of the code.
- `changelog/CHANGELOG.md` — every version, with the reasoning, newest first.

## 9 · DOCTRINE THAT MUST NOT BE LOST

- **Two books, never averaged.** Skylit = FLOW (|net|≡v, no call/put split). InsiderFinance = OI×gamma.
  Disagreement is displayed, never reconciled.
- **We store their numbers; we do not invent our own.** Skylit's velocity objects ARE our rate of
  change — that makes their strike popup a free test oracle.
- **Absence of data is not a reading.** Refuse and say why; never guess a number.
- **A clamped position is a false position.** Off-frame levels leave the drawing and are disclosed.
- **Gamma tells you HOW price moves, never WHICH WAY.** The read never says likely/will/should.
- **Anything unproven ships labelled unproven and scored nightly.**
