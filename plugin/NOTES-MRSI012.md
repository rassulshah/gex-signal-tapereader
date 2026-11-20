# lsMorrisRSI 0.1.2 (2026-11-20) - divergence vs the swing that was taken out

Request (CT, 2026-11-20): his ES screenshot of the 11-19/20 overnight - a clear bullish divergence was not marked: price low ~7,148 at ~11:15 PM (RSI ~20), then a lower low ~7,145 at ~1:18 AM (RSI ~47). 08:00 "go" on the fix.

Why 0.1.1 missed it
- 0.1.1 compared only two CONSECUTIVE 3-bar ZigZag pivots at most 30 candles apart. Smaller ZigZag lows sat between the two lows and they were ~44 candles apart.

What changed
- MorrisRsiLogic.h: when a pivot low is confirmed it is compared with the prior swing low it UNDERCUT = the lowest confirmed pivot low within the last 60 candles (DV_LOOK, the new one excluded; ties = the later one), provided the new low is lower. Bullish Dv if the RSI at the new low is higher and the RSI at the prior low <= 40. Mirror for highs (highest pivot high within 60, new high higher, RSI lower, prior RSI >= 60). Still decided when the 2nd pivot is confirmed, never changed after; one Dv per pivot. Search is newest-first and stops at 60 candles (bounded work).
- MorrisRSI.cpp: version 0.1.2 (header, MRS_VERSION, description, setVersion). Record file format unchanged (old records still load).

Count check (research 3-min bars Sep 1 - Nov 20, divergences whose 2nd pivot is in RTH, per session)
| Market | sessions | old 0.1.1 | new 0.1.2 | ratio | new + 5-pt RSI gap |
|---|---|---|---|---|---|
| ES | 58 | 1.95 | 2.02 | 1.04 | 1.33 |
| NQ | 49 | 2.10 | 2.08 | 0.99 | 1.18 |
| CL | 47 | 1.64 | 1.72 | 1.05 | 1.15 |
| GC | 49 | 1.33 | 1.43 | 1.08 | 1.06 |
| HG | 46 | 1.20 | 1.28 | 1.07 | 0.80 |
| NG | 47 | 1.32 | 1.49 | 1.13 | 0.91 |
| EU | 58 | 1.47 | 1.47 | 1.00 | 0.88 |
Nowhere near triple -> no minimum RSI gap added. (RTH windows CT: ES/NQ 8:30-15:00, CL/NG 8:00-13:30, GC 7:20-12:30, HG 7:20-12:00, EU 7:20-14:00.)

His case (research bars, labels = bar START, so IRT shows +3 min): 0.1.2 marks bullish Dv 11:15 PM low 7,149.75 RSI 20.3 -> 1:27 AM low 7,144.50 RSI 42.9 (44 candles apart). 0.1.1 marked nothing there.

Tests
- tests_morrisrsi/t_morrisrsi.cpp 39/39 (ASan / UBSan, -Werror). New: undercut-swing fixture (smaller lows between, Dv from the deep low), ES 11-19/20 real-bar fixture (mrsi_es1120_bars.txt) marked, consecutive case still marked from the 1st low, mirror bearish case, 60-candle limit (38 and exactly 60 apart marked, 61 not), one Dv per pivot, restart replay (fresh engine at many cut points, record merged by 2nd-pivot stamp like the host) = one-pass record with no decision changed.
- C++ engine vs the Python reference on all Sep - Nov bars in 7 markets: identical divergence lists (ES 451, NQ 358, CL 349, GC 353, HG 284, NG 253, EU 358).
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors / warnings in MorrisRSI.cpp / MorrisRsiLogic.h (only the same 28 SDK-only RT_LONG diagnostics as 0.1.0 / 0.1.1).

Risks / not verified
- Divergences already in a dv-<MKT>-180.txt record stay as they were (never repainted); older history is not re-marked with the new rule unless that record file is removed (not done - he decides).
- Not seen on his chart yet: needs IRT closed (auto build / install) and reopened. 0.1.1 is also still pending install; 0.1.2 includes it.
