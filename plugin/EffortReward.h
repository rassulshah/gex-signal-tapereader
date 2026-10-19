// EffortReward.h 1.0.0 (2026-10-19, Rassul "now what would the opposite for aggressiveness" -> "so both together can be an effort reward
// indicator" -> "build it already" 17:42 CT) - ATTACK EFFORT and REWARD, shared by Aggression (lsBidAskStats) and Book Flow (lsBookFlow).
//   Attack Effort = the lots a side traded aggressively in the candle / its average over the 20 candles before (sellers = lots hit at
//                   the bid, buyers = lots lifted at the offer). 2.0x = twice its normal attack.
//   Reward        = how far price went the attacker's way per 1,000 lots it traded (ticks), / that market's normal (1.0x = normal).
//                   Sellers' way = max(open, prior close) - low; buyers' = high - min(open, prior close).
//                   <= 0.5x = ABSORBED (much effort, little result), >= 2.0x = GOING THROUGH (little in the way).
// Evidence (unseen data: 49 sessions, 7 markets x 7 days, 1,285 swing pivots; a turn = a swing pivot within 2 candles before .. 1 after,
// at a new 10-bar low / high; base 22%): Attack 2.0x + absorbed turned 43% (lows) / 50% (highs), 7 / 7 markets; absorbed alone 36% /
// 37%; going through 13% / 13%; Attack 2.0x alone 28% / 30%. Shown with "?" until forward-tested.
// Normals (discovery median, ticks per 1,000 lots): ES 5.73, NQ 78.0, CL 50.6, GC 112.5, HG 150, NG 24.1, 6E 14.8.
// Pure C++ (no SDK) - tested in tests_aggbook/test_er100.cpp.
#pragma once
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "BoxRead.h"

namespace er {

static const double EFFORT_HI = 2.0, REWARD_LO = 0.5, REWARD_HI = 2.0;
static const int EFFORT_BARS = 20, EFFORT_MIN = 5;

inline double rewardNorm(const std::string& m) {
    static const struct { const char* m; double v; } T[] = { {"ES", 5.73}, {"NQ", 78.0}, {"CL", 50.6}, {"GC", 112.5}, {"HG", 150.0}, {"NG", 24.1}, {"EU", 14.8}, {"6E", 14.8} };
    for (size_t k = 0; k < sizeof(T) / sizeof(T[0]); ++k) if (m == T[k].m) return T[k].v;
    return 0;
}
// the attack effort: tot / the mean of the (up to 20) prior candles' totals, oldest first or any order; needs EFFORT_MIN priors
inline bool effortX(long long tot, const std::vector<long long>& prior, double& x) {
    x = 0; if (tot < 0) return false;
    size_t n = 0; double sum = 0;
    for (size_t k = prior.size() > (size_t)EFFORT_BARS ? prior.size() - EFFORT_BARS : 0; k < prior.size(); ++k) if (prior[k] >= 0) { sum += (double)prior[k]; ++n; }
    if (n < (size_t)EFFORT_MIN || !(sum > 0)) return false;
    x = (double)tot / (sum / (double)n); return std::isfinite(x);
}
// side 0 = sellers (attack down), 1 = buyers (attack up)
inline bool rewardX(double o, double h, double l, double pc, double tick, long long tot, int side, const std::string& mkt, double& x) {
    x = 0; const double nrm = rewardNorm(mkt);
    if (!(tick > 0) || !(nrm > 0) || tot <= 0 || !std::isfinite(o) || !std::isfinite(h) || !std::isfinite(l)) return false;
    if (!std::isfinite(pc)) pc = o;
    const double mv = side == 0 ? std::max(0.0, std::max(o, pc) - l) : std::max(0.0, h - std::min(o, pc));
    x = (mv / tick) / (double)tot * 1000.0 / nrm; return std::isfinite(x);
}
inline std::string x1(double v) { return br::x1(v) + "x"; }
inline std::string effortCell(bool ok, double x) { return ok ? x1(x) : std::string("n/a"); }
inline std::string rewardCell(bool ok, double x) {
    if (!ok) return "n/a";
    if (x <= REWARD_LO + 1e-9) return x1(x) + " absorbed";
    if (x >= REWARD_HI - 1e-9) return x1(x) + " through";
    return x1(x);
}
// the Read at a new 10-bar low (sellers attacking) / high (buyers attacking); false = nothing to say (the pane's own read stays)
struct Side { bool effOk = false, rewOk = false; double eff = 0, rew = 0; };
inline bool read(bool newLow, bool newHigh, const Side s[2], br::Read& r) {
    if (newLow == newHigh) return false;                                   // neither, or an outside bar: no side to judge
    const int a = newLow ? 0 : 1;                                          // the attacker into the extreme
    const Side& x = s[a];
    if (!x.rewOk) return false;
    const std::string who = a == 0 ? "Sellers" : "Buyers", where = a == 0 ? "New low" : "New high";
    r = br::Read();
    if (x.effOk && x.eff >= EFFORT_HI - 1e-9 && x.rew <= REWARD_LO + 1e-9) {
        r.impl = a == 0 ? br::BULL : br::BEAR;
        r.head = where + ": " + who + " attacked " + x1(x.eff) + " but moved price only " + x1(x.rew) + " normal - absorbed, turn odds up ?";
        return true;
    }
    if (x.rew >= REWARD_HI - 1e-9) {
        r.impl = a == 0 ? br::BEAR : br::BULL;
        r.head = where + ": " + who + " going through (" + x1(x.rew) + " normal) - don't fade ?";
        return true;
    }
    if (x.rew <= REWARD_LO + 1e-9) {
        r.impl = a == 0 ? br::BULL : br::BEAR;
        r.head = where + ": " + who + "' push absorbed (" + x1(x.rew) + " normal) ?";
        return true;
    }
    return false;
}

} // namespace er
