# TopBook 1.0.0 / lsBookFlow 1.37.0 / lsDealerProfile 2.54.0 (2026-11-09, Rassul 12:55 CT "do it")

The Limit Profile's Liquidity column showed nothing since lsLiquidityProfile was removed (11-05, IRT freezes). Now:
- `TopBook.h` (SDK-free): fed every market-by-order event inside lsBookFlow (on all 7 charts), asked once a second for the 3 biggest REAL bids
  below price and 3 biggest REAL offers above. Real = rested 30 s at its price, not re-priced 2+ times in 5 min, past the first 2 ticks, >= 2.0x
  the side's median real level within 3 ATR (3-min), not pulled on an earlier approach (lost >= 50% by cancels while price was within 3 ticks ->
  banned at that price for the session). Steady boards (Top10Stable n = 3). Tags: "?" = >= 70% one order (pulled less, held no better -
  "may not reload"), "S20" = holds one order >= the market's Big Lot S20, never moved, resting >= 2 min (the LMT order of
  INSTITUTIONAL-ORDERS-LMT-2026-10-22; GC / 6E never tagged). HELD / BROKE / pulled-while-hit counted, logged in the status only.
- lsBookFlow 1.37.0 posts SignalBoard kind **G** (ChainSpec: producer lsBookFlow, edge -> lsDealerProfile): lo = price, side +1 bid / -1 offer,
  val = lots, code = rank 1-6 (1 = K), bar = order count, proven bits 2 = "?", 4 = S20; code 99 = heartbeat (val = book orders; 0 = cold book).
- lsDealerProfile 2.54.0 reads G first (the lsLiquidityProfile 'B' path stays as the fallback): rows rank | bar | "lots" + " ?" / " S20".
  Status TOP_BOOK_G. Settings: no change (spec versions bumped with setVersion).

Tests (Linux g++, ASan/UBSan): tests_tb/t_topbook 23/23 (synthetic) + ES Nov 6 replay; tests_bf137/test_bf137_host 14/14; test_dealerprofile_2540
10/10; test_dealerprofile_2523 13/14 and tests_bf135 15/17 (only their version checks); tests_chain 17/17; settings contract PASS (24 tables).
Replay ES Nov 6 09:45 CT: offers 7454.50 (172, "?"), 7450 (76), 7470 (69); bids 7416 (64), 7420 (61), 7412 (60) - 5 of 6 = the mockup.
Worst evaluate 7 ms (10k orders), once a second.

Filter check (t_check: every touched level, held = 0.5 ATR back away before 0.25 ATR through, RTH):
| | ES Nov 6 | NQ Oct 27 | GC Oct 30 |
|---|---|---|---|
| shown | 40.9% (159) | 36.0% (283) | 31.3% (259) |
| shown, many orders | 42.4% (144) | 37.9% (219) | 32.5% (240) |
| shown, mostly one order ? | 43.2% (37) | 31.7% (123) | 26.5% (49) |
| shown, S20 order | 52.6% (19) | 28.6% (21) | - |
| removed: inside 2 ticks | 34.7% (366) | 32.5% (5383) | 30.5% (1222) |
| removed: young < 30 s | 31.9% (817) | 33.5% (6162) | 31.6% (2196) |
| removed: quoting | - | 50.0% (62) | 22.2% (18) |
| removed: pulled earlier | 32.6% (344) | 31.0% (481) | 31.6% (459) |
One day each = LEARNING. ES / NQ: kept levels hold +3 to +9 points more than removed ones; GC: no difference (its median real level is
1-2 lots - GC book levels are not reaction levels, as the 10-22 study found). Next: nightly multi-day check; the real-order model
(LRA-RealOrder-<MKT>.csv) "NN% real" on ES / CL / NG rows = follow-up.

## TopBook 1.1.0 / lsBookFlow 1.38.0 / lsDealerProfile 2.55.0 (Rassul approved 13:26 CT)
- A level shows only after holding big size for 2 MINUTES (board entry wait, was 30 s); 10 s grace and the 1.25x swap unchanged.
- CLUSTER rule: a level whose biggest order came with another big same-side order (>= the market's Big Lot S20, all 7 markets incl. GC 9 /
  6E 42 RTH) within 2 ticks inside 60 s is not shown.
- "NN% real" (row text "172 ?  48% real"): the per-market real-order model read natively from LRA-RealOrder-<MKT>.csv (re-read when the file
  changes, checked once a minute), shown only while the file says LIVE (ES / CL / NG today). Scored for the level's biggest order when it is
  >= half the market's S20 and was posted >= 2 ticks away. APPROXIMATION: the model's 2SD / 3SD tiers are not known natively - size_S = order
  >= S20, size_3SD is never set (its coefficient: ES -0.08, CL 0, NG +0.31).
- Tests: tests_tb/t_topbook 33/33; tests_bf137/test_bf138_host 16/16; test_dealerprofile_2550 10/10; chain 17/17; settings contract PASS.
- ES Nov 6 09:45 with the 2-min rule: the same 6 levels; % real 48 (7454.50 "?"), 92, 95, 88, 92, none (7412: biggest order 20 lots).

Hold time per market (t_check, one session each = LEARNING; shown levels' held %):
| show after | ES Nov 6 | NQ Oct 27 | GC Oct 30 |
|---|---|---|---|
| 30 s | 38.9% (162) | 36.7% (283) | 30.6% (258) |
| 60 s | 41.2% (153) | 36.2% (279) | 31.5% (251) |
| 2 min | 43.8% (144) | 38.2% (272) | 31.0% (248) |
| 5 min | 42.6% (129) | 38.2% (267) | 31.8% (223) |
ES 2 min: model >= 70% real held 50.0% (64) vs < 70% 47.6% (21). Cluster-removed: ES 15 (46.7%), NQ 2, GC 5.
NOT YET WIRED: the nightly per-market hold-time report (t_check takes the show time as its 2nd argument; needs a nightly step that streams the
day's raw MBO through it for 30 s / 60 s / 2 min / 5 min and reports a better hold time before anything changes).
