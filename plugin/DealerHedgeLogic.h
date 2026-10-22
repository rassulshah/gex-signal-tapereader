#ifndef LS_DEALERHEDGELOGIC_H
#define LS_DEALERHEDGELOGIC_H
// DealerHedgeLogic.h  0.1.0  (2026-10-22, lsDealerHedge 0.1.0)  --  "oH" DEALER-HEDGING ZONES, the SDK-free logic (unit-tested on Linux).
// Rassul agreed the design 15:23-15:24 CT and ordered the build 15:31 CT (MBO Roadmap row "Are dealers hedging in the futures book at
// the gamma level?", ticket T-20261022-27, "special attention").
//
// QUESTION it answers: are dealers hedging in the futures book at this gamma level right now - buying or selling?
// ZONE      each gamma level near price (MenthorQ CR / PS / HVL / 0DTE levels / gamma walls / G1-G10 from MenthorQLevels.csv, the SAME
//           file and parse the Dealer Profile draws from, + the CME gamma nodes of LRA-GexNodes-<MKT>.csv when present) +/- 0.5 ATR(14)
//           of the chart's bars. Levels closer than half a zone share one zone (names joined, the first by priority leads).
// INVISIBLE until hedging activity shows inside the zone. FIRST DETECTION RULE (to be sharpened by the nightly test; every number is a
//           per-market default, a PARAM row of lsFlexLevels\LRA-DealerHedge-<MKT>.csv or a setting):
//           inside the zone, over the last 60 s, one side's flow (passive adds + aggressive fills, lots) >= 2.0x NORMAL and >= MIN_LOTS,
//           NORMAL = the same side's flow in a band of the same width around price, for this market and 30-min slot (median of the
//           last 5 sessions' slot records, else the rolling last 30 min), AND any of
//             (a) SAME-SIZE orders repeating: the same size (>= REP_MIN_LOTS), same side, >= 4 new orders within 60 s, interval CV < 0.5
//             (b) steady REFILLS at one price: >= 4 adds at a price just hit (within 10 s) that rest >= 1 s
//             (c) a BURST: passive adds or aggressive fills alone >= 2.0x their normal
//           Side: passive bids / aggressive buys = dealers BUYING (green); offers / aggressive sells = SELLING (red).
// LIVE -> SETTLED: the zone lights up and PULSES while activity qualifies (and 20 s after); then a steady faint band. Never removed,
//           never changed; pulses again when activity restarts. The first decision is recorded (Z line) with the candle it was decided
//           on ("oH" below that bar); each restart is an L line. Restart = the record redrawn (no repaint).
// GAMMA CROSS-CHECK: long-gamma dealers buy dips into support and sell rallies into resistance; short-gamma dealers chase. The BOOK
//           side is shown; the gamma expectation (node sign W / F, else the HVL regime) and whether it agrees go in the hover.
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dh {
static const char* const VERSION = "0.1.0";
typedef long long Tick;
enum { BUY = 0, SELL = 1 };                        // side 0 = bids / aggressive buys (green), 1 = offers / aggressive sells (red)
enum { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum { HIDDEN = 0, LIVE = 1, SETTLED = 2 };
enum { M_ES, M_NQ, M_CL, M_GC, M_HG, M_NG, M_EU, M_N };
inline int marketIdx(const std::string& m) {
    static const char* const N[M_N] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
    for (int i = 0; i < M_N; ++i) if (m == N[i]) return i;
    return -1;
}

// ---- parameters ------------------------------------------------------------------------------------------------------------------
struct Params {
    double widthAtr = 0.5;      // zone = level +/- widthAtr x ATR
    int atrLen = 14;
    double reachAtr = 3.0;      // only levels within reachAtr x ATR of price are watched
    double flowX = 2.0;         // zone flow vs normal
    double burstX = 2.0;        // (c) adds or fills alone vs their normal
    int winS = 60;              // the look-back window
    long long minLots = 40;     // the zone flow must also be at least this many lots in the window
    int repN = 4;               // (a) same-size new orders
    double repCv = 0.5;
    long long repMinLots = 3;
    int refN = 4;               // (b) refills
    int refRestMs = 1000;
    int refWinMs = 10000;       // an add counts as a refill when its price was hit this recently
    int liveHoldS = 20;         // LIVE lasts this long after the last qualifying second
    int freshS = 5;             // a second qualifies only with new zone activity on that side in the last freshS seconds
    int normMin = 30;           // rolling normal: the last normMin minutes ...
    int normNeedMin = 10;       // ... needs at least this many
    int normSessions = 5;       // slot normal: median of the last N sessions' slot rows (needs 2)
    double nullShiftAtr = 0;    // replay only: the zone centre moved by this x ATR (the null test)
};
inline Params defaultsFor(int m) {
    Params p;
    static const long long minL[M_N]  = { 150, 40, 40, 30, 15, 30, 30 };
    static const long long repMin[M_N] = { 10, 3, 3, 3, 2, 3, 3 };
    if (m >= 0 && m < M_N) { p.minLots = minL[m]; p.repMinLots = repMin[m]; }
    return p;
}
inline bool numTok(const std::string& s, double& out) {
    if (s.empty() || s.size() > 32) return false;
    char* e = nullptr; errno = 0; const double v = std::strtod(s.c_str(), &e);
    if (errno || !e || *e || !std::isfinite(v)) return false;
    out = v; return true;
}
inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
inline std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> v; size_t a = 0;
    for (;;) { const size_t b = s.find(sep, a); v.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return v;
}
inline std::vector<std::string> lines(const std::string& t) {
    std::vector<std::string> v; size_t a = 0;
    while (a < t.size()) { size_t b = t.find('\n', a); if (b == std::string::npos) b = t.size(); std::string l = t.substr(a, b - a); if (!l.empty() && l.back() == '\r') l.pop_back(); v.push_back(l); a = b + 1; }
    return v;
}
// slot normal rows: NORM,<slot start minute of day>,<buy adds/min>,<sell adds/min>,<buy fills/min>,<sell fills/min>
struct SlotNorm { double add[2] = {0, 0}, fill[2] = {0, 0}; };
struct ParamLoad { Params p; std::string source = "defaults"; std::vector<std::string> notes; std::map<int, SlotNorm> norms; };
// lsFlexLevels\LRA-DealerHedge-<MKT>.csv: VERSION,1 / MARKET,<MKT> / PARAM,<NAME>,<value> / NORM,... ; garbled = defaults + the reason
inline ParamLoad parseParams(const std::string& text, bool found, const std::string& market) {
    ParamLoad L; L.p = defaultsFor(marketIdx(market));
    if (!found) { L.source = "defaults (no LRA-DealerHedge-" + market + ".csv)"; return L; }
    if (text.size() > 64 * 1024) { L.source = "defaults (params file over 64 KiB)"; return L; }
    const std::vector<std::string> ls = lines(text);
    bool ver = false, mk = false; Params p = L.p; int used = 0; std::map<int, SlotNorm> norms;
    for (size_t i = 0; i < ls.size() && i < 512; ++i) {
        const std::string ln = trim(ls[i]); if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = split(ln, ',');
        const std::string k = trim(c[0]);
        if (k == "VERSION") { ver = c.size() >= 2 && trim(c[1]) == "1"; continue; }
        if (k == "MARKET") { if (c.size() < 2 || trim(c[1]) != market) { L.source = "defaults (params file is for another market)"; return L; } mk = true; continue; }
        if (k == "NORM" && c.size() >= 6) {
            double s = 0, a0 = 0, a1 = 0, f0 = 0, f1 = 0;
            if (!numTok(trim(c[1]), s) || !numTok(trim(c[2]), a0) || !numTok(trim(c[3]), a1) || !numTok(trim(c[4]), f0) || !numTok(trim(c[5]), f1)
                || s < 0 || s >= 1440 || std::fmod(s, 30) != 0 || a0 < 0 || a1 < 0 || f0 < 0 || f1 < 0 || a0 > 1e7 || a1 > 1e7 || f0 > 1e7 || f1 > 1e7) { L.notes.push_back("bad NORM row " + std::to_string(i + 1)); continue; }
            SlotNorm n; n.add[0] = a0; n.add[1] = a1; n.fill[0] = f0; n.fill[1] = f1; norms[(int)s] = n; continue;
        }
        if (k != "PARAM" || c.size() < 3) continue;
        const std::string nm = trim(c[1]); double v = 0;
        if (!numTok(trim(c[2]), v)) { L.notes.push_back("bad value for " + nm); continue; }
        bool ok = true;
        if (nm == "WIDTH_ATR") { ok = v >= 0.1 && v <= 2.0; if (ok) p.widthAtr = v; }
        else if (nm == "REACH_ATR") { ok = v >= 0.5 && v <= 20; if (ok) p.reachAtr = v; }
        else if (nm == "FLOW_X") { ok = v >= 1.0 && v <= 20; if (ok) p.flowX = v; }
        else if (nm == "BURST_X") { ok = v >= 1.0 && v <= 20; if (ok) p.burstX = v; }
        else if (nm == "WIN_S") { ok = v >= 10 && v <= 600 && v == std::floor(v); if (ok) p.winS = (int)v; }
        else if (nm == "MIN_LOTS") { ok = v >= 0 && v <= 100000 && v == std::floor(v); if (ok) p.minLots = (long long)v; }
        else if (nm == "REP_N") { ok = v >= 2 && v <= 50 && v == std::floor(v); if (ok) p.repN = (int)v; }
        else if (nm == "REP_CV") { ok = v > 0 && v <= 2; if (ok) p.repCv = v; }
        else if (nm == "REP_MIN_LOTS") { ok = v >= 1 && v <= 10000 && v == std::floor(v); if (ok) p.repMinLots = (long long)v; }
        else if (nm == "REF_N") { ok = v >= 2 && v <= 50 && v == std::floor(v); if (ok) p.refN = (int)v; }
        else if (nm == "REF_REST_MS") { ok = v >= 0 && v <= 60000 && v == std::floor(v); if (ok) p.refRestMs = (int)v; }
        else if (nm == "REF_WIN_MS") { ok = v >= 1000 && v <= 120000 && v == std::floor(v); if (ok) p.refWinMs = (int)v; }
        else if (nm == "FRESH_S") { ok = v >= 1 && v <= 60 && v == std::floor(v); if (ok) p.freshS = (int)v; }
        else if (nm == "LIVE_HOLD_S") { ok = v >= 2 && v <= 600 && v == std::floor(v); if (ok) p.liveHoldS = (int)v; }
        else if (nm == "NORM_MIN") { ok = v >= 5 && v <= 240 && v == std::floor(v); if (ok) p.normMin = (int)v; }
        else if (nm == "ATR_LEN") { ok = v >= 2 && v <= 100 && v == std::floor(v); if (ok) p.atrLen = (int)v; }
        else { L.notes.push_back("unknown PARAM " + nm); continue; }
        if (!ok) L.notes.push_back("out of range: " + nm); else ++used;
    }
    if (!ver || !mk) { L.source = "defaults (params file without VERSION,1 / MARKET)"; return L; }
    L.p = p; L.norms = norms; L.source = "LRA-DealerHedge-" + market + ".csv (" + std::to_string(used) + " params, " + std::to_string(norms.size()) + " slot norms)";
    return L;
}

// ---- levels ----------------------------------------------------------------------------------------------------------------------
struct Level {
    std::string name; double px = 0; bool zero = false;      // zero: a 0DTE level
    int prio = 99;                                           // lower leads a shared zone
    double gexM = -1;                                        // |GEX| $M (per 1% move, MenthorQ convention) from a node; -1 unknown
    char sign = 0;                                           // node sign: 'W' long gamma, 'F' short, 'M' mixed, 0 unknown
};
inline std::string mqMarket(const std::string& sym) {
    static const char* const P[7][2] = { {"ENQ", "NQ"}, {"EP", "ES"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"} };
    for (int k = 0; k < 7; ++k) if (sym.compare(0, std::strlen(P[k][0]), P[k][0]) == 0) return P[k][1];
    return "";
}
inline bool monthOf(const std::string& s, char& m, char& y) {
    const size_t n = s.size(); auto isM = [](char c) { return c && std::strchr("FGHJKMNQUVXZ", c) != NULL; };
    if (n >= 3 && isM(s[n - 2]) && std::isdigit((unsigned char)s[n - 1])) { m = s[n - 2]; y = s[n - 1]; return true; }
    if (n >= 4 && isM(s[n - 3]) && std::isdigit((unsigned char)s[n - 2]) && std::isdigit((unsigned char)s[n - 1])) { m = s[n - 3]; y = s[n - 1]; return true; }
    return false;
}
// MenthorQLevels.csv (SYMBOL,PRICE,LABEL,...) - the file and the month rule lsBigBidAsk / the Dealer Profile use: this market, the chart's
// contract month only; labels split on '/': "CR 0D" -> "CR 0DTE", "GW 0D" -> "GW 0DTE", "HVL" / "HVL 0D", "PS", "GW", "G1"-"G10".
inline std::vector<Level> parseMq(const std::string& text, const std::string& mkt, const std::string& chartSym, double tick) {
    std::vector<Level> out; if (mkt.empty() || !(tick > 0) || text.size() > 64 * 1024) return out;
    char cm = 0, cy = 0; const bool haveM = monthOf(chartSym, cm, cy);
    const std::vector<std::string> ls = lines(text);
    for (size_t i = 0; i < ls.size() && out.size() < 60; ++i) {
        const std::string& ln = ls[i];
        if (ln.empty() || ln.size() > 512 || ln.compare(0, 6, "SYMBOL") == 0) continue;
        const std::vector<std::string> c = split(ln, ','); if (c.size() < 3) continue;
        std::string sym; for (char ch : c[0]) sym += (char)std::toupper((unsigned char)ch);
        if (mqMarket(sym) != mkt) continue;
        char fm = 0, fy = 0; if (haveM && monthOf(sym, fm, fy) && (fm != cm || fy != cy)) continue;
        double px = 0; if (!numTok(trim(c[1]), px) || !(px > 0)) continue;
        px = std::round(px / tick) * tick;
        const std::vector<std::string> toks = split(c[2], '/');
        for (const std::string& t0 : toks) {
            const std::string tok = trim(t0); if (tok.empty()) continue;
            std::string w = tok, rest; const size_t sp = tok.find(' '); if (sp != std::string::npos) { w = tok.substr(0, sp); rest = trim(tok.substr(sp + 1)); }
            const bool zero = rest == "0D" || rest == "0DTE";
            Level L; L.px = px; L.zero = zero;
            if (w == "CR") { L.name = zero ? "CR 0DTE" : "CR"; L.prio = zero ? 0 : 4; }
            else if (w == "PS") { L.name = zero ? "PS 0DTE" : "PS"; L.prio = zero ? 1 : 5; }
            else if (w == "HVL") { L.name = zero ? "HVL 0DTE" : "HVL"; L.prio = zero ? 2 : 6; }
            else if (w == "GW") { L.name = zero ? "GW 0DTE" : "GW"; L.prio = zero ? 3 : 7; }
            else if (w.size() >= 2 && w.size() <= 3 && w[0] == 'G' && std::isdigit((unsigned char)w[1]) && (w.size() == 2 || std::isdigit((unsigned char)w[2]))) {
                const int g = std::atoi(w.c_str() + 1); if (g < 1 || g > 10) continue; L.name = w; L.prio = 8 + g;
            } else continue;
            bool dup = false; for (const Level& o : out) if (o.name == L.name && std::fabs(o.px - L.px) < tick / 2) dup = true;
            if (!dup) out.push_back(L);
        }
    }
    return out;
}
// LRA-GexNodes-<MKT>.csv (gex_nodes): MARKET|<m>, ASOF|<unix>|..., STR|ord|strike|chart|share%|absM|sign|x_open|g_open%|name|rank.
// Nodes on a MenthorQ level give it |GEX| and the sign; other nodes become their own levels ("Node 7,750" / their MenthorQ name).
inline bool parseNodes(const std::string& text, const std::string& mkt, long long nowUnix, long long maxAgeS, double tick, std::vector<Level>& out, std::string& why) {
    out.clear(); if (text.size() > 64 * 1024) { why = "nodes file over 64 KiB"; return false; }
    const std::vector<std::string> ls = lines(text); bool mk = false; long long at = 0;
    for (const std::string& l : ls) {
        const std::vector<std::string> f = split(l, '|');
        if (f.size() >= 2 && f[0] == "MARKET") { if (f[1] != mkt) { why = "nodes file for another market"; return false; } mk = true; }
        else if (f.size() >= 2 && f[0] == "ASOF") { double v = 0; at = numTok(f[1], v) ? (long long)v : 0; }
    }
    if (!mk) { why = "nodes file without MARKET"; return false; }
    if (nowUnix > 0 && (at <= 0 || nowUnix - at > maxAgeS || at - nowUnix > 3600)) { why = "nodes stale"; return true; }
    for (const std::string& l : ls) {
        const std::vector<std::string> f = split(l, '|'); double px = 0, absM = -1;
        if (f.size() < 7 || f[0] != "STR" || !numTok(f[3], px) || !(px > 0)) continue;
        if (!numTok(f[5], absM) || absM < 0) absM = -1;
        Level L; L.px = std::round(px / tick) * tick; L.gexM = absM; L.sign = f[6].empty() ? 0 : f[6][0]; if (L.sign != 'W' && L.sign != 'F' && L.sign != 'M') L.sign = 0;
        const std::string lab = f.size() >= 10 ? trim(f[9]) : std::string();
        char b[48]; std::snprintf(b, sizeof(b), "Node %g", L.px);
        L.name = lab.empty() || lab == "-" ? std::string(b) : lab; L.prio = 30;
        if (out.size() < 32) out.push_back(L);
    }
    why = "ok"; return true;
}
// MenthorQ levels first; a node at a level's price only adds |GEX| and the sign to it
inline std::vector<Level> mergeNodes(std::vector<Level> mq, const std::vector<Level>& nodes, double tick) {
    for (const Level& n : nodes) {
        bool hit = false;
        for (Level& l : mq) if (std::fabs(l.px - n.px) < tick / 2) { l.gexM = n.gexM; l.sign = n.sign; hit = true; }
        if (!hit) mq.push_back(n);
    }
    return mq;
}

// ---- ATR, clock, text ------------------------------------------------------------------------------------------------------------
// simple mean of the true range of the last n CLOSED bars (h / l / c arrays, closed bars 0..count-1); 0 = not enough bars
inline double atrOf(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& c, int n) {
    const int N = (int)std::min(std::min(h.size(), l.size()), c.size()); if (n < 1 || N < n + 1) return 0;
    double s = 0;
    for (int i = N - n; i < N; ++i) {
        const double tr = std::max(h[i] - l[i], std::max(std::fabs(h[i] - c[i - 1]), std::fabs(l[i] - c[i - 1])));
        if (!std::isfinite(tr) || tr < 0) return 0;
        s += tr;
    }
    return s / n;
}
inline long long sessionOf(long long ctMs) { return ctMs > 0 ? (ctMs / 1000 - 17LL * 3600) / 86400 : -1; }
inline int minuteOfDay(long long ctMs) { return (int)(((ctMs / 60000) % 1440 + 1440) % 1440); }
inline std::string clock12(long long ctMs) {
    const int m = minuteOfDay(ctMs), h = m / 60, mi = m % 60; char b[24];
    std::snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, mi, h < 12 ? "AM" : "PM"); return b;
}
inline int decimalsOf(double tick) { int d = 0; double t = tick; while (d < 6 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10; ++d; } return d; }
inline std::string priceText(double px, double tick) {
    char b[48]; std::snprintf(b, sizeof(b), "%.*f", decimalsOf(tick), px);
    std::string s = b; const size_t dot = s.find('.'); size_t e = dot == std::string::npos ? s.size() : dot;
    const size_t st = (!s.empty() && s[0] == '-') ? 1 : 0;
    for (long long i = (long long)e - 3; i > (long long)st; i -= 3) s.insert((size_t)i, ",");
    return s;
}
inline std::string ratioText(double r) { char b[24]; std::snprintf(b, sizeof(b), "%.1fx", r); return b; }
inline std::string moneyText(double d) {
    char b[32];
    if (d >= 1e9) std::snprintf(b, sizeof(b), "$%.1fB", d / 1e9); else if (d >= 1e6) std::snprintf(b, sizeof(b), "$%.1fM", d / 1e6);
    else if (d >= 1e3) std::snprintf(b, sizeof(b), "$%.0fK", d / 1e3); else std::snprintf(b, sizeof(b), "$%.0f", d);
    return b;
}
inline double multiplierOf(const std::string& m) {
    static const double M[M_N] = { 50, 20, 1000, 100, 25000, 10000, 125000 };
    const int i = marketIdx(m); return i < 0 ? 0 : M[i];
}
// the 0DTE options' expiry (CT minute of day): ES / NQ 3:00 PM, CL / NG 1:30 PM, GC 12:30 PM, HG 12:00 PM, 6E 9:00 AM
inline int expiryMinute(const std::string& m) {
    static const int E[M_N] = { 900, 900, 810, 750, 720, 810, 540 };
    const int i = marketIdx(m); return i < 0 ? -1 : E[i];
}
// the pulse (0..1) from the wall clock: a smooth ~1 s breath, the same phase on every redraw
inline double pulse(long long wallMs, int periodMs = 1000) {
    if (periodMs < 200) periodMs = 200;
    const double ph = (double)(((wallMs % periodMs) + periodMs) % periodMs) / periodMs;
    return 0.5 - 0.5 * std::cos(2 * 3.14159265358979 * ph);
}
// colour (0x00RRGGBB) blended toward the background: k = 0 background, 1 the colour
inline unsigned long blend(unsigned long col, unsigned long bg, double k) {
    if (!(k >= 0)) k = 0;
    if (k > 1) k = 1;
    unsigned long out = 0;
    for (int sh = 0; sh <= 16; sh += 8) {
        const double a = (double)((col >> sh) & 0xFF), b = (double)((bg >> sh) & 0xFF);
        const unsigned long v = (unsigned long)std::lround(b + (a - b) * k);
        out |= (v > 255 ? 255ul : v) << sh;
    }
    return out;
}
// band intensity: LIVE breathes between 0.45 and 0.85 (moderate - translucent, price stays visible), SETTLED = faint (settings %)
inline double intensity(int state, long long wallMs, bool pulseOn, int faintPct) {
    if (state == LIVE) return pulseOn ? 0.45 + 0.40 * pulse(wallMs) : 0.70;
    if (state == SETTLED) return std::max(5, std::min(60, faintPct)) / 100.0;
    return 0;
}

// ---- the engine ------------------------------------------------------------------------------------------------------------------
struct Zone {
    std::string key, name; Tick lvl = 0, lo = 0, hi = 0;      // lo / hi frozen when the zone first lights
    int state = HIDDEN; int side = -1, liveSide = -1;         // side = the first decision (the band colour), liveSide = the current burst
    long long firstMs = 0, barKey = 0, lastActMs = 0, liveFromMs = 0; int episodes = 0;
    bool zero = false; double gexM = -1; char sign = 0;
    // at the first decision
    double flowX = 0, normLots = 0; long long lots = 0, adds = 0, fills = 0;
    long long repSize = 0; int repCount = 0; long long repMedMs = 0;
    int refCount = 0; Tick refPx = 0; std::string paths;      // "a", "b", "c" in any combination
    int pos = 0; char posSrc = 0;                             // dealer gamma: 1 long, -1 short, 0 unknown; 'N' node, 'H' HVL regime
    int expect = -1; bool above = false;                      // the side gamma expects; price above the level when decided
    double atrTicks = 0;
};
struct Decision { std::string key; bool first = false; int side = -1; long long ms = 0; };
struct Tally { long long decided[2] = {0, 0}, episodes = 0, pathA = 0, pathB = 0, pathC = 0, evaluated = 0, events = 0, garbage = 0; };

class Engine {
public:
    struct Ord { int side = 0; Tick px = 0; long long size = 0, lastFill = -1, at = 0; };
    struct Atom { long long ms; int side; Tick px; long long lots, size; bool fill, isNew; };
    struct PendRef { long long ms; int side; Tick px; long long id; };
    struct RefOk { long long ms; int side; Tick px; };
    struct Minute { long long m = -1; long long add[2] = {0, 0}, fill[2] = {0, 0}; };

    Params P; std::string market; double tick = 0.25;
    std::vector<Level> levels; std::vector<Zone> zones;
    std::unordered_map<long long, Ord> book;
    std::deque<Atom> atoms; std::deque<RefOk> refs; std::vector<PendRef> pend;
    std::unordered_map<Tick, long long> lastHit[2];
    std::deque<Minute> mins; std::map<int, SlotNorm> slotNorm; std::string normSrc = "learning";
    Tick lastPx = 0; double atrTicks = 0; long long nowMs = 0, session = -1, lastEvalMs = -1, curBarKey = 0;
    std::vector<std::string> rows;           // record lines to append (Z / L)
    std::vector<std::string> normRows;       // slot rows to append (N)
    std::vector<Decision> decisions; Tally tally;
    // slot accumulation for the norms record
    int slot = -1; long long slotMinutes = 0; double slotSum[4] = {0, 0, 0, 0};

    Tick toTick(double px) const { return (Tick)std::llround(px / tick); }
    double halfW() const { return P.widthAtr * atrTicks; }

    void newSession(long long s) {
        session = s; zones.clear(); atoms.clear(); refs.clear(); pend.clear(); lastHit[0].clear(); lastHit[1].clear();
        rebuild();
    }
    // prices: rebuild the hidden zones from the levels; lit zones are never touched
    void setLevels(const std::vector<Level>& lv) { levels = lv; rebuild(); }
    void setAtr(double atrPrice) { const double a = atrPrice / tick; if (a > 0 && std::isfinite(a) && std::fabs(a - atrTicks) > 1e-9) { atrTicks = a; rebuild(); } }
    void rebuild() {
        std::vector<Zone> keep; for (const Zone& z : zones) if (z.state != HIDDEN) keep.push_back(z);
        if (atrTicks <= 0) { zones = keep; return; }
        std::vector<Level> lv = levels; std::stable_sort(lv.begin(), lv.end(), [](const Level& a, const Level& b) { return a.prio < b.prio; });
        const double hw = halfW(), shift = P.nullShiftAtr * atrTicks;
        std::vector<Zone> hid;
        for (const Level& L : lv) {
            const Tick c = toTick(L.px) + (Tick)std::llround(shift);
            bool joined = false;
            for (Zone& z : hid) if (std::fabs((double)(c - z.lvl)) <= hw) {
                if (z.name.find(L.name) == std::string::npos && z.name.size() + L.name.size() < 40) z.name += "/" + L.name;
                if (L.zero) z.zero = true;
                if (z.gexM < 0 && L.gexM >= 0) { z.gexM = L.gexM; z.sign = L.sign; }
                joined = true; break;
            }
            if (joined) continue;
            bool covered = false; for (const Zone& k : keep) if (c >= k.lo && c <= k.hi) covered = true;   // inside a lit band: that zone speaks for it
            if (covered) continue;
            Zone z; z.name = L.name; z.lvl = c; z.zero = L.zero; z.gexM = L.gexM; z.sign = L.sign;
            z.key = L.name + "@" + std::to_string((long long)c);
            hid.push_back(z);
        }
        zones = keep; for (const Zone& z : hid) zones.push_back(z);
    }
    void trade(Tick px) { if (px > 0) lastPx = px; }

    // one order-book event (side: 0 bid / buy, 1 offer / sell; FILL: the aggressor's side, as IRT and the replay send it)
    void onEvent(int act, int side, Tick px, long long size, long long id, long long ms) {
        nowMs = std::max(nowMs, ms); ++tally.events;
        if (px <= 0 || size < 0 || act < NEW || act > FILL) { ++tally.garbage; return; }
        if (act == NEW) {
            if (side != 0 && side != 1) { ++tally.garbage; return; }
            Ord o; o.side = side; o.px = px; o.size = size; o.at = ms; book[id] = o;
            add(side, px, size, size, true, ms, id);
        } else if (act == MOD) {
            std::unordered_map<long long, Ord>::iterator it = book.find(id);
            if (it == book.end()) { if (side == 0 || side == 1) { Ord o; o.side = side; o.px = px; o.size = size; o.at = ms; book[id] = o; } return; }
            Ord& o = it->second;
            if (o.px != px) { o.px = px; o.size = size; o.at = ms; add(o.side, px, size, size, true, ms, id); }          // re-priced: a new placement
            else if (size > o.size) { const long long d = size - o.size; o.size = size; add(o.side, px, d, size, false, ms, id); }
            else o.size = size;                                                                                    // a fill sync or a pull
        } else if (act == DEL) {
            book.erase(id);
        } else {                                                                                                     // FILL
            std::unordered_map<long long, Ord>::iterator it = book.find(id);
            int rest = it != book.end() ? it->second.side : (side == 0 || side == 1 ? 1 - side : -1);
            if (rest < 0) { ++tally.garbage; return; }
            const int aggr = 1 - rest;
            if (it != book.end()) { it->second.lastFill = ms; it->second.size = std::max(0LL, it->second.size - size); }
            lastHit[rest][px] = ms;
            atom(Atom{ ms, aggr, px, size, size, true, false });
        }
    }
    // once a second (or faster: it rate-limits itself), with the CT ms clock and the forming candle's key
    void evaluate(long long ms, long long barKey) {
        nowMs = std::max(nowMs, ms); curBarKey = barKey;
        if (lastEvalMs >= 0 && nowMs - lastEvalMs < 1000) return;
        lastEvalMs = nowMs;
        const long long s = sessionOf(nowMs); if (s != session) newSession(s);
        rollMinutes(nowMs);
        confirmRefills();
        const long long w0 = nowMs - (long long)P.winS * 1000;
        while (!atoms.empty() && atoms.front().ms < w0 - 5000) atoms.pop_front();
        while (!refs.empty() && refs.front().ms < w0 - 5000) refs.pop_front();
        for (int k = 0; k < 2; ++k) if (lastHit[k].size() > 20000) lastHit[k].clear();
        // live -> settled
        for (Zone& z : zones) if (z.state == LIVE && nowMs - z.lastActMs > (long long)P.liveHoldS * 1000) z.state = SETTLED;
        if (atrTicks <= 0 || lastPx <= 0) return;
        double nAdd[2], nFill[2]; if (!normal(nAdd, nFill)) return;
        const double reach = P.reachAtr * atrTicks, hw = halfW();
        for (Zone& z : zones) {
            if (std::fabs((double)(z.lvl - lastPx)) > reach + hw) continue;
            const Tick lo = z.state == HIDDEN ? z.lvl - (Tick)std::floor(hw) : z.lo, hi = z.state == HIDDEN ? z.lvl + (Tick)std::floor(hw) : z.hi;
            ++tally.evaluated;
            Read best; int bestSide = -1;
            for (int sd = 0; sd < 2; ++sd) {
                Read r = readZone(sd, lo, hi, w0, nAdd[sd], nFill[sd]);
                if (r.ok && (bestSide < 0 || r.flowX > best.flowX)) { best = r; bestSide = sd; }
            }
            if (bestSide < 0) continue;
            if (z.state == HIDDEN) {
                z.state = LIVE; z.lo = lo; z.hi = hi; z.side = bestSide; z.liveSide = bestSide; z.firstMs = nowMs; z.barKey = barKey;
                z.lastActMs = nowMs; z.liveFromMs = nowMs; z.episodes = 1; z.atrTicks = atrTicks;
                z.flowX = best.flowX; z.normLots = best.norm; z.lots = best.adds + best.fills; z.adds = best.adds; z.fills = best.fills;
                z.repSize = best.repSize; z.repCount = best.repCount; z.repMedMs = best.repMedMs; z.refCount = best.refCount; z.refPx = best.refPx;
                z.paths = best.paths; z.above = lastPx > z.lvl;
                gammaCheck(z);
                ++tally.decided[bestSide]; ++tally.episodes;
                if (best.paths.find('a') != std::string::npos) ++tally.pathA;
                if (best.paths.find('b') != std::string::npos) ++tally.pathB;
                if (best.paths.find('c') != std::string::npos) ++tally.pathC;
                rows.push_back(zLine(z)); rows.push_back(lLine(z, bestSide, best.flowX));
                decisions.push_back(Decision{ z.key, true, bestSide, nowMs });
            } else {
                if (z.state == SETTLED) {
                    z.state = LIVE; z.liveFromMs = nowMs; ++z.episodes; ++tally.episodes; z.liveSide = bestSide;
                    rows.push_back(lLine(z, bestSide, best.flowX)); decisions.push_back(Decision{ z.key, false, bestSide, nowMs });
                }
                z.liveSide = bestSide; z.lastActMs = nowMs;
            }
        }
    }
    // ---- the record (append-only, one file per market / contract / bar size / session) ----
    static std::string clean(const std::string& s) { std::string o; for (char c : s) o += (c == '|' || c == '\n' || c == '\r') ? '/' : c; return o; }
    std::string zLine(const Zone& z) const {
        char b[512];
        std::snprintf(b, sizeof(b), "Z|%s|%s|%lld|%lld|%lld|%d|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%d|%lld|%d|%lld|%s|%lld|%d|%c|%d|%d|%d|%lld|%lld",
            clean(z.key).c_str(), clean(z.name).c_str(), (long long)z.lvl, (long long)z.lo, (long long)z.hi, z.side, z.firstMs, z.barKey,
            (long long)std::llround(z.flowX * 100), z.lots, z.adds, z.fills, z.repSize, z.repCount, z.repMedMs, z.refCount, (long long)z.refPx,
            z.paths.empty() ? "-" : z.paths.c_str(), (long long)std::llround(z.gexM * 100), z.pos, z.posSrc ? z.posSrc : '-', z.zero ? 1 : 0, z.expect, z.above ? 1 : 0,
            (long long)std::llround(z.normLots * 100), (long long)std::llround(z.atrTicks * 100));
        return b;
    }
    std::string lLine(const Zone& z, int side, double fx) const {
        char b[256]; std::snprintf(b, sizeof(b), "L|%s|%lld|%d|%lld", clean(z.key).c_str(), nowMs, side, (long long)std::llround(fx * 100)); return b;
    }
    // the session's record back: every recorded zone SETTLED with its first decision; L lines count the restarts. Torn / short lines skipped.
    int restore(const std::vector<std::string>& recLines) {
        int n = 0;
        for (const std::string& l : recLines) {
            const std::vector<std::string> f = split(l, '|');
            if (f.size() == 27 && f[0] == "Z") {
                Zone z; long long v[25]; bool ok = true;
                const int idx[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 22, 23, 24, 25, 26 };
                for (int k : idx) { char* e = nullptr; errno = 0; const long long x = std::strtoll(f[k].c_str(), &e, 10); if (errno || !e || *e || f[k].empty()) { ok = false; break; } v[k - 3 < 25 ? k - 3 : 0] = x; }
                if (!ok) continue;
                bool dup = false; for (const Zone& o : zones) if (o.key == f[1] && o.state != HIDDEN) dup = true; if (dup) continue;
                z.key = f[1]; z.name = f[2]; z.lvl = v[0]; z.lo = v[1]; z.hi = v[2]; z.side = (int)v[3]; z.firstMs = v[4]; z.barKey = v[5];
                if ((z.side != 0 && z.side != 1) || z.lo > z.hi) continue;
                z.flowX = v[6] / 100.0; z.lots = v[7]; z.adds = v[8]; z.fills = v[9]; z.repSize = v[10]; z.repCount = (int)v[11]; z.repMedMs = v[12];
                z.refCount = (int)v[13]; z.refPx = v[14]; z.paths = f[18] == "-" ? "" : f[18]; z.gexM = v[16] / 100.0; z.pos = (int)v[17];
                z.posSrc = f[21].empty() || f[21] == "-" ? 0 : f[21][0]; z.zero = v[19] != 0; z.expect = (int)v[20]; z.above = v[21] != 0;
                z.normLots = v[22] / 100.0; z.atrTicks = v[23] / 100.0;
                z.state = SETTLED; z.liveSide = z.side; z.lastActMs = z.firstMs; z.episodes = 0;
                zones.push_back(z); ++n;
            } else if (f.size() == 5 && f[0] == "L") {
                for (Zone& z : zones) if (z.key == f[1] && z.state != HIDDEN) { ++z.episodes; char* e = nullptr; const long long t = std::strtoll(f[2].c_str(), &e, 10); if (e && !*e && t > z.lastActMs) z.lastActMs = t; }
            }
        }
        // a hidden zone now covered by a restored band is dropped (that band speaks for it)
        std::vector<Zone> out; for (const Zone& z : zones) { bool cov = false; if (z.state == HIDDEN) for (const Zone& k : zones) if (k.state != HIDDEN && z.lvl >= k.lo && z.lvl <= k.hi) cov = true; if (!cov) out.push_back(z); }
        zones.swap(out);
        return n;
    }
    // ---- norms record: per 30-min slot, the per-minute near-price flow (x100) - N|session|slot|addB|addS|fillB|fillS|minutes ----
    void loadNormHistory(const std::vector<std::string>& ls, long long thisSession) {
        std::map<int, std::vector<std::pair<long long, SlotNorm> > > by;
        for (const std::string& l : ls) {
            const std::vector<std::string> f = split(l, '|'); if (f.size() != 8 || f[0] != "N") continue;
            long long v[7]; bool ok = true;
            for (int k = 1; k < 8; ++k) { char* e = nullptr; errno = 0; v[k - 1] = std::strtoll(f[k].c_str(), &e, 10); if (errno || !e || *e || f[k].empty() || v[k - 1] < 0) ok = false; }
            if (!ok || v[0] >= thisSession || v[1] < 0 || v[1] >= 1440 || v[6] < 20) continue;
            SlotNorm n; n.add[0] = v[2] / 100.0; n.add[1] = v[3] / 100.0; n.fill[0] = v[4] / 100.0; n.fill[1] = v[5] / 100.0;
            by[(int)v[1]].push_back(std::make_pair(v[0], n));
        }
        for (auto& kv : by) {
            std::vector<std::pair<long long, SlotNorm> >& v = kv.second;
            std::sort(v.begin(), v.end(), [](const std::pair<long long, SlotNorm>& a, const std::pair<long long, SlotNorm>& b) { return a.first > b.first; });
            std::map<long long, SlotNorm> lastPerSession; for (auto& p : v) if (!lastPerSession.count(p.first)) lastPerSession[p.first] = p.second;
            std::vector<SlotNorm> take; for (auto it = lastPerSession.rbegin(); it != lastPerSession.rend() && (int)take.size() < P.normSessions; ++it) take.push_back(it->second);
            if (take.size() < 2) continue;
            SlotNorm m;
            auto med = [&](std::vector<double> x) { std::sort(x.begin(), x.end()); const size_t n = x.size(); return n % 2 ? x[n / 2] : (x[n / 2 - 1] + x[n / 2]) / 2; };
            for (int s = 0; s < 2; ++s) {
                std::vector<double> a, f; for (const SlotNorm& t : take) { a.push_back(t.add[s]); f.push_back(t.fill[s]); }
                m.add[s] = med(a); m.fill[s] = med(f);
            }
            if (!slotNorm.count(kv.first)) slotNorm[kv.first] = m;    // a params-file NORM row (the nightly) wins
        }
    }

private:
    struct Read { bool ok = false; double flowX = 0, norm = 0; long long adds = 0, fills = 0, repSize = 0, repMedMs = 0; int repCount = 0, refCount = 0; Tick refPx = 0; std::string paths; };
    void atom(const Atom& a) {
        if (lastPx > 0 && atrTicks > 0 && std::fabs((double)(a.px - lastPx)) > P.reachAtr * atrTicks + 2 * halfW() + 4) { nearCount(a); return; }
        atoms.push_back(a); nearCount(a);
    }
    void add(int side, Tick px, long long lots, long long size, bool isNew, long long ms, long long id) {
        if (lots <= 0) return;
        std::unordered_map<Tick, long long>::const_iterator h = lastHit[side].find(px);
        if (h != lastHit[side].end() && ms - h->second <= P.refWinMs && ms >= h->second) pend.push_back(PendRef{ ms, side, px, id });
        atom(Atom{ ms, side, px, lots, size, false, isNew });
    }
    void confirmRefills() {
        std::vector<PendRef> keep;
        for (const PendRef& p : pend) {
            if (nowMs - p.ms < P.refRestMs) { keep.push_back(p); continue; }
            std::unordered_map<long long, Ord>::const_iterator it = book.find(p.id);
            if (it != book.end() && it->second.px == p.px && it->second.size > 0) refs.push_back(RefOk{ p.ms, p.side, p.px });
        }
        pend.swap(keep);
        std::sort(refs.begin(), refs.end(), [](const RefOk& a, const RefOk& b) { return a.ms < b.ms; });
    }
    // the near-price flow (the yardstick): atoms within +/- halfW of the price at the time, per minute
    void nearCount(const Atom& a) {
        if (lastPx <= 0 || atrTicks <= 0) return;
        const long long m = a.ms / 60000;
        if (firstValidMinute == LLONG_MIN) firstValidMinute = m + 1;     // the first whole minute that can be measured
        if (std::fabs((double)(a.px - lastPx)) > halfW()) return;
        if (mins.empty() || mins.back().m < m) { Minute x; x.m = m; mins.push_back(x); }
        Minute& b = mins.back(); if (b.m != m) return;          // an out-of-order stamp from an older minute: dropped
        if (a.fill) b.fill[a.side] += a.lots; else b.add[a.side] += a.lots;
    }
    void flushSlot() {
        if (slot >= 0 && slotMinutes >= 20) {
            char b[160]; std::snprintf(b, sizeof(b), "N|%lld|%d|%lld|%lld|%lld|%lld|%lld", slotSession < 0 ? 0 : slotSession, slot,
                (long long)std::llround(slotSum[0] * 100 / slotMinutes), (long long)std::llround(slotSum[1] * 100 / slotMinutes),
                (long long)std::llround(slotSum[2] * 100 / slotMinutes), (long long)std::llround(slotSum[3] * 100 / slotMinutes), slotMinutes);
            normRows.push_back(b);
        }
        slotMinutes = 0; for (double& v : slotSum) v = 0;
    }
    // every completed minute (since the near-price yardstick could be measured) joins its 30-min slot once; a finished slot with
    // 20+ minutes is written as an N row. A minute with no near-price flow counts as a zero minute (quiet is part of normal).
    void rollMinutes(long long ms) {
        const long long cur = ms / 60000;
        if (firstValidMinute == LLONG_MIN) return;
        if (lastSlotMinute < firstValidMinute - 1) lastSlotMinute = firstValidMinute - 1;
        if (cur - 1 - lastSlotMinute > 240) lastSlotMinute = cur - 241;
        for (long long m = lastSlotMinute + 1; m < cur; ++m) {
            const int s = (minuteOfDay(m * 60000) / 30) * 30; const long long ss = sessionOf(m * 60000);
            if (s != slot || ss != slotSession) { flushSlot(); slot = s; slotSession = ss; }
            for (const Minute& x : mins) if (x.m == m) { slotSum[0] += x.add[0]; slotSum[1] += x.add[1]; slotSum[2] += x.fill[0]; slotSum[3] += x.fill[1]; break; }
            ++slotMinutes;
        }
        if (cur - 1 > lastSlotMinute) lastSlotMinute = cur - 1;
        while (!mins.empty() && mins.front().m < cur - P.normMin - 2) mins.pop_front();
    }
    long long firstValidMinute = LLONG_MIN, slotSession = -1;
    long long lastSlotMinute = LLONG_MIN;
    // NORMAL flow per window: the slot normal (params file / the last sessions' records) else the rolling last normMin minutes
    bool normal(double nAdd[2], double nFill[2]) {
        const double perWin = P.winS / 60.0;
        const int s = (minuteOfDay(nowMs) / 30) * 30;
        std::map<int, SlotNorm>::const_iterator it = slotNorm.find(s);
        if (it != slotNorm.end()) {
            for (int k = 0; k < 2; ++k) { nAdd[k] = std::max(1.0, it->second.add[k] * perWin); nFill[k] = std::max(1.0, it->second.fill[k] * perWin); }
            normSrc = "slot"; return true;
        }
        const long long cur = nowMs / 60000; long long n = 0; double a[2] = {0, 0}, f[2] = {0, 0};
        for (const Minute& m : mins) if (m.m < cur && m.m >= cur - P.normMin) { ++n; for (int k = 0; k < 2; ++k) { a[k] += m.add[k]; f[k] += m.fill[k]; } }
        // minutes missing from the deque had no near-price flow: they count as zero-flow minutes once the window is old enough
        const long long span = firstValidMinute == LLONG_MIN ? 0 : std::min<long long>(P.normMin, cur - firstValidMinute);
        const long long den = std::max(n, span);
        if (den < P.normNeedMin) { normSrc = "learning"; return false; }
        for (int k = 0; k < 2; ++k) { nAdd[k] = std::max(1.0, a[k] / den * perWin); nFill[k] = std::max(1.0, f[k] / den * perWin); }
        normSrc = "rolling"; return true;
    }
    Read readZone(int sd, Tick lo, Tick hi, long long w0, double nAdd, double nFill) const {
        Read r; std::map<long long, std::vector<long long> > rep; long long lastMs = -1;
        for (const Atom& a : atoms) {
            if (a.ms < w0 || a.side != sd || a.px < lo || a.px > hi) continue;
            if (a.ms > lastMs) lastMs = a.ms;
            if (a.fill) r.fills += a.lots; else { r.adds += a.lots; if (a.isNew && a.size >= P.repMinLots) rep[a.size].push_back(a.ms); }
        }
        const long long flow = r.adds + r.fills; r.norm = nAdd + nFill; r.flowX = flow / r.norm;
        if (r.flowX < P.flowX || flow < P.minLots || lastMs < nowMs - (long long)P.freshS * 1000) return r;   // stale: the activity stopped
        // (a) same-size repeats
        for (auto& kv : rep) {
            const std::vector<long long>& t = kv.second; if ((int)t.size() < P.repN) continue;
            std::vector<double> iv; for (size_t i = 1; i < t.size(); ++i) iv.push_back((double)(t[i] - t[i - 1]));
            double m = 0; for (double x : iv) m += x; m /= iv.size(); if (m <= 0) continue;
            double v = 0; for (double x : iv) v += (x - m) * (x - m); v /= iv.size();
            const double cv = std::sqrt(v) / m;
            if (cv < P.repCv && (int)t.size() > r.repCount) {
                std::vector<double> s2 = iv; std::sort(s2.begin(), s2.end());
                r.repCount = (int)t.size(); r.repSize = kv.first; r.repMedMs = (long long)s2[s2.size() / 2];
            }
        }
        if (r.repCount >= P.repN) r.paths += "a";
        // (b) refills at one price
        std::map<Tick, int> rc; for (const RefOk& f : refs) if (f.ms >= w0 && f.side == sd && f.px >= lo && f.px <= hi) rc[f.px]++;
        for (auto& kv : rc) if (kv.second > r.refCount) { r.refCount = kv.second; r.refPx = kv.first; }
        if (r.refCount >= P.refN) r.paths += "b";
        // (c) a burst of adds or fills alone
        if (r.adds / nAdd >= P.burstX || r.fills / nFill >= P.burstX) r.paths += "c";
        r.ok = !r.paths.empty();
        return r;
    }
    // dealer gamma: the node's sign, else the HVL regime (price above HVL = long gamma); long gamma buys dips into support / sells
    // rallies into resistance, short gamma chases (sells a fall into the level, buys a rise into it)
    void gammaCheck(Zone& z) const {
        if (z.sign == 'W') { z.pos = 1; z.posSrc = 'N'; } else if (z.sign == 'F') { z.pos = -1; z.posSrc = 'N'; }
        else {
            Tick hvl = 0; for (const Level& L : levels) if (L.name == "HVL") hvl = toTick(L.px);
            if (!hvl) for (const Level& L : levels) if (L.name == "HVL 0DTE") hvl = toTick(L.px);
            if (hvl && lastPx != hvl) { z.pos = lastPx > hvl ? 1 : -1; z.posSrc = 'H'; }
        }
        if (z.pos == 1) z.expect = z.above ? BUY : SELL; else if (z.pos == -1) z.expect = z.above ? SELL : BUY; else z.expect = -1;
    }
};

// ---- the hover: the question / what / why / how / example; the header "?" = not proven yet --------------------------------------
struct HoverLine { std::string text; int tone; };   // tone: 0 header (side colour), 1 plain, 2 green, 3 red, 4 grey
inline std::vector<HoverLine> hover(const Zone& z, const std::string& mkt, double tick, long long nowMs, double lastPx) {
    std::vector<HoverLine> L; const bool buy = z.side == BUY; const int st = buy ? 2 : 3;
    const std::string lvl = priceText((double)z.lvl * tick, tick);
    L.push_back(HoverLine{ "oH ? Dealer hedging " + std::string(buy ? "BUYING" : "SELLING") + " at " + z.name + " " + lvl, 0 });
    L.push_back(HoverLine{ "Question: are dealers hedging in the futures book at this gamma level - buying or selling?", 1 });
    std::string what = "What it is: programmatic " + std::string(buy ? "buying (bids / aggressive buys)" : "selling (offers / aggressive sells)") +
        " inside the zone " + priceText((double)z.lo * tick, tick) + " - " + priceText((double)z.hi * tick, tick) + " (level +/- 0.5 ATR), first seen " + clock12(z.firstMs) + ".";
    L.push_back(HoverLine{ what, 1 });
    std::string book = "Book: ";
    std::vector<std::string> parts;
    if (z.repCount > 0) { char b[96]; std::snprintf(b, sizeof(b), "repeating %lld-lot %s every %.1f s (%dx)", z.repSize, buy ? "bids" : "offers", z.repMedMs / 1000.0, z.repCount); parts.push_back(b); }
    if (z.refCount > 0) { char b[96]; std::snprintf(b, sizeof(b), "refilled %dx at %s (held 1 s+)", z.refCount, priceText((double)z.refPx * tick, tick).c_str()); parts.push_back(b); }
    { char b[128]; std::snprintf(b, sizeof(b), "%lld lots in 60 s (%lld %s, %lld %s) = Flow: %s (2.0x) normal", z.lots, z.adds, buy ? "bid adds" : "offer adds", z.fills, buy ? "bought" : "sold", ratioText(z.flowX).c_str()); parts.push_back(b); }
    for (size_t i = 0; i < parts.size(); ++i) book += (i ? ", " : "") + parts[i];
    if (z.zero) book += ", 0DTE";
    L.push_back(HoverLine{ book + ".", st });
    // dealer positioning
    std::string pos;
    if (z.pos == 0) pos = "Dealers: gamma positioning unknown here (no node sign / HVL) - the book side is shown.";
    else {
        const bool lg = z.pos > 0;
        pos = std::string("Dealers: ") + (lg ? "LONG gamma" : "SHORT gamma") + (z.posSrc == 'N' ? " (gamma node)" : " (price vs HVL)") +
              (lg ? " - they buy dips into support and sell rallies into resistance" : " - they chase: sell as price falls, buy as it rises") +
              "; here they should " + (z.expect == BUY ? "BUY" : "SELL") + (z.expect == z.side ? " - the book agrees." : " - the book shows the opposite (book side shown).");
    }
    L.push_back(HoverLine{ pos, z.pos == 0 ? 4 : (z.expect == z.side ? st : 4) });
    // size
    const double mult = multiplierOf(mkt), px = (double)z.lvl * tick;
    std::string sz;
    if (mult > 0) { sz = "Size: " + std::to_string(z.lots) + " lots seen = " + moneyText((double)z.lots * px * mult) + " notional"; }
    if (z.gexM > 0 && mult > 0 && px > 0 && z.atrTicks > 0) {
        const double move = 0.5 * z.atrTicks * tick, usd = z.gexM * 1e6 * (move / px) / 0.01, ctr = usd / (px * mult);
        char b[160]; std::snprintf(b, sizeof(b), "; model gamma %s per 1%% -> about %.0f contracts (%s) per 0.5 ATR move", moneyText(z.gexM * 1e6).c_str(), ctr, moneyText(usd).c_str());
        sz += b;
    }
    if (!sz.empty()) L.push_back(HoverLine{ sz + ".", 1 });
    if (z.zero) {
        const int ex = expiryMinute(mkt), m = minuteOfDay(nowMs);
        if (ex >= 0) { char b[160]; const int left = ex - m;
            if (left > 0) std::snprintf(b, sizeof(b), "0DTE level: expires %s CT today (%.1f h left) - hedging gets stronger into expiry, then the level fades.", clock12((long long)ex * 60000).c_str(), left / 60.0);
            else std::snprintf(b, sizeof(b), "0DTE level: expired at %s CT - the hedging reason is gone for today.", clock12((long long)ex * 60000).c_str());
            L.push_back(HoverLine{ b, 1 }); }
    }
    if (z.state == LIVE) L.push_back(HoverLine{ "LIVE now: " + std::string(z.liveSide == BUY ? "buying" : "selling") + " since " + clock12(z.liveFromMs) + ".", z.liveSide == BUY ? 2 : 3 });
    else { char b[96]; std::snprintf(b, sizeof(b), "Settled: last activity %s; lit %d time%s.", clock12(z.lastActMs).c_str(), std::max(1, z.episodes), z.episodes == 1 ? "" : "s"); L.push_back(HoverLine{ b, 4 }); }
    L.push_back(HoverLine{ "Why it matters: dealer hedging is forced flow - long-gamma hedging holds a level, short-gamma hedging pushes through it.", 1 });
    L.push_back(HoverLine{ std::string("How to use: ") + (buy ? "green zone = buyers defending - look for a long trigger (CoB / A) inside it; a close below the zone = they stopped."
                                                              : "red zone = sellers defending - look for a short trigger (CoB / A) inside it; a close above the zone = they stopped."), 1 });
    L.push_back(HoverLine{ "Example: ES CR 0DTE 7,840 - price rallies in, a red zone pulses with repeating 20-lot offers every 2 s -> fade the rally on a CoB short.", 4 });
    L.push_back(HoverLine{ "? = not proven yet: the nightly test is measuring it.", 4 });
    (void)lastPx;
    return L;
}
} // namespace dh
#endif
