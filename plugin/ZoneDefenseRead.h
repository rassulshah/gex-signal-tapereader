// ZoneDefenseRead.h  --  lsZoneDefense 1.0.0: the journal model and the 4 Read lines (SDK-free; the pane, the price-chart marks
// overlay and the tests all draw from THIS model, built only from decided journal lines -> nothing is ever repainted, a restart
// reloads the same picture).
// Journal lines (lsFlexLevels\ZoneDefense\<MKT>-<spb>-<session>.csv, append-only; t = feed ms):
//   H,<tick>,<version>                                     header
//   ZN,id,T,side,lo,hi,size,dens,oldestMin,period,bigSz,bigPx   a zone (decided at a minute close)
//   ZT,id,t,atTouch,cancel60,cnorm,n60,pulling            the touch + the queue read (60 s before it)
//   ZR,id,t,verdict,sold,refills,atTouch,left,reloadX,zeroT,brokeT   the reload decision (touch + 60 s): 0 exhausted, 1 quiet, 2 holding
//   ZS,id,t,verdict,runT,runLots,runExt,lift,backIn,rebid,liftLots  the sweep decision (run end + 5 s): 1 weak, 2 swept
//   ZB,id,candleKey,sold,left                             a candle closed: sold into the zone in it, resting left
//   ZE,id,t,code,resting                                  the zone ended: 1 broken, 2 faded, 3 expired, 4 session end
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

// one Read line: text (verdict first), colour (+1 bullish green, -1 bearish red, 0 white neutral), the time (right-justified)
struct RLine { std::string text, time; int col = 0; };
// the market's measured signs (ZoneDefenseParams.h: reloadSign / pullSign / sweepSign; 0 = not proven -> white)
struct Signs { int reload = 0, pull = 0, sweep = 0; };
// clock(t) -> "10:03 AM" (12-hour, the host's local clock)
// JUMPSTART 2026-11-08 (71 market-days): heavy selling INTO the zone in the 60 s after the touch made it break MORE in 6 of 7
// markets (both halves) - so heavy = "Bid exhausted" (red) there, light selling with the zone still there = "Buyers holding".
template <class Clock> std::vector<RLine> readLines(const ZoneView& v, Clock clock, const Signs& sg) {
    const bool bid = v.side == 0; const int bull = bid ? +1 : -1;      // what defending means for this side
    const std::string who = bid ? "Bid" : "Offer", buyers = bid ? "Buyers" : "Sellers", side = bid ? "bid" : "offer", sides = bid ? "bids" : "offers";
    const std::string dot = " \xC2\xB7 ";
    std::vector<RLine> out(4);
    // 1 Reload (decided at the touch + 60 s)
    if (!v.reloaded) { out[0].text = "Reload: watching the touch"; out[0].time = clock(v.touchT); }
    else {
        out[0].time = clock(v.relT);
        if (v.reload == 0) { out[0].text = who + " exhausted" + dot + "broke through" + (v.brokeT >= 0 ? " at " + clock(v.brokeT) : std::string()); out[0].col = -bull; }
        else if (v.reload == 2 && sg.reload < 0) { out[0].text = who + " exhausted" + dot + "absorbed " + comma(v.sold) + " (" + xText(v.reloadX) + ")" + dot + "still at the zone"; out[0].col = -bull; }
        else if (v.reload == 2) { out[0].text = "Quiet" + dot + "absorbed " + comma(v.sold) + " (" + xText(v.reloadX) + ")"; }
        else if (sg.reload < 0) { out[0].text = buyers + " holding" + dot + "sold " + comma(v.sold) + " of " + comma(v.atTouch) + " shown"; out[0].col = bull; }
        else { out[0].text = "Quiet" + dot + "sold " + comma(v.sold) + " of " + comma(v.atTouch) + " shown"; }
    }
    // 2 Queue (the 60 s before the touch)
    if (v.cancel60 < 0) out[1].text = "Queue: too few resting orders to read";
    else { out[1].text = "Queue: cancels " + pct(v.cancel60) + " (norm " + pct(v.cnorm < 0 ? 0 : v.cnorm) + ")" + dot + (v.pulling ? "pulling" : "steady"); out[1].col = v.pulling && sg.pull < 0 ? -bull : 0; }
    out[1].time = clock(v.touchT);
    // 3 Refill (the same 60 s)
    if (!v.reloaded) { out[2].text = "Refill: counting"; out[2].time = clock(v.touchT); }
    else {
        out[2].text = "Refill: " + comma(v.refills) + " times" + dot + "absorbed " + comma(v.sold) + " (" + xText(v.reloadX) + ")" + dot + side + " " + comma(v.left) + " of " + comma(v.atTouch);
        if (v.zeroT >= 0) out[2].text += " \xE2\x86\x92 0 by " + clock(v.zeroT);
        out[2].col = (v.zeroT >= 0 || v.left == 0) ? -bull : out[0].col; out[2].time = clock(v.relT);
    }
    // 4 Sweep (the first stop run per touch, decided 5 s after it)
    if (!v.swept) out[3].text = "Sweep: watching";
    else if (v.sweep == 0) { out[3].text = "Sweep: none"; out[3].time = clock(v.swT); }
    else {
        out[3].text = std::string("Sweep: ") + (v.sweep == 2 ? "swept" : "weak") + " lift " + xText(v.lift);
        if (v.pulling) out[3].text += dot + sides + " pulled before";
        out[3].text += dot + (bid ? "re-bid " : "re-offer ") + xText(v.runLots > 0 ? (double)v.rebid / v.runLots : 0);
        out[3].col = sg.sweep > 0 ? (v.sweep == 2 ? bull : -bull) : 0; out[3].time = clock(v.swT);
    }
    return out;
}
// the zone label on the price chart: "Zone 7012.25-7013.00 · 450 lots (2.6x) · oldest 14m · held"
inline std::string priceText(long long ticks, double tick) {
    int d = 0; double t = tick; while (d < 6 && t > 0 && std::fabs(t - std::floor(t + 0.5)) > 1e-9) { t *= 10; ++d; }
    char b[48]; std::snprintf(b, sizeof b, "%.*f", d, ticks * tick); return b;
}
inline std::string zoneLabel(const ZoneView& v, double tick) {
    char b[64]; std::snprintf(b, sizeof b, " \xC2\xB7 %s lots (%.1fx) \xC2\xB7 oldest %.0fm", comma(v.size).c_str(), v.dens, v.oldest);
    std::string s = "Zone " + priceText(v.lo, tick) + "\xE2\x80\x93" + priceText(v.hi, tick) + b;
    if (v.ended && v.endCode == 1) s += " \xC2\xB7 broken"; else if (v.reloaded && v.reload != 0) s += " \xC2\xB7 held";
    else if (v.reloaded && v.reload == 0) s += " \xC2\xB7 broken";
    return s;
}
} // namespace zdr
