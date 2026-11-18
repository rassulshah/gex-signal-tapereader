# lsRevRead / lsRevReadMarks 0.17.0 - the Wyckoff strip (2026-11-18)

Rassul 05:45-08:15 CT: "show the list of ideas ... lets try to work through them", "filter based on a wyckoff perspective",
"lets add the letters on a strip on the top of the pane", "remove Ts", "springs are false breakdowns and upthrusts are false
breakouts ... use prior highs and lows and then see the orderflow on those attempted breaks", "use Sp and Ut", "get rid of the ? marks",
"dont put anything ont he candle chart , no lines", "lack of followthrough ... strong rejection with strength (I)", "see if there was a
stop run", "ok.. lets build this". Mockup: wyckoff-turn-chart.html (ES 11-17/18 overnight).

## What changed
- WyRead.h 1.0.0 (new, SDK-free): E (delta momentum fade / shortening thrust), A (effort without result), Sp / Ut (false break of the
  recent swing / range extreme, reclaimed within 3 candles, no follow-through, absorbed or flush, stop-run line), I (the other side's
  delta 2x+ and a close through the prior candle after a leg extreme, or the strong rejection after Sp / Ut).
- RevRead.cpp 0.17.0: a letter strip at the top of the delta half (up to 3 rows, fewer in a low pane; 11 pt bold, green / red); the read
  on the right shows the Wyckoff step for a lettered candle (the hovered one, or the newest letter of the last 3 candles).
  Nothing on the candle chart. Rebuilt only when a candle closes or a Book Flow record arrives.
- RevReadSettings.h / RevReadMarks.cpp: version 0.17.0 only (no settings change).
- tools/gex-build.next.bat: WyRead.h hashed by both plugins.

## Tests
- tests_revread/t_wyread_100.cpp on his ES overnight (es_wy1118.psv): Sp 12:03 (flush, stop run 72 lots / 5 prices), Sp 1:39 and
  3:03 (absorbed), I 1:42 / 3:06, A 2:00 / 3:45 / 5:18, E into 3:45 and 5:18; candle-by-candle replay: no letter ever changes or goes.
  17 / 17 pass, ASan / UBSan, -Werror. t_plainread_100 19 / 19, t_combread_100 20 / 20 unchanged.
- MinGW syntax check of RevRead.cpp / RevReadMarks.cpp against the real irtsdk.h: no new errors or warnings.

## Risks / not verified
- Not seen on his IRT yet: strip position and height in his pane (rows drop to 1 in a low pane). Check after the restart.
- Live delta comes from IRT's volume at price (the test used the Book Flow hits) - small differences in the ratios.
- The older pane letters C / A / E / I on the Book Flow bars are unchanged - same letters, older rules (ask Rassul whether to drop them).
- Cut-offs from one ES night: the nightly lra.wy_turns scores them per market.
