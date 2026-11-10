/********************************************************************************
 *  LimitProfile.cpp  --  Investor/RT RTX extension  lsLimitProfile  "Limit Profile"  2.57.0  (2026-11-09; 2.57.0 = the ENT / EXIT tags follow the nightly params; was 2.53.0 on 2026-11-05)
 *  Rassul 12:06 CT: "lets rename the dealer profile to limit profile" (he removed lsLiquidityProfile: "we have an actual liquidity
 *  profile which is called the dealer profile that has limit orders").
 *  = DealerProfile.cpp built as lsLimitProfile.dll under the new name; the drawing, rules, settings and files are lsDealerProfile's
 *  (same lsFlexLevels\DealerProfile files, so his settings carry over). Use ONE of the two per chart, never both.
 *  gex-build reads the version from this line:  p->setVersion("2.58.0")
 ********************************************************************************/
#define DP_LIMIT_NAME 1
#include "DealerProfile.cpp"
