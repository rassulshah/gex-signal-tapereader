# lsHodLod 2.23.0 / ReadBox 1.1.0 (2026-11-11)

## What he asked (CT)
- 11:40 "get rid of dly trend.and the values to the left and move the gamma regime were the daily trend is" - asked which values: "Dly + RTH
  opens + Bar left".
- 11:41 (screenshot of the read box, the HTF line and "0.8x ADR, Used 91%" crossed out) "also updat eht readt at the top to be more simple".

## What changed
- Header row of the HOD / LOD table: no "RTH opens 8:30 AM" / "RTH 83% done", no "Bar m:ss left", no "Dly:" trend. The gamma pill
  (+GAMMA / -GAMMA) now leads the centre group, where "Dly:" was, followed by "Hrly:" and "3m:". Right side: Exp, the range pill, the version.
- Read box (ReadBox 1.1.0): Hrly Trend / Day / Lvl / Mkt / Lmt. The first line is "Hrly Trend: Up / Down / NA" (11:45 CT "where it says HTF change it to Hrly Trend: and give it values like up or down or NA"; 2.23.1) by the Step Trend 1-bar hourly rule over the completed hours, green / red / white. The RTH
  Day line has no ADR multiple and no "Used %" ("Day: Nor 35%, -> CoB 1.33"). The overnight Day line still shows "ON 0.4x ADR".
- Flags: hl::HDR_DLY / HDR_LEFT / HDR_PILL_MID, rbx::SHOW_HTF / DAY_ADR. No settings change.

## Tests
- tests_readbox/test_readbox_110.cpp: 8 pass. MinGW real SDK: 0 errors.
  Settings contract passes.
- NOT run: the older lsHodLod host suites and tests_readbox assertions that expect the HTF line / the 5-line box (superseded).

## Not verified (needs his IRT)
- The look of the header row with the pill in the centre; nothing was rendered here.

## 2.24.0 / ReadBox 1.2.0 (2026-11-11)
- Rassul 11:52 CT "are thee any updates you think is should make to the read at the top"; my three proposals; 11:53 "1. ok. 2. ok 3. ok, and i
  dont even think it is working becasue the levels it is mentioning are passed".
- Lvl: always the nearest level above and below the last close with the distance in points ("LonHI 7620 (4.5 above) / CWH 7600.5 (15 below)");
  the one being tested / swept says so. Levels price has left behind are never named (was: "no level near", "-> R ...", "Swept ...").
- Hrly Trend: + the newest hourly breakout level going the trend's way ("Up, above 7607"; "Up, back below 7607" when the close is under it).
- Flow: ONE line replaces Mkt and Lmt, the verdict first ("Sellers in control: pushing 2.6x, absorbing 311 (5.1x) @87.75", "Mixed: ...",
  "Sellers trapped @..", "Even").
- BUG FIXED (his "the levels it is mentioning are passed"): Mkt / Lmt named every lsDeltaProfile event of the last 5 candles whatever price did
  afterwards. An event is now named only while it is in play: the last close is on its side of its price and within 1 ATR of it.
- The box is 4 lines. Tests: tests_readbox/test_readbox_120.cpp 19 pass (replaces test_readbox_110.cpp); MinGW 0 errors; settings contract passes.
- Not verified: the look on his chart. Not measured: whether the Flow verdict is right more often than not (the events are LOG-only letters).

## 2.25.0 / ReadBox 1.3.0 (2026-11-11)
- Rassul 11:55 "what would you reccomend for th ehod lod box ?", 11:57 "show mockup" (claude.ai/artifact/BGo1eWeEkJd1M8NhoqJTq2), 12:01 "update the
  read at the top to just be the MKT LMT or derflow read and keep the Day in the hod lod obx , it already has the lvl righ t", 12:02 "will the hod
  lod box highlight the level that is upcoming or going to be in play ?", 12:03 "ok.. lets do it. build".
- Read box (ReadBox 1.3.0): ONE line, the order-flow read ("Flow: ..."). Out still carries the trend / day / lvl reads + the 60-min ATR + price.
- HOD / LOD box:
  - header row: "Hrly: Up, above 7607" (the read's hourly trend + breakout level; was the ER trend words) and "Day: Nor 35%, favours CoB".
  - a CALL LINE under the header: [FLIPPED hh:mm] "1st extreme: <actual, else exp + the expected window>", "2nd: <the same>", then the Lvl read
    (nearest level above / below). FLIPPED = the side first called in changed during the session (kept in the chart's state: lost on a restart).
  - sweep tables: the UPCOMING level is the one bright row with "next" / "testing" (within a quarter of the 60-min ATR); every other row dim.
    HOD table = the nearest level at / above price that can still be swept; LOD table = at / below. After the first extreme is in, only the side
    still being watched has one (HOD first -> the LOD table) and the other table is dim.
  - the "1st extreme by / % of days" table shows only until the first extreme is in.
- Tests: tests_hodlod/t_hl2250.cpp 13 pass (swPrices, markSweeps); tests_readbox/test_readbox_130.cpp 19 pass (replaces 110 / 120); MinGW 0 errors;
  settings contract passes. The older layout suites t_hl2179 / 2180 / 2190 were run before and after: they already fail on 2.22.2 (stale goldens);
  the only new failures are the intended ones (the box one row taller, the gamma pill no longer right of the range pill); no text outside the
  table and no overlaps in the scenes they cover.
- NOT verified: the look on his chart - in particular the header row's width with the Day group, and the call line when it is longer than the table
  (segments that do not fit are dropped from the right). The SESSION block keeps its Exp / Act rows (the call line repeats them).
