/********************************************************************************
 *  CobSignal.cpp  --  Investor/RT RTX extension  lsCobSignal  "CoB Signal"  1.1.0  (2026-11-02; 1.0.0 2026-11-01)
 *
 *  Rassul 2026-11-01 22:12-22:24 CT ("i want you to build the cob signals instead of me having my own", "consider the new levels also
 *  PHH and PHL", labels "Cob , Cob+R , Cob+V", mockup cob_ba_mockup v2 "build"): his CoB computed natively from the chart's own bars,
 *  so his RTL cob_bull / cob_bear can come off the chart. Rules = CobSignalLogic.h (trade_def.cob_live + PHH / PHL; 202 of 202 parity
 *  on a year of ES without PHH / PHL). Label under a bull CoB bar (green) / over a bear CoB bar (red): Cob, Cob+R, Cob+V, Cob+RV.
 *  Decided once at the CoB bar's close, recorded, never repainted. No settings (NO_UI, SS_SAFE_DIALOG - the Candle Absorb lesson).
 *  FILES  lsFlexLevels\CobSignal\<MKT>-<spb>.csv          every CoB (append-only, each once)
 *         lsFlexLevels\CobSignal.status-<MKT>-<spb>.txt   version, rules, counts, the last CoBs
 *         lsFlexLevels\CobSignal-<MKT>-<spb>-live.csv     every CoB on the loaded bars (for lsWavePOC / the nightly), rewritten on change
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
#include "CobSignalLogic.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <sys/stat.h>

#if defined(_WIN32)
#define CS_SEP "\\"
#else
#define CS_SEP "/"
#endif

static const char* CS_VERSION = "1.1.0";   // = cobs::VERSION = the setVersion literal

static long long csMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string csFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + CS_SEP "InvestorRT" CS_SEP "rtx" CS_SEP "lsFlexLevels" : std::string(); }
static void csMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool csExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
static bool csPublish(const std::string& path, const std::string& text) {
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

struct CSState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false, mktOk = false;
    cobs::Engine E; std::vector<RTDATE> raw; int off = 0;
    std::string recPath, statusPath, livePath, recWhy = "not opened"; std::set<long long> written; int recFails = 0;
    long long statusMs = -1; std::string statusBody; size_t liveN = (size_t)-1; int drawn = 0;
};

class CobSignal : public cppExtension {
public:
    SS_SAFE_DIALOG(CobSignal, "CobSignal")      // the chart-less Add / settings copy never calls getRootSymbol (IRT crash 2026-11-01)
    CobSignal() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<CSState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(CSState& S);
    void feed(CSState& S);
    long long localSec(RTDATE d);
    void flush(CSState& S);
    void status(CSState& S, long long now);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<CobSignal*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<CobSignal*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { CobSignal* me = static_cast<CobSignal*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

long long CobSignal::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool CobSignal::identify(CSState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000 || mkt.empty() || spb <= 0) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.mktOk = S.E.setMarket(mkt, tick, spb); S.raw.clear(); S.off = 0; S.liveN = (size_t)-1;
        const std::string d = csFlex(); const std::string key = S.E.mkt + "-" + std::to_string(spb);
        S.recPath = d.empty() ? std::string() : d + CS_SEP "CobSignal" CS_SEP + key + ".csv";
        S.statusPath = d.empty() ? std::string() : d + CS_SEP "CobSignal.status-" + key + ".txt";
        S.livePath = d.empty() ? std::string() : d + CS_SEP "CobSignal-" + key + "-live.csv";
        S.written.clear(); S.recWhy = d.empty() ? "no USERPROFILE" : "new";
        std::ifstream f(S.recPath.c_str(), std::ios::binary);
        if (f) { std::string l; int n = 0; while (std::getline(f, l) && n < 200000) { ++n; if (l.rfind("COB,", 0) != 0) continue; char* e = nullptr; const long long t = std::strtoll(l.c_str() + 4, &e, 10); if (e && *e == ',' && t > 0) S.written.insert(t); } S.recWhy = "loaded " + std::to_string(S.written.size()) + " CoBs"; }
    }
    return S.mktOk;
}

// closed bars in order, each once; a changed history (first stamp moved, fewer bars) = replay from the start
void CobSignal::feed(CSState& S) {
    const long n = getBarCount(); if (n < 3) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const bool haveVol = (long)vo.count >= n;
    const int closed = (int)n - 1;                                       // the forming bar is never read
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.E.setMarket(S.mkt, S.tick, S.spb); S.raw.clear(); S.liveN = (size_t)-1; } }
    if (S.raw.empty()) S.off = std::max(0, closed - (cobs::MAX_BARS - 1000));
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        cobs::Bar b; b.t = localSec((RTDATE)times[i]); b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.v = haveVol ? (double)vo[i] : 0;
        if (!std::isfinite(b.h) || !std::isfinite(b.l)) { b.h = b.l = b.o = b.c = S.E.B.empty() ? 0 : S.E.B.back().c; }
        if (b.h < b.l) std::swap(b.h, b.l);
        S.E.addBar(b); S.raw.push_back((RTDATE)times[i]);
        if (S.E.trimmed > 0) { S.raw.clear(); S.off = std::max(0, closed - (cobs::MAX_BARS - 1000)); S.E.trimmed = 0; S.E.reset(); S.E.setMarket(S.mkt, S.tick, S.spb); break; }
    }
}

void CobSignal::flush(CSState& S) {
    if (!S.E.out.empty() && !S.recPath.empty()) {
        std::vector<std::string> fresh;
        for (const std::string& l : S.E.out) { char* e = nullptr; const long long t = std::strtoll(l.c_str() + 4, &e, 10); if (!S.written.count(t)) fresh.push_back(l); }
        if (!fresh.empty()) {
            const std::string d = csFlex(); csMkdir(d); csMkdir(d + CS_SEP "CobSignal");
            const bool isNew = !csExists(S.recPath);
            std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
            if (f) { if (isNew) f << "# lsCobSignal: COB,bar end (local secs),BULL/BEAR,label,levels swept,dratio,LOD/HOD,ADR,leg start,leg end,R since,V since,SHOW / LOG (1.1.0)\n"; for (const std::string& l : fresh) f << l << "\n"; f.flush(); }
            if (f.good()) { for (const std::string& l : fresh) { char* e = nullptr; S.written.insert(std::strtoll(l.c_str() + 4, &e, 10)); } S.recWhy = "appending"; }
            else { ++S.recFails; S.recWhy = "append failed " + std::to_string(S.recFails) + "x"; }
        }
        S.E.out.clear();
    }
    if (S.E.marks.size() != S.liveN && !S.livePath.empty()) {          // every CoB on the loaded bars, for the other indicators
        std::string body = std::string("COBSIGNAL,") + CS_VERSION + "\n";
        for (const cobs::Mark& m : S.E.marks) if (m.show) body += S.E.recLine(m) + "\n";   // (1.1.0) the drawn CoBs only (LOG ones stay in the record)
        csMkdir(csFlex()); if (csPublish(S.livePath, body)) S.liveN = S.E.marks.size();
    }
}

void CobSignal::status(CSState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = cobs::statusText(S.E, S.sym, S.spb, S.recWhy) + "DRAWN," + std::to_string(S.drawn) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    csMkdir(csFlex()); csPublish(S.statusPath, body);
}

void CobSignal::pump() {
    CSState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); flush(*S); status(*S, csMs());
}

static short csC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int CobSignal::draw(void) {
    CSState* S = slot_.get(this, false);
    if (!S || S->E.marks.empty()) return RTX_OK;                        // draw never reads bars
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        cobs::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.first = vs - S->off; v.last = ve - S->off;
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        v.Y = [this](double px) { return (double)getVerticalDrawPosition((float)px); };
        FONT fb; fb.id = HELVETICA; fb.size = 10; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        const std::vector<cobs::Prim> P = cobs::layout(S->E, v);
        S->drawn = (int)P.size();
        for (const cobs::Prim& p : P) {
            if (p.y0 < pane.top || p.y1 > pane.bottom || p.x0 < pane.left || p.x1 > pane.right) continue;
            setFont(fb); setTextColor((COLOR)p.col);
            RCT r; r.set(csC(p.x0), csC(p.y0), csC(p.x1 + 2), csC(p.y1)); r.drawText(p.text.c_str(), false, false);
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    CobSignal* p = new CobSignal();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA CoB Signal v1.1.0 - his CoB computed natively (sweep of ONH/ONL, PDH/PDL, PFH/PFL, LonHI/LonLO, PHH/PHL; leg >= 1.9x the last 20, >= 3 bars, 50-bar extreme, < 0.4 ADR; bulls to 11:00; a PHH/PHL-only CoB is drawn in ES, HG, GC and recorded only elsewhere): Cob, Cob+R (biggest range in 35+ bars at the origin), Cob+V (biggest volume). Decided at the bar close, never repainted.");
    p->setVersion("1.1.0");   // = CS_VERSION (gex-build reads this literal)
    return p;
}
