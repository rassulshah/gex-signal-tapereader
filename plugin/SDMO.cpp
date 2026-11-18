/********************************************************************************
 *  SDMO.cpp  --  Investor/RT RTX extension  lsSDMO  "SDMO"  0.1.0  (2026-11-18)
 *
 *  Rassul 2026-11-18 11:20 CT "create an indicator called sdmomo, it is made of two indicators that are on my chart sd and sdma ... create its
 *  read too"; 11:29 "use a green line and red for the avg"; 12:40 "just call it SDMO"; 12:51 "letsgo with yours.. build" (8 / 14).
 *  Its own pane: SD (green) = MA 8 of the 20-candle sum of up-close ranges minus down-close ranges, and its average (red) = MA 14 of SD, with a
 *  zero line, and a 3-line read right of the last candle (took over / fading at the low or high / even / building / fading). Rules, read and
 *  layout = SdmoLogic.h. Computed natively from the chart's own bars; closed candles only, never repainted. No settings (NO_UI), no hovers.
 *  FILE   lsFlexLevels\SDMO.status-<MKT>-<spb>.txt   version, rules, the last values and the read
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
#include "SdmoLogic.h"
#include "MarketKey.h"
#include "HostSlot.h"
#ifndef LSPERF_PLUGIN
  #define LSPERF_PLUGIN "SDMO"
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
#define SDM_SEP "\\"
#else
#define SDM_SEP "/"
#endif

static const char* SDM_VERSION = "0.1.0";   // = sdmo::VERSION = the setVersion literal

static long long sdmMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string sdmFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + SDM_SEP "InvestorRT" SDM_SEP "rtx" SDM_SEP "lsFlexLevels" : std::string(); }
static bool sdmPublish(const std::string& path, const std::string& text) {
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

struct SDMState {
    std::string mkt, sym; int spb = 0; bool keyed = false;
    sdmo::Engine E; std::vector<RTDATE> raw; int off = 0;
    std::string statusPath, statusBody; long long statusCk = -1; int drawn = 0;
};

class SDMO : public cppExtension {
public:
    SS_SAFE_DIALOG(SDMO, "SDMO")
    SDMO() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<SDMState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(SDMState& S);
    void feed(SDMState& S);
    void status(SDMState& S, long long now);
    long long localSec(RTDATE d);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<SDMO*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<SDMO*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); SDMO* me = static_cast<SDMO*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

long long SDMO::localSec(RTDATE d) {                 // the chart's wall clock in seconds (only the time of day is used, for the read)
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t)) return 0;
    return 86400LL + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;   // + a day so 0:00 is not "no time"
}

bool SDMO::identify(SDMState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    if (mkt.empty() || spb <= 0) return false;
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.keyed = true; S.E.reset(); S.raw.clear(); S.off = 0;
        const std::string d = sdmFlex();
        S.statusPath = d.empty() ? std::string() : d + SDM_SEP "SDMO.status-" + mkey::norm(mkt) + "-" + std::to_string(spb) + ".txt";
    }
    return true;
}

// closed candles in order, each once; a changed history (first stamp moved, fewer bars) = replay from the start
void SDMO::feed(SDMState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - (int)sdmo::MAX_BARS);
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        sdmo::Bar b; b.t = (long long)times[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.local = localSec((RTDATE)times[i]);
        S.E.add(b); S.raw.push_back((RTDATE)times[i]);
    }
    if (S.E.dropped > 0) {                                               // the engine trimmed its oldest bars: keep raw / off in step
        const size_t keep = S.E.B.size();
        if (S.raw.size() > keep) { const size_t cut = S.raw.size() - keep; S.raw.erase(S.raw.begin(), S.raw.begin() + (long)cut); S.off += (int)cut; }
        S.E.dropped = 0;
    }
}

void SDMO::status(SDMState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusCk >= 0 && now - S.statusCk < 1000) return;              // built at most once a second
    S.statusCk = now;
    const std::string body = sdmo::statusText(S.E, mkey::norm(S.mkt), S.sym, S.spb) + "HOST," + SDM_VERSION + "\nDRAWN," + std::to_string(S.drawn) + "\n";
    if (body == S.statusBody) return;                                     // unchanged: no write
    S.statusBody = body;
    sdmPublish(S.statusPath, body);
}

void SDMO::pump() {
    SDMState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, sdmMs());
}

static short sdmC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int SDMO::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    SDMState* S = slot_.get(this, false);
    if (!S || !S->keyed) return RTX_OK;                                   // draw never reads bars
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        sdmo::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.first = vs - S->off; v.last = ve - S->off; v.textH = 11;
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s, bool) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        const int lastClosed = (int)n - 2;
        v.readX = (ve >= (int)n - 1) ? (int)v.X(lastClosed + 1 - off) + std::max(1, (int)getPixelsPerBar()) + 18 : -1;
        const std::vector<sdmo::Prim> P = sdmo::layout(S->E, v);
        S->drawn = (int)P.size();
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const sdmo::Prim& p : P) {
            if (p.kind == 0) {
                const int x0 = std::max(L, std::min(R, p.x0)), x1 = std::max(L, std::min(R, p.x1)), y0 = std::max(T, std::min(Bt, p.y0)), y1 = std::max(T, std::min(Bt, p.y1));
                setPen((COLOR)p.col, (short)p.width, P_SOLID);
                PNT q0; q0.set(0, 0.0f); q0.h = sdmC(x0); q0.v = sdmC(y0); q0.setDrawPosition();
                PNT q1; q1.set(0, 0.0f); q1.h = sdmC(x1); q1.v = sdmC(y1); q1.drawLineTo();
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x0 > R) continue;
                setFont(fb); setTextColor((COLOR)p.col);
                RCT r; r.set(sdmC(p.x0), sdmC(p.y0), sdmC(std::min(R, p.x1 + 2)), sdmC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    SDMO* p = new SDMO();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI);                                    // its own pane, draws itself; no hovers, no settings
    p->setDescription("LRA SDMO v0.1.0 - SD momentum in its own pane: SD (green) = 8-bar average of the 20-candle sum of up-close ranges minus down-close ranges; its average (red) = 14-bar average of SD; a zero line; and a 3-line read right of the last candle: took over (SD crossed its average and pulled away), fading at the low / high (a new 20-candle extreme with SD better than at the earlier extreme), even (SD hugging its average), building or fading (the gap widening or closing, as a ratio of its normal size). Closed candles only, never repainted. No settings.");
    p->setVersion("0.1.0");   // = SDM_VERSION (gex-build reads this literal)
    return p;
}
