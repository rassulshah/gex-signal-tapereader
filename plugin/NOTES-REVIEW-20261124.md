# Turn 1.10.0 / RevRead 0.30.0 / VolRead + RangeRead 1.14.0 / OrderFlowFeed 1.18.0 (2026-11-24 evening)

## Requests
- 20:46 "add the question each answers to the read , so i know what question the read is answering in relation to the volume model"
- 20:46 "make the update for magnitude also so everything is covered"; 20:54 "go"
- 20:33 "i want you to do an independent design and code review for optimazation , memory leaks and efficiency and fix any issues along wthe way"

## Built
- Question column (grey, once per question) in the three reads: Volume (1 How much? / 4 How big? / 5 How?), Buy / sell (2 Who hit whom? / 4 How big?),
  Turn (3 Who won at the end? / 5 How? / 6 Did price agree?). Full questions when the read keeps 220 px, short ones when 150, none below.
- Magnitude on the bars: the buy / sell bars' DARK part = lots from big orders (>= the feed's large threshold = the S20 tier when the Large Lot
  pool is on), and a read line "Big orders (32+ lots): 31% of the buying, 12% of the selling." (OrderFlowFeed 1.18.0 Rec.bigC, in memory only:
  candles before a restart show no dark part). Research: a big order on the turning side 63% vs 58% (n 231) - descriptive, no "?" claim.

## Independent review (a separate reviewer, read-only) - fixed
1. Turn: 8 bar walks of map finds per candle per repaint (absAt / iniAt called twice each, iniAt calling absAt) -> ONE walk per side, bars looked up
   once per draw into a vector, fixed-size prior arrays (was ~100k-380k map finds + 4,800 allocations per paint with 600 candles in view).
2. RevRead: the delta-change data (40-candle history, sorts, a volume-at-price read of the forming candle) still computed every draw while hidden -> skipped.
3. Turn: the bars and the read could name different ends (the bigger number won even when grey) -> the end that counts (coloured) wins, then the bigger number.
4. Turn: crC / niC (the read's numbers per candle) never trimmed in memory -> capped at 1,800 entries each.
5. Turn: the display switch for Initiative hid it on sweep candles only -> everywhere.
6. Buy / sell read: "S20+" label could be wrong (the feed threshold can be a percentile) -> "Big orders (32+ lots)".
7. RevRead: session key computed twice per candle per draw (getLocaltime) -> once per draw; status counters now per draw.
8. Turn agreeText: a candle that both goes 1 ATR and trades through = FAILED for a hold - documented.
Checked fine: barRatios, iniCalc bounds, parseCrec N lines, drawPair array size, question widths, BigTier, bigC never > aggrC, no new static growth, mixed-type min / max.

## Tests (all pass)
- Turn: logic 3,287,189; golden 2,017; host 41,437; soak 680 (no growth); read 136 / 262; abs 31; cr 15; ini 31; agree 19; bigtier 6; real SDK 0 / 0.
- Volume: t_vr1100 37, t_vr1110 28, t_vr150_split 69, t_vr180 26, t_vr190 58, test_volrange_130 26, t_vr1120 33; real SDK 0 / 0.
- RevRead: t_bsread 16; real SDK 0 errors (the 6 old warnings). No host render test for the pane: check on his chart.
