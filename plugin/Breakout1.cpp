/********************************************************************************
 *  Breakout1.cpp  --  Investor/RT RTX extension  lsBreakout1  "BO 1-bar"  1.6.0 (built as 1.14.0: + Pboc, Pdoc, fact kind E from lsBreakout only)  (2026-11-04)
 *  Rassul 21:25 CT "there needs to be a 1bar option for breakout indicator .. this will help when i put it on higher timeframes"; 21:31 "go".
 *  = Breakout.cpp built with BKO_ONE_BAR: the BO / BD pivots, the waves, Rly and the Pb pivots come from the 1-bar High / Low ZigZag
 *  instead of the 3-bar one; every other rule is lsBreakout 1.6.0's (follow-through incl. the equal-close tie, the zigzag carried across
 *  the 17:00 CT break, Pb within 1/3 ATR of the broken pivot, no BI). Records: lsFlexLevels\Breakout1 (zz_rev_bars 1).
 *  gex-build reads the version from this line:  p->setVersion("1.20.0")
 ********************************************************************************/
#define BKO_ONE_BAR 1
#include "Breakout.cpp"
