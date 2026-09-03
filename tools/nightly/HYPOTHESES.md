# THE PRE-REGISTRATION BANK

Locked **2026-08-29**, when there were **six** sessions of gamma book. Every hypothesis below is
therefore registered *before the data that will test it exists* — the strongest form of
pre-registration available, and the reason this bank is worth more than anything derived by looking.

**Rules.** Nothing here may be edited after its `registered` date except to record a verdict. A
hypothesis that turns out to need a different definition is a NEW entry with a new id; the original
stays, with its verdict. Changing a definition after seeing data is how a null result becomes a
finding.

`n_needed` is the sessions required before the harness will score it at all.

---

### gx-001 · does the King's side of the open modify the GREEN/RED call?
- **claim** — when the SPY King sits above the open, the GD/RD rule is more reliable on GREEN calls
  and less on RED (and mirrored below).
- **feature** — `sign(king_spy_at_0900 - rth_open)`, interacted with the shipped call.
- **target** green · **direction** agreement raises accuracy · **must beat** the v14.91 rule alone.
- **falsified if** accuracy within ±2pp of the incumbent in both agreement states. **n_needed** 100.

### gx-002 · does the opening-range break run INTO a node or AWAY from one?
- **claim** — a break heading *toward* a top-5 node fails more often than one breaking into open space.
- **feature** — distance from the IB30 break price to the nearest top-5 SPY node, in ATR, signed by
  whether the node lies ahead of or behind the break direction.
- **target** green · **direction** node ahead ⇒ lower accuracy · **must beat** the v14.91 rule.
- **falsified if** the two halves differ by less than 3pp. **n_needed** 100.

### gx-003 · is a King touch different from an ordinary node touch? *(this is Q11)*
- **claim** — at an extreme, a node that is its book's KING reverses price more often than a node
  merely ranked in the top five.
- **feature** — `is_king(node_at_extreme)` from `tri.<BOOK>.top` at that bar.
- **target** deflect-vs-break · **direction** King ⇒ higher deflect rate · **must beat** the 44%
  deflect base measured 2026-08-29.
- **falsified if** the King and non-King deflect rates are within 5pp. **n_needed** 150.
- ⚠ **The counter-example is already on file** and must be respected: node 764 on 2026-08-24 carried
  one deflection AND two breakdowns. The node selects WHERE; it has never been shown to decide WHAT.

### gx-004 · is node mass GROWING into the touch the ex-ante discriminator?
- **claim** — the deflect/break split is decided by whether the node is *gaining* dollars in the
  minutes before price arrives, not by the node's size.
- **feature** — change in that strike's `%King` over the 5 bars before the touch bar.
- **target** deflect-vs-break · **direction** gaining ⇒ deflect · **must beat** gx-003.
- **falsified if** AUC ≤ 0.55 over ≥150 touches. **n_needed** 150.
- ⚠ This is the one that would make deflections tradeable. Measured 2026-08-29: the touch ALONE has
  no edge (t=+0.41 top-5, t=−0.32 kings) because deflect and break are mirror images that cancel.

### gx-005 · does trinity agreement modify every other call?
- **claim** — 3-of-3 book agreement raises the accuracy of whatever call is live; 2-of-3 lowers it.
- **feature** — `trinityRead().n / .of` at the moment of the call.
- **target** green, and hod_first · **direction** 3-of-3 ⇒ higher · **must beat** each incumbent.
- **falsified if** the 3-of-3 and 2-of-3 accuracies are within 3pp. **n_needed** 100.
- ⚠ Must be classified FILTER vs NEW SIGNAL — it is almost certainly a confidence modifier.

### gx-006 · does a King that MOVES early mean a trend day?
- **claim** — the King migrating in the first hour marks a session that closes far from its open.
- **feature** — count of latched King moves before 09:30 (`kingLatchTick`, drawn crown not raw).
- **target** |close − open| in the top tercile · **direction** more moves ⇒ larger body.
- **falsified if** the tercile rates are within 5pp. **n_needed** 120.
- ⚠ Use the DRAWN crown, never the raw tape — the raw/drawn distinction cost four wrong answers on
  2026-08-29 and `tools/study-kingmoves.py` exists to keep them separate.

### gx-007 · do the walls contain the day's range?
- **claim** — the session high/low respect CW0/PW0 more often than a distance-matched sham level.
- **feature** — was the extreme within 1 ATR of CW0/PW0, against a sham level at equal distance.
- **target** extreme-at-wall rate · **must beat** THE SHAM, not the raw rate.
- **falsified if** real and sham are within 3pp. **n_needed** 120.
- ⚠ The sham comparison is mandatory. The profile levels died exactly here on 2026-08-29: a prior
  POC tagged 46.6% of sessions against 46.3% for a sham at the same distance. Distance explained it.

### gx-008 · overnight compression as a confidence modifier *(promoted from tonight's run)*
- **claim** — on days whose overnight range is below its own 60-session median, the GD/RD rule is
  materially more reliable.
- **feature** — `overnight_range_bp < rolling_median_60(overnight_range_bp)`.
- **target** green · **must beat** the v14.91 rule · **classified** FILTER, not a new signal.
- **status** PROVISIONAL 2026-08-29 — 90.2% on n=61 (95% CI 83–98%) against the incumbent's 79.5%.
- **falsified if** the live forward rate falls below 82% over 40 sessions. **n_needed** forward only.
- **re-judged 2026-08-30** against `subset_null()`: 90.2% vs a p95 luck band of 85.2% — SURVIVES.
- ⚠ Needs NO gamma book, so it can be forward-scored starting immediately. It is 74% overlapping
  with "narrow IB30" — the same idea twice, and only this one is carried.

---

### gx-009 · prior-day value area · **CLOSED, NEGATIVE, DO NOT RE-PROPOSE**
- Registered and answered 2026-08-30. Open location vs prior POC/VAH/VAL predicts neither direction
  (all buckets within ±4pp, |z|<1), nor range (46% big-range outside value vs 50% expected), nor
  support/resistance (every test loses to a distance-matched sham, two of them by losing outright).
- ⚠ Eight combinations with the shipped rule all fell inside the random-subset luck band.
- **This entry exists so the idea is not re-proposed.** It is intuitive, widely believed, and dead
  on this corpus.

---

### gx-010 · price vs the GAMMA FLIP as the regime filter
- **claim** — with price BELOW the flip (negative-gamma side) breaks continue and fades get run over;
  ABOVE it (positive gamma) the reverse. The shipped GD/RD rule should be more reliable below the
  flip; deflections should be more reliable above it.
- **feature** — `sign(px - deriv.zg)` at the moment of the call, plus `|px - zg|` in ATR.
- **target** green (as a FILTER), and deflect-vs-break.
- **direction** below flip ⇒ continuation; above flip ⇒ reversion.
- **falsified if** the two sides are within 3pp on both targets.
- **must beat** `subset_null()` at its own subset size — this is a FILTER and inherits the
  incumbent's edge, so the random-subset band is the bar, not the shuffle test.
- **n_needed** 100 sessions with `zg` recorded on ≥80% of bars.
- ⚠ **This is the honest version of his question.** `bk.neg` cannot answer it — see gx-012.

### gx-011 · does the flip's DISTANCE matter, or only its side?
- **claim** — the regime effect is strongest when price is far from the flip and vanishes near it,
  because near the flip dealer positioning is ambiguous.
- **feature** — `|px - zg|` in ATR, bucketed; interacted with gx-010's side.
- **falsified if** the effect is flat across distance buckets. **n_needed** 150.
- ⚠ If gx-010 fails, this does not get tested — a distance effect on a non-existent effect is noise.

### gx-012 · `bk.neg` — BLOCKED, NOT A HYPOTHESIS
- Not testable and never was: `neg` was hardcoded `false` on the trinity read path, so all 284
  recorded samples across 9 sessions say "positive gamma" because the code said so.
- **Fixed to `null` in v14.92** so the gap is visible instead of silently wrong.
- ⚠ **Recording resumes only when the sign has a real source.** The Trinity pane does not expose it.
  Until then gx-010 (price vs flip) is the regime signal, and it is the better one anyway: it is
  continuous, and it is what the doctrine actually describes.


---

## ⚠ SUPERSEDED (v15.54)

**The register is `learning/register.json`** — read by the panel (Testing ②) and by `tools/nightly/run.py`. This file is history; add nothing here.
