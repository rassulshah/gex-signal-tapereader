# spec v11.5 — accumulation measured in DOLLARS (shadow), and one record per roll STEP

Status: PROPOSED · written 2026-08-20 after the live session · user-directed ("see support and resistance
rolling up and down knowing their impact on price") · **no face change in this build**

## 1. The problem, with today's numbers

`mapNodeState` reads accumulation from **%King** — a ratio whose denominator moves. Two consequences, both
observed live on 2026-08-20:

- **The King can never accumulate.** %King of the King is 100 by definition, so its 15-minute rate is pinned
  at 0 and its state is always `hold`. A roll INTO the King therefore cannot fire a transfer. At 10:18–10:54
  the King's dollar magnitude went **112,611 → 142,214 (+26%)** while the Map printed `765 hold, m15 0`.
- **Phantom dissipation.** While the King grows, every other node's percentage shrinks even if its dollars are
  flat. At 10:46 the Map read `766 dec −14 · 764 dec −19 · 763 dec −12` — the whole floor side "bleeding"
  during a session in which the floor was in fact rolling UP into 765.

Result: **44 ceiling transfers to 1 floor transfer** in a full session (the lone floor roll, `764→765`, was
caught at 08:48 before the King settled there). The asymmetry is an artefact of the denominator, not the tape.

Second, unrelated defect in the same feature: `map.transfer` records **every bar a roll stays active**, so
19 distinct steps became **45 records** (`ceil 767→768` alone counted 8 times). The observations are not
independent and n is inflated — the exact trap the review process exists to prevent.

## 2. What changes

**(a) A dollar-based accumulation reading, computed beside the current one, non-voting, off the face.**
The feed already carries absolute exposure per strike (`l[].v`, surfaced as `wall.abs`). Add to the node
ledger, per node, a series of ABS dollars alongside the existing %King series, and from it:
`absM15` (15-minute % change in dollars), `absFromPeak`, `absState` (acm | dec | gone | hold) using the
same ACM_UP / ACM_DN / ACM_DROP thresholds applied to the dollar series.
Recorded per bar as `nodeAbs` on the existing map/ledger records. **`mapNodeState` and everything on the face
keep using %King unchanged** until the evidence says otherwise — this is a SHADOW reading, the pattern
already used for drift.

**(b) One record per roll STEP.** `map.transfer` fires a record only when `(side, from, to)` changes — i.e.
on the step itself — with `stepT`, `barsActive` and, when it ends, `resolvedBars`. Repeats while the same
step stays active are suppressed. A parallel `map.transferAbs` records what the DOLLAR reading saw for the
same bar, so the nightly can compare: same step, different step, or a step only one basis could see.

**(c) The edge path, recorded not displayed.** Per bar, append the current floor and ceiling STRIKE to a
session series (`FCHIST` already samples this — it is read by nothing). Derive `edgePath{flr:[{k,t}...],
ceil:[...]}` = the strike series collapsed to its changes, so a roll is a trajectory (763 → 764 → 765) rather
than a pair. Exported per day; no UI.

## 3. What does NOT change in this build

The READ, the Map line, Nodes on watch, PB Entry, Next Stop, the grades, the thresholds, the ledger's existing
%King fields, and every scored feature. Nothing on the panel moves. This build only adds recording.

## 4. How it graduates

The nightly reports, with n and effN, for both bases side by side: floor-vs-ceiling transfer counts, hit-rate
of "did price move the roll's way" per step (not per bar), and how many steps each basis saw that the other
missed. When the dollar basis clears the promotion bar (eff n ≥ 20, 3 walk-forward sessions, no regime flip),
`mapNodeState` switches to it and the face inherits the better reading — through `applyProposals`, not by hand.

## 5. Acceptance / tests (`test_dollar_accum.js`)

1. A King whose dollars grow 26% while its %King stays 100 reads `absState:'acm'` and `state:'hold'`.
2. A node whose dollars are FLAT while the King grows 26% reads `absState:'hold'` and `state:'dec'`
   (phantom dissipation reproduced and separated).
3. A genuinely bleeding node reads `dec` on both bases.
4. `map.transfer` emits exactly ONE record for a step held 8 bars; a new `(from,to)` emits a second.
5. `edgePath` collapses 40 bars of floor samples 763,763,764,764,764,765 into three steps with timestamps.
6. Fixtures use the live 2026-08-20 ladder; the %King path is byte-identical to v11.4.4 output.

## 6. Open question for the user (not decided here)

Whether the ledger's `firstT` bug (native strikes report `born: 0`, epoch) is fixed in the same build — it
makes the `fresh` flag on PB Entry meaningless for native strikes. Small, related, but a separate change.
