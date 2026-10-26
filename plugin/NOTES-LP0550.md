# lsLiquidityProfile 0.55.0 - THE LIMIT-ORDER LINES (OrderLinesLogic.h 1.0.0), 2026-10-26

## Request
- Rassul's walk-through, Sun 2026-10-25 20:00-22:29 CT, ending with "build".
- Spec: outputs/BUILD-SPEC-2026-10-25.md (incl. "Final decisions, 22:29").
- Approved mockup: final_mockup/all-approved-mockup.html. Reference renderer: scratchpad stp/final.py.

## What is drawn (price chart, replaces the drawn LMT zones)
1. **Which orders get a line.** One line per big resting order:
   - shown at or above the market FLOOR (ES 50, NQ 15, CL 20, NG 10, GC 10, HG 5, EU 20 lots), or an iceberg whose shown + traded size reaches it;
   - drawn once it (or its rolled chain) has rested 9 min, back to its placement time (the | tick);
   - no line for an order placed at or after the late-day cut-off (ES / NQ 14:30, CL / NG 13:00, GC / HG 12:00, EU 13:30 CT).
2. **Moves and rolls.**
   - Same id re-priced: the line steps, with a vertical riser at the moment it moved.
   - Cancelled + a NEW order with exactly the same size, on the same side, within 15 s before or after, within 1 ATR (mean bar range of the last 40 closed bars), and the ONLY candidate: the same stepping line.
   - The pull is not shown until the 15 s are over. A FILL is never joined.
   - No roll tag: the step shows it. The hover says "Rolled: a -> b (re-placed x s after the pull)".
3. **Labels:**
   - single order: the size ("100");
   - iceberg (same id refilled after fills): "I R<refills> <lots filled>";
   - ladder (3+ orders of LADDER_PIECE_MIN .. floor-1 lots, one side, within 1 s, spanning <= LADDER_SPAN ticks, together >= the floor): "<n>o <total>", one line at the size-weighted price that steps when the pieces move.
4. **End.**
   - FILLED (at least half the size it showed traded): a fill dot in the side colour with a white rim, then one word beside the dot.
   - PULLED: the line turns very dark (30% of the side colour). No X.
   - LIVE: full colour to the right edge.
5. **Word.** Bid shown; an offer is the mirror.
   - **THRU:** in the fill bar + the next 3 bars, a bar with its HIGH below the bid.
   - **HELD:** the first GREEN bar closing above the bid.
   - Neither within 4 bars = THRU.
   - Decided on closed bars, never changed, drawn beside the fill dot (HELD in the side colour, THRU in the grey).
6. **One word per zone.**
   - A word decided within 3 bars of an earlier SAME word, on the same side, within 1 ATR, is not drawn (hover only).
   - Among words decided at the same close, the one nearest the pivot extreme wins (the earliest on a tie).
7. **Queue Q.** The QueueMeter's 10-s call as a small solid badge "Q" on the level, at the bar of the call.
   - Colour by meaning: bids holding / offers drained = green; bids drained / offers holding = red.
   - Where it shows: ES / CL / EU both colours, NQ / GC red only.
   - EU was added by Rassul 22:29 ("6e also gets q"), not validated: judged nightly.
8. **Removed:**
   - the LMT resting column (LMT F stays);
   - the LMT zone lines, labels, FILLING, RE-BID? / ACCEPTED, the X and the zone hover.
   - The LMT zone engine and its record still run (log).
9. **Settings.** The 13 LMT rows were relabelled in place (keys / positions unchanged, parameter version 10 -> 11):
   - Limit-order lines (master);
   - Icebergs;
   - Ladders;
   - Pulled lines (very dark);
   - Queue Q at levels;
   - Gamma lines;
   - Offer / Bid line colour;
   - THRU word colour;
   - Gamma line colour;
   - HELD / THRU words.
   - ABC / Gamma diamond are hidden (zone-only).
10. **Params.** lsFlexLevels\LRA-LimitLines-<MKT>.csv, written by the nightly reverse-engineering study; read every 60 s on a stamp change; missing file = the defaults above.
    - Keys: FLOOR, REST_MIN, ROLL_SEC, ROLL_ATR, LADDER_PIECE_MIN, LADDER_MIN_ORDERS, LADDER_SPAN_TICKS, WORD_BARS.
    - Lines are "KEY,VALUE" or "PARAM,KEY,VALUE".
11. **Record.** <stem>-orderlines-<session>.csv with N (line, incl. order id), S (step), O (roll: the new order id), E (end) and W (word) lines.
    - Read back after a restart: lines / steps / ends / words come back identical.
    - A restored live line re-attaches when the feed sends its order id again (never a second line).
12. **Status.** An ORDER_LINES line in LiquidityProfile.status-<MKT>-<spb>.txt (rule, counts, rolls, ladders, icebergs, held / thru, restored, writes).

## Tests
- **Logic.** tests_oln/test_oln.cpp 28 / 28 (-Wall -Wextra -Werror, ASan / UBSan): 9-min rest, roll incl. a re-place 0.45 s BEFORE the pull, roll ambiguity, fill never joined, same-id moves + steps kept from before the floor, cut-offs, iceberg label, ladder label + span, HELD / THRU rules, one word per zone, record / restore, params, hover format.
- **Host.** tests_oln/test_oln_host.cpp 24 / 24 (mock host, ES): lines at 9 min, labels, no LMT / RE-BID? / FILLING, LMT F kept, roll step, dark pulled offer, no X, HELD beside the fill dot, hover, record, status, versions, settings rows + version 11, restart identical.
- **Real MBO replay.** tests_oln/replay_oln.cpp, ES 2026-10-22, 14.9 M events in 7 s. It matches the approved mockup:
  - 1:04 PM low: one HELD on the 7,680 bids (the 100 rolled 7,675 -> 7,680; 7,682.25 / 7,685 / I R33 hover only);
  - I R15 500 at 7,690: THRU;
  - 1:21 PM high: one HELD on 7,702.25 (the offer that walked down from 7,724, 6 steps);
  - 2:02 PM low: HELD on the 52 at 7,680 (7,682 + 4o 87 hover only);
  - 7,683.50 (1:31 PM): THRU;
  - 7,688.75 (2:12 PM): THRU;
  - 7,695.75 (2:28 PM): HELD.
  - Session: 76 drawn lines, 8 rolls (1 ambiguous rejected), 9 ladders.
- **Regression.** test_lmz 116 / 116, test_lqp044 114 / 114, test_lqp452 27 / 27, test_ap047 55 / 55, test_lqp044_host 21 / 21.
  - test_lmz_host: 34 / 34; the 18 zone-DRAWING checks are retired (the zones are no longer drawn).
  - test_lqp453_host 19 / 20 and test_lqp461_host 8 / 10 fail exactly as on the 0.54.1 base (pre-existing, unchanged).
- **MinGW.** Against the real irtsdk.h (wrap_real_sdk): 0 errors.

## Not verified - needs his IRT
- The look on his chart (label stepping, fill dot size, very-dark shade on his background).
- The per-market floors other than ES are starting guesses until LRA-LimitLines-<MKT>.csv is written.
