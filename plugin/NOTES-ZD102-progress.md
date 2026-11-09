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

## 1.0.3 (2026-11-08, Rassul approved the at-level floor)
- At-level floor = max(research floor, levelFloorX x chart floor); shipped CAPPED (1.0) in ES NQ CL GC HG NG, ON (0.5) in EU, after a
  first noise check on the 13 fixture days (> 10 new zones/day at 0.5-0.8x). The jumpstart raw days are on OneDrive (mbo-store\raw),
  not deleted: zone_defense 1.4.0 re-runs the check every night over all days (0.5 / 0.65 / 0.8x, chart floor, budget-capped, cached in
  data\zone_defense\floor\) and promotes levelFloorX for a market only at <= 10 new zones/day AND new at-level holds >= off-level (20+ each),
  written by the params writer. Built + installed 19:00 (settings contract passed).

## 1.0.5 (2026-11-09, Rassul: option (a) live floors + the levels bug)
- WHY: ES drew 0 zones overnight and at 07:23 - the research floors (complete Databento books) are 2-3x what a since-connect Rithmic
  book shows (ES best band 62-108 vs floor 198-247). FIX (a): the chart floors now come from the recorded LIVE book.
- zone_defense 1.5.0 live-floor step (nightly): the newest 10 recorded sessions per market (C:\LRA-Data\mbo\live\<MKT>\<date>.mbo.csv.gz,
  else data\irt\<date>\<MKT>\mbo_HH.csv in session order 16..23, 00..15) are streamed in time order into tests_zd\zdlive (the plugin's
  engine; a gap > 30 min = IRT closed -> cold book + 10-min warm-up, as on the chart) at 0.3x the research floor; floor = p60 of those
  zones per period, >= 0.3x research, research kept where < 20 zones, raised together to ~60 zones/day. Written by the params writer
  (apply_live_floors). Research floors kept in data\zone_defense\live\research_floors.json (no feedback); result in floors.json.
  NOTE: an archive file holds its session as 00..16 then 17..23 of the evening before - the stream puts the evening first (tested).
- Levels bug: refreshLevels judged the level files' freshness on the order stamp (CT-as-epoch, 5-6 h behind UTC) -> every file looked
  "from the future" -> "levels 0 (0 levels from 0 files)". Now zdh::wallNow() = the real UTC clock. Test: ES key file with 16 levels +
  CT stamps -> "16 levels from 1 files" (his 12 used); the old call reproduces the 0.
- Data on hand: ES 10 sessions; NQ 2, GC 3, CL 3, EU 3, HG 2 (live MBO archives start 11-04 for those); NG none (research floors).

## 1.0.6 (2026-11-09 09:03 CT, Rassul on NQ live)
- Bars at FULL pane height (drawn first); the read lines on top in one opaque near-black box (C_BOX_BG) sized to the text (the time
  column sits right after the widest verdict + detail, like ReadBox 2.22.1) - the bars are hidden only behind the text.
- Plain ASCII in every drawn string: " | " for the middle dot, "-" for the range dash, "->" for the arrow (his pane showed mojibake).
  t_zd checks every drawn string is 7-bit ASCII, the box holds every text and fits it, the bars come first and reach the pane top.
- ES "waiting for a zone touch" with no bars (09:xx): ES had ONE zone today (303 lots at the 299 morning floor, never touched), book
  12,017 orders. The bars are the lots sold into the latest TOUCHED zone (the engine records them only after a touch), so no touched
  zone = no bars by design, not a draw bug. 1.0.5's ES floors drop asia/london/preopen/open (135-225) but morning is 308 (was 299).

## 1.0.7 (2026-11-09 09:35-09:50, Rassul on ES live; picked panel 4 of claude.ai/artifact/EaKHxKQXTnxgg3ZMU9aeRV)
- The zone label on the price chart is SHORT: lots + the level the zone is AT ("301 PS0", "450 VAL+PS0"), lots only when no level is
  within the reach - same place (under a bid band / over an offer band, 8 px), plain ASCII (zdr::zoneShortLabel). The long label
  ("Zone lo-hi | lots (x) | oldest | state") stays only in the pane read.
- The band ~22% lighter (ZONE_LIGHT 1.22 x the 1.0.6 brightness). Diamond / triangle unchanged.
- Tests: t_marks 12 (short label format + 3 cases, band brightness); t_zd 47 (versions; a dangling mock host fixed - ASan).
