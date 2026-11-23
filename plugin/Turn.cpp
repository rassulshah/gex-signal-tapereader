/*
 *  Turn.cpp  --  Investor/RT RTX extension  lsTurn  "Turn v1.0"  1.0.0  (2026-11-22)
 *
 *  Rassul 2026-11-22 CT: "Put the red bars below the green bars"; "I want you to sanity check and get independent sanity check , code and
 *  design review , no memory leaks and give me a final mockup before I approve to build"; "Let's rename the indicator to Turn since it
 *  describes the process of a turn"; "build". Spec SPEC-TURN-1.0.md, mockup turn-final.html, review Turn-v1.0-review.md.
 *  An upgrade of lsEffortRewardPane 0.2.0 (EffortRewardPane.cpp): the same host (order feed, files, volume at price), a new read.
 *
 *  Its own pane, no settings, no letters. Two columns per candle, drawn only where the candle pushes into a named key level (the highest
 *  named high it reached from below: green up; the lowest named low it reached from above: red down) and on every sweep candle:
 *    a swept HIGH (buyers push, sellers answer):  UP   left Stop Run (light green)   right Effort (green)
 *                                                 DOWN left Exhaustion (light red)   right Answer (red)        - the sweep candle only
 *    a swept LOW: the mirror (red down = what the sellers did, green up = the turn). Grey = the reading says No. Full height = 3.0x.
 *  The strip: "<level> <time>   Heavy Stop Run / Heavy Effort / Moderate Exhaustion / Moderate Answer   >> BEST ODDS ?" + the odds pill,
 *  for the newest sweep (lsRevRead's Sw) at or before the last candle in view.
 *  Stop Run, Exhaustion and the Answer come from the order-by-order feed while the pane is on the chart: decided once after the candle
 *  closes, recorded in lsFlexLevels\TurnPane\turn-<MKT>-<spb>.txt and never recomputed. Older sweeps: the nightly's SW rows in
 *  lsFlexLevels\LRA-TurnPane-<MKT>.csv (read from the order store, shown with "(nightly)"). Effort: IRT's volume at price.
 *  All logic: TurnLogic.h (namespace turn, SDK-free, tests_turn).
 */
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
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "Turn"
#endif
#include "PerfGuard.h"
#include "HostSlot.h"
#include "MarketKey.h"
#include "TurnLogic.h"
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <set>
#include <sys/stat.h>
#include <vector>

#define TURN_VERSION "1.0.0"   // = turn::VERSION = the setVersion literal

namespace trh {

typedef cppExtension::RTARRAYP RTARRAYP; typedef cppExtension::BARSTATISTICS BARSTATISTICS; typedef cppExtension::VOLPROFILE VOLPROFILE;
static const COLOR C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_LGREEN = 0x0086EFAC, C_LRED = 0x00FCA5A5, C_AMBER = 0x00F59E0B, C_TEXT = 0x00E2E8F0,
                   C_MUTED = 0x0094A3B8, C_AXIS = 0x0064748B, C_STRIP = 0x000F172A, C_BLACK = 0x00000000;
#ifndef TURN_FILE_CHECK_MS
#define TURN_FILE_CHECK_MS 5000
#endif
#ifndef TURN_STATUS_MS
#define TURN_STATUS_MS 30000
#endif
static const long long TIMER_MS = 250, TIMER_BUDGET_MS = 15, SLICE_BUDGET_MS = 8, FILE_CHECK_MS = TURN_FILE_CHECK_MS, STATUS_MS = TURN_STATUS_MS, FORMING_MS = 1000;
static const int TIMER_MAX = 20000, SLICE_MAX = 5000, BAR_KEEP = 3000, BAR_PASS = 400;
static const long long WAIT_MS = 60000, SILENT_MS = 30000, RECONNECT_GAP_MS = 60000, TRADE_LAG_MS = 5000, GUARD_STALE_S = 2 * 3600;
#ifndef TURN_DECIDE_WAIT_MS
#define TURN_DECIDE_WAIT_MS 5000
#endif
#ifndef TURN_NOSUM_WAIT_MS
#define TURN_NOSUM_WAIT_MS 20000
#endif
// the chart's new bar closes the live candle only after the feed was quiet for CLOSE_QUIET_MS; a read is decided DECIDE_WAIT_MS after the close
static const long long CLOSE_QUIET_MS = 3000, DECIDE_WAIT_MS = TURN_DECIDE_WAIT_MS, NOSUM_WAIT_MS = TURN_NOSUM_WAIT_MS, BEHIND_WAIT_MS = 60000, BAR_RECHECK_MS = 5000;
static const int COVER_MIN_PCT = 95;   // the candle's fills read must be >= this % of the chart's bar volume to decide live

inline long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline short coord(long long v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

// ---- files (no SDK) ----------------------------------------------------------------------------------------------------------
inline std::string flexDir() { const char* up = std::getenv("USERPROFILE"); return up && up[0] ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
inline std::string ownDir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\TurnPane"; }
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    if (!p.empty()) CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
inline long long fileSig(const std::string& p) {                  // mtime x 1000003 + size; -1 = no regular file
#if defined(_WIN32)
    struct _stat64 st; if (p.empty() || _stat64(p.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (p.empty() || stat(p.c_str(), &st) != 0) return -1;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return -1;
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
// bounded, torn-safe: the stamp before and after the read must agree; false = keep the last good copy
inline bool readCapped(const std::string& p, size_t cap, std::string& out, long long& sig) {
    const long long a = fileSig(p); if (a < 0) { sig = -1; out.clear(); return true; }
    std::ifstream f(p.c_str(), std::ios::binary); if (!f) return false;
    std::string s; s.reserve(8192); char buf[8192];
    while (f.good() && s.size() <= cap) { f.read(buf, sizeof buf); const std::streamsize n = f.gcount(); if (n <= 0) break; s.append(buf, (size_t)n); }
    f.close();
    if (fileSig(p) != a) return false;
    if (s.size() > cap) s.assign(cap + 1, ' ');                    // too big: the parser rejects it (never a half file)
    out.swap(s); sig = a; return true;
}
inline bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// <path>.tmp, the old file to .bak, rename into place; on failure the .bak goes back: a reader never sees a partial file
inline bool publishFile(const std::string& path, const std::string& contents) {
    if (path.empty()) return false;
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = fileSig(path) >= 0;
    if (hadOld) { std::remove(bak.c_str()); if (!moveOver(path, bak)) { std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    if (hadOld) std::remove(bak.c_str());
    return true;
}

// ---- the order feed of one chart (OrderFlowFeed.h Conn pattern, lsMboCheck 1.1.0) ---------------------------------------------
// bDboSubscribed is private: read through a pointer-to-member from an explicit instantiation (no layout assumptions)
template <class Tag, typename Tag::type M> struct Rob { friend typename Tag::type robbed(Tag) { return M; } };
struct DboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type robbed(DboFlag); };
template struct Rob<DboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;
// one getNext() behind a structured-exception guard: 1 = an event, 0 = none queued, -1 = the native call faulted
inline int safeNext(cppExtension::DEPTH_BY_ORDER* d) {
#if defined(_MSC_VER)
    __try { return d->getNext() == RTX_OK ? 1 : 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#else
    try { return d->getNext() == RTX_OK ? 1 : 0; } catch (...) { return -1; }
#endif
}
inline int safeFlag(cppExtension::DEPTH_BY_ORDER* d) {
#if defined(_MSC_VER)
    __try { return (d->*robbed(DboFlag())) ? 1 : 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#else
    try { return (d->*robbed(DboFlag())) ? 1 : 0; } catch (...) { return -1; }
#endif
}
struct Feed {
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr;
    int attempt = 0, flag = -1; bool blocked = false, dead = false, timerOff = false, named = false;
    long long openMs = -1, lastEventMs = -1, lastResubMs = -1, events = 0, faults = 0, capHits = 0, reconnects = 0, lastReconnectMs = -1;
    long long tradeKey = -1, lastTradeMs = -1, timerEvents = 0, pumpEvents = 0;
    std::string via, state = "waiting";
    Feed() {}
    Feed(const Feed&) = delete; Feed& operator=(const Feed&) = delete;
    ~Feed() { close(); }
    void close() { delete dbo; dbo = nullptr; }                   // the destructor unsubscribes
    void reset() { close(); attempt = 0; flag = -1; blocked = dead = timerOff = named = false; openMs = lastEventMs = lastResubMs = -1; via.clear(); state = "waiting"; }
    bool due(long long now) const {
        if (blocked || dead || attempt >= 2) return false;
        if (attempt == 1) return lastEventMs < 0 && openMs >= 0 && now - openMs >= WAIT_MS;
        return true;
    }
    // chart context only. 1st: the chart symbol spelled out (readable from the timer); after 60 s with no event, one 2nd try with NULL.
    // A crash-guard file marks each open: if IRT dies inside it, the next start does not open again (a guard older than 2 h is cleared).
    void open(const std::string& symbol, const std::string& guard, long long now) {
        if (!due(now)) return;
        if (attempt == 0 && !guard.empty() && fileSig(guard) >= 0) {
            const long long mt = fileSig(guard) / 1000003LL, age = (long long)std::time(nullptr) - mt;
            if (mt > 0 && age > GUARD_STALE_S) std::remove(guard.c_str());
            else { blocked = true; attempt = 2; state = "IRT closed while opening the order feed: delete " + guard + " to retry"; return; }
        }
        if (!guard.empty()) { std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (attempt + 1) << "\n"; }
        try {
            if (attempt == 0 && !symbol.empty()) { dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(symbol.c_str()); via = "chart symbol " + symbol; named = true; attempt = 1; }
            else { close(); dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(NULL); via = "default (this chart)"; named = false; attempt = 2; }
        } catch (...) { ++faults; dead = true; }
        flag = -1; openMs = now;
        if (!dbo && !dead) { dead = true; state = "order feed unavailable (out of memory)"; }
        if (!guard.empty()) std::remove(guard.c_str());
    }
    // chart context, before open(): a feed silent for 30 s while the chart trades is closed (open() reopens it); returns true when it did
    bool watch(long long now, long long volKey) {
        if (volKey != tradeKey) { if (tradeKey != -1) lastTradeMs = now; tradeKey = volKey; }
        if (dead || blocked) return false;                             // a natively faulted feed is never reopened (review #16)
        const bool silent = dbo && flag == 1 && lastEventMs >= 0 && now - lastEventMs >= SILENT_MS && lastTradeMs > lastEventMs + TRADE_LAG_MS;
        if (!silent || (lastReconnectMs >= 0 && now - lastReconnectMs < RECONNECT_GAP_MS)) return false;
        reset(); ++reconnects; lastReconnectMs = now; return true;
    }
    // read up to maxN events / budget ms into sink; a native fault stops the feed for good (never re-enter it)
    template <class Sink> int read(long long now, int maxN, long long budgetMs, bool fromTimer, Sink sink) {
        if (!dbo || dead) return 0;
        if (fromTimer && (timerOff || !named || flag != 1)) return 0;
        if (!fromTimer && flag != 1 && lastResubMs >= 0 && now - lastResubMs < 1000 && now >= lastResubMs) return 0;
        const long long t0 = steadyMs(); int n = 0;
        if (!fromTimer) { const int f0 = safeFlag(dbo); if (f0 < 0) { ++faults; dead = true; return 0; } if (!f0) lastResubMs = now; }
        for (;;) {
            if (n >= maxN) { ++capHits; break; }
            if ((n & 511) == 511 && steadyMs() - t0 > budgetMs) { ++capHits; break; }
            const int r = safeNext(dbo);
            if (r < 0) { ++faults; if (fromTimer) timerOff = true; else dead = true; break; }
            if (r == 0) break;
            ++n; sink(dbo->dbo);
        }
        if (!fromTimer && dbo && !dead) { const int f1 = safeFlag(dbo); if (f1 < 0) { ++faults; dead = true; } else flag = f1; }
        if (n > 0) { lastEventMs = now; events += n; if (fromTimer) timerEvents += n; else pumpEvents += n; }
        return n;
    }
    std::string text(long long now) const {
        if (blocked) return state;
        if (dead) return "order feed fault: reload the indicator";
        if (flag == 0) return "not subscribed";
        if (lastEventMs < 0) return openMs >= 0 && now - openMs >= WAIT_MS && attempt >= 2 ? "no events" : "waiting";
        return now - lastEventMs > SILENT_MS ? "quiet" : "ready";
    }
};

// SEH-guarded VAP reads (as lsRevRead): -1 = a fault (never read again on this chart), -2 = no data
inline int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
inline int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !rws || bar < 0 || count <= 0) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rws[i], 0, sizeof(VOLPROFILE)); if (vap->getVolumeProfile(bar, i, rws[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's volume at price in ticks; 1 = read, 0 = no VAP, -1 = fault
inline int vapRows(RTARRAYP& vap, int bar, double tick, std::vector<turn::Row>& out, std::vector<VOLPROFILE>& scratch) {
    out.clear(); if (vap.count <= bar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, bar, &st);
    if (prices == -1) return -1;
    if (prices <= 0 || prices > 20000) return 0;
    scratch.resize((size_t)prices);
    const int got = readPrices(&vap, bar, &scratch[0], prices);
    if (got == -1) return -1;
    if (got < 0) return 0;
    for (const VOLPROFILE& v : scratch) {
        if (!std::isfinite((double)v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.buyVolume + v.sellVolume <= 0) continue;
        const double q = (double)v.price / tick; if (!std::isfinite(q) || std::fabs(q) > 4e15) continue;
        turn::Row r; r.tick = (long long)std::llround(q); r.buy = (long long)v.buyVolume; r.sell = (long long)v.sellVolume; out.push_back(r);
    }
    return out.empty() ? 0 : 1;
}

// ---- per-chart state ---------------------------------------------------------------------------------------------------------
struct BarVal { turn::Bars b; bool have = false; };
struct State {
    bool busy = false;
    std::string identity, market, symbol, note; int spb = 0; double tick = 0; bool supported = false;
    Feed feed; turn::Live live;
    int timerId = 0; bool timerOn = false, timerTried = false;
    // Effort: chart bar end -> ratios from IRT's volume at price (closed bars final; the forming one refreshed once a second)
    std::map<long long, BarVal> bars; long long barsCount = -1, barsFirstKey = 0, formingKey = 0, formingMs = -100000; bool vapFault = false; long long vapMissing = 0;
    std::vector<turn::Row> rows; std::vector<VOLPROFILE> scratch;
    // files
    long long fileCheckMs = -100000; std::string paramsPath, wyPath, recPath;
    long long paramsSig = -2, wySig = -2; turn::Params P; std::vector<turn::Sw> sweeps; std::vector<turn::Level> levels; std::string wyWhy = "no sweep file yet";
    std::map<long long, size_t> swAt, histAt;                        // bar end -> index into sweeps / P.hist
    turn::Rec rec; bool recLoaded = false, recDirty = false; long long recWrites = 0, recFails = 0, recSig = -2;
    std::map<long long, long long> closedSeen;                       // live sum key -> steady ms first seen closed (decided >= 5 s later)
    std::map<long long, long long> barSeen;                          // chart bar key -> steady ms first seen closed (a level candle with no live sum)
    std::map<long long, long long> recheck;                          // closed bar key -> steady ms its volume at price is read once more
    std::set<long long> noLive;                                       // sweeps not decidable live (low coverage / other contract), bounded below
    long long candMark = 0;                                           // candles at a level: closed bars up to here are decided (bars closed before the load never are)
    long long capSeen = 0, lowCover = 0, wrongContract = 0, lastCoverPct = -1; bool dirMade = false;
    long long statusMs = -100000; std::string lastStatus, lastRead; long long draws = 0, calcs = 0, lastClosedKey = 0, decidedLive = 0, candsLive = 0;
};

}  // namespace trh

using namespace trh;

class TurnPane : public cppExtension {
public:
    TurnPane() : cppExtension() {}
    virtual int parmsTitle(char* pStr, int size) {
        if (!pStr || size <= 0) return RTX_FAIL;
        std::snprintf(pStr, (size_t)size, "Turn v%s", TURN_VERSION); return RTX_OK;
    }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = 1; return RTX_OK; }
    // ---- the 250 ms order-feed clock (IRT may deliver it in another chart's context: the id says whose state it is) ----
    virtual int timer(RTX_EVENT* e) {
        if (!e) return RTX_FAIL;
        State* S = nullptr; { std::map<int, State*>::const_iterator it = timers().find(e->v.timer.id); if (it != timers().end()) S = it->second; }
        if (!S) return RTX_FAIL;                                       // not ours: let IRT pass it on
        if (S->busy) return RTX_OK;
        S->busy = true; struct Unbusy { State* s; ~Unbusy() { s->busy = false; } } ub = { S };
        try { const long long now = steadyMs(); State& R = *S;
              S->feed.read(now, TIMER_MAX, TIMER_BUDGET_MS, true, [&R](const DEPTH_ORDER& o) { route(R, o); }); } catch (...) {}   // a fault sets timerOff; the pump stops the timer in chart context
        return RTX_OK;
    }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        pump();
        State* S = slot_.get(this, false); if (!S) return RTX_OK;
        if (S->busy) return RTX_OK;                                    // never draw inside a re-entered pump
        try { render(*S); ++S->draws; } catch (...) { S->note = "draw error"; }
        return RTX_OK;
    }
    void calcHook() { pump(); }
    void release() { State* S = slot_.get(this, false); if (S) { stopTimer(*S); persistStatus(*S, steadyMs(), true); } slot_.release(this); }
private:
    HostSlot<State> slot_;
    static std::map<int, State*>& timers() { static std::map<int, State*> m; return m; }
    void stopTimer(State& S) {
        if (S.timerOn) { destroyTimer(S.timerId); S.timerOn = false; }
        if (S.timerId) timers().erase(S.timerId);
        S.timerId = 0;
    }
    void startTimer(State& S) {                                        // chart context, once the named stream is subscribed
        if (S.timerTried || S.timerOn || !S.feed.named || S.feed.flag != 1 || S.feed.timerOff) return;
        S.timerTried = true;
        int id = 43000 + (int)(((uintptr_t)&S >> 4) % 800000);
        while (timers().count(id)) ++id;
        if (createTimer(id, (int)TIMER_MS) == RTX_OK) { S.timerId = id; S.timerOn = true; timers()[id] = &S; }
    }
    // one order event -> the logic (no SDK call: safe from the timer)
    static void route(State& S, const DEPTH_ORDER& o) {
        if (!(S.tick > 0) || S.spb <= 0) return;
        const double p = (double)o.price; if (!std::isfinite(p) || p <= 0) { ++S.live.n.bad; return; }
        const double q = p / S.tick; if (!std::isfinite(q) || q > 4e15) { ++S.live.n.bad; return; }
        S.live.onEvent((long long)o.timestamp, (int)o.action, (int)o.buySell, (long long)std::llround(q), (long long)o.size, (long long)o.orderID, (long long)o.aggressorOrderID);
    }
    void identify(State& S) {
        char rb[32]; std::memset(rb, 0, sizeof rb);
        const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
        const std::string root = rs ? rs : "", sym = sy ? sy : ""; const int spb = getSecondsPerBar();
        double tick = (double)getProperty(SYM_TICKINCR); if (!std::isfinite(tick) || tick <= 0 || tick >= 1000) tick = 0;
        const std::string id = root + "|" + sym + "|" + std::to_string(spb);
        if (id == S.identity && tick == S.tick) return;
        stopTimer(S); S.timerTried = false; S.feed.reset();          // a new symbol / bar size: start clean (unsubscribes)
        S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = mkey::norm(root.empty() ? sym.substr(0, 2) : root);
        static const char* const K[] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }; S.supported = false; for (const char* k : K) if (S.market == k) S.supported = true;
        if (spb != 180) S.supported = false;                          // the read was measured on 3-minute sweep candles only
        S.live.reset(spb > 0 && 86400 % spb == 0 ? spb : 0);
        S.bars.clear(); S.barsCount = -1; S.barsFirstKey = 0; S.formingKey = 0; S.vapFault = false; S.vapMissing = 0;
        const std::string own = ownDir(), flex = flexDir();
        S.paramsPath = flex.empty() || !S.supported ? std::string() : flex + "\\LRA-TurnPane-" + S.market + ".csv";
        S.wyPath = flex.empty() || !S.supported || spb <= 0 ? std::string() : flex + "\\RevRead\\wy-" + S.market + "-" + std::to_string(spb) + ".txt";
        S.recPath = own.empty() || !S.supported || spb <= 0 ? std::string() : own + "\\turn-" + S.market + "-" + std::to_string(spb) + ".txt";
        S.paramsSig = S.wySig = -2; S.P = turn::defaults(S.market); S.sweeps.clear(); S.levels.clear(); S.swAt.clear(); S.histAt.clear();
        S.rec = turn::Rec(); S.recLoaded = false; S.recDirty = false; S.recSig = -2;
        S.closedSeen.clear(); S.barSeen.clear(); S.recheck.clear(); S.noLive.clear(); S.candMark = 0; S.capSeen = 0; S.fileCheckMs = -100000;
        S.lastStatus.clear(); S.statusMs = -100000;
    }
    void pump() {
        State* P = slot_.get(this, true);
        if (!P || P->busy) return;
        State& S = *P; S.busy = true; struct Unbusy { State* s; ~Unbusy() { s->busy = false; } } ub = { P };
        try {
            identify(S); ++S.calcs;
            const long long now = steadyMs();
            const long count = getBarCount();
            if (!(S.tick > 0)) { S.note = "native tick size unavailable"; }
            else if (S.spb <= 0 || 86400 % S.spb != 0) { S.note = "needs bars that divide the day"; }
            else if (!S.supported) { S.note = "reads 3-minute charts of ES NQ CL GC HG NG EU only"; }
            else if (count < 2) { S.note = "waiting for bars"; }
            else {
                S.note.clear();
                RTARRAYI times(barDateTime), vols(barVolume);
                if (times.count >= count && vols.count >= count) {
                    S.lastClosedKey = (long long)times[(int)count - 2];
                    barsPass(S, count, times, now);
                    {
                        if (S.feed.watch(now, (long long)times[(int)count - 1] * 1000003LL + (long long)vols[(int)count - 1])) { stopTimer(S); S.timerTried = false; S.live.gap(); }
                        const std::string own = ownDir(); if (!S.dirMade) { makeDir(own); S.dirMade = true; }
                        S.feed.open(S.symbol, own.empty() ? std::string() : own + "\\_opening-" + S.market + "-" + std::to_string(S.spb) + ".txt", now);
                        S.live.setLimit((long long)times[(int)count - 1]);
                        { State& R = S; S.feed.read(now, SLICE_MAX, SLICE_BUDGET_MS, false, [&R](const DEPTH_ORDER& o) { route(R, o); }); }   // the backlog is read before any close
                        startTimer(S);
                        if (S.feed.timerOff) stopTimer(S);
                        if (S.feed.dead && S.timerOn) stopTimer(S);
                        if (S.feed.capHits == S.capSeen && (S.feed.lastEventMs < 0 || now - S.feed.lastEventMs >= CLOSE_QUIET_MS)) S.live.closeThrough(S.lastClosedKey);
                        S.capSeen = S.feed.capHits;   // a quiet feed: the chart's next bar closes it (else the next event does, in order)
                        files(S, now);
                        decide(S, now, times, vols, count);
                    }
                }
            }
            persistStatus(S, now, false);
        } catch (...) { S.note = "calculation error"; }
    }
    // ---- Effort from IRT's volume at price: closed bars once (newest first, BAR_PASS a pass), the forming bar once a second ----
    template <class TA> void barsPass(State& S, long count, TA& times, long long now) {
        if (S.vapFault) return;
        RTARRAY hi(barHigh), lo(barLow); if (hi.count < count || lo.count < count) return;
        if (S.barsCount > count) { S.bars.clear(); S.recheck.clear(); }   // the chart was reloaded (keys are bar times: a rolling window keeps them)
        S.barsCount = count; S.barsFirstKey = (long long)times[0];
        RTARRAYP vap(barVolumeProfile);
        auto one = [&](int i) -> bool {
            const double h = (double)hi[i], l = (double)lo[i]; BarVal v; v.have = true;
            if (std::isfinite(h) && std::isfinite(l) && h >= l && l > 0) {
                const int r = vapRows(vap, i, S.tick, S.rows, S.scratch);
                if (r < 0) { S.vapFault = true; S.note = "volume at price fault"; return false; }
                if (r == 0) ++S.vapMissing;
                else v.b = turn::barRatios(S.rows, (long long)std::llround(l / S.tick), (long long)std::llround(h / S.tick));
            }
            S.bars[(long long)times[i]] = v; return true;
        };
        const long long t0 = steadyMs(); int done = 0;
        for (int i = (int)count - 2; i >= 0 && i >= (int)count - 1 - BAR_KEEP && done < BAR_PASS; --i) {
            if (S.bars.count((long long)times[i])) continue;
            if (!one(i)) return;
            ++done; if ((done & 31) == 31 && steadyMs() - t0 > 10) break;
        }
        const long long fk = (long long)times[(int)count - 1];
        if (fk != S.formingKey && S.formingKey != 0) { S.bars.erase(S.formingKey); S.recheck[S.formingKey] = now; }   // the bar that just closed: read again now and once more 5 s later
        for (std::map<long long, long long>::iterator it = S.recheck.begin(); it != S.recheck.end();) {
            if (now - it->second >= BAR_RECHECK_MS || now < it->second) { S.bars.erase(it->first); it = S.recheck.erase(it); } else ++it; }
        for (int i = (int)count - 2; i >= 0 && i >= (int)count - 4; --i) if (!S.bars.count((long long)times[i])) { if (!one(i)) return; }
        if (fk != S.formingKey || now - S.formingMs >= FORMING_MS || now < S.formingMs) { S.formingKey = fk; S.formingMs = now; if (!one((int)count - 1)) return; }
        while ((long long)S.bars.size() > BAR_KEEP + 8) S.bars.erase(S.bars.begin());
    }
    // ---- params, the sweep file and the record: re-read only when their stamp changed, at most every 5 s ----
    void mergeRec(State& S, const turn::Rec& f) {                      // another chart of this market may have written: theirs kept, ours never replaced
        for (std::map<long long, turn::Read>::const_iterator it = f.reads.begin(); it != f.reads.end(); ++it) if (!S.rec.reads.count(it->first)) S.rec.reads[it->first] = it->second;
        for (std::map<long long, turn::Cand>::const_iterator it = f.cands.begin(); it != f.cands.end(); ++it) if (!S.rec.cands.count(it->first)) S.rec.cands[it->first] = it->second;
        turn::trimRec(S.rec);
    }
    void files(State& S, long long now) {
        if (now - S.fileCheckMs < FILE_CHECK_MS && now >= S.fileCheckMs) return;
        S.fileCheckMs = now;
        if (!S.recLoaded || fileSig(S.recPath) != S.recSig) {
            std::string t; long long sig = -1;
            if (readCapped(S.recPath, turn::REC_MAX_BYTES, t, sig)) {
                turn::Rec f; if (sig >= 0 && turn::parseRecFile(t, S.market, S.spb, f)) mergeRec(S, f);
                S.recLoaded = true; S.recSig = sig;
            }
        }
        if (S.recDirty) writeRec(S);                                    // a failed write is retried
        if (fileSig(S.paramsPath) != S.paramsSig) {
            std::string t; long long sig = -1;
            if (readCapped(S.paramsPath, turn::PARAMS_MAX_BYTES, t, sig)) {
                S.P = turn::parseParams(t, S.market); S.paramsSig = sig;
                S.histAt.clear(); for (size_t k = 0; k < S.P.hist.size(); ++k) S.histAt[S.P.hist[k].stamp] = k;
            }
        }
        if (fileSig(S.wyPath) != S.wySig) {
            std::string t; long long sig = -1;
            if (readCapped(S.wyPath, turn::WY_MAX_BYTES, t, sig)) {
                S.wySig = sig; std::vector<turn::Sw> v; std::vector<turn::Level> lv; std::string why;
                if (sig < 0) { S.sweeps.clear(); S.levels.clear(); S.swAt.clear(); S.wyWhy = "no sweep file (lsRevRead on a " + std::to_string(S.spb) + "-s chart writes it)"; }
                else if (turn::parseWy(t, S.market, S.spb, v, lv, why)) {
                    S.sweeps.swap(v); S.levels.swap(lv); S.wyWhy = "ok";
                    S.swAt.clear(); for (size_t k = 0; k < S.sweeps.size(); ++k) S.swAt[S.sweeps[k].stamp] = k;
                }
                else S.wyWhy = why;                                    // a garbled file keeps the last good list
            }
        }
    }
    int todOf(long long sec) {
        struct tm t; std::memset(&t, 0, sizeof t);
        if (sec <= 0 || !getLocaltime((RTDATE)sec, &t) || t.tm_hour < 0 || t.tm_hour > 23 || t.tm_min < 0 || t.tm_min > 59) return -1;
        return t.tm_hour * 2 + t.tm_min / 30;
    }
    void writeRec(State& S) {
        if (!S.recLoaded) return;                                       // never replace a file we could not read
        std::string t; long long sig = -1;                              // merge what another chart wrote since our read
        if (fileSig(S.recPath) != S.recSig && readCapped(S.recPath, turn::REC_MAX_BYTES, t, sig)) {
            turn::Rec f; if (sig >= 0 && turn::parseRecFile(t, S.market, S.spb, f)) mergeRec(S, f);
        }
        turn::trimRec(S.rec);
        if (!S.dirMade) { makeDir(ownDir()); S.dirMade = true; }
        if (publishFile(S.recPath, turn::recFileText(S.rec, S.market, S.spb))) { ++S.recWrites; S.recDirty = false; S.recSig = fileSig(S.recPath); } else { ++S.recFails; S.recDirty = true; }
    }
    static bool covered(const turn::Sum& s, long long vol, long long& pct) {
        if (vol <= 0) { pct = -1; return false; }
        pct = (s.lots * 100 + vol / 2) / vol; return pct >= COVER_MIN_PCT;
    }
    // ---- decided once, after the candle close: every closed candle at a named level (its Stop Run) and every Sw (the read) ----
    template <class TA, class VA> void decide(State& S, long long now, TA& times, VA& vols, long count) {
        bool wrote = false;
        for (const turn::Sum& x : S.live.sums) if (!S.closedSeen.count(x.key)) S.closedSeen[x.key] = now;
        while (S.closedSeen.size() > 2 * turn::SUMS_KEEP) S.closedSeen.erase(S.closedSeen.begin());
        while (S.noLive.size() > turn::SW_KEEP) S.noLive.erase(S.noLive.begin());
        // candles at a named level: only bars that closed while the pane was loaded, oldest first, each once
        if (S.candMark == 0) S.candMark = S.lastClosedKey;
        RTARRAY hi(barHigh), lo(barLow), op(barOpen);
        if (hi.count >= count && lo.count >= count && op.count >= count) {
            for (long i = std::max(0L, count - 2 - (long)turn::SUMS_KEEP); i <= count - 2; ++i) {
                const long long key = (long long)times[(int)i]; if (key <= S.candMark) continue;
                if (!S.barSeen.count(key)) S.barSeen[key] = now;
                const long long seen = S.barSeen[key];
                const turn::Sum* s = S.live.find(key); const turn::Sum* whole = nullptr; bool ready = false;
                if (s) {
                    const std::map<long long, long long>::const_iterator cs = S.closedSeen.find(key);
                    if (cs != S.closedSeen.end() && (now - cs->second >= DECIDE_WAIT_MS || now < cs->second)) {
                        ready = true; long long pct = -1;
                        if (s->complete && s->stopOk && covered(*s, (long long)vols[(int)i], pct)) whole = s;
                    }
                } else {
                    const bool behind = S.live.forming() != 0 && S.live.forming() <= key && S.feed.lastEventMs >= 0 && now - S.feed.lastEventMs < 5000 && now >= S.feed.lastEventMs;
                    ready = now < seen || now - seen >= (behind ? BEHIND_WAIT_MS : NOSUM_WAIT_MS);   // no order data for this candle: recorded without a Stop Run
                }
                if (!ready) break;
                const double h = (double)hi[(int)i], l = (double)lo[(int)i], o = (double)op[(int)i];
                if (!S.levels.empty() && !S.rec.cands.count(key) && std::isfinite(h) && std::isfinite(l) && std::isfinite(o) && h >= l && l > 0 && o >= l && o <= h) {
                    const turn::Cand c = turn::candOf(key, S.levels, (long long)std::llround(l / S.tick), (long long)std::llround(h / S.tick), (long long)std::llround(o / S.tick), whole, S.P);
                    if (c.hiL || c.loL) { S.rec.cands[key] = c; wrote = true; ++S.candsLive; }
                }
                S.candMark = key;
            }
        }
        while (S.barSeen.size() > 4 * turn::SUMS_KEEP) S.barSeen.erase(S.barSeen.begin());
        // sweeps: every Sw whose candle is still in the live sums
        for (const turn::Sw& w : S.sweeps) {
            if (w.stamp > S.lastClosedKey || S.rec.reads.count(w.stamp) || S.noLive.count(w.stamp)) continue;
            const turn::Sum* s = S.live.find(w.stamp); if (!s || !s->complete) continue;
            const std::map<long long, long long>::const_iterator cs = S.closedSeen.find(w.stamp);
            if (cs == S.closedSeen.end() || (now - cs->second < DECIDE_WAIT_MS && now >= cs->second)) continue;   // late reads first
            const bool oldest = !S.live.sums.empty() && S.live.sums.front().key == w.stamp;
            if (!S.P.fromFile && !oldest) continue;                       // wait for the nightly's tables while the candle is kept
            long long vol = -1; for (long i = count - 2; i >= 0 && i >= count - 40; --i) if ((long long)times[(int)i] == w.stamp) { vol = (long long)vols[(int)i]; break; }
            if (vol <= 0) continue;
            long long pct = -1; const bool cov = covered(*s, vol, pct); S.lastCoverPct = pct;
            if (!cov) { if (S.noLive.insert(w.stamp).second) ++S.lowCover; continue; }   // missed events: not decided live (the nightly row shows)
            const turn::Turn& t = w.dir < 0 ? s->hi : s->lo;
            const std::map<long long, BarVal>::const_iterator bi = S.bars.find(w.stamp);
            const bool effOk = bi != S.bars.end() && (w.dir < 0 ? bi->second.b.topOk : bi->second.b.botOk);
            const double eff = effOk ? (w.dir < 0 ? bi->second.b.top : bi->second.b.bot) : 0;
            const turn::Read r = turn::decideLive(*s, w, S.P, t.ok ? todOf(t.extSec) : -1, effOk, eff);
            if (r.src != 'L') { if (S.noLive.insert(w.stamp).second) ++S.wrongContract; continue; }
            S.rec.reads[w.stamp] = r; wrote = true; ++S.decidedLive; S.lastRead = turn::readText(r, '|');
        }
        if (wrote) writeRec(S);
    }
    // the read of a sweep: the live record, else the nightly's row, else none (Effort from the chart only)
    turn::Read readFor(const State& S, const turn::Sw& w) const {
        turn::Read r; bool have = false;
        const std::map<long long, turn::Read>::const_iterator it = S.rec.reads.find(w.stamp);
        if (it != S.rec.reads.end() && it->second.dir == w.dir && it->second.lvt == w.lvt) { r = it->second; have = true; }
        if (!have) { const std::map<long long, size_t>::const_iterator h = S.histAt.find(w.stamp);
            if (h != S.histAt.end() && h->second < S.P.hist.size()) { const turn::Read& x = S.P.hist[h->second]; if (x.dir == w.dir && x.lvt == w.lvt) { r = x; have = true; } } }
        if (!have) { r.stamp = w.stamp; r.dir = w.dir; r.lvt = w.lvt; }
        r.lv = w.lv;
        if (r.eff1000 < 0) { const std::map<long long, BarVal>::const_iterator bi = S.bars.find(w.stamp);
            if (bi != S.bars.end() && (w.dir < 0 ? bi->second.b.topOk : bi->second.b.botOk)) r.eff1000 = turn::x1000(w.dir < 0 ? bi->second.b.top : bi->second.b.bot); }
        return r;
    }
    // ---- drawing ----
    void fill(int l, int t, int r, int b, COLOR c) { setPen(c, 1, P_SOLID); RCT rc; rc.set(coord(l), coord(t), coord(r), coord(b)); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
    void text(const std::string& s, int l, int t, int r, int b, COLOR c, bool centred) { setTextColor(c); RCT rc; rc.set(coord(l), coord(t), coord(r), coord(b)); rc.drawText(s.c_str(), centred, false); }
    static COLOR toneColor(int tone, int dir, bool forFill) {
        return tone == turn::T_DIR ? (dir < 0 ? C_RED : C_GREEN) : tone == turn::T_AMBER ? C_AMBER : tone == turn::T_TEXT ? C_TEXT : (forFill ? C_AXIS : C_MUTED);
    }
    std::string prefixOf(const turn::Read& r, long long lastKey) {
        struct tm a, b; std::memset(&a, 0, sizeof a); std::memset(&b, 0, sizeof b);
        std::string p = r.lv;
        if (r.stamp > 0 && getLocaltime((RTDATE)r.stamp, &a)) {
            std::string d;
            if (lastKey > 0 && getLocaltime((RTDATE)lastKey, &b) && (a.tm_mday != b.tm_mday || a.tm_mon != b.tm_mon)) { char x[16]; std::snprintf(x, sizeof x, "%d-%02d ", (a.tm_mon + 1) % 100, a.tm_mday % 100); d = x; }
            const std::string c = turn::clockText(a.tm_hour, a.tm_min); if (!c.empty()) p += " " + d + c;
        }
        if (r.src == 'N') p += " (nightly)";
        return p;
    }
    // a strip piece: its width with the spaces at its ends counted as fixed gaps (never left to how the host measures a trailing space);
    // core = the text without them, lead = the gap before it
    int segWidth(const std::string& t, std::string& core, int& lead) {
        const int sp = std::max(3, (int)getTextWidth("a a", -1) - (int)getTextWidth("aa", -1));
        const size_t a = t.find_first_not_of(' '), b = t.find_last_not_of(' ');
        if (a == std::string::npos) { core.clear(); lead = 0; return (int)t.size() * sp; }
        core = t.substr(a, b - a + 1); lead = (int)a * sp;
        return lead + (int)getTextWidth(core.c_str(), -1) + (int)(t.size() - 1 - b) * sp;
    }
    int stripWidth(const turn::Strip& st, FONT& f, FONT& fb) {       // (the SDK's setFont takes a non-const FONT&)
        int w = 0; std::string core; int lead = 0;
        for (const turn::Seg& g : st.segs) { setFont(g.bold ? fb : f); w += segWidth(g.text, core, lead); }
        return w;
    }
    // one side of one candle: the pushing side's bars (Stop Run left, Effort right) and, on a sweep candle, the turn's bars on the other side
    void drawSide(int x, int dir, const turn::BarSet& bs, int axis, int half, int bw, bool labels) {
        const int gap = bw >= 12 ? 2 : bw >= 6 ? 1 : 0, w = std::max(1, std::min((bw - gap - 2) / 2, 20));
        const int xl = x - gap / 2 - w, xr = x + (gap - gap / 2);
        const bool pushUp = dir < 0;                                    // a swept high: the buyers' bars go up, the turn's bars down
        struct One { const turn::Bar1* b; int left; bool up; COLOR c; };
        const One all[4] = { { &bs.stop, xl, pushUp, pushUp ? C_LGREEN : C_LRED }, { &bs.eff, xr, pushUp, pushUp ? C_GREEN : C_RED },
                             { &bs.exh, xl, !pushUp, pushUp ? C_LRED : C_LGREEN }, { &bs.ans, xr, !pushUp, pushUp ? C_RED : C_GREEN } };
        for (const One& o : all) {
            if (!o.b->show) continue;
            const int h = turn::barPx(o.b->v, half); if (h <= 0) continue;
            const COLOR c = o.b->on ? o.c : C_AXIS;
            if (o.up) fill(o.left, axis - h, o.left + w, axis, c); else fill(o.left, axis + 1, o.left + w, axis + 1 + h, c);
            if (!labels || o.b->label.empty()) continue;
            const int lw = (int)getTextWidth(o.b->label.c_str(), -1) + 2; int l = o.left + w / 2 - lw / 2;
            if (o.left < x) { if (l + lw > x - 1) l = x - 1 - lw; } else if (l < x + 1) l = x + 1;   // the two labels of a candle never touch
            if (o.up) text(o.b->label, l, axis - h - 13, l + lw, axis - h - 1, c, true); else text(o.b->label, l, axis + 1 + h + 1, l + lw, axis + 1 + h + 13, c, true);
        }
    }
    void render(State& S) {
        RCT pane; pane.getPaneRect(false);
        const int L = pane.left, R = pane.right, T = pane.top, B = pane.bottom; if (B - T < 40 || R - L < 40) return;
        FONT f10; f10.id = HELVETICA; f10.size = 10; f10.style = PLAIN; FONT f10b = f10; f10b.style = BOLD;
        // nothing to read: say why in one grey line (the wrong kind of chart, or lsRevRead's sweep file is missing), never an empty pane
        const std::string why = !S.note.empty() ? S.note : (S.sweeps.empty() && S.wyWhy != "ok") ? S.wyWhy : std::string();
        if (!why.empty() && R - L >= 260) { setFont(f10); text("Turn: " + why, L + 8, T + 18, R - 8, T + 33, C_MUTED, false); }
        if (!S.note.empty()) return;
        const long count = getBarCount(); if (count < 2) return;
        RTARRAYI times(barDateTime); if (times.count < count) return;
        int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1); if (v1 < v0) return;
        const int bw = std::max(1, (int)getPixelsPerBar());
        int xLast = R; { PNT p; p.set(v1, 0.0f, kBarCenter); xLast = std::min(R - 2, (int)p.h + bw / 2 + 2); }
        setFont(f10b);
        const bool labels = bw >= 2 * ((int)getTextWidth("9.9x", -1) + 2) + 6;   // room for two ratio labels side by side on one candle
        // the strip (row below IRT's title row): the newest sweep at or before the last candle in view
        const int sTop = T + 16, sBot = sTop + 18; int areaTop = T + 16;
        const long long lastKey = std::min((long long)times[v1], S.lastClosedKey);
        const turn::Sw* sw = nullptr; for (size_t k = S.sweeps.size(); k-- > 0;) if (S.sweeps[k].stamp <= lastKey) { sw = &S.sweeps[k]; break; }
        if (sw && xLast - L >= 220 && B - T >= 90) {
            const turn::Read rd = readFor(S, *sw);
            const int l = L + 4, r = xLast;
            turn::Strip st = turn::stripOf(rd, S.P.bestProven, !labels, prefixOf(rd, (long long)times[v1]));
            setFont(f10b); const int pw = (int)getTextWidth(st.pill.c_str(), -1) + 14;
            const int tx0 = l + 10, avail = r - pw - 14 - tx0;
            if (stripWidth(st, f10, f10b) > avail) st = turn::stripOf(rd, S.P.bestProven, false, prefixOf(rd, (long long)times[v1]));   // too long: without the numbers,
            if (stripWidth(st, f10, f10b) > avail) st = turn::stripOf(rd, S.P.bestProven, false, std::string());                       // then without the level and time,
            size_t first = 0; if (stripWidth(st, f10, f10b) > avail && !st.segs.empty()) first = st.segs.size() - 1;                 // then the verdict alone
            fill(l, sTop, r, sBot, C_STRIP);
            fill(l, sTop, l + 4, sBot, toneColor(st.edgeTone, rd.dir, true));
            setFont(f10b);
            fill(r - pw - 4, sTop + 2, r - 4, sBot - 2, toneColor(st.pillTone, rd.dir, true)); text(st.pill, r - pw - 4, sTop + 2, r - 4, sBot - 2, C_BLACK, true);
            int tx = tx0;
            for (size_t k = first; k < st.segs.size(); ++k) {
                const turn::Seg& g = st.segs[k]; setFont(g.bold ? f10b : f10);
                std::string core; int lead = 0; const int w = segWidth(g.text, core, lead); if (tx + w > r - pw - 8) break;
                if (!core.empty()) text(core, tx + lead, sTop + 2, tx + lead + (int)getTextWidth(core.c_str(), -1), sBot - 1, toneColor(g.tone, rd.dir, false), false);
                tx += w;
            }
            areaTop = sBot + 2;
        }
        // the bars: one middle axis; a swept high's push goes up and its turn down, a swept low's the other way
        const int top = areaTop + (labels ? 13 : 2), bot = B - (labels ? 13 : 2); if (bot - top < 20) return;
        const int axis = (top + bot) / 2, half = std::max(4, (bot - top) / 2 - 1);
        fill(L, axis, xLast, axis + 1, C_AXIS);
        setFont(f10b);
        for (int i = v0; i <= v1; ++i) {
            const long long key = (long long)times[i];
            const std::map<long long, size_t>::const_iterator si = S.swAt.find(key);
            const std::map<long long, turn::Cand>::const_iterator ci = S.rec.cands.find(key);
            const bool isSw = si != S.swAt.end() && si->second < S.sweeps.size() && key <= S.lastClosedKey;
            if (!isSw && ci == S.rec.cands.end()) continue;
            PNT p; p.set(i, 0.0f, kBarCenter); const int x = p.h; if (x < L || x > R) continue;
            const std::map<long long, BarVal>::const_iterator bi = S.bars.find(key);
            if (isSw) {
                const turn::Sw& w = S.sweeps[si->second]; const turn::Read rd = readFor(S, w);
                if (rd.src != '-' || ci == S.rec.cands.end()) { drawSide(x, w.dir, turn::barsOf(rd), axis, half, bw, labels); continue; }
            }
            const turn::Cand& c = ci->second;                           // a candle at a named level: Stop Run + Effort of the side(s) it traded at
            if (c.hiL) { turn::BarSet bs; bs.stop = turn::ratioBar(c.hiX1000, 50); if (bi != S.bars.end() && bi->second.b.topOk) bs.eff = turn::ratioBar(turn::x1000(bi->second.b.top), 0); drawSide(x, -1, bs, axis, half, bw, labels); }
            if (c.loL) { turn::BarSet bs; bs.stop = turn::ratioBar(c.loX1000, 50); if (bi != S.bars.end() && bi->second.b.botOk) bs.eff = turn::ratioBar(turn::x1000(bi->second.b.bot), 0); drawSide(x, 1, bs, axis, half, bw, labels); }
        }
    }
    // ---- status: lsFlexLevels\TurnPane\status-<MKT>-<spb>.txt, on change or every 30 s ----
    void persistStatus(State& S, long long now, bool force) {
        if (!force && now - S.statusMs < STATUS_MS && now >= S.statusMs) return;
        const std::string own = ownDir(); if (own.empty() || S.market.empty() || S.spb <= 0) return;
        std::ostringstream o;
        o << "VERSION|" << TURN_VERSION << "\nMARKET|" << S.market << "|" << S.symbol << "|" << S.spb << "|tick " << S.tick << (S.supported ? "" : "|not a 3-minute chart of the 7 markets: nothing is read") << "\n";
        o << "STATE|" << (S.note.empty() ? "ok" : S.note) << "\n";
        o << "FEED|" << S.feed.text(now) << "|" << S.feed.via << "|events " << S.feed.events << "|timer " << (S.timerOn ? "on" : "off") << " " << S.feed.timerEvents << "|calc " << S.feed.pumpEvents
          << "|faults " << S.feed.faults << "|caps " << S.feed.capHits << "|reconnects " << S.feed.reconnects << "\n";
        o << "LIVE|fills " << S.live.n.fills << "|implied " << S.live.n.implied << "|stop fills " << S.live.n.stopFills << "|candles " << S.live.n.candles << "|complete " << S.live.n.complete
          << "|overflows " << S.live.n.overflows << "|late " << S.live.n.late << "|bad " << S.live.n.bad << "|no aggressor " << S.live.n.noAggr << "|open fills " << S.live.openFills()
          << "|orders " << S.live.orders() << "|order resets " << S.live.n.shownResets << "|clock " << S.live.clockPoints() << "|sums " << S.live.sums.size() << "\n";
        o << "PARAMS|" << S.P.why << "|stop " << S.P.stopTyp << "|hard " << S.P.hardCut << "|big " << S.P.bigCut << "|cuts " << S.P.q1 << " " << S.P.q2 << "|ranks " << S.P.rkLate.size() << " " << S.P.rkDecay.size()
          << " " << S.P.rkAfter.size() << "|odds " << S.P.odds[1] << " " << S.P.odds[2] << " " << S.P.odds[3] << (S.P.bestProven ? " proven" : "") << "|history " << S.P.hist.size() << "\n";
        o << "SWEEPS|" << S.wyWhy << "|" << S.sweeps.size() << "|levels " << S.levels.size() << "\n";
        o << "COVER|last " << S.lastCoverPct << "%|low (not decided) " << S.lowCover << "|other contract " << S.wrongContract << "|future events " << S.live.n.future << "\n";
        o << "RECORD|reads " << S.rec.reads.size() << "|level candles " << S.rec.cands.size() << "|writes " << S.recWrites << "|fails " << S.recFails << "|decided live " << S.decidedLive << "|candles live " << S.candsLive << "\n";
        o << "BARS|" << S.bars.size() << "|vap missing " << S.vapMissing << (S.vapFault ? "|VAP FAULT" : "") << "\n";
        if (!S.lastRead.empty()) o << "LAST|T|" << S.lastRead << "\n";
        const std::string t = o.str();
        S.statusMs = now;
        if (t == S.lastStatus && !force) return;
        if (!S.dirMade) { makeDir(own); S.dirMade = true; }
        if (publishFile(own + "\\status-" + S.market + "-" + std::to_string(S.spb) + ".txt", t)) S.lastStatus = t;
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { try { static_cast<TurnPane*>(this)->release(); } catch (...) {} return RTX_OK; }
int cppExtension::destroy(void) { try { static_cast<TurnPane*>(this)->release(); } catch (...) {} return RTX_OK; }
int cppExtension::setup(void)   { return RTX_OK; }    // no settings window
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); try { static_cast<TurnPane*>(this)->calcHook(); } catch (...) {} return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    TurnPane* p = new TurnPane();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED);                 // its own pane, draws itself, IRT's volume at price
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA Turn v1.0 - the process of a turn at a swept key level (lsRevRead Sw). Two columns per candle at a named level: Stop Run and Effort of the pushing side; on the sweep candle also Exhaustion and the Answer of the other side. Strip: the read (STAND ASIDE / WAIT / BEST ODDS ? / UNCLEAR) and the measured odds.");
    p->setVersion("1.0.0");
    return p;
}
