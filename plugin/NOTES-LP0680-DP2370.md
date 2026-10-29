# lsLiquidityProfile 0.68.0 (OrderLinesLogic 1.1.0) + lsDealerProfile 2.37.0 (LevelsColumn lrk 1.0.0) - 2026-10-29

Approved by Rassul 09:33 CT (mockup orderlines-levels-mockup.html), add-on 09:40 CT (column tags), 10:23 CT (HELD rules, mockup artifact AEYuhQfdcywsR8VxmhpUVq). Installs at his close (15:15 CT) via gex-build; nothing for him to do but exit / reopen IRT.

## A) lsLiquidityProfile 0.68.0 - the tested order lines
- Untouched big order: nothing on the candles (only the Liquidity column).
- TOUCH = the first trade at / through the order's price (bid: a fill at <= it, offer: >= it, any order's FILL from the order-by-order feed; a closed bar's range as fallback, stamped at the bar END). Record `T|id|touch_ms|rank`.
- Rank = the order's rank in lsDealerProfile's Liquidity column at the touch (SignalBoard Q, read <= 1/s, nearest same-side zone within max(1 tick, 0.25 ATR)), FROZEN; outside the top 6 = 0 = no rank box.
- Line from the PLACEMENT time; placed before the visible chart = from the left edge of the price area (cb::leftKeep = right of the Key Levels column + 3 px); right side clipped at the right-side columns / last bar.
- TESTING = thin dashed (1 px, P_DASH). DECISION unchanged (fill >= half, first favourable close within 4 bars = HELD; a bar wholly beyond / no favourable close in 4 bars = BREAK). THRU renamed BREAK everywhere drawn; the record keeps THRU and adds the word: `W|id|THRU|ms|shown|BREAK`.
- Decided = solid 2 px; label at the decision bar (bar END stamps: the decision bar is barOf(wordMs - 1)): [rank box] + HELD / BREAK, above the line, kept inside the price area.
- BREAK: shown 5 bars after its decision, then removed. HELD: solid, never dimmed (10:23 CT), to the right edge until a bar CLOSES through it (`X|id|ms`) or the session ends - always at least 2 bars after its decision. (A HELD line's order is already filled, so "pulled" cannot happen after a decision.) Pulled before a decision = gone. At most 3 tested lines (the newest by touch / decision; older ones are not drawn).
- No fill dots / start labels / pulled lines any more; LRA-OrderFills-<MKT>-<spb>.txt is published EMPTY (lsDefenseLines draws its own dot again). LRA-LmtLevels file unchanged.
- Lines restored from a pre-0.68 record (no T line) are not drawn.
- Status ORDER_LINES: "tested lines (OrderLines 1.1.0) touched N closed-through N shown now N Q reads N", "held N break N".

## B) lsDealerProfile 2.37.0
- Key Levels badge order: odds (left-justified) ... VALUE, NAME chip(s), LIQUIDITY RANK box right-most; no rank = no box; same 230 px; odds drop first (unchanged width rule).
- Rank rule (option c): a zone counts only from the level out to 0.25 ATR (3-min ATR) BEYOND it, one-sided away from price; each zone goes to ONE level only (the nearest qualifying level; tie = the level nearer price) - no rank can show twice. liqlevels.csv rows follow the same match.
- History: every change of a level's rank appended to lsFlexLevels\DealerProfile\<MKT>-liqlevels-history-<session>.csv (`time_ct_ms,level,price,rank,lots`; session = CT date of time + 7 h). Only on a change; a level scrolled off writes nothing.
- Liquidity column (09:40 add-on): row = rank | bar | lots | tag ("1 [bar] 92 STAYS"); tags (STAYS+ / STAYS / weak / GONE / GONE+) only on the row of a zone shown NOW - no tag-only rows, a zone that leaves takes its tag; HELD / BREAK never drawn in the column. Records unchanged.
- Status RANK_RULE line.

## Tests (Linux, ASan/UBSan)
- tests_oln/test_oln_110.cpp 20/0 (lifecycle: untouched, touch + rank frozen, rank > 6 = 0, HELD min-2-bar after a close through, HELD stays 8 bars un-dimmed, BREAK 5 bars, THRU+BREAK record, bar-range touch fallback, pulled before decision, max 3, restart = same states).
- tests_oln/test_oln.cpp 28/0 (1.0.0 suite still passes).
- tests_oln/test_oln_host_068.cpp 18/0 through the real LiquidityProfile.cpp (untouched = no line; dashed 1 px from the left edge right of a 230 px Key Levels column (x = 236); HELD solid to the last bar; [2] HELD label at the decision bar; rank box side colour; closed-through after 1 bar still shown, gone after the 2nd; record T / W / X). Mock SDK tests_lqp044/irtsdk.h now records each line's START and pen style.
- test_dealerprofile_2370.cpp 120/0 through the real DealerProfile.cpp (badge order value < name < rank; 0.25 ATR reach; 0.5 ATR = no chip; no duplicate rank; lrk assign / history unit checks; history file header + rows; tags on their zone's row after the amount; no HELD in the column; no floating tag after the zone leaves). Leak checker off (pre-existing harness leaks, same as 2.36.1).
- settings_contract PASS (45949 checks). MinGW -fsyntax-only vs the real irtsdk.h: DealerProfile / LiquidityProfile / LiquidityFlow / BookFlow 0 errors, no new warnings in the changed code.
- The old tests_oln/test_oln_host.cpp (0.55-0.59 drawing: labels, fill dot, dark pulled lines) is superseded by test_oln_host_068.cpp.

## Not verified - needs his IRT
- The Q-board match (rank) on live data; P_DASH rendering on his charts; label placement next to other marks. Check after the restart: BUILD-STATUS "installed", status ORDER_LINES "tested lines" and RANK_RULE lines, ChartView export in 2+ markets.
