/********************************************************************************
 *  ZzoCombo.cpp  --  Investor/RT RTX extension  lsZzoCombo  "ZZO Combo"  2.1.1  (2026-11-03)
 *
 *  2.1.1 (Rassul 2026-11-03 "build it all", ref picture gc_zz.png): the wave COUNT is drawn INSIDE the wave's bars - centred on the
 *    second-to-last bar, just below its top, white with a 1 px black outline; a bar too short for the text gets it just above its top.
 *    The ratio label is unchanged (centred above the last bar). View only (ZzoComboView.h); waves, counts, tags unchanged.
 *  2.1.0 (Rassul 2026-11-03 13:59-14:03 CT): waves from the 3-bar High/Low ZigZag by default ("make it 3bar using high and low by
 *    default" = IRT's ZIGZAG Min Price Change 3 bars, as lsWavePOC); the count compares the wave's volume ratio (the ramp height) with
 *    prior waves of its own colour only; "weak" not drawn ("remove weak"); the count one bar further left. No settings (SS_SAFE_DIALOG):
 *    1-bar vs 3-bar is the compiled constant zzd::COMBO_REV_BARS.
 *  2.0.1 (09:20 CT, Rassul's ES chart): labels / tags drawn after every column (the next wave covered the "weak" tag).
 *  2.0.0 (Rassul 2026-11-03 07:40-08:49 CT, "ZZO Combo Mockup" v19 + "build"): the default ZZO indicator - one row in its own pane:
 *    volume ramp per wave (running volume / the median of the last 5 same-way waves), the 2.0x+ ratio above the peak and the count
 *    ("biggest red in the last 12 red waves", 0 hidden, 50+ cap) right-justified in the top quarter, tags at the bottom centre of the wave
 *    (Cmx / AR / ST, CoB / weak after a key-level sweep, "Ab 4.9" = absorption with a heavy extreme + its effort / result - ES only until
 *    the nightly proves it elsewhere). Delta is computed (VAP) but not drawn: the tests showed only Ab with a heavy extreme beats an
 *    ordinary wave (ES 49% vs 33% reached 2R, both halves). The forming wave grows with every tick (darker, white top edge); tags only
 *    on completed waves. Fits any pane height (labels drop below 70 / 50 px). No settings, no hovers. Rules: ZzoComboLogic.h +
 *    ZzoComboView.h (tests_zzodelta/test_zzocombo.cpp). Thresholds per market: ZzoParams.h (nightly lra.zzo_score).
 *  FILES  lsFlexLevels\ZzoCombo.status-<MKT>-<spb>.txt  version, rules, the last 30 waves (on change / 30 s)
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
#include "ZzoComboView.h"
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

#define ZD_NAME "ZzoCombo"
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
    zzd::ComboScene last;
};

class ZzoCombo : public cppExtension {
public:
    SS_SAFE_DIALOG(ZzoCombo, "ZzoCombo")
    ZzoCombo() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<ZState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(ZState& S);
    void feed(ZState& S);
    int barDelta(ZState& S, int chartBar, long long& d, long long hT = 0, long long lT = 0, long long* xh = nullptr, long long* xl = nullptr);
    long long localSec(RTDATE d);
    void status(ZState& S, long long now);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<ZzoCombo*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<ZzoCombo*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { ZzoCombo* me = static_cast<ZzoCombo*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }   // bars + VAP read here only

// SEH-guarded VAP reads (as lsCandleAbsorb / lsWavePOC)
int ZzoCombo::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int ZzoCombo::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
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
int ZzoCombo::barDelta(ZState& S, int chartBar, long long& d, long long hT, long long lT, long long* xh, long long* xl) {
    d = 0; if (xh) *xh = 0; if (xl) *xl = 0;
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
        long long pt = 0;   // (2.0.0) the volume in the bar's top / bottom 2 ticks (Ab heavy extreme)
        if ((xh || xl) && zzd::toTicks((double)v.price, S.tick, pt)) { const long long vv = (long long)v.buyVolume + (long long)v.sellVolume;
            if (xh && (pt == hT || pt == hT - 1)) *xh += vv;
            if (xl && (pt == lT || pt == lT + 1)) *xl += vv; }
    }
    return any ? 1 : 0;
}

long long ZzoCombo::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool ZzoCombo::identify(ZState& S) {
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
        S.E.reset(); S.E.revBars = zzd::COMBO_REV_BARS; S.E.setMarket(mkt, spb); S.raw.clear(); S.off = 0; S.haveForm = false;   // (2.1.0) 3-bar ZigZag
        const std::string d = zdFlex(); const std::string key = mkt + "-" + std::to_string(spb);
        S.statusPath = d.empty() ? std::string() : d + ZD_SEP ZD_NAME ".status-" + key + ".txt";
    }
    return true;
}

// closed bars in order, each once; a changed history (first stamp, fewer bars, a stamp moved) = replay from the start
void ZzoCombo::feed(ZState& S) {
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
        long long d = 0, xh = 0, xl = 0; b.vap = barDelta(S, i, d, b.h, b.l, &xh, &xl) > 0; b.dlt = d; b.xh = xh; b.xl = xl; b.xok = b.vap;
    };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        zzd::Bar b; bar(i, b);
        S.E.addBar(b); S.raw.push_back((RTDATE)times[i]);
        if (S.E.trimmed > 0) { S.off += (int)S.E.trimmed; S.raw.erase(S.raw.begin(), S.raw.begin() + std::min<long long>(S.E.trimmed, (long long)S.raw.size())); S.E.trimmed = 0; }
    }
    zzd::Bar fb; bar(closed, fb); fb.seg = S.E.segOf(fb.t); S.formBar = fb; S.haveForm = true;   // the forming bar, re-read every calc
}

void ZzoCombo::status(ZState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = zzd::comboStatus(S.E, S.mkt, S.spb, S.vapFault ? "FAULT - remove and re-add the indicator" : "ok") +
        "DRAWN,columns " + std::to_string(S.last.cols) + ",ratios " + std::to_string(S.last.ratios) + ",counts " + std::to_string(S.last.counts) + ",Ab " + std::to_string(S.last.ab) +
        ",Cmx " + std::to_string(S.last.cmx) + ",AR " + std::to_string(S.last.ar) + ",ST " + std::to_string(S.last.st) + ",CoB " + std::to_string(S.last.cob) + ",weak " + std::to_string(S.last.weak) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    zdMkdir(zdFlex()); zdPublish(S.statusPath, body);
}

void ZzoCombo::pump() {
    ZState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, zdMs());
}

static short zdC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int ZzoCombo::draw(void) {
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
        const bool noFoot = !S->E.B.empty() && S->E.noVap >= (long long)S->E.B.size();   // no footprint on this chart: volume still works, Ab cannot
        const std::string head = std::string("ZZO Combo ") + zzd::COMBO_VERSION + (noFoot ? "  no footprint - Ab off" : "");
        S->last = zzd::layoutCombo(S->E, waves, S->haveForm ? S->formBar.vol : 0, v, head);
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
    ZzoCombo* p = new ZzoCombo();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);   // its own pane, draws itself; no mouse tracking (no hovers)
    p->setDescription("LRA ZZO Combo v2.2.0 - 3-bar High/Low ZigZag waves: volume ramp vs the last 5 same-way waves, 2.0x+ ratio and count (beats the last N same-colour waves, 50+), Cmx / AR / ST, CoB after a key-level sweep, Ab = absorption with a heavy extreme (effort / result).");
    p->setVersion("2.2.0");   // = zzd::COMBO_VERSION (gex-build reads this literal)
    return p;
}
