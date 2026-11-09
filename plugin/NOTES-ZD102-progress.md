# Zone Defense 1.0.2 - progress (2026-11-08, end of the build pass)

All 1.0.2 sources are on the PC (plugin\ZoneDefense*.{cpp,h}, tests_zd\, analytics\lra\zone_defense.py 1.2.0, setup_scorecard.py 1.3.1).
Tests (g++ + the mock SDK): t_zd 40/40, t_zd102 30/30, t_marks 10/10, test_zone_defense 11/11, test_setup_scorecard 5/5.
MSVC compile: the PC's GEX build job (every ~2 min) - see plugin\out\BUILD-STATUS.txt.

| Item | Status | Note |
|---|---|---|
| Review fixes H1-H4, M1-M8, L1-L3, L5 | DONE | M5 = the ended-order trim every minute (was every 15) |
| Review fix L4 (bar sizes) | DONE | Bars are cut on the epoch: only whole-minute sizes that divide the hour (1/2/3/5/10/15/20/30/60 min). Others (90 s, 45 min, 2 h): no engine, the pane / status say "unsupported bar size" (stricter than 'divides 86400': a 2-h grid is off the 17:00 CT start) |
| Review fix L6 (Central clock) | DONE | Every second the PC offset is compared with America/Chicago (DST rules built in); a mismatch shows ONE white "Clock" line in the pane and a WARNING in the status |
| Bar-stamp resolution | DONE | Inside the plugin IRT bar times are bar ENDs (BUILD-LESSONS 2026-10-29); the chart exports are bar STARTs. Marks keep barOfEnd (strict) |
| Live-style timing + jumpstart re-run | DONE | 71 days x 7 markets |
| Zone states (10-bar window) | DONE | t_zd LIVE vs CLOSE allows broken -> "broken -> reclaimed" (by design) |
| Sweep bug (GC Oct 26) | DONE | Fixture tests_zd\fx\GC-2026-10-26-sweep.bin |
| Queue normal 0% in GC/HG | DONE | Same-distance normal; the per-period numbers refresh with the nightly |
| Level tags | DONE | Engine + label "at VAL + PS0" + live level files (5 s) + nightly levels files and reach tuning |
| Recording/backfill watchdog row | DONE | |
| All-7-market parity | DONE | 13 fixtures, 7 markets: plugin path = engine, line for line (3,747 lines) |
| NQ memory / time | DONE | Kept ended orders NQ 1,000,022 -> 11,831 (same output on all 13 fixtures); 0.69 us/event plugin path |
| Scorecard test fix | DONE | No Zone Defense line (and no import) until the nightly has run |
| OneDrive online-only step | DONE | zone_defense 1.1.0 rehide (kept in 1.2.0) |
| Rithmic vs Databento day | WAITING | Needs the 1.0.2 install and one recorded live day (no ZoneDefense journal yet) |
| Build hash list | DONE | gex-build.bat: MarketKey.h added to ZoneDefense, NearLevels.h + MarketKey.h to ZoneDefenseMarks |
| GC Nov 6 example mockup + fixture | DONE | Mockup claude.ai/artifact/MavTNdRrT2c87VWgxzR9oD (from the raw portal DBN; his chart levels). Fixture tests_zd\fx\GC-2026-11-06-sweep.bin + 2 tests in t_zd102. FINDING: at the chart's GC floor (32-36 lots) neither stage draws a zone (the WPoc zone is 21 lots); only at the research floor 15 - his decision whether zones AT his levels get a lower floor |

Nightly 1.2.0: scores go to data\zone_defense\zr102\ (all days re-scored with the 1.0.2 engine, budget-capped); levels files in
data\zone_defense\levels\ (checked against the jumpstart's files: identical on HG 10-26); ZoneDefenseParams.h rows gain levelReach.
Live volume levels (1.2.0 nightly + host): the nightly writes lsFlexLevels\LRA-VolLevels-<MKT>.csv for the NEXT session
("SYMBOL|", "DATE|", "LEVEL|code|what|price": DPoc / VAH / VAL of the last stored session, WPoc of the prior week, standard 70% value
area). The pane uses it only when DATE = its session and SYMBOL = the chart's contract.
DEPENDENCY: the MBO store's trades table must hold the full prior session. A partial recording (< 25% of the usual rows, e.g. GC
2026-11-05 in the store has 2,170 trades) writes NO file - that day's tags fall back to key + MenthorQ.
NOT YET VERIFIED against his chart: GC Nov 6 VAL 4053 / WPoc 4055. The store cannot check it now (GC 11-04 missing, 11-05
partial; the prior week's POC computes to 4040.0, not 4055 - his WPoc may be the CURRENT week's developing POC). Re-check on the
GC Nov 2-6 data once the conversion lands, and switch the definition to what his chart uses.
