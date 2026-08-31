# OPEN QUESTIONS — asked, unanswered, and blocking something

**Operator-mandated 2026-08-30.** `LOCKED-ITEMS.md` holds work that is AGREED and unbuilt.
This file holds work that cannot even be specified yet because a question is open.

⚠⚠ **WHY THIS EXISTS.** Questions were being re-asked every context. The operator answered
"what is BOP / WICK / W.END / WICK% / MUD" once, in his own words, and that answer then lived only
in a resume note — the same mechanism that lost ITEM 18 for 24 versions. A question with no home
gets asked again, and an agent that re-asks a question the operator already answered is wasting the
one resource this project cannot regenerate: his attention.

**RULES**
- An entry leaves ONLY by being answered (record the answer and the date) or explicitly dropped.
- Every entry says **WHO can answer it** and **WHAT IT BLOCKS**. A question blocking nothing is a
  musing; delete it.
- ⚠ **NEVER GUESS AN ANSWER AND BUILD ON IT.** The wick family shipped as PENDING for exactly this
  reason and was the better call. Print `—`, say why, and ask.

---

## ⛔ BLOCKED ON THE OPERATOR — only he can answer these

### Q1 · What does **PTWICK** measure?
**Blocks:** the last column of ⓪a's 2ND block. PTWick% and PTMUD are built and measured; PTWICK is not.
**Why it cannot be guessed:** `WICK` = "the session open to the bar that RECLAIMS the open". It needs
an ANCHOR the move started from and later took back. The PT leg's anchor IS the second extreme, so
"reclaim" would mean returning to that extreme — a different event from anything the first-extreme
family measures. He defined BOP/WICK/W.END/WICK%/MUD himself when asked; ask again.
**Asked:** 2026-08-30, v14.88 delivery.

### ✅ Q2 · Does the **TREND section** earn its space? — **ANSWERED BY REMOVAL, v14.90**
**Closed 2026-08-31** (the entry sat here stale for eleven versions). He asked "do we need trend
section, can it be removed" and it was removed from the face at **v14.90**, on the evidence that was
already on file: the DRIFT chip is a measured coin flip (50.0%, n=68), `test_trendbadge` is one of
the six permanently-red files, and the confirm tally's own hover admitted its count had never been
scored. ⚠ `secBias()` is KEPT and still records — `bias.confirm` is an enrolled feature — so the
reads keep being scored nightly with nothing drawn.
⚠ **The lesson is the eleven versions, not the answer.** A question that has been settled BY A BUILD
does not close itself here; nothing links the two. Check this file against the face every build.

### Q3 · **Sweep levels** — plural. Show the furthest, or all of them?
**Blocks:** SLvl behaviour on a day that clears several levels. Currently the furthest is shown and
the rest go to the hover, because a growing cell reflows the whole top row. He said "sweep levels",
plural, and has not confirmed.
**Asked:** 2026-08-29.

### Q4 · What is the **"nd"** contract?
**Blocks:** a market row in `FUT_MARKETS`. Carried in `.gex-config.json` since 2026-08-28: "The
operator named 'nd' and no context has identified the contract." Still unidentified.

---

## 🔬 ANSWERABLE BY MEASUREMENT — no operator input needed, just the work

### Q10 · When is a return to the node a NEW deflection rather than the same test continuing?
**Blocks:** the deflection study, and therefore any claim about how often nodes deflect price.
**Settled already** (his teaching + 5 circled examples, 2026-08-30): one *visit* is one deflection, not
one per 3-minute bar; and price need not touch exactly — near, on, or slightly through then reversing
all count. Both are implemented in `tools/study-deflections.py` (episode latch + re-arm), and the
correction is large: counting bars gives **1203**, counting visits gives **65** — 19× too many.
**Still open:** how far price must clear the node before the NEXT visit is a new event. At
`REARM = 2 × ZONE` (1.0 SPY point), 2026-08-27 node 765 still scores four deflections at
08:42 / 08:51 / 09:00 / 09:09 — which by his pictures looks like ONE circle. Candidates:
scale the re-arm to the session range rather than a fixed 1.0; require a minimum time separation;
or require the reversal to have fully completed before re-arming.
**CALIBRATED 2026-08-30** against his circles: every one is within **0.30** of its node (not 0.50)
and every node he marks is **48-100% of King** (not 20%). Both thresholds moved, plus a REACH test —
the visit must actually get to the node, not merely enter a band. That killed the six phantom
"deflections" of 2026-08-27. **THIRD PASS 2026-08-30:** his white/red chart named the node universe in its own header
(`765.0: +88.4M  768.0: +54.7M ...`) — he watches the TOP FEW BY DOLLARS, so the selector is a
top-N RANK, not a %King threshold. All 12 marks sit within 0.40 of a node in the top 5.
⚠ Node 764 on 2026-08-24 carries one deflection AND two breakdowns — the node selects WHERE, the
price action decides WHAT.
**Residual: breaks MATCH him (2026-08-20: 2 detected, 2 marked); deflections ~3x his MARK COUNT.**

⚠⚠ **Q10 IS REFRAMED, 2026-08-29.** His ATR rule was tested as stated and COVERS the marks (recall
verified bar-by-bar on 4 of 5 first-calibration circles; the 5th, "768.30 on 08-27", cannot be
checked because price never came within 0.66 of it that day - most likely MY transcription error
off his chart, not a rule miss). The 2 ATR penetration leg carries only 3% of tests, so it is
permissive headroom, not the load-bearing half; the 1 ATR approach leg does the work.

So the rule is not the problem, and **the remaining gap may not be error at all.** He sent those
charts as "examples", never as a complete labelling. Tuning until 78 becomes ~12 would be fitting
to an illustrative sample. **Q10 IS CLOSED AS A GEOMETRY QUESTION, 2026-08-29.** His ATR rule is finalised: approach **1.0
ATR** (1.25 degrades the turn rate), penetration **1.5 ATR** (a non-question - only 3% of tests
reach past 1 ATR, so 1.5 and 2.0 differ by 0-4 events in ~400), triggered on the **WICK** not the
close. 79 deflections / 25 breaks over 8 sessions, 9.9 per session.

**Q11 (NEW, and now the only one that matters): the ex-ante deflect/break discriminator.** The
touch itself has NO edge - deflect and break are mirror images (+0.92/+0.26 vs +0.29/+0.86) and
56% break, so they cancel to t~0. Candidate discriminators to test when day files extend: which
book's king it is; whether the node is gaining or shedding mass into the touch; approach velocity;
time of day; whether an earlier test of the same node already held.

**⚠⚠ 2026-08-31 — THE SHARPEST CASE YET, AND IT SPLITS THE TWO CANDIDATE UNITS.** He circled SIX
deflections on today's SPY and said "there is 1 deflection in each circle". All six land on real
swing extremes in the recorded data. Measured against the finalised geometry:

    09:30 low  764.73   node 765   CAUGHT        10:54 high 766.34            MISSED
    11:09 low  764.98   node 765   CAUGHT        13:21 high 766.84  node 767  CAUGHT
    12:33 low  765.17   node 765   CAUGHT        13:45 low  765.05  node 765  CAUGHT

**5 of 6 by the node-first rule (15 deflections + 3 breaks detected), 4 of 6 by pullback-first
(5 events).** Neither unit reproduces his set.

⚠⚠ **THE MISS IS STRUCTURAL, NOT A THRESHOLD.** ATR(14) ran **0.33** today, so the test band is
**0.81 points wide against a 5.15-point session range** — 95% of bars closed inside the 765/766
bands, and on node 766 price never left the band between **09:39 and 11:00, 28 contiguous bars**. The
episode latch correctly refuses to call that 28 tests, and in doing so swallows his 10:54 turn.
**The rule that stops over-counting on a trending day is the same rule that blinds it on a coiled
one.** Pullback-first misses it for a different reason: the run-up was 0.94 points against a 3-ATR
(1.08) excursion floor, and it misses 13:21 too, on a shallow approach.

**STILL WANTED: one session labelled EXHAUSTIVELY** - every deflection
on that day, so the count has a denominator. Until then precision is unmeasurable and only recall
(currently 100% of checkable marks) means anything.
The remaining over-count is the RE-ARM distance.
**Only he can settle it** — it is a judgement about what counts as one test, and his charts are the
ground truth. ⚠ Do not tune this parameter to make a number look good.


### Q5 · What tolerance makes **HodN / LodN** honest?
**Blocks:** the three node fields. Proposed: nearest node within **0.10% of spot** (32 of 36 recorded
extremes land inside), printing `—` beyond it so "nothing was there" stays sayable.
**Status:** proposed, not agreed, not tested on more than 6 sessions.

### Q6 · Do the **profile levels** (pVAH/pVAL/pPOC) deserve a place?
**Measured 2026-08-29:** a prior POC is tagged 46.6% of the next session against **46.3%** for a sham
level at the same distance; VAL 43.5% vs 43.5%. **Distance explains the tags, not the level.**
**Open:** whether to ship them as a DESCRIPTIVE RECORD with no percentage. They must never be called
a "value area" — the VALUE 70% tile already means the gamma band around the King.

### Q7 · Does the **`dates=` probe** work — will Skylit serve a historical node map?
**Blocks:** every retrospective study, and it would have answered the 2026-08-30 node question
directly. **Unlocks:** the gamma-conditioning test becomes a weekend study over hundreds of sessions
instead of a month of collection.
⚠ The probe needs a line run in the panel: `LASTFEEDURL` and `LASTAUTH` live inside the userscript
IIFE and are not on `window`, so a console-side agent cannot build the request. **Either expose a
debug hook that returns the URL SHAPE with auth redacted, or he runs the line.**

### Q8 · Does the **dollar delta** beat %King growth at predicting succession?
**Measured so far:** by the node's own state at ≥80% of the King — BUILDING 42%, FADING 39%,
**STEADY 7%** against an 8% base rate. Motion is the signal; stasis is the anti-signal.
**Open:** whether `d5/d15/d60` sharpens that. The day files predate the `vend` schema; the current
schema records it. **Needs ~10 sessions of day files pushed.**

### Q9 · Re-derive the **succession numbers** against the DRAWN crown
`study-succession3.py` used the latched crown for the OUTCOME but the recorder for the node state.
Now that `tri.<SYM>.top` is known to carry full node lists, the whole study should be re-run on that.
**Until then, treat the 42/39/7 split as provisional.**

---

## ✅ ANSWERED — kept so nobody re-asks

- **"END TOOK or TCL?"** → neither. Resolved 2026-08-29 by putting PT back in the 2ND block, which
  freed `LC`/`HC` to mean Low/High→Close honestly. Two legs, two labels, both true.
- **"Is the 92% real?"** → No. Real-time it is 63% (F-12). Correcting it REVERSED the IN/NOT-IN
  ranking: NOT-IN (85%) is the stronger call.
- **"Does the SUCCESSION 76% reproduce?"** → No, at any horizon. 23% at 30m against the drawn crown.
  **Withdrawn.**
