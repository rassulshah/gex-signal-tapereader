# lsBookFlow 1.33.0 + lsHodLod 2.17.7 - two box layout changes (2026-11-03)

Rassul said "go" to both mockups (bf_box.png and hodlod_wide.png, "now vs proposed").

## lsBookFlow 1.32.1 -> 1.33.0: the info box is now one line
- **Before:** the Defense pane's info box had 4 lines: "Are they defending?" with an (i) icon, "Live", the Buyers / Sellers heads and "Refill Effort". It covered about 40 bars.
- **After:** ONE compact line in the pane's top-left corner: yellow **Defending?**, green **B +923 0.6x**, red **S +534 0.3x**, then a small white state word.
  - The state word is **Live**, the candle's 12-hour time, n/a, or **Feed silent** while the order feed is silent.
  - The numbers are the same as the old Refill Effort row (`netCell`). Only the parentheses are dropped.
  - A small dark backing (bf::LC_BG) sits just behind the text. It is QUESTION_H = 16 px high, inside the strip the pane already keeps free on top, so the bars are never under it.
- **Removed:**
  - The (i) icon. It did nothing since 1.32.1 (no hovers).
  - The other box rows: Refilled, Counter, Re-test, Moved, At level, At node.
  - The Read word and lines.
  - These are still built and recorded (`e->lastInfo`, `e->lastRead`, nodeRowText, status) but not drawn. In his short pane they were never visible anyway.
- **Narrow pane:** the state word drops first. If even the core does not fit, nothing is drawn.
- **Versions:** BF_VERSION, bf::VERSION, ofset::BF_VERSION and the setVersion literal are all 1.33.0. No settings change.
- **Logic:** `BookFlowLogic.h` gets `bf::oneLine`, `bf::refillCells` and `bf::compactCell` (SDK-free). `BookFlow.cpp renderNet` draws the line.

## lsHodLod 2.17.6 -> 2.17.7: the box runs to the price axis
- **After:** the HOD/LOD box's right edge sits on the price axis (the price pane's drawing-area right edge: the scale rect's left, else the pane's right). It may cover the bottom of lsDealerProfile's LIQUIDITY column (Rassul accepted this).
- **Unchanged:** same rows, font and colours.
- **Where the extra width goes:** it is spread over SESSION | HOD SWEEPS | LOD SWEEPS in proportion to their widths. Labels stay left; values, bars and % stay right-aligned to each section's right edge. The 1st-extreme times spread across the session section.
- **How it works:** `hl::stretchToAxis` (HodLodLogic.h).
  - Pass 1 is unchanged: placement, hysteresis and the Key Levels dock.
  - Pass 2 lays the table out exactly (axis - left - 1) wide, with pass 1's font, and moves it onto pass 1's final left / bottom.
  - Only the table's own items and rectangles are replaced.
  - The narrow-pane 2.12.0 box (no sweep sections) is left as it was.
- **Rectangles for other indicators:** the published rectangles (LRA_HLB_ / LRA_HLT_, BoxDock.h) are written after the stretch, so lsBigBidAsk's docking and every plugin's table keep-out (TableWatch) see the wider box.
- **Status:** status line `AXIS_FILL,on|off,axis X,width A -> B,right R`.
- **Versions:** HL_VERSION, hlset::PLUGIN_VERSION and the setVersion literal are all 2.17.7. No settings change.

## Tests (Linux, ASan/UBSan, -Werror)
- tests_bf131/test_bf133.cpp (new): 13/13 pass. Covers the line parts and colours, one row high, the numbers equal the old Refill Effort cells, narrow-pane drop, nothing when there is no room, and the versions.
- tests_aggbook/test_aggbook_host.cpp -DBOOK: 126 pass, 1 fail. The fail is pre-existing (line 337, "inside the pane over candle 52": hovers are off since 1.32.1, and it failed before as well; before this change it was 122 pass, 1 fail).
  - The box-row checks now read `e->lastInfo`.
  - New checks: one line top-left, height <= QUESTION_H, B / S equal the Refill Effort cells, colours, every part inside the backing in order, nothing else drawn in the line, no bordered box, no (i) icon.
- tests_hodlod/t_hl2177.cpp (new): 371/371 pass.
  - Right edge equals the axis (+-2 px) at 4 pane widths x 3 positions x 2 font metrics, plus docked or moved tables.
  - Left / top / bottom / height unchanged.
  - Sections proportional and butted up; the same texts (none lost); all inside; no overlaps.
  - Same fonts and colours; % right-aligned per section; the version at the right end.
  - Unchanged when there are no sweep sections or the axis is left of the box.
  - Versions; the host calls the stretch after the dock and before execute / publish.
- Existing tests show identical failure lists before and after. All of these failures are pre-existing (old version pins and an older design):

  | Test | Pass | Fail |
  |---|---|---|
  | t_hl2130 | 48 | 0 |
  | t_hl2120 | 23 | 2 |
  | t_hl244 | 29 | 20 |
  | t_hl230 | 45 | 1 |
  | t_hl240 | 43 | 2 |
  | t_hl270 | 316 | 1 |
  | t_po260 | 130 | 3 |
  | t_hl2170 | 20 | 0 |
  | test_bf131 | 10 | 0 |
  | test_bf132 | 16 | 1 |
  | test_aggbook_113 | 180 | 2 |
  | test_aggbook_115 | 89 | 0 |

- Settings contract: 45,949 checks, 0 failures.
- MinGW `-fsyntax-only` against the real irtsdk.h: BookFlow.cpp and HodLod.cpp have 0 errors and 0 non-SDK warnings.

## Not verified (needs his IRT)
- The real look after a restart, on 2+ markets.
- Whether IRT's getScaleRect() left equals the axis on his chart. The code falls back to the pane's right.
- Overlap with lsBigBidAsk (see Risks).

## Risks
- **lsBigBidAsk** docks its Big Lot box right of the table first (BoxDock placeMatched). With the table at the axis that spot is gone, so it tries left of the table, then its own Position avoiding the table. It may move. BigBidAsk files were NOT touched (another agent is editing them).
- **lsDealerProfile LIQUIDITY column:** its bottom rows are under the box. Plugins that use the table keep-out hide marks there.
- **BookFlow:** the rows that were only visible in a tall Defense pane (Refilled / Counter / Re-test / Moved / At level / At node / Read) are no longer drawn.
