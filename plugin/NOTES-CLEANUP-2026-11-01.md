# Plugin cleanup - 2026-11-01 (Rassul: "complete the clean up", "go with your recommendation")

## A) Plugins no longer used - retired
- Evidence: the last file each plugin wrote in lsFlexLevels (status / records). Silent since well before Fri Oct 30 = not on any chart; nothing
  still in use reads them (grep of the live plugins' code, comments excluded).
- Build list (tools\gex-build.bat): 38 build lines commented out (+ the hot-reload demo); it now builds 17: GammaProfile, DealerProfile,
  DeltaProfile, DeltaProfileHourly, HodLod, FirstExtreme, SessionVWAP, WavePOC, CandleAbsorb, IRTReader, ChartView, LiquidityProfile,
  BidAskStats, BookFlow, DefenseLines, Context, BigBidAsk.
- Retired: TapeFlow x7 + TapeFlowMarks, CVD, LiquidityFlow, DealerRead, DealerSig, DealerSummary, SessionInfo, TapeReader, TurnOrder, StopRun(Mark),
  SweepRefill, ReversalStage, KeyLevels, TrappedTraders, Trapped, TrappedMarks, CobImpulse, DealerHedge, ExhaustionV2, Exhaustion, MboCheck,
  FootprintReader x7, Pulling, KingTracker, TradeManager. Leftover DLLs (no longer built): IRTReader per-market x7, DayModel, DayStats, PullStats,
  StackStats, HotReloadDemo (+ hotreload folder), dllx64-hold (old ChartView backup).
- tools\retire-plugins-2026-11-01.bat (run with IRT closed): moves those DLLs to %USERPROFILE%\InvestorRT\_retired-2026-11-01, the sources to
  plugin\retired, their tests + the superseded Dealer Profile test snapshots (2.11-2.39) to plugin\retired\tests. Nothing deleted; undo = move back.

## B) Dead code in the plugins kept
- lsDealerProfile 2.40.1: ~700 lines that could no longer run removed (Dealer Profile bars render / node rows / header / strength marks, the pre-2.29
  Levels column + badge paths, GEX rows, the 2.35 code-2 zone mode, the 2.38 king rows, touch-tag rows / lane, badge rank chips + odds).
  test_dealerprofile_2400 25/0; the draw list is byte-identical to 2.40.0; MinGW vs the real irtsdk.h clean.
- lsLiquidityProfile 0.70.1: the tested order-line drawing (HELD / BREAK) and the G facts removed (records unchanged). MinGW clean;
  test_liqtouch_lp_host 13/2 (the same 2 known: retired code-2 check, pre-existing restart check).
