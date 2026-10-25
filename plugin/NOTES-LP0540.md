# lsLiquidityProfile 0.54.0 - the AFTER-FILL READ RE-BID / ACCEPTED (LmtZonesLogic.h 1.3.0), 2026-10-25

0.54.0 was the next free version (0.53.0 is the last shipped build; no 0.54 existed).

## Request
- Rassul's go, Sunday 2026-10-25 (IRT plugin changes allowed today).
- Remove the drawn HELD? tag (keep logging HELD).
- Add the research's PROVEN read **H6-FILL** (iter1024/iter6 ITER6-HARDENING.md 4.2 + Card F1) as drawn tags after a zone FILLS:
  - **RE-BID** (red for an offer swept above, green for a bid swept below): within 30 s of the fill, new defender size stacks beyond the
    filled price AND the aggressor delta goes to the defender side. Meaning: the sweep is likely failing.
  - **ACCEPTED** (grey): not both. Meaning: likely continuation.
- Decided once at the 30 s mark, never repainted, recorded; the "State tags" box renamed "After-fill tags (RE-BID / ACCEPTED)", default on.

## What changed
1. **Drawing.**
   - HELD? is no longer drawn. HELD is still decided on the closed bar and recorded (T lines), as in 0.53.0.
   - FILLING (grey) and the grey X stay. FILLING now goes with the zone line (the tags box is the after-fill box now).
   - RE-BID / ACCEPTED: one word at the filled price, on the bar of the decision (fill + 30 s): above an offer, below a bid.
     - RE-BID colour = "RE-BID offer colour" (red) / "RE-BID bid colour" (green). ACCEPTED = the zone grey.
     - Bid zones still have no line (bids are never drawn); a bid zone's read is drawn as the word only. The research read works on
       both sides (strict dedup: bids 54% vs 39%, offers 51% vs 39%).
     - Hidden by the After-fill box, the master box and the zone kind's box. No numbers, no borders, no sub-text.
2. **The read (research-exact, lmt6_indep.py):**
   - Trigger: the zone order is FULLY filled (traded >= the most it ever showed), inside the study's RTH window (ES / NQ 8:30, CL / NG
     8:00, GC / 6E 7:20, HG 7:10 to 15:00 CT), first hit >= 30 s after it showed the floor size, never re-priced since then. FAR zones never.
   - Window [fill, fill + 30 s).
   - Stack = defender-side NEW lots minus DEL lots at a FIXED band 1 .. floor(max(1, 0.25 ATR)) ticks beyond the filled price (below a bid,
     above an offer). A DEL counts the order's last shown size (= the Databento C row size, also after a full fill).
   - nb = stack / (adds + cancels) >= the market's top-third line (strict-dedup q2/3 of the research rows): CL 0.0102, ES 0.0144,
     6E 0.0114, GC -0.0076, HG 0.0149, NG 0.0414, NQ -0.0068. Nightly override: `PARAM,FILL_NB,<x>` in LRA-LmtZones-<MKT>.csv.
   - Delta = defender aggressor lots minus attacker aggressor lots in [fill + 10 s, fill + 30 s) > 0. The aggressor is the opposite of the
     resting order's side in the book; implied / unknown-side fills are not counted.
   - BOTH -> RE-BID, else ACCEPTED.
   - Strict dedup: one read per side and price per session (the research headline).
   - Decided at the first MBO event at / after fill + 30 s; with an idle feed, at the first bar close >= 60 s after the window.
3. **Record** `<stem>-lmtzones-<session>.csv`, new line `R|id|fill_ms|decide_ms|RE-BID or ACCEPTED|stack_lots|delta_30s|adds|cancels`.
   - The brief's `F|...` letter was already taken (F = a zone fill), so the line is `R`.
   - Read back after a restart (the read and the dedup key); a 1.2.0 reader skips R lines as an unknown type (not "bad").
   - A read whose 30 s were not seen (an IRT restart inside the window) is never decided: counted as "lost" on the status line.
4. **Hover:** the odds row "Once filled: RE-BID sweeps failed 53%, ACCEPTED 39%"; the how rows explain RE-BID / ACCEPTED; the Now /
   Ended row shows "RE-BID 12:21 PM (stack 10, delta 8)" in red / green (ACCEPTED grey); the example row is a real case (HG Oct 22,
   NG Oct 14 below). HELD? is no longer mentioned.
5. **Settings:** the tags row is now "After-fill tags (RE-BID / ACCEPTED)", default on; colour rows "RE-BID offer colour" / "RE-BID bid
   colour". Keys and positions unchanged; parameter version 9 -> 10 (old saved windows reload once). Per-market file `LMZ3`; an old
   `LMZ2` (0.53, its box was the State tags) or `LMZ1` file loads with the after-fill tags ON.
6. **Status line** LMT_ZONES: `tags filling N held (log only) N ... after-fill on re-bid N accepted N watching N (skipped: repeat price,
   outside RTH, hit < 30 s, re-priced, lost; stack line, RTH from)`.
7. **Versions:** LQ_VERSION = setVersion = lpset::PLUGIN_VERSION = lqp::VERSION = 0.54.0; lmz::VERSION 1.3.0.
8. **Test tool fix:** tests_lmz/conv_mbo.py no longer writes a synthetic DEL on a full fill. That DEL turned an iceberg reload (the same id
   re-armed by M after its fill) into a phantom order that never ended and later became a zone. The converter now mirrors the Databento
   rows (F = FILL only; C = DEL of what remains; M = MOD). Replay numbers from 0.53.0's NOTES used the old converter.

## Mapping differences vs the research (unavoidable or deliberate)
- **Universe:** the research reads every order >= the market floor (all 7 markets: ~115 fills per market-session). The live read is only
  for LMT zones (>= 3x the live median, picked at bar closes). The research itself said: restrict to drawn LMT zones / the q99 tier.
- **Bid zones are tagged without a line** (bids are never drawn by the approved 0.53 rule).
- **6E floor:** the zone floor is 5 lots (the research used 9 for its universe).
- **ATR / band:** the band uses the zone's ATR at its start bar (the research used the ATR at "first showed the floor + 30 s").
- **Delta:** from the order feed's FILL rows against known resting orders (the research used the trade tape with its side). Fills against
  orders the chart never saw, and implied fills, are not counted.
- **Time resolution:** live IRT stamps are whole seconds, so the 10 s / 30 s edges are at second resolution (the replay uses ms).
- **No outcome filter:** the research dropped rows resolved before the decision and OPEN rows (outcome-based); live cannot.
- **Dedup:** one read per side / price among zones; the research deduped over all orders.

## Tests (Linux, g++ 13, ASan / UBSan)
- `tests_lmz/test_lmz.cpp`: **114 / 114** (85 in 0.53.0) with -Werror. New cases:
  - partial fill = no read; full fill starts the watch;
  - adds / cancels counted only at the fixed band beyond and on the defender side;
  - delta from fill + 10 s; an event at exactly fill + 30 s is not counted;
  - RE-BID (offer), RE-BID (bid, green), ACCEPTED by delta, ACCEPTED by stack;
  - R line text; restart reads it back with no new R line;
  - dedup by side / price; RTH gate (GC 7:20-15:00); idle-feed decision; session-end lost; FAR never; hit < 30 s; re-priced;
  - FILL_NB param; garbled R lines; afterShown with the boxes; hover RE-BID red / ACCEPTED grey; LMZ3 / LMZ2 settings.
- `tests_lmz/test_lmz_host.cpp` (mock host, the whole plugin): **51 / 51** (45 in 0.53.0). New:
  - HELD decided + recorded but never drawn;
  - bid zone filled -> green "RE-BID" below the bid price, no bid line;
  - R line recorded;
  - restart: the RE-BID read back and drawn identically, 1 R line;
  - the After-fill box hides RE-BID (line, label, FILLING stay);
  - the RE-BID bid colour applied; the Near box hides the near bid's read;
  - LMZ2 with tags off loads with After-fill on; the settings table (label, default on, parameter version 10).
  - One master-off check now looks for the column-header "LMT" only (the new fixture also lights the absorption banner's LMT chip).
- **Replay vs the research FILL rows** (`tests_lmz/fill_parity.py`): 44 sessions, 2026-09-21 .. 10-22.
  - Sessions: HG 20, NG 15, CL 4, 6E 3, GC 2, ES 2, NQ 1.
  - **Jaccard of the decided fills 0.953** (target 0.9).
    - 818 decided; 40 orders added before the research's load window (RTH0 - 90 min) and 18 resolved before the decision are left out.
    - 747 common, 13 engine-only, 24 research-only.
  - **Read agreement on common fills 0.959** (RE-BID vs the research BOTH flag): 596 / 120 agree, 20 + 11 differ.
  - **Failed-sweep (reversal) rate: RE-BID 45.8% (n 131) vs ACCEPTED 40.6% (n 616).**
    - The direction is the research's. On the SAME rows the research's own flag gives 45.0% vs 40.7%, so the smaller gap comes from the
      population (LMT zone orders), not from the mapping.
    - Not significant on this sample: +4.0 pp, t 0.84 over all eligible.
    - By market: HG 55% vs 42% (58 / 204), NG 44 / 42, 6E 50 / 40 (n 2); CL 35 / 42, GC 30 / 41, ES 22 / 28, NQ 33 / 48 (n 3-17 RE-BIDs).
    - The research's 53% vs 39% is over all orders >= the floor (17,476 fills).
  - Every session replay: restart after every bar identical, IRT restart path 0 new T / E / B / R lines (44 / 44 "ok").
  - About 16 after-fill words a session (3 RE-BID + 13 ACCEPTED), e.g. ES 10-22: 4 RE-BID / 30 ACCEPTED.
- **Regression suites:**
  - test_lqp044 114 / 114; test_lqp452 27 / 27; test_lqp044_host 21 / 21; test_ap047 55 / 55;
  - test_lqp453_host 19 / 20 (the same MKT-ink failure as 0.53.0); test_lqp461_host 8 / 10 (the same two park checks as 0.53.0);
  - settings contract 43,642 checks, 0 failures (22 tables); dialog guard 24 / 24.
- **MinGW against the real irtsdk.h** (-Dsmall=char -Dfar= -Dnear=): Profile and Flow builds the same as 0.53.0 (3 errors inside irtsdk.h
  itself, 36 / 29 warnings, 11 / 4 outside the SDK; none in the changed code). LmtZonesLogic.h alone: -Wpedantic -Werror clean.
- **Mockup:** /mnt/user-data/outputs/lp054/mockup/limit-profile-v054.html. 5 real cases with zoom buttons:
  - HG Oct 22 offer RE-BID, failed sweep;
  - HG Oct 19 bid RE-BID, failed sweep;
  - NG Oct 7 offer RE-BID that FAILED (continuation);
  - HG Oct 22 offer ACCEPTED, continuation;
  - NG Oct 14 bid ACCEPTED, continuation.

## Risks / not verified (needs his IRT)
- **The edge on zone orders is not proven.** In the replay the read agreed with the research 96% and went in the research's direction,
  but on LMT zone orders the gap was only +4-5 pp (t 0.84), with CL / GC / ES / NQ the other way on small samples. His rule says show
  only what works: consider "RE-BID?" (unconfirmed) until the nightly measures 20+ per market on live zones.
- **Live look:** about 16 words a session; the ACCEPTED words are grey. Check his ChartView export after the install.
- **Bid words without a line.** A green RE-BID / grey ACCEPTED appears below a price with no line (bid zones are not drawn). The hover
  works only on drawn zones.
- **IRT restart inside a 30 s window:** that read is lost (counted), never guessed.
