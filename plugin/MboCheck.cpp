/********************************************************************************
 *  MboCheck.cpp  --  Investor/RT RTX extension  lsMboCheck  (1.0.0, 2026-10-10)
 *
 *  MBO CHECK: does this chart's data feed deliver market-by-order data (DBO / MBO: every order's NEW / MOD / DEL / FILL with
 *  its order id) to plugins through the SDK's DEPTH_BY_ORDER class, and how much full market depth (MARKET_DEPTH) does it give?
 *  Put it on any chart. No settings at all. It draws ONE small line at the top of the price pane, centred over the bars:
 *      MBO: YES 1,240 ev/min (N 410 M 22 D 380 F 428) | depth 10 lvls | last 9:41:05 AM | v1.0.0     green
 *      MBO: NO (not subscribed) | depth 10 lvls | 9:41 AM | v1.0.0                                   red
 *      MBO: subscribed but no events | ...                                                            red
 *      MBO: waiting 42s | ...                                                                         grey (first 60 s)
 *  and writes %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\MboCheck\<MKT>-<seconds per bar>.txt (tmp -> .bak -> rename), only when
 *  the verdict / flag / depth changes or every 30 s: counts per action and side (last 60 s and since load), the last event's
 *  time, bDboSubscribed, the depth levels, and the first 20 raw events. Nothing about the user, login or account is written.
 *
 *  How the SDK works (irtsdk.h 15.1.6 has no examples or docs beyond the class lists, so this was read from the SDK library
 *  irtsdkV143-x64.lib itself - every call is one callIRT() into Investor/RT):
 *    - DEPTH_BY_ORDER(ticker): callIRT(0x88, ticker or NULL) with sub-op 0 = SUBSCRIBE; bDboSubscribed = (IRT returned 0).
 *      getNext(): if not subscribed it asks to subscribe again (sub-op 0) and returns non-OK; if subscribed it asks for the
 *      next event (sub-op 2, &dbo, this) and returns RTX_OK with `dbo` filled, or non-OK (dbo zeroed) when none is queued.
 *      ~DEPTH_BY_ORDER(): sub-op 1 = UNSUBSCRIBE. IRT keeps the queue per object, so ONE object per chart is kept alive and
 *      getNext() is drained until it stops returning RTX_OK. A NULL ticker means "the chart IRT is calling for": an object
 *      opened with NULL is read only from this chart's own calc / draw, never from the timer.
 *      bDboSubscribed is PRIVATE in the header: read through a standard pointer-to-member (explicit template instantiation
 *      may name private members), never by guessing offsets.
 *    - MARKET_DEPTH(ticker): callIRT(0x2F) sub-op 0 = subscribe; maxLevels = what IRT reports (10 on old IRT builds);
 *      operator[](n) = sub-op 2, a pointer into IRT's book (n < maxLevels); available() = sub-op 4 and always asks about
 *      "this chart" (no ticker), so it is read from calc / draw only; the destructor UNSUBSCRIBES (sub-op 1). One object per
 *      chart is kept: IRTReader 0.6 built and destroyed one every second (subscribe + unsubscribe at once) and logged
 *      "snapshots 0" on every market.
 *    - Draining: calc (every tick), draw (POST_DRAWING) and an own 1-s timer each read at most 5,000 events or ~8 ms, then
 *      return; a backlog is carried to the next call and counted in the status file. CALL_CONTINUOUSLY keeps calc / draw
 *      coming while the feed is idle, so "waiting" turns into the verdict without forcing any repaint.
 *    - Opened from calc / draw only (the chart's own context). 1st try: the chart's symbol spelled out; if after 60 s there
 *      is no event, one 2nd try with NULL. A crash guard file (MboCheck\_opening-<MKT>-<sec>.txt) marks each open; if IRT
 *      dies inside it, the next start shows "not checked" instead of opening it again (delete the file to try again).
 *  Logic without the SDK: MboCheckLogic.h (tested). Per-chart state: HostSlot.h.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "MboCheckLogic.h"
#include "HostSlot.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>
#include <sys/types.h>
#include <sys/stat.h>

#define MC_VERSION "1.0.0"          // keep equal to mbo::VERSION and the setVersion literal below

enum { CTX_CALC = 0, CTX_DRAW = 1, CTX_TIMER = 2 };

// ---- bDboSubscribed is private: a pointer-to-member obtained through explicit instantiation (well-formed C++: [temp.explicit]
// lets an explicit instantiation name private members). Reads the real field the SDK sets; no layout assumptions.
template <class Tag, typename Tag::type M> struct McRob { friend typename Tag::type mcMember(Tag) { return M; } };
struct McDboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type mcMember(McDboFlag); };
template struct McRob<McDboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;

struct MCState {
    mbo::Probe p;
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr;
    cppExtension::MARKET_DEPTH* md = nullptr;
    bool busy = false, timerOn = false, timerTried = false, dboDead = false, mdDead = false;
    long long lastDepthMs = -1, lastWriteMs = -1, lastResubMs = -1;
    std::string lastKey;
    mbo::Ident id;
    std::string line; int colour = 0;
    long long writes = 0;              // test counter (#31)
    ~MCState() { delete dbo; delete md; }
};

#ifdef MC_TEST_CLOCK
long long mcTestClockMs();
static long long steadyMs() { return mcTestClockMs(); }
#else
static long long steadyMs()
{
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif
#ifdef MC_TEST_HOOKS
extern int (*mcTestRename)(const char* from, const char* to);
#endif

static std::string flexDir()
{
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static int mcTimerId(const void* me) { return 8311 + (int)(((uintptr_t)me >> 4) % 100000); }
static bool statFile(const std::string& p)
{
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    return (st.st_mode & S_IFMT) == S_IFREG;
}
static void makeDir(const std::string& p)
{
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool moveOver(const std::string& from, const std::string& to)
{
#ifdef MC_TEST_HOOKS
    if (mcTestRename) return mcTestRename(from.c_str(), to.c_str()) == 0;
#endif
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// checklist #5: <path>.tmp, old file to .bak, rename into place; on failure the .bak goes back
static bool publishFile(const std::string& path, const std::string& contents)
{
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    {
        std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(contents.data(), (std::streamsize)contents.size()); f.flush();
        if (!f) { f.close(); std::remove(tmp.c_str()); return false; }
    }
    const bool hadOld = statFile(path);
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    return true;
}
static short coord(int v) { return (short)(v < -32768 ? -32768 : v > 32767 ? 32767 : v); }

static const COLOR C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_GREY = 0x009CA3AF;

class MboCheck : public cppExtension {
public:
    MboCheck() : cppExtension() {}
    virtual int draw(void);
    virtual int timer(RTX_EVENT* e);
    virtual int parmsLoad(void)  { return RTX_OK; }          // no settings: nothing to load, nothing saved (#29)
    virtual int parmsApply(void) { return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { return RTX_OK; }
    void pump(int ctx);
    void release() { MCState* S = slot_.get(this, false); if (S && S->timerOn) { destroyTimer(mcTimerId(S)); S->timerOn = false; } slot_.release(this); }
    MCState* testState() { return slot_.get(this, false); }
private:
    HostSlot<MCState> slot_;
    std::string timeText(RTDATE d, bool withSec);
    void open(MCState& S, const std::string& sym);
    void drain(MCState& S, int ctx, long long nowMs);
    void readDepth(MCState& S, long long nowMs);
    void status(MCState& S, long long nowMs);
    void render(MCState& S);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<MboCheck*>(this)->release(); return RTX_OK; }   // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<MboCheck*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { setParameterVersion(1); return RTX_OK; }                     // no settings at all
int cppExtension::calc(int)     { static_cast<MboCheck*>(this)->pump(CTX_CALC); return RTX_OK; }
int MboCheck::draw(void)        { pump(CTX_DRAW); MCState* S = slot_.get(this, false); if (S) render(*S); return RTX_OK; }
int MboCheck::timer(RTX_EVENT* e)
{
    MCState* S = slot_.get(this, false);
    if (!S || !e || e->v.timer.id != mcTimerId(S)) return RTX_FAIL;
    pump(CTX_TIMER); return RTX_OK;
}

std::string MboCheck::timeText(RTDATE d, bool withSec)
{
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t)) return std::string();                 // IRT's local clock = Central on his PC
    return mbo::twelveHour(t.tm_hour, t.tm_min, t.tm_sec, withSec);
}

// open the DBO stream (and the depth book) - chart context only, behind the crash guard.
// 1st try: the chart's symbol spelled out (every later SDK call then names it, so a timer tick cannot read another chart's
// stream); 2nd try after 60 s without events: NULL (IRT's "this chart"), read from calc / draw only.
void MboCheck::open(MCState& S, const std::string& sym)
{
    mbo::Probe& p = S.p;
    const std::string dir = flexDir();
    const std::string guard = dir.empty() ? std::string() : dir + "\\MboCheck\\_opening-" + mbo::fileStem(S.id.market, S.id.spb) + ".txt";
    if (p.attempt == 0 && !guard.empty() && statFile(guard)) { p.blocked = true; p.attempt = 1; return; }
    if (!guard.empty()) { makeDir(dir + "\\MboCheck"); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (p.attempt + 1) << "\n"; }
    try {
        if (p.attempt == 0) {
            const bool named = !sym.empty();
            S.dbo = new (std::nothrow) DEPTH_BY_ORDER(named ? sym.c_str() : NULL);
            p.via = named ? "chart symbol " + sym : "default (NULL = this chart)";
            if (!S.md && !S.mdDead) S.md = new (std::nothrow) MARKET_DEPTH(named ? sym.c_str() : NULL);
            p.attempt = named ? 1 : 2;                               // no symbol: NULL was the only try
        } else {
            delete S.dbo; S.dbo = nullptr;                           // its destructor unsubscribes (SDK: 0x88 / 1)
            S.dbo = new (std::nothrow) DEPTH_BY_ORDER(NULL);
            p.via = "default (NULL = this chart), 2nd try";
            p.attempt = 2;
        }
    } catch (...) { p.faults++; S.dboDead = true; }
    p.dboSymbol = S.dbo ? mbo::cleanField(S.dbo->symbol, sizeof(S.dbo->symbol)) : std::string();
    p.flag = -1;
    p.windowMs = steadyMs();
    if (!guard.empty()) std::remove(guard.c_str());
}

// at most SLICE_MAX events or SLICE_BUDGET_MS per call; the rest waits for the next call.
// While IRT has not accepted the subscription, every SDK getNext() asks for it again - that is tried at most once a second.
void MboCheck::drain(MCState& S, int ctx, long long nowMs)
{
    mbo::Probe& p = S.p;
    if (!S.dbo || S.dboDead) return;
    if (ctx == CTX_TIMER && p.dboSymbol.empty()) return;            // a NULL stream is read only in this chart's own calls
    if (p.flag != 1 && S.lastResubMs >= 0 && nowMs - S.lastResubMs < 1000 && nowMs >= S.lastResubMs) return;
    const long long t0 = steadyMs();
    int n = 0; bool cap = false, haveLast = false; RTDATE lastStamp = 0;
    try {
        if (!(S.dbo->*mcMember(McDboFlag()))) S.lastResubMs = nowMs;
        while (true) {
            if (n >= mbo::SLICE_MAX) { cap = true; break; }
            if ((n & 511) == 511 && steadyMs() - t0 > mbo::SLICE_BUDGET_MS) { cap = true; break; }
            if (S.dbo->getNext() != RTX_OK) break;
            const DEPTH_ORDER& o = S.dbo->dbo;
            mbo::noteEvent(p, t0, o.action, o.buySell);
            lastStamp = o.timestamp; haveLast = true;
            if (p.samples.size() < (size_t)mbo::SAMPLE_MAX) {
                mbo::Sample e;
                e.when = timeText(o.timestamp, true); e.action = mbo::actionOf(o.action); e.side = mbo::sideOf(o.buySell);
                e.price = o.price; e.prev = o.previousPrice; e.bid = o.bid; e.ask = o.ask;
                e.size = (long long)o.size; e.bidsize = (long long)o.bidsize; e.asksize = (long long)o.asksize;
                e.orderID = o.orderID; e.aggressorID = o.aggressorOrderID;
                p.samples.push_back(e);
            }
            n++;
        }
        p.flag = (S.dbo->*mcMember(McDboFlag())) ? 1 : 0;            // after the reads: getNext may have just subscribed
    } catch (...) { p.faults++; S.dboDead = true; }                  // never re-enter a faulting call (#30)
    if (haveLast) p.lastEventWhen = timeText(lastStamp, true);
    mbo::noteSlice(p, n, cap);
}

// once a second, from the chart's own calls only (the SDK's available() always asks about "this chart")
void MboCheck::readDepth(MCState& S, long long nowMs)
{
    if (!S.md || S.mdDead) return;
    if (S.lastDepthMs >= 0 && nowMs - S.lastDepthMs < 1000 && nowMs >= S.lastDepthMs) return;
    S.lastDepthMs = nowMs;
    mbo::Probe& p = S.p;
    try {
        p.depthSeen = true;
        p.depthAvail = S.md->available() != 0;
        p.depthMax = S.md->maxLevels;
        p.depthLevels = 0; p.bestBid = p.bestAsk = 0; p.bestBidSize = p.bestAskSize = 0;
        if (!p.depthAvail) return;
        int n = p.depthMax; if (n > mbo::DEPTH_READ_MAX) n = mbo::DEPTH_READ_MAX; if (n < 0) n = 0;
        for (int k = 0; k < n; k++) {
            const DEPTH_LEVEL L = (*S.md)[(unsigned)k];                 // a copy: the SDK hands out a pointer into IRT's book
            const bool b = std::isfinite(L.bid) && L.bid > 0 && L.bidsize > 0, a = std::isfinite(L.ask) && L.ask > 0 && L.asksize > 0;
            if (b || a) p.depthLevels++;
            if (k == 0) { if (b) { p.bestBid = L.bid; p.bestBidSize = L.bidsize; } if (a) { p.bestAsk = L.ask; p.bestAskSize = L.asksize; } }
        }
    } catch (...) { p.faults++; S.mdDead = true; }
}

void MboCheck::status(MCState& S, long long nowMs)
{
    const std::string key = mbo::changeKey(S.p, nowMs);
    if (!mbo::statusDue(key, S.lastKey, nowMs, S.lastWriteMs)) return;
    const std::string dir = flexDir(); if (dir.empty()) return;
    makeDir(dir + "\\MboCheck");
    RTDATE now = currentDate();
    struct tm t; std::memset(&t, 0, sizeof(t));
    std::string when;
    if (getLocaltime(now, &t)) {
        char b[16]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        when = b + mbo::twelveHour(t.tm_hour, t.tm_min, t.tm_sec, true);
    }
    const std::string doc = mbo::statusText(S.p, S.id, nowMs, when, S.line);
    S.lastWriteMs = nowMs;                                           // a failed write waits for the next 30 s / change, never loops
    if (publishFile(dir + "\\MboCheck\\" + mbo::fileStem(S.id.market, S.id.spb) + ".txt", doc)) { S.lastKey = key; S.writes++; }
}

void MboCheck::pump(int ctx)
{
    MCState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = true;
    struct Unbusy { MCState* s; ~Unbusy() { s->busy = false; } } unbusy = { S };
    try {
        const long long nowMs = steadyMs();
        mbo::Probe& p = S->p;
        if (p.loadMs < 0) p.loadMs = nowMs;
        if (!S->timerTried) { S->timerTried = true; S->timerOn = createTimer(mcTimerId(S), 1000) == RTX_OK; }
        std::string sym;
        if (ctx != CTX_TIMER) {                                     // the chart's own context: who we are, and open the streams
            char rb[32]; std::memset(rb, 0, sizeof(rb));
            const char* rs = getRootSymbol(rb);
            const char* sy = getSymbol();
            sym = sy ? sy : "";
            S->id.symbol = mbo::cleanField(sym.c_str(), sym.size());
            S->id.market = mbo::marketOf(rs ? rs : "", sym);
            S->id.spb = getSecondsPerBar();
            if (p.attempt == 0) open(*S, S->id.symbol);
            else if (p.attempt == 1 && !p.blocked && p.events == 0 && !S->dboDead && p.windowMs >= 0 && nowMs - p.windowMs >= mbo::WAIT_MS)
                open(*S, S->id.symbol);                             // 2nd try: NULL
        }
        drain(*S, ctx, nowMs);
        if (ctx != CTX_TIMER) readDepth(*S, nowMs);
        struct tm t; std::memset(&t, 0, sizeof(t));
        std::string nowText; if (getLocaltime(currentDate(), &t)) nowText = mbo::twelveHour(t.tm_hour, t.tm_min, t.tm_sec, false);
        S->line = mbo::lineText(p, nowMs, nowText);
        S->colour = mbo::colourOf(mbo::verdict(p, nowMs));
        if (!S->id.market.empty()) status(*S, nowMs);
    } catch (...) {
        // never let a string / file failure reach IRT
    }
}

// one line at the top of the price pane, centred over the bars (never over the empty space right of the last bar)
void MboCheck::render(MCState& S)
{
    if (S.line.empty()) return;
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int right = pane.right;
    if (scale.left > pane.left && scale.left < pane.right) right = scale.left - 2;
    const long n = getBarCount();
    if (n > 0) { PNT last; last.set((int)n - 1, 0.0f); if (last.h > pane.left + 40 && last.h < right) right = last.h; }
    const int left = pane.left + 4;
    if (pane.bottom - pane.top < 24 || right - left < 60) return;   // pane too small: draw nothing rather than overlap
    FONT f; f.id = HELVETICA; f.size = 11; f.style = PLAIN; setFont(f);
    std::string s = S.line;
    int w = getTextWidth(s.c_str(), -1);
    const int avail = right - left;
    if (w > avail) {                                                 // narrow: drop the per-action counts first
        size_t a = s.find(" (N "), b = s.find(") |");
        if (a != std::string::npos && b != std::string::npos && b > a) { s.erase(a, b + 1 - a); w = getTextWidth(s.c_str(), -1); }
    }
    if (w > avail) {                                                 // still too wide: cut the tail, end with "..."
        while (s.size() > 12 && w > avail) { s.resize(s.size() - 1); w = getTextWidth((s + "...").c_str(), -1); }
        s += "...";
    }
    int x = (left + right) / 2 - w / 2; if (x < left) x = left;
    const int y = pane.top + 4;
    setTextColor(S.colour == 1 ? C_GREEN : S.colour == 2 ? C_RED : C_GREY);
    RCT r; r.set(coord(x), coord(y), coord(x + w + 6), coord(y + 16));
    r.drawText(s.c_str(), false, false);
}

extern "C" cppExtension *CreateExtension(void)
{
    MboCheck *p = new MboCheck();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the verdict must appear (and age) while the feed is idle, without forcing repaints
    p->setDescription("LRA MboCheck: shows whether this chart's feed delivers market-by-order (MBO / DBO) events and full depth. One line on the chart; no settings.");
    p->setVersion("1.0.0");   // MC_VERSION
    return p;
}
