# lsBookFlow 1.35.0 - Wave POC levels in the level box (2026-11-04)

Rassul, 2026-11-04 (trading, IRT open): "add wave poc as one of the levels for bookflow".

## What changed
- The BookFlow level box (BookLevelBox 1.0.0 -> **1.1.0**) now also watches the **Wave POC lines lsWavePOC draws** on his chart.
  Box text: `WPOC 7705.00 | Defenders ... | Behind ... | Attackers ...?`, side by the direction of approach as every other level
  (from above = support, from below = resistance), zone = POC +-4 ticks, the same away / arrival / persistence / squares rules.
- Level type `L_WPOC` = record / status name **`wave_poc`** (the nightly's ltype).
- Nothing else in the box, the squares, the turn marks or the settings changed (no window change, parameter version unchanged).

## Which WPOC levels (= what is drawn on his chart)
Source: lsWavePOC's own record `lsFlexLevels\WavePOC\<MKT>-180-nodes.csv` (append-only; WAVE line + NODE rank 0 = the wave's volume
POC = the cyan line). Nothing is recomputed (BookFlow has no volume-at-price); the rule is `wpc::drawPocLines` (lsWavePOC 1.8.0-1.8.3,
Rassul 2026-11-03 11:14-11:37 CT) line for line, `blb::wpocLines`, on BookFlow's chart CLOSED bars:
- waves of the chart's reversal-bars N (the newest WAVE line's N), confirmed by the last closed bar; an extended leg (same direction
  and start, later end) replaces the earlier wave;
- only waves whose extreme bar is in today's CME trading day (17:00 CT starts the next) - "a line ends with its trading day";
- UNTESTED POCs (no closed bar after the extreme traded through): only the **2 nearest above and the 2 nearest below** the last close;
- TESTED POCs (a bar traded through): kept (the dim line ending at that bar is still on the chart) until a newer wave of the same
  direction exists.
- Not used (documented differences from the screen): the LIVE provisional wave's POC (it is not recorded until IRT's zigzag confirms
  the wave), and its effect of hiding older tested lines; whether the line is inside the visible price range / long enough in px.
- Exact on 1-min and 3-min BookFlow charts; on other timeframes the tested-bar timing is approximate (status says so).

Read cost: the last 3 MiB once, then only the new complete lines every 5 s (a torn last line waits for its newline; a truncated /
rewritten file is read again from its tail); the line set is rebuilt only on a new bar or when the record grew. Waves older than 3 days
are pruned. Status `LEVEL_BOX,1.1.0,...,levels ... wave_poc N,...,WPOC <n> WPOC (<u> untested, <t> tested) from <w> recorded waves
(N 3, 0 bad lines, <r> reads), file ...`.

## Nightly - lra.book_level_combo 1.3.0 (BLC130)
- `LEVEL_TYPES` + `"wave_poc"`, scored separately (table / judge per ltype, as the other types).
- `read_wpoc_records` (= blb::WpFeed), `wpoc_lines` (= blb::wpocLines), `wpoc_windows`: the line set at every 3-min close of the
  session, held until the next close -> one window per level per drawn stretch; `find_touches(..., "wave_poc", t_from, t_to)` inside
  the windows only (from RTH open - 60 min, as the other levels). Coverage `wave_poc` / `wave_poc_src`.
- Days without lsWavePOC's record have no wave_poc rows (ES record from 2026-10-11; GC / HG / NQ / CL / NG records exist).
- `CACHE_V` blc2 -> **blc3**: cached days are replayed again (under the 45-min nightly budget) so they get their wave_poc cases.

## Tests (Linux)
- `tests_bf135/test_bf135_wpoc.cpp` (ASan / UBSan, -Werror) **22 / 22**: PARITY on the real ES 10-29 / 10-30 fixture - the WPOC lines
  built from the record lsWavePOC's engine writes == `wpc::drawPocLines` on that engine, **247 / 247 closed bars identical** (209 bars
  with lines; open and tested both exercised); chunked 37-byte reads = the same; 2 nearest a side; extended leg replaces; no look-ahead
  (counts from the confirmation bar); other N = none; tested = to the bar that traded through, hidden by a newer same-direction wave,
  kept by the other direction; the extreme bar never tests; trading day around 17:00 CT (= wpc::tday); parser (torn / garbled lines,
  rank-0 only); names "WPOC <price>" by tick decimals, zone +-4, one per price; ltype "wave_poc".
- `tests_bf135/test_bf135_host.cpp` (mock SDK, ASan / UBSan) **17 / 17**: no record = no level, no crash; record -> "WPOC 7010.00"
  watched (old trading day's wave never a level); hidden 8 ticks away; arrival from above = support; box "WPOC 7010.00 | Defenders ..."
  one row right of the Defending? line, inside the pane, **nothing drawn over it, segments in order**; levelbox record S line
  `|WPOC 7010.00|wave_poc|support|`; a torn line held back then completed (WPOC 7005.00 added); status counts; restart = same levels;
  truncated file re-read.
- `tests_bf135/parity_wpoc.py` + `dump_wpoc.cpp`: **Python (book_level_combo 1.3.0) == C++** - wpoc_lines 247 / 247 bars, wpoc_windows
  on 10-30 cover exactly the C++ set at 130 / 130 session bars; names identical. Real record ES-180-nodes.csv: 623 waves, 597 with a
  POC, 0 bad lines in both parsers.
- `tests_bf134` (pins moved to 1.35.0 / BookLevelBox 1.1.0): host 32 / 32, blb logic 41 / 41.
- MinGW `-fsyntax-only` against the real irtsdk.h: 0 errors / 0 new warnings outside the SDK (the 3 SDK RT_LONG errors and the 5
  misleading-indentation / pragma warnings in OrderFlowFeed.h / TrapLogic.h / BookFlow.cpp stripTail are pre-existing, identical on 1.34.0).

## Not verified (needs his IRT)
- The look on his chart (wide Samsung monitor) when price reaches a wave POC, 2+ markets.
- That his Wave POC chart is the 3-min one (the record read is `<MKT>-180-nodes.csv`).
- No study yet: wave_poc rows appear in the nightly scorecard from tonight (LEARNING until 20 a side).

## Versions
BF_VERSION = bf::VERSION = ofset::BF_VERSION = setVersion literal = 1.35.0; BookLevelBox 1.1.0; book_level_combo 1.3.0. gex-build.bat
unchanged (BookFlow's line already hashes BookLevelBox.h; no new includes).
