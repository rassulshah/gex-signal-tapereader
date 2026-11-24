# ls3hBO 1.0.0 "3h BO" - the 3-hour BO / BD / Tst as its own indicator - 2026-11-23

## His requests (CT)
- 19:05 "Do you have a breakout indicator or my 3hr chart similar to the 3min"
- 19:23 (GC 180-minute picture) "the 3hr bo just looks for breaks of pivots and then their test . so the idea is to look for an expanding
  range break through a pivot and then its test and label"
- 19:26 "1. bigger than the prior bar 2. candle trades through 3. test is like 3min test rules"; 19:27 "actually you are right , the close
  should take out the pivot"; 19:30 "it should have been drawn"; 19:31 "38.2% minimum pullback is not required here because of the atr
  rule"; 19:33 "the test needs to be .33 at of the pivot break"; 19:34 "yes both"; "build and let me know when it is ready"
- 20:17 "rename the bo indicator to  3h BO"; 20:20 "im talking about the htf bo right .. dont mess up the 3min bo indicator"; 20:25 "1"
Mockup: breakout-3h-mockup.html v5.

## What is on the PC
- NEW Bo3h.cpp (ls3hBO.dll, "3h BO") + Bo3hLogic.h (namespace b3h 1.0.0, every rule in its header). Draws only on a 10800-second chart:
  BO green above the candle, BD red below, bullish Tst green below the pullback low, bearish Tst red above the high. Letters only.
  Status lsFlexLevels\Bo3h.status-<MKT>-<spb>.txt (H3 line: counts, drawn; "off" on any other chart period).
- lsBreakout: Breakout.cpp and BreakoutLogic.h are back to 1.29.0 byte for byte. The 1.30.0 that carried the 3-hour marks for an hour
  (19:45 - 20:55) was built but never installed. NOTES-BO1300.md and tests_bo100/test_bo1300_*.cpp now only point here.
- gex-build.bat: + "call :one Bo3h ls3hBO ..." after HtfSR, + Bo3h in the :status list (two additions, nothing else changed).

## Rules decided with him vs decided by me
- His: close beyond the pivot; range bigger than the prior candle; Tst = the 3-minute Tst rules without the 38.2% minimum, with the
  1/3 ATR(14) rule of lsBreakout 1.6.0 (2026-11-04) AND the overlap rule; Tst drawn live; its own indicator named 3h BO.
- Mine (told to him 19:28): no follow-through candle needed; a pivot stays live until a candle CLOSES beyond it, and a close-through by a
  candle that is not bigger gives no BO; BO looks at the last two pivots of a side.
- The live Tst is the one place these marks repaint: while the pullback leg runs it follows the low and can vanish; final at confirmation.

## Tests (all pass)
- tests_bo3h/t_bo3h_logic.cpp: 9170 checks (ASan / UBSan -Werror C++17; C++14): his picture, every prefix of the 233 candles (a final
  mark never changes, no BO on the forming candle), each rule, mirror, NaN / empty.
- tests_bo3h/t_bo3h_host.cpp: 258 checks on the mock SDK (tests_bo3h/mock/irtsdk.h = the lsBreakout test mock; ASan / UBSan / LSan,
  -Werror): only text, only BO / BD / Tst, colours, sides, no overlaps, every mark in view once, bar by bar = all at once = after a
  restart, a 3-minute chart draws nothing and says off, two charts on one DLL, status, version.
- Real irtsdk.h syntax C++14 / C++17: 0 errors, 0 warnings outside the SDK.
- GC 2026-10-13 .. 11-23 (232 closed candles): 11 BO, 17 BD, 10 Tst (1 live) = the mockup.

## Not verified / risks
- Not seen on his live chart (needs his restart, then adding ls3hBO to the 180-minute chart's price pane).
- A NEW DLL: the first build and install of ls3hBO; check BUILD-STATUS "Bo3h: installed" after the restart.
- Checked on GC only; NOT measured for what happens after a BO / Tst on any market.
- No record file: the marks are recomputed from the chart's candles; a different first candle can change the very oldest pivots.
- A letter that would cross the pane's left edge is not drawn.

## 1.1.0 (2026-11-23 23:06-23:10 CT): SP and UT
Rassul 23:06 "update the bo on the htf that we did to include sp and ut .  the code can be found in these two indicators"
(his IRT signals sot_mom_div_dn_basic = SP, sot_mom_div_up_basic = UT; RTL read from data\irt\library\rtl), 23:10 "go".
- b3h::spUt inside run(): on the same 1-bar High / Low ZigZag, decided on the CLOSED candle that confirms the pivot (his Uppivot =
  Ups#1 = 1), drawn on the pivot candle, final at once (no live state). UT (red, above the pivot high) when the new pivot high H1:
  1. >= H2 with its candle's low <= H2 and (H1 - H2 < 0.5 x (H2 - L1) or the confirming close < H2 or a lower momentum high);
  2. >= H3 (H2 lower than H3 and H1) the same way, with L2;  3. >= H4 (above H2 and H3), its low <= H4, H1 - H4 <= the confirming
  candle's true range. Momentum = close - close 10 candles back (IRT MOM; his MOM length to confirm), the highest in each up leg.
  SP (green, below the pivot low) = the mirror.
- Host: SP / UT words, side by b3h::above(); at most 2 letters a candle side, drawn BO / BD first, then SP / UT, then Tst; a letter
  moves outward a row when it would touch a neighbour candle's letter. Status line adds ",SP n,UT n".
- His GC candles 10-13 .. 11-23: BO 11, BD 17, Tst 10 (unchanged), SP 13, UT 9. Live ES 3-hour: SP 8, UT 11.
- Tests: logic 15,136 (no repaint over every prefix, mirror SP <-> UT, hand-made UT cases 1 / momentum / no overlap of H2 /
  close back / not before a closed confirm), host 473 (words, sides, colours, no overlap), real irtsdk.h C++14 / C++17 clean.
- Not verified: the candles against his two signals on the live ES 3-hour chart (his MOM length).
