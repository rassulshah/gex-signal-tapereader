# One bar per candle - lsBidAskStats 1.7.0, lsStackStats 1.4.0, lsPullStats 1.6.0, OrderFlowFeed.h 1.6.0 (2026-10-14 night)

Rassul approved at 22:37 CT (mockup-one-bar.png v2). Corrections relayed 23:00 CT: remove the white letters and white dashes, then
"ONLY in the StackStats pane" (BidAskStats and PullStats keep their letters and marks on the single winning bar).

| What | Change |
|---|---|
| All three panes | One bar per candle = the bigger side (colour of that side, height = its lots). Tie -> the side of the candle's close direction (chart open / close), else red. Bright only when the WINNING side is a spike (its spike label above, CR5300 persistence unchanged); dim otherwise. White tick (2 px, across the bar, 1 px past each side inside the slot) at the smaller side's lots ONLY when both sides met their own spike rule. |
| BidAskStats 1.7.0 | Buyers = side 1 (offers lifted, green), sellers = side 0 (red). S / L / R? on the single bar for the winner; the loser's letters stack in the same column (above) only on a both-spiked candle; refill dash = the winner's. Title (parmsTitle) "Bid/Ask Stats - Who is Pressing v1.7.0"; message "Both sides pressing evenly"; description "who is PRESSING". |
| PullStats 1.6.0 | Buyers = side 0 (bids pulled, green). Letters as BidAskStats. Title "Pull Stats - Who is Leaving v1.6.0". |
| StackStats 1.4.0 | Buyers = side 0 (bids added, green). NO letters (I? / L) and NO refill dash drawn (Rassul 23:00 CT) - the tick is its only white mark; the record and hover keep them. Title "Stack Stats - Who is Arriving v1.4.0". |
| OrderFlowFeed.h 1.6.0 | oneBarOf / oneBarWinner / oneTickBox / oneBarLetters / copyTitle; BarLook.dir; drawBars(..., oneBull = -1, marks = true) - the defaults keep the old two bars for any other user (Liquidity Profile / Flow do not call drawBars); MboPane::oneBarBull() (-1) and barMarks() (true). |
| Unchanged | Hover (both sides' numbers), read lines (ZoneRead.h 1.1.0), settings, records, axis (already the max of the two sides = the single bar), spike label persistence, Liquidity Profile. |

Tests (ASan/UBSan, -Werror): tests_of/run_tests.sh all green - new test_onebar.cpp: logic 41, BidAskStats host 7, PullStats host 4, StackStats host 4;
adapted (intended rule change, each marked "1.7.0 / 1.6.0 adapted"): test_bas_host (two bars -> one per candle, 103), test_bas130_host (51), test_pane_host Pull 71 / Stack 58
(StackStats: no letter / dash drawn, record keeps them), version checks in test_off_logic / test_of140_logic / test_panel_logic / test_pane_host.
MinGW against the real irtsdk.h: BidAskStats, PullStats, StackStats, TapeReader, LiquidityProfile, LiquidityFlow 0 errors / 0 warnings outside the SDK.

Not verified on his IRT yet: the pane title row (parmsTitle is used by lsTapeFlow; check the three titles show after the restart) and the look of the tick on a live two-sided candle.
