# THE NIGHTLY REFINEMENT LOOP

> "can you make the data points, models goto the llm everynight for refinement and prediction with
> the gamma book ... its very important that this feature be a world class feature"

## ⚠⚠ THE CONSTRAINT THAT SHAPES EVERYTHING: THERE ARE SIX SESSIONS OF GAMMA BOOK

| corpus | sessions | has price | has the gamma book |
|---|---|---|---|
| `data/es-1min/ES TestingData.txt` | **284** | yes | **no** |
| `data/2026-*.json` (trinity snapshots) | **6** | yes | yes |

Every model shipped so far — HOD/LOD, the far side, GREEN/RED — is built on the 284 price-only
sessions. **Nothing that uses the gamma book can be tested yet.** Six sessions cannot separate a
real effect from noise at any threshold worth having; a feature measured on six days will look
spectacular roughly half the time.

So the loop does **not** begin by refining. It begins by **accumulating**, and by locking hypotheses
in *before the data to test them exists* — which is the strongest pre-registration obtainable. You
cannot fit to data that has not happened yet.

    PHASE 1  (now → ~100 sessions, about 5 months)   ACCUMULATE + PRE-REGISTER
    PHASE 2  (100 → 250 sessions)                    TEST, in registration order
    PHASE 3  (ongoing)                               FORWARD-SCORE the survivors

## THE DIVISION OF LABOUR

    THE LLM PROPOSES.  THE HARNESS DISPOSES.

The LLM never sees a result before its hypothesis is locked, and has **no vote on the verdict**.
This is not distrust of the model; it is arithmetic. Asked nightly to find predictive features in
282 sessions against a 51% base rate, it will find one *every night*. Test twenty ideas and the best
lands near 60% on pure noise — measured, not asserted: the shuffle test over four proposals put the
95th percentile of the noise band at **64%**.

## WHAT A PROPOSAL MUST DECLARE, BEFORE ANY MEASUREMENT

```json
{ "id": "gx-014",
  "claim": "one sentence, falsifiable",
  "feature": "a deterministic expression over recorded fields — no free parameters",
  "target": "green|hod_first|deflect|...",
  "direction": "signed: which way it should point",
  "falsified_if": "the concrete result that kills it",
  "must_beat": "the incumbent it replaces or improves" }
```

A feature that can only be defined *after* seeing the outcome is fitting, not hypothesising. The
`direction` field exists to stop a sign flip being rediscovered as a finding.

## THE FOUR BARS — a proposal must clear all of them

1. **The base rate.** Not 50%. Green is 51%, HOD-first 48%. Beating 50% means nothing.
2. **The simplest rule on the same inputs.** v14.91 shipped a one-liner that beat its own logistic
   regression 77–74. If a proposal cannot beat a one-liner, the one-liner ships.
3. **The incumbent.** New must beat shipped, not merely beat chance.
4. **The shuffle test.** The same K proposals re-scored against randomly permuted outcomes, 250+
   times, to learn what the *best of K* looks like under noise. A proposal beats **that**, not the
   null for a single test.

Then one classification, which the first run of this harness proved is necessary:

5. **NEW SIGNAL or FILTER?** Two proposals cleared all four bars at 90% and 84% — and both were
   *subsets of the incumbent's firing days*. They were not finding anything new; they were selecting
   the days the shipped rule already works. That is valuable, but it is a **confidence modifier**.
   Called a new predictor it double-counts one edge.

## THE VERDICT LADDER — nothing reaches the face on a backtest

    REJECTED     fails a bar. Recorded with the reason, so it is not re-proposed monthly.
    PROVISIONAL  clears all bars. Goes to the forward ledger. NOT to the face.
    CANDIDATE    survived 40+ LIVE sessions at its claimed rate.
    SHIPPED      forward evidence, then the face — with its n and CI in the hover.

**A backtest has never been sufficient in this project and is not sufficient here.** The 92% figure
that stood on the face until v14.88 came from a study that picked whichever extreme *turned out* to
print first. It looked rigorous. It was a decision made after the answer was known.

## WHAT THE LLM IS ACTUALLY FOR

Not evaluation — the harness does that, deterministically, the same way every night. The LLM is a
**hypothesis generator over a space too large to enumerate by hand**: the interaction of the gamma
book (four books, ranked nodes, walls, flip, regime) with the candle decomposition (five legs, three
shape fractions, six spans). That is where a human stops looking and where the value is.

Its second job, during Phase 1, is **integrity**: read the night's recording and report what is
missing, stale, or self-inconsistent, before six months of accumulation turn out to have a hole in
them.
