# lsLimitProfile 2.67.0 - ONE KING + its L / P marks (2026-11-20)

## Request
- Rassul 05:05 CT 2026-11-20: "no, just 1 king.. we can have the mark as well" (on mockup limit-profile-king-mockup.html: middle + right panels).

## What changed
- LIQUIDITY column: ONE king = the biggest REAL resting level within reach on EITHER side (TopBook filters unchanged: rested 30 s + 2 min on the
  board, not re-priced 2x in 5 min, no just-placed cluster, past the first 2 ticks, within 3 ATR). Row "K 8 1m" (lots, time, no %). Red = offer
  above, green = bid below. The ranked rows 2-6 are gone (an lsBookFlow kind G still posts them; they are skipped and counted).
- Level rows ("PDL 8 100% 10m ?") unchanged; their % is of the king. They are now de-duplicated against the king only (before: against the
  top 6), so a level whose liquidity was a rank-2..6 row keeps its level row.
- MARKS on the king row (KingMark.h 1.0.0; own MBO feed only - an lsBookFlow / no-MBO source gives no mark):
  - L = more than half of the king went by CANCELS (DEL, MOD away, size cut) - not fills - before price reached its price.
  - P = the same after price reached it (traded at / through it, or fills there) = pulled while hit.
  - A FILLED king (fills >= cancels) just goes, no mark. A marked row stays 1 minute (frozen), then the next king. The 1-minute node hold is
    bypassed when a mark appears / ends so the mark shows at once.
  - A new king replaces the old one only when strictly bigger (a tie keeps the king - no bid / offer flicker); a level that already lost
    half its size is never crowned (the board's 10 s grace).
- Records for the nightly: every king end -> `lsFlexLevels\DealerProfile\<MKT>-<spb>-king-<session>.csv`:
  `K|end_ct_s|kind (L P F R X G)|side (1 bid, -1 offer)|price|lots|peak|cancelled|filled|reached|king_from_ct_s|last`.
  The nightly scores L / P then held vs broke from the price after end_ct_s (needs a scorer - not added here).
- Status: TOP_BOOK_G gains "ONE KING (2.67.0) mark now X drawn N,ranked rows not shown N,king records N"; a new KING line (KingMark counters).
- TopBook 1.3.0: public restingAt / cxlAt / fillAt (read-outs only). LimitFeed 1.2.0: posts code 1 (state = 'L' / 'P') + code 7 + 99.
- No settings change (parameter version unchanged).

## Tests (test_dealerprofile_2670.cpp, ASan/UBSan, -Werror -Wall -Wextra; SDK as system header)
- 85 passed, 1 failed - the failing check ("the level line is bright ...") fails identically on 2.66.2 (fixture issue, not this change).
- New: K1 one king across both sides; K2 / K2b re-priced and young orders never king; K3 L on a cancel before price; K4 / K4b P on a cancel /
  >half size cut while price is at it; K4c a one-third cut keeps the king, no mark; K5 fill = no mark, next king, no repeated ends; K6 L lasts
  59 s then the next king; K7 level rows unchanged + no copy of the king; K8 two charts / markets independent; no-MBO = no mark; tie = no
  flicker; host: "505  L" drawn at once, inside the column, no text overlap, recorded once, status lines.
- KingMark.h + TopBook.h + LimitFeed.h: -Werror -Wall -Wextra clean. MinGW -fsyntax-only vs the real irtsdk.h: 0 errors, the same 8
  pre-existing DealerProfile.cpp warnings as 2.66.2.

## Differs from the mockup
- None on the face. The king is the biggest real PRICE LEVEL (the orders resting at one price), as today's K - not a single order id.

## Risks / not verified
- Not verified on his IRT yet: needs the restart, then screenshots in 2+ markets.
- If IRT reports a fill as FILL followed by a size MOD of the same order, the engine counts no cancel (fill already reduced it) - right; if
  IRT sends a size MOD without a FILL for traded size, an L / P could be over-called. Check the first live king records.
