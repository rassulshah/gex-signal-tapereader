# FINDINGS — what WE measured, beside what the Academy says

**This file is named by three live hovers in the panel and had NEVER EXISTED in any commit** —
verified 2026-08-28 with `git log --all --diff-filter=A`. It was not lost; it was never written.
Created 2026-08-28 with its first real entry.

**Read it alongside `skylit-docs/learn/`, never merged into it.** The Academy is doctrine — the
vendor's prior. This file is evidence. Where they disagree, the disagreement is the finding, and
`SOURCE-OF-TRUTH.md`'s 2026-08-25 amendment governs: doctrine is the default where there is no
evidence; evidence wins where there is.

**Every entry carries a STATUS: OPEN · PROVISIONAL · CONFIRMED · SUPERSEDED.**
⚠ A SUPERSEDED finding is kept, never deleted, so a later context does not rediscover a dead end.

---

## F-1 · IS THE LOW / HIGH OF DAY IN? THE INITIAL BALANCE ANSWERS IT. SWEEPS DO NOT.
**Status: PROVISIONAL** (one corpus, one instrument, 284 sessions) · measured 2026-08-28
Script: `tools/study-lodhod.py` · Corpus: `data/es-1min/ES TestingData.txt`, EPM26 1-minute,
284 complete RTH sessions, 2025-06-02 → 2026-08-21.

**The operator's question, verbatim:** *"identify a low or a high of day so i can profit when it
goes to the other extremity ... you need to help identify whether the lod or hod is done."*

### Method, and why each control is there
- **One row per (session, side), at the bar the rule FIRST fires.** 188,000 bar-observations are not
  188,000 observations — the label barely changes within a session. Effective n is **568
  session-sides**, and that is what is quoted.
- **Lift is measured against the base rate AT THE SAME CLOCK TIME.** The chance a standing extreme
  is the day's climbs from ~40% at 09:30 to ~64% by noon, so any rule that can only fire late
  collects that for free. This is PROJECT-CONSTANTS pattern 7, and it is the single reason the
  first pass of this study looked twice as good as it was.
- **Train/holdout split by date** (60/40), because 18 rules were tested.

### Result

| rule | n | hit | median fire | base at that hour | **lift** |
|---|---|---|---|---|---|
| IB60 broken + extreme stood ≥60m | 323 | **82%** | 10:17 | 50% | **+32** |
| IB60 broken + 60m breakout | 332 | 80% | 10:03 | 47% | +33 |
| **IB60 broken** | 339 | **78%** | 10:01 | 46% | **+32** |
| **IB30 broken** | 364 | 74% | **09:39** | 41% | **+33** |
| >75% of range from the extreme | 428 | 69% | 09:35 | 40% | +29 |
| 60-minute breakout | 551 | 70% | 10:31 | 52% | +18 |
| SMA50 (150 min) | 567 | 55% | 09:30 | 39% | +16 |
| **sweep + reclaim** | 230 | **48%** | 09:58 | 45% | **+3** |
| *wait 120m (the ladder alone)* | 508 | 74% | 11:09 | 58% | +16 |
| *wait 180m* | 431 | 84% | 12:02 | 64% | +20 |

### What this says
1. **THE INITIAL BALANCE IS THE SIGNAL.** IB30 and IB60 deliver +32/+33 over the clock, and deliver
   it at 09:39–10:17 — while the session still has its range ahead of it. Waiting to noon reaches
   84% but only +20, on a base already at 64%.
2. **SWEEPS ARE DEAD FOR THIS QUESTION. 48%, lift +3, BELOW the same-hour base.** Two independent
   samples agree (a 50-session Yahoo ES pilot gave 53%; this corpus gives 48%). ⚠ The `SWP` chip
   ships on ⓪a as a confirmation and should not be read as one.
3. **THE 50-SMA CARRIES NO INDEPENDENT INFORMATION HERE.** Alone: 55%. And `IB60` and `IB60+SMA`
   return *identical* n and hit (339 / 78%) — whenever IB60 has broken, the SMA condition is already
   true. ⚠ This is NOT a verdict on the 50-SMA as a direction tool, which is the operator's stated
   backbone and a different question.
4. **STACKING CONFIRMATIONS DOES NOTHING.** IB60, IB60+SMA, IB60+far and IB60+SMA+far are all 78% at
   n≈339. Every added condition shrinks the sample without moving the rate. **One clean trigger.**

### Against the Academy
`learn/` has no article on session-extremity timing, so **no doctrine is contradicted or confirmed
here** — this is ours. The nearest neighbour is `charts-first`, which treats prior highs and lows as
chart structure rather than exposure, and is consistent with an initial-balance break mattering.

### What would move this
Another instrument (NQ/GC/CL — the corpus tap now collects them), a second contract to rule out an
`EPM26`-specific artefact, and a live forward-scored season. Until then: **PROVISIONAL**.

---

## F-2 · A CALIBRATED PROBABILITY FOR "IS THE EXTREME IN" — AND WHAT ACTUALLY DRIVES IT
**Status: PROVISIONAL** (ES only; the cross-market half is UNTESTED) · measured 2026-08-28
Script: `tools/model-lodhod.py` · 284 sessions, EPM26 1-minute, decision sampled every 5 minutes.

**The operator's ask:** *"test the various combinations with divergences ... create a predictive
probabilistic model so i can use it to identify if a hod or lod has occurred."*

### The control that decides whether any of this is real
The chance a standing extreme is the day's rises from ~40% at 09:30 to ~64% by noon **for free**. So
a model given only the clock already scores **AUC 0.8204**. That is the number to beat, and every
figure below is quoted against it. Folds are **GroupKFold by session date** — bars inside one session
share a label, so a random split would leak it.

### Result

| model | features | AUC | Brier |
|---|---|---|---|
| time only (the baseline) | 3 | 0.8204 | 0.1627 |
| time + `posr` | 4 | 0.8778 | 0.1331 |
| **time + `posr` + `rsi`** | **5** | **0.8795** | **0.1321** |
| everything | 14 | 0.8792 | 0.1323 |

**Five features equal fourteen.** Total headroom over the clock is **+0.059 AUC**, and `posr` alone —
how far price has travelled from the extreme as a fraction of the session range — is **+0.0574 of it**.

### What each feature is worth ON TOP OF TIME (add-one), and what is lost when DROPPED

| feature | add-one | drop-one | reading |
|---|---|---|---|
| `posr` distance travelled from the extreme | **+0.0574** | **−0.0067** | **carries the model** |
| `rsi` momentum level | +0.0472 | −0.0011 | strong alone, mostly redundant with posr |
| `sma` 50-SMA (150m) | +0.0365 | 0.0000 | a proxy for posr |
| `ib30` broken | +0.0181 | +0.0004 | a proxy for posr |
| `opn` open reclaimed | +0.0175 | 0.0000 | a proxy for posr |
| `ib60` broken | +0.0150 | +0.0004 | a proxy for posr |
| `swp` sweep + reclaim | +0.0048 | −0.0013 | ~nothing (see F-1: 48% standalone) |
| `bN` 60-minute breakout | +0.0021 | 0.0000 | nothing |
| **`mdiv` MOMENTUM DIVERGENCE** | **−0.0004** | **+0.0003** | **NOTHING. Measured, not assumed.** |
| `xdiv` cross-market divergence | 0.0000 | 0.0000 | ⚠ **UNTESTED** — NQ corpus not present |
| `side` low vs high | −0.0001 | +0.0004 | the model is symmetric, as F-1 found |

### The three things this says
1. **ONE QUANTITY EXPLAINS ALMOST ALL OF IT.** IB30, IB60, the 50-SMA, open-reclaimed and the 60-minute
   breakout are all proxies for *price has travelled away from the extreme and stayed away*. `posr`
   measures that directly and absorbs them — which is exactly why F-1 found IB60, IB60+SMA, IB60+far
   and IB60+SMA+far returning identical numbers. **Stacking confirmations is measuring one thing five
   times.**
2. **MOMENTUM DIVERGENCE DOES NOT HELP. −0.0004 AUC.** RSI *level* carries information (+0.047 alone);
   RSI *divergence at the extreme* carries none. This was the operator's own hypothesis and the answer
   is no — recorded so it is not re-litigated.
3. **CROSS-MARKET DIVERGENCE IS NOT ANSWERED.** `NQ TestingData.txt` exists on the operator's machine
   but is not on GitHub, so `xdiv` was constant 0. **Do not read its 0.0000 as a negative result.**

### The shippable model
`P = 1/(1+exp(-z))`, standardised inputs, fitted on all 284 sessions:

    z = 1.14337
        +0.70156 * ((mins   - 225.000) /  96.698)     minutes since the RTH open
        +0.52858 * ((stood  - 132.809) / 105.981)     minutes the extreme has stood
        +0.12454 * ((extmin -  92.191) /  94.910)     minutes from the open to the extreme
        +1.19937 * ((posr   -   0.500) /   0.3167)    (price - extreme) / session range
        +0.37715 * ((rsi    -  50.000) /   5.3794)    RSI, oriented to the side being asked

⚠ Every coefficient is POSITIVE and that is the whole story: later in the session, longer standing,
further travelled, stronger momentum away — each independently makes the extreme more likely to hold.

### Calibration (out-of-fold) — this is what makes it usable for sizing

| predicted | 7% | 15% | 25% | 35% | 45% | 55% | 65% | 75% | 86% | 96% |
|---|---|---|---|---|---|---|---|---|---|---|
| **actual** | 9% | 17% | 25% | 34% | 45% | 57% | 65% | 74% | 84% | 96% |

### Decision thresholds — first crossing, one row per session-side

| threshold | n | hit | median fire |
|---|---|---|---|
| P ≥ 0.60 | 562 | 66% | 09:55 |
| P ≥ 0.75 | 536 | 75% | 10:25 |
| P ≥ 0.80 | 521 | 79% | 10:40 |
| P ≥ 0.85 | 497 | 84% | 11:00 |
| P ≥ 0.90 | 463 | 88% | 11:30 |

⚠ **Compare honestly with F-1.** The plain `IB60 broken` rule gives **78% at 10:01** on 339
session-sides. The model reaches 79% at 10:40 — but fires on 521. The model's real advantage is not a
higher hit rate, it is a **calibrated number available at every moment**, which supports sizing;
the binary rule is still competitive as a single trigger.

### What would move this
The NQ corpus (cross-market divergence, the one hypothesis still open), a second instrument to rule
out an EPM26 artefact, and forward-scoring on live sessions via the FEATURES registry.

---

## F-3 · BOTH DIVERGENCES ARE DEAD, AND THE EARLINESS/ACCURACY FRONTIER
**Status: PROVISIONAL** · measured 2026-08-28 · 163 sessions where ES and NQ overlap
(2025-11-24 → 2026-08-21) · `tools/model-lodhod.py`

**The operator's requirement, verbatim:** *"i just want to know with high accuracy whether the hod or
lod have occurred ... I want to know it as early as possible."*

### 1 · CROSS-MARKET (NQ) DIVERGENCE DOES NOT HELP — now actually tested

    time only                0.8440
    + posr + rsi             0.8956
    + xdiv (NQ divergence)   0.8942     delta -0.0014

Raw and uncontrolled, the direction is **opposite to the intuition**: when ES makes a new low and NQ
does not, the low holds **66%** of the time against **70%** when both make it. Divergence makes the
extreme slightly LESS reliable. ⚠ F-2 recorded this as UNTESTED because the corpus was absent; it is
tested now and the answer is no.

### 2 · WHY BOTH DIVERGENCES *LOOK* LIKE STRONG INVERSE SIGNALS, AND ARE NOT
Time-controlled, momentum divergence fires and the extreme holds only 24-31% against a 46-60% base —
which reads as a powerful "the extreme is NOT in" call. **It is an artefact.** A divergence can only
fire at the instant a fresh extreme prints, which is precisely when `posr` ≈ 0. The model already
knows price is sitting on the extreme, so the divergence adds nothing (-0.0004 / -0.0014 AUC).
⚠ **A feature that can only fire in one state is measuring that state, not predicting anything.**
This is failure pattern 7 in a new costume and it fooled me for one pass.

### 3 · THE EXHAUSTIVE SEARCH, AND WHY ITS WINNERS ARE A TRAP
All 1/2/3-way combinations of 16 binarised features; 339 cleared a coverage floor; split by date.
The top of the table reaches **98-100% on holdout** — and **every one of them contains `stood120`
and fires between 11:30 and 12:45**. By then the base rate is already ~64%.
⚠ **They are the clock wearing a costume.** With 339 combinations tested, the top of any such
ranking is partly luck; the honest reading is the frontier below, not the leaderboard.

### 4 · THE FRONTIER — the answer to "as early as possible"

| accuracy wanted | earliest the model reaches it | share of session-sides |
|---|---|---|
| 70% | 09:50 | 98% |
| 75% | 10:00 | 94% |
| **80%** | **10:10** | **90%** |
| 85% | 10:30 | 85% |
| 90% | 11:15 | 79% |
| 95% | 12:20 | 65% |

**80% by 10:10 on 90% of days is the practical operating point.** Going to 90% costs an hour and a
fifth of the days. Held to a fixed clock instead: by **09:45**, `P>=0.85` already labels 73
session-sides at **84%**.

### 5 · WHAT IS AND IS NOT SOLID
**Solid:** the calibration (predicted vs actual within 1-2 points at every decile), the frontier, and
the three negative results — sweeps (F-1), momentum divergence, NQ divergence — each confirmed on
independent samples.
**Not solid:** one instrument, one 15-month regime, **no forward test on unseen days**. The single
thing that would settle it is forward scoring through the FEATURES registry, per the 2026-08-17
mandate: predict live, score the outcome, and let n accumulate honestly.

---

## F-4 · THE MODEL DOES NOT EARN ITS COMPLEXITY — SHIP THE TABLE
**Status: CONFIRMED** (the comparison is internal and reproducible) · 2026-08-28
Script: `tools/sanity-lodhod.py` · 284 sessions.

**The operator asked the right question:** *"is this a standard model. is this the right use case for
it ... step back and see if you made the right choice for this type of problem."*

### 1 · A TWO-DIMENSIONAL LOOKUP TABLE EQUALS THE REGRESSION

    lookup table  posr x time            AUC 0.8787   Brier 0.1321
    lookup table  posr x time x stood    AUC 0.8729   Brier 0.1349
    logistic, 2 features (posr, mins)    AUC 0.8759   Brier 0.1340
    logistic, 5 features                 AUC 0.8795   Brier 0.1321

**Identical Brier. 0.0008 of AUC.** The five-feature model is ceremony. ⚠ And adding `stood` to the
table made it WORSE — thin cells overfit. Two dimensions is the right size.

**SHIP THE TABLE, NOT THE REGRESSION.** Same accuracy, same calibration, and it is INSPECTABLE: every
cell carries its own n, the operator can argue with a number he can see, and live drift against it is
visible. Five coefficients cannot be argued with. This also honours his standing instruction —
*"why are you making things more complicated"* — which this project has been burned by ignoring.

### 2 · THE SURVIVAL FRAMING IS NOT BETTER, AND IT WAS WORTH CHECKING
"Will a new extreme print before the close?" is a time-to-event question, so a discrete-time hazard
model is the textbook fit. Built and measured:

    hazard -> survival curve             AUC 0.8751   Brier 0.1464
    direct classification (shipped)      AUC 0.8795   Brier 0.1321

Direct classification wins, mainly on calibration. ⚠ The classification framing was the right call —
but it had been an ASSUMPTION until this test, and the project's rule is that an assumption written
in the voice of a measurement is a defect.

### 3 · IT IS REGIME-STABLE

| slice | AUC | base |
|---|---|---|
| quiet days (range < median) | 0.858 | 67% |
| volatile days (range >= median) | 0.899 | 65% |
| first half of the corpus | 0.869 | 63% |
| second half | 0.894 | 69% |

No collapse, and it is slightly BETTER when the market actually moves — which is when it is used.

### 4 · THE TABLE ITSELF — P(this extreme is the day's), 284 sessions, 38,054 observations

| posr \ CT | 9:15 | 10:00 | 10:45 | 11:30 | 12:15 | 13:00 | 13:45 |
|---|---|---|---|---|---|---|---|
| 0.88-1.00 | 78% | 84% | 90% | 96% | 99% | 99% | 100% |
| 0.75-0.88 | 62% | 76% | 86% | 89% | 95% | 98% | 99% |
| 0.62-0.75 | 60% | 72% | 75% | 84% | 88% | 95% | 98% |
| 0.50-0.62 | 46% | 62% | 67% | 73% | 81% | 91% | 97% |
| 0.38-0.50 | 36% | 50% | 55% | 65% | 78% | 85% | 95% |
| 0.25-0.38 | 27% | 34% | 49% | 56% | 65% | 68% | 84% |
| 0.12-0.25 | 19% | 21% | 33% | 36% | 39% | 57% | 66% |
| 0.00-0.12 | 7% | 8% | 9% | 12% | 15% | 20% | 28% |

⚠ Cells under 25 observations are not shown and must not be invented.

### 5 · THE CRITICISM THAT SURVIVES
**The target is not the decision.** This predicts *does this extreme hold*. The operator's stated goal
is *can I profit travelling to the other extremity*. A low can hold perfectly while price goes
nowhere. **Expected travel to the opposite extreme, conditional on the extreme holding, is a
DIFFERENT measurement and has not been made.** Do not present this table as answering it.

---

## F-5 · THERE IS STILL A TRADE LEFT WHEN THE CALL FIRES — the fear was wrong
**Status: PROVISIONAL** · 2026-08-28 · `tools/study-travel.py` · 284 sessions.

**The worry, stated before it was tested:** the table's confidence comes almost entirely from `posr` —
how far price has ALREADY moved off the extreme — so it should be most confident exactly when the
move is over. If true, it would call the low at the moment there is nothing left to trade, and the
whole feature would be answering the wrong question.

**Measured, at the first bar the (out-of-fold) table reaches each confidence:**

| confidence | n | correct | median CT | range still ahead | actually travelled |
|---|---|---|---|---|---|
| P ≥ 0.60 | 558 | 65% | 09:55 | 51% | 34% |
| **P ≥ 0.70** | **552** | **73%** | **10:10** | **49%** | **30%** |
| P ≥ 0.75 | 550 | 75% | 10:40 | 49% | 29% |
| P ≥ 0.80 | 546 | 81% | 11:15 | 47% | 25% |
| P ≥ 0.85 | 509 | 84% | 11:25 | 44% | 25% |
| P ≥ 0.90 | 477 | 91% | 12:25 | 41% | 21% |

**The fear was wrong.** At the 70% call, **half the day's range is still ahead**, and price does
travel a median **30% of the day's range** toward the far extreme before the close. Waiting from 70%
to 90% buys **+18 points of accuracy** and costs **2h15m and a third of the remaining travel**.

⚠ **`actually travelled` IS MAXIMUM FAVOURABLE EXCURSION, NOT A REALIZED TRADE.** It is the correct
descriptive statistic for a tool that is forbidden entries, stops and sizing — but it is the best
case inside the move, not what anyone banks. Never present it as a return.

### The operating point this argues for
**P ≥ 0.70.** Below it the edge over the clock is thin; above it the cost in time and forgone travel
rises faster than the accuracy does. The data picks this point, not a preference.

---

## F-6 · RED / GREEN DAY IS ALREADY ANSWERED BY THE OPEN — do not build it
**Status: CONFIRMED** (the baseline is definitional) · 2026-08-28 · `tools/study-redgreen.py` ·
284 sessions.

**Operator's enhancement idea:** *"predicting if today will be a red day or green day where the close
is greater than the open or less than the open."*

**The control:** "is price above the open RIGHT NOW" is already a strong guess at the close. Quoting
accuracy against 50% would make anything look brilliant.

| features | AUC | accuracy |
|---|---|---|
| sign now (the baseline) | 0.796 | **83%** |
| + time + which extreme came first + posr | 0.819 | **83%** |
| + 50-SMA | 0.825 | **83%** |
| everything | 0.905 | **83%** |

**Accuracy does not move at any hour** — 73/73 at 09:30, 76/76 at 10:30, 79/79 at 11:00, 85/85 at
13:00, 88/88 at 14:00. The extra features reorder CONFIDENCE but never change a CALL.

⚠ **And the confidence is overconfident where it would be used.** Aggregate calibration is decent
(deciles 6/14/25/39/47/57/61/70/83/97), but taken at the FIRST moment it looks sure — which is how a
panel would surface it — it says ≥90% and delivers 83%, says ≥95% and delivers 90%. First-crossing
selection, the same effect that inflated the first pass of F-1.

**NOT BUILT.** It would be machinery around a comparison the operator can make by looking at the
chart. If it is ever revisited, the only honest form is a continuous confidence readout, never an
alarm — and it must be quoted against the sign-now baseline, never against 50%.

---

## F-7 · THE TABLE TRANSFERS BETWEEN INSTRUMENTS — the "one instrument" caveat is half-answered
**Status: PROVISIONAL → strengthened** · 2026-08-28 · `tools/study-transfer.py`
ES: 284 sessions / 38,054 rows · NQ: 188 sessions / 25,192 rows.

The strongest objection to F-4 was that the table stood on ONE instrument. Tested:

    ES table   -> NQ data     AUC 0.8877   Brier 0.1237
    NQ's OWN table -> NQ      AUC 0.8853   Brier 0.1225   (out-of-fold)
    NQ table   -> ES data     AUC 0.8804   Brier 0.1338

**The ES table predicts NQ as well as NQ's own table does** — very slightly better, most likely
because it is built on 96 more sessions. Cell by cell: **64 comparable cells, mean absolute gap 4.5
points, only 5 differ by more than 10.**

### What this means
1. **ONE TABLE SERVES BOTH MARKETS.** No per-instrument build, and no reason to assume GC/CL will
   need one either — though that stays untested until the corpus tap collects them.
2. **It is measuring session STRUCTURE, not something about ES.** "How far price has travelled off
   the extreme, and what time it is" behaves the same way on a different contract with a different
   tick, a different multiplier and a different volatility. That is a much stronger claim than a
   backtest on one symbol.
3. ⚠ **Still PROVISIONAL.** Two correlated index futures over overlapping windows is not two
   independent tests — ES and NQ move together. It answers "is this an ES artefact" (no); it does
   NOT answer "does this hold in a different regime" or "does it hold forward".

---

## F-8 · THE PANEL CALLS THE FIRST-PRINTED EXTREME, AND THAT CHANGES BOTH NUMBERS
**Status: PROVISIONAL** · 2026-08-28 · `tools/study-notin.py` · 284 sessions.

Two questions were tested before building, and both changed the build.

### 1 · Restricting to what ⓪a ACTUALLY calls makes the IN call much stronger

⓪a reports on `D.first` — the extremity that printed FIRST. An earlier pass measured over BOTH sides
at every bar, which also asks "is the SECOND extreme in", a different and harder question.

| | n | correct | median CT | far side still ahead |
|---|---|---|---|---|
| P ≥ 70% | 284 | **94%** | 09:55 | **97%** |
| P ≥ 75% | 284 | 95% | 10:00 | 94% |
| P ≥ 80% | 284 | 98% | 10:20 | 91% |

**94% at 09:55, not the 76% previously quoted.** ⚠ The mixed-sample figure was answering a question
the panel does not ask. **Always measure the question the face actually puts.**

### 2 · The NOT-IN call is REAL but much weaker than the first pass suggested

| | n | broke | median CT |
|---|---|---|---|
| P ≤ 15% | 71 | 77% | 09:45 |
| **P ≤ 20%** | **85** | **72%** | **09:45** |
| P ≤ 25% | 100 | 67% | 09:35 |

An unrestricted pass measured **93%** here. That number came from the same mixed sample and **must
not be quoted**. Restricted properly it is **72% on n=85**, against a ~57% base at that hour — a real
+15 edge, arriving early, on thin data. It ships with those numbers on its hover, not the 93%.

### 3 · "toward the HOD" was being printed on days the HOD had already happened
The clause fired whenever a ladder tier existed. Measured across both sides it was wrong ~47% of the
time; restricted to the first-printed extreme it is wrong only 3% at the 70% call. **Either way it
was guessing at something directly observable** (`D.secondT > D.clock`), so it is now gated, and when
both extremes are in the face says *"both extremes in — the range is set"* instead.

### 4 · When the far side IS still ahead, it prints at
**median 13:33, IQR 11:51–14:47.** That is the clause the operator asked for, and it is measured
rather than borrowed from the unconditional E-row median.

---

## F-9 · THE LEARNING LAYER HAS NEVER RUN — the review could not open its own input
**Status: CONFIRMED** (arithmetic, not inference) · 2026-08-28 · `tools/day-digest.py`

The operator asked whether the nightly LLM review was built. It is built, scheduled and firing — and
it has produced **one 216-byte artefact in ten days.**

### The cause is arithmetic

    data/2026-08-27.json     4.2 MB   ~= 1,041,000 tokens
    the review model's context             200,000 tokens
    -> the day file is 5.2x the ENTIRE window; the WEEKLY reads all of them, ~7M tokens (36x)

### And the correlation is exact

| day file | size | review outcome |
|---|---|---|
| 2026-08-18 | **1.3 MB** | **LOG WRITTEN** |
| 2026-08-19 | 4.3 MB | nothing |
| 2026-08-20 | 5.9 MB | nothing |
| …every day since | | nothing |

**The last review that ever landed is the last day the file was small enough to read.** Nothing about
Drive, the mover or the operator's PC was broken: `review-pull.bat` did its job on 08-18 and has had
nothing to move since. `GEX-review-inbox` is empty; `_done/` holds one file.

⚠⚠ **THE CONSEQUENCE, AND IT INVALIDATES A LOT OF LANGUAGE USED IN THIS PROJECT:** every rule in
`learning/rules.json` reads `n=0`, `promoted:false`, `lastVerified:null`. **Nothing has ever been
forward-scored.** Every "it will be scored nightly" and "enrolled so the live rate accumulates"
written into this codebase — including for `lodhod` — has been describing a loop that never closed.

⚠ **AND THE PROJECT ALREADY KNEW.** `DECISIONS.md` D-11, 2026-08-24: *"a day export is 5.9 MB … The
archive needs a digest."* Measured, written down, and the review was pointed at the raw file anyway.
**The repo is the first place to look, not the last** — third occurrence of that lesson.

### The fix
`tools/day-digest.py` — 4.2 MB → ~7 KB (600x), emitting only aggregates: per-feature n / resolved /
scored / rate / **vote split** / oneWay flag / MFE / MAE / per-regime, `effectiveN`, node events
summarised, and **`dataHealth`**, which flags a day whose feature records cover almost no bars:

    2026-08-20   3822 records / 122 bars of 131 snapshots (93%)   ok
    2026-08-27     15 records /   1 bar  of 133 snapshots ( 1%)   COLLAPSED

All four scheduled tasks now digest first, clone FULL (every one used `--depth 1`, banned since
2026-08-27), check `dataHealthVerdict` before computing anything, and are told to score `lodhod`
without copying its backtest into `rules.json`.

⚠ **A REVIEW THAT CANNOT SEE ITS INPUT FAILS SILENTLY AND LOOKS LIKE A QUIET MARKET.** Nothing
alerted for ten days because "no findings" and "no data" produced the same empty result.

---

## F-10 · localStorage WAS FULL. THAT IS THE FEATURE-RECORD COLLAPSE, AND FOUR OTHER "BUGS".
**Status: CONFIRMED** (measured on the live panel) · 2026-08-28

    localStorage total   10,240 KB   = exactly Chrome's 10 MB cap
    gpts_recorder_v7      5,957 KB   holding ONE day (2026-08-27)
    gpts_nodeevents_v1    3,228 KB   one day of node events
    write probe, 40 KB    QuotaExceededError

**Every write in the system was failing, silently, behind `catch(e){}`.**

### It was not five bugs. It was one.

| symptom chased | actual cause |
|---|---|
| feature records collapsed (15 records / 1 bar vs 133 snapshots) | `recorderSave()` → `setItem` → quota |
| the Yahoo corpus tap "never ran" | `futStore()` → quota (and its fallback → quota) |
| the base-rate courier never delivered | same |
| InsiderFinance levels 8.5 hours stale | the companion's `store()` → quota |
| "the companion is running v1.14" | **wrong.** v1.15 was installed and correct all along |

⚠⚠ **AND A DIAGNOSTIC CREATED ITS OWN EVIDENCE.** Deleting `gpts_evcal_v1` and `gpts_futbars_v1` to
test "is the companion alive" freed just enough room for the tiny calendar object to write — which
looked like proof the script was running and the Yahoo courier specifically was broken. **The probe
changed the state it was measuring.** Before concluding from a storage experiment, check the quota.

⚠ **THE INSTRUMENT ADDED IN v14.67 WAS AIMED AT THE WRONG LAYER.** `FEATH` counters were built to
find out why `featRecordAll` produced nothing — registry vs dedupe. Neither. The records were built
correctly every bar and thrown away at the final `setItem`. **One quota check would have found in
thirty seconds what a night of reasoning did not.** Measure the cheapest thing first.

### Why nothing noticed for a week
`recorderSave()` HAS quota handling: on failure it drops the oldest non-today day and retries. It
could not work here — the recorder held **only** today, so there was no victim, and today alone was
6 MB. `nevSave()` has no size cap at all (`NEV_MAX=4000` caps EVENTS, not bytes; 1,332 events reached
3.2 MB because each carries a why-vector plus three outcome objects). And every failure path is
`catch(e){}`, so the panel kept drawing, the recorder kept "recording", and nothing said the disk was
full. **Failure pattern #5 — a swallowed error is invisible — in the storage layer.**

### The fix — **BUILT v14.68/v14.76** (this section said "NOT YET BUILT" until 2026-08-31)
Both keys are bounded by BYTES through `lsPut` against `LS_BUDGET_KB`; `recorderSave()` sheds today
oldest-first; quota failures go through `swallow()` into `__gptsDebug.renderErrors()` and are counted
in `LS_HEALTH`; days already exported are pruned; `__gptsDebug.storage()` exists. `test_storage.js`
executes `lsPut` against a fake storage with a real quota and is green.

⚠ A **parked patch** (`session-state/pending/v14.68-bounded-writes.patch`, base v14.67) is what kept
this reading as unbuilt in `LOCKED-ITEMS.md` for 42 versions. **A patch file is evidence of an
intention, never of a state.**

### ⚠⚠ F-10b · THE FIX WORKS AND THE BUDGET DOES NOT — THE MORNING IS SHED, SILENTLY
**Status: CONFIRMED** (measured on the day file itself) · 2026-08-31

    data/2026-08-31.json    snapshots       131    08:30 -> 15:00 CT   the whole session
                            feature records 1370   13:36 -> 15:00 CT   the NEWEST 29 bars, all 48 keys
                            day-digest verdict     COLLAPSED, 22% bar coverage

`LS_BUDGET_KB['gpts_recorder_v7']` is **3600 KB**. A session measures **~6 MB** — the rate recorded in
F-10 above. So the shedder trims today oldest-first exactly as designed, the export runs at the close
**after** the shedding, and every day file now systematically retains only the last ~90 minutes.

⚠⚠ **THE DISCARDED HOURS ARE THE ONES THE MODELS LIVE IN.** The ⓪a NOT-IN call fires at a median
**08:40** (F-11) and the GREEN/RED call at **09:03** (`GD_META`). Neither is ever present in the
record that is supposed to forward-score it. **Every "it will be scored nightly" claim in this
codebase is again describing a loop that cannot close** — the same sentence F-9 had to write, one
layer down.

⚠ **AND THE DAY FILE CANNOT SAY SO.** `LS_HEALTH` counts `shed`, `quotaHits` and `lastErr`;
`buildDayExport` carries none of them (zero hits for `lsHealth`/`shed`/`quota` in the file).
**The silent write failure became silent shedding.** Exporting `LS_HEALTH` is the cheapest next step
and the one that turns the inference above — drawn from timestamps — into a measurement.

⚠ Do NOT simply raise the budget: it exists to respect a 10,240 KB cap that was measured full. The
candidates are shedding to IndexedDB instead of dropping, exporting intraday rather than at the
close, or recording features more cheaply.

---

## F-11 · A DELETED FEATURE'S CONSTRAINT WAS COSTING THE BETTER HALF OF THE TABLE
**Status: PROVISIONAL** · 2026-08-28 · 284 sessions, EPM26 1-minute.

The ⓪a table skipped the **first 60 minutes of every session**. That exclusion came from the original
study requiring `IB60` to exist before a row could be scored. **IB60 was then measured as worthless**
(AUC 0.655; adds nothing once `posr` is known — F-1/F-2) and dropped from the model. The constraint
stayed behind.

Consequence: the 08:30 column of the table was entirely empty, and the panel printed
**"no rate (thin cell, n=0)" for the first 45 minutes of every session** — the hour the operator
prepares in.

### Re-derived from minute 5

| | before (t≥60) | after (t≥5) |
|---|---|---|
| observations | 38,054 | **44,302** |
| cells with data | 64 / 72 | **72 / 72** |
| 08:30 column | all thin | 4 · 10 · 16 · 22 · 27 · 32 · 40 · 47 % |
| **AUC on the SAME late-session rows** | 0.8787 | **0.8787 — identical** |

⚠ **Strictly additive.** The overlap AUC is unchanged to four decimals, so nothing that already
worked was traded for the new coverage. (Pooled Brier rises 0.132 → 0.138 because the new early rows
are intrinsically harder, not because the model degraded.)

### And the NOT-IN call lived almost entirely in the missing cells

| call | before | after |
|---|---|---|
| **IN** (cell ≥70%) | 94%, n=284, 09:55 | 92%, n=284, **09:35** |
| **NOT IN** (cell ≤20%) | 72%, n=**85**, 09:45 | **85%**, n=**230**, **08:40** |

The NOT-IN call is where the early cells live — a fresh extreme at 08:35 is a 4% chance of being the
day's. It went from a thin modest edge to **85% on n=230, an hour earlier**. The IN call trades 2
points of accuracy for 20 minutes.

⚠ **THE LESSON, AND IT IS THE POINT OF THIS ENTRY:** when a feature is removed, the constraints it
imposed on the STUDY do not remove themselves. IB60 was deleted from the model in the same session
that measured it worthless, and its footprint went on silently costing 45 minutes of coverage a day.
**Grep the study for every assumption a deleted feature justified.**

### Tests had to be decoupled from the data
Two assertions used the empty 08:30 column as their fixture for "a thin cell refuses". Filling the
table broke them — they were testing DATA, not LOGIC. They now inject a thin cell and assert the
refusal directly, so the table's contents can change without breaking a test of behaviour.


---

## F-12 · THE ADVERTISED "92% IN CALL" IS 63% IN REAL TIME — THE SIDE WAS PICKED WITH HINDSIGHT
**Status: CONFIRMED** (three independent reproductions) · 2026-08-28 · `tools/study-second.py`

The ⓪a hover states the decision rate as **92% correct, n=284, median 09:35** (`HLTAB_META.inHit`).
Reproduced on the same 284 sessions, the number the PANEL earns in real time is **63%**.

    the panel, live      calls whichever extreme is FIRST-PRINTED AT THAT MOMENT (running loT<hiT)
                         first crossing of the 70% cell   ->  63%, median 09:20, n=284
    the study behind 92  selected the side that TURNED OUT to print first (final flT<fhT)
                         first crossing of the 70% cell   ->  91%, median 09:31, n=284
    same, OOF cells, t>=60, restricted to that side       ->  94%, median 09:55   (the shipped 92/94)

**Selecting the side with hindsight conditions on the answer.** When a standing low breaks at 11:00
the panel was WRONG — but in that session the HIGH becomes the first-printed extreme, so the study
asks about the high instead and the miss is never recorded. Every failure is relabelled out of the
sample. This is landmine **L-N** ("measure the question the face actually puts") in a new costume,
one version after L-N was written.

⚠ **WHAT IS NOT WRONG: the CELL rate on the face is honest.** Calibrated over all 44,302
observations on the first-printed side:

    table says  10-19% -> actual 12%    40-49% -> 47%    70-79% -> 75%    90-99% -> 97%

So "LOD IN — 74%" is a true 74%. It is the DECISION figure in the hover that is inflated.

⚠ **The 63% is the FIRST CROSSING — the earliest and weakest moment.** Accuracy is buyable with time:

    cell >=70%  63% @ 09:20      >=85%  86% @ 10:45      >=95%  92% @ 11:46
    cell >=80%  79% @ 10:06      >=90%  89% @ 10:53

⚠ **The study that produced 92/85 was never committed.** v14.70 shipped the table, the tests and the
FINDINGS entry, but no tool that reproduces those two numbers — so they could not be re-checked from
the repo. **A number on the face needs a committed script that regenerates it.**

---

## F-13 · THE OPPOSITE EXTREMITY: A TWO-LINE RULE, NOT A MODEL — AND THE CLOCK OWNS THE TIMING
**Status: PROVISIONAL** · 2026-08-28 · `tools/study-second.py`, `study-secondpred.py`,
`study-secondpred2.py` · ES 284 sessions (+NQ 188 for transfer)

**The operator's ask:** *"HOD expected around 7772-7792 in 3.5 Hrs between 1:30pm and 2pm - 80%"*,
then *"different probabilities for the HOD time and the HOD price range"*, then *"did you try your
best ... different factors, combinations, models"*.

⚠⚠ **A BUG IN MY OWN HARNESS INFLATED THE FIRST VERSION OF THIS ENTRY.** `held` was evaluated with
the running extremes read AFTER the bar loop — i.e. the FINAL session extremes — so it was always 1
and the "the first extreme held" filter was a no-op that kept every FAILED call in the sample.
Corrected numbers below; the discarded ones (AUC 0.628 holdout, realized volatility +0.017 on
timing, an 82% NQ band transfer) were measured on a contaminated sample and **must not be quoted**.
**The same class of error as F-12, one hour after writing F-12.** State WHEN a variable is read.

### 1 · The 30-minute box does not exist
+-15 minutes around the median lands **4%** of the time; +-30 lands **9%**. An 80% time window is
~3.5 hours wide. The opposite extreme is near-uniform across the session with a spike into the
close: **35-40% print in the final 45 minutes**.

### 2 · THE TRADEOFF THE OPERATOR ACTUALLY CHOOSES FROM (real-time side selection, no hindsight)

| call at | n | correct | median CT | travel left to the far side | 80% price band |
|---|---|---|---|---|---|
| cell >=70% | 284 | 63% | 09:20 | 27.0 pts | 55 pts |
| cell >=80% | 284 | 79% | 10:06 | 21.2 pts | 44 pts |
| cell >=85% | 284 | 86% | 10:45 | 18.8 pts | 41 pts |
| **cell >=90%** | 284 | **89%** | 10:53 | **17.8 pts** | **41 pts** |
| cell >=95% | 284 | 92% | 11:46 | 16.1 pts | 37 pts |

### 3 · PRICE — the ML model DOES NOT BEAT TWO LINES OF ARITHMETIC (n=206, call >=90%)
Predicting the final range fixes the far-side price. Median absolute error:

    "the range is already done"        14.5 pts
    "a typical day" (the E row)        18.1 pts
    fixed 1.36x expansion from here     9.2 pts     <- THE RULE
    gradient-boosted model, 10 factors  9.9 pts     <- WORSE

And the interval is the same story:

    80% band, conditional CQR (10 factors)                    cov 81%   width 42 pts
    80% band, expansion-multiple quantiles x today's range    cov 80%   width 41 pts

**SHIP THE RULE.** `far side ~ standing extreme + 1.36 x (range so far)`, banded by the historical
spread of the expansion multiple. Third occurrence of F-4's lesson in one project.

⚠ **AND SAY HOW WEAK IT IS.** The point estimate is out by a median **9.2 pts on a target that is
typically 17.8 pts away**, and the 80% band (41 pts) is ~70% of a typical 57-pt day. **A 20-pt band
- the width the operator asked for - is worth about 50%, not 80%.**

### 4 · TIME — the clock is the model, and nothing else survives (5,738 bar-rows, corrected)

    clock alone (minutes since open, minutes left)   AUC 0.7441
    clock + realized volatility                      AUC 0.6632
    clock + all 12 other factors                     AUC 0.7061

The usable form is the hazard: given the far side has NOT printed yet, P(it prints in the last 45
minutes) = **33% by 11:00 · 53% by 12:30 · 69% by 13:15 · 90% by 14:00.**

### 5 · MEASURED AND REJECTED — sixteen factors, one pass each, none survived
Overnight/globex range (**the corpus is 24h and every earlier study threw ETH away** — it is the
WORST of the sixteen, -0.06 AUC), volume and volume rate, IB30 size, efficiency ratio, day of week,
gap vs prior close, distance to prior-day high/low, minutes-to-first-extreme, realized volatility,
posr at the call, wick%, elapsed since the call, prior-day range, and every 2-3 way combination the
add-one search covered. ⚠ The overnight session is now TESTED for this question - LOCKED-ITEMS
carried it as "untested, cheap".

### 6 · WHAT IS NOT TESTED, AND IT IS THE ONE THAT WOULD MAKE THIS OURS
**The gamma book.** Nine recorded sessions, several of them F-10 collapsed, so "does a low printing
at a put wall, or price sitting on the King, tighten the band" cannot be asked yet. Until it can,
this feature is generic price structure that any charting package could compute - measured
honestly, but not ours. **That is the strongest argument for the storage fix.**


---

## F-14 · CHANGE THE QUESTION: "CAN PRICE GET THERE" IS PREDICTABLE, "WHERE WILL THE EXTREME BE" IS NOT
**Status: PROVISIONAL** · 2026-08-28 · `tools/study-touch.py` · 284 ES sessions, 71,568 decisions

F-13 exhausted the extremum framing: the best point estimate is a fixed 1.36x expansion and the ML
model loses to it. So the framing was changed - from *predict the far-side price* to **P(a NAMED
LEVEL is touched before the close)**, evaluated every 15 minutes at level distances of
0.25-2.0 sigma on both sides, sigma = realized 1-minute vol x sqrt(minutes left).

    AUC 0.826   Brier 0.147   base rate 28%

### Calibration, out-of-fold, grouped by session
| predicted | 0-10 | 10-20 | 20-30 | 30-40 | 40-50 | 50-60 | 60-70 | 70-80 |
|---|---|---|---|---|---|---|---|---|
| **actual** | 4% | 17% | 25% | 35% | 43% | 53% | 66% | 71% |

### And it is SHARP where it matters
**48% of all decisions land at P<=20%, and those levels were touched only 8% of the time** - i.e. a
92%-accurate "price does NOT get there today" call, available on half of all decisions. The high
side is thin (0.9% of rows reach P>=80%), which is honest: reaching a far level is rarely a
near-certainty.

### Why this is the right shape for THIS panel
1. **The operator trades from nodes.** "Will price reach the node at 7792, and when" is the question
   the tool exists to answer; "what will the exact HOD price be" is a question nobody trades.
2. **The median of the touch curve IS the far-side estimate** - the price where P(touch) = 50% - so
   the band from F-13 falls out of this model instead of needing its own.
3. **It is where the gamma book plugs in.** The levels stop being sigma multiples and become the
   King, the node strikes, the EM edges, the prior-day levels. **Does a level's node identity change
   its touch probability at the same distance?** That is a single, measurable experiment - and it is
   the first question in this project where the answer would make the tool ours rather than generic.
   It needs the recorder working (F-10) and ~40+ clean sessions.


---

## F-15 · TIMING IS PREDICTABLE ONCE THE QUESTION IS POSED AS FIRST PASSAGE, NOT AS AN EXTREMUM
**Status: PROVISIONAL** · 2026-08-28 · `tools/study-firstpass.py` · 284 ES sessions, 7,168 arrivals

**Written because the operator asked "did you do your best" and the answer was no.** F-13 concluded
"the clock is the model" for timing. That conclusion is CORRECT FOR THE QUESTION IT ASKED — *when
does the extremum print* — and that question is nearly unanswerable: unconditional, near-uniform,
with a spike into the close. Re-posed as **"given price REACHES this level, when does it get
there"**, timing becomes partially predictable.

### Point prediction, minutes-to-touch (median absolute error, median horizon 86 min)

    the unconditional median                  49.0 min
    the clock alone                           46.0 min
    the analytic first-passage scaling T~(d/sigma)^2   42.0 min   <- PHYSICS, one feature
    distance + vol + time left                43.3 min
    + the touch model's inputs                42.3 min
    + APPROACH VELOCITY + first-passage       41.7 min

⚠ **The analytic scaling law does 95% of the work of the full model with ONE feature.** Fourth
occurrence of this project's oldest lesson: the simple thing wins, ship the simple thing.

### As the call a face can make — "does it arrive within the hour?"

    clock alone                          AUC 0.637
    distance + vol + time left           AUC 0.670
    + approach velocity + first-passage  AUC 0.692    base rate 36%

    calibration:  predicted 0-20% -> actual 14% | 20-40% -> 33% | 40-60% -> 47% | 60-80% -> 62%

### The three things that had not been tried, and what each was worth
1. **Posing it conditionally** (given it arrives) — the whole difference. Without it, timing is a
   clock; with it, it is a first-passage problem with real structure.
2. **APPROACH VELOCITY** — the rate price is closing on the level, oriented toward it, over 15 and
   30 minutes. Every earlier feature was static. Worth **+0.022 AUC**.
3. **The analytic first-passage scaling** — worth more alone than ten ML features together.

### And one that was worth nothing
**REGIME SPLIT.** Fitting trend-ish (price at a range extreme) and chop (mid-range) separately:
AUC 0.684 and 0.691 against 0.692 pooled. **One model serves both.**

### What this composes into
Per level, two calibrated numbers and a horizon:
`7772 · touched today 78% (F-14) · within the hour 45% · typically ~50 min (F-15)`.
⚠ Median error is **42 minutes on an 86-minute median horizon** — roughly 50% relative. It is a
range statement, never a clock time.


---

## F-16 · THE DAILY ATR ADDS NOTHING, LEVEL IDENTITY ADDS NOTHING — AND A SPARSE CONTROL NEARLY SOLD ME A PHANTOM
**Status: CONFIRMED** (the negative), **PROVISIONAL** (round numbers) · 2026-08-28
`tools/study-atrlevels.py` · 279 sessions, 59,108 level-decisions

**The operator asked:** *"can you use the expected move, the daily atr ... any levels, indicators,
data that would allow you to better predict the timing"*.

### A · DAILY ATR(14): NO
| added to the touch model | AUC |
|---|---|
| the shipped inputs (distance/sigma, clock, range, realized vol) | **0.8771** |
| + daily ATR(14) | 0.8767 |
| + range-so-far / ATR ("how much of a normal day is used") | 0.8770 |
| + room left in a normal day · + distance measured in ATR | 0.8770 / 0.8769 |

**The realized-volatility sigma already contains it.** ATR is a slower estimate of the same quantity.

### B · LEVEL IDENTITY: NO — AND THE FIRST ANSWER WAS A PHANTOM
The test: train a distance-only model on synthetic levels placed at k*sigma, then ask whether a
NAMED level is touched more often than that model expects at the same distance, time and volatility.

    with a SPARSE control (sigma levels at only 0.75 and 1.5)      with a DENSE control (0.15 -> 2.5)
      prior-day high   +10.4 pts                                     -0.3 pts
      prior-day low    +11.4                                         +2.4
      prior-day close  +12.6                                         -0.3
      overnight high   +13.7                                         -1.0
      overnight low    +12.2                                         +0.9
      round number     +45.4                                         +4.6  (+-2.5)

⚠⚠ **EVERY ONE OF THOSE "LIFTS" WAS THE CONTROL EXTRAPOLATING.** With levels at only two distances,
the model had to guess the curve in between, and the named levels sat in the gap. Densify the control
and the entire effect vanishes. **I was one write-up away from reporting "prior-day levels are worth
+12 points of touch probability", which is false.**
**THE RULE: a matched control must SPAN the range of the thing it is controlling for, densely.**

Round numbers survive at **+4.6 pts (+-2.5, session-clustered)** — weak, ~1.8 SE, PROVISIONAL.

### C · WHY THIS MATTERS MORE THAN THE ANSWER: it is the gamma experiment, already built
`study-atrlevels.py`'s residual test is exactly the design the gamma question needs - swap the named
levels for the King, the put wall and the node strikes, and ask whether they beat their own
distance-matched expectation. Two consequences:
1. **The experiment is ready** the moment the recorder has clean sessions (F-10).
2. ⚠ **Temper the prior.** If prior-day and overnight extremes - the levels every chart package draws
   - carry nothing once distance is controlled, "the put wall is a magnet" deserves the same
   scepticism and the same dense control. Gamma is mechanistically different (dealer hedging
   transacts AT the strike), so it is still worth asking. It is not worth assuming.
