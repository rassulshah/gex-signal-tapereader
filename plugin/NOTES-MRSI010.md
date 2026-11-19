# lsMorrisRSI 0.1.0 (2026-11-19) - Morris modified RSI + divergences

Requests (CT, 2026-11-19):
- 11:55 "make a morris modified rsi based on 20 bars, the color is blue but below 30 make it green and above 70 make it red and if there are diversions also .. do you undertand"
- 12:00 screenshot of his IRT RSI settings: RSI Period 20, Smoothing Period 3 (Simple), Price Close, Momentum Period 1, Morris Modified RSI checked, Ref Line 1 70, Ref Line 2 30 - "are you using these values"
- 12:07 "go" (mockup morris-rsi-mockup.html; the 4 open questions unanswered -> the proposed defaults: regular divergences only, Dv in the RSI pane only, a divergence appears when the 2nd pivot is confirmed and never changes, no settings)

What it is
- Own pane, axis 0 - 100 (scale()). RSI: change = close - close 1 candle earlier; AG / AL = simple means over ONLY the last 20 changes (Morris, per the Linnsoft RSI help: "computed using only the last X periods"); RSI = 100 - 100/(1+AG/AL) (AL=0 -> 100, flat -> 50); plotted = 3-candle simple average.
- Colour: blue 30-70, green < 30, red > 70 (each segment by its middle value); dashed 70 (red) / 50 (grey) / 30 (green); title "Morris RSI v0.1.0  (20, 3): <value>" top left (10 pt bold), in the value's colour.
- Divergence (Dv): consecutive 3-bar High / Low ZigZag pivots (wyr::legs rules) at most 30 candles apart; bullish = lower price low + higher RSI low, 1st RSI <= 40; bearish = higher price high + lower RSI high, 1st RSI >= 60; decided at the 2nd pivot's confirmation; line between the RSI points + "Dv" (white bold on green / red, no border), under a bullish point / over a bearish one.
- Records: lsFlexLevels\MorrisRSI\dv-<MKT>-<spb>.txt (MRSI|1|MKT|spb|ver, then t1|t2|dir; atomic writes, strict parse, wrong market rejected); drawn from the record = never repainted. status-<MKT>-<spb>.txt (every 30 s, only on change).
- Continuous over the chart's loaded candles (newest 3,000; no session reset). Forming candle drawn live (liveValue), closed candles fixed.

Tests
- tests_morrisrsi/t_morrisrsi.cpp 24/24 (ASan / UBSan, -Werror): hand calc, Morris window, smoothing, AL = 0 / flat, colours, bullish divergence, 30-candle limit, 40 / 60 zones, replay = one pass + decided divergences never change, live = closed value, record round trip / wrong market / garbled lines, two engines independent, layout (inside the pane, title + version, one Dv tag per divergence, 0-100 axis).
- Parity with the Python reference (mockup script) on ES 11-18 17:00 - 11-19 09:48 research bars: 315 values, 0 differences (max 1.4e-14).
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors, 0 warnings in MorrisRSI.cpp / MorrisRsiLogic.h.

Risks / not verified
- Not seen on his chart yet: needs an IRT restart and the indicator added to a new pane.
- Must be compared with IRT's own RSI (Morris on, 20, smoothing 3 simple) on the same chart: IRT's help page describes the Morris option only in words; a small difference would mean its exact averaging differs.
- No MBO / VAP needed; cost per calc = one RSI per new closed candle.
