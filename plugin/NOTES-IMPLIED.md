# Implied fills excluded - OrderFlowFeed 1.14.0 / lsBidAskStats 1.29.0 / lsBookFlow 1.24.0 / lsSweepRefill 1.1.1 (ImpliedFill.h 1.0.0) - 2026-10-22 evening

## Request (Rassul, CT)
- 17:16 approved IMPLIED FILLS option B. Implied (spread-driven) fills arrive in the live Rithmic order feed as FILL with side "?"
  (nightly feed check: CL 8.44% live = Databento). Share of volume: CL 10%, NG 5.4%, HG 3.3%, 6E 2.6%, GC 0.7%, ES / NQ 0.
  Exclude them from delta, big-lot counts, St / Sta, Sr and the attacker / defense letters; keep them in total volume; hover row
  "Implied: 23% of this candle's volume"; params lsFlexLevels\LRA-Implied-<MKT>.csv (PARAM,exclude,1 / PARAM,live_code,?), missing
  file = exclude anyway, no hover row.

## What changed
- ImpliedFill.h 1.0.0 (new, SDK-free): side "?" = raw buySell not 0 / 1; strict params parser (MARKET must match, 6E = EU; garbled /
  wrong market / no exclude row = exclude + reason); the hover percentage.
- OrderFlowFeed.h 1.14.0: Ev::implied set ONCE in evOf (every MBO consumer gets the flag). Engine: an implied FILL (while exclude)
  takes its lots off the resting order and sets the last trade, but is no hit, aggressor order, sweep, big lot, pace second, touch or
  refill trigger. Acc / Rec fillAll + fillImpl (NOT in the record line: records unchanged, no repaint). ImpCache / refreshImplied;
  status IMPLIED line. Before this, an implied fill on a known resting order was counted as an attack on that order's side.
- lsBidAskStats 1.29.0 / lsBookFlow 1.24.0: read the params file; box row "Implied | NN% of | volume" (white) for the candle shown
  when the file is present and the candle's fills were read live; status IMPLIED.
- lsSweepRefill 1.1.1: an implied fill never feeds a sweep (book still updated); its own params read; status IMPLIED.
- lsStopRun (St / Sta): no change needed - its drain already drops FILLs whose side is not 0 / 1 (restSide 0).

## NOT covered (by design of the data, needs a decision)
- lsDeltaProfile, lsCVD and lsBigBidAsk compute delta / big prints from IRT's chart VAP (bid / ask volume per price), not from the
  order feed: the implied lots inside IRT's VAP cannot be separated there natively. Options: (a) leave as is (ES / NQ 0%, GC 0.7%);
  (b) have lsBidAskStats publish per-candle per-price implied lots for those plugins to subtract (CL / NG mostly).
- lsLiquidityProfile (air pocket / queue meter) and lsTurnOrder see implied fills through their own event hooks (not in the list).

## Tests
- tests_of/test_implied.cpp: 62 checks (synthetic feed with "?" fills on known / unknown orders: hits, delta, aggressor orders,
  biggest order / sweep, refills, pulls unchanged; volume includes them; record line identical except the events count, reload
  round-trip identical = no repaint; include mode; params parser; hover row; SweepRefill).
- tests_aggbook full (BidAskStats / BookFlow hosts etc.) PASS; tests_sr 62 / 32 PASS; MinGW real irtsdk.h -Dsmall=char: BidAskStats,
  BookFlow, DefenseLines, SweepRefill, LiquidityProfile 0 errors (warnings only pre-existing); settings contract PASS.
- Version pins in tests updated (aggbook 113, attack 126 host / logic, moves, sr host).

## Risks / not verified (needs his IRT)
- Hover row shows only for candles read live since the indicator loaded (not after a restart: fillAll is not in the record).
- Screenshots after his restart.
