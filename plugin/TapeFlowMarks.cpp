/********************************************************************************
 *  TapeFlowMarks.cpp  --  Investor/RT RTX extension  lsTapeFlowMarks  (v1.0.0, 2026-10-09)
 *
 *  Rassul approved the TapeFlow 2.0.3 mockup: "do a sanity check and test then implement .. i want to see how it looks on a chart".
 *  The PRICE CHART half of TapeFlow's absorption signal. (1.1.0, Rassul: "just put A on top of the candle that was absorbed. The
 *  ratio I will know from the indicator below, otherwise it will be too cluttered on the price chart.") ONE letter just above the
 *  high of the candle where the absorption formed: "A?" until confirmed, then "A" in place - red = bearish (buyers absorbed),
 *  green = bullish (sellers absorbed). The multiple stays in the TapeFlow pane and header. Only bar-node A records are drawn; old
 *  2.0.0 / 2.0.1 lines (20-s method, no multiple) are skipped and counted in TapeFlowMarks.status-<MKT>.txt.
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

static const char* TFM_VERSION = "1.1.0";   // (1.1.0, TF 2.0.4) the letter "A?" / "A" just above the absorbed candle's high (no dash, no multiple); old non-node records skipped and counted; a status file written only on change
static const COLOR C_BUY  = 0x0022C55E;
static const COLOR C_SELL = 0x00EF4444;
static const COLOR C_GRAY = 0x0064748B;
static const int FONT_PT = 9;
static const int LETTER_GAP = 4;   // px between the candle's high and the letter
static const int LETTER_H = 14;    // the letter's box height

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
    struct File { std::string path; long long mtime = -1, size = -1; std::vector<tfl::Mark> marks; long skipped = 0; };
    File cur, prev;
    std::map<long long, tfl::Mark> byT;          // bar end -> the drawn A (absorption only)
    bool timerOn = false, timerRefused = false; int timerId = 0;
    time_t lastCheck = 0;
    long reads = 0, skipped = 0, stats = 0, statusWrites = 0;
    std::mutex mx;
    std::string statusBody;                       // (2.0.4) the status file is rewritten only when this changes                                // the timer (re-read) and draw never touch byT at the same time
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
    bool refresh(MarksState& s);
    void writeStatus(MarksState& s);                 // true = something changed (redraw); uses what identify() stored, never the call context
    // the letter centred on x with its BOTTOM at yBottom (so it never touches the candle's high below it)
    void textC(short x, short yBottom, const char* str, COLOR col)
    {
        FONT f; f.id = HELVETICA; f.size = (short)FONT_PT; f.style = BOLD; setFont(f); setTextColor(col);
        RCT rc; rc.set((short)(x - 12), (short)(yBottom - LETTER_H), (short)(x + 12), yBottom); rc.drawText(str, true, false);
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
        tfl::SignalBook bk; bk.nodeOnly = true; bk.loadText(o.str()); f.skipped = bk.skippedOld;   // (2.0.4) bar-node A only; old 20-s lines counted, never drawn                               // a half-written last line is simply not a record
        f.path = path; f.mtime = mt; f.size = sz; f.marks = bk.marks; s.reads++; changed = true;
    };
    load(s.cur, pathOf(s, s.sid));
    long long ps = s.sid - 1; std::string pp;
    for (; ps >= s.sid - 4; --ps) { long long a, b; pp = pathOf(s, ps); if (statFile(pp, a, b)) break; }
    load(s.prev, pp);
    if (changed) {
        s.byT.clear();
        writeStatus(s);
        for (const tfl::Mark& k : s.prev.marks) if (k.drawn()) s.byT[k.barT] = k;
        for (const tfl::Mark& k : s.cur.marks) if (k.drawn()) s.byT[k.barT] = k;
    }
    return changed;
}

// (2.0.4, audit #31) a small status file, written atomically and ONLY when its content changes (a file was re-read)
void TapeFlowMarks::writeStatus(MarksState& s)
{
    const std::string d = tfDir(); if (d.empty() || s.mkt.empty()) return;
    const size_t drawn = s.byT.size();
    std::ostringstream o;
    o << "VERSION," << TFM_VERSION << "\nMARKET," << s.mkt << "\nBAR_SECONDS," << s.spb << "\nSESSION," << s.sid
      << "\nRECORD," << s.cur.path << "," << s.cur.marks.size() << " signals\nPREVIOUS," << s.prev.path << "," << s.prev.marks.size() << " signals"
      << "\nDRAWN_A," << drawn << "\nOLD_RECORDS_SKIPPED," << (s.cur.skipped + s.prev.skipped) << " non-node A lines (2.0.0 / 2.0.1, no multiple) - never drawn"
      << "\nOVERSIZED_FILES_SKIPPED," << s.skipped << "\nREADS," << s.reads << "\n";
    const std::string body = o.str();
    if (body == s.statusBody) return;
    const std::string path = d.substr(0, d.size() - 9) + "\\TapeFlowMarks.status-" + s.mkt + (s.spb > 0 && s.spb != 180 ? "-" + std::to_string(s.spb) + "s" : std::string()) + ".txt", tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f.is_open()) return; f << body; f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return; } }
#if defined(_WIN32)
    if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { std::remove(tmp.c_str()); return; }
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { std::remove(tmp.c_str()); return; }
#endif
    s.statusBody = body; s.statusWrites++;
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
        const long n = getBarCount(); if (n <= 0) return RTX_OK;
        int b0 = 0, b1 = (int)n - 1;
        if (getVisibleBars(&b0, &b1) != RTX_OK) { b0 = std::max(0, (int)n - 200); b1 = (int)n - 1; }
        if (b1 >= n) b1 = (int)n - 1;
        if (b0 < 0) b0 = 0;
        RTARRAYI dt(barDateTime);
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        const long long firstT = s->byT.begin()->first, lastT = s->byT.rbegin()->first;
        RTARRAY hiA(barHigh);
        for (int i = b0; i <= b1; ++i) {
            const long long te = localSec((RTDATE)dt[i]);
            if (te < firstT || te > lastT) continue;
            auto it = s->byT.find(te); if (it == s->byT.end()) continue;
            const tfl::Mark& m = it->second;
            const float hi = hiA[i];
            if (!std::isfinite(hi) || !(hi > 0)) continue;
            // (2.0.4, Rassul: "just put A on top of the candle that was absorbed") the letter only - "A?" pending, "A" confirmed - just
            // ABOVE the candle's high (footprint signals go above the bar), red bearish / green bullish; the multiple stays in TapeFlow
            PNT p; p.set(i, hi, kBarCenter);
            const COLOR c = m.dir > 0 ? C_BUY : C_SELL;
            const char* label = m.question() ? "A?" : "A";
            textC(p.h, (short)(p.v - LETTER_GAP), label, c);
        }
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    TapeFlowMarks *p = new TapeFlowMarks();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA TapeFlow Marks: TapeFlow's absorption on the price chart - the letter A just above the candle that was absorbed, "
                      "'A?' until a later bar confirms it, then 'A' (red = buyers absorbed, green = sellers absorbed). The ratio is in the TapeFlow pane. Reads what TapeFlow<market> decided.");
    p->setVersion(TFM_VERSION);
    return p;
}
