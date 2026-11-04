# lsLiquidityProfile 0.73.0 (LiqTouch 1.6.0) + lsDealerProfile 2.52.2 + lsDeltaProfile 2.32.1 - 2026-11-03 night

Request: Rassul "fix all" after the Liquidity Profile audit of his live ES files at 7:49 PM CT (LiquidityProfile.status-ES-180.txt,
LRA-LiqZones-ES.csv, LRA-LiqNorms-ES.csv, bandnorms-ES.txt, DealerProfile.status-ES-180.txt, his 7:48 PM ES screenshot).
Mockup (now vs proposed, real data, zoom viewer): scratchpad lpfix/lp_mock.png + lp_mock.html.

## Finding first: the "3.9x / 3x / 2.6x" labels on his chart are NOT Liquidity Profile zones
They are lsDeltaProfile (DLT 2.32.0) ratio labels on the 3 biggest delta bars of its profile column (his 2.18.0 rule). The Liquidity
Profile has drawn nothing on the price chart since 0.60.0 (LQ_LEFT_COL = false) and the Dealer Profile 2.40+ shows no zone ratios.
The LP zone ratio still mattered: it feeds LRA-LiqZones (Tape Reader), LRA-LiqLevels (Levels column), the LQ level confirmation and the
Market State LIQ facts - so items 1 and 2 were fixed in that data, and item 7a (labels too close) in the Delta Profile.

## 1) Zone ratio measured the EMPTY book behind a zone (lsLiquidityProfile 0.73.0)
- prominence() = zone lots / median of the 8 bands behind it (1-lot floor) - 7:49 PM: a 380-lot zone = 54x a 7-lot book behind it, but
  only 1.4x a normal band (266 lots).
- Now a WALL (drawn state -> LRA-LiqZones Z rows, LRA-LiqLevels lots / x, LQ confirmation, Market State) needs BOTH bx >= 2.0x the book
  behind AND lots >= 1.5x the slot's normal band (the normal the plugin already uses for ZONE_HW: norms file slot / 5-session pool slot /
  session median). No normal yet = size gate off (0.72 rule, said in the status DRAW line). Removal still only at a bar close.
- The LABEL of a zone = lots x the normal band ("1.4x" = 1.4 times a normal band of its width, "10x+" from 10x): LRA-LiqZones field 7,
  the (unused) left-column node label. bx itself is unchanged (the LQ / hold studies were measured on it).
- 7:49 PM ES: the 0.72.1 walls (7571.5 7.5x, 7564.75 4.6x, 7560.75 2.5x, 7557.25, 7503.25 3.0x, 7500 54x) are all 0.8-1.4x a normal band:
  none is a wall in 0.73.0.

## 2) Zones near price never shown (0.73.0)
- The NEAREST zone each side inside the near band (1 x the 3-min range, its hysteresis), durable 30 s, not a wall, and bigger than the
  book behind it (bx > 1.0x, his 0.14.0 "never under 1.0x" floor) is published as an `N,<S|R>,lo,hi,bx,lots,xNormal` row in
  LRA-LiqZones (its own row kind: readers of Z rows never take it for a wall). 7:49 PM: R 7543.75-7546.75 (1.1x normal), S 7522-7525 (1.3x).
- NOT drawn: no chart surface draws LP zones today. Drawing them dim needs the Liquidity Profile column back (he removed it 10-27) or a
  Dealer Profile row - his decision.

## 3) Warming after an IRT restart (LiqTouch 1.6.0 + LP 0.73.0 + DP 2.52.2)
- LiqTouch warming(): the first 10 min (WARM_MS = the 10-min rest rule of the whole-book nodes / king) after the order book's first event.
- DEPTH SEED (feasible, done): every second the LP hands the chart's depth book (MBP) to the engine; at each price the displayed lots the
  order book cannot explain (= orders placed before the restart) count as real resting lots once displayed continuously 10 min (a price
  that leaves the depth window / drops to no remainder restarts its clock; a depth book > 5 s old is ignored). They join the whole-book
  nodes (B code 4) and the king. Coverage (known / displayed lots) in the status.
- LP: status line WARMING (warming | ready, tracked s, depth explained %, seeded lots / prices, kings posted); SignalBoard B code 9
  (val = tracked s, state 1 warming / 0 ready, proven = coverage %); no king posted while warming.
- DP: B code 9 state 1 -> title "DP v2.52.2  LIQUIDITY" + "LIQUIDITY \xB7 warming" on the row under it (the one-line title does not fit 170 px);
  no rank / K text in the rank boxes, no rank chips on the level badges, no edge-marker ranks, liqlevels.csv rank 0. Status LIQ_WARM.

## 7a) "3.9x" and "3x" 11 px apart (lsDeltaProfile 2.32.1)
- ratioLabelClash(): two profile ratio labels clash when their rows are closer than one text row + 2 px, whatever their x (2.32.0 also
  required them to overlap side by side). The weaker steps up / down a row with its leader line (existing placement), else is not drawn.

## 7b) King hand-off (LP 0.73.0 -> DP 2.52.2)
- Root cause: lsLiquidityProfile 0.70.0 retired SignalBoard B codes 2 / 3 (its status still computed KING_NODES offer 50 @ 7587, 16.7x),
  and lsDealerProfile 2.40+ took rank 1 of its own whole-book board as K - 7625 (50 lots too, outside today's high - ADR .. low + ADR
  range). The key / chart / spb were fine (code 4 nodes arrived: LIQ_BOOK nodes 40).
- Fix: LP posts its king again as B code 3 (one per side, val = lots, proven = x median x 100, bar = since; only when ready). DP 2.52.2:
  K = the bigger LP king (then the nearer); its node (same side, within 1 tick) becomes rank 1 and every node ahead of it moves down one; a
  king not on the board yet is added as rank 1. LP 0.73 with no king = no K (rank 1 shows "1"). An older LP (no code 9) = unchanged.
  Status KING_NODES "K = the LP king <px> <lots> (its node | added)" / "none (no LP king)" / "none (warming)".
- Note: rows keep "% of the king"; a node bigger than the LP king shows over 100% (e.g. "900  180%"), its bar capped at full length.

## Tests (Linux, ASan / UBSan)
- tests_liqtouch/test_liqtouch_warm.cpp (LiqTouch 1.6.0, -Wall -Wextra -Werror): 19/0 - restart replay: warming 10 min, coverage, the old
  300 bid seeded only after 10 min of display, rank 1 + king, no double count once known, stale depth / vanished remainder / garbage rows.
- tests_lqp044/test_lp073.cpp (-Werror): 20/0 - 54x-behind / 0.94x-normal zone never a wall / LQ, label 0.9x; 2.5x normal = wall; shrink
  under 1.5x normal removed at the close only; 10x+; no normal = gate off; nearest per side, wall never also nearest, flat 1.0x book none.
- tests_liqtouch/test_lp073_host.cpp (real LiquidityProfile.cpp, mock host): 16/0 - B code 9 warming 5 min / ready 10 min, no king while
  warming, king code 3 = old bid 7830 seeded from depth, the bigger 400 node outside the range not king, status WARMING / DRAW / KING_NODES,
  LiqZones 7 fields, IRT restart -> warming again. LP073_WRITE writes tests_liqtouch/lp073_bfacts.txt (the end-to-end fixture).
- test_dealerprofile_2522.cpp (real DealerProfile.cpp): 30/0 - old LP unchanged; LP king on its node; king added; no king; warming
  (title, no ranks / chips / markers, Q facts still posted); END-TO-END with the facts the real LP posted: K = 7830, the 400 node = 2.
- tests_dp217/test_dp2321.cpp: 16/0 - his screenshot geometry (3x 11 px over 3.9x = clash, one row up = none), renders, no circles.
- Existing suites before / after: see the summary at the end (pre-existing failures listed separately).
- MinGW -fsyntax-only vs the real irtsdk.h: LiquidityProfile, LiquidityFlow, DealerProfile, DeltaProfile 0 errors; the same warnings as
  before (pre-existing).

## Versions
LQ_VERSION = setVersion = LiquidityProfileSettings PLUGIN_VERSION = lqp::VERSION = 0.73.0; ltc::VERSION 1.6.0;
DP_VERSION = setVersion = DealerProfileSettings PLUGIN_VERSION = 2.52.2; DLT_VERSION = setVersion = DeltaProfileSettings PLUGIN_VERSION
= 2.32.1. No parameter changes (no settings window change).

## Not verified - needs his IRT
- Installs at his next File -> Exit (gex-build). Then: LiquidityProfile.status-ES-180.txt VERSION 0.73.0 + WARMING line (warming for 10
  min, then ready + "seeded from depth N lots"), DealerProfile status VERSION 2.52.2 + KING_NODES "K = the LP king ...", DeltaProfile 2.32.1.
- Real MBP vs MBO: implied quantities in the depth book could be seeded as "real" if they sit 10 min at one price (watch "depth explained %").
