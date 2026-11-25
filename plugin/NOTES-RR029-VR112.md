# lsRevRead 0.29.0 + lsVolRead / lsRangeRead 1.12.0 - the panes read in order (2026-11-24)

## Requests
- 19:31 "currently we show the change in delta, should this be updated to buy and sell volume instead"
- 19:33 "lets do that and you can then determine the reads for all and how best to coordinate the indicators and the read. i would also hide the book flow ... i would read the volume, then the buy sell vol, then the absorbtion volume"
- 19:44 "Go" (panes-read-mockup.html)

## The order (his volume model)
1. Volume pane - how much, the biggest orders / sweeps, icebergs (whole candle) - NO verdict
2. Reversal Read pane - buy vs sell (whole candle): who hit whom, delta, the delta swing
3. Turn pane (1.8.0) - who won at the candle's end (Absorption / Initiative) - the only verdict

## lsRevRead 0.29.0
- Book Flow hidden (RV_DRAW_BOOKFLOW false; its numbers still computed); no halves, no divider.
- The delta-change bars are replaced by BUY (green, left) / SELL (red, right) volume bars over the pane under the strip, up from zero, scaled per session
  on its 95th percentile; "biggest in N" (Poc.bbv / bsv) on a bar from 10. The signal strip and every price-chart mark unchanged.
- The read (BsRead.h 1.0.0): "Buyers 909 vs sellers 598" (+ "- the most buying in N candles" from 10; "Even" within 10%), "Buyers 1.9x normal,
  sellers 1.3x - delta +311.", "Delta swung +284 from the candle before (3.2x) ?" at 1.33x+ its normal; a strip letter on the same candle adds "Strip: <its step>".
- Evidence (new 10-candle lows / highs closed out of the end zone, 7 markets, n 7,947): the turning side's volume 1.5x+ 69% vs 55% (halves 69 / 69);
  the delta swing 1.33x+ the turn's way 66% vs 57% (halves 68 / 65, all 7 markets); both 72%, neither 54%.

## lsVolRead / lsRangeRead 1.12.0 (VolRangeLogic 1.12.0, VRRead 1.3.0)
- The Volume read: "Heavy - the biggest green bar in 99+ candles (1.9x)" (Heavy 1.5x+, Above normal 1.2x+, Normal, Below normal < 0.8x, Light < 0.5x;
  coloured by the candle only when it is the biggest in 20+), "Biggest orders: buyers 25, sellers 21; one sell order swept 9 prices, buyers 7.",
  "Icebergs: bids refilled 29 lots, offers 34.", then one neutral note (climax / heavy volume that barely moved price / a run / drying up).
  No Bullish / Bearish / pushing / Watch. The numbers column (BVMax / BV / BVG / BVR), the bars and the Range read are unchanged.

## Tests
- BsRead t_bs 11 checks; VolRange t_vr1100 37, t_vr1110 28, t_vr150_split 69, t_vr180 26, t_vr190 58, test_volrange_130 26, t_vr1120 22 (new, real GC 11-20 candles) - all pass.
  test_volrange_logic fails before and after this change (needs a fixture path) - unchanged.
- Real SDK C++14 / C++17: RevRead 0 errors (the same 6 old misleading-indentation warnings), VolRead / RangeRead 0 errors 0 warnings.

## Not verified - needs his IRT
- The Reversal Read pane has no host render test: check the bars and the read on his charts after the restart (ChartView).
