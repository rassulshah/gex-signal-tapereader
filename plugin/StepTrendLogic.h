#ifndef LS_STEPTREND_LOGIC_H
#define LS_STEPTREND_LOGIC_H
// StepTrendLogic.h 1.1.0 (2026-11-11; 1.1.0 = the support lines end when tested or at the next step) -- lsStepTrend "Step Trend": every rule of the indicator, no SDK (unit-tested on Linux, the same
// walk as the mockup Rassul approved and as lra.step_trend_score scores every night).
//
// Rassul 2026-11-10: "a stepwise trend indicator. It looks at the hourly bar and the current zig zag rotation (going up or down), if the
// hourly close and high are above the prior hourly close or high, it considers this an hourly breakout and on the 3min ltf chart the
// level drawn is on the prior hourly bar high"; "those hourly highs should be coming from the top of the waves"; "they should be connected
// steps"; "get rid of the labels"; "connect the tops without going across"; "once it steps up, the prior high becomes support, so you can
// extend it as a subtle dashed line, not too thick"; 23:53 "build" (Step Trend indicator, clock hours); 2026-11-11 00:08 "when the rotation
// is up and the hourly bar closes are above prior bar highs the lines are green, and if its vice versa ... red"; 00:11 "the bar closing
// higher needs to be green and vice versa".
//
//   HOURS     clock hours of the chart's own bars (a bar belongs to the hour its START falls in). An hour is CLOSED when a later bar exists.
//   BREAKOUT  decided once, when the hour closes: rule 0 = it closed above the prior hour's high (up) / below its low (down).
//   BODY      the breakout hour itself closes green for a step up, red for a step down (its close vs its own open).
//   ROTATION  the hourly ZigZag (High / Low, 1 bar): a breakout counts only while it rotates the same way (or has not rotated yet).
//   STEP      level = the prior hour's high (up) / low (down); it starts at the candle that made it (the wave top / bottom).
//   SUPPORT   once the next step in the same direction exists, the old level runs on (thin, dashed) until it is TESTED (the first candle
//             after the new step's start that trades back to it) or until ANOTHER STEP is made, up or down (it stops where that step
//             starts, at its wave top / bottom; a test that happened before that step's hour closed keeps its place) (1.1.0, Rassul 2026-11-11 07:21 CT "the retest lines should not extend indefinitely.
//             they should stop once they are tested or once another step up is made or a step down is made").
// Nothing decided is ever changed: a step is added when its hour closes and stays; a support line only gets its end, once. (While untested
// it is drawn to the right edge; when another step is confirmed it is cut back to where that step starts, as the steps themselves are
// drawn back to their wave tops.)
#include <algorithm>
#include <string>
#include <vector>
namespace stp {
static const char* const VERSION = "1.1.0";
static const int MAX_BARS = 5000, SLACK = 2000;      // the host feeds the last MAX_BARS closed candles and re-feeds when SLACK more have come

struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; };                         // t = candle START (wall seconds of the chart); prices in ticks
struct Hour { long long k = 0, o = 0, h = 0, l = 0, c = 0; int hiI = 0, loI = 0, n = 0; };   // hiI / loI = the candle that made the high / low (the last one on a tie)
struct Step { int dir = 0; long long px = 0, hour = 0, tConf = 0; int oi = 0; long long ot = 0; bool ok = true; int rotAt = 0; int sup = 0; int supEnd = -1; };
//   sup: 0 = no support line (no later step the same way yet / the next step turned), 1 = running, 2 = ended at candle supEnd
struct Seg { int i0 = 0; long long p0 = 0; int i1 = 0; long long p1 = 0; int dir = 0; int kind = 0; };   // kind 0 solid step, 1 the live level (dashed, to the right edge), 2 support (thin dashed)
struct Dot { int i = 0; long long px = 0; int dir = 0; };

inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline bool periodOk(int spb) { return spb > 0 && spb <= 1800 && 3600 % spb == 0; }

struct Engine {
    int rule = 0; bool rot = true, body = true;
    std::vector<Bar> B; std::vector<Hour> H; std::vector<Step> S, skipped;    // S = the steps drawn (in order); skipped = breakouts against the rotation
    bool curClosed = false;                                                    // the newest hour in H has been closed (by a later candle or sealTo)
    int zdir = 0; std::vector<int> rotv;                                       // hourly ZigZag rotation after each closed hour (+1 up, -1 down, 0 none yet)
    struct Piv { int i; long long px; int s; }; std::vector<Piv> P;            // its pivots (hour index, price, +1 high / -1 low)

    static bool tested(const Step& o, const Bar& b) { return o.dir > 0 ? b.l <= o.px : b.h >= o.px; }   // (1.1.0) price traded back to the level
    void reset() { B.clear(); H.clear(); S.clear(); skipped.clear(); curClosed = false; zdir = 0; rotv.clear(); P.clear(); }

    // the hour H.back() is complete: rotate the ZigZag, then decide the breakout (once)
    void closeHour() {
        if (H.empty() || curClosed) return;
        curClosed = true;
        const int k = (int)H.size() - 1, i = k - 1;
        if (i >= 1) {
            const bool isH = H[i].h > H[i - 1].h && H[i].h >= H[k].h, isL = H[i].l < H[i - 1].l && H[i].l <= H[k].l;
            if (zdir >= 0 && isH && !(zdir == 0 && isL)) { if (zdir == 0) P.push_back({ 0, H[0].l, -1 }); P.push_back({ i, H[i].h, 1 }); zdir = -1; }
            else if (zdir <= 0 && isL) { if (zdir == 0) P.push_back({ 0, H[0].h, 1 }); P.push_back({ i, H[i].l, -1 }); zdir = 1; }
            else if (zdir > 0 && isL && !P.empty() && P.back().s < 0 && H[i].l < P.back().px) P.back() = { i, H[i].l, -1 };
            else if (zdir < 0 && isH && !P.empty() && P.back().s > 0 && H[i].h > P.back().px) P.back() = { i, H[i].h, 1 };
        }
        rotv.push_back(zdir);
        if (k < 1) return;
        const Hour& p = H[k - 1]; const Hour& c = H[k];
        bool up = rule == 0 ? c.c > p.h : (c.h > p.h && c.c > p.c), dn = rule == 0 ? c.c < p.l : (c.l < p.l && c.c < p.c);
        if (body) { up = up && c.c > c.o; dn = dn && c.c < c.o; }             // the breakout hour itself is a green (up) / red (down) candle
        if (up == dn) return;                                                  // no breakout (or both ways: none)
        Step q; q.dir = up ? 1 : -1; q.px = up ? p.h : p.l; q.hour = c.k * 3600; q.tConf = (c.k + 1) * 3600; q.oi = up ? p.hiI : p.loI; q.ot = B[(size_t)q.oi].t;
        q.rotAt = zdir; q.ok = !rot || zdir == 0 || (up && zdir > 0) || (dn && zdir < 0);
        if (!q.ok) { skipped.push_back(q); return; }
        const int last = (int)B.size() - 1;                                    // the candle that closed this hour
        for (size_t s = 0; s < S.size(); ++s) if (S[s].sup == 1) { S[s].sup = 2; S[s].supEnd = std::max(S[s].oi, q.oi); }   // (1.1.0) another step was made: a running line stops where that step starts (its wave top / bottom)
        if (!S.empty() && S.back().dir == q.dir) {                             // the old level becomes support / resistance
            Step& o = S.back(); o.sup = 1;
            for (int j = q.oi + 1; j <= last; ++j) if (tested(o, B[(size_t)j])) { o.sup = 2; o.supEnd = j; break; }
        }
        S.push_back(q);
    }
    // a CLOSED candle, in time order; false = out of order (the host re-feeds)
    bool addBar(const Bar& b) {
        if (!B.empty() && b.t <= B.back().t) return false;
        const long long k = floorDiv(b.t, 3600); const int idx = (int)B.size();
        if (!H.empty() && k != H.back().k) closeHour();
        B.push_back(b);
        if (H.empty() || k != H.back().k) { Hour h; h.k = k; h.o = b.o; h.h = b.h; h.l = b.l; h.c = b.c; h.hiI = h.loI = idx; h.n = 1; H.push_back(h); curClosed = false; }
        else { Hour& h = H.back(); if (b.h >= h.h) { h.h = b.h; h.hiI = idx; } if (b.l <= h.l) { h.l = b.l; h.loI = idx; } h.c = b.c; ++h.n; }
        for (size_t s = 0; s + 1 < S.size(); ++s) { Step& o = S[s]; if (o.sup == 1 && idx > S[s + 1].oi && tested(o, b)) { o.sup = 2; o.supEnd = idx; } }
        return true;
    }
    // the forming candle starts at t: every candle of an earlier hour is closed, so that hour is complete
    void sealTo(long long t) { if (!H.empty() && !curClosed && floorDiv(t, 3600) > H.back().k) closeHour(); }
};

// what the chart draws (candle indices of the engine; lastIdx = the newest candle on the chart, in the same index space)
inline void layout(const Engine& E, int lastIdx, std::vector<Seg>& segs, std::vector<Dot>& dots) {
    segs.clear(); dots.clear();
    const size_t n = E.S.size();
    for (size_t i = 0; i < n; ++i) { const Step& q = E.S[i];                   // support first (under the steps)
        const int e = q.sup == 2 ? q.supEnd : lastIdx;
        if (q.sup != 0 && e > q.oi) { Seg s; s.i0 = q.oi; s.p0 = q.px; s.i1 = e; s.p1 = q.px; s.dir = q.dir; s.kind = 2; segs.push_back(s); } }
    for (size_t i = 0; i < n; ++i) { const Step& q = E.S[i];
        if (i + 1 < n) { const Step& x = E.S[i + 1];
            Seg v; v.i0 = q.oi; v.p0 = q.px; v.i1 = q.oi; v.p1 = x.px; v.dir = x.dir; v.kind = 0; segs.push_back(v);          // straight up (down) at the top
            Seg h; h.i0 = q.oi; h.p0 = x.px; h.i1 = x.oi; h.p1 = x.px; h.dir = x.dir; h.kind = 0; segs.push_back(h); }        // then across to the next top
        else if (lastIdx >= q.oi) { Seg l; l.i0 = q.oi; l.p0 = q.px; l.i1 = lastIdx; l.p1 = q.px; l.dir = q.dir; l.kind = 1; segs.push_back(l); }
        Dot d; d.i = q.oi; d.px = q.px; d.dir = q.dir; dots.push_back(d); }
}
} // namespace stp
#endif
