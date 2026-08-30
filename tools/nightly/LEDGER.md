# THE VERDICT LEDGER

Every proposal ever scored, with its verdict and the reason — **including the rejections**, so the
same idea is not re-proposed every month. Newest first.

## 2026-08-30 · THE GAMMA REGIME · not tested — an integrity failure found first

> "If we are in a positive gamma vs negative gamma regime would be an important filter ... Is there
> any way that can be obtained and included in testing"

**It is recorded, and the recording is worthless.** `bk.neg` reads *positive gamma on 284 of 284
snapshots across all 9 sessions* — because `neg:false` was **hardcoded** on the trinity read path.
Not a calm market: a constant.

⚠⚠ **A CONSTANT THAT LOOKS LIKE DATA IS WORSE THAN A GAP.** Had this not been checked, the first
regime study would have concluded "no negative-gamma sessions in the corpus" and moved on, or worse,
found a spurious effect against a field that never varies. Fixed to `null` in v14.92 so the hole is
visible. ⚠ And the contradiction was the tell: `bk.neg` said 100% positive gamma while price sat
**below** the flip on 60% of the same snapshots. **Two recorded fields that should agree, didn't.**

**The usable regime signal is PRICE vs the FLIP (`deriv.zg`)** — real, varying, 40% above / 60%
below, median distance −0.46 pts. Registered as gx-010 and gx-011. ⚠ It is recorded on only 25–70%
of bars per session, which is itself a Phase-1 integrity item.

**Nothing was tested.** With 6 gamma sessions and a broken regime field there is no test worth
running, and running one anyway is how a number nobody can defend ends up on the face.

---

## 2026-08-30 · prior-day VALUE AREA (POC / VAH / VAL) · 12 proposals · targets: green, range, S/R

> "if we open above prior day poc or vah or in between vah and Val, how does that help us predict?
> What about combinations."

**Answer: it does not, on any test run.** n=283 sessions, base green 51.6%.

**Does the open's location predict GREEN/RED?** No.

| open is | n | green | vs base | |
|---|---|---|---|---|
| ABOVE VAH | 115 | 53% | +1 pp | \|z\|=0.3 |
| VALUE UPPER | 53 | 47% | −4 pp | \|z\|=0.6 |
| VALUE LOWER | 46 | 48% | −4 pp | \|z\|=0.5 |
| BELOW VAL | 69 | 55% | +3 pp | \|z\|=0.6 |

Above vs below the prior **POC** — the one line most people use — is 51% against 52%, z=±0.1.

**The steelman also fails.** Profile theory claims range and rotation, not direction. Opening
outside value gave a big-range day **46%** of the time against 50% by construction (z=−1.0) — if
anything slightly *less* trendy.

**And every S/R test loses to its own sham.** A fake level at the same distance, on the wrong side:

| test | real | sham | |
|---|---|---|---|
| the day's HIGH sat at a profile level | 16% | **21%** | the sham does BETTER |
| the day's LOW sat at a profile level | 22% | 19% | inside noise |
| opened outside value → returned to it | 61% | **64%** | the sham does BETTER |

⚠ This matches the earlier, narrower finding (`tools/study-profile.py`, 2026-08-29): a prior POC is
tagged 46.6% of sessions against 46.3% for a sham. **Distance explains these levels, not the levels.**

### ⚠⚠ COMBINATIONS — AND THE RUN THAT EXPOSED A SECOND HOLE IN THE HARNESS

Eight combinations of the open's location with the shipped GD/RD rule. Four came back PROVISIONAL:

    break agrees & open ABOVE VAH   82.1%      break agrees & open IN VALUE   82.2%
    break agrees & open ABOVE POC   82.8%      break agrees & NEAR POC        83.7%

**They are mutually contradictory** — above-VAH and in-value are disjoint sets — yet all four beat
the incumbent's 79.5%. That is the tell. The control that settles it: on the incumbent's own days,
a **RANDOM** subset of the same size has a median of 79.5% and a **p95 of 87.2%**. All four sit
inside the luck band. **All four REJECTED.**

A FILTER inherits the incumbent's edge, so the shuffle test cannot see it — shuffling labels
destroys the very edge the subset keeps. `subset_null()` was added to the harness in response.

**gx-008 re-judged against the new control and SURVIVES**: 90.2% on n=61 against a p95 luck band of
85.2%. It stays PROVISIONAL and still needs forward evidence.

---

## 2026-08-29 · first run · 4 proposals · target: green

Context: base rate **51.6%** · simplest one-liner **72.4%** · incumbent (v14.91) **79.5%** on 72% of
days · shuffle null over 4 proposals **p95 = 64.0%**, p99 = 70.0%.

| proposal | acc | fires | verdict | reason |
|---|---|---|---|---|
| overnight compression | 90.2% | 37% | **PROVISIONAL** | clears all four bars → gx-008, forward ledger |
| narrow IB30 | 84.0% | 31% | **REJECTED** | 74% overlap with the above — the same idea twice |
| overnight expansion | 67.9% | 34% | REJECTED | the one-liner does as well |
| break alone, no momentum | 72.4% | 100% | REJECTED | that IS the one-liner |

⚠⚠ **THE RUN FOUND A HOLE IN THE HARNESS AND THE HOLE MATTERED.** Both survivors cleared all four
bars, and both turned out to be **subsets of the incumbent's firing days** — they select WHEN the
shipped rule works, they do not add a new signal. `relation()` was added to the harness in response.
Reported as a new predictor, either would have double-counted one edge.

⚠ 90.2% on n=61 is **not** a 90% model. The CI is 83–98%, it fires on a third of days, and it has
never been scored forward on a single unseen session. That is precisely what PROVISIONAL means.
