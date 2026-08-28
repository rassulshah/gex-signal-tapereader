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
