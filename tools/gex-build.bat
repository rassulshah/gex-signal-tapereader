@echo off
setlocal EnableDelayedExpansion
REM ============================================================================
REM  GEX BUILD  -- installed by setup-gex-build.bat  (2026-09-17)
REM  Every 2 minutes, hidden: for each RTX plugin, if its source (the .cpp + the
REM  logic headers) changed since the last build, compile it into plugin\out\;
REM  then, if the built DLL is not the installed one, try to copy it into the
REM  Investor/RT extension folder. A loaded DLL cannot be replaced, so while IRT
REM  is open the copy fails harmlessly and is retried every tick -- close IRT
REM  and the new build installs itself within two minutes; reopen IRT to load it.
REM  Nothing changed = nothing happens. Never interactive.
REM  Log: tools\gex-build.log   Status: plugin\out\BUILD-STATUS.txt
REM  (2026-10-22, MBO recorder fix P3) NO INSTALLS ON WEEKDAYS 07:00-15:15 CT: a build made in trading hours is HELD and installs
REM  only after 15:15 (when IRT is closed). An IRT crash or close at midday no longer loads new, unwatched code into the recording
REM  session (10-20 12:43-12:50: 5 installs right after the 12:49 crash; 10-21 11:59-12:30: 47 installs after an 11:52 close).
REM  Rassul can still ask for a midday install: Claude creates plugin\out\INSTALL-NOW.flag (honoured until 15:15, then deleted).
REM  NO POWERSHELL ANYWHERE (Avast flags it: IDP.HELU.PSE88).
REM ============================================================================
set REPO=C:\Dev\gex-signal-tapereader
set PLUG=%REPO%\plugin
set OUT=%PLUG%\out
set LOGF=%REPO%\tools\gex-build.log
set LOCK=%REPO%\tools\gex-build.lock
set STATUS=%OUT%\BUILD-STATUS.txt
set "SDK=C:\Program Files\LinnSoft\InvestorRT\sdk\c++"
set "LIB143=%SDK%\lib\irtsdkV143-x64.lib"
set "DLLDIR=%USERPROFILE%\InvestorRT\dllx64"

if not exist "%PLUG%" exit /b 1
if exist "%LOCK%" exit /b 0
echo %DATE% %TIME% > "%LOCK%"
if not exist "%OUT%" mkdir "%OUT%"
call :window

set VCOK=
set ANY=
REM (2026-10-18 settings audit, Rassul 15:13 CT "make sure every indicator always has its settings verified and tested so there are not bugs
REM   anymore") THE SETTINGS CONTRACT GATES EVERY BUILD: plugin\tests_settings\settings_contract.cpp (SettingsSpec.h + every <Name>Settings.h +
REM   the migrated sources) is compiled and run first; FAIL = nothing is built this tick (the installed DLLs stay), the log says why once,
REM   plugin\out\SETTINGS-CONTRACT.txt has every failing check. SETSRC = the plugin sources on the layer (add one when it migrates).
set "SETSRC=HodLod.cpp DeltaProfile.cpp TapeReader.cpp BidAskStats.cpp BookFlow.cpp LiquidityProfile.cpp TradeManager.cpp DealerProfile.cpp SessionVWAP.cpp BigBidAsk.cpp CVD.cpp CobImpulse.cpp DefenseLines.cpp KeyLevels.cpp DealerHedge.cpp ReversalStage.cpp"
call :settings
if defined SETFAIL (
  call :status
  del "%LOCK%" >nul 2>&1
  exit /b 0
)
call :one GammaProfile lsGammaProfile GammaProfileLogic.h ContractOffsetLogic.h SettingsGuard.h SettingsHost.h SettingsSpec.h
REM (MT100 2026-10-09) lsDayModel + lsDayStats RETIRED - superseded by lsHodLod (+ SessionInfo) and the HOD / LOD studies.
REM   Their sources are kept in plugin\retired\ (see plugin\retired\RETIRE-NOTES.md); they are no longer built or installed.
call :one KingTracker  lsKingTracker  KingTrackerLogic.h ContractOffsetLogic.h SettingsGuard.h SettingsHost.h SettingsSpec.h BoxDock.h BannerBoard.h KeepOutDraw.h
REM (2026-10-18 night) Market State posts: MarketState.h / BannerBoard.h / SignalBoard.h 1.1.0 / Fresh.h added to the hash lists of every plugin that posts or reads it (LQP 0.44.0, BidAskStats 1.13.0, BookFlow 1.3.0, DeltaProfile 2.17.0, BigBidAsk 1.15.0, HodLod 2.4.0, DealerProfile 2.17.0, SessionVWAP 1.6.0, CobImpulse 0.4.1, TapeReader 1.3.0)
REM (2026-10-14 CR5200) the right-side columns: ColumnBoard.h shared by DealerProfile 2.11.0 / DeltaProfile 2.8.0 / LiquidityProfile + Flow 0.16.0
REM (2026-10-21 night) lsDealerProfile 2.19.0 level % badges: LevelPctLogic.h added to its hash list; ColumnBoard.h 1.3.0 (Levels column up to 250 px) rebuilds Delta / Liquidity / BigBidAsk too
call :one DealerProfile lsDealerProfile DealerLogic.h DealerProfileAuditLogic.h HostSlot.h OnTouchParams.h KeyLevelsLogic.h SessionVWAPLogic.h OnTouchLogic.h ColumnBoard.h DealerColumnsLogic.h BoxPlace.h BannerBoard.h ClaimGates.h Fresh.h MarketState.h DealerProfileSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h LevelPctLogic.h QueueMeter.h
call :one DealerRead   lsDealerRead   DealerLogic.h HostSlot.h BoxPlace.h ClaimGates.h DealerReadGates.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one DealerSig    lsDealerSig    DealerLogic.h ClaimGates.h SettingsGuard.h SettingsHost.h SettingsSpec.h BoxDock.h BannerBoard.h KeepOutDraw.h
call :one DealerSummary lsDealerSummary DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
REM (2026-10-21) DeltaProfile 2.20.0 / BigBidAsk 1.20.0 hash AbsorbLogic.h (wick absorption + absorbed-print circle); BidAskStats 1.25.0 hashes SignalBoard.h (posts kind O)
REM (2026-10-22) the SHARED absorption A: SharedAbsorb.h 1.0.0 in DeltaProfile 2.22.0 / BidAskStats 1.27.0 / BookFlow 1.22.0 / BigBidAsk 1.22.0 (one rule, one record lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv)
call :one DeltaProfile lsDeltaProfile AbsorbLogic.h TrapLogic.h DeltaProfileCore.h ColumnBoard.h BoxPlace.h BannerBoard.h SignalBoard.h ZoneNear.h MarketState.h DeltaProfileSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h BoxDock.h KeepOutDraw.h SharedAbsorb.h
call :one DeltaProfileHourly lsDeltaProfileHourly DeltaProfileHourlyCore.h DeltaProfileHourlyEngine.h DeltaProfileHourlyParams.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one SessionInfo  lsSessionInfo  DealerLogic.h SessionInfoLayout.h HodLodLogic.h HodLodExpected.h HodLodExpectedV2.h HodLodMedians.h HodLodReadTables.h HodLodEvents.h HostSlot.h
REM (2026-10-22) lsHodLod 2.6.0 - the PRE-OPEN BOOK row (Rassul 15:57 CT "yes hod lod box"; MBO Roadmap T-20261022-29): PreOpenLogic.h hashed
call :one HodLod       lsHodLod       HodLodLogic.h HodLodGex.h HodLodExpected.h HodLodExpectedV2.h HodLodMedians.h HodLodReadTables.h HodLodEvents.h HostSlot.h HodLodFx.h HodLodQ.h FirstExtremeLogic.h FirstExtremeView.h first_extreme_inference.cpp first_extreme_state.hpp BoxPlace.h Fresh.h MarketState.h BannerBoard.h VwapTrendLogic.h BoxDock.h HodLodSettings.h SettingsSpec.h SettingsHost.h DealerProfileSettings.h OnTouchLogic.h OnTouchParams.h SessionVWAPLogic.h SettingsGuard.h PreOpenLogic.h
REM (CR204 2026-10-12) lsFirstExtreme: the First Final Extreme v2 model, shadow mode - (CR210) RETIRED stub 2.0.0: the model runs inside lsHodLod 2.0.0
call :one FirstExtreme lsFirstExtreme FirstExtremeLogic.h FirstExtremeView.h first_extreme_inference.cpp first_extreme_state.hpp HostSlot.h
call :one TradeManager lsTradeManager DealerLogic.h HostSlot.h TradeGuardLayout.h ClaimGates.h TradeManagerSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h
call :one SessionVWAP  lsSessionVWAP  DealerLogic.h TouchParams.h HostSlot.h SessionVWAPLogic.h OnTouchParams.h OnTouchLogic.h VwapTrendLogic.h VwapOddsLogic.h VwapOddsParams.h SessionVWAPStyle.h MarketState.h BannerBoard.h SessionVWAPSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h BoxDock.h KeepOutDraw.h
call :one IRTReader    lsIRTReader    IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderES lsFootprintReaderES IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderNQ lsFootprintReaderNQ IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderCL lsFootprintReaderCL IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderGC lsFootprintReaderGC IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderHG lsFootprintReaderHG IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderNG lsFootprintReaderNG IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one FootprintReaderEU lsFootprintReaderEU IRTReader.cpp IRTReaderLogic.h DealerLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ArrivalStamp.h
call :one TapeFlowES lsTapeFlowES TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowNQ lsTapeFlowNQ TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowCL lsTapeFlowCL TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowGC lsTapeFlowGC TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowHG lsTapeFlowHG TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowNG lsTapeFlowNG TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowEU lsTapeFlowEU TapeFlow.cpp TapeFlowLogic.h TapeFlowSupport.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one TapeFlowMarks lsTapeFlowMarks TapeFlowLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h BoxDock.h BannerBoard.h KeepOutDraw.h
call :one ChartView    lsChartView    ChartViewLogic.h HostSlot.h DealerLogic.h ChartViewCamera.h SettingsGuard.h SettingsHost.h SettingsSpec.h
call :one MboCheck     lsMboCheck     MboCheckLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
REM (LQ010 2026-10-11) the Liquidity Profile - native depth + VAP, TESTING
REM (LQ150 2026-10-13) 0.15.0 includes the order-by-order feed: rebuild when OrderFlowFeed.h / OptionsFlow.h / NearLevels.h change
REM (2026-10-14 night) lsLiquidityProfile 0.18.0: the biggest resting order lines (BigOrderLines.h)
REM (2026-10-22) lsLiquidityProfile / lsLiquidityFlow 0.48.0: the LMT zones (LmtZonesLogic.h)
call :one LiquidityProfile lsLiquidityProfile LiquidityProfileLogic.h HostSlot.h KeyLevelsLogic.h SessionVWAPLogic.h OnTouchLogic.h OnTouchParams.h OrderFlowFeed.h OptionsFlow.h NearLevels.h HoverBoard.h BannerBoard.h BoxPlace.h ColumnBoard.h ZoneRule.h ZoneRead.h BigOrderLines.h MarketState.h SignalBoard.h BigFlow.h LiquidityProfileSettings.h SettingsSpec.h SettingsHost.h BannerSettings.h LargeLotPool.h SettingsGuard.h ZoneNear.h MoveLogic.h AirPocketLogic.h BoxDock.h KeepOutDraw.h LmtZonesLogic.h QueueMeter.h
REM (LQ020 2026-10-11) the aggression / defense panes - the same source built as its own pane
call :one LiquidityFlow lsLiquidityFlow LiquidityProfile.cpp LiquidityProfileLogic.h HostSlot.h KeyLevelsLogic.h SessionVWAPLogic.h OnTouchLogic.h OnTouchParams.h BannerBoard.h BoxPlace.h ColumnBoard.h OrderFlowFeed.h OptionsFlow.h NearLevels.h HoverBoard.h ZoneRule.h ZoneRead.h MarketState.h SignalBoard.h BigOrderLines.h BigFlow.h LiquidityProfileSettings.h SettingsSpec.h SettingsHost.h BannerSettings.h LargeLotPool.h SettingsGuard.h ZoneNear.h MoveLogic.h AirPocketLogic.h BoxDock.h KeepOutDraw.h LmtZonesLogic.h QueueMeter.h
REM (2026-10-11) lsPulling - who pulled their orders near price, from the order-by-order (MBO) feed
REM (2026-10-14 night) lsPulling RETIRED stub 1.0.0: its read is lsPullStats 1.5.0's line (ZoneRead.h 1.1.0); the stub includes no header (0.3.0 source in plugin\retired\Pulling_0_3_0.cpp)
call :one Pulling lsPulling
REM (2026-10-11) lsExhaustion - the pusher fading at a new extreme, per-market settings from lra.exh_study
call :one Exhaustion lsExhaustion ExhaustionLogic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
REM (2026-10-12) lsExhaustionV2 - pullback watch E? / trend response R? / EXIT? warnings (outside review), calibration from lra.exh2_nightly
call :one ExhaustionV2 lsExhaustionV2 ExhaustionV2Logic.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h
REM (2026-10-12) lsCVD - CVD line + delta-pct bars + Div / Abs / Exh / BR marks on the CVD line (outside package CVD Signals v0.1, reviewed), TESTING
call :one CVD lsCVD CvdEngine.h CvdFeed.h HostSlot.h KeyLevelsLogic.h SessionVWAPLogic.h OnTouchLogic.h OnTouchParams.h CVDSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h
REM (2026-10-13) lsBidAskStats - biggest bid (red) / biggest ask (green) per candle, BBid / BAsk / BidRatio / AskRatio box
REM (2026-10-13) lsBidAskStats 1.3.0 - who is attacking: S / L letters from the order feed, message line + info box, spike labels (shared OrderFlowFeed.h)
REM (2026-10-13 of140) BidAskStats 1.5.0 / PullStats 1.1.0 / StackStats 1.1.0 also hash OptionsFlow.h (BidAskStats StopRun.h); TapeReader 1.1.0 hashes OptionsFlow.h
REM (2026-10-13 fix1) BidAskStats 1.5.1 / PullStats 1.1.1 / StackStats 1.1.1 / TapeReader 1.1.1 / LiquidityProfile also hash HoverBoard.h (the chart-wide hover)
REM (2026-10-21 20:23 CT) lsBidAskStats 1.26.0 attacker letters: AttackLogic.h + DefenseTurn.h / TrapLogic.h / DefenseLinesSettings.h hashed (G / Tr / Defense letters read from lsBookFlow / the Delta Profile records)
call :one BidAskStats lsBidAskStats SignalBoard.h BidAskStatsLogic.h OrderFlowFeed.h NearLevels.h HostSlot.h StopRun.h OptionsFlow.h HoverBoard.h BoxPlace.h ZoneRule.h ZoneRead.h MarketState.h BannerBoard.h OrderFlowPaneSettings.h SettingsSpec.h SettingsHost.h BigLotTiers.h SessionVWAPLogic.h PaneCursor.h BoxRead.h BoxReadDraw.h EffortReward.h LargeLotPool.h OnTouchLogic.h OnTouchParams.h SettingsGuard.h MoveLogic.h AttackLogic.h DefenseTurn.h TrapLogic.h DefenseLinesSettings.h SharedAbsorb.h LetterStrips.h
REM (2026-10-14 night) lsPullStats 1.5.0 - lsPulling's arrival read merged in (ZoneRead.h 1.1.0; the line shows only the locked read)
REM (2026-10-13) lsPullStats 1.0.0 - who is leaving: resting orders pulled near price per candle (MBO), L, message line + info box (Rassul "number of bids and ask pulled like a histogram")
REM (retired 2026-10-16) call :one PullStats lsPullStats PullStatsLogic.h OrderFlowFeed.h NearLevels.h HostSlot.h OptionsFlow.h HoverBoard.h MboRecord.h BoxPlace.h ZoneRule.h ZoneRead.h
REM (2026-10-13) lsStackStats 1.0.0 - who is arriving: orders added near price per candle (MBO), refill dash, R / L, message line + info box (Rassul "add a stacking indicator", "keep seperate")
REM (retired 2026-10-16) call :one StackStats lsStackStats StackStatsLogic.h OrderFlowFeed.h NearLevels.h HostSlot.h OptionsFlow.h HoverBoard.h BoxPlace.h ZoneRule.h ZoneRead.h
REM (2026-10-16 Rassul 07:49 CT "dont keep the old panes, replace them") lsPullStats + lsStackStats RETIRED from the build: lsBookFlow 1.0.0 replaces both
REM   (one net bar per side; it still writes the StackStats\ and PullStats\ records and the PullStats zone read). Their sources stay in plugin\ for reference.
REM (2026-10-21) lsBookFlow 1.19.0 reads the Delta Profile's TR record (Tr on the Defense badge): TrapLogic.h hashed
call :one BookFlow lsBookFlow BookFlowLogic.h StackStatsLogic.h PullStatsLogic.h OrderFlowFeed.h NearLevels.h HostSlot.h OptionsFlow.h HoverBoard.h MboRecord.h BoxPlace.h ZoneRule.h ZoneRead.h MarketState.h BannerBoard.h OrderFlowPaneSettings.h SettingsSpec.h SettingsHost.h PaneCursor.h BoxRead.h BoxReadDraw.h DefenseTurn.h DefenseLinesSettings.h EffortReward.h LargeLotPool.h SettingsGuard.h MoveLogic.h TrapLogic.h SharedAbsorb.h ArrivalStamp.h LetterStrips.h
REM (2026-10-20 Rassul 12:22 CT "first build the lines, do it now") lsDefenseLines 1.0.0 - the Defense pane's pivot turns + refill lines on the price chart
REM (2026-10-22) lsDefenseLines 1.10.0 trails (MoveLogic 1.2.0) + the trail hover: HoverBoard.h hashed
call :one DefenseLines lsDefenseLines DefenseTurn.h DefenseLinesSettings.h SettingsSpec.h SettingsHost.h HostSlot.h SettingsGuard.h MoveLogic.h BoxDock.h BannerBoard.h KeepOutDraw.h HoverBoard.h
REM (2026-10-22 Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build 15:20 CT) lsReversalStage 1.0.0 - "Which stage is the pivot in?" staircase pane, reads the Strips\ letter records of lsBidAskStats 1.28.0 / lsBookFlow 1.23.0
call :one ReversalStage lsReversalStage ReversalStageLogic.h LetterStrips.h HostSlot.h SettingsGuard.h ReversalStageSettings.h SettingsSpec.h SettingsHost.h
REM (2026-10-20 22:41 CT Rassul "a seperate levels indicator without the open .. for my hourly charts") lsKeyLevels 1.0.0 - key levels across the chart, badges right-justified
call :one KeyLevels lsKeyLevels KeyLevelsHLogic.h KeyLevelsHSettings.h DealerProfileAuditLogic.h DealerLogic.h SettingsSpec.h SettingsHost.h HostSlot.h SettingsGuard.h BoxDock.h BannerBoard.h KeepOutDraw.h
REM (2026-10-13) lsTapeReader - the Tape Reader (bias, voters, turn odds from LRA-Turns / LRA-TapeReader); sources deployed by the 4:05 PM task
REM (2026-10-14 claim gates) DealerRead 4.6.0 / TapeReader 1.1.4 also hash ClaimGates.h (DealerRead DealerReadGates.h)
call :one TapeReader lsTapeReader TapeReaderLogic.h TapeReaderText.h DealerLogic.h VwapTrendLogic.h SessionVWAPLogic.h OnTouchLogic.h OnTouchParams.h HostSlot.h OptionsFlow.h HoverBoard.h BoxPlace.h BannerBoard.h ClaimGates.h MarketState.h ZoneStory.h OddsTable.h SignalBoard.h TapeReaderSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h
REM (2026-10-14) lsCobImpulse 0.1.0 - CoB impulse tag Cob+R? / Cob+V? / Cob+RV? + 1 px box on the impulse bar (Rassul 12:07 CT approved; CR5500); 0.3.0 gamma part Cob+R+CR? (reads ClaimGates.h)
call :one CobImpulse lsCobImpulse CobImpulseLogic.h HostSlot.h HoverBoard.h ClaimGates.h CobOdds.h OddsTable.h MarketState.h BannerBoard.h CobImpulseSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h BoxDock.h KeepOutDraw.h
REM (2026-10-14 night) lsBigBidAsk 1.0.0 - biggest bid (red, left) / biggest ask (green, right) lots on every candle of the price chart, bold >= 2.0x, thin circle = absorption (Rassul approved the mockup 20:41 CT)
REM (2026-10-18) lsBigBidAsk 1.16.0 compact Big Lot box docked to the lsHodLod 2.4.1 table (BoxDock.h), category order 1 > 4 > 2 / 3, settings window fixes
REM (2026-10-22) lsBigBidAsk 1.23.0 BO / BD: green / red only + the same-wave confirmation (bba::boWaves in BigBidAskLogic.h, already hashed); failed wick breaks logged only
REM (2026-10-22) lsBigBidAsk 1.24.0 Rly (rally) on 3-min charts: RallyLogic.h 1.0.0 added to its hash list (Rassul approved the Rly mockup v0.2 15:10-15:15 CT)
REM (2026-10-22) lsBigBidAsk 1.25.0 OB / BO boxes of strong waves: OrderBlockLogic.h 1.0.0 added to its hash list (agreed with Rassul 16:00-16:01 CT)
call :one BigBidAsk lsBigBidAsk AbsorbLogic.h RallyLogic.h OrderBlockLogic.h BigBidAskLogic.h ColumnBoard.h HoverBoard.h BoxPlace.h BannerBoard.h HostSlot.h SignalBoard.h ZoneNear.h VwapTrendLogic.h KeyLevelsLogic.h SessionVWAPLogic.h OnTouchLogic.h BoxDock.h BigBidAskSettings.h SettingsSpec.h SettingsHost.h SettingsGuard.h OnTouchParams.h SharedAbsorb.h
REM (2026-10-22) lsStopRun 1.0.0 - St / Sta stop-run mark on the price chart (Rassul 15:33-15:37 CT "show yes", "Sta can indicate the stop run absorbed"; MBO Roadmap T-20261022-15)
call :one StopRunMark lsStopRun StopRunLogic.h StopRunMarkSettings.h NearLevels.h HostSlot.h SettingsSpec.h SettingsHost.h SettingsGuard.h
REM (2026-10-22) lsTurnOrder 1.0.0 - the TURN ORDER mark: the resting limit order that made each pivot (Rassul 15:45 CT "I'll go with your recommendation"; MBO Roadmap T-20261022-25)
call :one TurnOrder lsTurnOrder TurnOrderLogic.h TurnOrderSettings.h OrderFlowFeed.h LmtZonesLogic.h HostSlot.h SettingsSpec.h SettingsHost.h SettingsGuard.h HoverBoard.h BoxPlace.h LargeLotPool.h MoveLogic.h NearLevels.h OptionsFlow.h ZoneRead.h ZoneRule.h
REM (2026-10-22) lsDealerHedge 0.1.0 - "oH" dealer-hedging zones on the price pane (Rassul agreed 15:23-15:24 CT, build 15:31 CT; MBO Roadmap T-20261022-27): gamma level +/- 0.5 ATR, lights / pulses on programmatic MBO flow, settles faint, never removed
call :one DealerHedge lsDealerHedge DealerHedgeLogic.h DealerHedgeSettings.h HostSlot.h HoverBoard.h KeepOutDraw.h BoxDock.h BannerBoard.h SettingsGuard.h SettingsHost.h SettingsSpec.h
REM (2026-10-22) lsSweepRefill 1.0.0 - "Sr" SWEEP REFILLED on the price chart (Rassul approved 16:16 CT, 16:18 no adjacent candles, 16:28 bearish drawn + the v3 taken-out rule; MBO Roadmap "Did the swept prices refill fast after a sweep?")
call :one SweepRefill lsSweepRefill SweepRefillLogic.h SweepRefillSettings.h NearLevels.h HostSlot.h SettingsSpec.h SettingsHost.h SettingsGuard.h
REM (2026-10-18 HOT-RELOAD) hot-reload test indicator lsHotReloadDemo: its implementation publishes WHILE IRT IS OPEN (refresh the chart),
REM   the shim installs only when IRT is closed - tools\gex-hot-build.bat, docs\build\HOT-RELOAD.md
if not defined HOLD if exist "%REPO%\tools\gex-hot-build.bat" call "%REPO%\tools\gex-hot-build.bat" HotReloadDemo
del "%LOCK%" >nul 2>&1
exit /b 0

REM ---- :one <Name> <lsName> <header...> ---------------------------------------
:one
set NAME=%~1
set DLL=%~2.dll
set SRCHASH=
call :hashof "%PLUG%\%NAME%.cpp"
set SRCHASH=!H!
for %%X in (%*) do (
  if exist "%PLUG%\%%X" ( call :hashof "%PLUG%\%%X" & set SRCHASH=!SRCHASH!-!H! )
)
set BUILT=
if exist "%OUT%\%NAME%.built" set /p BUILT=<"%OUT%\%NAME%.built"
if not "!BUILT!"=="!SRCHASH!" (
  call :vc
  if not defined VCOK ( call :log %NAME%: VISUAL STUDIO C++ TOOLS NOT FOUND - cannot build & goto :eof )
  cd /d "%PLUG%"
  cl /nologo /LD /EHsc /O2 /MD %NAME%.cpp /I "%SDK%\include" /Fo"%OUT%\\" /Fe:"%OUT%\%DLL%" /link "%LIB143%" > "%OUT%\%NAME%.compile.txt" 2>&1
  if errorlevel 1 (
    call :log %NAME%: BUILD FAILED - see plugin\out\%NAME%.compile.txt
    call :status
    goto :eof
  )
  > "%OUT%\%NAME%.built" echo !SRCHASH!
  set BUILT=!SRCHASH!
  call :ver
  call :log %NAME%: BUILT %DLL% !VER!
)
REM ---- install when the built DLL is not the installed one
set INST=
if exist "%OUT%\%NAME%.installed" set /p INST=<"%OUT%\%NAME%.installed"
if "!INST!"=="!BUILT!" goto :eof
if not exist "%OUT%\%DLL%" goto :eof
if not exist "%DLLDIR%" ( call :log %NAME%: extension folder not found: %DLLDIR% & goto :eof )
if defined HOLD (
  REM (P3) trading hours: never copy now; say so once per build
  if not exist "%OUT%\%NAME%.held" ( call :log %NAME%: HELD - built in trading hours !HOLDWHY!, installs after 15:15 CT when Investor/RT is closed - do not restart IRT for it & > "%OUT%\%NAME%.held" echo !BUILT! )
  call :status
  goto :eof
)
copy /Y "%OUT%\%DLL%" "%DLLDIR%\%DLL%" >nul 2>&1
if errorlevel 1 (
  REM the DLL is loaded (Investor/RT open) - retried next tick; say so once per build
  if not exist "%OUT%\%NAME%.pending" ( call :log %NAME%: PENDING - installs by itself within 2 min of closing Investor/RT after 15:15 CT & > "%OUT%\%NAME%.pending" echo !BUILT! )
  call :status
  goto :eof
)
> "%OUT%\%NAME%.installed" echo !BUILT!
del "%OUT%\%NAME%.pending" >nul 2>&1
del "%OUT%\%NAME%.held" >nul 2>&1
call :log %NAME%: INSTALLED %DLLDIR%\%DLL% - reopen Investor/RT to load it
call :status
goto :eof

REM ---- :ver  -> VER  the setVersion("x.y") of the plugin (outside any block: the delims hold parentheses) --
:ver
set VER=
for /f "tokens=2 delims=()" %%v in ('findstr /c:"setVersion(" "%PLUG%\%NAME%.cpp"') do set VER=%%v
REM (GP 0.71) a plugin that passes a CONSTANT (setVersion(GP_VERSION)) carries the literal on its definition line
if /i "!VER!"=="GP_VERSION" for /f "tokens=2 delims==;" %%v in ('findstr /c:"GP_VERSION = " "%PLUG%\%NAME%.cpp"') do set VER=%%v
goto :eof

REM ---- :hashof <file>  -> H  (certutil, no PowerShell) -------------------------
:hashof
set H=
for /f "usebackq delims=" %%l in (`certutil -hashfile "%~1" SHA1 ^| findstr /v /i "hash CertUtil"`) do set H=%%l
set H=!H: =!
REM (2026-10-18 settings audit) only the first 12 hex digits: "set /p" reads at most 1023 characters back from the .built file, and
REM   HodLod's list (26 files x 41 characters) passed that at 16:26 CT - it rebuilt on every tick. 12 digits keep every list far below.
set H=!H:~0,12!
goto :eof

REM ---- :vc  load the x64 compiler once -----------------------------------------
:vc
if defined VCOK goto :eof
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :eof
set "VSINST="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSINST=%%i"
if not defined VSINST goto :eof
call "%VSINST%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 goto :eof
if not exist "%SDK%\include\irtsdk.h" goto :eof
set VCOK=1
goto :eof

REM ---- :log <text> ---------------------------------------------------------------
:log
echo %DATE% %TIME% %* >> "%LOGF%"
goto :eof

REM ---- :settings  the settings contract (run when its inputs change; a failure blocks every build until fixed) ---------------
:settings
set SETFAIL=
set SH=
for %%X in (SettingsSpec.h SettingsHost.h %SETSRC%) do (
  if exist "%PLUG%\%%X" ( call :hashof "%PLUG%\%%X" & set SH=!SH!-!H! )
)
for %%F in ("%PLUG%\*Settings.h" "%PLUG%\tests_settings\settings_contract.cpp") do ( call :hashof "%%~F" & set SH=!SH!-!H! )
set SOK=
set SBAD=
if exist "%OUT%\settings-contract.ok" set /p SOK=<"%OUT%\settings-contract.ok"
if exist "%OUT%\settings-contract.fail" set /p SBAD=<"%OUT%\settings-contract.fail"
if "!SOK!"=="!SH!" goto :eof
if "!SBAD!"=="!SH!" ( set SETFAIL=1& goto :eof )
call :vc
if not defined VCOK goto :eof
if not exist "%PLUG%\tests_settings\settings_contract.cpp" goto :eof
cd /d "%PLUG%\tests_settings"
cl /nologo /EHsc /O1 settings_contract.cpp /Fo"%OUT%\\" /Fe:"%OUT%\settings_contract.exe" > "%OUT%\SETTINGS-CONTRACT.txt" 2>&1
if errorlevel 1 (
  set SETFAIL=1
  > "%OUT%\settings-contract.fail" echo !SH!
  call :log SETTINGS CONTRACT DID NOT COMPILE - nothing built, see plugin\out\SETTINGS-CONTRACT.txt
  goto :eof
)
"%OUT%\settings_contract.exe" "%PLUG%" >> "%OUT%\SETTINGS-CONTRACT.txt" 2>&1
if errorlevel 1 (
  set SETFAIL=1
  > "%OUT%\settings-contract.fail" echo !SH!
  call :log SETTINGS CONTRACT FAILED - nothing built, see plugin\out\SETTINGS-CONTRACT.txt
  goto :eof
)
> "%OUT%\settings-contract.ok" echo !SH!
del "%OUT%\settings-contract.fail" >nul 2>&1
call :log SETTINGS CONTRACT PASSED
goto :eof

REM ---- :window  (P3, 2026-10-22) HOLD=1 on Monday-Friday 07:00-15:15 (this PC's clock = Central) unless plugin\out\INSTALL-NOW.flag
REM   exists. %TIME% is " 7:05:12.34" or "07:05:12.34"; %DATE% starts with the day name ("Thu 10/22/2026"). The "1HH*60 + 1MM - 6100"
REM   form avoids cmd reading 08 / 09 as octal. An unreadable time or day name counts as a weekday in the window (holds: the safe side).
:window
set HOLD=
set HOLDWHY=
set "HT=%TIME: =0%"
set HMIN=
set /a HMIN=1!HT:~0,2!*60 + 1!HT:~3,2! - 6100 >nul 2>&1
if not defined HMIN set HMIN=-1
if !HMIN! GTR 1439 set HMIN=-1
set "HDAY=%DATE:~0,3%"
set WEEKEND=
if /i "!HDAY!"=="Sat" set WEEKEND=1
if /i "!HDAY!"=="Sun" set WEEKEND=1
if defined WEEKEND ( set "HOLDWHY=(!HDAY! - weekend)" & goto :windowflag )
if !HMIN! LSS 0 ( set HOLD=1& set "HOLDWHY=(clock unreadable: %TIME%)" & goto :windowflag )
if !HMIN! GEQ 420 if !HMIN! LSS 915 ( set HOLD=1& set "HOLDWHY=(!HDAY! %TIME:~0,5% CT, 07:00-15:15)" )
if not defined HOLD set "HOLDWHY=(!HDAY! %TIME:~0,5% CT)"
:windowflag
if exist "%OUT%\INSTALL-NOW.flag" (
  if defined HOLD ( set HOLD=& set "HOLDWHY=!HOLDWHY! - INSTALL-NOW.flag: Rassul asked for a midday install" ) else ( del "%OUT%\INSTALL-NOW.flag" >nul 2>&1 )
)
goto :eof

REM ---- :status  one file a human (or Claude, over the bridge) can read at a glance -
:status
> "%STATUS%" echo GEX BUILD STATUS  %DATE% %TIME%
if defined HOLD ( >> "%STATUS%" echo INSTALL WINDOW: HELD !HOLDWHY! - new builds install after 15:15 CT, when Investor/RT is closed ) else ( >> "%STATUS%" echo INSTALL WINDOW: open !HOLDWHY! )
if defined SETFAIL ( >> "%STATUS%" echo SETTINGS CONTRACT: FAILED - nothing is built until it passes, see plugin\out\SETTINGS-CONTRACT.txt ) else if exist "%OUT%\settings-contract.ok" ( >> "%STATUS%" echo SETTINGS CONTRACT: passed )
for %%N in (GammaProfile KingTracker DealerProfile DealerRead DealerSig DealerSummary DeltaProfile DeltaProfileHourly SessionInfo HodLod TradeManager SessionVWAP IRTReader FootprintReaderES FootprintReaderNQ FootprintReaderCL FootprintReaderGC FootprintReaderHG FootprintReaderNG FootprintReaderEU TapeFlowES TapeFlowNQ TapeFlowCL TapeFlowGC TapeFlowHG TapeFlowNG TapeFlowEU TapeFlowMarks ChartView MboCheck LiquidityProfile LiquidityFlow Pulling Exhaustion ExhaustionV2 CVD FirstExtreme BidAskStats BookFlow DefenseLines ReversalStage KeyLevels TapeReader CobImpulse BigBidAsk StopRunMark DealerHedge TurnOrder SweepRefill) do (
  set B=& set I=& set P=
  if exist "%OUT%\%%N.built" set /p B=<"%OUT%\%%N.built"
  if exist "%OUT%\%%N.installed" set /p I=<"%OUT%\%%N.installed"
  if exist "%OUT%\%%N.pending" set "P=- PENDING, installs when Investor/RT is closed after 15:15 CT"
  if exist "%OUT%\%%N.held" if defined HOLD set "P=- HELD until 15:15 CT (trading hours), do not restart Investor/RT for it"
  if "!B!"=="" ( >> "%STATUS%" echo %%N: not built yet ) else if "!B!"=="!I!" ( >> "%STATUS%" echo %%N: installed ) else ( >> "%STATUS%" echo %%N: built, not installed !P! )
)
goto :eof
