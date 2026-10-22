# lsLiquidityProfile 0.48.0 - LMT ZONES (LmtZonesLogic.h 1.0.0), 2026-10-22

## Request
- 09:34 CT, Rassul approved mockup v3 (`research-artifacts\learning\lmt-zones-mockup-v3.png`): "Ok build it", with two fixes:
  1. Gamma lines GREY, not purple. The yellow confluence diamond stays. ABC internal-breakout zones stay grey.
  2. Far fewer small order lines: draw only the zone orders and the far orders. Every other order stays in the LMT column.
- 09:36 CT: check how other platforms show large resting orders before finalising the drawing (see Research).
- Rules: `PULLBACK-ZONES-FINAL-2026-10-22.md`, section 5.

## What it does
- **Zone.** One REAL large resting limit order: one MBO order id at or above Big Lot S20, rested 30 s or more, re-priced at most once.
  - Pulled: an X, and the zone ends.
  - Re-priced once inside its band: the zone follows the order.
  - Re-priced a second time, or out of its band: an X.
- **Kinds.**
  - BD print (with-trend breakdown print area).
  - Origin.
  - Near (the biggest other real order within 2 ATR).
  - Far (2-3 ATR): thin line, map only.
  - ABC (internal break): grey, context only.
- **What each market draws.** Defaults below; the nightly `LRA-LmtZones-<MKT>.csv` overrides them.
  - NG / HG: offers drawn.
  - NQ: offers with "?".
  - CL: offers only when on a gamma level.
  - ES: only the with-trend BD-print offer, with "?".
  - GC / 6E: log only.
  - Bids: never drawn (LMT / LMT F only).
- **Gamma.** The whole MenthorQ set is drawn as thin grey lines with no label. A zone order within 0.25 ATR of one gets the yellow diamond.
- **Columns.** The BIG column becomes LMT (the zone orders resting now) | LMT F (the same order ids, lots filled; " I" = iceberg).
- **Hover.** Question, what it is, why it matters, how to use it (wait for the close back, never at the touch: -0.8R), study numbers, example, and its state now.
- **No repaint.** Zones are decided at the bar close and recorded (`<stem>-lmtzones-<session>.csv`, Z / M / F / E lines). After a restart they are read back, never decided again.
- **Settings.** 12 rows appended, parameter version 8:
  - master on / off;
  - print / origin / near / far / ABC / gamma lines / diamond;
  - 4 colours.
  - Stored in `LiquidityProfile.lmtzones-<MKT>.txt`.
- **Status.** `LMT_ZONES` line.

## Research: how other platforms show big resting orders (09:36 request)
The prior pass is in `INSTITUTIONAL-ORDERS-LMT-2026-10-22.md` section 1 (Sierra MBO columns, ATAS MBO DOM, Bookmap MBO / bait walls, Quantower DOM Surface, Jigsaw, CME MBO spec). These sources were added this time:
- [Bookmap heatmap settings](https://bookmap.com/knowledgebase/docs/KB-SettingUpAndOperating-HeatmapSettings)
- [Sierra Chart Market Depth Historical Graph (NexusFi)](https://nexusfi.com/sierra-chart/40835-market-depth-historical-graph-5.html)
- [ATAS indicators](https://atas.net/blog/atas-indicators/)

| Verdict | Idea | Source | How it is used here |
|---|---|---|---|
| ADOPT | Size threshold from the data (a percentile or the market's own distribution), not a fixed number | Bookmap upper cutoff as a percentile; Sierra per-symbol MBO minimum quantity | Big Lot S20 per market and RTH / overnight. Source order: nightly params, then the live LMT footer S20, then the study default. |
| ADOPT | Track one order id from resting to filled | Bookmap MBO bundle; CME MBO (the order id is constant for its life) | LMT and LMT F show the same order ids. A fill puts a ring on the chart. |
| ADOPT | Persistence filter against flicker and fleeting quotes | Bookmap bait walls; Hasbrouck & Saar fleeting orders | Rested 30 s+, re-priced at most once. The zone is decided only at the bar close, then only extends and ends once. |
| ADOPT | A pulled order looks different from a filled one | Bookmap (filled vs vanished walls); our own test (pulled orders = random) | X = pulled / moved (void); ring = filled. |
| ADOPT | Thin, low-contrast marks for context levels | Sierra users run markers at about 40% transparency | Far orders and gamma levels are 1 px lines; zone fills are 30% of the side colour. |
| CONSIDER | Cap the number of large marks (ATAS Adaptive Big Trades shows the 10 largest in view) | ATAS | Already capped: one zone per kind per side and one far line per side. The HG replay gives 38 drawn a day (16 zones + 22 far lines). If that is still busy, a per-screen cap is the next lever. |
| CONSIDER | Age shown on the order (ATAS Ctrl-hover shows the time placed) | ATAS MBO DOM | In the hover ("rested 2 min"). Not drawn on the chart. |
| NO | Heatmap or intensity shading of the whole book | Bookmap, Quantower DOM Surface, ATAS DOM Levels, Exocharts | That is the Liquidity Profile column's job; a heatmap on the price chart breaks his clutter rule. |
| NO | Queue position or per-order DOM columns | Sierra / Jigsaw / Rithmic DOM | DOM-only information; not readable on a 3-min chart. |

No approved rule or mockup element changed because of this research.

## Tests
- `tests_lmz/test_lmz.cpp`: 43 / 43.
- `tests_lmz/test_lmz_host.cpp`: 29 / 29 (mock host, whole plugin).
- `tests_lmz/replay_lmz.cpp` on HG 2026-10-20 (974k MBO events from the store):
  - 74 zones tracked; 38 drawn: 16 zones + 22 far lines.
  - Ends: 15 filled, 34 pulled, 7 moved.
  - A restart after every bar gives identical zones.
- Existing suites:
  - test_lqp044_host 21 / 21, test_lqp461_host 10 / 10, test_ap047 55 / 55 (version / parameter-version expectations updated).
  - test_lqp452_host and test_lqp453_host have 2 and 1 failures, the same before this change (the MKT column has been off since 0.46.0).
- Build checks:
  - check_hovers_off.py 39 / 39: the contract now allows the LMT zone hover only.
  - test_build_dependencies 300 passed.
  - MinGW against the real irtsdk.h, Profile and Flow builds, with -Dsmall=char: 0 new warnings (12 / 5, the same as 0.47.1).

## Risks / not verified (needs his IRT)
- **Gamma lines.** Nothing in his plugins drew purple gamma lines on the 3-min chart (the mockup did). This build adds thin grey lines for the whole `MenthorQLevels.csv` set. If IRT FlexLevels also draws that file, he will see both: untick "Gamma lines (grey)".
- **GC / 6E.** The study says "all markets draw the with-trend BD-print offer with ?". The coordinator's per-market list says GC no zones and 6E log only, so GC / 6E draw nothing. The nightly `PARAM,PRINT,2` can switch it on.
- **History.** Zones exist only from the moment the chart's order feed is live; there is no historical MBO.
- **Visual check.** The live chart look still needs his ChartView export after the install.
