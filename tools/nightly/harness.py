#!/usr/bin/env python3
"""
THE NIGHTLY HARNESS — the part that says NO.

An LLM asked to find predictive features in 282 sessions will find one every single night. That is
not a flaw in the LLM; it is arithmetic. Test twenty ideas against a 51% base rate and the best of
them lands near 60% on noise alone. So the loop is split:

    THE LLM PROPOSES.  THE HARNESS DISPOSES.  The LLM never sees a result before its
    hypothesis is locked, and never has a vote on the verdict.

Every proposal is scored against FOUR bars, and must clear all four:
  1. THE BASE RATE — not 50%. Green is 51%; HOD-first is 48%. Beating 50% means nothing.
  2. THE SIMPLEST RULE using the same inputs. v14.91 shipped a one-line rule that beat its own
     logistic regression 77-74. If a proposal cannot beat a one-liner, the one-liner ships.
  3. THE INCUMBENT. A new feature must beat what is already on the face, not merely beat chance.
  4. THE SHUFFLE TEST — the one that actually bites. The same K proposals are re-scored against
     RANDOMLY PERMUTED outcomes, many times, to learn what the BEST OF K looks like under pure
     noise. A proposal must beat that null, not the null for a single test.

⚠ WALK-FORWARD ONLY. Random cross-validation on a time series leaks the future across the fold
boundary and always flatters. Refit forward, score forward, never both ways.
"""
import math, random, statistics

def base_rate(y): return max(sum(y)/len(y), 1-sum(y)/len(y))

def walk_forward(rows, y, call, start=120, step=10):
    """`call(row, history)` -> 1 / 0 / None(silent). Only ever sees rows BEFORE the one it scores."""
    P=[]
    for i in range(start, len(rows)):
        c=call(rows[i], rows[:i])
        if c is None: continue
        P.append((i, c, y[i]))
    if not P: return None
    hit=sum(1 for _,c,t in P if c==t)
    return dict(n=len(P), acc=hit/len(P), fires=len(P)/(len(rows)-start))

def shuffle_null(rows, y, calls, start=120, iters=400, seed=11):
    """The distribution of the BEST accuracy across K proposals when the labels are meaningless."""
    rnd=random.Random(seed); best=[]
    for _ in range(iters):
        z=y[:]; rnd.shuffle(z)
        b=0.0
        for c in calls:
            r=walk_forward(rows, z, c, start=start)
            if r and r['n']>=20: b=max(b, r['acc'])
        best.append(b)
    best.sort()
    return dict(p50=best[len(best)//2], p95=best[int(.95*len(best))], p99=best[int(.99*len(best))])

def verdict(res, base, simplest, incumbent, null95):
    """The ladder. Nothing reaches the face on backtest alone — PROVISIONAL is as far as it goes."""
    if res is None or res['n']<30:            return 'REJECTED', 'too few firings to judge'
    a=res['acc']
    if a <= base + .02:                       return 'REJECTED', 'does not clear the base rate'
    if simplest is not None and a <= simplest+.01:
                                              return 'REJECTED', 'a one-line rule on the same inputs does as well'
    if incumbent is not None and a <= incumbent+.01:
                                              return 'REJECTED', 'does not beat what is already shipped'
    if a <= null95:                           return 'REJECTED', 'inside the best-of-K noise band'
    return 'PROVISIONAL', 'clears all four bars — goes to the forward ledger, NOT the face'


def relation(cand_days, incumbent_days):
    """
    ⚠⚠ IS THIS A NEW SIGNAL, OR A FILTER ON THE OLD ONE? The harness missed this on its first run
    and the miss mattered. Two proposals cleared all four bars at 90% and 84% — and both turned out
    to be SUBSETS of the incumbent's firing days. They were not finding anything new; they were
    selecting the days the shipped rule already works on. That is genuinely useful, but it is a
    CONFIDENCE MODIFIER and must be described as one. Called a new predictor it would double-count
    the same edge and inflate every downstream number.
    """
    if not cand_days: return 'EMPTY', 0.0
    inter=len(cand_days & incumbent_days)/len(cand_days)
    if inter > .95: return 'FILTER',  inter      # selects WHEN the incumbent works
    if inter < .20: return 'NEW',     inter      # fires where the incumbent is silent — additive
    return 'OVERLAPPING', inter                  # entangled; report both, ship neither alone


def duplicate_of(cand_days, other_days):
    """Two proposals that fire on the same days are one proposal. 74% overlap on the first run."""
    if not cand_days or not other_days: return 0.0
    return len(cand_days & other_days)/min(len(cand_days), len(other_days))


def subset_null(incumbent_hits, k, iters=3000, seed=3):
    """
    ⚠⚠ THE BAR A FILTER MUST CLEAR, AND THE HARNESS SHIPPED WITHOUT IT.

    A FILTER is a subset of the incumbent's firing days, so it INHERITS the incumbent's edge. The
    shuffle test cannot see this: shuffling labels destroys that edge, so any subset that keeps it
    looks miraculous. Comparing to `incumbent + 1pp` is no better — it ignores that a small subset
    of a 79.5% rule scatters widely by luck alone.

    Measured 2026-08-30: on the incumbent's own days, a RANDOM 24% subset (n=39) has a median of
    79.5% and a p95 of 87.2%. Four prior-day-value-area combinations scored 82.1 / 82.2 / 82.8 /
    83.7% — all inside that band, and mutually contradictory (above-VAH and in-value are disjoint
    subsets that scored the same). Without this control the harness called all four PROVISIONAL.

    `incumbent_hits` = [1/0 correct] over the incumbent's firing days. Returns the p95/p99 a subset
    of size k reaches on luck alone.
    """
    import random
    rnd=random.Random(seed); out=[]
    k=max(8, min(k, len(incumbent_hits)))
    for _ in range(iters):
        s=rnd.sample(incumbent_hits, k)
        out.append(sum(s)/k)
    out.sort()
    return dict(p50=out[len(out)//2], p95=out[int(.95*len(out))], p99=out[int(.99*len(out))])
