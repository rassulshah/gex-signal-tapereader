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
