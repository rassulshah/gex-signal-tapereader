# lsBreakout / lsBreakout1 1.18.0 (2026-11-11)

## What he asked (CT)
- 07:33 "also if there is a pb and it is below this level then it should be called a PbT"; 07:38 "it would be the dashed lne that would be tested ..
  can you update mockup"; 07:40 "update pboc to pbocT". Approved with "all" (07:46).
- 07:50 (ES screenshot) "this pb should have gotting a pboc too , it whoudl have been the second pboc"; rule chosen: "allow if it doens't close below".

## What changed
1. ORIGIN RULE (BO / BD Pb and the no-BO Rly Pboc): a pullback is void only when a candle CLOSES beyond its wave's origin. Before: any
   trade beyond it. His case: ES low 7604.75 under the Rly origin 7605.75, no close below -> the second Pboc now prints.
2. PbT / PbocT: a decided, drawn Pb / Pboc whose pullback (the candles after the wave end up to the pivot candle) traded at / through a
   Step Trend level of its direction is drawn "PbT" / "PbocT". The steps are lsStepTrend 1.2.0's (StepTrendLogic.h, clock hours CT,
   1-bar hourly rotation), computed natively from the chart's own closed candles up to the decision candle (no look-ahead, no file).
   A level counts while it is the newest level or still running as support (not tested before, no later step past it), from the candle
   after the one that made it, and only when the wave end was beyond it. Decided once with the Pb, never changed (no repaint).
   Periods that do not divide the hour: no T. Records: BPT|t_pivot|side|kind|level|t_test|t_dec|version in the -pb- file.
   Entry facts (Limit Profile ENT tags) keep the base name Pb / Pboc.

## Tests
- tests_bo100/test_bo1180_pbt.cpp (new, 24 checks, ASan / UBSan, real ES 11-09 / 10-21 / 11-04 replayed bar by bar): no label with a close
  beyond its origin; some with a trade beyond it; every T re-checked from the bars alone; text PbT / PbocT exactly for the T marks; a
  final label never changes; one straight pass = the replay; 7-minute chart = no T. Found: 5 PbT + 1 PbocT in the three days.
- The older suites, each run on its own before and after: no new failure except the intended ones (more Pbs under the close rule; text
  PbT / PbocT), which test_bo1130_pdoc / test_bo1140_entry / test_bo100_host now accept.
- FOUND: run_tests.sh has stopped at test_bo180_pb since 1.9.3 (it pins version 1.9.2), so the later suites were not run by the script,
  and 10 of the 14 suites already fail on 1.17.0 (stale version pins and rules replaced since). Not fixed here - needs its own pass.

## Not verified (needs his IRT)
- The look of PbT / PbocT on his charts (5 letters wide) next to the wave-time labels.
- Nightly scoring of PbT vs Pb (does the tested pullback win more) is not built yet.

# 1.19.0 (2026-11-11) - the Rev rule
- Rassul 11:07 CT: "update as follows: Its range is at least 1.2x the 13-bar ATR (measured on the bar before) or it closes above the prior bar
  high. also note that it can be displayed after any type of pb including pboc etc."
- Rev = the first bar of the right colour from the Pb pivot bar whose low is below the pivot bar's high and whose range is >= 1.2 x ATR(13)
  OR that closes above the prior bar's high (short: below the prior bar's low). Drawn after every pullback label: Pb, Pboc, PbT, PbocT and
  the no-BO Rly Pboc (its Rev was recorded as BRO only; now drawn too, the record line unchanged).
- Tests: tests_bo100/test_bo1190_rev.cpp (12 checks, ES 11-09 / 10-21 / 11-04 replay): every Rev re-checked from the bars, first qualifying
  bar, both kinds occur, the Rly Pboc Rev is drawn, replay = straight pass. test_bo1180_pbt 24 pass. MinGW 0 errors / 0 warnings.
- Not done: the in-sample PB Rev numbers (69% / +0.07R) were measured with the range-only rule; lra.pb_rev reads the plugin's records, so
  new days are scored with the new rule from tonight - the old table no longer describes this rule.

# 1.20.0 (2026-11-11) - the retracement under every pullback label
- Rassul 12:27 CT "I want to add the retracement amount right below any type of pb like 45%", "Keep it two digits".
- Under every DECIDED pullback label (Pb / Pboc / PbT / PbocT / the Rly Pboc): wave end -> pivot over origin -> wave end, two digits + "%",
  the label's colour and size; no room = not drawn; a provisional label has none. tests_bo100/test_bo1200_pct.cpp 11 pass (22 of 23 labels).

# 1.21.0 (2026-11-11) - a Rly only inside an hourly breakout bar
- Rassul 08:19 CT "the Rly ont the lower time frame ... should represent the breakout that is happening on the houly and be part of the hourly
  breakout bar"; mockup claude.ai/artifact/7pSqG2fDsUWHwFGFhfECDn; 10:21 "the rally is only displyed if it is consisetent witht the hourly
  breakout bar right"; 12:51 "Build".
- A Rly is drawn (and can start a Rly Pboc) only when the candle it sits on is inside a clock hour that is a Step Trend breakout hour of its
  direction. Decided once, when that hour has closed (kept in rlyHbo; a Rly can print later than before, never be taken back). Periods that do
  not divide the hour: rule off. Rly records (RLY lines) are unchanged - the rule is a display / pullback gate.
- tests_bo100/test_bo1210_rlyhbo.cpp 12 pass: ES 11-09 8 of 9 drawn, 10-21 8 of 18, 11-04 11 of 18 (= the mockup). 1180 / 1190 / 1200 still pass.
  MinGW 0 errors / 0 warnings.
- Not verified: his charts. Not done: lra.pb_rev / nightly Rly scoring do not know the gate yet (they read the RLY records).
