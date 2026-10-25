// KeyLevelsLogic.h - the key levels, computed natively from the chart's own bars (lsDealerProfile 2.7.0; 2.10.0 Asia + current week).
// (Rassul 2026-10-09 08:40 "it would also need to calculate the PFH/PFL and PWH/PWL ... it would probably be better if the dealer
// profile did all the levels and their probabilities"; 08:41 "those lines dont change once they are drawn till the next day")
// Same definitions as lra.key_levels / lra.level_touch (the nightly studies' reference):
//   PFH / PFL   the previous Globex session (17:00 -> 16:00 CT), all bars
//   PDH / PDL   that session's RTH bars (bar START >= open, END <= close - SessionVWAPLogic's rthOf table)
//   PWH / PWL   the Monday-Friday sessions of the previous week
//   ONH / ONL   this session's overnight bars (17:00 -> the RTH open); shown once RTH has started
//   LonHI/LonLO this session's London bars (END in (02:00, 07:30]); shown after 07:30
//   AsiaHI/LO   (2.10.0) this session's Asia bars (END in (18:00, 01:00] - his IRT session #119 CT, per-market table asiaWindow); shown after 02:00
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

struct Bar { long days; int endSec; float h, l, c; float o = NAN; double v = -1; };   // days = civil days of the bar END (local), endSec = seconds of day
// (DP 2.18.0) o = the bar's open (the RTH Open level), v = its volume (the profile's coverage check); NAN / -1 = not supplied
struct Level { std::string code; double price; bool pivot; };

struct Agg { double h = -1e300, l = 1e300; int n = 0; void add(double hi, double lo) { h = std::max(h, hi); l = std::min(l, lo); n++; } bool ok() const { return n > 0; } };

// (2.10.0) the Asia window per market, CT minutes of the bar END: (start, end]. One window for all markets today (18:00 -> 02:00,
// ends where London starts); kept per market so the nightly studies can tune it.
inline void asiaWindow(const char* m, int& startMin, int& endMin)
{
    startMin = 18 * 60; endMin = 1 * 60;   // (2026-10-24) his IRT Asia session #119 = 18:00-01:00 (was 02:00)
    (void)m;
}
// (2.10.0) the merge distance for a NEW level (AsiaHI/LO, CWH/CWL) joining an existing line: the Liquidity Profile zone
// half-width minimum in ticks (NearLevels.h nl::zoneFor().hwMin) - ES 4, NQ 8, CL 4, GC 8, HG 2, NG 2, EU 2
inline int mergeTicksFor(const std::string& m)
{
    // (2026-10-24, Rassul 21:30 "option 1") a new level (AsiaHI/LO, CWH/CWL) merges ONLY on the exact same tick, like every other
    // level: within-4-ticks merging made "LonHI/AsiaHI" show London's 7712.25 for an Asia high of 7711.75. Close tags stack instead.
    (void)m; return 0;
}
inline bool isNewCode(const std::string& c) { return c.compare(0, 4, "Asia") == 0 || c.compare(0, 2, "CW") == 0; }

// ---- (DP 2.18.0, Rassul 2026-10-19 13:14 / 13:25 / 13:33) Open + the volume-profile levels -------------------------------
// Open        today's RTH open: the open of this session's first RTH bar, drawn from the RTH open on (white)
// DPoc        the prior FULL session's (17:00 -> 16:00 CT) volume POC - his ProfPrevDay (IRT Profile, Volume, POC (Prev), daily)
// VAH / VAL   that same profile's 70% value area high / low
// WPoc        the PRIOR week's (Monday..Friday sessions) volume POC - fixed all week (his ProfPrevWeekly showed this week's developing
//             POC; the name and the no-repaint rule say the prior week)
// Rows are the chart's volume-at-price (IRT barVolumeProfile) summed per session; rowTicks groups rows (a row's price = its lowest
// tick). POC = the biggest row; a tie goes to the row nearest the profile's middle, then the lower one. Value area: start at the
// POC, add the bigger of the next two rows above / the next two below (the CBOT rule; equal -> both) until >= vaPct of the volume.
// A profile is used only when its sessions are fully loaded and the volume-at-price covers >= 98% of the bars' volume.
struct Vap { double price; double vol; };
struct Prof { double poc = NAN, vah = NAN, val = NAN, vol = 0; bool ok = false; };
inline Prof profileOf(const std::map<long long, double>& ticks, double tick, int rowTicks = 1, double vaPct = 0.70)
{
    Prof P; if (ticks.empty() || !(tick > 0)) return P;
    if (rowTicks < 1) rowTicks = 1;
    std::map<long long, double> rows;                               // row key (lowest tick of the row) -> volume
    for (const auto& kv : ticks) {
        if (!(kv.second > 0)) continue;
        long long k = kv.first; long long r = k >= 0 ? (k / rowTicks) * rowTicks : -(((-k) + rowTicks - 1) / rowTicks) * rowTicks;
        rows[r] += kv.second;
    }
    if (rows.empty()) return P;
    std::vector<long long> key; std::vector<double> vol;
    for (const auto& kv : rows) { key.push_back(kv.first); vol.push_back(kv.second); P.vol += kv.second; }
    const double mid = 0.5 * ((double)key.front() + (double)key.back());
    size_t pi = 0;
    for (size_t i = 1; i < vol.size(); ++i) {
        if (vol[i] > vol[pi] + 1e-9) pi = i;
        else if (std::fabs(vol[i] - vol[pi]) <= 1e-9 && std::fabs((double)key[i] - mid) < std::fabs((double)key[pi] - mid) - 1e-9) pi = i;
    }
    size_t lo = pi, hi = pi; double cum = vol[pi]; const double need = vaPct * P.vol;
    while (cum + 1e-9 < need && (lo > 0 || hi + 1 < vol.size())) {
        double up = 0, dn = 0; size_t nu = 0, nd = 0;
        for (size_t j = hi + 1; j < vol.size() && nu < 2; ++j, ++nu) up += vol[j];
        for (size_t j = lo; j > 0 && nd < 2; --j, ++nd) dn += vol[j - 1];
        if (nu == 0) { lo -= nd; cum += dn; continue; }
        if (nd == 0) { hi += nu; cum += up; continue; }
        if (up > dn + 1e-9) { hi += nu; cum += up; }
        else if (dn > up + 1e-9) { lo -= nd; cum += dn; }
        else { hi += nu; lo -= nd; cum += up + dn; }
    }
    P.poc = (double)key[pi] * tick; P.vah = (double)key[hi] * tick; P.val = (double)key[lo] * tick; P.ok = true;
    return P;
}
// what the profile levels were built from - for the status file (verification against IRT's own Profile)
// (2.18.0) the profile row size in ticks per market (IRT's Profile 'Price/Row' Automatic); 1 until checked against his DPoc / WPoc
// lines on every market (the status file's PROFILE line shows the POC for 1 / 2 / 4-tick rows next to the one used)
inline int profileRowTicks(const std::string& m) { (void)m; return 1; }
struct ProfDiag { double dayCover = 0, weekCover = 0; double dpoc1 = NAN, dpoc2 = NAN, dpoc4 = NAN, wpoc1 = NAN, wpoc4 = NAN; bool dayUsed = false, weekUsed = false; };

inline std::vector<Level> compute(const std::vector<Bar>& B, int perSec, int openMin, int closeMin, double tick,
                                  int asiaStartMin = 18 * 60, int asiaEndMin = 1 * 60,
                                  const std::vector<std::vector<Vap>>* vap = nullptr, int rowTicks = 1, ProfDiag* diag = nullptr)
{
    std::vector<Level> out;
    if (B.empty() || perSec <= 0 || !(tick > 0)) return out;
    struct S { Agg full, rth, on, lon, asia; double open = NAN; std::map<long long, double> prof; double barVol = 0, vapVol = 0; int barsNoVol = 0; };
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
        if (c.kind == svl::RTH) { if (!s.rth.ok() && std::isfinite(b.o)) s.open = b.o; s.rth.add(b.h, b.l); }
        if (vap && i < vap->size()) {                             // (2.18.0) the session's volume profile
            if (b.v >= 0) s.barVol += b.v; else s.barsNoVol++;
            for (const Vap& r : (*vap)[i]) if (std::isfinite(r.price) && r.vol > 0) { s.prof[(long long)std::llround(r.price / tick)] += r.vol; s.vapVol += r.vol; }
        }
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
        // (2.18.0) DPoc / VAH / VAL: the prior full session's volume profile
        if (vap) {
            const S& ps = p->second;
            const double cover = ps.barVol > 0 ? ps.vapVol / ps.barVol : 0;
            if (diag) { diag->dayCover = cover; diag->dpoc1 = profileOf(ps.prof, tick, 1).poc; diag->dpoc2 = profileOf(ps.prof, tick, 2).poc; diag->dpoc4 = profileOf(ps.prof, tick, 4).poc; }
            if (cover >= 0.98 && ps.barsNoVol == 0) {
                const Prof P = profileOf(ps.prof, tick, rowTicks);
                if (P.ok) { push("DPoc", P.poc, false); push("VAH", P.vah, false); push("VAL", P.val, false); if (diag) diag->dayUsed = true; }
            }
        }
    }
    // prior week: Monday..Friday sessions of the week before this session's week
    {
        int wd = svl::weekdayOf(cur);                            // 0 = Sunday
        long monday = cur - ((wd + 6) % 7);
        Agg wk;
        if (complete(monday - 7))                                // the whole previous week is loaded (from its Monday session)
        for (auto& kv : by) if (kv.first >= monday - 7 && kv.first <= monday - 3 && kv.second.full.ok()) { wk.add(kv.second.full.h, kv.second.full.l); }
        if (wk.ok()) { push("PWH", wk.h, false); push("PWL", wk.l, false); }
        // (2.18.0) WPoc: the prior week's volume POC (the same sessions as PWH / PWL)
        if (vap && wk.ok()) {
            std::map<long long, double> wp; double bv = 0, vv = 0; int nov = 0;
            for (auto& kv : by) if (kv.first >= monday - 7 && kv.first <= monday - 3 && kv.second.full.ok()) {
                for (const auto& r : kv.second.prof) wp[r.first] += r.second;
                bv += kv.second.barVol; vv += kv.second.vapVol; nov += kv.second.barsNoVol;
            }
            const double cover = bv > 0 ? vv / bv : 0;
            if (diag) { diag->weekCover = cover; diag->wpoc1 = profileOf(wp, tick, 1).poc; diag->wpoc4 = profileOf(wp, tick, 4).poc; }
            if (cover >= 0.98 && nov == 0) { const Prof P = profileOf(wp, tick, rowTicks); if (P.ok) { push("WPoc", P.poc, false); if (diag) diag->weekUsed = true; } }
        }
    }
    const S& cs = it->second;
    bool rthStarted = kindOf[last] == svl::RTH || kindOf[last] == svl::POST;
    if (rthStarted && cs.on.ok() && complete(cur)) { push("ONH", cs.on.h, false); push("ONL", cs.on.l, false); }
    if (rthStarted && std::isfinite(cs.open)) push("Open", cs.open, false);   // (2.18.0) today's RTH open, from the open on
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
