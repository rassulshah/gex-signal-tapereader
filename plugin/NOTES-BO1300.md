# lsBreakout 1.30.0 - the 3-hour BO / BD / Tst (b3h 1.0.0) - 2026-11-23

## His requests (CT)
- 19:05 "Do you have a breakout indicator or my 3hr chart similar to the 3min"
- 19:23 (GC 180-minute picture: BO through 3597, Tst 3600, BO through 3636, Tst 3619) "the 3hr bo just looks for breaks of pivots and then
  their test . so the idea is to look for an expanding range break through a pivot and then its test and label"
- 19:26 "1. bigger than the prior bar 2. candle trades through 3. test is like 3min test rules"
- 19:27 "actually you are right , the close should take out the pivot"
- 19:30 "it should have been drawn" (the Tst at 3619, on the leg still running)
- 19:31 "38.2% minimum pullback is not required here because of the atr rule"; 19:33 "the test needs to be .33 at of the pivot break"
- 19:34 "yes both" (overlap the BO candle AND within 1/3 ATR); "build and let me know when it is ready"
Mockup: breakout-3h-mockup.html v5 (his own candles from lsFlexLevels\ChartView\GC_10800_GCZ6.json).

## What changed
- BreakoutLogic.h: + namespace b3h (appended; namespace bko untouched). Rules in its header comment.
- Breakout.cpp 1.30.0: on a chart of 10800 seconds per bar (and not lsBreakout1) calc runs b3h::run on the chart's own candles
  (feed3h, only when a bar was added / history changed / the forming candle moved) and draw writes the letters (draw3h): BO green above
  the candle, BD red below, bullish Tst green below the pullback low, bearish Tst red above the high. Letters only, the BO font, black edge.
  Status file Breakout.status-<MKT>-10800.txt gets a line `H3,b3h 1.0.0,BO n,BD n,Tst n,live n,bars n,drawn n`.
- Every other chart period: unchanged. Since 1.29.0 the indicator draws NOTHING on other charts (records only) - still so.
- lsBreakout1 (Breakout1.cpp, same source): b3h is off there; it only recompiles.

## Rules decided with him vs decided by me
- His: close beyond the pivot; range bigger than the prior candle; Tst = the 3-minute Tst rules without the 38.2% minimum, with the
  1/3 ATR(14) rule of lsBreakout 1.6.0 (2026-11-04) AND the overlap rule; Tst drawn live.
- Mine (told to him 19:28, he did not object): no follow-through candle needed (his 2nd BO had none); a pivot stays live until a candle
  CLOSES beyond it, and a close-through by a candle that is not bigger gives no BO; BO looks at the last two pivots of a side.
- Live Tst = the one place these marks repaint: while the pullback leg runs the Tst follows its low and can vanish; final at confirmation.

## Tests (all pass)
- tests_bo100/test_bo1300_logic.cpp: 9170 checks (ASan / UBSan, C++17; C++14 plain): his picture, every prefix of the 233 candles
  (a final mark never changes, no BO on the forming candle), each rule, mirror, NaN / empty.
- tests_bo100/test_bo1300_h3.cpp: 257 checks on the mock SDK (ASan / UBSan / LSan): only text, only BO / BD / Tst, colours, sides, no
  overlaps, every mark in view once, bar-by-bar = all at once = after a restart, a 3-minute chart draws nothing, two charts on one DLL,
  status line, version.
- Real irtsdk.h syntax check C++14 / C++17: 0 errors, the same 5 warnings as before the change; Breakout1.cpp: 0 errors.
- GC 2026-10-13 .. 11-23 (232 closed candles): 11 BO, 17 BD, 10 Tst (1 live) - equal to the mockup.

## Not verified / risks
- Not seen on his live chart yet (needs his restart and the indicator on the 180-minute chart).
- Checked on GC only; NOT measured for what happens after a BO / Tst on any market ("?"-level evidence).
- No record file for b3h: the marks are recomputed from the chart's candles. If the chart loads a different first candle, the very
  oldest pivots can differ; the walk re-synchronises within a few pivots.
- A letter that would cross the pane's left edge is not drawn.
- 19:05 I told him the Breakout indicator would "draw" on the 180-minute chart as it was. Wrong: since 1.29.0 it draws nothing. Corrected in chat.
