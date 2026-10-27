#include "../DeltaProfileCore.h"
#include <cstdio>
namespace dp = delta_profile;
int fails = 0;
#define CK(c) do { if (!(c)) { std::printf("FAIL %s\n", #c); ++fails; } } while (0)
int main() {
    // body from y=100 (top, max(o,c)) to y=140 (bottom, min(o,c)); half = 6
    CK(dp::outsideBodyY(120, 100, 140, 6, true) == 147);   // bullish node inside the body -> below it
    CK(dp::outsideBodyY(138, 100, 140, 6, true) == 147);   // touching the bottom edge -> below
    CK(dp::outsideBodyY(160, 100, 140, 6, true) == 160);   // already below the body (lower wick) -> own place
    CK(dp::outsideBodyY(120, 100, 140, 6, false) == 93);   // bearish inside -> above
    CK(dp::outsideBodyY(80, 100, 140, 6, false) == 80);    // already above -> own place
    CK(dp::outsideBodyY(120, 120, 120, 6, true) == 127);   // doji body
    CK(dp::outsideBodyY(120, 120, 120, 6, false) == 113);
    std::printf(fails ? "FAILED %d\n" : "ALL PASS body placement\n", fails); return fails != 0;
}
