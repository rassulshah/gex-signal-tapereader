# lsTurn 1.0.1 "Turn v1.0" - mouse, strip position, ratios (2026-11-22)

## Requests (Rassul, 2026-11-22 CT, on his CL 3-minute chart after the first restart with Turn 1.0.0)
- 22:50 "indicator should update when my mouse hovers over different candles"
- 22:51, with a screenshot of the Turn pane: "top strip should be moved higher to the top of the pane, right now there is blackspace at the top . i cant see the ratios either. do you undertand"
- 23:00 "go" on the three fixes (estimate 75 min, $0; mockup first).
- 23:07, on the mockup (turn-pane-mockup.html): "the font for the ratio looks big , it may overlap on the other bar next to it, maybe get rid of the x and make it smaller"

## What changed (only what is DRAWN and which candle the strip is about)
1. **The strip follows the mouse.** While the mouse is in the Turn pane the strip is about the candle under it:
   - a sweep candle that shows its read: that sweep's strip (level, time, the four parts, the verdict, the odds pill);
   - a candle at a named level that is not a sweep: "<level> <time>   <word> Stop Run 2.9x / <word> Effort 2.8x" - what is drawn on it, nothing else (no Exhaustion, no Answer, no verdict; the pill is a dash). A candle that pushed into a named high AND a named low: the side the mouse is on (above the axis = the high);
   - a candle with no bars, the forming candle, or the mouse outside the pane: the newest sweep at or before the last candle in view, as 1.0.0. (First build of the day used "the newest sweep at or before the candle under the mouse" for a candle with no bars; dropped before delivery: before the first sweep the strip vanished and the bars jumped while the mouse moved.)
   - How: TRACK_MOUSE + `mouse()` (move and hover events; never swallowed), PaneCursor.h to notice that the cursor left the pane or is over another window (IRT sends no leave event) - the same parts as lsVolRead 1.2.0 / lsZzoCombo 3.1.0. A redraw is asked only when the candle (or the side of the axis) changes.
2. **The strip sits on the pane's top edge** (2 px under it; was 16 px, a row kept free for a pane title this NO_UI pane does not have). The grey "Turn: ..." line (nothing to read) moved up with it and is drawn only when no strip is.
3. **Ratios on the bars at every zoom.** 1.0.0 drew them only when two labels fit side by side on one candle (about 58 px a candle; his chart is 22 px), so he saw none. Now, on candles too narrow for two labels, each candle side shows ONE ratio: its taller bar's (equal: Stop Run / Exhaustion), centred on the candle, in that bar's colour. A label that would touch a label already kept or a bar of another candle is left out; the taller bar is asked first, then the newer candle (`turn::pickLabels`). Wide candles: every bar keeps its own, as 1.0.0.
4. **The ratio on a bar is 9 pt and has no x** ("2.9"; "14" from 10 up; "99+" cap; `turn::labelText`): 18 px, inside its own 22 px candle. The strip keeps "2.9x" / "13.7x". The Answer's "2/3" is unchanged.
- The bars keep a 13 px row above and below at every zoom (for the ratios); a pane too low for that draws the bars alone.
- Status file: new line `DISPLAY|1.0.1|strip on the pane top: a sweep / a candle at a level / none|ratios one per candle side N of M|mouse ...|mouse moves N` (what the draw before the last calc showed).
- NOT changed: every reading, the bars' heights and colours, the verdict, the records (turn-<MKT>-180.txt), the params, the feed, the nightly. Nothing the mouse does decides, records or repaints anything.

## Files
Turn.cpp, TurnLogic.h (1.0.1 = TURN_VERSION = turn::VERSION = the one setVersion literal), uses PaneCursor.h (already on the PC, unchanged since 2026-10-18). gex-build.bat is NOT edited: that shared file is only appended to, and the Turn line lists headers for change detection only - a later edit of PaneCursor.h needs Turn.cpp touched to rebuild lsTurn. tests_turn/test_turn.cpp, test_turn_host.cpp, soak.cpp. No parameter change, no new file on his disk.

## Tests (2026-11-22, research machine)
- Real irtsdk.h syntax check, C++14 and C++17, -Wall -Wextra: 0 errors, 0 warnings outside the SDK. Mock SDK compile C++14 -Werror: ok.
- Logic (ASan / UBSan, -Werror): 2,438,349 checks, 0 failed (378 before; the three label checks now expect no x). New: labelText (never wider than three characters), levelNameAt, candSide, stripOfCand on his CL candles of 11-22 (VAH 9:36 / 9:57 / 10:00 PM, PDH 10:12 PM), pickLabels - his chart at 22 px keeps all four labels with the 9 pt text and three of four with the first mockup's 10 pt "13.7x" (the taller neighbour wins); a label is never kept over another candle's bar; 20,000 random panes: kept labels touch nothing, every label left out has a reason, the answer does not depend on the order.
- Whole plugin on the mock SDK, Friday's real ES / GC order events (ASan / UBSan / LSan, -Werror): 2,957 checks, 0 failed (1,485 before). New: the strip box is rows 2..20 and nothing is drawn above it; the mouse on a sweep candle (8:51 LonHI), on a level candle (9:06 ONH), on both sides of a two-level candle (9:00: ONH above / PDL below), on candles with no bars, on the forming candle, hover events, bad bar numbers, points outside the pane, an unreadable cursor, leaving the pane (the next calc asks for one redraw), another chart's window, two charts on one DLL, scrolled out of view; after all of it the bars and the record file are byte-identical. Narrow candles: at 22 px 10 of 10 labels drawn, at 12 px fewer than offered and no overlap, at 3 px and in a 60 px pane no crash; no label lies over another candle's bar; restart without repaint still byte-identical.
- Golden candles through the C++: 110 sweeps + 45 mockup candles, 2,017 checks, 0 mismatches (the readings are untouched). Params files of the 7 markets parse.
- Soak with the mouse over every candle in every cycle: 40 cycles plain, memory growth 0 KB (435,524 KB after cycle 5 and after the last), 680 checks, 0 failed; 8 cycles under ASan / LSan: 136 checks, 0 failed, no leak report.
- Render check: the draw list at 22 px with the mouse on a level candle, painted from the plugin's own calls (work/render_narrow_mouse.png): strip on the top edge, one ratio per candle side in the bar's colour, none touching.

## Not verified (needs his IRT)
- The pane on his chart after the restart: the strip at the top, the ratios at his zoom, the strip changing under the mouse. Checked afterwards from the status file's DISPLAY line (ratios N of M, mouse moves) and his own look.
- That IRT calls mouse() for this NO_UI pane the way it does for lsVolRead (same flags there: POST_DRAWING | NO_UI | TRACK_MOUSE). If "mouse moves" stays 0 while he moves the mouse over the pane, the events are not arriving.
- The 9 pt label's real width in IRT (the mock measures 0.6 x size a character = 16 px for "2.9"); if it is wider than 20 px at his zoom, neighbours start to compete and the taller keeps its label.

## Seen while checking (not part of this change)
- CL Turn status at 23:03 CT: `reconnects 5`, `late 4581`, `bad 1284`, `timer off`, `candles 4 | complete 0` since the 22:31 restart - the order feed on CL restarted several times in a thin Sunday-night market and no candle was whole, so nothing is decided live there yet. NG: `FEED|waiting`, 0 events. Needs its own look.
