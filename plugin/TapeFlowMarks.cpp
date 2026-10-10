/********************************************************************************
 *  TapeFlowMarks.cpp  --  Investor/RT RTX extension  lsTapeFlowMarks  (v1.0.0, 2026-10-09)
 *
 *  Rassul approved the TapeFlow 2.0.3 mockup: "do a sanity check and test then implement .. i want to see how it looks on a chart".
 *  The PRICE CHART half of TapeFlow's absorption signal: a short horizontal DASH at the node price across the candle that formed
 *  it, with a small "A? 3.6x" / "A 3.6x" label to its right - red = bearish (buyers absorbed), green = bullish (sellers absorbed).
 *
 *  ONE DLL for every market: the chart's symbol is mapped to the market the same way TapeFlow / ChartView do (EP -> ES, ENQ -> NQ,
 *  CLE -> CL, GCE -> GC, CPE -> HG, NGE -> NG, EU6 -> EU, micros included). It only READS what TapeFlow<MKT> decided:
 *  %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\TapeFlow\<MKT>-signals-<session>.csv (and the previous session's), so it draws
 *  exactly what the file says - the same no-repaint record (a '?' only ever becomes confirmed; nothing moves or disappears).
 *  A 2-s timer re-reads a file only when its size or time changed; a missing or half-written file is simply skipped. No settings,
 *  no outputs in the settings window. Nothing heavy: a file stat every 2 s, and the drawing touches only the visible bars.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "TapeFlowLogic.h"
#include "HostSlot.h"
#include <atomic>
#include <mutex>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>

static const char* TFM_VERSION = "1.0.0";
static const COLOR C_BUY  = 0x0022C55E;
static const COLOR C_SELL = 0x00EF4444;
static const COLOR C_GRAY = 0x0064748B;
static const int FONT_PT = 9;

// the market of a chart symbol (the same table as TapeFlow / ChartView; root first, then the full symbol)
static std::string marketOfOne(const std::string& in)
{
    std::string r; for (char c : in) r += (char)std::toupper((unsigned char)c);
    const auto has = [&](const char* key) { return r.find(key) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return "";
}
static std::string marketOf(const std::string& root, const std::string& sym) { std::string m = marketOfOne(root); return m.empty() ? marketOfOne(sym) : m; }
static double tickOf(const std::string& m)
{
    return m == "ES" || m == "NQ" ? 0.25 : m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 : m == "HG" ? 0.0005 : m == "EU" ? 0.00005 : 0.0;
}
static long long civil(int y, int mo, int d, int h, int mi, int s)
{
    y -= mo <= 2; long long era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (unsigned)(mo + (mo > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + h * 3600 + mi * 60 + s;
}
static bool statFile(const std::string& p, long long& mtime, long long& size)
{
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    mtime = (long long)st.st_mtime; size = (long long)st.st_size; return true;
}

struct MarksState {
    std::string mkt, root, sym;
    int spb = 0;
    long long sid = LLONG_MIN;
    std::string idMkt; int idSpb = 0; bool identified = false;   // what this chart IS (from its own calc / draw)
    struct File { std::string path; long long mtime = -1, size = -1; std::vector<tfl::Mark> marks; };
    File cur, prev;
    std::map<long long, tfl::Mark> byT;          // bar end -> the drawn A (absorption only)
    bool timerOn = false, timerRefused = false; int timerId = 0;
    time_t lastCheck = 0;
    long reads = 0, skipped = 0, stats = 0;
    double tick = 0;
    std::mutex mx;                                // the timer (re-read) and draw never touch byT at the same time
};
static int nextTimerId() { static std::atomic<int> next(8711); return next.fetch_add(1); }
static const long long MAX_FILE_BYTES = 4LL * 1024 * 1024;   // (audit #33) a signals file is ~30 KB a session; anything bigger is not read
// (audit #1) a timer is matched to its chart's state by id, whatever context IRT calls it in
static std::mutex& regMx() { static std::mutex m; return m; }
static std::map<int, MarksState*>& timerReg() { static std::map<int, MarksState*> m; return m; }

class TapeFlowMarks : public cppExtension {
public:
    HostSlot<MarksState> slot_;
    virtual int timer(RTX_EVENT* e);
    virtual int draw(void);
    virtual int parmsLoad(void)  { return RTX_OK; }
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    long long localSec(RTDATE d) { struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(d, &t); return civil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); }
    std::string tfDir() const { const char* up = getenv("USERPROFILE"); return up && *up ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\TapeFlow" : std::string(); }
    std::string pathOf(const MarksState& s, long long sid) const
    {
        std::string p = tfDir() + "\\" + s.mkt + "-signals-" + std::to_string(sid);
        if (s.spb > 0 && s.spb != 180) p += "-" + std::to_string(s.spb) + "s";
        return p + ".csv";
    }
    void identify(MarksState& s);                // the chart's market / bar size / tick - ONLY in the chart's own calls (calc / draw)
    bool refresh(MarksState& s);                 // true = something changed (redraw); uses what identify() stored, never the call context
    void textLJ(short x, short y, const char* str, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
        RCT rc; rc.set(x, (short)(yy - sz), (short)(x + 300), (short)(yy + sz)); rc.drawText(str, false, false);
    }
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void)
{
    TapeFlowMarks* me = static_cast<TapeFlowMarks*>(this);
    MarksState* s = me->slot_.get(this, false);
    if (s && s->timerOn) { destroyTimer(s->timerId); s->timerOn = false; }
    return RTX_OK;
}
int cppExtension::destroy(void)
{
    TapeFlowMarks* me = static_cast<TapeFlowMarks*>(this);
    MarksState* s = me->slot_.get(this, false);
    if (s && s->timerOn) { destroyTimer(s->timerId); s->timerOn = false; }
    if (s) { std::lock_guard<std::mutex> g(regMx()); timerReg().erase(s->timerId); }
    me->slot_.release(this);                                           // deletes this chart's state and calls setUserData(NULL)
    return RTX_OK;
}
int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(1);
    CPEN p(C_GRAY, 1, P_SOLID);
    setOutputParameter("marks", DRAW_INVISIBLE, &p, C_GRAY, OUTPUT_NO_UI);   // nothing in the settings window
    return RTX_OK;
}
int cppExtension::calc(int)
{
    TapeFlowMarks* me = static_cast<TapeFlowMarks*>(this);
    MarksState* s = me->slot_.get(this, true);
    if (!s) return RTX_OK;
    { std::lock_guard<std::mutex> g(s->mx); try { me->identify(*s); } catch (...) {} }
    if (!s->timerOn && !s->timerRefused) {
        s->timerId = nextTimerId();
        if (createTimer(s->timerId, 2000) == RTX_OK) { s->timerOn = true; std::lock_guard<std::mutex> g(regMx()); timerReg()[s->timerId] = s; }
        else s->timerRefused = true;                                       // refused: draw re-checks every 2 s
    }
    return RTX_OK;
}

// the chart's market / bar size / tick: asked of IRT only in this chart's own calc / draw (a timer may run in another chart's context)
void TapeFlowMarks::identify(MarksState& s)
{
    char rb[32] = {0}; const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    s.root = rs ? rs : ""; s.sym = sy ? sy : "";
    s.idMkt = marketOf(s.root, s.sym);
    s.idSpb = getSecondsPerBar();
    {   // (audit #24) the chart's own tick (SYM_TICKINCR), finite and positive; the market table only as the fallback
        const float pt = getProperty(SYM_TICKINCR); const double known = tickOf(s.idMkt);
        s.tick = (std::isfinite(pt) && pt > 0 && pt < 1000) ? ((known > 0 && std::fabs(pt - known) <= known * 1e-5) ? known : (double)pt) : known;
    }
    s.identified = true;
}
// the session; re-read a signals file only when its size or time changed
bool TapeFlowMarks::refresh(MarksState& s)
{
    if (!s.identified) return false;
    const std::string m = s.idMkt; const int spb = s.idSpb;
    const long long sid = tfl::sessionOf(localSec(currentDate()));
    bool changed = false;
    if (m != s.mkt || spb != s.spb || sid != s.sid) { s.mkt = m; s.spb = spb; s.sid = sid; s.cur = MarksState::File(); s.prev = MarksState::File(); changed = true; }
    if (s.mkt.empty() || tfDir().empty()) { if (!s.byT.empty()) { s.byT.clear(); changed = true; } return changed; }
    auto load = [&](MarksState::File& f, const std::string& path) {
        long long mt = -1, sz = -1; s.stats++;
        if (!statFile(path, mt, sz)) { if (!f.marks.empty() || f.path != path) { f = MarksState::File(); f.path = path; changed = true; } return; }
        if (path == f.path && mt == f.mtime && sz == f.size) return;           // unchanged: nothing read
        if (sz > MAX_FILE_BYTES) { s.skipped++; f.path = path; f.mtime = mt; f.size = sz; if (!f.marks.empty()) { f.marks.clear(); changed = true; } return; }
        std::ifstream in(path.c_str(), std::ios::binary); std::ostringstream o; o << in.rdbuf();
        tfl::SignalBook bk; bk.loadText(o.str());                               // a half-written last line is simply not a record
        f.path = path; f.mtime = mt; f.size = sz; f.marks = bk.marks; s.reads++; changed = true;
    };
    load(s.cur, pathOf(s, s.sid));
    long long ps = s.sid - 1; std::string pp;
    for (; ps >= s.sid - 4; --ps) { long long a, b; pp = pathOf(s, ps); if (statFile(pp, a, b)) break; }
    load(s.prev, pp);
    if (changed) {
        s.byT.clear();
        for (const tfl::Mark& k : s.prev.marks) if (k.drawn()) s.byT[k.barT] = k;
        for (const tfl::Mark& k : s.cur.marks) if (k.drawn()) s.byT[k.barT] = k;
    }
    return changed;
}

int TapeFlowMarks::timer(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    MarksState* s = nullptr;
    { std::lock_guard<std::mutex> g(regMx()); auto it = timerReg().find(e->v.timer.id); if (it != timerReg().end()) s = it->second; }
    if (!s) { s = slot_.get(this, false); if (!s || e->v.timer.id != s->timerId) return RTX_FAIL; }
    s->lastCheck = time(0);
    bool changed = false;
    try { std::lock_guard<std::mutex> g(s->mx); changed = refresh(*s); } catch (...) { changed = false; }
    if (changed) invalidateChart(false);
    return RTX_OK;
}

int TapeFlowMarks::draw(void)
{
    MarksState* s = slot_.get(this, true);
    if (!s) return RTX_OK;
    try {
        std::lock_guard<std::mutex> g(s->mx);
        identify(*s);                                                       // draw runs in this chart's context
        if (s->sid == LLONG_MIN || s->mkt != s->idMkt || s->spb != s->idSpb) { s->lastCheck = time(0); refresh(*s); }   // first draw / the chart switched symbol or bar size: at once
        else if (!s->timerOn && time(0) - s->lastCheck >= 2) { s->lastCheck = time(0); refresh(*s); }                    // no timer: the same 2-s rhythm
        if (s->byT.empty()) return RTX_OK;
        const double tick = s->tick; if (!(tick > 0) || !std::isfinite(tick)) return RTX_OK;   // (only for records written before 2.0.3 carried their tick)
        const long n = getBarCount(); if (n <= 0) return RTX_OK;
        int b0 = 0, b1 = (int)n - 1;
        if (getVisibleBars(&b0, &b1) != RTX_OK) { b0 = std::max(0, (int)n - 200); b1 = (int)n - 1; }
        if (b1 >= n) b1 = (int)n - 1;
        if (b0 < 0) b0 = 0;
        RTARRAYI dt(barDateTime);
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        const long long firstT = s->byT.begin()->first, lastT = s->byT.rbegin()->first;
        for (int i = b0; i <= b1; ++i) {
            const long long te = localSec((RTDATE)dt[i]);
            if (te < firstT || te > lastT) continue;
            auto it = s->byT.find(te); if (it == s->byT.end()) continue;
            const tfl::Mark& m = it->second;
            const float price = (float)(m.px * (m.tickSize > 0 ? m.tickSize : tick));   // the record's own tick (the deciding chart's); else this chart's
            PNT p; p.set(i, price, kBarCenter);
            const short half = (short)std::max(3, std::max(1, (int)(ppb * 0.9)) / 2);   // the candle's x-range: the same 90% span as TapeFlow's minute bars
            const COLOR c = m.dir > 0 ? C_BUY : C_SELL;
            setPen(c, 3, P_SOLID);                                              // the dash across the candle that formed it
            const int span = std::max(1, (int)(ppb * 0.9));
            PNT a; a.set(0, 0.0f); a.h = (short)(p.h - span / 2); a.v = p.v; a.setDrawPosition();
            PNT b; b.set(0, 0.0f); b.h = (short)(p.h - span / 2 + span - 1); b.v = p.v; b.drawLineTo();
            const short lx = (short)(p.h + half + 4);
            {   // a thin leader from the candle to the label: the mark stays visible when the node sits on a same-coloured body
                setPen(c, 1, P_SOLID);
                PNT a2; a2.set(0, 0.0f); a2.h = (short)(b.h + 1); a2.v = p.v; a2.setDrawPosition();
                PNT b2; b2.set(0, 0.0f); b2.h = (short)(lx - 2); b2.v = p.v; if (b2.h > a2.h) b2.drawLineTo();
            }
            const std::string label = tfl::markLabel(m);                       // "A? 3.6x" / "A 3.6x"
            textLJ(lx, (short)(p.v + FONT_PT / 2), label.c_str(), c, FONT_PT, true);
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    TapeFlowMarks *p = new TapeFlowMarks();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA TapeFlow Marks: TapeFlow's absorption on the price chart - a dash at the node price across the candle that formed it, "
                      "'A? 3.6x' until a bar closes away, then 'A 3.6x' (red = buyers absorbed, green = sellers absorbed). Reads what TapeFlow<market> decided.");
    p->setVersion(TFM_VERSION);
    return p;
}
