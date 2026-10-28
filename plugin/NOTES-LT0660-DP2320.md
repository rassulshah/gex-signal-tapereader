# lsLiquidityProfile 0.66.0 + lsDealerProfile 2.32.0 - touch tags, HELD, top 3 within 3 ATR (2026-10-28)

## Requests (Central time, 2026-10-28)
- 11:05 Rassul approved the liquidity-column build from the 10-28 study (report liquidity_full.html section 9): the 15-s touch tag
  (STAYS+ / STAYS / STAYS weak / GONE / GONE+), colour by meaning, final once printed, the Tape Reader votes, per-market tiers from a
  nightly-refit params file, log-only reads, no K-x magnet mark (the study refuted it); the column = KING + top real orders.
- 11:08 "i also like the held signal, so i know we dim the liquidity now and i like to show it when it held" - HELD on the chart.
- 11:27 "i want the HELD lines that show held and the line"; the column = KING + next 2 = TOP 3, only within 3 ATR (3-min) of price.

## What changed
- NEW LiqTouch.h 1.0.0 (SDK-free): the study's wall-episode engine (eng2.py) + touch reads (touch.py) ported exactly: big level = resting
  size at the market tier (p97: ES 72, NQ 12, CL 16, GC 10, HG 10, NG 30, 6E 128 lots) for 30 s, 2 ticks .. 3 ATR from the last trade;
  touch = the first trade there; at +15 s STAYS (>= 50% left and refilled >= half the eaten) / STAYS+ (+ <= 4 s at the level and churn
  < 15x, NG no churn rule) / STAYS weak (14-15 s at the level) / GONE (< 30% left) / GONE+ (refill < 25% or > its size pulled in the last
  minute) / MID / EARLY (already held or broken); votes (STAYS+ +2, STAYS +1, weak 0, defenders also aggressive +1, GONE +1 attacker, GONE+
  or all-in attack into a non-STAYS level +2 attacker), signed + bullish; the 30-s read (log); the outcome HELD / BROKE / EXPIRED (study
  'hold': 0.5 ATR away before 0.25 ATR through, 10 min).
- lsLiquidityProfile 0.66.0: runs the engine on its DEPTH_BY_ORDER feed (Central ms per event, a 2-s-grace host clock so a quiet feed still
  decides), the per-market params lsFlexLevels\LRA-LiqTouch-<MKT>.csv (60 s re-read, strict; missing / garbled = defaults + reason),
  the record LiquidityProfile\<MKT>-<sym>-<spb>-liqtouch-<session>.csv (T / D / O lines + a B line per minute = p97 of book level
  sizes within 3 ATR for the nightly tier refit), read back after a restart (open outcomes resumed); SignalBoard facts kind S
  (code 0 touched live, 1-7 tag with val = vote, 11-13 outcome); status LIQ_TOUCH. Nothing new drawn by LP.
- lsDealerProfile 2.32.0: the Liquidity column = the TOP 3 real orders (king + 2) ranked over the posted book within 3 ATR (3-min ATR =
  the chart's 14-candle ATR scaled to 3 min) of the last price; the king edge marker unchanged; touch tags latched ONLY for levels shown
  at the touch (DealerProfile.touch-<MKT>-<spb>.csv, atomic, 36 h, read back = no repaint); the tag in a reserved lane after the rank
  chip (bars never jump), tag-only rows for levels that left the top 3; HELD = the level line bright 2 px from the touch candle to the HELD
  candle + "HELD" right of it (bid green / offer red), stepped right a candle at a time past candles and level badges, else no word;
  BROKE = no mark; status TOUCH_TAGS.
- ChainSpec 1.4.0: kind S (lsLiquidityProfile, layer 10) -> lsDealerProfile DISPLAY. test_chain: the stale 'Z has no producer' injection
  fixed (uses an unowned kind).
- Nightly part lra.liq_touch_fit 1.0.0 (parts.json): held rate per tag per market vs the study, tier refit (median of the per-minute p97,
  >= 60 samples, at most x1.5 a night), guarded dwell / churn refit (300+ touches, better in both halves), writes LRA-LiqTouch-<MKT>.csv.

## Tests (Linux, mock host)
- tests_liqtouch/test_liqtouch_parity.cpp: REAL ES MBO 22 Oct, six study touches (STAYS+, GONE, weak, STAYS, GONE+, offer STAYS): tags,
  outcomes, sizes, episode start, pull-60, ATR identical to touch.py; 91/91 (also under ASan/UBSan). Known source difference: the study's
  Databento engine nets an iceberg refill against the fill right before it - net (added - eaten) identical, each side +N.
- tests_liqtouch/test_liqtouch_logic.cpp 38/38 (params, tags / votes, colours, no repaint, outcome, feed gap); test_liqtouch_lp_host.cpp
  12/12 (real LiquidityProfile.cpp: follow, STAYS+, vote once, record, HELD, restart = same facts, params / garbled file).
- test_dealerprofile_2320.cpp 46/46 (top 3 within 3 ATR, tags latched for shown levels only, colours, final once printed, HELD line +
  word not over candles / badges, read back after a restart). test_dealerprofile_2312.cpp is the 2.31.2 snapshot (expects 2.31.2 / top 5).
- tests_chain 15/15. LP regressions unchanged vs 0.65.2 (lmtf_host 1 / lqp044_host 4 / ap047 9 pre-existing failures, identical).
- MinGW -fsyntax-only against the real irtsdk.h: DealerProfile / LiquidityProfile / LiquidityFlow, no new warnings.

## Not verified - needs his IRT
- The live IRT FILL / MOD sequence after a partial fill (assumed as OrderFlowFeed.h documents: FILL then MOD with the rest / DEL).
- Placement of HELD on his wide chart (screenshots after the restart, ES + NQ).
- Deferred: the votes are posted on the S facts and recorded, but no combiner / Tape Reader reads them yet (needs a fitted weight -
  the nightly scores the tags first); the far-king bright-wait gate is moot while only levels within 3 ATR are shown (params carried).
