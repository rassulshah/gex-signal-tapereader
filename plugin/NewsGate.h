#ifndef LS_NEWSGATE_H
#define LS_NEWSGATE_H
// NewsGate.h 0.1.0 (2026-10-26) -- awareness_plus item 4: the shared context knows the scheduled release windows per market and
// detects SHOCKS (bar range / pace far above the slot normal). SDK-free, unit-tested on Linux.
//   window   from the market profile's NEWS rows: dated rows ("2026-10-28,0730,CPI,5,15") or weekday rules ("wd3,0930,EIA_CRUDE,5,30");
//            state pre (release - before .. release), in (release .. release + after), off
//   shock    the last CLOSED bar's range >= shock_rng_x x the slot's median range AND volume >= shock_vol_x x the slot's median volume
//            (normals are for 3-min bars; other bar sizes scale range by sqrt(spb/180) and volume by spb/180); a shock stays 'hot'
//            for shock_hot_min minutes (default 15) after the shock bar
//   effect   inside a window or a hot shock: conf multiplier = news_conf_x (per market, default 0.5), every read gets the 'news' flag,
//            the nightly excludes / separates those minutes from learning. Unknown slot normal -> shock '?' (never 'no').
// News is context, never a reason (news-gate rule): nothing here gives a side.
#include <cmath>
#include <map>
#include <string>
#include <cstdio>
#include "MarketProfile.h"

namespace apn {

static const char* const VERSION = "0.1.0";

struct Clock { int y = 0, mo = 0, d = 0, wd = 0, minOfDay = 0; };   // CT; wd 0 = Sunday

inline std::string dateStr(const Clock& c) { char b[16]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d", c.y, c.mo, c.d); return b; }
// weekday from a civil date (Sakamoto), 0 = Sunday
inline int weekday(int y, int m, int d) { static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }; if (m < 3) y -= 1; return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; }

struct Window { std::string state = "off", event = "-"; int minsTo = 9999, minsSince = 9999; };

inline Window window(const apm::Profile& P, const Clock& c) {
    Window W; const std::string ds = dateStr(c), wds = "wd" + std::to_string(c.wd);
    int bestRank = 0, bestAbs = 1 << 30;           // rank 2 = in, 1 = pre
    for (const auto& r : P.news) {
        if (r.when != ds && r.when != wds) continue;
        // a dated row for the SAME event today replaces its weekday rule (holiday shift)
        if (r.when == wds) { bool shifted = false; for (const auto& q : P.news) if (q.when == ds && q.event == r.event) shifted = true; if (shifted) continue; }
        const int rel = (r.hhmm / 100) * 60 + r.hhmm % 100, dm = c.minOfDay - rel;
        int rank = 0;
        if (dm >= -r.before && dm < 0) rank = 1; else if (dm >= 0 && dm <= r.after) rank = 2;
        if (rank == 0) { if (dm < 0 && -dm < W.minsTo) W.minsTo = -dm; continue; }
        if (rank > bestRank || (rank == bestRank && std::abs(dm) < bestAbs)) { bestRank = rank; bestAbs = std::abs(dm); W.event = r.event; W.minsSince = rank == 2 ? dm : 9999; if (rank == 1) W.minsTo = -dm; }
    }
    if (bestRank == 2) { W.state = "in"; W.minsTo = 9999; } else if (bestRank == 1) W.state = "pre";
    return W;
}

struct Shock { std::string state = "?"; double rngX = -1, volX = -1; };   // state: yes / no / ? (no normal)

inline Shock shock(const apm::Profile& P, int minOfDay, double rngTicks, double vol, long spb) {
    Shock S; const apm::Slot* s = P.slotFor(minOfDay);
    if (!s || !(s->rngMed > 0) || !(s->volMed > 0) || spb <= 0) return S;
    const double f = (double)spb / 180.0, rn = s->rngMed * std::sqrt(f), vn = s->volMed * f;
    S.rngX = rngTicks / rn; S.volX = vol >= 0 ? vol / vn : -1;
    const bool big = S.rngX >= P.get("shock_rng_x", 3.0), fast = S.volX >= P.get("shock_vol_x", 2.5);
    S.state = (big && fast) ? "yes" : "no";
    return S;
}

// the gate one chart keeps: windows by the clock, shocks by the bars, with the hot period
struct Gate {
    long long shockBar = -1;     // bar id (CT seconds) of the last shock bar
    std::string shockWhy;
    std::map<std::string, std::string> kv;   // the last decision (posted as NEWS facts and folded into CTX)
    double confX = 1.0; bool news = false;

    // call once per closed bar t (CT seconds) with that bar's clock, range (ticks) and volume (-1 unknown)
    void onBar(const apm::Profile& P, const Clock& c, long long t, double rngTicks, double vol, long spb) {
        const Window W = window(P, c);
        const Shock S = shock(P, c.minOfDay, rngTicks, vol, spb);
        if (S.state == "yes") { shockBar = t; char b[64]; std::snprintf(b, sizeof(b), "range %.1fx, volume %.1fx slot normal", S.rngX, S.volX); shockWhy = b; }
        const int hot = (int)P.get("shock_hot_min", 15);
        const bool shockHot = shockBar >= 0 && t >= shockBar && (t - shockBar) <= (long long)hot * 60;
        news = W.state != "off" || shockHot;
        confX = news ? P.get("news_conf_x", 0.5) : 1.0;
        kv.clear();
        kv["class"] = "METRIC"; kv["window"] = W.state; kv["event"] = W.event;
        kv["mins_to"] = W.minsTo >= 9999 ? "?" : std::to_string(W.minsTo); kv["mins_since"] = W.minsSince >= 9999 ? "?" : std::to_string(W.minsSince);
        kv["shock"] = S.state; kv["shock_hot"] = shockHot ? "1" : "0"; kv["shock_why"] = shockHot ? shockWhy : "-";
        char b[24]; std::snprintf(b, sizeof(b), "%.2f", S.rngX); kv["rng_x"] = S.rngX < 0 ? "?" : b;
        std::snprintf(b, sizeof(b), "%.2f", S.volX); kv["vol_x"] = S.volX < 0 ? "?" : b;
        std::snprintf(b, sizeof(b), "%.2f", confX); kv["conf_x"] = b;
        kv["news"] = news ? "1" : "0";
    }
};

}  // namespace apn
#endif
