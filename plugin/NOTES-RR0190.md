# lsRevRead / lsRevReadMarks 0.19.0 - Bk / Sw / Cx + the I with their level on the price candles (2026-11-19)

Rassul (CT): 10:20 "lets show bk sw sr and cx and the level they apply to . but be mindfull of not overlapping one another and putting lettes
on top of green and below red as well as indicating the key level they apply to .. show mockup"; 10:26 "if you showed it without the price
could you put it in", "but on 2nd line", "and abbreviate like Op or LLO or LHI etc"; 10:29 "whiy didn't you mention anything on PFL/PDL ?";
10:31 "you can also mention other levels that it is breaking even the highs like PDH etc"; 10:34 "to simplify, we can mention only 1 level
giving priority first to session levels like overnight, then pd levels, pf, then volume levels"; 10:37 "yes, put level under I too. ok, cut
repeats"; 10:38 "1. skip for now 2. i will think about this 4. ok. build"; 10:40 "go". Mockup: level-marks-mockup.html v8.

What changed
- LevelMarks.h 1.0.0 (new, SDK-free): Bk (a close through a level; key / volume / weekly levels any such close, hour levels with the biggest
  volume in 5+; no Bk when the same level was crossed on a close in the 10 candles before), Sw (one failed attempt on an unbroken level,
  marked on its deepest candle, the attempt's biggest volume >= the 5 candles before), Cx (ZigZag pivot candle or the one before, biggest
  volume in 20+, green at a pivot low / red at a high). One mark per candle (Sw, then Bk, then Cx). ONE level per mark: session (ON, Lon,
  Asia, Open) > PD > PF > volume > weekly > hours (PHL / PHH = prior hour, then 2h / 3h). Short names Op LLO LHI ALO AHI ONH ONL PDL PDH
  DP WP ... MenthorQ / gamma / VWAP never make or label a mark. Repeats cut: a code once per level per session unless price went 1+
  ATR(13) away and came back.
- WyRead 1.7.0: keeps the level each I reclaimed (Out.ilv / ilvHb) - no rule change.
- lsRevRead 0.19.0: computes the marks per session (the current one with the level files, earlier ones with their hour levels) and writes
  them to RevRead\wy-<MKT>-<secs>.txt: stamp|Bk|dir|level|px (the I lines get |level|px too; E / A / Sp / Ut unchanged).
- lsRevReadMarks 0.19.0 (WyMarks 2.0.0): tags = code (bold) + level name on a 2nd line; over a green candle, under a red one; the I first,
  the level mark further out; no tag covers a candle (wick to wick) or another tag (rows outward, +-5 / 9 px sideways, then the other side);
  nothing right of the last bar; a short dashed yellow line at the level price. No settings.
- gex-build.bat: LevelMarks.h on the RevRead and RevReadMarks lines.

Tests: tests_levelmarks 15/15 (ES 11-19 7:00-9:48 AM = mockup v8 exactly: 7:06 Bk PFH, 7:33 Bk AHI, 8:27 Cx Op, 8:30 Bk PHH, 8:36 Bk Op,
8:39 Sw PHL, 8:45 Bk PHL, 8:54 Bk LLO, 9:12 Bk PDH, 9:18 Bk ALO, 9:30 Bk ONL, 9:39 Sw PDL; the 7:54 I = I / PHL; replay no repaint; rule
cases), tests_wymarks 25/25 (old 3-field file still read, 5-field lines, 400-candle layout with no overlaps), WyRead suites unchanged and
passing; MinGW syntax check of RevRead.cpp / RevReadMarks.cpp against the real irtsdk.h: 0 errors.

Risks / not verified: not seen on his chart yet - needs an IRT restart (File -> Exit, wait for "installed", reopen). Times above are candle
starts (IRT labels candle ends, +3 min). Sw beats Bk on the same candle (as in the mockup). The key file's PHL / PHH are used as given.

## + lsBreakout / lsBreakout1 1.28.0 and coordination (Rassul 10:50 CT "ok build", 10:51 "remember placement of other signals like cmx so you have to coordinate everything")
- BreakoutLogic.h 1.28.0: a fired BO / BD with a Reversal Read Bk of the same direction on its candle or the next one is NOT drawn (Bk + level
  carries it; Pb / Rev / records / decisions unchanged; no letters file, a torn file or a Bk the other way = BO / BD drawn as before). The Rly
  carries its level on a 2nd line (8 pt): LevelMarks 1.1.0 levelNear - a level inside the wave-extreme candle or within 2 ticks of its extreme,
  same priority / short names; the key levels come from the letters file (L|px|name lines lsRevRead now writes); hour levels from the candles.
- Breakout.cpp feedWy: reads RevRead\wy-<MKT>-<secs>.txt every 3 s, bounded 1 MB, re-read on size / time change, last good copy kept.
- WyMarks 2.1.0: parseLevels, civilDays (SignalBoard bar key); layout(..., taken) never covers a taken box. lsRevReadMarks reads SignalBoard
  'L' (the BO indicator's letters as drawn: candle, side, px out from the wick) and places Bk / Sw / Cx / I around them.
- Tests: tests_bo1280/t_merge 10/10, tests_levelmarks 17/17 (+ levelNear: 9:39 AM low = PDL), tests_wymarks 29/29; MinGW syntax check of
  Breakout.cpp / Breakout1.cpp / RevRead.cpp / RevReadMarks.cpp: 0 errors.
- Coordination inventory (price-chart tags): lsBreakout / lsBreakout1 BO BD Pb Rev Rly FT BI % - COORDINATED (SignalBoard 'L');
  lsZzoComboMarks CB / CB35 / CB99 over the Cob bar, SR under / over the stop-run candle (wave tags Cmx / AR / ST / Ab are logged, not drawn,
  SHOW_WAVE_TAGS false) - NOT YET (no published positions; needs a small publish); lsCobSignal Cob / Cob+R / +V / +C labels and Lvp - NOT YET
  (same); lsRevReadMarks I - its own. Line / box indicators (SessionVWAP, WavePOC, StepTrend, HtfSR, HodLod box, EventCal box, ZoneDefense,
  DeltaProfile circles, FirstExtreme) do not put letter tags on candles.
