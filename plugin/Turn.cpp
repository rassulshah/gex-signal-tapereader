/*
 *  Turn.cpp  --  Investor/RT RTX extension  lsTurn  "Turn v1.9"  1.9.0  (2026-11-24; 1.8.0 the same evening; 1.3.0 .. 1.7.0 2026-11-23 / 24; 1.0.0 2026-11-22)
 *
 *  (1.9.0) Rassul 2026-11-24 19:59-20:00 CT "So have the indicators implemented the volume model", "Go": the read's biggest orders carry their Big Lot
 *  tier ("Biggest 41 (S20)", BigTier.h - layer 4, magnitude) and a "Price" line says whether price agreed over the next 10 closed candles: went 1 ATR
 *  the expected way, failed (traded through the extreme / came back 1 ATR), held so far or waiting (layer 6). Notes NOTES-TURN190.md.
 *
 *  (1.8.0) Rassul 2026-11-24 CT: 17:52 "the absorbtion indicator can be updated to have two bars per candle , 1 for absorbtion and 1 for
 *  initiative"; 18:01 "i want more detail for the intiative buyers .. we have icebergs for passive buyers so what would be for initiative
 *  buyers"; 18:53 "build" (turn-initiative-mockup.html v3). EVERY closed candle: LEFT bar = Absorption at its stronger end, RIGHT bar =
 *  Initiative at its stronger end (up = at the high, down = at the low). Initiative = the COUNTER side's heaviest single price in the end
 *  quarter (sellers at the high, buyers at the low) / its 20-candle average; coloured from 1.5 when those lots beat the attackers' heaviest
 *  price there and the candle closed outside that quarter (buyers at the low green, sellers at the high red). A Stop Run of a named level
 *  takes a free column on its half. Sweep candles keep Stop Run / Absorption / Exhaustion; their right-hand turn bar is the new Initiative
 *  (the old Answer 0-3 is retired). THE READ of an Initiative candle: "Initiative - buyers lifted the offer at the bottom", Initiative
 *  (lots, price, ratio), How (burst vs the prior hour's pace, prices swept by one order, biggest order - from the order feed, "?" until
 *  proven), Against (the attackers there). The How numbers are kept per candle in cread-<MKT>-<spb>.txt ("N" lines). Notes NOTES-TURN180.md.
 *
 *  (1.3.0) Rassul 2026-11-23 CT: 18:15 "is this similar to  stoprun absorbtion exhaustion intiative" ... "can you update my turn indicator to
 *  this .  so update effort to absrobtion also"; 18:28 "i want you to do the absorbtion the right way"; 18:38 "Research first before
 *  building so you do it right"; 19:48 "build" (turn-four-pieces-mockup.html v3 + the research report of 19:10). THE PANE SHOWS THE FOUR
 *  PIECES by default: Stop Run / Absorption / Exhaustion / Initiative (= the full setting of 1.2.0; Initiative is the old Answer).
 *  ABSORPTION replaces Effort on every closed candle and in the top line: the lots at the HEAVIEST SINGLE PRICE in the candle's end quarter
 *  (buys at its top, sells at its bottom) against the average of the same over the 20 closed candles before (10 at least); coloured from
 *  2.0 AND only when the candle CLOSED OUTSIDE that end quarter - in the winner's colour: absorbed at the high = red, at the low = green;
 *  grey otherwise (the number stays). The top line of a plain candle: "<time>   High: <word> Absorption 8.6x / Low: <word> Absorption 4.3x".
 *  Research (603 first RTH tests of a major level, 7 markets, 09-10 .. 11-20): the close test separates turned from failed (73% vs 46%),
 *  Effort does not (0.48); volume adds only after the close test. Per market the volume step is LEARNING (under 20 a side).
 *  The record file, the reads and their Effort column are UNCHANGED (Absorption is drawn from the chart's volume at price; calc reads the
 *  close with each bar, draw reads no bars). display.txt can still switch pieces off (absorption_from|2.0; effort_from still read).
 *  Notes NOTES-TURN130.md.
 *
 *  (1.2.1) Rassul 2026-11-23 11:47 CT "go" on the Stop Run fault (CL counted 81% of its fills as stops since the 9:38 AM load; NQ 42% in
 *  the session before): the order-ID clock was stopped by ONE old order whose number lies above the new ones (IRT's IDs are an 8-digit
 *  counter that starts again at 0). TurnLogic.h IdClock follows the running count only now; the status file's LIVE line tells how often
 *  it had to ("id strays N|id lanes N|id set-backs N"). Nothing is drawn differently. Notes NOTES-TURN121.md.
 *
 *  (1.2.0) Rassul 2026-11-23 CT: 11:00 "show me this indicator with just effort .. show mockup"; 11:04, on the mockup, "meke values that are
 *  less than 1.5 gray"; 11:06 "this look simple and easy  i like it.   lets use this for now.. save other setting because i might change my
 *  mind and add it pieces later". THE PANE SHOWS JUST EFFORT: on every closed candle ONE bar a side, centred on the candle - the buyers'
 *  Effort at its top (up), the sellers' Effort at its bottom (down) - with its number; green / red only when the number written on the bar
 *  is 1.5 or more, grey under it. The top line tells the candle under the mouse, else the last closed candle in view: "<time>   Buyers
 *  <word> Effort 0.6x / Sellers <word> Effort 3.2x". THE OTHER SETTING IS SAVED, NOT REMOVED: the Stop Run bar, the Exhaustion bar, the
 *  Answer bar and the read strip (level, verdict, odds) are whole in this file and are switched on, piece by piece, by
 *  lsFlexLevels\TurnPane\display.txt (turn::Show; no file = Effort only; the full setting is kept next to it as display-full.txt). The
 *  file is re-read when it changes: a piece comes back without a rebuild or a restart. Everything is still read, decided once and recorded
 *  as before (the nightly keeps scoring the reads); only what is DRAWN changed. Mockup turn-just-effort-mockup.html. Notes NOTES-TURN120.md.
 *
 *  (1.1.0) Rassul 2026-11-23 CT, on his CL chart: 09:45 "does the turn indicator only show values when there is a level"; 09:47 "can you
 *  show the indicator on all bars ?"; 09:51 "go -  for now just do effort". EFFORT ON EVERY CLOSED CANDLE: the buyers' Effort at the top
 *  of the candle goes up in green, the sellers' Effort at its bottom goes down in red, both in the right-hand column (where Effort stands
 *  at a level), each with its ratio; grey under 0.5x as before. A candle at a named level keeps its Stop Run and Effort of that side and
 *  gains the other side's Effort; a sweep candle with a read is unchanged (its other half carries Exhaustion and the Answer). The strip
 *  under the mouse on a plain candle: "<time>   Buyers <word> Effort 1.2x / Sellers <word> Effort 1.9x". Effort is IRT's volume at
 *  price of the closed candle (as before): nothing new is recorded, nothing else is read. Stop Run, Exhaustion, the Answer and the
 *  verdict stay at named levels / sweeps. Mockup turn-effort-all-mockup.html. Notes NOTES-TURN110.md.
 *
 *  (1.0.1) Rassul 2026-11-22 22:50-23:00 CT, on his CL chart: "indicator should update when my mouse hovers over different candles";
 *  "top strip should be moved higher to the top of the pane, right now there is blackspace at the top . i cant see the ratios either";
 *  "go" on the three fixes: (1) the strip is about the candle under the mouse while the mouse is in this pane (a sweep candle: its read;
 *  a candle at a named level: its level, time, Stop Run and Effort; a candle with no bars, or the mouse outside the pane: the newest
 *  sweep at or before the last candle in view, as before) - TRACK_MOUSE + PaneCursor.h, as lsVolRead 1.2.0;
 *  (2) the strip sits at the top edge of the pane (the row above it was kept free for a pane title this pane does not have);
 *  (3) candles too narrow for two ratio labels side by side (most zooms: two labels need about 58 px) show ONE ratio per candle side, on
 *  its tallest bar, in that bar's colour; a label that would touch a neighbour's label or bar is left out, the taller bar first.
 *  23:07 CT on the mockup: "the font for the ratio looks big , it may overlap on the other bar next to it, maybe get rid of the x and make it
 *  smaller" - the ratio on a bar is 9 pt and has no x ("2.9", "14"): it fits inside its own 22 px candle. The strip keeps "2.9x".
 *  Nothing else changes: the bars, the reads, the records, the files. Mockup turn-pane-mockup.html. Notes NOTES-TURN101.md.
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
 *  for the newest sweep (lsRevRead's Sw) at or before the last candle in view - or (1.0.1) for the candle under the mouse.
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
#include "PaneCursor.h"
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

#define TURN_VERSION "1.9.0"   // = turn::VERSION = the setVersion literal
#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "user32.lib")   // (1.0.1) WindowFromPoint (PaneCursor.h: GetCursorPos)
#endif

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

// (1.0.1) the window under the screen cursor: tells "the mouse is still over this pane" from "it left" (IRT sends no leave event)
#if defined(TURN_TEST_WINDOW)
}  // namespace trh
extern void* turn_test_window;                                       // the host test's stand-in for WindowFromPoint
namespace trh {
inline void* cursorWindow(int, int) { return turn_test_window; }
#elif defined(_WIN32)
inline void* cursorWindow(int sx, int sy) { POINT p; p.x = sx; p.y = sy; return (void*)WindowFromPoint(p); }
#else
inline void* cursorWindow(int, int) { return nullptr; }
#endif
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
struct BarVal { turn::Bars b; bool have = false; bool awayUp = false, awayDn = false; bool hlc = false; long long hiT = 0, loT = 0, clT = 0; };   // (1.4.0) hlc: the candle's high / low / close in ticks, for the Absorption read   // (1.3.0) awayUp / awayDn = the candle closed outside its top / bottom quarter (Absorption's close test), read with the bar
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
    // (1.0.1) the mouse: the chart bar it is on while it is inside this pane (-1 = not in the pane), where it is, the axis row of the last draw
    pc::Tracker cur; int hov = -1, hovY = 0, axisY = 0; long long mouseMoves = 0;
    int readVerdict = 0, readLines = 0; std::string readHead, readWhy, readWatch;   // (1.4.0) the Absorption read right of the last candle, as last drawn (the status file)
    int stripKind = 0, labMode = 0, labOffered = 0, labDrawn = 0;      // what the last draw showed (the status file): strip 0 none / 1 a sweep / 2 a level candle / (1.1.0) 3 a plain candle
    int iniCandles = 0, iniOn = 0;                                     // (1.8.0) candles the last draw gave an Initiative bar / a coloured one
    int effCandles = 0;                                                // (1.1.0) candles the last draw gave an Effort bar; (1.2.0) every such candle
    // (1.2.0) what the pane shows besides Effort: lsFlexLevels\TurnPane\display.txt (no file = Effort only), re-read when its stamp changes
    turn::Show show; long long showSig = -2; std::string showPath; bool showChanged = false;
    // (1.5.0) the Abs candles lsRevRead published (wy file) and this pane's P / Ex? for each (RevRead\\turnabs-<MKT>-<spb>.txt, read by lsRevReadMarks)
    std::map<long long, std::pair<int, int> > teq; std::string teqPath; bool teqLoaded = false, teqDirty = false; long long teqWrites = 0, teqFails = 0;   /* (1.6.0) every candle's Ex? */ std::vector<turn::AbsLine> abs; std::map<long long, turn::TabRec> tab; std::string tabPath; bool tabLoaded = false, tabDirty = false; long long tabWrites = 0, tabFails = 0; int absShown = 0; std::string absHead, absWhy;
    std::map<std::pair<long long, int>, turn::AbsCalc> crC; std::map<std::pair<long long, int>, turn::IniCalc> niC; /* (1.8.0) the Initiative numbers, both ends */ std::string crPath; bool crLoaded = false, crDirty = false;   /* (1.7.0) every closed candle's order-book numbers, both ends */ std::map<long long, turn::AbsCalc> absC; std::map<long long, turn::NextCalc> absN; std::string absRecPath; bool absRecLoaded = false, absRecDirty = false;   // (1.5.0) the read's numbers once decided (a restart draws the same read)
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
        pump(true);
        State* S = slot_.get(this, false); if (!S) return RTX_OK;
        if (S->busy) return RTX_OK;                                    // never draw inside a re-entered pump
        try { render(*S); ++S->draws; } catch (...) { S->note = "draw error"; }
        return RTX_OK;
    }
    void calcHook() { pump(false); }
    // (1.0.1) the mouse: IRT sends move events only while the cursor is over this pane (TRACK_MOUSE: also when the indicator is not
    // selected). The event names the chart bar it points at; where the pane sits on the screen is remembered so the pump can tell when
    // the cursor has left (no event comes then) - the strip then goes back to the newest sweep. The event is never swallowed.
    virtual int mouse(RTX_EVENT* ev) {
        if (!ev || (ev->type != E_MOUSE_MOVE && ev->type != E_MOUSE_HOVER)) return RTX_FAIL;
        State* S = slot_.get(this, false); if (!S || S->busy) return RTX_FAIL;
        try {
            int sx = 0, sy = 0; const bool ok = pc::cursorPos(sx, sy);
            const int h = (int)ev->v.mouse.h, v = (int)ev->v.mouse.v;
            S->cur.onEvent(ok ? cursorWindow(sx, sy) : nullptr, ok, sx, sy, h, v);
            const long n = getBarCount(); const long b = (long)ev->v.mouse.bar;
            const int hov = (ok && b >= 0 && b < n && S->cur.eventInside(h, v)) ? (int)b : -1;
            const bool sideFlip = hov >= 0 && (v < S->axisY) != (S->hovY < S->axisY);   // a candle at a high AND a low level: which side the mouse is on
            S->hovY = v;
            if (hov != S->hov || sideFlip) { S->hov = hov; ++S->mouseMoves; invalidateChart(false); }
        } catch (...) {}
        return RTX_FAIL;
    }
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
        S.showPath = own.empty() || !S.supported ? std::string() : own + "\\display.txt"; S.showSig = -2; S.show = turn::Show(); S.showChanged = false;   // (1.2.0)
        S.teqPath = flex.empty() || !S.supported || spb <= 0 ? std::string() : flex + "\\RevRead\\turnexq-" + S.market + "-" + std::to_string(spb) + ".txt"; S.teq.clear(); S.teqLoaded = false; S.teqDirty = false;   // (1.6.0)
        S.tabPath = flex.empty() || !S.supported || spb <= 0 ? std::string() : flex + "\\RevRead\\turnabs-" + S.market + "-" + std::to_string(spb) + ".txt"; S.abs.clear(); S.tab.clear(); S.tabLoaded = false; S.tabDirty = false;   // (1.5.0)
        S.crPath = own.empty() || !S.supported || spb <= 0 ? std::string() : own + "\\cread-" + S.market + "-" + std::to_string(spb) + ".txt"; S.crC.clear(); S.niC.clear(); S.crLoaded = false; S.crDirty = false;   // (1.7.0)
        S.absRecPath = own.empty() || !S.supported || spb <= 0 ? std::string() : own + "\\absread-" + S.market + "-" + std::to_string(spb) + ".txt"; S.absC.clear(); S.absN.clear(); S.absRecLoaded = false; S.absRecDirty = false;
        S.paramsSig = S.wySig = -2; S.P = turn::defaults(S.market); S.sweeps.clear(); S.levels.clear(); S.swAt.clear(); S.histAt.clear();
        S.rec = turn::Rec(); S.recLoaded = false; S.recDirty = false; S.recSig = -2;
        S.closedSeen.clear(); S.barSeen.clear(); S.recheck.clear(); S.noLive.clear(); S.candMark = 0; S.capSeen = 0; S.fileCheckMs = -100000;
        S.lastStatus.clear(); S.statusMs = -100000;
        S.hov = -1; S.cur = pc::Tracker();                              // (1.0.1) the bar numbers are another chart's now
    }
    // (1.0.1) the cursor is no longer over this pane: back to the newest sweep (from calc: redraw; from draw: the draw that follows shows it)
    void mouseLeft(State& S, bool fromDraw) {
        if (S.hov < 0) return;
        int sx = 0, sy = 0; const bool ok = pc::cursorPos(sx, sy);
        if (!S.cur.inside(ok ? cursorWindow(sx, sy) : nullptr, ok, sx, sy)) { S.hov = -1; if (!fromDraw) invalidateChart(false); }
    }
    void pump(bool fromDraw) {
        State* P = slot_.get(this, true);
        if (!P || P->busy) return;
        State& S = *P; S.busy = true; struct Unbusy { State* s; ~Unbusy() { s->busy = false; } } ub = { P };
        try { mouseLeft(S, fromDraw); } catch (...) {}
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
                        if (S.showChanged) { S.showChanged = false; if (!fromDraw) invalidateChart(false); }   // (1.2.0) a piece was switched: show it now
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
        RTARRAY hi(barHigh), lo(barLow), cl(barClose); if (hi.count < count || lo.count < count || cl.count < count) return;
        if (S.barsCount > count) { S.bars.clear(); S.recheck.clear(); }   // the chart was reloaded (keys are bar times: a rolling window keeps them)
        S.barsCount = count; S.barsFirstKey = (long long)times[0];
        RTARRAYP vap(barVolumeProfile);
        auto one = [&](int i) -> bool {
            const double h = (double)hi[i], l = (double)lo[i]; BarVal v; v.have = true;
            if (std::isfinite(h) && std::isfinite(l) && h >= l && l > 0) {
                const int r = vapRows(vap, i, S.tick, S.rows, S.scratch);
                if (r < 0) { S.vapFault = true; S.note = "volume at price fault"; return false; }
                if (r == 0) ++S.vapMissing;
                else { const long long lt = (long long)std::llround(l / S.tick), ht = (long long)std::llround(h / S.tick); v.b = turn::barRatios(S.rows, lt, ht);
                    const double c = (double)cl[i];   // (1.3.0) the close test of Absorption
                    if (std::isfinite(c)) { v.hlc = true; v.hiT = ht; v.loT = lt; v.clT = (long long)std::llround(c / S.tick); }
                    if (std::isfinite(c) && v.b.zone > 0) { const long long ct = (long long)std::llround(c / S.tick); v.awayUp = turn::absAway(ht, lt, ct, v.b.zone, true); v.awayDn = turn::absAway(ht, lt, ct, v.b.zone, false); } }
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
        if (!S.tabLoaded && !S.tabPath.empty()) {                       // (1.5.0) what this pane (or another chart of the market) decided before
            std::string t; long long sig = -1;
            if (readCapped(S.tabPath, 256 * 1024, t, sig)) { std::map<long long, turn::TabRec> f; if (sig >= 0 && turn::parseTabFile(t, S.market, S.spb, f)) for (const auto& kv : f) if (!S.tab.count(kv.first)) S.tab[kv.first] = kv.second; S.tabLoaded = true; }
        }
        if (!S.teqLoaded && !S.teqPath.empty()) {                       // (1.6.0)
            std::string t; long long sig = -1;
            if (readCapped(S.teqPath, 256 * 1024, t, sig)) { std::map<long long, std::pair<int, int> > f; if (sig >= 0 && turn::parseTeqFile(t, S.market, S.spb, f)) for (const auto& kv : f) if (!S.teq.count(kv.first)) S.teq[kv.first] = kv.second; S.teqLoaded = true; }
        }
        if (!S.crLoaded && !S.crPath.empty()) {                         // (1.7.0)
            std::string t; long long sig = -1;
            if (readCapped(S.crPath, 512 * 1024, t, sig)) { std::map<std::pair<long long, int>, turn::AbsCalc> f; std::map<std::pair<long long, int>, turn::IniCalc> fn; if (sig >= 0 && turn::parseCrec(t, S.market, S.spb, f, fn)) { for (const auto& kv : f) if (!S.crC.count(kv.first)) S.crC[kv.first] = kv.second; for (const auto& kv : fn) if (!S.niC.count(kv.first)) S.niC[kv.first] = kv.second; } S.crLoaded = true; }
        }
        if (S.crDirty && S.crLoaded) { if (!S.dirMade) { makeDir(ownDir()); S.dirMade = true; } if (publishFile(S.crPath, turn::crecText(S.crC, S.niC, S.market, S.spb))) S.crDirty = false; }
        if (!S.absRecLoaded && !S.absRecPath.empty()) {
            std::string t; long long sig = -1;
            if (readCapped(S.absRecPath, 512 * 1024, t, sig)) { std::map<long long, turn::AbsCalc> a; std::map<long long, turn::NextCalc> n;
                if (sig >= 0 && turn::parseAbsRec(t, S.market, S.spb, a, n)) { for (const auto& kv : a) if (!S.absC.count(kv.first)) S.absC[kv.first] = kv.second; for (const auto& kv : n) if (!S.absN.count(kv.first)) S.absN[kv.first] = kv.second; }
                S.absRecLoaded = true; }
        }
        if (S.absRecDirty && S.absRecLoaded) { if (!S.dirMade) { makeDir(ownDir()); S.dirMade = true; } if (publishFile(S.absRecPath, turn::absRecText(S.absC, S.absN, S.market, S.spb))) S.absRecDirty = false; }
        if (S.tabDirty && S.tabLoaded) { if (publishFile(S.tabPath, turn::tabFileText(S.tab, S.market, S.spb))) { ++S.tabWrites; S.tabDirty = false; } else ++S.tabFails; }
        if (fileSig(S.paramsPath) != S.paramsSig) {
            std::string t; long long sig = -1;
            if (readCapped(S.paramsPath, turn::PARAMS_MAX_BYTES, t, sig)) {
                S.P = turn::parseParams(t, S.market); S.paramsSig = sig;
                S.histAt.clear(); for (size_t k = 0; k < S.P.hist.size(); ++k) S.histAt[S.P.hist[k].stamp] = k;
            }
        }
        if (fileSig(S.showPath) != S.showSig) {                         // (1.2.0) the display file: missing / garbled = Effort only
            std::string t; long long sig = -1;
            if (readCapped(S.showPath, turn::SHOW_MAX_BYTES, t, sig)) {
                const turn::Show n = sig < 0 ? turn::Show() : turn::parseShow(t);
                if (n.stop != S.show.stop || n.exh != S.show.exh || n.ans != S.show.ans || n.read != S.show.read || n.effFrom10 != S.show.effFrom10) S.showChanged = true;
                S.show = n; S.showSig = sig;
            }
        }
        if (fileSig(S.wyPath) != S.wySig) {
            std::string t; long long sig = -1;
            if (readCapped(S.wyPath, turn::WY_MAX_BYTES, t, sig)) {
                S.wySig = sig; std::vector<turn::Sw> v; std::vector<turn::Level> lv; std::string why;
                if (sig < 0) { S.sweeps.clear(); S.levels.clear(); S.swAt.clear(); S.wyWhy = "no sweep file (lsRevRead on a " + std::to_string(S.spb) + "-s chart writes it)"; }
                else if (turn::parseWy(t, S.market, S.spb, v, lv, why)) {
                    S.sweeps.swap(v); S.levels.swap(lv); S.wyWhy = "ok"; S.abs = turn::parseAbs(t, S.market, S.spb);   // (1.5.0) + the Abs / Ex lines
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
        // (1.5.0) the Abs candles: P (the biggest single attacking order inside the absorbed quarter, biggest in P candles) and the Ex? (the
        // attackers' last minute 0.5x or less), decided once from a candle this pane saw whole; an Abs older than the kept candles = not known
        for (const turn::AbsLine& a : S.abs) {
            if (!S.tabLoaded) break;                                      // never decide before the file was read (it would hide earlier P's)
            if (a.t > S.lastClosedKey || S.tab.count(a.t)) continue;
            const turn::Sum* s = S.live.find(a.t); turn::TabRec r; r.dir = a.dir;
            if (s) {
                const std::map<long long, long long>::const_iterator cs = S.closedSeen.find(a.t);
                if (cs == S.closedSeen.end() || (now - cs->second < DECIDE_WAIT_MS && now >= cs->second)) continue;
                long long vol = -1; for (long i = count - 2; i >= 0 && i >= count - 40; --i) if ((long long)times[(int)i] == a.t) { vol = (long long)vols[(int)i]; break; }
                long long pct = -1;
                if (vol > 0 && covered(*s, vol, pct)) { const turn::AbsCalc c = turn::absCalc(S.live, a.t, a.dir); S.absC[a.t] = c; S.absRecDirty = true; if (c.feed) { r.p = c.p; r.plus = c.plus; r.exq = c.exq; S.absWhy = "decided"; } else S.absWhy = std::string("no feed: complete ") + (s->complete ? "1" : "0") + " thin " + (s->thin ? "1" : "0") + " overflow " + (s->overflow ? "1" : "0"); }
                else S.absWhy = "coverage " + std::to_string(pct) + "% vol " + std::to_string(vol);
            } else { if (!S.live.sums.empty() && a.t >= S.live.sums.front().key) continue;   // not closed in the feed yet
                S.absWhy = "no candle in the feed (sums " + std::to_string(S.live.sums.size()) + ")"; }
            if (!S.absC.count(a.t)) { S.absC[a.t] = turn::AbsCalc(); S.absRecDirty = true; }   // decided without the feed: the read says so, the same after a restart
            S.tab[a.t] = r; S.tabDirty = true;
#ifdef TURN_ABS_DEBUG
            std::fprintf(stderr, "ABSDBG decide t %lld p %d why %s sums %zu lastClosed %lld\n", a.t, r.p, S.absWhy.c_str(), S.live.sums.size(), S.lastClosedKey);
#endif
        }
        if (S.tabDirty && S.tabLoaded) { if (publishFile(S.tabPath, turn::tabFileText(S.tab, S.market, S.spb))) { ++S.tabWrites; S.tabDirty = false; } else ++S.tabFails; }   // written at once (lsRevReadMarks waits for it)
        // (1.6.0) every closed candle this pane read whole: its sellers' / buyers' Ex? (the Tst badge's E), decided once
        if (S.teqLoaded) for (const turn::Sum& x : S.live.sums) {
            if (x.key > S.lastClosedKey || S.teq.count(x.key) || !x.complete) continue;
            const std::map<long long, long long>::const_iterator cs = S.closedSeen.find(x.key);
            if (cs == S.closedSeen.end() || (now - cs->second < DECIDE_WAIT_MS && now >= cs->second)) continue;
            long long vol = -1; for (long i = count - 2; i >= 0 && i >= count - 40; --i) if ((long long)times[(int)i] == x.key) { vol = (long long)vols[(int)i]; break; }
            long long pct = -1; if (vol <= 0 || !covered(x, vol, pct)) continue;
            S.teq[x.key] = turn::exqOf(x); S.teqDirty = true;
            if (S.crLoaded) for (int dd : { 1, -1 }) { S.crC[std::make_pair(x.key, dd)] = turn::absCalc(S.live, x.key, dd); S.niC[std::make_pair(x.key, dd)] = turn::iniCalc(S.live, x.key, dd, S.spb); S.crDirty = true; } }   // (1.8.0) + the Initiative numbers   // (1.7.0) the compact read's numbers, both ends
        if (S.teqDirty && S.teqLoaded) { if (publishFile(S.teqPath, turn::teqFileText(S.teq, S.market, S.spb))) { ++S.teqWrites; S.teqDirty = false; } else ++S.teqFails; }
        // (1.5.0) the candle after each Abs: its attackers and book, decided once it closed
        for (const turn::AbsLine& a : S.abs) {
            if (!S.absRecLoaded || S.absN.count(a.t)) continue;
            long long kn = 0; for (long i = count - 2; i >= 1 && i >= count - 40; --i) if ((long long)times[(int)i - 1] == a.t) { kn = (long long)times[(int)i]; break; }
            if (kn == 0 || kn > S.lastClosedKey) { if (kn == 0 && !S.live.sums.empty() && a.t < S.live.sums.front().key) { S.absN[a.t] = turn::NextCalc(); S.absRecDirty = true; } continue; }
            const turn::Sum* s1 = S.live.find(kn);
            if (s1) { const std::map<long long, long long>::const_iterator cs = S.closedSeen.find(kn); if (cs == S.closedSeen.end() || (now - cs->second < DECIDE_WAIT_MS && now >= cs->second)) continue; }
            else if (!S.live.sums.empty() && kn >= S.live.sums.front().key) continue;
            S.absN[a.t] = turn::nextCalc(S.live, a.t, kn, a.dir); S.absRecDirty = true;
        }
        if (S.absRecDirty && S.absRecLoaded) { if (!S.dirMade) { makeDir(ownDir()); S.dirMade = true; } if (publishFile(S.absRecPath, turn::absRecText(S.absC, S.absN, S.market, S.spb))) S.absRecDirty = false; }
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
        return tone == turn::T_DIR ? (dir < 0 ? C_RED : C_GREEN) : tone == turn::T_BUY ? C_GREEN : tone == turn::T_SELL ? C_RED   // (1.1.0) buyers green, sellers red
             : tone == turn::T_AMBER ? C_AMBER : tone == turn::T_TEXT ? C_TEXT : (forFill ? C_AXIS : C_MUTED);
    }
    std::string prefixOf(const turn::Read& r, long long lastKey) { return prefixText(r.lv, r.stamp, r.src == 'N', lastKey); }
    // "<level> <time>" (the date too when the candle is not of the last candle's day); (1.0.1) also for a candle at a named level
    std::string prefixText(const std::string& lv, long long stamp, bool nightly, long long lastKey) {
        struct tm a, b; std::memset(&a, 0, sizeof a); std::memset(&b, 0, sizeof b);
        std::string p = lv;
        if (stamp > 0 && getLocaltime((RTDATE)stamp, &a)) {
            std::string d;
            if (lastKey > 0 && getLocaltime((RTDATE)lastKey, &b) && (a.tm_mday != b.tm_mday || a.tm_mon != b.tm_mon)) { char x[16]; std::snprintf(x, sizeof x, "%d-%02d ", (a.tm_mon + 1) % 100, a.tm_mday % 100); d = x; }
            const std::string c = turn::clockText(a.tm_hour, a.tm_min); if (!c.empty()) p += (p.empty() ? "" : " ") + d + c;
        }
        if (nightly) p += " (nightly)";
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
    // one side of one candle: the pushing side's bars (Stop Run left, Effort right) and, on a sweep candle, the turn's bars on the other side.
    // (1.0.1) wide = room for two ratio labels side by side: every bar is labelled, as in 1.0.0. Else, with offer: each half of the candle
    // (the bars going up, the bars going down) offers ONE label - its taller bar's ratio, centred on the candle - and turn::pickLabels
    // decides which are drawn. Returns the labels it drew itself (wide).
    struct LabText { std::string text; COLOR c; };
    // (1.1.0) the width of a ratio label in the 9 pt font, asked once per text and draw (every candle carries labels now). A scratch of ONE
    // draw: emptied at the start of every render, so it never carries anything from one chart to the next (the per-chart state is State).
    std::map<std::string, int> labW_;
    int labW(const std::string& t) { const std::map<std::string, int>::const_iterator it = labW_.find(t); if (it != labW_.end()) return it->second; const int w = (int)getTextWidth(t.c_str(), -1) + 2; if (labW_.size() < 512) labW_[t] = w; return w; }
    // (1.2.0) one = the pane shows Effort only: ONE bar a side, centred on the candle and wider (the two columns are for the other pieces)
    int drawSide(int owner, int x, int dir, const turn::BarSet& bs, int axis, int half, int bw, bool wide, bool offer, int padL, int padR,
                 std::vector<turn::BarBox>& boxes, std::vector<turn::LabCand>& cands, std::vector<LabText>& texts, bool one) {
        const int gap = one ? 0 : bw >= 12 ? 2 : bw >= 6 ? 1 : 0;
        const int w = one ? std::max(1, std::min(bw - (bw >= 12 ? 6 : bw >= 6 ? 2 : 1), 40)) : std::max(1, std::min((bw - gap - 2) / 2, 20));
        const int xl = one ? x - w / 2 : x - gap / 2 - w, xr = one ? x - w / 2 : x + (gap - gap / 2);
        const bool pushUp = dir < 0;                                    // a swept high: the buyers' bars go up, the turn's bars down
        const One all[4] = { { &bs.stop, xl, pushUp, pushUp ? C_LGREEN : C_LRED }, { &bs.eff, xr, pushUp, pushUp ? C_GREEN : C_RED },   // (1.4.2, Rassul 2026-11-24 11:21 CT "top should be green and bottom should be red") Absorption in the attackers' colour: the top bar (buyers at the offer) green, the bottom bar (sellers at the bid) red   // (1.3.0) was the winner's colour (a high red, a low green)
                             { &bs.exh, xl, !pushUp, pushUp ? C_LRED : C_LGREEN }, { &bs.ans, xr, !pushUp, pushUp ? C_RED : C_GREEN } };
        return drawOnes(owner, x, all, 4, w, axis, half, wide, offer, padL, padR, boxes, cands, texts);
    }
    struct One { const turn::Bar1* b; int left; bool up; COLOR c; };
    // (1.8.0) one candle's bars, each in its column (left / right of the candle centre) and half (up / down); the ratio labels as drawSide's
    int drawOnes(int owner, int x, const One* all, int nAll, int w, int axis, int half, bool wide, bool offer, int padL, int padR,
                 std::vector<turn::BarBox>& boxes, std::vector<turn::LabCand>& cands, std::vector<LabText>& texts) {
        int drawn = 0, bestH[2] = { 0, 0 }; const One* best[2] = { nullptr, nullptr };   // [0] the bars going up, [1] the bars going down
        for (int q = 0; q < nAll; ++q) { const One& o = all[q];
            if (!o.b->show) continue;
            const int h = turn::barPx(o.b->v, half); if (h <= 0) continue;
            const COLOR c = o.b->on ? o.c : C_AXIS;
            if (o.up) fill(o.left, axis - h, o.left + w, axis, c); else fill(o.left, axis + 1, o.left + w, axis + 1 + h, c);
            turn::BarBox bb; bb.owner = owner; bb.box.l = o.left; bb.box.r = o.left + w; bb.box.t = o.up ? axis - h : axis + 1; bb.box.b = o.up ? axis : axis + 1 + h; boxes.push_back(bb);
            if (o.b->label.empty()) continue;
            if (wide) {
                const int lw = labW(o.b->label); int l = o.left + w / 2 - lw / 2;
                if (o.left < x) { if (l + lw > x - 1) l = x - 1 - lw; } else if (l < x + 1) l = x + 1;   // the two labels of a candle never touch
                if (o.up) text(o.b->label, l, axis - h - 12, l + lw, axis - h - 1, c, true); else text(o.b->label, l, axis + 1 + h + 1, l + lw, axis + 1 + h + 12, c, true);
                ++drawn; continue;
            }
            if (!offer) continue;
            const int k = o.up ? 0 : 1;                                 // the left bar of a half comes first: equal heights keep it (turn::tallerOf)
            if (!best[k] || turn::tallerOf(bestH[k], h) == 1) { best[k] = &o; bestH[k] = h; }
        }
        for (int k = 0; k < 2; ++k) {
            if (!best[k]) continue;
            const std::string& t = best[k]->b->label; const int lw = labW(t);
            int l = x - lw / 2; if (l + lw > padR) l = padR - lw; if (l < padL) l = padL;
            turn::LabCand lc; lc.owner = owner; lc.h = bestH[k]; lc.box.l = l; lc.box.r = l + lw;
            if (k == 0) { lc.box.t = axis - bestH[k] - 12; lc.box.b = axis - bestH[k] - 1; } else { lc.box.t = axis + 1 + bestH[k] + 1; lc.box.b = axis + 1 + bestH[k] + 12; }
            cands.push_back(lc); LabText lt; lt.text = t; lt.c = best[k]->b->on ? best[k]->c : C_AXIS; texts.push_back(lt);
        }
        return drawn;
    }
    // (1.8.0) a plain candle (or one at a named level): Absorption in the LEFT column, Initiative in the RIGHT one, each up at the candle's high or
    // down at its low; a Stop Run of a named level takes a free column on its half (none free: it is in the strip only)
    int drawPair(int owner, int x, const turn::Bar1& ab, bool abUp, const turn::Bar1& in, bool inUp, const turn::Bar1& stUp, const turn::Bar1& stDn,
                 int axis, int half, int bw, bool wide, bool offer, int padL, int padR, std::vector<turn::BarBox>& boxes, std::vector<turn::LabCand>& cands, std::vector<LabText>& texts) {
        const int gap = bw >= 12 ? 2 : bw >= 6 ? 1 : 0;
        const int w = std::max(1, std::min((bw - gap - 2) / 2, 20));
        const int xl = x - gap / 2 - w, xr = x + (gap - gap / 2);
        One all[4]; int n = 0;
        if (ab.show) all[n++] = One{ &ab, xl, abUp, abUp ? C_GREEN : C_RED };   // the attackers' colour: buyers at the high green, sellers at the low red
        if (in.show) all[n++] = One{ &in, xr, inUp, inUp ? C_RED : C_GREEN };   // the counter side's colour: sellers at the high red, buyers at the low green
        for (int k = 0; k < 2; ++k) { const turn::Bar1& st = k == 0 ? stUp : stDn; const bool up = k == 0; if (!st.show) continue;
            const bool leftBusy = ab.show && abUp == up, rightBusy = in.show && inUp == up;
            if (!leftBusy) all[n++] = One{ &st, xl, up, up ? C_LGREEN : C_LRED }; else if (!rightBusy) all[n++] = One{ &st, xr, up, up ? C_LGREEN : C_LRED }; }
        return drawOnes(owner, x, all, n, w, axis, half, wide, offer, padL, padR, boxes, cands, texts);
    }
    void render(State& S) {
        RCT pane; pane.getPaneRect(false);
        const int L = pane.left, R = pane.right, T = pane.top, B = pane.bottom;
        S.cur.pane.l = L; S.cur.pane.t = T; S.cur.pane.r = R; S.cur.pane.b = B;   // (1.0.1) where the pane is: is the mouse inside, has it left
        S.readVerdict = 0; S.readLines = 0; S.readHead.clear(); S.readWhy.clear(); S.readWatch.clear();
        S.stripKind = 0; S.labMode = 0; S.labOffered = 0; S.labDrawn = 0; S.effCandles = 0; S.iniCandles = 0; S.iniOn = 0; labW_.clear();
        if (B - T < 40 || R - L < 40) return;
        FONT f10; f10.id = HELVETICA; f10.size = 10; f10.style = PLAIN; FONT f10b = f10; f10b.style = BOLD;
        FONT f9b = f10b; f9b.size = 9; FONT f9 = f10; f9.size = 9;                                   // (1.0.1) the ratios on the bars
        // nothing to read: say why in one grey line (the wrong kind of chart, or lsRevRead's sweep file is missing), never an empty pane.
        // (1.0.1) on the pane's top row, and only when no strip is drawn there
        const std::string why = !S.note.empty() ? S.note : (S.sweeps.empty() && S.wyWhy != "ok") ? S.wyWhy : std::string();
        const bool whyRoom = !why.empty() && R - L >= 260;
        const auto sayWhy = [&]() { if (whyRoom) { setFont(f10); text("Turn: " + why, L + 8, T + 4, R - 8, T + 19, C_MUTED, false); } };
        if (!S.note.empty()) { sayWhy(); return; }
        const long count = getBarCount(); if (count < 2) { sayWhy(); return; }
        RTARRAYI times(barDateTime); if (times.count < count) { sayWhy(); return; }
        int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) { sayWhy(); return; } v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1); if (v1 < v0) { sayWhy(); return; }
        const int bw = std::max(1, (int)getPixelsPerBar());
        int xLast = R; { PNT p; p.set(v1, 0.0f, kBarCenter); xLast = std::min(R - 2, (int)p.h + bw / 2 + 2); }
        setFont(f9b);
        const turn::Show& W = S.show; const bool one = !W.twoCol();          // (1.2.0) Effort only: one centred bar a side, one number a side
        const bool wide = !one && bw >= 2 * ((int)getTextWidth("9.9", -1) + 2) + 6;   // room for two ratio labels side by side on one candle
        // the strip, (1.0.1) on the pane's top edge: the candle under the mouse while the mouse is in this pane, else the newest sweep at or
        // before the last candle in view. A sweep candle: its read. A candle at a named level: what is drawn on it. No bars on the candle:
        // the same strip as with no mouse (the strip never comes and goes while the mouse moves, so the bars never jump).
        const int sTop = T + 2, sBot = sTop + 18; int areaTop = T + 2;
        const long long viewKey = (long long)times[v1];
        const long long atKey = std::min(viewKey, S.lastClosedKey);
        // (1.3.0) Absorption of chart bar i at its top / bottom, from the bars calc read (no bar is read here): the heaviest price against
        // its average over the 20 candles before. A candle whose earlier candles are not read yet (or are older than the kept window)
        // gives none, so a number never changes once shown.
        const auto absAt = [&](int i, bool top) -> turn::Abs {
            turn::Abs a; if (i < 0 || i >= (int)count) return a;
            const std::map<long long, BarVal>::const_iterator bi = S.bars.find((long long)times[i]); if (bi == S.bars.end() || bi->second.b.levels < 4) return a;
            std::vector<long long> prior; prior.reserve(turn::ABS_AVG);
            for (int j = i - 1; j >= 0 && j >= i - turn::ABS_BACK && (int)prior.size() < turn::ABS_AVG; --j) {
                const std::map<long long, BarVal>::const_iterator bj = S.bars.find((long long)times[j]); if (bj == S.bars.end()) return a;
                if (bj->second.b.levels < 4) continue;
                prior.push_back(top ? bj->second.b.topMax : bj->second.b.botMax); }
            a.x1000 = turn::absX1000(top ? bi->second.b.topMax : bi->second.b.botMax, prior); a.away = top ? bi->second.awayUp : bi->second.awayDn; return a; };
        // (1.8.0) Initiative of chart bar i at its top (sellers) / bottom (buyers): the counter side's heaviest price against its 20-candle average,
        // with the attackers' heaviest price there and their own number (the read's Against line)
        const auto iniAt = [&](int i, bool top) -> turn::Ini {
            turn::Ini a; if (i < 0 || i >= (int)count) return a;
            const std::map<long long, BarVal>::const_iterator bi = S.bars.find((long long)times[i]); if (bi == S.bars.end() || bi->second.b.levels < 4) return a;
            std::vector<long long> prior; prior.reserve(turn::ABS_AVG);
            for (int j = i - 1; j >= 0 && j >= i - turn::ABS_BACK && (int)prior.size() < turn::ABS_AVG; --j) {
                const std::map<long long, BarVal>::const_iterator bj = S.bars.find((long long)times[j]); if (bj == S.bars.end()) return a;
                if (bj->second.b.levels < 4) continue;
                prior.push_back(top ? bj->second.b.topIni : bj->second.b.botIni); }
            const turn::Bars& b = bi->second.b;
            a.x1000 = turn::absX1000(top ? b.topIni : b.botIni, prior); a.away = top ? bi->second.awayUp : bi->second.awayDn;
            a.lots = top ? b.topIni : b.botIni; a.att = top ? b.topMax : b.botMax; a.tick = top ? b.topIniTick : b.botIniTick;
            a.attX1000 = absAt(i, top).x1000; return a; };
        const auto indexOf = [&](long long key) -> int { int lo2 = 0, hi2 = (int)count - 1; while (lo2 <= hi2) { const int m = lo2 + (hi2 - lo2) / 2; const long long t = (long long)times[m]; if (t == key) return m; if (t < key) lo2 = m + 1; else hi2 = m - 1; } return -1; };
        // (1.4.0) THE ABSORPTION READ, right of the last candle (the other panes' read format: time + verdict in colour, why, Watch). As of
        // the last closed candle in view; from the bars calc read and the named levels only; nothing is recorded (a read, not a mark).
        {
            const int iEnd = indexOf(atKey);
            if (iEnd >= 0) {
                // (1.7.0) THE COMPACT READ (Rassul 17:28-17:30 CT): the candle under the mouse (closed), else the last closed candle in view - who won at
                // the end that was pushed, then Attack and Absorbed / Not absorbed (no Result, no Next); an Abs of lsRevRead on it names its level
                const int hv0 = (S.hov >= v0 && S.hov <= v1 && (long long)times[S.hov] <= S.lastClosedKey) ? S.hov : -1;
                const int ic = hv0 >= 0 ? hv0 : iEnd; const long long key = (long long)times[ic];
                const turn::Abs up = absAt(ic, true), dn = absAt(ic, false);
                const turn::Ini iUp = iniAt(ic, true), iDn = iniAt(ic, false); const int ie = turn::iniEnd(iUp, iDn);   // (1.8.0) an Initiative candle reads as Initiative
                const int dir = turn::pushedEnd(up.x1000, dn.x1000);
                const int x0 = xLast + 16, wAv = R - 6 - x0, rh = 13, rows = (B - T - 2) / rh;
                const auto tw = [&](const std::string& t) { return (int)getTextWidth(t.c_str(), -1); };
                S.absShown = 0; S.readLines = 0; turn::CText ct;
                const std::string when = prefixText(std::string(), key, false, 0);
                if (ie) {
                    const std::map<std::pair<long long, int>, turn::IniCalc>::const_iterator sn = S.niC.find(std::make_pair(key, ie));
                    const turn::IniCalc nc = sn != S.niC.end() ? sn->second : turn::iniCalc(S.live, key, ie, S.spb);   // decided numbers first (a restart draws the same read)
                    ct = turn::iniText(ie, ie > 0 ? iDn : iUp, nc, when, S.tick, S.market, btr::rthOf(key, S.spb));
                }
                else if (!dir) ct = turn::quietText(when, up.x1000, dn.x1000);
                else {
                    const turn::AbsLine* ab = nullptr; for (const turn::AbsLine& a : S.abs) if (a.t == key && a.dir == dir) { ab = &a; break; }
                    turn::AbsCalc c; const std::map<long long, turn::AbsCalc>::const_iterator sa = ab ? S.absC.find(key) : S.absC.end();
                    const std::map<std::pair<long long, int>, turn::AbsCalc>::const_iterator sc = S.crC.find(std::make_pair(key, dir));
                    if (sa != S.absC.end()) c = sa->second; else if (sc != S.crC.end()) c = sc->second; else c = turn::absCalc(S.live, key, dir);   // decided numbers first (a restart draws the same read)
                    const turn::Abs& e = dir > 0 ? dn : up;
                    ct = turn::compactText(dir, e.x1000, e.away, c, when, ab ? ab->lv : std::string(), ab ? ab->lvPx : 0, S.tick, S.market, btr::rthOf(key, S.spb));
                    if (ab) S.absShown = 1;
                }
                if (!ct.quiet) {   // (1.9.0) LAYER 6 - the Price line: did price agree with the read, over the next (up to 10) closed candles
                    const int pe = ie ? ie : dir; const bool hold = ie ? true : (pe > 0 ? dn.away : up.away);
                    const std::map<long long, BarVal>::const_iterator bc = S.bars.find(key);
                    if (pe && bc != S.bars.end() && bc->second.hlc) {
                        const long long ext = pe > 0 ? bc->second.loT : bc->second.hiT;
                        std::vector<turn::Nx> nx; for (int j = ic + 1; j < (int)count && (int)nx.size() < 10; ++j) { const long long kj = (long long)times[j]; if (kj > S.lastClosedKey) break;
                            const std::map<long long, BarVal>::const_iterator bj = S.bars.find(kj); if (bj == S.bars.end() || !bj->second.hlc) break;
                            turn::Nx q; q.hi = bj->second.hiT; q.lo = bj->second.loT; q.when = prefixText(std::string(), kj, false, 0); nx.push_back(q); }
                        std::vector<std::vector<long long> > hc; for (int j = std::max(0, ic - 40); j < ic; ++j) { const std::map<long long, BarVal>::const_iterator bj = S.bars.find((long long)times[j]);
                            if (bj == S.bars.end() || !bj->second.hlc) { hc.clear(); continue; } hc.push_back(std::vector<long long>{ bj->second.hiT, bj->second.loT, bj->second.clT }); }
                        ct.rows.push_back(std::make_pair(std::string("Price"), turn::agreeText(pe, hold, ext, nx, turn::atrTicks(hc), S.tick))); } }
                S.readVerdict = ct.quiet ? 0 : ct.win; S.readHead = ct.title; S.readWhy.clear(); for (const auto& rw : ct.rows) S.readWhy += rw.first + ": " + rw.second + " "; S.readWatch.clear();
                if (wAv >= 150 && rows >= 1) {
                    int y = T + 1, used = 0; setFont(f9b);
                    const std::vector<std::string> hl = turn::wrapRead(ct.title, wAv, 2, tw);
                    for (size_t q = 0; q < hl.size() && used < rows; ++q, ++used, y += rh) text(hl[q], x0, y, R - 2, y + rh, ct.quiet ? C_MUTED : ct.win > 0 ? C_GREEN : C_RED, false);
                    for (const auto& rw : ct.rows) {
                        if (used >= rows) break;
                        const std::vector<std::string> wl = turn::wrapRead(rw.first + ": " + rw.second, wAv, std::max(1, std::min(2, rows - used)), tw);
                        for (size_t q = 0; q < wl.size() && used < rows; ++q, ++used, y += rh) {
                            if (q == 0) { setFont(f9b); const std::string lab = rw.first + ":"; text(lab, x0, y, x0 + tw(lab) + 2, y + rh, C_TEXT, false);
                                const std::string rest = wl[q].size() > lab.size() + 1 ? wl[q].substr(lab.size() + 1) : std::string(); const int lw = tw(lab + " "); setFont(f9); text(rest, x0 + lw, y, R - 2, y + rh, C_TEXT, false); }
                            else { setFont(f9); text(wl[q], x0, y, R - 2, y + rh, C_TEXT, false); } }
                    }
                    S.readLines = used;
                }
            }
        }
        int kind = 0; turn::Read rd; long long cStop = -1; turn::Abs cAbs, aUp, aDn, rAbs; int cSide = 0; std::string cPrefix;
        const int hv = (S.hov >= v0 && S.hov <= v1) ? S.hov : -1;
        // the two Absorptions of a closed candle, as its bars say them (x1000 -1 = no bar)
        const auto effortsOf = [&](long long key) { const int i = indexOf(key); aUp = absAt(i, true); aDn = absAt(i, false); };
        if (!W.read) {
            // (1.2.0) no read strip: the top line is about Effort only - the closed candle under the mouse, else the last closed candle in
            // view. Always there (a dash where a side has no bar), so the bars never jump while the mouse moves.
            const long long key = (hv >= 0 && (long long)times[hv] <= S.lastClosedKey) ? (long long)times[hv] : atKey;
            if (key > 0 && key <= S.lastClosedKey) { effortsOf(key); cPrefix = prefixText(std::string(), key, false, viewKey); kind = 3; }
        }
        else if (hv >= 0) {
            const long long key = (long long)times[hv];
            const std::map<long long, size_t>::const_iterator si = S.swAt.find(key);
            const std::map<long long, turn::Cand>::const_iterator ci = S.rec.cands.find(key);
            const bool isSw = si != S.swAt.end() && si->second < S.sweeps.size() && key <= S.lastClosedKey;
            const bool above = S.hovY < S.axisY;
            // a sweep with a read: its strip. A sweep with no read and no level record: its strip (NO READ) while the mouse is on the half
            // its push is drawn on, as 1.0.1; (1.1.0) on the other half it is a plain candle
            if (isSw) { const turn::Sw& w = S.sweeps[si->second]; rd = readFor(S, w); if (rd.src != '-' || (ci == S.rec.cands.end() && (w.dir < 0) == above)) kind = 1; }
            // (1.1.0) a closed candle with no level - or the half of a level candle that has none: its two Effort bars
            const bool plain = !kind && key <= S.lastClosedKey && (ci == S.rec.cands.end() || turn::candPlainSide(ci->second, above));
            if (plain) {
                effortsOf(key);
                if (aUp.x1000 >= 0 || aDn.x1000 >= 0) { cPrefix = prefixText(std::string(), key, false, viewKey); kind = 3; }
            }
            if (!kind && ci != S.rec.cands.end()) {
                const turn::Cand& c = ci->second; const int side = turn::candSide(c, S.hovY < S.axisY);
                if (side) {
                    cStop = side < 0 ? c.hiX1000 : c.loX1000;
                    cAbs = absAt(hv, side < 0); cSide = side;
                    cPrefix = prefixText(turn::levelNameAt(S.levels, side < 0 ? c.hiL : c.loL), key, false, viewKey);
                    kind = 2;
                }
            }
        }
        if (!kind && W.read) {
            // (1.4.1, Rassul 2026-11-23 22:40 "fix everything"; the HG pane showed "1hL 1:03 PM ... NO READ" all evening) the newest sweep
            // of THIS Globex session (17:00 CT on) at or before the last candle in view; none yet = the last closed candle's Absorption
            const turn::Sw* sw = nullptr; for (size_t k = S.sweeps.size(); k-- > 0;) if (S.sweeps[k].stamp <= atKey) { sw = &S.sweeps[k]; break; }
            if (sw && turn::sessionOf(sw->stamp) == turn::sessionOf(atKey)) { rd = readFor(S, *sw); kind = 1; }
            else if (atKey > 0 && atKey <= S.lastClosedKey) { effortsOf(atKey); if (aUp.x1000 >= 0 || aDn.x1000 >= 0) { cPrefix = prefixText(std::string(), atKey, false, viewKey); kind = 3; } }
        }
        bool stripDrawn = false;
        if (kind && xLast - L >= 220 && B - T >= 90) {
            const int l = L + 4, r = xLast, dir = kind == 1 ? rd.dir : 0;
            const std::string px = kind == 1 ? prefixOf(rd, viewKey) : cPrefix;
            turn::Ini rIni; if (kind == 1) { rAbs = absAt(indexOf(rd.stamp), rd.dir < 0); rIni = iniAt(indexOf(rd.stamp), rd.dir < 0); }   // (1.8.0) + the sweep end's Initiative   // (1.3.0) the sweep's Absorption: at the end it pushed into
            const auto make = [&](bool numbers, bool withPrefix) {
                return kind == 1 ? turn::stripOf(rd, S.P.bestProven, numbers, withPrefix ? px : std::string(), rAbs, W.effFrom10, W.ans ? &rIni : nullptr)
                     : kind == 3 ? turn::stripOfAbs(aUp, aDn, numbers, withPrefix ? px : std::string(), W.effFrom10)
                     : turn::stripOfCand(cStop, cAbs, cSide, numbers, withPrefix ? px : std::string(), W.effFrom10); };
            turn::Strip st = make(!wide, true);                           // the ratios in words where not every bar carries its own
            setFont(f10b); const int pw = W.read ? (int)getTextWidth(st.pill.c_str(), -1) + 14 : 0;   // (1.2.0) the odds pill belongs to the read strip
            const int tx0 = l + 10, avail = r - pw - 14 - tx0;
            if (stripWidth(st, f10, f10b) > avail) st = make(false, true);    // too long: without the numbers,
            if (stripWidth(st, f10, f10b) > avail) st = make(false, false);   // then without the level and time,
            size_t first = 0; if (kind != 3 && stripWidth(st, f10, f10b) > avail && !st.segs.empty()) first = st.segs.size() - 1;   // then the verdict alone ((1.1.0) a plain candle: what fits, from the buyers on)
            fill(l, sTop, r, sBot, C_STRIP);
            fill(l, sTop, l + 4, sBot, toneColor(st.edgeTone, dir, true));
            setFont(f10b);
            if (W.read) { fill(r - pw - 4, sTop + 2, r - 4, sBot - 2, toneColor(st.pillTone, dir, true)); text(st.pill, r - pw - 4, sTop + 2, r - 4, sBot - 2, C_BLACK, true); }
            int tx = tx0;
            for (size_t k = first; k < st.segs.size(); ++k) {
                const turn::Seg& g = st.segs[k]; setFont(g.bold ? f10b : f10);
                std::string core; int lead = 0; const int w = segWidth(g.text, core, lead); if (tx + w > r - pw - 8) break;
                if (!core.empty()) text(core, tx + lead, sTop + 2, tx + lead + (int)getTextWidth(core.c_str(), -1), sBot - 1, toneColor(g.tone, dir, false), false);
                tx += w;
            }
            areaTop = sBot + 2; stripDrawn = true; S.stripKind = kind;
        }
        if (!stripDrawn && whyRoom) { sayWhy(); areaTop = T + 21; }
        // the bars: one middle axis; a swept high's push goes up and its turn down, a swept low's the other way. (1.0.1) a row for the
        // ratios above and below at every zoom; a pane too low for that keeps the bars alone
        bool lab = true; int top = areaTop + 13, bot = B - 13;
        if (bot - top < 40) { lab = false; top = areaTop + 2; bot = B - 2; }
        if (bot - top < 20) return;
        const int axis = (top + bot) / 2, half = std::max(4, (bot - top) / 2 - 1);
        S.axisY = axis;
        fill(L, axis, xLast, axis + 1, C_AXIS);
        setFont(f9b);
        std::vector<turn::BarBox> boxes; std::vector<turn::LabCand> cands; std::vector<LabText> texts; int owner = 0, wideDrawn = 0;
        // (1.1.0) EVERY closed candle carries its two Effort bars (buyers up, sellers down). (1.2.0) That is all the pane draws unless the
        // display file switches a piece on: then two columns a candle - the Stop Run of a candle at a named level (left), a sweep candle's
        // Exhaustion (left) and Answer (right) on its turn half - each only when its piece is on.
        const bool wd = wide && lab, of = lab && !wide;
        for (int i = v0; i <= v1; ++i) {
            const long long key = (long long)times[i];
            if (key > S.lastClosedKey) continue;                        // the forming candle: nothing (its Effort is not final)
            const std::map<long long, size_t>::const_iterator si = one ? S.swAt.end() : S.swAt.find(key);
            const std::map<long long, turn::Cand>::const_iterator ci = W.stop ? S.rec.cands.find(key) : S.rec.cands.end();
            const bool isSw = si != S.swAt.end() && si->second < S.sweeps.size();
            const std::map<long long, BarVal>::const_iterator bi = S.bars.find(key);
            if (!isSw && ci == S.rec.cands.end() && bi == S.bars.end()) continue;
            PNT p; p.set(i, 0.0f, kBarCenter); const int x = p.h; if (x < L || x > R) continue;
            turn::EffPair e; e.up = turn::absBar(absAt(i, true), W.effFrom10); e.dn = turn::absBar(absAt(i, false), W.effFrom10);   // (1.3.0) Absorption at the candle's high / low
            if (isSw) {
                const turn::Sw& w = S.sweeps[si->second]; const turn::Read sr = readFor(S, w);
                if (sr.src != '-') {                                    // a sweep candle with its read: the push on its side, the turn on the other
                    turn::BarSet bs = turn::barsOf(sr); bs.eff = w.dir < 0 ? e.up : e.dn;   // (1.3.0) the Absorption at the end the sweep pushed into
                    if (!W.stop) bs.stop = turn::Bar1();
                    if (!W.exh) bs.exh = turn::Bar1();
                    if (!W.ans) bs.ans = turn::Bar1(); else bs.ans = turn::iniBar(iniAt(i, w.dir < 0));   // (1.8.0) the new Initiative at the end it pushed into (the old Answer 0-3 retired)
                    wideDrawn += drawSide(owner++, x, w.dir, bs, axis, half, bw, wd, of, L + 1, R - 1, boxes, cands, texts, false);
                    if (!W.ans) { turn::BarSet o; o.eff = w.dir < 0 ? e.dn : e.up;      // the Answer is off: its column holds the other side's Effort
                        if (o.eff.show) wideDrawn += drawSide(owner++, x, -w.dir, o, axis, half, bw, wd, of, L + 1, R - 1, boxes, cands, texts, false); }
                    continue;
                }
            }
            turn::Bar1 stU, stD;
            if (ci != S.rec.cands.end()) { const turn::Cand& c = ci->second;   // a candle at a named level: + the Stop Run of the side(s) it traded at
                if (c.hiL) stU = turn::ratioBar(c.hiX1000, 50);
                if (c.loL) stD = turn::ratioBar(c.loX1000, 50); }
            // (1.8.0) TWO BARS A CANDLE: Absorption at its stronger end (left), Initiative at its stronger end (right)
            const turn::Abs aU = absAt(i, true), aD = absAt(i, false); const turn::Ini nU = iniAt(i, true), nD = iniAt(i, false);
            const bool abUp = aU.x1000 >= 0 && aU.x1000 >= aD.x1000, inUp = nU.x1000 >= 0 && nU.x1000 > nD.x1000;   // ties: Absorption at the top, Initiative at the bottom
            const turn::Bar1 ab = abUp ? e.up : e.dn, in = turn::iniBar(inUp ? nU : nD);
            if (e.up.show || e.dn.show) ++S.effCandles;
            if (in.show) ++S.iniCandles;
            if (in.on) ++S.iniOn;
            if (ab.show || in.show || stU.show || stD.show) wideDrawn += drawPair(owner++, x, ab, abUp, in, inUp, stU, stD, axis, half, bw, wd, of, L + 1, R - 1, boxes, cands, texts);
        }
        const std::vector<size_t> keep = turn::pickLabels(cands, boxes);
        for (size_t q = 0; q < keep.size(); ++q) { const size_t k = keep[q]; text(texts[k].text, cands[k].box.l, cands[k].box.t, cands[k].box.r, cands[k].box.b, texts[k].c, true); }
        S.labMode = !lab ? 0 : wide ? 2 : 1; S.labOffered = wide ? wideDrawn : (int)cands.size(); S.labDrawn = wide ? wideDrawn : (int)keep.size();
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
          << "|orders " << S.live.orders() << "|order resets " << S.live.n.shownResets << "|clock " << S.live.clockPoints() << "|sums " << S.live.sums.size()
          << "|id strays " << S.live.idStrays() << "|id lanes " << S.live.idLanes() << "|id set-backs " << S.live.idSetBacks() << "\n";   // (1.2.1)
        o << "PARAMS|" << S.P.why << "|stop " << S.P.stopTyp << "|hard " << S.P.hardCut << "|big " << S.P.bigCut << "|cuts " << S.P.q1 << " " << S.P.q2 << "|ranks " << S.P.rkLate.size() << " " << S.P.rkDecay.size()
          << " " << S.P.rkAfter.size() << "|odds " << S.P.odds[1] << " " << S.P.odds[2] << " " << S.P.odds[3] << (S.P.bestProven ? " proven" : "") << "|history " << S.P.hist.size() << "\n";
        o << "SWEEPS|" << S.wyWhy << "|" << S.sweeps.size() << "|levels " << S.levels.size() << "\n";
        o << "COVER|last " << S.lastCoverPct << "%|low (not decided) " << S.lowCover << "|other contract " << S.wrongContract << "|future events " << S.live.n.future << "\n";
        o << "RECORD|reads " << S.rec.reads.size() << "|level candles " << S.rec.cands.size() << "|writes " << S.recWrites << "|fails " << S.recFails << "|decided live " << S.decidedLive << "|candles live " << S.candsLive << "\n";
        o << "BARS|" << S.bars.size() << "|vap missing " << S.vapMissing << (S.vapFault ? "|VAP FAULT" : "") << "\n";
        if (!S.lastRead.empty()) o << "LAST|T|" << S.lastRead << "\n";
        o << "DISPLAY|" << TURN_VERSION << "|strip on the pane top: " << (S.stripKind == 1 ? "a sweep" : S.stripKind == 2 ? "a candle at a level" : S.stripKind == 3 ? "a candle" : "none")
          << "|ratios " << (S.labMode == 2 ? "on every bar (wide candles)" : S.labMode == 1 ? "one per candle side" : "none") << " " << S.labDrawn << " of " << S.labOffered
          << "|mouse " << (S.hov >= 0 ? "on a candle of this pane" : "not in this pane") << "|mouse moves " << S.mouseMoves << "|effort candles " << S.effCandles << "|initiative candles " << S.iniCandles << " coloured " << S.iniOn << "\n";
        o << "TEQ|" << S.teq.size() << " candles with their Ex? (turnexq file)|writes " << S.teqWrites << " fails " << S.teqFails << "\n";   // (1.6.0)
        o << "ABS|" << S.abs.size() << " Abs lines in the wy file|" << S.tab.size() << " decided (P / Ex?)|writes " << S.tabWrites << " fails " << S.tabFails << "|read shows an Abs " << S.absShown << "|" << S.absWhy << "\n";   // (1.5.0)
        o << "READ|" << (S.readVerdict == 1 ? "bullish" : S.readVerdict == -1 ? "bearish" : S.readVerdict == 2 ? "range" : "quiet") << "|" << S.readHead << "|" << S.readWhy << "|" << S.readWatch << "|lines drawn " << S.readLines << "\n";   // (1.4.0) the Absorption read right of the last candle
        o << "SHOW|" << S.show.why << "|" << turn::showText(S.show) << "\n";   // (1.2.0) what the pane draws: Effort + the pieces the display file switched on
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
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);   // its own pane, draws itself, IRT's volume at price; (1.0.1) the mouse picks the candle the strip is about
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA Turn v1.8 - two bars a candle: Absorption (left) and Initiative (right), each up at the candle's high or down at its low. Absorption = the attackers' heaviest price in that end quarter vs the 20 candles before (buyers at the high green, sellers at the low red, from 2.0 when the candle closed away from that end). Initiative = the counter side's heaviest price there (sellers at the high red, buyers at the low green, from 1.5 when bigger than the attackers there and closed away). Read: the candle under the mouse.");
    p->setVersion("1.9.0");
    return p;
}
