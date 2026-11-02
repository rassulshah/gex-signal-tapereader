/********************************************************************************
 *  BigPrint.cpp  --  Investor/RT RTX extension  lsBigPrint  "Big Print"  1.0.1  (2026-11-02; 1.0.0 2026-11-01)
 *
 *  Rassul 2026-11-01 22:12-22:24 CT (mockup cob_ba_mockup v2 "build"): ONE pane replacing his BV_Since / BVGB / BVRB / BRB_Since /
 *  BGB_Since panes. Per bar a green bar (volume bought at the ask) and a red bar (sold at the bid) side by side up from the baseline;
 *  the number over each = how many bars back its biggest ask / bid print is the biggest (from 10; bold and bright from 35; cap 100);
 *  "V42" at the top = biggest volume in 42 bars (from 35); bright bar = the biggest green / red candle (range) in 35+ bars.
 *  Logic: BigPrintLogic.h (SDK-free, tested). No settings (NO_UI, SS_SAFE_DIALOG). VAP read in calc only, never in draw.
 *  FILES  lsFlexLevels\BigPrint.status-<MKT>-<spb>.txt
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "BigPrintLogic.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <map>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#if defined(_WIN32)
#define BP_SEP "\\"
#else
#define BP_SEP "/"
#endif

static const char* BP_VERSION = "1.0.1";   // (1.0.1) the numbers = lsBigBidAsk's boxes   // was "1.0.0":   // = bpr::VERSION = the setVersion literal

static long long bpMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string bpFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + BP_SEP "InvestorRT" BP_SEP "rtx" BP_SEP "lsFlexLevels" : std::string(); }
static void bpMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool bpPublish(const std::string& path, const std::string& text) {
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

struct BPState {
    std::string mkt, sym; int spb = 0; long long tickKey = 0; bool keyed = false;
    bpr::Engine E; std::vector<RTDATE> raw; int off = 0;
    bool vapFault = false; long long noVap = 0;
    bool haveForm = false; bpr::Bar form; bpr::Counts formC; int formIdx = -1;
    std::string statusPath; long long statusMs = -1; std::string statusBody; bpr::Scene last;
};

class BigPrint : public cppExtension {
public:
    SS_SAFE_DIALOG(BigPrint, "BigPrint")
    BigPrint() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<BPState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(BPState& S);
    void feed(BPState& S);
    int prints(BPState& S, int chartBar, bpr::Bar& b);
    void status(BPState& S, long long now);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<BigPrint*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BigPrint*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { BigPrint* me = static_cast<BigPrint*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

int BigPrint::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int BigPrint::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
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
// one bar's VAP: total bought / sold and the biggest one-price ask / bid print; 1 = read, 0 = no VAP, -1 = fault (never read again)
int BigPrint::prints(BPState& S, int chartBar, bpr::Bar& b) {
    b.vap = false; b.buy = b.sell = b.bigAsk = b.bigBid = 0;
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
    // (1.0.1) rows of the SAME price (a float price can arrive twice for one tick) are added together first, as lsBigBidAsk does
    const double tick = S.tickKey > 0 ? (double)S.tickKey / 1e9 : 0.0;
    std::map<long long, std::pair<double, double> > at;   // tick key -> (bought at the ask, sold at the bid)
    for (const VOLPROFILE& v : rv) {
        if (v.buyVolume < 0 || v.sellVolume < 0) continue;
        b.buy += (double)v.buyVolume; b.sell += (double)v.sellVolume;
        const double px = (double)v.price;
        const long long key = tick > 0 && std::isfinite(px) ? std::llround(px / tick) : (long long)std::llround(px * 1e6);
        std::pair<double, double>& r = at[key]; r.first += (double)v.buyVolume; r.second += (double)v.sellVolume;
    }
    for (const auto& r : at) { b.bigAsk = std::max(b.bigAsk, r.second.first); b.bigBid = std::max(b.bigBid, r.second.second); }
    b.vap = b.buy + b.sell > 0;
    return b.vap ? 1 : 0;
}

bool BigPrint::identify(BPState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    const double tick = (double)getProperty(SYM_TICKINCR);
    if (mkt.empty() || !std::isfinite(tick) || !(tick > 0)) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt == "6E" ? "EU" : mkt; S.sym = sym; S.spb = spb; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.raw.clear(); S.off = 0; S.haveForm = false; S.noVap = 0;
        const std::string d = bpFlex();
        S.statusPath = d.empty() ? std::string() : d + BP_SEP "BigPrint.status-" + S.mkt + "-" + std::to_string(spb) + ".txt";
    }
    return true;
}

// closed bars in order, each once; a changed history = replay from the start; the forming bar re-read every calc
void BigPrint::feed(BPState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime), vo(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const bool haveVol = (long)vo.count >= n;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - (bpr::MAX_BARS - 2000));
    auto bar = [&](int i, bpr::Bar& b) { b.t = (long long)times[i]; b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.v = haveVol ? (double)vo[i] : 0; if (b.h < b.l) std::swap(b.h, b.l); };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        bpr::Bar b; bar(i, b); if (prints(S, i, b) <= 0) ++S.noVap;
        S.E.addBar(b); S.raw.push_back((RTDATE)times[i]);
        if (S.E.trimmed > 0) { const long long t = S.E.trimmed; S.off += (int)t; S.raw.erase(S.raw.begin(), S.raw.begin() + std::min<long long>(t, (long long)S.raw.size())); S.E.trimmed = 0; }
    }
    bpr::Bar f; bar(closed, f); prints(S, closed, f);
    S.form = f; S.formIdx = closed - S.off; S.formC = bpr::countsAt(S.E.B, (int)S.E.B.size(), f); S.haveForm = true;
}

void BigPrint::status(BPState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    std::string body = bpr::statusText(S.E, S.mkt, S.sym, S.spb, S.noVap, S.last);
    if (S.vapFault) body += "VAP,FAULT - remove and re-add the indicator\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    bpMkdir(bpFlex()); bpPublish(S.statusPath, body);
}

void BigPrint::pump() {
    BPState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); status(*S, bpMs());
}

static short bpC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int BigPrint::draw(void) {
    BPState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) return RTX_OK;                            // draw never reads bars / VAP
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        bpr::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.first = vs - S->off; v.last = ve - S->off;
        v.bw = std::max(1, (int)getPixelsPerBar());
        const int off = S->off;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        FONT fn; fn.id = HELVETICA; fn.size = 10; fn.style = PLAIN; FONT fb = fn; fb.style = BOLD;
        v.textW = [this, &fn, &fb](const std::string& s, bool bold) { setFont(bold ? fb : fn); return (int)getTextWidth(s.c_str(), -1); };
        v.haveForm = S->haveForm; v.form = S->form; v.formC = S->formC; v.formIdx = S->formIdx;
        S->last = bpr::layout(S->E, v);
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const bpr::Prim& p : S->last.p) {
            if (p.k == bpr::Prim::FILL) {
                const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
                if (x1 < x0 || y1 < y0) continue;
                RCT r; r.set(bpC(x0), bpC(y0), bpC(x1 + 1), bpC(y1 + 1)); r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) continue;
                setFont(p.bold ? fb : fn); setTextColor((COLOR)p.col);
                RCT r; r.set(bpC(p.x0), bpC(p.y0), bpC(p.x1 + 2), bpC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    BigPrint* p = new BigPrint();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);   // its own pane, draws itself
    p->setDescription("LRA Big Print v1.0.1 - one pane for the biggest prints: green = bought at the ask, red = sold at the bid (side by side); the number over each bar = its biggest one-price ask / bid print is the biggest in N bars (lsBigBidAsk's count: smaller prints in a row, cap 50; from 10, bold from 35); V = biggest volume in N bars (from 35); a bright bar = the biggest green / red candle in 35+ bars. Replaces BV_Since / BVGB / BVRB / BRB_Since / BGB_Since.");
    p->setVersion("1.0.1");   // = BP_VERSION (gex-build reads this literal)
    return p;
}
