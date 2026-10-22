// OrderBlockLogic.h 1.0.0 (2026-10-22) -- ORDER BLOCK (OB) + BREAKOUT (BO) boxes of STRONG WAVES, SDK-free (lsBigBidAsk 1.25.0).
// Rassul's idea (sketch 13:55 CT, agreed 16:00-16:01 CT 2026-10-22): "a strong wave starts at a low (the ORDER BLOCK, its origin) and
// breaks through the prior pivot high (the BREAKOUT box). Both should hold big prints. On the pullback, test 1 = back to the breakout
// area, which bounces; test 2 = deeper, back to the OB, which may sweep just past it and then reverse to a higher high" (mirror down);
// "strong waves typically have strong breaks and order blocks, and you may have to find the order block after the strong wave has
// occurred and then print it". Strong wave = BOTH (his answer "both"):
//   (a) a wave that earned an Rly (RallyLogic.h: confirmed 1-bar ZigZag wave >= 3 bars, >= 2.5 ATR, tagged +-1 SD, 20-bar extreme,
//       vwapTrend) = the WITH-TREND strong wave, drawn at normal strength;
//   (b) ANY other confirmed wave >= 3.0 ATR (setting) = drawn lighter (never grey - grey is ABC / internal context elsewhere).
// When the wave is CONFIRMED (the ZigZag turns, bar gc) the host looks BACK and decides, once:
//   OB box = the wave ORIGIN: its first `origin` candles (1-2, setting) from the pivot low to the highest high of those candles (up;
//            mirror down), capped at 40% of the wave so a huge second candle cannot swallow the wave;
//   BO box = where the wave broke the PRIOR pivot (the ZigZag pivot before the wave's start: the high that began the down-wave into
//            the low p, for an up-wave): the first candle of the wave trading through it, its range clipped to the pivot +- 0.5 ATR;
//            none when the wave never got through it; dropped when it would overlap the OB (at most 2 boxes per wave, never overlapping).
//   Big prints inside each box (from the box candles' volume at price, decided by the host with the Big Bid/Ask rules): a cell >= the
//   session's Big Lot 20 tier, or the candle's own Big Bid / Ask print. No big print = the box is drawn FAINT ("no real size").
// Afterwards (closed candles only, a pure function of them = identical after a restart): the box extends right until a candle CLOSES
// through it (below lo for a bullish box / above hi for a bearish one) or the session ends; every visit is a TEST:
//   held   = it traded into the box and left it again without trading through its far side,
//   swept  = it traded past the far side but no candle closed beyond it (a sweep, then back),
//   broke  = a candle closed through it (the box ends there).
// Nothing is repainted: the geometry and prints are recorded once (OBX| lines in lsFlexLevels\OrderBlock\<MKT>-<CONTRACT>-<spb>-
// <session>.csv), the record wins after a restart, and the tests (OBT| lines) are appended once each for the nightly to score.
// Not proven yet ("?"): the OB idea is tested by the nightly before any odds are shown. Earlier research: an OB means nothing unless
// real resting liquidity is there; a retest alone is a coin flip; with-trend breakouts matter more; internal ABC breaks are pullbacks.
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

static const char* const VERSION = "1.0.0";
static const double OB_CAP = 0.40;        // the OB never taller than 40% of its wave
static const double BO_HALF_ATR = 0.5;    // the BO box = the breakout candle within the pivot +- 0.5 ATR
static const int MAX_PRINTS = 6;          // dots kept per box (largest first)

enum { OB = 0, BO = 1 };
enum { CLS_RLY = 1, CLS_ATR = 2 };        // (a) the Rly wave, (b) any confirmed wave >= the ATR setting
enum { T_HELD = 1, T_SWEPT = 2, T_BROKE = 3, T_OPEN = 4 };

struct Params { double atrB = 3.0; int origin = 2; double tick = 0.25; };

// one box as decided (geometry from chart indices; the host turns them into times)
struct Cand {
    int kind = OB, dir = 0, cls = 0;
    int i0 = -1, i1 = -1;                 // the box candles (chart indices)
    int ip = -1, ie = -1, ic = -1;        // wave pivot, extreme, confirming bar
    double lo = 0, hi = 0, pivot = 0;     // pivot = the prior pivot price (BO), the wave origin (OB)
    int nb = 0; double ratio = 0, atr = 0, size = 0; bool rly = false; long sess = 0;
};

// whole ticks (the box edges are prices the chart can show)
inline double snapDn(double x, double tk) { return std::floor(x / tk + 1e-7) * tk; }
inline double snapUp(double x, double tk) { return std::ceil(x / tk - 1e-7) * tk; }
inline bool strongClass(const rly::Wave& W, const Params& P, int& cls) {
    if (W.fired) { cls = CLS_RLY; return true; }
    if (std::isfinite(W.atr) && W.atr > 0 && W.ratio >= P.atrB && W.nb >= 1) { cls = CLS_ATR; return true; }
    return false;
}

// the boxes of the strong waves of one session (S already filled by rly::judge)
inline void boxesOf(const std::vector<rly::Bar>& B, rly::Sess& S, const std::vector<rly::Wave>& W, const Params& P, std::vector<Cand>& out) {
    std::vector<rly::Pivot> piv; rly::zigzag(B, S, piv);
    std::map<int, int> loc; for (size_t k = 0; k < S.g.size(); ++k) loc[S.g[k]] = (int)k;
    auto X = [&](int k) -> const rly::Bar& { return B[(size_t)S.g[(size_t)k]]; };
    const double tk = P.tick > 0 ? P.tick : 1e-9;
    for (const rly::Wave& w : W) {
        int cls = 0; if (!strongClass(w, P, cls)) continue;
        const int p = loc.count(w.gp) ? loc[w.gp] : -1, e = loc.count(w.ge) ? loc[w.ge] : -1;
        if (p < 0 || e <= p) continue;
        int q = -1; for (size_t j = 0; j + 1 < piv.size(); ++j) if (piv[j].i == p && piv[j + 1].i == e && piv[j + 1].type == w.dir) { q = (int)j; break; }
        const bool up = w.dir > 0;
        Cand base; base.dir = w.dir; base.cls = cls; base.ip = w.gp; base.ie = w.ge; base.ic = w.gc; base.nb = w.nb; base.ratio = w.ratio;
        base.atr = w.atr; base.size = w.size; base.rly = w.fired; base.sess = S.sess;
        // OB: the origin candles
        Cand ob = base; ob.kind = OB;
        const int last = std::min(p + std::max(1, std::min(2, P.origin)) - 1, e - 1 >= p ? e - 1 : p);
        ob.i0 = S.g[(size_t)p]; ob.i1 = S.g[(size_t)last];
        if (up) { ob.lo = X(p).l; double h = X(p).h; for (int k = p; k <= last; ++k) h = std::max(h, X(k).h); ob.hi = snapDn(std::min(h, ob.lo + OB_CAP * w.size), tk); }
        else    { ob.hi = X(p).h; double l = X(p).l; for (int k = p; k <= last; ++k) l = std::min(l, X(k).l); ob.lo = snapUp(std::max(l, ob.hi - OB_CAP * w.size), tk); }
        if (ob.hi - ob.lo < tk * 0.5) { if (up) ob.hi = ob.lo + tk; else ob.lo = ob.hi - tk; }
        ob.pivot = up ? ob.lo : ob.hi;
        out.push_back(ob);
        // BO: the prior pivot of the same side, broken inside the wave
        if (q < 1) continue;
        const int pp = piv[(size_t)q - 1].i; if (pp < 0 || pp >= p) continue;
        const double piv0 = up ? X(pp).h : X(pp).l;
        int k = -1; for (int j = p + 1; j <= e; ++j) if (up ? X(j).h > piv0 : X(j).l < piv0) { k = j; break; }
        if (k < 0) continue;
        const double half = (std::isfinite(w.atr) && w.atr > 0 ? w.atr : 4 * tk) * BO_HALF_ATR;
        Cand bo = base; bo.kind = BO; bo.i0 = bo.i1 = S.g[(size_t)k]; bo.pivot = piv0;
        if (up) { bo.lo = snapUp(std::max(X(k).l, piv0 - half), tk); bo.hi = snapDn(std::min(X(k).h, std::max(piv0, bo.lo) + half), tk); if (bo.lo <= ob.hi) bo.lo = ob.hi + tk; }
        else    { bo.hi = snapDn(std::min(X(k).h, piv0 + half), tk); bo.lo = snapUp(std::max(X(k).l, std::min(piv0, bo.hi) - half), tk); if (bo.hi >= ob.lo) bo.hi = ob.lo - tk; }
        if (bo.hi < bo.lo - tk * 0.01) continue;                     // squeezed out by the OB: one box only
        out.push_back(bo);
    }
}

// every RTH session of the chart (B = CLOSED bars only); boxes whose wave is confirmed at index >= from
inline std::vector<Cand> find(const std::vector<rly::Bar>& B, int per, int openMin, int closeMin, const Params& P, int from = 0) {
    std::vector<Cand> out; std::vector<double> atr; rly::atrWilder(B, atr);
    rly::Sess cur; bool open = false;
    auto flush = [&]() { if (open && !cur.g.empty()) { std::vector<rly::Wave> W; rly::judge(B, atr, cur, W); boxesOf(B, cur, W, P, out); } cur = rly::Sess(); open = false; };
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

// ---- big prints inside a box -------------------------------------------------------------------------------------------------------
struct Print { long long t = 0; double px = 0; int side = 0; long long lots = 0; };   // side +1 bought at the ask, -1 sold at the bid
// cells of one box candle: price, lots at the bid (sellers), lots at the ask (buyers); bigBid / bigAsk = the candle's own Big Bid / Ask
// print (the chart rule: the biggest of its side over the "Biggest in" candles before it), given by the host with its price
struct CandleCells { long long t = 0; std::vector<double> px; std::vector<long long> bid, ask; double bigBidPx = NAN, bigAskPx = NAN; };
inline std::vector<Print> bigPrints(const std::vector<CandleCells>& C, double lo, double hi, long long tier, double tick) {
    std::vector<Print> v; const double e = (tick > 0 ? tick : 1e-9) * 0.25;
    for (const CandleCells& c : C)
        for (size_t k = 0; k < c.px.size() && k < c.bid.size() && k < c.ask.size(); ++k) {
            const double x = c.px[k]; if (!(x >= lo - e && x <= hi + e)) continue;
            for (int s = -1; s <= 1; s += 2) {
                const long long lots = s < 0 ? c.bid[k] : c.ask[k]; if (lots <= 0) continue;
                const double bp = s < 0 ? c.bigBidPx : c.bigAskPx;
                const bool own = std::isfinite(bp) && std::fabs(bp - x) <= e;
                if ((tier > 0 && lots >= tier) || own) { Print p; p.t = c.t; p.px = x; p.side = s; p.lots = lots; v.push_back(p); }
            }
        }
    std::sort(v.begin(), v.end(), [](const Print& a, const Print& b) { return a.lots != b.lots ? a.lots > b.lots : a.t != b.t ? a.t < b.t : a.px != b.px ? a.px < b.px : a.side < b.side; });
    if (v.size() > (size_t)MAX_PRINTS) v.resize((size_t)MAX_PRINTS);
    return v;
}

// ---- the recorded box ----------------------------------------------------------------------------------------------------------------
struct Box {
    int kind = OB, dir = 0, cls = 0;
    long long t0 = 0, t1 = 0, tp = 0, te = 0, tc = 0;   // bar END stamps: box candles, wave pivot / extreme / confirming bar
    double lo = 0, hi = 0, pivot = 0; int nb = 0; double ratio = 0, atr = 0; bool rly = false;
    long long tier = 0; bool tierKnown = false; std::vector<Print> prints;
    long long id() const { return te * 4 + kind * 2 + (dir > 0 ? 1 : 0); }
    bool faint() const { return tierKnown && prints.empty(); }
};
static const char* const REC_HEADER = "OBX|kind|dir|cls|t0|t1|t_p|t_e|t_conf|lo|hi|pivot|nb|ratio|atr|rly|tier|n_prints|prints|version";
static const char* const TEST_HEADER = "OBT|id|kind|dir|n|t_start|t_end|outcome|far_px|version";
inline std::string fmtD(double x, int d) { char b[48]; std::snprintf(b, sizeof b, "%.*f", d, std::isfinite(x) ? x : 0.0); return b; }
inline std::string line(const Box& b, const char* ver) {
    std::string pr; for (const Print& p : b.prints) { if (!pr.empty()) pr += ";"; pr += std::to_string(p.t) + ":" + fmtD(p.px, 6) + ":" + (p.side > 0 ? "B" : "S") + ":" + std::to_string(p.lots); }
    return "OBX|" + std::string(b.kind == OB ? "OB" : "BO") + "|" + std::to_string(b.dir) + "|" + std::to_string(b.cls) + "|" + std::to_string(b.t0) + "|" + std::to_string(b.t1)
        + "|" + std::to_string(b.tp) + "|" + std::to_string(b.te) + "|" + std::to_string(b.tc) + "|" + fmtD(b.lo, 6) + "|" + fmtD(b.hi, 6) + "|" + fmtD(b.pivot, 6)
        + "|" + std::to_string(b.nb) + "|" + fmtD(b.ratio, 2) + "|" + fmtD(b.atr, 4) + "|" + (b.rly ? "1" : "0") + "|" + (b.tierKnown ? std::to_string(b.tier) : std::string("-"))
        + "|" + std::to_string(b.prints.size()) + "|" + (pr.empty() ? "-" : pr) + "|" + ver;
}
inline std::vector<std::string> split(const std::string& s, char d) { std::vector<std::string> f; size_t a = 0; for (;;) { const size_t b = s.find(d, a); f.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; } return f; }
inline bool toLL(const std::string& x, long long& v) { if (x.empty() || x.size() > 20) return false; char* e = nullptr; v = std::strtoll(x.c_str(), &e, 10); return e && *e == 0; }
inline bool toD(const std::string& x, double& v) { if (x.empty() || x.size() > 40) return false; char* e = nullptr; v = std::strtod(x.c_str(), &e); return e && *e == 0 && std::isfinite(v); }
inline bool parseLine(const std::string& s0, Box& b) {
    std::string s = s0; if (!s.empty() && s.back() == '\r') s.pop_back();
    if (s.compare(0, 4, "OBX|") != 0 || s.compare(0, 9, "OBX|kind|") == 0) return false;
    const std::vector<std::string> f = split(s, '|'); if (f.size() != 20) return false;
    b = Box();
    if (f[1] == "OB") b.kind = OB; else if (f[1] == "BO") b.kind = BO; else return false;
    long long d = 0, cl = 0, nb = 0, r = 0, np = 0;
    if (!toLL(f[2], d) || (d != 1 && d != -1) || !toLL(f[3], cl) || (cl != CLS_RLY && cl != CLS_ATR)) return false;
    if (!toLL(f[4], b.t0) || !toLL(f[5], b.t1) || !toLL(f[6], b.tp) || !toLL(f[7], b.te) || !toLL(f[8], b.tc)) return false;
    if (b.t0 <= 0 || b.t1 < b.t0 || b.te <= 0 || b.tc < b.te) return false;
    if (!toD(f[9], b.lo) || !toD(f[10], b.hi) || !toD(f[11], b.pivot) || b.hi < b.lo) return false;
    if (!toLL(f[12], nb) || nb < 0 || nb > 100000 || !toD(f[13], b.ratio) || !toD(f[14], b.atr) || !toLL(f[15], r) || (r != 0 && r != 1)) return false;
    if (f[16] == "-") { b.tierKnown = false; b.tier = 0; } else { if (!toLL(f[16], b.tier) || b.tier < 0) return false; b.tierKnown = true; }
    if (!toLL(f[17], np) || np < 0 || np > MAX_PRINTS) return false;
    if (f[18] != "-") for (const std::string& x : split(f[18], ';')) {
        const std::vector<std::string> g = split(x, ':'); Print p; if (g.size() != 4) return false;
        if (!toLL(g[0], p.t) || !toD(g[1], p.px) || (g[2] != "B" && g[2] != "S") || !toLL(g[3], p.lots) || p.lots <= 0) return false;
        p.side = g[2] == "B" ? 1 : -1; b.prints.push_back(p);
    }
    if ((long long)b.prints.size() != np) return false;
    b.dir = (int)d; b.cls = (int)cl; b.nb = (int)nb; b.rly = r == 1;
    return true;
}

// ---- the tests after the box was drawn (closed bars of the same session from the confirming bar on) --------------------------------
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
    return "OBT|" + std::to_string(b.id()) + "|" + (b.kind == OB ? "OB" : "BO") + "|" + std::to_string(b.dir) + "|" + std::to_string(t.n) + "|" + std::to_string(t.t0)
        + "|" + std::to_string(t.t1) + "|" + outcomeName(t.outcome) + "|" + fmtD(t.far, 6) + "|" + ver;
}

// ---- drawing state (pure): how the box looks ----------------------------------------------------------------------------------------
// style: 0 normal (a), 1 lighter (b); faint = border only; live = price is in the box now (brighter border)
inline unsigned scaleCol(unsigned c, double k) {
    auto ch = [&](int s) { const int v = (int)((c >> s) & 0xFF); return (unsigned)std::max(0, std::min(255, (int)std::lround(v * k))) << s; };
    return ch(16) | ch(8) | ch(0);
}
struct Look { unsigned border = 0, fill = 0; short width = 1; bool fillOn = true; };
inline Look look(const Box& b, unsigned col, bool live) {
    Look L; const double k = b.cls == CLS_RLY ? 1.0 : 0.62;
    L.border = scaleCol(col, k); L.fill = scaleCol(col, k * 0.55); L.width = 1; L.fillOn = true;
    if (b.faint()) { L.fillOn = false; L.border = scaleCol(col, k * 0.55); }
    if (live) { L.width = 2; L.border = col; }
    return L;
}

// ---- the hover (question / what / why / how to use / example; "?" = not proven yet) -------------------------------------------------
// clk(t) formats a bar stamp as the bar's START "9:24 AM"; px(v) a price
template <class Clk, class Px>
inline std::vector<std::string> hover(const Box& b, const Track& T, Clk clk, Px px) {
    std::vector<std::string> L; const bool up = b.dir > 0, ob = b.kind == OB;
    const std::string wave = std::string(up ? "up" : "down") + "-wave " + std::to_string(b.nb) + " bars, " + fmtD(b.ratio, 1) + " ATR, Rly " + (b.rly ? "yes" : "no");
    L.push_back(std::string(ob ? "OB ?" : "BO ?") + " - " + (ob ? "order block (origin)" : "breakout area") + " of a strong " + wave + "   (wave confirmed " + clk(b.tc) + ")");
    std::string box = "Box: " + px(b.lo) + " - " + px(b.hi) + (ob ? ", the first " + std::string(b.t1 > b.t0 ? "2 candles " : "candle ") + "of the wave from " + clk(b.t0)
                                                               : ", the candle through the prior pivot " + px(b.pivot) + " at " + clk(b.t0));
    L.push_back(box);
    if (!b.tierKnown && b.prints.empty()) L.push_back("Big prints inside: Big Lot sizes still learning (no judgement on size yet)");
    else if (b.prints.empty()) L.push_back("Big prints inside: none - no real size here (drawn faint)");
    else { const Print& p = b.prints.front(); long long buy = 0, sell = 0; for (const Print& q : b.prints) (q.side > 0 ? buy : sell) += q.lots;
        L.push_back("Big prints inside: " + std::to_string(b.prints.size()) + ", largest " + std::to_string(p.lots) + (p.side > 0 ? " bought at " : " sold at ") + px(p.px) + " (" + clk(p.t) + ")"
                    + "; buyers " + std::to_string(buy) + " / sellers " + std::to_string(sell) + " lots"); }
    std::string ts = "Tests: ";
    if (T.tests.empty()) ts += "none yet";
    for (const Test& t : T.tests) { if (ts.size() > 7) ts += ",  "; ts += std::to_string(t.n) + ") " + clk(t.t0) + " " + outcomeName(t.outcome) + (t.outcome == T_SWEPT ? " (to " + px(t.far) + ")" : ""); }
    if (T.ended) ts += "  - box ended " + clk(T.tEnd) + (up ? " (closed below)" : " (closed above)");
    L.push_back(ts);
    if (ob) {
        L.push_back(up ? "Question: where did this strong up-move start, and will buyers defend that origin on a deep pullback?"
                       : "Question: where did this strong down-move start, and will sellers defend that origin on a deep bounce?");
        L.push_back(up ? "What it is: the first candles of a strong up-wave - where the buying that drove the wave began (the order block)."
                       : "What it is: the first candles of a strong down-wave - where the selling that drove the wave began (the order block).");
        L.push_back("Why it matters: only with real size inside: big prints at the origin show who launched the wave and may defend it again; an empty box is just a candle.");
        L.push_back(up ? "How to use: test 2 into a with-trend OB is the buy zone - buy only with a close back above / your reversal bar or CoB; a close below ends it."
                       : "How to use: test 2 into a with-trend OB is the sell zone - sell only with a close back below / your reversal bar or CoB; a close above ends it.");
    } else {
        L.push_back(up ? "Question: where did this strong up-wave break the prior high, and will that area hold as support on the first pullback?"
                       : "Question: where did this strong down-wave break the prior low, and will that area hold as resistance on the first bounce?");
        L.push_back(up ? "What it is: the candle that broke through the prior pivot high, within half an ATR of the pivot." : "What it is: the candle that broke through the prior pivot low, within half an ATR of the pivot.");
        L.push_back("Why it matters: breakout buyers / sellers who are right defend their entry - the first pullback (test 1) usually comes back here.");
        L.push_back(up ? "How to use: test 1 into the BO is a with-trend buy only with a close back above / your reversal bar or CoB; a close below = look for test 2 at the OB."
                       : "How to use: test 1 into the BO is a with-trend sell only with a close back below / your reversal bar or CoB; a close above = look for test 2 at the OB.");
    }
    L.push_back(std::string("Example: ") + (up ? "test 1 dips into the BO and holds; test 2 sweeps just under the OB low, closes back inside, and the wave makes a higher high."
                                                : "test 1 pops into the BO and holds; test 2 sweeps just over the OB high, closes back inside, and the wave makes a lower low."));
    L.push_back("? = not proven yet: the nightly scores every box and test (lsFlexLevels\\OrderBlock); no odds until 20+ tests a side.");
    return L;
}

} // namespace obk
