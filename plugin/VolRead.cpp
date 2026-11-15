/********************************************************************************
 *  VolRead.cpp  --  Investor/RT RTX extension  lsVolRead "Volume Read" / lsRangeRead "Range Read" (RangeRead.cpp = this file + VR_RANGE)  1.0.0  (2026-11-15)
 *
 *  Rassul 2026-11-14 21:58-22:05 CT ("a little reader for the volume and range ... BVG, BVR, BV ... and BVMax", "put the max stat on top
 *  then Bv then BV green annd then BVR"), 2026-11-15 16:07 / 17:06 CT ("i also want to implement the new volume and range indicators that
 *  you are showing in the mockup", "go build"): its own pane - the chart's volume (or range) bars, green / red, the Max count over every
 *  closed bar with Max 20+, and a read box at the left (Max, BV, BVG, BVR - Range: BRMax, BR, BRG, BRR) for the last closed bar.
 *  Rules = VolRangeLogic.h. Computed natively from the chart's own bars; closed bars decide, never repainted. No settings (NO_UI).
 *  FILE   lsFlexLevels\VolRead.status-<MKT>-<spb>.txt / RangeRead.status-<MKT>-<spb>.txt   version, rules, the last bars' counts
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
#include "VolRangeLogic.h"
#include "MarketKey.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define VR_SEP "\\"
#else
#define VR_SEP "/"
#endif
#ifdef VR_RANGE
#define VR_NAME "RangeRead"
#define VR_MODE vrr::RANGE
#else
#define VR_NAME "VolRead"
#define VR_MODE vrr::VOL
#endif

static const char* VR_VERSION = "1.0.0";   // = vrr::VERSION = the setVersion literal
static const int VR_MAX_BARS = 40000;

static long long vrMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string vrFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + VR_SEP "InvestorRT" VR_SEP "rtx" VR_SEP "lsFlexLevels" : std::string(); }
static bool vrPublish(const std::string& path, const std::string& text) {
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

struct VRState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    vrr::Engine E; std::vector<RTDATE> raw; int off = 0;
    vrr::Bar form; bool haveForm = false;
    std::string statusPath, statusBody; long long statusMs = -1; int drawn = 0;
};

class VolRead : public cppExtension {
public:
    SS_SAFE_DIALOG(VolRead, VR_NAME)             // the chart-less Add / settings copy never calls into the chart
    VolRead() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<VRState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(VRState& S);
    void feed(VRState& S);
    void status(VRState& S, long long now);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<VolRead*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<VolRead*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { VolRead* me = static_cast<VolRead*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

bool VolRead::identify(VRState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000 || mkt.empty() || spb <= 0) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.E.mode = VR_MODE; S.E.tick = tick; S.raw.clear(); S.off = 0; S.haveForm = false;
        const std::string d = vrFlex();
        S.statusPath = d.empty() ? std::string() : d + VR_SEP VR_NAME ".status-" + mkey::norm(mkt) + "-" + std::to_string(spb) + ".txt";
    }
    return true;
}

// closed bars in order, each once; a changed history (first stamp moved, fewer bars) = replay from the start; the forming bar is kept apart
void VolRead::feed(VRState& S) {
    const long n = getBarCount(); if (n < 2) { S.haveForm = false; return; }
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.haveForm = false; return; }
    const bool haveVol = (long)vo.count >= n;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - VR_MAX_BARS);
    auto bar = [&](int i) { vrr::Bar b; b.t = (long long)times[i]; b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.v = haveVol ? (double)vo[i] : 0;
        if (!std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.o) || !std::isfinite(b.c)) { b.o = b.h = b.l = b.c = 0; b.v = 0; }
        if (b.h < b.l) std::swap(b.h, b.l);
        return b; };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) { S.E.addBar(bar(i)); S.raw.push_back((RTDATE)times[i]); }
    S.form = bar(closed); S.haveForm = true;
}

void VolRead::status(VRState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = vrr::statusText(S.E, mkey::norm(S.mkt), S.sym, S.spb) + "HOST," + VR_VERSION + "\nDRAWN," + std::to_string(S.drawn) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    if (body == S.statusBody) { S.statusMs = now; return; }                // unchanged: no write
    S.statusMs = now; S.statusBody = body;
    vrPublish(S.statusPath, body);
}

void VolRead::pump() {
    VRState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, vrMs());
}

static short vrC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int VolRead::draw(void) {
    VRState* S = slot_.get(this, false);
    if (!S || !S->keyed) return RTX_OK;                                   // draw never reads bars
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        vrr::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.first = vs - S->off; v.last = ve - S->off;
        v.bw = std::max(1, (int)getPixelsPerBar()); v.textH = 11;
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        FONT fp; fp.id = HELVETICA; fp.size = 9; fp.style = PLAIN; FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        const std::vector<vrr::Prim> P = vrr::layout(S->E, v, S->haveForm ? &S->form : nullptr);
        S->drawn = (int)P.size();
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const vrr::Prim& p : P) {
            if (p.fill) {
                const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
                if (x1 < x0 || y1 < y0) continue;
                RCT r; r.set(vrC(x0), vrC(y0), vrC(x1 + 1), vrC(y1 + 1)); r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) continue;
                setFont(p.bold ? fb : fp); setTextColor((COLOR)p.col);
                RCT r; r.set(vrC(p.x0), vrC(p.y0), vrC(p.x1 + 2), vrC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    VolRead* p = new VolRead();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI);                                    // its own pane, draws itself; no hovers, no settings
#ifdef VR_RANGE
    p->setDescription("LRA Range Read v1.0.0 - its own pane: each bar's range (high - low), green / red, with BRMax over every closed bar that is the biggest in 20 or more bars, and a read box for the last closed bar: BRMax, BR (bars in a row with a smaller range), BRG (green bars only), BRR (red bars only). Counts stop at 99+. Decided at the bar close, never repainted.");
#else
    p->setDescription("LRA Volume Read v1.0.0 - its own pane: each bar's volume, green / red, with BVMax over every closed bar that is the biggest in 20 or more bars, and a read box for the last closed bar: BVMax, BV (bars in a row with a smaller volume), BVG (green bars only), BVR (red bars only). Counts stop at 99+. Decided at the bar close, never repainted.");
#endif
    p->setVersion("1.0.0");   // = VR_VERSION (gex-build reads this literal)
    return p;
}
