# lsDeltaProfile 2.34.0 - the chosen A and its circle (2026-11-07 night)

Rassul ~22:10 CT: "i'll go with your recommendations, show me mockup". Engine: DeltaIntel.h 1.1.0 (= lra.dp_intel 1.2.0).

## The A
Bullish: one of the CANDLE's own 3 biggest delta rows (the 90-min profile's row size), a selling row, >= 2.0x the candle's average row;
same candle only; green close above the node; node at / below the open and below 1/2 of the range (in the lower wick); close in the top
1/3; the bid POC or the volume POC in the bottom 1/3; his 3-bar ZigZag pivot rule. Bearish mirror (ask POC / volume POC in the top 1/3).
The 90-min top-3 rows now give I only. Old A rules: DINTEL_A_MODE 1 (wick) / 0 (researchA).
Measured (7 markets, 249 days, 9-min hold): 73% right, 72% / 73% on the two halves, ~4 per day per market (random candle 56-59%).

## On the chart
- A hollow CIRCLE on each A candle at the node price, winner's colour, one per candle (the strongest node).
- The forming candle's A = a thin live circle, no label, never recorded; final at the close. A closed circle never moves.
- MARK: a short stroke on top of a green circle / under a red one when the attackers' own POC sat in the absorbing third (acc, at the
  close) or an I the same way followed within 5 candles (ai, added later). Note: acc holds on ~95% of circles.
- Label LEFT of the circle: "A? 3.9x 7745.75" - "A" without "?" only where the nightly A gate shows it (SHOW + its context).
  Labels never overlap (step up / down a row, else the circle is drawn without its label).
- The other letters keep the 2.33.0 gate (letter in front of the profile ratio).

## Records / status
DC|version|root|side|candle_end|lo|hi|tick|ratio|acc (the circle), DM|version|root|side|circle_end|mark_end|acc or ai (the mark).
Status line A_CIRCLES (decided, marks acc / ai, drawn, marked, no room, off the chart, live, read back).

## Gate (dp_intel 1.2.0 re-run with the new A)
A "?" in ES (mom_hi + swing_old), NQ (bo_with), CL (bo_against + own_good), GC (at_pivot + mom_hi), HG (bo_with + bo_against),
NG (mom_hi + swing_old); EU LOG. Other letters unchanged. Nightly: res["circle"] scores the circle and the mark per market and FLAGS
either under 65% on 20+ cases of the last 20 days (none now).

## Tests
tests_dintel/t_dintel.cpp: parity vs Python 4209 / 4209 letters on 242 sessions (11,370 checks), circle / mark parity (956 circles),
live probe = closed-candle A (1,043 probes), no repaint (circle fixed, mark only added), label layout, DC / DM lines. Fixtures in
tests_dintel/fx (7 sessions, 368 checks). lra test_dp_intel 6 / 6.
