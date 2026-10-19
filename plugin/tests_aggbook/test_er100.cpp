// EffortReward.h 1.0.0
#include "EffortReward.h"
#include <cstdio>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; printf("FAIL %s\n", m); } } while (0)
int main() {
    double x = 0;
    std::vector<long long> pr(20, 500);
    CHECK(er::effortX(1000, pr, x) && std::fabs(x - 2.0) < 1e-9, "effort 1000 vs 500 avg = 2.0x");
    CHECK(!er::effortX(1000, std::vector<long long>(4, 500), x), "fewer than 5 priors: n/a");
    std::vector<long long> pr30(30, 100); for (int k = 10; k < 30; ++k) pr30[k] = 400;
    CHECK(er::effortX(800, pr30, x) && std::fabs(x - 2.0) < 1e-9, "only the last 20 priors count");
    // ES: sellers traded 2,000 lots, low 2.75 under the open (11 ticks) -> 5.5 t / 1k lots / 5.73 = 0.96x
    CHECK(er::rewardX(100, 101, 97.25, 100, 0.25, 2000, 0, "ES", x) && std::fabs(x - (11.0 / 2.0 / 5.73)) < 1e-9, "sellers' reward from max(open, prior close) to the low");
    CHECK(er::rewardX(100, 101, 97.25, 100.5, 0.25, 2000, 0, "ES", x) && std::fabs(x - (13.0 / 2.0 / 5.73)) < 1e-9, "a gap: from the prior close");
    CHECK(er::rewardX(100, 103, 99, 100, 0.25, 1000, 1, "ES", x) && std::fabs(x - (12.0 / 5.73)) < 1e-9, "buyers' reward to the high");
    CHECK(!er::rewardX(100, 101, 99, 100, 0.25, 0, 0, "ES", x), "no lots: n/a");
    CHECK(!er::rewardX(100, 101, 99, 100, 0.25, 10, 0, "ZZ", x), "unknown market: n/a");
    CHECK(er::rewardCell(true, 0.3) == "0.3x absorbed" && er::rewardCell(true, 2.4) == "2.4x through" && er::rewardCell(true, 1.0) == "1.0x" && er::rewardCell(false, 0) == "n/a", "reward cells");
    er::Side s[2]; s[0].effOk = s[0].rewOk = true; s[0].eff = 2.4; s[0].rew = 0.3;
    br::Read r;
    CHECK(er::read(true, false, s, r) && r.impl == br::BULL && r.head.find("absorbed") != std::string::npos && r.head.find("2.4x") != std::string::npos, "new low, sellers 2.4x, 0.3x: absorbed, bullish ?");
    s[0].rew = 2.5; CHECK(er::read(true, false, s, r) && r.impl == br::BEAR && r.head.find("don't fade") != std::string::npos, "new low, going through: bearish, don't fade");
    s[0].rew = 1.0; CHECK(!er::read(true, false, s, r), "normal reward: no ER read");
    s[1].effOk = s[1].rewOk = true; s[1].eff = 3.0; s[1].rew = 0.2; CHECK(er::read(false, true, s, r) && r.impl == br::BEAR, "new high, buyers absorbed: bearish");
    CHECK(!er::read(true, true, s, r) && !er::read(false, false, s, r), "outside bar / no extreme: none");
    {   er::Side g; g.effOk = true; g.eff = 0.3; g.rewOk = true; g.rew = 20.5; er::guard(g);
        CHECK(!g.rewOk && g.thin && er::rewardCellOf(g) == "-", "(1.1.0) 14 lots overnight (effort 0.3x): Reward \"-\", not 20.5x");
        er::Side h; h.effOk = true; h.eff = 0.5; h.rewOk = true; h.rew = 0.3; er::guard(h);
        CHECK(h.rewOk && er::rewardCellOf(h) == "0.3x absorbed", "(1.1.0) effort 0.5x: Reward shown");
        er::Side k; k.effOk = false; k.rewOk = true; k.rew = 1.0; er::guard(k); CHECK(!k.rewOk, "(1.1.0) effort unknown: Reward hidden");
        er::Side s2[2]; s2[0] = g; br::Read r2; CHECK(!er::read(true, false, s2, r2), "(1.1.0) a thin side gives no read"); }
    printf("EffortReward 1.1.0: %d checks, %d failed\n", p + f, f); return f ? 1 : 0;
}
