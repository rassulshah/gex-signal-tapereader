// ThrustDot.h 1.0.0 (2026-11-27) -- the cyan THRUST DOT, SDK-free (tests_levelmarks/t_td100). Rassul 2026-11-27 12:38-12:44 CT: "i want to add a blue
// cyan dot on a green candle that has its low below the high of the down pivot and vice versa, that has its thrust and pic greater than or equal to 1.2";
// 12:39 "put the cyan dot in the range bar and in the candle in the middle"; 12:44 "1. yes 2. every 3. all 4. no repaint" (the pivot candle itself counts,
// every candle that passes, overnight and RTH, never repaints). Mockup Dot-mockup.html.
//   Bullish (bearish = mirror): a GREEN candle with Thrust >= 1.20 and PIC >= 1.20 (VolRangeLogic 1.18.0: range / ATR(14), |close - prior close| /
//   the average |close change| of the 14 candles before) whose low is below the HIGH of the down-pivot candle (the 3-bar High / Low ZigZag pivot low the
//   running up-leg started from) - or, while the leg is still DOWN, the candle that is the leg's lowest low (the pivot candle itself).
//   Decided when the candle closes, from the candles up to it only (the ZigZag of that prefix): later candles never change a dot - no repaint.
#pragma once
#include "WyRead.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace tdot {
static const char* const VERSION = "1.0.0";
static const int N = 14; static const double MIN_X = 1.2;
struct Chart { std::vector<long long> T; std::vector<double> O, H, L, C; };   // the chart's closed candles (T ascending, the stamps of B)
// Thrust and PIC of chart candle k over the 14 usable candles before it (an empty candle - all zero - is skipped); false = not enough candles
inline bool thPic(const Chart& X, int k, double& th, double& pic) {
    const int n = (int)X.T.size(); if (k < 1 || k >= n || (int)X.H.size() != n || (int)X.L.size() != n || (int)X.C.size() != n) return false;
    auto dead = [&](int j) { return X.H[(size_t)j] == 0 && X.L[(size_t)j] == 0 && X.C[(size_t)j] == 0; };
    std::vector<int> J; for (int j = k - 1; j >= 1 && (int)J.size() < N + 1; --j) if (!dead(j)) J.push_back(j);
    if ((int)J.size() < N + 1 || dead(k)) return false;
    double tr = 0, dc = 0;
    for (int q = 0; q < N; ++q) { const int j = J[(size_t)q], p = J[(size_t)q + 1]; const double pc = X.C[(size_t)p];
        tr += std::max(X.H[(size_t)j] - X.L[(size_t)j], std::max(std::fabs(X.H[(size_t)j] - pc), std::fabs(X.L[(size_t)j] - pc))); dc += std::fabs(X.C[(size_t)j] - pc); }
    tr /= N; dc /= N; if (!(tr > 0) || !(dc > 0)) return false;
    th = (X.H[(size_t)k] - X.L[(size_t)k]) / tr; pic = std::fabs(X.C[(size_t)k] - X.C[(size_t)J[0]]) / dc;
    return std::isfinite(th) && std::isfinite(pic);
}
// candle stamp -> +1 (bullish dot) / -1 (bearish) for the session candles B (the same candles run() / seq() use); `done` = stamps already decided
// (skipped - their answer never changes), so a host re-run only works on the new candles
inline std::map<long long, int> dots(const std::vector<wyr::Bar>& B, const Chart& X, const std::map<long long, int>* done = nullptr, std::vector<long long>* decided = nullptr) {
    std::map<long long, int> out; const int n = (int)B.size(); if (n < 2) return out;
    for (int i = 1; i < n; ++i) {
        const wyr::Bar& b = B[(size_t)i];
        if (done) { const auto it = done->find(b.t); if (it != done->end()) { if (it->second) out[b.t] = it->second; continue; } }
        if (decided) decided->push_back(b.t);
        const bool g = b.c > b.o, r = b.c < b.o; if (!g && !r) continue;
        const auto kt = std::lower_bound(X.T.begin(), X.T.end(), b.t); if (kt == X.T.end() || *kt != b.t) continue;
        double th = 0, pc = 0; if (!thPic(X, (int)(kt - X.T.begin()), th, pc) || th < MIN_X || pc < MIN_X) continue;
        const std::vector<wyr::Bar> pre(B.begin(), B.begin() + i + 1); const std::vector<wyr::Leg> LG = wyr::legs(pre);
        const int a = LG[(size_t)i].a, d = LG[(size_t)i].dir; if (a < 0 || a > i) continue;
        bool ok = false;
        if (g) { if (d == 1) ok = b.l < B[(size_t)a].h;                                                    // the up-leg from the pivot low a
                 else if (d == -1) { ok = true; for (int j = a; j < i; ++j) if (B[(size_t)j].l < b.l) { ok = false; break; } } }   // still the down-leg: the pivot candle itself (its lowest low)
        else    { if (d == -1) ok = b.h > B[(size_t)a].l;
                 else if (d == 1) { ok = true; for (int j = a; j < i; ++j) if (B[(size_t)j].h > b.h) { ok = false; break; } } }
        if (ok) out[b.t] = g ? 1 : -1;
    }
    return out;
}
}  // namespace tdot
