/********************************************************************************
 *  LimitProfile.cpp  --  Investor/RT RTX extension  lsLimitProfile  "Limit Profile"  2.65.0  (2026-11-11; 2.65.0 = level liquidity rows + dark lot text; 2.64.0 = performance fix (the status files were replaced on every draw); 2.63.0 = reads the order feed itself (LimitFeed.h); 2.62.0 = lsDealerProfile retired, header "LP v"; 2.61.0 = the level-line stretches are recorded for the nightly; 2.60.0 = liquidity on the level lines; 2.59.0 = the node profile: nodes facing price, labels inside, 1-min updates; 2.57.0 = the ENT / EXIT tags follow the nightly params; was 2.53.0 on 2026-11-05)
 *  Rassul 12:06 CT: "lets rename the dealer profile to limit profile" (he removed lsLiquidityProfile: "we have an actual liquidity
 *  profile which is called the dealer profile that has limit orders").
 *  = DealerProfile.cpp built as lsLimitProfile.dll under the new name; the drawing, rules, settings and files are lsDealerProfile's
 *  (same lsFlexLevels\DealerProfile files, so his settings carry over). Use ONE of the two per chart, never both.
 *  gex-build reads the version from this line:  p->setVersion("2.65.0")
 ********************************************************************************/
#define DP_LIMIT_NAME 1
#include "DealerProfile.cpp"
