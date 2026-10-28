# lsLiquidityProfile 0.67.0 + lsDealerProfile 2.35.0 + LiqTouch 1.1.0 - the zone candidate rule (2026-10-28)

## Request (Rassul approved 14:04 CT)
Replace "king + top 3 within 3 ATR, tier >= p97 (ES 72 lots)" with:
1. Real orders only: rested >= 30 s at their price, re-priced < 2x in 5 min (quoting / market-maker style orders excluded).
2. Zones: real lots at a price + its +-1 tick neighbours (same side); the zone's price = the price with the most lots.
3. Size: zone >= 2.0x the median real level (non-zero, same side, within 3 ATR), median recomputed every 60 s (per market automatically).
   Own distribution: >= 3 ticks from a bigger node AND the smallest raw level between them <= 0.7x the smaller (else the bigger is kept).
4. Top 3 BIDS below price + top 3 OFFERS above (max 6, fewer when fewer pass). 5. King = the biggest of all shown (white chip + K, edge
   marker unchanged). 6. Touch tags + HELD on every shown level; ranks / king over the whole book in range (never the viewport).
7. Hysteresis (Top10Stable) kept, one board per side, cap 3 each. The p97 tier stays a log field only.

## Where it is decided (one rule, one place)
- LiqTouch.h 1.1.0: ltc::pickZones / sideMedian (SDK-free) + Engine::zoneStep (Params.zoneGate): real-order book from the order-by-order
  feed (per order: time at its price, the last two re-price times), median per side every 60 s (sooner while a side has none), the
  candidates every 1-s boundary; every candidate is followed by the touch engine at once (its orders already rested 30 s); a touch = the
  first trade inside the zone (+-1 tick). zoneGate false = the 1.0.0 study engine exactly (parity 91/91 unchanged).
- lsLiquidityProfile 0.67.0: zoneGate on; posts the candidates as SignalBoard B facts code 2 (val = zone lots, proven = the lots at the zone
  price) - only these (no order feed yet = the 0.66 list, DP's old path). Status LIQ_CANDIDATES (rule, medians, candidates, p97 tier log).
- lsDealerProfile 2.35.0: code-2 facts = zone mode: taken as posted (no second reach / moves filter, so the column and the tags follow the
  same levels), two Top10Stable boards (bids / offers, n 3), merged biggest first -> rank 1 = king; touch tags / HELD for all shown (up to 6).
  Status LIQUIDITY_TOP10: candidates ZONES ..., shown bids x / offers y (max 3 a side), shown top 6.

## Tests (Linux, mock host)
- tests_liqtouch/test_liqtouch_zone.cpp (new) 18/0: ladder 3 x 20 = one 60-lot zone passes at median 22 (fails at 31), 2.0x gate with a
  computed median, own-distribution rule, offers above / bids below, fewer when fewer pass, no median = nothing, real orders only (30 s rest),
  re-quoting (2 re-prices) excluded, zone touch +-1 tick, defaults keep the tier gate.
- test_liqtouch_lp_host 15/0 (updated to the zone rule + 2 new checks: candidates, B = code 2 only), logic 38/0, parity 91/0.
- test_dealerprofile_2350.cpp (new) 87/0: the 2340 suite (73, version check -> 2.35.0) + zone mode: top 3 bids + 2 offers, king = biggest
  overall, 3-per-side cap, code-0 never shown, ranks, sides, status, per-side hysteresis (1.1x out, 1.33x in), viewport independence, tags
  latched for rank 4 / 5 levels only if shown, HELD on the rank-5 offer, same HELD after a restart (no repaint).
- tests_chain 17/0; LP regressions identical to before (lqp044 4 / lqp452 2 / lqp453 2 pre-existing failures).

## Known limits / not verified
- STAYS / GONE still measure the size at the zone PRICE (the biggest tick), not the whole +-1 tick zone.
- MBO has no participant ids: "market maker excluded" = re-quoting orders (re-priced 2+ times in 5 min) are excluded.
- LP uses its own 3-min ATR (the touch engine's) for the 3-ATR reach; DP no longer re-filters by its chart ATR in zone mode.
- Needs his IRT: the live column after the restart (ES + NQ), how many zones pass per market overnight vs RTH.
