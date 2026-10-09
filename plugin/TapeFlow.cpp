/********************************************************************************
 *  TapeFlow.cpp  --  Investor/RT RTX extension  lsTapeFlow<MKT>  (v2.0.0, 2026-10-09)
 *
 *  Rassul 2026-10-09: "implement proposed tapeflow. add an up arrow head or down arrowhead to the signal. make the whole thing look
 *  nice and neat with clear signals. use 1 letter to identify a signal but you can keep the ? until it is confirmed. use another
 *  color than a purple line." Earlier: "take the information that is there now and put it in that gray header so only the tape is
 *  in the pane"; "once a signal is given it should not change"; no Calibrating / Quiet uselessness - ready at the open everywhere.
 *
 *  The pane: the 30-s pressure bars (green buyers / red sellers, grey when the tape is quiet), the 180-s line (soft slate
 *  0xCBD5E1 - the one extra colour besides green / red / grey), a faint zero line, and the signals - ONE letter with a solid
 *  arrowhead: bullish = green UP arrowhead at the bottom, bearish = red DOWN arrowhead at the top, '?' until confirmed.
 *  P push, E exhaustion, A absorption, F flip (exact rules: TapeFlowLogic.h "THE SIGNALS"). At most one per bar; never two
 *  touching (the weaker is not drawn at that zoom); decided on CLOSED bars only and kept in TapeFlow\<MKT>-signals-<session>.csv,
 *  so they never repaint and a restart redraws exactly what was shown.
 *  The readout is IRT's grey header (parmsTitle): "TF 2.0 ES  BUYERS 1.4x  last A 7,861.50". If IRT does not re-read the title,
 *  the same text is one small line at the top-left of the pane (no box).
 *  No settings. Diagnostics for Claude: TapeFlow.status-<MKT>.txt (atomic, every 5 s when changed).
 *
 *  Data: IRT's own trades (RTTICKS) - native, no outside dependency. Back-fill: the last 10 minutes at once (the pane is never
 *  empty), then the whole session in 200-ms slices (the engine then equals one straight run); the calibration history (10
 *  complete prior sessions) ONLY outside this market's RTH, one session at a time in 200-ms slices, resumed after a restart from
 *  the baseline files on disk.
 ********************************************************************************/
#ifndef TF_FIXED
#error "build TapeFlowES.cpp ... TapeFlowEU.cpp (each defines TF_FIXED and includes this file)"
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>     // MoveFileExA for the status file; BEFORE irtsdk.h (RTBOOL / BOOL order, as DealerProfile)
#endif
#include "irtsdk.h"
#include "TapeFlowLogic.h"
#include "TapeFlowSupport.h"
#include <chrono>
#include <iomanip>
#include <climits>
#include <utility>
#include <exception>
#include <mutex>
#include <atomic>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <cctype>
#include <limits>
#include <direct.h>

static const char* TF_VERSION = "2.0.0";   // (2.0.0, Rassul 2026-10-09 "implement proposed tapeflow") one-letter signals with a solid arrowhead - P push / E exhaustion / A absorption / F flip, '?' until confirmed, at most one per bar, never overlapping, decided on closed bars only and recorded in TapeFlow\<MKT>-signals-<session>.csv (no repaint, a restart redraws the same marks); the AW / AR / IN / x markers are gone from the pane (still in the events file); the 180-s line is soft slate instead of purple; no neutral band / +-35 lines / box; the readout is IRT's grey header "TF 2.0 ES  BUYERS 1.4x  last A 7,861.50" (one small pane line only if IRT does not re-read it); no settings at all; the session back-fill runs in 200-ms slices and then becomes the live engine (the same as one straight run); the calibration back-fill is one prior session at a time, 200-ms slices, outside RTH only, never copies the baseline store and resumes after a restart; only changed baseline files are rewritten; the previous session's flow and signals are shown from their files   // (1.1.5) baseline windows no longer need a trade every 5 s and quiet 20 s count; a thin 5-min slot borrows +-1 / +-2 neighbours; QUIET only after max(5 s, 3 x the slot's typical gap between trades)   // (1.1.4) a back-fill whose history is not strictly in time order is put in time order instead of rejected   // (1.1.3) every second's pressure is kept in TapeFlow\<MKT>-sec-<session>.csv for the nightly scoring   // (1.1.2) no gap in the pane where IRT was restarted   // (1.1.1) the audited 1.1.0 + integration fixes   // (1.0.3) 30-SEC pressure bars   // (1.0.2) one engine per DLL (timer-safe)
static const int LATE_MARGIN = 8; // quiet-second allowance retained; tune only from measured CQG delivery latency

// colours (IRT COLOR = 0x00RRGGBB): green bullish, red bearish, grey (quiet bars, zero line, the fallback text) and ONE more -
// the 180-s line in soft slate (Tailwind slate-300) - light, cool and clearly not the grey of a quiet bar or the old purple
static const COLOR C_BUY    = 0x0022C55E;
static const COLOR C_SELL   = 0x00EF4444;
static const COLOR C_GRAY   = 0x0064748B;
static const COLOR C_LINE   = 0x00CBD5E1;
static const COLOR C_BG     = 0x00000000;   // the chart background the opacity blends into
static const int FONT_PT = 9;

static std::string stamp(long long t);
static std::string memoryEventKey(const tfl::Ev& e);
static int nextTimerId() { static std::atomic<int> next(5711); return next.fetch_add(1); }
static int timerIdFor(const void* me);

// Local civil timestamps retained for compatibility; they are NOT monotonic across DST rollback.
static long long civilSec(int y, int mo, int d, int h, int mi, int s)
{
    y -= mo <= 2; long long era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (unsigned)(mo + (mo > 2 ? -3 : 9)) + 2) / 5 + (unsigned)d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + h * 3600 + mi * 60 + s;
}
static double marketTick(const std::string& m)
{
    return m == "ES" || m == "NQ" ? 0.25 : m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 : m == "HG" ? 0.0005 : m == "EU" ? 0.00005 : 0.0;
}
static int marketDecimals(const std::string& m)
{
    return m == "ES" || m == "NQ" || m == "CL" ? 2 : m == "NG" ? 3 : m == "GC" ? 1 : m == "HG" ? 4 : m == "EU" ? 5 : 2;
}
static std::string marketForRoot(const std::string& rootIn)
{
    std::string r;
    for (size_t i = 0; i < rootIn.size(); i++) r += (char)std::toupper((unsigned char)rootIn[i]);
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

typedef tf_support::SnapshotCursor TCursor;

struct TStep { const char* name; int back; };
// (2.0.0) 10 minutes at once, then the whole session (sliced). The calibration history is the separate per-session deep job.
static const TStep STEPS[] = { {"10m", 600}, {"session", -1} };
static const int NSTEPS = (int)(sizeof(STEPS) / sizeof(STEPS[0]));
static const size_t COMPLETE_WINDOWS = 2400;   // a session with >= 2,400 of its ~4,140 20-s windows is complete
static const long long SLICE_MS = 200;         // the most work one timer tick does on a back-fill

// (2.0.0) the signal record lives OUTSIDE the per-chart state, keyed by symbol | seconds per bar | session (DeltaProfile 2.5.2):
// a chart reload / a new extension object finds the same decided signals
struct BookHolder { std::mutex mx; tfl::SignalBook book; bool loaded = false; std::string path; };
static std::map<std::string, std::shared_ptr<BookHolder> >& books() { static std::map<std::string, std::shared_ptr<BookHolder> > m; return m; }
static std::mutex& booksMx() { static std::mutex m; return m; }

// a back-fill replay that runs in slices across timer ticks
struct ReplayJob {
    bool active = false;
    long long sid = LLONG_MIN, fetchedAtS = 0, endS = LLONG_MAX;
    std::vector<tfl::Tick> ticks; size_t pos = 0;
    std::unique_ptr<tfl::Engine> e; tfl::Store tiny; TCursor c2;
    long long fetchMs = 0, workMs = 0; int slices = 0; long trades = 0;
    void clear() { active = false; ticks.clear(); ticks.shrink_to_fit(); pos = 0; e.reset(); tiny.clear(); c2.reset(); fetchMs = workMs = 0; slices = 0; trades = 0; }
};

struct TapeFlowState {
    int uniqueTimerId = nextTimerId();
    std::string mkt = TF_FIXED, root, sym, chartRoot, err;
    double tick = 0;
    tfl::Engine eng; tfl::Store store; bool storeLoaded = false;
    TCursor cur;
    time_t wallAtTick = 0; long long dataAtTick = -1;
    bool timerOn = false, timerRefused = false, busy = false;
    time_t firstPass = 0, lastTimerTick = 0, timerAt = 0, lastStepAt = 0, lastSave = 0, lastEvWrite = 0, lastStatus = 0, lastFallback = 0;
    int step = 0;                       // the next back-fill step
    bool blocked[NSTEPS] = {false}; bool guardsRead = false; bool deepBlocked = false;
    std::string stepNote, lastStepName;
    long long stepTicks[NSTEPS] = {0};
    size_t evWritten = (size_t)-1; long long evLastT = 0;
    std::set<std::string> written; std::set<long long> keysLoaded;
    std::map<long long, std::uint64_t> savedDigest; std::set<long long> dirty;
    bool storageReady = false;
    bool bootstrapDone = false;         // the live engine has a cursor (10 minutes or more)
    bool sessionDone = false;           // (2.0.0) the session replay became the live engine: signals may be decided
    std::vector<tfl::Ev> committed;
    std::set<std::string> committedKeys;
    size_t captureCount = 0;
    long long captureFirst = LLONG_MIN;
    std::chrono::steady_clock::time_point steadyAtTick;
    std::recursive_mutex access;
    tf_support::RenderIndex renderIndex;
    size_t indexedSize = (size_t)-1;
    long long indexedFirst = LLONG_MIN, indexedLast = LLONG_MIN;
    long long liveTicks = 0; int passes = 0;
    long long secWrittenT = LLONG_MIN; bool secInit = false;   // the per-second research file
    // the header title, the pane's scale, diagnostics
    std::mutex titleMx; std::string title;
    std::string titleSeen; time_t titleChangedAt = 0, lastInvalidate = 0; bool invalidatePending = false;
    std::atomic<long> titleCalls{0}; std::atomic<long long> lastTitleCall{0};
    double range = 60; int paneH = 0, band = 22;
    std::string stepResult[NSTEPS]; long long stepMs[NSTEPS] = {0};
    std::vector<std::string> warns;
    std::string statusBody; time_t statusWrittenAt = 0;
    long long lateSid = LLONG_MIN, lateBase = 0;
    std::vector<std::pair<time_t, long long> > lateSamples;
    // (2.0.0) jobs, bars, signals
    ReplayJob sess, deep;
    std::set<long long> deepEmpty, deepTried; long long deepExhaustedBelow = LLONG_MIN; bool deepProgressRead = false;
    std::string deepNote = "not started"; int deepSessionsDone = 0; time_t deepCheckAt = 0;
    std::map<long long, tfl::BarIn> bars;           // closed chart bars by time (captured in calc)
    int spb = 0;
    std::shared_ptr<BookHolder> book; long long bookSid = LLONG_MIN; std::string bookKey;
    std::vector<tfl::Mark> prevMarks;               // the previous session's record (drawn only)
    bool prevLoaded = false;
    long long sigWrittenLines = 0;
};
static int timerIdFor(const void* me) { return static_cast<const TapeFlowState*>(me)->uniqueTimerId; }

class TapeFlow : public cppExtension {
public:
    TapeFlow() : cppExtension(), own_(new TapeFlowState()) {}
    ~TapeFlow() { delete own_; }
    virtual int timer(RTX_EVENT* e);
    virtual int draw(void);
    virtual int parmsLoad(void)  { return RTX_OK; }
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);   // symmetric, fitted to the visible data
    virtual int parmsTitle(char* pStr, int size);                               // the readout for IRT's grey header

    mutable TapeFlowState* own_ = nullptr;
    TapeFlowState* existingState() const { return own_; }
    TapeFlowState& S() const
    {
        if (!own_) own_ = new TapeFlowState();
        return *own_;
    }

    std::string evPath(long long sid) const;
    void pump(const char* via);
    void pass(bool fromTimer);
    bool identify();
    long long localSec(RTDATE d) { struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(d, &t); return civilSec(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); }
    int toTicks(double p) const
    {
        double tickSize = S().tick;
        if (!(tickSize > 0) || !std::isfinite(p)) return 0;
        double q = p / tickSize;
        if (q < (double)std::numeric_limits<int>::min() + 0.5 || q > (double)std::numeric_limits<int>::max() - 0.5) return 0;
        return (int)std::llround(q);
    }
    std::string fmtPx(int ticks) const;
    std::string dir() const { const char* up = getenv("USERPROFILE"); return up && *up ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
    std::string tfDir() const { const std::string base = dir(); return base.empty() ? std::string() : base + "\\TapeFlow"; }
    void trace(const std::string& s);
    void live();
    bool snapshot(RTTICKS& data, std::vector<tf_support::TradeIdentity>& out, long long upTo = LLONG_MAX);
    void captureCommitted();
    bool rebuild10m();
    bool startSessionJob();
    void sessionSlice();
    void finishSessionJob();
    void runSteps();
    void deepSlice();
    void readDeepProgress();
    void writeDeepProgress();
    void loadStore();
    void saveStore(bool all = false);
    std::string basePath(long long sid) const;
    void writeEvents(bool force);
    void writeSeconds();
    bool inOwnRth() const;
    void writeStatus();
    std::string stateText() const;
    tfl::StateInfo stateInfo() const { return tfl::classify(S().eng, !S().sym.empty(), !S().err.empty()); }
    void updateTitle();
    bool headerLive() const;
    double visibleMaxAbs(int b0, int b1);
    double rangeFor(int b0, int b1) { return tfl::fitRange(visibleMaxAbs(b0, b1)); }
    void warn(const std::string& s);
    bool offHours() const;
    // signals
    std::string sigPath(long long sid) const;
    void decideSignals();
    void flushSignals();
    void loadPrevious();
    void captureBars(int from);
    // guard files
    std::string guardPath(const char* kind, const char* name) const { return tfDir() + "\\_" + kind + "-" + S().mkt + "-" + name + ".txt"; }
    void readGuards();
    void guardWrite(const char* name);
    void guardClear(const char* name);
    // drawing
    void render();
    int textW(const char* s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s, -1); }
    void textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
        RCT rc; rc.set(x, (short)(yy - sz), (short)(x + 600), (short)(yy + sz)); rc.drawText(s, false, false);
    }
    void textC(short x, short yc, const char* s, COLOR col, int sz, bool bold, int w)   // centred on (x, yc), as SessionVWAP's badges
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        const short h = (short)(sz + 6);
        RCT rc; rc.set((short)(x - w / 2 - 2), (short)(yc - h / 2), (short)(x + w / 2 + 3), (short)(yc + h / 2)); rc.drawText(s, true, false);
    }
    void line(short x1, short y1, short x2, short y2, COLOR c, int w)
    {
        setPen(c, (short)w, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
        PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
    }
    void fill(short l, short t, short r, short b, COLOR c) { if (r < l) std::swap(l, r); if (b < t) std::swap(t, b); RCT rc; rc.set(l, t, (short)(r + 1), (short)(b + 1)); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
    void arrowhead(short x, short tipY, bool up, COLOR c, int half, int height);
};

#define mkt (S().mkt)
#define root (S().root)
#define sym (S().sym)
#define err (S().err)
#define tick (S().tick)
#define eng (S().eng)
#define store (S().store)
#define storeLoaded (S().storeLoaded)
#define cur (S().cur)
#define timerOn (S().timerOn)
#define timerRefused (S().timerRefused)
#define busy (S().busy)
#define firstPass (S().firstPass)
#define lastTimerTick (S().lastTimerTick)
#define timerAt (S().timerAt)
#define lastStepAt (S().lastStepAt)
#define lastSave (S().lastSave)
#define lastEvWrite (S().lastEvWrite)
#define lastStatus (S().lastStatus)
#define lastFallback (S().lastFallback)
#define step (S().step)
#define blocked (S().blocked)
#define guardsRead (S().guardsRead)
#define stepNote (S().stepNote)
#define evWritten (S().evWritten)
#define evLastT (S().evLastT)
#define written (S().written)
#define keysLoaded (S().keysLoaded)
#define savedDigest (S().savedDigest)
#define liveTicks (S().liveTicks)
#define passes (S().passes)

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void)
{
    TapeFlow* me = static_cast<TapeFlow*>(this);
    TapeFlowState* state = me->existingState();
    if (!state) return RTX_OK;
    auto S = [&]() -> TapeFlowState& { return *state; };
    if (timerOn) { destroyTimer(timerIdFor(state)); timerOn = false; }
    timerRefused = false;
    lastTimerTick = 0;
    if (storeLoaded) { eng.flushSession(); S().dirty.insert(eng.curSid); me->saveStore(); }
    me->writeEvents(true); me->flushSignals();
    for (int i = 0; i < NSTEPS; i++) me->guardClear(STEPS[i].name);   // completion, not a crashed request
    me->guardClear("deep");
    return RTX_OK;
}
int cppExtension::destroy(void)
{
    TapeFlow* me = static_cast<TapeFlow*>(this);
    TapeFlowState* state = me->existingState();
    if (!state) return RTX_OK;
    auto S = [&]() -> TapeFlowState& { return *state; };
    if (timerOn) { destroyTimer(timerIdFor(state)); timerOn = false; }
    if (storeLoaded) { eng.flushSession(); S().dirty.insert(eng.curSid); me->saveStore(); }
    me->writeEvents(true); me->flushSignals();
    for (int i = 0; i < NSTEPS; i++) me->guardClear(STEPS[i].name);   // a normal close is not a crash
    me->guardClear("deep");
    delete state;
    me->own_ = nullptr;
    return RTX_OK;
}

int cppExtension::setup(void)
{
    // (2.0.0) NO settings: the settings window shows nothing to change. The two outputs (30-s pressure, 180-s line, each bar's
    // value at its close) are OUTPUT_NO_UI + DRAW_INVISIBLE: IRT never draws them and they are not in the settings window.
    setParameterVersion(3);
    setParameterDialogHeight(1);
    CPEN p(C_GRAY, 1, P_SOLID);
    setOutputParameter("30s pressure", DRAW_INVISIBLE, &p, C_GRAY, OUTPUT_NO_UI);
    setOutputParameter("180s line", DRAW_INVISIBLE, &p, C_GRAY, OUTPUT_NO_UI);
    return RTX_OK;
}

int cppExtension::calc(int iStartBar)
{
    TapeFlow* me = static_cast<TapeFlow*>(this);
    me->pump("calc");
    long n = getBarCount();
    if (n > 0) {
        RTARRAY o1(fOut1), o2(fOut2);
        int from = iStartBar > 0 ? iStartBar : 0;
        auto S = [&]() -> TapeFlowState& { return me->S(); };     // the state macros (eng, ...) expand to S().x
        std::unique_lock<std::recursive_mutex> lock(S().access, std::try_to_lock);
        const std::vector<tfl::SecRec>* H = lock.owns_lock() ? &eng.hist : nullptr;
        RTARRAYI dt(barDateTime);
        for (long i = from; i < n; i++) {
            float a = 0.f, b = 0.f;
            if (H && !H->empty()) {
                const long long te = me->localSec((RTDATE)dt[(int)i]);
                auto it = std::upper_bound(H->begin(), H->end(), te - 1, [](long long t, const tfl::SecRec& r) { return t < r.t; });
                if (it != H->begin()) {
                    --it;
                    if (te - 1 - it->t <= 600) {
                        if (std::isfinite(it->f30)) a = (float)std::round(it->f30);
                        if (std::isfinite(it->f180)) b = (float)std::round(it->f180);
                    }
                }
            }
            o1[(int)i] = a; o2[(int)i] = b;
        }
        if (lock.owns_lock()) me->captureBars(from);
    }
    return RTX_OK;
}

// (2.0.0) the chart's CLOSED bars (never the forming last one), by bar time: open / high / low / close in ticks. Only the last few
// are re-read each calc; a reload (iStartBar 0) or a gap re-reads up to 600.
void TapeFlow::captureBars(int from)
{
    const long n = getBarCount();
    if (n < 2 || tick <= 0) return;
    const int spb = getSecondsPerBar();
    if (spb <= 0) return;
    if (spb != S().spb) { S().spb = spb; S().bars.clear(); }
    RTARRAYI dt(barDateTime);
    RTARRAY O(barOpen), Hh(barHigh), Lw(barLow), C(barClose);
    int i0 = (int)std::max(0L, n - 600);
    if (!S().bars.empty() && from > 0) i0 = std::max(i0, std::min(from, (int)n - 2) - 6);
    for (int i = std::max(0, i0); i <= (int)n - 2; ++i) {
        tfl::BarIn b; b.te = localSec((RTDATE)dt[i]);
        long long prev = i > 0 ? localSec((RTDATE)dt[i - 1]) : b.te - spb;
        b.ts = (b.te - prev > 4 * spb || b.te <= prev) ? b.te - spb : prev;
        b.o = toTicks(O[i]); b.h = toTicks(Hh[i]); b.l = toTicks(Lw[i]); b.c = toTicks(C[i]);
        if (b.o <= 0 || b.h <= 0 || b.l <= 0 || b.c <= 0) continue;
        S().bars[b.te] = b;
    }
    while (S().bars.size() > 1500) S().bars.erase(S().bars.begin());
}

// Timer performs back-fill. Calc may perform only a live fallback; draw never queries RTTICKS.
void TapeFlow::pump(const char* via)
{
    std::unique_lock<std::recursive_mutex> lock(S().access,std::try_to_lock);
    if (!lock.owns_lock()) return;
    time_t now = time(0);
    if (!timerOn && !timerRefused) {
        if (createTimer(timerIdFor(&S()), 1000) == RTX_OK) { timerOn = true; timerAt = now; trace(std::string("timer granted (") + via + ")"); }
        else { timerRefused = true; trace("timer refused; calc-only live fallback (no draw requests)"); }
    }
    const bool noTimer = timerRefused || (timerOn && (lastTimerTick ? now - lastTimerTick > 60 : now - timerAt > 20));
    if (noTimer && std::strcmp(via, "calc") == 0 && now != lastFallback) {
        lastFallback = now;
        pass(false); // fallback never initiates history requests
    }
}

int TapeFlow::timer(RTX_EVENT* e)
{
    std::unique_lock<std::recursive_mutex> lock(S().access,std::try_to_lock);
    if (!lock.owns_lock()) return RTX_OK;
    if (!e || e->v.timer.id != timerIdFor(&S())) return RTX_FAIL;
    lastTimerTick = time(0);
    pass(true);
    lastTimerTick = time(0);                                      // a long pass is not a stopped timer
    // the header text changed: ask IRT to repaint (and re-read the titles), at most every 2 s, outside our lock
    bool inval = false;
    { time_t now = time(0); if (S().invalidatePending && now - S().lastInvalidate >= 2) { S().invalidatePending = false; S().lastInvalidate = now; inval = true; } }
    lock.unlock();
    if (inval) invalidateChart(true);
    return RTX_OK;
}

bool TapeFlow::identify()
{
    char buf[32] = {0}; const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    std::string m = marketForRoot(root);
    if (m != mkt) return false;                                   // this DLL serves ONE market
    const char* s = getSymbol(); std::string sy = s ? s : "";
    float propertyTick = getProperty(SYM_TICKINCR);
    const double knownIncrement=marketTick(mkt);
    tick = (knownIncrement>0 && std::isfinite(propertyTick) && std::fabs(propertyTick-knownIncrement)<=knownIncrement*1e-5)
        ? knownIncrement : (std::isfinite(propertyTick) && propertyTick>0 && propertyTick<1000 ? (double)propertyTick : knownIncrement);
    if (!sy.empty() && sy != sym) {
        if (!sym.empty()) {                                        // the chart moved to another contract: start over on it
            trace("contract changed " + sym + " -> " + sy + ": starting over");
            eng.flushSession(); S().dirty.insert(eng.curSid); saveStore();
            captureCommitted(); writeEvents(true); flushSignals();
            eng = tfl::Engine(); cur.reset(); S().dataAtTick = -1; step = 0; evWritten = (size_t)-1;
            S().bootstrapDone = false; S().sessionDone = false; S().committed.clear(); S().committedKeys.clear();
            S().captureCount=0; S().captureFirst=LLONG_MIN; S().indexedSize=(size_t)-1;
            S().sess.clear(); S().deep.clear(); S().book.reset(); S().bookSid = LLONG_MIN; S().bookKey.clear(); S().prevMarks.clear(); S().prevLoaded = false;
            keysLoaded.clear(); written.clear();
        }
        sym = sy; eng.attach(&store, sym);
    }
    return tick > 0 && !sym.empty();
}

void TapeFlow::pass(bool fromTimer)
{
    std::unique_lock<std::recursive_mutex> lock(S().access,std::try_to_lock);
    if (!lock.owns_lock()) return;
    if (busy) return;
    busy = true;
    struct BusyReset { bool& value; ~BusyReset() { value = false; } } reset{busy};
    try {
    if (!identify()) return;
    if (!firstPass) {
        firstPass = time(0);
        const std::string base = dir(), target = tfDir();
        if (!base.empty() && !target.empty()) {
            const bool parentOk = _mkdir(base.c_str()) == 0 || errno == EEXIST;
            const bool targetOk = parentOk && (_mkdir(target.c_str()) == 0 || errno == EEXIST);
            S().storageReady = targetOk;
        }
        if (!S().storageReady) err = "local persistence directory unavailable";
        trace(std::string("loaded on ") + sym + " - " + TF_VERSION);
    }
    if (!storeLoaded) loadStore();
    if (!guardsRead) readGuards();
    passes++;
    if (S().storageReady) err.clear();
    auto t_a = std::chrono::steady_clock::now();
    auto slowIf = [&](const char* what) {
        const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_a).count();
        if (ms >= 250) trace(std::string("SLOW ") + what + " " + std::to_string(ms) + " ms");
        t_a = std::chrono::steady_clock::now();
    };
    if (fromTimer) { runSteps(); slowIf("back-fill"); }
    if (step > 0 || !fromTimer) { live(); slowIf("live trades"); }
    captureCommitted();
    if (fromTimer && S().sessionDone) { deepSlice(); slowIf("calibration back-fill"); }
    if (S().sessionDone && !S().prevLoaded) { loadPrevious(); slowIf("previous session files"); }
    decideSignals();
    time_t now = time(0);
    if (S().storageReady && now - lastSave >= 300) { lastSave = now; eng.flushSession(); S().dirty.insert(eng.curSid); saveStore(); slowIf("baseline save"); }
    if (S().storageReady && now - lastEvWrite >= 5) { lastEvWrite = now; writeEvents(false); writeSeconds(); slowIf("event / second files"); }
    flushSignals();
    updateTitle();
    if (S().storageReady && now - lastStatus >= 5) { lastStatus = now; writeStatus(); }
    busy = false;
    } catch (const std::exception& ex) {
        err = std::string("processing failed: ") + ex.what();
        warn(err);
        eng.gap(eng.lastT + 1, err);
        captureCommitted();
    } catch (...) {
        err = "processing failed: unknown exception";
        eng.gap(eng.lastT + 1, err);
        captureCommitted();
    }
}

// ---- trades from an IRT tick array (upTo: keep only seconds before it; the rest is skipped cheaply)
bool TapeFlow::snapshot(RTTICKS& data, std::vector<tf_support::TradeIdentity>& out, long long upTo)
{
    out.clear();
    if (data.count <= 0) return true;
    if (!data.dt || !data.price || !data.size || data.count > INT_MAX) {
        err = "invalid IRT tick array"; return false;
    }
    if (upTo == LLONG_MAX) out.reserve(static_cast<size_t>(data.count));
    double rawCut = std::numeric_limits<double>::infinity();
    for (long i=0; i<data.count; ++i) {
        const RTDATE raw = (RTDATE)(*data.dt)[(int)i];
        long long sec = 0;
        if (upTo != LLONG_MAX) {
            if (!std::isfinite((double)raw)) { err = "invalid / off-grid trade record"; return false; }
            if ((double)raw >= rawCut + 7200.0) continue;          // surely past the cut (2 h clock margin): no local-time call
            sec = localSec(raw);
            if (sec >= upTo) { if ((double)raw < rawCut) rawCut = (double)raw; continue; }
        }
        const double qty = (double)(*data.size)[(int)i];
        int px=0, bid=0, ask=0;
        if (!std::isfinite((double)raw) || !std::isfinite(qty) || qty <= 0 ||
            qty >= (double)LLONG_MAX || std::floor(qty) != qty ||
            !tf_support::nativeToTicks((*data.price)[(int)i], tick, &px)) {
            err = "invalid / off-grid trade record"; return false;
        }
        if (data.bid && (*data.bid)[(int)i] > 0 && !tf_support::nativeToTicks((*data.bid)[(int)i], tick, &bid)) bid = 0;
        if (data.ask && (*data.ask)[(int)i] > 0 && !tf_support::nativeToTicks((*data.ask)[(int)i], tick, &ask)) ask = 0;
        out.push_back(tf_support::TradeIdentity{upTo != LLONG_MAX ? sec : localSec(raw),(double)raw,px,bid,ask,(long long)qty});
    }
    return true;
}

void TapeFlow::live()
{
    RTDATE now = currentDate();
    long long nowS = localSec(now);
    long long back = cur.lastSec >= 0 ? nowS - cur.lastSec + 2 : 600;
    if (back < 2) back = 2;
    if (cur.lastSec >= 0 && back > 3600) {
        eng.gap(std::max(eng.lastT + 1, nowS - 600), "cursor over one hour old");
        captureCommitted(); cur.reset(); back = 600;
    }
    RTTICKS data((RTDATE)(now - (RTDATE)back));
    std::vector<tf_support::TradeIdentity> records;
    size_t begin=0; std::string issue;
    if (!snapshot(data,records) || !cur.plan(records,&begin,&issue)) {
        if (!issue.empty()) err = issue;
        eng.gap(std::max(nowS,eng.lastT+1), err.empty() ? "invalid snapshot" : err);
        captureCommitted();
        trace("DATA INVALID: " + err);
        cur.reset(); S().bootstrapDone = false; S().sessionDone = false; S().sess.clear(); step = 0;   // a fresh bootstrap on the next timer
        return;
    }
    long long got=0;
    for (size_t i=begin; i<records.size(); ++i) {
        const auto& r=records[i];
        tfl::Tick k; k.t=r.second; k.px=r.price; k.q=r.quantity; k.bid=r.bid; k.ask=r.ask;
        const bool accepted=eng.add(k);
        cur.commit(r);
        if (accepted) ++got;
    }
    if (got) {
        S().wallAtTick=time(0); S().steadyAtTick=std::chrono::steady_clock::now();
        S().dataAtTick=cur.lastSec; liveTicks+=got; S().bootstrapDone=true;
    }
    if (S().dataAtTick>=0) {
        const long long elapsed=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-S().steadyAtTick).count();
        const long long watermark=std::min(nowS-LATE_MARGIN,S().dataAtTick+elapsed-LATE_MARGIN);
        eng.advanceTo(watermark);
    }
}

bool TapeFlow::offHours() const
{
    struct tm t; std::memset(&t,0,sizeof(t)); const_cast<TapeFlow*>(this)->getLocaltime(const_cast<TapeFlow*>(this)->currentDate(),&t);
    int m = t.tm_hour * 60 + t.tm_min;
    if (t.tm_wday==6) return true;
    if (t.tm_wday==0) return m<17*60;
    if (t.tm_wday==5 && m>=16*60) return true;
    return m>=16*60 && m<17*60;
}

// ---- back-fill: 10 minutes at once, then the session job (sliced) which becomes the live engine
void TapeFlow::runSteps()
{
    time_t now = time(0);
    if (step == 0) {
        if (blocked[0]) { S().stepResult[0] = "blocked (IRT stopped during it twice)"; step = 1; lastStepAt = now; return; }
        rebuild10m(); step = 1; lastStepAt = time(0); return;
    }
    if (step == 1) {
        if (S().sess.active) { sessionSlice(); return; }
        if (blocked[1]) { S().stepResult[1] = "blocked (IRT stopped during it twice) - signals from the 10-minute start"; step = 2; S().sessionDone = S().bootstrapDone; return; }
        if (now - firstPass < 30 || now - lastStepAt < 20) return;   // let IRT settle first
        if (!startSessionJob()) { step = 2; S().sessionDone = S().bootstrapDone; lastStepAt = time(0); }
    }
}

bool TapeFlow::rebuild10m()
{
    RTDATE now = currentDate();
    long long nowS = localSec(now);
    trace("back-fill 10m...");
    guardWrite("10m");
    RTTICKS T((RTDATE)(now - (RTDATE)600));
    tfl::Engine e2; e2.cfg = eng.cfg; e2.attach(&store, sym);
    TCursor c2; std::vector<tf_support::TradeIdentity> records; std::string issue; size_t begin = 0;
    if (!snapshot(T, records)) { guardClear("10m"); S().stepResult[0] = "rejected: " + err; warn(S().stepResult[0]); return false; }
    tf_support::sortByTime(records);
    if (!c2.plan(records, &begin, &issue)) { guardClear("10m"); if (!issue.empty()) err = issue; S().stepResult[0] = "rejected: " + err; warn(S().stepResult[0]); return false; }
    for (const auto& r : records) { tfl::Tick k; k.t=r.second; k.px=r.price; k.q=r.quantity; k.bid=r.bid; k.ask=r.ask; e2.add(k); c2.commit(r); }
    guardClear("10m");
    char b[160]; snprintf(b, sizeof(b), "back-fill 10m: %ld trades from IRT, %lld used", (long)T.count, e2.ticks); trace(b); stepNote = b;
    S().stepResult[0] = std::string("done: ") + std::to_string(e2.ticks) + " trades";
    if (e2.ticks <= 0) return false;
    e2.advanceTo(nowS - LATE_MARGIN);
    eng = std::move(e2); eng.attach(&store, sym);
    S().captureCount=0; S().captureFirst=LLONG_MIN; S().indexedSize=(size_t)-1;
    captureCommitted();
    cur = c2; S().wallAtTick = time(0); S().steadyAtTick = std::chrono::steady_clock::now(); S().dataAtTick = cur.lastSec;
    S().bootstrapDone = true;
    return true;
}

// the whole session from 17:00 in ONE request (as 1.1.x did), copied compactly, then replayed in 200-ms slices
bool TapeFlow::startSessionJob()
{
    RTDATE now = currentDate();
    long long nowS = localSec(now);
    long long sid = tfl::sessionOf(nowS);
    long long start = (sid - 1) * 86400 + 17LL * 3600;
    long long back = nowS - start + 5;
    trace("back-fill session...");
    const auto t0 = std::chrono::steady_clock::now();
    guardWrite("session");
    RTTICKS T((RTDATE)(now - (RTDATE)back));
    std::vector<tf_support::TradeIdentity> records; std::string issue; size_t begin = 0;
    ReplayJob& J = S().sess; J.clear();
    if (!snapshot(T, records)) { guardClear("session"); S().stepResult[1] = "rejected: " + err; warn(S().stepResult[1]); return false; }
    const size_t moved = tf_support::sortByTime(records);
    if (!J.c2.plan(records, &begin, &issue)) { guardClear("session"); if (!issue.empty()) err = issue; S().stepResult[1] = "rejected: " + err; warn(S().stepResult[1]); return false; }
    J.ticks.reserve(records.size());
    for (const auto& r : records) { tfl::Tick k; k.t=r.second; k.px=r.price; k.q=r.quantity; k.bid=r.bid; k.ask=r.ask; J.ticks.push_back(k); J.c2.commit(r); }
    guardClear("session");
    records.clear(); records.shrink_to_fit();
    J.fetchMs = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    J.active = true; J.sid = sid; J.fetchedAtS = nowS; J.pos = 0;
    J.e.reset(new tfl::Engine()); J.e->cfg = eng.cfg; J.e->attach(&store, sym);   // windows go straight to the store (merged, first wins)
    char b[200]; snprintf(b, sizeof(b), "back-fill session: %ld trades from IRT (%zu put back in time order), fetched in %lld ms - replaying in %lld-ms slices", (long)T.count, moved, J.fetchMs, SLICE_MS);
    trace(b); stepNote = b; S().stepResult[1] = "replaying";
    if (J.fetchMs >= 250) trace("SLOW session fetch " + std::to_string(J.fetchMs) + " ms");
    return true;
}
void TapeFlow::sessionSlice()
{
    ReplayJob& J = S().sess;
    const auto t0 = std::chrono::steady_clock::now();
    size_t n = 0;
    while (J.pos < J.ticks.size()) {
        J.e->add(J.ticks[J.pos++]); J.trades++;
        if ((++n & 1023) == 0 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() >= SLICE_MS) break;
    }
    J.slices++; J.workMs += (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    if (J.pos >= J.ticks.size()) finishSessionJob();
}
void TapeFlow::finishSessionJob()
{
    ReplayJob& J = S().sess;
    J.e->advanceTo(J.fetchedAtS - LATE_MARGIN);
    J.e->flushSession();
    // the replay becomes the live engine: events already shown / written keep their place; the replay's own events start after them
    captureCommitted();
    // (the replay keeps ALL its events - the signals read them - but those at or before the last event already written are
    // marked as captured, so the events file stays append-only and never gets a second version of the same minutes)
    const long long protectedThrough = S().committed.empty() ? LLONG_MIN : S().committed.back().t;
    const long long startOf10m = eng.hist.empty() ? LLONG_MAX : eng.hist.front().t;   // what the 10-minute start already covered
    for (const tfl::Ev& ev : J.e->evs) if (ev.t >= startOf10m && ev.t <= protectedThrough) S().committedKeys.insert(memoryEventKey(ev));
    const long long ticksBefore = eng.ticks;
    eng = std::move(*J.e); eng.attach(&store, sym); eng.noFlushSid = LLONG_MIN;
    S().captureCount=0; S().captureFirst=LLONG_MIN; S().indexedSize=(size_t)-1;
    captureCommitted();
    cur = J.c2; S().wallAtTick = time(0); S().steadyAtTick = std::chrono::steady_clock::now(); S().dataAtTick = cur.lastSec;
    S().bootstrapDone = true; S().sessionDone = true;
    char b[220]; snprintf(b, sizeof(b), "done: %ld trades replayed in %d slices (%lld ms work, fetch %lld ms); the replay is now the live engine (the 10-minute start had %lld)",
                          J.trades, J.slices, J.workMs, J.fetchMs, ticksBefore);
    S().stepResult[1] = b; trace(std::string("back-fill session ") + b);
    J.clear(); step = 2; lastStepAt = time(0);
    S().dirty.insert(eng.curSid); saveStore();
    evWritten = (size_t)-1;
}

// ---- (2.0.0) the calibration history: ONE prior session per job, newest missing first, outside this market's RTH only, in
// 200-ms slices; the job's windows go into a one-session store merged into the baselines when done (the store is never copied).
// Progress = the baseline files themselves (complete sessions are skipped after a restart) + TapeFlow\_deep-<MKT>.txt (sessions
// IRT had no trades for, and where IRT's history ends).
void TapeFlow::readDeepProgress()
{
    S().deepProgressRead = true;
    std::ifstream f((tfDir() + "\\_deep-" + mkt + ".txt").c_str()); std::string ln;
    while (std::getline(f, ln)) {
        long long v = 0;
        if (ln.compare(0, 6, "empty,") == 0 && tfl::parseStoreLong(ln.substr(6), &v)) S().deepEmpty.insert(v);
        else if (ln.compare(0, 6, "tried,") == 0 && tfl::parseStoreLong(ln.substr(6), &v)) S().deepTried.insert(v);
        else if (ln.compare(0, 15, "exhaustedBelow,") == 0 && tfl::parseStoreLong(ln.substr(15), &v)) S().deepExhaustedBelow = v;
    }
}
void TapeFlow::writeDeepProgress()
{
    if (!S().storageReady) return;
    std::ofstream o((tfDir() + "\\_deep-" + mkt + ".txt").c_str(), std::ios::trunc);
    o << "# TapeFlow " << TF_VERSION << " calibration back-fill progress (delete to retry everything)\n";
    for (long long s : S().deepEmpty) o << "empty," << s << "\n";
    for (long long s : S().deepTried) o << "tried," << s << "\n";
    if (S().deepExhaustedBelow != LLONG_MIN) o << "exhaustedBelow," << S().deepExhaustedBelow << "\n";
}
void TapeFlow::deepSlice()
{
    if (!S().storageReady || S().deepBlocked) return;
    if (!S().deepProgressRead) readDeepProgress();
    ReplayJob& J = S().deep;
    if (inOwnRth() && !offHours()) { S().deepNote = J.active ? "paused for RTH" : "waiting for the end of RTH"; return; }
    if (J.active) {
        const auto t0 = std::chrono::steady_clock::now(); size_t n = 0;
        while (J.pos < J.ticks.size()) {
            J.e->add(J.ticks[J.pos++]); J.trades++;
            if ((++n & 1023) == 0 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() >= SLICE_MS) break;
        }
        J.slices++; J.workMs += (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        if (J.pos < J.ticks.size()) { S().deepNote = "session " + std::to_string(J.sid) + ": " + std::to_string(J.pos) + " of " + std::to_string(J.ticks.size()) + " trades"; return; }
        J.e->advanceTo(J.endS - 1); J.e->flushSession();
        size_t got = 0; { auto it = J.tiny.find(J.sid); if (it != J.tiny.end()) for (auto& c : it->second) got += c.second.size(); }
        tfl::mergeStore(store, J.tiny);                               // ONE session merged
        S().dirty.insert(J.sid); saveStore();
        eng.refreeze();
        S().deepSessionsDone++;
        char b[200]; snprintf(b, sizeof(b), "calibration back-fill: session %lld measured (%zu windows, %ld trades, fetch %lld ms + %lld ms in %d slices)", J.sid, got, J.trades, J.fetchMs, J.workMs, J.slices);
        trace(b); S().deepNote = b;
        if (got == 0) S().deepEmpty.insert(J.sid);
        S().deepTried.insert(J.sid);                                  // measured once: never fetched again (even if IRT had only part of it)
        writeDeepProgress();
        J.clear(); lastStepAt = time(0);
        return;
    }
    if (time(0) - lastStepAt < 5) return;                           // a breath between sessions
    if (time(0) < S().deepCheckAt) return;                          // nothing to do: look again in a minute (cheap timer ticks)
    const long long sidNow = eng.curSid;
    if (sidNow == LLONG_MIN) return;
    const int complete = tfl::completeSessions(store, sidNow, COMPLETE_WINDOWS);
    if (complete >= eng.cfg.sessionsBack) { S().deepNote = std::to_string(complete) + " complete prior sessions on file - nothing to do"; S().deepCheckAt = time(0) + 60; return; }
    long long target = LLONG_MIN;
    for (long long s = sidNow - 1; s >= sidNow - 21; --s) {
        if (!tfl::tradingWeekday(s) || S().deepEmpty.count(s) || S().deepTried.count(s) || tfl::completeSession(store, s, COMPLETE_WINDOWS)) continue;
        if (S().deepExhaustedBelow != LLONG_MIN && s < S().deepExhaustedBelow) break;
        target = s; break;
    }
    if (target == LLONG_MIN) { S().deepNote = std::to_string(complete) + " complete prior sessions on file - IRT has no more history to measure"; S().deepCheckAt = time(0) + 60; return; }
    RTDATE now = currentDate(); const long long nowS = localSec(now);
    const long long startS = (target - 1) * 86400 + 17LL * 3600, endS = target * 86400 + 17LL * 3600;
    trace("calibration back-fill: session " + std::to_string(target) + " (" + stamp(startS) + ")...");
    const auto t0 = std::chrono::steady_clock::now();
    guardWrite("deep");
    std::vector<tf_support::TradeIdentity> records;
    long count = 0;
    {
        RTTICKS T((RTDATE)(now - (RTDATE)(nowS - startS)));
        count = T.count;
        if (!snapshot(T, records, endS)) { guardClear("deep"); warn("calibration back-fill rejected: " + err); err.clear(); S().deepEmpty.insert(target); writeDeepProgress(); return; }
    }
    guardClear("deep");
    J.clear();
    J.fetchMs = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    if (J.fetchMs >= 250) trace("SLOW calibration fetch " + std::to_string(J.fetchMs) + " ms");
    tf_support::sortByTime(records);
    for (const auto& r : records) if (tfl::sessionOf(r.second) == target) { tfl::Tick k; k.t=r.second; k.px=r.price; k.q=r.quantity; k.bid=r.bid; k.ask=r.ask; J.ticks.push_back(k); }
    records.clear(); records.shrink_to_fit();
    if (J.ticks.empty()) {
        // no trade in that session: a holiday - or IRT's history begins later. Two empty weekday sessions in a row = the end.
        S().deepEmpty.insert(target);
        if (count <= 0 || S().deepEmpty.count(target + 1) || (!tfl::tradingWeekday(target + 1) && S().deepEmpty.count(target + 3))) S().deepExhaustedBelow = target;
        trace("calibration back-fill: no trades in session " + std::to_string(target) + (S().deepExhaustedBelow == target ? " - IRT's history ends here" : " (holiday?)"));
        writeDeepProgress(); lastStepAt = time(0); J.clear();
        return;
    }
    J.active = true; J.sid = target; J.endS = endS; J.pos = 0;
    J.e.reset(new tfl::Engine()); J.e->cfg = eng.cfg; J.e->attach(&J.tiny, sym);
    S().deepNote = "session " + std::to_string(target) + ": " + std::to_string(J.ticks.size()) + " trades fetched";
}

// ---- guards (a request that IRT did not survive twice is blocked)
void TapeFlow::readGuards()
{
    guardsRead = true;
    if (!S().storageReady) return;
    const char* names[] = { "10m", "session", "deep" };
    for (int i = 0; i < 3; i++) {
        bool* flag = i < NSTEPS ? &blocked[i] : &S().deepBlocked;
        { std::ifstream b(guardPath("blocked", names[i]).c_str()); if (b.good()) { *flag = true; continue; } }
        std::ifstream g(guardPath("trying", names[i]).c_str());
        if (!g.good()) continue;
        int strikes = 0; g >> strikes; g.close();
        strikes++;
        if (strikes >= 2) {
            *flag = true;
            std::ofstream o(guardPath("blocked", names[i]).c_str(), std::ios::trunc); o << "IRT stopped during this back-fill request twice - blocked. Delete this file to allow it again.\n";
            std::remove(guardPath("trying", names[i]).c_str());
            trace(std::string("back-fill ") + names[i] + " BLOCKED (IRT stopped during it twice)");
        } else {
            std::ofstream o(guardPath("trying", names[i]).c_str(), std::ios::trunc); o << strikes << "\n";
            trace(std::string("back-fill ") + names[i] + ": IRT stopped during it last time (or was closed) - one more try");
        }
    }
}
void TapeFlow::guardWrite(const char* name)
{
    if (!S().storageReady) return;
    std::string p = guardPath("trying", name);
    int strikes = 0; { std::ifstream g(p.c_str()); if (g.good()) g >> strikes; }
    std::ofstream o(p.c_str(), std::ios::trunc); o << strikes << "\n";
}
void TapeFlow::guardClear(const char* name) { if (S().storageReady) std::remove(guardPath("trying", name).c_str()); }

// ---- the baseline store (one file per session; only CHANGED sessions are rewritten)
std::string TapeFlow::basePath(long long sid) const { return tfDir() + "\\" + mkt + "-base-v110-" + std::to_string(sid) + ".csv"; }
void TapeFlow::loadStore()
{
    storeLoaded = true;
    long long now = localSec(currentDate()), sid0 = tfl::sessionOf(now);
    long rows = 0;
    for (long long sid = sid0; sid >= sid0 - 21; sid--) {
        std::ifstream f(basePath(sid).c_str());
        if (!f.is_open()) { f.clear(); f.open((basePath(sid)+".bak").c_str()); }
        if (!f.is_open()) continue;
        std::string ln; long n = 0;
        while (std::getline(f, ln)) {
            long long s; std::string sy; tfl::Win w;
            if (!tfl::parseStoreLine(ln, &s, &sy, &w) || s != sid) continue;
            store[sid][sy].push_back(w); n++;
        }
        rows += n;
    }
    trace("baseline files: " + std::to_string(store.size()) + " sessions, " + std::to_string(rows) + " windows");
    if (eng.curSid != LLONG_MIN) eng.refreeze();
}
void TapeFlow::saveStore(bool all)
{
    if (!storeLoaded || !S().storageReady || tfDir().empty()) return;
    while (store.size() > 16) { S().dirty.erase(store.begin()->first); store.erase(store.begin()); }   // bounded RAM; disk files untouched
    for (const auto& kv:store) {
        if (!all && !S().dirty.count(kv.first)) continue;
        std::ostringstream payload;
        payload << "# TapeFlow " << TF_VERSION << " baseline " << mkt << " session " << kv.first << "\n";
        for (const auto& c:kv.second) for (const auto& w:c.second)
            payload << tfl::storeLine(kv.first,c.first,w) << "\n";
        const std::string bytes=payload.str();
        const std::uint64_t digest=tf_support::fingerprint(bytes);
        const auto prior=savedDigest.find(kv.first);
        if (prior!=savedDigest.end() && prior->second==digest) { S().dirty.erase(kv.first); continue; }
        const std::string path=basePath(kv.first), tmp=path+".tmp", bak=path+".bak";
        { std::ofstream out(tmp.c_str(),std::ios::binary|std::ios::trunc);
          if (!out) { err="cannot write "+tmp; continue; }
          out.write(bytes.data(),static_cast<std::streamsize>(bytes.size())); out.flush();
          if (!out.good()) { err="cannot flush "+tmp; continue; } }
        bool hadOld=false; { std::ifstream old(path.c_str()); hadOld=old.good(); }
        if (hadOld) {
            std::remove(bak.c_str());
            if (std::rename(path.c_str(),bak.c_str())!=0) { err="cannot stage "+path; continue; }
        }
        if (std::rename(tmp.c_str(),path.c_str())!=0) {
            err="cannot publish "+path;
            if (hadOld) std::rename(bak.c_str(),path.c_str());
            continue;
        }
        if (hadOld) std::remove(bak.c_str());
        savedDigest[kv.first]=digest; S().dirty.erase(kv.first);
    }
}

std::string TapeFlow::fmtPx(int t) const
{
    char b[48]; snprintf(b, sizeof(b), "%.*f", marketDecimals(mkt), t * tick);
    std::string s = b; size_t dot = s.find('.'); if (dot == std::string::npos) dot = s.size();
    size_t start = (s[0] == '-') ? 1 : 0;
    for (long i = (long)dot - 3; i > (long)start; i -= 3) s.insert((size_t)i, ",");
    return s;
}
static std::string stamp(long long t)
{
    long long d = t >= 0 ? t / 86400 : -((-t + 86399) / 86400); long long r = t - d * 86400;
    long long z = d + 719468; long long era = (z >= 0 ? z : z - 146096) / 146097; unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = (long long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); unsigned mp = (5 * doy + 2) / 153; unsigned dd = doy - (153 * mp + 2) / 5 + 1;
    unsigned mm = mp < 10 ? mp + 3 : mp - 9; y += mm <= 2;
    char b[64]; snprintf(b, sizeof(b), "%04lld-%02u-%02u %02lld:%02lld:%02lld", y, mm, dd, r / 3600, (r / 60) % 60, r % 60);
    return b;
}
static std::string clock12(long long t)
{
    long long r = ((t % 86400) + 86400) % 86400; int h = (int)(r / 3600), m = (int)(r / 60 % 60);
    char b[24]; snprintf(b, sizeof(b), "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM");
    return b;
}

// every committed engine event, APPEND-ONLY (the nightly scoring reads it)
std::string TapeFlow::evPath(long long sid) const {
    std::string contract=sym;
    for (char& ch:contract) if (!std::isalnum((unsigned char)ch) && ch!='-' && ch!='_') ch='_';
    return tfDir() + "\\" + mkt + "-events-v110-" + contract + "-" + stamp(sid * 86400).substr(0, 10) + ".csv";
}
static std::string evKey(const std::string& line) { return tf_support::eventKey(line); }
static std::string memoryEventKey(const tfl::Ev& e) {
    return std::to_string(e.t)+"|"+std::to_string(e.ep)+"|"+e.kind+"|"+std::to_string(e.dir);
}
void TapeFlow::captureCommitted()
{
    bool added=false;
    const long long first=eng.evs.empty()?LLONG_MIN:eng.evs.front().t;
    const size_t begin=(first==S().captureFirst && S().captureCount<=eng.evs.size())?S().captureCount:0;
    for (size_t i=begin;i<eng.evs.size();++i) {
        const auto& e=eng.evs[i];
        const std::string key=memoryEventKey(e);
        if (S().committedKeys.insert(key).second) { S().committed.push_back(e); added=true; }
    }
    S().captureCount=eng.evs.size(); S().captureFirst=first;
    if (added) std::stable_sort(S().committed.begin(),S().committed.end(),[](const tfl::Ev& a,const tfl::Ev& b){ return a.t<b.t; });
    if (!S().committed.empty() && S().committed.size()>20000) {
        const long long cutoff=eng.lastT-eng.cfg.keepSec;
        const auto end=std::lower_bound(S().committed.begin(),S().committed.end(),cutoff,
            [](const tfl::Ev& e,long long t){return e.t<t;});
        for (auto it=S().committed.begin();it!=end;++it) S().committedKeys.erase(memoryEventKey(*it));
        S().committed.erase(S().committed.begin(),end);
    }
}
// this market's RTH (Central, lra.markets) and the 15 min before the open: the calibration back-fill never runs inside it
bool TapeFlow::inOwnRth() const
{
    struct tm t; std::memset(&t,0,sizeof(t)); const_cast<TapeFlow*>(this)->getLocaltime(const_cast<TapeFlow*>(this)->currentDate(),&t);
    if (t.tm_wday == 0 || t.tm_wday == 6) return false;
    const int m = t.tm_hour * 60 + t.tm_min; const std::string k = mkt;
    int a = 510, b = 900;                                            // ES / NQ
    if (k == "CL" || k == "NG") { a = 480; b = 810; }
    else if (k == "GC" || k == "HG") { a = 440; b = 750; }
    else if (k == "EU") { a = 440; b = 840; }
    return m >= a - 15 && m < b;
}

// every closed second's pressure -> TapeFlow\<MKT>-sec-<session>.csv (appended; unchanged format - the nightly scoring reads it)
void TapeFlow::writeSeconds()
{
    if (!S().storageReady || mkt.empty() || tfDir().empty() || eng.hist.empty()) return;
    const long long sidNow = eng.curSid;
    if (sidNow == LLONG_MIN) return;
    auto pathOf = [&](long long sid) { return tfDir() + "\\" + mkt + "-sec-" + std::to_string(sid) + ".csv"; };
    if (!S().secInit) {
        S().secInit = true;
        std::ifstream f(pathOf(sidNow).c_str(), std::ios::binary);
        if (f.good()) {
            f.seekg(0, std::ios::end); long long sz = (long long)f.tellg();
            f.seekg(sz > 4096 ? sz - 4096 : 0); std::string ln, lastLn;
            while (std::getline(f, ln)) if (!ln.empty() && ln[0] >= '0' && ln[0] <= '9') lastLn = ln;
            size_t bar = lastLn.find('|');
            if (bar != std::string::npos) { long long t = std::atoll(lastLn.c_str()); if (t > 0) S().secWrittenT = std::max(S().secWrittenT, t); }
        }
    }
    const long long upTo = eng.lastT - 1;
    std::map<long long, std::string> add;
    auto num = [](float x, int dp) { if (!std::isfinite(x)) return std::string(); char q[24]; snprintf(q, sizeof(q), "%.*f", dp, (double)x); return std::string(q); };
    long long last = S().secWrittenT;
    auto it = std::upper_bound(eng.hist.begin(), eng.hist.end(), S().secWrittenT, [](long long t, const tfl::SecRec& r) { return t < r.t; });
    for (; it != eng.hist.end(); ++it) {
        const tfl::SecRec& r = *it;
        if (r.t > upTo) break;
        const long long sid = tfl::sessionOf(r.t);
        if (sid < sidNow - 1) continue;
        std::string& o = add[sid];
        o += std::to_string(r.t); o += '|'; o += stamp(r.t); o += '|';
        o += num(r.f30, 2); o += '|'; o += num(r.f180, 2); o += '|'; o += num(r.a, 3); o += '|'; o += num(r.c30, 3); o += '|';
        o += std::to_string(r.b); o += '|'; o += std::to_string(r.s); o += '|'; o += std::to_string(r.u); o += '|'; o += std::to_string((int)r.flags); o += '\n';
        if (r.t > last) last = r.t;
    }
    for (auto& kv : add) {
        const std::string p = pathOf(kv.first);
        bool fresh; { std::ifstream t(p.c_str(), std::ios::binary | std::ios::ate); fresh = !t.good() || t.tellg() == std::streampos(0); }
        std::ofstream o(p.c_str(), std::ios::app | std::ios::binary);
        if (!o.is_open()) return;
        if (fresh) o << "t|time|f30|f180|activity|cov30|buy|sell|unknown|flags\n";
        o << kv.second; o.flush();
        if (!o.good()) return;
    }
    S().secWrittenT = last;
}

void TapeFlow::writeEvents(bool force)
{
    if (!S().storageReady || mkt.empty() || tfDir().empty() || eng.curSid == LLONG_MIN) return;
    captureCommitted();
    const auto& events=S().committed;
    if (!force && !events.empty() && events.size()==evWritten && events.back().t==evLastT) return;
    typedef std::pair<std::string, std::string> PendingEvent;
    std::map<long long, std::vector<PendingEvent> > add;
    std::set<std::string> pending;
    for (const tfl::Ev& v : events) {
        long long sid = tfl::sessionOf(v.knownAt);
        if (sid < eng.curSid - 1) continue;
        if (!keysLoaded.count(sid)) {
            keysLoaded.insert(sid);
            std::ifstream f(evPath(sid).c_str()); std::string ln;
            while (std::getline(f, ln)) {
                const bool hadNewline = !f.eof();
                if (hadNewline && tf_support::completeEventLine(ln + "\n")) written.insert(evKey(ln));
            }
        }
        auto num = [](double x, int dp) { if (!std::isfinite(x)) return std::string(); char q[32]; snprintf(q, sizeof(q), "%.*f", dp, x); return std::string(q); };
        std::ostringstream out;
        out << stamp(v.knownAt) << '|' << v.ep << '|' << v.kind << '|' << v.dir << '|'
            << fmtPx(v.lo) << '|' << fmtPx(v.hi) << '|' << v.h << '|' << num(v.q95,8) << '|'
            << num(v.f30,8) << '|' << num(v.f180,8) << '|' << num(v.a,8) << '|' << num(v.cov,8) << '|'
            << num(v.qzone,8) << '|' << num(v.conc,8) << '|' << num(v.prog,8) << '|'
            << tf_support::safeField(v.ctx) << '|' << tf_support::safeField(v.why) << '|' << v.ver << '|' << TF_VERSION << "|quote-at-trade\n";
        std::string line=out.str(), key=evKey(line);
        if (written.count(key) || pending.count(key)) continue;
        pending.insert(key);
        add[sid].push_back(PendingEvent(key, line));
    }
    bool complete = true;
    for (auto& kv : add) {
        std::string p = evPath(kv.first);
        bool fresh = false, incompleteTail = false;
        { std::ifstream t(p.c_str(),std::ios::binary|std::ios::ate);
          fresh = !t.good() || t.tellg()==std::streampos(0);
          if (!fresh) { t.seekg(-1,std::ios::end); char last='\0'; t.get(last); incompleteTail=last!='\n'; } }
        std::ofstream o(p.c_str(), std::ios::app);
        if (!o.is_open()) { err = "cannot write " + p; complete = false; continue; }
        if (incompleteTail) o << '\n';
        if (fresh) o << "time|episode|kind|dir|zone_lo|zone_hi|h|q95|f30|f180|activity|cov30|q_zone|concentration|progress_ticks|context|why|baseline_ver|version|mode\n";
        for (const PendingEvent& event : kv.second) o << event.second;
        o.flush();
        if (!o.good()) { err = "cannot write " + p; complete = false; continue; }
        for (const PendingEvent& event : kv.second) written.insert(event.first);
    }
    if (complete) {
        evWritten = events.size(); evLastT = events.empty() ? 0 : events.back().t;
    } else {
        keysLoaded.clear(); written.clear(); evWritten = (size_t)-1; evLastT = 0;
    }
}

// ---- (2.0.0) the signals: decided on closed chart bars once the session replay is the live engine
std::string TapeFlow::sigPath(long long sid) const
{
    std::string p = tfDir() + "\\" + mkt + "-signals-" + std::to_string(sid);
    if (S().spb > 0 && S().spb != 180) p += "-" + std::to_string(S().spb) + "s";
    return p + ".csv";
}
static std::string slurpFile(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream s; s << f.rdbuf(); return s.str(); }
void TapeFlow::decideSignals()
{
    if (!S().sessionDone || S().spb <= 0 || eng.curSid == LLONG_MIN || S().bars.empty()) return;
    const long long sid = eng.curSid;
    const std::string key = sym + "|" + std::to_string(S().spb) + "|" + std::to_string(sid);   // symbol | seconds per bar | session
    if (!S().book || S().bookKey != key) {
        if (S().book && S().bookSid != LLONG_MIN && S().bookSid < sid) {           // the session rolled: today's marks become "previous"
            std::lock_guard<std::mutex> g(S().book->mx); S().prevMarks = S().book->book.marks;
        } else if (S().book && S().bookSid == sid) S().prevLoaded = false;        // another bar size: reload the previous session's record for it
        std::shared_ptr<BookHolder> h;
        { std::lock_guard<std::mutex> g(booksMx()); auto& m = books(); auto it = m.find(key); if (it == m.end()) { h = std::make_shared<BookHolder>(); m[key] = h; } else h = it->second;
          for (auto it2 = m.begin(); it2 != m.end();) { if (it2->second != h && it2->second.use_count() == 1 && it2->first.find(sym + "|") == 0 && it2->first != key) it2 = m.erase(it2); else ++it2; } }
        S().book = h; S().bookSid = sid; S().bookKey = key;
        std::lock_guard<std::mutex> g(h->mx);
        if (!h->loaded) {
            h->loaded = true; h->path = sigPath(sid); h->book.version = TF_VERSION;
            if (S().storageReady) h->book.loadText(slurpFile(h->path));       // a restart: exactly what was decided before
            trace("signal record " + h->path + ": " + std::to_string(h->book.marks.size()) + " signals, decided through " +
                  (h->book.decidedThrough == LLONG_MIN ? std::string("-") : stamp(h->book.decidedThrough)));
        }
    }
    BookHolder& H = *S().book;
    std::lock_guard<std::mutex> g(H.mx);
    const long long nowS = localSec(currentDate());
    const long long sessStart = (sid - 1) * 86400 + 17LL * 3600;
    for (auto it = S().bars.upper_bound(std::max(H.book.lastFed, sessStart)); it != S().bars.end(); ++it) {
        const tfl::BarIn& b = it->second;
        if (b.ts < sessStart) continue;
        // the engine has not closed this bar's seconds yet - unless the tape stopped before the bar ended (no trade since; the
        // bar is long over): then it is decided with what there is
        if (b.te - 1 > eng.lastT && !(eng.lastTrade() < b.te && nowS - LATE_MARGIN - 60 > b.te)) break;
        H.book.feed(b, tfl::barFlow(eng.hist, eng.evs, b.ts, b.te, H.book.cfg));
    }
}
void TapeFlow::flushSignals()
{
    if (!S().book) return;
    BookHolder& H = *S().book;
    std::lock_guard<std::mutex> g(H.mx);
    if (H.book.journal.empty()) return;
    if (!S().storageReady || H.path.empty()) { H.book.journal.clear(); return; }
    bool fresh; { std::ifstream t(H.path.c_str(), std::ios::binary | std::ios::ate); fresh = !t.good() || t.tellg() == std::streampos(0); }
    std::ofstream o(H.path.c_str(), std::ios::app | std::ios::binary);
    if (!o.is_open()) return;                                          // kept in memory, retried next pass
    if (fresh) o << tfl::SignalBook::header() << "\n";
    for (const std::string& l : H.book.journal) o << l << "\n";
    o.flush();
    if (!o.good()) return;
    S().sigWrittenLines += (long long)H.book.journal.size();
    H.book.journal.clear();
}
// the previous session: its per-second file (display only, in front of today's) and its signal record (drawn only)
void TapeFlow::loadPrevious()
{
    S().prevLoaded = true;
    if (!S().storageReady || eng.curSid == LLONG_MIN) return;
    const long long prev = eng.curSid - 1;
    long long psid = prev;
    for (; psid >= eng.curSid - 4; --psid) { std::ifstream t((tfDir() + "\\" + mkt + "-sec-" + std::to_string(psid) + ".csv").c_str()); if (t.good()) break; }
    if (psid < eng.curSid - 4) return;
    std::vector<tfl::SecRec> v;
    {
        std::ifstream f((tfDir() + "\\" + mkt + "-sec-" + std::to_string(psid) + ".csv").c_str()); std::string ln;
        const long long firstNow = eng.hist.empty() ? LLONG_MAX : eng.hist.front().t;
        auto fnum = [](const std::string& s) -> float { if (s.empty()) return NAN; char* e = nullptr; float x = std::strtof(s.c_str(), &e); return e && *e == '\0' && std::isfinite(x) ? x : NAN; };
        while (std::getline(f, ln)) {
            if (ln.empty() || !(ln[0] >= '0' && ln[0] <= '9')) continue;
            std::vector<std::string> c; size_t a = 0;
            while (true) { size_t p = ln.find('|', a); c.push_back(ln.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; }
            if (c.size() < 10) continue;
            tfl::SecRec r; if (!tfl::parseStoreLong(c[0], &r.t)) continue;
            if (r.t >= firstNow || (!v.empty() && r.t <= v.back().t)) continue;
            r.f30 = fnum(c[2]); r.f180 = fnum(c[3]); r.a = fnum(c[4]); r.c30 = fnum(c[5]);
            long long x = 0; r.b = tfl::parseStoreLong(c[6], &x) && x >= 0 ? x : 0; r.s = tfl::parseStoreLong(c[7], &x) && x >= 0 ? x : 0; r.u = tfl::parseStoreLong(c[8], &x) && x >= 0 ? x : 0;
            int fl = 0; r.flags = (unsigned char)(tfl::parseStoreInt(c[9], &fl) ? fl : 0);
            v.push_back(r);
        }
    }
    if (!v.empty()) {
        S().secWrittenT = std::max(S().secWrittenT, v.back().t);     // never written again
        eng.hist.insert(eng.hist.begin(), v.begin(), v.end());
        S().indexedSize = (size_t)-1;
    }
    tfl::SignalBook pb; pb.loadText(slurpFile(sigPath(psid)));
    S().prevMarks = pb.marks;
    trace("previous session " + std::to_string(psid) + ": " + std::to_string(v.size()) + " seconds and " + std::to_string(pb.marks.size()) + " signals from its files");
}

// ---- the state, the header, the status file
std::string TapeFlow::stateText() const
{
    const tfl::StateInfo st = stateInfo();
    const std::string& c = st.code;
    char b[200];
    if (c == "WAIT") return "waiting for the chart";
    if (c == "NOTRADES") return "WAITING FOR TRADES";
    if (c == "ERROR") return "DATA / STORAGE ISSUE: " + err;
    if (c == "QUIET") { snprintf(b, sizeof(b), "QUIET - no trade for %lld s (more than %d s, 3 x this time of day's usual gap)", st.sinceTrade, st.quietAfter); return b; }
    if (c == "NOQUOTE") return "DATA INVALID - quote-at-trade snapshot unavailable";
    if (c == "LOWSIDES") { snprintf(b, sizeof(b), "LOW SIDE COVERAGE - signals unavailable (sides known %.0f%%)", std::isfinite(st.sides) ? st.sides * 100 : 0.0); return b; }
    if (c == "WIDESPREAD") return "WIDE SPREAD - signals unavailable";
    if (c == "NOSIDES") return "NO RECENT CLASSIFIED EXECUTION";
    if (c == "WATCH") return std::string("READY - absorption watch ") + (st.dir > 0 ? "(sellers stalled at " : "(buyers stalled at ") + fmtPx(st.zoneLo) + "-" + fmtPx(st.zoneHi) + ")";
    if (c == "SIGNAL") return std::string("READY - engine ") + (st.kind == "AR" ? "response " : "initiative ") + (st.dir > 0 ? "bullish" : "bearish");
    if (c == "WARM") return "WARMING UP " + std::to_string(st.n) + "/" + std::to_string(st.need) + " s";
    if (c == "CAL") {
        snprintf(b, sizeof(b), "CALIBRATING - %d of %d windows for this 5-min slot%s (%d sessions on file)", st.n, st.need,
                 st.k ? (" incl. +-" + std::to_string(st.k) + " slots").c_str() : "", eng.baseSessions);
        return b;
    }
    if (c == "LOWACT") return "LOW ACTIVITY";
    return "READY";
}

void TapeFlow::updateTitle()
{
    const tfl::StateInfo st = stateInfo();
    const tfl::Feat& f = eng.last;
    tfl::Mark last; bool haveLast = false;
    if (S().book) { std::lock_guard<std::mutex> g(S().book->mx); const tfl::Mark* m = S().book->book.lastConfirmed(); if (m) { last = *m; haveLast = true; } }
    const std::string t = tfl::titleLine(TF_VERSION, mkt, st, f.f180ok, f.f180, haveLast ? &last : nullptr, haveLast ? fmtPx(last.px) : std::string());
    { std::lock_guard<std::mutex> g(S().titleMx); S().title = t; }
    if (t != S().titleSeen) { S().titleSeen = t; S().titleChangedAt = time(0); S().invalidatePending = true; }
}
// IRT shows the title in the grey header only if it re-reads it after it changed; otherwise the pane shows it (one small line)
bool TapeFlow::headerLive() const
{
    if (S().titleCalls.load() <= 0) return false;
    const long long asked = S().lastTitleCall.load();
    return asked >= (long long)S().titleChangedAt || time(0) - S().titleChangedAt < 6;
}
int TapeFlow::parmsTitle(char* pStr, int size)
{
    if (!pStr || size <= 1) return RTX_FAIL;
    std::string t;
    { std::lock_guard<std::mutex> g(S().titleMx); t = S().title; }
    if (t.empty()) t = std::string("TF ") + tfl::shortVersion(TF_VERSION) + " " + mkt;
    S().titleCalls.fetch_add(1); S().lastTitleCall = (long long)time(0);
    snprintf(pStr, (size_t)size, "%s", t.c_str());
    return RTX_OK;
}

void TapeFlow::writeStatus()
{
    if (dir().empty()) return;
    const tfl::Feat& x = eng.last;
    const tfl::StateInfo st = stateInfo();
    long long prior = tfl::priorSessions(store, eng.curSid);
    const int complete = tfl::completeSessions(store, eng.curSid, COMPLETE_WINDOWS);
    const time_t now = time(0);
    auto num = [](double v, int dp) { if (!std::isfinite(v)) return std::string("nan"); char q[32]; snprintf(q, sizeof(q), "%.*f", dp, v); return std::string(q); };
    if (eng.curSid != S().lateSid) { S().lateSid = eng.curSid; S().lateBase = eng.late; }
    if (eng.late < S().lateBase) S().lateBase = eng.late;
    S().lateSamples.push_back(std::make_pair(now, eng.late));
    while (S().lateSamples.size() > 1 && now - S().lateSamples[1].first >= 300) S().lateSamples.erase(S().lateSamples.begin());
    const long long late5 = std::max(0LL, eng.late - S().lateSamples.front().second);
    int aw[2] = {0, 0}, ar[2] = {0, 0}, in[2] = {0, 0};
    for (const tfl::Ev& v : S().committed) {
        if (tfl::sessionOf(v.knownAt) != eng.curSid) continue;
        const int d = v.dir > 0 ? 0 : 1;
        if (v.kind == "AW") aw[d]++; else if (v.kind == "AR") ar[d]++; else if (v.kind == "IN" || v.kind == "INSUP") in[d]++;
    }
    const int bin = x.bin;
    const int binMin = (17 * 60 + bin * 5) % 1440;
    char slot[40]; snprintf(slot, sizeof(slot), "%02d:%02d-%02d:%02d", binMin / 60, binMin % 60, ((binMin + 5) % 1440) / 60, (binMin + 5) % 60);
    std::string title; { std::lock_guard<std::mutex> g(S().titleMx); title = S().title; }
    std::ostringstream f; f.imbue(std::locale::classic());
    // the 1.1.4 keys first, unchanged (LRA / ChartView readers)
    f << "VERSION," << TF_VERSION << "\nMARKET," << mkt << "\nCHART," << root << "\nSYMBOL," << sym << "\nTICK," << tick
      << "\nTIMER," << (timerOn ? "on" : timerRefused ? "REFUSED (calc-only fallback; backfill paused)" : "starting")
      << "\nDATA,trades " << eng.ticks << ",live " << liveTicks << ",late " << eng.late << ",last " << (eng.lastT ? stamp(eng.lastT) : "-")
      << "\nBACKFILL,next step " << (step < NSTEPS ? STEPS[step].name : "done") << "," << stepNote
      << "\nCALIBRATION,prior sessions on file " << prior << ",slot windows " << x.bb()->n << ",baseline version " << eng.ver
      << "\nNOW,f30 " << (x.f30ok ? x.f30 : NAN) << ",f180 " << (x.f180ok ? x.f180 : NAN) << ",activity " << (x.aok ? x.A : NAN) << ",sides " << x.c30
      << "\nSTATE," << stateText() << "\nWHY_NO_SIGNAL," << eng.lastWhy << "\nEVENTS," << eng.evs.size() << "\nERROR," << err;
    f << "\nSTATE_CODE," << st.code
      << "\nTITLE," << title
      << "\nHEADER,parmsTitle calls " << S().titleCalls.load() << ",last asked " << (S().lastTitleCall.load() ? std::to_string((long long)now - S().lastTitleCall.load()) + " s ago" : std::string("never"))
      << ",title changed " << (S().titleChangedAt ? std::to_string((long long)(now - S().titleChangedAt)) + " s ago" : std::string("-"))
      << ",header live " << (headerLive() ? "yes" : "no") << ",pane line " << (!headerLive() ? "shown" : "hidden")
      << "\nCAL_SLOT,bin " << bin << " (" << slot << "),windows " << x.bb()->n << ",need " << eng.cfg.minWin << ",own " << x.bb()->own << ",pooled k " << x.bb()->k
      << ",calibrated " << (x.baseOk ? "yes" : "no") << ",med20 " << num(x.bb()->med20, 3) << ",med5 " << num(x.bb()->med5, 3) << ",med spread " << num(x.bb()->medSpread, 1)
      << "\nCAL_SESSIONS,used " << eng.baseSessions << ",dropped thin " << eng.baseDropped << ",on file " << prior << ",complete " << complete
      << " (>= " << COMPLETE_WINDOWS << " windows),target " << eng.cfg.sessionsBack;
    for (int i = 0; i < NSTEPS; i++)
        f << "\nBACKFILL_" << STEPS[i].name << "," << (S().stepResult[i].empty() ? (i < step ? std::string("done") : std::string("pending")) : S().stepResult[i])
          << (blocked[i] ? ",BLOCKED" : "");
    if (S().sess.active) f << ",replaying " << S().sess.pos << " of " << S().sess.ticks.size() << " trades";
    f << "\nBACKFILL_calibration," << S().deepNote << ",sessions measured this run " << S().deepSessionsDone << ",no-trade sessions " << S().deepEmpty.size()
      << ",history ends " << (S().deepExhaustedBelow == LLONG_MIN ? std::string("-") : std::to_string(S().deepExhaustedBelow)) << (S().deepBlocked ? ",BLOCKED" : "");
    f << "\nQUIET,since last trade " << (st.sinceTrade < 0 ? std::string("-") : std::to_string(st.sinceTrade) + " s")
      << ",typical gap " << num(x.gapTypical, 1) << " s (" << (x.gapFromBase ? "this slot" : "live") << "),quiet after " << (int)x.quoteAgeEff << " s"
      << ",quote " << (x.quoteOk ? "current" : x.quoteResumed ? "resumed" : "not current") << ",hard stop " << eng.cfg.maxQuietGap << " s"
      << "\nFLOW,f30 " << (x.f30ok ? num(x.f30, 1) : std::string("-")) << ",f180 " << (x.f180ok ? num(x.f180, 1) : std::string("-"))
      << ",activity " << (x.aok ? num(x.A, 2) : std::string("-")) << ",sides 30s " << num(x.c30 * 100, 0) << "%,sides 180s " << num(x.c180 * 100, 0) << "%"
      << ",rate20 " << num(x.rate20, 2) << ",warm " << eng.warmN << "/" << eng.cfg.warm << ",range +-" << num(S().range, 0)
      << "\nSIGNALS_TODAY,AW+ " << aw[0] << ",AW- " << aw[1] << ",AR+ " << ar[0] << ",AR- " << ar[1] << ",IN+ " << in[0] << ",IN- " << in[1]
      << "\nLATE,today " << (eng.late - S().lateBase) << ",last 5 min " << late5 << ",total " << eng.late;
    // (2.0.0) the pane's signals today, newest first (letter, side, state, bar time, price, why)
    {
        std::vector<tfl::Mark> ms; long long through = LLONG_MIN; std::string path;
        if (S().book) { std::lock_guard<std::mutex> g(S().book->mx); ms = S().book->book.marks; through = S().book->book.decidedThrough; path = S().book->path; }
        int cnt[4][3] = {{0}}; const char* K = "PEAF";
        for (const tfl::Mark& m : ms) { const char* p = std::strchr(K, m.kind); if (p) cnt[p - K][m.state]++; }
        f << "\nTF_SIGNALS,decided through " << (through == LLONG_MIN ? std::string("-") : stamp(through)) << ",bars on chart " << S().bars.size() << ",record " << path
          << ",lines written " << S().sigWrittenLines << (S().sessionDone ? "" : ",WAITING for the session replay");
        f << "\nTF_COUNTS";
        for (int k = 0; k < 4; ++k) f << "," << K[k] << " " << cnt[k][tfl::MK_CONFIRMED] << " confirmed / " << cnt[k][tfl::MK_PENDING] << " open? / " << cnt[k][tfl::MK_EXPIRED] << " never confirmed";
        int shown = 0;
        for (auto it = ms.rbegin(); it != ms.rend() && shown < 12; ++it, ++shown)
            f << "\nTF_SIG" << (shown + 1) << "," << it->kind << (it->question() ? "?" : "") << "," << (it->dir > 0 ? "bullish" : "bearish") << ","
              << (it->state == tfl::MK_CONFIRMED ? "confirmed" : it->state == tfl::MK_EXPIRED ? "never confirmed" : "open") << "," << clock12(it->barT) << ","
              << fmtPx(it->px) << "," << tf_support::safeField(it->why);
    }
    for (size_t i = 0; i < 5; i++) f << "\nWARN" << (i + 1) << "," << (i < S().warns.size() ? S().warns[S().warns.size() - 1 - i] : std::string());
    const std::string body = f.str();
    if (body == S().statusBody && now - S().statusWrittenAt < 60) return;
    const std::string all = body + "\nUPDATED," + stamp(localSec(currentDate())) + "\n";
    const std::string path = dir() + "\\TapeFlow.status-" + mkt + ".txt", tmp = path + ".tmp";
    {
        std::ofstream o(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!o.is_open()) return;
        o.write(all.data(), (std::streamsize)all.size()); o.flush();
        if (!o.good()) { o.close(); std::remove(tmp.c_str()); return; }
    }
#ifdef _WIN32
    if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { std::remove(tmp.c_str()); return; }
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { std::remove(tmp.c_str()); return; }
#endif
    S().statusBody = body; S().statusWrittenAt = now;
}

void TapeFlow::warn(const std::string& s)
{
    time_t n = time(0); struct tm lt; localtime_s(&lt, &n);
    char b[24]; strftime(b, sizeof(b), "%m-%d %H:%M:%S", &lt);
    S().warns.push_back(std::string(b) + " " + tf_support::safeField(s));
    if (S().warns.size() > 5) S().warns.erase(S().warns.begin());
}

void TapeFlow::trace(const std::string& s)
{
    if (dir().empty()) return;
    std::string p = dir() + "\\TapeFlow.trace-" + mkt + ".txt";
    { std::ifstream t(p.c_str(), std::ios::ate | std::ios::binary); if (t.good() && (long long)t.tellg() > 300000) { t.close(); std::remove(p.c_str()); } }
    std::ofstream f(p.c_str(), std::ios::app); if (!f.is_open()) return;
    time_t n = time(0); struct tm lt; localtime_s(&lt, &n);
    char b[24]; strftime(b, sizeof(b), "%m-%d %H:%M:%S", &lt);
    f << b << " " << TF_VERSION << " " << s << "\n";
    if (s.find("DATA INVALID") != std::string::npos || s.find("SLOW") == 0 || s.find("BLOCKED") != std::string::npos ||
        s.find("refused") != std::string::npos || s.find("contract changed") == 0 || s.find("one more try") != std::string::npos) warn(s);
}

// ---------------------------------------------------------------- drawing
static COLOR blend(COLOR c, COLOR bg, double a)
{
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    int r = (int)(((c >> 16) & 0xFF) * a + ((bg >> 16) & 0xFF) * (1 - a));
    int g = (int)(((c >> 8) & 0xFF) * a + ((bg >> 8) & 0xFF) * (1 - a));
    int b = (int)((c & 0xFF) * a + (bg & 0xFF) * (1 - a));
    return (COLOR)((r << 16) | (g << 8) | b);
}

// a small SOLID arrowhead (filled triangle, no outline): tip at (x, tipY), pointing up or down
void TapeFlow::arrowhead(short x, short tipY, bool up, COLOR c, int half, int height)
{
    for (int k = 0; k < height; k++) {
        const int w = (k * half + height / 2) / std::max(1, height - 1);
        const short yy = (short)(up ? tipY + k : tipY - k);
        line((short)(x - w), yy, (short)(x + w + 1), yy, c, 1);
    }
}

int TapeFlow::draw(void)
{
    pump("draw");
    render();
    return RTX_OK;
}

// the biggest |value| the pane shows for bars b0..b1: the 30-s sub-bars (same buckets as render) and the 180-s line
double TapeFlow::visibleMaxAbs(int b0, int b1)
{
    const std::vector<tfl::SecRec>& H = eng.hist;
    long n = getBarCount();
    if (H.empty() || n <= 0) return 0;
    if (b1 >= n) b1 = (int)n - 1;
    if (b0 < 0) b0 = 0;
    if (b1 < b0) return 0;
    const long long firstTime = H.front().t, lastTime = H.back().t;
    if (S().indexedSize != H.size() || S().indexedFirst != firstTime || S().indexedLast != lastTime) {
        S().renderIndex.build(H, tfl::SR_SIDES);
        S().indexedSize = H.size(); S().indexedFirst = firstTime; S().indexedLast = lastTime;
    }
    const auto& index = S().renderIndex;
    int spb = getSecondsPerBar(); if (spb <= 0) return 0;
    int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
    RTARRAYI dt(barDateTime);
    double m = 0;
    for (int i = b0; i <= b1; i++) {
        long long te = localSec((RTDATE)dt[i]);
        long long ts = i > 0 ? localSec((RTDATE)dt[i - 1]) : te - spb;
        if (te - ts > 4 * spb || te <= ts) ts = te - spb;
        int span = std::max(1, (int)(ppb * 0.9));
        int slots = (int)((te - ts + 29) / 30); slots = std::max(1, std::min(slots, std::max(1, span / 2)));
        for (int sl = 0; sl < slots; sl++) {
            long long a0 = ts + (te - ts) * sl / slots, a1 = ts + (te - ts) * (sl + 1) / slots;
            const size_t j0 = index.start(a0), j1 = index.start(a1);
            const long double sb = index.buys[j1] - index.buys[j0], ss = index.sells[j1] - index.sells[j0];
            if (sb + ss > 0) m = std::max(m, std::fabs((double)(100 * (sb - ss) / (sb + ss))));
        }
        const size_t k = index.start(te);
        if (k > 0 && H[k - 1].t >= ts && std::isfinite(H[k - 1].f180)) m = std::max(m, std::fabs((double)H[k - 1].f180));
    }
    return m;
}

int TapeFlow::scale(int iStartBar, int iEndBar, double* dMin, double* dMax)
{
    double r = S().range;
    std::unique_lock<std::recursive_mutex> lock(S().access, std::try_to_lock);
    if (lock.owns_lock()) { r = rangeFor(iStartBar, iEndBar); S().range = r; }
    if (!(r > 0) || !std::isfinite(r)) r = 60;
    r = tfl::axisRange(r, S().paneH, S().band);                       // IRT's axis = the drawing's inner area
    if (dMin) *dMin = -r;
    if (dMax) *dMax = r;
    return RTX_OK;
}

void TapeFlow::render()
{
    std::unique_lock<std::recursive_mutex> lock(S().access,std::try_to_lock);
    if (!lock.owns_lock()) return;
    RCT pane; pane.getPaneRect(false);
    short L = pane.left, T = pane.top, R = pane.right, B = pane.bottom;
    if (R - L < 60 || B - T < 30) return;
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    std::string chartMkt = marketForRoot(rs ? rs : "");
    const int fs = FONT_PT;
    const char* activeSymbol = getSymbol();
    if (activeSymbol && !sym.empty() && sym != activeSymbol) { textLJ((short)(L+8),(short)(T+12),"TapeFlow: one contract per indicator - add it again on this chart",C_GRAY,fs,false); return; }
    if (chartMkt != mkt) {
        std::string s = std::string("TapeFlow ") + mkt + " " + TF_VERSION + " - this one is for the " + mkt + " chart (add TapeFlow" + (chartMkt.empty() ? std::string("<market>") : chartMkt) + " here)";
        textLJ((short)(L + 8), (short)(T + 12), s.c_str(), C_GRAY, fs, false);
        return;
    }
    // layout: [the header row: IRT's grey title - or our fallback line] [bearish band] [bars + line] [bullish band]. The header row is
    // always kept free so a signal never sits under IRT's title text.
    const int paneH = B - T;
    const bool tinyPane = paneH < 90;   // (not "small": MSVC's windows.h #defines small)
    const int band = tinyPane ? 12 : 22;                                  // small panes: arrowheads only, no letters
    const bool fallback = !headerLive();
    const int headRow = fs + 6;
    const int top = T + headRow;
    S().paneH = paneH; S().band = band + headRow / 2;
    const short yTopBand = (short)(top), yInnerTop = (short)(top + band), yInnerBot = (short)(B - band);
    long n = getBarCount();
    int b0 = 0, b1 = (int)n - 1;
    if (getVisibleBars(&b0, &b1) != RTX_OK) { b0 = std::max(0, (int)n - 200); b1 = (int)n - 1; }
    if (b1 >= n) b1 = (int)n - 1;
    if (b0 < 0) b0 = 0;
    const double rng = rangeFor(b0, b1); S().range = rng;
    const double mid = (yInnerTop + yInnerBot) / 2.0, half = std::max(4.0, (yInnerBot - yInnerTop) / 2.0);
    auto Y = [&](double v) { return (short)(mid - v / rng * half + 0.5); };
    line(L, Y(0), R, Y(0), blend(C_GRAY, C_BG, 0.55), 1);           // the faint zero line
    const std::vector<tfl::SecRec>& H = eng.hist;
    const long long firstTime=H.empty()?LLONG_MIN:H.front().t, lastTime=H.empty()?LLONG_MIN:H.back().t;
    if (S().indexedSize!=H.size() || S().indexedFirst!=firstTime || S().indexedLast!=lastTime) {
        S().renderIndex.build(H,tfl::SR_SIDES);
        S().indexedSize=H.size(); S().indexedFirst=firstTime; S().indexedLast=lastTime;
    }
    const auto& index=S().renderIndex;
    std::vector<std::pair<tfl::Mark, short> > marks;                 // the visible signals and their x
    if (n > 0 && b1 >= b0) {
        RTARRAYI dt(barDateTime);
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        int spb = getSecondsPerBar();
        if (spb<=0) { textLJ((short)(L+8),(short)(T+12),"TapeFlow needs a time-based chart (your 3-minute chart)",C_GRAY,fs,false); return; }
        // the signals by bar time (today's book + the previous session's record)
        std::map<long long, tfl::Mark> byT;
        for (const tfl::Mark& m : S().prevMarks) byT[m.barT] = m;
        if (S().book) { std::lock_guard<std::mutex> g(S().book->mx); for (const tfl::Mark& m : S().book->book.marks) byT[m.barT] = m; }
        auto idxAtOrBefore = [&](long long t) -> long {
            long lo = 0, hi = (long)H.size();
            while (lo < hi) { long m = (lo + hi) / 2; if (H[(size_t)m].t <= t) lo = m + 1; else hi = m; }
            return lo - 1;
        };
        short px180 = -1, py180 = -1;
        for (int i = b0; i <= b1; i++) {
            long long te = localSec((RTDATE)dt[i]);
            long long ts = i > 0 ? localSec((RTDATE)dt[i - 1]) : te - spb;
            if (te - ts > 4 * spb || te <= ts) ts = te - spb;
            PNT p; p.set(i, 0.0f, kBarCenter); short x = p.h;
            { auto mk = byT.find(te); if (mk != byT.end()) marks.push_back(std::make_pair(mk->second, x)); }
            if (H.empty()) continue;
            long k = idxAtOrBefore(te-1);
            if (k < 0 || H[(size_t)k].t < ts) { px180 = -1; continue; }
            const tfl::SecRec& q = H[(size_t)k];
            {   // 30-s pressure bars: 6 per 3-min candle at normal zoom; at low zoom fewer, wider buckets (never overlapping)
                int span = std::max(1, (int)(ppb * 0.9));
                int slots=(int)((te-ts+29)/30);
                slots=std::max(1,std::min(slots,std::max(1,span/2)));
                double sw = (double)span / slots;
                short xl = (short)(x - span / 2);
                for (int sl = 0; sl < slots; sl++) {
                    long long a0=ts+(te-ts)*sl/slots, a1=ts+(te-ts)*(sl+1)/slots;
                    const size_t j0=index.start(a0), j1=index.start(a1);
                    const long double sb=index.buys[j1]-index.buys[j0], ss=index.sells[j1]-index.sells[j0];
                    const size_t nall=j1-j0, nq=index.quality[j1]-index.quality[j0];
                    const size_t na=index.activityCount[j1]-index.activityCount[j0];
                    const long double sa=index.activity[j1]-index.activity[j0];
                    if (!(sb+ss>0)) continue;
                    const double fb=(double)(100*(sb-ss)/(sb+ss));
                    // coloured when most seconds have known sides and the tape is not quiet (< 0.5x normal); brightness = how busy
                    const bool quiet = na > 0 && (double)(sa / na) < 0.5;
                    const bool good = nall > 0 && nq * 2 >= nall && !quiet;
                    double al = na ? std::max(0.30, std::min(0.90, 0.30 + 0.30 * (double)(sa / na))) : 0.50;
                    COLOR c = good ? blend(fb >= 0 ? C_BUY : C_SELL, C_BG, al) : blend(C_GRAY, C_BG, 0.6);
                    short x1 = (short)(xl + (int)(sl * sw)), x2 = (short)(xl + (int)((sl + 1) * sw) - (sw >= 3 ? 1 : 0));
                    const long long completedThrough=std::min(a1,H.back().t+1);
                    const double completedFraction=std::max(0.0,std::min(1.0,(double)(completedThrough-a0)/(double)(a1-a0)));
                    x2=std::min(x2,(short)(xl+(sl+completedFraction)*sw));
                    if (x2 < x1) x2 = x1;
                    short y0 = Y(0), y1 = Y(fb);
                    fill(x1, std::min(y0, y1), x2, std::max(y0, y1), c);
                }
            }
            if (!std::isnan(q.f180)) {
                short y = Y(q.f180);
                const double position=std::max(0.0,std::min(1.0,(double)(q.t+1-ts)/(double)(te-ts)));
                const short sampleX=(short)(x+(position-0.5)*ppb*0.9);
                if (px180 >= 0) line(px180, py180, sampleX, y, C_LINE, 2);
                px180 = sampleX; py180 = y;
            } else px180 = -1;
        }
        // the signals: one letter + a solid arrowhead, bearish in the top band (letter on top, arrowhead pointing down at the tape),
        // bullish in the bottom band (arrowhead pointing up, letter under it). Never two touching: the stronger one wins the spot.
        std::vector<tfl::Slot> slots(marks.size());
        std::vector<std::string> labels(marks.size());
        for (size_t i = 0; i < marks.size(); ++i) {
            const tfl::Mark& m = marks[i].first;
            labels[i] = std::string(1, m.kind) + (m.question() ? "?" : "");
            const int tw = tinyPane ? 0 : textW(labels[i].c_str(), fs, true);
            slots[i].x = marks[i].second; slots[i].w = std::max(11, tw) + 2; slots[i].band = m.dir > 0 ? 1 : -1;
            slots[i].rank = tfl::markRank(m); slots[i].t = m.barT;
        }
        tfl::layoutMarks(slots);
        for (size_t i = 0; i < marks.size(); ++i) {
            if (!slots[i].show) continue;
            const tfl::Mark& m = marks[i].first; const short x = marks[i].second;
            const COLOR c = m.dir > 0 ? C_BUY : C_SELL;
            if (m.dir < 0) {                                               // top band: letter, then the arrowhead below it
                if (!tinyPane) { textC(x, (short)(yTopBand + fs / 2 + 3), labels[i].c_str(), c, fs, true, slots[i].w); arrowhead(x, (short)(yInnerTop - 1), false, c, 5, 6); }
                else arrowhead(x, (short)(yInnerTop - 2), false, c, 4, 6);
            } else {                                                       // bottom band: the arrowhead, the letter under it
                if (!tinyPane) { arrowhead(x, (short)(yInnerBot + 1), true, c, 5, 6); textC(x, (short)(B - fs / 2 - 3), labels[i].c_str(), c, fs, true, slots[i].w); }
                else arrowhead(x, (short)(yInnerBot + 2), true, c, 4, 6);
            }
        }
    }
    // the fallback: IRT did not re-read the header title -> the same text, one small grey line at the top-left (no box)
    if (fallback) {
        std::string t; { std::lock_guard<std::mutex> g(S().titleMx); t = S().title; }
        if (t.empty()) t = std::string("TF ") + tfl::shortVersion(TF_VERSION) + " " + mkt;
        textLJ((short)(L + 6), (short)(T + fs + 3), t.c_str(), C_GRAY, fs, false);
    }
}

extern "C" cppExtension *CreateExtension(void)
{
    TapeFlow *p = new TapeFlow();
    p->setArrayCount(2);
    p->setFlags(POST_DRAWING | NO_UI);                             // its own pane (no OVERLAY), draws itself
    p->setExtendedFlags(CALL_CONTINUOUSLY);
    std::string d = std::string("LRA TapeFlow ") + TF_FIXED + " " + TF_VERSION + ": who is hitting the tape - 30-s pressure bars and the 180-s line - with "
                    "one-letter signals: P push, E exhaustion, A absorption, F flip (green = bullish, red = bearish, '?' until confirmed). "
                    "Signals are decided on closed bars only and never change. Put it on the " + TF_FIXED + " chart only.";
    static std::string desc; desc = d;
    p->setDescription(desc.c_str());
    p->setVersion(TF_VERSION);
    return p;
}
