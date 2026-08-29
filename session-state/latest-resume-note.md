# RESUME NOTE — read this before anything else
_written 2026-08-28 · panel v14.90 · companion v1.16 · supersedes every earlier resume note_

---

## ⚠ 0 · THE CARRY-FORWARD CONTRACT

**This note is rewritten IN FULL every build. Anything not re-typed is GONE, silently.**
It went SEVEN builds stale on 2026-08-28 and then FOUR more the same day; `test_savedone.js` now
fails the build when it does not declare the current panel version. Update it in the SAME COMMIT.

0. ⚠⚠ **`session-state/LESSONS.md`** — READ IT FIRST AND IN FULL. Section 1 is the failure-pattern
   register (nearly every bug here is one of them); section 2 is the per-build log and it names
   results that have been **WITHDRAWN**. Quoting a withdrawn number is the most expensive thing a
   fresh context can do, because it looks like knowledge. `test_lessons.js` keeps it current.
1. **`session-state/LOCKED-ITEMS.md`** — agreed-but-unbuilt work. Check every build.
2. **THE LOAD CLONES FULL** (never `--depth 1`).
3. **`session-state/CHAT-HISTORY.md`** — what was *said*. Read second.
4. **`design/DATA-ARCHITECTURE.md`** — who can reach what.
5. **`skylit-docs/FINDINGS.md`** — F-1…**F-16**. Read F-12 through F-16 before touching ⓪a.

⚠⚠ **A COMMIT IS NOT A PUSH.** The cloud gets a 403 from the git proxy. `installvNNNN.bat`, run on
his machine, is the only route to GitHub.

He works **one item at a time** and expects you to **discuss before building**. He has caught more
real defects than the test suite has. **When he pushes back, he is usually right** — on 2026-08-28
he asked "did you do your best" about a model I had already concluded on, and the honest answer was
no; re-posing the question turned an AUC of 0.53 into 0.83.

---

## 1 · THE STANDING BUSINESS REQUIREMENT (his words)

> "I am a trader and need to know where to take trades from and where price is going, so basically I
> need to know potential support and resistance especially if it is weakening and new support and
> resistance is forming as well as where price is going."

And for ⓪a, 2026-08-28:

> "i just want to know with high accuracy whether the hod or lod have occurred … I want to know it as
> early as possible" → then → "the reads focus should be on determining if the first extremity is in
> with the %, you do this now, and then predicting things like the opposite extremity and when it
> will be reached. And if we are on track to reach it or not."

> "i want this feature to be a top priority, its ok for now but it must be able to improve as data is
> gathered and insights are given by the nightly review and those changes are incorporated."

---

## 2 · WHERE WE ARE — v14.80, and what ⓪a now says

    LOD IN — 89%            (travelled 93% off it · n=430)
    HOD not before 12:00 — 80% · most likely 12:00–14:30 (50%) · 69% into the close if not in by 13:15
    FAR SIDE — 7745 EM high·node 62%K  66%  ~35m (15m–1h10)
               7758 KING 100%K         34%  ~1h20 (40m–2h40)
               7772 prior-day high     15%  ~2h10 (1h–3h30)
               7790 and beyond — 96% it does not trade there today

**THE MODEL IS A TABLE AND A SCALING LAW, NOT A REGRESSION — for the third time in this project.**
- **P(a level trades before the close)** from a distance-in-σ × minutes-left table: **AUC 0.826,
  Brier 0.147, calibrated at every decile**, 388,494 observations over 197 ES sessions (**F-14**).
- **Timing** = the first-passage distribution GIVEN the level is reached (**F-15**). The analytic law
  `T ~ (d/σ)²` does 95% of the work of a ten-feature model.
- σ = 1-bar sd × √(bars left), computed from the panel's own candles, in BAR units then converted
  once (mixing bar and minute units is landmine L-F).

**AND THE CHART EXPORT (v14.79, corrected v14.80).** `irtBuildCsv()` now writes SEVEN rows per configured symbol —
three Kings plus the three 0DTE levels he asked for on 2026-08-28:

    SPXW KING / SPY KING / QQQ KING   SOLID    (SPY was dashed until v14.79)
    CW0    call wall 0DTE             RED      dotted
    PW0    put wall 0DTE              GREEN    dotted
    FLIP0  zero gamma 0DTE            PURPLE   dotted

⚠⚠ **THE FLIP ROW IS `dte0.gf.flip` AND NOTHING ELSE — v14.79 had this backwards and he caught it
the same hour.** His words: *"the flip 0dte is displayed when 0dte is selected just like how you get
the walls for 0 dte."* The companion fetches `insiderfinance.io/gamma-exposure/SPX` with **no expiry
parameter**, so `pub.zeroGamma` scraped off their header is their DEFAULT **ALL-EXPIRY** view — the
same reason their header walls read 7900/7500 on 2026-08-22 while 0DTE was 7700/7665
(`session-state/INSIDERFINANCE.md`, `pub.wallsAreAllExpiry`). Their payload carries every contract,
and `gammaFlip()` over the FRONT EXPIRY ONLY is what their page draws with 0DTE selected. Same
provenance as CW0/PW0 — so the label is plain **`FLIP0`**, no asterisk, exactly as CR0/PS0 carry
none. **There is NO all-expiry fallback** (his call, asked and answered): no 0DTE flip, no row.

⚠ Every price in that file comes from `ifLadder(sym).rows[].disp` — the SAME array the rail draws.
The exporter re-derives NOTHING (DECISIONS v13.2). This rule was broken twice on 2026-08-28 and
both times produced levels that disagreed with the panel's own rail.

---

## 3 · THE FIVE FINDINGS THAT PRODUCED IT — read before proposing anything

| | |
|---|---|
| **F-12** | ⚠⚠ **the hover's 92% IN call is 63% in real time.** The study picked the side WITH HINDSIGHT, so failed calls were relabelled out of the sample. The CELL rate is honest; the DECISION figure is not. **`HLTAB_META.inHit` STILL SAYS 92 — fix it.** |
| **F-13** | predicting the far side's PRICE directly is dead: a fixed **1.36× expansion beats gradient boosting** (9.2 vs 9.9 pts). Timing as an extremum is the clock. |
| **F-14** | the reframe: **"will price REACH this level"** — AUC 0.826, and **half of all readings land at ≤20%, where levels traded 8% of the time.** |
| **F-15** | timing works as **first passage**, AUC 0.692, calibrated. Approach velocity worth +0.022. Regime splits worth nothing. |
| **F-16** | **daily ATR: nothing. Level identity: nothing** under a dense control. ⚠ **A sparse control claimed +12 points — a phantom.** |

⚠ **DO NOT RE-PROPOSE:** sweeps, momentum divergence, NQ divergence, IB30/IB60, the 50-SMA,
open-reclaimed, the 60-minute breakout, the daily ATR, the overnight range, volume, day-of-week,
gap, prior-day/overnight level identity, red/green days, a 5-feature logistic, or a narrow
high-probability timing box. Every one is measured and recorded.

---

## 4 · WHAT HE ASKED FOR THAT CANNOT BE BUILT, AND WHY THE FACE SAYS SO

> "i wanted something like LOD IN 80%, HOD in about 3-3.5 hrs 80%. are you saying this cannot be done
> with high probability?"

Measured, n=197: a **±15-minute box lands 15%** of the time, an hour 24%, and a **two-sided window
must be 3.6 hours wide to reach 80%**. So the face carries a **one-sided 80% floor** ("not before
12:00") plus the middle-half window at its true **50%**, and prints the arithmetic underneath.
**Printing "3–3.5h, 80%" would be a lie of precision.** He accepted this.

---

## 5 · HOW IT IMPROVES — this is the part he cares most about

1. **DATA.** `farside` is enrolled and records, every bar, the three nearest rated levels with their
   distance in σ, the table's p, the expected first-passage time, **and each level's node identity**
   (`kind`, %King, polarity, role). That last field is the gamma dataset nobody has ever collected.
2. **ANALYSIS.** `docs/LLM-NIGHTLY-BRIEF.md` carries a ⭐ section telling the review exactly what to
   do: score the touch call by decile, score the ≤20% NO call separately, run the gamma test
   (**with a dense distance control** — F-16), and **propose a new `FARSIDE.json`, never apply one**.
3. **ADOPTION.** Companion v1.16 couriers `data/es-1min/FARSIDE.json` into `gpts_farside_v1`; the
   panel validates it (≥120 sessions, every rated cell n≥60, monotone in distance) and keeps the
   baked table if the payload fails. **A better table reaches the face without a build.**
4. **THE UNTESTED INPUT.** v1.16 also couriers two years of daily **^VIX** — not wired into anything.
   Implied vol is the one volatility measure that is not a slower copy of what the panel computes.
   The next cloud-side study is implied-vs-realized σ over the corpus.

---

## 5b · THE FACE SHRANK — v14.81, and what is NOT coming back

The **GAMMA PROFILE** (bars + 5m/15m/60m ROC matrix + WALLS summary + legend) was **removed at his
request** on 2026-08-28: *"just remove it."* 169px of a 1016px face.

⚠⚠ **DO NOT PROPOSE REBUILDING IT.** He asked for a gamma profile back at v11.x, node bands were
substituted for it (recorded as a failure), it was built for real in v14.0–14.4 — and then he cut it.
Both facts were put to him before the cut, so this is a decision, not an accident. If it returns it
returns because HE asks.

⚠ It was ALSO broken when cut — 21 overlapping label pairs measured live. That is worth remembering
before assuming any dense block on this face is readable: **render it and audit overlaps, don't
eyeball it.**

**ROLL BIAS survived and MOVED** — it had ridden the profile header since v14.4 by his direction; it
now sits on the ② LOCATION row. Its rehome was broken on first writing (read `ROLLS`, a local of
`secLoc()`, from `secFrame()` — a ReferenceError a bare catch would have eaten, chip absent, suite
green). **A rehomed line must re-derive its inputs, never inherit a scope it can no longer see.**

Three test files now pin the REMOVAL (`test_lastbook` r4, `test_garma_p1` k1/k2,
`test_velocity_policy`) so a reinstated profile turns them red and forces the dual-crown fix and the
replay display guard to be re-decided rather than rediscovered.

---

## 5c · THE LADDER'S LEFT HALF — v14.83 geometry, and what must not be re-litigated

    SPXW col      2   26      bars+labels  140  234      delta value  428  472
    SPY col      28   52      chute pills  246  290      state        476  530
    level name   56  102      mk           294  340      roc          534  618
    price       104  138      tap          344  364
                              delta bar    368  424

**LAD_W is still 618.** Name→price gap is **2px** — that is deliberate and asserted (`g5c`): he asked
for `PDC 7741` to read as ONE token, and a wider gap makes them two columns again.

⚠⚠ **v14.82 PUT THE NAMES IN THE CHUTE AND HE REJECTED IT.** Do not propose it again. The chute is
**price's alone** once more — that invariant was reversed for one build and is now restored, EM pill
included. Everything in the chute is a 44px right-justified pill.

⚠⚠ **`ladderRolls` IS RETIRED — ITS LANE IS THE KING COLUMNS.** It duplicated what `secLoc()` already
draws (v13.9) and ROLL BIAS still states whole-book direction. ⚠ Retiring it ALMOST deleted the
*"INFERRED from paired changes, never an observed transfer"* caveat, which lived only in its hover;
r11/r12 caught it and it now sits on the surviving roll hover. **Deleting a drawing must never delete
a claim's caveat** — check for orphaned caveats before retiring any drawer.

### The King track (`gpts_kingtrack_v1`, `__gptsDebug.kingTrack()`)

⚠⚠ **IT READS `ladderKings()`, THE SAME CALL THE CROWN PILLS READ.** Measured 2026-08-28:
`snap.king` and `snap.tri.SPY.king` disagreed at 13:24 and 13:36 (769 vs 771). Two fields, two
answers to "how many times did it move". Never re-source this from `tapeMap` or the recorder.

⚠⚠ **IT STORES `raw` (the book's own strike), NOT `at`.** `at` drifts with the basis; tracking it
would record a migration every few minutes.

⚠⚠ **THREE THINGS ARE NOT MIGRATIONS** — a change that reverts before `KT_DWELL` observations (a
flicker: three of them on 2026-08-28 alone), the expiry roll at the close (RTH-gated, expiry stamped
per point), and anything seen while `recorderBlind()`.

**Measured baseline, 2026-08-28 13:15→close:** SPXW **0** durable moves, SPY **1** (771→769 at
13:39). The feature is viable precisely BECAUSE migrations are rare.

⚠ **The recorder only held 13:15 onward that day** — the panel was down for the morning. An empty
column therefore says WHICH kind of empty; "nothing moved" and "nothing was watched" must never look
the same.

**Ladder width vs the 454px body remains open** — 618px is unchanged.

---

## 5d · CORRECTED NUMBERS — v14.84, and what the corrections exposed

**`HLTAB_META.inHit` is 63, not 92** (F-12). The withdrawn figure is kept as `inHindsight:92`.
⚠⚠ **Correcting it REVERSED a ranking the hover asserted**: NOT-IN (85%, n=230) is now the STRONGER
call and fires an hour earlier. `test_hodlod` u3 had been pinning the OLD ranking and stayed green
until the bug was fixed — **a test can pin a consequence of a defect and look healthy doing it.**

**The SUCCESSION 76% is WITHDRAWN.** It does not reproduce at any horizon (23% / 41% / 55%). The
original was 4 days scored against the RAW TAPE; the crown the panel DRAWS is the latched one.
⚠ **All succession numbers now live in `SUCC_META`, once.** The withdrawn figure had been typed into
FIVE strings. Never re-type one of these into a hover.

**Measured, and it is the useful part:** at ≥80% of the King, by the node's own state —
BUILDING 42%, FADING 39%, **STEADY 7%** against an 8% base rate. **Motion is the signal, stasis is
the anti-signal, size alone is the weakest of the three.**

⚠⚠ **THE CROWN MOVES ~5-6 TIMES A SESSION (SPXW, drawn), NOT 0-1 AND NOT 11.6.** I gave four
different answers in one thread. The errors, in order: a truncated recording window; the SPY book
reported as SPXW; the RAW tape counted instead of the latched crown (the operator caught this —
*"i am looking at atlas and it doesn't seem to move that much"*); and a denominator that divided by
9 days when only 6 carry an SPXW series. **`tools/study-kingmoves.py` separates raw from drawn —
use it, and state which one any King number refers to.**

**The step bar and section headers are gone** (operator, 2026-08-29). `STEP_TIPS` is KEPT — it holds
the only written statement of what each section is for.

### Asked for, mocked, approved, NOT BUILT
- **♕2ND challenger glyph** on the ladder + **state-conditioned SUCCESSION tile**. Mockups approved;
  he said *"lets leave this for now"*. Nothing to design — build when he asks.
- **The TREND section**: he ASKED whether it can be removed and it was never answered with evidence.
  Known: its DRIFT chip is a measured coin flip (50.0%, n=68, "DESCRIPTIVE ONLY"), and
  `test_trendbadge` is one of the six permanently-red files.

---

## 5e · THE ⓪a READ LINE — v14.86, and the format that keeps being asked for

The face now says, exactly:

    HOD IN 100% · LOD after 1:30pm — 80%

⚠⚠ **HE HAS ASKED THREE TIMES FOR A TWO-SIDED WINDOW AT A HIGH NUMBER** — *"HOD expected in 2.5-3hrs
76%"*, *"LOD expected 3.5-4hrs 75%"* — and it cannot be built honestly. Measured over 197 sessions
(F-13/F-15): **±15 min lands 15%, ±30 lands 24%, and a two-sided window must be 3.6 HOURS wide to
reach 80%.** "after X — 80%" is a ONE-SIDED FLOOR and is the same claim "not before X" made. When
this comes up again, show him those three numbers rather than re-deriving the refusal.

**The MIDDLE HALF is the honest two-sided answer and it is 50%**, stated in the hover against the
80% floor so they cannot be read as one number. "most likely" is banned from this line — MIDDLE HALF
is what an interquartile range is.

⚠ **THREE THINGS LIVE IN THE HOVER NOW AND THEY ARE NOT DECORATION**: the travel % and **n** behind
the call, the middle-half window with its 50%, and the hazard clause. Compressing the line would
have deleted all three. **A percentage whose n is not reachable is what ⓪a exists not to be.**

⚠ The clock is **12-hour on this line only**; the stats table stays 24-hour because columns of times
align. Two formats on one face, taken deliberately.

---

## 5f · ⓪a — THE AGREED DESIGN, AND WHAT IS STILL UNBUILT (v14.87)

He redesigned this section over **eight mockup rounds** on 2026-08-29. The layout is settled; do not
re-litigate it. Mockups: `/tmp/mock/day*.html` are gone with the sandbox — the spec is here.

**AGREED LAYOUT** — A / E / **Δ** rows, fields as columns:

    top strip, beside the read:   HL GAP · HL RANGE · LC|HC GAP · LC|HC RANGE
    1ST block:  1ST | SLvl | TIME | TOOK | BOP | WICK | W.END | WICK% | MUD
    2ND block:  2ND | TLvl | TIME | PT TOOK | PT | PTWICK |  —  | PTWick% | PTMUD
    FAR SIDE block: REMOVED

⚠ **PTWICK / PTWick% / PTMUD align under WICK / WICK% / MUD**, with the slot under **W.END empty** —
that gap is deliberate and was in his very first sketch. **SLvl sits immediately after HOD, TLvl
immediately after LOD** — the level is part of what that extreme did, not a trailing column.
Colours: EXPECTED carries the colour, Δ is quiet grey, actual HOD/LOD times are WHITE.

**BUILT v14.87:** PT TOOK / PT, LC|HC GAP+RANGE, side-specific expecteds, pills back to 62px.
**BUILT v14.88:** SLvl / TLvl (furthest level taken out between the open and that extreme, from the
panel's OWN level sets only), PTWick% (~44%) and PTMUD (~0h54), both side-specific.

⚠⚠ **PTWICK IS STILL UNBUILT AND NEEDS HIM.** WICK = "the open to the bar that RECLAIMS the open" —
it needs an ANCHOR the move started from and later took back. The PT leg's anchor IS the second
extreme, so "reclaim" would mean returning to that extreme, a different event. **He defined
BOP/WICK/W.END/WICK%/MUD himself when asked — ask him for this one rather than inventing a sixth.**
`test_hodlod` L14/L15 keep it absent until he answers.

**FIRST THING TO REVIEW:** the whole ⓪a section on LIVE bars. Everything shipped 2026-08-29 was only
ever seen against an empty session (market closed, A row all dashes, E row proving the wiring).

### ⚠⚠ NOT BUILT, AND WHY — do not ship these with invented numbers
- **PTWICK / PTWick% / PTMUD** — the wick study has ONLY ever run on the FIRST extreme. Mirror it on
  the second (`tools/study-pt.py` has the leg; the wick definitions are the operator's own) BEFORE
  building. Three columns of guessed expecteds is what made the wick family untrustworthy in v14.57.
- **SLvl / TLvl** — needs the profile levels below plus tag detection.

### The profile levels (measured 2026-08-29, `tools/study-profile.py`)
`futBarsRaw()` rows are `[epoch,o,h,l,c,VOLUME]` and the ES corpus has Volume too, so a TRUE volume
profile is computable live and historically. ⚠ **BUT PRICE DOES NOT CARE**: prior POC tagged next
session **46.6%** vs a sham level at the same distance **46.3%**; VAL **43.5% vs 43.5%**. Distance
explains the tags. Ship as a DESCRIPTIVE RECORD with **no percentage**. ⚠ Name them **pVAH/pVAL/pPOC**
— never "value area", which the **VALUE 70%** tile already means (the gamma band around the King).

### PT vs LC — the distinction that cost a re-measurement
    PT   second extreme -> the FURTHEST point back    ~0h49 / ~19.8 pts   "was there a trade"
    LC   second extreme -> the CLOSE                  ~1h43 / ~12.5 pts   "did it hold"
58% apart. Side-specific: PT after a LOD **24.0 pts**, after a HOD **17.0**.

⚠ **HE COMMENTS ON DELIVERED FILES AND THOSE COMMENTS DO NOT REACH US.** He commented on a mockup
file card; the only reason they were read is that he screenshotted them. **Publish design mockups as
ARTIFACTS**, which carry readable comment threads.

---

## 6 · WHAT TO DO NEXT, IN ORDER

1. ⚠⚠ **CONFIRM THE PANEL IS ACTUALLY RUNNING.** On 2026-08-28 `__gptsDebug` answered at 09:57 and
   was **undefined by 11:20** in the same tab. If it stayed down, that session recorded nothing.
   Read the running version off the panel; never ask.
2. **CORRECT `HLTAB_META.inHit`** from 92 to the real-time **63%**, and re-word the hover (F-12).
   Oldest open defect; it is a number on his face that hindsight inflated.
3. **PUSH THE DAY FILES.** 2026-08-28 was the first session recording the `farside` node identity —
   the gamma dataset ⓪a's self-improvement depends on. A commit is not a push; they reach GitHub
   only when he runs an installer.
4. **THE `dates=` PROBE.** He was given a console line to test whether Skylit serves historical node
   maps. If it does, the gamma question is a weekend study over hundreds of sessions instead of a
   month of collection. The live request shape is
   `/tv/api/gex/levels?…&data_type=combined&nodes=p20&exp_mode=current&dates=YYYY-MM-DD`.
   ⚠ Those params have DRIFTED from `SKYLIT-FEEDS.md` (it documents `gamma|vanna`, `500`, `week`).
5. **The implied-vs-realized σ study** — `gpts_vix_daily_v1` has delivered 503 daily closes
   (companion v1.16). The question: does VIX beat the panel's own realized σ in the first-passage
   scaling law, and at what hour does it stop helping?
6. **The gamma-conditioning test** — design exists in `tools/study-atrlevels.py`, needs ~40 clean
   sessions and a **DENSE** distance control. A sparse control invented a +12-point effect on
   2026-08-28 that vanished the moment the control was tightened (**F-16**).
7. **The ladder width** — 618px in a 454px body, still his call.
8. **ITEM 18 Tier 1/2** and the **2026-09-16 backfill deadline**, untouched.

⚠⚠ **THE IRT PIPE IS SOLVED — DO NOT RE-LITIGATE IT (2026-08-28).**
IRT's **Remote File** field accepts a `file://` URL and reads it **once, on Apply — it does NOT
poll.** Proof: after v14.73 removed `100%` from the King labels, both charts still drew
`SPXW KING 100%`, and `Refresh` did not move them; his words were "they dont change". An earlier
note in this file claimed `file://` polled; that was written from ONE observation in which the read
and the settings change happened at the same instant. **A polling claim needs two reads with no
user action between them.**
**Standing config, verified live:** `irtserve.bat` running (autostart installed via
`setupautostart.bat`, he confirmed DONE), BOTH charts pointed at
`http://127.0.0.1:8000/FlexLevelsExport.csv`, Check Every 1 Minute, feed left on `gex`.
⚠ **Never run that server against a panel older than v14.74** — before the latch, a partial write
erased levels the polling client was watching, and he photographed it.

---

## 7 · HOW TO WORK WITH HIM

1. **ONE AT A TIME.** State one item, its fix, ask, STOP.
2. **Do not build until he says build.** He says it plainly ("lets finish this off and build").
3. **TEST BEFORE YOU BUILD.** Twice on 2026-08-28 a measurement changed the build after I had already
   described it to him.
4. **RENDER EVERY MOCKUP HEADLESS** with the pairwise overlap audit, at **454px**.
5. ⚠⚠ **DELIVER EXACTLY ONE FILE.** His words, 2026-08-15 and restated 2026-08-27: *"you are
   supposed to just give me an install file."* One `installvNNNN.bat`, dash-free and dot-free,
   **plus the Tampermonkey links as text**, plus **tell him to click them** — Tampermonkey's default
   update check is once a day, so the click is the reliable step.
6. **Bump both version strings** and the four test pins; the rule-count pin is **75** now.
7. **One edit, one write, verify.** A multi-edit script that aborts writes nothing.
8. **MUTATE EVERY NEW ASSERTION INDIVIDUALLY** — six were mutation-tested this build and all six
   fired on the right assertion.
9. **Run the whole suite; 6 baseline reds are expected** (`expiry_profile`, `node_map`, `sma_cont`,
   `tapeking` (needs jsdom), `trendbadge`, `v1126_process`).
10. **VERIFY THE INSTALLER BY DECODING IT** before sending — it has silently dropped directories.
11. **End every build message with `✅ SAVE DONE`** naming what was updated.

---

## 8 · DOCTRINE THAT MUST NOT BE LOST

- **Two books, never averaged.** Skylit = FLOW. InsiderFinance = OI×gamma.
- **Name both units out loud before comparing two numbers.**
- **Absence of data is not a reading.** Thin cells refuse; they do not guess.
- **A well-formed number is not a supported one.** Monotone ≠ evidence.
- **Measure the question the FACE actually puts** — and state WHEN each variable was read. Both
  halves of that rule were broken this session and caught (F-12, F-13).
- **A matched control must SPAN its range densely**, or it invents an effect (F-16).
- **Gamma tells you HOW price moves, never WHICH WAY.**
- **Anything unproven ships labelled unproven and scored nightly.**
