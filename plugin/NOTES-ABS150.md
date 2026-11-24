# The Abs badge + the Abs read - lsRevRead / lsRevReadMarks 0.27.0, lsTurn 1.5.0 (2026-11-24)

Rassul 2026-11-24 (CT): 08:34 "absorption levels should match bk levels"; 09:5x "In the bade the 2nd line will be candle location and the
third line will be biggest print"; 10:03 "I also want to know how big the print was like biggest print in 50 bars"; 10:06 "Remove since and
watch because we won't have that much vertical space"; 10:07 "Attack and defense should be more focused on how 1 side absorbed the other";
10:11 "The badge can be updated like ABS/Ex when there is evidence of exhaustion"; 11:12 "ok. build this" (abs-badge-mockup.html v15).
Research: ABS-ORDERBOOK / ABS-EXHAUSTION / ABS-PRACTITIONERS 2026-11-24 (research-artifacts/setups).

## What it does
- PRICE CHART (lsRevReadMarks 0.27.0): an "Abs" badge on the candle that tested a Bk level for the FIRST time today with effort but no
  result - volume >= 1.5x the 20 candles before, range <= 1.3x, closed in the half away from the level (LevelMarks 1.18.0 absMarks, the
  research rule ported line by line; RTH window as the research: ES/NQ 8:33-15:00, CL/NG 8:03-13:30, GC 7:23-12:30, HG 7:13-12:00,
  EU 7:23-14:00 candle ends). Green under a low, red over a high, black text, at the end that was absorbed:
    line 1  "Abs", or "Abs/Ex?" when lsTurn saw the attackers' last minute fall to 0.5x or less of their first two minutes
    line 2  "Low 21%" / "Top 21%" = where the heaviest attacking price of that end quarter sits (from the extreme)
    line 3  "P16" = the biggest single attacking order wholly inside that quarter was the biggest of its side in 16 candles
            (fills grouped by the aggressor's order ID); left off under P5; "P12+" = the pane's history ran out first
  plus "Ex" on the NEXT candle when the attackers traded 0.5x or less of their Abs-candle lots, it closed further away and did not trade past
  the Abs extreme by more than 1 tick (research: 32% exhausted vs 13%, held 77% vs 59%, both halves). A short dashed line at the level.
- TURN PANE (lsTurn 1.5.0): the read right of the last candle is now the newest Abs of the last 20 candles (else "No Abs at a Bk level in
  the last hour", muted): title (time, Bullish ? / Bearish ?, the level and price), then
    Attack    who hit / lifted how many lots into the end quarter, the biggest single order and what it hit (a sweep of N ticks / an iceberg /
              an N-lot order showing on the book); or "in small orders (biggest N - no big print)"
    Absorbed  the most showing at those prices, all the lots taken, lots put back (added - pulled), iceberg lots (orders refilled after a fill)
    Result    volume x / range x vs the 20 candles before, which half it closed in; the attackers' last minute vs their first two - Ex? at 0.5x
    Ex / Next the candle after: the attackers' lots vs the Abs candle, the defenders rebuilt (net > +15%) / steady / thinning (< +7%, warning),
              the attackers' own resting side pulled (< +6%) / stacked (> +15%)
  The 1.4.x "Bullish ? - above the absorbed low" read is retired (it had no edge on the fair test). The bars and the top strip are unchanged.
- The order feed now keeps, per candle and price, the resting side's lots added / pulled / most showing / refills (orders within 64 ticks),
  and per side the biggest single order. Decided numbers are kept (TurnPane\absread-<MKT>-<spb>.txt) so a restart draws the same read.
- Files: RevRead\wy-<MKT>-<spb>.txt gains "stamp|Abs|dir|<pos>|<level px>|<level>" and "stamp|Ex|dir" (frozen with the session);
  RevRead\turnabs-<MKT>-<spb>.txt (lsTurn -> lsRevReadMarks): "stamp|dir|P|plus|exq".
- Nightly: lra.abs_badge 1.0.0 scores every Abs forward from these files (stop just past the level, 1R / 2R vs every RTH candle traded the
  same way; exhaustion detection; Ex? / Ex / P5+ groups; per market, both halves) -> research-artifacts/setups/ABS-BADGE.md. The "?"
  stays in every market until it is WORKS there (20+ a side, +5 points in both halves) and Rassul says yes.

## Tests
- LevelMarks: t_abs118 22 checks (first test, the band, volume / range / close, hour levels by name, the Ex, RTH window, the overlay's
  parse / P line); t_bk17 97, t_major 60, t_ar 94, t_levelmarks 95, t_cx115 15, t_cxlive 9; t_abs_readers 7 (SweepVoice, NowLevels,
  TopSignals, Turn, Volume / Range read the marks file with and without Abs / Ex lines the same); research parity: 178 of the 188 research
  cases fire on the same candle and side (95%), 4 extra.
- lsTurn: t_abs150 31 (book per price, refills, the biggest order, P, Ex?, the read text, the badge file); test_turn 3,287,189; host 44,113
  (real ES feed: the Abs read with the feed, the badge file, a restart draws the same); golden 2017; soak plain + ASan; t_read140 262.
- Real irtsdk.h syntax check C++14 / C++17: lsRevRead, lsRevReadMarks, lsTurn - 0 errors, 0 warnings outside the SDK.

## Not verified - needs his IRT
- The badge on the live price chart (placement with Bk / Sw / Cx / Tst), the read's height in the Turn pane, the order feed's book counts
  live (Rithmic MBO); no Abs appears on NG's P line without its order feed.
