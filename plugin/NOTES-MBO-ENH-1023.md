# MBO enhancements merge - 2026-10-23 (Rassul approved option A, 04:56 CT)

The outside review's two patches were merged onto the 2026-10-22 evening fix build. His files were not overwritten. Each file was 3-way merged (`git merge-file`, his current file / their base / their final file). Their base was built by reverse-applying their patches. Order: primary patch (reversal pipeline), then final patch (11 indicators).

## What was merged

| Area | What changed | Versions (his before -> now) |
|---|---|---|
| Reversal pipeline (primary) | Typed evidence files EV-ATK / EV-DEF / EV-QUEUE in lsFlexLevels\Strips (ReversalEvidence.h, new). Arrival journal + frozen post-decision checkpoints (ReversalStageStream.h, new). New attacker letter **E**: an S / K attack's execution pace faded to <= 50%. Running-out priority R > Tr > E > G. **Q!** = a queue that broke or drained, shown as a warning and never hidden behind A / Q. Stages step only on COMPLETE typed MBO evidence; untyped letters are review only (*). The Now box shows evidence readiness. | lsReversalStage 1.2.0 -> **1.3.0** (ReversalStageLogic 1.3.0), lsBidAskStats 1.30.0 -> **1.31.0**, lsBookFlow 1.25.0 -> **1.26.0**, LetterStrips.h 1.1.0 -> **1.2.0**, lsLiquidityProfile / lsLiquidityFlow 0.51.0 -> **0.52.0** |
| QueueMeter | A same-id add that was later withdrawn is no longer counted as a retained refill (`Flow.before`). | QueueMeter.h 1.3.0 -> **1.4.0**, lsDealerProfile 2.23.0 -> **2.24.0** |
| Dealer Hedge | Zero-size fills are not hits. Snapshot / backlog fills or adds outside the flow window are not refills. | lsDealerHedge 0.2.0 -> **0.2.1** |
| Defense Lines | A trail end binds to its recorded chart key and is never shifted to a later bar. | lsDefenseLines 1.11.0 -> **1.11.1** |
| IRT Reader | A timeframe-only rebind keeps the DBO arrival ordering (same market + contract). | IRTReader 0.8.2 -> **0.8.3** (7 FootprintReaders rebuild) |
| MBO Check | Recorder lost / unassigned line counts. Trading-day (17:00 CT) session tag. | lsMboCheck 1.1.1 -> **1.1.2** |
| Pre-open (HOD/LOD) | PreOpenLogic fixes from the review. | PreOpenLogic 1.0.0 -> **1.1.0**, lsHodLod 2.7.1 -> **2.7.2** |
| Stop Run | Resting fills are keyed by (order id, price), so pre-level partial fills cannot satisfy the Sta big-order rule. | lsStopRun 1.2.0 -> **1.2.1** |
| Sweep Refill | Only a positive fill of a mapped resting ID seeds a sweep. A zero-lot fill is ignored. A book reset aborts pending observations instead of deciding a false non-refill. | lsSweepRefill 1.1.1 -> **1.1.2** |
| Tape Reader | Session-source fix. | lsTapeReader 1.3.3 -> **1.3.4** |
| Turn Order | Review fixes. Record format M\|1.0.0 unchanged. | lsTurnOrder 1.1.0 -> **1.1.1** |
| gex-build.bat | Their new hash entries (LetterStrips.h, ReversalEvidence.h, ReversalStageStream.h) plus his SwingRef.h entries. Kept CRLF. | - |

## Conflicts and how they were resolved

1. **ReversalStageLogic.h (4) / ReversalStage.cpp (3)**: took their typed engine and **re-applied last night's fix #7 inside it**. A queue Q steps to stage 2 only when its line is within max(4 ticks, 0.3 ATR) of the attempt's own extreme (`qAtExtreme`). Otherwise the hover says "queue not at the attempt's own extreme". The tk anchor stays hidden in the hover (`numsShown`). Their safe price-to-tick conversion was kept.
2. **test_strips.cpp**: the old untyped state-machine tests moved to their test_stage_core.cpp. A new block there tests the Q tolerance, the anchor parse and the hidden tk.
3. **StopRunLogic.h**: their (id, price) keying, with his `endSec = startSec` (his struct has no lastSec).
4. **SweepRefillLogic.h**: their mapped-only + zero-lot + reset changes. His no-erase-on-fill behaviour was kept.
5. **gex-build.bat**: both lists were merged (their headers + his SwingRef.h).
6. **Tests (trails / oh / sr)**: their new tests were kept. Blocks that test the 10-22 partial-audit fixes he never installed were dropped: second-chart trail record, 17:00 rollover reset, erase-on-full-fill.
7. **LiquidityProfile.cpp (clean merge, edited)**: the typed EV-QUEUE detail now uses his "200%+" cap. It is emitted in the same loop as his gated Q rows: levels / pivots, debounced.
8. **LetterStrips.h hovers**: example wording restored for E / Q! / H / L (his hover rule).

Not shipped: test_mbocheck_logic.cpp. It tests a partial-audit helper (noteStatusAttempt) that his MboCheck does not have. tests_strips/test_stage_host.cpp (1.1.0, untyped letters) is superseded by test_stage_pipeline.cpp and is not run.

## Tests (Linux, ASan/UBSan, -Werror; MinGW real irtsdk.h -Dsmall=char)
- tests_strips: 1301 + stage core 59 + evidence transport 45 + stage pipeline 18, 0 failed. No-repaint replay on ES 10-22: 0 prefix diffs.
- tests_aggbook --full: all targets 0 failed, including BidAskStats host 71, BookFlow host 139, DefenseLines host 49, trails host 32, attack host 48, dependency checks 344. Real-SDK syntax: BidAskStats / BookFlow / DefenseLines.
- tests_qm 107 + LP host 17. tests_lmz 51 + host 30. tests_sr 66 + 32. tests_st 22 + 24. tests_tor 40 + 25. tests_oh 57 + host 25. dealerprofile_2200 31. IRT rebind 10 + contracts. ArrivalStamp 17. MboCheck recording 0 failed. TapeReader session 7. t_hl270 317, t_hl240 45.
- Settings contract: 40,909 checks, 0 failures.
- MinGW syntax: all 15 changed plugins, 0 errors. No new warnings (LiquidityProfile has the same 5 old warnings).
- These failures already happen on his current files: t_po260 2 and t_hl244 2 (old 2.6.0 version asserts), settings host 1 ("HodLod parameter version 7").

## What to watch live (after the IRT restart)
- **Reversal Stage starts empty**: old letter records are now review only (*). Steps appear only from new typed EV-ATK / EV-DEF / EV-QUEUE rows, which lsBidAskStats 1.31.0 / lsBookFlow 1.26.0 / lsLiquidityProfile 0.52.0 start writing after the restart. Each pane must be on the chart for its EV file. Old RS-*.csv records are not reused (new RS2 file).
- **Now box**: the new evidence readiness line (confirmed / review / pending / partial / unavailable). "pending" right after a candle closes is normal (20 s settle + source acknowledgement).
- **E letter** (attacker pane, Running-out strip): attack pace halved. Not a proven edge yet; it is logged for the nightly.
- **Q!** (Holding strip): the defended queue broke or drained. It should fail a stage-2+ attempt only when it is at the same defended cause.
- A Q far from the attempt's extreme must stay hover only (fix #7 kept).
