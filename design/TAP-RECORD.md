# THE TAP RECORD — the level study (R-25) — DESIGN, on paper, nothing built (2026-09-08, evening)

> Operator, 2026-09-08 15:3x CT: *"are you running a study on the gamma levels deflection. basically testing spx and
> spy and qqq top 5 gamma levels and their ability to deflect and if they do deflect, under what conditions and if
> they dont deflect, under what conditions."* — *"the purpose of the study is to identify what makes gamma levels work
> as deflectors of price for the purpose of trading reversals from levels."* — *"yes"* to the design.

This is the corpus the registry has been waiting for: **110 of its 193 studies name `corpus: tap record`** (every K1–K6
King question, S0 the King deflection setup, S1–S8, F1–F8 the deflection mechanics, P1–P7 the pullback, H3/H6 which
node printed the extreme) and the tap record does not exist. The roadmap has carried it as *"the TAP record — one row
per fresh tap with the node's condition, both zones, extent, wasSessionExtreme"* since v15.67. His question is that
item, asked in his own words, and this paper is its design. Nothing here is built; every element is his call, one at
a time, in the order below.

## 0 · Why the answer is "partly" today

Three things exist and none of them can answer *"does the top-5 line deflect, and under what conditions"*:

- **The deflection ledger** (`day.defl`, `recordDeflections`, v10.36) records a row only when `deflectionAt` has already
  CONFIRMED a tap-and-reverse (zone ±0.50 SPY, away ≥ 0.45, held ≥ 2 bars). It has no failures in it — a level that
  broke never gets a row — so it has no denominator. It can say what a deflection was made of; it cannot say how often
  a level deflects. Its outcome (`cont`: +0.30 within 10 bars) is continuation, not the reversal's size.
- **The node ledger** (`ledger[SYM].nodes[].touches`) has both outcomes (deflect / through / stall) but per BAR, not per
  event — 62 of 94 episodes carried contradictory per-bar labels (F-12) — on every node above 20% (not by rank or
  book), and, as of tonight, on a book whose window and breadth changed 34–61 times a session (F-22).
- **The per-bar `node` feature** and the L-rules score a node's condition against a 5/10/20-bar hold, pooled; the
  learn gauge reads them. Same per-bar problem, same book.

What none of them carries: **the book and the rank** (SPX KING / G2–G5, SPY KING / S2–S5, QQQ KING / G2–G5 — the
thirteen ES lines and five NQ lines he trades from), the tap NUMBER (1st / 2nd / 3rd — the Academy's 80 / 66 / 33), an
outcome in a trader's terms (points before the level is violated, the overshoot, held or broke), the QQQ book on NQ at
all, and a CONTROL (what price does at a point that is not a level — F-12 and F-14 both found that without one, a
touch is a coin and a name is a name).

## 1 · The doctrine gate (checked before designing; the tape is the arbiter)

- `execution-doctrine.md`: *enter at the DIRECT TAP*; zones **±0.50 SPY/QQQ · ±5 SPX**; *1st tap ~80% · 2nd ~66% ·
  3rd ~33% · 4th+ spent*; **stop = one node beyond**; 3:1 minimum. → The zone, the tap number and the R measure below
  FOLLOW it, and the record is what will measure the 80 / 66 / 33 (F2.1 is registered on exactly this).
- `node-lifecycle.md`: FRESH / TESTED / DELIVERED / DECAYING; *is it FRESH? is it GROWING?* — both yes or find another
  level; growth across sessions = real, fading = hedge. → The lifecycle state and the two growth clocks (into the
  tap; day-over-day) are conditions, and the two-question gate is the pre-registered hypothesis (H11).
- `reading-heatseeker.md`: King = largest |v|; floor / ceiling = largest below / above spot; gatekeeper = between two
  larger nodes; air pocket = pathway; *never trade the midpoint*. → The role column; the gatekeeper count; the pocket
  on the far side; and the MIDPOINT is the control — the doctrine says nothing happens there, so it is the honest zero.
- `gamma-regimes.md`: +γ fade at nodes, −γ follow. → The regime column, and the outcome measured both ways (held, and
  what the hold paid).
- FINDINGS already measured: F-7/F-8/F-12 (a touch holds ~50% and pins ~87% — pinning IS the level working, the
  question is which touch is the TURN), F-14 (the flush, the clock and the speed matter, the name does not), F-19
  (a node tap was the session's turn 1 of 19), F-21 (acm vs dec did not separate — now flagged mixed-window).
  → The outcome is the TURN, not the hold; the clock, the flush depth and the speed are conditions; and no rate in
  the report is read without its control beside it.

DEVIATION, stated: the doctrine's tap probabilities are about "major nodes" without saying which; this record scores
them by book and rank so the number can be his lines' number. NO DOCTRINE COVERS: the confluence of two books' lines
at one price (SPX 7680 and SPY 766 at the same ES price) — recorded, to be read, not assumed.

## 2 · THE RECORD — one row per tap of an exported level

### 2a · The universe (which lines get rows) — ELEMENT 1, his call

**The exported lines, exactly as the IRT file carries them** — the same price, the same rank, the same second:

    ES   SPXW KING · G2 · G3 · G4 · G5      SPY KING · S2 · S3 · S4 · S5      (10 gamma lines; CW0 / PW0 / FLIP0 are IF structure, tier 2)
    NQ   QQQ KING · G2 · G3 · G4 · G5                                          (5 lines)

`book` = SPXW | SPY | QQQ · `rank` = 1 (the King) … 5 · `pct` = the signed %King the label carries (`G3 -56%`) ·
`usd` = |v| in dollars · `pol` = + / − · the price = Skylit's own ES1 / NQ1 derived price (v15.81) — **the line on his
chart is the level in the record**, to the tick. A line is what it is AT THE TAP (rank 3 at 10:04 may be rank 2 by
10:30); the row keeps the rank at the tap and the rank when the outcome closed.

**Two controls in the same row format, judged the same way**, so every rate has its zero beside it:

- `rest` — every other 0DTE node ≥ 20% of King in the two books (the nodes the ladder shows but the file does not) —
  answers F5.1 *top-5 vs the rest* directly.
- `mid` — the midpoint between two adjacent exported lines (the doctrine's "imaginary support") — the honest zero,
  F1.3. One synthetic level per gap, re-derived each bar.

Not in the universe (his call could add them, one at a time): the IF rows (CW0 / PW0 / FLIP0 — a different book, a
different question; the sweep study already has them), his key levels (PDH / PDL / ON / POC / VA / W — F-14 measured
those on 284 sessions), VWAP.

### 2b · The tap (what makes a row) — ELEMENT 2

On the **ES 3-minute bars** for the ES lines, NQ bars for the NQ lines (the instrument he trades; the panel's measure
rule since v15.08): a row is born the first bar whose range enters the zone **from outside it** — the bar opened
outside the zone, or the previous bar closed outside — with the approach side kept (`from: below` = the line is
resistance; `from: above` = support). The zone is the doctrine's, in chart points: **±5 ES for every ES line** (SPX
±5 and SPY ±0.50 × 10.03 are the same five points), **±20 NQ** (QQQ ±0.50 × 41). A second row for the same line needs
price to have LEFT the zone by a full zone width first (an "inside" bar is the same tap); `tapNo` = 1, 2, 3 … for the
line today — the F2 decay question. A tap is written the moment it happens, with the conditions of that second, and
its outcome fields are filled later (the queue pattern the feature records already use).

Why 3-minute and not 1-minute: the whole panel measures on closed 3-minute bars, the doctrine's "the close decides"
(F1.2 shipped) is on those bars, and his IRT chart is 3-minute. Why the wick and not the close for the TAP: the
doctrine's entry is the direct tap. (The outcome is on closes.)

### 2c · The conditions at the tap (the why-vector) — ELEMENT 3

Stamped from the SAME second's book and bars, every field nullable, nothing invented:

| group | fields | the study it serves |
|---|---|---|
| identity | `book` `rank` `pct` `usd` `pol` `role` (King / floor / ceiling / gatekeeper / other, by `reading-heatseeker`) | K1, F5, F6, S2 |
| lifecycle | `tapNo` · `state` FRESH / TESTED / DELIVERED / DECAYING (today's touches of this line; decaying = |v| down ≥ 20% from its session peak with no touch) | F2, F3, K5.2 |
| growth | `d5` `d15` `d60` (% change of |v| over 5 / 15 / 60 min) · `born` (crossed 20% of King today, and when) · `dod` (|v| vs the same strike yesterday at this clock: real vs hedge) | F4, S6, K5.3, S0.5 |
| geometry | `kingDist` (pts to the book's King) · `gates` (nodes ≥ 20% between spot and the King) · `pocketFar` (pts to the next node ≥ 20% on the far side) · `otherBook` (the other book's exported line within ±5 pts: yes / which) · `ifWall` (CW0 / PW0 / FLIP0 within ±5) · `keyLevel` (his list within ±5) | K5.4, S4, P5, X4 |
| approach | `from` · `speed` (pts over the last 5 bars, and in ATR) · `flush` (the wick's depth past the line, pts) · `barsFromLast` (bars since the last exported line was touched) | F8, F1.4, F-14's flush and speed |
| context | `regime` (net γ sign; the day type read) · `trinity` (3/3 · 2/3 · split at the tap) · `kroll` (the King's roll state today, per book) · `latch` (a crown flap in progress) · `proj` (Skylit's projection regime / target, when on) | F6, F7, K4, K2.6/7 |
| clock | `ct` · `bucket` (08:30–09:00 · 09:00–10:30 · 10:30–13:30 · 13:30–14:00 · 14:00–15:00) · `sinceOpen` · `beforeFirst` (filled at the close: was the tap before the day's first extreme?) · `dayColour` (green / red so far) · `rangePos` (where in the day's range so far, 0–100) · `vwapDist` | K3, P3, F-14's clock, P7.3 |

### 2d · The outcome (in trading terms) — ELEMENT 4

Measured on closes after the tap bar, over **10 bars (30 min) and 20 bars (60 min)**, and "until violated":

- `broke` — a close beyond the line by more than the zone on the far side (the doctrine's "through the zone");
  `brokeBar`.
- `mfe` — the furthest close AWAY from the line (the reversal's size in points) before the break, if any; `mfeBar`,
  `mfeMin` (minutes to it).
- `mae` — the deepest wick PAST the line before the reversal (the overshoot; the beach-ball depth; where the stop
  must live).
- `reach` — did the move reach the next exported line on the far side (the doctrine's target)? which, and in how many
  bars.
- `R` = `mfe` ÷ (`mae` + zone) — the trade the doctrine describes (tap entry, stop one node beyond): the 3:1 check.
- `verdict` — **DEFLECT** = not broke within 20 bars AND mfe ≥ 2 zones (10 ES pts) · **PIN** = not broke, mfe under
  that (the level worked, price sat on it) · **BREAK** = broke. This is F-21's language (pinning is the level
  working; the trader's question is the turn) made per event.
- filled at the close: `wasExtreme` (the tap printed the day's HOD or LOD), `resumed` (for a pullback tap — a new
  extreme in the leg's direction within 30 bars; P6), `rankAtClose`.

The same fields for `rest` and `mid` rows, so DEFLECT % at a G3 is always read against DEFLECT % at a midpoint.

### 2e · Where it lives — ELEMENT 5

A new IndexedDB store `taps` (keyed `sym|book|rank-or-k|tapBar`), written at the tap and UPDATED as the outcome
fills (the F-20 lesson: the archive first, the localStorage mirror is a window); mirrored into the day file as
`taps: { ES:[…], NQ:[…] }` beside `defl`; read by the nightly (`tools/nightly/run.py`) like `feat` and `defl`;
`__gptsDebug.taps()` shows today's rows live. The book beneath it is the one-window 0DTE chain (v15.84) — **the
clean sample begins 2026-09-09.** No back-fill of the G / S rows (their ranks and %s before v15.84 came off a
mixed-window book); the KING rows CAN be back-filled from the archive's `snaps` + the ES bars for 20 sessions
(the King is window-proof) — a separate, later, clearly labelled table, if he wants it.

## 3 · THE REPORT — nightly, on Analysis — ELEMENT 6 (mockup before anything is drawn)

One table, `by book × rank`, then split by each condition group, every cell with its n and a 95% interval, and the
`mid` and `rest` rows always beside it:

    line          n    DEFLECT   PIN    BREAK   med mfe   med mae   R     reach     vs mid
    SPX KING     31      35%     42%     23%     14.0      3.5     1.6    48%      +12
    SPX G2–G5    88      27%     45%     28%      9.5      4.0     1.1    39%       +4
    SPY KING     29      …
    SPY S2–S5    71      …
    QQQ (NQ)     …
    rest        140      …
    mid         160      23%     …                                                   0

then the splits his question names — **FRESH vs TESTED vs DELIVERED · growing into the tap vs flat vs shedding ·
+γ vs −γ · 1st / 2nd / 3rd tap · the clock buckets · before / after the first extreme · confluence (the other book's
line within 5 pts) vs alone · King distance · fast approach vs grind** — as rows of the same table, and a "conditions
that separate" line naming only the splits whose interval clears the pooled rate (the review's own rule; a cell under
minN says THIN, never a number). The registry's rows (K1.1, S0.2–S0.4, F2.1, F3.1, F4.1, F6.1, F7.1, F8.1 …) read
from this table; each moves to READ at its minN.

## 4 · THE HYPOTHESIS, written before the data — H11 (register it the day the record starts)

*"Among the exported lines, a FRESH line that is GROWING into the tap (d15 > 0) DEFLECTS (by 2d's verdict, 20 bars)
more often than a line that is either tested or shedding, and both more often than the midpoint control; predicted:
fresh+growing ≥ 40%, mid ≤ 25%, at n ≥ 40 per cell. Refute if the fresh+growing cell's Wilson high is under the
midpoint's rate at n ≥ 40."* Read once, at minN, on sessions from the register date. Registered as a NULL if it fails
— that too is an answer to his question.

## 5 · The order of building — one element per ask, nothing before his word

1. the universe (2a) — does the record carry the `rest` and `mid` controls, and only the exported lines otherwise?
2. the tap (2b) — 3-minute bars, ±5 ES / ±20 NQ, the leave-the-zone rule for a second tap.
3. the conditions (2c) — the table above; anything he wants added or struck.
4. the outcome (2d) — the DEFLECT / PIN / BREAK verdict and the 10-pt bar; the two windows.
5. the store (2e) — `taps`, the day file, the nightly.
6. the report (3) — a MOCKUP rendered from real rows first (a week of them), then the Analysis tab.
7. H11 registered; the first read at n ≥ 40 per cell (≈ 2–3 weeks of sessions at ~15 exported-line taps a day).

What it costs: one recorder module (the tap detector + the outcome filler, ~250 lines, all pure and testable on
fixture bars), one IDB store, one export key, one nightly reader, one table. What it does not touch: the face (until
the report), the existing ledgers (they keep running; the tap record does not replace `defl` until it has proved
itself against it), the Rec discipline (the report goes on the face through R-25).
