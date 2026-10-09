// KeyLevelsLogic.h - the key levels, computed natively from the chart's own bars (lsDealerProfile 2.7.0).
// (Rassul 2026-10-09 08:40 "it would also need to calculate the PFH/PFL and PWH/PWL ... it would probably be better if the dealer
// profile did all the levels and their probabilities"; 08:41 "those lines dont change once they are drawn till the next day")
// Same definitions as lra.key_levels / lra.level_touch (the nightly studies' reference):
//   PFH / PFL   the previous Globex session (17:00 -> 16:00 CT), all bars
//   PDH / PDL   that session's RTH bars (bar START >= open, END <= close - SessionVWAPLogic's rthOf table)
//   PWH / PWL   the Monday-Friday sessions of the previous week
//   ONH / ONL   this session's overnight bars (17:00 -> the RTH open); shown once RTH has started
//   LonHI/LonLO this session's London bars (END in (02:00, 07:30]); shown after 07:30
//   HrHI / HrLO the latest confirmed clock-hour swing (2 complete hours each side, strictly higher / lower), not yet traded through
// Pure C++ (no SDK) - tested in test_keylevels_logic.cpp.
#pragma once
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "SessionVWAPLogic.h"

namespace klv {

struct Bar { long days; int endSec; float h, l, c; };          // days = civil days of the bar END (local), endSec = seconds of day
struct Level { std::string code; double price; bool pivot; };

struct Agg { double h = -1e300, l = 1e300; int n = 0; void add(double hi, double lo) { h = std::max(h, hi); l = std::min(l, lo); n++; } bool ok() const { return n > 0; } };

inline std::vector<Level> compute(const std::vector<Bar>& B, int perSec, int openMin, int closeMin, double tick)
{
    std::vector<Level> out;
    if (B.empty() || perSec <= 0 || !(tick > 0)) return out;
    struct S { Agg full, rth, on, lon; };
    std::map<long, S> by;                                         // session (civil days) -> aggregates
    std::vector<long> sessOf(B.size(), 0); std::vector<int> kindOf(B.size(), svl::NONE);
    for (size_t i = 0; i < B.size(); i++) {
        const Bar& b = B[i];
        if (!std::isfinite(b.h) || !std::isfinite(b.l) || b.h < b.l) continue;
        svl::Cls c = svl::classify(b.days, b.endSec, perSec, openMin, closeMin);
        sessOf[i] = c.sess; kindOf[i] = c.kind;
        if (c.kind == svl::NONE) continue;
        S& s = by[c.sess];
        s.full.add(b.h, b.l);
        if (c.kind == svl::RTH) s.rth.add(b.h, b.l);
        if (c.kind == svl::ON) {
            s.on.add(b.h, b.l);
            int endMin = b.endSec / 60;
            if (endMin > 2 * 60 && endMin <= 7 * 60 + 30 && b.endSec < 17 * 3600) s.lon.add(b.h, b.l);
        }
    }
    if (by.empty()) return out;
    const size_t last = B.size() - 1;
    long cur = sessOf[last];
    if (kindOf[last] == svl::NONE || !by.count(cur)) return out;
    const int lastEndMin = B[last].endSec / 60;
    auto push = [&](const char* code, double p, bool piv) { if (std::isfinite(p) && p > -1e299 && p < 1e299) out.push_back(Level{code, std::round(p / tick) * tick, piv}); };
    // prior session (the latest earlier one with bars)
    auto it = by.find(cur);
    if (it != by.begin()) {
        auto p = std::prev(it);
        if (p->second.full.ok()) { push("PFH", p->second.full.h, false); push("PFL", p->second.full.l, false); }
        if (p->second.rth.ok()) { push("PDH", p->second.rth.h, false); push("PDL", p->second.rth.l, false); }
    }
    // prior week: Monday..Friday sessions of the week before this session's week
    {
        int wd = svl::weekdayOf(cur);                            // 0 = Sunday
        long monday = cur - ((wd + 6) % 7);
        Agg wk;
        for (auto& kv : by) if (kv.first >= monday - 7 && kv.first <= monday - 3 && kv.second.full.ok()) { wk.add(kv.second.full.h, kv.second.full.l); }
        if (wk.ok()) { push("PWH", wk.h, false); push("PWL", wk.l, false); }
    }
    const S& cs = it->second;
    bool rthStarted = kindOf[last] == svl::RTH || kindOf[last] == svl::POST;
    if (rthStarted && cs.on.ok()) { push("ONH", cs.on.h, false); push("ONL", cs.on.l, false); }
    bool lonDone = rthStarted || (B[last].endSec < 17 * 3600 && lastEndMin > 7 * 60 + 30);
    if (lonDone && cs.lon.ok()) { push("LonHI", cs.lon.h, false); push("LonLO", cs.lon.l, false); }
    // hourly swings: clock-hour bars from bar START times, the last 3 sessions
    struct H { long key; double h, l; int n; };
    std::vector<H> hrs;
    for (size_t i = 0; i < B.size(); i++) {
        if (kindOf[i] == svl::NONE || sessOf[i] < cur - 4) continue;
        long startAbs = B[i].days * 86400L + B[i].endSec - perSec;
        long key = startAbs >= 0 ? startAbs / 3600 : -((-startAbs + 3599) / 3600);
        if (hrs.empty() || hrs.back().key != key) hrs.push_back(H{key, B[i].h, B[i].l, 0});
        H& x = hrs.back(); x.h = std::max(x.h, (double)B[i].h); x.l = std::min(x.l, (double)B[i].l); x.n++;
    }
    const int full = 3600 / perSec, minBars = std::max(1, (int)std::floor(0.9 * full));
    const long nowHour = (B[last].days * 86400L + B[last].endSec) / 3600;
    double hiP = NAN, loP = NAN;
    for (size_t j = 2; j + 2 < hrs.size(); j++) {
        bool ok = true;
        for (size_t k = j - 2; k <= j + 2; k++) if (hrs[k].n < minBars) ok = false;
        for (size_t k = j - 2; k < j + 2; k++) if (hrs[k + 1].key - hrs[k].key != 1) ok = false;
        if (!ok || hrs[j].key + 3 > nowHour) continue;              // confirmed only once the 2 right-hand hours have CLOSED
        bool hi = true, lo = true;
        for (size_t k = j - 2; k <= j + 2; k++) if (k != j) { if (!(hrs[j].h > hrs[k].h)) hi = false; if (!(hrs[j].l < hrs[k].l)) lo = false; }
        if (hi || lo) {                                           // still standing: not traded through after it was confirmed
            double mxAfter = -1e300, mnAfter = 1e300;
            for (size_t k = j + 1; k < hrs.size(); k++) { mxAfter = std::max(mxAfter, hrs[k].h); mnAfter = std::min(mnAfter, hrs[k].l); }
            if (hi && mxAfter < hrs[j].h) hiP = hrs[j].h;
            if (lo && mnAfter > hrs[j].l) loP = hrs[j].l;
        }
    }
    if (std::isfinite(hiP)) push("HrHI", hiP, true);
    if (std::isfinite(loP)) push("HrLO", loP, true);
    return out;
}

// same price to the tick -> one level with the names joined ("PDH/HrHI"); key levels before pivots in the name
struct Merged { std::string name; double price; bool pivotOnly; };
inline std::vector<Merged> merge(const std::vector<Level>& L, double tick)
{
    std::map<long long, Merged> m;
    for (const Level& x : L) {
        long long k = (long long)std::llround(x.price / tick);
        auto f = m.find(k);
        if (f == m.end()) m[k] = Merged{x.code, x.price, x.pivot};
        else {
            std::string& n = f->second.name;
            if (("/" + n + "/").find("/" + x.code + "/") == std::string::npos) n = n + "/" + x.code;   // compute() emits key levels first, pivots last
            f->second.pivotOnly = f->second.pivotOnly && x.pivot;
        }
    }
    std::vector<Merged> v; for (auto& kv : m) v.push_back(kv.second);
    return v;
}

} // namespace klv
