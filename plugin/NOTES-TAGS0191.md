# Candle-tag coordination - lsCobSignal 1.9.2, lsZzoComboMarks 3.0.4, lsRevRead / lsRevReadMarks 0.19.1, SignalBoard.h 1.6.0 (2026-11-19)

Rassul 10:51 CT "remember placement of other signals like cmx so you have to coordinate everything"; 11:13 CT "ok" (next step: ZZO / CoB).

What changed (nothing that is drawn moves; signal logic untouched):
- SignalBoard.h 1.6.0: kinds 'Z' (lsZzoComboMarks badges) and 'K' (lsCobSignal candle labels) with their own keys (LRA_SGZ_ /
  LRA_SGK_ - unknown kinds used to share F's key); sb::tagEv (label box -> bar, side, px from the extreme to the outer edge, width) and
  sb::tagBox (the reverse).
- lsCobSignal 1.9.2: every Cob / Cob+R / +V / +C, Lvp and Rev label actually drawn is posted as 'K' each draw (bar = the candle under the
  label, CT wall seconds; side; val; width). Fib / 3-hour line labels are not candle tags and are not posted.
- lsZzoComboMarks 3.0.4: every SR / CB35 / CB99 badge drawn is posted as 'Z' each draw (an empty list when none).
- lsRevReadMarks 0.19.1: the taken boxes now come from 'L' (BO indicator), 'Z' and 'K'; with a known width the box is that wide (was a
  fixed 28 px). Its Bk / Sw / Cx / I tags go around all of them.
- gex-build.bat: SignalBoard.h / BannerBoard.h added to the CobSignal, ZzoCombo and ZzoComboMarks lines.

Tests: tests_tagboard/t_tagboard.cpp 15/15 (ASan/UBSan -Werror): own keys, tagEv / tagBox both sides and the unknown-width case, Z / K
round trip without overwriting each other, 60 s freshness, other symbol, and wyi::layout placing a Bk tag off a CB badge.
MinGW -fsyntax-only against the real irtsdk.h: CobSignal.cpp, ZzoCombo.cpp (pane + marks), RevRead.cpp, RevReadMarks.cpp - 0 errors.

Not coordinated / limits:
- lsBreakout and lsBreakout1 both post 'L' to the same key; if both are on one chart the later draw wins (pre-existing).
- ZZO Cmx / AR / ST / Ab wave tags are not drawn (SHOW_WAVE_TAGS false), so nothing to post.
- The other indicators' labels do not avoid the new marks (they draw first and never move); only lsRevReadMarks steps around.
- Not verified on his chart - needs an IRT restart (File -> Exit, wait for "installed", reopen).
