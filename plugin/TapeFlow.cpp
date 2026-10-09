/********************************************************************************
 *  TapeFlow.cpp  --  Investor/RT RTX extension  lsTapeFlow<MKT>  (v1.0.3, 2026-10-08)
 *
 *  Rassul 2026-10-08 13:17-13:37: the TapeFlow brief; "i'll take your recommendation but i want to get it on all the markets,
 *  if signals are tentative just add ?"; "the markers should be on the indicator on a separate pane"; "keep levels out of it,
 *  similar to the delta profile"; "build for irt ... add backfill capabilities similar to other indicators and it must be
 *  native without any outside dependencies"; "make sure you test and check for errors and inconsistencies".
 *
 *  ONE DLL PER MARKET (TapeFlowES ... TapeFlowEU = this file with TF_FIXED): IRT gives each DLL one object and one timer,
 *  and every data request is made from that timer - the pattern that keeps the Footprint Readers stable (IRTReader 0.5.0).
 *  Put TapeFlow<MKT> on that market's chart (any bar size; his 3-min chart). It opens in its own pane below the price.
 *
 *  Data: IRT's own trades for the chart's contract (RTTICKS: time, price, size, bid, ask) - nothing else, no files from
 *  anyone else. The logic (pressure, activity, baselines, AW / AR / IN, outcomes) is TapeFlowLogic.h (tested: 53 behaviour
 *  tests from the brief + 300 random tapes for mirror symmetry, prefix / redraw invariance and memory safety).
 *  Back-fill: on load it reads the last 10 minutes, then (guarded, one step at a time, 20 s apart) the last hour, 4 hours
 *  and the whole session from 17:00, and redraws the pane from each. Baselines: its own record of every 20-s window it has
 *  seen (lsFlexLevels\TapeFlow\<MKT>-base-<session>.csv, one per session, written by this plugin only; each session's
 *  front contract is used, so a roll does not restart the calibration); when fewer than 10 prior sessions are on file
 *  it also reads older trades from IRT (3, 6, 11 days back) OUTSIDE trading hours, each step guarded. Signals start once a
 *  5-minute slot has 60 windows (about 4 sessions); until then the pane shows the pressure and CALIBRATING.
 *  Crash guard: a request leaves lsFlexLevels\TapeFlow\_trying-<MKT>-<step>.txt until IRT survives it; found at the next
 *  start twice in a row = that step is blocked for good (delete _blocked-<MKT>-<step>.txt to allow it again).
 *  Outputs it writes (for the nightly scoring, never read back for the display):
 *     lsFlexLevels\TapeFlow\<MKT>-events-<session>.csv   every committed event with its signal-time numbers
 *     lsFlexLevels\TapeFlow.status-<MKT>.txt             version, data, back-fill, calibration, state
 *  Every marker carries "?" until the events are scored and proven (TF_PROVEN).
 ********************************************************************************/
#ifndef TF_FIXED
#error "build TapeFlowES.cpp ... TapeFlowEU.cpp (each defines TF_FIXED and includes this file)"
#endif
#include "irtsdk.h"
#include "TapeFlowLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
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

static const char* TF_VERSION = "1.0.3";   // (1.0.3) the histogram is 30-SEC pressure bars (6 per 3-min candle) as designed, not one last-30-s snapshot per candle   // (1.0.2) audit 1.0.1 fixes, but one engine per DLL (timer-safe)
static const bool TF_PROVEN = false;
static const int LATE_MARGIN = 8;            // seconds a quiet second waits for late trades before it is evaluated          // the "?" goes when the nightly scoring proves the signals

static const COLOR C_BUY    = 0x0022C55E;
static const COLOR C_SELL   = 0x00EF4444;
static const COLOR C_AMBER  = 0x00F59E0B;
static const COLOR C_PURPLE = 0x00A78BFA;
static const COLOR C_INK    = 0x00E5E7EB;
static const COLOR C_MUTED  = 0x009CA3AF;
static const COLOR C_GRID   = 0x00334155;
static const COLOR C_ZERO   = 0x00475569;
static const COLOR C_BAND   = 0x00141A24;
static const COLOR C_BOX    = 0x000B0E14;
static const COLOR C_GRAY   = 0x0064748B;
static const COLOR C_BG     = 0x00000000;   // the chart background the opacity blends into

static int timerIdFor(const void* me) { return 5711 + (int)(((uintptr_t)me >> 4) % 100000); }

// local wall-clock seconds (the date / time fields as if UTC): the 17:00 session and 5-min bins follow the clock through DST
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
// This extension only needs the root-to-market lookup, not DealerLogic's file
// parser and rendering helpers. Keeping this small fixed-market lookup local
// avoids a production dependency on that unrelated shared implementation.
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

struct TCursor {                 // trades already taken: the last second and how many of its trades
    long long lastSec = -1; int doneInSec = 0, seen = 0;
    void rewind() { seen = 0; }
    bool take(long long sec)
    {
        if (sec < lastSec) return false;
        if (sec == lastSec) { if (seen < doneInSec) { seen++; return false; } doneInSec++; seen++; return true; }
        lastSec = sec; doneInSec = 1; seen = 1; return true;
    }
};

struct TStep { const char* name; int back; bool deep; };
static const TStep STEPS[] = { {"10m", 600, false}, {"1h", 3600, false}, {"4h", 4 * 3600, false}, {"session", -1, false},
                               {"3d", 3 * 86400, true}, {"6d", 6 * 86400, true}, {"11d", 11 * 86400, true} };
static const int NSTEPS = (int)(sizeof(STEPS) / sizeof(STEPS[0]));

struct TIdx { int fontParam; };
static TIdx TX;

// The SDK can dispatch one extension object for different chart hosts.  Keep every
// mutable value in the host's user-data slot instead of letting a chart's tape,
// timer fallback and file bookkeeping leak into another chart.
struct TapeFlowState {
    std::string mkt = TF_FIXED, root, sym, chartRoot, err;
    double tick = 0;
    tfl::Engine eng; tfl::Store store; bool storeLoaded = false;
    TCursor cur;
    time_t wallAtTick = 0; long long dataAtTick = -1;
    bool timerOn = false, timerRefused = false, busy = false;
    time_t firstPass = 0, lastTimerTick = 0, timerAt = 0, lastStepAt = 0, lastSave = 0, lastEvWrite = 0, lastStatus = 0, lastFallback = 0;
    int step = 0;                       // the next back-fill step
    bool blocked[NSTEPS] = {false}; bool guardsRead = false; bool deepDone = false;
    std::string stepNote, lastStepName;
    long long stepTicks[NSTEPS] = {0};
    size_t evWritten = (size_t)-1; long long evLastT = 0;
    std::set<std::string> written; std::set<long long> keysLoaded;
    std::map<long long, size_t> savedCount;
    long long liveTicks = 0; int passes = 0;
    int font = 9;
};

class TapeFlow : public cppExtension {
public:
    TapeFlow() : cppExtension() {}
    ~TapeFlow() { delete own_; }
    virtual int timer(RTX_EVENT* e);
    virtual int draw(void);
    virtual int parmsLoad(void)  { readSettings(); return RTX_OK; }
    virtual int parmsApply(void) { readSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { readSettings(); return RTX_OK; }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = -135; if (dMax) *dMax = 135; return RTX_OK; }

    // (1.0.2) ONE state per DLL, not per host user-data slot: each lsTapeFlow<MKT> DLL serves ONE market (TF_FIXED) and its
    // 1-s timer callback is not guaranteed to run in a chart's user-data context - a per-host slot could split the engine fed
    // by the timer from the one the chart draws. The audit's other 1.0.1 fixes are kept.
    mutable TapeFlowState* own_ = nullptr;
    TapeFlowState* existingState() const { return own_; }
    TapeFlowState& S() const
    {
        if (!own_) own_ = new TapeFlowState();
        return *own_;
    }

    std::string evPath(long long sid) const;

    void readSettings() { int f = getIntegerValue(TX.fontParam); S().font = (f >= 6 && f <= 16) ? f : 9; }
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
    std::string dir() const { const char* up = getenv("USERPROFILE"); return up ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
    std::string tfDir() const { return dir() + "\\TapeFlow"; }
    void trace(const std::string& s);
    void live();
    bool rebuild(int idx);
    void runSteps();
    void loadStore();
    void saveStore();
    std::string basePath(long long sid) const;
    void writeEvents(bool force);
    void writeStatus();
    std::string stateText(COLOR* col) const;
    bool offHours() const;
    // guard files
    std::string guardPath(const char* kind, const char* name) const { return tfDir() + "\\_" + kind + "-" + S().mkt + "-" + name + ".txt"; }
    void readGuards();
    void guardWrite(int idx);
    void guardClear(int idx);
    // drawing
    void render();
    int textW(const char* s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s, -1); }
    void textLJ(short x, short y, const char* s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
        RCT rc; rc.set(x, (short)(yy - sz), (short)(x + 600), (short)(yy + sz)); rc.drawText(s, false, false);
    }
    void line(short x1, short y1, short x2, short y2, COLOR c, int w)
    {
        setPen(c, (short)w, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
        PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
    }
    void fill(short l, short t, short r, short b, COLOR c) { if (r < l) std::swap(l, r); if (b < t) std::swap(t, b); RCT rc; rc.set(l, t, (short)(r + 1), (short)(b + 1)); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
    void triangle(short x, short y, bool up, bool filled, COLOR c);
};

#define mkt (S().mkt)
#define root (S().root)
#define sym (S().sym)
#define chartRoot (S().chartRoot)
#define err (S().err)
#define tick (S().tick)
#define eng (S().eng)
#define store (S().store)
#define storeLoaded (S().storeLoaded)
#define cur (S().cur)
#define wallAtTick (S().wallAtTick)
#define dataAtTick (S().dataAtTick)
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
#define deepDone (S().deepDone)
#define stepNote (S().stepNote)
#define lastStepName (S().lastStepName)
#define stepTicks (S().stepTicks)
#define evWritten (S().evWritten)
#define evLastT (S().evLastT)
#define written (S().written)
#define keysLoaded (S().keysLoaded)
#define savedCount (S().savedCount)
#define liveTicks (S().liveTicks)
#define passes (S().passes)
#define font (S().font)

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
    if (storeLoaded) { eng.flushSession(); me->saveStore(); }
    me->writeEvents(true);
    for (int i = 0; i < NSTEPS; i++) me->guardClear(i);          // completion, not a crashed request
    return RTX_OK;
}
int cppExtension::destroy(void)
{
    TapeFlow* me = static_cast<TapeFlow*>(this);
    TapeFlowState* state = me->existingState();
    if (!state) return RTX_OK;
    auto S = [&]() -> TapeFlowState& { return *state; };
    if (timerOn) { destroyTimer(timerIdFor(state)); timerOn = false; }
    if (storeLoaded) { eng.flushSession(); me->saveStore(); }
    me->writeEvents(true);
    for (int i = 0; i < NSTEPS; i++) me->guardClear(i);          // a normal close is not a crash
    delete state;
    me->own_ = nullptr;
    return RTX_OK;
}

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(2);
    TX.fontParam = getParameterCount(); setIntegerParameter("Font pt", 9, 50);
    // two invisible outputs keep the pane's own scale at +-135 (the drawing does not depend on it)
    CPEN p(C_GRAY, 1, P_SOLID);
    setOutputParameter("Top", DRAW_INVISIBLE, &p, C_GRAY, OUTPUT_ENABLED);
    setOutputParameter("Bottom", DRAW_INVISIBLE, &p, C_GRAY, OUTPUT_ENABLED);
    return RTX_OK;
}

int cppExtension::calc(int iStartBar)
{
    TapeFlow* me = static_cast<TapeFlow*>(this);
    long n = getBarCount();
    if (n > 0) {
        RTARRAY o1(fOut1), o2(fOut2);
        int from = iStartBar > 0 ? iStartBar : 0;
        for (long i = from; i < n; i++) { o1[(int)i] = 135.0f; o2[(int)i] = -135.0f; }
    }
    me->pump("calc");
    return RTX_OK;
}

// the timer drives everything; draw / calc only start it (requests from draw hung IRT in IRTReader 0.4.0). Only if IRT refuses
// the timer, or no tick comes for 10 s, does a pass run from calc / draw (at most once a second).
void TapeFlow::pump(const char* via)
{
    time_t now = time(0);
    if (!timerOn && !timerRefused) {
        if (createTimer(timerIdFor(&S()), 1000) == RTX_OK) { timerOn = true; timerAt = now; trace(std::string("timer granted (") + via + ")"); }
        else { timerRefused = true; trace("timer REFUSED - running from calc / draw"); }
    }
    // (IRTReader's thresholds) a pass from calc / draw only if the timer was refused, never ticked within 20 s of being granted,
    // or stopped for 60 s; and then only the small live read - the back-fill steps run from the timer only (or with no timer at all)
    bool noTimer = timerRefused || (timerOn && (lastTimerTick ? now - lastTimerTick > 60 : now - timerAt > 20));
    if (noTimer && now != lastFallback) { lastFallback = now; pass(timerRefused); }
}

int TapeFlow::timer(RTX_EVENT* e)
{
    if (!e || e->v.timer.id != timerIdFor(&S())) return RTX_FAIL;
    lastTimerTick = time(0);
    pass(true);
    lastTimerTick = time(0);                                      // a long pass (a back-fill) is not a stopped timer
    return RTX_OK;
}

bool TapeFlow::identify()
{
    char buf[32] = {0}; const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    std::string m = marketForRoot(root);
    if (m != mkt) return false;                                   // this DLL serves ONE market: elsewhere it only says so
    const char* s = getSymbol(); std::string sy = s ? s : "";
    tick = marketTick(mkt);
    if (!(tick > 0)) { float t = getProperty(SYM_TICKINCR); tick = (t > 0 && t < 1000) ? t : 0; }
    if (!sy.empty() && sy != sym) {
        if (!sym.empty()) {                                        // the chart moved to another contract: start over on it
            trace("contract changed " + sym + " -> " + sy + ": starting over");
            eng.flushSession(); saveStore();
            eng = tfl::Engine(); cur = TCursor(); dataAtTick = -1; step = 0; deepDone = false; evWritten = (size_t)-1;
        }
        sym = sy; eng.attach(&store, sym);
    }
    return tick > 0 && !sym.empty();
}

void TapeFlow::pass(bool fromTimer)
{
    if (busy) return;
    busy = true;
    if (!identify()) { busy = false; return; }
    if (!firstPass) { firstPass = time(0); _mkdir(dir().c_str()); _mkdir(tfDir().c_str()); trace(std::string("loaded on ") + sym + " - " + TF_VERSION); }
    if (!storeLoaded) loadStore();
    if (!guardsRead) readGuards();
    passes++;
    err.clear();
    if (fromTimer) runSteps();        // the back-fill (one guarded step at a time) - from the timer only
    if (step > 0) live();             // then the new trades since the cursor (a market that opens later starts here)
    time_t now = time(0);
    if (now - lastSave >= 300) { lastSave = now; eng.flushSession(); saveStore(); }
    if (now - lastEvWrite >= 5) { lastEvWrite = now; writeEvents(false); }
    if (now - lastStatus >= 5) { lastStatus = now; writeStatus(); }
    busy = false;
}

// ---- new trades since the cursor (at most 1 h back - RTTICKS(0) once stalled a pass in IRTReader 0.1.3)
void TapeFlow::live()
{
    RTDATE now = currentDate();
    long long nowS = localSec(now);
    long long back = cur.lastSec >= 0 ? nowS - cur.lastSec + 2 : 600;
    if (back < 2) back = 2;
    if (cur.lastSec >= 0 && back > 3600) {                                            // asleep / stalled for an hour: a hole - say so, carry on from 10 min
        eng.gap(nowS - 600, "no data for over an hour");
        trace("gap: cursor more than 1 h old");
        cur = TCursor(); back = 600;
    }
    RTTICKS T((RTDATE)(now - (RTDATE)back));
    long got = 0;
    if (T.count > 0 && T.dt && T.price && T.size) {
        cur.rewind();
        for (long i = 0; i < T.count; i++) {
            long long s = localSec((RTDATE)(*T.dt)[(int)i]);
            if (!cur.take(s)) continue;
            tfl::Tick k; k.t = s; k.px = toTicks((*T.price)[(int)i]); k.q = (long long)(*T.size)[(int)i];
            k.bid = T.bid ? toTicks((*T.bid)[(int)i]) : 0; k.ask = T.ask ? toTicks((*T.ask)[(int)i]) : 0;
            eng.add(k); got++;
        }
    }
    if (got) { wallAtTick = time(0); dataAtTick = cur.lastSec; liveTicks += got; }
    // the clock moves on without trades: close the seconds up to 8 s behind the data clock (a second with trades closes as soon
    // as a later trade arrives; only quiet seconds wait). A trade later than that is counted, never placed.
    if (dataAtTick >= 0) eng.advanceTo(dataAtTick + (long long)(time(0) - wallAtTick) - LATE_MARGIN);
}

bool TapeFlow::offHours() const
{
    time_t n = time(0); struct tm t; localtime_s(&t, &n);
    int m = t.tm_hour * 60 + t.tm_min;
    if (t.tm_wday == 0 || t.tm_wday == 6) return true;
    return m >= 15 * 60 + 15 || m < 7 * 60;
}

void TapeFlow::runSteps()
{
    time_t now = time(0);
    while (step < NSTEPS && blocked[step]) { if (!STEPS[step].deep) { step++; continue; } deepDone = true; step = NSTEPS; }
    if (step >= NSTEPS) return;
    const TStep& spec = STEPS[step];
    if (step == 0) { rebuild(0); step = 1; lastStepAt = time(0); return; }      // the first 10 minutes at once
    if (now - firstPass < 30 || now - lastStepAt < (spec.deep ? 60 : 20)) return;  // let IRT settle; one step at a time
    if (spec.deep) {
        if (deepDone) { step = NSTEPS; return; }
        if (tfl::priorSessions(store, eng.curSid) >= 10) { deepDone = true; step = NSTEPS; return; }
        if (!offHours()) return;                                                  // older history only outside trading hours
    }
    rebuild(step);
    lastStepAt = time(0);
    step++;
}

// read trades from (now - back) to now into a fresh engine and use it (the back-fill). Same logic, same order as live.
bool TapeFlow::rebuild(int idx)
{
    const TStep& spec = STEPS[idx];
    RTDATE now = currentDate();
    long long nowS = localSec(now);
    long long back = spec.back;
    if (back < 0) {                                               // the session: from 17:00
        long long sid = tfl::sessionOf(nowS);
        long long start = (sid - 1) * 86400 + 17LL * 3600;
        back = nowS - start + 5;
        if (back <= 4 * 3600 + 60) { stepNote = "session already covered"; return true; }
    }
    trace(std::string("back-fill ") + spec.name + "...");
    guardWrite(idx);
    RTTICKS T((RTDATE)(now - (RTDATE)back));
    long n = T.count;
    tfl::Engine e2; e2.attach(&store, sym);
    {                                                             // the request starts inside an older session: that one is partial
        long long startS = nowS - back, sidS = tfl::sessionOf(startS);
        if (sidS != tfl::sessionOf(nowS) && startS > (sidS - 1) * 86400 + 17LL * 3600 + 60) e2.noFlushSid = sidS;
    }
    TCursor c2; long long first = -1, quoted = 0;
    if (n > 0 && T.dt && T.price && T.size) {
        for (long i = 0; i < n; i++) {
            long long s = localSec((RTDATE)(*T.dt)[(int)i]);
            if (!c2.take(s)) continue;
            if (first < 0) first = s;
            tfl::Tick k; k.t = s; k.px = toTicks((*T.price)[(int)i]); k.q = (long long)(*T.size)[(int)i];
            k.bid = T.bid ? toTicks((*T.bid)[(int)i]) : 0; k.ask = T.ask ? toTicks((*T.ask)[(int)i]) : 0;
            if (k.bid > 0 && k.ask >= k.bid) quoted++;
            e2.add(k);
        }
    }
    guardClear(idx);
    stepTicks[idx] = e2.ticks;
    lastStepName = spec.name;
    char b[260];
    snprintf(b, sizeof(b), "back-fill %s: %ld trades from IRT, %lld used, from %s, bid/ask on %.0f%% of them", spec.name, n, e2.ticks,
             first < 0 ? "-" : std::to_string(nowS - first).append(" s ago").c_str(), e2.ticks > 0 ? 100.0 * (double)quoted / (double)e2.ticks : 0.0);
    trace(b); stepNote = b;
    if (e2.ticks <= 0) { if (spec.deep) deepDone = true; return false; }
    if (spec.deep && first > 0 && nowS - first < back - 86400) deepDone = true;    // IRT keeps no older trades: stop deepening
    e2.flushSession();
    eng = std::move(e2); eng.attach(&store, sym); eng.noFlushSid = LLONG_MIN;
    cur = c2; wallAtTick = time(0); dataAtTick = cur.lastSec;
    eng.advanceTo(dataAtTick - LATE_MARGIN);
    if (spec.deep) { eng.refreeze(); }                               // older sessions just arrived: freeze the baselines again (new version)
    saveStore(); evWritten = (size_t)-1;
    return true;
}

// ---- guards
void TapeFlow::readGuards()
{
    guardsRead = true;
    for (int i = 0; i < NSTEPS; i++) {
        { std::ifstream b(guardPath("blocked", STEPS[i].name).c_str()); if (b.good()) { blocked[i] = true; continue; } }
        std::ifstream g(guardPath("trying", STEPS[i].name).c_str());
        if (!g.good()) continue;
        int strikes = 0; g >> strikes; g.close();
        strikes++;
        if (strikes >= 2) {                                       // twice in a row IRT did not survive this request
            blocked[i] = true;
            std::ofstream o(guardPath("blocked", STEPS[i].name).c_str(), std::ios::trunc); o << "IRT stopped during this back-fill request twice - blocked. Delete this file to allow it again.\n";
            std::remove(guardPath("trying", STEPS[i].name).c_str());
            trace(std::string("back-fill ") + STEPS[i].name + " BLOCKED (IRT stopped during it twice)");
        } else {
            std::ofstream o(guardPath("trying", STEPS[i].name).c_str(), std::ios::trunc); o << strikes << "\n";   // kept: a second stop blocks it
            trace(std::string("back-fill ") + STEPS[i].name + ": IRT stopped during it last time (or was closed) - one more try");
        }
    }
}
void TapeFlow::guardWrite(int idx)
{
    std::string p = guardPath("trying", STEPS[idx].name);
    int strikes = 0; { std::ifstream g(p.c_str()); if (g.good()) g >> strikes; }
    std::ofstream o(p.c_str(), std::ios::trunc); o << strikes << "\n";
}
void TapeFlow::guardClear(int idx) { std::remove(guardPath("trying", STEPS[idx].name).c_str()); }

// ---- the baseline store (this plugin's own files: one per session, so only today's file is ever rewritten)
std::string TapeFlow::basePath(long long sid) const { return tfDir() + "\\" + mkt + "-base-" + std::to_string(sid) + ".csv"; }
void TapeFlow::loadStore()
{
    storeLoaded = true;
    long long now = localSec(currentDate()), sid0 = tfl::sessionOf(now);
    long rows = 0;
    for (long long sid = sid0; sid >= sid0 - 21; sid--) {      // three weeks of sessions (weekends have no file)
        std::ifstream f(basePath(sid).c_str()); if (!f.is_open()) continue;
        std::string ln; long n = 0;
        while (std::getline(f, ln)) {
            long long s; std::string sy; tfl::Win w;
            if (!tfl::parseStoreLine(ln, &s, &sy, &w) || s != sid) continue;
            store[sid][sy].push_back(w); n++;
        }
        savedCount[sid] = (size_t)n; rows += n;
    }
    trace("baseline files: " + std::to_string(store.size()) + " sessions, " + std::to_string(rows) + " windows");
    if (eng.curSid != LLONG_MIN) eng.refreeze();
}
void TapeFlow::saveStore()
{
    if (!storeLoaded || tfDir().empty()) return;
    while (store.size() > 14) store.erase(store.begin());          // the newest 14 sessions are plenty for 10
    for (auto& kv : store) {
        size_t tot = 0; for (auto& c : kv.second) tot += c.second.size();
        if (savedCount.count(kv.first) && savedCount[kv.first] == tot) continue;   // unchanged
        std::string p = basePath(kv.first), tmp = p + ".tmp";
        {
            std::ofstream o(tmp.c_str(), std::ios::trunc); if (!o.is_open()) { err = "cannot write " + p; continue; }
            o << "# TapeFlow " << TF_VERSION << " baselines " << mkt << " session " << kv.first << ": W|session|contract|bin|rate20|rate5 x4|spread|Mbuy h1..8|Msell h1..8\n";
            for (auto& c : kv.second) for (auto& w : c.second) o << tfl::storeLine(kv.first, c.first, w) << "\n";
        }
        bool hadOld = false; { std::ifstream old(p.c_str()); hadOld = old.good(); }
        std::string bak = p + ".bak";
        std::remove(bak.c_str());
        if (hadOld && std::rename(p.c_str(), bak.c_str()) != 0) { err = "cannot replace " + p; std::remove(tmp.c_str()); continue; }
        if (std::rename(tmp.c_str(), p.c_str()) != 0) {
            err = "cannot publish " + p;
            if (hadOld) std::rename(bak.c_str(), p.c_str());
            std::remove(tmp.c_str());
            continue;
        }
        if (hadOld) std::remove(bak.c_str());
        savedCount[kv.first] = tot;
    }
    if (!store.empty()) {                                          // files older than 3 weeks are not read any more: remove them
        long long newest = store.rbegin()->first;
        for (long long sid = newest - 22; sid >= newest - 40; sid--) std::remove(basePath(sid).c_str());
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
    // civil from days
    long long z = d + 719468; long long era = (z >= 0 ? z : z - 146096) / 146097; unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = (long long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); unsigned mp = (5 * doy + 2) / 153; unsigned dd = doy - (153 * mp + 2) / 5 + 1;
    unsigned mm = mp < 10 ? mp + 3 : mp - 9; y += mm <= 2;
    char b[64]; snprintf(b, sizeof(b), "%04lld-%02u-%02u %02lld:%02lld:%02lld", y, mm, dd, r / 3600, (r / 60) % 60, r % 60);
    return b;
}
static std::string clock12(long long t)
{
    long long r = ((t % 86400) + 86400) % 86400; int h = (int)(r / 3600), m = (int)(r / 60 % 60), s = (int)(r % 60);
    char b[24]; snprintf(b, sizeof(b), "%d:%02d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, s, h < 12 ? "AM" : "PM");
    return b;
}

// every committed event, APPEND-ONLY (brief section 14): a record once written is never rewritten or removed. Episode ids come
// from the creation second, so a back-fill's events match the ones already written and are not written twice.
std::string TapeFlow::evPath(long long sid) const { return tfDir() + "\\" + mkt + "-events-" + stamp(sid * 86400).substr(0, 10) + ".csv"; }
static std::string evKey(const std::string& line)              // time|episode|kind|dir
{
    size_t p = 0; for (int k = 0; k < 4 && p != std::string::npos; k++) p = line.find('|', p + 1);
    return p == std::string::npos ? line : line.substr(0, p);
}
void TapeFlow::writeEvents(bool force)
{
    if (mkt.empty() || tfDir().empty() || eng.curSid == LLONG_MIN) return;
    if (!force && !eng.evs.empty() && eng.evs.size() == evWritten && eng.evs.back().t == evLastT) return;
    typedef std::pair<std::string, std::string> PendingEvent;       // key, complete line
    std::map<long long, std::vector<PendingEvent> > add;
    std::set<std::string> pending;
    for (const tfl::Ev& v : eng.evs) {
        long long sid = tfl::sessionOf(v.t);
        if (sid < eng.curSid - 1) continue;                       // older sessions were written at the time
        if (!keysLoaded.count(sid)) {                             // the keys already in that session's file
            keysLoaded.insert(sid);
            std::ifstream f(evPath(sid).c_str()); std::string ln;
            while (std::getline(f, ln)) if (!ln.empty() && ln[0] != 't') written.insert(evKey(ln));
        }
        auto num = [](double x, int dp) { if (!std::isfinite(x)) return std::string(); char q[32]; snprintf(q, sizeof(q), "%.*f", dp, x); return std::string(q); };
        char b[700];
        snprintf(b, sizeof(b), "%s|%d|%s|%d|%s|%s|%d|%.0f|%s|%s|%s|%s|%.0f|%.2f|%.1f|%s|%s|%d|%s|quote-at-trade\n",
                 stamp(v.t).c_str(), v.ep, v.kind.c_str(), v.dir, v.lo ? fmtPx(v.lo).c_str() : "", v.hi ? fmtPx(v.hi).c_str() : "",
                 v.h, v.q95, num(v.f30, 1).c_str(), num(v.f180, 1).c_str(), num(v.a, 2).c_str(), num(v.cov, 3).c_str(), v.qzone, v.conc, v.prog,
                 v.ctx.c_str(), v.why.c_str(), v.ver, TF_VERSION);
        std::string line = b, key = evKey(line);
        if (written.count(key) || pending.count(key)) continue;
        pending.insert(key);
        add[sid].push_back(PendingEvent(key, line));
    }
    bool complete = true;
    for (auto& kv : add) {
        std::string p = evPath(kv.first);
        bool fresh = false; { std::ifstream t(p.c_str()); fresh = !t.good(); }
        std::ofstream o(p.c_str(), std::ios::app);
        if (!o.is_open()) { err = "cannot write " + p; complete = false; continue; }
        if (fresh) o << "time|episode|kind|dir|zone_lo|zone_hi|h|q95|f30|f180|activity|cov30|q_zone|concentration|progress_ticks|context|why|baseline_ver|version|mode\n";
        for (const PendingEvent& event : kv.second) o << event.second;
        o.flush();
        if (!o.good()) { err = "cannot write " + p; complete = false; continue; }
        for (const PendingEvent& event : kv.second) written.insert(event.first);
    }
    if (complete) {
        evWritten = eng.evs.size(); evLastT = eng.evs.empty() ? 0 : eng.evs.back().t;
    } else {
        // A failed/partial append must be re-scanned before the next retry; do not
        // let the in-memory cursor discard an event that did not reach the file.
        keysLoaded.clear(); written.clear(); evWritten = (size_t)-1; evLastT = 0;
    }
}

std::string TapeFlow::stateText(COLOR* col) const
{
    *col = C_INK;
    if (sym.empty()) { *col = C_MUTED; return "waiting for the chart"; }
    if (eng.ticks <= 0) { *col = C_MUTED; return "WAITING FOR TRADES"; }
    const tfl::Feat& f = eng.last;
    int d = 0; const tfl::Ep* e = nullptr;
    if (eng.watching(&d, &e) && e) {
        *col = C_AMBER;
        long left = (long)(e->fireT + eng.cfg.watchTTL - eng.lastT); if (left < 0) left = 0;
        return std::string("WATCH ") + (d > 0 ? "bullish - sellers stalled at " : "bearish - buyers stalled at ") + fmtPx(e->lo) + "-" + fmtPx(e->hi) +
               " (" + std::to_string(left) + " s left)";
    }
    for (auto it = eng.evs.rbegin(); it != eng.evs.rend(); ++it) {
        if (eng.lastT - it->t > 60) break;
        if (it->kind == "AR" || it->kind == "IN") {
            *col = it->dir > 0 ? C_BUY : C_SELL;
            return std::string("CONFIRMED ") + (it->dir > 0 ? "bullish " : "bearish ") + (it->kind == "AR" ? "response" : "initiative") + (TF_PROVEN ? "" : "?") + " at " + clock12(it->t);
        }
    }
    if (!f.warm) { *col = C_MUTED; return "WARMING UP " + std::to_string(eng.warmN) + "/" + std::to_string(eng.cfg.warm) + " s"; }
    if (f.f180ok && f.c180 < 0.5) { *col = C_MUTED; char b[64]; snprintf(b, sizeof(b), "SIDE DATA REQUIRED (sides known %.0f%%)", f.c180 * 100); return b; }
    if (!f.baseOk) {
        *col = C_MUTED;
        int n = f.bb()->n;
        return "CALIBRATING - " + std::to_string(n) + " of 60 windows for this 5-min slot (" + std::to_string(eng.baseSessions) + " sessions on file)";
    }
    if (!f.quoteOk) { *col = C_MUTED; return "DATA INVALID - no current quote"; }
    if (f.aok && f.A < 0.5) { *col = C_MUTED; return "LOW ACTIVITY"; }
    return "READY";
}

void TapeFlow::writeStatus()
{
    if (dir().empty()) return;
    std::ofstream f((dir() + "\\TapeFlow.status-" + mkt + ".txt").c_str(), std::ios::trunc); if (!f.is_open()) return;
    COLOR c; const tfl::Feat& x = eng.last;
    long long prior = tfl::priorSessions(store, eng.curSid);
    f << "VERSION," << TF_VERSION << "\nMARKET," << mkt << "\nCHART," << root << "\nSYMBOL," << sym << "\nTICK," << tick
      << "\nTIMER," << (timerOn ? "on" : timerRefused ? "REFUSED (running from draw)" : "starting")
      << "\nDATA,trades " << eng.ticks << ",live " << liveTicks << ",late " << eng.late << ",last " << (eng.lastT ? stamp(eng.lastT) : "-")
      << "\nBACKFILL,next step " << (step < NSTEPS ? STEPS[step].name : "done") << "," << stepNote
      << "\nCALIBRATION,prior sessions on file " << prior << ",slot windows " << x.bb()->n << ",baseline version " << eng.ver
      << "\nNOW,f30 " << (x.f30ok ? x.f30 : NAN) << ",f180 " << (x.f180ok ? x.f180 : NAN) << ",activity " << (x.aok ? x.A : NAN) << ",sides " << x.c30
      << "\nSTATE," << stateText(&c) << "\nWHY_NO_SIGNAL," << eng.lastWhy << "\nEVENTS," << eng.evs.size() << "\nERROR," << err << "\n";
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

void TapeFlow::triangle(short x, short y, bool up, bool filled, COLOR c)
{
    const int H = 6;                                           // half size
    if (filled) {
        for (int k = 0; k <= 2 * H; k++) {                     // scan lines from the tip
            int w = (k * H) / (2 * H);
            short yy = (short)(up ? y - H + k : y + H - k);
            line((short)(x - w), yy, (short)(x + w), yy, c, 1);
        }
    } else {
        short ty = (short)(up ? y - H : y + H), by = (short)(up ? y + H : y - H);
        line(x, ty, (short)(x + H), by, c, 2); line((short)(x + H), by, (short)(x - H), by, c, 2); line((short)(x - H), by, x, ty, c, 2);
    }
}

int TapeFlow::draw(void)
{
    pump("draw");
    render();
    return RTX_OK;
}

void TapeFlow::render()
{
    RCT pane; pane.getPaneRect(false);
    short L = pane.left, T = pane.top, R = pane.right, B = pane.bottom;
    if (R - L < 60 || B - T < 40) return;
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    std::string chartMkt = marketForRoot(rs ? rs : "");
    int fs = font;
    if (chartMkt != mkt) {                                         // this DLL is for one market only
        std::string s = std::string("TapeFlow ") + mkt + " " + TF_VERSION + " - this one is for the " + mkt + " chart (add TapeFlow" + (chartMkt.empty() ? std::string("<market>") : chartMkt) + " here)";
        textLJ((short)(L + 8), (short)(T + 12), s.c_str(), C_MUTED, fs, false);
        return;
    }
    double span = 270.0, h = (double)(B - T);
    auto Y = [&](double v) { return (short)(T + (135.0 - v) / span * h + 0.5); };
    // guides: the neutral band +-15, the +-35 lines, zero
    fill(L, Y(15), R, Y(-15), C_BAND);
    line(L, Y(35), R, Y(35), C_GRID, 1); line(L, Y(-35), R, Y(-35), C_GRID, 1);
    line(L, Y(0), R, Y(0), C_ZERO, 1);
    long n = getBarCount();
    int b0 = 0, b1 = (int)n - 1;
    if (getVisibleBars(&b0, &b1) != RTX_OK) { b0 = std::max(0, (int)n - 200); b1 = (int)n - 1; }
    if (b1 >= n) b1 = (int)n - 1;
    if (b0 < 0) b0 = 0;
    const std::vector<tfl::SecRec>& H = eng.hist;
    if (n > 0 && b1 >= b0 && !H.empty()) {
        RTARRAYI dt(barDateTime);
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        // (1.0.3) bar width: the 30-s sub-bars split 90% of the candle spacing
        int spb = getSecondsPerBar(); if (spb <= 0) spb = 180;
        auto idxAtOrBefore = [&](long long t) -> long {
            long lo = 0, hi = (long)H.size();                      // first rec with t > T, minus one
            while (lo < hi) { long m = (lo + hi) / 2; if (H[(size_t)m].t <= t) lo = m + 1; else hi = m; }
            return lo - 1;
        };
        short px180 = -1, py180 = -1;
        std::vector<std::pair<int, const tfl::Ev*>> marks;
        for (int i = b0; i <= b1; i++) {
            long long te = localSec((RTDATE)dt[i]);               // bars are stamped at their close
            long long ts = i > 0 ? localSec((RTDATE)dt[i - 1]) : te - spb;
            if (te - ts > 4 * spb || te <= ts) ts = te - spb;      // the first bar after a break
            PNT p; p.set(i, 0.0f, kBarCenter); short x = p.h;
            long k = idxAtOrBefore(te);
            if (k < 0 || H[(size_t)k].t <= ts) { px180 = -1; continue; }   // no flow data for this bar
            const tfl::SecRec& q = H[(size_t)k];
            // (1.0.3) no grey whisker: not in the design, and it read as extra bars
            {   // (1.0.3, Rassul 20:46 "see if you made mistakes" - the design is 30-SEC pressure bars) one thin bar per 30 s inside
                // each chart bar (6 per 3-min candle): bought - sold over bought + sold in that 30 s, as in the original design and
                // the mockup. (1.0.0-1.0.2 drew ONE bar per candle = only the last 30 s, so the colour flipped candle to candle.)
                // Brightness = how busy those 30 s were vs normal for this time of day; grey = quiet tape (signals off).
                int slots = (int)((te - ts + 29) / 30); if (slots < 1) slots = 1; if (slots > 12) slots = 12;
                int span = std::max(1, (int)(ppb * 0.9));
                double sw = (double)span / slots;
                short xl = (short)(x - span / 2);
                for (int sl = 0; sl < slots; sl++) {
                    long long a0 = ts + 30LL * sl, a1 = std::min(te, ts + 30LL * (sl + 1));
                    double sb = 0, ss = 0, sa = 0; int na = 0, nq = 0, nall = 0;
                    for (long j = idxAtOrBefore(a1); j >= 0 && H[(size_t)j].t > a0; j--) {
                        const tfl::SecRec& r = H[(size_t)j];
                        sb += r.b; ss += r.s; nall++;
                        if (!std::isnan(r.a)) { sa += r.a; na++; }
                        if (r.flags & tfl::SR_QUALITY) nq++;
                    }
                    if (!(sb + ss > 0)) continue;
                    double fb = 100.0 * (sb - ss) / (sb + ss);
                    bool good = nq * 2 >= nall;
                    double al = na ? std::max(0.20, std::min(0.85, 0.20 + 0.30 * (sa / na))) : 0.45;
                    COLOR c = good ? blend(fb >= 0 ? C_BUY : C_SELL, C_BG, al) : blend(C_GRAY, C_BG, 0.6);
                    short x1 = (short)(xl + (int)(sl * sw)), x2 = (short)(xl + (int)((sl + 1) * sw) - (sw >= 3 ? 1 : 0));
                    if (x2 < x1) x2 = x1;
                    short y0 = Y(0), y1 = Y(fb);
                    fill(x1, std::min(y0, y1), x2, std::max(y0, y1), c);
                }
            }
            if (!std::isnan(q.f180)) {
                short y = Y(q.f180);
                if (px180 >= 0) line(px180, py180, x, y, C_PURPLE, 2);
                px180 = x; py180 = y;
            } else px180 = -1;
            {                                                          // events are in time order: find this bar's by search
                auto it = std::upper_bound(eng.evs.begin(), eng.evs.end(), ts, [](long long t, const tfl::Ev& e) { return t < e.t; });
                for (; it != eng.evs.end() && it->t <= te; ++it) marks.push_back(std::make_pair(i, &*it));
            }
        }
        // markers: bullish lane +115, bearish -115, stacked toward the middle when a bar has several
        std::map<std::pair<int, int>, int> stack;
        for (auto& m : marks) {
            const tfl::Ev& v = *m.second;
            bool sig = v.kind == "AW" || v.kind == "AR" || v.kind == "IN";
            bool x_ = v.kind == "FAIL" || v.kind == "INVP" || v.kind == "DINV";
            if (!sig && !x_) continue;
            int lane = v.dir > 0 ? 1 : -1;
            int k = stack[std::make_pair(m.first, lane)]++;
            if (k > 3) continue;                                   // the 5th and later are counted below (+n)
            PNT p; p.set(m.first, 0.0f, kBarCenter); short x = p.h;
            short y = (short)(Y(lane * 115.0) + (lane > 0 ? k * 15 : -k * 15));   // stacked toward the middle
            const char* q = TF_PROVEN ? "" : "?";
            if (x_) { textLJ((short)(x - 3), y, "x", C_MUTED, fs + 1, true); continue; }
            COLOR c = v.kind == "AW" ? C_AMBER : (v.dir > 0 ? C_BUY : C_SELL);
            triangle(x, y, v.dir > 0, v.kind != "AW", c);
            std::string lab = (v.kind == "AW" ? "AW" : v.kind == "AR" ? "AR" : "IN") + std::string(q);
            if (ppb >= 4) textLJ((short)(x + 9), y, lab.c_str(), c, std::max(7, fs - 1), true);
        }
        for (auto& kv : stack) if (kv.second > 4) {                // more than 4 in one bar and lane: say how many more
            PNT p; p.set(kv.first.first, 0.0f, kBarCenter);
            int lane = kv.first.second; short y = (short)(Y(lane * 115.0) + (lane > 0 ? 4 * 15 : -4 * 15));
            std::string more = "+" + std::to_string(kv.second - 4);
            textLJ((short)(p.h - 6), y, more.c_str(), C_MUTED, std::max(7, fs - 1), true);
        }
    }
    // the readout: version, the live numbers, the state
    {
        const tfl::Feat& f = eng.last;
        COLOR sc; std::string st = stateText(&sc);
        char a30[16], a180[16], act[16], sides[16];
        if (f.f30ok) snprintf(a30, sizeof(a30), "%+.0f", f.f30); else snprintf(a30, sizeof(a30), "-");
        if (f.f180ok) snprintf(a180, sizeof(a180), "%+.0f", f.f180); else snprintf(a180, sizeof(a180), "-");
        if (f.aok) snprintf(act, sizeof(act), "%.1fx", f.A); else snprintf(act, sizeof(act), "-");
        if (f.f30ok || f.c30 > 0) snprintf(sides, sizeof(sides), "%.0f%%", f.c30 * 100); else snprintf(sides, sizeof(sides), "-");
        std::string head = std::string("TapeFlow ") + TF_VERSION + " " + mkt;
        int w1 = textW(head.c_str(), fs, true) + 14;
        std::string p1 = "30s ", p2 = "   180s ", p3 = std::string("   activity ") + act + "   sides " + sides;
        int wt = w1 + textW(p1.c_str(), fs, false) + textW(a30, fs, true) + textW(p2.c_str(), fs, false) + textW(a180, fs, true) + textW(p3.c_str(), fs, false);
        int ws = textW(st.c_str(), fs, true);
        short bwid = (short)(std::max(wt, ws) + 16), bh = (short)(2 * fs + 20);
        short x0 = (short)(L + 6), y0 = (short)(T + 4);
        if (x0 + bwid > R - 4) bwid = (short)(R - 4 - x0);
        RCT bx; bx.set(x0, y0, (short)(x0 + bwid), (short)(y0 + bh)); bx.draw(1, C_GRID, C_BOX, DRAW_OPAQUE, PAT_SOLID);
        short x = (short)(x0 + 8), yA = (short)(y0 + fs + 6), yB = (short)(y0 + 2 * fs + 14);
        textLJ(x, yA, head.c_str(), C_INK, fs, true); x = (short)(x + w1);
        textLJ(x, yA, p1.c_str(), C_INK, fs, false); x = (short)(x + textW(p1.c_str(), fs, false));
        textLJ(x, yA, a30, f.f30ok ? (f.f30 >= 0 ? C_BUY : C_SELL) : C_MUTED, fs, true); x = (short)(x + textW(a30, fs, true));
        textLJ(x, yA, p2.c_str(), C_INK, fs, false); x = (short)(x + textW(p2.c_str(), fs, false));
        textLJ(x, yA, a180, f.f180ok ? (f.f180 >= 0 ? C_BUY : C_SELL) : C_MUTED, fs, true); x = (short)(x + textW(a180, fs, true));
        textLJ(x, yA, p3.c_str(), C_INK, fs, false);
        textLJ((short)(x0 + 8), yB, st.c_str(), sc, fs, true);
    }
}

extern "C" cppExtension *CreateExtension(void)
{
    TapeFlow *p = new TapeFlow();
    p->setArrayCount(2);
    p->setFlags(POST_DRAWING | NO_UI);                             // its own pane (no OVERLAY), draws itself
    p->setExtendedFlags(CALL_CONTINUOUSLY);
    std::string d = std::string("LRA TapeFlow ") + TF_FIXED + ": who is hitting the tape (30 s bars, 180 s line), absorption watches (AW), "
                    "responses (AR) and initiative (IN), all from IRT's own trades. Put it on the " + TF_FIXED + " chart only. '?' = not proven yet.";
    static std::string desc; desc = d;
    p->setDescription(desc.c_str());
    p->setVersion(TF_VERSION);
    return p;
}
