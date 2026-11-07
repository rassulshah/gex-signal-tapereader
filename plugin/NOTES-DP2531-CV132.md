# DealerProfile 2.53.1 + ChartView 1.3.2 (2026-11-07, Rassul 13:09 CT "yes" - IRT review #3 small fixes)

Asked: per-market status files (a GC ChartView export carried ROOT EU) and a nightly check of IRT's level VALUES vs research.

Found: DealerProfile (2.17.0) and DeltaProfile (2.17.0) already write `<Name>.status-<MKT>-<spb>.txt`; the bug was the READERS:
ChartView copied the shared `DealerProfile.status.txt` / `DeltaProfile.status.txt` (last chart wins) and lra.code_health could pick it.

Changed
- ChartView 1.3.2 (ChartViewLogic.h CV_VERSION_STR 1.3.2): export copies DealerProfile / DeltaProfile / HodLod `.status-<MKT>-<sec>.txt`
  and `LRA-KeyLevels-<MKT>.csv`; never the shared names. No settings change.
- DealerProfile 2.53.1 (settings spec 2.53.1, no settings change): once per session, 3-min chart, 10:00-17:00 CT, appends
  `<YYYY-MM-DD>|<unix>|<name>=<price>;...` to `lsFlexLevels\LRA-KeyLevels-<MKT>-hist.csv` (dedupe on the file tail; marked done only
  after a flushed good append). Helpers in DealerProfileAuditLogic.h (histDue / histDate / histLine / histHas). Nothing on the chart changes.
- lra.consistency_check 1.2.0 step 8 LEVEL VALUES -> RULES-PARITY.md "## Level values" (> 1 tick = DIFF); `levels` CLI.
- lra.code_health 1.1.1 per-market status file beats the shared one; lra.chat_brief 1.1.0 one level-values line.

Tests: test_dealerprofile_2531 13/0, test_dealerprofile_274 44/0, test_chartview_logic 166 pass (4 fixture-file fails, same as before:
tests_cv110/fixtures absent), settings contract 45,949 checks 0 failures, MinGW syntax: only the SDK's own RT_LONG errors (same as before);
pytest test_consistency_check 10/10, test_code_health 9/9.

First real run (GC 11-06, IRT file vs research bars): 8 OK, 6 DIFF (ONL/AsiaLO 119 ticks, LonHI 83, PDH 71, PFH 78, LonLO 47) - the
research 3-min bars are missing GC 11-05 20:57-09:51 and 11-06 21:30-06:03 (289 / 203 of ~460 bars): a data gap, not a definition gap.

Not verified - needs his IRT: exit IRT, wait for "installed" in out\BUILD-STATUS.txt, reopen; after 10:00 CT Monday
LRA-KeyLevels-<MKT>-hist.csv should have one line per market.
