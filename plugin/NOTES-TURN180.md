# lsTurn 1.8.0 - Absorption and Initiative bars (2026-11-24)

## Requests
- 17:51 "so basically heavy ask volume in the bottom 25% > 1.5 on the dnpivot right ?"
- 17:52 "lets do it. i think we have an iniaive tag we can re implement that"; "the absorbtion indicator can be updated to have two bars per candle , 1 for absorbtion and 1 for initiative"
- 18:01 "i want more detail for the intiative buyers .. we have icebergs for passive buyers so what would be for initiative buyers"
- 18:53 "build" (turn-initiative-mockup.html v3)

## What changed
- Every closed candle: LEFT bar = Absorption at its stronger end, RIGHT bar = Initiative at its stronger end (up = at the high, down = at the low).
- Initiative = the counter side's heaviest single price in the end quarter (sells at the high, buys at the low) / its average over the 20 candles before.
  Coloured from 1.5 when those lots beat the attackers' heaviest price there AND the candle closed outside that quarter: buyers at the low green,
  sellers at the high red; grey otherwise. Absorption is unchanged (attackers' colour, from 2.0).
- A Stop Run of a named level takes a free column on its half (both taken: it is in the strip only).
- Sweep candles keep Stop Run / Absorption / Exhaustion; their right-hand turn bar is the new Initiative of the end swept; the strip's
  Initiative word follows it (the old Answer 0-3 is no longer drawn; the verdict's internal rule is unchanged).
- The read of an Initiative candle (under the mouse, else the last closed one):
  "9:30 AM  Initiative - buyers lifted the offer at the bottom" / Initiative: lots at price (x) / How: Burst x pace (market level) ? | Swept N prices (4) ? | Biggest N / Against: the attackers there.
- How comes from the order feed (fills grouped by aggressor ID); kept per candle in TurnPane\cread-<MKT>-<spb>.txt as "N" lines (older readers skip them).
  Burst = the most lots in one 5-second step in the quarter / the side's lots per 5 s over the 20 candles before (10+ known).
  Burst levels (90th percentile, research): CL 8.6, ES 5.4, EU 12.1, GC 10.4, HG 13.0, NG 9.5, NQ 7.7.

## Evidence (research 2026-11-24, decided at the close)
- New 10-candle lows / highs, 7 markets: 1 ATR the turn's way first - initiative only 66% (n 711, halves 66/67), absorption only 61%, both 61%, ordinary 57%; all 7 markets better (ES weakest 61 vs 59).
- Order-by-order (267 initiative candles): burst at the market level 83% vs 67% (n 30, "?"); swept 4+ prices 76% vs 67% (n 55, first half no difference, "?"); slicing, stop clusters, offers pulled: no difference (left out).

## Tests (all pass)
- logic 3,287,189; golden 2,017; host 33,260 (two-bar layout, colours, labels, heights, the Initiative read on hover, no overlap, no repaint after removal); soak OK (no growth); read 136 / 262; abs 31; cr 15; t_ini180 31 (incl. the real GC 9:30 candle: 135 lots at 3529.9 vs 24).
- Order details replayed from IRT events = the research trades store (ES 11-20 10:27 / 10:33: biggest 105 / 48, sweep 6 / 4, burst 317 / 231).
- Real SDK C++14 / C++17: 0 errors, 0 warnings.

## Not verified - needs his IRT
- The look on his charts (ChartView after the restart). The How line needs the pane on the chart while the candle trades (no order feed before that).
- Burst levels are in-sample per market (LEARNING, "?").
