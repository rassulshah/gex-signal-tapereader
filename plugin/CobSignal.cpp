/********************************************************************************
 *  CobSignal.cpp  --  Investor/RT RTX extension  lsCobSignal  "CoB Signal"  1.3.0  (2026-11-04: + LVP low-volume pullback; 1.2.0 2026-11-02 evening = his cob_bull_3m_1b / cob_bear_3m_1b + 11:00 CT; 1.0.0 2026-11-01)
 *
 *  Rassul 2026-11-01 22:12-22:24 CT ("i want you to build the cob signals instead of me having my own", "consider the new levels also
 *  PHH and PHL", labels "Cob , Cob+R , Cob+V", mockup cob_ba_mockup v2 "build"): his CoB computed natively from the chart's own bars,
 *  so his RTL cob_bull / cob_bear can come off the chart. Rules = CobSignalLogic.h (trade_def.cob_live + PHH / PHL; 202 of 202 parity
 *  on a year of ES without PHH / PHL). Label under a bull CoB bar (green) / over a bear CoB bar (red): Cob, Cob+R, Cob+V, Cob+RV.
 *  Decided once at the CoB bar's close, recorded, never repainted. No settings (NO_UI, SS_SAFE_DIALOG - the Candle Absorb lesson).
 *  1.3.0 (2026-11-04, Rassul approved the LVP mockup - GC 3-min Nov 3 - and said "build"): + LVP = a pullback wave (3-bar High / Low ZigZag,
 *  zzd::Engine::step) among the first 3 after the CoB that stays inside the CoB wave's range on <= 1/3 of the CoB wave's volume: red "LVP"
 *  above it after a bear CoB (no demand), green below it after a bull CoB (no supply); decided when the wave is confirmed, never repainted;
 *  one "LVP,..." record line each (rules: CobSignalLogic.h).
 *  FILES  lsFlexLevels\CobSignal\<MKT>-<spb>.csv          every CoB (append-only, each once) + (1.3.0) every LVP ("LVP,..." lines, each once)
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
#include "AccumLogic.h"            // (1.8.3) lsRevRead's accumulation / distribution zones: the boxes on the price chart
#include "HostSlot.h"
#include "SignalBoard.h"           // (1.9.2) sb::publish('K', ...) - the candle labels as drawn (lsRevReadMarks keeps clear of them)
#ifndef LSPERF_PLUGIN   // (2026-11-18 memory hunt) the IRT performance guard: time and memory of every calc / draw (Perf\perf- / permem-<plugin>)
  #define LSPERF_PLUGIN "CobSignal"
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
#include <set>
#include <string>
#include <vector>
#include <sys/stat.h>

#if defined(_WIN32)
#define CS_SEP "\\"
#else
#define CS_SEP "/"
#endif

static const char* CS_VERSION = "1.12.1";   // (1.12.1, 2026-11-26 17:21 CT) Asia 6:00 PM - 2:00 AM CT (CobSignalLogic 1.12.1)
// was static const char* CS_VERSION = "1.12.0";   // (1.12.0, Rassul 2026-11-25 15:43-15:44 CT "get rid of tracking that type of cob", "cob must have at least 10" (GC 11-25 8:18 AM CoB, ZZO 0, was logged)) CoBs under ZZO 10 are not tracked (CobSignalLogic 1.12.0 gate)
// was "1.11.1";   // (1.11.1, Rassul 2026-11-22 19:41 CT "R should be red and S should be green", 20:22 "A") the 3-hour R1 / R2 lines and names red, S1 / S2 green   // was "1.11.0"   // (1.11.0) strength line from 10 up (Rassul 18:19 CT "switch to 10")   // was "1.10.0"   // (1.10.0, Rassul 2026-11-19 15:56-16:58 CT, price-pane-cleanup-mockup.html, "ok"): Cob / Lvp / Rev as badges (black text), the Cob badge with its ZZO count + strength, the Rev with its strength; only the 38.2% line; Rev labels posted with flag 7 (one Rev per candle with lsRevReadMarks); CobSignalLogic 1.10.0   // was "1.9.2"   // (1.9.2, Rassul 2026-11-19 11:13 CT "ok" - coordinate the candle tags: the Cob / Lvp / Rev labels as drawn are posted to SignalBoard kind K so lsRevReadMarks keeps its tags off them; nothing drawn changes)   // was "1.9.1";   // (1.9.1, Rassul 2026-11-18 19:54 CT "get rid of the accumulation and distribution boxes": the boxes are no longer read or drawn - CS_ACCUM_BOXES false; lsRevRead still keeps the zones in the background)   // was "1.9.0";   // (1.9.0) the CoB sweep = a 3-hour level OR a named key level   // was "1.8.3";   // (1.8.3, 2026-11-16 Rassul 10:54 CT) draws lsRevRead 0.9.0's accumulation / distribution boxes on the price chart (outline + the absorbed price)   // was "1.8.2";   // (1.8.2) the 3-hour S / R lines: one line each across the chart, no past steps   // was "1.8.1";   // (1.8.1) review fixes: the status text is built at most once a second; a REV line is keyed by its bar AND its CoB   // was "1.8.0";   // (1.8.0) + the CoB pullback Rev ("Rev" on the bar, REV record lines)   // was "1.7.1":   // (1.7.1) the CoB window opens at the 5:00 PM session start (overnight CoBs)   // was "1.7.0":   // (1.7.0) the sweep = the 3-hour S1 / S2 / R1 / R2 (spring / upthrust), the four lines drawn   // was "1.6.5":   // (1.6.5) the CoB entry line at the 38.2% retracement (was 50%)   // (1.6.0) the CoB = a completed 3-bar ZigZag wave from a swing that swept a key level
// was "1.5.1";   // (1.5.1) CQG roots mapped (cobs::mktKey) - it never ran on his CQG charts before   // (1.5.0) + the CoB Fib lines (targets 1.272 / 1.618 / 2.0, entry 50%, stop 0)   // = cobs::VERSION = the setVersion literal

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

static long long csKey(const std::string& l);
static long long csKeyOf(const std::string& l) { return csKey(l); }
struct CSState {
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; bool keyed = false, mktOk = false;
    cobs::Engine E; std::vector<RTDATE> raw; int off = 0;
    std::string recPath, statusPath, livePath, recWhy = "not opened"; std::set<long long> written, writtenLvp, writtenZzo, writtenRev; int recFails = 0; std::vector<std::string> pend;   /* (1.12.0) lines of CoBs whose ZZO count is still open */   // (1.4.0) + writtenZzo (ZZO lines)
    std::vector<acc::Zone> acZ; long long acMs = -100000; std::string acSig, acPath; int acDrawn = 0;   // (1.8.3) the zones lsRevRead saved (RevRead\\accum-<MKT>-<spb>.txt), looked at every 2 s
    long long statusMs = -1, statusCk = -1; std::string statusBody; size_t liveN = (size_t)-1; int drawn = 0;   // statusCk (1.8.1) = when the text was last built
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
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); CobSignal* me = static_cast<CobSignal*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

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
        S.written.clear(); S.writtenLvp.clear(); S.writtenZzo.clear(); S.writtenRev.clear(); S.recWhy = d.empty() ? "no USERPROFILE" : "new";
        std::ifstream f(S.recPath.c_str(), std::ios::binary);
        if (f) { std::string l; int n = 0; while (std::getline(f, l) && n < 200000) { ++n; const bool isLvp = l.rfind("LVP,", 0) == 0, isZzo = l.rfind("ZZO,", 0) == 0, isRev = l.rfind("REV,", 0) == 0; if (isRev) { char* e2 = nullptr; const long long t2 = std::strtoll(l.c_str() + 4, &e2, 10); if (e2 && *e2 == ',' && t2 > 0) S.writtenRev.insert(csKeyOf(l)); continue; } if (!isLvp && !isZzo && l.rfind("COB,", 0) != 0) continue; char* e = nullptr; const long long t = std::strtoll(l.c_str() + 4, &e, 10); if (e && *e == ',' && t > 0) (isLvp ? S.writtenLvp : isZzo ? S.writtenZzo : S.written).insert(t); } S.recWhy = "loaded " + std::to_string(S.written.size()) + " CoBs, " + std::to_string(S.writtenLvp.size()) + " LVPs"; }
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

// (1.8.1) a record line's key: its bar time; a REV line also carries its CoB (two CoBs can give a Rev on one bar)
static long long csKey(const std::string& l) {
    char* e = nullptr; const long long t = std::strtoll(l.c_str() + 4, &e, 10);
    if (l.rfind("REV,", 0) != 0) return t;
    long long cob = 0; const size_t a = l.find(',', 4), b = a == std::string::npos ? a : l.find(',', a + 1);
    if (b != std::string::npos) cob = std::strtoll(l.c_str() + b + 1, &e, 10);
    return t * 131 + (cob % 131 + 131) % 131;
}
void CobSignal::flush(CSState& S) {
    if ((!S.E.out.empty() || !S.E.lvpOut.empty() || !S.pend.empty()) && !S.recPath.empty()) {
        // (1.12.0) only CoBs that reached ZZO 10 are written; lines of a CoB still counting wait in S.pend; the rest are dropped
        std::vector<std::string> all = S.pend, pass, lvpPass; all.insert(all.end(), S.E.out.begin(), S.E.out.end()); all.insert(all.end(), S.E.lvpOut.begin(), S.E.lvpOut.end());
        S.pend.clear(); cobs::gate(all, S.E.marks, pass, S.pend); if (S.pend.size() > 2000) S.pend.erase(S.pend.begin(), S.pend.begin() + (long)(S.pend.size() - 2000));
        S.E.out.clear(); S.E.lvpOut.clear();
        for (const std::string& l : pass) (l.rfind("LVP,", 0) == 0 ? S.E.lvpOut : S.E.out).push_back(l);
        std::vector<std::string> fresh;
        for (const std::string& l : S.E.out) { const long long t = csKey(l); if (!(l.rfind("ZZO,", 0) == 0 ? S.writtenZzo : l.rfind("REV,", 0) == 0 ? S.writtenRev : S.written).count(t)) fresh.push_back(l); }   // (1.4.0) ZZO lines keyed apart
        for (const std::string& l : S.E.lvpOut) { char* e = nullptr; const long long t = std::strtoll(l.c_str() + 4, &e, 10); if (!S.writtenLvp.count(t)) fresh.push_back(l); }   // (1.3.0)
        if (!fresh.empty()) {
            const std::string d = csFlex(); csMkdir(d); csMkdir(d + CS_SEP "CobSignal");
            const bool isNew = !csExists(S.recPath);
            std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
            if (f) { if (isNew) f << "# lsCobSignal: COB,bar end (local secs),BULL/BEAR,label,levels swept,dratio,LOD/HOD,ADR,leg start,leg end,R since,V since,SHOW / LOG (1.1.0) | (1.3.0) LVP,pullback pivot bar end,BULL/BEAR (the CoB),CoB bar end,pullback first bar end,confirmed bar end,pullback volume,CoB wave volume,pct,nth pullback,SHOW / LOG\n"; for (const std::string& l : fresh) f << l << "\n"; f.flush(); }
            if (f.good()) { for (const std::string& l : fresh) { (l.rfind("LVP,", 0) == 0 ? S.writtenLvp : l.rfind("ZZO,", 0) == 0 ? S.writtenZzo : l.rfind("REV,", 0) == 0 ? S.writtenRev : S.written).insert(csKey(l)); } S.recWhy = "appending"; }
            else { ++S.recFails; S.recWhy = "append failed " + std::to_string(S.recFails) + "x"; }
        }
        S.E.out.clear(); S.E.lvpOut.clear();
    }
    if (S.recPath.empty()) { S.E.out.clear(); S.E.lvpOut.clear(); }   // (1.3.0) no record path: keep the queues bounded
    if (S.E.marks.size() != S.liveN && !S.livePath.empty()) {          // every CoB on the loaded bars, for the other indicators
        std::string body = std::string("COBSIGNAL,") + CS_VERSION + "\n";
        for (const cobs::Mark& m : S.E.marks) if (m.show && m.zOk) body +=   /* (1.4.0) drawn = passed the ZZO wave filter */ S.E.recLine(m) + "\n";   // (1.1.0) the drawn CoBs only (LOG ones stay in the record)
        csMkdir(csFlex()); if (csPublish(S.livePath, body)) S.liveN = S.E.marks.size();
    }
}

void CobSignal::status(CSState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusCk >= 0 && now - S.statusCk < 1000) return;              // (1.8.1) built at most once a second (was: on every calc between writes)
    S.statusCk = now;
    const std::string body = cobs::statusText(S.E, S.sym, S.spb, S.recWhy) + "DRAWN," + std::to_string(S.drawn) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    csMkdir(csFlex()); csPublish(S.statusPath, body);
}

static const bool CS_ACCUM_BOXES = false;   // (1.9.1, Rassul 19:54 CT) the accumulation / distribution boxes are gone from the price chart
void CobSignal::pump() {
    CSState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); flush(*S); status(*S, csMs());
    if (CS_ACCUM_BOXES) {   // (1.8.3) lsRevRead's zones: read (bounded) only when the file's size / time changed; never from the draw - (1.9.1) off
        const long long now = csMs(); if (now - S->acMs >= 2000 || now < S->acMs) { S->acMs = now;
            const std::string d = csFlex(); const std::string p = d.empty() || S->E.mkt.empty() ? std::string() : d + CS_SEP "RevRead" CS_SEP "accum-" + S->E.mkt + "-" + std::to_string(S->spb) + ".txt";
            if (p != S->acPath) { S->acPath = p; S->acSig.clear(); S->acZ.clear(); }
            struct stat st; if (p.empty() || stat(p.c_str(), &st) != 0) { if (!S->acSig.empty()) { S->acSig.clear(); S->acZ.clear(); invalidateChart(false); } }
            else { const std::string sig = std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime);
                if (sig != S->acSig) { std::ifstream f(p.c_str(), std::ios::binary); if (f) { std::string txt(262144, '\0'); f.read(&txt[0], (std::streamsize)txt.size()); txt.resize((size_t)f.gcount());
                        if (txt.size() < 262144) { S->acSig = sig; S->acZ = acc::parseFile(txt); invalidateChart(false); } } } } } }
}

static short csC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int CobSignal::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    CSState* S = slot_.get(this, false);
    if (!S || (S->E.marks.empty() && S->E.hs.empty() && S->acZ.empty())) return RTX_OK;   // draw never reads bars
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
        S->acDrawn = 0;
        if (!S->acZ.empty() && !S->raw.empty()) {   // (1.8.3) the accumulation / distribution boxes, under everything else: an outline (amber = undecided, green / red = confirmed) + one line at the absorbed price
            const std::vector<RTDATE>& R = S->raw; const int nR = (int)R.size(); const double tk = S->tick;
            auto idx = [&](long long t) { int a = 0, b = nR - 1; while (a < b) { const int m = (a + b) / 2; if ((long long)R[(size_t)m] < t) a = m + 1; else b = m; } return a; };
            const int i0 = std::max(0, std::min(nR - 1, vs - off)), i1 = std::max(0, std::min(nR - 1, ve - off));
            auto X = [&](long long t) { return (int)std::lround(v.X(idx(t))); }; auto Y = [&](long long ticks) { return (int)std::lround(v.Y((double)ticks * tk)); };
            const int half = std::max(2, (int)getPixelsPerBar() / 2);
            for (const acc::Box& b : acc::boxes(S->acZ, (long long)R[(size_t)i0], (long long)R[(size_t)i1], S->spb, X, Y, half)) {
                const int x0 = std::max((int)pane.left, b.x0), x1 = std::min((int)pane.right, b.x1), y0 = std::max((int)pane.top, b.y0 - 2), y1 = std::min((int)pane.bottom, b.y1 + 2); if (x1 <= x0 || y1 <= y0) continue;
                setPen((COLOR)b.col, 1, P_SOLID);
                auto ln = [&](int ax, int ay, int bx, int by) { PNT p0; p0.set(0, 0.0f); p0.h = csC(ax); p0.v = csC(ay); p0.setDrawPosition(); PNT p1; p1.set(0, 0.0f); p1.h = csC(bx); p1.v = csC(by); p1.drawLineTo(); };
                ln(x0, y0, x1, y0); ln(x1, y0, x1, y1); ln(x1, y1, x0, y1); ln(x0, y1, x0, y0);
                if (b.yAbs > y0 && b.yAbs < y1) { setPen((COLOR)b.col, 2, P_SOLID); ln(x0, b.yAbs, x1, b.yAbs); }
                ++S->acDrawn; }
        }
        std::vector<cobs::Prim> P = cobs::htfLayout(S->E, v);           // (1.7.0) the 3-hour R1 / R2 / S1 / S2 lines first
        { const std::vector<cobs::Prim> Q = cobs::fibLayout(S->E, v); P.insert(P.end(), Q.begin(), Q.end()); }   // (1.5.0) the Fib lines (under the labels)
        const size_t labFrom = P.size();                                  // (1.9.2) the candle labels (Cob..., Lvp, Rev) start here
        { const std::vector<cobs::Prim> Q = cobs::layout(S->E, v); P.insert(P.end(), Q.begin(), Q.end()); }
        S->drawn = (int)P.size();
        std::vector<sb::Ev> board;                                        // (1.9.2) SignalBoard 'K': each candle label as drawn
        FONT fs; fs.id = HELVETICA; fs.size = 8; fs.style = PLAIN;
        for (const cobs::Prim& p : P) {
            if (p.line) {                                                  // (1.5.0) 1 px solid, clipped to the pane
                if (p.y0 < pane.top || p.y0 > pane.bottom) continue;
                const int x0 = std::max((int)pane.left, p.x0), x1 = std::min((int)pane.right, p.x1); if (x1 <= x0) continue;
                setPen((COLOR)p.col, 1, P_SOLID);
                PNT a; a.set(0, 0.0f); a.h = csC(x0); a.v = csC(p.y0); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = csC(x1); b.v = csC(p.y0); b.drawLineTo();
                continue;
            }
            if (p.y0 < pane.top || p.y1 > pane.bottom || p.x0 < pane.left || p.x1 > pane.right) continue;
            if (p.badge) {   // (1.10.0) a badge: the side's colour, black text (as the Reversal Read tags), the 2nd line small
                RCT bx; bx.set(csC(p.x0), csC(p.y0), csC(p.x1), csC(p.y1)); bx.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
                setTextColor((COLOR)0x000000); setFont(fb); RCT t1; t1.set(csC(p.x0), csC(p.y0), csC(p.x1), csC(p.y0 + cobs::BADGE_H1)); t1.drawText(p.text.c_str(), true, false);
                if (!p.text2.empty()) { setFont(fs); RCT t2; t2.set(csC(p.x0), csC(p.y0 + cobs::BADGE_H1 - 1), csC(p.x1), csC(p.y1)); t2.drawText(p.text2.c_str(), true, false); }
            } else {
            setFont(p.tiny ? fs : fb); setTextColor((COLOR)p.col);
            RCT r; r.set(csC(p.x0), csC(p.y0), csC(p.x1 + 2), csC(p.y1)); r.drawText(p.text.c_str(), false, false); }
            if ((size_t)(&p - &P[0]) >= labFrom && !p.tiny && board.size() < sb::CAP) {   // (1.9.2) its candle: the one centred under it
                const int xc = (p.x0 + p.x1) / 2, j0 = std::max(0, v.first), j1 = std::min(v.last, (int)S->E.B.size() - 1); int bj = -1; double bd = 1e9;
                for (int j = j0; j <= j1; ++j) { const double d = std::fabs(v.X(j) - xc); if (d < bd) { bd = d; bj = j; } }
                if (bj >= 0 && bd <= 3) { const cobs::Bar& cb = S->E.B[(size_t)bj]; sb::Ev e;
                    if (sb::tagEv(cb.t, (int)std::lround(v.Y(cb.h)), (int)std::lround(v.Y(cb.l)), p.x0, p.y0, p.x1, p.y1, e)) { e.flag = p.flag; board.push_back(e); } }   // (1.10.0) flag 7 = a Rev
            }
        }
        {   const char* sy = getSymbol(); const char* chl = getChartLabel();   // (1.9.2) posted every draw (fresh 60 s); none drawn = an empty list
            const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            sb::publish('K', sy ? sy : "", getSecondsPerBar(), chl ? chl : "", ms, board); }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    CobSignal* p = new CobSignal();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA CoB Signal v1.10.0 - the CoB = a COMPLETED 3-bar High/Low ZigZag wave whose origin swept a named key level (in RTH) or a 3-hour support or resistance (spring / upthrust): the origin is the low of the 3-hour down swing below S1 or S2 (bear: the high above R1 or R2), the level still standing (no 3-hour close beyond it), and the wave closes back through it. 3-hour bars from 5:00 PM CT, 1-bar ZigZag; R1 / S1 = the last pivot high / low, R2 / S2 = the higher / lower of the last two; the four lines are drawn. From the 5:00 PM session start to 11:00 AM CT, >= 1.9x the last 20 waves, >= 3 bars, room 0.1 ADR, ZZO Combo count >= 10. Cob, Cob+R, Cob+V; Fib lines (stop 0, entry 38.2%, BE, 1.272 / 1.618 / 2.0); Lvp = low-volume pullback after a drawn CoB. Rev = the CoB pullback Rev: the first bar that closes above the prior bar's high (below its low for a bear CoB) while the pullback has carried 0.5x of the CoB wave's volume or less; up to 3 Revs on 3 separate pullbacks, until a bar closes through the CoB origin. Decided at the bar close, never repainted.");
    p->setVersion("1.12.1");   // = CS_VERSION (gex-build reads this literal)
    return p;
}
