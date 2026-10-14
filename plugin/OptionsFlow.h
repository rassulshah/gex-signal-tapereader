#ifndef LS_OPTIONSFLOW_H
#define LS_OPTIONSFLOW_H
// OptionsFlow.h (1.1.0, 2026-10-13) -- SDK-free reader of LRA-Options-<MKT>.csv (written by the LRA bridge, lra.options_irt, after
// every MenthorQ snapshot) and of the show gates in LRA-OrderFlow-<MKT>.csv (written nightly by lra.orderflow_params). Shared by
// lsBidAskStats 1.5.0 / lsPullStats 1.1.0 / lsStackStats 1.1.0 (the blue options stat line under each pane's message) and
// lsTapeReader 1.1.0 (the OPTIONS section). The plugins only READ the file: no runtime dependency beyond it. Unit-tested on Linux.
// Approved by Rassul 2026-10-13 4:49 PM CT (SPEC: /home/claude/aud/of140/SPEC.md).
//
// LRA-Options-<MKT>.csv (every field checked; a bad row is dropped, the rest of the file still counts):
//   VERSION,1
//   MARKET,<ES|NQ|CL|GC|HG|NG|EU>
//   ASOF,<unix s>                      the MenthorQ grid's own time (not the write time)
//   OHIT,<strike>,<P|C>,<ratio>,<thr>,<up|dn|flat>,<side>,<dealers>
//        the strike with the biggest options volume vs its norm since the previous snapshot, that ratio and its threshold, the
//        strike's IV change net of the smile (up = bought / dn = sold), the side read from it (put buyers / put sellers / call buyers
//        / call sellers / ?), and the futures dealers must trade for the snapshot's whole flow (+ buy / - sell, contracts; - = none)
//   OPULL,<level>,<strike>,<gex %>,<up|dn|flat>
//        the MenthorQ level near price whose strike lost the most ABSOLUTE GEX since the RTH open (%), and the front ATM IV change
//   OSTACK,<level>,<strike>,<gex %>,<+|->,<snaps>
//        the MenthorQ level near price whose strike gained the most ABSOLUTE GEX since the open (%), the sign of its NET GEX
//        (+ = wall: dealers long gamma there, - = accelerant), and how many snapshots in a row that strike traded
// (1.1.0, Rassul 5:49 PM "Options content - make this part of the build"): WALL size = the key-level strike's ABSOLUTE GEX as a
//   ratio vs normal + its NET sign "(+)" holds / pins, "(-)" accelerant; building / fading vs the change price / time alone would cause
//   (+ the next-morning OI change d/d); REGIME above / below HVL; DEALERS forced to trade = net DEX change in contracts and $ notional,
//   shown only at >= 1.5x its norm and never sized for CL / NG (sign doubtful there); options BUYERS vs SELLERS = strike volume spike
//   + strike IV residual (price effect removed) + skew direction. Extra rows (the old ones still parse):
//   OPULL,<level>,<strike>,<gex %>,<iv>[,<+|->]                     (1.1.0: + the net sign)
//   OSTACK,<level>,<strike>,<gex %>,<+|->,<snaps>[,<wall x>,<building|fading|steady>]
//   OWALL,<level>,<strike>,<wall x>,<+|->,<chg %>,<building|fading|steady>,<OI d/d % | ->      the key-level wall now (nearest)
//   ODEX,<contracts>,<usd>,<ratio>,<sized 0|1>                      dealers' forced futures for the last grid (+ buy)
//   OHVL,<hvl>,<above|below>                                        the regime
//   OSKEW,<up|dn|flat>                                              front skew (put wing - call wing) since the open: up = fear up
//   OLOG,...                                                        logged only (abs DEX, vanna, charm, 0DTE) - never drawn
// Stale: a line is shown only while (reference time - ASOF) <= 20 min (reference = the candle's end for a hovered candle, else now).
// Gates (LRA-OrderFlow-<MKT>.csv): PACE_SHOW / OHIT_SHOW / OPULL_SHOW / OSTACK_SHOW / REBUILT_SHOW,<on|?|off>; missing = "?".
//   on = shown plain, ? = shown with " ?" (not proven yet in this market), off = not shown.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace ofo {

static const char* const VERSION = "1.1.0";
enum Gate { GATE_OFF = 0, GATE_Q = 1, GATE_ON = 2 };
enum GateId { G_PACE = 0, G_OHIT = 1, G_OPULL = 2, G_OSTACK = 3, G_REBUILT = 4, G_ODEX = 5, G_OHVL = 6, NGATE = 7 };
static const char* const GATE_KEY[NGATE] = { "PACE_SHOW", "OHIT_SHOW", "OPULL_SHOW", "OSTACK_SHOW", "REBUILT_SHOW", "ODEX_SHOW", "OHVL_SHOW" };
inline void gatesDefault(int g[NGATE]) { for (int i = 0; i < NGATE; ++i) g[i] = GATE_Q; }
static_assert(NGATE == 7, "every gate array initializer lists 7 values (a missing one would read 0 = off)");
static const long long STALE_S = 20 * 60;          // (SPEC) > 20 min old = hidden
static const long long FUTURE_S = 5 * 60;          // a grid stamped more than 5 min in the future = not trusted

inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }
inline std::string trimS(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}
inline std::vector<std::string> cells(const std::string& ln) {
    std::vector<std::string> c; size_t a = 0;
    for (;;) { const size_t p = ln.find(',', a); c.push_back(trimS(ln.substr(a, p == std::string::npos ? std::string::npos : p - a))); if (p == std::string::npos) break; a = p + 1; }
    return c;
}
inline bool real(const std::string& t, double& out) {
    if (t.empty() || t.size() > 32) return false;
    char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (!e || *e != 0 || !finiteD(v)) return false;
    out = v; return true;
}
inline bool whole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    size_t i = t[0] == '-' || t[0] == '+' ? 1 : 0; if (i >= t.size()) return false;
    for (size_t k = i; k < t.size(); ++k) if (t[k] < '0' || t[k] > '9') return false;
    const long long v = std::strtoll(t.c_str(), nullptr, 10);
    if (v < lo || v > hi) return false;
    out = v; return true;
}
inline int ivDirOf(const std::string& t, bool& ok) { ok = true; if (t == "up") return 1; if (t == "dn") return -1; if (t == "flat") return 0; ok = false; return 0; }
inline const char* ivText(int d) { return d > 0 ? "up" : d < 0 ? "dn" : "flat"; }
inline bool levelName(const std::string& s) {
    if (s.empty() || s.size() > 8) return false;
    for (char c : s) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    return true;
}

struct Hit { bool ok = false; double strike = 0; char type = 'P'; double ratio = 0, thr = 2.0; int iv = 0; std::string side; bool dealersOk = false; long long dealers = 0; };
struct Pull { bool ok = false; std::string lvl; double strike = 0; long long gexPct = 0; int iv = 0; int sign = 0; };   // sign 0 = not given (1.0 file)
struct Stack { bool ok = false; std::string lvl; double strike = 0; long long gexPct = 0; int sign = 0; long long snaps = 0; double wallX = -1; int vsExp = 0; };   // vsExp +1 building, -1 fading
struct Wall { bool ok = false; std::string lvl; double strike = 0, x = 0; int sign = 0; long long chgPct = 0; int vsExp = 0; bool oiOk = false; long long oiPct = 0; };
struct Dex { bool ok = false; long long contracts = 0, usd = 0; double ratio = 0; bool sized = false; };
struct Hvl { bool ok = false; double px = 0; bool above = false; };
struct Snap { bool ok = false; long long asof = 0; std::string market; Hit hit; Pull pull; Stack stack; Wall wall; Dex dex; Hvl hvl; int skew = 0; bool skewOk = false; std::vector<std::string> notes; };
inline int vsOf(const std::string& t, bool& ok) { ok = true; if (t == "building") return 1; if (t == "fading") return -1; if (t == "steady") return 0; ok = false; return 0; }

inline bool sideOk(const std::string& s) { return s == "put buyers" || s == "put sellers" || s == "call buyers" || s == "call sellers" || s == "?"; }

// the whole file's lines (bounded by the caller); ok = VERSION,1 first, MARKET matches, ASOF present
inline Snap parse(const std::vector<std::string>& lines, const std::string& market) {
    Snap S; bool ver = false, mkt = false, asof = false;
    for (size_t i = 0; i < lines.size() && i < 200; ++i) {
        const std::string ln = trimS(lines[i]);
        if (ln.empty() || ln[0] == '#') continue;
        if (ln.size() > 300) { S.notes.push_back("line " + std::to_string(i + 1) + " too long"); continue; }
        const std::vector<std::string> c = cells(ln);
        if (!ver) { if (c.size() == 2 && c[0] == "VERSION" && c[1] == "1") { ver = true; continue; } S.notes.push_back("first line must be VERSION,1"); return S; }
        if (c[0] == "MARKET") { if (c.size() != 2 || c[1] != market) { S.notes.push_back("MARKET is not " + market); return S; } mkt = true; S.market = c[1]; continue; }
        if (c[0] == "ASOF") { long long v = 0; if (c.size() != 2 || !whole(c[1], 1000000000LL, 32503680000LL, v)) { S.notes.push_back("bad ASOF"); return S; } S.asof = v; asof = true; continue; }
        if (c[0] == "OHIT") {
            Hit h; double v = 0; bool ok = c.size() == 8, d = false;
            if (ok) ok = real(c[1], h.strike) && h.strike > 0;
            if (ok) { ok = c[2] == "P" || c[2] == "C"; h.type = ok ? c[2][0] : 'P'; }
            if (ok) ok = real(c[3], h.ratio) && h.ratio >= 0 && h.ratio < 1e6;
            if (ok) ok = real(c[4], h.thr) && h.thr > 0 && h.thr < 100;
            if (ok) { h.iv = ivDirOf(c[5], d); ok = d; }
            if (ok) { ok = sideOk(c[6]); h.side = c[6]; }
            if (ok) { if (c[7] == "-") h.dealersOk = false; else { long long n = 0; ok = whole(c[7], -100000000LL, 100000000LL, n); h.dealers = n; h.dealersOk = ok; } }
            (void)v;
            if (!ok) { S.notes.push_back("bad OHIT row"); continue; }
            h.ok = true; S.hit = h; continue;
        }
        if (c[0] == "OPULL") {
            Pull p; bool ok = c.size() == 5 || c.size() == 6, d = false;
            if (ok) { ok = levelName(c[1]); p.lvl = c[1]; }
            if (ok) ok = real(c[2], p.strike) && p.strike > 0;
            if (ok) ok = whole(c[3], -100000, 100000, p.gexPct);
            if (ok) { p.iv = ivDirOf(c[4], d); ok = d; }
            if (ok && c.size() == 6) { ok = c[5] == "+" || c[5] == "-"; p.sign = c[5] == "+" ? 1 : -1; }
            if (!ok) { S.notes.push_back("bad OPULL row"); continue; }
            p.ok = true; S.pull = p; continue;
        }
        if (c[0] == "OSTACK") {
            Stack k; bool ok = c.size() == 6 || c.size() == 8;
            if (ok) { ok = levelName(c[1]); k.lvl = c[1]; }
            if (ok) ok = real(c[2], k.strike) && k.strike > 0;
            if (ok) ok = whole(c[3], -100000, 100000, k.gexPct);
            if (ok) { ok = c[4] == "+" || c[4] == "-"; k.sign = c[4] == "+" ? 1 : -1; }
            if (ok) ok = whole(c[5], 0, 1000, k.snaps);
            if (ok && c.size() == 8) { bool d = false; ok = real(c[6], k.wallX) && k.wallX >= 0 && k.wallX < 1e6; if (ok) { k.vsExp = vsOf(c[7], d); ok = d; } }
            if (!ok) { S.notes.push_back("bad OSTACK row"); continue; }
            k.ok = true; S.stack = k; continue;
        }
        if (c[0] == "OWALL") {
            Wall w; bool ok = c.size() == 8, d = false;
            if (ok) { ok = levelName(c[1]); w.lvl = c[1]; }
            if (ok) ok = real(c[2], w.strike) && w.strike > 0;
            if (ok) ok = real(c[3], w.x) && w.x >= 0 && w.x < 1e6;
            if (ok) { ok = c[4] == "+" || c[4] == "-"; w.sign = c[4] == "+" ? 1 : -1; }
            if (ok) ok = whole(c[5], -100000, 100000, w.chgPct);
            if (ok) { w.vsExp = vsOf(c[6], d); ok = d; }
            if (ok) { if (c[7] == "-") w.oiOk = false; else { ok = whole(c[7], -100000, 100000, w.oiPct); w.oiOk = ok; } }
            if (!ok) { S.notes.push_back("bad OWALL row"); continue; }
            w.ok = true; S.wall = w; continue;
        }
        if (c[0] == "ODEX") {
            Dex x; bool ok = c.size() == 5; double r = 0;
            if (ok) ok = whole(c[1], -100000000LL, 100000000LL, x.contracts) && whole(c[2], -1000000000000000LL, 1000000000000000LL, x.usd);
            if (ok) ok = real(c[3], r) && r >= 0 && r < 1e6;
            if (ok) { ok = c[4] == "0" || c[4] == "1"; x.sized = c[4] == "1"; }
            if (!ok) { S.notes.push_back("bad ODEX row"); continue; }
            x.ratio = r; x.ok = true; S.dex = x; continue;
        }
        if (c[0] == "OHVL") {
            Hvl h; bool ok = c.size() == 3 && real(c[1], h.px) && h.px > 0 && (c[2] == "above" || c[2] == "below");
            if (!ok) { S.notes.push_back("bad OHVL row"); continue; }
            h.above = c[2] == "above"; h.ok = true; S.hvl = h; continue;
        }
        if (c[0] == "OSKEW") {
            bool d = false; const int v = c.size() == 2 ? ivDirOf(c[1], d) : 0;
            if (!d) { S.notes.push_back("bad OSKEW row"); continue; }
            S.skew = v; S.skewOk = true; continue;
        }
        // other upper-case keys: ignored (the bridge may add fields)
    }
    if (!ver) { S.notes.push_back("empty file"); return S; }
    if (!mkt) { S.notes.push_back("no MARKET line"); return S; }
    if (!asof) { S.notes.push_back("no ASOF line"); return S; }
    S.ok = true; return S;
}
// fresh at the reference time (unix s): not older than 20 min, not more than 5 min in the future
inline bool fresh(const Snap& S, long long ref) { return S.ok && S.asof > 0 && ref > 0 && ref - S.asof <= STALE_S && S.asof - ref <= FUTURE_S; }

// gates from the LRA-OrderFlow-<MKT>.csv lines: <KEY>,<on|?|off>; anything else = "?"
inline void parseGates(const std::vector<std::string>& lines, int out[NGATE]) {
    for (int g = 0; g < NGATE; ++g) out[g] = GATE_Q;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::vector<std::string> c = cells(trimS(lines[i]));
        if (c.size() != 2) continue;
        for (int g = 0; g < NGATE; ++g) if (c[0] == GATE_KEY[g]) out[g] = c[1] == "on" ? GATE_ON : c[1] == "off" ? GATE_OFF : GATE_Q;
    }
}
inline const char* gateText(int g) { return g == GATE_ON ? "on" : g == GATE_OFF ? "off" : "?"; }
inline std::string q(int gate) { return gate == GATE_ON ? std::string() : std::string(" ?"); }

// ---- text ---------------------------------------------------------------------------------------------------------------------
inline std::string strikeText(double k) {
    char b[32]; std::snprintf(b, sizeof(b), "%.4f", k);
    std::string s = b; while (!s.empty() && s.back() == '0') s.pop_back(); if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}
inline std::string kText(long long n) {           // 1200 -> "1.2K", 850 -> "850", 25000 -> "25K"
    if (n < 0) n = -n;
    if (n < 1000) return std::to_string(n);
    char b[24];
    if (n < 10000) std::snprintf(b, sizeof(b), "%.1fK", (double)n / 1000.0); else std::snprintf(b, sizeof(b), "%lldK", (n + 500) / 1000);
    std::string s = b; const size_t p = s.find(".0K"); if (p != std::string::npos) s.erase(p, 2);
    return s;
}
inline std::string ratioX(double r) { char b[24]; std::snprintf(b, sizeof(b), "%.1fx", r > 999 ? 999.0 : r); return b; }
inline std::string mktText(const std::string& m) { return m == "EU" ? "6E" : m; }
inline std::string pctText(long long p) { return (p > 0 ? "+" : p < 0 ? "-" : "") + std::to_string(p < 0 ? -p : p) + "%"; }
inline std::string usdText(long long u) {           // 487000000 -> "$487M", 1.2e9 -> "$1.2B", 85000 -> "$85K"
    if (u < 0) u = -u;
    char b[32];
    if (u >= 1000000000LL) std::snprintf(b, sizeof(b), "$%.1fB", (double)u / 1e9);
    else if (u >= 1000000LL) std::snprintf(b, sizeof(b), "$%lldM", (u + 500000) / 1000000);
    else std::snprintf(b, sizeof(b), "$%lldK", (u + 500) / 1000);
    return b;
}
static const char* const DOT = " \xB7 ";                // the separator of the SPEC examples (the chart's ANSI code page)
inline std::string signText(int s) { return s > 0 ? "(+)" : s < 0 ? "(-)" : ""; }
// (1.1.0) the dealers' forced futures, shown only when sized (not CL / NG) and >= 1.5x the norm: "dealers buy 1.2K ES ($487M)"
inline bool dexShown(const Snap& S, int gate) { return S.ok && S.dex.ok && S.dex.sized && S.dex.contracts != 0 && S.dex.ratio >= 1.5 - 1e-9 && gate != GATE_OFF; }
inline std::string dexText(const Snap& S, const std::string& mkt) {
    return std::string(S.dex.contracts > 0 ? "buy " : "sell ") + kText(S.dex.contracts) + " " + mktText(mkt) + " (" + usdText(S.dex.usd) + ")";
}
// options buyers / sellers from the strike's IV residual: "-> sellers" / "-> buyers" ('?' = not readable)
// (6:18 PM smoke run on the real 13 Oct grids) buyers / sellers are named only on a volume SPIKE (ratio >= its threshold, SPEC item 4):
// a 1.0x strike with an IV tick is not a read of who is trading
inline bool hitSpike(const Hit& h) { return h.ok && h.thr > 0 && h.ratio + 1e-9 >= h.thr; }
inline std::string whoText(const Hit& h) {
    if (!hitSpike(h)) return std::string();
    if (h.side.find("buyers") != std::string::npos) return "buyers";
    if (h.side.find("sellers") != std::string::npos) return "sellers";
    return std::string();
}
// the panes' blue stat lines (SPEC 5:49 PM examples; ASCII arrows / minus, the middle dot in the chart's code page)
//   "oHit: 5800 puts 2.4x \xB7 IV dn -> sellers ? \xB7 dealers buy 1.2K ES ($487M)"
inline std::string hitLine(const Snap& S, int gate, const std::string& mkt, int dexGate = GATE_Q) {
    if (!S.ok || !S.hit.ok || gate == GATE_OFF) return std::string();
    const Hit& h = S.hit;
    std::string o = "oHit: " + strikeText(h.strike) + (h.type == 'P' ? " puts " : " calls ") + ratioX(h.ratio) + DOT + "IV " + ivText(h.iv);
    const std::string w = whoText(h);
    if (!w.empty()) o += " -> " + w;
    o += q(gate);
    if (dexShown(S, dexGate)) o += std::string(DOT) + "dealers " + dexText(S, mkt) + q(dexGate);
    return o;
}
//   "oPull: CR 5840 wall -30% since open (-) \xB7 IV dn ?"
inline std::string pullLine(const Snap& S, int gate) {
    if (!S.ok || !S.pull.ok || gate == GATE_OFF) return std::string();
    const std::string sg = signText(S.pull.sign);
    return "oPull: " + S.pull.lvl + " " + strikeText(S.pull.strike) + " wall " + pctText(S.pull.gexPct) + " since open" + (sg.empty() ? "" : " " + sg) + DOT + "IV " + ivText(S.pull.iv) + q(gate);
}
//   "oStack: PS 5790 wall 2.4x (+) building +40% \xB7 3 snaps ?"
inline std::string stackLine(const Snap& S, int gate) {
    if (!S.ok || !S.stack.ok || gate == GATE_OFF) return std::string();
    const Stack& k = S.stack;
    std::string o = "oStack: " + k.lvl + " " + strikeText(k.strike) + " wall";
    if (k.wallX >= 0) o += " " + ratioX(k.wallX);
    o += " " + signText(k.sign);
    o += k.vsExp > 0 ? " building " : k.vsExp < 0 ? " fading " : " ";
    o += pctText(k.gexPct) + (k.snaps > 0 ? std::string(DOT) + std::to_string(k.snaps) + (k.snaps == 1 ? " snap" : " snaps") : std::string()) + q(gate);   // (6:18 PM) no "0 snaps"
    return o;
}
// the Tape Reader's OPTIONS lines (SPEC 5:49 PM):
//   "PS 5790 wall 2.4x (+) building \xB7 above HVL"   /   "Dealers buy 1.2K ES ($487M) 2.1x \xB7 put buyers fading ?"
inline std::string trLine1(const Snap& S, const int gates[NGATE]) {
    if (!S.ok) return std::string();
    std::vector<std::string> parts;
    if (S.wall.ok && gates[G_OSTACK] != GATE_OFF) {
        const Wall& w = S.wall;
        parts.push_back(w.lvl + " " + strikeText(w.strike) + " wall " + ratioX(w.x) + " " + signText(w.sign) + (w.vsExp > 0 ? " building" : w.vsExp < 0 ? " fading" : "") + q(gates[G_OSTACK]));
    } else if (S.stack.ok && gates[G_OSTACK] != GATE_OFF)
        parts.push_back(S.stack.lvl + " " + strikeText(S.stack.strike) + " wall " + pctText(S.stack.gexPct) + " " + signText(S.stack.sign) + q(gates[G_OSTACK]));
    else if (S.pull.ok && gates[G_OPULL] != GATE_OFF)
        parts.push_back(S.pull.lvl + " " + strikeText(S.pull.strike) + " wall " + pctText(S.pull.gexPct) + (S.pull.sign ? " " + signText(S.pull.sign) : std::string()) + q(gates[G_OPULL]));
    if (S.hvl.ok && gates[G_OHVL] != GATE_OFF) parts.push_back(std::string(S.hvl.above ? "above" : "below") + " HVL" + q(gates[G_OHVL]));
    std::string o; for (size_t i = 0; i < parts.size(); ++i) o += (i ? DOT : "") + parts[i];
    return o;
}
// dir is kept for the caller's API (the bounce / drop wording of 1.0.0 was replaced by the SPEC's ratio)
inline std::string trLine2(const Snap& S, const int gates[NGATE], const std::string& mkt, int dir) {
    (void)dir;
    if (!S.ok) return std::string();
    std::vector<std::string> parts;
    if (dexShown(S, gates[G_ODEX])) { std::string d = "Dealers " + dexText(S, mkt) + " " + ratioX(S.dex.ratio) + q(gates[G_ODEX]); parts.push_back(d); }
    if (hitSpike(S.hit) && gates[G_OHIT] != GATE_OFF) {   // (6:18 PM) only on a strike volume spike
        const std::string who = S.hit.type == 'P' ? "put" : "call";
        std::string p;
        if (S.hit.iv < 0) p = who + " buyers fading";
        else if (S.hit.iv > 0) p = who + " buyers active";
        else p = who + " volume " + ratioX(S.hit.ratio);
        if (S.skewOk && S.skew != 0) p += S.skew > 0 ? ", fear up" : ", fear fading";
        parts.push_back(p + q(gates[G_OHIT]));
    }
    std::string o; for (size_t i = 0; i < parts.size(); ++i) o += (i ? DOT : "") + parts[i];
    if (!o.empty() && o[0] >= 'a' && o[0] <= 'z') o[0] = (char)(o[0] - 'a' + 'A');
    return o;
}

// ---- a snapshot history: each new grid is kept with the bar it ARRIVED on (drawn there, never earlier: no repaint) ------------
struct Arrived { long long barKey = 0; Snap s; };
struct History {
    std::vector<Arrived> v; long long lastAsof = 0;
    // a newly read snapshot; kept only when its grid time is new. barKey = the forming bar (the bar it arrived on)
    bool add(const Snap& s, long long barKey) {
        if (!s.ok || s.asof <= 0 || s.asof == lastAsof || barKey <= 0) return false;
        lastAsof = s.asof; Arrived a; a.barKey = barKey; a.s = s; v.push_back(a);
        if (v.size() > 400) v.erase(v.begin());
        return true;
    }
    // the latest snapshot that arrived on or before the bar t (freshness checked by the caller); nullptr = none
    const Snap* latest(long long t) const { for (size_t i = v.size(); i-- > 0;) if (v[i].barKey <= t) return &v[i].s; return nullptr; }
    // the snapshot a candle (bar end t) shows: the latest that arrived on or before it, fresh at ref; nullptr = none
    const Snap* at(long long t, long long ref) const {
        for (size_t i = v.size(); i-- > 0;) if (v[i].barKey <= t) return fresh(v[i].s, ref) ? &v[i].s : nullptr;
        return nullptr;
    }
};

} // namespace ofo
#endif
