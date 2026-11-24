# lsTurn 1.4.0 - the Absorption read (2026-11-23)

Rassul 20:47 CT "build the read to the right llike you do for all the other indicators"; 20:48 "go"; 20:48 "i need something more
specific because i already have the ratios on the bars"; 20:52 "I think the first two bullets are good but what else can you add that
would be a bullish or bearish indication or telling me about direciton"; 20:53 "lets go with this".

## What changed
- Three lines right of the last candle (the Book Flow / Volume read format: time white, verdict in colour, then why, then Watch),
  as of the last closed candle in view. Bars and the top strip are unchanged.
- Events = candle sides whose Absorption is coloured (2.0+ and closed outside that end quarter) in the last 20 closed candles.
  Price = the heaviest single price in that end quarter (new: Bars.topMaxTick / botMaxTick), lots = lots at that price.
- Held = no later candle closed beyond the price; tested = a later candle traded to it and closed on the right side;
  failed = a later candle closed beyond it.
- Line 1: above a holding absorbed low = Bullish ?; below a holding absorbed high = Bearish ?; both = Range ? (both prices);
  the newest thing a failure = the other way ("the absorbed low at P failed" = Bearish ?). A named level in that end quarter
  (or the swept level of a sweep candle) is added: "(VAL)", "At the ONL sweep."
- Line 2: lots, x normal, held / tested counts, "Absorbed lows are rising" / "highs are falling" when 2+ holding ones step.
- Line 3: the close that changes the read.
- "?" stays: only the close test at major levels is measured (73% held, 435 cases); held / failed afterwards is not.
- Status file: READ|bullish/bearish/range/quiet|head|why|watch|lines drawn N.
- Prices printed with the decimals of the native tick (5.6380 copper, 1.16805 EU).

## Tests
- tests_turn/t_read140.cpp: 258 checks (price text, quiet, held / tested / failed, range, rising lows, the 20-candle window,
  sweep name, wrapping, heaviest-price tick).
- test_turn.cpp 3,287,189 checks; golden 2017; host test 36,999 checks (read lines right of every bar, inside the pane, no overlap,
  READ line in the status, restart = same picture); soak plain + ASan no growth; real irtsdk.h C++14 / C++17 0 errors.
- Copper replay (effmock on today's HG events): 10:24 AM "Bullish ? - above the absorbed low at 5.6465"; 12:06 PM
  "Bullish ? - the absorbed high at 5.6380 failed".

## Not verified - needs his IRT
- The read on the live chart (room right of the last candle: needs about 150 px; fewer = not drawn).

## 1.4.1 (2026-11-23 22:45 CT, Rassul "fix everything")
- The top line falls back only to a sweep of the CURRENT Globex session (17:00 CT on, turn::sessionOf); with none yet it shows the last
  closed candle's Absorption. Before, the newest sweep of an earlier session ("1hL 1:03 PM ... NO READ") stayed there all evening.
- Tests: t_read140 262 (+ sessionOf), logic 3,287,189, host 36,999, golden 2017, soak; real irtsdk.h C++14 / C++17 clean.
