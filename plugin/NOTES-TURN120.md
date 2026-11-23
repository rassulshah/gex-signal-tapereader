# lsTurn 1.2.0 "Turn v1.2" - just Effort; the rest saved behind a display file (2026-11-23)

## Requests (Rassul, 2026-11-23 CT)
- 11:00 "show me this indicator with just effort .. show mockup" (mockup turn-just-effort-mockup.html: Turn pane as built / Just Effort / Mouse on a candle; real ES orders of Friday 8:24 - 10:33 AM, drawn by this plugin's code).
- 11:04, on the mockup: "meke values that are less than 1.5 gray" (mockup updated: 14 of 86 bars coloured).
- 11:06 "this look simple and easy  i like it.   lets use this for now.. save other setting because i might change my mind and add it pieces later" (= option 1 of the mockup: the Turn pane becomes this; estimate given 45 min, $0, one restart).
- 1.1.0 (Effort on every candle next to the turn read, built 10:29 AM) was never installed: this version takes its place.

## What the pane shows now (the default: no display file)
1. **Every closed candle: ONE bar a side, centred on the candle** (16 px at his 22 px zoom; 1.1.0 had two 9 px columns):
   - up = the buyers' Effort at the top of the candle (offer-traded lots per price level in the top 25% / per level in the rest);
   - down = the sellers' Effort at the bottom (bid-traded lots, bottom 25%).
   The numbers and the height scale are those of 1.1.0 (full height 3.0x). Nothing on the forming candle.
2. **Green / red only when the number written on the bar is 1.5 or more; grey under it.** The colour follows what the bar SAYS (`turn::shown10`, `effOn`): a value of 1.49 is written "1.5" and is coloured; 1.44 is written "1.4" and is grey. The number has the bar's colour.
3. **The top line** is about Effort only: the closed candle under the mouse, else the last closed candle in view - "<time>   Buyers <word> Effort 0.6x / Sellers <word> Effort 3.2x". The word (No / Light / Moderate / Heavy) and the colour follow the number written: only a Heavy side (1.5+) is green / red. No level, no verdict, no odds pill. The line is always there (a dash where a side has no bar), so the bars never jump under the mouse.
4. Not drawn any more: the Stop Run bar, the Exhaustion bar, the Answer bar, the read strip (level, the four words, the verdict, the odds).

## The other setting is saved, not removed
- Everything of the turn read is still READ, DECIDED ONCE AND RECORDED exactly as before (turn-<MKT>-180.txt, the nightly's scoring): only what is drawn changed. Tests: the record is byte-identical in both display modes.
- `lsFlexLevels\TurnPane\display.txt` switches the pieces (no file = Effort only). The plugin re-reads it when its stamp changes (at most every 5 s) and redraws: a piece comes back with no rebuild and no restart.
  ```
  TURNSHOW|1
  stop|1            the Stop Run bar (left column, at a named level / on a sweep)
  exhaustion|1      the Exhaustion bar (left column, the turn half of a sweep candle)
  answer|1          the Answer bar (right column, the turn half of a sweep candle)
  read|1            the read strip: level, the four words, the verdict, the odds pill
  effort_from|0.5   Effort bars coloured when the number written is this or more
  END
  ```
  A missing key keeps its default (0 / 1.5); an unknown key is ignored; a wrong value, a missing first line or a missing END = Effort only for everything, with the reason in the status file's SHOW line. Any piece on = two columns a candle again (Effort in the right one).
- **The saved full setting = `lsFlexLevels\TurnPane\display-full.txt`** (the block above). Copied to `display.txt` it brings back the 1.1.0 pane draw for draw, with one refinement: an Effort bar that reads 0.5 (value 0.45 - 0.499) is coloured, because the colour now follows the number written.
- To add a piece later: write `display.txt` with that piece's line set to 1 (Claude does it when he asks). To go back to just Effort: remove `display.txt` (move it to `_retired`).

## Files
Turn.cpp, TurnLogic.h (1.2.0 = TURN_VERSION = turn::VERSION = the one setVersion literal). gex-build.bat is NOT edited. tests_turn: test_turn.cpp, test_turn_host.cpp, soak.cpp, effmock.cpp (changed); eff_only.py, eff_full_vs_110.py, all120.sh (new). His disk: `lsFlexLevels\TurnPane\display-full.txt` (new, the saved setting; never read by the plugin).

## Tests (2026-11-23, research machine)
- Real irtsdk.h syntax check, C++14 and C++17, -Wall -Wextra: 0 errors, 0 warnings outside the SDK. Mock SDK compile C++14 -Werror: ok.
- Logic (ASan / UBSan, -Werror, C++17; C++14 -O2): 2,539,468 checks, 0 failed (2,439,588 in 1.1.0). New: the display file (defaults, the full setting, Windows line ends, one piece, unknown keys; 15 wrong files = Effort only with a reason; too big); shown10 = the label digit for digit over 32,000 values; the colour = the number written for every value 0 - 120 (1.4996 and 1.49 coloured, 1.4494 grey); the top line never disagrees with the bar (words, colours, dashes).
- Whole plugin on the mock SDK, Friday's real ES / GC order events (ASan / UBSan / LSan, -Werror): 8,519 checks, 0 failed. The whole 1.1.0 suite runs with the saved full setting; in the middle the display file is removed while the pane runs: the next calc asks for one redraw and the pane draws just Effort - every closed candle 8:42 - 9:06 AM has its one centred bar a side (40 px at 60 px a candle, 16 px at 22 px) with the height of a calculation made apart from the plugin, green / red exactly when its number is 1.5+, the number in the bar's colour, and NOTHING else under the top line; the top line = the last closed candle, grey ("9:06 AM   Buyers No Effort 0.3x / Sellers Moderate Effort 1.1x"), no pill; the mouse on a sweep candle (8:51, 9:03 - both sides 1.5+: green and red words), on the forming candle, off the pane, scrolled back; 22 / 12 / 3 px and a small pane; the record untouched; a garbled and a half-written display file = Effort only with the reason; ONE piece back (stop|1): two columns, the Stop Run left, Effort right on both halves of every candle, no Exhaustion / Answer; then the full setting back = the pane of before, draw for draw, the record unchanged.
- Friday's ES morning 8:24 - 10:33 AM (44 candles), the plugin's own draw lists:
  - the default at 22 px = the mockup he approved, draw for draw (no mouse; the mouse on 10:06 AM);
  - the default at 60 / 22 / 12 / 6 / 3 px: the 86 bars of the 43 closed candles = the calculation apart from the plugin (pandas), 14 coloured (6 green, 8 red), every number coloured as its bar says; restart after the morning with no order events: the same picture, the record unchanged;
  - the saved full setting against 1.1.0 at the five zooms, with and without the mouse: the same draws in the same order; the only differences are 7 Effort bars that read 0.5 (grey in 1.1.0, coloured now).
- Golden candles through the C++: 110 sweeps + 45 mockup candles, 2,017 checks, 0 mismatches (the readings are untouched).
- Soak, every other cycle Effort only and the others the full setting, the mouse over every candle: 40 cycles plain, memory growth 0 KB, 680 checks, 0 failed (the record byte-identical in every cycle = in both modes); 8 cycles under ASan / LSan: 136 checks, 0 failed, no leak report.
- Draw cost on the mock (our code): 300 candles at 22 px 0.4 ms; 1,000 at 6 px 1.1 ms; 3,000 at 3 px 3.5 ms.

## Not verified (needs his IRT)
- The pane on his charts after the restart. Checked afterwards from `status-<MKT>-180.txt`: VERSION|1.2.0, `SHOW|no display file (Effort only)|effort|effort coloured from 1.5`, DISPLAY `strip on the pane top: a candle|ratios one per candle side N of M|...|effort candles N`; and from Perf\perf-Turn-<date>.csv (draw p95; 1.0.1 was about 1.2 - 1.8 ms).
- The real width of the 9 pt numbers at his zoom (N of M tells: at 22 px the mock keeps every number).

## Left as found
- CL: `stop fills 15662` of `fills 19436` (81%) at 11:02 AM, the order-ID clock stuck at 431 points since the 9:38 AM load (ES / NQ at 4,096 and normal). The Stop Run recorded for CL since then is too high (it is not drawn any more, but it is recorded and scored). Fix offered (45 - 60 min), not approved.
