# lsRevRead / lsRevReadMarks 0.21.0 - new letters on the Reversal Read strip (2026-11-20)

Request: Rassul 05:52 CT 2026-11-20 "ok.. you decide and add it. you can also add for example b for break and c for climax etc."
Files: RevRead.cpp (+ RevReadMarks.cpp version line), WyRead.h 1.8.0. LevelMarks.h / WyMarks.h / WyFreeze.h unchanged.

## What changed
- The strip (top of the "Delta + change" pane) now also draws:
  - **B** = a candle the price chart marks Bk (a break of a named key level), green up / red down - read from the same wy file lines the
    price chart draws (frozen sessions included), so the two always agree.
  - **C** = a candle the price chart marks Cx (climax), green at a pivot low / red at a pivot high.
  - **T** = trapped traders (ES, NQ, CL only - measured): a new 10-candle low where sellers hit 1.5x+ their 10-candle mean with negative
    delta, then within 3 candles (no lower low first) a candle closes above that candle's high with positive delta. T on the reclaim
    candle, green. Mirror at highs (red).
- Computed every candle and journaled, NOT drawn (did not beat their base): **P** (defenders pulled 2x+ while hit 1.2x+ at the 10-candle
  low / high) and **X** (exhaustion: attackers fell to 0.7x of the prior 3 candles' peak at a new extreme while the defenders pulled 1.0x
  or less). Journal: `lsFlexLevels\RevRead\wyx-<MKT>-<spb>.txt` ("stamp|T/P/X|dir|shown"). Not written to wy-<MKT>-<spb>.txt, so
  lsRevReadMarks / lsSDMO / lsBreakout / lsHodLod / lsVolRead / lsWavePOC / the nightly revread_letters see no new codes.
- At most 2 letters per candle, priority Sp/Ut > (I) > T > P > A > X > E > B > C (wyr::stripPick). Same colours (green bullish / red
  bearish) and sizes (9 pt alone, 7 pt side by side). No "?", no numbers, no hovers. The read on the right also explains a drawn T.
- Each letter is decided at its candle's close from closed candles only (T needs buy / sell volume, P / X the Book Flow record).

## The study (MBO store 3-min bars, 2026-09-10 .. 11-18; ES 49, NQ 49, CL 43 sessions; first half of the days vs second half)
Outcome = the extreme on the turn side holds 10 candles after the decision candle (P: the tested extreme breaks within 10). Base = the
same situation without the letter. 2R = entry at the close, stop 1 tick beyond the extreme, 2R target, 20 candles.

| Letter | Mkt | n h1 / h2 | Letter h1 / h2 | Base h1 / h2 | Avg R h1 / h2: letter vs base | Verdict |
|---|---|---|---|---|---|---|
| T vs the same reclaim without the trap | ES | 156 / 159 | 59.6 / 59.7% | 52.6 / 56.0% | -0.11 / 0.00 vs -0.10 / +0.04 | DRAW |
| | NQ | 199 / 161 | 63.3 / 60.2% | 51.6 / 53.0% | +0.05 / -0.02 vs -0.07 / 0.00 | DRAW |
| | CL | 181 / 177 | 57.5 / 63.3% | 54.2 / 53.7% | -0.13 / -0.07 vs -0.09 / 0.00 | DRAW |
| P (break) vs hit without pulling | ES | 412 / 362 | 74.5 / 76.2% | 80.6 / 81.8% | | hold back |
| | NQ | 317 / 259 | 76.7 / 73.8% | 82.0 / 84.0% | | hold back |
| | CL | 373 / 375 | 72.1 / 73.6% | 81.6 / 79.7% | | hold back |
| X vs any fresh extreme (same age) | ES | 273 / 327 | 23.1 / 17.1% | 20.6 / 18.7% | | hold back |
| | NQ | 410 / 456 | 15.9 / 20.0% | 17.9 / 17.8% | | hold back |
| | CL | 352 / 321 | 14.5 / 19.9% | 19.5 / 21.0% | | hold back |

T marks a low / high that holds more often in all 3 markets and both halves, but its 2R trade from the reclaim close is no better than
base (the stop sits far below): it is a context letter ("the low is more likely done"), not an entry. P contradicts the liquidity
study's 78% vs 54% on this definition (bar-level pulls near price, 1-tick break) - logged for the nightly to refine. GC / HG / 6E / NG
not measured -> T not drawn there yet.

Existing letters on the same data (same-age base; context only - the nightly report covers them as I context):
A ES 24.6 / 20.0% vs 20.6 / 18.7%, NQ 18.9 / 21.3% vs 17.9 / 17.8%, CL 15.6 / 19.1% vs 19.5 / 21.0%;
E ES 21.6 / 23.3% vs 20.6 / 18.7%, NQ 22.7 / 17.3% vs 17.9 / 17.8%, CL 19.8 / 24.1% vs 19.5 / 21.0%;
Sp (decided 2 candles later) ES 60.0 / 58.2% vs 45.6 / 46.8%, NQ 59.8 / 49.3% vs 44.6 / 44.3%, CL 47.8 / 61.7% vs 46.0 / 46.7%;
Ut ES 58.8 / 63.3% vs 46.5 / 47.4%, NQ 54.2 / 47.4% vs 44.4 / 44.3%, CL 57.8 / 55.7% vs 45.6 / 46.2%.
Study code: outputs/rr0210/study (drv.cpp runs the shipped WyRead.h; score.py; tctl.py). Limits: no key levels in the store bars
(hour levels only for Sp / Ut), store pulls = contracts pulled near price vs the plugin's pullC (both read as x their own mean).

## Tests (all pass)
- tests_rr0210/t_rr0210.cpp (ASan / UBSan, -Werror): 45 checks - T / P / X rules and each failing condition, mirrors, T without book,
  P / X only with a Book Flow record, stripPick priority / cap 2 / dedupe / dir kept (colours), showMore table; bar-by-bar replay with a
  restart before every bar on real ES 11-18 (MBO store, 460 candles) and real ES 11-19 (his chart OHLC + the pane's V buy / sell + 193
  Book Flow records): 0 mismatches, 0 back-dated letters.
- run() (A / E / Sp / Ut / I) byte-identical to WyRead 1.7.1 on 49 ES sessions (2,493 letters + reads).
- tests_levelmarks 59/59, tests_wymarks 42/42, tests_wyfreeze 365/365 with the new header.
- MinGW -fsyntax-only vs the real irtsdk.h (RevRead + RevReadMarks): 0 errors / 0 new warnings outside the SDK.

## Not verified - needs his IRT
The strip on the live chart after he exits IRT and the build installs; the wyx journal being written.
