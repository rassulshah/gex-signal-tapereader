/********************************************************************************
 *  VolRead.cpp  --  Investor/RT RTX extension  lsVolRead "Volume Read" / lsRangeRead "Range Read" (RangeRead.cpp = this file + VR_RANGE)  1.3.0  (2026-11-16: 1.3.0 = both panes mark notable bars - CB / SR (Cob Reader), Bk / Sw of a key, volume or 3-hour level, Rev, Ct, P, biggest in 50+ (Rassul 05:55-06:18 CT; rules in VolRangeLogic.h); 1.2.0 = the read box follows the mouse (Rassul 05:49 CT "the values in the infobox should change as my mouse hovers on different bars in the pane"): the bar under the mouse while it is in the pane, the last closed bar otherwise; 1.1.1 = review fixes (status text built once a second; lsBreakout file names by its own key); 1.1.0 = Range pane highlight of BO / PB Rev / CoB Rev bars; 1.0.0 2026-11-15)
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
#include "PaneCursor.h"          // (1.2.0) is the mouse inside THIS pane right now (IRT sends no event when it leaves)
#include "MarketKey.h"
#include "HostSlot.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

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

static const char* VR_VERSION = "1.3.0";   // (1.1.0) the Range pane highlights BO (>= 1.2x ATR), PB Rev and CoB Rev bars   // = vrr::VERSION = the setVersion literal
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
    std::string statusPath, statusBody; long long statusMs = -1, statusCk = -1; int drawn = 0;   // statusCk (1.1.1) = when the text was last built
    int hov = -1; pc::Tracker cur;                     // (1.2.0) the chart bar under the mouse (-1 = not in this pane) and where the pane sits on the screen
    long long hlMs = -1; std::string hlSig;            // (1.1.0) when the signal files were last looked at / their stamps
    long long lvMs = -1; std::string lvSig;            // (1.3.0) the same for the level files and the Cob Reader status
};

class VolRead : public cppExtension {
public:
    SS_SAFE_DIALOG(VolRead, VR_NAME)             // the chart-less Add / settings copy never calls into the chart
    VolRead() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* ev);
    void mouseLeft(VRState& S);
    HostSlot<VRState> slot_;
    void clear() { slot_.release(this); }
    void pump();
    bool identify(VRState& S);
    void feed(VRState& S);
    void status(VRState& S, long long now);
    long long localSec(RTDATE d);
    void signals(VRState& S, long long now);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<VolRead*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<VolRead*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { VolRead* me = static_cast<VolRead*>(this); try { me->pump(); } catch (...) {} return RTX_OK; }

long long VolRead::localSec(RTDATE d) {              // as lsCobSignal stamps its records: the chart's local wall clock in seconds
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

// (1.1.0) the Range pane's signal bars from his own indicators' records (read here in calc, never in draw; at most every 5 s and only when a
// file's size / time changed; bounded reads): lsBreakout's breakouts + pb files of the last 10 days and lsCobSignal's record (REV lines)
static bool vrSlurp(const std::string& path, size_t cap, std::string& out) {
    std::ifstream f(path.c_str(), std::ios::binary | std::ios::ate); if (!f) return false; std::streamoff sz = (std::streamoff)f.tellg(); if (sz < 0) return false;
    const std::streamoff from = sz > (std::streamoff)cap ? sz - (std::streamoff)cap : 0; f.seekg(from); out.resize((size_t)(sz - from)); if (!out.empty()) f.read(&out[0], (std::streamsize)out.size()); out.resize((size_t)f.gcount()); return true;
}
static void vrMarks(VRState& S, long long now);
void VolRead::signals(VRState& S, long long now) {
    S.E.hlOn = true;                                     // (1.3.0) both panes
    vrMarks(S, now);
    if (S.hlMs >= 0 && now - S.hlMs < 5000) return;
    S.hlMs = now;
    const std::string d = vrFlex(); if (d.empty() || S.E.LT.empty()) return;
    const std::string mk = mkey::norm(S.mkt), base = d + VR_SEP "Breakout" VR_SEP + vrr::boMarket(S.mkt) + "-" + vrr::boFileKey(S.sym) + "-" + std::to_string(S.spb) + "-";   // (1.1.1) lsBreakout's own file key
    const long day = (long)(S.E.LT.back() / 86400);
    std::vector<std::string> files; for (long k = day - 9; k <= day + 1; ++k) { files.push_back(base + "breakouts-" + std::to_string(k) + ".csv"); files.push_back(base + "pb-" + std::to_string(k) + ".csv"); }
    const std::string cob = d + VR_SEP "CobSignal" VR_SEP + mk + "-" + std::to_string(S.spb) + ".csv"; files.push_back(cob);
    std::string sig; for (const std::string& p : files) { struct stat st; if (stat(p.c_str(), &st) == 0) { sig += std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime) + ";"; } else sig += "-;"; }
    if (sig == S.hlSig) return;
    S.hlSig = sig;
    std::map<long long, vrr::Hl> raw, loc; std::string txt;
    for (size_t i = 0; i + 1 < files.size(); ++i) { if (!vrSlurp(files[i], 512 * 1024, txt)) continue; if (i % 2 == 0) vrr::parseBo(txt, raw); else vrr::parsePbRev(txt, raw); }
    if (vrSlurp(cob, 4 * 1024 * 1024, txt)) vrr::parseCobRev(txt, loc);
    S.E.hlRaw.swap(raw); S.E.hlLocal.swap(loc);
}
// (1.3.0) the levels (lsLimitProfile's LRA-KeyLevels-<MKT>-hist.csv = one line per past session, LRA-KeyLevels-<MKT>.csv = the running session) and
// the Cob Reader pane's status (CK lines: the CB / SR bars of its last Cobs). Looked at every 5 s, read (bounded) only when a size / time changed.
static void vrMarks(VRState& S, long long now) {
    if (S.lvMs >= 0 && now - S.lvMs < 5000) return;
    S.lvMs = now;
    const std::string d = vrFlex(); if (d.empty() || S.E.LT.empty()) return;
    const std::string mk = mkey::norm(S.mkt);
    const std::string fh = d + VR_SEP "LRA-KeyLevels-" + mk + "-hist.csv", fl = d + VR_SEP "LRA-KeyLevels-" + mk + ".csv", fc = d + VR_SEP "ZzoCombo.status-" + S.mkt + "-" + std::to_string(S.spb) + ".txt";
    const long long day = vrr::sessionDay(S.E.LT.back());
    std::string sig = std::to_string(day) + ";"; for (const std::string* p : { &fh, &fl, &fc }) { struct stat st; if (stat(p->c_str(), &st) == 0) sig += std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime) + ";"; else sig += "-;"; }
    if (sig == S.lvSig) return;
    S.lvSig = sig;
    std::string txt; std::map<long long, std::vector<double>> days;
    if (vrSlurp(fh, 1024 * 1024, txt)) vrr::parseLvHist(txt, days);
    if (vrSlurp(fl, 64 * 1024, txt)) { long long at = 0; std::vector<double> live; const long long wall = (long long)std::time(nullptr);
        if (vrr::parseLvLive(txt, at, live) && !live.empty() && wall - at <= 6 * 3600 && at - wall <= 600) days[day] = live; }   // the running session: the live levels win
    for (const auto& kv : S.E.lvDay) if (!days.count(kv.first)) days[kv.first] = kv.second;                                     // a day once known stays known
    S.E.lvDay.swap(days);
    std::vector<vrr::CobIt> cobs; if (vrSlurp(fc, 256 * 1024, txt)) vrr::parseCk(txt, cobs);
    if (!cobs.empty()) S.E.setCobs(cobs);
}

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
        S.E.reset(); S.E.mode = VR_MODE; S.E.tick = tick; S.E.spb = spb; S.E.lvDay.clear(); S.E.cobs.clear(); S.lvSig.clear(); S.lvMs = -1; S.raw.clear(); S.off = 0; S.haveForm = false;
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
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) { S.E.addBar(bar(i), localSec((RTDATE)times[i])); S.raw.push_back((RTDATE)times[i]); }
    S.form = bar(closed); S.haveForm = true;
}

void VolRead::status(VRState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusCk >= 0 && now - S.statusCk < 1000) return;                 // (1.1.1) the text is built at most once a second (was: on every calc between writes)
    S.statusCk = now;
    const std::string body = vrr::statusText(S.E, mkey::norm(S.mkt), S.sym, S.spb) + "HOST," + VR_VERSION + "\nDRAWN," + std::to_string(S.drawn) + "\n";
    if (body == S.statusBody) { S.statusMs = now; return; }                // unchanged: no write
    S.statusMs = now; S.statusBody = body;
    vrPublish(S.statusPath, body);
}

void VolRead::pump() {
    VRState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    feed(*S); signals(*S, vrMs()); S->E.markLevels(); S->E.cobMarks(); status(*S, vrMs());   // (1.3.0) the marks of the new closed bars, after the files were looked at
    mouseLeft(*S);
}

// (1.2.0) the mouse: IRT sends move events only while the cursor is over this pane (TRACK_MOUSE: also when the indicator is not selected).
// The event gives the chart bar it points at; where the pane sits on the screen is remembered so calc / draw can tell when the cursor
// has left (no event comes then) - the box goes back to the last closed bar. The event is never swallowed.
#if defined(_WIN32)
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#endif
static void* vrCursorWindow(int sx, int sy) { POINT p; p.x = sx; p.y = sy; return (void*)WindowFromPoint(p); }
#else
static void* vrCursorWindow(int, int) { return nullptr; }
#endif
int VolRead::mouse(RTX_EVENT* ev) {
    if (!ev || (ev->type != E_MOUSE_MOVE && ev->type != E_MOUSE_HOVER)) return RTX_FAIL;
    VRState* S = slot_.get(this, false); if (!S || !S->keyed) return RTX_FAIL;
    try {
        int sx = 0, sy = 0; const bool ok = pc::cursorPos(sx, sy);
        S->cur.onEvent(ok ? vrCursorWindow(sx, sy) : nullptr, ok, sx, sy, (int)ev->v.mouse.h, (int)ev->v.mouse.v);
        const long n = getBarCount(); const long b = (long)ev->v.mouse.bar;
        const int hov = (ok && b >= 0 && b < n && S->cur.eventInside((int)ev->v.mouse.h, (int)ev->v.mouse.v)) ? (int)b : -1;
        if (hov != S->hov) { S->hov = hov; invalidateChart(false); }
    } catch (...) {}
    return RTX_FAIL;
}
void VolRead::mouseLeft(VRState& S) {
    if (S.hov < 0) return;
    int sx = 0, sy = 0; const bool ok = pc::cursorPos(sx, sy);
    if (!S.cur.inside(ok ? vrCursorWindow(sx, sy) : nullptr, ok, sx, sy)) { S.hov = -1; invalidateChart(false); }
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
        S->cur.pane.l = pane.left; S->cur.pane.t = pane.top; S->cur.pane.r = pane.right; S->cur.pane.b = pane.bottom;   // (1.2.0) for mouseLeft
        const int pick = S->hov >= 0 ? S->hov - S->off : -1;               // the bar under the mouse as an engine index (the forming bar = the bar count)
        const std::vector<vrr::Prim> P = vrr::layout(S->E, v, S->haveForm ? &S->form : nullptr, pick);
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
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);   // (1.2.0) + the mouse picks the bar the read box is about                                    // its own pane, draws itself; no hovers, no settings
#ifdef VR_RANGE
    p->setDescription("LRA Range Read v1.3.0 - its own pane: each bar's range (high - low) in grey; a marked bar is green (bullish) or red (bearish) with one short code: CB = the Cob bar near the origin, SR = the stop-run bar into the Cob origin, Bk / Sw = a break / sweep of a key, volume or 3-hour level on a bar that is the biggest in 5 or more, Rev = a PB Rev or CoB pullback Rev bar, Ct = a counter bar bigger than every bar since the Cob origin, P = a ZigZag pivot bar that is the biggest in 20 or more; no code: the biggest in 50 or more bars, a breakout candle of 1.2x ATR(13) or more. BRMax over every closed bar that is the biggest in 20 or more bars, and a read box that follows the mouse. Decided at the close of each bar, never repainted. No settings.");
#else
    p->setDescription("LRA Volume Read v1.3.0 - its own pane: each bar's volume in grey; a marked bar is green (bullish) or red (bearish) with one short code: CB = the Cob bar near the origin, SR = the stop-run bar into the Cob origin, Bk / Sw = a break / sweep of a key, volume or 3-hour level on a bar that is the biggest in 5 or more, Rev = a PB Rev or CoB pullback Rev bar, Ct = a counter bar bigger than every bar since the Cob origin, P = a ZigZag pivot bar that is the biggest in 20 or more; no code: the biggest in 50 or more bars. BVMax over every closed bar that is the biggest in 20 or more bars, and a read box that follows the mouse. Decided at the close of each bar, never repainted. No settings.");
#endif
    p->setVersion("1.3.0");   // = VR_VERSION (gex-build reads this literal)
    return p;
}
