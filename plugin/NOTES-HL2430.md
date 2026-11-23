# lsHodLod 2.43.0 "Now line only" (2026-11-22)

## Requests (Rassul, 2026-11-22 CT)
- 19:10, with a screenshot of the Now line, the Options line and the HOD / LOD box on his CL chart: "i need to simplify the display and only keep the now line".
- 19:36, on the mockup (now-line-mockup.html, today vs proposal) and the two questions (spoken alerts stay on? a version number on the line?): "1. yes.   2. no    go".

## What changed (only the DRAWING)
- NOT drawn any more: the HOD / LOD box (SW row, COB row, session line with close time / last trade date / market strength / Range %, the 1st / 2nd extreme table, note rows, the version number that lived in the box) and the Options (gamma) line with its +GAMMA / -GAMMA regime pill and 0DTE tag.
- Drawn: the Now line (the Level line: TESTING / WATCH / BREAK / SWEEP badge, Next, Last Bk), unchanged and live. It sits where the box's last row was: centred on the box's spot, its bottom edge on the box's bottom edge (the bottom of the price pane). No version number on it.
- Unchanged: the spoken sweep and Cob alerts, every value in the status file and the reads file, the Options line's nightly record (lsFlexLevels\HodLod\GammaLine-<MKT>-...csv), the 1st-extreme calls, the projection line, the LOD? / HOD? marker and the HOD / LOD confirm pills.

## How
- HodLodLogic.h: two constants (no settings - his rule), `HL_BOX_DRAW = false` and `HL_OPTIONS_LINE_DRAW = false`; `true` brings each element back exactly as in 2.42.4. `hl::nowSpot()` decides where the Now line goes: form 0 the box is drawn (4 px above it, the 2.42.4 arithmetic), form 1 the box was laid out but is not drawn (on its bottom edge), form 2 no box (the pane's bottom centre, as before).
- HodLod.cpp draw(): the box is still laid out (placement, hysteresis, the Key Levels dock, the width to the last candle - all unchanged) only for its SPOT; then the layout is redone with no table, which is the existing "Show table off" picture, so the projection runs uncut. The Options line is laid out for its status text and never painted; the regime pill is not put on the Now row instead.
- Status file: new line `DISPLAY,<version>,now line only (2.43.0),box NOT drawn - laid out at l t r b,options line NOT drawn (decided + recorded),now line drawn,spot form 1 ...`. `GAMMA_LINE` says "not drawn (2.43.0 now line only; decided + recorded)" and still carries the line's text. `TABLE_BOX` / `HLB_BOX` are no longer written (there is no table on the pane).
- Version 2.43.0 = hl::HL_VERSION = hlset::PLUGIN_VERSION = the one setVersion literal. No parameter change (setParameterVersion untouched). No new header (the gex-build line is unchanged).

## Tests (2026-11-22, research machine)
- Real irtsdk.h syntax check of HodLod.cpp, C++17 and C++14, -Wall -Wextra: 0 errors; the only warning outside the SDK is the old one in first_extreme_inference.cpp (same as before the change).
- tests_hodlod/t_hl2430.cpp (new, ASan / UBSan, -Werror): 2,265 checks, 0 failed - the switches and the version wiring; nowSpot (form 0 = the 2.42.4 arithmetic on 450 boxes, form 1, form 2, the pane clamp, note rows); the host's own sequence on the real layout for 6 markets x 6 pane sizes x projection up / down (72 cases): no part of the box is left, what is left equals the "Show table off" picture item for item, the projection is uncut in all 72 (the table cut it in all 72 before), his real Now line ("TESTING: 2hH Bk / Sw | Next 3hH . VAH | Last Bk PHH") lands inside the pane, left of the last candle, under the title row, centred on the box's spot with its bottom edge on the box's bottom edge, and is always less tall than the box.
- The existing suites, old source against new source (same binaries' results = no regression): see RESULTS below.

| Suite (logic-level, as on the PC) | 2.42.4 source | 2.43.0 source |
|---|---|---|
| t_hl2390 gamma line | 73 passed, 0 failed | 73 passed, 0 failed (one source-text check updated: the regime pill falls back to the Now row only while the Options line is drawn) |
| t_hl2391 | 39 / 0 | 39 / 0 |
| t_hl2380 | 35 / 0 | 35 / 0 |
| t_hl2360 Now levels | 99 / 3 | 99 / 3 |
| t_hl2410 | 17,671 / 2 | 17,671 / 2 |
| t_hl2420 | 288 / 1 | 288 / 1 |
| t_hl240 | 44 / 1 | 44 / 1 |
| t_hl2400, t_hl2370, t_hl244, t_hl270 | 5,957 / 2,740; 2,774 / 586; 28 / 21; 253 / 64 | identical |

No regression: every older suite gives the same result on the old and the new source. FOUND, NOT FIXED HERE: most of these suites already fail on the unchanged 2.42.4 source in this run - version pins (t_hl2410 / t_hl2420 expect HL_VERSION "2.33.0"), tests of designs replaced since (t_hl2400 = the 2.40.0 plan sections), and possibly fixtures this run did not have (t_hl2370, t_hl244, t_hl270 not examined). They need their own clean-up.

## Not verified (needs his IRT)
- The host's drawing was not run on a mock SDK (lsHodLod has no host harness; its tests are logic-level): the picture is checked after his restart from the status file's DISPLAY / NOW_LEVELS lines and a ChartView export.
- Other indicators that read the box's rectangle (the Limit Profile's "line parts under the HOD / LOD table skipped", the Big Lot dock) now see no table: their lines run through the freed space, as in the mockup. Not seen live yet.
- The MQ Reader's "hodlod_box" reads the status file's values (SWROW, the cells): those lines are still written; the Reader was not opened to confirm.
