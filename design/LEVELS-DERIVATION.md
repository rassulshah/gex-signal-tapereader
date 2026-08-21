# How every level is derived, and what guards it

**Status 2026-08-21 · v11.24.** Written after a week in which we shipped levels that were wrong, and
after finding the single defect that made them wrong. Every claim here is either *verified against
InsiderFinance's own published table* or explicitly marked as unproven.

---

## 0. THE DEFECT THAT INVALIDATED EVERYTHING BEFORE v11.24

The feed's `levels` field is a **~390-entry time series**, and the LAST entry is the **current,
still-forming bar**. We were reading `levels[levels.length-1]`.

Measured on the live SPY feed, counting strikes where `|net| < v` (i.e. carrying a real call/put split):

| snapshot | strikes with a real split |
|---|---|
| 0 | 84 |
| 195 | 78 |
| 388 | 82 |
| **389 (the one we read)** | **0** |

During RTH that final frame gets populated properly. **After the close it degenerates to `net = ±v` on
every strike.** Confirmed on both the self-fetched sets and the passive feed — `callPut()` reported
`lt:0, eq:208, gt:0, decomposable:false`.

Consequences, all of which we saw and misdiagnosed:

* Every strike classified as **100% call or 100% put** — a binary, because `call=(v−net)/2` collapses to
  `v` or `0` when `|net| == v`.
* Our call wall picked **765**, a strike their book shows as **heavily put-dominant** (put 1300M vs call
  342M).
* Values appeared to **swing 20×** between fetches. They did not. I was comparing a degenerate frame
  against a complete one.

**Fix:** `pickSnapshot()` walks back to the most recent frame that carries a decomposition, bounded by
`SNAP_MAX_STEPBACK` (8). Beyond that bound it returns the newest frame **flagged**, because a frame from
twenty minutes ago wearing a current timestamp is worse than admitting we have nothing.

---

## 1. THE FEED

Row shape: `{k, v, d, net}` — strike, magnitude ($), a direction flag, and net ($).

`__gptsDebug.feedShape('SPY')` reports the shape; `__gptsDebug.callPut()` reports whether the chosen
frame decomposes.

**Sign convention — the trap.** Their `net` is call−put (negative on a put-heavy strike). **Ours is the
opposite.** Our 760 reads `+278M` and is the put wall; their equivalent reads negative. So:

| level | their rule | **our rule (mirrored)** |
|---|---|---|
| CR | most **positive** net above spot | most **negative** net above spot |
| PS | most **negative** net below spot | most **positive** net below spot |
| Mag | largest \|net\| | largest \|net\| (sign-free) |

Invert this and both walls swap sides **while still looking like plausible levels**. Nothing on the face
would look wrong. `gexSanity` checks `crAboveSpot` / `psBelowSpot` specifically to catch it.

---

## 2. THE RULES — VERIFIED, NOT INFERRED

Extracted their full strike table (**782 rows** on All-expirations, **585** on 1W) and tested every
candidate definition against the rows **they tag**:

| rule | strike | their tag |
|---|---|---|
| most POSITIVE net above spot | 7775 (1W) / 7900 (All) | **CALL WALL** ✓ |
| most NEGATIVE net below spot | 7640 (both) | **PUT WALL** ✓ |
| largest \|net\| anywhere | 7645 (both) | **Peak GEX / Magnet** ✓ |
| max CALL gamma above spot | 7650 | ✗ |
| max call OPEN INTEREST above spot | 8800 | ✗ |

Confirmed across **two different filters where the call wall itself moves** (7775 → 7900) while the put
wall and magnet stay put. Rules that survive a changing input are far better evidence than rules fitted to
one snapshot.

**Their prose credits open interest. The tagged row does not follow it. The table is ground truth.**

---

## 3. LEVEL BY LEVEL

| level | window | rule | status |
|---|---|---|---|
| **CR** | `week` | most call-dominant strike above spot | rule verified; needs re-check on a non-degenerate frame |
| **CR0** | `dte0` | same, today's expiry only | verified absent when the 0DTE book has no call side |
| **PS** | `week` | most put-dominant strike below spot | **matches theirs exactly (760)** |
| **PS0** | `dte0` | same, today only | 2 points off — see §5 |
| **Mag** | `week` | largest \|net\| | **matches theirs exactly (760)** |
| **HVL** | — | zero gamma | **not computable — see below** |

**HVL is ruled out empirically, not guessed.** On their own table, cumulating net across strikes from the
bottom crosses zero **once, at 2200**; from the top it gives 7682.86 against their 7674.06. Their method
re-prices exposure at hypothetical spot levels, which needs per-option greeks we do not receive. Ours is
an approximation at best, and it is withheld entirely beyond `HVL_MAX_DIST` (3% from spot) because a
crossing 10% away is a tail artefact, not a flip.

**Not computable at all:** Max Pain and the put/call OI ratio (both need open interest, which the payload
has none of); 1D Max/Min (needs implied vol); Blind Spots (cross-asset).

---

## 4. THE WINDOWS

* `dte0` — today. `exp_mode=current&exp_count=1`
* `week` — through **this Friday**. `exp_mode=week&exp_count=1` (Skylit's own tooltip: *"every expiration
  through this week's Friday"*). Verified live: returned Aug 20 + Aug 21 on a Thursday.
* `wk7` — rolling 7 days, **never displayed**. The control that will eventually say which window price
  actually respected. Forward-only data.

**Ours is NOT their "Next week".** Theirs is the 1W preset = a **rolling 7 days** (their expiry list
checked 0DTE + 1/4/5/6/7DTE and excluded 8DTE). Ours shrinks as the week runs down. **A mid-week CR
difference is a definition difference, not an error** — the sanity check deliberately does not flag it.
The **put wall is the one that must match**: it held at the same strike across every one of their expiry
filters, so a gap there has no window excuse and IS enforced.

**Unresolved: Friday degeneracy.** "Through this Friday" on a Friday = today = 0DTE, so CR would equal
CR0. Agreed plan is to roll to next week once this week is exhausted. **Not yet built** — verify what
`exp_mode=week` actually returns on a Friday first.

---

## 5. THE GUARDS

`__gptsDebug.sanity()`. Every one of these corresponds to a failure that actually occurred:

| guard | catches |
|---|---|
| `snapshotHasDecomposition` | **§0** — levels read from a degenerate frame |
| `crAboveSpot` / `psBelowSpot` | an inverted sign convention — two plausible levels on the wrong sides |
| `levelsInRange` | a level outside the strikes actually read |
| `noDuplicateStrikes` | the duplicate-row bug |
| `levelsNearPrice` | tail artefacts (the 479.7 and 687 class) |
| `setFresh` | a stale set silently showing yesterday's walls |
| `putWallMatchesTheirs` | drift against the one level with no window excuse |

Guards inside the computation itself:

* **Lean test** — a wall is refused if the best candidate on that side does not actually lean that way.
  Killed the phantom `CR0 792` on an expiry-day book with a 0.2% call share.
* **Side share** — a side holding <5% of the book's gamma names no wall. Their page reports N/A at ~2%.
* **HVL distance** — a crossing beyond 3% from spot is withheld with its reason.
* **Roster accounting** — no level ever vanishes silently; "none today", "same as CR" and "set not
  loaded" are three distinct statements.

---

## 6. OPEN, HONEST

1. **CR is not yet re-verified on a clean frame.** The rule is confirmed on their data; our own call wall
   was computed from the degenerate frame every time we checked it. **Re-run the comparison during RTH.**
2. **PS0 reads 762 against their 760.** Partly explained: our spot is ES-derived after hours
   (ES 7665.25 ÷ ratio = 762.565) against their SPY cash close of 761.14, and that 1.4-point gap decides
   whether the 762 strike counts as below spot. Needs confirming during RTH, when both read the same cash.
3. **Magnitude gap.** Our per-strike values ran ~10–20× smaller than theirs. May be a units convention
   (per-1% vs per-point) — a uniform scale factor would not change any ranking, so it is not urgent, but
   it is not understood either.
4. **UI immunity holds.** These levels come from the network payload, not the DOM, unlike the ladder
   scraper that has broken repeatedly. A Skylit redesign does not touch them.

**The honest summary: PS and Mag are corroborated against an independent source. CR is computed by a
verified rule from a frame we now know was bad, so it is unproven until re-checked in live hours.**
