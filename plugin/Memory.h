#ifndef LS_MEMORY_H
#define LS_MEMORY_H
// Memory.h 0.1.0 (2026-10-26) -- awareness_plus NEXT item 2, MEMORY BEYOND TODAY: multi-day levels, yesterday's trapped zones and the
// overnight / London story, carried into the session story with a DECAY. SDK-free, unit-tested on Linux.
//
//   lsFlexLevels\LRA-Memory-<MKT>.csv   (written nightly by lra.awareness_plus from the recorded CTX / EXP lines; missing -> no memory, '?')
//     VERSION,1
//     MARKET,<MKT>
//     HALF,<days>                                   half-life of a memory's weight, learned per market (default 2 days)
//     LVL,<kind>,<price>,<yyyy-mm-dd>,<w0>,<note>   kind: PDH PDL (prior RTH high / low), ONH ONL (overnight), LONH LONL (London),
//                                                   TRAP (a price where a read's extreme was taken: trapped traders), HOLD (a held bid / offer)
//     STORY,<yyyy-mm-dd>,<key>,<value>              e.g. on_dir up, on_range_x 1.4, rth_close_pos 0.8, broken_n 12
//   weight(today) = w0 x 0.5 ^ (age_days / HALF); a memory under 0.1 is forgotten. Age counts calendar days from the memory's day.
//   live, per closed bar t (data <= t only): the nearest remembered level above and below the close (kind, price, weight, distance in
//   ticks and ATR), today's overnight high / low / direction from the chart's own bars before the RTH open, and yesterday's story.
// Everything here is DATA (prices remembered) or METRIC (distances, weights) - never a READ; memory gives no side by itself.
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include "MarketProfile.h"

namespace apmem {

static const char* const VERSION = "0.1.0";

struct Level { std::string kind, day, note; double px = 0, w0 = 1; };
struct Mem {
    std::string market, note = "no memory file";
    bool loaded = false; double half = 2.0;
    std::vector<Level> levels;
    std::map<std::string, std::map<std::string, std::string>> story;   // day -> key -> value
};

inline long daysFrom(const std::string& ymd) {   // civil date -> days since 1970-01-01 (Hinnant); -1 on a bad date
    if (ymd.size() != 10 || ymd[4] != '-' || ymd[7] != '-') return -1;
    int y = std::atoi(ymd.substr(0, 4).c_str()), m = std::atoi(ymd.substr(5, 2).c_str()), d = std::atoi(ymd.substr(8, 2).c_str());
    if (m < 1 || m > 12 || d < 1 || d > 31) return -1;
    y -= m <= 2; const long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

inline Mem parse(const std::string& text, const std::string& market) {
    Mem M; M.market = market; if (text.empty()) return M;
    bool v1 = false, mk = false; int bad = 0;
    size_t a = 0;
    while (a < text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        const std::string line = text.substr(a, e - a); a = e + 1;
        if (line.empty() || line[0] == '#') continue;
        const auto f = apm::splitCsv(line);
        if (f[0] == "VERSION") v1 = f.size() > 1 && f[1] == "1";
        else if (f[0] == "MARKET") mk = f.size() > 1 && f[1] == market;
        else if (f[0] == "HALF" && f.size() >= 2) { double h; if (apm::toD(f[1], h) && h > 0.1 && h < 60) M.half = h; else ++bad; }
        else if (f[0] == "LVL" && f.size() >= 5) { Level L; L.kind = f[1]; L.day = f[3]; L.note = f.size() > 5 ? f[5] : ""; if (apm::toD(f[2], L.px) && apm::toD(f[4], L.w0) && daysFrom(L.day) >= 0) M.levels.push_back(L); else ++bad; }
        else if (f[0] == "STORY" && f.size() >= 4) M.story[f[1]][f[2]] = f[3];
        else ++bad;
    }
    if (!v1 || !mk) { Mem D; D.market = market; D.note = !v1 ? "memory file has no VERSION,1" : "memory file is for another market"; return D; }
    M.loaded = true; char b[96]; std::snprintf(b, sizeof(b), "file (%d levels, %d story days, half %.1f d, %d bad)", (int)M.levels.size(), (int)M.story.size(), M.half, bad); M.note = b;
    return M;
}

inline double weight(const Mem& M, const Level& L, long todayDays) {
    const long age = todayDays - daysFrom(L.day);
    if (age < 0) return 0;                                // a memory from the future (bad clock): ignore
    return L.w0 * std::pow(0.5, (double)age / M.half);
}

struct Near { bool ok = false; Level lv; double w = 0, distTicks = 0, distAtr = -1; };

// the nearest remembered level above (dir +1) / below (dir -1) the price, weight >= 0.1
inline Near nearest(const Mem& M, double px, int dir, long todayDays, double tick, double atr) {
    Near N; double best = 1e300;
    for (const auto& L : M.levels) {
        const double w = weight(M, L, todayDays); if (w < 0.1) continue;
        const double d = (L.px - px) * dir; if (d < 0) continue;
        if (d < best) { best = d; N.ok = true; N.lv = L; N.w = w; }
    }
    if (N.ok) { N.distTicks = tick > 0 ? std::round(best / tick) : -1; N.distAtr = atr > 0 ? best / atr : -1; }
    return N;
}

// today's overnight from the chart's closed bars (minOfDay CT): from 17:00 the day before to the RTH open; London 02:00 -> RTH open
struct Overnight { bool ok = false; double hi = 0, lo = 0, open = 0, last = 0, lonHi = 0, lonLo = 0; bool lonOk = false; };
template <class BarT>
inline Overnight overnight(const std::vector<BarT>& bars, int rthOpenMin) {
    Overnight O; if (bars.empty()) return O;
    // walk back from the newest bar while still inside today's globex night (stop at the 16:00-17:00 break)
    const BarT& last = bars.back();
    if (last.minOfDay >= 16 * 60 && last.minOfDay < 17 * 60) return O;
    long long dayStart = -1;
    for (size_t i = bars.size(); i-- > 0;) { const int m = bars[i].minOfDay; if (m >= 16 * 60 && m <= 17 * 60) { dayStart = (long long)i + 1; break; } if (i == 0) dayStart = 0; }
    if (dayStart < 0 || dayStart >= (long long)bars.size()) return O;
    bool any = false;
    for (size_t i = (size_t)dayStart; i < bars.size(); ++i) {
        const BarT& b = bars[i]; const int m = b.minOfDay;
        const bool night = m >= 17 * 60 || m < rthOpenMin; if (!night) continue;
        if (!any) { O.hi = b.h; O.lo = b.l; O.open = b.o; any = true; }
        O.hi = std::max(O.hi, b.h); O.lo = std::min(O.lo, b.l); O.last = b.c;
        if (m >= 120 && m < rthOpenMin) { if (!O.lonOk) { O.lonHi = b.h; O.lonLo = b.l; O.lonOk = true; } O.lonHi = std::max(O.lonHi, b.h); O.lonLo = std::min(O.lonLo, b.l); }
    }
    // only a COMPLETE night (we must have reached 17:00 of the prior day: the first bar of the walk is right after the break)
    O.ok = any && dayStart > 0;
    return O;
}

inline std::string num(double x, int p = 6) { char b[32]; std::snprintf(b, sizeof(b), "%.*g", p, x); return b; }

// the CTX fields (class-tagged) for bar t
template <class BarT>
inline void fold(std::map<std::string, std::string>& kv, const Mem& M, const std::vector<BarT>& bars, long todayDays, double tick, double atr, int rthOpenMin,
                 const std::string& today) {
    if (bars.empty()) return;
    const double px = bars.back().c;
    const Near up = nearest(M, px, +1, todayDays, tick, atr), dn = nearest(M, px, -1, todayDays, tick, atr);
    auto put = [&](const char* side, const Near& N) {
        const std::string k = std::string("mem_") + side;
        if (!N.ok) { kv["D." + k] = "?"; kv["M." + k + "_w"] = "?"; kv["M." + k + "_atr"] = "?"; return; }
        kv["D." + k] = N.lv.kind + "@" + num(N.lv.px) + "(" + N.lv.day + ")";
        kv["M." + k + "_w"] = num(N.w, 3); kv["M." + k + "_atr"] = N.distAtr < 0 ? "?" : num(N.distAtr, 3);
    };
    put("above", up); put("below", dn);
    const Near* nz = nullptr; if (up.ok && (!dn.ok || up.distTicks <= dn.distTicks)) nz = &up; else if (dn.ok) nz = &dn;
    kv["M.mem_near"] = (nz && nz->distAtr >= 0 && nz->distAtr <= 0.5) ? nz->lv.kind : "-";   // within half an ATR of a remembered level
    int live = 0; for (const auto& L : M.levels) if (weight(M, L, todayDays) >= 0.1) ++live;
    kv["M.mem_levels"] = M.loaded ? std::to_string(live) : "?";
    const Overnight O = overnight(bars, rthOpenMin);
    if (O.ok) {
        kv["D.on_high"] = num(O.hi); kv["D.on_low"] = num(O.lo);
        kv["M.on_dir"] = O.last > O.open ? "up" : O.last < O.open ? "down" : "flat";
        kv["M.on_pos"] = O.hi > O.lo ? num((px - O.lo) / (O.hi - O.lo), 3) : "?";            // where price is inside the overnight range
        if (O.lonOk) { kv["D.lon_high"] = num(O.lonHi); kv["D.lon_low"] = num(O.lonLo); } else { kv["D.lon_high"] = "?"; kv["D.lon_low"] = "?"; }
    } else { kv["D.on_high"] = "?"; kv["D.on_low"] = "?"; kv["M.on_dir"] = "?"; kv["M.on_pos"] = "?"; kv["D.lon_high"] = "?"; kv["D.lon_low"] = "?"; }
    auto st = M.story.end();                               // the latest story day before today (the prior session)
    for (auto it = M.story.begin(); it != M.story.end(); ++it) if (it->first < today) st = it;
    kv["M.mem_story"] = st == M.story.end() ? "?" : [&]() { std::string s; for (const auto& p : st->second) { if (!s.empty()) s += " "; s += p.first + ":" + p.second; } return s; }();
}

}  // namespace apmem
#endif
