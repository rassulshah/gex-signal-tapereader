#ifndef LS_BREAKOUT_LOGIC_H
#define LS_BREAKOUT_LOGIC_H
// BreakoutLogic.h 1.0.0 (2026-11-03) -- the SDK-free part of lsBreakout ("BO", Breakout.cpp). Rassul approved the mockup
// (bo_mockup.html, ES Oct 30: "BO / BD sit small, right at the breakout candle ... letters only") and said "build".
// The chart shows ONLY letters:
//   "BO" green just ABOVE the breakout candle's high, "BD" red just BELOW the breakdown candle's low (GAP px from the wick), small font;
//        the decision is lsBigBidAsk's, by the SAME code: bba::boFromCandles (BigBidAskLogic.h 1.32.0) = an intact swing of the last 3
//        traded through, a 10-candle high / low (20-bar extreme in the wave when against his vwapTrend), range >= the prior candle's,
//        volume or range up, green / red, one per wave, and a FOLLOW-THROUGH candle later in the same wave (green, higher high and higher
//        close; BD mirrored). The letter sits on the breakout candle and appears when the follow-through candle closes.
//   "Rly" green above (end of a rally) / red below (end of a decline) on the wave's extreme bar, from RallyLogic.h (rly::run, the
//        rly2.py v0.2 port lsBigBidAsk uses), 3-min charts only, one row further out than BO / BD so the two never collide.
// No numbers, no prints, no hovers, no boxes. Decided on closed candles only; a decision once made is kept for the chart's life and
// never changes or disappears (keyed by the bar time; a later recompute never removes or alters one). Tested: tests_bo100.
#include "BigBidAskLogic.h"
#include "VwapTrendLogic.h"
#include "RallyLogic.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace bko {

static const char* const VERSION = "1.0.0";
static const int GAP = 3;              // px between the wick end and the BO / BD letter
static const int RLY_ROW = 16;         // the Rly letter sits this many px further out than the BO / BD row
static const int FONT_BO = 7;          // ~9 px letters (Rassul: small, smaller than lsBigBidAsk's 10)
static const int FONT_RLY = 9;         // the mockup's Rly size
static const unsigned long C_UP = 0x0022C55E, C_DN = 0x00EF4444;   // green bullish / red bearish (his C_BUY / C_SELL)
static const int MAX_BARS = 20000;     // bars considered (as lsBigBidAsk's Rly pass)

// one chart bar (the host fills it from IRT; t = the raw IRT stamp = the bar END, days / endSec = local CT civil day / second)
struct In { long long t = 0; bool ok = false; long days = 0; int endSec = 0; double o = 0, h = 0, l = 0, c = 0; long long vol = 0; };
struct BoMark { long long t0 = 0, t = 0; int dir = 0; double piv = 0; bool fired = false; int reason = 0; };
struct RlyMark { long long te = 0; int dir = 0; bool fired = false; };

struct Engine {
    // inputs of the last update
    std::vector<In> bars; double tick = 0; int spb = 0, openMin = 510, closeMin = 900;
    // decisions (kept for the chart's life; never changed once made)
    std::map<long long, bba::BoDec> bo;        // t0 -> lsBigBidAsk's decision record (fired or not)
    std::map<long long, RlyMark> rly;          // te * 2 + (dir > 0) -> judged wave
    std::map<long long, int> trend;            // closed bar time -> vwapTrend level (cache)
    long long lastClosedT = 0, firstT = 0; long count = -1; bool full = true;   // firstT / count = the CHART's first bar stamp and bar count
    int runs = 0, fullRuns = 0; long long boDup = 0;
    void reset() { *this = Engine(); }

    // every closed bar is final: recompute only on a new closed bar / changed history; a changed history (first bar moved, fewer bars)
    // = one FULL pass again (new decisions only are added; the kept ones stay as they are - no repaint)
    // B = the chart's last bars (the forming bar last); chartN / chartFirst = the chart's bar count and first stamp (history identity)
    bool update(const std::vector<In>& B, long chartN, long long chartFirst, double tk, int per, int om, int cm) {
        const long n = (long)B.size(); if (n < 3 || !(tk > 0) || !std::isfinite(tk)) return false;
        const long long lc = B[(size_t)n - 2].t;
        if (chartN == count && lc == lastClosedT && chartFirst == firstT && tk == tick && per == spb) return false;
        if (count >= 0 && (chartFirst != firstT || chartN < count || tk != tick || per != spb)) full = true;
        bars = B; tick = tk; spb = per; openMin = om; closeMin = cm; count = chartN; lastClosedT = lc; firstT = chartFirst;
        boPass(); rlyPass(); full = false; ++runs; return true;
    }
    int trendAt(int k) {
        if (k <= 0 || k >= (int)bars.size()) return 0;
        const long long t = bars[(size_t)k].t; const auto it = trend.find(t); if (it != trend.end()) return it->second;
        std::vector<svl::Bar> sb; sb.reserve((size_t)k);
        const int a = std::max(0, k - 1500);                                         // a session is < 500 3-min bars; plenty of room
        for (int i = a; i < k; ++i) { const In& b = bars[(size_t)i]; sb.push_back(svl::Bar{ b.days, b.endSec, (float)b.h, (float)b.l, (float)b.c, (double)b.vol }); }
        const vtl::Trend tr = vtl::fromChart(sb, (int)sb.size(), spb > 0 ? spb : 180, openMin, closeMin);
        const int lvl = tr.ready ? vtl::level(tr) : 0;
        if (k < (int)bars.size() - 1) { trend[t] = lvl; if (trend.size() > 60000) trend.erase(trend.begin()); }
        return lvl;
    }
    void boPass() {
        const int n = (int)bars.size();
        std::vector<bba::BoCandle> C((size_t)n);
        for (int k = 0; k < n; ++k) { const In& b = bars[(size_t)k]; bba::BoCandle& c = C[(size_t)k]; c.ok = b.ok; c.days = b.days; c.endSec = b.endSec; c.o = b.o; c.h = b.h; c.l = b.l; c.c = b.c; c.vol = b.vol; }
        int from = 1, emitFrom = 1, fill0 = 0;
        if (!full) bba::boWindow(n, from, emitFrom, fill0);                          // lsBigBidAsk's live window
        else { fill0 = std::max(0, n - 1 - MAX_BARS); from = emitFrom = std::max(1, fill0); ++fullRuns; }
        const bba::BoRun R = bba::boFromCandles(C, tick, from, n - 1, emitFrom, fill0,
            [&](int k) { return trendAt(k); }, [](int) { return true; }, [](int, bba::BoIn&) {});
        for (bba::BoDec D : R.ev) {
            if (D.i0 < 0 || D.im < 0 || D.im >= n - 1) continue;
            D.t0 = bars[(size_t)D.i0].t; D.t = bars[(size_t)D.im].t;
            if (bo.count(D.t0)) continue;                                            // decided before: never again
            if (bba::boDuplicate(bo, D)) { ++boDup; continue; }
            bo[D.t0] = D;
        }
        while (bo.size() > 40000) bo.erase(bo.begin());
    }
    void rlyPass() {
        if (spb != 180) return;                                                      // the rule was built and tested on 3-min bars
        const int n = (int)bars.size(); const int first = std::max(0, n - 1 - MAX_BARS);
        std::vector<rly::Bar> B; std::vector<long long> T;
        for (int k = first; k < n - 1; ++k) {                                        // closed bars only
            const In& x = bars[(size_t)k];
            if (!x.ok || !std::isfinite(x.o) || !std::isfinite(x.h) || !std::isfinite(x.l) || !std::isfinite(x.c) || x.h < x.l) continue;
            rly::Bar b; b.days = x.days; b.endSec = x.endSec; b.o = x.o; b.h = x.h; b.l = x.l; b.c = x.c; b.v = (double)x.vol; B.push_back(b); T.push_back(x.t);
        }
        const int from = full ? 0 : std::max(0, (int)B.size() - 600);
        const std::vector<rly::Wave> W = rly::run(B, spb, openMin, closeMin, from);
        for (const rly::Wave& w : W) {
            if (w.ge < 0 || w.gp < 0 || w.gc < 0 || w.ge >= (int)T.size()) continue;
            const long long te = T[(size_t)w.ge], key = te * 2 + (w.dir > 0 ? 1 : 0);
            if (rly.count(key)) continue;                                            // judged before: never again
            RlyMark m; m.te = te; m.dir = w.dir; m.fired = w.fired; rly[key] = m;
        }
        while (rly.size() > 20000) rly.erase(rly.begin());
    }
};

// ---- layout: what the host draws (one text per letter; the host adds the 1 px black edge as lsBigBidAsk did) ---------------------
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline bool hits(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline bool inside(const Box& a, const Box& in) { return a.l >= in.l && a.r <= in.r && a.t >= in.t && a.b <= in.b; }
struct Txt { std::string s; Box f; unsigned long col = 0; int font = 0; int bar = 0; int dir = 0; int kind = 0; };   // kind 0 BO / BD, 1 Rly
struct View {
    int first = 0, last = -1;                       // visible chart bars (inclusive)
    Box area;                                       // where letters may go (price area, below the IRT title row)
    std::vector<Box> keepOut;                       // other indicators' boxes (Tape Reader banner, HOD/LOD table)
    std::function<int(int)> X;                      // bar -> centre x
    std::function<int(double)> Y;                   // price -> y
    std::function<int(const char*, int)> textW;     // text, font -> px
    int lh[2] = { 12, 14 };                         // letter row heights: BO / BD font, Rly font
};
struct Board { long long t = 0; int side = 0; int val = 0; };   // SignalBoard 'L': bar time, +1 above / -1 below, px from the extreme to the outer edge
struct Scene { std::vector<Txt> txt; std::vector<Board> board; int boShown = 0, rlyShown = 0, noRoom = 0; };

inline Scene layout(const Engine& E, const View& v) {
    Scene S; const int n = (int)E.bars.size(); if (n < 2 || !v.X || !v.Y || !v.textW) return S;
    std::map<long long, int> idx; const int a = std::max(0, v.first), z = std::min(v.last, n - 2);   // closed bars in view
    for (int i = a; i <= z; ++i) idx[E.bars[(size_t)i].t] = i;
    std::map<std::pair<int, int>, int> reach;       // (bar, side) -> outer edge in px from the wick end
    auto put = [&](int bar, int dir, int kind) {
        const In& b = E.bars[(size_t)bar]; if (!std::isfinite(b.h) || !std::isfinite(b.l)) return;
        const char* tx = kind == 1 ? "Rly" : (dir > 0 ? "BO" : "BD"); const int font = kind == 1 ? FONT_RLY : FONT_BO;
        const int w = v.textW(tx, font) + 2, cx = v.X(bar), yh = v.Y(b.h), yl = v.Y(b.l), top = std::min(yh, yl), bot = std::max(yh, yl);
        const int lh = v.lh[kind == 1 ? 1 : 0], off = kind == 1 ? GAP + std::max(RLY_ROW, v.lh[0] + 3) : GAP;
        Txt T; T.s = tx; T.font = font; T.bar = bar; T.dir = dir; T.kind = kind; T.col = dir > 0 ? C_UP : C_DN;
        T.f.l = cx - w / 2; T.f.r = T.f.l + w;
        if (dir > 0) { T.f.b = top - off; T.f.t = T.f.b - lh; } else { T.f.t = bot + off; T.f.b = T.f.t + lh; }
        bool ok = inside(T.f, v.area); for (const Box& k : v.keepOut) if (k.r > k.l && k.b > k.t && hits(T.f, k)) ok = false;
        if (!ok) { ++S.noRoom; return; }
        S.txt.push_back(T); if (kind == 1) ++S.rlyShown; else ++S.boShown;
        int& r = reach[std::make_pair(bar, dir > 0 ? 1 : -1)]; r = std::max(r, dir > 0 ? top - T.f.t : T.f.b - bot);
    };
    std::vector<std::pair<int, int>> boBars;
    for (const auto& kv : E.bo) { const bba::BoDec& D = kv.second; if (!D.fired) continue; const auto it = idx.find(D.t0); if (it == idx.end()) continue; put(it->second, D.dir, 0); boBars.push_back({ it->second, D.dir > 0 ? 1 : -1 }); }
    for (const auto& kv : E.rly) { const RlyMark& R = kv.second; if (!R.fired) continue; const auto it = idx.find(R.te); if (it == idx.end()) continue; put(it->second, R.dir, 1); }
    for (const auto& p : boBars) { const auto it = reach.find(p); if (it == reach.end()) continue; Board B; B.t = E.bars[(size_t)p.first].t; B.side = p.second; B.val = it->second; S.board.push_back(B); }
    return S;
}

// the status file body (version first; counts of today's decisions by the last closed bar's CT day)
inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb, const Scene& last) {
    long today = 0; if (E.bars.size() >= 2) today = E.bars[E.bars.size() - 2].days;
    int bo = 0, bd = 0, nf = 0, ru = 0, rd = 0;
    std::map<long long, long> dayOf; for (const In& b : E.bars) dayOf[b.t] = b.days;
    for (const auto& kv : E.bo) { const auto it = dayOf.find(kv.first); if (it == dayOf.end() || it->second != today) continue; if (kv.second.fired) (kv.second.dir > 0 ? bo : bd)++; else ++nf; }
    for (const auto& kv : E.rly) { const auto it = dayOf.find(kv.second.te); if (it == dayOf.end() || it->second != today || !kv.second.fired) continue; (kv.second.dir > 0 ? ru : rd)++; }
    std::string s = std::string("VERSION,") + VERSION + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nSPB," + std::to_string(spb) + "\n";
    s += "RULES,BO / BD = lsBigBidAsk " + std::string(bba::VERSION) + " bba::boFromCandles (rule " + std::to_string(bba::BO_RULE) + ", follow-through in the same wave); Rly = RallyLogic " + rly::VERSION + (spb == 180 ? "" : " (3-min charts only: off on this chart)") + "\n";
    s += "TODAY,BO " + std::to_string(bo) + ",BD " + std::to_string(bd) + ",not fired " + std::to_string(nf) + ",Rly up " + std::to_string(ru) + ",Rly down " + std::to_string(rd) + "\n";
    s += "KEPT,breakout decisions " + std::to_string(E.bo.size()) + ",rally waves " + std::to_string(E.rly.size()) + ",duplicates skipped " + std::to_string(E.boDup) + ",runs " + std::to_string(E.runs) + " (full " + std::to_string(E.fullRuns) + ")\n";
    s += "DRAWN,BO/BD " + std::to_string(last.boShown) + ",Rly " + std::to_string(last.rlyShown) + ",no room " + std::to_string(last.noRoom) + "\n";
    return s;
}

} // namespace bko
#endif
