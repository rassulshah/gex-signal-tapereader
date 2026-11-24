# LevelMarks 1.17.0 / lsRevRead + lsRevReadMarks 0.26.0 - Bk on hourly pivots and 4-hour highs / lows (2026-11-24)

Rassul 08:33 CT "go for hourly pivots and 4h high and lows"; 08:34 "absorption levels should match bk levels"; 09:01 "go for bk with hourly
pivots and 4 hour highs and lows etc....". Mockup: bk-levels-mockup.html (gold / copper, real candles).

## What changed
- New Bk levels: HpH / HpL = the hourly pivots (his 1-bar High / Low ZigZag on 1-hour bars, R1 R2 S1 S2 walked like HtfSr.h, standing
  only; PivWalk is hsr::Walk with the block size as a parameter - proven identical to it on 3-hour blocks, every GC candle) and 4hH / 4hL =
  the last COMPLETED 4-hour bar's high / low (4-hour bars from 5 PM CT: 17-21, 21-01, 01-05, 05-09, 09-13, 13-17).
- Bk through them needs the hour levels' rule (the candle's volume the biggest in 5+) and the BO candle rule.
- They rank LAST (4h 65, pivots 70) and older levels come first in the 4-level cap, so no existing Bk changes its name.
- Never a Sw, Cx or I level. A Bk made ONLY by the new levels gives way to a Cx on the same candle.
- The RevRead host walks them over the chart's closed candles (same loop as the 3-hour S / R) - no new file, nothing from other indicators.

## Tests
- tests_levelmarks/t_bk17.cpp 97 ok; t_major 60, t_ar 94, t_levelmarks 95, t_cxlive 9 - all pass (ASan / UBSan, -Werror).
- Real candles 11-10 .. 11-24: gold 8 new Bk, copper 9 new Bk; 0 existing Sw / Bk / Cx / labels changed.
- Host: RevRead.cpp / RevReadMarks.cpp syntax-checked against the real irtsdk.h - 0 errors, no new warnings.

## Not changed yet (on purpose)
- The Turn indicator's Absorption read does not yet use these levels: the practitioner study (Trader Dale / Creamer, next) decides what
  the absorption piece becomes, then the level gating is built once.
- The hourly pivot / 4-hour lines are not drawn on the chart (only the Bk label names them).
