# MarketProfile.h 0.1.1 + lra.awareness_plus 0.1.1 + lra.refill_rule 1.0.0 - 2026-10-26 night

**Request** (Rassul 21:29 CT approved Quality Check item 6): 6E refills showed 0% in the Market Awareness profile; use ONE refill rule in
every market - a NEW order or a size increase, same side, same price, within refillWindowMs = 1 s after a fill - in the awareness_plus
MarketProfile and the Python feature; verify 6E ~21-24%, ES ~22-24%.

**Root cause**: refill_share came from mbof_px.refills = CME NATIVE iceberg refills only (same order id, same packet), which 6E barely has.

**Rule** (lra.refill_rule): a passive fill at (side, price) is refilled when the NEXT fill-or-refill event at that side + price is a refill
(A = new order, or M with new size above what the order had left) within 1,000 ms. Synthetic rows skipped. Another fill first = not refilled.

**Verified on the raw store 2026-10-22**: 6E 21.3% of 81,320 fills; ES 24.6% of 1,694,474 (audit A-only check: 21.1% / 24.3%).

**Changes**
- lra/refill_rule.py (new): per-session share, cached per (market, session) in data/awareness_plus/refills_<MKT>.json, newest first, 150 s
  budget a night across markets (ES ~35 s a session), profile = mean of the newest 10 computed sessions.
- lra/awareness_plus.py 0.1.1: refill_share = the rule (+ refill_sessions, refill_window_ms); the native-iceberg number kept as
  ice_refill_share; an old refill_share without refill_window_ms is not carried over (unknown, never 0).
- MarketProfile.h 0.1.1: refill_window_ms default 1000; the rule documented; refill_share still has no default ('?').
- lsContext / lsDealerProfile etc. that include MarketProfile.h rebuild by header hash; no drawing change.

**Tests**: test_refill_rule.py 5/5; tests_plus test_plus 68/0 (+2), test_pump_plus 18/0, test_next 20/0, e2e 280 EXP, real-SDK Context 0 errors.
**Not verified**: test_awareness_plus.py (needs the PC store + fixtures; refill budget 40 s there). The first night fills the cache gradually.
