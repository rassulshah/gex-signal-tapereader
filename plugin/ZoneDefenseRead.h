// ZoneDefenseRead.h  --  lsZoneDefense 1.0.1 (5 numbered Read lines, aligned columns): the journal model and the 5 Read lines (SDK-free; the pane, the price-chart marks
// overlay and the tests all draw from THIS model, built only from decided journal lines -> nothing is ever repainted, a restart
// reloads the same picture).
// Journal lines (lsFlexLevels\ZoneDefense\<MKT>-<spb>-<session>.csv, append-only; t = feed ms):
//   H,<tick>,<version>                                     header
//   ZN,id,T,side,lo,hi,size,dens,oldestMin,period,bigSz,bigPx   a zone (decided at a minute close)
//   ZT,id,t,atTouch,cancel60,cnorm,n60,pulling            the touch + the queue read (60 s before it)
//   ZR,id,t,verdict,sold,refills,atTouch,left,reloadX,zeroT,brokeT   the reload decision (touch + 60 s): 0 exhausted, 1 quiet, 2 holding
//   ZS,id,t,verdict,runT,runLots,runExt,lift,backIn,rebid,liftLots  the sweep decision (run end + 5 s): 1 weak, 2 swept
//   ZB,id,candleKey,sold,left                             a candle closed: sold into the zone in it, resting left
//   ZX,id,t,state[,bars]                                  (1.0.2) the zone state at a 3-min close: 1 testing, 2 held, 3 reclaimed, 4 broken,
//                                                         5 broken -> reclaimed (bars since the break)
//   ZE,id,t,code,resting                                  the zone ended: 1 broken (confirmed), 2 faded, 3 expired / unresolved, 4 session end, 5 stopped
#pragma once
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace zdr {
struct ZoneView {
    int id = 0, side = 0, per = 0; long long T = 0, lo = 0, hi = 0, size = 0, bigSz = 0, bigPx = 0; double dens = 0, oldest = 0;
    bool touched = false; long long touchT = 0, atTouch = 0; double cancel60 = -1, cnorm = -1; int n60 = 0; bool pulling = false;
    bool reloaded = false; long long relT = 0, sold = 0, refills = 0, left = 0, zeroT = -1, brokeT = -1; int reload = -1; double reloadX = 0;
    bool swept = false; long long swT = 0, runT = 0, runLots = 0, runExt = 0, rebid = 0, liftLots = 0; int sweep = -1; double lift = 0; bool backIn = false;
    bool ended = false; long long endT = 0, endResting = 0; int endCode = 0;
    int state = 0; long long stateT = 0; int reclaimBars = -1;                        // (1.0.2) ZX: 1 testing, 2 held, 3 reclaimed, 4 broken (the latest)
    std::string lvl; long long lvlT = -1;                                               // (1.0.2 levels) ZL: the level names it sits at ("VAL + PS0"), latest wins
    std::map<long long, long long> barSold, barLeft;
};
struct Journal {
    double tick = 0; std::map<int, ZoneView> z; long long lines = 0, bad = 0;
    static std::vector<std::string> split(const std::string& s) { std::vector<std::string> v; std::string f; std::stringstream ss(s); while (std::getline(ss, f, ',')) v.push_back(f); return v; }
    static long long L(const std::string& s) { return std::atoll(s.c_str()); }
    static double D(const std::string& s) { return std::atof(s.c_str()); }
    bool add(const std::string& lnIn) {
        std::string ln = lnIn; while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n')) ln.pop_back();
        const std::vector<std::string> f = split(ln); if (f.empty()) return false;
        const std::string& k = f[0];
        if (k == "H" && f.size() >= 2) { tick = D(f[1]); return true; }
        if (f.size() < 3) { ++bad; return false; }
        const int id = (int)L(f[1]);
        if (k == "ZN" && f.size() >= 12) { ZoneView& v = z[id]; v.id = id; v.T = L(f[2]); v.side = (int)L(f[3]); v.lo = L(f[4]); v.hi = L(f[5]); v.size = L(f[6]); v.dens = D(f[7]); v.oldest = D(f[8]); v.per = (int)L(f[9]); v.bigSz = L(f[10]); v.bigPx = L(f[11]); ++lines; return true; }
        std::map<int, ZoneView>::iterator it = z.find(id); if (it == z.end()) { ++bad; return false; }
        ZoneView& v = it->second;
        if (k == "ZT" && f.size() >= 8 && !v.touched) { v.touched = true; v.touchT = L(f[2]); v.atTouch = L(f[3]); v.cancel60 = D(f[4]); v.cnorm = D(f[5]); v.n60 = (int)L(f[6]); v.pulling = L(f[7]) != 0; }
        else if (k == "ZR" && f.size() >= 11 && !v.reloaded) { v.reloaded = true; v.relT = L(f[2]); v.reload = (int)L(f[3]); v.sold = L(f[4]); v.refills = L(f[5]); v.atTouch = L(f[6]); v.left = L(f[7]); v.reloadX = D(f[8]); v.zeroT = L(f[9]); v.brokeT = L(f[10]); }
        else if (k == "ZS" && f.size() >= 11 && !v.swept) { v.swept = true; v.swT = L(f[2]); v.sweep = (int)L(f[3]); v.runT = L(f[4]); v.runLots = L(f[5]); v.runExt = L(f[6]); v.lift = D(f[7]); v.backIn = L(f[8]) != 0; v.rebid = L(f[9]); v.liftLots = L(f[10]); }
        else if (k == "ZB" && f.size() >= 5) { const long long key = L(f[2]); if (!v.barSold.count(key)) { v.barSold[key] = L(f[3]); v.barLeft[key] = L(f[4]); } }
        else if (k == "ZX" && f.size() >= 4) { const long long t = L(f[2]); if (t >= v.stateT) { v.state = (int)L(f[3]); v.stateT = t; if (v.state == 5 && f.size() >= 5) v.reclaimBars = (int)L(f[4]); } }
        else if (k == "ZL" && f.size() >= 3) { const long long t = L(f[2]); if (t >= v.lvlT) { v.lvlT = t; v.lvl.clear(); if (f.size() >= 4) { std::stringstream ss(f[3]); std::string tg; while (std::getline(ss, tg, ';')) { const std::string nm = tg.substr(0, tg.find(':')); if (nm.empty()) continue; if (!v.lvl.empty()) v.lvl += " + "; v.lvl += nm; } } } }
        else if (k == "ZE" && f.size() >= 5 && !v.ended) { v.ended = true; v.endT = L(f[2]); v.endCode = (int)L(f[3]); v.endResting = L(f[4]); }
        else { ++bad; return false; }
        ++lines; return true;
    }
    const ZoneView* latestTouched() const { const ZoneView* b = nullptr; for (std::map<int, ZoneView>::const_iterator it = z.begin(); it != z.end(); ++it) if (it->second.touched && (!b || it->second.touchT >= b->touchT)) b = &it->second; return b; }
};

// numbers as he reads them: 2,333 / 13x / 0.59x / 34%
inline std::string comma(long long v) { char b[32]; std::snprintf(b, sizeof b, "%lld", v < 0 ? -v : v); std::string s = b, o; int n = 0; for (size_t i = s.size(); i-- > 0;) { o.insert(o.begin(), s[i]); if (++n % 3 == 0 && i) o.insert(o.begin(), ','); } return (v < 0 ? "-" : "") + o; }
inline std::string xText(double x) { char b[32]; std::snprintf(b, sizeof b, x >= 10 ? "%.0fx" : x < 1 ? "%.2fx" : "%.1fx", x); return b; }
inline std::string pct(double x) { char b[16]; std::snprintf(b, sizeof b, "%.0f%%", x * 100); return b; }

inline std::string priceText(long long ticks, double tick) {
    int d = 0; double t = tick; while (d < 6 && t > 0 && std::fabs(t - std::floor(t + 0.5)) > 1e-5 * std::max(1.0, t)) { t *= 10; ++d; }
    char b[48]; std::snprintf(b, sizeof b, "%.*f", d, ticks * tick); return b;
}
// the market's measured signs (ZoneDefenseParams.h: reloadSign / pullSign / sweepSign; 0 = not proven -> white)
struct Signs { int reload = 0, pull = 0, sweep = 0; };
// (1.0.2, review M8 + Rassul's break rule) the zone's state - the SAME word on the Read line and the price-chart label, decided at
// 3-min candle closes (ZX lines): forming (white) until the touch candle closes; testing (white) = traded / closed through, not
// confirmed; held (the defenders' colour) = closed back with no close beyond; reclaimed (defenders' colour) = closed beyond, then back
// before a follow-through; broken (the attackers' colour) = a close beyond + a follow-through bar; pulled (attackers' colour) = what
// rested fell below half of its peak; closed (white) = the pane stopped before it was decided
inline std::string zoneState(const ZoneView& v, const Signs&, int& col) {
    const int bull = v.side == 0 ? +1 : -1; col = 0;
    if (v.state == 5) { col = bull; return "broken -> reclaimed"; }   // a failed break: trapped attackers
    if (v.state == 4) { col = -bull; return "broken"; }
    // (1.0.9, 2026-11-10 Rassul 08:19 "is the zone here suppose to be a lvl?") a held / reclaimed zone whose orders were then pulled
    // stayed "held" after price ran through it: the end now wins - "held -> pulled" (attackers' colour), "expired" after 60 min untouched
    if (v.ended && v.endCode == 2) { col = -bull; return (v.state == 2 || v.state == 3) ? "held -> pulled" : "pulled"; }
    if (v.ended && v.endCode == 3) return "expired";
    if (v.ended && v.endCode >= 4 && v.state == 0) return "closed";
    if (v.state == 1) return "testing";
    if (v.state == 3) { col = bull; return "reclaimed"; }
    if (v.state == 2) { col = bull; return "held"; }
    return "forming";
}
// one Read line (1.0.1, Rassul 2026-11-08: numbered in event order, aligned columns): the label ("Zone"), the verdict word (coloured,
// bolder), the detail (same line, a calmer tint of the same colour), the colour (+1 bullish green, -1 bearish red, 0 white neutral) and
// the time (right-justified, 12-hour). text = verdict + "  " + detail (status file / tests).
struct RLine { std::string label, verdict, detail, text, time; int col = 0; };
inline void setLine(RLine& r, const char* label, const std::string& verdict, const std::string& detail) { r.label = label; r.verdict = verdict; r.detail = detail; r.text = verdict + (detail.empty() ? "" : "  " + detail); }
// JUMPSTART 2026-11-08 (71 market-days): heavy selling INTO the zone in the 60 s after the touch made it break MORE in 6 of 7
// markets (both halves) - so heavy = "Bid exhausted" (red) there, light selling with the zone still there = "Buyers holding".
// Lines in event order: 1 Zone (formed), 2 Queue (the 60 s before the touch), 3 Pressure (the selling pressure into the zone, decided at
// the touch + 60 s; Rassul 2026-11-08: "Reload" renamed "Pressure"), 4 Refill (same 60 s), 5 Sweep.
template <class Clock> std::vector<RLine> readLines(const ZoneView& v, Clock clock, const Signs& sg, double tick = 0) {
    const bool bid = v.side == 0; const int bull = bid ? +1 : -1;      // what defending means for this side
    const std::string who = bid ? "Bid" : "Offer", buyers = bid ? "Buyers" : "Sellers", side = bid ? "bid" : "offer", sides = bid ? "bids" : "offers";
    const std::string dot = " | ";
    std::vector<RLine> out(5);
    // 1 Zone: ONE state for the line and the price-chart label (zoneState, review M8)
    {   char b[96]; const double oldestAtTouch = v.oldest + (v.touched ? (v.touchT - v.T) / 60000.0 : 0);
        std::snprintf(b, sizeof b, " | %s lots | oldest %.0fm", comma(v.size).c_str(), oldestAtTouch);
        const std::string range = tick > 0 ? priceText(v.lo, tick) + "-" + priceText(v.hi, tick) : std::string();
        int col = 0; const std::string st = zoneState(v, sg, col);
        setLine(out[0], "Zone", st, range + (v.lvl.empty() ? std::string(" | no level") : " | at " + v.lvl) + b +   /* (1.0.10) the level(s) it sits at */ (v.state == 5 && v.reclaimBars >= 0 ? std::string(" | ") + std::to_string(v.reclaimBars) + " bars after the break" : std::string())); out[0].col = col; out[0].time = clock(v.ended && (v.endCode == 2 || v.endCode == 3) && v.endT > 0 ? v.endT : (v.stateT > 0 ? v.stateT : v.T));
    }
    // 2 Queue (the 60 s before the touch)
    if (v.cancel60 < 0) setLine(out[1], "Queue", "thin", "too few resting orders to read");
    else { setLine(out[1], "Queue", v.pulling ? "pulling" : "holding", "cancels " + pct(v.cancel60) + " (norm " + pct(v.cnorm < 0 ? 0 : v.cnorm) + ")"); out[1].col = v.pulling && sg.pull < 0 ? -bull : 0; }
    out[1].time = clock(v.touchT);
    // 3 Pressure (selling pressure into the zone, decided at the touch + 60 s)
    if (!v.reloaded) { setLine(out[2], "Pressure", v.ended ? "not decided" : "watching", v.ended ? "the pane stopped before the 60 s" : "the touch"); out[2].time = clock(v.touchT); }
    else {
        out[2].time = clock(v.relT);
        if (v.reload == 0) { setLine(out[2], "Pressure", who + " exhausted", "broke through" + (v.brokeT >= 0 ? " at " + clock(v.brokeT) : std::string())); out[2].col = -bull; }
        else if (v.reload == 2 && sg.reload < 0) { setLine(out[2], "Pressure", who + " exhausted", "absorbed " + comma(v.sold) + " (" + xText(v.reloadX) + ")" + dot + "still at the zone"); out[2].col = -bull; }
        else if (v.reload == 2) setLine(out[2], "Pressure", "Quiet", "absorbed " + comma(v.sold) + " (" + xText(v.reloadX) + ")");
        else if (sg.reload < 0) { setLine(out[2], "Pressure", buyers + " holding", "sold " + comma(v.sold) + " of " + comma(v.atTouch) + " shown"); out[2].col = bull; }
        else setLine(out[2], "Pressure", "Quiet", "sold " + comma(v.sold) + " of " + comma(v.atTouch) + " shown");
    }
    // 4 Refill (the same 60 s)
    if (!v.reloaded) { setLine(out[3], "Refill", v.ended ? "not decided" : "counting", ""); out[3].time = clock(v.touchT); }
    else {
        std::string d = "absorbed " + comma(v.sold) + " (" + xText(v.reloadX) + ")" + dot + side + " " + comma(v.left) + " of " + comma(v.atTouch);
        if (v.zeroT >= 0) d += " -> 0 by " + clock(v.zeroT);
        setLine(out[3], "Refill", comma(v.refills) + " times", d);
        out[3].col = (v.zeroT >= 0 || v.left == 0) ? -bull : out[2].col; out[3].time = clock(v.relT);
    }
    // 5 Sweep (the first stop run per touch, decided 5 s after it)
    if (!v.swept) setLine(out[4], "Sweep", v.ended ? "not decided" : "watching", "");
    else if (v.sweep == 0) { setLine(out[4], "Sweep", "none", "no stop run through the zone"); out[4].time = clock(v.swT); }
    else {
        std::string d = "lift " + xText(v.lift);
        if (v.pulling) d += dot + sides + " pulled before";
        d += dot + (bid ? "re-bid " : "re-offer ") + xText(v.runLots > 0 ? (double)v.rebid / v.runLots : 0);
        if (v.sweep == 1 && (v.state == 3 || v.state == 5)) { setLine(out[4], "Sweep", "reclaimed", "after a weak run" + dot + d); out[4].col = bull; out[4].time = clock(v.stateT); }   // the run + the reclaim, read together
        else { setLine(out[4], "Sweep", v.sweep == 2 ? "swept" : "weak", d); out[4].col = sg.sweep > 0 ? (v.sweep == 2 ? bull : -bull) : 0; out[4].time = clock(v.swT); }
    }
    return out;
}
// (1.0.7, Rassul panel 4) the SHORT label on the price chart: lots + the level(s) the zone is AT ("301 PS0", "450 VAL+PS0"), lots only
// when no level is within the reach; plain ASCII
inline std::string zoneShortLabel(const ZoneView& v) {
    std::string lv; for (size_t i = 0; i < v.lvl.size(); ++i) { const char c = v.lvl[i]; if (c == ' ') continue; if ((unsigned char)c >= 32 && (unsigned char)c < 127) lv += c; }
    return std::to_string(v.size) + (lv.empty() ? std::string() : " " + lv);
}
// the zone label on the price chart: "Zone 7012.25-7013.00 | 450 lots (2.6x) | oldest 14m | at VAL + PS0 | held" (the level part only when tagged)
inline std::string zoneLabel(const ZoneView& v, double tick, const Signs& sg = Signs()) {
    char b[64]; std::snprintf(b, sizeof b, " | %s lots (%.1fx) | oldest %.0fm", comma(v.size).c_str(), v.dens, v.oldest);
    int col = 0; return "Zone " + priceText(v.lo, tick) + "-" + priceText(v.hi, tick) + b + (v.lvl.empty() ? std::string() : " | at " + v.lvl) + " | " + zoneState(v, sg, col);
}
} // namespace zdr
