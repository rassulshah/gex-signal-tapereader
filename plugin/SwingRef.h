#ifndef LS_SWINGREF_H
#define LS_SWINGREF_H
// SwingRef.h (1.0.0, 2026-10-22) -- SDK-free, shared by lsSweepRefill 1.1.0 (Sr) and lsStopRun 1.2.0 (St / Sta): the ONE rule for
// "which swing / level did this sweep take out". Rassul approved the Sr mockup v4 rule at 17:39 CT 2026-10-22 and asked for the SAME
// swing rule on St / Sta, so St and Sr mark the same swings (sweep-refill-mockup-v4.png).
//
// THE ZIGZAG (1-bar, from the RTH open; the research rly2.py / zz.py rules, byte for byte):
//   the first RTH candle sets the direction (green = up). An UP swing ends on a candle that makes a LOWER LOW - unless it is an
//   outside candle (also a higher high) that closes green; a DOWN swing ends on a HIGHER HIGH - unless it is an outside candle (also a
//   lower low) that closes red. The swing's end is its extreme candle (ties: the later one); it is CONFIRMED by the candle that flips.
//
// THE REFERENCE A SWEEP TAKES OUT (down sweep = through a LOW; the mirror for an up sweep through a HIGH):
//   1. an INTACT swing low among the last 3 CONFIRMED swing lows (confirmed by a candle that closed before the sweep's candle):
//        1 back always counts; 2 back counts if it is below 1 back; 3 back counts if it is below 1 and 2 back.
//   2. else a key level (ONL PDL PFL LonLO PWL AsiaLO) or a MenthorQ level (either side).
//   5-candle extremes and overnight pivots no longer count.
//   TAKEN OUT = the sweep ENDS beyond it (its lowest fill < the reference) AND the sweep STARTS at most 0.5 ATR past it (first fill
//   - reference <= 0.5 ATR below) AND no candle before the last closed one had already traded through it (a swing: the candles after
//   its pivot candle up to the one before the last; a level: the 5 candles before the last one).
//   More than one taken out = the DEEPEST one (the lowest low / highest high).
//   ATR = the mean TRUE RANGE of the last 14 candles that closed before the sweep's candle.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace swg {

static const char* const VERSION = "1.0.0";
static const int ZZ_CAP = 1600, WIN_CAP = 64;
enum Kind { SWING = 0, KEY = 1, MQ = 2, VP = 3 };   // VP = a volume-profile level (POC / VAH / VAL, prior day, naked prior POCs)

// RTH open / close (CT minutes), the same table as SessionVWAPLogic.h svl::rthOf
inline bool rthOf(const std::string& m, int& openMin, int& closeMin) {
    if (m == "ES" || m == "NQ") { openMin = 8 * 60 + 30; closeMin = 15 * 60; return true; }
    if (m == "CL" || m == "NG") { openMin = 8 * 60; closeMin = 13 * 60 + 30; return true; }
    if (m == "GC" || m == "HG") { openMin = 7 * 60 + 20; closeMin = 12 * 60 + 30; return true; }
    if (m == "EU" || m == "6E") { openMin = 7 * 60 + 20; closeMin = 14 * 60; return true; }
    return false;
}
// the candle's start (CT second of the day = END - spb) inside [open, close) and its END <= close
inline bool isRth(int endSecOfDay, int spb, int openMin, int closeMin) {
    const int s = endSecOfDay - spb;
    return s >= openMin * 60 && endSecOfDay <= closeMin * 60;
}

struct ZBar { long long t = 0, o = 0, h = 0, l = 0, c = 0; long long day = 0; bool rth = false; };   // ticks; t = the candle END
struct Swing { int idx = 0; long long barT = 0, px = 0; int dir = 0; int confirmIdx = 0; };       // dir -1 = swing low, +1 = high

// ---- the 1-bar ZigZag of today's RTH candles (closed candles only, in time order) ----------------------------------------------
struct ZigZag {
    long long day = 0; int spb = 180; int rthLenSec = 0;
    std::vector<ZBar> bars; std::vector<Swing> swings; int st = 0, ext = 0;
    void reset(long long d) { day = d; bars.clear(); swings.clear(); st = 0; ext = 0; }
    void add(const ZBar& b) {
        if (!b.rth) return;
        if (b.day != day) reset(b.day);
        if (!bars.empty() && b.t <= bars.back().t) return;
        if ((int)bars.size() >= ZZ_CAP) return;
        bars.push_back(b);
        const int i = (int)bars.size() - 1;
        if (i == 0) { st = b.c >= b.o ? 1 : -1; ext = 0; return; }
        const ZBar& p = bars[(size_t)i - 1];
        const bool hh = b.h > p.h, ll = b.l < p.l;
        if (st == 1) {
            if (ll && !(hh && b.c >= b.o)) { Swing s; s.idx = ext; s.barT = bars[(size_t)ext].t; s.px = bars[(size_t)ext].h; s.dir = 1; s.confirmIdx = i; swings.push_back(s); st = -1; ext = i; }
            else if (b.h >= bars[(size_t)ext].h) ext = i;
        } else {
            if (hh && !(ll && b.c < b.o)) { Swing s; s.idx = ext; s.barT = bars[(size_t)ext].t; s.px = bars[(size_t)ext].l; s.dir = -1; s.confirmIdx = i; swings.push_back(s); st = 1; ext = i; }
            else if (b.l <= bars[(size_t)ext].l) ext = i;
        }
    }
    // the sweep's candle (END barT) is one of today's RTH candles of this zigzag
    bool covers(long long barT) const {
        if (bars.empty() || rthLenSec <= 0) return false;
        const long long open = bars.front().t - spb;
        return barT > open && barT <= open + rthLenSec;
    }
    // the line (as the v4 mockup): the CONFIRMED swing ends only, in order (bar END, price) - a confirmed point never moves
};

// ---- the closed candles of the whole session (ATR, the level pre-range) --------------------------------------------------------
struct BarWin {
    std::vector<ZBar> v;
    void add(const ZBar& b) { if (!v.empty() && b.t <= v.back().t) return; v.push_back(b); if ((int)v.size() > WIN_CAP) v.erase(v.begin()); }
    // mean true range of the last 14 candles that closed before the sweep's candle (END < barT); 0 = none
    double atr(long long barT) const {
        int last = -1; for (int i = (int)v.size() - 1; i >= 0; --i) if (v[(size_t)i].t < barT) { last = i; break; }
        if (last < 0) return 0;
        double s = 0; int m = 0;
        for (int i = last; i >= 0 && m < 14; --i, ++m) {
            const ZBar& b = v[(size_t)i]; double tr = (double)(b.h - b.l);
            if (i > 0) { const double pc = (double)v[(size_t)i - 1].c; tr = std::max(tr, std::max(std::fabs((double)b.h - pc), std::fabs((double)b.l - pc))); }
            s += tr;
        }
        return m ? s / m : 0;
    }
    // the `look` candles before the last one that closed before the sweep's candle (END <= barT - 2 spb)
    bool pre(long long barT, int spb, int look, long long& lo, long long& hi) const {
        int m = 0; lo = hi = 0;
        for (int i = (int)v.size() - 1; i >= 0 && m < look; --i) {
            const ZBar& b = v[(size_t)i]; if (b.t > barT - 2LL * spb) continue;
            if (m == 0 || b.l < lo) lo = b.l;
            if (m == 0 || b.h > hi) hi = b.h;
            ++m;
        }
        return m > 0;
    }
};

// ---- the reference -----------------------------------------------------------------------------------------------------------
struct Lvl { std::string name; long long px = 0; int dir = 0; int kind = KEY; };   // dir -1 a low, +1 a high, 0 either (MenthorQ)
// his session levels: -1 low / +1 high; joined names ("PFH/PDH") count when every part is one side; else 0 (not a session level)
inline int keyDir(const std::string& n) {
    static const char* const H[6] = { "ONH", "PDH", "PFH", "LonHI", "PWH", "AsiaHI" }, * const L[6] = { "ONL", "PDL", "PFL", "LonLO", "PWL", "AsiaLO" };
    size_t a = 0; int d = 0;
    for (;;) {
        const size_t p = n.find('/', a); std::string t = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        while (!t.empty() && t[0] == ' ') t.erase(0, 1);
        while (!t.empty() && t[t.size() - 1] == ' ') t.erase(t.size() - 1);
        int td = 0; for (int i = 0; i < 6; ++i) { if (t == H[i]) td = 1; if (t == L[i]) td = -1; }
        if (td == 0 || (d != 0 && td != d)) return 0;
        d = td;
        if (p == std::string::npos) break;
        a = p + 1;
    }
    return d;
}
struct Sweep { bool down = true; long long ext = 0, start = 0, barT = 0; };   // ext = the last fill (deepest), start = the first fill
struct Ref { bool ok = false; int kind = SWING; std::string name; long long px = 0, fromT = 0; int back = 0; };
struct Params { double nearAtr = 0.5; int look = 5; int maxBack = 3; bool useKey = true, useMq = true; long long maxBefore = -1; };   // maxBefore: St only - starts at most this many ticks before the reference (-1 = off)   // maxBack = how many swings back (1-3)

inline bool beyond(const Sweep& s, long long px) { return s.down ? s.ext < px : s.ext > px; }
inline bool nearStart(const Sweep& s, long long px, double atr, double nearAtr, long long maxBefore = -1) {
    const double far = s.down ? (double)(px - s.start) : (double)(s.start - px);   // > 0 = it started already past the reference
    if (maxBefore >= 0 && -far > (double)maxBefore) return false;                   // started too far before it (St: not "at" the level)
    return far <= nearAtr * atr + 1e-9;
}
// the swing / level this sweep took out, or ok = false
inline Ref pick(const ZigZag& Z, const BarWin& W, const std::vector<Lvl>& levels, const Sweep& s, int spb, const Params& P = Params()) {
    Ref out; const double atr = W.atr(s.barT); const int dir = s.down ? -1 : 1;
    if (Z.covers(s.barT)) {
        int fi = 0; while (fi < (int)Z.bars.size() && Z.bars[(size_t)fi].t < s.barT) ++fi;      // candles closed before the sweep's candle
        std::vector<const Swing*> conf;
        for (size_t i = 0; i < Z.swings.size(); ++i) if (Z.swings[i].confirmIdx < fi && Z.swings[i].dir == dir) conf.push_back(&Z.swings[i]);
        const int n = (int)conf.size();
        for (int k = 0; k < P.maxBack && k < 3 && k < n; ++k) {
            const Swing& p = *conf[(size_t)(n - 1 - k)];
            bool elig = true;
            for (int q = 0; q < k && elig; ++q) { const Swing& r = *conf[(size_t)(n - 1 - q)]; elig = s.down ? p.px < r.px : p.px > r.px; }
            if (!elig || !beyond(s, p.px) || !nearStart(s, p.px, atr, P.nearAtr, P.maxBefore)) continue;
            bool intact = true;
            for (int i = p.idx + 1; i <= fi - 2 && intact; ++i) { const ZBar& b = Z.bars[(size_t)i]; intact = s.down ? b.l >= p.px : b.h <= p.px; }
            if (!intact) continue;
            if (!out.ok || (s.down ? p.px < out.px : p.px > out.px)) {
                out.ok = true; out.kind = SWING; out.px = p.px; out.fromT = p.barT; out.back = k + 1; out.name = s.down ? "swing low" : "swing high";
            }
        }
        if (out.ok) return out;
    }
    long long lo = 0, hi = 0; const bool havePre = W.pre(s.barT, spb, P.look, lo, hi);
    for (size_t i = 0; i < levels.size(); ++i) {
        const Lvl& L = levels[i];
        if (L.dir != 0 && L.dir != dir) continue;
        if (L.kind == MQ ? !P.useMq : !P.useKey) continue;                          // key and volume-profile levels: the key flag
        if (!beyond(s, L.px) || !nearStart(s, L.px, atr, P.nearAtr, P.maxBefore)) continue;
        if (havePre && (s.down ? lo < L.px : hi > L.px)) continue;                              // a pre-move candle already traded through it
        if (!out.ok || (s.down ? L.px < out.px : L.px > out.px)) { out.ok = true; out.kind = L.kind; out.px = L.px; out.fromT = 0; out.back = 0; out.name = L.name; }
    }
    return out;
}

// ---- VOLUME-PROFILE LEVELS (Rassul 18:07 CT: "St, Sta and Sr must also count volume profile levels ... POC, VAH and VAL for the
// current session and the prior day, plus naked (untested) prior POCs"), from the chart's own volume at price, closed candles only:
//   POC / VAH / VAL        this session (17:00 -> 16:00 CT) as of the last closed candle (no look-ahead; it develops)
//   pdPOC / pdVAH / pdVAL  the prior full session's profile (the same session as lsDealerProfile's DPoc / VAH / VAL)
//   nPOC                   an older session's POC that no later candle has traded at yet (naked); up to 10 sessions back
// Profile rule = KeyLevelsLogic.h klv::profileOf with 1-tick rows: POC = the biggest row (a tie: nearest the middle, then the lower);
// value area from the POC adding the bigger of the next two rows above / below (equal = both) until >= 70% of the volume.
struct Prof { long long poc = 0, vah = 0, val = 0; double vol = 0; bool ok = false; };
inline Prof profileOf(const std::map<long long, double>& rows, double vaPct = 0.70) {
    Prof P; std::vector<long long> key; std::vector<double> vol;
    for (std::map<long long, double>::const_iterator it = rows.begin(); it != rows.end(); ++it) if (it->second > 0) { key.push_back(it->first); vol.push_back(it->second); P.vol += it->second; }
    if (key.empty()) return P;
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
        if (up > dn + 1e-9) { hi += nu; cum += up; } else if (dn > up + 1e-9) { lo -= nd; cum += dn; } else { hi += nu; lo -= nd; cum += up + dn; }
    }
    P.poc = key[pi]; P.vah = key[hi]; P.val = key[lo]; P.ok = true; return P;
}
struct VolProf {
    struct Done { long long sess = 0; Prof p; bool naked = true; };
    long long sess = 0, lastT = 0; std::map<long long, double> cur; Prof now; std::vector<Done> done;   // done: oldest first
    // one CLOSED candle (in time order): its session key (17:00 CT trading day), range and volume at price (tick -> lots)
    void add(long long t, long long session, long long h, long long l, const std::vector<std::pair<long long, double> >& rows) {
        if (t <= lastT) return;
        lastT = t;
        if (session != sess) {
            if (!cur.empty()) { Done d; d.sess = sess; d.p = profileOf(cur); if (d.p.ok) { done.push_back(d); if (done.size() > 11) done.erase(done.begin()); } }
            cur.clear(); sess = session; now = Prof();
        }
        for (size_t i = 0; i < done.size(); ++i) if (done[i].naked && l <= done[i].p.poc && h >= done[i].p.poc) done[i].naked = false;   // traded at = tested
        bool added = false;
        for (size_t i = 0; i < rows.size(); ++i) if (rows[i].second > 0 && rows[i].second < 1e12) { cur[rows[i].first] += rows[i].second; added = true; }
        if (added) now = profileOf(cur);
        if (cur.size() > 20000) { cur.clear(); now = Prof(); }                         // a corrupt feed: start over (never unbounded)
    }
    void levels(std::vector<Lvl>& out) const {
        auto put = [&](const char* n, long long px) { Lvl L; L.name = n; L.px = px; L.dir = 0; L.kind = VP; out.push_back(L); };
        if (now.ok) { put("POC", now.poc); put("VAH", now.vah); put("VAL", now.val); }
        if (!done.empty() && done.back().sess != sess) { const Prof& p = done.back().p; put("pdPOC", p.poc); put("pdVAH", p.vah); put("pdVAL", p.val); }
        for (size_t i = 0; i + 1 < done.size(); ++i) if (done[i].naked) put("nPOC", done[i].p.poc);
    }
};

// ---- the per-market params of St / Sta and Sr: lsFlexLevels\LRA-StopSweep-<MKT>.csv (VALIDATE-STOPRUN-SWEEP-2026-10-22.md) -----------
// VERSION,1 first; MARKET must match; PARAM,<name>,<value>; <MARK>_<SIDE>_SHOW,SHOW|?|LOG. Missing / garbled file = the defaults
// below with every mark LOG (recorded, not drawn). Unknown PARAM names are ignored; a bad value = the whole file rejected.
enum Show { LOG = 0, QUERY = 1, SHOW = 2 };
struct StopSweep {
    bool loaded = false; std::string why = "no file";
    int stRefPivots = 1, stKey = 1, stMq = 1, stWindowMs = 50, stMinFills = 5, stNearTicks = 4, staBigLots = 5, staDecideLive = 1, staReclaimSec = 10, staAbsorbBars = 1;
    double stRatio = 3.0;
    int staBull = LOG, staBear = LOG, stGrey = LOG;
    int srRefPivots = 3, srKey = 1, srMq = 1, srMinPrices = 8, srWindowMs = 50, srRefillPct = 50, srWindowSec = 5, srStaySec = 5, srMinRestSec = 1, srFollowSec = 0;
    int srBull = LOG, srBear = LOG;
};
inline std::string showText(int v) { return v == SHOW ? "SHOW" : v == QUERY ? "?" : "LOG"; }
inline StopSweep parseStopSweep(const std::vector<std::string>& lines, const std::string& market) {
    StopSweep d, P; bool ver = false, mk = false; std::string bad;
    auto num = [](const std::string& t, double lo, double hi, double& out) {
        if (t.empty() || t.size() > 24) return false;
        char* e = nullptr;
        const double v = std::strtod(t.c_str(), &e);
        if (!e || *e || !(v == v) || v < lo || v > hi) return false;
        out = v;
        return true;
    };
    for (size_t i = 0; i < lines.size() && bad.empty(); ++i) {
        std::string ln = lines[i]; while (!ln.empty() && (ln[ln.size() - 1] == '\r' || ln[ln.size() - 1] == ' ')) ln.erase(ln.size() - 1);
        if (ln.empty() || ln[0] == '#') continue;
        std::vector<std::string> c; size_t a = 0; for (;;) { const size_t q = ln.find(',', a); c.push_back(ln.substr(a, q == std::string::npos ? std::string::npos : q - a)); if (q == std::string::npos) break; a = q + 1; }
        if (!ver) { if (c.size() == 2 && c[0] == "VERSION" && c[1] == "1") { ver = true; continue; } bad = "VERSION,1 must be the first line"; break; }
        if (c[0] == "MARKET") { if (c.size() != 2 || c[1] != market) { bad = "MARKET is not " + market; break; } mk = true; continue; }
        if (c[0] == "ASOF" || c[0] == "SOURCE") continue;
        if (c.size() == 2 && c[0].size() > 5 && c[0].compare(c[0].size() - 5, 5, "_SHOW") == 0) {
            const int v = c[1] == "SHOW" ? SHOW : c[1] == "?" ? QUERY : c[1] == "LOG" ? LOG : -1;
            if (v < 0) { bad = c[0] + " is not SHOW / ? / LOG"; break; }
            if (c[0] == "STA_BULL_SHOW") P.staBull = v; else if (c[0] == "STA_BEAR_SHOW") P.staBear = v; else if (c[0] == "ST_GREY_SHOW") P.stGrey = v;
            else if (c[0] == "SR_BULL_SHOW") P.srBull = v; else if (c[0] == "SR_BEAR_SHOW") P.srBear = v;
            continue;
        }
        if (c[0] != "PARAM" || c.size() != 3) { bad = "bad line " + std::to_string(i + 1); break; }
        struct I { const char* n; int* p; double lo, hi; } ints[] = {
            { "stRefPivots", &P.stRefPivots, 1, 3 }, { "stRefKeyLevels", &P.stKey, 0, 1 }, { "stRefMenthorQ", &P.stMq, 0, 1 }, { "stWindowMs", &P.stWindowMs, 10, 1000 },
            { "stMinFills", &P.stMinFills, 2, 200 }, { "stNearTicks", &P.stNearTicks, 1, 50 }, { "staBigLots", &P.staBigLots, 1, 1000 }, { "staDecideLive", &P.staDecideLive, 0, 1 },
            { "staReclaimSec", &P.staReclaimSec, 0, 60 }, { "staAbsorbBars", &P.staAbsorbBars, 0, 10 },
            { "srRefPivots", &P.srRefPivots, 1, 3 }, { "srRefKeyLevels", &P.srKey, 0, 1 }, { "srRefMenthorQ", &P.srMq, 0, 1 }, { "srMinPrices", &P.srMinPrices, 2, 100 },
            { "srWindowMs", &P.srWindowMs, 10, 1000 }, { "srRefillPct", &P.srRefillPct, 10, 300 }, { "srWindowSec", &P.srWindowSec, 1, 30 }, { "srStaySec", &P.srStaySec, 0, 30 },
            { "srMinRestSec", &P.srMinRestSec, 0, 10 }, { "srFollowSec", &P.srFollowSec, 0, 3600 } };
        bool known = false; double v = 0;
        for (size_t k = 0; k < sizeof ints / sizeof ints[0]; ++k) if (c[1] == ints[k].n) {
            known = true;
            if (!num(c[2], ints[k].lo, ints[k].hi, v) || v != (double)(long long)v) { bad = c[1] + " out of range"; break; }
            *ints[k].p = (int)v;
        }
        if (!known && c[1] == "stRatio") { if (!num(c[2], 1.0, 20.0, v)) { bad = "stRatio out of range"; break; } P.stRatio = v; }
    }
    if (bad.empty() && !ver) bad = "empty";
    if (bad.empty() && !mk) bad = "no MARKET line";
    if (!bad.empty()) { d.why = "rejected: " + bad + " (defaults, every mark LOG)"; return d; }
    P.loaded = true; P.why = "loaded"; return P;
}

// ---- text: "the swing low from 8:48 AM, 2 swings back" / "the LonLO 31178.50" -----------------------------------------------
inline std::string backText(int back) { return back <= 1 ? std::string() : ", " + std::to_string(back) + " swings back"; }

} // namespace swg
#endif
