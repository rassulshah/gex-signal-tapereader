/********************************************************************************
 *  MorrisRSI.cpp  --  Investor/RT RTX extension  lsMorrisRSI  "Morris RSI"  0.1.1  (2026-11-20)
 *
 *  Rassul 2026-11-19 11:55 CT "make a morris modified rsi based on 20 bars, the color is blue but below 30 make it green and above 70 make it
 *  red and if there are diversions also"; 12:00 his IRT RSI settings (20, smoothing 3 simple, Close, momentum 1, Morris modified, 70 / 30);
 *  12:07 "go" on morris-rsi-mockup.html. Its own pane, axis 0 - 100 (scale()). Rules, divergences and layout = MorrisRsiLogic.h.
 *  Computed natively from the chart's own bars (continuous over the loaded candles, no session reset). Closed candles fixed, the forming
 *  candle drawn live. Divergences decided once (2nd pivot confirmed), recorded and redrawn from the record - never repainted.
 *  No settings (NO_UI), no hovers.
 *  0.1.1 (2026-11-20 05:46 CT "make the red and green lines dashed and slightly thinner and get rid of the gray dashed line"): 70 / 30 lines
 *        = short 1 px dashes (each dash a solid segment from the logic's draw list), grey 50 line removed.
 *  FILES  lsFlexLevels\MorrisRSI\dv-<MKT>-<spb>.txt         the divergences (t1|t2|dir)
 *         lsFlexLevels\MorrisRSI\status-<MKT>-<spb>.txt     version, bars, last value, divergence count
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
#include "MorrisRsiLogic.h"
#include "MarketKey.h"
#include "HostSlot.h"
#ifndef LSPERF_PLUGIN
  #define LSPERF_PLUGIN "MorrisRSI"
#endif
#include "PerfGuard.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>
#define MRS_SEP "\\"
#else
#define MRS_SEP "/"
#endif

static const char* MRS_VERSION = "0.1.1";   // = mrsi::VERSION = the setVersion literal
static const int MRS_MAX_BARS = 3000;        // candles fed from the chart (newest)

static long long mrsMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string mrsFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + MRS_SEP "InvestorRT" MRS_SEP "rtx" MRS_SEP "lsFlexLevels" : std::string(); }
static bool mrsPublish(const std::string& path, const std::string& text) {
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
static bool mrsRead(const std::string& path, std::string& out) {   // bounded read (64 KiB)
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return false;
    std::string s; s.resize(65536); f.read(&s[0], 65536); s.resize((size_t)f.gcount()); out.swap(s); return true;
}

struct MRSState {
    std::string mkt, sym; int spb = 0; bool keyed = false;
    mrsi::Engine E; std::vector<RTDATE> raw; int off = 0;
    std::map<long long, mrsi::Dv> dv; size_t dvSeen = 0;              // the record (file + new) keyed by the 2nd pivot's stamp
    std::string dvPath, statusPath, statusBody; long long statusCk = -1; int drawn = 0;
    double liveClose = 0; bool hasLive = false;
};

class MorrisRSI : public cppExtension {
public:
    SS_SAFE_DIALOG(MorrisRSI, "MorrisRSI")
    MorrisRSI() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);
    HostSlot<MRSState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(MRSState& S);
    void feed(MRSState& S);
    void record(MRSState& S);
    void status(MRSState& S, long long now);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<MorrisRSI*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<MorrisRSI*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); MorrisRSI* me = static_cast<MorrisRSI*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

int MorrisRSI::scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0.0; if (dMax) *dMax = 100.0; return RTX_OK; }   // the pane's own 0 - 100 axis

bool MorrisRSI::identify(MRSState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    if (mkt.empty() || spb <= 0) return false;
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.keyed = true; S.E.reset(); S.raw.clear(); S.off = 0; S.dv.clear(); S.dvSeen = 0; S.statusBody.clear();
        const std::string d = mrsFlex();
        const std::string mk = mkey::norm(mkt);
        if (!d.empty()) {
            const std::string dir = d + MRS_SEP "MorrisRSI";
#if defined(_WIN32)
            _mkdir(dir.c_str());
#endif
            S.dvPath = dir + MRS_SEP "dv-" + mk + "-" + std::to_string(spb) + ".txt";
            S.statusPath = dir + MRS_SEP "status-" + mk + "-" + std::to_string(spb) + ".txt";
            std::string txt; if (mrsRead(S.dvPath, txt)) mrsi::dvParse(txt, mk, spb, S.dv);   // earlier decisions are kept as they were
        }
    }
    return true;
}

// closed candles in order, each once; a changed history = replay from the start (decided divergences stay in the record)
void MorrisRSI::feed(MRSState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.raw.clear(); S.dvSeen = 0; } }
    if (S.raw.empty()) S.off = std::max(0, closed - MRS_MAX_BARS);
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        const double h = hi[i], l = lo[i], c = cl[i]; if (!(h == h) || !(l == l) || !(c == c)) continue;
        mrsi::Bar b; b.t = (long long)times[i]; b.h = h; b.l = l; b.c = c;
        S.E.add(b); S.raw.push_back((RTDATE)times[i]);
    }
    const double lc = cl[closed]; S.hasLive = lc == lc; S.liveClose = lc;
}

void MorrisRSI::record(MRSState& S) {
    bool changed = false;
    for (size_t k = S.dvSeen; k < S.E.dv.size(); ++k) { const mrsi::Dv& d = S.E.dv[k]; if (!S.dv.count(d.t2)) { S.dv[d.t2] = d; changed = true; } }
    S.dvSeen = S.E.dv.size();
    if (changed && !S.dvPath.empty()) mrsPublish(S.dvPath, mrsi::dvText(mkey::norm(S.mkt), S.spb, S.dv));
}

void MorrisRSI::status(MRSState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusCk >= 0 && now - S.statusCk < 30000) return;              // at most every 30 s, only on change
    S.statusCk = now;
    const std::string body = mrsi::statusText(S.E, mkey::norm(S.mkt), S.sym, S.spb) + "HOST," + MRS_VERSION + "\nRECORDED," + std::to_string(S.dv.size()) + "\nDRAWN," + std::to_string(S.drawn) + "\n";
    if (body == S.statusBody) return;
    S.statusBody = body; mrsPublish(S.statusPath, body);
}

void MorrisRSI::pump() {
    MRSState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); record(*S); status(*S, mrsMs());
}

static short mrsC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int MorrisRSI::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    MRSState* S = slot_.get(this, false);
    if (!S || !S->keyed) return RTX_OK;
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        // the engine draws from its own candles; the record's divergences (incl. earlier sessions' decisions) replace the engine's list
        mrsi::Engine view = S->E; view.dv.clear(); for (const auto& kv : S->dv) view.dv.push_back(kv.second);
        mrsi::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.first = vs - S->off; v.last = ve - S->off; v.textH = 13;
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        FONT fb; fb.id = HELVETICA; fb.size = 10; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        v.hasLive = S->hasLive; v.liveIdx = (int)S->E.B.size(); v.live = S->hasLive ? S->E.liveValue(S->liveClose) : 0;
        const std::vector<mrsi::Prim> P = mrsi::layout(view, v);
        S->drawn = (int)P.size();
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const mrsi::Prim& p : P) {
            if (p.kind == 2) {
                const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
                if (x1 < x0 || y1 < y0) continue;
                setPen((COLOR)p.col, 1, P_SOLID); RCT r; r.set(mrsC(x0), mrsC(y0), mrsC(x1 + 1), mrsC(y1 + 1)); r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            } else if (p.kind == 0) {
                const int x0 = std::max(L, std::min(R, p.x0)), x1 = std::max(L, std::min(R, p.x1)), y0 = std::max(T, std::min(Bt, p.y0)), y1 = std::max(T, std::min(Bt, p.y1));
                setPen((COLOR)p.col, (short)p.width, P_SOLID);            // dashes are explicit segments (0.1.1)
                PNT q0; q0.set(0, 0.0f); q0.h = mrsC(x0); q0.v = mrsC(y0); q0.setDrawPosition();
                PNT q1; q1.set(0, 0.0f); q1.h = mrsC(x1); q1.v = mrsC(y1); q1.drawLineTo();
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x0 > R) continue;
                setFont(fb); setTextColor((COLOR)p.col);
                RCT r; r.set(mrsC(p.x0), mrsC(p.y0), mrsC(std::min(R, p.x1 + 2)), mrsC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    MorrisRSI* p = new MorrisRSI();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI);                                    // its own pane, draws itself; no hovers, no settings
    p->setDescription("LRA Morris RSI v0.1.1 - the Morris modified RSI (20, Close, momentum 1, simple means over only the last 20 changes) smoothed with a 3-candle simple average, in its own 0-100 pane: blue between 30 and 70, green under 30, red over 70, short-dashed 70 (red) / 30 (green) lines; regular divergences (Dv) between consecutive 3-bar ZigZag pivots at most 30 candles apart - bullish: a lower price low with a higher RSI low (1st RSI <= 40), bearish: a higher price high with a lower RSI high (1st RSI >= 60) - decided when the 2nd pivot is confirmed and never repainted. No settings.");
    p->setVersion("0.1.1");   // = MRS_VERSION (gex-build reads this literal)
    return p;
}
