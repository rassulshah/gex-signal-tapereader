# lsTurn 1.0.0 "Turn v1.0" (2026-11-22)

## Requests (Rassul, 2026-11-22 CT)
- "Put the red bars below the green bars"; "Can you now do a sanity check ... Reverse engineer winners and see if they fit the approach we have taken"; "Some markets may be slightly different like es vs copper"; "My concern is whether absorbtion is better than effort".
- "No I want you to sanity check and get independent sanity check , code and design review , no memory leaks and give me a final mockup before I approve to build"; "I also want a sanity review of whether the whole approach fits how practitioners understand the reversal process"; "Redo I selected a higher effort model so you get this right".
- "Let's rename the indicator to Turn since it describes the process of a turn"; "build" (the five open decisions were built as recommended: the four reads, bars on candles at a named level, low side mirrored, SR pane untouched, the "?" kept and every Sw scored nightly).
- Design: SPEC-TURN-1.0, mockup turn-final.html, review Turn-v1.0-review.md.

## What it is
New plugin (own pane), an upgrade of lsEffortRewardPane 0.2.0 (same host: order feed, files, volume at price). Files: Turn.cpp (host) + TurnLogic.h (namespace turn, SDK-free). lsEffortRewardPane and the SR pane are NOT touched.
Reads the process of a turn at a swept key level (lsRevRead's Sw candle). Written for a swept HIGH (buyers push, sellers answer); a swept LOW is the exact mirror. Candle = [bar end - 180 s, bar end); prints in feed order, whole seconds; extreme print = the FIRST print at the candle's high.
1. **Stop Run** = buy-aggressor lots at price >= the level from order IDs issued 60+ s earlier that are not a known resting order, / STOP_TYP (ES 21.5, NQ 24, GC 8, CL 11.5, HG 6, NG 5, EU 6). No < 0.5x, Light < 1x, Moderate < 3x, Heavy 3x+.
2. **Effort** = offer-traded lots per price level in the top 25% of the candle / per level in the rest (IRT volume at price; the Effort Reward bar, unchanged). No < 0.5x, Light < 1x, Moderate < 1.5x, Heavy 1.5x+. Not in the verdict.
3. **Exhaustion** = lack of demand. B = buys up to the extreme print, dur = max(extreme second - candle start, 1): late = buying in the last 10 s before the extreme vs the candle's pace; decay = buys per new-high print in the second half of the climb / the first half (4+ new highs); after = buying after the extreme vs the pace. dem = mean rank of the three among the market's PAST sweeps (nightly tables). No (demand strong) dem > 0.5932, Moderate, Heavy < 0.4562. Bar = (1 - dem) / 0.5.
4. **Answer** = marks out of 3 after the extreme print (as lsEffortRewardPane): FAST a counter order ID issued < 2 s before through 3+ prices within 15 s; HARD net counter delta in 15 s / the half hour's normal >= HARD_CUT; BIG the biggest counter order in 30 s / the half hour's average order > BIG_CUT. A missing normal = not available (no upgrade).
5. **Verdict** (Sw candle only): UNCLEAR (extreme in the first 20 s or the last 30 s of the candle, or no exhaustion value) / STAND ASIDE (dem > 0.5932) / BEST ODDS ? (exhaustion and marks >= 2) / WAIT. Pill: STAND ASIDE and WAIT their measured odds (20+ sweeps), BEST ODDS "?" until the nightly proves it, UNCLEAR "-".

## What is drawn
- Two columns per candle, full height = 3.0x, grey = the reading says No. A swept high: UP left Stop Run (light green), right Effort (green); DOWN left Exhaustion (light red), right Answer (red, "2/3"). A swept low: the mirror (red down = the sellers, green up = the turn).
- Sweep candles (lsRevRead Sw): all four bars. Other candles: Stop Run + Effort when the candle pushed into a named level (lsRevRead's L lines: the highest named high it reached from below = green up; the lowest named low it reached from above = red down), only for candles that closed while the pane was loaded.
- Strip (row under IRT's title), for the newest sweep at or before the last candle in view: "<level> <time>   Heavy Stop Run / Heavy Effort / Moderate Exhaustion / Moderate Answer   >> BEST ODDS ?" + pill. Sweeps read by the nightly from the store say "(nightly)"; a sweep with no data says NO READ.
- No settings, no letters, no lines. One grey line instead of an empty pane when there is nothing to read (not a 3-minute chart of the 7 markets; no sweep file).

## Differences from the final mockup (all small; tell Rassul)
1. The strip starts with which sweep it reads ("PhrH 9:03 AM"), because it follows the candle in view.
2. Ratio labels sit on the bars only when a candle is about 60 px wide; on narrower candles the numbers go into the strip ("Heavy Stop Run 5.2x / ...").
3. A neighbour candle is "at a level" when it pushed INTO a named level from the right side (open below a high / above a low); the mockup used "the sweep's level is inside the candle's range".
4. 3-minute charts only (the read was measured on 3-minute sweep candles).
5. Data names are TurnPane (lsFlexLevels\TurnPane, LRA-TurnPane-<MKT>.csv, lra.turn_pane): lra.turn / LRA-Turns already exist (the turn ledger) and are untouched.

## Live / files
- Order feed (DEPTH_BY_ORDER, one per chart, 250 ms clock). A read is decided once, 5 s after the candle's close (coverage >= 95% of the bar volume), recorded in lsFlexLevels\TurnPane\turn-<MKT>-180.txt (T = sweep read, C = candle at a level) and never recomputed. Restart = the same picture from the record.
- No Stop Run until the order-ID clock is 60 s old; the first candle after loading is never read live.
- Params lsFlexLevels\LRA-TurnPane-<MKT>.csv from the nightly part turn_pane (lra.turn_pane 1.0.0): STOP_TYP / HARD_CUT / BIG_CUT / EXH_CUT / ODDS / BEST_PROVEN / N15[48] / AVG_ORDER[48] / RANK_LATE / RANK_DECAY / RANK_AFTER / SW rows. Missing or garbled = built-in constants, no rank tables (every read UNCLEAR) + the reason in the status file.
- Status: lsFlexLevels\TurnPane\status-<MKT>-180.txt. Crash guard: TurnPane\_opening-<MKT>-180.txt.

## Memory / speed
Fills kept for the forming candle only (cap 150,000 x 40 bytes), summed per price at the close; resting orders tracked within 64 ticks of the last trade (cap 50,000); order-ID clock ring 4,096; 12 closed-candle sums; Effort cache <= 3,000 candles; record <= 300 reads + 1,500 level candles; rank tables <= 600 values; files read only on a stamp change, bounded, torn-read checked, atomic writes; status every 30 s.

## Tests (all pass, 2026-11-22; run on the research machine, tests_turn/)
- Real irtsdk.h syntax check, C++14 (wrap_real_sdk.cpp): 0 errors, 0 warnings outside the SDK. It caught one real error the mock had let through (setFont takes a non-const FONT&); the mock is now as strict.
- test_turn.cpp: 378 checks (words, levels, params, the worked candle, mirror = same values to the last bit, no look-ahead, ID clock, caps, record round trip, strip / bars) - ASan / UBSan, -Werror.
- golden.py: the reviewed reference (turnref.py) on 110 past sweeps + the mockup's 45 candles through the C++ logic: 2,017 checks, 0 mismatches (stop lots, effort, extreme, late / decay / after, FAST / HARD / BIG, dem, marks, verdict).
- test_turn_host.cpp: the whole plugin on the mock SDK, fed like a live morning (the real 11-20 order events, ES + GC on one DLL, charts growing candle by candle): 1,364 checks. ES 8:51 LonHI UNCLEAR (19 s), ES 9:03 ONH UNCLEAR (6 s, Stop Run 5.2x, 3/3), GC 9:03 PhrH BEST ODDS ? - as the mockup. Scroll back, narrow candles, small panes, restart = byte-identical drawing and record, nightly rows, garbled / foreign files, symbol change, backlog cap, faulting feed, unsupported chart. No overlap, nothing outside the pane, fill-only bars.
- soak.cpp: 40 cycles x 2.5 million events: 0 KB growth; 8 cycles under ASan / LSan: no leak, no error.
- lra.turn_pane: 12 tests (analytics/test_turn_pane.py); its readings = the golden values on all 110 sweeps; two runs the same night = byte-identical files.

## Risks / not verified (needs his IRT)
- Not yet seen on a live chart: the real compile (cl) and the pane on his monitor are checked after his restart.
- Live Stop Run can be a little under the store's: trades against IMPLIED liquidity have no resting-order fill, so IRT may not show them (GC 9:03: 31 lots live form vs 38 in the store, both Heavy). The nightly's Stop Run has no "never resting" filter (ES 9:03: 115 vs 112 lots).
- Order age comes from the order-ID clock (IDs assumed increasing): the nightly compares every live read with the store (TURN-PANE.md "Live vs store").
- STAND ASIDE vs exhaustion present is the proven part (26% vs 65%). BEST ODDS vs WAIT, the low side, per-market reads, the 20 s rule and the 0.59 cutoff are NOT proven (set in-sample, falling market): hence the "?" and the nightly forward test. FAST is not calibrated per market (NQ fires on about 9 of 10 sweeps, EU never).
- The Sw mark can arrive up to 3 candles late: until then the candle shows its level bars, then the four bars (once).
- lsEffortRewardPane 0.2.0 stays installed and its nightly part keeps running (not retired in this build).

## Nightly
lra.turn_pane (part "turn_pane:nightly", enrolled 2026-11-22): scores every new IRT Sw from the store exactly as the plugin reads it, freezes its verdict, measures the outcome (fade at the close, stop 1 tick beyond the extreme, target 2 x ATR, 60 min), logs the challengers (two-candle Exhaustion for early extremes, FAST rate per market, live vs store), judges (odds only from 20+ sweeps; BEST_PROVEN needs 20+ NEW BEST ODDS a side, each 15+ points above that side's other reads) and writes the params. Data: data/turn_pane/ (seed_sweeps.csv = the 110 reviewed sweeps, norms_seed.csv). Report: research-artifacts/setups/TURN-PANE.md.
