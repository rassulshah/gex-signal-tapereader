/********************************************************************************
 *  ChartView.cpp  --  Investor/RT RTX extension  lsChartView  (1.2.0, 2026-10-10)
 *
 *  CHART VIEW: lets Claude "see" inside an IRT chart. Put ONE on each chart (his 3-min LTF and 60-min charts for ES, NQ, CL,
 *  GC, HG, NG, 6E). It draws nothing and requests no data: it only reads what the chart already has. No settings (task #300:
 *  always on, always the last 300 bars - the settings window is empty).
 *
 *  Every new bar (and at most every 10 s while a bar is forming, and at once when ChartView.want.txt is touched) it writes,
 *  safely (tmp, old copy to .bak, rename), %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\ChartView\<MKT>_<seconds per bar>.json:
 *    header   version, written (ISO, local = Central), why (first / new bar / forming bar / request)
 *    chart    market, symbol, root, chart label, period, the last 300 bars (time, O, H, L, C, V)
 *    series   for each label in lsFlexLevels\ChartView.labels.txt: every output array 0, 1, ... it can read, the last 300 values
 *             aligned to the bars; the label exactly as listed, the legend parts, the name that worked; "lookup" says why a
 *             missing label failed
 *    files    the status / marks / level files our own plugins write or draw from for this market (64 KB each, cut at lines)
 *
 *  THE CAMERA (picture request): lsFlexLevels\ChartView.snap.txt holding "ES_180 <epoch>", "GC" or "*". The chart that matches
 *  takes a picture of ITS OWN window - same market AND same bar size - into lsFlexLevels\ChartView\img\<MKT>_<sec>.png and
 *  writes <MKT>_<sec>.status.txt (OK / BLANK / FAILED, the window title used, attempts). The picture is taken from this
 *  plugin's 2-s timer only, never inside the chart's paint; a blank price pane is retried (up to 3 tries) and never replaces
 *  the last good picture. Logic without the SDK: ChartViewLogic.h (tested).
 *
 *  1.2.0  window = market + bar size (no more same picture for 180 and 3600); capture outside the paint + blank retry;
 *         labels looked up by their legend name (CI[x] -> x, updn[y] -> y / updn), custom indicators calculated on a new
 *         bar, a "lookup" reason per missing label; no settings at all; per-chart caches (no DLL-wide state); .bak publish.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
// windows.h may define far as a legacy macro; DealerLogic.h (via ChartViewLogic.h) uses far as a member name
#ifdef far
#undef far
#endif
#include "ChartViewLogic.h"
#include "ChartViewCamera.h"           // the IRT-only chart camera (Windows)
#include "HostSlot.h"
#include <chrono>
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
#include <limits>
#include <new>
#include <sys/types.h>
#include <sys/stat.h>

#define CV_VERSION "1.2.0"          // keep equal to cvl::CV_VERSION_STR and the setVersion literal below

enum { CTX_CALC = 0, CTX_DRAW = 1, CTX_TIMER = 2 };

// a picture request being worked on: noticed in any callback, taken only from the timer (or calc if the timer was refused)
struct SnapJob {
    bool pending = false;
    int attempt = 0;
    long long nextMs = 0;
    long long startMs = 0;
    std::string request;
};
// one chart's schedule. IRT may run ONE object of this DLL for every chart (and HostSlot falls back to one shared state when
// the host keeps no user-data slot), so each chart is keyed by symbol | seconds per bar | chart label inside the state.
struct ChartSub {
    cvl::Sched sched;
    unsigned long long lastHash = 0;
    bool everWritten = false;
    long long wantSeen = -1;          // the want file's stamp this chart last answered (or ignored)
    long long wantCheckMs = -1;       // the want file is looked at most once a second per chart
    long writes = 0;
    long long snapSeen = -1;          // the picture-request file's stamp this chart last answered
    long long snapCheckMs = -1;
    SnapJob job;
    std::map<std::string, std::pair<long long, cvl::Series> > seriesCache;   // label -> (last bar stamp, last good read)
};
struct CachedFile { long long mtime = -1; unsigned long long size = 0; bool truncated = false; std::string text; };
struct WatchCache { long long stamp = -2; std::vector<std::string> labels; bool present = false; };
struct CVState {
    std::map<std::string, ChartSub> subs;
    std::map<std::string, CachedFile> files;     // (1.2.0) per chart, not DLL-wide: the copied files, re-read only on change
    WatchCache watch;                            // (1.2.0) per chart: the watch-list file
    bool busy = false;
    bool timerOn = false, timerTried = false;    // own 2-s clock: charts on hidden tabs keep updating; pictures are taken here
    long long fileReads = 0;                     // test counter (#31)
};

#ifdef CV_TEST_CLOCK
long long cvTestClockMs();                                         // the Linux SDK-mock test drives the clock
long long cvTestEpoch();
static long long steadyMs() { return cvTestClockMs(); }
static long long epochNow() { return cvTestEpoch(); }
#else
static long long steadyMs()
{
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static long long epochNow() { return (long long)time(nullptr); }
#endif
#ifdef CV_TEST_HOOKS
extern int (*cvTestRename)(const char* from, const char* to);       // the test makes a rename fail
#endif

static std::string flexDir()
{
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static int cvTimerId(const void* me) { return 7311 + (int)(((uintptr_t)me >> 4) % 100000); }
static bool localOf(time_t t, struct tm& out)
{
#if defined(_WIN32)
    return localtime_s(&out, &t) == 0;
#else
    return localtime_r(&t, &out) != nullptr;
#endif
}
static bool utcOf(time_t t, struct tm& out)
{
#if defined(_WIN32)
    return gmtime_s(&out, &t) == 0;
#else
    return gmtime_r(&t, &out) != nullptr;
#endif
}
static cvl::Civil civilOf(const struct tm& t)
{
    cvl::Civil c; c.y = t.tm_year + 1900; c.mo = t.tm_mon + 1; c.d = t.tm_mday; c.h = t.tm_hour; c.mi = t.tm_min; c.s = t.tm_sec; return c;
}
static std::string nowText()
{
    struct tm lt; memset(&lt, 0, sizeof(lt));
    if (!localOf((time_t)epochNow(), lt)) return std::string();
    return cvl::stamp(civilOf(lt));
}
// a short trace so a chart that never writes can be diagnosed: lsFlexLevels\ChartView.trace.txt (capped at 64 KB)
static void cvTrace(const std::string& line)
{
    try {
        const std::string d = flexDir(); if (d.empty()) return;
        const std::string p = d + "\\ChartView.trace.txt";
        FILE* f = std::fopen(p.c_str(), "ab"); if (!f) return;
        long sz = 0; if (std::fseek(f, 0, SEEK_END) == 0) sz = std::ftell(f);
        if (sz > 65536) { std::fclose(f); f = std::fopen(p.c_str(), "wb"); if (!f) return; }
        std::fprintf(f, "%s  %s\r\n", nowText().c_str(), line.c_str()); std::fclose(f);
    } catch (...) {}
}
static bool statFile(const std::string& p, long long& mtime, unsigned long long& size)
{
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    mtime = (long long)st.st_mtime; size = (unsigned long long)st.st_size;
    return true;
}
static bool readRange(const std::string& p, size_t from, size_t len, std::string& out)
{
    out.clear();
    if (len == 0) return true;
    std::ifstream f(p.c_str(), std::ios::binary);
    if (!f.is_open()) return false;
    f.seekg((std::streamoff)from, std::ios::beg);
    if (!f) return false;
    out.resize(len);
    f.read(&out[0], (std::streamsize)len);
    out.resize((size_t)f.gcount());
    return true;
}
static void makeDir(const std::string& p)
{
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool fileExists(const std::string& p) { long long m = 0; unsigned long long s = 0; return statFile(p, m, s); }
static bool moveOver(const std::string& from, const std::string& to)
{
#ifdef CV_TEST_HOOKS
    if (cvTestRename) return cvTestRename(from.c_str(), to.c_str()) == 0;
#endif
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// (1.2.0, checklist #5) write <path>.tmp, move the old file to <path>.bak, rename into place; on failure the .bak goes back.
// A reader never sees a partial file and the last good copy is never lost.
static bool publishFile(const std::string& path, const std::string& contents)
{
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    {
        std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(contents.data(), (std::streamsize)contents.size()); f.flush();
        if (!f) { f.close(); std::remove(tmp.c_str()); return false; }
    }
    const bool hadOld = fileExists(path);
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) {
        if (hadOld) moveOver(bak, path);
        std::remove(tmp.c_str());
        return false;
    }
    return true;
}
static std::string mtimeText(long long mt)
{
    struct tm t; memset(&t, 0, sizeof(t));
    if (!localOf((time_t)mt, t)) return std::string();
    return cvl::stamp(civilOf(t));
}

// ---- the camera, behind one seam so the Linux mock can drive it (CV_TEST_CAMERA)
struct CamShot { std::string err, title; std::vector<unsigned char> rgb; int w = 0, h = 0, score = 0, matches = 0; };
#if defined(CV_TEST_CAMERA)
CamShot cvTestShot(const std::string& sym, int spb, const std::string& per, unsigned pwFlags);
void cvTestRepaint(const std::string& sym, int spb);
static CamShot camShot(const std::string& sym, int spb, const std::string& per, unsigned f) { return cvTestShot(sym, spb, per, f); }
static void camRepaint(const std::string& sym, int spb, const std::string&) { cvTestRepaint(sym, spb); }
#elif defined(_WIN32)
static CamShot camShot(const std::string& sym, int spb, const std::string& per, unsigned pwFlags)
{
    CamShot s;
    std::vector<cvl::WinCand> c; std::vector<HWND> hw;
    cvcam::listWindows(c, hw);
    cvl::Pick p = cvl::pickChart(c, sym, spb, per, 0, 0);
    s.matches = p.matches;
    if (p.index < 0) { s.err = "chart window not found: " + p.why + " (" + std::to_string(p.seen) + " IRT windows looked at)"; return s; }
    s.title = c[(size_t)p.index].title; s.score = p.score;
    s.err = cvcam::grab(hw[(size_t)p.index], pwFlags, s.rgb, &s.w, &s.h);
    return s;
}
static void camRepaint(const std::string& sym, int spb, const std::string& per)
{
    std::vector<cvl::WinCand> c; std::vector<HWND> hw;
    cvcam::listWindows(c, hw);
    cvl::Pick p = cvl::pickChart(c, sym, spb, per, 0, 0);
    if (p.index >= 0) cvcam::askRepaint(hw[(size_t)p.index]);
}
#else
static CamShot camShot(const std::string&, int, const std::string&, unsigned) { CamShot s; s.err = "the camera runs on Windows only"; return s; }
static void camRepaint(const std::string&, int, const std::string&) {}
#endif

class ChartView : public cppExtension {
public:
    ChartView() : cppExtension() {}
    virtual int draw(void);
    virtual int timer(RTX_EVENT* e);
    virtual int parmsLoad(void)  { return RTX_OK; }          // no settings: nothing to load, nothing ever saved (#29)
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    void pump(int ctx);
    void release() { CVState* S = slot_.get(this, false); if (S && S->timerOn) { destroyTimer(cvTimerId(S)); S->timerOn = false; } slot_.release(this); }
    CVState* testState() { return slot_.get(this, false); }
private:
    HostSlot<CVState> slot_;
    bool readSeries(const std::string& label, long n, long from, bool mayCalc, cvl::Series& out, std::string& diag);
    void readWatch(CVState& S, const std::string& dir, std::vector<std::string>& labels, cvl::Snap& s);
    void readFiles(CVState& S, const std::string& dir, cvl::Snap& s);
    bool wantNow(const std::string& dir, ChartSub& C, const std::string& mkt, int period, long long nowMs);
    bool snapNow(const std::string& dir, ChartSub& C, const std::string& mkt, int period, long long nowMs, std::string& text);
    void runSnap(const std::string& dir, ChartSub& C, const std::string& mkt, int spb, const std::string& sym, long long nowMs);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<ChartView*>(this)->release(); return RTX_OK; }   // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<ChartView*>(this)->release(); return RTX_OK; }
int cppExtension::calc(int)     { static_cast<ChartView*>(this)->pump(CTX_CALC); return RTX_OK; }
int ChartView::draw(void)       { pump(CTX_DRAW); return RTX_OK; }       // draws nothing: draw() only gives a second chance to run
int ChartView::timer(RTX_EVENT* e)
{
    CVState* S = slot_.get(this, false);
    if (!S || !e || e->v.timer.id != cvTimerId(S)) return RTX_FAIL;
    pump(CTX_TIMER); return RTX_OK;
}

int cppExtension::setup(void)
{
    // (1.2.0, task #300) no settings at all: always on, always the last 300 bars, the watch list is ChartView.labels.txt.
    // Version 3: IRT drops the values a 1.0.x / 1.1.x chart saved (Enabled, Bars, Extra labels).
    setParameterVersion(3);
    return RTX_OK;
}

// ---- one label: each lookup name, arrays 0, 1, ... until one fails (array 0 also as the default array -1); last the RTL
// custom indicator by name (calculated by IRT only from calc on a new bar / request, like DealerProfile's bar file).
bool ChartView::readSeries(const std::string& label, long n, long from, bool mayCalc, cvl::Series& out, std::string& diag)
{
    out = cvl::Series(); out.label = label;
    const long want = n - from;
    auto take = [&](RTARRAY& a, int k) {
        if (out.values.empty()) {
            out.sdkName = cvl::fixedField(a.info.name, sizeof(a.info.name));
            bool cut = false; out.sdkTextLabel = cvl::fixedField(a.info.textLabel, sizeof(a.info.textLabel), &cut); out.sdkTextCut = cut;
        }
        std::vector<double> vals; vals.reserve((size_t)want);
        const long shift = n - (long)a.count;                     // the indicator's array ends on the chart's last bar
        for (long i = from; i < n; i++) {
            long j = i - shift;
            double v = (j >= 0 && j < (long)a.count) ? (double)a[(int)j] : std::numeric_limits<double>::quiet_NaN();
            vals.push_back(std::isfinite(v) ? v : std::numeric_limits<double>::quiet_NaN());
        }
        out.arrayNo.push_back(k); out.values.push_back(vals);
    };
    std::vector<std::string> names = cvl::lookupNames(label);
    for (size_t v = 0; v < names.size(); v++) {
        char nb[96]; memset(nb, 0, sizeof(nb)); strncpy(nb, names[v].c_str(), sizeof(nb) - 1);
        for (int k = 0; k < cvl::MAX_ARRAYS; k++) {
            RTARRAY a(fEmptyArray);
            if (a.getChartIndicator(nb, k) == RTX_OK && a.count > 0) { take(a, k); continue; }
            if (k == 0) {                                          // some indicators answer only for the default array
                RTARRAY d(fEmptyArray);
                if (d.getChartIndicator(nb) == RTX_OK && d.count > 0) { take(d, 0); continue; }
            }
            break;
        }
        if (!out.values.empty()) { out.foundAs = names[v]; out.via = "chart"; return true; }
        diag += (diag.empty() ? "" : "; ") + std::string("chart '") + names[v] + "' not found";
    }
    const std::string cn = cvl::customName(label);
    if (!cn.empty()) {
        char buf[96]; memset(buf, 0, sizeof(buf)); strncpy(buf, cn.c_str(), sizeof(buf) - 1);
        RTARRAY a(fEmptyArray);
        if (a.getCustomIndicator(buf, 1, mayCalc) == RTX_OK && a.count > 0) { take(a, 0); out.foundAs = cn; out.via = mayCalc ? "custom" : "custom (not recalculated)"; return true; }
        diag += std::string("; custom '") + cn + "' not found" + (mayCalc ? "" : " (not recalculated here: tried again on the next new bar)");
    }
    return false;
}

void ChartView::readWatch(CVState& S, const std::string& dir, std::vector<std::string>& labels, cvl::Snap& s)
{
    const std::string p = dir + "\\ChartView.labels.txt";
    s.watchFile = p;
    long long mt = -1; unsigned long long sz = 0;
    long long stamp = statFile(p, mt, sz) ? mt * 1000003LL + (long long)sz : -1;
    if (stamp != S.watch.stamp) {
        S.watch.stamp = stamp; S.watch.labels.clear(); S.watch.present = stamp >= 0;
        if (stamp >= 0 && sz <= 65536) { std::string t; if (readRange(p, 0, (size_t)sz, t)) { S.fileReads++; S.watch.labels = cvl::parseLabelFile(t); } }
        else if (stamp >= 0) s.notes.push_back("ChartView.labels.txt is over 64 KB - ignored");
    }
    if (!S.watch.present) s.notes.push_back("ChartView.labels.txt not found");
    labels = S.watch.labels;
}

void ChartView::readFiles(CVState& S, const std::string& dir, cvl::Snap& s)
{
    struct tm lt; memset(&lt, 0, sizeof(lt)); localOf((time_t)epochNow(), lt);
    const long long today = cvl::daysFromCivil(lt.tm_year + 1900, (unsigned)(lt.tm_mon + 1), (unsigned)lt.tm_mday);
    std::vector<cvl::Src> src = cvl::sourcesFor(s.market, s.symbol, today);
    size_t total = 0;
    if (S.files.size() > 160) S.files.clear();                     // bounded: a few dozen paths per market in practice
    for (size_t k = 0; k < src.size(); k++) {
        const std::string p = dir + "\\" + src[k].rel;
        long long mt = -1; unsigned long long sz = 0;
        if (!statFile(p, mt, sz)) { S.files.erase(p); continue; }       // not written by that plugin (yet): simply absent
        CachedFile& C = S.files[p];
        if (C.mtime != mt || C.size != sz) {
            cvl::Plan pl = cvl::planRead((size_t)sz, src[k].cut);
            std::string head, tail;
            bool ok = readRange(p, 0, pl.headLen, head) && readRange(p, pl.tailFrom, (size_t)sz - pl.tailFrom, tail);
            S.fileReads++;
            if (!ok) { S.files.erase(p); s.filesSkipped.push_back(src[k].rel + " (read failed)"); continue; }
            C.text = cvl::joinCut(head, tail, pl, (size_t)sz); C.truncated = pl.truncated; C.mtime = mt; C.size = sz;
        }
        if (total + C.text.size() > cvl::FILES_TOTAL_CAP) { s.filesSkipped.push_back(src[k].rel + " (total size cap)"); continue; }
        total += C.text.size();
        cvl::FileBlob b; b.name = src[k].rel; b.bytes = C.size; b.mtime = mtimeText(C.mtime); b.truncated = C.truncated; b.text = C.text;
        s.files.push_back(b);
    }
}

// ChartView.want.txt: answered once per change of the file (it is never deleted here - every matching chart answers it)
bool ChartView::wantNow(const std::string& dir, ChartSub& C, const std::string& mkt, int period, long long nowMs)
{
    if (C.wantCheckMs >= 0 && nowMs - C.wantCheckMs < 1000 && nowMs >= C.wantCheckMs) return false;
    C.wantCheckMs = nowMs;
    const std::string p = dir + "\\ChartView.want.txt";
    long long mt = -1; unsigned long long sz = 0;
    if (!statFile(p, mt, sz)) return false;
    const long long stamp = mt * 1000003LL + (long long)sz;
    if (stamp == C.wantSeen) return false;
    C.wantSeen = stamp;
    std::string t; if (sz > 4096 || !readRange(p, 0, (size_t)sz, t)) t.clear();
    return cvl::wantMatches(t, mkt, period);
}

// a picture request: lsFlexLevels\ChartView.snap.txt holding "GC", "GC_180", "ES_180 1791603738" or "*". Looked at once a
// second per chart, answered once per change; too old = ignored (its epoch > 120 s off, or no epoch and > 30 s old at load).
bool ChartView::snapNow(const std::string& dir, ChartSub& C, const std::string& mkt, int period, long long nowMs, std::string& text)
{
    if (C.snapCheckMs >= 0 && nowMs - C.snapCheckMs < 1000 && nowMs >= C.snapCheckMs) return false;
    C.snapCheckMs = nowMs;
    const std::string p = dir + "\\ChartView.snap.txt";
    long long mt = -1; unsigned long long sz = 0;
    if (!statFile(p, mt, sz)) return false;
    const long long stamp = mt * 1000003LL + (long long)sz;
    if (stamp == C.snapSeen) return false;
    const bool first = C.snapSeen == -1;
    C.snapSeen = stamp;
    std::string t; if (sz > 4096 || !readRange(p, 0, (size_t)sz, t)) t.clear();
    cvl::Request r = cvl::parseRequest(t);
    if (!cvl::requestFresh(r, epochNow(), mt, first)) return false;
    text = cvl::trim(t);
    return cvl::requestMatches(r, mkt, period);
}

// one attempt at the picture (from the timer). Blank -> a repaint is queued and the next attempt waits for a later tick.
void ChartView::runSnap(const std::string& dir, ChartSub& C, const std::string& mkt, int spb, const std::string& sym, long long nowMs)
{
    SnapJob& J = C.job;
    const char* pl = getPeriodicityLabel();
    const std::string per = pl ? pl : "";
    const std::string stem = cvl::fileStem(mkt, spb);
    const std::string imgDir = dir + "\\ChartView\\img";
    makeDir(dir + "\\ChartView"); makeDir(imgDir);
    CamShot shot;
    try { shot = camShot(sym, spb, per, cvl::printFlags(J.attempt)); } catch (...) { shot = CamShot(); shot.err = "capture threw"; }
    cvl::Blank bl; if (shot.err.empty()) bl = cvl::blankCheck(shot.rgb.data(), shot.w, shot.h);
    const int next = cvl::afterCapture(shot.err.empty(), bl.blank, J.attempt);
    cvl::SnapStatus st; st.request = J.request; st.window = shot.title; st.score = shot.score; st.matches = shot.matches;
    st.w = shot.w; st.h = shot.h; st.attempts = J.attempt + 1; st.blankFrac = shot.err.empty() ? bl.frac : -1; st.when = nowText();
    st.ms = nowMs - J.startMs;
    if (next == cvl::RETRY) {
        J.attempt++; J.nextMs = nowMs + cvl::SNAP_RETRY_MS;
        if (shot.err.empty()) camRepaint(sym, spb, per);
        cvTrace("picture " + stem + " try " + std::to_string(J.attempt) + ": " + (shot.err.empty() ? "blank price pane (" + std::to_string(bl.frac) + "), retrying" : shot.err + ", retrying"));
        return;
    }
    J.pending = false;
    std::string err;
    if (next == cvl::SAVE) {
        const std::string png = cvl::pngEncode(shot.rgb.data(), shot.w, shot.h);
        if (png.empty()) err = "PNG encode failed";
        else if (!publishFile(imgDir + "\\" + stem + ".png", png)) err = "PNG write failed";
        st.result = err.empty() ? "OK" : "FAILED"; st.reason = err;
    } else if (shot.err.empty()) {                                 // blank after every try: keep the last good picture
        const std::string png = cvl::pngEncode(shot.rgb.data(), shot.w, shot.h);
        if (!png.empty()) publishFile(imgDir + "\\" + stem + ".blank.png", png);
        st.result = "BLANK"; st.reason = "the price pane stayed blank after " + std::to_string(st.attempts) + " tries; " + stem + ".png is the last good picture";
    } else { st.result = "FAILED"; st.reason = shot.err; }
    publishFile(imgDir + "\\" + stem + ".status.txt", cvl::snapStatusText(st));
    cvTrace("picture " + stem + " (" + sym + " " + std::to_string(spb) + " s, window \"" + shot.title + "\"): " + st.result
            + (st.reason.empty() ? "" : " - " + st.reason) + ", " + std::to_string(st.w) + "x" + std::to_string(st.h) + ", tries " + std::to_string(st.attempts));
}

void ChartView::pump(int ctx)
{
    CVState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = true;
    struct Unbusy { CVState* s; ~Unbusy() { s->busy = false; } } unbusy = { S };
    try {
        if (!S->timerTried) {                                       // ask once for the 2-s clock
            S->timerTried = true;
            S->timerOn = createTimer(cvTimerId(S), 2000) == RTX_OK;
            cvTrace(std::string("loaded ") + CV_VERSION + (S->timerOn ? " - timer granted" : " - timer REFUSED, pictures from calc"));
        }
        const long n = getBarCount();
        if (n < 2) return;
        RTARRAYI dt(barDateTime);
        const long long lastT = (long long)dt[(int)n - 1];
        char rb[32]; memset(rb, 0, sizeof(rb));
        const char* rs = getRootSymbol(rb);
        const char* sy = getSymbol();
        const char* cl = getChartLabel();
        const std::string root = rs ? rs : "", sym = sy ? sy : "", chart = cl ? cl : "";
        const int spb = getSecondsPerBar();
        const std::string key = sym + "|" + std::to_string(spb) + "|" + chart;
        if (S->subs.size() > 64 && !S->subs.count(key)) S->subs.clear();
        ChartSub& C = S->subs[key];
        const long long nowMs = steadyMs();
        const std::string dir = flexDir();
        if (dir.empty()) return;
        const std::string mkt = cvl::marketOf(root, sym);
        const int per = spb > 0 ? spb : 0;
        bool want = wantNow(dir, C, mkt, per, nowMs);
        std::string req;
        if (snapNow(dir, C, mkt, per, nowMs, req)) {               // noticed here, taken below only outside the chart's paint
            C.job = SnapJob(); C.job.pending = true; C.job.nextMs = nowMs; C.job.startMs = nowMs; C.job.request = req;
            want = true;                                           // a fresh data snapshot goes with the picture
        }
        if (C.job.pending && nowMs >= C.job.nextMs && (ctx == CTX_TIMER || (ctx == CTX_CALC && !S->timerOn)))
            runSnap(dir, C, mkt, per, sym, nowMs);

        const int why = cvl::due(C.sched, n, lastT, nowMs, want);
        if (why == cvl::NONE) return;
        cvl::built(C.sched, n, lastT, nowMs);                     // a failed build waits for the next new bar / 10 s, never loops

        // ---- the snapshot
        cvl::Snap s;
        s.market = mkt; s.symbol = sym; s.root = root; s.chart = chart; s.chartBars = n;
        { const char* pl = getPeriodicityLabel(); s.periodicity = pl ? pl : ""; }
        const int N = cvl::FIXED_BARS;                              // (task #300) fixed: no Bars setting
        s.barsWanted = N;
        const long from = n > N ? n - N : 0;
        RTARRAY o(barOpen), h(barHigh), l(barLow), c(barClose);
        RTARRAYI v(barVolume);
        std::vector<long long> secs; secs.reserve((size_t)(n - from));
        for (long i = from; i < n; i++) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[(int)i], &t);
            cvl::Civil cv = civilOf(t);
            s.t.push_back(cvl::stamp(cv)); secs.push_back(cvl::secsOf(cv));
            s.o.push_back(o[(int)i]); s.h.push_back(h[(int)i]); s.l.push_back(l[(int)i]); s.c.push_back(c[(int)i]); s.v.push_back((double)(long long)v[(int)i]);
        }
        s.period = cvl::periodOf(spb, secs);
        if (spb <= 0) s.notes.push_back("seconds per bar from the bar stamps");

        std::vector<std::string> labels;
        readWatch(*S, dir, labels, s);
        s.watchFromFile = (int)labels.size();
        const bool mayCalc = ctx == CTX_CALC && (why == cvl::FIRST || why == cvl::NEWBAR || why == cvl::WANT);
        if (C.seriesCache.size() > (size_t)cvl::MAX_LABELS * 2) C.seriesCache.clear();
        for (size_t k = 0; k < labels.size(); k++) {
            cvl::Series x; std::string diag;
            if (readSeries(labels[k], n, from, mayCalc, x, diag)) { s.series.push_back(x); C.seriesCache[labels[k]] = std::make_pair(lastT, x); continue; }
            auto it = C.seriesCache.find(labels[k]);
            if (it != C.seriesCache.end() && it->second.first == lastT) {          // read on this same bar from calc: reuse it
                cvl::Series y = it->second.second; y.via += " +cached"; s.series.push_back(y); continue;
            }
            s.missing.push_back(labels[k]);
            s.lookup.push_back(labels[k] + ": " + diag);
        }
        readFiles(*S, dir, s);

        // ---- write only when it changed (a new bar, a request and the first snapshot always)
        const std::string b = cvl::body(s);
        const unsigned long long hsh = cvl::fnv1a(b);
        if (!cvl::shouldWrite(why, hsh, C.lastHash, C.everWritten)) return;
        const time_t now = (time_t)epochNow(); struct tm lt, ut; memset(&lt, 0, sizeof(lt)); memset(&ut, 0, sizeof(ut));
        localOf(now, lt); utcOf(now, ut);
        const cvl::Civil lc = civilOf(lt);
        const std::string doc = cvl::document(b, cvl::iso(lc, cvl::utcOffsetMin(lc, civilOf(ut))), (long long)now, why);
        const std::string outDir = dir + "\\ChartView";
        makeDir(outDir);
        const std::string stem = cvl::fileStem(mkt, s.period);
        if (publishFile(outDir + "\\" + stem + ".json", doc)) {
            if (!C.everWritten) cvTrace("first snapshot " + stem + " (" + key + ")");
            C.lastHash = hsh; C.everWritten = true; C.writes++;
        } else if (C.writes == 0) cvTrace("write FAILED " + stem + " (" + key + ")");
    } catch (...) {
        // never let a file / string failure reach IRT; the next new bar or 10 s tries again
    }
}

extern "C" cppExtension *CreateExtension(void)
{
    ChartView *p = new ChartView();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | CALC_LAST);
    p->setDescription("LRA ChartView: writes this chart's bars and indicator values to lsFlexLevels\\ChartView for Claude, and takes a picture of this chart on request. Draws nothing; no settings.");
    p->setVersion("1.2.0");   // CV_VERSION
    return p;
}
