# 2026-11-19 18:18-18:19 CT - panes read the price chart's Bk / Sw; strength from 10

Rassul 18:18 "ok" (the Volume / Range panes must show Bk / Sw exactly where the price chart has them - his 6:00 PM NQ pane "Sw" had no price-chart Sw), 18:19 "switch to 10".

| File | Version | Change |
|---|---|---|
| VolRangeLogic.h / VolRead.cpp / RangeRead.cpp | 1.8.0 | Bk / Sw no longer computed from levels: read from lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt (lsRevRead's file, same as lsRevReadMarks draws), keyed by chart time; looked at every 5 s, re-read only on size / time change, 512 KB cap, a read without the WY header keeps the last good marks, no file = no Bk / Sw. A Bk / Sw wins over an SR on the same bar (CB still first). Count on a Bk / Sw bar from 10 up (other bars 20 as before). Status line WY. |
| LevelMarks.h | 1.4.0 | ST_MIN 20 -> 10: Bk / Sw / Rev / Tst-Rev strength line from 10 up. |
| RevRead / RevReadMarks / RevReadSettings | 0.20.1 | picks up LevelMarks 1.4.0 (no settings change). |
| CobSignalLogic.h / CobSignal.cpp | 1.11.0 | Cob tag + Cob Rev strength line from 10 up. |

Tests (Linux, -Werror, ASan): tests_volrange/t_vr180 26/26 (NQ 11-19 real bars + wy file: 5:39 PM = Sw bullish with Max 15, 5:36 PM = Bk bearish, 6:00 PM NOT a Sw / Bk, every Bk / Sw of the file marked and no other, no file = no Bk / Sw); test_volrange_130 24/25 (3 level-rule checks rewritten for 1.8.0; the 1 failure is pre-existing); t_vr150_split 69/69; tests_levelmarks 54/54 (strength threshold test updated; NQ 5:39 PM Sw PFL strength = V15); cobsignal fib 29/29, htf 44/44 (version string). MinGW syntax check vs the real irtsdk.h: VolRead, RangeRead, RevRead, RevReadMarks, CobSignal clean.
