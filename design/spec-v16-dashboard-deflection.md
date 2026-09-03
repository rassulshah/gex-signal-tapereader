# SPEC — DASHBOARD, DEFLECTION BUILD  ·  **DRAFT, FOR HIS REVIEW**

_agreed in conversation 2026-09-02 · baseline v15.50 · **not yet built**_

> Judged against `design/PURPOSE.md` and sequenced by `roadmap/DEFLECTION-ROADMAP.md`.
> ⚠ **NOTHING HERE IS BUILT UNTIL HE APPROVES THIS FILE.**

---

## 1 · THE STATE MACHINE — three states, and silence

    live, price inside the node's band      TESTING
    on the 3-MINUTE CLOSE                   DEFLECTED  or  BROKE
    the following bars                      follow-through confirms or withdraws it
    nothing near                            SILENT — no row state, no callout

**His words: "testing is better than holding".** And **"lets wait for the close"** — the panel shows
settled facts only; a forming bar is never graded.

⚠ **DECLINED: an `APPROACHING` state.** Asked and answered — no. The consequence, stated so nobody
"restores" it later: **the state is COINCIDENT, not early.** The early read lives in the
⇄ / Δ15m / STATE / ROC group (gamma building into a node price is walking toward). The state speaks
only once price is actually there.

## 2 · THE GEOMETRY — already settled, reused verbatim

    DEFL_NEAR = 1.0 ATR   may stop this far SHORT of the node and still be a test
    DEFL_THRU = 1.5 ATR   may penetrate this far THROUGH it and still be a test
    the WICK says price TESTED it · the CLOSE says DEFLECTED or BROKE

⚠ **`LVL_INPLAY_PTS = 3` IS RETIRED BY THIS SPEC.** A fixed chart-point threshold is three strike
gaps on cash and under half a gap on ES — the same constant, two different tests — and it is why the
MARK column is empty on his panel today. **One geometry, ATR-scaled, for both "price is on it" and
"a deflection happened".**

## 3 · BARS — 3-minute, aggregated on the chart's own boundaries

He trades the **3m chart**. The courier gives 1-minute bars; aggregate to 3m on :00/:03/:06 so the
panel and Skylit read identical bars. ⚠ The recorder already stores one frame per closed 3m bar, so
detection, recording and replay all land on the same unit.
⚠ **Latency is inherent and must not be hidden:** a deflection confirms on the close, up to three
minutes after the low prints. That is his rule, not a defect.

## 4 · WHERE IT APPEARS

**The row** — the state, in the MARK column that is empty today. Nothing else.
**The hover** — ATR depth, rejection depth, test count, gamma into the touch, node age.
**The callout line** — one line above the ladder, the single node in play:

    TESTING · SPY king 7655 · 0.3 ATR in · pullback node · gamma +$34M into the touch
             · 2nd test today, 1st held · king for 3h

⚠ **SILENT when nothing is near** — his call. No "nearest node" fallback.
⚠ **Two nodes in range → the NEAREST BY ATR wins**, not the biggest: price meets it first.

## 5 · PRICE-ACTION GRADERS — measured, not trusted

Detected on the closed 3m bar, at the node. In order of expected value:

1. **Rejection depth** — wick beyond the node as a fraction of the bar's range. Turns a binary
   close-back-inside into a quality. (2026-08-25: a 1.5-point rejection off node 763 — low 763.28,
   close 764.80 — is a different event from a close back inside by a tick.)
2. **Follow-through failure** — the next N bars fail to make a new extreme beyond the node. This is
   his "lack of FOLLOWTHROUGH" **without needing volume**: attempt-without-extension.
3. **Two-bar reversal / engulfing** at the node — confirmation, arrives a bar later.
4. **Outside bar / sweep-and-reverse** — rarest, strongest.

⚠⚠ **THESE ARE NEAR-RANDOM IN OPEN AIR AND THE PANEL MUST NOT IMPLY OTHERWISE.** Pin bars and
engulfing bars have no standalone edge. Their entire claim is CONDITIONAL — *at a gamma node* — and
whether that conditioning does anything is exactly `OPEN-QUESTIONS` **Q11**, which is unanswered.
**They ship as recorded GRADERS scored against the labelled corpus, never as a verdict on the face.**

## 6 · ⓪a GAINS THE DAY'S DEFLECTION LEDGER

A short list under the candle: **time · node · book · ATR depth · rejection depth · held or broke.**
It belongs in ⓪a because that section is the anatomy of the day's turning points, and it is what
makes a deflection reviewable instead of remembered.
⚠ **Harvest `day.defl` first** — the panel has recorded deflections every tick since v10.36 and shown
none of them, because the only reader is shadowed by a same-named function. There may already be a
corpus in the recorder. **Check before deleting anything.**

## 7 · THE CUTS — display only, never the measurement (v11.95)

| out | why | what survives |
|---|---|---|
| **King lanes S + Y** (52px) | they draw where the crown has BEEN — history does not help call a test happening now | the migration track keeps recording; **"king for 3h" / "crowned 8m ago"** becomes a clause, and node age is a new Q11 candidate |
| **`NODE · %KING` bar** (70px → ~12px) | his own rule: *the node universe is a RANK, not a threshold* — all 12 of his marks sat within 0.40 of a **top-5** node | a ①–⑤ badge on the top five; the percentage moves to the hover ⚠ he loses at-a-glance *gaps* between node sizes |
| `deflectionBlock`, `_deflChipHtml`, `deflUnlockN`, `deflGrade` | retired v10.50, zero callers | nothing — delete |
| `outOfSyncBlock`, `breakoutConviction`, `trendIsUpish/Dnish/CodeOf`, `countBarsSince` | dead paths per `ARCHITECTURE-AUDIT` | nothing — delete |

**Net ≈ 107px returned**, which pays for the callout line without the ladder getting wider.

## 8 · DEFERRED BY HIM, DELIBERATELY

- **Volume / absorption.** *"lets focus on volume later because it can complicate things."* The design
  stands and is recorded in §9 below; it is not in this build.
- **The W formation.** *"dont worry about the W"* — the **node is the reference**, not a prior pivot.
- **`APPROACHING`.** No.

## 9 · ABSORPTION — the agreed design, parked for later

Anchored to the **node**, not a pattern: price enters the band, volume is heavy on the bars inside
it, price fails to progress through. **Effort without result, measured against the node.**

    effort = volume inside the band vs the session's typical bar volume
    result = penetration past the node, in ATR

⚠ **AND THE LIMIT THAT MUST TRAVEL WITH IT:** we have total volume, not bid/ask. So it can say
*heavy volume with no progress through the node* — it **cannot** say *buyers absorbed sellers*.
Name it for what it measures; do not infer a side we cannot see.
⚠ **This is the one candidate independent of the gamma book.** Every other Q11 candidate is
gamma-derived and correlated; this is the tape. That is why it is worth building — later.

## 10 · WHAT THIS BUILD MAY NOT CLAIM

⚠⚠ **The touch itself has no edge**: 79 deflections / 25 breaks, **56% break**, mirror-image
excursions, **t = +0.41** / **t = −0.32** — both null. This build makes the test **visible and
recorded**. It does **not** predict the resolution, and no wording on the face may suggest it does.
**A confident surface over a coin-flip is the failure `PURPOSE §4.4` names.**

## 11 · TEST PLAN

- `levelMarkerOf` executed against the ATR band on both scales — the cash/ES asymmetry that emptied
  MARK must be impossible to reintroduce.
- The state machine executed through TESTING → DEFLECTED and TESTING → BROKE on recorded bars.
- 1m → 3m aggregation asserted on boundary alignment, not just bar count.
- The callout line: silent when nothing is near; nearest-by-ATR when two are in range.
- Every cut mutation-tested to prove the **measurement** survived the **display**.
- ⚠ A guard that the face carries no predictive wording — the §10 rule, enforced.
