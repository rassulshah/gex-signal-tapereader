/********************************************************************************
 *  CandleAbsorb.cpp  --  Investor/RT RTX extension  lsCandleAbsorb  "Candle Absorb"  1.0.0  (2026-11-01)
 *
 *  1.0.0 (Rassul 2026-10-31 22:00-22:45 CT, the VS ES chart; mockup candle_abs.html approved with "the dash is supposed to be only on
 *    the candles"): at each IRT ZigZag pivot (3 bars, High/Low) and the next bar when it re-tests the extreme within 4 ticks, the bar's
 *    delta POC (biggest |buy - sell| row of IRT's VAP) in the extreme end (at a low: at / below open and close and in the lower half;
 *    a high mirrored) and >= 2.0x the bar's median row gets ONE code:
 *      A   attackers absorbed (sellers' POC at a low / buyers' POC at a high)
 *      Ae  A + exhaustion: the next row past the POC <= 20% of it
 *      D   demand (buyers' POC at a low)      S   supply (sellers' POC at a high)
 *    green under lows, red over highs; no "?" (Rassul 2026-11-01 09:17 CT "get rid of the question mark"); the nightly still scores them. Decided at the bar that confirms the pivot
 *    (closed bars only), recorded, never repainted. Plus on EVERY candle a short dash ON the candle at its delta POC (a little wider
 *    than the body, black edge; green = buyers made the POC, red = sellers) - the forming candle's dash moves until it closes.
 *  Rules / layout: CandleAbsorbLogic.h (SDK-free, tested: tests_candleabsorb). No settings (controls none, NO_UI).
 *  FILES (lsFlexLevels\CandleAbsorb\):  <MKT>-<spb>.csv   every decided mark (append-only, each once)
 *          lsFlexLevels\CandleAbsorb.status-<MKT>-<spb>.txt  version, rules, counts, last marks (on change / 30 s)
 *  Per chart state in the host slot; each closed bar read once (VAP in calc only, never in draw).
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
#include "CandleAbsorbLogic.h"
#include "HostSlot.h"
#include "SettingsGuard.h"   // (1.0.1) SS_SAFE_DIALOG: the chart-less Add / settings copy never faults
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#if defined(_WIN32)
#define CA_SEP "\\"
#else
#define CA_SEP "/"
#endif

static const char* CA_VERSION = "1.1.1";   // (1.1.1) code one row further out   // was "1.1.0":   // (1.1.0, 2026-11-02 Rassul 13:10 CT) the live mark at the leg extreme's close, repainted until the ZigZag confirms   // was "1.0.1":   // (1.0.1) the Add-window crash fix (SS_SAFE_DIALOG); drawing / rules unchanged   // was "1.0.0"   // = cab::VERSION = the setVersion literal (test_candleabsorb_host checks)

static long long caMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string caFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + CA_SEP "InvestorRT" CA_SEP "rtx" CA_SEP "lsFlexLevels" : std::string(); }
static void caMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool caExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
static bool caPublish(const std::string& path, const std::string& text) {   // .tmp -> replace (atomic on Windows), .bak dance elsewhere
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    const bool had = caExists(path);
    if (had) { std::remove(bak.c_str()); if (std::rename(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; } }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (had) std::rename(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    if (had) std::remove(bak.c_str());
    return true;
#endif
}

struct CState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false;
    cab::Engine E; std::vector<RTDATE> raw; int off = 0;
    bool vapFault = false; long long vapMissing = 0;
    cab::BarRead form; int formIdx = -1; bool haveForm = false; cab::Bar formBar;
    std::string recPath, statusPath, recWhy = "not opened"; std::set<long long> written; int recFails = 0;
    long long statusMs = -1; std::string statusBody;
    cab::Scene last;
};

class CandleAbsorb : public cppExtension {
public:
    SS_SAFE_DIALOG(CandleAbsorb, "CandleAbsorb")   // (1.0.1, 2026-11-01 crash fix) IRT gives the Add / settings window a copy with NO chart:
                                                   // getRootSymbol / getSymbol there crashed IRT (xnmba150.dll, 17:05 CT) - guarded like ChartView
    CandleAbsorb() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }          // no settings: nothing to load or save
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<CState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(CState& S);
    void feed(CState& S);
    int rows(CState& S, int chartBar, std::vector<cab::DRow>& out);
    long long localSec(RTDATE d);
    void flush(CState& S);
    void status(CState& S, long long now);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }   // (1.0.1) no settings, as ChartView
int cppExtension::done(void) { static_cast<CandleAbsorb*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<CandleAbsorb*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { CandleAbsorb* me = static_cast<CandleAbsorb*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }   // bars + VAP read here only

// SEH-guarded VAP reads (as lsWavePOC / lsDeltaProfile)
int CandleAbsorb::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int CandleAbsorb::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
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
// one bar's same-price delta rows (buy - sell volume); 1 = read, 0 = no VAP, -1 = fault (never read again on this chart)
int CandleAbsorb::rows(CState& S, int chartBar, std::vector<cab::DRow>& out) {
    out.clear();
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
    for (const VOLPROFILE& v : rv) {
        if (!std::isfinite((double)v.price) || v.buyVolume < 0 || v.sellVolume < 0) continue;
        if (v.buyVolume + v.sellVolume <= 0) continue;
        cab::DRow r; if (!cab::toTicks((double)v.price, S.tick, r.tick)) continue;
        r.dlt = (long long)v.buyVolume - (long long)v.sellVolume; out.push_back(r);
    }
    return out.empty() ? 0 : 1;
}

long long CandleAbsorb::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

bool CandleAbsorb::identify(CState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string mkt = rs ? rs : "";
    for (char& c : mkt) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;   // SYM_TICKINCR only (never a guessed tick)
    if (mkt.empty()) return false;
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.raw.clear(); S.off = 0; S.haveForm = false; S.formIdx = -1;
        const std::string d = caFlex(); const std::string key = mkt + "-" + std::to_string(spb);
        S.recPath = d.empty() ? std::string() : d + CA_SEP "CandleAbsorb" CA_SEP + key + ".csv";
        S.statusPath = d.empty() ? std::string() : d + CA_SEP "CandleAbsorb.status-" + key + ".txt";
        S.written.clear(); S.recWhy = d.empty() ? "no USERPROFILE" : "new";
        std::ifstream f(S.recPath.c_str(), std::ios::binary);   // marks already written (so a restart never writes one twice)
        if (f) { std::string l; int n = 0; while (std::getline(f, l) && n < 200000) { ++n; if (l.rfind("MARK,", 0) != 0) continue; char* e = nullptr; const long long t = std::strtoll(l.c_str() + 5, &e, 10); if (e && *e == ',' && t > 0) S.written.insert(t); } S.recWhy = "loaded " + std::to_string(S.written.size()) + " marks"; }
    }
    return true;
}

// closed bars in order, each once; a changed history (first stamp, fewer bars, a stamp moved) = replay from the start
void CandleAbsorb::feed(CState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;
    const int fed = (int)S.raw.size();
    if (fed > 0) { const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) { S.E.reset(); S.raw.clear(); } }
    if (S.raw.empty()) S.off = std::max(0, closed - cab::MAX_BARS);
    std::vector<cab::DRow> rw;
    auto bar = [&](int i, cab::Bar& b) {
        b.t = localSec((RTDATE)times[i]);
        if (!cab::toTicks(op[i], S.tick, b.o) || !cab::toTicks(hi[i], S.tick, b.h) || !cab::toTicks(lo[i], S.tick, b.l) || !cab::toTicks(cl[i], S.tick, b.c)) { b.o = b.h = b.l = b.c = S.E.B.empty() ? 0 : S.E.B.back().c; }
        if (b.h < b.l) std::swap(b.h, b.l);
    };
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        cab::Bar b; bar(i, b);
        if (rows(S, i, rw) <= 0) { rw.clear(); ++S.vapMissing; }   // no VAP for the bar: no dash / no mark (never guessed)
        S.E.addBar(b, rw); S.raw.push_back((RTDATE)times[i]);
        if (S.E.trimmed > 0) { S.off += (int)S.E.trimmed; S.raw.erase(S.raw.begin(), S.raw.begin() + std::min<long long>(S.E.trimmed, (long long)S.raw.size())); S.E.trimmed = 0; }
    }
    // the forming candle: its dash only (re-read every calc; marks need closed bars)
    cab::Bar fb; bar(closed, fb); S.formBar = fb; S.formIdx = closed - S.off;
    S.haveForm = rows(S, closed, rw) > 0; S.form = S.haveForm ? cab::readBar(fb, rw) : cab::BarRead();
}

void CandleAbsorb::flush(CState& S) {
    if (S.E.out.empty() || S.recPath.empty()) { S.E.out.clear(); return; }
    std::vector<std::string> fresh; for (const std::string& l : S.E.out) { char* e = nullptr; const long long t = std::strtoll(l.c_str() + 5, &e, 10); if (!S.written.count(t)) fresh.push_back(l); }
    if (fresh.empty()) { S.E.out.clear(); return; }
    const std::string d = caFlex(); caMkdir(d); caMkdir(d + CA_SEP "CandleAbsorb");
    const bool isNew = !caExists(S.recPath);
    std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
    if (f) { if (isNew) f << "# lsCandleAbsorb marks: MARK,bar end (local secs),time,LOW/HIGH,code,poc tick,delta,x median,pivot,which bar,confirmed\n"; for (const std::string& l : fresh) f << l << "\n"; f.flush(); }
    if (f.good()) { for (const std::string& l : fresh) { char* e = nullptr; S.written.insert(std::strtoll(l.c_str() + 5, &e, 10)); } S.E.out.clear(); S.recWhy = "appending"; }
    else { ++S.recFails; S.recWhy = "append failed " + std::to_string(S.recFails) + "x (kept for the next try)"; if (S.E.out.size() > 5000) S.E.out.clear(); }
}

void CandleAbsorb::status(CState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;
    const std::string vap = S.vapFault ? "FAULT - remove and re-add the indicator" : "ok, bars without VAP " + std::to_string(S.vapMissing);
    const std::string body = cab::statusText(S.E, S.mkt, S.spb, vap, S.recWhy) + "DRAWN,dashes " + std::to_string(S.last.dashes) + ",marks " + std::to_string(S.last.marksDrawn) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    caMkdir(caFlex()); caPublish(S.statusPath, body);
}

void CandleAbsorb::pump() {
    CState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); flush(*S); status(*S, caMs());
}

static short caC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int CandleAbsorb::draw(void) {
    CState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) return RTX_OK;   // draw never reads bars / VAP
    try {
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        RCT pane; pane.getPaneRect(false);
        int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
        cab::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom;
        v.first = vs - S->off; v.last = ve - S->off; v.bw = std::max(1, (int)getPixelsPerBar());
        const int off = S->off; const double tick = S->tick;
        v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
        v.Y = [this, tick](double tk) { return (double)getVerticalDrawPosition((float)(tk * tick)); };
        FONT fb; fb.id = HELVETICA; fb.size = 10; fb.style = BOLD;
        v.textW = [this, &fb](const std::string& s) { setFont(fb); return (int)getTextWidth(s.c_str(), -1); };
        v.haveForming = S->haveForm; v.forming = S->formBar; v.formRead = S->form; v.formIdx = S->formIdx;
        v.clipL = pane.left; v.clipR = pane.right;
        S->last = cab::layout(S->E, v);
        const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
        for (const cab::Prim& p : S->last.p) {
            if (p.k == cab::Prim::FILL) {
                const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
                if (x1 <= x0 || y1 < y0) continue;
                RCT r; r.set(caC(x0), caC(y0), caC(x1 + 1), caC(y1 + 1)); r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            } else {
                if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) continue;
                setFont(fb); setTextColor((COLOR)p.col);
                RCT r; r.set(caC(p.x0), caC(p.y0), caC(p.x1 + 2), caC(p.y1)); r.drawText(p.text.c_str(), false, false);
            }
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    CandleAbsorb* p = new CandleAbsorb();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setDescription("LRA Candle Absorb v1.1.0 - at ZigZag pivots the bar's delta POC in the extreme end: A absorbed, Ae absorbed + exhausted, D demand, S supply; a delta POC dash on every candle.");
    p->setVersion("1.1.1");   // = CA_VERSION (gex-build reads this literal)
    return p;
}
