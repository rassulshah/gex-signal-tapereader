# QueueMeter 1.5.0 + lsLiquidityProfile / lsLiquidityFlow 0.57.0 - 2026-10-26 night

**Request** (Rassul 21:29 CT, approved Quality Check item 5 "I'll take your recommendation"): the Queue Meter's HELD / BROKE must use HIS HELD rule;
distances ATR-based (0.25 ATR, PARAM); '?' on the mark until the nightly proves it; no repaint; version bump, tests, real-SDK syntax, settings
contract, tests_perf.

## What changed
- **Decision (QueueMeter.h closeBar)** = his HELD rule from the definitions registry (2026-10-25): at a BID, a candle whose HIGH is below the bid =
  **broke**; a GREEN candle (close > open) closing above the bid, with its high not below it = **held**; neither by the 3rd candle after the
  contact candle = **not held** (red). Offer = mirror (LOW above the offer = broke; RED candle closing below = held). Decided once on closed
  candles, never repainted. The old queue-left fallback ("refilled" / "drained" at the 3rd close) no longer decides.
- **Break distance** brk() = 0.25 x ATR in ticks (Params.breakAtr, PARAM; the old breakTicks 2 is only the no-ATR fallback). It drives the live
  "broke" (badge / break risk) and the test-entry reach. The 2-tick value was 0.02 ATR in NQ and 0.4 ATR in 6E.
- **Badge**: " ?" after the badge text while the market's MARK is SHOW? (not proven: 20+ a side with lift). LRA-QueueMeter-ES.csv and -CL.csv set
  to PARAM,MARK,SHOW? (were SHOW). NQ / GC default SHOW? already, so they show the '?' too.
- **Host (LiquidityProfile.cpp, also lsLiquidityFlow)**: passes the bar OPEN to closeBar; the Holding-strip text and the typed evidence carry
  "not held" (adverse, mapped to rse::DRAINED). LQ_VERSION / setVersion / PLUGIN_VERSION 0.57.0. No settings change.
- **lsDealerProfile**: code unchanged (2.26.0); it rebuilds because QueueMeter.h is in its gex-build hash list - the badge '?' comes from
  qm::badgeText.

## Tests (Linux, this run)
- tests_qm/test_qm.cpp: 120 passed, 0 failed (ASan/UBSan -Werror). 16 old-rule scenarios rewritten; new: his held / broke / not held on bars,
  late green close within 3 bars, green close below the bid = not held, offer mirror, never decided twice (a later green close does not repaint),
  ATR break distance + fallback, '?' badge for SHOW? and none for SHOW.
- tests_qm/test_qm_lp_host.cpp (whole plugin on the mock host): 18 passed, 0 failed (fixture candles now agree with the prints; version pin 0.57.0).
- Real-SDK MinGW syntax (irtsdk.h): LiquidityProfile.cpp, LiquidityFlow.cpp, DealerProfile.cpp 0 errors; no new warnings vs the unchanged files
  (11 / 5 / 0 pre-existing).
- tests_settings: contract 44,250 checks 0 failures. settings_host_test: 1 FAIL "HodLod parameter version 7" - pre-existing (HodLod only, not
  touched here).
- tests_perf: perfguard 58/0, macro sim, e2e writer, real-SDK PerfGuard, perf_lint 0 new findings.
- test_dealerprofile_2200.cpp: its SHOW? assertion updated to expect " ?"; the harness itself segfaults in prime() on the unchanged tree too
  (stale harness, not run).

## Risks / not verified
- Not verified on his IRT. Held rate will be LOWER than the old queue rule (his rule needs a green close above the level within 3 candles).
- Old Q records of today (refilled / drained) still parse and read back.
- Step for Rassul: none tonight - the overnight GEX build compiles and installs when IRT is closed; restart IRT in the morning.
