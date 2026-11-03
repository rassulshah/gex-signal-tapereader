/********************************************************************************
 *  Breakout.cpp  --  Investor/RT RTX extension  lsBreakout  "BO"  1.1.0  (2026-11-03)
 *
 *  1.1.0 (2026-11-03, Rassul approved the Pb mockup - ES Oct 30 - and said "build"): + "Pb", a pullback that TESTS the BO / BD candle:
 *    every confirmed 3-bar High / Low ZigZag pivot LOW after a BO whose bar overlaps the BO candle gets a green "Pb" just below its low
 *    (pivot HIGHs after a BD: red, just above the high), until the next BO / BD or price trades beyond the BO / BD wave's origin pivot.
 *    Same small font as BO / BD, never on another letter, drawn when the pivot is confirmed and never changed (BreakoutLogic.h 1.1.0).
 *    Pb is now drawn ONLY here (lsWavePOC 1.8.3 stopped drawing its POC-pullback Pb).
 *  1.0.0 (2026-11-03, Rassul approved the mockup bo_mockup.html - ES Oct 30, the real decisions lsBigBidAsk 1.31.3 recorded that day -
 *    and said "build"): a separate, letters-only indicator on the price chart:
 *      BO  green, just above the breakout candle's high      BD  red, just below the breakdown candle's low   (small, ~9 px)
 *      Rly green above the end of a rally / red below the end of a decline, one row further out (3-min charts only)
 *    Nothing else: no prints, lots, numbers, boxes, hovers or info box. No settings window rows (SS_SAFE_DIALOG, as lsCandleAbsorb).
 *    THE SAME DECISIONS as lsBigBidAsk: BO / BD through bba::boFromCandles (BigBidAskLogic.h 1.32.0, the function lsBigBidAsk's
 *    boPass calls), Rly through rly::run (RallyLogic.h 1.1.0). lsBigBidAsk 1.32.0 stopped drawing these letters (it still decides and
 *    records them for the nightly and lsLiquidityProfile).
 *  Calculated natively from the chart's bars (OHLC + volume); no file is read at run time. Closed candles only; a decision is kept for
 *  the chart's life and never changes or disappears (no repaint). Posts SignalBoard kind 'L' (the BO letters as drawn) for
 *  lsLiquidityProfile's Ex letter, as lsBigBidAsk 1.31.1 did.
 *  FILES: lsFlexLevels\Breakout.status-<MKT>-<spb>.txt  version, rules, today's counts, drawn counts (on change / 30 s; write only)
 *  Logic + layout: BreakoutLogic.h (SDK-free, tests_bo100). Per chart state in the host slot (HostSlot.h).
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
#include "BreakoutLogic.h"
#include "HostSlot.h"
#include "SettingsGuard.h"   // SS_SAFE_DIALOG: the chart-less Add / settings copy never faults
#include "ColumnBoard.h"     // cb::leftKeep - nothing under the left Liquidity / Key Levels columns (as lsBigBidAsk 1.31.3)
#include "BoxDock.h"         // bdk::peekHodLod - never over the HOD/LOD table
#include "SignalBoard.h"     // sb::publish('L', ...) - the BO letters as drawn (lsLiquidityProfile reads them)
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

#if defined(_WIN32)
#define BKO_SEP "\\"
#else
#define BKO_SEP "/"
#endif

#define BKO_VERSION "1.1.0"   // = bko::VERSION = the setVersion literal below (test_bo100_host checks)

static long long bkoMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string bkoFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + BKO_SEP "InvestorRT" BKO_SEP "rtx" BKO_SEP "lsFlexLevels" : std::string(); }
static void bkoMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool bkoExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
static bool bkoPublish(const std::string& path, const std::string& text) {   // .tmp -> replace (atomic on Windows), .bak dance elsewhere
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    const bool had = bkoExists(path);
    if (had) { std::remove(bak.c_str()); if (std::rename(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; } }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (had) std::rename(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    if (had) std::remove(bak.c_str());
    return true;
#endif
}
static short bkoC(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

struct BState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    int openMin = 510, closeMin = 900;
    bko::Engine E; bko::Scene last;
    std::string statusPath, statusBody; long long statusMs = -1;
    bb::Beat trBeat, hlBeat;   // the Tape Reader banner / HOD/LOD table present while their stamps keep moving (as lsBigBidAsk)
};

class Breakout : public cppExtension {
public:
    SS_SAFE_DIALOG(Breakout, "Breakout")   // every plugin without settings: the Add / settings window copy has NO chart (lsCandleAbsorb 1.0.1)
    Breakout() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings: nothing to load or save
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<BState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(BState& S);
    void feed(BState& S);
    void status(BState& S, long long now);
    bool parts(RTDATE d, long& days, int& endSec);
    BState* testState() { return slot_.get(this, false); }
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }   // no settings (as lsCandleAbsorb / lsChartView)
int cppExtension::done(void) { static_cast<Breakout*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<Breakout*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { Breakout* me = static_cast<Breakout*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }   // bars read here only

// the bar stamp as the local (Central) civil day + second of the day (IRT stamps = the bar END)
bool Breakout::parts(RTDATE d, long& days, int& endSec) {
    struct tm m; std::memset(&m, 0, sizeof m);
    if (d == 0 || !getLocaltime(d, &m) || m.tm_mon < 0 || m.tm_mon > 11 || m.tm_mday < 1 || m.tm_mday > 31) return false;
    days = svl::daysFromCivil(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday); endSec = m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec; return true;
}

bool Breakout::identify(BState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    const double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;   // SYM_TICKINCR only (never a guessed tick)
    if (mkt.empty()) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {   // a new market / symbol / period: start over
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true; S.E.reset();
        S.openMin = 510; S.closeMin = 900; { int o = 0, c = 0; if (svl::rthOf(mkt.c_str(), o, c)) { S.openMin = o; S.closeMin = c; } }
        const std::string d = bkoFlex();
        S.statusPath = d.empty() ? std::string() : d + BKO_SEP "Breakout.status-" + mkt + "-" + std::to_string(spb) + ".txt";
        S.statusBody.clear(); S.statusMs = -1;
    }
    return true;
}

void Breakout::feed(BState& S) {
    const long n = getBarCount(); if (n < 3) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n || (long)vo.count < n) return;
    const long long lastClosed = (long long)times[(int)n - 2];
    if (S.E.count == n && S.E.lastClosedT == lastClosed && S.E.firstT == (long long)times[0]) return;   // nothing new closed (closed bars are final)
    const int first = std::max(0, (int)n - 1 - bko::MAX_BARS - 400);
    std::vector<bko::In> B((size_t)(n - first));
    for (int k = first; k < (int)n; ++k) {
        bko::In& b = B[(size_t)(k - first)]; b.t = (long long)times[k];
        b.o = op[k]; b.h = hi[k]; b.l = lo[k]; b.c = cl[k]; b.vol = (long long)vo[k];
        b.ok = parts((RTDATE)times[k], b.days, b.endSec);
    }
    S.E.update(B, n, (long long)times[0], S.tick, S.spb, S.openMin, S.closeMin);
}

void Breakout::status(BState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string body = bko::statusText(S.E, S.mkt, S.sym, S.spb, S.last);
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    bkoMkdir(bkoFlex()); bkoPublish(S.statusPath, body);
}

void Breakout::pump() {
    BState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, bkoMs());
}

int Breakout::draw(void) {
    BState* S = slot_.get(this, false);
    if (!S || S->E.bars.size() < 3) return RTX_OK;   // draw never reads bars for decisions (calc does)
    try {
        const long n = getBarCount(); if (n < 3) return RTX_OK;
        const int off = (int)n - (int)S->E.bars.size();   // chart index of E.bars[0]
        if (off < 0) return RTX_OK;
        RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
        int axisR = pane.right;
        if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
        if (pane.bottom - pane.top < 60 || axisR - pane.left < 60) return RTX_OK;
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0, (int)n - 200); ve = (int)n - 1; }
        const char* sy = getSymbol(); const char* chl = getChartLabel();
        const std::string sym = sy ? sy : "", chart = chl ? chl : ""; const long long ms = bkoMs();
        bko::View v; v.first = vs - off; v.last = ve - off;
        v.area.l = std::max((int)pane.left + 2, cb::leftKeep(sym, getSecondsPerBar(), chart, (int)pane.left, ms));
        v.area.t = pane.top + 20; v.area.r = axisR - 1; v.area.b = pane.bottom - 1;   // the IRT title row stays clear
        {   bb::Rect r; long long bms = -1; const bool hb = bb::peekBanner(sym, getSecondsPerBar(), chart, r, bms);
            if (S->trBeat.step(hb, bms, ms)) { bko::Box k; k.l = r.l; k.t = r.t; k.r = r.r; k.b = r.b; v.keepOut.push_back(k); }
            bdk::Box h; long long hms = -1; const bool hh = bdk::peekHodLod(sym, getSecondsPerBar(), chart, h, hms);
            if (S->hlBeat.step(hh, hms, ms)) { bko::Box k; k.l = h.l - 2; k.t = h.t - 2; k.r = h.r + 2; k.b = h.b + 2; v.keepOut.push_back(k); } }
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (int)p.h; };
        v.Y = [this](double px) { return (int)getVerticalDrawPosition((float)px); };
        FONT f0; f0.id = HELVETICA; f0.size = bko::FONT_BO; f0.style = BOLD;
        FONT f1 = f0; f1.size = bko::FONT_RLY;
        int lh[2] = { 0, 0 }, lift[2] = { 1, 1 };
        for (int k = 0; k < 2; ++k) { FONT& f = k ? f1 : f0; setFont(f); int ll = 0, la = 0, ld = 0; getFontMetrics(&ll, &la, &ld);
            lh[k] = std::max((int)f.size + 5, la + ld); lift[k] = std::max(1, (ll + ld + 1) / 2); }
        v.lh[0] = lh[0]; v.lh[1] = lh[1];
        v.textW = [this, &f0, &f1](const char* s, int font) { setFont(font == bko::FONT_RLY ? f1 : f0); return (int)getTextWidth(s, -1); };
        S->last = bko::layout(S->E, v);
        for (const bko::Txt& T : S->last.txt) {   // lsBigBidAsk's letter drawing: a 1 px black edge, then the colour
            const int k = T.kind == 1 ? 1 : 0; setFont(k ? f1 : f0); const int lf = lift[k];
            setTextColor(0x00000000);
            for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
                RCT r; r.set(bkoC(T.f.l + dx), bkoC(T.f.t - lf + dy), bkoC(T.f.r + dx), bkoC(T.f.b - lf + dy)); r.drawText(T.s.c_str(), true, false); }
            setTextColor((COLOR)T.col);
            RCT r; r.set(bkoC(T.f.l), bkoC(T.f.t - lf), bkoC(T.f.r), bkoC(T.f.b - lf)); r.drawText(T.s.c_str(), true, false);
        }
        std::vector<sb::Ev> ev;   // SignalBoard 'L' (SignalBoard.h 1.3.0): bar (CT wall seconds), side, px from the extreme to the outer edge
        for (const bko::Board& b : S->last.board) { long d = 0; int e = 0; if (!parts((RTDATE)b.t, d, e)) continue;
            sb::Ev x; x.bar = (long long)d * 86400LL + e; x.side = b.side; x.val = (double)b.val; ev.push_back(x); }
        sb::publish('L', sym, getSecondsPerBar(), chart, ms, ev);
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    Breakout* p = new Breakout();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA BO v1.1.0 - letters only on the price chart: BO (green, above) / BD (red, below) on a confirmed breakout candle (lsBigBidAsk's rule, shown when the follow-through candle closes), Rly at the end of a rally / decline (3-min) and Pb on a pullback pivot that tests the BO / BD candle. No settings.");
    p->setVersion("1.1.0");   // = BKO_VERSION (gex-build reads this literal)
    return p;
}
