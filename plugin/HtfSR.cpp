/********************************************************************************
 *  HtfSR.cpp  --  Investor/RT RTX extension  lsHtfSR  "3-Hour S / R"  1.0.2  (2026-11-22: R1 / R2 red, S1 / S2 green - Rassul 19:41 CT "R should be red and S should be green", 20:22 "A"; 1.0.1 2026-11-16: one line per level across the chart, no past steps; 1.0.0 2026-11-15)
 *
 *  Rassul 2026-11-15 20:33 CT "did you build any indicator for s1 s2 r1 and r2 for the hourly"; 20:55 "just build them".
 *  On the PRICE pane of any chart of 3 hours or less: the standing 3-hour R1 / R2 / S1 / S2 as lines (R red, S green; orange until 1.0.1) with the name at the
 *  right end - the same levels lsCobSignal sweeps and the Limit Profile lists (HtfSr.h: 3-hour bars from 5:00 PM CT built from the chart's
 *  own bars, his 1-bar High / Low ZigZag, R1 = the last pivot high, R2 = the higher of the last two, S1 / S2 the mirror; a level a
 *  completed 3-hour bar closed beyond is broken and no longer drawn). Each closed bar keeps the value it had at its close (never
 *  repainted); the forming bar's value is live. Computed natively from the chart's bars. No settings, no hovers, no text but the names.
 *  For charts WITHOUT lsCobSignal (which draws its own 3-hour lines): his 60-min / 180-min charts, or a 3-min chart he keeps clean.
 *  FILE   lsFlexLevels\HtfSR.status-<MKT>-<spb>.txt   version, rule, the levels now
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
#include "HtfSr.h"
#include "MarketKey.h"
#include "HostSlot.h"
#ifndef LSPERF_PLUGIN   // (2026-11-18 memory hunt) the IRT performance guard: time and memory of every calc / draw (Perf\perf- / permem-<plugin>)
  #define LSPERF_PLUGIN "HtfSR"
#endif
#include "PerfGuard.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define HS_SEP "\\"
#else
#define HS_SEP "/"
#endif
static const char* HS_VERSION = "1.0.2";   // = hsr::VERSION = the setVersion literal
static const int HS_MAX_BARS = 40000;

static long long hsMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string hsFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + HS_SEP "InvestorRT" HS_SEP "rtx" HS_SEP "lsFlexLevels" : std::string(); }
static bool hsPublish(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { std::remove(tmp.c_str()); return false; }
    return true;
#endif
}

struct HSState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    hsr::Walk W; std::vector<hsr::Snap> hs; std::vector<RTDATE> raw; int off = 0;   // hs[i] = the levels at closed bar off + i
    hsr::Snap form; bool haveForm = false;                                          // the forming bar's live value
    std::string statusPath, statusBody; long long statusMs = -1; int drawn = 0;
};

class HtfSR : public cppExtension {
public:
    SS_SAFE_DIALOG(HtfSR, "HtfSR")             // the chart-less Add / settings copy never calls into the chart
    HtfSR() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<HSState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(HSState& S);
    void feed(HSState& S);
    void status(HSState& S, long long now);
    long long localSec(RTDATE d);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<HtfSR*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<HtfSR*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); HtfSR* me = static_cast<HtfSR*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

long long HtfSR::localSec(RTDATE d) {              // the chart's local wall clock in seconds (as lsCobSignal)
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool HtfSR::identify(HSState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000 || mkt.empty() || spb <= 0) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.W.reset(); S.hs.clear(); S.raw.clear(); S.off = 0; S.haveForm = false;
        const std::string d = hsFlex();
        S.statusPath = d.empty() ? std::string() : d + HS_SEP "HtfSR.status-" + mkey::norm(mkt) + "-" + std::to_string(spb) + ".txt";
    }
    return true;
}

// closed bars in order, each once; a changed history (first stamp moved, fewer bars) = replay from the start. The forming bar is walked on
// a copy every calc (a few hundred 3-hour bars), never into the kept walk.
void HtfSR::feed(HSState& S) {
    S.haveForm = false;
    if (!hsr::periodOk(S.spb)) return;
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.W.reset(); S.hs.clear(); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - HS_MAX_BARS);
    auto bar = [&](int i) { hsr::Bar b; b.t = localSec((RTDATE)times[i]); b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; if (b.h < b.l) std::swap(b.h, b.l); return b; };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        const hsr::Bar b = bar(i); const hsr::Snap prev = S.hs.empty() ? hsr::Snap() : S.hs.back();
        S.hs.push_back(b.t > 0 ? S.W.add(b, S.spb) : prev);              // an unreadable stamp / price keeps the levels as they stood
        if (b.t > 0 && S.hs.back().dir == 0 && prev.dir != 0) S.hs.back() = prev;
        S.raw.push_back((RTDATE)times[i]);
    }
    { const hsr::Bar b = bar(closed); if (b.t > 0 && std::isfinite(b.h) && std::isfinite(b.l) && std::isfinite(b.c)) { hsr::Walk W2 = S.W; S.form = W2.add(b, S.spb); S.haveForm = S.form.dir != 0; } }
}

void HtfSR::status(HSState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const hsr::Snap last = S.haveForm ? S.form : S.hs.empty() ? hsr::Snap() : S.hs.back();
    const std::string body = std::string("lsHtfSR ") + HS_VERSION + "\nMARKET," + mkey::norm(S.mkt) + "," + S.sym + "," + std::to_string(S.spb) + "\n" +
        (hsr::periodOk(S.spb) ? hsr::statusText(last, S.tick, (int)S.W.HB.size(), S.spb) : std::string("HTFSR,off: the chart's bars do not divide 3 hours")) + "\nBARS," + std::to_string(S.hs.size()) + "\n";
    if (body == S.statusBody) { S.statusMs = now; return; }                // unchanged: no write
    if (S.statusMs >= 0 && now - S.statusMs < 30000 && !S.statusBody.empty() && S.statusBody.size() == body.size()) return;   // the live swing price alone: at most every 30 s
    S.statusMs = now; S.statusBody = body;
    hsPublish(S.statusPath, body);
}

void HtfSR::pump() {
    HSState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, hsMs());
}

static short hsC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int HtfSR::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    HSState* S = slot_.get(this, false);
    if (!S || !S->keyed || S->hs.empty()) return RTX_OK;                  // draw never reads bars
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        std::vector<hsr::Snap> hs; const int a = std::max(0, vs - S->off), b = std::min((int)S->hs.size() - 1 + (S->haveForm ? 1 : 0), ve - S->off);
        if (b < a) return RTX_OK;
        hs.reserve((size_t)(b - a + 1)); for (int i = a; i <= b; ++i) hs.push_back(i < (int)S->hs.size() ? S->hs[(size_t)i] : S->form);   // the visible bars only
        hsr::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.first = 0; v.last = (int)hs.size() - 1; v.textH = 12;
        const int base = a + S->off;
        v.X = [this, base](int j) { PNT p; p.set(j + base, 0.0f, kBarCenter); return (double)p.h; };
        v.Y = [this](double px) { return (double)getVerticalDrawPosition((float)px); };
        FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        const std::vector<hsr::Prim> P = hsr::layout(hs, S->tick, v);
        S->drawn = (int)P.size();
        for (const hsr::Prim& p : P) {
            if (p.line) { setPen((COLOR)p.col, 1, P_SOLID); PNT q0; q0.set(0, 0.0f); q0.h = hsC(p.x0); q0.v = hsC(p.y0); q0.setDrawPosition(); PNT q1; q1.set(0, 0.0f); q1.h = hsC(p.x1); q1.v = hsC(p.y0); q1.drawLineTo(); }
            else { setFont(fb); setTextColor((COLOR)p.col); RCT r; r.set(hsC(p.x0), hsC(p.y0), hsC(p.x1 + 2), hsC(p.y1)); r.drawText(p.text.c_str(), false, false); }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    HtfSR* p = new HtfSR();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI);       // on the price pane, on the instrument's scale; no hovers, no settings
    p->setDescription("LRA 3-Hour S / R v1.0.2 - on the price pane of a chart of 3 hours or less: the standing 3-hour R1 / R2 (red) / S1 / S2 (green) as lines across the chart, named at the right, at where they stand now (3-hour bars from 5:00 PM CT built from the chart's bars; 1-bar High / Low ZigZag; R1 = the last pivot high, R2 = the higher of the last two, S1 / S2 the mirror; a level a completed 3-hour bar closed beyond is dropped). The same levels lsCobSignal sweeps and the Limit Profile lists - use it on charts without lsCobSignal. No settings.");
    p->setVersion("1.0.2");   // = HS_VERSION (gex-build reads this literal)
    return p;
}
