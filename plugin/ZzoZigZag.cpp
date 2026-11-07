/********************************************************************************
 *  ZzoZigZag.cpp  --  Investor/RT RTX extension  lsZzoZigZag  "ZZO ZigZag"  1.1.0  (2026-11-03)
 *
 *  1.1.0 (Rassul 2026-11-03 13:59 CT "make it 3bar using high and low by default" for ZZO Combo): this overlay draws the waves ZZO Combo
 *    measures, so it follows Combo to the 3-bar High/Low ZigZag (zzd::COMBO_REV_BARS = IRT's ZIGZAG Min Price Change 3 bars, as
 *    lsWavePOC); ZZO Delta / ZZO Volume keep their 1-bar walk.
 *  1.0.0 (Rassul 2026-11-02 21:18 CT "add a zig zag also" + 21:33 "yes, fix it"): on the price chart, the 1-bar High / Low ZigZag
 *    that ZZO Delta measures its waves on (ZzoDeltaLogic.h, the same walk) - a white line pivot to pivot; the live leg dashed (it
 *    moves with the forming bar until the 1-bar reversal confirms it). No settings, no hovers, no files.
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
#include "ZzoDeltaLogic.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

struct ZZState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    zzd::Engine E; std::vector<RTDATE> raw; int off = 0;
    bool haveForm = false; zzd::Bar formBar;
};

class ZzoZigZag : public cppExtension {
public:
    SS_SAFE_DIALOG(ZzoZigZag, "ZzoZigZag")
    ZzoZigZag() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<ZZState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(ZZState& S);
    void feed(ZZState& S);
    long long localSec(RTDATE d);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<ZzoZigZag*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZzoZigZag*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { ZzoZigZag* me = static_cast<ZzoZigZag*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

long long ZzoZigZag::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool ZzoZigZag::identify(ZZState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;
    if (mkt.empty()) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.E.revBars = zzd::COMBO_REV_BARS; S.E.setMarket(mkt, spb); S.raw.clear(); S.off = 0; S.haveForm = false;   // (1.1.0) Combo's 3-bar walk
    }
    return true;
}

// closed bars in order, each once; a changed history = replay from the start (no VAP needed: prices only)
void ZzoZigZag::feed(ZZState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.E.setMarket(S.mkt, S.spb); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - zzd::MAX_BARS + zzd::TRIM);
    auto bar = [&](int i, zzd::Bar& b) {
        b.t = localSec((RTDATE)times[i]); b.vap = true; b.dlt = 0;
        if (!zzd::toTicks(hi[i], S.tick, b.h) || !zzd::toTicks(lo[i], S.tick, b.l)) { b.h = b.l = S.E.B.empty() ? 0 : S.E.B.back().h; }
        if (b.h < b.l) std::swap(b.h, b.l);
    };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        zzd::Bar b; bar(i, b);
        S.E.addBar(b); S.raw.push_back((RTDATE)times[i]);
        if (S.E.trimmed > 0) { S.off += (int)S.E.trimmed; S.raw.erase(S.raw.begin(), S.raw.begin() + std::min<long long>(S.E.trimmed, (long long)S.raw.size())); S.E.trimmed = 0; }
    }
    zzd::Bar fb; bar(closed, fb); fb.seg = S.E.segOf(fb.t); S.formBar = fb; S.haveForm = true;
}

void ZzoZigZag::pump() {
    ZZState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S);
}

int ZzoZigZag::draw(void) {
    ZZState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) return RTX_OK;   // draw never reads bars
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        const std::vector<zzd::ZPt> pts = zzd::zigzag(S->E, S->haveForm ? &S->formBar : nullptr);
        for (size_t k = 1; k < pts.size(); ++k) {
            const zzd::ZPt& a = pts[k - 1]; const zzd::ZPt& b = pts[k];
            const int ia = a.i + S->off, ib = b.i + S->off;
            if (ib < vs - 1 || ia > ve + 1) continue;
            setPen((COLOR)0xE5E7EB, 2, b.live ? P_DASH : P_SOLID);
            PNT p0; p0.set(ia, (float)(a.px * S->tick), kBarCenter); p0.setDrawPosition();
            PNT p1; p1.set(ib, (float)(b.px * S->tick), kBarCenter); p1.drawLineTo();
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    ZzoZigZag* p = new ZzoZigZag();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);   // on the price chart; no mouse tracking (no hovers)
    p->setDescription("LRA ZZO ZigZag v1.2.0 - the 3-bar High/Low ZigZag ZZO Combo measures its waves on (white; the live leg dashed, drawn from the last pivot to the newest extreme at once).");
    p->setVersion("1.2.1");   // = zzd::ZIGZAG_VERSION (gex-build reads this literal)
    return p;
}
