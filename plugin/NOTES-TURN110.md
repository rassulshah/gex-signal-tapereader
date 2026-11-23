# lsTurn 1.1.0 "Turn v1.1" - Effort on every candle (2026-11-23)

## Requests (Rassul, 2026-11-23 CT, on his CL 3-minute chart)
- 09:45, with a screenshot of the Turn pane: "does the turn indicator only show values when there is a level" (answer: yes - a candle draws only where it traded through a named high / low, and on sweep candles).
- 09:47 "can you show the indicator on all bars ?" (answer: Effort can go on every candle today, from the chart's own volume at price; Stop Run needs a level to be counted against, Exhaustion and the Answer are read at a sweep; estimate: mockup 30 min, build 1.5 h, $0, one restart).
- 09:51 "go -  for now just do effort"
- Mockup sent 10:09 AM (turn-effort-all-mockup.html: Today / Effort on every candle / Mouse on a candle / Option B), drawn by this plugin's own code from Friday's real ES orders, 8:24 - 10:33 AM, 22 px a candle. Built as "Effort on every candle" with today's colour rule; Option B (colour only from 1.0x) is NOT built - only on his word.

## What changed (only what is DRAWN)
1. **Every closed candle carries two Effort bars**, in the right-hand column (where Effort stands at a level):
   - up, green: the buyers' Effort at the TOP of the candle = offer-traded lots per price level in the top 25% of the candle / per level in the rest;
   - down, red: the sellers' Effort at the BOTTOM = bid-traded lots per level in the bottom 25% / per level in the rest.
   The same ratio, the same height scale (full height at 3.0x), the same grey under 0.5x and the same 9 pt number as the Effort bar of a candle at a level (`turn::effOf` -> `turn::ratioBar`). Under 4 price levels, or nothing traded in the rest: no bar.
2. **A candle at a named level** keeps its Stop Run (left column) and Effort of that side and gains the other side's Effort.
3. **A sweep candle with a read is drawn as before** (push: Stop Run + Effort; turn: Exhaustion + the Answer). Its other half has no room for the other side's Effort: there the right-hand bar is the Answer.
4. **Nothing on the forming candle.** A candle gets its bars when it has closed (its volume at price is read at the close and once more 5 s later, as before).
5. **The strip under the mouse** on a candle with no level: "<time>   Buyers <word> Effort 1.2x / Sellers <word> Effort 1.9x" (buyers green, sellers red, a No side grey, a dash where there is no bar; the numbers only when the bars are too narrow to carry every label - as the other strips). On a candle at ONE level: the half that has the level keeps the level's strip, the other half (only the other side's Effort bar stands there) shows the plain strip. A sweep with a read: its strip on either half. Mouse off the pane / on the forming candle: the newest sweep, as before.
6. Status file: the DISPLAY line ends with `|effort on plain candles N` (candles of the last draw with an Effort bar and no level); strip kind "a candle".
- NOT changed: Stop Run, Exhaustion, the Answer, the verdict and the odds (named levels / sweeps only), every reading, the records (turn-<MKT>-180.txt), the params, the order feed, the nightly. Effort is IRT's volume at price of the closed candle, as before: nothing new is recorded and nothing is read from the order feed for it, so past candles show it too.
- Faster drawing for a pane where every candle carries bars: `turn::pickLabels` finds the same labels through 64 px columns (the 1.0.1 double loop is kept as `pickLabelsPlain` for the tests); a label's width is asked once per text and draw.

## Files
Turn.cpp, TurnLogic.h (1.1.0 = TURN_VERSION = turn::VERSION = the one setVersion literal). gex-build.bat is NOT edited. tests_turn: test_turn.cpp, test_turn_host.cpp, soak.cpp (changed); effmock.cpp (the mockup's and the comparison's driver), eff_diff.py, eff_ref.py + eff_ref_ES.csv (the calculation apart from the plugin), mk_events_fast.py (= mk_events.py, byte for byte, vectorised), all110.sh.

## Tests (2026-11-23, research machine)
- Real irtsdk.h syntax check, C++14 and C++17, -Wall -Wextra: 0 errors, 0 warnings outside the SDK. Mock SDK compile C++14 -Werror: ok.
- Logic (ASan / UBSan, -Werror, C++17; C++14 -O2): 2,439,588 checks, 0 failed (2,438,349 before). New: effOf (his ES 9:51 AM candle, grey under 0.5x, no bar under 4 levels / NaN / negative, the 99+ cap), stripOfEffort (texts, colours, dashes, never a verdict), candPlainSide, and the fast label picker = the plain one on 600 random panes (up to 1,460 candles with bars on both sides, panes left of zero, empty / inverted boxes) and on boxes too far apart for columns; 1,500 candles x 2 sides: 1.9 ms a pick under ASan (the plain loop 28.5 ms).
- Whole plugin on the mock SDK, Friday's real ES / GC order events (ASan / UBSan / LSan, -Werror): 6,527 checks, 0 failed. New: EVERY closed candle 8:42 - 9:06 AM has its two Effort bars in the right column with the height, colour and number of a calculation made apart from the plugin (pandas, eff_ref.py: 9 of 9 candles to 0.0002); the left column holds a bar only where the record has a Stop Run; the two sweep candles keep their four bars (the right-hand bar going down is the Answer); nothing on the forming candle; the strip on a plain candle (wide and at 22 px), on the lower half of a one-level candle, on a sweep's lower half; a chart with no sweep file still shows Effort; a candle whose volume at price cannot be read and a one-price candle carry none; after every mouse move the bars and the record are byte-identical; restart without repaint still byte-identical.
- 1.0.1 against 1.1.0 on Friday's ES morning 8:24 - 10:33 AM (44 candles, 22 at a level, 2 sweeps), the plugin's own draw lists at 60 / 22 / 12 / 6 / 3 px: every box 1.0.1 drew is still drawn at the same place in the same colour, the strip is the same, the 59 new boxes are all Effort bars in the right column, and the Effort of all 43 closed candles equals the calculation apart from the plugin. Labels: at 60 and 22 px (his zoom) every old label is kept (52 / 27) and every new bar has its number (111 / 86 labels); at 12 px 13 of 15 old labels are kept (a taller new neighbour wins twice), at 6 px 11 of 13, at 3 px 6 of 7. Restart after the morning, no order events: the same picture, the record unchanged, at all five zooms, old and new.
- Golden candles through the C++: 110 sweeps + 45 mockup candles, 2,017 checks, 0 mismatches (the readings are untouched). Params files of the 7 markets parse.
- Soak with the mouse over every candle in every cycle: 40 cycles plain, memory growth 0 KB, 680 checks, 0 failed; 8 cycles under ASan / LSan: 136 checks, 0 failed, no leak report.
- Draw cost on the mock (our code, not IRT's painting): 300 candles at 22 px 0.4 ms (1,236 draw calls); 1,000 at 6 px 1.4 ms; 3,000 at 3 px 4.4 ms (6,877 draw calls). 1.0.1: 0.03 - 0.2 ms (under 100 calls).
- Render check: the mockup itself is the plugin's draw list (effmock.cpp) painted at 22 px.

## Not verified (needs his IRT)
- The pane on his charts after the restart: bars on every closed candle, the numbers at his zoom, the strip on a plain candle. Checked afterwards from `status-<MKT>-180.txt`: VERSION|1.1.0 and DISPLAY ... `effort on plain candles N` with N > 0.
- IRT's painting time with a bar on every candle (about 2 rectangles and up to 2 numbers a candle in view). If the chart feels slower when zoomed far out, the numbers can be dropped under a candle width.
- That past candles carry volume at price on every chart (today's status files say `vap missing 0` on ES / NQ / CL for 3,008 bars).

## Left as found (seen while checking, not part of this change)
- CL Turn status 9:59 AM: `stop fills 3356` of `fills 6202` (54%) since the morning restart; NQ, which showed 42% before that restart, is at 0.6% now. The Stop Run on the affected market over-counts until the next restart. The fix offered at 7:51 AM (45 - 60 min) is not approved yet.
