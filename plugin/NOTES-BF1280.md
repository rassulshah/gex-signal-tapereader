# lsBookFlow 1.28.0 (FMirror.h 1.0.0, LetterStrips 1.4.0) - 2026-10-26, BUILD-SPEC "Defense pane (lsBookFlow)" (Rassul approved 22:29 CT 2026-10-25)

## What changed
- **F? (amber)** - the pivot likely dies at once (FMirror.h; research o4reads f_mirror_skip, PROVEN read: 46% die at once vs 13%).
  At each pivot-bar close (a 5-candle low / high closing in the far half of its bar): front = the attackers' adds - pulls in the
  0.5-ATR band in front of the close, behind = the defenders' pulls - adds in the band behind, over the last 60 s, in floor units
  (ES 10, NQ / CL 5, GC / HG / NG 3, 6E 9), from the engine's classified book events (fills are never pulls). Decided once, written to
  lsFlexLevels\BookFlow\<MKT>-<spb>-fmirror.csv (F lines, read back after a restart). **Drawn only when lsFlexLevels\LRA-FMirror-<MKT>.csv
  sets F_FRONT / F_BEHIND** (the market's top third of prior sessions; SHOW,0 hides it). No file yet = recorded, not drawn
  (status F_MIRROR says "thresholds NOT SET"). F? takes the Release strip on its pivot bar and goes to the Reversal Stage's amber row.
- **Ab (grey)** = the shared A (absorbing). **Hb? (defender colour)** = H, drawn only when an Ab was on the same pivot side within 10
  candles (else recorded, not drawn). **L? / X?** = the re-test letters; their numbers + hover add **same order still there / new big
  order (>= the market's line floor, ES 50) / small orders only (count, biggest)** and the lots vs normal (classified from the book by
  order id at the close before the re-test; R lines in BookFlow\<MKT>-<spb>-retest-ids.csv). P / B kept.
- **Colour by meaning**: green bullish / red bearish - the defenders' reads in their colour (a low = green); W, X, Q! and the bids
  stepping back (B) at a low = red. Ab grey, F? amber. At most 2 codes per candle (one per strip). Records keep the codes A / H / L / X.
- Hovers in his format (Question / Answer / How to use / Example) for F?, L? / X?, Ab.
- Status: F_MIRROR (reads, armed, thresholds, file), RETEST_IDS.

## Tests (Linux mock, ASan/UBSan, -Werror)
- tests_aggbook/test_fmirror.cpp: 15/15 (floors, params, pivot rule, front / behind sums incl. the window edges and fills, armed / not /
  thresholds missing, F line round trip, high mirror, hover format).
- tests_aggbook/test_aggbook_host (BookFlow): 144/144 - Ab grey, Hb? green, X? red, B red, F? amber on the Release strip with
  thresholds, SHOW 0 hides it, the L letter carries the re-test ids, status lines; (BidAskStats) 74/74.
- tests_aggbook/test_aggbook_113 182/182, attack host 44/44, every other tests_aggbook suite unchanged and passing.
- tests_strips 1303 / 119 / 45 / stage box 26 + 18 / pipeline 18. Settings contract 43,863. MinGW real irtsdk.h: 0 errors.

## Open
- F? thresholds per market (F_FRONT / F_BEHIND) must come from the nightly (o4 thresholds, top third of the prior 20 sessions);
  until then F? is recorded but never drawn. Live parity vs the research (ES / NQ "full book") still pending.
