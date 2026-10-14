// KeyLevelsLogic.h - the key levels, computed natively from the chart's own bars (lsDealerProfile 2.7.0; 2.10.0 Asia + current week).
// (Rassul 2026-10-09 08:40 "it would also need to calculate the PFH/PFL and PWH/PWL ... it would probably be better if the dealer
// profile did all the levels and their probabilities"; 08:41 "those lines dont change once they are drawn till the next day")
// Same definitions as lra.key_levels / lra.level_touch (the nightly studies' reference):
//   PFH / PFL   the previous Globex session (17:00 -> 16:00 CT), all bars
//   PDH / PDL   that session's RTH bars (bar START >= open, END <= close - SessionVWAPLogic's rthOf table)
//   PWH / PWL   the Monday-Friday sessions of the previous week
//   ONH / ONL   this session's overnight bars (17:00 -> the RTH open); shown once RTH has started
//   LonHI/LonLO this session's London bars (END in (02:00, 07:30]); shown after 07:30
//   AsiaHI/LO   (2.10.0) this session's Asia bars (END in (18:00, 02:00] CT, per-market table asiaWindow); shown after 02:00
//               (docs/study-methods/level-definitions-asia-week.md, reviewed 2026-10-14)
//   CWH / CWL   (2.10.0) this week's high / low set BEFORE the prior session: the max / min of this week's completed sessions
//               older than yesterday's; shown only when beyond the prior session's PFH / PFL (so never Monday or Tuesday).
//               Fixed at the session start - never repaints.
// (2.10.0, Rassul 2026-10-14) the hourly swings HrHI / HrLO are no longer computed or drawn here (lra.level_touch still logs them).
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

// (2.10.0) the Asia window per market, CT minutes of the bar END: (start, end]. One window for all markets today (18:00 -> 02:00,
// ends where London starts); kept per market so the nightly studies can tune it.
inline void asiaWindow(const char* m, int& startMin, int& endMin)
{
    startMin = 18 * 60; endMin = 2 * 60;
    (void)m;
}
// (2.10.0) the merge distance for a NEW level (AsiaHI/LO, CWH/CWL) joining an existing line: the Liquidity Profile zone
// half-width minimum in ticks (NearLevels.h nl::zoneFor().hwMin) - ES 4, NQ 8, CL 4, GC 8, HG 2, NG 2, EU 2
inline int mergeTicksFor(const std::string& m)
{
    if (m == "NQ" || m == "GC") return 8;
    if (m == "HG" || m == "NG" || m == "EU") return 2;
    return 4;
}
inline bool isNewCode(const std::string& c) { return c.compare(0, 4, "Asia") == 0 || c.compare(0, 2, "CW") == 0; }

inline std::vector<Level> compute(const std::vector<Bar>& B, int perSec, int openMin, int closeMin, double tick,
                                  int asiaStartMin = 18 * 60, int asiaEndMin = 2 * 60)
{
    std::vector<Level> out;
    if (B.empty() || perSec <= 0 || !(tick > 0)) return out;
    struct S { Agg full, rth, on, lon, asia; };
    std::map<long, S> by;                                         // session (civil days) -> aggregates
    std::vector<long> sessOf(B.size(), 0); std::vector<int> kindOf(B.size(), svl::NONE);
    // (2.7.4) the chart's history may start part-way through a session: a level built from a session (or week) that is only
    // partly loaded would be drawn as if it were the real one. firstSess / firstStart = the first loaded bar's session and start.
    bool haveFirst = false; long firstSess = 0; int firstStart = 0;
    for (size_t i = 0; i < B.size(); i++) {
        const Bar& b = B[i];
        if (!std::isfinite(b.h) || !std::isfinite(b.l) || b.h < b.l) continue;
        svl::Cls c = svl::classify(b.days, b.endSec, perSec, openMin, closeMin);
        sessOf[i] = c.sess; kindOf[i] = c.kind;
        if (c.kind == svl::NONE) continue;
        if (!haveFirst) { haveFirst = true; firstSess = c.sess; firstStart = c.startSec; }
        S& s = by[c.sess];
        s.full.add(b.h, b.l);
        if (c.kind == svl::RTH) s.rth.add(b.h, b.l);
        if (c.kind == svl::ON) {
            s.on.add(b.h, b.l);
            int endMin = b.endSec / 60;
            if (endMin > 2 * 60 && endMin <= 7 * 60 + 30 && b.endSec < 17 * 3600) s.lon.add(b.h, b.l);
            // (2.10.0) Asia: the evening part after asiaStart, and the early-morning part up to asiaEnd (a bar ending 00:00 = 0)
            if ((b.endSec >= 17 * 3600 && endMin > asiaStartMin) || (b.endSec < 17 * 3600 && endMin <= asiaEndMin)) s.asia.add(b.h, b.l);
        }
    }
    if (by.empty()) return out;
    const size_t last = B.size() - 1;
    long cur = sessOf[last];
    if (kindOf[last] == svl::NONE || !by.count(cur)) return out;
    const int lastEndMin = B[last].endSec / 60;
    // a session is complete when it is after the first loaded one, or the first one starts at the 17:00 Globex open (15 min grace)
    const bool firstFromOpen = firstStart >= 17 * 3600 && firstStart <= 17 * 3600 + 15 * 60;
    auto complete = [&](long s) { return s > firstSess || (s == firstSess && firstFromOpen); };
    auto push = [&](const char* code, double p, bool piv) { if (std::isfinite(p) && p > -1e299 && p < 1e299) out.push_back(Level{code, std::round(p / tick) * tick, piv}); };
    // prior session (the latest earlier one with bars)
    auto it = by.find(cur);
    if (it != by.begin() && complete(std::prev(it)->first)) {
        auto p = std::prev(it);
        if (p->second.full.ok()) { push("PFH", p->second.full.h, false); push("PFL", p->second.full.l, false); }
        if (p->second.rth.ok()) { push("PDH", p->second.rth.h, false); push("PDL", p->second.rth.l, false); }
    }
    // prior week: Monday..Friday sessions of the week before this session's week
    {
        int wd = svl::weekdayOf(cur);                            // 0 = Sunday
        long monday = cur - ((wd + 6) % 7);
        Agg wk;
        if (complete(monday - 7))                                // the whole previous week is loaded (from its Monday session)
        for (auto& kv : by) if (kv.first >= monday - 7 && kv.first <= monday - 3 && kv.second.full.ok()) { wk.add(kv.second.full.h, kv.second.full.l); }
        if (wk.ok()) { push("PWH", wk.h, false); push("PWL", wk.l, false); }
    }
    const S& cs = it->second;
    bool rthStarted = kindOf[last] == svl::RTH || kindOf[last] == svl::POST;
    if (rthStarted && cs.on.ok() && complete(cur)) { push("ONH", cs.on.h, false); push("ONL", cs.on.l, false); }
    bool lonDone = rthStarted || (B[last].endSec < 17 * 3600 && lastEndMin > 7 * 60 + 30);
    const bool lonComplete = complete(cur) || (firstSess == cur && firstStart < 2 * 3600);   // loaded from before 02:00
    if (lonDone && cs.lon.ok() && lonComplete) { push("LonHI", cs.lon.h, false); push("LonLO", cs.lon.l, false); }
    // (2.10.0) Asia high / low: once the window is over (after asiaEnd, or RTH started) and the whole window is loaded
    {
        const bool asiaDone = rthStarted || (B[last].endSec < 17 * 3600 && lastEndMin > asiaEndMin);
        const bool asiaComplete = complete(cur) || (firstSess == cur && firstStart >= 17 * 3600 && firstStart <= asiaStartMin * 60);
        if (asiaDone && cs.asia.ok() && asiaComplete) { push("AsiaHI", cs.asia.h, false); push("AsiaLO", cs.asia.l, false); }
    }
    // (2.10.0) current week: the sessions of this week (from Monday's, the Sunday 17:00 open) that are older than the prior session
    if (it != by.begin()) {
        const long prior = std::prev(it)->first;
        const int wd = svl::weekdayOf(cur);
        const long monday = cur - ((wd + 6) % 7);
        if (prior >= monday && complete(monday)) {
            Agg e;
            for (auto& kv : by) if (kv.first >= monday && kv.first < prior && kv.second.full.ok()) e.add(kv.second.full.h, kv.second.full.l);
            const Agg& pf = std::prev(it)->second.full;
            if (e.ok() && pf.ok()) {
                if (e.h > pf.h) push("CWH", e.h, false);
                if (e.l < pf.l) push("CWL", e.l, false);
            }
        }
    }
    return out;
}

// same price to the tick -> one level with the names joined ("PDH/PFH"). (2.10.0) a NEW level (AsiaHI/LO, CWH/CWL) within
// nearTicks of an existing line joins it (the nearest; the line keeps its own price, the new name goes last: "ONH/AsiaHI");
// otherwise it gets its own line. Existing levels still merge only on the exact tick.
struct Merged { std::string name; double price; bool pivotOnly; };
inline std::vector<Merged> merge(const std::vector<Level>& L, double tick, int nearTicks = 0)
{
    std::map<long long, Merged> m;
    auto join = [](Merged& g, const Level& x) {
        std::string& n = g.name;
        if (("/" + n + "/").find("/" + x.code + "/") == std::string::npos) n = n + "/" + x.code;
        g.pivotOnly = g.pivotOnly && x.pivot;
    };
    for (int pass = 0; pass < 2; ++pass)
    for (const Level& x : L) {
        const bool nw = isNewCode(x.code);
        if ((pass == 0) == nw) continue;                          // existing levels first, then the new ones
        long long k = (long long)std::llround(x.price / tick);
        auto f = m.find(k);
        if (f == m.end() && nw && nearTicks > 0) {               // the nearest existing line within nearTicks
            long long best = 0; bool have = false;
            for (auto& kv : m) { long long d = kv.first > k ? kv.first - k : k - kv.first; if (d <= nearTicks && (!have || d < (best > k ? best - k : k - best))) { best = kv.first; have = true; } }
            if (have) f = m.find(best);
        }
        if (f == m.end()) m[k] = Merged{x.code, x.price, x.pivot};
        else join(f->second, x);
    }
    std::vector<Merged> v; for (auto& kv : m) v.push_back(kv.second);
    return v;
}
// (2.10.0, Rassul 2026-10-14 "ONH (dot) AsiaHI") the name on the chart badge: "/" -> " \xB7 " (the middle dot, CP1252 like DayStats);
// the files keep "/" (every parser splits on it)
inline std::string displayName(const std::string& n)
{
    std::string r; r.reserve(n.size() + 8);
    for (char c : n) { if (c == '/') r += " \xB7 "; else r += c; }
    return r;
}

} // namespace klv
