/********************************************************************************
 *  EventCal.cpp  --  Investor/RT RTX extension  lsEventCal  "Event Calendar"  1.0.0  (2026-11-17)
 *
 *  Rassul 2026-11-17 18:09 CT: "the calendar at the top center of important events that are coming up for the market that im trading
 *  ... for crude it could include inventories. other events are like fomc etc ... after 30 min of the even it can be removed";
 *  18:22 "i only want market moving events medium and high impact ... be carefull with the font size and spacing"; 18:25 "build".
 *  On the PRICE pane: a compact box at the top centre (beside the HOD / LOD table when that table is on the chart, never over it,
 *  never right of the last bar): the next 3 medium / high impact events of the chart's market within 2 days, one line each
 *  ("Wed 1:00 PM  FOMC Minutes  18h 35m"); high impact bold, medium plain; amber inside the last hour, red "out 11m" for 30 min after
 *  the release, then gone. No title row, no settings, no hovers. Logic: EventCalLogic.h (SDK-free, Linux-tested).
 *  INPUT  lsFlexLevels\LRA-Calendar-ALL.csv (the nightly lra.event_calendar, which also scores each event type's real impact per
 *         market and drops / demotes the ones that do not move it); missing / stale = the built-in list.
 *  FILE   lsFlexLevels\EventCal.status-<MKT>-<spb>.txt   version, source, place, the rows shown
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
#include "EventCalLogic.h"
#include "BoxDock.h"
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
#include <sys/stat.h>
#include <vector>

#if defined(_WIN32)
#define EC_SEP "\\"
#else
#define EC_SEP "/"
#endif
static const long long EC_MAX_FILE = 256 * 1024;

static long long ecMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string ecFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + EC_SEP "InvestorRT" EC_SEP "rtx" EC_SEP "lsFlexLevels" : std::string(); }
static bool ecPublish(const std::string& path, const std::string& text) {
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
// bounded, torn-safe read: the stamp before and after must match, else keep the last good copy
static bool ecRead(const std::string& path, std::string& out, long long& stamp) {
    struct stat a; if (stat(path.c_str(), &a) != 0) return false;
    if ((long long)a.st_size <= 0 || (long long)a.st_size > EC_MAX_FILE) return false;
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return false;
    std::string s((size_t)a.st_size, '\0'); f.read(&s[0], (std::streamsize)s.size()); if ((long long)f.gcount() != (long long)a.st_size) return false;
    struct stat b; if (stat(path.c_str(), &b) != 0 || b.st_mtime != a.st_mtime || b.st_size != a.st_size) return false;
    out.swap(s); stamp = (long long)a.st_mtime * 1000000LL + (long long)a.st_size; return true;
}

struct ECState {
    std::string mkt, sym, chart; int spb = 0;
    ecal::File file; long long fileStamp = -1; long long checkMs = -1; std::string fileWhy = "not read yet";
    bdk::TableWatch hl;
    std::string statusPath, statusBody; long long statusMs = -1;
};

class EventCal : public cppExtension {
public:
    SS_SAFE_DIALOG(EventCal, "EventCal")       // the chart-less Add / settings copy never calls into the chart
    EventCal() : cppExtension() {}
    virtual int parmsLoadBody(void)  { return RTX_OK; }
    virtual int parmsApplyBody(void) { return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int) { return RTX_OK; }
    virtual int draw(void);
    HostSlot<ECState> slot_;
    void clear() { slot_.release(this); }
    bool identify(ECState& S);
    void loadFile(ECState& S, long long ms);
    void status(ECState& S, long long ms, const std::string& body);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::setup(void) { setParameterVersion(1); return RTX_OK; }
int cppExtension::done(void) { static_cast<EventCal*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<EventCal*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { return RTX_OK; }   // nothing per bar: the box depends only on the clock and the calendar file

bool EventCal::identify(ECState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb); std::string root = rs ? rs : "";
    for (char& c : root) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) c = '_';
    const char* sy = getSymbol(); const char* cl = getChartLabel();
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    if (root.empty()) return false;
    const std::string mkt = mkey::norm(root);
    if (mkt != S.mkt || spb != S.spb) {
        S.mkt = mkt; S.spb = spb; S.statusBody.clear(); S.statusMs = -1;
        const std::string d = ecFlex();
        S.statusPath = d.empty() ? std::string() : d + EC_SEP "EventCal.status-" + mkt + "-" + std::to_string(spb) + ".txt";
    }
    S.sym = sy ? sy : ""; S.chart = cl ? cl : "";
    return true;
}

void EventCal::loadFile(ECState& S, long long ms) {      // at most every 10 s; re-parsed only when the stamp changes
    if (S.checkMs >= 0 && ms - S.checkMs < 10000) return;
    S.checkMs = ms;
    const std::string d = ecFlex(); if (d.empty()) { S.file = ecal::File(); S.fileWhy = "no USERPROFILE"; return; }
    const std::string p = d + EC_SEP "LRA-Calendar-ALL.csv";
    struct stat a; if (stat(p.c_str(), &a) != 0) { S.file = ecal::File(); S.fileStamp = -1; S.fileWhy = "no file"; return; }
    const long long st = (long long)a.st_mtime * 1000000LL + (long long)a.st_size;
    if (st == S.fileStamp) return;
    std::string text; long long stamp = -1;
    if (!ecRead(p, text, stamp)) { S.fileWhy = "read failed (kept the last good copy)"; return; }
    ecal::File F = ecal::parse(text);
    if (F.ok || !S.file.ok) S.file = F;                   // a garbled file never replaces a good one
    S.fileStamp = stamp; S.fileWhy = F.ok ? "ok" : F.why;
}

void EventCal::status(ECState& S, long long ms, const std::string& body) {
    if (S.statusPath.empty() || body == S.statusBody) return;
    if (S.statusMs >= 0 && ms - S.statusMs < 30000 && S.statusBody.size() == body.size()) return;   // countdowns alone: at most every 30 s
    S.statusMs = ms; S.statusBody = body; ecPublish(S.statusPath, body);
}

static short ecC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

int EventCal::draw(void) {
    ECState* S = slot_.get(this, true);
    if (!S) return RTX_OK;
    try {
        if (!identify(*S)) return RTX_OK;
        const long long ms = ecMs();
        loadFile(*S, ms);
        const std::time_t tt = std::time(nullptr); struct tm lt; std::memset(&lt, 0, sizeof lt);
#if defined(_WIN32)
        if (localtime_s(&lt, &tt) != 0) return RTX_OK;
#else
        if (!localtime_r(&tt, &lt)) return RTX_OK;
#endif
        const long long now = ecal::minAbs(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min);   // the PC's clock (Central)
        std::string srcWhy; const std::vector<ecal::Ev> src = ecal::source(S->file, S->mkt, now, &srcWhy);
        if (!S->file.ok && S->fileWhy != "ok" && srcWhy.find(S->fileWhy) == std::string::npos) srcWhy += " [" + S->fileWhy + "]";
        const std::vector<ecal::Row> R = ecal::select(src, S->mkt, now);
        std::string placeWhy = "nothing to show";
        if (!R.empty()) {
            FONT fb; fb.id = HELVETICA; fb.size = 9; fb.style = BOLD;
            FONT fn; fn.id = HELVETICA; fn.size = 9; fn.style = PLAIN;
            const ecal::Layout L = ecal::layout(R, [this, &fb, &fn](const std::string& s, bool b) { setFont(b ? fb : fn); return (int)getTextWidth(s.c_str(), -1) + 1; });
            RCT pane; pane.getPaneRect(false);
            int right = pane.right;
            const long n = getBarCount();
            if (n > 0) { PNT p; p.set((int)n - 1, 0.0f, kBarRight); if (p.h > pane.left && p.h < pane.right) right = p.h; }   // his rule: only up to the last bar
            const bdk::Box area = bdk::mk(pane.left, pane.top + 16, right, pane.bottom);   // below the IRT title row
            S->hl.refresh(S->sym.c_str(), (long)S->spb, S->chart.c_str());
            const bdk::Result P = bdk::place(S->hl.on, S->hl.box, L.w, L.h, area, 2, std::vector<bdk::Box>());   // 2 = top centre
            placeWhy = P.why;
            if (P.mode != bdk::NONE) {
                const bdk::Box& B = P.b;
                RCT bg; bg.set(ecC(B.l), ecC(B.t), ecC(B.r), ecC(B.b)); bg.draw(1, (COLOR)ecal::C_EDGE, (COLOR)ecal::C_BG, DRAW_OPAQUE, PAT_SOLID);
                for (const ecal::Cell& c : L.cells) {
                    setFont(c.bold ? fb : fn); setTextColor((COLOR)c.col);
                    RCT r; r.set(ecC(B.l + c.x), ecC(B.t + c.y), ecC(B.l + c.x + c.w), ecC(B.t + c.y + c.h));
                    r.drawText(c.text.c_str(), false, c.right);
                }
            }
        }
        status(*S, ms, ecal::statusText(S->mkt, srcWhy, R, placeWhy));
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    EventCal* p = new EventCal();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI);       // on the price pane, on the instrument's scale; no hovers, no settings
    p->setDescription("LRA Event Calendar v1.0.0 - on the price pane: the next 3 medium / high impact scheduled events of this chart's market within 2 days (FOMC, CPI, NFP, GDP + PCE, retail sales, EIA crude for CL, EIA gas for NG, ECB for 6E, China PMI for HG, jobless claims, PPI, ISM, API ...), at the top centre beside the HOD / LOD table. High impact bold; amber within the hour; red for 30 min after the release, then removed. The nightly keeps the list and drops event types that do not move the market. No settings.");
    p->setVersion("1.0.0");   // = ecal::VERSION (gex-build reads this literal)
    return p;
}
