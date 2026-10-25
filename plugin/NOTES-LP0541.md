# lsLiquidityProfile 0.54.1 - the drawn after-fill word is "RE-BID?" (label only), 2026-10-25

0.54.1 was free (0.54.0 is the last shipped build; no 0.54.1 anywhere in the plugin sources or notes).

## Request
- Sunday 2026-10-25 follow-up to 0.54.0. The live replay on LMT zone orders gave RE-BID 45.8% vs ACCEPTED 40.6% failed sweeps
  (t 0.84, not significant; only HG holds). His rule: unproven reads show with "?".
- Change the drawn word to "RE-BID?"; ACCEPTED unchanged; the record (R lines) and the logic unchanged except the label; the hover says
  "not yet proven on zone orders - judged nightly".

## What changed
- LmtZonesLogic.h: `afterText()` returns "RE-BID?" (drawn word + the hover's Now / Ended row). `afterCode()` (the R record word) still
  "RE-BID". The hover's how row says RE-BID?, plus one row: "RE-BID? = not yet proven on zone orders - judged nightly (zone replay:
  failed 46% vs ACCEPTED 41%)." Worst case 12 rows (PRINT + diamond + read), the Now / Ended row kept.
- Versions: LQ_VERSION = setVersion = lpset::PLUGIN_VERSION = lqp::VERSION = 0.54.1. lmz::VERSION kept 1.3.0 (it is written into every
  Z record line; keeping it keeps the session records byte-identical). Parameter version 10 kept (no settings rows changed; the colour
  rows keep their labels "RE-BID offer / bid colour").

## Tests (Linux, g++ 13, ASan / UBSan)
- tests_lmz/test_lmz.cpp 116 / 116 (-Werror; 114 in 0.54.0 + 2 new: drawn RE-BID? vs record RE-BID / ACCEPTED unchanged; hover row and
  the 12-row worst case).
- tests_lmz/test_lmz_host.cpp 52 / 52 (51 + 1 new: the bare word RE-BID is never drawn); green "RE-BID?" below the bid, R line still
  "|RE-BID|20|3|20|0", restart draws it identically, After-fill / Near boxes hide it, the bid colour applied.
- Regression unchanged: test_lqp044 114 / 114, test_lqp452 27 / 27, test_lqp044_host 21 / 21, test_ap047 55 / 55.
- Replay (0.54.0 vs 0.54.1 engines, same recorded MBO): HG 10-22, NG 10-14, ES 10-22 - stdout, the after-fill CSV and the whole session
  record identical; only the drawn word differs (3 + 0 + 6 RE-BID -> RE-BID?). No repaint and IRT-restart checks pass.
- MinGW against the real irtsdk.h: the same diagnostics as 0.54.0 (11 outside the SDK, none in changed code).

## Not verified - needs his IRT
- The look of "RE-BID?" on the chart (one character wider; the text width is measured, the clash check uses it).
