/********************************************************************************
 *  ChartView.cpp  --  Investor/RT RTX extension  lsChartView  (1.0.0, 2026-10-09)
 *
 *  CHART VIEW: lets Claude "see" inside an IRT chart without a screenshot. Put ONE on each chart (his 3-min LTF and 60-min
 *  charts for ES, NQ, CL, GC, HG, NG, 6E). It draws nothing and requests no data: it only reads what the chart already has.
 *
 *  Every new bar (and at most every 10 s while a bar is forming, and at once when ChartView.want.txt is touched) it writes,
 *  atomically (tmp + rename), %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\ChartView\<MKT>_<seconds per bar>.json:
 *    header   version, written (ISO, local = Central), why (first / new bar / forming bar / request)
 *    chart    market, symbol, root, chart label, period, the last N bars (time, O, H, L, C, V; setting, default 300)
 *    series   for each label in the watch list: every output array 0, 1, ... it can read (stops at the first that fails), the
 *             last N values aligned to the bars, with IRT's name / text label for the indicator; labels not found -> "missing"
 *    files    the status / marks / level files our own plugins write or draw from for this market (64 KB each, cut at lines)
 *  Watch list: lsFlexLevels\ChartView.labels.txt (one chart label per line, # = comment; re-read when it changes) plus the
 *  chart's own "Extra labels" setting (comma separated).
 *  A forming-bar rebuild that changed nothing is not written. Logic without the SDK: ChartViewLogic.h (tested).
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

#define CV_VERSION "1.0.0"          // keep equal to cvl::CV_VERSION_STR and the setVersion literal below

struct CVIdx { int enabled, bars, extra; };
static CVIdx CX = { -1, -1, -1 };

// one chart's schedule. IRT may run ONE object of this DLL for every chart (and HostSlot falls back to one shared state when
// the host keeps no user-data slot), so each chart is keyed by symbol | seconds per bar | chart label inside the state.
struct ChartSub {
    cvl::Sched sched;
    unsigned long long lastHash = 0;
    bool everWritten = false;
    long long wantSeen = -1;          // the want file's stamp this chart last answered (or ignored)
    long long wantCheckMs = -1;       // the want file is looked at most once a second per chart
    long writes = 0;
};
struct CVState {
    std::map<std::string, ChartSub> subs;
    bool busy = false;
};

// DLL-wide caches (IRT calls extensions on its UI thread): the watch-list file and the copied files, re-read only on change
struct CachedFile { long long mtime = -1; unsigned long long size = 0; bool truncated = false; std::string text; };
static std::map<std::string, CachedFile> g_files;
struct WatchCache { long long stamp = -2; std::vector<std::string> labels; bool present = false; };
static WatchCache g_watch;

#ifdef CV_TEST_CLOCK
long long cvTestClockMs();                                         // the Linux SDK-mock test drives the clock
static long long steadyMs() { return cvTestClockMs(); }
#else
static long long steadyMs()
{
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif
static std::string flexDir()
{
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
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
static bool atomicWrite(const std::string& path, const std::string& contents)
{
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(contents.data(), (std::streamsize)contents.size()); f.flush();
        if (!f) { f.close(); std::remove(tmp.c_str()); return false; }
    }
#if defined(_WIN32)
    if (MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#else
    if (std::rename(tmp.c_str(), path.c_str()) == 0) return true;
#endif
    std::remove(tmp.c_str());
    return false;
}
static cvl::Civil civilOf(const struct tm& t)
{
    cvl::Civil c; c.y = t.tm_year + 1900; c.mo = t.tm_mon + 1; c.d = t.tm_mday; c.h = t.tm_hour; c.mi = t.tm_min; c.s = t.tm_sec; return c;
}
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
static std::string mtimeText(long long mt)
{
    struct tm t; memset(&t, 0, sizeof(t));
    if (!localOf((time_t)mt, t)) return std::string();
    return cvl::stamp(civilOf(t));
}
static std::string fixedStr(const char* p, size_t cap)        // INDICATOR_INFO strings may not be terminated
{
    if (!p) return std::string();
    size_t n = 0; while (n < cap && p[n]) n++;
    return std::string(p, n);
}

class ChartView : public cppExtension {
public:
    ChartView() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoad(void)  { return RTX_OK; }
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    void pump();
    void release() { slot_.release(this); }
private:
    HostSlot<CVState> slot_;
    bool readSeries(const std::string& label, long n, long from, cvl::Series& out);
    void readWatch(const std::string& dir, std::vector<std::string>& labels, cvl::Snap& s);
    void readFiles(const std::string& dir, cvl::Snap& s);
    bool wantNow(const std::string& dir, ChartSub& C, const std::string& mkt, int period, long long nowMs);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<ChartView*>(this)->release(); return RTX_OK; }   // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<ChartView*>(this)->release(); return RTX_OK; }
int cppExtension::calc(int)     { static_cast<ChartView*>(this)->pump(); return RTX_OK; }
int ChartView::draw(void)       { pump(); return RTX_OK; }    // draws nothing: draw() only gives a second chance to run

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(3);
    int pc = 0;
    CX.enabled = pc++; setBoolParameter("Enabled", true);
    CX.bars    = pc++; setIntegerParameter("Bars", cvl::DEFAULT_BARS, NUMW);
    CX.extra   = pc++; setStringParameter("Extra labels", "", 240);
    return RTX_OK;
}

// ---- one label: arrays 0, 1, ... until one fails; "CI[x]" also tried as "x"; last resort the RTL custom indicator x
bool ChartView::readSeries(const std::string& label, long n, long from, cvl::Series& out)
{
    out = cvl::Series(); out.label = label;
    const long want = n - from;
    std::vector<std::string> names = cvl::labelVariants(label);
    for (size_t v = 0; v < names.size(); v++) {
        for (int k = 0; k < cvl::MAX_ARRAYS; k++) {
            RTARRAY a(fEmptyArray);
            if (a.getChartIndicator(names[v].c_str(), k) != RTX_OK || a.count <= 0) break;
            if (k == 0) { out.name = fixedStr(a.info.name, sizeof(a.info.name)); out.textLabel = fixedStr(a.info.textLabel, sizeof(a.info.textLabel)); }
            std::vector<double> vals; vals.reserve((size_t)want);
            const long shift = n - a.count;                       // the indicator's array ends on the chart's last bar
            for (long i = from; i < n; i++) {
                long j = i - shift;
                vals.push_back(j >= 0 && j < a.count ? (double)a[(int)j] : std::numeric_limits<double>::quiet_NaN());
            }
            out.arrayNo.push_back(k); out.values.push_back(vals);
        }
        if (!out.values.empty()) { out.via = v == 0 ? "chart" : "chart:" + names[v]; return true; }
    }
    const std::string cn = cvl::customName(label);
    if (!cn.empty()) {
        char buf[128]; memset(buf, 0, sizeof(buf));
        strncpy(buf, cn.c_str(), sizeof(buf) - 1);
        RTARRAY a(fEmptyArray);
        if (a.getCustomIndicator(buf, 1, false) == RTX_OK && a.count > 0) {   // bCalculate false: never asks IRT to compute
            out.name = fixedStr(a.info.name, sizeof(a.info.name)); out.textLabel = fixedStr(a.info.textLabel, sizeof(a.info.textLabel));
            std::vector<double> vals; vals.reserve((size_t)want);
            const long shift = n - a.count;
            for (long i = from; i < n; i++) { long j = i - shift; vals.push_back(j >= 0 && j < a.count ? (double)a[(int)j] : std::numeric_limits<double>::quiet_NaN()); }
            out.arrayNo.push_back(0); out.values.push_back(vals); out.via = "custom";
            return true;
        }
    }
    return false;
}

void ChartView::readWatch(const std::string& dir, std::vector<std::string>& labels, cvl::Snap& s)
{
    const std::string p = dir + "\\ChartView.labels.txt";
    s.watchFile = p;
    long long mt = -1; unsigned long long sz = 0;
    long long stamp = statFile(p, mt, sz) ? mt * 1000003LL + (long long)sz : -1;
    if (stamp != g_watch.stamp) {
        g_watch.stamp = stamp; g_watch.labels.clear(); g_watch.present = stamp >= 0;
        if (stamp >= 0 && sz <= 65536) { std::string t; if (readRange(p, 0, (size_t)sz, t)) g_watch.labels = cvl::parseLabelFile(t); }
    }
    if (!g_watch.present) s.notes.push_back("ChartView.labels.txt not found");
    labels = g_watch.labels;
}

void ChartView::readFiles(const std::string& dir, cvl::Snap& s)
{
    time_t now = time(0); struct tm lt; memset(&lt, 0, sizeof(lt)); localOf(now, lt);
    const long long today = cvl::daysFromCivil(lt.tm_year + 1900, (unsigned)(lt.tm_mon + 1), (unsigned)lt.tm_mday);
    std::vector<cvl::Src> src = cvl::sourcesFor(s.market, s.symbol, today);
    size_t total = 0;
    if (g_files.size() > 160) g_files.clear();                    // bounded: a few dozen paths per market in practice
    for (size_t k = 0; k < src.size(); k++) {
        const std::string p = dir + "\\" + src[k].rel;
        long long mt = -1; unsigned long long sz = 0;
        if (!statFile(p, mt, sz)) { g_files.erase(p); continue; }       // not written by that plugin (yet): simply absent
        CachedFile& C = g_files[p];
        if (C.mtime != mt || C.size != sz) {
            cvl::Plan pl = cvl::planRead((size_t)sz, src[k].cut);
            std::string head, tail;
            bool ok = readRange(p, 0, pl.headLen, head) && readRange(p, pl.tailFrom, (size_t)sz - pl.tailFrom, tail);
            if (!ok) { g_files.erase(p); s.filesSkipped.push_back(src[k].rel + " (read failed)"); continue; }
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

void ChartView::pump()
{
    CVState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = true;
    struct Unbusy { CVState* s; ~Unbusy() { s->busy = false; } } unbusy = { S };
    try {
        if (CX.enabled >= 0 && !isBoxChecked(CX.enabled)) return;
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
        const bool want = wantNow(dir, C, mkt, spb > 0 ? spb : 0, nowMs);
        const int why = cvl::due(C.sched, n, lastT, nowMs, want);
        if (why == cvl::NONE) return;
        cvl::built(C.sched, n, lastT, nowMs);                     // a failed build waits for the next new bar / 10 s, never loops

        // ---- the snapshot
        cvl::Snap s;
        s.market = mkt; s.symbol = sym; s.root = root; s.chart = chart; s.chartBars = n;
        { const char* pl = getPeriodicityLabel(); s.periodicity = pl ? pl : ""; }
        int N = getIntegerValue(CX.bars);
        if (N <= 0) N = cvl::DEFAULT_BARS;
        if (N < cvl::MIN_BARS) N = cvl::MIN_BARS;
        if (N > cvl::MAX_BARS) N = cvl::MAX_BARS;
        s.barsWanted = N;
        const long from = n > N ? n - N : 0;
        RTARRAY o(barOpen), h(barHigh), l(barLow), c(barClose);
        RTARRAYI v(barVolume);
        std::vector<long long> secs; secs.reserve((size_t)(n - from));
        for (long i = from; i < n; i++) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[(int)i], &t);
            cvl::Civil cv = civilOf(t);
            s.t.push_back(cvl::stamp(cv)); secs.push_back(cvl::secsOf(cv));
            s.o.push_back(o[(int)i]); s.h.push_back(h[(int)i]); s.l.push_back(l[(int)i]); s.c.push_back(c[(int)i]); s.v.push_back((double)v[(int)i]);
        }
        s.period = cvl::periodOf(spb, secs);
        if (spb <= 0) s.notes.push_back("seconds per bar from the bar stamps");

        std::vector<std::string> fileLabels;
        readWatch(dir, fileLabels, s);
        std::vector<std::string> extra;
        { char eb[256]; memset(eb, 0, sizeof(eb)); if (getParameterText(CX.extra, eb, sizeof(eb) - 1) == RTX_OK) extra = cvl::parseExtra(eb); }
        s.watchFromFile = (int)fileLabels.size(); s.watchExtra = (int)extra.size();
        std::vector<std::string> labels = cvl::mergeLabels(fileLabels, extra);
        for (size_t k = 0; k < labels.size(); k++) {
            cvl::Series x;
            if (readSeries(labels[k], n, from, x)) s.series.push_back(x); else s.missing.push_back(labels[k]);
        }
        readFiles(dir, s);

        // ---- write only when it changed (a new bar, a request and the first snapshot always)
        const std::string b = cvl::body(s);
        const unsigned long long hsh = cvl::fnv1a(b);
        if (!cvl::shouldWrite(why, hsh, C.lastHash, C.everWritten)) return;
        time_t now = time(0); struct tm lt, ut; memset(&lt, 0, sizeof(lt)); memset(&ut, 0, sizeof(ut));
        localOf(now, lt); utcOf(now, ut);
        const cvl::Civil lc = civilOf(lt);
        const std::string doc = cvl::document(b, cvl::iso(lc, cvl::utcOffsetMin(lc, civilOf(ut))), (long long)now, why);
        const std::string outDir = dir + "\\ChartView";
        makeDir(outDir);
        if (atomicWrite(outDir + "\\" + cvl::fileStem(mkt, s.period) + ".json", doc)) { C.lastHash = hsh; C.everWritten = true; C.writes++; }
    } catch (...) {
        // never let a file / string failure reach IRT; the next new bar or 10 s tries again
    }
}

extern "C" cppExtension *CreateExtension(void)
{
    ChartView *p = new ChartView();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | CALC_LAST);
    p->setDescription("LRA ChartView: writes this chart's bars and indicator values to lsFlexLevels\\ChartView for Claude. Draws nothing.");
    p->setVersion("1.0.0");   // CV_VERSION
    return p;
}
