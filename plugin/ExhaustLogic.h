#ifndef LS_EXHAUST_LOGIC_H
#define LS_EXHAUST_LOGIC_H
// ExhaustLogic.h 1.0.0 (2026-10-27, Rassul "ok lets build it. i will take your recommendation" = option A of ex_mockup.html) -- the Ex
// (EXHAUSTED) mark, SDK-free and unit-tested on Linux (tests_ex/test_ex.cpp). Host: lsLiquidityProfile 0.61.0 (its live order-by-order
// feed, OrderFlowFeed.h).
//
// THE TESTED DEFINITION (mbo_study.exh_at_zone; Rassul 17:49 CT "lets just use key price levels only" -> the key-level re-test
// mbo_marks_test/results_keylev.json: 43 marked, held 72% vs 62% control, matched +8 pts held / +14 swing, validation +14), copied exactly:
//   ZONES     KEY PRICE levels only (KEY_CODES: PDH/PDL, PFH/PFL, ONH/ONL, LonHI/LonLO, PWH/PWL, AsiaHI/AsiaLO, Open) - no volume / POC,
//             no gamma, no resting-order zones - merged when their zones overlap; half-width = clamp(round(k x ATR(3-min) / tick),
//             min, max) ticks, the MEDIUM variant per market (zoneCfg).
//   TOUCH     the previous closed candle was fully outside the zone and price now trades inside it (support when that candle closed
//             above the zone, else resistance). RTH only (the study's touches are RTH bars).
//   ARRIVAL   the first trade inside the zone; DECISION = arrival + 180 s.
//   MEASURE   per second from arrival (181 s): the last trade price (carried forward), "at the zone" = inside or within 1 tick;
//             ATTACKERS = sellers hitting bids at a support, buyers lifting offers at a resistance, their lots traded INSIDE the zone.
//             halves: seconds 0-89 / 90-180; s1 / s2 = seconds at the zone in each half (both >= 5), 1st-half attacker lots > 0;
//             exh = (lots2 / s2) / (lots1 / s1); share = seconds at the zone / 181.
//   Ex        exh <= 0.164 AND share >= 0.5 (the threshold picked on the build sessions, q 0.9).
// THE MARK   "Ex" (or "Ex?") on the DECISION candle (the candle that contains arrival + 180 s), drawn once that candle has closed:
//            red ABOVE at a resistance (buyers exhausted = bearish), green BELOW at a support (sellers exhausted = bullish). One per
//            candle. The decision (and its label) is written to the record and read back after a restart: never repainted.
// THE GATE   "Ex" only where the market's nightly params file (lsFlexLevels\LRA-Exhaust-<MKT>.csv, written by lra.ex_mark_params)
//            says n >= 30 tested KEY-LEVEL cases (min_n / min_lift columns, defaults 30 / 0); otherwise (or no file) "Ex?". With the
//            data of 2026-10-27 no market qualifies (EU 25 is closest), so every market shows "Ex?" for now.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace exh {
static const char* const VERSION = "1.0.0";
typedef long long Tick;
static const double THR = 0.164;            // exh at or below = exhausted
static const double SHARE_MIN = 0.5;        // price at the zone at least half the window
static const int DEC_S = 180;               // arrival -> decision (DEFENCE_S)
static const int NSEC = DEC_S + 1;          // the study's nsec = (td - ta) // 1 s + 1
static const int HALF = NSEC / 2;           // 90
static const int SECS_MIN = 5;              // seconds at the zone per half
static const long long GATE_N = 30;         // tested cases before "Ex"
static const double GATE_LIFT = 0.0;        // held - control before "Ex" (the file may raise it; the coordinator's gate is the count)
static const size_t ACTIVE_CAP = 64, DONE_CAP = 20000;
enum Side { SUP = 0, RES = 1 };             // support (bullish when exhausted) / resistance (bearish)
enum Att { BUYER = 0, SELLER = 1, NOATT = -1 };

// ---- per market ---------------------------------------------------------------------------------------------------------------
struct ZoneCfg { double k = 0.30; int lo = 4, hi = 6; int rthFrom = 8 * 60 + 30, rthTo = 15 * 60; bool ok = false; };
inline std::string studyCode(const std::string& m) { return m == "6E" || m == "EU" || m == "EC" ? std::string("EU") : m; }
inline ZoneCfg zoneCfg(const std::string& m0) {
    const std::string m = studyCode(m0); ZoneCfg z; z.ok = true;
    // mbo_fetch.ZONES medium; RTH = markets.MARKETS rth_first - 3 min .. rth_last (bar END stamps, minutes of the CT day)
    if (m == "ES")      { z.k = 0.30; z.lo = 4; z.hi = 6;  z.rthFrom = 8 * 60 + 30; z.rthTo = 15 * 60; }
    else if (m == "NQ") { z.k = 0.11; z.lo = 8; z.hi = 12; z.rthFrom = 8 * 60 + 30; z.rthTo = 15 * 60; }
    else if (m == "GC") { z.k = 0.25; z.lo = 8; z.hi = 15; z.rthFrom = 7 * 60 + 21; z.rthTo = 12 * 60 + 30; }
    else if (m == "HG") { z.k = 0.30; z.lo = 2; z.hi = 5;  z.rthFrom = 7 * 60 + 10; z.rthTo = 12 * 60 + 30; }
    else if (m == "CL") { z.k = 0.25; z.lo = 4; z.hi = 8;  z.rthFrom = 8 * 60;      z.rthTo = 13 * 60 + 30; }
    else if (m == "NG") { z.k = 0.30; z.lo = 2; z.hi = 4;  z.rthFrom = 8 * 60;      z.rthTo = 13 * 60 + 30; }
    else if (m == "EU") { z.k = 0.40; z.lo = 2; z.hi = 3;  z.rthFrom = 7 * 60 + 21; z.rthTo = 14 * 60; }
    else z.ok = false;
    return z;
}
// half-width in whole ticks from the 3-min ATR in ticks (no / bad ATR = 15 ticks, as half_width)
inline Tick halfWidth(const ZoneCfg& z, double atrTicks) {
    const double a = (std::isfinite(atrTicks) && atrTicks > 0) ? atrTicks : 15.0;
    double w = std::round(z.k * a); if (w < z.lo) w = z.lo; if (w > z.hi) w = z.hi;
    return (Tick)w;
}
// a chart's own ATR (any candle length) -> the 3-min one (volatility grows with the square root of time)
inline double atr3m(double atrTicks, long spb) { return spb > 0 && atrTicks > 0 ? atrTicks * std::sqrt(180.0 / (double)spb) : atrTicks; }
inline bool inRth(const ZoneCfg& z, long long ctMs) { const long long m = ((ctMs / 60000) % 1440 + 1440) % 1440; return m >= z.rthFrom && m < z.rthTo; }

// ---- zones ------------------------------------------------------------------------------------------------------------------
// his key price levels (KeyLevelsLogic.h codes); anything else (DPoc / WPoc / VAH / VAL / CWH / CWL / gamma) is not an Ex zone
inline bool isKeyCode(const std::string& c) {
    static const char* const K[] = { "PDH", "PDL", "PFH", "PFL", "ONH", "ONL", "LonHI", "LonLO", "PWH", "PWL", "AsiaHI", "AsiaLO", "Open" };
    for (const char* k : K) if (c == k) return true;
    return false;
}
struct Level { std::string name; Tick px = 0; };
struct Zone { std::string key, name; char kind = 'T'; Tick lo = 0, hi = 0; int side = -1; };
// merge_zones: sorted by price, a level whose zone overlaps the previous one joins it (names joined with '+', outer edges)
inline std::vector<Zone> mergeLevels(std::vector<Level> L, Tick hw) {
    std::sort(L.begin(), L.end(), [](const Level& a, const Level& b) { return a.px < b.px || (a.px == b.px && a.name < b.name); });
    std::vector<Zone> out;
    for (const Level& l : L) {
        if (l.px <= 0 || !isKeyCode(l.name)) continue;
        if (!out.empty() && l.px - hw <= out.back().hi) {
            Zone& z = out.back(); z.hi = std::max(z.hi, l.px + hw);
            if (("+" + z.name + "+").find("+" + l.name + "+") == std::string::npos && !l.name.empty()) z.name += (z.name.empty() ? "" : "+") + l.name;
        } else { Zone z; z.kind = 'T'; z.name = l.name.empty() ? std::string("lvl") : l.name; z.lo = l.px - hw; z.hi = l.px + hw; out.push_back(z); }
    }
    for (Zone& z : out) z.key = "T" + std::to_string((z.lo + z.hi) / 2);   // keyed by the centre: the same level keeps its touch state
    return out;
}

// ---- one visit (arrival -> decision) --------------------------------------------------------------------------------------------
struct Visit {
    std::string key, name; char kind = 'T'; int side = SUP; Tick lo = 0, hi = 0; long long ta = 0;
    Tick last[NSEC]; bool seen[NSEC]; double att[NSEC];
    Visit() { for (int i = 0; i < NSEC; ++i) { last[i] = 0; seen[i] = false; att[i] = 0; } }
};
struct Dec {
    std::string key, name; char kind = 'T'; int side = SUP; Tick lo = 0, hi = 0; long long ta = 0, td = 0;
    double exh = -1, share = 0; int s1 = 0, s2 = 0; double att1 = 0, att2 = 0; bool isEx = false; std::string label = "Ex?";
    long long gateN = -1; bool restored = false;
    std::string id() const { return key + "@" + std::to_string(ta); }
};
// the measure itself (exh_at_zone on one visit)
inline Dec decide(const Visit& v) {
    Dec d; d.key = v.key; d.name = v.name; d.kind = v.kind; d.side = v.side; d.lo = v.lo; d.hi = v.hi; d.ta = v.ta; d.td = v.ta + DEC_S * 1000LL;
    Tick cur = 0; bool have = false; int at = 0;
    for (int k = 0; k < NSEC; ++k) {
        if (v.seen[k]) { cur = v.last[k]; have = true; }
        const bool a = have && cur >= v.lo - 1 && cur <= v.hi + 1;
        if (a) { ++at; if (k < HALF) ++d.s1; else ++d.s2; }
        if (k < HALF) d.att1 += v.att[k]; else d.att2 += v.att[k];
    }
    d.share = std::round((double)at / (double)NSEC * 1000.0) / 1000.0;                       // the study rounds both to 3 decimals
    if (d.s1 >= SECS_MIN && d.s2 >= SECS_MIN && d.att1 > 0) d.exh = std::round((d.att2 / d.s2) / (d.att1 / d.s1) * 1000.0) / 1000.0;
    d.isEx = d.exh >= 0 && d.exh <= THR + 1e-9 && d.share >= SHARE_MIN - 1e-9;
    return d;
}

// ---- the gate (the nightly params) ----------------------------------------------------------------------------------------------
struct Gate { bool file = false, ok = false; long long n = -1; double held = -1, control = -1, minLift = GATE_LIFT; long long minN = GATE_N; std::string why = "no params file"; };
// lines: "market,n,held,control,min_n,min_lift,updated" then "<MKT>,210,0.819,0.592,30,0.15,2026-10-27"
inline Gate parseGate(const std::vector<std::string>& lines, const std::string& mkt) {
    Gate g; g.file = true; g.why = "params file: no line for " + mkt;
    const std::string want = studyCode(mkt);
    for (std::string ln : lines) {
        while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n' || ln.back() == ' ')) ln.pop_back();
        std::vector<std::string> f; size_t p = 0; for (;;) { const size_t q = ln.find(',', p); f.push_back(ln.substr(p, q == std::string::npos ? q : q - p)); if (q == std::string::npos) break; p = q + 1; }
        if (f.size() < 4 || studyCode(f[0]) != want) continue;
        char* e = nullptr; const long long n = std::strtoll(f[1].c_str(), &e, 10); if (!e || *e || n < 0) { g.why = "params file: bad n"; continue; }
        e = nullptr; const double h = std::strtod(f[2].c_str(), &e); if (!e || *e || !(h >= 0 && h <= 1)) { g.why = "params file: bad held"; continue; }
        e = nullptr; const double c = std::strtod(f[3].c_str(), &e); if (!e || *e || !(c >= 0 && c <= 1)) { g.why = "params file: bad control"; continue; }
        g.n = n; g.held = h; g.control = c;
        if (f.size() >= 5) { e = nullptr; const long long mn = std::strtoll(f[4].c_str(), &e, 10); if (e && !*e && mn >= 1 && mn <= 100000) g.minN = mn; }
        if (f.size() >= 6) { e = nullptr; const double ml = std::strtod(f[5].c_str(), &e); if (e && !*e && ml >= 0 && ml <= 1) g.minLift = ml; }
        g.ok = g.n >= g.minN && g.held - g.control >= g.minLift - 1e-9;
        char b[160]; std::snprintf(b, sizeof b, "n %lld held %.0f%% control %.0f%% (needs n >= %lld, +%.0f pts)", g.n, g.held * 100, g.control * 100, g.minN, g.minLift * 100);
        g.why = b; return g;
    }
    return g;
}
inline const char* label(const Gate& g) { return g.ok ? "Ex" : "Ex?"; }

// ---- the record (one line per decision; read back after a restart) --------------------------------------------------------------
inline std::string header() { return "EXH|ver|market|zone_key|zone|kind|side|lo_tk|hi_tk|t_arr_ms|t_dec_ms|exh|share|s1|s2|att1|att2|is_ex|label|gate_n"; }
inline std::string recLine(const Dec& d, const std::string& mkt) {
    std::string nm = d.name; for (char& c : nm) if (c == '|' || c == '\n' || c == '\r') c = '/';
    char b[512]; std::snprintf(b, sizeof b, "EXH|%s|%s|%s|%s|%c|%c|%lld|%lld|%lld|%lld|%.3f|%.3f|%d|%d|%.0f|%.0f|%d|%s|%lld",
        VERSION, mkt.c_str(), d.key.c_str(), nm.c_str(), d.kind, d.side == SUP ? 'S' : 'R', d.lo, d.hi, d.ta, d.td, d.exh, d.share, d.s1, d.s2,
        d.att1, d.att2, d.isEx ? 1 : 0, d.label.c_str(), d.gateN);
    return b;
}
inline bool parseRec(std::string ln, Dec& d) {
    while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n')) ln.pop_back();
    std::vector<std::string> f; size_t p = 0; for (;;) { const size_t q = ln.find('|', p); f.push_back(ln.substr(p, q == std::string::npos ? q : q - p)); if (q == std::string::npos) break; p = q + 1; }
    if (f.size() != 20 || f[0] != "EXH" || f[1] == "ver") return false;
    auto L = [](const std::string& s, long long& o) { char* e = nullptr; o = std::strtoll(s.c_str(), &e, 10); return e && !*e && !s.empty(); };
    auto D = [](const std::string& s, double& o) { char* e = nullptr; o = std::strtod(s.c_str(), &e); return e && !*e && !s.empty() && std::isfinite(o); };
    d = Dec(); long long x = 0;
    d.key = f[3]; d.name = f[4]; if (d.key.empty() || f[5].size() != 1 || (f[5][0] != 'T' && f[5][0] != 'L')) return false;
    d.kind = f[5][0];
    if (f[6] != "S" && f[6] != "R") return false;
    d.side = f[6] == "S" ? SUP : RES;
    if (!L(f[7], d.lo) || !L(f[8], d.hi) || d.hi < d.lo || !L(f[9], d.ta) || !L(f[10], d.td) || d.td < d.ta || d.ta <= 0) return false;
    if (!D(f[11], d.exh) || !D(f[12], d.share) || !L(f[13], x)) return false;
    d.s1 = (int)x;
    if (!L(f[14], x)) return false;
    d.s2 = (int)x;
    if (!D(f[15], d.att1) || !D(f[16], d.att2) || !L(f[17], x) || (x != 0 && x != 1)) return false;
    d.isEx = x == 1;
    if (f[18] != "Ex" && f[18] != "Ex?") return false;
    d.label = f[18];
    if (!L(f[19], d.gateN)) return false;
    d.restored = true; return true;
}

// ---- the engine -------------------------------------------------------------------------------------------------------------------
struct Tally { long long trades = 0, visits = 0, decided = 0, ex = 0, exConfirmed = 0, notEx = 0, tooShort = 0, skippedDup = 0, restored = 0, outRth = 0, capped = 0; };
class Engine {
public:
    std::string market; ZoneCfg cfg; Gate gate;
    std::vector<Zone> zones;                       // the current key-level zones
    std::map<std::string, int> armed;              // level zone key -> side it would start as (the last closed candle was fully outside)
    std::vector<Visit> active;
    std::vector<Dec> decided;                      // every decision of the loaded sessions (restored + new)
    std::set<std::string> done;                    // ids decided (never twice)
    std::vector<std::string> rows;                 // record lines waiting to be written (cleared only after a good write)
    long long lastT = 0; Tally tally;

    void setMarket(const std::string& m) { if (m != market) { market = m; cfg = zoneCfg(m); } }
    // the zones for now: the key levels (merged); the touch state of a zone survives a refresh by its key
    void setZones(const std::vector<Level>& lv, Tick hw) {
        std::vector<Zone> Z = mergeLevels(lv, hw);
        zones.swap(Z);
        std::map<std::string, int> keep; for (const Zone& z : zones) { const auto it = armed.find(z.key); if (it != armed.end()) keep[z.key] = it->second; }
        armed.swap(keep);
    }
    // a closed candle (ticks): a level zone is armed when this candle was fully outside it (side by its close), else disarmed
    void closeBar(Tick h, Tick l, Tick c) {
        for (const Zone& z : zones) {
            if (l > z.hi || h < z.lo) armed[z.key] = c > z.hi ? SUP : RES; else armed.erase(z.key);
        }
    }
    // one trade: ms on the Central clock, price, lots, the aggressor (BUYER / SELLER / NOATT for an implied fill)
    void onTrade(long long ms, Tick px, double lots, int aggr) {
        if (ms <= 0 || px <= 0) return;
        ++tally.trades; step(ms, true);
        lastT = std::max(lastT, ms);
        // arrivals: the first trade inside an armed zone (RTH only)
        for (const Zone& z : zones) {
            if (px < z.lo || px > z.hi) continue;
            const auto it = armed.find(z.key); if (it == armed.end()) continue;
            const int side = it->second; armed.erase(it);
            if (!inRth(cfg, ms)) { ++tally.outRth; continue; }
            bool dup = false;   // one visit per place: an active visit on the same side whose zone overlaps this one
            for (const Visit& v : active) if (v.side == side && v.lo <= z.hi && v.hi >= z.lo) { dup = true; break; }
            if (dup) { ++tally.skippedDup; continue; }
            if (active.size() >= ACTIVE_CAP) { ++tally.capped; continue; }
            Visit v; v.key = z.key; v.name = z.name; v.kind = z.kind; v.side = side; v.lo = z.lo; v.hi = z.hi; v.ta = ms;
            active.push_back(v); ++tally.visits;
        }
        for (Visit& v : active) {
            if (ms > v.ta + DEC_S * 1000LL) continue;                       // the window is arrival .. decision, both ends included
            const long long k = (ms - v.ta) / 1000; if (k < 0 || k >= NSEC) continue;
            v.last[k] = px; v.seen[k] = true;
            const int want = v.side == SUP ? SELLER : BUYER;
            if (aggr == want && px >= v.lo && px <= v.hi && lots > 0) v.att[k] += lots;
        }
    }
    // the clock: a visit is decided once its decision time has passed (a trade after it, or the clock 3 s after it - late feed rows)
    void step(long long nowMs, bool byTrade = false) {
        if (nowMs <= 0) return;
        for (size_t i = 0; i < active.size();) {
            const long long td = active[i].ta + DEC_S * 1000LL;
            const bool due = byTrade ? nowMs > td : nowMs > td + 3000;   // a trade AT the decision time still counts
            if (!due) { ++i; continue; }
            Dec d = decide(active[i]); active.erase(active.begin() + (long)i);
            if (done.count(d.id())) continue;
            d.label = label(gate); d.gateN = gate.n;
            done.insert(d.id()); decided.push_back(d); rows.push_back(recLine(d, market)); ++tally.decided;
            if (d.exh < 0) ++tally.tooShort;
            if (d.isEx) { ++tally.ex; if (gate.ok) ++tally.exConfirmed; } else ++tally.notEx;
        }
        if (decided.size() > DONE_CAP) decided.erase(decided.begin(), decided.begin() + (long)(DONE_CAP / 4));
    }
    // the record of a session read back (first line wins; never decided again)
    int adopt(const std::vector<std::string>& lines) {
        int n = 0;
        for (const std::string& ln : lines) { Dec d; if (!parseRec(ln, d) || done.count(d.id())) continue; done.insert(d.id()); decided.push_back(d); ++n; }
        tally.restored += n; return n;
    }
};

// ---- the marks to draw ---------------------------------------------------------------------------------------------------------
struct Mark { long long td = 0; int side = SUP; std::string label; size_t dec = 0; };
// candle end times (ms, ascending, the chart's bar END stamps) -> one mark per CLOSED candle: the Ex decision whose decision time
// falls inside the candle (end - spb, end]; the first by decision time wins. lastClosed = index of the last closed candle.
inline std::map<int, Mark> marksByBar(const std::vector<Dec>& D, const std::vector<long long>& endMs, long spb, int lastClosed) {
    std::map<int, Mark> out; if (endMs.empty() || spb <= 0) return out;
    std::vector<size_t> idx; for (size_t i = 0; i < D.size(); ++i) if (D[i].isEx) idx.push_back(i);
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return D[a].td < D[b].td || (D[a].td == D[b].td && D[a].key < D[b].key); });
    for (size_t i : idx) {
        const long long td = D[i].td;
        const auto it = std::lower_bound(endMs.begin(), endMs.end(), td);   // the first candle ending at or after td
        if (it == endMs.end()) continue;                                      // its candle has not closed yet (still forming / not on the chart)
        const int b = (int)(it - endMs.begin());
        if (b > lastClosed || td <= *it - spb * 1000LL) continue;             // not closed / a gap: td is not inside this candle
        if (out.count(b)) continue;                                           // one mark per candle
        Mark m; m.td = td; m.side = D[i].side; m.label = D[i].label; m.dec = i; out[b] = m;
    }
    return out;
}

// ---- placement (screen pixels; y grows downward) ------------------------------------------------------------------------------
struct Box { int l = 0, t = 0, r = 0, b = 0; bool valid() const { return r > l && b > t; } };
inline bool hits(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline bool inside(const Box& a, const Box& area) { return a.l >= area.l && a.r <= area.r && a.t >= area.t && a.b <= area.b; }
struct Place { bool ok = false; Box box; int nudged = 0; bool kept = false; };
// the letter just beyond the candle (above for RES, below for SUP), 2 px off its high / low. If it touches another letter, circle or a
// neighbouring candle: nudged SIDEWAYS one step (half its width, right then left); if that is not free, moved further OUT past what it
// touched (up to 3 times, still centred on its candle); only then 2 - 3 steps sideways. Never inside the keep-out (the HOD/LOD table),
// never outside the price area - then not drawn.
inline Place place(int cx, int candleTop, int candleBot, int side, int w, int h, const std::vector<Box>& others, const Box& keepOut, const Box& area) {
    Place p; Box f; f.l = cx - w / 2; f.r = f.l + w;
    if (side == RES) { f.b = candleTop - 2; f.t = f.b - h; } else { f.t = candleBot + 2; f.b = f.t + h; }
    const int step = std::max(1, w / 2 + 1);
    auto clashOf = [&](const Box& g) -> const Box* { for (const Box& o : others) if (o.valid() && hits(g, o)) return &o; return nullptr; };
    auto ok = [&](const Box& g, int nudged) {
        if (!inside(g, area)) return false;
        if (keepOut.valid() && hits(g, keepOut)) { p.kept = true; return false; }
        if (clashOf(g)) return false;
        p.ok = true; p.box = g; p.nudged = nudged; p.kept = false; return true;
    };
    if (ok(f, 0)) return p;
    for (int sgn : { 1, -1 }) { Box g = f; g.l += sgn * step; g.r += sgn * step; if (ok(g, 1)) return p; }
    Box g = f;
    for (int k = 0; k < 3; ++k) {                       // further out, past the box it touches
        const Box* c = clashOf(g); if (!c) break;
        if (side == RES) { g.b = c->t - 2; g.t = g.b - h; } else { g.t = c->b + 2; g.b = g.t + h; }
        if (ok(g, 1)) return p;
    }
    for (int k = 2; k <= 3; ++k) for (int sgn : { 1, -1 }) { Box q = f; q.l += sgn * k * step; q.r += sgn * k * step; if (ok(q, 1)) return p; }
    return p;
}

// ---- other plugins' letters to stay clear of -------------------------------------------------------------------------------------
// lsBigBidAsk's breakout record line (BBO|t|t0|dir|fired|...|rule|trend|ext30, 27 fields): a FIRED rule-5 BO / BD is drawn on candle
// t0 (raw IRT key), "BO" green ABOVE (dir +1) / red BELOW (dir -1). -> (t0, dir) or false.
inline bool parseBoMark(const std::string& line0, long long& t0, int& dir) {
    std::string line = line0; while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    std::vector<std::string> f; size_t p = 0; for (;;) { const size_t q = line.find('|', p); f.push_back(line.substr(p, q == std::string::npos ? q : q - p)); if (q == std::string::npos) break; p = q + 1; }
    if (f.size() != 27 || f[0] != "BBO" || f[4] != "1") return false;
    char* e = nullptr; const long long rule = std::strtoll(f[24].c_str(), &e, 10); if (!e || *e || rule < 5) return false;
    e = nullptr; t0 = std::strtoll(f[2].c_str(), &e, 10); if (!e || *e || t0 <= 0) return false;
    e = nullptr; const long long d = std::strtoll(f[3].c_str(), &e, 10); if (!e || *e || (d != 1 && d != -1)) return false;
    dir = (int)d; return true;
}
// lsDeltaProfile's signal record (tf|...|A or A?|side|...|lo|hi|...): an A circle at (CT second tf, node lo..hi) -> true
inline bool parseDeltaA(const std::string& line0, long long& tf, double& lo, double& hi) {
    std::string line = line0; while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    std::vector<std::string> f; size_t p = 0; for (;;) { const size_t q = line.find('|', p); f.push_back(line.substr(p, q == std::string::npos ? q : q - p)); if (q == std::string::npos) break; p = q + 1; }
    if (f.size() < 8 || (f[2] != "A" && f[2] != "A?")) return false;
    char* e = nullptr; tf = std::strtoll(f[0].c_str(), &e, 10); if (!e || *e || tf <= 0) return false;
    e = nullptr; lo = std::strtod(f[5].c_str(), &e); if (!e || *e) return false;
    e = nullptr; hi = std::strtod(f[6].c_str(), &e); if (!e || *e || hi < lo) return false;
    return true;
}

}  // namespace exh
#endif
