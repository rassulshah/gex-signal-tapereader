/********************************************************************************
 *  TapeFlow.cpp  --  Investor/RT RTX extension  lsTapeFlow<MKT>  (v1.1.0, corrected 2026-10-09)
 *
 *  Rassul 2026-10-08 13:17-13:37: the TapeFlow brief; "i'll take your recommendation but i want to get it on all the markets,
 *  if signals are tentative just add ?"; "the markers should be on the indicator on a separate pane"; "keep levels out of it,
 *  similar to the delta profile"; "build for irt ... add backfill capabilities similar to other indicators and it must be
 *  native without any outside dependencies"; "make sure you test and check for errors and inconsistencies".
 *
 *  v1.1.0 corrections: no RTTICKS requests from draw; validated overlapping
 *  snapshot cursor; frozen live events across staged backfill; identified W2
 *  baseline windows; safe journal retry; precise quantities and cached rendering.
 *  Build one launcher per market with the actual vendor RTX SDK.
 *  One active contract per extension object is assumed. Vendor host/timer context
 *  and array/timestamp semantics still require integration testing in Investor/RT.
 *  Signals are level-free, native, tentative, and drawn in one separate pane.
 *  Test results and known limitations are documented in README.md, not assumed
 *  from inherited comments. No production SDK/DLL or trading-edge claim is made.
 ********************************************************************************/
#ifndef TF_FIXED
#error "build TapeFlowES.cpp ... TapeFlowEU.cpp (each defines TF_FIXED and includes this file)"
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

static const char* TF_VERSION = "1.1.2";   // (1.1.2) no gap in the pane where IRT was restarted (cold 10-min catch-up filled from the longer replay)   // (1.1.1) the audited 1.1.0 + two integration fixes: compiles with the real SDK (currentDate non-const); a quiet second pauses the warm-up instead of restarting it (HG etc. could never warm up)   // (1.0.3) the histogram is 30-SEC pressure bars (6 per 3-min candle) as designed, not one last-30-s snapshot per candle   // (1.0.2) audit 1.0.1 fixes, but one engine per DLL (timer-safe)
static const bool TF_PROVEN = false;
static const int LATE_MARGIN = 8; // quiet-second allowance retained; tune only from measured CQG delivery latency

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

static int nextTimerId() { static std::atomic<int> next(5711); return next.fetch_add(1); }
static int timerIdFor(const void* me);

// Local civil timestamps retained for compatibility; they are NOT monotonic across DST rollback.
// Nonmonotonic snapshots fail closed. Configure IRT/session timezone deliberately.
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

typedef tf_support::SnapshotCursor TCursor;

struct TStep { const char* name; int back; bool deep; };
static const TStep STEPS[] = { {"10m", 600, false}, {"1h", 3600, false}, {"4h", 4 * 3600, false}, {"session", -1, false},
                               {"3d", 3 * 86400, true}, {"6d", 6 * 86400, true}, {"11d", 11 * 86400, true} };
static const int NSTEPS = (int)(sizeof(STEPS) / sizeof(STEPS[0]));

struct TIdx { int fontParam; };
static TIdx TX;

// State belongs to this extension object, not an undocumented chart user-data slot.
// Do not assume the vendor creates one object per DLL or one object per chart.
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
    bool blocked[NSTEPS] = {false}; bool guardsRead = false; bool deepDone = false;
    std::string stepNote, lastStepName;
    long long stepTicks[NSTEPS] = {0};
    size_t evWritten = (size_t)-1; long long evLastT = 0;
    std::set<std::string> written; std::set<long long> keysLoaded;
    std::map<long long, std::uint64_t> savedDigest;
    bool storageReady = false;
    bool bootstrapDone = false;
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
    int font = 9;
};
static int timerIdFor(const void* me) { return static_cast<const TapeFlowState*>(me)->uniqueTimerId; }

class TapeFlow : public cppExtension {
public:
    TapeFlow() : cppExtension(), own_(new TapeFlowState()) {}
    ~TapeFlow() { delete own_; }
    virtual int timer(RTX_EVENT* e);
    virtual int draw(void);
    virtual int parmsLoad(void)  { readSettings(); return RTX_OK; }
    virtual int parmsApply(void) { readSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { readSettings(); return RTX_OK; }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = -135; if (dMax) *dMax = 135; return RTX_OK; }

    // Per-object state. The SDK must supply a stable contract context for timer calls.
    // Multi-host dispatch requires a vendor-supported host registry; not invented here.
    mutable TapeFlowState* own_ = nullptr;
    TapeFlowState* existingState() const { return own_; }
    TapeFlowState& S() const
    {
        if (!own_) own_ = new TapeFlowState();
        return *own_;
    }

    std::string evPath(long long sid) const;

    void readSettings() { std::lock_guard<std::recursive_mutex> lock(S().access); int f = getIntegerValue(TX.fontParam); S().font = (f >= 6 && f <= 16) ? f : 9; }
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
    bool snapshot(RTTICKS& data, std::vector<tf_support::TradeIdentity>& out);
    void captureCommitted();
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
#define savedDigest (S().savedDigest)
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

// Timer performs backfill. Calc may perform only a live fallback; draw never queries RTTICKS.
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
    // draw must remain read-only: RTTICKS calls from a draw callback previously hung IRT.
    if (noTimer && std::strcmp(via, "calc") == 0 && now != lastFallback) {
        lastFallback = now;
        pass(false); // fallback never initiates deep history requests
    }
}

int TapeFlow::timer(RTX_EVENT* e)
{
    std::unique_lock<std::recursive_mutex> lock(S().access,std::try_to_lock);
    if (!lock.owns_lock()) return RTX_OK;
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
    float propertyTick = getProperty(SYM_TICKINCR);
    const double knownIncrement=marketTick(mkt);
    // Normalize float-valued SDK metadata to exact known decimal increments.
    tick = (knownIncrement>0 && std::isfinite(propertyTick) && std::fabs(propertyTick-knownIncrement)<=knownIncrement*1e-5)
        ? knownIncrement : (std::isfinite(propertyTick) && propertyTick>0 && propertyTick<1000 ? (double)propertyTick : knownIncrement);
    if (!sy.empty() && sy != sym) {
        if (!sym.empty()) {                                        // the chart moved to another contract: start over on it
            trace("contract changed " + sym + " -> " + sy + ": starting over");
            eng.flushSession(); saveStore();
            captureCommitted(); writeEvents(true);
            eng = tfl::Engine(); cur.reset(); dataAtTick = -1; step = 0; deepDone = false; evWritten = (size_t)-1;
            S().bootstrapDone = false; S().committed.clear(); S().committedKeys.clear();
            S().captureCount=0; S().captureFirst=LLONG_MIN; S().indexedSize=(size_t)-1;
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
    if (fromTimer) runSteps();        // the back-fill (one guarded step at a time) - from the timer only
    if (step > 0 || !fromTimer) live();
    captureCommitted();             // then the new trades since the cursor (a market that opens later starts here)
    time_t now = time(0);
    if (S().storageReady && now - lastSave >= 300) { lastSave = now; eng.flushSession(); saveStore(); }
    if (S().storageReady && now - lastEvWrite >= 5) { lastEvWrite = now; writeEvents(false); }
    if (S().storageReady && now - lastStatus >= 5) { lastStatus = now; writeStatus(); }
    busy = false;
    } catch (const std::exception& ex) {
        err = std::string("processing failed: ") + ex.what();
        eng.gap(eng.lastT + 1, err);
        captureCommitted();
    } catch (...) {
        err = "processing failed: unknown exception";
        eng.gap(eng.lastT + 1, err);
        captureCommitted();
    }
}

// ---- new trades since the cursor (at most 1 h back - RTTICKS(0) once stalled a pass in IRTReader 0.1.3)
bool TapeFlow::snapshot(RTTICKS& data, std::vector<tf_support::TradeIdentity>& out)
{
    out.clear();
    if (data.count <= 0) return true;
    if (!data.dt || !data.price || !data.size || data.count > INT_MAX) {
        err = "invalid IRT tick array"; return false;
    }
    out.reserve(static_cast<size_t>(data.count));
    for (long i=0; i<data.count; ++i) {
        const RTDATE raw = (RTDATE)(*data.dt)[(int)i];
        const double qty = (double)(*data.size)[(int)i];
        int px=0, bid=0, ask=0;
        if (!std::isfinite((double)raw) || !std::isfinite(qty) || qty <= 0 ||
            qty >= (double)LLONG_MAX || std::floor(qty) != qty ||
            !tf_support::nativeToTicks((*data.price)[(int)i], tick, &px)) {
            err = "invalid / off-grid trade record"; return false;
        }
        if (data.bid && (*data.bid)[(int)i] > 0 && !tf_support::nativeToTicks((*data.bid)[(int)i], tick, &bid)) bid = 0;
        if (data.ask && (*data.ask)[(int)i] > 0 && !tf_support::nativeToTicks((*data.ask)[(int)i], tick, &ask)) ask = 0;
        out.push_back(tf_support::TradeIdentity{localSec(raw),(double)raw,px,bid,ask,(long long)qty});
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
        // Do not silently guess a new prefix. Explicit fresh bootstrap on the next safe timer.
        cur.reset(); S().bootstrapDone = false; step = 0;
        return;
    }
    long long got=0;
    for (size_t i=begin; i<records.size(); ++i) {
        const auto& r=records[i];
        tfl::Tick k; k.t=r.second; k.px=r.price; k.q=r.quantity; k.bid=r.bid; k.ask=r.ask;
        const bool accepted=eng.add(k);
        cur.commit(r); // consumed even if late; engine invalidates affected evidence
        if (accepted) ++got;
    }
    if (got) {
        wallAtTick=time(0); S().steadyAtTick=std::chrono::steady_clock::now();
        dataAtTick=cur.lastSec; liveTicks+=got; S().bootstrapDone=true;
    }
    if (dataAtTick>=0) {
        const long long elapsed=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-S().steadyAtTick).count();
        const long long watermark=std::min(nowS-LATE_MARGIN,dataAtTick+elapsed-LATE_MARGIN);
        eng.advanceTo(watermark);
    }
}

bool TapeFlow::offHours() const
{
    struct tm t; std::memset(&t,0,sizeof(t)); const_cast<TapeFlow*>(this)->getLocaltime(const_cast<TapeFlow*>(this)->currentDate(),&t);   // (1.1.0 integration) the real SDK declares currentDate() non-const
    int m = t.tm_hour * 60 + t.tm_min;
    // Same configured local clock as the 17:00 session policy. CME-style Central
    // maintenance assumption; Sunday evening is LIVE, not "off hours".
    if (t.tm_wday==6) return true;
    if (t.tm_wday==0) return m<17*60;
    if (t.tm_wday==5 && m>=16*60) return true;
    return m>=16*60 && m<17*60;
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
        if (!S().storageReady) { stepNote="deep backfill paused: crash-guard directory unavailable"; return; }
        if (tfl::priorSessions(store, eng.curSid) >= 10) { deepDone = true; step = NSTEPS; return; }
        if (!offHours()) return;                                                  // older history only outside trading hours
    }
    rebuild(step);
    lastStepAt = time(0);
    step++;
}

// Replay native history into scratch state; publish only calibration and unseen historical prefixes.
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
    tfl::Store scratch = store;
    tfl::Engine e2; e2.cfg = eng.cfg; e2.attach(&scratch, sym);
    {                                                             // the request starts inside an older session: that one is partial
        long long startS = nowS - back, sidS = tfl::sessionOf(startS);
        if (sidS != tfl::sessionOf(nowS) && startS > (sidS - 1) * 86400 + 17LL * 3600 + 60) e2.noFlushSid = sidS;
    }
    TCursor c2; long long first = -1, quoted = 0;
    std::vector<tf_support::TradeIdentity> records;
    std::size_t begin = 0; std::string issue;
    if (!snapshot(T,records) || !c2.plan(records,&begin,&issue)) {
        if (!issue.empty()) err=issue;
        guardClear(idx); stepNote="backfill rejected: " + err; trace(stepNote); return false;
    }
    for (const auto& r:records) {
        if (first < 0) first=r.second;
        tfl::Tick k; k.t=r.second; k.px=r.price; k.q=r.quantity; k.bid=r.bid; k.ask=r.ask;
        if (k.bid > 0 && k.ask > k.bid) ++quoted;
        e2.add(k); c2.commit(r);
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
    e2.advanceTo(nowS - LATE_MARGIN);
    e2.flushSession();
    // Publish imported calibration, not recomputed live episodes/history.
    store = std::move(scratch);
    if (!S().bootstrapDone) {
        captureCommitted();
        const long long protectedThrough = S().committed.empty() ? LLONG_MIN : S().committed.back().t;
        e2.evs.erase(std::remove_if(e2.evs.begin(),e2.evs.end(),[&](const tfl::Ev& event) {
            return event.t <= protectedThrough;
        }),e2.evs.end());
        eng = std::move(e2); eng.attach(&store, sym); eng.noFlushSid = LLONG_MIN;
        S().captureCount=0; S().captureFirst=LLONG_MIN; S().indexedSize=(size_t)-1;
        // Restore committed records at/after the recovery cut; calibration is never a rewrite.
        captureCommitted();
        cur = c2; wallAtTick = time(0); S().steadyAtTick = std::chrono::steady_clock::now(); dataAtTick = cur.lastSec;
        S().bootstrapDone = true;
    } else {
        // Extend only previously unseen historical data; preserve live samples/events.
        const long long firstLive = eng.hist.empty() ? eng.lastT : eng.hist.front().t;
        const auto prefixEnd = std::lower_bound(e2.hist.begin(),e2.hist.end(),firstLive,
            [](const tfl::SecRec& rec,long long time) { return rec.t < time; });
        eng.hist.insert(eng.hist.begin(),e2.hist.begin(),prefixEnd);
        // (1.1.2) the first catch-up (10 min) started cold, so its first 30-180 s have no 30-s / 180-s values: the pane showed a
        // gap where IRT was restarted. Fill ONLY those missing display values from this longer replay (same seconds, same
        // trades); signals, episodes and committed events are untouched.
        {
            size_t j = (size_t)(prefixEnd - e2.hist.begin());
            for (size_t i = (size_t)(prefixEnd - e2.hist.begin()); i < eng.hist.size() && j < e2.hist.size(); ++i) {
                tfl::SecRec& h = eng.hist[i];
                while (j < e2.hist.size() && e2.hist[j].t < h.t) ++j;
                if (j >= e2.hist.size() || e2.hist[j].t != h.t) continue;
                const bool any = std::isnan(h.f30) || std::isnan(h.f180);
                if (std::isnan(h.f30)) h.f30 = e2.hist[j].f30;
                if (std::isnan(h.f180)) h.f180 = e2.hist[j].f180;
                if (!any) break;                                   // past the cold start: nothing more to fill
            }
            S().indexedSize = (size_t)-1;                          // rebuild the drawing index
        }
        for (const auto& event:e2.evs) if (event.t < firstLive) {
            const std::string key=std::to_string(event.t)+"|"+std::to_string(event.ep)+"|"+event.kind+"|"+std::to_string(event.dir);
            if (S().committedKeys.insert(key).second) S().committed.push_back(event);
        }
        std::stable_sort(S().committed.begin(),S().committed.end(),[](const tfl::Ev& a,const tfl::Ev& b){ return a.t<b.t; });
    }
    if (spec.deep && S().bootstrapDone) {
        // Future candidates can use newly imported prior-only calibration. Existing
        // episodes keep their own frozen thresholds/version; historical samples are untouched.
        eng.refreeze();
        tfl::Ev calibration; calibration.t=eng.lastT; calibration.knownAt=eng.lastT+1;
        calibration.kind="BASE"; calibration.ep=eng.ver; calibration.ver=eng.ver;
        calibration.why="prior-only calibration refreshed; no live replay";
        eng.evs.push_back(calibration); captureCommitted();
    }
    saveStore(); evWritten = (size_t)-1;
    return true;
}

// ---- guards
void TapeFlow::readGuards()
{
    guardsRead = true;
    if (!S().storageReady) return;
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
    if (!S().storageReady || idx<0 || idx>=NSTEPS) return;
    std::string p = guardPath("trying", STEPS[idx].name);
    int strikes = 0; { std::ifstream g(p.c_str()); if (g.good()) g >> strikes; }
    std::ofstream o(p.c_str(), std::ios::trunc); o << strikes << "\n";
}
void TapeFlow::guardClear(int idx) { if (S().storageReady && idx>=0 && idx<NSTEPS) std::remove(guardPath("trying", STEPS[idx].name).c_str()); }

// ---- the baseline store (this plugin's own files: one per session, so only today's file is ever rewritten)
std::string TapeFlow::basePath(long long sid) const { return tfDir() + "\\" + mkt + "-base-v110-" + std::to_string(sid) + ".csv"; }
void TapeFlow::loadStore()
{
    storeLoaded = true;
    long long now = localSec(currentDate()), sid0 = tfl::sessionOf(now);
    long rows = 0;
    for (long long sid = sid0; sid >= sid0 - 21; sid--) {      // three weeks of sessions (weekends have no file)
        std::ifstream f(basePath(sid).c_str());
        if (!f.is_open()) { f.clear(); f.open((basePath(sid)+".bak").c_str()); }
        if (!f.is_open()) continue;
        std::string ln; long n = 0;
        while (std::getline(f, ln)) {
            long long s; std::string sy; tfl::Win w;
            if (!tfl::parseStoreLine(ln, &s, &sy, &w) || s != sid) continue;
            store[sid][sy].push_back(w); n++;
        }
        rows += n; // content digest is established on the first verified save
    }
    trace("baseline files: " + std::to_string(store.size()) + " sessions, " + std::to_string(rows) + " windows");
    if (eng.curSid != LLONG_MIN) eng.refreeze();
}
void TapeFlow::saveStore()
{
    if (!storeLoaded || !S().storageReady || tfDir().empty()) return;
    while (store.size() > 14) store.erase(store.begin()); // bounded RAM; existing disk files untouched
    for (const auto& kv:store) {
        std::ostringstream payload;
        payload << "# TapeFlow " << TF_VERSION << " baseline " << mkt << " session " << kv.first << "\n";
        for (const auto& c:kv.second) for (const auto& w:c.second)
            payload << tfl::storeLine(kv.first,c.first,w) << "\n";
        const std::string bytes=payload.str();
        const std::uint64_t digest=tf_support::fingerprint(bytes);
        const auto prior=savedDigest.find(kv.first);
        if (prior!=savedDigest.end() && prior->second==digest) continue;
        const std::string path=basePath(kv.first), tmp=path+".tmp", bak=path+".bak";
        { std::ofstream out(tmp.c_str(),std::ios::binary|std::ios::trunc);
          if (!out) { err="cannot write "+tmp; continue; }
          out.write(bytes.data(),static_cast<std::streamsize>(bytes.size())); out.flush();
          if (!out.good()) { err="cannot flush "+tmp; continue; } }
        bool hadOld=false; { std::ifstream old(path.c_str()); hadOld=old.good(); }
        // Never delete a backup until its original has been established intact.
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
        savedDigest[kv.first]=digest;
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
    // Disk journal retains history. Bound only the in-memory drawing cache.
    if (!S().committed.empty() && S().committed.size()>20000) {
        const long long cutoff=eng.lastT-eng.cfg.keepSec;
        const auto end=std::lower_bound(S().committed.begin(),S().committed.end(),cutoff,
            [](const tfl::Ev& e,long long t){return e.t<t;});
        for (auto it=S().committed.begin();it!=end;++it) S().committedKeys.erase(memoryEventKey(*it));
        S().committed.erase(S().committed.begin(),end);
    }
}
void TapeFlow::writeEvents(bool force)
{
    if (!S().storageReady || mkt.empty() || tfDir().empty() || eng.curSid == LLONG_MIN) return;
    captureCommitted();
    const auto& events=S().committed;
    if (!force && !events.empty() && events.size()==evWritten && events.back().t==evLastT) return;
    typedef std::pair<std::string, std::string> PendingEvent;       // key, complete line
    std::map<long long, std::vector<PendingEvent> > add;
    std::set<std::string> pending;
    for (const tfl::Ev& v : events) {
        long long sid = tfl::sessionOf(v.knownAt);
        if (sid < eng.curSid - 1) continue;                       // older sessions were written at the time
        if (!keysLoaded.count(sid)) {                             // the keys already in that session's file
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
        if (incompleteTail) o << '\n'; // preserve bad tail but separate a complete retry
        if (fresh) o << "time|episode|kind|dir|zone_lo|zone_hi|h|q95|f30|f180|activity|cov30|q_zone|concentration|progress_ticks|context|why|baseline_ver|version|mode\n";
        for (const PendingEvent& event : kv.second) o << event.second;
        o.flush();
        if (!o.good()) { err = "cannot write " + p; complete = false; continue; }
        for (const PendingEvent& event : kv.second) written.insert(event.first);
    }
    if (complete) {
        evWritten = events.size(); evLastT = events.empty() ? 0 : events.back().t;
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
    if (!err.empty()) { *col=C_MUTED; return "DATA / STORAGE ISSUE: "+err; }
    if (!f.quoteOk && f.warm) { *col=C_MUTED; return "DATA INVALID - quote-at-trade snapshot unavailable"; }
    if (f.warm && f.c30 < eng.cfg.cov) { *col=C_MUTED; return "LOW SIDE COVERAGE - signals unavailable"; }
    if (!f.spreadOk) { *col=C_MUTED; return "WIDE SPREAD - signals unavailable"; }
    if (!f.classRecent && f.warm) { *col=C_MUTED; return "NO RECENT CLASSIFIED EXECUTION"; }
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
            return std::string("RESPONSE / INITIATIVE ") + (it->dir > 0 ? "bullish " : "bearish ") + (it->kind == "AR" ? "response" : "initiative") + (TF_PROVEN ? "" : "?") + " at " + clock12(it->knownAt);
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
      << "\nTIMER," << (timerOn ? "on" : timerRefused ? "REFUSED (calc-only fallback; backfill paused)" : "starting")
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
    std::unique_lock<std::recursive_mutex> lock(S().access,std::try_to_lock);
    if (!lock.owns_lock()) return;
    RCT pane; pane.getPaneRect(false);
    short L = pane.left, T = pane.top, R = pane.right, B = pane.bottom;
    if (R - L < 60 || B - T < 40) return;
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    std::string chartMkt = marketForRoot(rs ? rs : "");
    int fs = font;
    const char* activeSymbol = getSymbol();
    if (activeSymbol && !sym.empty() && sym != activeSymbol) {
        textLJ((short)(L+8),(short)(T+12),"Wrong contract context: one active contract per extension object",C_MUTED,fs,false);
        return;
    }
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
    const long long firstTime=H.empty()?LLONG_MIN:H.front().t, lastTime=H.empty()?LLONG_MIN:H.back().t;
    if (S().indexedSize!=H.size() || S().indexedFirst!=firstTime || S().indexedLast!=lastTime) {
        S().renderIndex.build(H,tfl::SR_SIDES);   // (1.1.1) colour = the sides are known; signal quality is not a drawing gate
        S().indexedSize=H.size(); S().indexedFirst=firstTime; S().indexedLast=lastTime;
    }
    const auto& index=S().renderIndex;
    const auto& displayEvents=S().committed;
    if (n > 0 && b1 >= b0 && !H.empty()) {
        RTARRAYI dt(barDateTime);
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        // (1.0.3) bar width: the 30-s sub-bars split 90% of the candle spacing
        int spb = getSecondsPerBar();
        if (spb<=0) {
            textLJ((short)(L+8),(short)(T+12),"TapeFlow requires a time-based chart; use your 3-minute chart",C_MUTED,fs,false);
            return;
        }
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
            long k = idxAtOrBefore(te-1); // SecRec.t labels [t,t+1), not the boundary's next second
            if (k < 0 || H[(size_t)k].t < ts) { px180 = -1; continue; }   // no flow data for this bar
            const tfl::SecRec& q = H[(size_t)k];
            // (1.0.3) no grey whisker: not in the design, and it read as extra bars
            {   // (1.0.3, Rassul 20:46 "see if you made mistakes" - the design is 30-SEC pressure bars) one thin bar per 30 s inside
                // each chart bar (6 per 3-min candle): bought - sold over bought + sold in that 30 s, as in the original design and
                // the mockup. (1.0.0-1.0.2 drew ONE bar per candle = only the last 30 s, so the colour flipped candle to candle.)
                // Brightness = how busy those 30 s were vs normal for this time of day; grey = quiet tape (signals off).
                int span = std::max(1, (int)(ppb * 0.9));
                int slots=(int)((te-ts+29)/30);
                slots=std::max(1,std::min(slots,std::max(1,span/2)));
                // At sufficient zoom, six 30s buckets per 3min candle. At low zoom,
                // aggregate ALL time into fewer buckets, rather than overlap/omit bars.
                double sw = (double)span / slots;
                short xl = (short)(x - span / 2);
                for (int sl = 0; sl < slots; sl++) {
                    long long a0=ts+(te-ts)*sl/slots, a1=ts+(te-ts)*(sl+1)/slots;
                    const size_t j0=index.start(a0), j1=index.start(a1); // [a0,a1)
                    const long double sb=index.buys[j1]-index.buys[j0], ss=index.sells[j1]-index.sells[j0];
                    const size_t nall=j1-j0, nq=index.quality[j1]-index.quality[j0];
                    const size_t na=index.activityCount[j1]-index.activityCount[j0];
                    const long double sa=index.activity[j1]-index.activity[j0];
                    if (!(sb+ss>0)) continue;
                    const double fb=(double)(100*(sb-ss)/(sb+ss));
                    // (1.1.1) coloured when most of the bucket's seconds have known sides and the tape is not quiet (< 0.5x normal);
                    // 1.1.0 greyed a bucket if ANY second lacked full signal quality (which also needs the baseline), so the
                    // whole pane was grey while calibrating and patchy in thin markets.
                    const bool quiet = na > 0 && (double)(sa / na) < 0.5;
                    const bool good = nall > 0 && nq * 2 >= nall && !quiet;
                    double al = na ? std::max(0.20, std::min(0.85, 0.20 + 0.30 * (double)(sa / na))) : 0.45;
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
                if (px180 >= 0) line(px180, py180, sampleX, y, C_PURPLE, 2);
                px180 = sampleX; py180 = y;
            } else px180 = -1;
            {                                                          // events are in time order: find this bar's by search
                auto it = std::upper_bound(displayEvents.begin(), displayEvents.end(), ts, [](long long t, const tfl::Ev& e) { return t < e.knownAt; });
                for (; it != displayEvents.end() && it->knownAt <= te; ++it) marks.push_back(std::make_pair(i, &*it));
            }
        }
        // markers: bullish lane +115, bearish -115, stacked toward the middle when a bar has several
        std::map<std::pair<int, int>, int> stack;
        for (auto& m : marks) {
            const tfl::Ev& v = *m.second;
            bool sig = v.kind == "AW" || v.kind == "AR" || v.kind == "IN";
            bool x_ = v.kind == "FAIL" || v.kind == "INVP" || v.kind == "DINV" || v.kind == "EXP";
            if (!sig && !x_) continue;
            int lane = v.dir > 0 ? 1 : -1;
            int k = stack[std::make_pair(m.first, lane)]++;
            if (k > 3) continue;                                   // the 5th and later are counted below (+n)
            PNT p; p.set(m.first, 0.0f, kBarCenter); short x = p.h;
            const long long end=localSec((RTDATE)dt[m.first]);
            const long long start=end-spb;
            const double fraction=std::max(0.0,std::min(1.0,(double)(v.knownAt-start)/(double)spb));
            x=(short)(x+(fraction-0.5)*ppb*0.9);
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
