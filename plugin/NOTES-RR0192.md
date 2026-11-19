# lsRevRead / lsRevReadMarks 0.19.2 - the strength line (2026-11-19)

Rassul 11:22 CT "sweeps of key levels on big volume or range are very meaningfull because they are rejecting the break"; 11:24 "maybe consider
sw and a second line like v50 or r50, same with breaks"; 11:26 "for Bk too, and you can use the max numbers that we have right so it would take a
look at the biggest in green bars, red bars, consecutive and print that"; 11:30 "ok.. good build it"; 11:35 "will you add this strength line to
the I also" (mockup level-marks-bo-mockup.html, chart 2).

What changed
- Bk, Sw and I tags get a 3rd line = his BVMax / BRMax, exactly as the Volume / Range panes count it (VolRangeLogic Engine::statFor): the larger
  of BV (smaller candles in a row before it) and BVG / BVR (the same over its own colour), capped at 99 ("99+"), 1,500 candles back, over the
  chart's candles (not only the session). Shown as "V<N>" (volume) or "R<N>" (range in ticks), whichever is larger (volume wins a tie), only
  from 20 up. A Sw takes the max over its attempt's candles; Bk / I their own candle. Cx: no strength line.
- LevelMarks.h 1.2.0 (stCount / strength / strengthAt / fillStrength, Mark.t0 = the Sw attempt's first candle, Mark.st), WyMarks.h 2.2.0
  (letters-file lines may carry a 6th field = the strength; 3 / 5 / 6 fields read), RevRead.cpp 0.19.2 (fills the strength from the chart's
  closed candles, writes the 6th field for Bk / Sw / Cx / I lines, draws the 3rd line in the level's font; the tag grows by one line).
- Nightly lra.revread_letters 1.2.0: every Bk / Sw / I row carries its strength; the report splits each by < 20 / >= 20 / >= 50.

Tests
- tests_levelmarks/t_levelmarks.cpp 38/38 (ASan / UBSan -Werror), new history fixture es1119hist.psv (1,500 candles before + the session):
  ES 11-19 parity with the mockup / the panes: 7:33 Bk AHI R99+, 8:30 Bk PHH V99+, 8:36 Bk Op R99+, 8:39 Sw PHL V99+, 9:30 Bk ONL V99+,
  9:39 Sw PDL V99+; no line on Bk PFH / PHL / LLO / PDH / ALO; Cx none; rule cases (consecutive stop, same colour, cap 99, tie -> V, under 20,
  Sw attempt max). The 7:54 I: V11 / R6 -> no line.
- tests_wymarks/t_wymarks.cpp 36/36 (6-field lines, empty strength, I strength, 5 / 3 fields still read, oversized rejected).
- MinGW syntax check vs the real irtsdk.h: RevRead.cpp, RevReadMarks.cpp, Breakout.cpp (WyMarks reader) - 0 errors.
- Nightly on ES (in-sample, hour levels only, move 10 candles in ATR): Bk <20 +0.11 (back 62%), >=20 +0.38 (back 55%), >=50 +0.33;
  Sw <20 +0.54, >=20 +0.67, >=50 +0.74 (Sw includes hindsight - see the report's caution); I <20 -0.42, >=20 -0.11, >=50 -0.25.

Risks / not verified
- Not seen on his chart yet (needs an IRT restart). IRT's chart volume (barVolume) is used, as the panes do; the research check used the
  research bars' volume.
- The BO indicator reads the same letters file (WyMarks parseAll): it accepts the 6th field; it rebuilds with the new WyMarks.h.
