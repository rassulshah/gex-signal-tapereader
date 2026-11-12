#ifndef LS_STEPTREND_LOGIC_H
#define LS_STEPTREND_LOGIC_H
// StepTrendLogic.h 1.2.0 (2026-11-11; 1.2.0 = the hourly rotation follows a 1-bar ZigZag; 1.1.0 = the support lines end when tested or at the next step) -- lsStepTrend "Step Trend": every rule of the indicator, no SDK (unit-tested on Linux, the same
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
//             (1.2.0, Rassul 2026-11-11 07:55 CT "the green step didnt draw when the nq made a higher green bar") 1.0.0 / 1.1.0 turned the
//             rotation only at a 3-hour pivot, so after one lower high it stayed "down" while the hours kept breaking higher (NQ 4:00 and
//             5:00 AM skipped; 24 on NQ, 28 on ES). His ZigZag turns on ONE bar: higher high = up, lower low = down, outside hour = its colour.
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
static const char* const VERSION = "1.5.0";   // (1.5.0) the shorter periods draw the 60-minute design (stp::layoutLtf)   // was "1.4.5";   // (1.4.5) hourly: a flip's line starts at the candle before the step candle, at the broken level   // was "1.4.4";   // (1.4.4) host: B over the high of the breaking red candle (under the low of a breaking green candle)   // was "1.4.3";   // (1.4.3, 2026-11-11 Rassul 17:06 CT "the red line should be from the green candle low not its high"): when the trend flips, the new line starts at the OTHER end of the last step candle (a step down after step ups starts at that green candle's low; a step up after step downs at that red candle's high)   // was "1.4.2";   // (1.4.2) host: T / B 8 px off the wick   // was "1.4.1";   // (1.4.1) the B is drawn under the low of the breaking candle (host), red for a broken up-step   // was "1.4.0";   // (1.4.0, 2026-11-11 Rassul 16:59 CT "show B for Trend Break when the close is below the step up, like the last red candle"): B = the first candle that CLOSES below the newest step-up candle's low (above the high for a step down)   // was "1.3.3";   // (1.3.3, 2026-11-11 Rassul 16:56 CT "there is a rule that the low needs to be above to display T, lets make this close instead"): the test is judged by the CLOSE - T when the candle touches the level and closes at / above the step-up candle's low; a close below it ends the line with no T   // was "1.3.2";   // (1.3.2, 2026-11-11 Rassul 16:53 CT "the line goes up from the former step up and then across to the next step up"): the solid line rises AT the former step-up candle to the next step's high, then runs across to the next step-up candle   // was "1.3.1";   // (1.3.1, 2026-11-11 Rassul 16:47-16:51 CT, his sketch) the 60-minute chart draws TWO lines: the solid stairs from the high of one step-up candle across to the next step-up candle and up to its high; the dashed test line from the prior high across   // was "1.3.0";   // (1.3.0, 2026-11-11 Rassul 15:08 / 15:12 CT) the 60-minute chart: the step sits on the breakout candle, the dashed line starts there, "T" = the level tested   // was "1.2.0";
static const int MAX_BARS = 5000, SLACK = 2000;      // the host feeds the last MAX_BARS closed candles and re-feeds when SLACK more have come

struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; };                         // t = candle START (wall seconds of the chart); prices in ticks
struct Hour { long long k = 0, o = 0, h = 0, l = 0, c = 0; int hiI = 0, loI = 0, n = 0; };   // hiI / loI = the candle that made the high / low (the last one on a tie)
struct Step { int dir = 0; long long px = 0, hour = 0, tConf = 0; int oi = 0; long long ot = 0; bool ok = true; int rotAt = 0; int sup = 0; int supEnd = -1;
    int bi = -1; long long blo = 0, bhi = 0; int tI = -1, endI = -1; int brI = -1; };   // (1.4.0) brI = the candle that BROKE the trend: the first close below this (newest) step-up candle's low   // (1.3.0, hourly chart) bi = the breakout candle (its low / high), tI = the candle that TESTED the level, endI = where the dashed line stops
//   sup: 0 = no support line (no later step the same way yet / the next step turned), 1 = running, 2 = ended at candle supEnd
struct Seg { int i0 = 0; long long p0 = 0; int i1 = 0; long long p1 = 0; int dir = 0; int kind = 0; };   // kind 0 solid step, 1 the live level (dashed, to the right edge), 2 support (thin dashed)
struct Dot { int i = 0; long long px = 0; int dir = 0; };
struct Mark { int i = 0; int dir = 0; int kind = 0; };   // kind 0 = T (tested), 1 = B (trend break; 1.4.1: under the low of the breaking candle for a broken up-step, red; over the high for a broken down-step, green)   // (1.3.0) "T": under the low of the candle that tested a step up (over the high for a step down)

inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline bool periodOk(int spb) { return spb > 0 && spb <= 3600 && 3600 % spb == 0; }   // (1.2.0) + the 60-minute chart itself: each candle is an hour, the same steps

struct Engine {
    int rule = 0; bool rot = true, body = true;
    // (1.3.0, Rassul 2026-11-11 15:08 CT "only draw the dashed line after the stp up. when a candle then pulls back and touches it, without breaking the
    // step up candle's low, it can be called a T which means that the hbo is tested ... you put the step on the wrong red candle next to it even though only
    // green candles that close higher are suppose to have the step"; 15:12 "it should go under the low of the candle. only when it is touched or exceeded")
    // HOURLY CHART (each candle is an hour): the step belongs to the BREAKOUT candle (the green one that closed above the prior high), not to the candle
    // whose high it broke; the dashed line runs from the breakout candle on, at the broken level, until a later candle TESTS it (its low touches or goes
    // through the level without trading below the breakout candle's low = "T"), or trades below that low (no T), or another step is made. Mirror for down.
    bool hourly = false;
    std::vector<Bar> B; std::vector<Hour> H; std::vector<Step> S, skipped;    // S = the steps drawn (in order); skipped = breakouts against the rotation
    bool curClosed = false;                                                    // the newest hour in H has been closed (by a later candle or sealTo)
    int zdir = 0; std::vector<int> rotv;                                       // hourly ZigZag rotation after each closed hour (+1 up, -1 down, 0 none yet)

    static bool tested(const Step& o, const Bar& b) { return o.dir > 0 ? b.l <= o.px : b.h >= o.px; }   // (1.1.0) price traded back to the level
    void reset() { B.clear(); H.clear(); S.clear(); skipped.clear(); curClosed = false; zdir = 0; rotv.clear(); }

    // the hour H.back() is complete: rotate the ZigZag, then decide the breakout (once)
    void closeHour() {
        if (H.empty() || curClosed) return;
        curClosed = true;
        const int k = (int)H.size() - 1, i = k - 1;
        if (i >= 0) {   // (1.2.0) the hourly ZigZag, High / Low, 1 bar: a higher high turns it up, a lower low turns it down, an outside hour goes by its own colour, an inside hour changes nothing
            const bool hh = H[k].h > H[i].h, ll = H[k].l < H[i].l;
            if (hh && !ll) zdir = 1; else if (ll && !hh) zdir = -1; else if (hh && ll) zdir = H[k].c > H[k].o ? 1 : H[k].c < H[k].o ? -1 : zdir;
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
        if (hourly) { q.bi = last; q.blo = B[(size_t)last].l; q.bhi = B[(size_t)last].h; for (size_t s = 0; s < S.size(); ++s) if (S[s].endI < 0) S[s].endI = last; }   // (1.3.0)
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
        if (hourly && !S.empty()) { Step& nw = S.back();   // (1.4.0) TREND BREAK: the first candle that closes below the newest step-up candle's low (above the high of a step-down candle); once
            if (nw.brI < 0 && idx > nw.bi && (nw.dir > 0 ? b.c < nw.blo : b.c > nw.bhi)) nw.brI = idx; }
        if (hourly) for (size_t s = 0; s < S.size(); ++s) { Step& o = S[s]; if (o.endI >= 0 || idx <= o.bi) continue;   // (1.3.0) the test: touched or exceeded, the breakout candle's low (high) not broken
            if (o.dir > 0 ? b.c < o.blo : b.c > o.bhi) { o.endI = idx; continue; }   // (1.3.3) by the CLOSE (was the low / high)
            if (o.dir > 0 ? b.l <= o.px : b.h >= o.px) { o.tI = idx; o.endI = idx; } }
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
// (1.3.1, Rassul 2026-11-11 16:47 CT "there are suppose to be 2 lines, the solid line stepping up from high to high based on a hbo and a dashed testing
// line which goes from the prior high accross when there is a step up"; 16:50 "the solid line is from high of a prior step up to the next step up";
// 16:51 his sketch) the 60-minute chart:
//   SOLID   (1.3.2) from the HIGH of a step-up candle UP at that candle to the next step-up candle's high, then across to that candle (step down: lows); the dot on
//           the step candle at its high (low).
//   DASHED  the test line at the broken level (the prior high), from the candle before the step candle across to its test / end (else the right edge).
//   T       a mark at each test (stp::Mark).
inline long long stepTop(const Step& q) { return q.dir > 0 ? q.bhi : q.blo; }
inline void layoutHourly(const Engine& E, int lastIdx, std::vector<Seg>& segs, std::vector<Dot>& dots, std::vector<Mark>& marks) {
    segs.clear(); dots.clear(); marks.clear(); const size_t n = E.S.size();
    for (size_t i = 0; i < n; ++i) { const Step& q = E.S[i]; if (q.bi < 1) continue; const int e = q.endI >= 0 ? q.endI : lastIdx;
        if (e > q.bi - 1) { Seg s; s.i0 = q.bi - 1; s.p0 = q.px; s.i1 = e; s.p1 = q.px; s.dir = q.dir; s.kind = 2; segs.push_back(s); } }
    for (size_t i = 0; i < n; ++i) { const Step& q = E.S[i]; if (q.bi < 0) continue;
        if (i > 0 && E.S[i - 1].bi >= 0) { const Step& p = E.S[i - 1];
            // (1.4.5, Rassul 2026-11-11 19:33-19:34 CT "shouldn't the line go from the high of the bar before the hbo to the hbo bar hi" -> flips only)
            // same direction: up at the former step candle from its high (1.3.2). A FLIP: at the candle BEFORE the step candle, from the level
            // that was broken (that candle's high; step down: its low) to the step candle's high (low) - always points the step's way
            const bool flip = p.dir != q.dir; const int at = flip ? q.bi - 1 : p.bi; const long long from = flip ? q.px : stepTop(p);   // was (1.4.3): at p.bi from q.dir > 0 ? p.bhi : p.blo
            Seg v; v.i0 = at; v.p0 = from; v.i1 = at; v.p1 = stepTop(q); v.dir = q.dir; v.kind = 0; segs.push_back(v);
            Seg h; h.i0 = at; h.p0 = stepTop(q); h.i1 = q.bi; h.p1 = stepTop(q); h.dir = q.dir; h.kind = 0; segs.push_back(h); }   // then across to the next step-up candle, at its high
        Dot d; d.i = q.bi; d.px = stepTop(q); d.dir = q.dir; dots.push_back(d);
        if (q.tI >= 0) { Mark m; m.i = q.tI; m.dir = q.dir; marks.push_back(m); }
        if (q.brI >= 0) { Mark m; m.i = q.brI; m.dir = q.dir; m.kind = 1; marks.push_back(m); } }
}
// (1.5.0, Rassul 2026-11-11 19:32 CT "the 3m ltf is inconsistent with the 60m steps" -> 19:37 "go") the SHORTER periods (his 3-minute chart) draw
// the SAME design as the 60-minute chart: the closed hours of the chart's own candles are walked exactly as the hourly chart walks its
// candles (the hourly Engine: same steps, tests by the hour's low + close, trend break by the hour's close), then placed on the candles:
//   SOLID   up (down) at the candle that made the former step hour's high (low), from that high to the new step hour's high, then across to
//           the candle that made it (the dot). A flip: at the candle that made the BROKEN high (low), from that level.
//   DASHED  the test line at the broken level, from the candle that made it to the candle that first touched it in the hour that tested it
//           (T under / over that candle), else to the last candle of the hour that closed through the step hour's low (high), else to the
//           next step's candle, else the right edge.
//   B       on the last candle of the hour whose close broke the newest step hour's low (high).
// The forming hour is never used: everything appears when its hour closes and is not changed after.
inline void layoutLtf(const Engine& E, int lastIdx, std::vector<Seg>& segs, std::vector<Dot>& dots, std::vector<Mark>& marks) {
    segs.clear(); dots.clear(); marks.clear();
    const int nh = (int)E.H.size() - (E.curClosed ? 0 : 1); if (nh < 2) return;
    std::vector<int> st(E.H.size() + 1, 0); for (size_t j = 0; j < E.H.size(); ++j) st[j + 1] = st[j] + E.H[j].n;
    Engine X; X.rule = E.rule; X.rot = E.rot; X.body = E.body; X.hourly = true;
    for (int j = 0; j < nh; ++j) { const Hour& h = E.H[(size_t)j]; Bar b; b.t = h.k * 3600; b.o = h.o; b.h = h.h; b.l = h.l; b.c = h.c; X.addBar(b); }
    X.sealTo((E.H[(size_t)nh - 1].k + 1) * 3600);
    auto top = [&](int j, int dir) { return dir > 0 ? E.H[(size_t)j].hiI : E.H[(size_t)j].loI; };
    auto lastOf = [&](int j) { return st[(size_t)j + 1] - 1; };
    auto touch = [&](int j, const Step& q) { for (int i = st[(size_t)j]; i < st[(size_t)j + 1] && i < (int)E.B.size(); ++i) if (q.dir > 0 ? E.B[(size_t)i].l <= q.px : E.B[(size_t)i].h >= q.px) return i; return lastOf(j); };
    const size_t n = X.S.size();
    for (size_t i = 0; i < n; ++i) { const Step& q = X.S[i]; if (q.bi < 1) continue;
        int e = lastIdx;
        if (q.tI >= 0) e = touch(q.tI, q);
        else if (q.endI >= 0) { e = lastOf(q.endI); for (size_t k = i + 1; k < n; ++k) if (X.S[k].bi == q.endI) { e = top(q.endI, X.S[k].dir); break; } }
        const int a = top(q.bi - 1, q.dir);
        if (e > a) { Seg s; s.i0 = a; s.p0 = q.px; s.i1 = e; s.p1 = q.px; s.dir = q.dir; s.kind = 2; segs.push_back(s); } }
    for (size_t i = 0; i < n; ++i) { const Step& q = X.S[i]; if (q.bi < 0) continue;
        if (i > 0 && X.S[i - 1].bi >= 0 && q.bi >= 1) { const Step& p = X.S[i - 1];
            const bool flip = p.dir != q.dir; const int at = flip ? top(q.bi - 1, q.dir) : top(p.bi, p.dir); const long long from = flip ? q.px : stepTop(p);
            Seg v; v.i0 = at; v.p0 = from; v.i1 = at; v.p1 = stepTop(q); v.dir = q.dir; v.kind = 0; segs.push_back(v);
            Seg h; h.i0 = at; h.p0 = stepTop(q); h.i1 = top(q.bi, q.dir); h.p1 = stepTop(q); h.dir = q.dir; h.kind = 0; segs.push_back(h); }
        Dot d; d.i = top(q.bi, q.dir); d.px = stepTop(q); d.dir = q.dir; dots.push_back(d);
        if (q.tI >= 0) { Mark m; m.i = touch(q.tI, q); m.dir = q.dir; marks.push_back(m); }
        if (q.brI >= 0) { Mark m; m.i = lastOf(q.brI); m.dir = q.dir; m.kind = 1; marks.push_back(m); } }
}
} // namespace stp
#endif
