/********************************************************************************
 *  StepTrend.cpp  --  Investor/RT RTX extension  lsStepTrend  "Step Trend"  1.4.5  (2026-11-11)
 *
 *  1.4.5 (Rassul 2026-11-11 19:33 CT "on the 60 min shouldn't the line go from the high of the bar before the hbo to the hbo bar hi", 19:34
 *  "i'll go with your recommendation" = flips only) hourly: the first step after a change of direction starts at the candle before the
 *  step candle, from the broken level to the step candle's high (low), then across; same-direction steps unchanged.
 *  (was 1.4.4)
 *
 *  1.4.4 (Rassul 2026-11-11 17:13 CT "change the location of B to be on top of the red bar so it doesn't overlap the lines and vice versa") B over the
 *  high of the red candle that broke a step up (red); under the low of the green candle that broke a step down (green).
 *  (was 1.4.3)
 *
 *  1.4.3 (Rassul 2026-11-11 17:06 CT) a flipped line starts at the other end of the last step candle (red from the green candle's low).
 *  (was 1.4.2)
 *
 *  1.4.2 the T / B letters sit 8 px off the wick so they clear the step dot (a breaking candle is often the next step candle).
 *  (was 1.4.1)
 *
 *  1.4.1 (Rassul 2026-11-11 17:01 CT "since its a redbar put the B below") the B sits under the low of the breaking candle (red); mirror: over the high (green).
 *  (was 1.4.0)
 *
 *  1.4.0 (Rassul 2026-11-11 16:59 CT) "B" = Trend Break on the 60-minute chart: the first candle that closes below the newest step-up candle's low
 *  (red, over its high); mirror for a step down (green, under its low).
 *  (was 1.3.3)
 *
 *  1.3.3 (Rassul 2026-11-11 16:56 CT) the T test is judged by the close: a wick below the step-up candle's low still earns a T when the candle closes back above it.
 *  (was 1.3.2)
 *
 *  1.3.2 (Rassul 2026-11-11 16:53 CT "the line goes up from the former step up and then across to the next step up") the solid line of the
 *  60-minute chart rises at the former step-up candle to the next step's high, then runs across to the next step-up candle.
 *  (was 1.3.1)
 *
 *  1.3.1 (Rassul 2026-11-11 16:47-16:51 CT, his sketch) the 60-minute chart: two lines - solid stairs high to high, dashed test line from the prior high.
 *
 *  1.3.0 (Rassul 2026-11-11 15:08 / 15:12 CT) the 60-minute chart: the step on the breakout candle, the dashed line from it on, "T" under the low of
 *  the candle that tested the level (stp::Engine::hourly, stp::layoutHourly). Shorter periods are unchanged.
 *
 *  1.2.0 (Rassul 2026-11-11 07:55 CT "the green step didnt draw when the nq made a higher green bar"): the hourly rotation follows his
 *    1-bar High / Low ZigZag (a higher high turns it up, a lower low down, an outside hour by its colour). 1.0.0 / 1.1.0 used a 3-hour pivot
 *    and skipped valid breakouts after one lower high (NQ 2026-11-11 4:00 and 5:00 AM).
 *    + (08:04 CT "i also want a version of this indicator to draw on the hourly charts") the same indicator works on a 60-minute chart:
 *    each candle is an hour, so the steps are the same levels, drawn from the candle that made each one.
 *  1.1.0 (Rassul 2026-11-11 07:21 CT "the retest lines should not extend indefinitely . they should stop once they are tested or once another
 *    step up is made or a step down is made"): the thin dashed support / resistance line of an old level ends at the first candle that
 *    trades back to it (after the new step's start), or where the NEXT step (up or down) starts (its wave top / bottom) - whichever is
 *    first. (1.0.0 ran it until a candle CLOSED back through the level, so an untested level ran to the right edge.)
 *  1.0.0 (Rassul 2026-11-10 22:28 CT "I am also interested in a stepwise trend indicator. It looks at the hourly bar and the current zig
 *    zag rotation ... an hourly breakout and on the 3min ltf chart the level drawn is on the prior hourly bar high"; 22:40 - 23:20 the
 *    mockup: "those hourly highs should be coming from the top of the waves", "they should be connected steps", "get rid of the labels",
 *    "connect the tops without going across", "once it steps up, the prior high becomes support, so you can extend it as a subtle dashed
 *    line, not too thick"; 23:53 "build" - Step Trend indicator, clock hours).
 *    2026-11-11 00:08 "when the rotation is up and the hourly bar closes are above prior bar highs the lines are green, and if its vice
 *    versa ... red"; 00:11 "the bar closing higher needs to be green and vice versa" (the breakout hour is itself a green / red candle).
 *    On the price chart (any period that divides the hour; his is 3 minutes): the hourly breakout levels as connected steps over the wave
 *    tops - green up, red down; the newest level dashed to the right edge; an old level runs on thin and dashed as support / resistance
 *    until it is tested or the next step is made (1.1.0). A dot marks the candle that made each level. No text, no hovers, no settings.
 *    Every rule is in StepTrendLogic.h (stp::Engine, tested against the mockup's own walk on his ES / NQ / GC / CL days); the per-market
 *    rule values are StepTrendParams.h. Steps are decided when the hour closes and never change. One short status file per chart
 *    (lsFlexLevels\StepTrend\<MKT>-<seconds>.status.txt, on a change or once a minute) so the build can be checked without a screenshot.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>     // BEFORE irtsdk.h
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "StepTrendLogic.h"
#include "StepTrendParams.h"
#include "MarketKey.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define STEPTREND_VERSION "1.4.5"

namespace sth {
static const COLOR C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_GREEN_D = 0x00178A42, C_RED_D = 0x00B03A3A;
inline std::string flexDir() { const char* u = std::getenv("USERPROFILE"); return u && *u ? std::string(u) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;                                                     // the tests make their own folders
#endif
}
inline long long& testWall() { static long long w = 0; return w; }
inline long long wallNow() { return testWall() > 0 ? testWall() : (long long)std::time(nullptr); }
inline short coord(long long v) { return (short)std::max(-30000LL, std::min(30000LL, v)); }
inline std::string hm(long long t) { long long s = ((t % 86400) + 86400) % 86400; int h = (int)(s / 3600), m = (int)(s % 3600 / 60); const char* a = h >= 12 ? "PM" : "AM"; h = h % 12; if (!h) h = 12; char b[24]; std::snprintf(b, sizeof b, "%d:%02d %s", h, m, a); return b; }
} // namespace sth

struct STState {
    std::string mkt, root, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    stp::Engine E; std::vector<RTDATE> raw; int off = 0; int lastIdx = -1;      // raw[i] = the chart time of engine candle i; off = its chart index
    long long resets = 0, badBars = 0, statusAt = 0; size_t statusSteps = (size_t)-1, statusSkipped = (size_t)-1; int statusFails = 0; std::string note;
};

class StepTrend : public cppExtension {
public:
    SS_SAFE_DIALOG(StepTrend, "StepTrend")
    StepTrend() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<STState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(STState& S);
    void feed(STState& S);
    void status(STState& S);
    long long localSec(RTDATE d);
    STState* testState() { return slot_.get(this, false); }
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<StepTrend*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<StepTrend*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { StepTrend* me = static_cast<StepTrend*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

long long StepTrend::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool StepTrend::identify(STState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string root = rs ? rs : "";
    for (char& c : root) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '@')) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    const double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) { S.note = "no tick size"; return false; }
    if (root.empty()) { S.note = "no symbol"; return false; }
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || root != S.root || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.root = root; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.mkt = mkey::norm(root); for (char& c : S.mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
        const stpp::Mkt& M = stpp::forMarket(S.mkt);
        S.E.reset(); S.E.rule = M.rule; S.E.rot = M.rot != 0; S.E.body = M.body != 0; S.E.hourly = spb == 3600;   // (1.3.0) the 60-minute chart S.raw.clear(); S.off = 0; S.lastIdx = -1; S.statusSteps = (size_t)-1;
    }
    if (!stp::periodOk(S.spb)) { S.note = "the chart's period must divide the hour (1, 2, 3, 5, 10, 15, 30 or 60 minutes)"; return false; }
    S.note.clear();
    return true;
}

// closed candles in order, each once; a changed history = replay from the start. The forming candle only tells the engine the time.
void StepTrend::feed(STState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    bool again = false;
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) again = true; }
    if (fed > stp::MAX_BARS + stp::SLACK) again = true;
    if (again) { S.E.reset(); S.raw.clear(); ++S.resets; }
    if (S.raw.empty()) S.off = std::max(0, closed - stp::MAX_BARS);
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        stp::Bar b; b.t = localSec((RTDATE)times[i]) - S.spb;                  // IRT stamps a candle with its END
        const double o = (double)op[i], h = (double)hi[i], l = (double)lo[i], c = (double)cl[i];
        bool ok = std::isfinite(o) && std::isfinite(h) && std::isfinite(l) && std::isfinite(c) && o > 0 && h > 0 && l > 0 && c > 0 && b.t > 0;
        if (ok) { b.h = std::llround(h / S.tick); b.l = std::llround(l / S.tick); b.c = std::llround(c / S.tick); if (b.h < b.l) std::swap(b.h, b.l); b.c = std::max(b.l, std::min(b.h, b.c)); b.o = std::max(b.l, std::min(b.h, std::llround(o / S.tick))); }
        if (!ok || !S.E.addBar(b)) {                                           // a candle that cannot be read / out of order keeps its place with the prior candle's close (no step can come from it)
            ++S.badBars; stp::Bar f; f.t = S.E.B.empty() ? std::max(1LL, b.t) : S.E.B.back().t + 1; f.o = f.h = f.l = f.c = S.E.B.empty() ? 0 : S.E.B.back().c;
            if (S.E.B.empty() && !ok) { S.raw.clear(); S.off = i + 1; continue; }
            S.E.addBar(f);
        }
        S.raw.push_back((RTDATE)times[i]);
    }
    if (!S.raw.empty()) S.E.sealTo(localSec((RTDATE)times[closed]) - S.spb);
    S.lastIdx = closed;
}

void StepTrend::status(STState& S) {
    const long long now = sth::wallNow();
    const bool changed = S.E.S.size() != S.statusSteps || S.E.skipped.size() != S.statusSkipped;
    if (!changed && now - S.statusAt < 60) return;
    if (now - S.statusAt < 2) return;
    S.statusAt = now; S.statusSteps = S.E.S.size(); S.statusSkipped = S.E.skipped.size();
    const std::string dir = sth::flexDir(); if (dir.empty() || S.mkt.empty()) return;
    try {
        sth::makeDir(dir + "\\StepTrend");
        const std::string f = dir + "\\StepTrend\\" + S.mkt + "-" + std::to_string(S.spb) + ".status.txt", tmp = f + ".tmp";
        std::ostringstream o;
        o << "STEPTREND,version " << STEPTREND_VERSION << ",market " << S.mkt << ",root " << S.root << ",symbol " << S.sym << ",seconds " << S.spb << ",tick " << S.tick
          << "\nRULE," << (S.E.rule == 0 ? "hour closes beyond the prior hour's high / low" : "higher high and higher close") << ",breakout hour " << (S.E.body ? "green for up / red for down" : "any colour") << ",rotation " << (S.E.rot ? "required" : "off")
          << "\nDATA,candles " << S.E.B.size() << ",hours " << S.E.H.size() << ",steps " << S.E.S.size() << ",not counted (against the rotation) " << S.E.skipped.size()
          << ",re-feeds " << S.resets << ",unreadable candles " << S.badBars << (S.note.empty() ? std::string() : ",note " + S.note);
        const size_t n = S.E.S.size();
        for (size_t i = n > 8 ? n - 8 : 0; i < n; ++i) { const stp::Step& q = S.E.S[i]; char b[160];
            std::snprintf(b, sizeof b, "\nSTEP,%s,level %.6f,from %s,hour %s,confirmed %s,support %s", q.dir > 0 ? "UP" : "DOWN", (double)q.px * S.tick, sth::hm(q.ot).c_str(), sth::hm(q.hour).c_str(), sth::hm(q.tConf).c_str(),
                          q.sup == 0 ? "-" : q.sup == 1 ? "running" : "ended");
            o << b; }
        o << "\n";
        { std::ofstream w(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!w.is_open()) { ++S.statusFails; return; } w << o.str(); w.flush(); if (!w.good()) { ++S.statusFails; return; } }
        std::remove(f.c_str());
        if (std::rename(tmp.c_str(), f.c_str()) != 0) ++S.statusFails;
    } catch (...) { ++S.statusFails; }
}

void StepTrend::pump() {
    STState* S = slot_.get(this, true); if (!S) return;
    if (identify(*S)) feed(*S);
    status(*S);
}

int StepTrend::draw(void) {
    STState* S = slot_.get(this, false);
    if (!S || S->E.S.empty() || S->lastIdx < 0) return RTX_OK;   // draw never reads candles
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        vs = std::max(vs, 0); ve = std::min(ve, (int)n - 1);
        if (ve < vs) return RTX_OK;
        std::vector<stp::Seg> segs; std::vector<stp::Dot> dots; std::vector<stp::Mark> marks;
        if (S->E.hourly) stp::layoutHourly(S->E, S->lastIdx - S->off, segs, dots, marks); else stp::layout(S->E, S->lastIdx - S->off, segs, dots);
        const double tk = S->tick;
        for (size_t k = 0; k < segs.size(); ++k) {
            const stp::Seg& g = segs[k];
            int a = g.i0 + S->off, b = g.i1 + S->off; const bool toEdge = (g.kind == 1) || (g.kind == 2 && b >= S->lastIdx);
            if (b < vs || a > ve) continue;
            if (g.p0 != g.p1) {                                                 // the riser at a wave top
                PNT p0; if (p0.set(a, (float)(g.p0 * tk), kBarCenter) != RTX_OK) continue;
                PNT p1; if (p1.set(a, (float)(g.p1 * tk), kBarCenter) != RTX_OK) continue;
                setPen(g.dir > 0 ? sth::C_GREEN : sth::C_RED, 3, P_SOLID); p0.setDrawPosition(); p1.drawLineTo(); continue;
            }
            const bool cutL = a < vs, cutR = b > ve; if (cutL) a = vs; if (cutR) b = ve;      // a flat line is clipped to the candles on screen
            PNT p0; if (p0.set(a, (float)(g.p0 * tk), cutL ? kBarLeft : kBarCenter) != RTX_OK) continue;
            PNT p1; if (p1.set(b, (float)(g.p1 * tk), (cutR || toEdge) ? kBarRight : kBarCenter) != RTX_OK) continue;
            if (g.kind == 0) { setPen(g.dir > 0 ? sth::C_GREEN : sth::C_RED, 3, P_SOLID); p0.setDrawPosition(); p1.drawLineTo(); }
            else if (g.kind == 2) { setPen(g.dir > 0 ? sth::C_GREEN_D : sth::C_RED_D, 1, P_DASH); p0.setDrawPosition(); p1.drawLineTo(); }   // thin: a 1-pixel pen is the one width Windows always dashes
            else {                                                              // the live level: thick, dashed by hand (a wide pen is drawn solid)
                setPen(g.dir > 0 ? sth::C_GREEN : sth::C_RED, 3, P_SOLID);
                const int x0 = p0.h, x1 = p1.h, y = p0.v; int drawn = 0;
                for (int x = x0; x < x1 && drawn < 600; x += 10, ++drawn) { PNT q0 = p0; q0.h = sth::coord(x); q0.v = sth::coord(y); PNT q1 = p0; q1.h = sth::coord(std::min(x + 6, x1)); q1.v = sth::coord(y); q0.setDrawPosition(); q1.drawLineTo(); }
            }
        }
        for (size_t k = 0; k < dots.size(); ++k) {
            const int i = dots[k].i + S->off; if (i < vs || i > ve) continue;
            PNT c; if (c.set(i, (float)(dots[k].px * tk), kBarCenter) != RTX_OK) continue;
            const COLOR col = dots[k].dir > 0 ? sth::C_GREEN : sth::C_RED;
            setPen(col, 1, P_SOLID); CBRUSH br(col, PAT_SOLID); br.set();
            RCT o; o.set(sth::coord(c.h - 4), sth::coord(c.v - 4), sth::coord(c.h + 4), sth::coord(c.v + 4)); o.drawOval(DRAW_OPAQUE);
        }
        if (!marks.empty()) {   // (1.3.0) "T" = the level tested: under the low of the testing candle (step up, green) / over its high (step down, red)
            FONT ft; ft.id = HELVETICA; ft.size = 10; ft.style = BOLD; setFont(ft); const int tw = (int)getTextWidth("T", -1);
            for (size_t k = 0; k < marks.size(); ++k) { const int j = marks[k].i; if (j < 0 || j >= (int)S->E.B.size()) continue; const int i = j + S->off; if (i < vs || i > ve) continue;
                const stp::Bar& b = S->E.B[(size_t)j]; const bool brk = marks[k].kind == 1; const bool stepUp = marks[k].dir > 0; const bool up = brk ? !stepUp : stepUp;   // (1.4.4) up = drawn under the low. T: under the low for a step up. B: on the OTHER side (over the high of the red candle that broke a step up), clear of the lines
                const char* tx = brk ? "B" : "T";
                PNT c; if (c.set(i, (float)((up ? b.l : b.h) * tk), kBarCenter) != RTX_OK) continue;
                const int top = up ? c.v + 8 : c.v - 8 - 14;   // (1.4.2) clear of the step dot when the marked candle is itself a step candle (was 3 px)
                setFont(ft); setTextColor((stepUp != brk) ? sth::C_GREEN : sth::C_RED);
                RCT r; r.set(sth::coord(c.h - tw / 2), sth::coord(top), sth::coord(c.h + tw / 2 + 2), sth::coord(top + 14)); r.drawText(tx, false, false); }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    StepTrend* p = new StepTrend();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);   // on the price chart; no mouse tracking (no hovers)
    p->setDescription("LRA Step Trend v1.4.5 - hourly breakouts as connected steps over the wave tops (green up, red down); the newest level dashed; an old level runs on thin and dashed as support until it is tested or the next step is made.");
    p->setVersion("1.4.5");
    return p;
}
