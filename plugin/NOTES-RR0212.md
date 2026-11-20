# lsRevRead / lsRevReadMarks 0.21.2 - the I is back (2026-11-20)

Request: Rassul 09:20 CT 2026-11-20 "im thinking of adding I back since it considers levels it should be ok.. add it and also add the mark to the
signals line in reversal indicator, add the badge for it also and include the level and strength line, if both sweep and i are true, only show
sweep"; 09:24 CT "build". Mockup: outputs/i-badge-mockup.html. Built on 0.21.1 (re-staged from the PC, identical to outputs/rr0211, installed 08:20).

## What changed
- RevRead.cpp 0.21.2 (lsRevRead + lsRevReadMarks): `RV_SHOW_I = true` (0.19.5 had set it false). The 0.19.4 rule stays: an I is kept only when it
  reclaimed a NAMED key level (now `lvm::keyForI`, same test as before: no VWAP / bands, HBO line, gamma / MenthorQ names, merged names).
- NEW (no-repaint fix): the I's level list leaves out the level FILE's PHH / PHL (`lvm::levelForI`). The file holds the prior hour as of NOW and the
  live session is recomputed every candle, so an earlier I tied to it would vanish when the hour turned. The prior hours' highs / lows still count -
  WyRead takes them from the candles (same names PHH / PHL / 2hL ...). Bk / Sw are unchanged (they still use the file's PHH / PHL).
- Price chart (lsRevReadMarks, WyMarks 2.4.0): the I badge = line 1 "I", line 2 the ONE short level (LevelMarks 1.7.0 `iPick`: the Bk / Sw priority
  session > PD > PF > volume > weekly > hours, a tie = the furthest reclaimed; WyRead 1.9.0 keeps every reclaimed level in `Out.ilvC`), line 3 the
  strength V<N> / R<N> (BVMax / BRMax, from 10 up). Green UNDER a bullish I, red OVER a bearish I (`sideOf`), black text, the WyMarks layout (no
  overlap with candles, other tags, BO / ZZO / Cob labels). No repeats: an I needs the FIRST break of its level (WyRead 1.5.0 rule).
- Strip: the I is drawn after Sp / Ut: Sp / Ut > I > T > A > X > E > B > C (P / X still not drawn), 2 per candle. The strip's I / B / C now come from
  the PUBLISHED wy lines (`wyr::stripFromLines`), so the strip and the price chart always agree (frozen sessions too).
- Sw + I on the same candle (same direction): only the Sw shows (badge and strip). The I is still written to RevRead\wy-<MKT>-<spb>.txt as
  `stamp|I|dir|lv|px|st|Sw` (7 fields, `wyi::iLine`) for the nightly; every drawing reader skips it.
- wy-file readers checked (tests_rr0212/t_consumers.cpp): lsHodLod Now line (NowLevels Bk / Sw / FT only) and signal line (TopSignals Sw / Tst / Rev
  only), lsVolRead / lsRangeRead (Bk / Sw only), the Sw voice (Sw only), lsBreakout / lsWavePOC (Bk only), lsSDMO (I strip removed in 0.3.0) - none
  shows the I; no reader code changed.
- Nightly: revread_letters 1.5.0 (read_session / initiation take the session's key levels with the same filter + iPick; Python = C++ on the 4
  fixtures, identical I candles and levels); level_setups 1.1.0 adds the I set-up (RTH, entry at the I close, stop 1 tick past the candle, 2R else
  the RTH close, R_scorer 1.5R, same costs; column i_under_sw).

## Effect (fixtures: store OHLC + the pane's V buy / sell + that session's key-level file; Sp / Ut-rejection I's need Book Flow - not in them)
- ES 11-19: 2 I (9:21 PM +2hL, 7:54 AM +PHL V11); ES 11-20 (to 8:36 AM): 6 (5:12 PM +LHI, 9:12 PM +PHL, 11:24 PM +PHL R34, 1:36 AM +ALO V19,
  4:00 AM +2hL R20, 7:15 AM +PHL R88). NQ 11-19: 3 (1:09 AM -AHI, 3:33 AM +PHL R29, 6:27 AM +PHL R99+); NQ 11-20: 0. None in RTH, none under a Sw.
  (times = candle start)
- In-sample RTH I set-up (level_setups 1.1.0 on the store, 2026-08..11-20): ES n 41, mean -0.34R (3 under a Sw); NQ n 30, -0.40R (NQ November store
  bars carry no buy / sell - the delta rule is unreliable there). LEARNING - forward from 11-20.

## Tests
- tests_rr0212/t_rr0212.cpp 59/59 (ASan / UBSan, -Werror): keyForI / levelForI, iPick priority + ties, iUnderSw, iLine 6 / 7 fields, parseAll skips
  the flagged I, stripFromLines, I side (under bullish / over bearish), strip priority, layout (5 tags on 4 candles, no overlap), 4 real sessions:
  every I badge names a key level, never on a same-direction Sw, strip I = badge I; gamma / VWAP levels added at the I prices change nothing; forced
  Sw + I -> only the Sw, the I flagged; replay with a restart after every candle: no I changes / vanishes, each decided at its own close, = one pass.
- tests_rr0212/t_consumers.cpp 11/11: the readers above give identical output with the I lines added.
- tests_levelmarks 67/67, tests_rr0210 45/45, tests_wymarks 42/42, tests_wyfreeze 365/365 (the 0.21.1 "crash" was the missing fixture
  tests_volrange/wy_NQ_1119.txt when run outside the plugin tree - not a code fault).
- pytest test_level_setups.py (+ test_i_setup_chart_rules) + lra/test_revread_letters_none.py: 9 passed.
- MinGW -fsyntax-only vs the real irtsdk.h: RevRead / RevReadMarks no new warnings (7 / 3 as 0.21.1; the 3 RT_LONG errors are irtsdk.h under MinGW).

## Seen while building (not changed)
- 09:30 CT the live wy-ES-180.txt / wy-NQ-180.txt were empty (header only) and wymarks-<MKT>-180-20261120.txt had 0 lines; the RevRead status said
  "MBO: waiting", last closed bar 8:39 AM - IRT looked like it was restarting / reloading. No wymarks-*-20261119.txt exists, so 11-19 is not frozen.
- The live level file has merged names (ES "ONL/PHL"); merged names are not in the priority list, so that ONL is not a level for Bk / Sw / I today.

## Not verified - needs his IRT
The I badges and strip letters on the live chart after he exits IRT and the build installs.
