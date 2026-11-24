# lsTurn 1.3.0 - Stop Run / Absorption / Exhaustion / Initiative - 2026-11-23

## His requests (CT)
- 18:15 "is this similar to  stoprun absorbtion exhaustion intiative"; "can you update my turn indicator to this .  so update effort to absrobtion also"
- 18:19 "i'll go with your recommendation .  show mockup"
- 18:27 "what is the correct way to do it"; 18:28 "i want you to do the absorbtion the right way"
- 18:38 "Research first before building so you do it right"; 18:40 "Go"
- 19:48 "build" (after the research report of 19:10 and turn-four-pieces-mockup.html v3)

## Research behind it (2026-11-23, MBO store, $0)
603 first RTH tests of a major level (LonHI / LonLO / ONH / ONL / PDH / PDL / PFH / PFL), ES NQ CL GC HG NG EU, 2026-09-10 .. 11-20.
Turned = 2 ATR away before 0.5 ATR through, within 10 candles.
- Close outside the tested end quarter: 73% turned (319 / 435); close in it: 46% (78 / 168). Holds in both halves of the dates.
- Effort (1.2.x's number): AUC 0.48 - does not separate; ES 0.31 (backwards).
- After the close test: heaviest single price >= 2.0x its 20-candle average 80% (126 / 157) vs 69%; the largest single resting order
  (one order ID) 80% (133 / 166) vs 69%. With the close AT that end heavy volume is 51%.
- A time-of-day average: 0.49 (no help). The quarter average: weaker than the heaviest price.
- Per market the volume step is under 20 a side (clear on CL / GC / EU, nothing on NQ / HG / NG): LEARNING.
Work files: research machine /home/claude/absr (gen3.py = the query, events.json = the 638 events, an.py / an2.py = the tables).

## What changed
- TurnLogic.h 1.3.0: Bars.topMax / botMax (heaviest price in the end quarter); Abs, absAway, absX1000, absClass10, absBar; stripOf /
  stripOfCand take the Absorption; stripOfAbs replaces stripOfEffort ("High: ... / Low: ..."); "Answer" -> "Initiative"; Show defaults =
  all four pieces, coloured from 2.0; display key absorption_from (effort_from still read); status text "absorption ... initiative".
- Turn.cpp 1.3.0: calc reads the close with each bar (BarVal.awayUp / awayDn); draw computes Absorption from the kept bars (absAt: the 20
  candles before it must be read, so a number never changes once shown); the Absorption bar is the winner's colour (red at a high,
  green at a low) when it counts, grey otherwise; sweep candles show the Absorption of the end they pushed into.
- UNCHANGED: the record file (TRN|1), the reads, the recorded Effort column, Stop Run, Exhaustion, Initiative (= Answer) values, the
  nightly params file, the order-ID clock. effOf / effOn / effClass10 stay in the header (unused by the host).
- NOT built: the largest-resting-order measure in the pane. It is computable from the MBO store (trades.oid) for scoring; drawing it
  waits for 20 a side per market.

## Tests (all pass)
- tests_turn/test_turn.cpp: 3,287,189 checks (ASan / UBSan C++17; C++14 -Werror).
- tests_turn/test_turn_host_130.cpp (new): 31,837 checks on the mock SDK under ASan / UBSan / LSan, Friday's ES 8:24 - 10:21 AM candle
  by candle: 57 Absorption bars = an independent calculation (number, colour, side), words nowhere say Effort / Answer, mouse strips,
  no overlaps, chart removed and added = the same picture and record, display file, status, version.
- tests_turn/soak.cpp: 8 cycles ASan / LSan and 40 plain (growth 0 KB).
- golden.py: 2017 checks, 0 mismatches (the reads are untouched).
- Real irtsdk.h syntax C++14 / C++17: 0 errors, 0 warnings outside the SDK; mock compile C++14 -Werror.
- Copper 2026-11-23 9:42 AM - 12:06 PM: draw for draw the mockup he approved (12:00 PM climax 8.6 red at the high; 8 of 49 coloured).
- ES Friday, 60 / 22 / 6 px: the Stop Run / Exhaustion bars, the Initiative marks, the axis and the record file are identical to 1.2.1's
  full setting; a restart repaints nothing.
- tests_turn/test_turn_host.cpp pins 1.2.1's words and its Effort-only default: superseded by test_turn_host_130.cpp (left in place).

## Not verified / risks
- Not seen on his live charts (needs his restart). Check after: status-<MKT>-180.txt VERSION|1.3.0 and SHOW|no display file (the defaults).
- The first candles after a chart load and candles older than the kept window (3000 bars) have no Absorption bar until 10 earlier
  candles are read. Right after 5:00 PM the average comes from the prior session's last candles.
- "How often the level held" is not a trade result.
