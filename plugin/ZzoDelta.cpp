/********************************************************************************
 *  ZzoDelta.cpp  --  Investor/RT RTX extension  lsZzoDelta  "ZZO Delta"  1.2.0  (2026-11-02)  -  also built as lsZzoVolume "ZZO Volume" 1.0.0 (ZzoVolume.cpp = ZZO_VOLUME_VIEW + this file)
 *
 *  1.2.0 / ZZO Volume 1.0.0 (Rassul 22:40 CT "lets build, use delta as default and keep the Ab tag and add the cmx and cob tags for volume"):
 *    ZZO Delta unchanged (Ab?); ZZO Volume = each wave's volume vs the median of the last 5 same-way waves (Weis Wave), green up /
 *    red down waves, tags Ab? / Cmx / CoB (ZzoDeltaLogic.h). Two indicators instead of a dialog setting (no settings dialogs).
 *
 *  1.1.1 (Rassul 21:37-21:48 CT): every wave's bars point UP from one bottom line (easier to compare); colour = buying / selling.
 *
 *  1.1.0 (Rassul 21:33 CT "yes, fix it"): the standard delta sign (+ green buying / - red selling) and Ab? absorption marks (rules A / B,
 *    ZzoDeltaLogic.h); the ZigZag itself is drawn on the price chart by lsZzoZigZag (ZzoZigZag.cpp, same walk).
 *
 *  1.0.0 (Rassul 2026-11-02 19:40-20:05 CT, mockup "ZZO Delta Mockup" option 1 + "ok lets do option 2"): its own pane; a 1-bar
 *    High / Low ZigZag; each bar = the wave's running delta (VAP buy - sell) / the median |delta| of the last 5 waves in the same
 *    direction (same part of the day; the history restarts at the RTH open, needs 3); green up waves, red down waves; the ratio
 *    with the wave at each wave's end ("2.6x"). Rules: ZzoDeltaLogic.h (SDK-free, tested: tests_zzodelta). No settings, no hovers.
 *  FILES  lsFlexLevels\ZzoDelta.status-<MKT>-<spb>.txt  version, rules, counts, the last waves (on change / 30 s)
 *  Per chart state in the host slot; each closed bar read once (VAP in calc only, never in draw); the forming bar re-read each calc.
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
#include "SettingsGuard.h"   // SS_SAFE_DIALOG: the chart-less Add / settings copy never faults
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#ifndef ZZO_VOLUME_VIEW
#define ZZO_VOLUME_VIEW 0   // 1 = the ZZO Volume indicator (ZzoVolume.cpp)
#endif
#if ZZO_VOLUME_VIEW
#define ZD_NAME "ZzoVolume"
#else
#define ZD_NAME "ZzoDelta"
#endif
#if defined(_WIN32)
#define ZD_SEP "\\"
#else
#define ZD_SEP "/"
#endif

static long long zdMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string zdFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + ZD_SEP "InvestorRT" ZD_SEP "rtx" ZD_SEP "lsFlexLevels" : std::string(); }
static void zdMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool zdPublish(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    std::remove(path.c_str()); return std::rename(tmp.c_str(), path.c_str()) == 0;
#endif
}

struct ZState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    zzd::Engine E; std::vector<RTDATE> raw; int off = 0;
    bool vapFault = false;
    bool haveForm = false; zzd::Bar formBar;
    std::string statusPath; long long statusMs = -1; std::string statusBody;
    zzd::Scene last;
};

class ZzoDelta : public cppExtension {
public:
    SS_SAFE_DIALOG(ZzoDelta, "ZzoDelta")
    ZzoDelta() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<ZState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(ZState& S);
    void feed(ZState& S);
    int barDelta(ZState& S, int chartBar, long long& d);
    long long localSec(RTDATE d);
    void status(ZState& S, long long now);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<ZzoDelta*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZzoDelta*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { ZzoDelta* me = static_cast<ZzoDelta*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }   // bars + VAP read here only

// SEH-guarded VAP reads (as lsCandleAbsorb / lsWavePOC)
int ZzoDelta::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
int ZzoDelta::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !rws || bar < 0 || count <= 0) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rws[i], 0, sizeof(VOLPROFILE)); if (vap->getVolumeProfile(bar, i, rws[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's delta (sum of buy - sell over its VAP rows): 1 = read, 0 = no VAP (delta 0, counted), -1 = fault (never read again)
int ZzoDelta::barDelta(ZState& S, int chartBar, long long& d) {
    d = 0;
    if (S.vapFault) return -1;
    RTARRAYP vap(barVolumeProfile);
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) { S.vapFault = true; return -1; }
    if (prices <= 0 || prices > 100000) return 0;
    std::vector<VOLPROFILE> rv((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rv[0], prices);
    if (got == -1) { S.vapFault = true; return -1; }
    if (got < 0) return 0;
    bool any = false;
    for (const VOLPROFILE& v : rv) {
        if (!std::isfinite((double)v.price) || v.buyVolume < 0 || v.sellVolume < 0) continue;
        if (v.buyVolume + v.sellVolume <= 0) continue;
        d += (long long)v.buyVolume - (long long)v.sellVolume; any = true;
    }
    return any ? 1 : 0;
}

long long ZzoDelta::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool ZzoDelta::identify(ZState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;   // SYM_TICKINCR only
    if (mkt.empty()) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.E.setMarket(mkt, spb); S.raw.clear(); S.off = 0; S.haveForm = false;
        const std::string d = zdFlex(); const std::string key = mkt + "-" + std::to_string(spb);
        S.statusPath = d.empty() ? std::string() : d + ZD_SEP ZD_NAME ".status-" + key + ".txt";
    }
    return true;
}

// closed bars in order, each once; a changed history (first stamp, fewer bars, a stamp moved) = replay from the start
void ZzoDelta::feed(ZState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime), vol(barVolume); RTARRAY hi(barHigh), lo(barLow);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.E.setMarket(S.mkt, S.spb); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - zzd::MAX_BARS + zzd::TRIM);
    const bool haveVol = (long)vol.count >= n;
    auto bar = [&](int i, zzd::Bar& b) {
        b.t = localSec((RTDATE)times[i]);
        b.vol = haveVol ? std::max(0LL, (long long)vol[i]) : 0;   // (1.2.0) bar volume
        if (!zzd::toTicks(hi[i], S.tick, b.h) || !zzd::toTicks(lo[i], S.tick, b.l)) { b.h = b.l = S.E.B.empty() ? 0 : S.E.B.back().h; }
        if (b.h < b.l) std::swap(b.h, b.l);
        long long d = 0; b.vap = barDelta(S, i, d) > 0; b.dlt = d;
    };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        zzd::Bar b; bar(i, b);
        S.E.addBar(b); S.raw.push_back((RTDATE)times[i]);
        if (S.E.trimmed > 0) { S.off += (int)S.E.trimmed; S.raw.erase(S.raw.begin(), S.raw.begin() + std::min<long long>(S.E.trimmed, (long long)S.raw.size())); S.E.trimmed = 0; }
    }
    zzd::Bar fb; bar(closed, fb); fb.seg = S.E.segOf(fb.t); S.formBar = fb; S.haveForm = true;   // the forming bar, re-read every calc
}

void ZzoDelta::status(ZState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = zzd::statusText(S.E, S.mkt, S.spb, S.vapFault ? "FAULT - remove and re-add the indicator" : "ok") +
        "VIEW," ZD_NAME "\nDRAWN,bars " + std::to_string(S.last.bars) + ",labels " + std::to_string(S.last.labels) + ",Ab? " + std::to_string(S.last.marks) + ",Cmx " + std::to_string(S.last.cmx) + ",AR " + std::to_string(S.last.cob) + ",ST " + std::to_string(S.last.st) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    zdMkdir(zdFlex()); zdPublish(S.statusPath, body);
}

void ZzoDelta::pump() {
    ZState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, zdMs());
}

static short zdC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int ZzoDelta::draw(void) {
    ZState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) return RTX_OK;   // draw never reads bars / VAP
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        zzd::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom;
        v.first = vs - S->off; v.last = ve - S->off; v.bw = std::max(1, (int)getPixelsPerBar());
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        v.textH = 12;
        const std::vector<zzd::Wave> waves = S->E.view(S->haveForm ? &S->formBar : nullptr);
        v.volume = ZZO_VOLUME_VIEW != 0;
        const std::string head = v.volume ? std::string("ZZO Volume ") + zzd::VOL_VERSION + "  wave volume vs median of last 5"
                                          : std::string("ZZO Delta ") + zzd::VERSION + "  + buying / - selling vs median of last 5";
        S->last = zzd::layout(S->E, waves, S->haveForm ? S->formBar.dlt : 0, v, head, S->haveForm ? S->formBar.vol : 0);
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const zzd::Prim& p : S->last.p) {
            if (p.k == zzd::Prim::FILL) {
                const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
                if (x1 < x0 || y1 < y0) continue;
                RCT r; r.set(zdC(x0), zdC(y0), zdC(x1 + 1), zdC(y1 + 1)); r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) continue;
                setFont(fb); setTextColor((COLOR)p.col);
                RCT r; r.set(zdC(p.x0), zdC(p.y0), zdC(p.x1 + 2), zdC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    ZzoDelta* p = new ZzoDelta();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);   // its own pane, draws itself; no mouse tracking (no hovers)
#if ZZO_VOLUME_VIEW
    p->setDescription("LRA ZZO Volume v1.1.0 - 1-bar High/Low ZigZag waves: wave volume / the median of the last 5 same-way waves (Weis Wave); Ab? absorption, Cmx climax, AR automatic rally, ST secondary test.");
    p->setVersion(ZZO_VOLUME_VERSION_LITERAL);
#else
    p->setDescription("LRA ZZO Delta v1.2.0 - 1-bar High/Low ZigZag waves: wave delta (+ green buying / - red selling) / the median of the last 5 same-way waves (RTH restarts); Ab? = absorption.");
    p->setVersion("1.2.0");   // = zzd::VERSION (gex-build reads this literal)
#endif
    return p;
}
