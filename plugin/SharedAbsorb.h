#ifndef LS_SHARED_ABSORB_H
#define LS_SHARED_ABSORB_H
// SharedAbsorb.h 1.0.0 (2026-10-22, Rassul 06:43 CT "then do it" - the SHARED ABSORPTION A; 06:46 "only one circle per candle" on the
// Delta Profile; 06:49 "then require it" - the favourable close is REQUIRED on every absorption mark). SDK-free, unit-tested
// (tests_sab/test_sab.cpp). Research: research-artifacts\learning\SHARED-ABSORPTION-STUDY-2026-10-21.md + shared_absorption_odds.json.
//
// THE RULE (one rule for lsDeltaProfile, lsBidAskStats (attacker pane), lsBookFlow (Defense pane) and lsBigBidAsk (the circle)):
//   A = on a CLOSED candle at a 5-candle high / low (location(): swappable, see LOCATION), the ATTACKERS' aggressive lots on that candle
//       (buyers lifting the offer at a high, sellers hitting the bid at a low; the chart's VAP) >= E x their own average of the up to 20
//       candles before it (>= 5 needed), AND the candle closes in the DEFENDERS' colour (green at a low, red at a high - strictly).
//   E = 2.0x in every market (independent review 2026-10-22: the study's per-market ES 3.0 / NQ 1.5 / CL 3.0 / GC 2.5 / HG 2.5 / NG 2.0 /
//   6E 2.0 were fitting noise). The nightly params file LRA-Badges-<MKT>.csv (lra.orderflow_badges) overrides it per market with
//   PARAM,ABS_MIN_EFFORT_X,<x>; missing / garbled = these defaults and the reason in the status file. A file that sets
//   ABS_REQUIRE_DEFENDER_CLOSE 0 is IGNORED (Rassul 06:49: the close is always required) and said so.
//   Decided once at the close, never repainted. Pooled: held 62.8% vs 46.9% (n 884); book absorbed in the wick 68.9%.
//
// ONE SHARED RECORD per market and bar size: lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv, append-only. Every plugin DRAWS only from
// this record (so all of them show the SAME A); a plugin that has the chart's VAP may DECIDE a candle that has no line yet and append
// it (first line per candle + side wins - lsBidAskStats decides first, lsDeltaProfile only when no attacker pane is on the chart).
//   SAB|1|<sab version>|<MKT>|<spb>|<bar key CT s>|<bar raw>|<L/H>|<effort x>|<E>|<att lots>|<avg lots>|<G/R close>|<extreme px>|
//       <close px>|<book 1/0/->|<book lots>|<refills>|<decided CT s>|<by>|<location rule>          (21 fields)
//   SABX|1|<MKT>|<spb>|<bar key>|<L/H>|<k=v;k=v...>|<by>   extras another plugin adds later (pullx, e100, into100, level, lvd, gamma,
//       book, booklots, refills) - the first value of each field wins.
// The nightly (lra.orderflow_badges 1.2.0) scores the SAB lines as read SHARED_A and writes READ,SHARED_A,<status>,<n>,<held>,
// <matched>,<lift>,<swing> into LRA-Badges-<MKT>.csv; the hover shows that once n >= 20, else the study's own numbers.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sab {
static const char* const VERSION = "1.0.0";
enum { LOW = 0, HIGH = 1 };                                  // the side of the extreme (= dt::LOW / dt::HIGH)
static const int EXT_BARS = 5, EFFORT_BARS = 20, EFFORT_MIN = 5;

// ---- per-market numbers (study 2026-10-21, final table) -----------------------------------------------------------------------------
// (2026-10-22 independent review REVIEW-2026-10-22.md, applied before delivery) ONE E = 2.0x in every market (the per-market E was fitting
// noise: out of sample 2.0x everywhere +15.3 vs +15.5 tuned); held / base = the study's data at 2.0x, base = MATCHED 5-candle extremes
// (ATR + close-back terciles; pooled 61.6% vs 52.5%, +9.1, n 1,070); learning = GC failed the study's kill rule, ES / CL small forward n.
struct Mkt { const char* m; double E; double held; int n; double base; double riskAtr; bool learning; };
static const Mkt MKTS[] = {
    { "ES", 2.0, 66.0, 141, 51.0, 1.62, true }, { "NQ", 2.0, 56.2, 121, 51.7, 1.12, false }, { "CL", 2.0, 62.8, 145, 52.4, 1.93, true },
    { "GC", 2.0, 60.5, 157, 52.6, 1.36, true }, { "HG", 2.0, 60.6, 165, 50.9, 1.63, false }, { "NG", 2.0, 62.9, 143, 53.7, 1.88, false },
    { "EU", 2.0, 61.6, 198, 54.3, 1.50, false } };
inline std::string canon(const std::string& m) { return m == "6E" ? std::string("EU") : m; }
inline const Mkt* mktOf(const std::string& m0) {
    const std::string m = canon(m0);
    for (size_t k = 0; k < sizeof(MKTS) / sizeof(MKTS[0]); ++k) if (m == MKTS[k].m) return &MKTS[k];
    return nullptr;
}
inline double defaultE(const std::string& m) { const Mkt* p = mktOf(m); return p ? p->E : 2.0; }   // unknown market: the pooled 2.0

// ---- the candles ---------------------------------------------------------------------------------------------------------------------
// prices in whole ticks; buy = lots bought at the ask (buyers lifting), sell = lots sold at the bid (sellers hitting); < 0 = unknown
struct Bar { long long o = 0, h = 0, l = 0, c = 0; long long buy = -1, sell = -1; bool ok = false; };

// ---- LOCATION (swappable - 2026-10-22 coordinator: a study compares the 5-candle extreme vs his pivot rule vs both; switch LOCATION) ----
enum LocRule { LOC_EXT5 = 0, LOC_PIVOT = 1, LOC_BOTH = 2 };
static const int LOCATION = LOC_EXT5;                         // <- the one place to switch
inline const char* locName(int r) { return r == LOC_PIVOT ? "pivot" : r == LOC_BOTH ? "ext5+pivot" : "ext5"; }
// a 5-candle high: high >= each of the 5 highs before it (low mirror); needs 5 valid candles before
inline bool ext5(const std::vector<Bar>& b, size_t i, int side) {
    if (i < (size_t)EXT_BARS || i >= b.size() || !b[i].ok) return false;
    for (size_t k = i - EXT_BARS; k < i; ++k) { if (!b[k].ok) { return false; } if (side == HIGH ? b[k].h > b[i].h : b[k].l < b[i].l) return false; }
    return true;
}
// his pivot rule (DeltaProfileCore.h pivotOk 2.8.3, the 1-bar ZigZag over candles 0 .. i - no look-ahead): bullish = candle i is the down
// pivot or its low is below the down-pivot candle's high; bearish mirror
inline bool pivotLoc(const std::vector<Bar>& b, size_t pk, int side) {
    if (pk < 1 || pk >= b.size()) return false;
    for (size_t i = 0; i <= pk; ++i) if (!b[i].ok || b[i].h < b[i].l) return false;
    int dirn = b[1].h >= b[0].h ? 1 : -1; size_t ext = 0, start = 0;
    for (size_t i = 1; i <= pk; ++i) {
        if (dirn == 1) { if (b[i].h > b[ext].h) ext = i; else if (b[i].l < b[i - 1].l && i != ext) { start = ext; ext = i; dirn = -1; } }
        else { if (b[i].l < b[ext].l) ext = i; else if (b[i].h > b[i - 1].h && i != ext) { start = ext; ext = i; dirn = 1; } }
    }
    if (side == LOW) { const size_t piv = dirn == -1 ? ext : start; return piv == pk || b[pk].l < b[piv].h; }
    const size_t piv = dirn == 1 ? ext : start; return piv == pk || b[pk].h > b[piv].l;
}
inline bool location(const std::vector<Bar>& b, size_t i, int side, int rule = LOCATION) {
    if (rule == LOC_PIVOT) return pivotLoc(b, i, side);
    if (rule == LOC_BOTH) return ext5(b, i, side) && pivotLoc(b, i, side);
    return ext5(b, i, side);
}

// ---- the decision ------------------------------------------------------------------------------------------------------------------
inline bool defendersClose(const Bar& x, int side) { return side == HIGH ? x.c < x.o : x.c > x.o; }   // strict: a doji is not
inline long long attackOf(const Bar& x, int side) { return side == HIGH ? x.buy : x.sell; }
struct Decision {
    bool decided = false;            // false = cannot be judged yet (too few candles / no lots) - never written
    bool fired = false, loc = false, colour = false;
    double effort = 0, E = 0, avg = 0; long long att = 0; int side = LOW;
    std::string why;
};
// candle i (closed) of b (oldest first), side LOW / HIGH, threshold E
inline Decision decide(const std::vector<Bar>& b, size_t i, int side, double E, int rule = LOCATION) {
    Decision d; d.side = side; d.E = E;
    if (i >= b.size() || !b[i].ok) { d.why = "no candle"; return d; }
    d.loc = location(b, i, side, rule);
    if (!d.loc) { d.decided = true; d.why = std::string("not at a ") + (rule == LOC_PIVOT ? "pivot" : "5-candle extreme"); return d; }
    const long long a = attackOf(b[i], side); if (a < 0) { d.why = "no lots for this candle"; return d; }
    d.att = a;
    size_t n = 0; double sum = 0;
    for (size_t k = i > (size_t)EFFORT_BARS ? i - EFFORT_BARS : 0; k < i; ++k) { const long long p = b[k].ok ? attackOf(b[k], side) : -1; if (p >= 0) { sum += (double)p; ++n; } }
    if (n < (size_t)EFFORT_MIN || !(sum > 0)) { d.why = "learning: fewer than 5 earlier candles with lots"; return d; }
    d.avg = sum / (double)n; d.effort = (double)a / d.avg; d.decided = true;
    d.colour = defendersClose(b[i], side);
    if (!(E > 0) || !std::isfinite(d.effort)) { d.why = "no threshold"; return d; }
    if (d.effort < E) { d.why = "effort under the threshold"; return d; }
    if (!d.colour) { d.why = "closed in the attackers' colour"; return d; }
    d.fired = true; d.why = "absorbed"; return d;
}

inline std::string fmt(double v, int d = 2) { char b[48]; std::snprintf(b, sizeof b, "%.*f", d, std::isfinite(v) ? v : 0.0); return b; }
// ---- params (LRA-Badges-<MKT>.csv: VERSION,1 / MARKET,<m> / ASOF / PARAM,<k>,<v> / READ,<read>,<status>,<n>,<held>,<matched>,<lift>,<swing>)
struct Params { double E = 2.0; bool fromFile = false; double held = 0, base = 0; int n = 0; bool oddsFromFile = false, learning = false; std::string why; std::string asof; };
inline bool num(const std::string& s0, double& v) {
    std::string s = s0; while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
    if (s.empty()) { return false; } errno = 0; char* e = nullptr; v = std::strtod(s.c_str(), &e);
    return e && *e == 0 && errno == 0 && std::isfinite(v);
}
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> f; std::string c; for (char ch : s) { if (ch == '\r') continue; if (ch == d) { f.push_back(c); c.clear(); } else c += ch; } f.push_back(c); return f; }
inline Params defaults(const std::string& m) {
    Params p; const Mkt* k = mktOf(m); p.E = defaultE(m);
    if (k) { p.held = k->held; p.n = k->n; p.base = k->base; p.learning = k->learning; }
    return p;
}
inline Params parseParams(const std::string& text, const std::string& market, bool present = true) {
    Params p = defaults(market);
    if (!present) { p.why = "LRA-Badges-" + canon(market) + ".csv missing - study defaults"; return p; }
    if (text.size() > 256 * 1024) { p.why = "params file too big - study defaults"; return p; }
    std::istringstream in(text); std::string ln; bool ver = false, mk = false; double E = 0; bool haveE = false; int lines = 0;
    Params f = p; std::string notes; bool pooledWriter = false;
    while (std::getline(in, ln)) {
        if (++lines > 2000) break;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty()) continue;
        if (ln[0] == '#') {                                              // the writer: orderflow_badges 1.0.x / 1.1.x wrote ONE pooled 2.0 for every market
            const size_t w = ln.find("orderflow_badges ");
            if (w != std::string::npos) { int mj = 0, mn = 0; if (std::sscanf(ln.c_str() + w + 17, "%d.%d", &mj, &mn) == 2) pooledWriter = mj < 1 || (mj == 1 && mn < 2); }
            continue;
        }
        const std::vector<std::string> c = split(ln, ',');
        if (!ver) { if (c.size() == 2 && c[0] == "VERSION" && c[1] == "1") { ver = true; continue; } p.why = "garbled params file (first line is not VERSION,1) - study defaults"; return p; }
        if (c[0] == "MARKET") { if (c.size() < 2 || canon(c[1]) != canon(market)) { p.why = "params file is for another market - study defaults"; return p; } mk = true; continue; }
        if (c[0] == "ASOF" && c.size() >= 2) { f.asof = c[1]; continue; }
        if (c[0] == "PARAM" && c.size() >= 3) {
            if (c[1] == "ABS_MIN_EFFORT_X") { double v = 0; if (!num(c[2], v) || v < 1.0 || v > 10.0) { p.why = "ABS_MIN_EFFORT_X garbled / out of range 1-10 - study defaults"; return p; } E = v; haveE = true; }
            else if (c[1] == "ABS_REQUIRE_DEFENDER_CLOSE" && c[2] != "1") notes += "; ABS_REQUIRE_DEFENDER_CLOSE " + c[2] + " ignored (the close is always required)";
            else if (c[1] == "ABS_REQUIRE_EXT5" && c[2] != "1") notes += "; ABS_REQUIRE_EXT5 " + c[2] + " ignored (location rule " + locName(LOCATION) + ")";
            continue;
        }
        if (c[0] == "READ" && c.size() >= 6 && c[1] == "SHARED_A") {
            double n = 0, held = 0, matched = 0;
            if (num(c[3], n) && num(c[4], held) && num(c[5], matched) && n >= 20 && held >= 0 && held <= 100 && matched >= 0 && matched <= 100) {
                f.n = (int)n; f.held = held; f.base = matched; f.oddsFromFile = true; f.learning = c[2] != "SHOW"; }   // the nightly decides LEARNING (status SHOW = proven)
            continue;
        }
    }
    if (!ver || !mk) { p.why = "params file without VERSION / MARKET - study defaults"; return p; }
    if (haveE && pooledWriter) { notes += "; ABS_MIN_EFFORT_X " + fmt(E, 1) + " ignored (orderflow_badges before 1.2.0 wrote one pooled value, not this market's)"; haveE = false; }
    if (haveE) { f.E = E; f.fromFile = true; }
    f.why = std::string(haveE ? "E from LRA-Badges" : "E study default (no ABS_MIN_EFFORT_X row)") + (f.oddsFromFile ? ", held rate from the nightly" : ", held rate from the study") + notes;
    return f;
}

// ---- the record ----------------------------------------------------------------------------------------------------------------------
struct Rec {
    std::string ver, mkt, by, loc; int spb = 0;
    long long key = 0;               // the bar END as Central wall-clock seconds (SignalBoard's bar key)
    long long raw = 0;               // the chart's own RTDATE of the bar (IRT key)
    int side = LOW; double effort = 0, E = 0; long long att = 0; double avg = 0; char close = 'G';
    double extPx = 0, closePx = 0; int book = -1; long long bookLots = 0; int refills = 0; long long decided = 0;
    // extras (SABX), NaN / -1 = n/a
    double pullx = -1; long long e100 = -1, into100 = -1; std::string level; double lvd = -1; int gamma = 0;
};
inline std::string px(double v) { char b[48]; std::snprintf(b, sizeof b, "%.10g", std::isfinite(v) ? v : 0.0); return b; }
inline std::string line(const Rec& r) {
    std::ostringstream o;
    o << "SAB|1|" << VERSION << '|' << canon(r.mkt) << '|' << r.spb << '|' << r.key << '|' << r.raw << '|' << (r.side == HIGH ? 'H' : 'L') << '|'
      << fmt(r.effort) << '|' << fmt(r.E, 1) << '|' << r.att << '|' << fmt(r.avg, 1) << '|' << (r.close == 'R' ? 'R' : 'G') << '|' << px(r.extPx) << '|' << px(r.closePx) << '|'
      << (r.book < 0 ? std::string("-") : std::to_string(r.book)) << '|' << r.bookLots << '|' << r.refills << '|' << r.decided << '|' << r.by << '|' << r.loc;
    return o.str();
}
inline bool toLL(const std::string& s, long long& v) { if (s.empty() || s.size() > 20) { return false; } errno = 0; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0 && errno == 0; }
inline bool parse(const std::string& s, Rec& out) {
    if (s.compare(0, 4, "SAB|") != 0) return false;
    const std::vector<std::string> f = split(s, '|');
    if (f.size() != 21 || f[1] != "1") return false;
    Rec r; r.ver = f[2]; r.mkt = f[3]; long long v = 0;
    if (r.mkt.empty() || r.mkt.size() > 8) return false;
    if (!toLL(f[4], v) || v <= 0 || v > 86400) { return false; } r.spb = (int)v;
    if (!toLL(f[5], r.key) || r.key <= 0 || !toLL(f[6], r.raw) || r.raw < 0) return false;
    if (f[7] != "L" && f[7] != "H") { return false; } r.side = f[7] == "H" ? HIGH : LOW;
    if (!num(f[8], r.effort) || r.effort < 0 || !num(f[9], r.E) || r.E <= 0) return false;
    if (!toLL(f[10], r.att) || r.att < 0 || !num(f[11], r.avg) || r.avg < 0) return false;
    if (f[12] != "G" && f[12] != "R") { return false; } r.close = f[12][0];
    if (!num(f[13], r.extPx) || !num(f[14], r.closePx)) return false;
    if (f[15] == "-") r.book = -1; else if (f[15] == "0" || f[15] == "1") r.book = f[15][0] - '0'; else return false;
    if (!toLL(f[16], r.bookLots) || r.bookLots < 0 || !toLL(f[17], v) || v < 0 || v > 1000) { return false; } r.refills = (int)v;
    if (!toLL(f[18], r.decided) || r.decided < 0) return false;
    r.by = f[19].substr(0, 16); r.loc = f[20].substr(0, 16);
    // the colour rule is part of the record: a line whose close is the attackers' colour is never a valid A (06:49 "then require it")
    if ((r.side == LOW) != (r.close == 'G')) return false;
    out = r; return true;
}
// extras
struct Extra { std::string mkt; int spb = 0; long long key = 0; int side = LOW; std::map<std::string, std::string> kv; std::string by; };
inline std::string xline(const Extra& x) {
    std::string kv; for (std::map<std::string, std::string>::const_iterator i = x.kv.begin(); i != x.kv.end(); ++i) { if (!kv.empty()) kv += ';'; kv += i->first + "=" + i->second; }
    return "SABX|1|" + canon(x.mkt) + "|" + std::to_string(x.spb) + "|" + std::to_string(x.key) + "|" + (x.side == HIGH ? "H" : "L") + "|" + kv + "|" + x.by;
}
inline bool xparse(const std::string& s, Extra& out) {
    if (s.compare(0, 5, "SABX|") != 0) return false;
    const std::vector<std::string> f = split(s, '|');
    if (f.size() != 8 || f[1] != "1") return false;
    Extra x; x.mkt = f[2]; long long v = 0;
    if (!toLL(f[3], v) || v <= 0 || v > 86400) { return false; } x.spb = (int)v;
    if (!toLL(f[4], x.key) || x.key <= 0) return false;
    if (f[5] != "L" && f[5] != "H") { return false; } x.side = f[5] == "H" ? HIGH : LOW;
    for (const std::string& p : split(f[6], ';')) { const size_t e = p.find('='); if (e == std::string::npos || e == 0 || p.size() > 64) continue; x.kv[p.substr(0, e)] = p.substr(e + 1); }
    x.by = f[7].substr(0, 16); out = x; return true;
}
inline void apply(Rec& r, const Extra& x, std::map<std::string, bool>& seen) {
    for (std::map<std::string, std::string>::const_iterator i = x.kv.begin(); i != x.kv.end(); ++i) {
        if (seen[i->first]) continue;                                   // the first value of a field wins
        double v = 0; long long w = 0; bool used = true;
        if (i->first == "pullx" && num(i->second, v) && v >= 0) r.pullx = v;
        else if (i->first == "e100" && toLL(i->second, w) && w >= 0) r.e100 = w;
        else if (i->first == "into100" && toLL(i->second, w) && w >= 0) r.into100 = w;
        else if (i->first == "level" && i->second.size() <= 24) r.level = i->second;
        else if (i->first == "lvd" && num(i->second, v) && v >= 0) r.lvd = v;
        else if (i->first == "gamma" && toLL(i->second, w) && w >= -1 && w <= 1) r.gamma = (int)w;
        else if (i->first == "book" && r.book < 0 && (i->second == "0" || i->second == "1")) r.book = i->second[0] - '0';
        else if (i->first == "booklots" && r.bookLots == 0 && toLL(i->second, w) && w >= 0) r.bookLots = w;
        else if (i->first == "refills" && r.refills == 0 && toLL(i->second, w) && w >= 0 && w <= 1000) r.refills = (int)w;
        else used = false;
        if (used) seen[i->first] = true;
    }
}

// ---- the store: every A of one market + bar size, read from the shared file (incremental, bounded), first line per candle + side wins --
struct Store {
    std::map<std::pair<int, long long>, Rec> byKey;                 // [side, CT key]
    std::map<std::pair<int, long long>, long long> rawKey;          // [side, raw] -> CT key
    std::map<std::pair<int, long long>, std::map<std::string, bool> > seen;
    std::vector<Extra> orphan;                                      // extras that came before their SAB line (another writer's order)
    long long off = 0, lines = 0, bad = 0, wrongMkt = 0; std::string mkt; int spb = 0;
    void clear() { byKey.clear(); rawKey.clear(); seen.clear(); orphan.clear(); off = 0; lines = bad = wrongMkt = 0; }
    void add(const std::string& ln) {
        Rec r; Extra x;
        if (parse(ln, r)) {
            if ((!mkt.empty() && canon(r.mkt) != canon(mkt)) || (spb > 0 && r.spb != spb)) { ++wrongMkt; return; }
            const std::pair<int, long long> k(r.side, r.key);
            if (byKey.count(k)) return;                                 // first line wins
            byKey[k] = r; if (r.raw > 0) rawKey[std::make_pair(r.side, r.raw)] = r.key; ++lines;
            for (size_t i = 0; i < orphan.size();) { if (orphan[i].key == r.key && orphan[i].side == r.side) { apply(byKey[k], orphan[i], seen[k]); orphan.erase(orphan.begin() + (long)i); } else ++i; }
            return;
        }
        if (xparse(ln, x)) {
            if ((!mkt.empty() && canon(x.mkt) != canon(mkt)) || (spb > 0 && x.spb != spb)) { ++wrongMkt; return; }
            const std::pair<int, long long> k(x.side, x.key);
            std::map<std::pair<int, long long>, Rec>::iterator it = byKey.find(k);
            if (it == byKey.end()) { if (orphan.size() < 2000) orphan.push_back(x); return; }
            apply(it->second, x, seen[k]); ++lines; return;
        }
        if (!ln.empty() && ln[0] != '#') ++bad;
    }
    const Rec* at(int side, long long key) const { std::map<std::pair<int, long long>, Rec>::const_iterator i = byKey.find(std::make_pair(side, key)); return i == byKey.end() ? nullptr : &i->second; }
    const Rec* atRaw(int side, long long raw) const { std::map<std::pair<int, long long>, long long>::const_iterator i = rawKey.find(std::make_pair(side, raw)); return i == rawKey.end() ? nullptr : at(side, i->second); }
    bool has(int side, long long key) const { return byKey.count(std::make_pair(side, key)) != 0; }
    void trim(size_t cap = 6000) { while (byKey.size() > cap) { const std::pair<int, long long> k = byKey.begin()->first; rawKey.erase(std::make_pair(k.first, byKey.begin()->second.raw)); seen.erase(k); byKey.erase(byKey.begin()); } }
};
// read the new whole lines of the file from S.off (bounded to the newest 4 MB); a shorter file = read again from the start
inline bool refresh(Store& S, const std::string& path, long long maxRead = 4 * 1024 * 1024) {
    if (path.empty()) return false;
    std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
    in.seekg(0, std::ios::end); const long long size = (long long)in.tellg(); if (size < 0) return false;
    if (size < S.off) { const std::string m = S.mkt; const int sp = S.spb; S.clear(); S.mkt = m; S.spb = sp; }
    if (size == S.off) return false;
    long long from = S.off; if (size - from > maxRead) from = size - maxRead;
    in.seekg(from, std::ios::beg); std::string buf((size_t)(size - from), '\0'); in.read(&buf[0], (std::streamsize)buf.size());
    if (in.gcount() != (std::streamsize)buf.size()) return false;
    const size_t last = buf.rfind('\n'); if (last == std::string::npos) return false;   // a torn last line waits for its newline
    size_t a = 0; if (from != S.off) { const size_t nl = buf.find('\n'); a = nl == std::string::npos ? last + 1 : nl + 1; }   // skipped part: start at a whole line
    while (a <= last) { const size_t b = buf.find('\n', a); std::string ln = buf.substr(a, b - a); a = b + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (!ln.empty() && ln.size() < 2048) S.add(ln); }
    S.off = from + (long long)last + 1;
    return true;
}
// append lines (one write, flushed); true only when all of it was written
inline bool append(const std::string& path, const std::vector<std::string>& lines) {
    if (path.empty() || lines.empty()) return false;
    std::string all; for (const std::string& l : lines) all += l + "\n";
    std::ofstream o(path.c_str(), std::ios::binary | std::ios::app); if (!o.is_open()) return false;
    o.write(all.data(), (std::streamsize)all.size()); o.flush(); return (bool)o;
}
inline std::string fileName(const std::string& market, int spb) { return "SAB-" + canon(market) + "-" + std::to_string(spb) + ".csv"; }

// ---- the book from the SignalBoard 'O' events of the candle (lsBidAskStats): defenders' resting prices inside the wick that absorbed -
struct ObEv { double px = 0; int side = 0; double traded = 0; int refills = 0; };     // side = resting (+1 offers / -1 bids)
// wick: at a low [low, min(o, c) - 1], at a high [max(o, c) + 1, high] (ticks). present = the candle was decided by the order feed
inline void bookOf(const Bar& x, int side, bool present, const std::vector<ObEv>& ev, double tick, int& book, long long& lots, int& refills) {
    book = -1; lots = 0; refills = 0; if (!present || !(tick > 0)) return;
    const long long lo = side == HIGH ? std::max(x.o, x.c) + 1 : x.l, hi = side == HIGH ? x.h : std::min(x.o, x.c) - 1;
    book = 0; if (hi < lo) return;
    for (const ObEv& e : ev) {
        if (e.side != (side == HIGH ? 1 : -1)) continue;
        const long long k = (long long)std::llround(e.px / tick); if (k < lo || k > hi) continue;
        lots += (long long)std::llround(e.traded); refills = std::max(refills, e.refills); book = 1;
    }
}
// nearest named level within 0.5 ATR of the extreme, gamma sign from a level named HVL (close above = +1 positive gamma, below = -1)
struct Lv { std::string name; double px = 0; };
inline void levelOf(const std::vector<Lv>& v, double extPx, double closePx, double atr, std::string& name, double& lvd, int& gamma) {
    name.clear(); lvd = -1; gamma = 0; double best = 1e300;
    for (const Lv& l : v) {
        if (!(l.px > 0) || !std::isfinite(l.px)) continue;
        std::string up = l.name; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        if (gamma == 0 && up.find("HVL") != std::string::npos && std::isfinite(closePx)) gamma = closePx > l.px ? 1 : closePx < l.px ? -1 : 0;
        if (!(atr > 0)) continue;
        const double d = std::fabs(l.px - extPx) / atr; if (d <= 0.5 && d < best) { best = d; name = l.name.substr(0, 24); lvd = d; }
    }
    for (char& ch : name) if (ch == '|' || ch == ';' || ch == '=' || ch == ',') ch = ' ';
}

// the 14-candle average true range at candle i (ticks; candles up to i only) - the study's ATR(14)
inline double atrTicks(const std::vector<Bar>& b, size_t i, int n = 14) {
    double s = 0; int k = 0;
    for (size_t j = i + 1; j-- > 1 && k < n;) { if (!b[j].ok || !b[j - 1].ok) continue;
        s += (double)std::max(b[j].h - b[j].l, std::max(std::llabs(b[j].h - b[j - 1].c), std::llabs(b[j].l - b[j - 1].c))); ++k; }
    return k ? s / k : 0.0;
}

// ---- the host side every plugin shares: one file + params per market / bar size, read incrementally, writes queued until flushed -----
struct Host {
    Store st; Params P; std::string mkt, path, ppath; int spb = 0; bool bound = false;
    long long pReadMs = -1, sReadMs = -1, written = 0, writeFails = 0, decided = 0, fired = 0, learning = 0, notLoc = 0, attackerClose = 0, underE = 0, adopted = 0, extras = 0;
    std::vector<std::string> pending; std::map<std::string, bool> xDone;
    // (re)bind to a market / bar size / file; a change starts over (per chart: the caller keeps one Host per chart)
    void bind(const std::string& m, int s, const std::string& file, const std::string& params) {
        if (bound && m == mkt && s == spb && file == path && params == ppath) return;
        *this = Host(); bound = true; mkt = m; spb = s; path = file; ppath = params; st.mkt = m; st.spb = s; P = defaults(m); P.why = "params not read yet - study defaults";
    }
    void readParams() {
        std::ifstream in(ppath.c_str(), std::ios::binary);
        if (ppath.empty() || !in.is_open()) { P = parseParams("", mkt, false); return; }
        std::string t; t.resize(256 * 1024 + 1); in.read(&t[0], (std::streamsize)t.size()); t.resize((size_t)in.gcount());
        P = parseParams(t, mkt, true);
    }
    void flush() {
        if (pending.empty() || path.empty()) return;
        if (append(path, pending)) { written += (long long)pending.size(); pending.clear(); } else ++writeFails;
    }
    // params every 30 s, the record at most once a second, queued lines retried
    void poll(long long nowMs) {
        if (!bound) return;
        if (pReadMs < 0 || nowMs < pReadMs || nowMs - pReadMs >= 30000) { pReadMs = nowMs; readParams(); }
        flush();
        if (sReadMs < 0 || nowMs < sReadMs || nowMs - sReadMs >= 1000) { sReadMs = nowMs; refresh(st, path); }
    }
    // a decided A: shown at once (the store), written once (first line per candle + side wins - another writer's line is kept)
    bool put(const Rec& r) {
        if (st.has(r.side, r.key)) { ++adopted; return false; }
        const std::string l = line(r); st.add(l); pending.push_back(l); ++fired; flush(); return true;
    }
    // extras for an A already in the record, written once per (candle, side, writer)
    bool putX(const Extra& x) {
        if (!st.has(x.side, x.key) || x.kv.empty()) return false;
        const std::string id = std::to_string(x.key) + (x.side == HIGH ? "H" : "L") + x.by;
        if (xDone[id]) return false;
        xDone[id] = true; const std::string l = xline(x); st.add(l); pending.push_back(l); ++extras; flush(); return true;
    }
    void count(const Decision& d) {
        ++decided; if (!d.loc) ++notLoc; else if (d.effort < d.E) ++underE; else if (!d.colour) ++attackerClose;
    }
    std::string statusLine() const {
        std::ostringstream o;
        o << "SHARED_A,sab " << VERSION << ",E " << fmt(P.E, 1) << " (" << P.why << "),location " << locName(LOCATION) << ",record " << path
          << ",A in record " << st.byKey.size() << ",decided here " << decided << ",fired here " << fired << ",adopted " << adopted << ",not at extreme " << notLoc
          << ",under E " << underE << ",attackers' close (no A) " << attackerClose << ",learning " << learning << ",extras " << extras << ",written " << written
          << ",write failures " << writeFails << ",queued " << pending.size() << ",bad lines " << st.bad << ",other market lines " << st.wrongMkt;
        return o.str();
    }
};

// ---- the hover (Rassul's format: the question, what it is, the data points in the agreed order, how to use it, an example) -------------
inline std::string lots(long long v) { std::string d = std::to_string(v < 0 ? -v : v), o; for (size_t k = 0; k < d.size(); ++k) { if (k && (d.size() - k) % 3 == 0) o += ','; o += d[k]; } return (v < 0 ? "-" : "") + o; }
inline std::string xs(double v) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", std::isfinite(v) ? v : 0.0); return b; }
inline std::string pct(double v) { char b[16]; std::snprintf(b, sizeof b, "%.0f%%", v); return b; }
inline std::vector<std::string> hover(const Rec& r, const Params& P) {
    // (independent review 2026-10-22) the A means "this extreme is likely to HOLD" (a support / resistance call), not "price will go 1 ATR
    // your way" (from the A close a fair +/-1 ATR test is a coin flip, +0.7): the question, the held line and the use line say exactly that
    const bool low = r.side == LOW; const std::string m = canon(r.mkt).empty() ? std::string("this market") : canon(r.mkt);
    const std::string att = low ? "sellers" : "buyers", ext = low ? "low" : "high";
    std::vector<std::string> v;
    v.push_back(std::string("A  Absorbed - will this ") + ext + " hold?");
    v.push_back("   What it is: " + att + " " + (low ? "hit " : "lifted ") + xs(r.effort) + " their 20-candle average (" + m + " needs " + xs(r.E) + ") at a "
                + (r.loc == "pivot" ? "pivot " : "5-candle ") + ext + " and the candle still closed " + (low ? "green" : "red") + " - the attack lost the candle");
    v.push_back("   Effort: " + xs(r.effort) + " (" + xs(r.E) + ")  " + lots(r.att) + " lots vs a " + lots((long long)std::llround(r.avg)) + "-lot average");
    v.push_back(std::string("   Close: ") + (low ? "green" : "red") + " - the defenders' colour (" + (low ? "bulls" : "bears") + " won the candle)");
    if (r.book < 0) v.push_back("   Book: n/a (no order feed for this candle)");
    else if (r.book == 0) v.push_back(std::string("   Book: the ") + (low ? "bids" : "offers") + " did not absorb in the wick");
    else v.push_back(std::string("   Book: ") + (low ? "bids" : "offers") + " absorbed " + lots(r.bookLots) + " lots in the wick, " + std::to_string(r.refills) + " refills - a stronger A");
    if (r.pullx < 0) v.push_back("   Pulling: n/a (no order feed after the " + ext + ")");
    else v.push_back("   Pulling: " + att + " pulled their own orders at " + xs(r.pullx) + " normal after the " + ext + (r.pullx >= 2.0 ? " - a stronger A" : ""));
    if (r.e100 >= 0 && r.e100 <= 50) v.push_back("   Attack: faded to " + xs((double)r.e100 / 100.0) + " of its pace after the " + ext);
    else if ((r.into100 >= 150) || (r.e100 >= 150)) v.push_back("   Attack: accelerating " + xs((double)std::max(r.into100, r.e100) / 100.0) + " - a warning (the attack may come back)");
    else if (r.e100 >= 0 || r.into100 >= 0) v.push_back("   Attack: steady (neither faded nor accelerating)");
    else v.push_back("   Attack: pace n/a (no order feed)");
    std::string lv = r.level.empty() ? std::string("no key level within 0.5 ATR") : r.level + " " + fmt(r.lvd, 1) + " ATR away";
    lv += r.gamma > 0 ? "; gamma positive (above HVL)" : r.gamma < 0 ? "; gamma negative (below HVL)" : "; gamma n/a";
    v.push_back("   Level: " + lv);
    if (P.n > 0) {
        char d[24]; std::snprintf(d, sizeof d, "%+.0f", P.held - P.base);
        v.push_back("   Held: " + m + " " + (P.oddsFromFile ? "nightly" : "Sep 10 - Oct 9") + ": this " + ext + " held " + pct(P.held) + " vs " + pct(P.base)
                    + " for comparable 5-candle " + ext + "s (" + d + " points, n " + std::to_string(P.n) + ")" + (P.learning ? " - LEARNING in this market" : ""));
    } else v.push_back("   Held: LEARNING in " + m + " (n 0)");
    const Mkt* k = mktOf(m); char risk[32]; std::snprintf(risk, sizeof risk, "%.1f", k ? k->riskAtr : 1.5);
    v.push_back(std::string("   Use: arms your reversal bar / CoB entry - not a direction call and not an entry on this candle; a stop beyond the ") + ext + " is ~" + risk + " ATR away; H after it = follow-through");
    v.push_back("   Example: ES 8:36 AM low 7,758.75 at ONL - sellers 3.8x (2.0x), closed green; that low held");
    return v;
}
}  // namespace sab
#endif
