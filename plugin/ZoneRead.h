#ifndef LS_ZONE_READ_H
#define LS_ZONE_READ_H
// ZoneRead.h (1.1.0, 2026-10-14 night) -- SDK-free.
// 1.1.0 (2026-10-14 night; Rassul approved 20:04 CT "merge the Pulling read into PullStats and retire lsPulling", then changed the
//   design: "the PullStats line shows ONLY the locked arrival read after the 3-min window ... it emphasises the pull"): the PullStats
//   pane's line is now the ARRIVAL read (the arrival cohort cancelled vs traded in the first 3 minutes - what lsPulling 0.3.0 showed),
//   drawn ONLY once it locks; nothing while approaching or measuring. The pre-touch share is still measured at arrival, recorded and
//   shown in the hover as a secondary fact. The PullStats host records an "arrived" row at arrival (pre-touch numbers + the arrival
//   size), so a restart inside the 3 minutes keeps the pre-touch part and, when the window has passed, shows a grey "arrival read
//   not known (restarted during the 3 min) ?" instead of a read it could not measure. Record rows gain pre_moved|pre_partial|
//   pre_impl|pre_text at the END; an existing session file's header is upgraded once (atomic rewrite) before the first longer row;
//   1.0.0 rows (27 fields) are still read (a 1.0.0 PullStats row is rebuilt as the arrival read from its own numbers).
//   BidAskStats / StackStats / Liquidity lines: unchanged. ONE read line per order-flow pane (lsBidAskStats, lsStackStats, lsPullStats,
// lsPulling), in place of the read box. Rassul 2026-10-14 night (via the coordinator): "every order-flow pane gets its own ONE read
// line in place of its box" - shown ONLY while price is inside a key-level or gamma ZONE (ZoneRule.h: level +/- 0.06 ADR, the same
// width as the Liquidity Profile zone rule), otherwise nothing; plain words; values as ratios vs the same-market, same-30-min-slot
// normal; always "?"; locked once its window closes (no repaint); detail + the study caveat in the hover; green = buyers / bullish,
// red = sellers / bearish, grey = neither.
//   Pulling     the arrival cohort (bids resting at a support level when price arrived, offers at a resistance) in the first
//               3 minutes: CANCELLED or TRADED. "ES 7750 - bids pulled, not filled (62% in 3 min) - less support ?"
//               Study: research-artifacts\review\MBO-LIQUIDITY-READS-2026-10-14.md Q2v ("arrival cohort cancelled during the 3-min
//               defence", z >= 1 vs the market's past same-slot normal: held about 19 pp less often, NOT YET).
//   StackStats  size ADDED on the defending side at the level in those 3 minutes. "ES 7750 - bids stacking 2.3x normal ?"
//               Study Q3: refill / stacking size on its own mostly reflects a weak attack (NO SIGN once attack pace is held).
//   BidAskStats the attackers' lots traded into the level in those 3 minutes and where price is at the end.
//               "ES 7750 - sellers hitting 3.1x normal, price held ?" / "... price through ?"
//   PullStats   PRE-TOUCH: the share of the size resting at the level 5 minutes before arrival that was cancelled before price got
//               there. "ES 7750 - bids pulled before touch (45%) ?" Study Q2 primary: NO SIGN yet (+7 pp breaks, not significant).
// MEASUREMENT (the study's clock, lra.mbo_study): the level BAND = level +/- the MEDIUM half-width (the Liquidity Profile / NearLevels
// zone half-width per market); ARRIVAL = the first trade in the band after price entered the zone from outside (armed by a CLOSED
// bar outside the zone, the level-confluence encounter rule); the defence window = arrival .. arrival + 3 min; pre-touch = the band's
// orders 5 min before arrival. Orders come from the chart's own order-by-order feed (native DEPTH_BY_ORDER); an order that rested
// before the indicator loaded is unknown (the read says so: "partial").
// NORMALS: lsFlexLevels\LRA-ZoneReadNorms-<MKT>.csv - NORM|metric|slot|median|mad|n per 30-min CT slot (slot = minute of the day the
// slot starts) or ALL; a slot needs n >= 8, else ALL with n >= 15, else no normal (the line shows the share only, grey). Seeded from the
// study's event cache (medium zones, RTH touches); "present" = robust z = (x - median) / (1.4826 MAD) >= 1.0 (the study's line).
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <sstream>
#include <locale>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "ZoneRule.h"

namespace zread {

static const char* const VERSION = "1.1.0";
typedef long long Tick;
typedef long long Ms;
enum Pane { P_BAS = 0, P_STACK = 1, P_PULLSTATS = 2, P_PULLING = 3, P_N = 4 };
enum Action { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Side { BID = 0, ASK = 1 };
enum State { NONE = 0, APPROACH = 1, MEASURE = 2, FROZEN = 3 };
static const Ms WINDOW_MS = 180000, PRE_MS = 300000, SNAP_MS = 15000, DEP_KEEP_MS = 420000, RESTORE_MAX_MS = 3600000;
static const size_t ORD_CAP = 60000, DEP_CAP = 200000, SNAP_CAP = 40;
static const double Z_LINE = 1.0;                    // the study's "feature present" line, fixed before looking (never tuned)

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }
inline const char* paneName(int p) { return p == P_BAS ? "BidAskStats" : p == P_STACK ? "StackStats" : p == P_PULLSTATS ? "PullStats" : "Pulling"; }

// ---- normals ----------------------------------------------------------------------------------------------------------------
struct Norm { double med = -1, mad = -1; int n = 0; bool slot = false; bool ok() const { return n > 0 && med >= 0 && mad >= 0; } };
struct Norms {
    std::map<std::string, std::map<int, Norm> > m;      // metric -> slot (-1 = ALL) -> norm
    bool ok = false; std::string why = "no file";
    Norm get(const std::string& metric, int minuteOfDay) const {
        std::map<std::string, std::map<int, Norm> >::const_iterator a = m.find(metric); if (a == m.end()) return Norm();
        if (minuteOfDay >= 0) {
            std::map<int, Norm>::const_iterator s = a->second.find((minuteOfDay / 30) * 30);
            if (s != a->second.end() && s->second.n >= 8) { Norm r = s->second; r.slot = true; return r; }
        }
        std::map<int, Norm>::const_iterator all = a->second.find(-1);
        if (all != a->second.end() && all->second.n >= 15) return all->second;
        return Norm();
    }
};
inline bool parseNorms(const std::string& text, const std::string& market, Norms& out) {
    Norms N; const std::vector<std::string> L = zrule::lines(text);
    bool mk = false, ver = false;
    for (size_t i = 0; i < L.size(); ++i) {
        if (L[i].empty() || L[i][0] == '#') continue;
        const std::vector<std::string> f = zrule::splitBar(L[i]);
        if (f[0] == "VERSION") { if (f.size() < 2 || f[1] != "1") { out = Norms(); out.why = "bad VERSION"; return false; } ver = true; continue; }
        if (f[0] == "MARKET") { if (f.size() < 2 || f[1] != market) { out = Norms(); out.why = "MARKET is not " + market; return false; } mk = true; continue; }
        if (f[0] != "NORM" || f.size() < 6) continue;
        double med = 0, mad = 0, n = 0;
        if (!zrule::numTok(f[3], med) || !zrule::numTok(f[4], mad) || !zrule::numTok(f[5], n) || med < 0 || mad < 0 || n < 1 || n > 1e7) continue;
        int slot = -1;
        if (f[2] != "ALL") { double s = 0; if (!zrule::numTok(f[2], s) || s < 0 || s >= 1440 || std::fmod(s, 30.0) != 0) continue; slot = (int)s; }
        if (f[1].empty() || f[1].size() > 24) continue;
        Norm x; x.med = med; x.mad = mad; x.n = (int)n; N.m[f[1]][slot] = x;
    }
    if (!ver || !mk) { out = Norms(); out.why = !ver ? "no VERSION line" : "no MARKET line"; return false; }
    N.ok = !N.m.empty(); N.why = N.ok ? "file" : "file has no NORM rows"; out = N; return true;
}
// robust z vs the normal: (x - median) / (1.4826 MAD); a zero MAD = +/-99 off the median, 0 on it
inline double zOf(double x, const Norm& n) {
    if (!n.ok() || !finite(x)) return 0;
    if (n.mad <= 0) return x > n.med ? 99 : x < n.med ? -99 : 0;
    return (x - n.med) / (1.4826 * n.mad);
}
inline std::string ratioText(double r) { char b[24]; if (!finite(r) || r < 0) return "-"; if (r >= 9.95) std::snprintf(b, sizeof(b), "%.0fx", std::min(r, 99.0)); else std::snprintf(b, sizeof(b), "%.1fx", r); return b; }
inline std::string pctText(double s) { char b[16]; std::snprintf(b, sizeof(b), "%.0f%%", std::max(0.0, std::min(1.0, s)) * 100.0); return b; }
inline std::string clock12(int minuteOfDay) {
    if (minuteOfDay < 0) return "n/a";
    const int h = (minuteOfDay / 60) % 24, m = minuteOfDay % 60, h12 = h % 12 == 0 ? 12 : h % 12;
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h12, m, h < 12 ? "AM" : "PM"); return b;
}
// a level price for the line: the market's decimals, trailing ".00" dropped ("7750", "92.90", "1.1625")
inline std::string pxText(Tick t, double tick) {
    int d = 2; if (tick > 0) { d = 0; double x = tick; while (d < 8 && std::fabs(x - std::floor(x + 0.5)) > 1e-9) { x *= 10; ++d; } }
    char b[48]; std::snprintf(b, sizeof(b), "%.*f", d, (double)t * tick);
    std::string r = b; const size_t dot = r.find('.');
    if (dot != std::string::npos) { bool zero = true; for (size_t i = dot + 1; i < r.size(); ++i) if (r[i] != '0') zero = false; if (zero) r.erase(dot); }
    return r;
}
static const char* const DASH = " \x97 ";              // " - " as an em dash in IRT's cp1252 text

// ---- the read of one pane, as drawn ----------------------------------------------------------------------------------------
struct Line { bool show = false; std::string text; int impl = 0; std::vector<std::string> hover; bool locked = false; };

struct LevelZone { std::string name; Tick c = 0; int kind = 0; };
struct Ord { int side = BID; Tick px = 0; long long size = 0; Ms since = 0; bool lastFill = false; };
struct Dep { long long id = 0; int side = BID; Tick px = 0; long long size = 0; Ms left = 0; int how = 0; };   // how 0 cancelled, 1 traded, 2 moved
struct SnapOrd { long long id; int side; Tick px; long long size; };
struct Snap { Ms t = 0; std::vector<SnapOrd> o; };

struct Enc {
    int lvl = -1; Tick c = 0; std::string name; int kind = 0; bool support = true; int state = NONE;
    Ms tStart = 0, tArr = 0, tEnd = 0; int minArr = -1; long long wallArr = 0; bool partial = false, prePartial = false;
    std::unordered_map<long long, long long> cohort;     // id -> lots still resting of what was shown at arrival
    long long nOrders = 0, shown = 0, pulled = 0, filled = 0, moved = 0, added = 0, aggr = 0;
    long long preShown = -1, prePulled = 0, preFilled = 0, preMoved = 0;
    Tick pxEnd = 0; bool through = false;
    Line pre; bool arrWritten = false;                    // (1.1.0) the pre-touch read (hover fact for PullStats), the "arrived" row written
    Line out[P_N];                                        // the frozen lines (pre-touch frozen at arrival)
};

class Engine {
public:
    std::string market = "ES"; double tick = 0.25; Tick zoneHalf = 0, hw = 4; double adr = -1; int adrDays = 0;
    std::vector<LevelZone> lv; std::vector<char> armed, armSup; std::vector<long long> armWall;   // armWall = the arming close's bar end (s)
    std::unordered_map<long long, Ord> ord; std::deque<Dep> deps; std::deque<Snap> snaps;
    Enc cur; Tick last = 0; bool haveLast = false; Ms firstEventMs = -1, lastSnapMs = -1, lastTradeMs = -1;
    Norms norms;
    std::vector<std::string> recPending;                  // frozen read lines for the host to append (one per pane per encounter)
    bool arrRec = false;                                  // (1.1.0) this host records an "arrived" row (lsPullStats)
    struct Kept { Ms t = 0; long long wall = 0; Line out[P_N]; bool open = false; Line unknown; };   // (1.1.0) open = arrived, never locked (restart inside the window)
    std::map<Tick, Kept> kept;                           // level centre -> its last frozen read (this session; restored on load)
    long long encounters = 0, frozen = 0, orderDrops = 0;

    // ---- the levels (key / MenthorQ / gamma, merged) and the zone geometry: zone half-width = 0.06 ADR, band = the MEDIUM half-width
    void setGeometry(double adrPrice, int days, Tick bandHalf) {
        adr = adrPrice; adrDays = days; hw = bandHalf > 0 ? bandHalf : 1;
        zoneHalf = adrPrice > 0 && tick > 0 ? (Tick)std::llround(zrule::ZONE_W * adrPrice / tick) : 0;
        if (zoneHalf < hw) zoneHalf = adrPrice > 0 ? hw : 0;        // the zone is never narrower than the measurement band
    }
    void setLevels(const std::vector<zrule::Level>& L) {
        std::vector<LevelZone> nl; std::vector<char> na, ns; std::vector<long long> nw;
        for (size_t i = 0; i < L.size() && nl.size() < 64; ++i) {
            const double t = L[i].px / tick; if (!finite(t) || t <= 0 || t > 1e12) continue;
            LevelZone z; z.name = L[i].name; z.c = (Tick)std::llround(t); z.kind = L[i].kind;
            char a = 0, s = 0; long long w = 0;                         // a level that was already known keeps its arming
            for (size_t k = 0; k < lv.size(); ++k) if (lv[k].c == z.c) { a = armed[k]; s = armSup[k]; w = armWall[k]; }
            nl.push_back(z); na.push_back(a); ns.push_back(s); nw.push_back(w);
        }
        int keepLvl = -1;
        if (cur.state != NONE) for (size_t k = 0; k < nl.size(); ++k) if (nl[k].c == cur.c) keepLvl = (int)k;
        lv.swap(nl); armed.swap(na); armSup.swap(ns); armWall.swap(nw);
        if (cur.state != NONE) { if (keepLvl < 0 && cur.state != MEASURE) cur = Enc(); else if (keepLvl >= 0) cur.lvl = keepLvl; }
    }
    bool inBand(Tick px, Tick c) const { return px >= c - hw && px <= c + hw; }
    bool anyBand(Tick px) const { for (size_t i = 0; i < lv.size(); ++i) if (inBand(px, lv[i].c)) return true; return false; }
    int zoneOf(Tick px) const {                                  // the level whose zone holds px (nearest centre), -1 = none
        if (zoneHalf <= 0) return -1;
        int best = -1; Tick bd = 0;
        for (size_t i = 0; i < lv.size(); ++i) { const Tick d = std::llabs(px - lv[i].c); if (d <= zoneHalf && (best < 0 || d < bd)) { best = (int)i; bd = d; } }
        return best;
    }
    int defSide() const { return cur.support ? BID : ASK; }
    bool measuring() const { return cur.state == MEASURE; }
    bool inCurBand(Tick px) const { return cur.state != NONE && inBand(px, cur.c); }

    // ---- one order-by-order event (arrival clock t). action NEW / MOD / DEL / FILL; side = the RESTING order's side
    void onEvent(Ms t, int action, int side, long long id, Tick px, long long size) {
        if ((side != BID && side != ASK) || px <= 0 || size < 0 || size > 10000000 || action < NEW || action > FILL) return;
        if (firstEventMs < 0) firstEventMs = t;
        snapshot(t);                                              // every 15 s: the band book as it stood just before this event (pre-touch read)
        apply(t, action, side, id, px, size);
    }
private:
    void apply(Ms t, int action, int side, long long id, Tick px, long long size) {
        std::unordered_map<long long, Ord>::iterator it = ord.find(id);
        const bool known = it != ord.end();
        if (action == FILL) {
            last = px; haveLast = true; lastTradeMs = t;
            if (cur.state == APPROACH && inCurBand(px)) arrive(t);                  // the first trade at the level: the cohort is what rested before it
            if (known) { it->second.lastFill = true; it->second.size = std::max(0LL, it->second.size - size); }
            if (measuring()) {
                std::unordered_map<long long, long long>::iterator c = cur.cohort.find(id);
                if (c != cur.cohort.end()) { const long long f = std::min(size, c->second); cur.filled += f; c->second -= f; }
                if (side == defSide() && inCurBand(px)) cur.aggr += size;          // the attackers' lots traded into the level
            }
            return;
        }
        if (action == NEW) {
            if (!anyBand(px)) return;
            if (!known && ord.size() >= ORD_CAP) { orderDrops++; return; }
            Ord o; o.side = side; o.px = px; o.size = size; o.since = t; ord[id] = o;
            if (measuring() && side == defSide() && inCurBand(px)) cur.added += size;
            return;
        }
        if (action == MOD) {
            if (known) {
                Ord& o = it->second;
                if (px == o.px) {
                    const long long d = size - o.size;
                    if (d < 0 && !o.lastFill) cohortPull(id, -d);
                    if (d > 0 && measuring() && o.side == defSide() && inCurBand(px)) cur.added += d;
                    o.size = size; o.lastFill = false;
                    return;
                }
                // moved to another price: it left this price (the study's "moved", not a cancel)
                cohortMove(id);
                pushDep(id, o.side, o.px, o.size, t, 2);
                if (anyBand(px)) { o.px = px; o.size = size; o.since = t; o.lastFill = false;
                    if (measuring() && o.side == defSide() && inCurBand(px)) cur.added += size; }
                else ord.erase(it);
                return;
            }
            if (!anyBand(px)) return;
            if (ord.size() >= ORD_CAP) { orderDrops++; return; }
            Ord o; o.side = side; o.px = px; o.size = size; o.since = t; ord[id] = o;   // an order moved INTO a band (or first seen)
            if (measuring() && side == defSide() && inCurBand(px)) cur.added += size;
            return;
        }
        // DEL
        if (!known) return;
        const Ord o = it->second; ord.erase(it);
        if (o.lastFill) { cohortGone(id); pushDep(id, o.side, o.px, o.size, t, 1); }
        else { cohortPull(id, -1); pushDep(id, o.side, o.px, o.size, t, 0); }
    }
public:
    // the last trade price when no fill is seen (the chart's close)
    void onPrice(Ms t, Tick px) { if (px <= 0) return; last = px; haveLast = true; if (cur.state == APPROACH && inCurBand(px)) arrive(t); }
    // a CLOSED bar's close arms every level it closed outside the zone of (approach side = that close)
    void onBarClose(Tick close, long long barEndSec = 0) {
        if (close <= 0 || zoneHalf <= 0) return;
        for (size_t i = 0; i < lv.size(); ++i) {
            if (close > lv[i].c + zoneHalf) { armed[i] = 1; armSup[i] = 1; armWall[i] = barEndSec; }
            else if (close < lv[i].c - zoneHalf) { armed[i] = 1; armSup[i] = 0; armWall[i] = barEndSec; }
        }
    }
    // the clock: starts / freezes / clears the encounter. wallSec = the CT wall clock (s, for the record), minuteOfDay = CT minute
    void step(Ms t, long long wallSec, int minuteOfDay) {
        if (!haveLast) return;
        const int zi = zoneOf(last);
        if (cur.state == APPROACH && cur.lvl >= 0 && inCurBand(last)) arrive(t, wallSec, minuteOfDay);
        if (cur.state == MEASURE && cur.wallArr == 0) { cur.wallArr = wallSec; cur.minArr = minuteOfDay; cur.pre = preLine(); }
        if (cur.state == MEASURE && arrRec && !cur.arrWritten && cur.wallArr > 0) { recPending.push_back(recLineState(P_PULLSTATS, "arrived")); cur.arrWritten = true; }
        if (cur.state == MEASURE && t - cur.tArr >= WINDOW_MS) freeze(t);
        if (cur.state != NONE && cur.lvl != zi && cur.state != MEASURE) cur = Enc();          // price left the zone: the read ends
        if (cur.state == NONE && zi >= 0 && armed[(size_t)zi]) {
            // a read already locked AFTER this arming close (e.g. read back after a restart) is that same encounter: no new one
            std::map<Tick, Kept>::const_iterator k = kept.find(lv[(size_t)zi].c);
            if (k != kept.end() && k->second.wall > 0 && armWall[(size_t)zi] > 0 && k->second.wall >= armWall[(size_t)zi]) { armed[(size_t)zi] = 0; return; }
            Enc e; e.lvl = zi; e.c = lv[(size_t)zi].c; e.name = lv[(size_t)zi].name; e.kind = lv[(size_t)zi].kind; e.support = armSup[(size_t)zi] != 0;
            e.state = APPROACH; e.tStart = t; e.partial = firstEventMs < 0 || firstEventMs >= t;
            armed[(size_t)zi] = 0; cur = e; encounters++;
            if (inCurBand(last)) arrive(t, wallSec, minuteOfDay);
        }
    }
    // what pane p shows now
#ifdef ZREAD_TEST_HOOK
    bool hookOn = false; Line hook;                            // host tests: a fixed line, to test placement / hover without a market
#endif
    Line line(int p, Ms t) const {
#ifdef ZREAD_TEST_HOOK
        if (hookOn) return hook;
#endif
        Line L; if (!haveLast || p < 0 || p >= P_N) return L;
        const int zi = zoneOf(last); if (zi < 0) return L;
        const LevelZone& z = lv[(size_t)zi];
        const std::string pfx = market + " " + pxText(z.c, tick) + DASH;
        if (cur.state != NONE && cur.lvl == zi) {
            const char* d = cur.support ? "bids" : "offers";
            if (p == P_PULLSTATS && cur.state != FROZEN) return L;            // (1.1.0) PullStats: nothing until the arrival read locks
            if (cur.state == APPROACH) { L.show = true; L.text = pfx + "level not reached yet ?"; L.hover = approachHover(); return L; }
            if (cur.state == MEASURE) {
                const long long el = std::max(0LL, (long long)(t - cur.tArr)) / 1000;
                char b[96]; std::snprintf(b, sizeof(b), "%s: measuring %lld:%02lld of 3:00 ?", d, el / 60, el % 60);
                L.show = true; L.text = pfx + b; L.hover = measureHover(p); return L;
            }
            if (cur.state == FROZEN) return cur.out[p];
        }
        std::map<Tick, Kept>::const_iterator k = kept.find(z.c);       // a read already locked for this level (this session)
        if (k != kept.end() && t - k->second.t <= RESTORE_MAX_MS) {
            if (p == P_PULLSTATS && k->second.open) return t - k->second.t >= WINDOW_MS ? k->second.unknown : L;   // (1.1.0) restarted inside the window
            if (k->second.out[p].show) return k->second.out[p];
        }
        return L;
    }
    // ---- record (one line per pane per frozen read): the host appends recPending to <Pane>\<MKT>-<sym>-<spb>-zoneread-<session>.csv
    static std::string header() { return "wall_arrival|minute|level|name|kind|side|zone_half|band_half|adr|state|pane|impl|text|orders|shown|pulled|filled|moved|added|aggr|pre_shown|pre_pulled|pre_filled|px_end|through|partial|version|pre_moved|pre_partial|pre_impl|pre_text"; }
    static const size_t OLD_FIELDS = 27, NEW_FIELDS = 31;   // (1.1.0) 1.0.0 rows have 27 fields; the four pre_* columns are appended
    static std::string clean(const std::string& s) { std::string o; for (char c : s) { if (o.size() >= 200) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
    std::string recLine(int p) const { return recLineState(p, "frozen"); }
    // (1.1.0) state "frozen" (one row per pane when the window closes) or "arrived" (lsPullStats only, at arrival: the pre-touch part)
    std::string recLineState(int p, const char* state) const {
        const bool arr = std::string(state) == "arrived";
        const Line& shown = arr ? cur.pre : cur.out[p];
        std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
        o << cur.wallArr << '|' << cur.minArr << '|' << (double)cur.c * tick << '|' << clean(cur.name) << '|' << cur.kind << '|' << (cur.support ? "support" : "resistance")
          << '|' << zoneHalf << '|' << hw << '|' << adr << '|' << state << '|' << paneName(p) << '|' << shown.impl << '|' << clean(shown.text)
          << '|' << cur.nOrders << '|' << cur.shown << '|' << cur.pulled << '|' << cur.filled << '|' << cur.moved << '|' << cur.added << '|' << cur.aggr
          << '|' << cur.preShown << '|' << cur.prePulled << '|' << cur.preFilled << '|' << (double)cur.pxEnd * tick << '|' << (cur.through ? 1 : 0)
          << '|' << (cur.partial ? 1 : 0) << '|' << VERSION
          << '|' << cur.preMoved << '|' << (cur.prePartial ? 1 : 0) << '|' << cur.pre.impl << '|' << clean(cur.pre.text);
        return o.str();
    }
    // a record line read back on load (this session): the level's locked read for pane p comes back exactly as drawn
    bool restore(const std::string& ln, int p, Ms nowMs, long long nowWallSec) {
        const std::vector<std::string> f = zrule::splitBar(ln);
        if (p == P_PULLSTATS) return restorePullStats(f, nowMs, nowWallSec);
        if (f.size() < OLD_FIELDS || f[10] != paneName(p) || f[9] != "frozen") return false;
        double px = 0, imp = 0, wa = 0; if (!zrule::numTok(f[2], px) || !zrule::numTok(f[11], imp) || !zrule::numTok(f[0], wa) || px <= 0) return false;
        const double t = px / tick; if (!finite(t) || t <= 0) return false;
        const Tick c = (Tick)std::llround(t);
        Kept& k = kept[c];
        const long long age = nowWallSec > 0 && wa > 0 ? (nowWallSec - (long long)wa) : 0;
        k.t = nowMs - (age > 0 ? age * 1000 : 0); k.wall = (long long)wa;
        Line L; L.show = true; L.locked = true; L.text = f[12]; L.impl = imp > 0 ? 1 : imp < 0 ? -1 : 0;
        L.hover.push_back(std::string(paneName(p)) + " read \xB7 " + f[3] + " \xB7 " + f[5] + " (locked, from this session's record)");
        double mn = 0; if (zrule::numTok(f[1], mn)) L.hover.push_back("Arrived " + clock12((int)mn) + "; the window closed 3 minutes later");
        L.hover.push_back("Shown " + f[14] + " \xB7 cancelled " + f[15] + " \xB7 traded " + f[16] + " \xB7 added " + f[18] + " \xB7 hit " + f[19] + " lots");
        k.out[p] = L;
        return true;
    }

private:
    void pushDep(long long id, int side, Tick px, long long size, Ms t, int how) {
        Dep d; d.id = id; d.side = side; d.px = px; d.size = size; d.left = t; d.how = how; deps.push_back(d);
        while (!deps.empty() && (deps.size() > DEP_CAP || t - deps.front().left > DEP_KEEP_MS)) deps.pop_front();
    }
    void cohortPull(long long id, long long lots) {       // lots < 0 = the whole rest
        if (!measuring()) return;
        std::unordered_map<long long, long long>::iterator c = cur.cohort.find(id); if (c == cur.cohort.end()) return;
        const long long p = lots < 0 ? c->second : std::min(lots, c->second);
        cur.pulled += p; c->second -= p; if (lots < 0 || c->second <= 0) cur.cohort.erase(c);
    }
    void cohortMove(long long id) {
        if (!measuring()) return;
        std::unordered_map<long long, long long>::iterator c = cur.cohort.find(id); if (c == cur.cohort.end()) return;
        cur.moved += c->second; cur.cohort.erase(c);
    }
    void cohortGone(long long id) { if (measuring()) cur.cohort.erase(id); }
    void snapshot(Ms t) {
        if (lastSnapMs >= 0 && t - lastSnapMs < SNAP_MS && t >= lastSnapMs) return;
        lastSnapMs = t;
        Snap s; s.t = t; s.o.reserve(ord.size());
        for (std::unordered_map<long long, Ord>::const_iterator i = ord.begin(); i != ord.end(); ++i) { SnapOrd x = { i->first, i->second.side, i->second.px, i->second.size }; s.o.push_back(x); }
        snaps.push_back(s);
        while (snaps.size() > SNAP_CAP || (!snaps.empty() && t - snaps.front().t > PRE_MS + 4 * SNAP_MS)) snaps.pop_front();
    }
    void arrive(Ms t, long long wallSec = 0, int minuteOfDay = -1) {
        cur.state = MEASURE; cur.tArr = t; cur.wallArr = wallSec; cur.minArr = minuteOfDay;
        cur.partial = cur.partial || firstEventMs < 0 || firstEventMs > t - 1000;
        const int s = defSide();
        for (std::unordered_map<long long, Ord>::const_iterator i = ord.begin(); i != ord.end(); ++i)
            if (i->second.side == s && inBand(i->second.px, cur.c) && i->second.size > 0) { cur.cohort[i->first] = i->second.size; cur.shown += i->second.size; cur.nOrders++; }
        // pre-touch: the band's orders 5 minutes before arrival (the newest snapshot at least 5 minutes old)
        const Snap* sp = nullptr;
        for (size_t k = 0; k < snaps.size(); ++k) if (t - snaps[k].t >= PRE_MS) sp = &snaps[k];
        cur.prePartial = !sp || firstEventMs < 0 || (sp && firstEventMs > sp->t);
        if (sp) {
            cur.preShown = 0;
            std::unordered_map<long long, int> how;              // the last departure of each id after the snapshot
            for (size_t k = 0; k < deps.size(); ++k) if (deps[k].left > sp->t && deps[k].left <= t) how[deps[k].id] = deps[k].how;
            for (size_t k = 0; k < sp->o.size(); ++k) {
                const SnapOrd& o = sp->o[k]; if (o.side != s || !inBand(o.px, cur.c) || o.size <= 0) continue;
                cur.preShown += o.size;
                std::unordered_map<long long, Ord>::const_iterator now = ord.find(o.id);
                if (now != ord.end() && now->second.px == o.px) continue;          // still resting there
                std::unordered_map<long long, int>::const_iterator h = how.find(o.id);
                if (h == how.end()) continue;
                if (h->second == 0) cur.prePulled += o.size; else if (h->second == 1) cur.preFilled += o.size; else cur.preMoved += o.size;
            }
        }
        cur.pre = preLine();                                      // the pre-touch read is complete at arrival (1.1.0: a hover fact, recorded)
    }
    std::string pfx() const { return pfxOf(cur); }
    std::string pfxOf(const Enc& c) const { return market + " " + pxText(c.c, tick) + DASH; }
    std::string where() const { return whereOf(cur); }
    std::string whereOf(const Enc& c) const {
        const char* k = c.kind == 0 ? "key" : c.kind == 1 ? "MenthorQ" : "gamma";
        return market + " " + pxText(c.c, tick) + " " + c.name + " (" + k + ") \xB7 " + (c.support ? "support (price came down)" : "resistance (price came up)");
    }
    std::string slotText(const Norm& n) const { return slotTextOf(cur, n); }
    std::string slotTextOf(const Enc& c, const Norm& n) const {
        if (!n.ok()) return "no normal yet";
        if (!n.slot || c.minArr < 0) return "this market's normal (all slots, n " + std::to_string(n.n) + ")";
        const int a = (c.minArr / 30) * 30;
        return "the " + clock12(a) + " - " + clock12(a + 30) + " normal (n " + std::to_string(n.n) + ")";
    }
    Line preLine() const {
        Line L; L.show = true; L.locked = true;
        const char* d = cur.support ? "bids" : "offers";
        const Norm n = norms.get("pull_app", cur.minArr);
        L.hover.push_back("PullStats pre-touch read \xB7 " + where());
        L.hover.push_back("What it is: how much of the size resting at the level 5 minutes before price arrived was CANCELLED before it got there");
        L.hover.push_back("Why it matters: displayed size that leaves before the touch is size that will not be there to trade against");
        if (cur.preShown <= 0 || cur.prePartial) {
            L.text = pfx() + (cur.prePartial ? std::string("pre-touch book not known (indicator loaded < 5 min before) ?") : std::string("no ") + d + " resting at the level 5 min before ?");
            L.impl = 0;
        } else {
            const double sh = (double)cur.prePulled / (double)cur.preShown, z = zOf(sh, n);
            const bool hi = n.ok() && z >= Z_LINE;
            if (hi) { L.text = pfx() + d + " pulled before touch (" + pctText(sh) + ") ?"; L.impl = cur.support ? -1 : 1; }
            else { L.text = pfx() + d + " stayed before touch (" + pctText(sh) + " pulled) ?"; L.impl = 0; }
            char b[200]; std::snprintf(b, sizeof(b), "5 min before: %lld lots resting \xB7 cancelled %lld (%s) \xB7 traded %lld \xB7 moved %lld", cur.preShown, cur.prePulled, pctText(sh).c_str(), cur.preFilled, cur.preMoved);
            L.hover.push_back(b);
            if (n.ok()) L.hover.push_back("Normal cancelled share: " + pctText(n.med) + " (" + slotText(n) + "); this one z " + zText(z) + " (line z 1.0)");
            else L.hover.push_back("Normal: " + slotText(n) + " - shown as a share only");
        }
        L.hover.push_back("Use: context only - the study found no clear sign yet; your trigger is still RB / CoB after the sweep");
        L.hover.push_back("Example: ES 7750 - 45% of the bids there 5 min earlier were cancelled before price came down to it");
        L.hover.push_back("Study 10-14 (MBO, 22 days): pre-touch pulling showed NO clear sign yet (+7 pp breaks, interval includes 0) ?");
        return L;
    }
    static std::string zText(double z) { char b[16]; std::snprintf(b, sizeof(b), "%+.1f", std::max(-99.0, std::min(99.0, z))); return b; }
    std::vector<std::string> approachHover() const {
        std::vector<std::string> h;
        h.push_back("In the zone of " + where());
        char b[160]; std::snprintf(b, sizeof(b), "Zone = level +/- 0.06 ADR (%s); the read starts at the first trade within %lld ticks of the level", pxRange().c_str(), (long long)hw);
        h.push_back(b);
        return h;
    }
    std::string pxRange() const { return pxText(cur.c - zoneHalf, tick) + " - " + pxText(cur.c + zoneHalf, tick); }
    std::vector<std::string> measureHover(int p) const {
        std::vector<std::string> h;
        h.push_back(std::string(paneName(p)) + " read \xB7 " + where());
        h.push_back("Measuring the first 3 minutes after price reached the level (arrived " + clock12(cur.minArr) + "); the line locks when they end");
        return h;
    }
    void freeze(Ms t) {
        cur.state = FROZEN; cur.tEnd = t; cur.pxEnd = last;
        cur.through = cur.support ? last < cur.c - hw : last > cur.c + hw;
        cur.out[P_PULLING] = pullingLine(); cur.out[P_STACK] = stackLine(); cur.out[P_BAS] = basLine();
        cur.out[P_PULLSTATS] = pullStatsLineOf(cur);                 // (1.1.0) the arrival read, with the pre-touch part in the hover
        for (int p = 0; p < P_N; ++p) { cur.out[p].locked = true; recPending.push_back(recLine(p)); }
        Kept& k = kept[cur.c]; k.t = t; k.wall = cur.wallArr; for (int p = 0; p < P_N; ++p) k.out[p] = cur.out[p];
        frozen++;
    }
    std::string partialNote() const { return partialNoteOf(cur); }
    std::string partialNoteOf(const Enc& c) const { return c.partial ? "Partial: the indicator (order feed) started after price came into the zone - orders resting before are unknown" : "Orders = those the order feed has seen (an order resting since before the indicator loaded is unknown)"; }
    Line pullingLine() const { return pullingLineOf(cur); }
    Line pullingLineOf(const Enc& c) const {
        Line L; L.show = true;
        const char* d = c.support ? "bids" : "offers"; const char* lev = c.support ? "support" : "resistance";
        const Norm np = norms.get("pull_def", c.minArr), nf = norms.get("fill_def", c.minArr);
        L.hover.push_back("Pulling read \xB7 " + whereOf(c));
        L.hover.push_back("What it is: the size resting AT the level when price arrived - in the next 3 minutes, was it cancelled or traded?");
        L.hover.push_back("Why it matters: size that is cancelled instead of traded is not defending the level any more");
        L.hover.push_back("Arrived " + clock12(c.minArr) + ", locked 3 minutes later at " + clock12(c.minArr < 0 ? -1 : (c.minArr + 3) % 1440));
        if (c.shown <= 0) { L.text = pfxOf(c) + "no " + d + " seen at the level at arrival ?"; L.impl = 0; L.hover.push_back(partialNoteOf(c)); addPullCaveat(L); return L; }
        const double p = (double)c.pulled / (double)c.shown, f = (double)c.filled / (double)c.shown;
        const double zp = zOf(p, np); const double fx = nf.ok() && nf.med > 0 ? (double)c.filled / nf.med : -1;
        char b[220];
        std::snprintf(b, sizeof(b), "%s resting at the level at arrival: %lld lots (%lld orders)", c.support ? "Bids" : "Offers", c.shown, c.nOrders); L.hover.push_back(b);
        std::snprintf(b, sizeof(b), "In 3 min: traded %lld lots (%s) \xB7 cancelled %lld (%s) \xB7 moved away %lld", c.filled, pctText(f).c_str(), c.pulled, pctText(p).c_str(), c.moved); L.hover.push_back(b);
        if (np.ok()) L.hover.push_back("Normal cancelled share " + pctText(np.med) + " (" + slotTextOf(c, np) + "): this one z " + zText(zp) + " (line z 1.0)");
        if (nf.ok() && nf.med > 0) L.hover.push_back("Normal traded " + std::to_string((long long)std::llround(nf.med)) + " lots (" + slotTextOf(c, nf) + "): this one " + ratioText(fx));
        if (!np.ok()) { L.text = pfxOf(c) + d + ": " + pctText(p) + " pulled, " + pctText(f) + " traded in 3 min (no normal yet) ?"; L.impl = 0; }
        else if (zp >= Z_LINE) { L.text = pfxOf(c) + d + " pulled, not filled (" + pctText(p) + " in 3 min)" + DASH + "less " + lev + " ?"; L.impl = c.support ? -1 : 1; }
        else if (fx >= 1.0 && !c.through) { L.text = pfxOf(c) + d + " traded, held (" + ratioText(fx) + " normal)" + DASH + lev + " holding ?"; L.impl = c.support ? 1 : -1; }
        else if (fx >= 1.0 && c.through) { L.text = pfxOf(c) + d + " traded (" + ratioText(fx) + " normal), price through ?"; L.impl = c.support ? -1 : 1; }
        else { L.text = pfxOf(c) + d + ": " + pctText(p) + " pulled, " + pctText(f) + " traded in 3 min ?"; L.impl = 0; }
        L.hover.push_back(partialNoteOf(c));
        addPullCaveat(L);
        return L;
    }
    void addPullCaveat(Line& L) const {
        L.hover.push_back("Use: heavy cancelling = give the level less weight; traded and held = the size stood; either way wait for RB / CoB");
        L.hover.push_back("Example: ES 7750 - 62% of the bids there at arrival cancelled in 3 min, few filled -> less support ?");
        L.hover.push_back("Study 10-14 (MBO, 22 days, Q2v): top-of-normal cancelling -> the zone held about 19 pp less often (-19 / -5 / -13 pp");
        L.hover.push_back("  build / validate / forward with attack pace held) - an association, NOT YET proven; first live forward days pending ?");
    }
    // ---- (1.1.0) lsPullStats: ONE line = the arrival read, shown only once locked; the pre-touch share is a secondary hover fact ------
    std::string slotShort(const Enc& c, const Norm& n) const {   // "9:00 AM - 9:30 AM, n 31" / "all slots, n 120"
        if (!n.slot || c.minArr < 0) return "all slots, n " + std::to_string(n.n);
        const int a = (c.minArr / 30) * 30; return clock12(a) + " - " + clock12(a + 30) + ", n " + std::to_string(n.n);
    }
    std::string preFactOf(const Enc& c) const {
        const char* d = c.support ? "bids" : "offers";
        if (c.preShown < 0 || c.prePartial) return "Before the touch (secondary): not known - the order feed started less than 5 minutes before price arrived";
        if (c.preShown == 0) return std::string("Before the touch (secondary): no ") + d + " resting at the level 5 minutes before";
        const double sh = (double)c.prePulled / (double)c.preShown; const Norm n = norms.get("pull_app", c.minArr);
        char b[220];
        std::snprintf(b, sizeof(b), "Before the touch (secondary): %s of the %lld lots of %s there 5 min earlier were cancelled", pctText(sh).c_str(), c.preShown, d);
        std::string o = b;
        if (n.ok()) o += " (normal " + pctText(n.med) + ", z " + zText(zOf(sh, n)) + ")";
        return o;
    }
    // the hover rows are kept short (<= ~115 characters) and in order of importance: the pane shows as many as fit
    Line pullStatsLineOf(const Enc& c) const {
        const Line A = pullingLineOf(c);                       // the same words / colour rule as lsPulling 0.3.0's arrival read
        Line L; L.show = true; L.locked = true; L.text = A.text; L.impl = A.impl;
        const char* d = c.support ? "bids" : "offers";
        const Norm np = norms.get("pull_def", c.minArr), nf = norms.get("fill_def", c.minArr);
        L.hover.push_back("PullStats read \xB7 " + whereOf(c));
        char b[220];
        if (c.shown > 0) {
            const double p = (double)c.pulled / (double)c.shown, f = (double)c.filled / (double)c.shown;
            std::snprintf(b, sizeof(b), "At arrival (%s): %lld lots of %s at the level (%lld orders)", clock12(c.minArr).c_str(), c.shown, d, c.nOrders); L.hover.push_back(b);
            std::snprintf(b, sizeof(b), "In 3 min: cancelled %lld (%s) \xB7 traded %lld (%s) \xB7 moved away %lld", c.pulled, pctText(p).c_str(), c.filled, pctText(f).c_str(), c.moved); L.hover.push_back(b);
            std::string nm;
            if (np.ok()) nm = "cancelled " + pctText(np.med) + " - this one z " + zText(zOf(p, np)) + " (line z 1.0)";
            if (nf.ok() && nf.med > 0) nm += std::string(nm.empty() ? "" : " \xB7 ") + "traded " + std::to_string((long long)std::llround(nf.med)) + " lots - this one " + ratioText((double)c.filled / nf.med);
            L.hover.push_back(nm.empty() ? std::string("Normal: no normal yet - shown as shares only") : "Normal (" + slotShort(c, np.ok() ? np : nf) + "): " + nm);
        } else {
            L.hover.push_back(std::string("At arrival (") + clock12(c.minArr) + "): the order feed saw no " + d + " resting at the level");
        }
        L.hover.push_back(preFactOf(c));
        L.hover.push_back("What it is: was the size resting AT the level cancelled or traded in the 3 min after price arrived? (locks at 3:00)");
        L.hover.push_back("Why it matters: cancelled size no longer defends the level; size that trades while price holds stood its ground");
        L.hover.push_back("Use: heavy cancelling = give the level less weight; traded and held = the size stood; either way wait for RB / CoB");
        L.hover.push_back("Example: ES 7750 - 62% of the bids there at arrival cancelled in 3 min, few filled -> less support ?");
        L.hover.push_back("Study 10-14 (MBO, 22 days): top-of-normal cancelling -> held about 19 pp less often - association only, NOT YET proven ?");
        L.hover.push_back("Study 10-14: pre-touch pulling showed NO clear sign yet (+7 pp breaks, interval includes 0) ?");
        L.hover.push_back(c.partial ? "Partial: the order feed started after price came into the zone - orders resting before are unknown" : "Orders = those the order feed has seen (one resting since before the indicator loaded is unknown)");
        return L;
    }
    Line unknownLineOf(const Enc& c) const {                   // restarted inside the 3 minutes: the cohort could not be followed
        Line L; L.show = true; L.locked = true; L.impl = 0;
        L.text = pfxOf(c) + (c.support ? "bids" : "offers") + ": arrival read not known (restarted during the 3 min) ?";
        L.hover.push_back("PullStats read \xB7 " + whereOf(c));
        L.hover.push_back("Price arrived " + clock12(c.minArr) + "; the indicator restarted before the 3 minutes ended, so which resting orders were cancelled or traded is not known");
        L.hover.push_back(preFactOf(c));
        L.hover.push_back("Study 10-14 (MBO, 22 days): top-of-normal cancelling -> held about 19 pp less often - association only, NOT YET proven ?");
        return L;
    }
public:
    // (1.1.0) a PullStats record row read back: "frozen" = the locked line (1.1.0 rows: the text as drawn; 1.0.0 rows: rebuilt as the
    // arrival read from the row's own numbers), "arrived" = an encounter that never locked here (restart inside the window)
    bool restorePullStats(const std::vector<std::string>& f, Ms nowMs, long long nowWallSec) {
        if (f.size() < OLD_FIELDS || f[10] != paneName(P_PULLSTATS) || (f[9] != "frozen" && f[9] != "arrived")) return false;
        const bool fresh = f.size() >= NEW_FIELDS, arrived = f[9] == "arrived";
        if (arrived && !fresh) return false;
        double v[OLD_FIELDS] = {0}; const int need[] = { 0, 1, 2, 4, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 };
        for (size_t i = 0; i < sizeof(need) / sizeof(need[0]); ++i) if (!zrule::numTok(f[(size_t)need[i]], v[need[i]])) return false;
        if (v[2] <= 0) return false;
        const double tt = v[2] / tick; if (!finite(tt) || tt <= 0 || tt > 1e12) return false;
        Enc c; c.c = (Tick)std::llround(tt); c.name = f[3]; c.kind = (int)v[4]; c.support = f[5] == "support";
        c.wallArr = (long long)v[0]; c.minArr = (int)v[1]; c.nOrders = (long long)v[13]; c.shown = (long long)v[14]; c.pulled = (long long)v[15];
        c.filled = (long long)v[16]; c.moved = (long long)v[17]; c.added = (long long)v[18]; c.aggr = (long long)v[19];
        c.preShown = (long long)v[20]; c.prePulled = (long long)v[21]; c.preFilled = (long long)v[22];
        c.pxEnd = (Tick)std::llround(v[23] / tick); c.through = v[24] != 0; c.partial = v[25] != 0; c.prePartial = c.preShown < 0;
        if (fresh) { double pm = 0, pp = 0; if (zrule::numTok(f[27], pm)) c.preMoved = (long long)pm; if (zrule::numTok(f[28], pp)) c.prePartial = pp != 0; }
        if (c.minArr < -1 || c.minArr >= 1440 || c.shown < 0 || c.pulled < 0 || c.filled < 0) return false;
        Kept& k = kept[c.c];
        if (arrived && !k.open && k.wall == c.wallArr && k.out[P_PULLSTATS].show) return true;   // already locked (rows are in time order)
        const long long age = nowWallSec > 0 && c.wallArr > 0 ? (nowWallSec - c.wallArr) : 0;
        k.t = nowMs - (age > 0 ? age * 1000 : 0); k.wall = c.wallArr;
        if (arrived) { k.open = true; k.unknown = unknownLineOf(c); k.out[P_PULLSTATS] = Line(); return true; }
        Line L = pullStatsLineOf(c);
        if (fresh) { L.text = f[12]; L.impl = v[11] > 0 ? 1 : v[11] < 0 ? -1 : 0; }
        k.open = false; k.out[P_PULLSTATS] = L;
        return true;
    }
private:
    Line stackLine() const {
        Line L; L.show = true;
        const char* d = cur.support ? "bids" : "offers";
        const Norm n = norms.get("added", cur.minArr);
        L.hover.push_back("StackStats read \xB7 " + where());
        L.hover.push_back("What it is: new size ADDED on the defending side at the level in the 3 minutes after price arrived");
        L.hover.push_back("Why it matters: stacking shows displayed size arriving - but on its own it mostly reflects a WEAK attack");
        const double x = n.ok() && n.med > 0 ? (double)cur.added / n.med : -1, z = zOf((double)cur.added, n);
        char b[200]; std::snprintf(b, sizeof(b), "Added at the level in 3 min: %lld lots (arrived %s)", cur.added, clock12(cur.minArr).c_str()); L.hover.push_back(b);
        if (n.ok() && n.med > 0) L.hover.push_back("Normal added " + std::to_string((long long)std::llround(n.med)) + " lots (" + slotText(n) + "): this one " + ratioText(x) + ", z " + zText(z));
        if (cur.added <= 0) { L.text = pfx() + "no new " + d + " ?"; L.impl = 0; }
        else if (!n.ok() || n.med <= 0) { L.text = pfx() + d + " added " + std::to_string(cur.added) + " lots (no normal yet) ?"; L.impl = 0; }
        else if (z >= Z_LINE) { L.text = pfx() + d + " stacking " + ratioText(x) + " normal ?"; L.impl = cur.support ? 1 : -1; }
        else { L.text = pfx() + d + " added " + ratioText(x) + " normal ?"; L.impl = 0; }
        L.hover.push_back(partialNote());
        L.hover.push_back("Use: read it WITH BidAsk - stacking against a heavy attack means more than stacking against a quiet one");
        L.hover.push_back("Example: ES 7750 - 2.3x the normal bids added while sellers hit 3x normal and price held");
        L.hover.push_back("Study 10-14 (Q3): stacking / refill size on its own mostly reflects a weak attack - NO SIGN once attack pace is held ?");
        return L;
    }
    Line basLine() const {
        Line L; L.show = true;
        const char* att = cur.support ? "sellers hitting" : "buyers lifting";
        const Norm n = norms.get("aggr", cur.minArr);
        L.hover.push_back("BidAskStats read \xB7 " + where());
        L.hover.push_back(std::string("What it is: the attackers' lots traded into the level in the 3 minutes after price arrived, and where price is at the end"));
        L.hover.push_back("Why it matters: heavy hitting with price still at the level = the size there traded and stood (absorption-like)");
        const double x = n.ok() && n.med > 0 ? (double)cur.aggr / n.med : -1, z = zOf((double)cur.aggr, n);
        char b[220]; std::snprintf(b, sizeof(b), "%s into the level in 3 min: %lld lots \xB7 price at the end %s (%s)", cur.support ? "Sold" : "Bought", cur.aggr,
                                   pxText(cur.pxEnd, tick).c_str(), cur.through ? "beyond the level band" : "at / before the level band"); L.hover.push_back(b);
        if (n.ok() && n.med > 0) L.hover.push_back("Normal " + std::to_string((long long)std::llround(n.med)) + " lots (" + slotText(n) + "): this one " + ratioText(x) + ", z " + zText(z));
        const std::string held = cur.through ? "price through" : "price held";
        if (cur.aggr <= 0) { L.text = pfx() + "no " + (cur.support ? "selling" : "buying") + " into the level, " + held + " ?"; L.impl = 0; }
        else if (!n.ok() || n.med <= 0) { L.text = pfx() + att + " " + std::to_string(cur.aggr) + " lots, " + held + " (no normal yet) ?"; L.impl = cur.through ? (cur.support ? -1 : 1) : 0; }
        else if (cur.through) { L.text = pfx() + att + " " + ratioText(x) + ", price through ?"; L.impl = cur.support ? -1 : 1; }
        else if (z >= Z_LINE) { L.text = pfx() + att + " " + ratioText(x) + " normal, price held ?"; L.impl = cur.support ? 1 : -1; }
        else { L.text = pfx() + att + " " + ratioText(x) + " normal, price held ?"; L.impl = 0; }
        L.hover.push_back(partialNote());
        L.hover.push_back("Use: heavy hitting + held = watch for your RB / CoB; through = the level gave way (the zone rule still decides a break)");
        L.hover.push_back("Example: ES 7750 - sellers hit 3.1x the normal into the bids and price stayed at the level");
        L.hover.push_back("Study 10-14 (Q5): heavy trading into a zone alone leaned toward breaks (-16 pp, not significant); untested as a signal ?");
        return L;
    }
};


// ---- the host side, still SDK-free: files (levels, normals, the record), the chart's bars (ADR, closes), the session ------------
#if defined(_WIN32)
inline long long fileSig(const std::string& path) { struct _stat64 st; if (_stat64(path.c_str(), &st) != 0) return -1; if ((st.st_mode & S_IFMT) != S_IFREG) return -1; return (long long)st.st_mtime * 1000003LL + (long long)st.st_size; }
inline long long fileMtime(const std::string& path) { struct _stat64 st; if (_stat64(path.c_str(), &st) != 0) return -1; return (long long)st.st_mtime; }
#else
inline long long fileSig(const std::string& path) { struct stat st; if (stat(path.c_str(), &st) != 0) return -1; if ((st.st_mode & S_IFMT) != S_IFREG) return -1; return (long long)st.st_mtime * 1000003LL + (long long)st.st_size; }
inline long long fileMtime(const std::string& path) { struct stat st; if (stat(path.c_str(), &st) != 0) return -1; return (long long)st.st_mtime; }
#endif
inline bool readSmall(const std::string& path, std::string& out, size_t cap = 256 * 1024) {
    out.clear(); std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    char buf[4096]; while (in) { in.read(buf, sizeof(buf)); const std::streamsize n = in.gcount(); if (n <= 0) break; if (out.size() + (size_t)n > cap) { out.clear(); return false; } out.append(buf, (size_t)n); }
    return true;
}
struct BarRow { long long session = 0, endSec = 0; double h = 0, l = 0, c = 0; };
struct Host {
    Engine e; int pane = P_PULLING;
    long long lvChecked = -1, nmChecked = -1, sKey = -2, sMq = -2, sGex = -2, sNm = -2;
    std::vector<zrule::Level> key, mq, gex;
    std::string lvNote = "not read", nmNote = "not read", adrNote = "no bars";
    long long lastClosedEnd = 0, session = -1, barsSig = -1; double adrCached = -1; int adrDays = 0;
    std::string recPath; long long recWrites = 0, recFails = 0, restored = 0;
    // levels every 10 s, normals every 60 s (re-read only when the file changed); dir ends with the path separator
    void files(const std::string& dir, const std::string& market, long long nowMs, long long nowUnix) {
        e.market = market; e.arrRec = pane == P_PULLSTATS;      // (1.1.0) only lsPullStats records the "arrived" row
        if (dir.empty()) { lvNote = nmNote = "no level folder"; return; }
        if (lvChecked < 0 || nowMs - lvChecked >= 10000 || nowMs < lvChecked) {
            lvChecked = nowMs; bool ch = false; std::string t;
            const std::string pk = dir + "LRA-KeyLevels-" + market + ".csv", pm = dir + "LRA-Levels-" + market + ".csv", pg = dir + "LRA-GexNodes-" + market + ".csv";
            long long g = fileSig(pk);
            if (g != sKey) { sKey = g; std::vector<zrule::Level> v; if (g >= 0 && readSmall(pk, t) && zrule::parseKey(t, nowUnix, 18 * 3600, v)) key = v; else if (g < 0) key.clear(); ch = true; }
            g = fileSig(pm);
            if (g != sMq) { sMq = g; std::vector<zrule::Level> v; const long long mt = fileMtime(pm);
                if (g >= 0 && mt > 0 && nowUnix - mt <= 18 * 3600 && readSmall(pm, t) && zrule::parseMq(t, market, v)) mq = v; else mq.clear(); ch = true; }
            g = fileSig(pg);
            if (g != sGex) { sGex = g; std::vector<zrule::Level> v; if (g >= 0 && readSmall(pg, t) && zrule::parseGex(t, market, nowUnix, 18 * 3600, v)) gex = v; else if (g < 0) gex.clear(); ch = true; }
            if (ch) {
                e.setLevels(zrule::mergeLevels(key, mq, gex, (double)e.hw * e.tick));
                lvNote = std::to_string(key.size()) + " key, " + std::to_string(mq.size()) + " MenthorQ, " + std::to_string(gex.size()) + " gamma -> " + std::to_string(e.lv.size()) + " levels";
            }
        }
        if (nmChecked < 0 || nowMs - nmChecked >= 60000 || nowMs < nmChecked) {
            nmChecked = nowMs;
            const std::string pn = dir + "LRA-ZoneReadNorms-" + market + ".csv"; const long long g = fileSig(pn);
            if (g != sNm) { sNm = g; std::string t; Norms n;
                if (g < 0) { e.norms = Norms(); e.norms.why = "no file"; }
                else if (readSmall(pn, t) && parseNorms(t, market, n)) e.norms = n;
                else { e.norms.why = n.why.empty() ? "unreadable" : n.why; }    // garbled: keep the last good normals, say why
                nmNote = e.norms.why; }
        }
    }
    // every CLOSED bar of the chart (ascending) + the session in progress: the lagged ADR (prior sessions only) and new closes arm levels
    void bars(const std::vector<BarRow>& closed, long long curSession, int bandHalf) {
        if (closed.empty()) return;
        const long long sig = (long long)closed.size() * 1000003LL + closed.back().endSec + curSession * 7;
        if (sig != barsSig) {
            barsSig = sig;
            zrule::Adr a; for (size_t i = 0; i < closed.size(); ++i) a.add(closed[i].session, closed[i].h, closed[i].l, closed[i].c);
            int n = 0; double adr = a.adrFor(curSession, n, zrule::ADR_MIN); bool shortAdr = false;
            if (adr <= 0) { adr = a.adrFor(curSession, n, 5); shortAdr = adr > 0; }          // fewer than 10 sessions loaded: 5+ (labelled)
            adrCached = adr; adrDays = n;
            adrNote = adr > 0 ? (std::to_string(n) + " sessions" + (shortAdr ? " (short: the study needs 10)" : "")) : "under 5 sessions on the chart - no zones";
        }
        e.setGeometry(adrCached, adrDays, bandHalf);
        for (size_t i = 0; i < closed.size(); ++i) {
            if (closed[i].endSec <= lastClosedEnd) continue;
            if (lastClosedEnd > 0 || i + 1 == closed.size()) { Tick c = 0; const double t = closed[i].c / e.tick; if (finite(t) && t > 0) { c = (Tick)std::llround(t); e.onBarClose(c, closed[i].endSec); } }
        }
        lastClosedEnd = closed.back().endSec;
    }
    // a new session: the old reads are gone; this session's record (if any) is read back so a restart shows the same locked reads
    void newSession(long long s, const std::string& path, long long nowMs, long long nowWall) {
        if (s == session && path == recPath) return;
        session = s; recPath = path; e.kept.clear(); e.cur = Enc(); restored = 0; headerOk = false;   // (arming comes from closes: kept)
        std::string t; if (path.empty() || !readSmall(path, t, 4 * 1024 * 1024)) return;
        const std::vector<std::string> L = zrule::lines(t);
        for (size_t i = 1; i < L.size(); ++i) if (e.restore(L[i], pane, nowMs, nowWall)) restored++;
    }
    // (1.1.0, lesson 66) a session file written by 1.0.0 has the 27-column header: rewrite it once with the new header (atomic:
    // .tmp -> old to .bak -> rename -> .bak removed; the old file is put back on failure) before the first longer row is appended
    bool upgradeHeader() {
        std::string t; if (!readSmall(recPath, t, 4 * 1024 * 1024)) return false;
        const size_t nl = t.find('\n'); std::string first = t.substr(0, nl == std::string::npos ? t.size() : nl);
        if (!first.empty() && first[first.size() - 1] == '\r') first.erase(first.size() - 1);
        if (first == Engine::header()) return true;
        if (first.compare(0, 13, "wall_arrival|") != 0) return false;     // not ours: never touched
        const std::string body = nl == std::string::npos ? std::string() : t.substr(nl + 1);
        const std::string tmp = recPath + ".tmp", bak = recPath + ".bak";
        { std::ofstream o(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!o) return false; o << Engine::header() << "\n" << body; o.flush(); if (!o) { std::remove(tmp.c_str()); return false; } }
        std::remove(bak.c_str());
        if (std::rename(recPath.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; }
        if (std::rename(tmp.c_str(), recPath.c_str()) != 0) { std::rename(bak.c_str(), recPath.c_str()); std::remove(tmp.c_str()); return false; }
        std::remove(bak.c_str()); headerUpgrades++;
        return true;
    }
    long long headerUpgrades = 0; bool headerOk = false;
    void flush() {
        if (e.recPending.empty() || recPath.empty()) { if (recPath.empty()) e.recPending.clear(); return; }
        const bool fresh = fileSig(recPath) < 0;
        if (!fresh && !headerOk) { if (!upgradeHeader()) { recFails++; return; } headerOk = true; }
        std::ofstream f(recPath.c_str(), std::ios::binary | std::ios::app); if (!f) { recFails++; return; }
        if (fresh) f << Engine::header() << "\n";
        for (size_t i = 0; i < e.recPending.size(); ++i) f << e.recPending[i] << "\n";
        f.flush(); if (!f) { recFails++; return; }
        recWrites++; e.recPending.clear();
    }
    std::string status() const {
        std::ostringstream o; o.imbue(std::locale::classic());
        o << "ZONE_READ," << VERSION << ",pane " << paneName(pane) << ",state " << (e.cur.state == NONE ? "none" : e.cur.state == APPROACH ? "approach" : e.cur.state == MEASURE ? "measuring" : "locked")
          << ",encounters " << e.encounters << ",locked " << e.frozen << ",restored " << restored << ",record writes " << recWrites << " fails " << recFails << ",header upgrades " << headerUpgrades
          << "\nZONE_GEOMETRY,adr " << e.adr << " (" << adrNote << "),zone +/- " << e.zoneHalf << " ticks (0.06 ADR),band +/- " << e.hw << " ticks"
          << "\nZONE_LEVELS," << lvNote << "\nZONE_NORMS," << nmNote << "\nZONE_ORDERS,tracked " << e.ord.size() << ",drops " << e.orderDrops;
        return o.str();
    }
};

} // namespace zread
#endif
