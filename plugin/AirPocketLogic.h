#ifndef LS_AIRPOCKETLOGIC_H
#define LS_AIRPOCKETLOGIC_H
// AirPocketLogic.h 1.0.0 (2026-10-21, MBO roadmap MIDDLE AHEAD #1 "Is the path to the target open?", Rassul approved option A of
// air-pocket-mockup-v2 at 14:25 CT: the AIR POCKET drawn on the Liquidity Profile column). SDK-free, tested in tests_lqp044/test_ap047.cpp.
// Research (roadmap-mockups/middle-ahead1-path-open-findings.md, 6 markets Sep 24 - Oct 9, matched controls): after a 5-candle local
// extreme, the resting size on the OPPOSITE side between the close and T1 (close +/- 1 ATR in the reversal direction) shrinking to
// <= 0.67x of what it was 60 s earlier, mostly by PULLS (pulls > fills in the window), reached T1 63% vs 49% matched (+14.0, CI +7..+20,
// stable halves); pull-driven +15.9, fill-driven +5.9; static thinness alone +3.7. Close-back >= 0.5 is the filter the research proposed
// (shallow close-backs were the main failures). Decided ONCE at the turn candle's close (no look-ahead); the band lives while fresh and
// ends when price reaches T1, the size comes back above 0.67x, or 15 min pass - the end is recorded once. "?" until proven per market.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace apl {

static const char* const VERSION = "1.0.0";
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };

struct Params {
    double shrinkMax = 0.67;        // ahead now <= 0.67x ahead 60 s earlier
    long long lookbackMs = 60000;   // the minute before the close
    double closeBackMin = 0.5;      // the candle closed back at least half its range from the extreme
    int localN = 5;                 // a 5-candle local extreme (the research universe)
    double atrMult = 1.0;           // T1 = close +/- 1 ATR
    long long liveMaxMs = 15LL * 60 * 1000;   // the band lives at most 15 min
    long long snapKeepMs = 180000;  // book snapshots / flows kept 3 min
    long long snapTolMs = 15000;    // a snapshot this close to the asked time is usable
};

struct Snap { long long ms = 0; std::vector<std::pair<Tick, double> > lv[2]; };   // ascending ticks per side
struct Flow { long long ms = 0; int side = BID; Tick px = 0; double lots = 0; bool fill = false; };

// the book's recent past: one snapshot per depth read (about 1 s), and every pull / fill from the order-by-order feed
struct Tracker {
    std::deque<Snap> snaps; std::deque<Flow> flows;
    void snap(long long ms, const std::map<Tick, double>& bid, const std::map<Tick, double>& ask, Tick ref, Tick reach, const Params& P = Params()) {
        if (!snaps.empty() && ms <= snaps.back().ms) return;
        Snap s; s.ms = ms;
        for (std::map<Tick, double>::const_iterator i = bid.lower_bound(ref - reach); i != bid.end() && i->first <= ref + reach; ++i) if (i->second > 0) s.lv[BID].push_back(*i);
        for (std::map<Tick, double>::const_iterator i = ask.lower_bound(ref - reach); i != ask.end() && i->first <= ref + reach; ++i) if (i->second > 0) s.lv[ASK].push_back(*i);
        snaps.push_back(s);
        while (!snaps.empty() && ms - snaps.front().ms > P.snapKeepMs) snaps.pop_front();
    }
    void flow(long long ms, int side, Tick px, double lots, bool fill, const Params& P = Params()) {
        if (!(lots > 0) || px <= 0) return;
        Flow f; f.ms = ms; f.side = side; f.px = px; f.lots = lots; f.fill = fill; flows.push_back(f);
        while (!flows.empty() && ms - flows.front().ms > P.snapKeepMs) flows.pop_front();
        if (flows.size() > 200000) flows.pop_front();
    }
    // the resting lots on `side` with lo <= px <= hi in the last snapshot at or before ms (within tol); ok = such a snapshot exists
    double ahead(long long ms, int side, Tick lo, Tick hi, bool& ok, long long tol) const {
        ok = false; const Snap* best = 0;
        for (std::deque<Snap>::const_reverse_iterator r = snaps.rbegin(); r != snaps.rend(); ++r) if (r->ms <= ms) { best = &*r; break; }
        if (!best || ms - best->ms > tol) return 0;
        ok = true; double s = 0;
        for (size_t k = 0; k < best->lv[side].size(); ++k) { const Tick p = best->lv[side][k].first; if (p >= lo && p <= hi) s += best->lv[side][k].second; }
        return s;
    }
    void flowsIn(long long t0, long long t1, int side, Tick lo, Tick hi, double& pulled, double& filled) const {
        pulled = filled = 0;
        for (std::deque<Flow>::const_iterator i = flows.begin(); i != flows.end(); ++i) {
            if (i->ms <= t0 || i->ms > t1 || i->side != side || i->px < lo || i->px > hi) continue;
            (i->fill ? filled : pulled) += i->lots;
        }
    }
};

struct Candle { double o = 0, h = 0, l = 0, c = 0; };

// +1 = a local LOW (the reversal is up: the air pocket is above, in the offers), -1 = a local HIGH, 0 = neither / both
inline int localExtreme(const std::vector<Candle>& c, int n = 5) {
    if ((int)c.size() < n + 1) return 0;
    const Candle& x = c.back(); double mh = -1e300, ml = 1e300;
    for (size_t i = c.size() - 1 - (size_t)n; i < c.size() - 1; ++i) { mh = std::max(mh, c[i].h); ml = std::min(ml, c[i].l); }
    const bool top = x.h >= mh, bot = x.l <= ml;
    if (top == bot) return 0;
    return bot ? 1 : -1;
}
inline double closeBack(const Candle& x, int dir) {
    const double r = x.h - x.l; if (!(r > 0)) return 0;
    return dir > 0 ? (x.c - x.l) / r : (x.h - x.c) / r;
}

struct Case {
    long long turnMs = 0;   // the turn candle's END (decision time)
    int dir = 0;            // +1 path up (local low), -1 path down (local high)
    Tick closeTk = 0, t1Tk = 0;
    double before = 0, now = 0, ratio = -1, pulled = 0, filled = 0, cb = 0;
    bool fired = false; std::string why;     // why it did / did not fire
    long long endMs = -1; std::string endWhy;   // set once: "T1" / "size back" / "15 min"
    bool live(long long t, const Params& P = Params()) const { return fired && endMs < 0 && t >= turnMs && t - turnMs < P.liveMaxMs; }
    int side() const { return dir > 0 ? ASK : BID; }       // the side standing in the path
    Tick lo() const { return dir > 0 ? closeTk + 1 : t1Tk; }
    Tick hi() const { return dir > 0 ? t1Tk : closeTk - 1; }
};

// the decision at the turn candle's close. c = the closed candles (last = the turn candle), atr in price units.
inline Case decide(const std::vector<Candle>& c, long long endMs, double tick, double atr, const Tracker& T, const Params& P = Params()) {
    Case k; k.turnMs = endMs;
    const int dir = localExtreme(c, P.localN);
    if (!dir) { k.why = "not a local extreme"; return k; }
    k.dir = dir;
    const Candle& x = c.back();
    if (!(tick > 0) || !(atr > 0)) { k.why = "no tick / ATR"; return k; }
    k.closeTk = (Tick)std::llround(x.c / tick);
    const Tick reach = std::max<Tick>(1, (Tick)std::llround(P.atrMult * atr / tick));
    k.t1Tk = dir > 0 ? k.closeTk + reach : k.closeTk - reach;
    k.cb = closeBack(x, dir);
    bool ok0 = false, ok1 = false;
    k.now = T.ahead(endMs, k.side(), k.lo(), k.hi(), ok1, P.snapTolMs);
    k.before = T.ahead(endMs - P.lookbackMs, k.side(), k.lo(), k.hi(), ok0, P.snapTolMs);
    T.flowsIn(endMs - P.lookbackMs, endMs, k.side(), k.lo(), k.hi(), k.pulled, k.filled);
    if (!ok0 || !ok1) { k.why = "no book history for the minute"; return k; }
    if (!(k.before > 0)) { k.why = "nothing resting ahead a minute ago"; return k; }
    k.ratio = k.now / k.before;
    if (k.ratio > P.shrinkMax) { k.why = "path did not open"; return k; }
    if (!(k.pulled > k.filled)) { k.why = "opened by trades, not pulls"; return k; }
    if (k.cb < P.closeBackMin) { k.why = "shallow close-back"; return k; }
    k.fired = true; k.why = "air pocket";
    return k;
}

// a live band ends once: price reached T1, the size came back above the threshold, or 15 min passed. true = it ended now.
inline bool step(Case& k, long long t, Tick lastPx, bool havePx, const Tracker& T, const Params& P = Params()) {
    if (!k.fired || k.endMs >= 0) return false;
    if (t - k.turnMs >= P.liveMaxMs) { k.endMs = k.turnMs + P.liveMaxMs; k.endWhy = "15 min"; return true; }
    if (havePx && ((k.dir > 0 && lastPx >= k.t1Tk) || (k.dir < 0 && lastPx <= k.t1Tk))) { k.endMs = t; k.endWhy = "T1"; return true; }
    bool ok = false; const double a = T.ahead(t, k.side(), k.lo(), k.hi(), ok, P.snapTolMs);
    if (ok && k.before > 0 && a / k.before > P.shrinkMax && t - k.turnMs >= 5000) { k.endMs = t; k.endWhy = "size back"; return true; }
    return false;
}

// the record: "AP|turnMs|dir|close|t1|before|now|ratio|pulled|filled|closeback|fired|why|endMs|endWhy|version" (one line at the decision,
// one END line "APE|turnMs|endMs|endWhy" when it ends); a restart restores the band from them and never re-decides
inline std::string line(const Case& k) {
    char b[400];
    std::snprintf(b, sizeof(b), "AP|%lld|%d|%lld|%lld|%.0f|%.0f|%.3f|%.0f|%.0f|%.2f|%d|%s|%s", k.turnMs, k.dir, k.closeTk, k.t1Tk, k.before, k.now, k.ratio,
                  k.pulled, k.filled, k.cb, k.fired ? 1 : 0, k.why.c_str(), VERSION);
    return b;
}
inline std::string endLine(const Case& k) { char b[120]; std::snprintf(b, sizeof(b), "APE|%lld|%lld|%s", k.turnMs, k.endMs, k.endWhy.c_str()); return b; }
inline std::vector<std::string> fields(const std::string& s) { std::vector<std::string> o; std::string cur; for (size_t i = 0; i < s.size(); ++i) { if (s[i] == '|') { o.push_back(cur); cur.clear(); } else if (s[i] != '\r' && s[i] != '\n') cur += s[i]; } o.push_back(cur); return o; }
inline bool num(const std::string& t, long long& v) { if (t.empty()) return false; char* e = 0; v = std::strtoll(t.c_str(), &e, 10); return e && !*e; }
inline bool dnum(const std::string& t, double& v) { if (t.empty()) return false; char* e = 0; v = std::strtod(t.c_str(), &e); return e && !*e && std::isfinite(v); }
inline bool parse(const std::string& s, Case& k) {
    const std::vector<std::string> f = fields(s);
    if (f.size() < 14 || f[0] != "AP") return false;
    long long tm, d, c, t1, fired; double b, n, r, p, fl, cb;
    if (!num(f[1], tm) || !num(f[2], d) || !num(f[3], c) || !num(f[4], t1) || !dnum(f[5], b) || !dnum(f[6], n) || !dnum(f[7], r) || !dnum(f[8], p) || !dnum(f[9], fl) || !dnum(f[10], cb) || !num(f[11], fired)) return false;
    if (d != 1 && d != -1) return false;
    Case z; z.turnMs = tm; z.dir = (int)d; z.closeTk = c; z.t1Tk = t1; z.before = b; z.now = n; z.ratio = r; z.pulled = p; z.filled = fl; z.cb = cb; z.fired = fired == 1; z.why = f[12];
    k = z; return true;
}
inline bool parseEnd(const std::string& s, long long& turnMs, long long& endMs, std::string& why) {
    const std::vector<std::string> f = fields(s);
    if (f.size() < 4 || f[0] != "APE") return false;
    if (!num(f[1], turnMs) || !num(f[2], endMs)) return false;
    why = f[3]; return true;
}

// the band in the column: y of the close and of T1 (any order) -> top / bottom clipped to [top, bottom]; false = nothing visible
inline bool bandRect(int yClose, int yT1, int top, int bottom, int& yt, int& yb) {
    yt = std::min(yClose, yT1); yb = std::max(yClose, yT1);
    if (yb <= top || yt >= bottom) return false;
    yt = std::max(top, yt); yb = std::min(bottom, yb);
    if (yb - yt < 2) yb = yt + 2;
    return true;
}

} // namespace apl
#endif
