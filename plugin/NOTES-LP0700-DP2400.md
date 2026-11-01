# lsLiquidityProfile 0.70.0 (LiqTouch 1.3.0) + lsDealerProfile 2.40.0 (LiqPick 1.0.0) + lsExhaustion retired - 2026-11-01

Rassul's decisions 2026-11-01 09:49-10:21 CT (mockup lp_proposal.html, then "you dont need these ranks ... give it the rank whatever it is like 6, 7
etc. remove exhaustion indicator, book momentum line, held lines"). Installs at his next IRT close via gex-build.

## Liquidity column = the whole book
- lsLiquidityProfile 0.70.0 posts the WHOLE-BOOK real nodes as SignalBoard B code 4 (LiqTouch 1.3.0 bookNodes): real = rested >= 10 min at its
  price and re-priced < 2x in 5 min; ANY distance from price (no 3-ATR reach); bids below the last trade, offers above; node = price +-1 tick at
  the price with the most lots; one node per distribution (>= 3 ticks apart and a valley <= 0.7x); 20 per side. f.bar = when it first became a node.
  Codes 2 (zone candidates) and 3 (kings) are no longer posted; the touch engine still follows its candidates internally (records unchanged).
- lsDealerProfile 2.40.0 ranks them on ONE steady board (Top10Stable: in after 30 s + 1.20x, out 10 s after gone, swap 1.25x; 15 deep) and shows
  (LiqPick.h): ranks 1-5 (bids and offers together) + any other ranked node >= 50% of the king within 1 x the 3-min ATR ABOVE a high level (PDH,
  PFH, ONH, LonHI, VAH, HOD, ...) or BELOW a low level (PDL, PFL, ONL, LonLO, VAL, LOD, ...); weekly POC / dPOC count on either side. Such a node
  keeps its REAL rank (6, 7, ...), a dimmer bar and the level names, nearest first ("164  62%  PFL/PDL"; shortened to "VAH+" when it does not fit).
- Row: [K white box for the king / rank number] | bar (length = % of the king) | "265  100%" (king bold). No age, no "(3.4x)", no K rows / king
  edge markers (the king is rank 1), no STAYS / GONE / R? tags.
- Status: LIQ_BOOK line (nodes, king, top / level-linked counts). No code-4 facts (older LP) = the 2.39 path, said in LIQ_BOOK.

## Layout
- The Key Levels column moved RIGHT: it is the left 200 px of the published Levels width (430 = Key Levels 200 + Liquidity 230), so the right side
  is: price | Delta | Key Levels | Liquidity | Dealer | axis. The price area starts at the pane's left edge (the LRA_COLV_ board key is cleared:
  no plugin keeps out of the left edge any more). Badges left-justified at the column's left edge, ticks from the left edge (price side).
- lsHodLod no longer docks next to a left Key Levels column (none on the board): its table uses its own Position setting.
- No odds % on the badges (DP_ODDS false; the "level odds" setting has no effect) and no rank chips on the badges (DP_RANK_CHIPS false). The
  liqlevels.csv / rank history for the MenthorQ Reader are still written (same 0.25 ATR one-level match).

## Removed
- HELD / BREAK order lines and words on the chart (LP LQ_ORDER_LINES_DRAW false; OrderLines records still written; no G facts).
- The LMT F column (LP LQ_BIG_COL false: nothing published on the BIG board key; the big-order engine keeps running for the book / facts).
- lsExhaustion (and with it the book momentum line, S / P letters, T marks): its gex-build line is commented out. Rassul removes it from his charts.

## Tests (Linux, ASan/UBSan)
- test_dealerprofile_2400.cpp 25/0 through the real DealerProfile.cpp (layout 430 = 200 + 230, Delta | Keys | Liquidity | Dealer, keys cleared on the
  board, leftKeep = pane edge, badges left-justified, no odds / chips, ticks from the left edge; ranks K 2-5 + 6 / 7 / 8 level-linked (VAH+ / PFL /
  DPoc), not shown: 50% with no level, beyond 1 ATR, 33%, a code-2 zone; "900  100%" bold; no tags / ratios; rows inside the column; Q facts 8;
  status LIQ_BOOK; 30 s anti-flicker kept; no code-4 producer = old path).
- tests_liqpick/test_liqpick.cpp 14/0 (the approved mockup's book: top 5 + rank 6 PFL 65% + rank 7 PFL/PDL 62%; 39% and the off-level 57% not shown).
- tests_liqtouch/test_liqtouch_book.cpp 13/0 (+ king 7/0, zone 18/0, logic 38/0 unchanged).
- Superseded by design: test_dealerprofile_2390.cpp (left Key Levels column, rank chips, odds, 170 px liquidity column) and the HELD-line checks of
  tests_oln/test_oln_host_068.cpp.
- MinGW -fsyntax-only vs the real irtsdk.h: DealerProfile, LiquidityProfile, LiquidityFlow 0 errors (LiquidityProfile's 12 misleading-indentation
  warnings are pre-existing).

## Not verified - needs his IRT
- Live whole-book nodes per market (how many real 10-min nodes exist overnight), the 430 px right side on his screen width, lsHodLod's own position.
