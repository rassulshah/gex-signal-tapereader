# lsEffortRewardPane 0.2.0 "Effort Reward v0.2" (2026-11-21)

## Requests (Rassul, CT)
- 11-20: "lets stop here and make this just the effort reward indicator. it calculates the effort at the top 25% of the bar (1st bar) and the bottom 25% of the bar (2nd bar)"; "put the red bar on the opposite side of the horzontal axis"; "from this indicator remove the 1.0 lines".
- "no i dont want signals on this indicator.. we need to have a read instead maybe a t the top a strip that gives the read"; "make sure you mention magnitude".
- 11-21: "The word comes from the quantifier only"; "0.0x is No absorbtion"; "i would say .5 >= and < 1 is light"; "moderate is 1-1.5, 1.5 and higher is heavy"; "i'l go with your reccomendation and all makets".
- "build it. make sure you do an independed code review and no memory leasks etc... then build".

## What it is
New plugin (own pane). Files: EffortRewardPane.cpp (host) + ErPaneLogic.h (namespace erp, SDK-free). Does NOT touch EffortReward.h 1.5.0 (lsBidAskStats / lsBookFlow).
- Bars (IRT volume at price, all history): green up = offer-traded lots per price level in the top 25% of the candle / per level in the rest; red down = bid-traded lots per level in the bottom 25% / the rest. L = (high - low) / tick + 1 levels, zone = max(1, round(0.25 L)) (half up). Under 4 levels = no ratio. Ratio label on each bar when bars are >= 24 px wide. No lines, no letters, no settings.
- Read strip (row under IRT's title): the newest Sw of lsRevRead (RevRead\wy-<MKT>-<spb>.txt): "<WORD> ABSORPTION <x>x - <lots> of <total> buy lots above <level> got no further (<n> hidden) - thin poke" + the turn pill.
  - strict lots = aggressive buy lots above the swept level at prices never traded higher later in the candle (low sweep mirrored); implied fills left out.
  - x = lots / ABS_MED (median of earlier sweeps, nightly); NO < 0.5, LIGHT < 1.0, MODERATE < 1.5, HEAVY 1.5+.
  - hidden = fills on resting orders beyond the largest size the order ever showed (live only).
  - pill = TURN_PCT[F + H + B] (fresh 3-price counter order within 15 s, net counter delta 15 s / half-hour normal >= HARD_CUT, biggest counter order 30 s / average order > BIG_CUT); "late" when the extreme is in the last 30 s; "-" without live data or normals.
- Live from the order feed (DEPTH_BY_ORDER, one per chart, 250 ms clock), decided once after the candle closes and recorded in lsFlexLevels\EffortReward\er-<MKT>-<spb>.txt (no repaint). Older sweeps: the nightly's SW rows (lots only).
- Params: lsFlexLevels\LRA-EffortReward-<MKT>.csv from the nightly part effort_reward (lra.effort_reward 0.2.0). Missing / garbled = built-in defaults + reason in the status file.
- Status: lsFlexLevels\EffortReward\status-<MKT>-<spb>.txt.

## Memory / speed
Fills kept for the forming candle only (cap 150,000), summed per price at the close; resting orders tracked within 64 ticks of the last trade (cap 50,000); order-ID clock ring 4,096; 12 closed-candle sums; bars cache <= 3,000 candles (400 a pass, 10 ms); files read only on a stamp change, <= 64 KiB, torn-read checked, atomic writes; status every 30 s. Replay of ES 11-20 08:20-09:12 (4.3 M events): peak 39,593 fills held, 4,546 orders, 1.2 s.

## Tests (all pass)
- tests_erpane/test_erpane.cpp: 116 checks (ratios by levels, words, wy / params / record parsing incl. garbled / oversized / foreign, strict no-progress, hidden iceberg lots, fresh-ID clock incl. 32-bit wrap, F / H / B windows and late, low mirror, gap / late events / overflow caps, strip text) - ASan / UBSan -Werror.
- tests_erpane/test_erpane_host.cpp: the whole plugin on the mock SDK with the real 11-20 order events (ES, GC) on two charts of one DLL: ES 9:03 ONH = 441 of 5,934 lots, 25 hidden, score 3 -> "HEAVY ABSORPTION 1.8x - 441 of 5,934 buy lots above ONH got no further (25 hidden)" "67% turn"; GC 9:03 PhrH 42 of 602; restart = same strip, record not rewritten; garbled params / foreign sweep file reasons; symbol change; backlog cap; faulting feed; no leaks (LeakSanitizer).
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors / 0 warnings outside the SDK.
- Replay: plugin logic on the Databento 11-20 order data = the mockup numbers (ES 8:51 12 / 49, ES 9:03 441 / 5,934 / 25 hidden).

## Risks / not verified (needs his IRT)
- IRT's FILL events: verified from his recorded IRT order files (buySell = the aggressor side, "?" = implied, no self-fills). Trades against IMPLIED resting liquidity have no resting order, so IRT may not show them: in GC / HG the live lots can be ~2% under the store (GC 9:03 42 / 602 vs 43 / 613) and H / B can differ (HG 9:03 biggest order 3 vs 9). The nightly compares the live record with the store every night.
- Event times are whole seconds: the 15 / 30 s windows can hold up to 1 s more than the research's (ES 9:03 hard 256 vs 259 lots).
- All cutoffs and TURN_PCT are LEARNING (under 20 sweeps a side per market).
- The pane needs lsRevRead on the same market and bar size (its wy file); without it the strip is empty.
