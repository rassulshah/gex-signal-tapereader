// EffortReward.h 1.5.0 (1.5.0, 20:48 CT: Abs needs the close against the side; "lost the candle" read) 1.4.0 (1.4.0, 19:31 CT: Reward cells "Abs" / "Rwd") 1.3.0 (1.3.0, 19:25 CT: lotsText, effortRead) 1.2.0 (1.2.0, 18:54 CT: counterRead - support / resistance coming in at 1.5x) 1.1.0 (1.1.0, 18:37 CT: Reward hidden ("-") when that side attacked under 0.5x its normal) 1.0.0 (2026-10-19, Rassul "now what would the opposite for aggressiveness" -> "so both together can be an effort reward
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
// (1.1.0, Rassul 18:37 CT, overnight: 14 lots showed "20.5x through") Reward only when that side's Attack Effort is at least 0.5x its normal
// (a few lots moving price a couple of ticks is not a result); otherwise "-" and no Effort vs Reward read
static const double REWARD_MIN_EFFORT = 0.5;
struct Side;
inline void guard(Side& s);

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
// (1.5.0, Rassul 20:33-20:48 "since the close is green, sellers didn't win" -> data: Abs only when the side pushed little AND the candle
// closed against it - unseen sessions, 7 markets, Effort 2.0x+ at a new 10-bar extreme: 49% (lows, n 76) / 51% (highs, n 63) turned vs
// 22% base; pushed little alone 43% / 50%; pushed well (> 0.5x) but closed against 31% / 21% (about base); pure open-to-close 34% / 38%)
inline std::string rewardCell(bool ok, double x, bool lost = true) {
    if (!ok) return "n/a";
    if (x <= REWARD_LO + 1e-9) return lost ? x1(x) + " Abs" : x1(x);     // (1.4.0, Rassul 19:31) absorbed
    if (x >= REWARD_HI - 1e-9) return x1(x) + " Rwd";     // (1.4.0) rewarded (going through)
    return x1(x);
}
// the Read at a new 10-bar low (sellers attacking) / high (buyers attacking); false = nothing to say (the pane's own read stays)
struct Side { bool effOk = false, rewOk = false; double eff = 0, rew = 0; bool thin = false; bool lost = false; };   // lost (1.5.0) = the candle closed against this side (sellers: green close, buyers: red)   // thin = Reward hidden: too few lots (1.1.0)
inline void guard(Side& s) { if (s.rewOk && (!s.effOk || s.eff < REWARD_MIN_EFFORT - 1e-9)) { s.rewOk = false; s.thin = true; } }
inline std::string rewardCellOf(const Side& s) { return s.thin ? std::string("-") : rewardCell(s.rewOk, s.rew, s.lost); }
inline bool read(bool newLow, bool newHigh, const Side s[2], br::Read& r) {
    if (newLow == newHigh) return false;                                   // neither, or an outside bar: no side to judge
    const int a = newLow ? 0 : 1;                                          // the attacker into the extreme
    const Side& x = s[a];
    if (!x.rewOk) return false;
    const std::string who = a == 0 ? "Sellers" : "Buyers", where = a == 0 ? "New low" : "New high";
    r = br::Read();
    if (x.lost && x.effOk && x.eff >= EFFORT_HI - 1e-9 && x.rew <= REWARD_LO + 1e-9) {
        r.impl = a == 0 ? br::BULL : br::BEAR;
        r.head = where + ": " + who + " attacked " + x1(x.eff) + " but moved price only " + x1(x.rew) + " normal - absorbed, turn odds up ?";
        return true;
    }
    if (x.lost && x.rew > REWARD_LO + 1e-9) {                             // (1.5.0) pushed well, then lost the close: about base odds - say so, no side
        r.impl = br::NEUTRAL;
        r.head = where + ": " + who + " pushed " + x1(x.rew) + " but closed " + (a == 0 ? "green" : "red") + " - lost the candle ?";
        return true;
    }
    if (x.rew >= REWARD_HI - 1e-9) {
        r.impl = a == 0 ? br::BEAR : br::BULL;
        r.head = where + ": " + who + " going through (" + x1(x.rew) + " normal) - don't fade ?";
        return true;
    }
    if (x.lost && x.rew <= REWARD_LO + 1e-9) {
        r.impl = a == 0 ? br::BULL : br::BEAR;
        r.head = where + ": " + who + "' push absorbed (" + x1(x.rew) + " normal) ?";
        return true;
    }
    return false;
}

// (1.3.0, Rassul 19:25 "go with your recommendation") the Aggression bars and Lots row = each side's TOTAL aggressive lots; the plain read
// from Attack Effort (was the biggest lots at one price): one side 2.0x+ = "Sellers attacking 2.4x", both = a fight, else quiet
inline std::string lotsText(long long v) { std::string s = std::to_string(v < 0 ? 0 : v); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ","); return s; }
inline br::Read effortRead(const Side s[2]) {
    br::Read r;
    const bool hit[2] = { s[0].effOk && s[0].eff >= EFFORT_HI - 1e-9, s[1].effOk && s[1].eff >= EFFORT_HI - 1e-9 };
    if (!s[0].effOk && !s[1].effOk) { r.head = "Not enough candles yet to judge the attack"; return r; }
    if (hit[0] && hit[1]) { r.head = "Both sides attacking (" + x1(s[0].eff) + " / " + x1(s[1].eff) + ") - a fight; watch the close and Book Flow"; return r; }
    if (!hit[0] && !hit[1]) { r.head = "Quiet - no side attacking"; return r; }
    const int a = hit[0] ? 0 : 1;
    r.impl = a == 0 ? br::BEAR : br::BULL;
    r.head = std::string(a == 0 ? "Sellers attacking " : "Buyers attacking ") + x1(s[a].eff);
    if (s[a].rewOk) r.opt.push_back(", moving price " + x1(s[a].rew) + " normal");
    return r;
}
// (1.2.0, Rassul 2026-10-19 18:48 "it usually waits for 2x but those are more rare so what should it be for a reversal to see that support
// is coming in" -> "do it" 18:54) the COUNTER-SIDE read: at a new 10-bar low, buyers' Attack Effort >= 1.5x = support coming in; at a new
// high, sellers' >= 1.5x = resistance coming in. Unseen data (49 sessions, 7 markets): 1.5x+ turned 30% (lows) / 31% (highs) vs 22% base,
// 2.0x+ 33% / 35%, under 1.25x 17-18% (worse than base). Used when the attacker's Effort vs Reward read has nothing to say.
static const double COUNTER_X = 1.5, COUNTER_STRONG_X = 2.0;
inline bool counterRead(bool newLow, bool newHigh, const Side s[2], br::Read& r) {
    if (newLow == newHigh) return false;
    const int c = newLow ? 1 : 0;                                          // the side coming in against the move
    if (!s[c].effOk || s[c].eff < COUNTER_X - 1e-9) return false;
    const bool strong = s[c].eff >= COUNTER_STRONG_X - 1e-9;
    r = br::Read(); r.impl = newLow ? br::BULL : br::BEAR;
    r.head = std::string(newLow ? "New low: Buyers " : "New high: Sellers ") + (strong ? (newLow ? "lifting " : "hitting ") : "stepping in ") + x1(s[c].eff)
           + (newLow ? " - " : " - ") + (strong ? "strong " : "") + (newLow ? "support" : "resistance") + " coming in ?";
    return true;
}

} // namespace er
