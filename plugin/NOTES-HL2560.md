# lsHodLod 2.56.0 + Asia 6:00 PM - 2:00 AM CT (2026-11-26)

Requests (CT): 17:11 "indicate on a level that is drawn on a chart whether it was swept already or not ... a swept badge in the center
of the level"; 17:14 "if a level has not been swept, it can say stops. also the next line should also mention if the level was swept
already or not"; 17:19-17:21 Asia session 6:00 PM - 2:00 AM CT ("yes, but can you also check to make sure it is correct"); 17:21 "make
the updates whatever you can and build"; 17:29 "add the time the level was swept to the badge". Mockup: LevelSwept-mockup.html.

## What changed
- HodLodStops.h 1.1.0: chips() - gold "Stops" / grey "Swept 5:45" in the centre of every stop level's line (session, daily, weekly
  highs / lows), never on another chip or a box (steps right / left 56 px); status() for the Next line; Asia forms at 2:00 AM.
- HodLod.cpp 2.56.0: draws the chips last (clear of the box, Stops section, Level line, last 5 days panel, under the calendar); the
  Next line gets "stops" / "swept" after each stop level; status line CHIPS.
- Asia window (18:00, 02:00] CT bar END everywhere (was 01:00 - his IRT session #119 since 2026-10-24):
  KeyLevelsLogic.h (asiaWindow + default) -> lsLimitProfile 2.71.1 (the chart's AsiaHI / AsiaLO, LRA-KeyLevels), lsCobSignal 1.12.1;
  LevelMarks.h 1.30.0 -> lsRevRead 0.42.0 / lsRevReadMarks 0.38.0; ZzoCobFinder.h -> lsZzoCombo 3.3.1 / lsZzoComboMarks 3.0.6;
  DealerColumnsLogic.h text. Plugins that include KeyLevelsLogic.h through other headers (Breakout, Breakout1, ...) rebuild with it,
  versions unchanged (they do not draw Asia levels).
- Research: hodlod_levels 1.16.0, key_levels.ASIA_END "02:00", trade_def asia window -> 02:00. Sweep files rebuilt.
- Source checked: NinjaTrader "Asia session ... 7:00 pm to 3:00 am ET" = 6:00 PM - 2:00 AM CT; London from 3:00 AM ET = 2:00 AM CT.

## Tests
t_hl2560 24 pass (status, chips, Swept time, Asia 1:30 / 2:03 AM); t_hl2550 29, t_hl2540 30, t_hl2510 83, t_hl2500 54, t_hl2490 37,
t_hl2480 53 pass; LevelMarks tests: only the version pins changed (1.29.0 -> 1.30.0), 0 new failures; MinGW syntax (real irtsdk.h):
HodLod, LimitProfile, RevRead, RevReadMarks, ZzoCombo, ZzoComboMarks 0 errors; nightly tests 23 pass (1 environmental).

## His step in IRT
His Asia session template (#119) still says 18:00-01:00: his own RTL formulas that use it (the CoB RTL "pwl / alo") need it at
18:00-02:00 to match. The plugins no longer depend on it.
