# Fix build 2026-10-22 evening (live records 19:09-19:48 CT, ES + NQ)

Built while IRT was open: gex-build compiles now, the DLLs install at Rassul's next IRT restart.

| # | Defect (live record) | Fix | Versions |
|---|---|---|---|
| 1 | Squashed pair bars: tallest bar ~4 px on ~100 px panes, axis 20,000 vs ~800-lot bars | pairPx floor = 50% of the pane (pairFloorPx / pairTopRow), compact letter strips (ONE 11 px row per strip, 8-pt letters; the spike labels share the strip band) | OrderFlowFeed.h 1.15.0, LetterStrips.h 1.1.0, lsBidAskStats 1.30.0, lsBookFlow 1.25.0 |
| 2 | Implied row spread over Buyers / Sellers ("volume" / "0% of") | InfoRow.wide: one row "Implied: 0% of volume"; never widens the columns | OrderFlowFeed.h 1.15.0 |
| 3 | Queue % nonsense ("held 1114% q0 7", live 999%); Q flood (ES 29 / NQ 30 rows in 45 min) | base = max(q0, normal queue) (session median q0 at touched levels, else median resting lots per tick within 1 ATR x zone ticks); no percent / no call while building; "200%+" cap; badge "G1  Q 143 (2.4x)"; Q rows only at key / MenthorQ levels and confirmed ZigZag pivots (SwingRef.h last-3 intact), one per level per touch, debounced 1 candle, none for defended prices or tests that never reached the normal | QueueMeter.h 1.3.0, lsLiquidityProfile / lsLiquidityFlow 0.51.0, lsDealerProfile 2.23.0 |
| 4 | LMT zones 0 all evening (S20 194 = the RTH tail at night) | LMT = >= MULT (3.0x) the live median resting order within +/- 2 ATR, per-market floor (ES 10, NQ 5, CL 5, GC 3, HG 3, NG 3, 6E 5), rested 30 s, hysteresis KEEP_MULT 2.5x (end SHRANK); S20 context only; params MULT / KEEP_MULT / FLOOR / MED_ATR in LRA-LmtZones-<MKT>.csv | LmtZonesLogic.h 1.1.0 |
| 5 | BL5 / BL10 / BL20 box docked to the HOD/LOD box | not drawn (BBA_BL_BOX false); averages kept in the BL_BOX status line | lsBigBidAsk 1.27.0 |
| 6 | lsHodLod DAYTYPE "log fails 17" | lsFlexLevels\HodLod did not exist: hlAppend creates the folder first (CreateDirectoryA) | lsHodLod 2.7.1 |
| 7 | "NOW HIGH stage 2 failed chain Q" | a queue Q steps only when its anchor (tk<N> in the strip line) is at the attempt's own extreme (max(4 ticks, 0.3 ATR)); else hover only | ReversalStageLogic.h 1.2.0, lsReversalStage 1.2.0 |
| 8 | NQ has no Strips\ATK / DEF and no ReversalStage file | NOT a bug: lsBidAskStats / lsBookFlow / lsReversalStage have never written any NQ status or record (no BidAskStats.status-NQ*, nothing NQ in BidAskStats\ or BookFlow\) = the panes are not on his NQ chart. Strips\QUEUE-NQ exists because lsLiquidityProfile is on NQ. | - |

Tests (Linux, ASan/UBSan, -Werror): tests_strips 1526 + host 30 + ES replay (no repaint: 0 prefix diffs); tests_qm 104 + LP host 17; tests_lmz 51 + host 30; tests_aggbook full (all targets, incl. the new fix-build checks: pane 80 / 100 / 150 px bar 50-90%, axis top ~ tallest / 0.5-0.9; Implied one row); tests_hodlod t_hl270 317 + t_hl240 45 + folder-create append test; test_dealerprofile_2200 28; settings contract 40,909 checks 0 failures; MinGW real irtsdk.h -Dsmall=char: 0 errors, no new warnings (11 plugins). Not run: replay_qm / replay_lmz on recorded MBO (data not staged) - logic tests use synthetic books with a realistic small-order background.
Not verified - needs his IRT: the panes at his real heights, the strip letters at 11 px, the LMT zone count overnight with the live median.
