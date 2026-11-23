# lsVolRead / lsRangeRead 1.10.0 - Cx: one rule for the badge and the pane (2026-11-23)

## Requests (Rassul, 2026-11-23 CT, NQ and ES 3-minute charts)
- 07:26, with a screenshot (NQ, the Cx badge at 6:45 AM, the Volume pane's "Cx 31" at 6:21 AM and "Cx 41" at 6:48 AM circled): "are the rules for the cx badge and the cx label on the volume different . see the pic"
- 07:32, after the answer (yes: the badge wants the biggest volume of ALL the 20 candles before; the pane also passed a candle that was only the biggest of its own colour in 20+): "go with badge for both"
- 07:33, with a screenshot (ES 2026-11-20, "Cx 20" and "Cx 38" circled in the Range pane): "why is cx on the range ?"
- Estimate given before the work: about 45 min, $0, mockup first, one Investor/RT restart. Mockup: cx-badge-label-mockup.html (Today / After / option C), drawn from the pane's own layout() on his two windows.

## What was wrong
- The price chart's Cx (LevelMarks.h, lsRevRead): a 3-bar ZigZag pivot candle, or the candle before it, whose volume is at least the biggest of the 20 candles before it; no Cx on a candle that already has a Sw / Bk; green at a pivot low, red at a pivot high.
- The panes' Cx (VolRangeLogic 1.3.0 zigStep): the same pivot, but "biggest in 20+" counted as Max = the larger of (all candles in a row, candles of the SAME COLOUR in a row). A candle that was only the biggest of its colour passed.
  - NQ 2026-11-23 6:21 AM: green, 1,179 lots, biggest green in 31, of all in 1 (6:03 had 1,364, 6:15 had 1,325) -> pane "Cx 31", no badge.
  - NQ 6:48 AM: the pivot low, red, 1,367 lots, biggest red in 41, of all in 0 (6:45 had 1,430) -> pane Cx on 6:48; the badge moved one candle back to 6:45 (biggest of all in 69).
  - ES 2026-11-20 1:33 PM: red, biggest red in 51, of all in 11 -> pane "Cx 51", no badge.
- The Range pane ran the same code on range: "biggest range at a pivot" carried the letters Cx. Not climactic volume, never a badge above it.
- Why it drifted: 1.8.0 (2026-11-19) made the panes take Bk / Sw from the price chart's marks file "so the panes and the price chart can never disagree"; Cx was left on the panes' own rule.

## What changed
1. **Volume pane: Cx = the price chart's Cx.** `parseWy` also reads the `t|Cx|dir|lv|px|` lines of `lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt` (the file Bk / Sw come from); `Engine::cxAt(i)`; `markOf` shows Cx only there. No file = no Cx. The label appears when the badge does (the price chart writes the line when the pivot is confirmed).
2. **Range pane: no Cx.** `cxAt` is 0 in the Range pane.
3. The panes' own pivot rule (`zigStep`, `pv`) is removed.
4. The read's sentence "Climax volume at the high / low" (Volume pane) follows the file's Cx on the candle or the one before it.
5. Status file: new line `CX,1.10.0 Cx from the price chart's marks file,<n> Cx read` (Range: `CX,1.10.0 no Cx in the Range pane (Cx = climactic volume)`). The `WY,1.8.0` and `MARKS` lines are unchanged (`P n` = the Cx shown).
- NOT changed: the Max numbers over the bars and their thresholds, Bk / Sw, CB / SR / Ct and their priority over a Cx, the bars' sizes, the colour rule (grey unless marked or the biggest in 50+; a marked bar in its candle's colour - so a bar that loses its Cx turns grey unless it is a 50+ bar), the text and numbers right of the last candle, the badges on the price chart, every file the panes write.
- The letters "Cx" keep the candle's colour (as every pane label). The badge is coloured by the pivot (red at a high). Option C in the mockup (letters in the badge's colour) is NOT built; only on his word.

## Files
VolRangeLogic.h 1.10.0, VolRead.cpp (VR_VERSION and the setVersion literal 1.10.0; the two descriptions), RangeRead.cpp (its version line 1.10.0). gex-build.bat is not edited: both plugins already list VolRangeLogic.h. tests_volrange: t_vr1100.cpp (new) + nq_1123.csv, es_1120.csv, wy_NQ_1123.txt, wy_ES_1123.txt (his two windows); t_vr180.cpp (version pin); test_volrange_130.cpp (the pivot check now expects the file's line; two checks that had been stale since 1.3.5 / 1.8.0 corrected).

## Tests (2026-11-23, research machine)
- t_vr1100 (ASan / UBSan, -Werror, C++17 and C++14): 37 checks, 0 failed. The parser (Cx with and without a level, bad directions, a Bk / Sw never replaced by a Cx); NQ 6:21 / 6:45 / 6:48 AM and ES 12:21 / 12:57 PM / 1:33 PM exactly as the price chart; every Cx of the file on a loaded candle shown, none that the file does not have; the Range pane has none while Bk / Sw match the Volume pane; no file = no Cx; CB / SR / Ct over a Cx; the label drawn once, bold, over the badge's candle, the bars that lost it grey with their numbers; before the price chart writes the line the pane shows no Cx; bar by bar = one pass; the status lines. The old header fails these tests.
- Old suites on the new header: t_vr150_split 69, t_vr180 26, t_vr190 58, test_volrange_130 26 (37 with a 7,189-candle ES fixture): 0 failed.
- Before / after on EVERY candle, from the panes' own layout(): NQ 2026-11-17..23 and ES 2026-11-14..23, both panes, 9,148 candle-panes. Every count, every Bk / Sw / other mark and every bar size identical; the only differences are Cx. Volume pane: NQ 25 kept, 22 gone, 4 new (29 = the file's 29); ES 22 kept, 40 gone, 4 new (26 = the file's 26). Range pane: 49 and 57 gone. Where a Cx label left, a neighbour's number that had no room may now show (5 cases at 10 px a candle).
- Real irtsdk.h syntax check, C++14, -Wall -Wextra, VolRead.cpp and RangeRead.cpp: 0 errors, 0 warnings outside the SDK. Mock SDK compile, -Werror: ok.

## Not verified (needs his IRT)
- The panes on his charts after the restart: status files `VolRead.status-<MKT>-180.txt` / `RangeRead.status-<MKT>-180.txt` must say VERSION,1.10.0 and carry the CX line; his own look at a Cx badge and the label under it.
- The host (VolRead.cpp) was compiled, not run: only its version and description strings changed.

## Left as found
- tests_volrange/test_volrange_logic.cpp: 4 checks fail on the old AND the new header (they still expect the Rev mark removed in 1.3.5 on 2026-11-16). Not touched here.
