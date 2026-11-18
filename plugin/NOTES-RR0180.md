# lsRevReadMarks / lsRevRead 0.18.0: the I on the candle (2026-11-18)

**Request (Rassul):** 17:06 CT "I also want the I indicator on the candle, put it on top of green candle and below red candles"; 17:11 "go" after the mockup (`i-on-candles.html`, today's ES, 6 real I's).

## What changed
- The Reversal Read Marks indicator (price chart, the one that draws Cx) now draws every **I** of the Reversal Read:
  - **over a green candle** (close >= open), **under a red candle**;
  - a small tag: **green = bullish I**, **red = bearish I**, white letter (a bare "I" in IRT's font reads as a wick);
  - one row further out when a Cx already sits on that side of the candle (never overlaps).
- Source: `lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt`, which the Reversal Read pane writes once per closed candle, so the candle and the pane always agree, and a decided I never changes (no repaint). Looked at every 3 s; re-read only when the file changes; 1 MB cap; a torn or foreign file keeps the last good set.
- The Reversal Read pane must be on the same market and candle size (it writes the file). A / E / Sp / Ut stay in the pane only.
- New SDK-free header `WyMarks.h` 1.0.0 (parse + placement); gex-build line for RevReadMarks lists it. No settings change (parameter version 1 unchanged).
- Versions: RVS_VERSION, setVersion, RevReadSettings PLUGIN_VERSION, RevReadMarks.cpp = 0.18.0.

## Tests
- `tests_wymarks/t_wymarks.cpp` 15/15 under ASan / UBSan / -Werror: parse (I among other letters, CRLF, header only, foreign / torn file refused, bad lines skipped), placement (green over, red under, Cx step, off-pane not drawn), his real ES file of 11-18 (18 I read = 18 lines).
- MinGW syntax against the real irtsdk.h: RevReadMarks.cpp and RevRead.cpp, 0 errors, no new warnings.

## Not verified (needs his IRT)
- After his restart: the tags on ES / NQ in the right place, over / under, no overlap with Cx or the POC dash (ChartView export check).
