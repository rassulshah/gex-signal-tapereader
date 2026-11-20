/********************************************************************************
 *  LimitProfile.cpp  --  Investor/RT RTX extension  lsLimitProfile  "Limit Profile"  2.69.0  (2026-11-20; 2.69.0 = brighter 2 px key level lines; 2.68.0 = what the liquidity on each level line did (LevelLiq.h: building / leaving / X / H / P recorded + published; only H drawn, lines end at a break); 2.67.0 = ONE KING in the Liquidity column (the biggest real level of either side, "K 8 1m") + its mark L (left before price reached it) / P (pulled while hit), 1 minute; ranks 2-6 removed (Rassul 05:05 CT "no, just 1 king.. we can have the mark as well"); 2.66.2 = the 3-hour S / R line is lsCobSignal's alone (2.66.1 drew a second one); 2.66.0 = the standing 3-hour R1 / R2 / S1 / S2 in the level list; 2.65.1 = level rows reach 1 ATR either side of the level; 2.65.0 = level liquidity rows + dark lot text; 2.64.0 = performance fix (the status files were replaced on every draw); 2.63.0 = reads the order feed itself (LimitFeed.h); 2.62.0 = lsDealerProfile retired, header "LP v"; 2.61.0 = the level-line stretches are recorded for the nightly; 2.60.0 = liquidity on the level lines; 2.59.0 = the node profile: nodes facing price, labels inside, 1-min updates; 2.57.0 = the ENT / EXIT tags follow the nightly params; was 2.53.0 on 2026-11-05)
 *  Rassul 12:06 CT: "lets rename the dealer profile to limit profile" (he removed lsLiquidityProfile: "we have an actual liquidity
 *  profile which is called the dealer profile that has limit orders").
 *  = DealerProfile.cpp built as lsLimitProfile.dll under the new name; the drawing, rules, settings and files are lsDealerProfile's
 *  (same lsFlexLevels\DealerProfile files, so his settings carry over). Use ONE of the two per chart, never both.
 *  gex-build reads the version from this line:  p->setVersion("2.69.0")
 ********************************************************************************/
#define DP_LIMIT_NAME 1
#include "DealerProfile.cpp"
