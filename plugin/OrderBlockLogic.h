// OrderBlockLogic.h 2.0.0 (2026-10-22 evening) -- the ORDER BLOCK (OB) PRINT of a strong wave, SDK-free (lsBigBidAsk 1.26.0).
// Rassul 18:01 CT 2026-10-22 (replaces the 1.0.0 OB / BO rectangles): "you only put the volume at the bid or ask like we currently do at
// the bo or the second high volume bid or ask if it exists near the pivot using the pivot rule, its as simple as that".
//   * NO boxes, NO zones, NO extended bands: the resting-limit zones come only from the LMT zones of lsLiquidityProfile.
//   * Strong wave (unchanged from 1.0.0): (a) a wave that earned an Rly (RallyLogic.h: confirmed 1-bar ZigZag wave, 3+ bars, 2.5+ ATR,
//     tagged +-1 SD, 20-bar extreme, vwapTrend) or (b) any other confirmed 1-bar ZigZag wave >= 3.0 ATR (setting).
//   * OB = near the ZigZag pivot where the strong wave started: the pivot candle and the next 1-3 candles (setting, default 2; never past
//     the wave's extreme). The highest-volume cell AT THE BID or AT THE ASK over those candles is the OB print. When the BO mark of one of
//     those candles already shows that very print (same candle, price and side), the SECOND highest cell is used, if one exists.
//   * Shown like a print (lots in the side's box on its candle at its price) with the letter "OB" BELOW the candle of an up-wave, ABOVE
//     the candle of a down-wave. The BO letter itself is the breakout signal of BigBidAskLogic.h (boWaves).
//   * Decided ONCE when the wave is confirmed (the ZigZag turns: bar ic), recorded (OBX| line in lsFlexLevels\OrderBlock\<MKT>-<CONTRACT>-
//     <spb>-<session>.csv), the record wins after a restart (never repainted). The visits to the print's price afterwards are logged
//     (OBT| lines: held / swept / broke) for the nightly, never drawn.
// Not proven yet ("?"): no odds until the nightly has 20+ tests a side.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "RallyLogic.h"

namespace obk {

static const char* const VERSION = "2.0.0";
static const int ORIGIN_MAX = 3;          // the pivot candle + at most 3 candles after it

enum { OB = 0 };
enum { CLS_RLY = 1, CLS_ATR = 2 };        // (a) the Rly wave, (b) any confirmed wave >= the ATR setting
enum { T_HELD = 1, T_SWEPT = 2, T_BROKE = 3, T_OPEN = 4 };

struct Params { double atrB = 3.0; int origin = 2; double tick = 0.25; };

// one OB to decide (chart indices; the host turns them into times)
struct Cand {
    int dir = 0, cls = 0;
    int i0 = -1, i1 = -1;                 // the OB candles: the pivot candle .. pivot + origin (capped at the extreme)
    int ip = -1, ie = -1, ic = -1;        // wave pivot, extreme, confirming bar
    double pivot = 0;                     // the wave's origin price (the pivot low of an up-wave / high of a down-wave)
    int nb = 0; double ratio = 0, atr = 0, size = 0; bool rly = false; long sess = 0;
};

inline bool strongClass(const rly::Wave& W, const Params& P, int& cls) {
    if (W.fired) { cls = CLS_RLY; return true; }
    if (std::isfinite(W.atr) && W.atr > 0 && W.ratio >= P.atrB && W.nb >= 1) { cls = CLS_ATR; return true; }
    return false;
}

// the OB candidates of the strong waves of one session (W from rly::judge)
inline void obOf(const rly::Sess& S, const std::vector<rly::Wave>& W, const Params& P, std::vector<Cand>& out) {
    std::map<int, int> loc; for (size_t k = 0; k < S.g.size(); ++k) loc[S.g[k]] = (int)k;
    const int og = std::max(1, std::min(ORIGIN_MAX, P.origin));
    for (const rly::Wave& w : W) {
        int cls = 0; if (!strongClass(w, P, cls)) continue;
        const auto ip = loc.find(w.gp), ie = loc.find(w.ge); if (ip == loc.end() || ie == loc.end()) continue;
        const int p = ip->second, e = ie->second; if (e <= p) continue;
        Cand c; c.dir = w.dir; c.cls = cls; c.ip = w.gp; c.ie = w.ge; c.ic = w.gc; c.nb = w.nb; c.ratio = w.ratio; c.atr = w.atr; c.size = w.size;
        c.rly = w.fired; c.sess = S.sess; c.pivot = w.from;
        c.i0 = S.g[(size_t)p]; c.i1 = S.g[(size_t)std::min(p + og, e)];
        out.push_back(c);
    }
}

// every RTH session of the chart (B = CLOSED bars only); OBs whose wave is confirmed at index >= from
inline std::vector<Cand> find(const std::vector<rly::Bar>& B, int per, int openMin, int closeMin, const Params& P, int from = 0) {
    std::vector<Cand> out; std::vector<double> atr; rly::atrWilder(B, atr);
    rly::Sess cur; bool open = false;
    auto flush = [&]() { if (open && !cur.g.empty()) { std::vector<rly::Wave> W; rly::judge(B, atr, cur, W); obOf(cur, W, P, out); } cur = rly::Sess(); open = false; };
    for (int i = 0; i < (int)B.size(); ++i) {
        const svl::Cls c = svl::classify(B[(size_t)i].days, B[(size_t)i].endSec, per, openMin, closeMin);
        if (c.kind != svl::RTH) { if (open && c.sess != cur.sess) flush(); continue; }
        if (open && c.sess != cur.sess) flush();
        if (!open) { cur.sess = c.sess; open = true; }
        cur.g.push_back(i);
    }
    flush();
    if (from > 0) { std::vector<Cand> k; for (const Cand& c : out) if (c.ic >= from) k.push_back(c); out.swap(k); }
    return out;
}

// ---- the OB print -----------------------------------------------------------------------------------------------------------------
struct Print { long long t = 0; double px = 0; int side = 0; long long lots = 0; };   // side +1 bought at the ask, -1 sold at the bid
// one OB candle's volume at price: price, lots at the bid (sellers), lots at the ask (buyers)
struct CandleCells { long long t = 0; std::vector<double> px; std::vector<long long> bid, ask; };
inline bool samePrint(const Print& a, const Print& b, double tick) {
    return a.t == b.t && a.side == b.side && std::fabs(a.px - b.px) <= (tick > 0 ? tick : 1e-9) * 0.25;
}
// the highest-volume bid / ask cell of the OB candles; when it is a print a BO mark already shows (taken), the SECOND highest (if any).
// rank = 1 (the top) / 2 (the second) / 0 (no cell). Ties: earlier candle, then lower price, then the bid first (deterministic).
inline Print pick(const std::vector<CandleCells>& C, const std::vector<Print>& taken, double tick, int& rank) {
    std::vector<Print> v;
    for (const CandleCells& c : C)
        for (size_t k = 0; k < c.px.size() && k < c.bid.size() && k < c.ask.size(); ++k)
            for (int s = -1; s <= 1; s += 2) { const long long lots = s < 0 ? c.bid[k] : c.ask[k]; if (lots <= 0 || !std::isfinite(c.px[k])) continue;
                Print p; p.t = c.t; p.px = c.px[k]; p.side = s; p.lots = lots; v.push_back(p); }
    std::sort(v.begin(), v.end(), [](const Print& a, const Print& b) { return a.lots != b.lots ? a.lots > b.lots : a.t != b.t ? a.t < b.t : a.px != b.px ? a.px < b.px : a.side < b.side; });
    rank = 0; if (v.empty()) return Print();
    bool used = false; for (const Print& t : taken) if (samePrint(t, v[0], tick)) { used = true; break; }
    if (!used) { rank = 1; return v[0]; }
    if (v.size() >= 2) { rank = 2; return v[1]; }
    return Print();
}

// ---- the recorded OB ----------------------------------------------------------------------------------------------------------------
// 1.26.0 lines: t0 = t1 = the print's candle, lo = hi = the print's price, field "rank" = 1 top / 2 second / 0 none (no print: logged,
// not drawn); n_prints 0 or 1. 1.25.0 box lines (version 1.25.0, or kind BO) are not read back (the boxes are gone).
struct Box {
    int kind = OB, dir = 0, cls = 0;
    long long t0 = 0, t1 = 0, tp = 0, te = 0, tc = 0;   // bar END stamps: print candle, wave pivot / extreme / confirming bar
    double lo = 0, hi = 0, pivot = 0; int nb = 0; double ratio = 0, atr = 0; bool rly = false;
    int rank = 0; int origin = 0; std::vector<Print> prints;
    long long id() const { return te * 4 + (dir > 0 ? 1 : 0); }
    bool has() const { return !prints.empty(); }
};
static const char* const REC_HEADER = "OBX|kind|dir|cls|t0|t1|t_p|t_e|t_conf|lo|hi|pivot|nb|ratio|atr|rly|rank|n_prints|prints|version";
static const char* const TEST_HEADER = "OBT|id|kind|dir|n|t_start|t_end|outcome|far_px|version";
inline std::string fmtD(double x, int d) { char b[48]; std::snprintf(b, sizeof b, "%.*f", d, std::isfinite(x) ? x : 0.0); return b; }
inline std::string line(const Box& b, const char* ver) {
    std::string pr; for (const Print& p : b.prints) { if (!pr.empty()) pr += ";"; pr += std::to_string(p.t) + ":" + fmtD(p.px, 6) + ":" + (p.side > 0 ? "B" : "S") + ":" + std::to_string(p.lots); }
    return "OBX|OB|" + std::to_string(b.dir) + "|" + std::to_string(b.cls) + "|" + std::to_string(b.t0) + "|" + std::to_string(b.t1)
        + "|" + std::to_string(b.tp) + "|" + std::to_string(b.te) + "|" + std::to_string(b.tc) + "|" + fmtD(b.lo, 6) + "|" + fmtD(b.hi, 6) + "|" + fmtD(b.pivot, 6)
        + "|" + std::to_string(b.nb) + "|" + fmtD(b.ratio, 2) + "|" + fmtD(b.atr, 4) + "|" + (b.rly ? "1" : "0") + "|" + std::to_string(b.rank)
        + "|" + std::to_string(b.prints.size()) + "|" + (pr.empty() ? "-" : pr) + "|" + ver;
}
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> f; size_t a = 0; for (;;) { const size_t b = s.find(d, a); f.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; } return f; }
inline bool toLL(const std::string& x, long long& v) { if (x.empty() || x.size() > 20) return false; char* e = nullptr; v = std::strtoll(x.c_str(), &e, 10); return e && *e == 0; }
inline bool toD(const std::string& x, double& v) { if (x.empty() || x.size() > 40) return false; char* e = nullptr; v = std::strtod(x.c_str(), &e); return e && *e == 0 && std::isfinite(v); }
// legacy = a well-formed line of the 1.25.0 box rule (not read back; counted)
inline bool parseLine(const std::string& s0, Box& b, bool* legacy = nullptr) {
    if (legacy) *legacy = false;
    std::string s = s0; if (!s.empty() && s.back() == '\r') s.pop_back();
    if (s.compare(0, 4, "OBX|") != 0 || s.compare(0, 9, "OBX|kind|") == 0) return false;
    const std::vector<std::string> f = split(s, '|'); if (f.size() != 20) return false;
    if (f[1] == "BO" || f[19] == "1.25.0") { if (legacy) *legacy = true; return false; }
    b = Box();
    if (f[1] != "OB") return false;
    long long d = 0, cl = 0, nb = 0, r = 0, np = 0, rk = 0;
    if (!toLL(f[2], d) || (d != 1 && d != -1) || !toLL(f[3], cl) || (cl != CLS_RLY && cl != CLS_ATR)) return false;
    if (!toLL(f[4], b.t0) || !toLL(f[5], b.t1) || !toLL(f[6], b.tp) || !toLL(f[7], b.te) || !toLL(f[8], b.tc)) return false;
    if (b.t0 <= 0 || b.t1 < b.t0 || b.te <= 0 || b.tc < b.te) return false;
    if (!toD(f[9], b.lo) || !toD(f[10], b.hi) || !toD(f[11], b.pivot) || b.hi < b.lo) return false;
    if (!toLL(f[12], nb) || nb < 0 || nb > 100000 || !toD(f[13], b.ratio) || !toD(f[14], b.atr) || !toLL(f[15], r) || (r != 0 && r != 1)) return false;
    if (!toLL(f[16], rk) || rk < 0 || rk > 2) return false;
    if (!toLL(f[17], np) || np < 0 || np > 1) return false;
    if (f[18] != "-") for (const std::string& x : split(f[18], ';')) {
        const std::vector<std::string> g = split(x, ':'); Print p; if (g.size() != 4) return false;
        if (!toLL(g[0], p.t) || !toD(g[1], p.px) || (g[2] != "B" && g[2] != "S") || !toLL(g[3], p.lots) || p.lots <= 0) return false;
        p.side = g[2] == "B" ? 1 : -1; b.prints.push_back(p);
    }
    if ((long long)b.prints.size() != np || (np == 0) != (rk == 0)) return false;
    b.dir = (int)d; b.cls = (int)cl; b.nb = (int)nb; b.rly = r == 1; b.rank = (int)rk;
    return true;
}

// ---- the visits to the OB print's price after the wave (closed bars of the same session from the confirming bar on; log only) ------
struct OHLC { long long t = 0; double h = 0, l = 0, c = 0; };
struct Test { int n = 0; long long t0 = 0, t1 = 0; int outcome = T_OPEN; double far = 0; };
struct Track { std::vector<Test> tests; long long tEnd = 0; bool ended = false; };
// bars = the closed bars from the confirming bar to the end of the box's session (in order)
inline Track track(const Box& b, const std::vector<OHLC>& bars) {
    Track T; const bool bull = b.dir > 0; bool in = false; Test cur;
    for (size_t i = 0; i < bars.size(); ++i) {
        const OHLC& x = bars[i];
        const bool touch = bull ? x.l <= b.hi : x.h >= b.lo;
        const bool through = bull ? x.c < b.lo : x.c > b.hi;
        if (!in && touch) { in = true; cur = Test(); cur.n = (int)T.tests.size() + 1; cur.t0 = x.t; cur.far = bull ? x.l : x.h; cur.outcome = T_HELD; }
        if (in) {
            cur.far = bull ? std::min(cur.far, x.l) : std::max(cur.far, x.h);
            if (bull ? cur.far < b.lo : cur.far > b.hi) cur.outcome = T_SWEPT;
            if (through) { cur.outcome = T_BROKE; cur.t1 = x.t; T.tests.push_back(cur); T.ended = true; T.tEnd = x.t; return T; }
            if (!touch) { cur.t1 = x.t; T.tests.push_back(cur); in = false; }   // left the box again
        }
    }
    if (in) { cur.t1 = 0; cur.outcome = T_OPEN; T.tests.push_back(cur); }   // still in the box: not resolved (never recorded until it is)
    return T;
}
inline const char* outcomeName(int o) { return o == T_HELD ? "held" : o == T_SWEPT ? "swept" : o == T_BROKE ? "broke" : "in it now"; }
inline std::string testLine(const Box& b, const Test& t, const char* ver) {
    return "OBT|" + std::to_string(b.id()) + "|OB|" + std::to_string(b.dir) + "|" + std::to_string(t.n) + "|" + std::to_string(t.t0)
        + "|" + std::to_string(t.t1) + "|" + outcomeName(t.outcome) + "|" + fmtD(t.far, 6) + "|" + ver;
}

// ---- the hover on the OB letter (question / what / why / how to use / example; "?" = not proven yet) ---------------------------------
// clk(t) formats a bar stamp as the bar's START "9:24 AM"; px(v) a price
template <class Clk, class Px>
inline std::vector<std::string> hover(const Box& b, const Track& T, Clk clk, Px px) {
    std::vector<std::string> L; const bool up = b.dir > 0;
    const Print p = b.prints.empty() ? Print() : b.prints.front();
    L.push_back(std::string("OB ? - the order block print of a strong ") + (up ? "up" : "down") + "-wave (" + std::to_string(b.nb) + " bars, " + fmtD(b.ratio, 1) + " ATR, "
                + (b.rly ? "an Rly wave" : "3+ ATR") + "), decided " + clk(b.tc));
    L.push_back(std::to_string(p.lots) + " lots " + (p.side > 0 ? "bought at the ask" : "sold at the bid") + " at " + px(p.px) + " (" + clk(p.t) + "), "
                + (b.rank == 2 ? "the 2nd biggest cell near the pivot (the biggest is the BO print)" : "the biggest cell near the pivot") + " " + px(b.pivot));
    std::string ts = "Back at this price later: ";
    if (T.tests.empty()) ts += "not yet";
    for (const Test& t : T.tests) { if (ts.size() > 26) ts += ",  "; ts += clk(t.t0) + " " + outcomeName(t.outcome); }
    L.push_back(ts);
    L.push_back(up ? "Question: who started this strong up-wave, and at what price?" : "Question: who started this strong down-wave, and at what price?");
    L.push_back(std::string("What it is: the biggest bid / ask volume on the pivot candle and the next candles, where the ") + (up ? "up" : "down") + "-wave began.");
    L.push_back(std::string("Why it matters: real size at the origin shows who launched the wave; they may defend that price on a deep ") + (up ? "pullback." : "bounce."));
    L.push_back(up ? "How to use: on a deep pullback to this price, buy only with your reversal bar or CoB; a close well below it = the origin failed."
                   : "How to use: on a deep bounce to this price, sell only with your reversal bar or CoB; a close well above it = the origin failed.");
    L.push_back(std::string("Example: ") + (up ? "OB 412 sold at the bid at the 9:42 low; the 11:05 pullback holds there and the wave makes a higher high."
                                                : "OB 388 bought at the ask at the 10:15 high; the 12:20 bounce fails there and the wave makes a lower low."));
    L.push_back("? = not proven yet: the nightly scores every OB (lsFlexLevels\\OrderBlock); no odds until 20+ tests a side.");
    return L;
}

} // namespace obk
