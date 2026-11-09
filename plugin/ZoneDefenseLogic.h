// ZoneDefenseLogic.h  --  lsZoneDefense 1.0.7 (2026-11-09) - the SDK-free Zone Defense engine (one copy: the plugin, the zdrun CLI used
// by the jumpstart / nightly (lra.zone_defense) and the mock-SDK tests all compile THIS file, so the research and the chart agree).
//
// Rassul 2026-11-08 "Zone Defense is good. lets build it" (design: claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G).
// Input: the market-by-order feed (order ids): ADD / MOD / DEL / FILL (+ CLEAR). A FILL of a resting BID = a sell aggressor hit it.
// What it decides (each decided ONCE, at decision time, never changed afterwards):
//   ZONE  (at a minute close)  the densest 4-tick band of resting orders aged >= 2 min, 2..40 ticks behind the last trade, >= densX x the
//                               median band and >= minLots; bids below price (support), offers above (resistance). One active zone
//                               per side at a time near a price (a new band overlapping the active one only refreshes it).
//   TOUCH (first trade into it, within 60 min)   + QUEUE: the share of the orders that had rested >= 30 s at the zone (or 1 tick
//                               behind it) cancelled in the 60 s before the touch vs its normal (the same filter in the 2..16 earlier
//                               60-s windows) - "pulling" = >= pullX x normal and >= normal + 10 pts (zone study fix).
//   RELOAD (touch + 60 s)       lots sold into the zone vs what was shown at the touch, the refills (an add at a price <= 250 ms after a
//                               fill there), what is left, and whether price broke >= 2 ticks through inside the 60 s:
//                               broke = "Bid exhausted" (bearish), reload >= reloadX and held = "Buyers holding" (bullish), else "Quiet".
//   SWEEP (first stop run per touch: the first burst of attacker fills beyond the zone within 15 min, decided 5 s after its end)
//                               lift = opposite lots in the 5 s after / run lots; swept = lift >= liftX AND the last trade back in or
//                               above the zone (green up-triangle), else weak (red). One per touch, never changes.
//   END  the zone ends: broken (a trade >= 2 ticks through), faded (resting < 50% of its peak), or 60 min with no touch.
// Truth (for the nightly only, never drawn): held9 / held30 after the RELOAD decision and after the SWEEP decision.
#pragma once
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "ZoneDefenseParams.h"

namespace zd {
static const char* const VERSION = "1.0.8";
enum Act { ADD = 0, MOD = 1, DEL = 2, FILL = 3, CLEAR = 9 };
enum Side { BID = 0, ASK = 1 };
static const long long MIN_MS = 60000;
// the LOCKED design values (approved design 2026-11-08; lra.consistency_check PARITY rows zd_* read these lines and lra.zone_defense)
static const int MIN_DIST = 2, MAX_DIST = 40, BAND = 4, MIN_AGE_MIN = 2;   // zone: ticks behind the last trade, band width, order age
static const double DENS_X = 1.5;                                          // the band vs the median band
static const int BROKE_TICKS = 2;                                          // broken = a trade 2+ ticks through
static const int RELOAD_SEC = 60;                                          // the reload window after the touch
static const double LIFT_X = 1.0;                                          // swept = lift >= 1.0x (and back in the zone)
static const int SWEEP_LIFT_S = 5;                                         // the lift window after the run
static const int SWEEP_BAR_S = 180;                                        // (1.0.2, GC 10-26 8:57 case) "back in the zone" is read at the CLOSE of the
                                                                           // run's 3-min candle (his reclaim read), not 5 s after the run
static const int QUEUE_REST_S = 30;                                        // queue: orders rested 30 s+ at the window start
static const double PULL_X = 2.0;                                          // pulling = cancels >= 2x normal and normal + 10 pts
// (1.0.2, review H2) ONE CLOCK for research and live: IRT stamps whole seconds and delivers in 250 ms batches, so every time the
// engine sees is a whole second (the host / zdrun floor to the second) and the sub-second rules are written on that clock:
static const long long RUN_GAP_MS = 1000;                                  // a stop run continues while the next attacker fill is in the same or next second
static const long long RUN_SPAN_MS = 2000;                                 // ... and within 2 s of its first fill
static const int RECLAIM_BARS = 10;                                        // (1.0.2, Rassul) a BROKEN zone closed back inside within 10 bars = broken -> reclaimed
static const int RECLAIM_WATCH = 20;                                       // the research records the bar count up to 20 (the nightly tests 5 / 10 / 20)
static const long long WARM_MS = 10 * 60000;
static const int ATR_BARS = 14;                                            // (1.0.2 levels) the 3-min ATR = mean range of the last 14 closed 3-min candles
static const int LVL_FLOOR_TICKS = 2;                                      // the level reach never below 2 ticks (LineLevelTag's floor)
static const int LVL_SHOW = 2;                                             // the label names the nearest 1-2 levels                               // (M2) after a cold start (empty book): no new zones for 10 min

struct In { long long t = 0; int act = ADD, side = -1; long long px = 0, sz = 0, oid = 0; };   // t = ms since the epoch (CT wall clock used for periods)

inline int periodOf(int ctMin, const zdp::Mkt& m) {            // minute of the CT day -> period index (zdp::P_*)
    const int o = m.openMin, c = m.closeMin, le = std::min(750, c - 30);
    if (ctMin >= 1020 || ctMin < 60) return zdp::P_ASIA;
    if (ctMin < o - 60) return zdp::P_LONDON;
    if (ctMin < o) return zdp::P_PREOPEN;
    if (ctMin < o + 30) return zdp::P_OPEN;
    if (ctMin < std::min(660, c - 30)) return zdp::P_MORNING;
    if (ctMin < std::min(le, c - 30)) return zdp::P_LUNCH;
    if (ctMin < c - 30) return zdp::P_AFTERNOON;
    if (ctMin < c) return zdp::P_CLOSE;
    return zdp::P_POST;
}

// (1.0.2, Rassul 2026-11-08 "zones at key levels, volume levels and MenthorQ CR0 / PS0") the levels a zone sits at. Only his list counts:
// key ONH ONL PDH PDL PFH PFL LonHI LonLO PhrH PhrL AsiaHI AsiaLO, volume VAH VAL DPoc WPoc, MenthorQ CR0 PS0 HVL (HVL0).
// AT a level = the zone overlaps it, or sits from the level out to levelReach x the 3-min ATR BEYOND it - below a support-type level
// (a low, VAL, PS0), above a resistance-type level (a high, VAH, CR0); a level with no side (DPoc, WPoc, HVL) reaches the zone's own
// side (bids below, offers above). His 2026-10-27 LineLevelTag rule; the reach is a per-market parameter (Mkt.levelReach, the nightly
// tries 0.5 / 1.0 / 1.5 ATR by hold rate).
enum LvlKind { LK_NONE = 0, LK_KEY = 1, LK_VOLUME = 2, LK_MQ = 3 };
struct Lvl { std::string name; long long px = 0; int kind = LK_NONE; int dir = 0; };   // px in ticks; dir +1 resistance / high, -1 support / low, 0 none
struct LTag { std::string name; long long px = 0; int kind = LK_NONE; long long gap = 0; };
// one token of a level name ("PFL/PDL" -> "PFL", "PDL"; "CR 0D" -> "CR0") -> his canonical name, kind and side; "" = not his list
inline std::string levelCanon(const std::string& tokIn, int& kind, int& dir) {
    std::string u; for (size_t i = 0; i < tokIn.size(); ++i) { const char c = tokIn[i]; if (c != ' ' && c != '_' && c != '-') u += (char)((c >= 'a' && c <= 'z') ? c - 32 : c); }
    struct C { const char* k; const char* name; int kind, dir; };
    static const C T[] = {
        {"ONH","ONH",LK_KEY,1},{"ONL","ONL",LK_KEY,-1},{"PDH","PDH",LK_KEY,1},{"PDL","PDL",LK_KEY,-1},{"PFH","PFH",LK_KEY,1},{"PFL","PFL",LK_KEY,-1},
        {"LONHI","LonHI",LK_KEY,1},{"LONLO","LonLO",LK_KEY,-1},{"LONH","LonHI",LK_KEY,1},{"LONL","LonLO",LK_KEY,-1},
        {"PHRH","PhrH",LK_KEY,1},{"PHRL","PhrL",LK_KEY,-1},{"ASIAHI","AsiaHI",LK_KEY,1},{"ASIALO","AsiaLO",LK_KEY,-1},{"ASIAH","AsiaHI",LK_KEY,1},{"ASIAL","AsiaLO",LK_KEY,-1},
        {"VAH","VAH",LK_VOLUME,1},{"VAL","VAL",LK_VOLUME,-1},{"DPOC","DPoc",LK_VOLUME,0},{"WPOC","WPoc",LK_VOLUME,0},
        {"CR0","CR0",LK_MQ,1},{"CR0D","CR0",LK_MQ,1},{"PS0","PS0",LK_MQ,-1},{"PS0D","PS0",LK_MQ,-1},{"HVL","HVL",LK_MQ,0},{"HVL0","HVL0",LK_MQ,0},{"HVL0D","HVL0",LK_MQ,0} };
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); ++i) if (u == T[i].k) { kind = T[i].kind; dir = T[i].dir; return T[i].name; }
    kind = LK_NONE; dir = 0; return std::string();
}
// a level name as the level files write it ("PFL/PDL", "LH+HVL", "CR 0D/GW 0D", "ONH \xC2\xB7 AsiaHI") -> his levels in it
inline std::vector<Lvl> levelsOf(const std::string& name, long long px) {
    std::vector<Lvl> out; std::string cur;
    auto flush = [&]() { int k = 0, d = 0; const std::string c = levelCanon(cur, k, d); if (!c.empty()) { bool dup = false; for (size_t i = 0; i < out.size(); ++i) dup = dup || out[i].name == c; if (!dup) { Lvl l; l.name = c; l.px = px; l.kind = k; l.dir = d; out.push_back(l); } } cur.clear(); };
    for (size_t i = 0; i < name.size(); ++i) {
        const unsigned char c = (unsigned char)name[i];
        if (c == '/' || c == '+' || c == '|' || c == ',' || c == 0xB7) { flush(); continue; }
        if (c == 0xC2 && i + 1 < name.size() && (unsigned char)name[i + 1] == 0xB7) { flush(); ++i; continue; }
        cur += name[i];
    }
    flush(); return out;
}
// how far the zone [lo, hi] of side sits BEYOND the level (ticks): 0 = overlaps; -1 = on the wrong side (never at it)
inline long long levelGap(long long lo, long long hi, int side, const Lvl& L) {
    if (lo <= L.px && L.px <= hi) return 0;
    const int dir = L.dir != 0 ? L.dir : (side == 0 ? -1 : 1);    // 0 = bid side
    if (dir < 0) return hi < L.px ? L.px - hi : -1;               // support: the zone below it
    return lo > L.px ? lo - L.px : -1;                            // resistance: the zone above it
}
inline const char* kindName(int k) { return k == LK_MQ ? "mq" : k == LK_VOLUME ? "volume" : k == LK_KEY ? "key" : "none"; }

struct Zone {
    int id = 0, side = BID, per = 0; long long T = 0, lo = 0, hi = 0, size = 0, peak = 0; double dens = 0, oldestMin = 0;
    // touch
    long long touchT = -1; long long atTouch = 0; double cancel60 = -1, cnorm = -1; int n60 = 0; bool pulling = false;
    // reload window
    long long sold = 0, refills = 0, left = 0, zeroT = -1, decT = -1; bool brokeEarly = false; long long brokeT = -1; int reload = -1;   // reload verdict: 0 exhausted, 1 quiet, 2 holding
    double reloadX = 0;
    // sweep
    bool through = false; bool runOpen = false; int sweep = -1; long long runT = -1, runEnd = -1, runLots = 0, runExt = 0, liftLots = 0, rebid = 0, sweepDecT = -1; double lift = 0; bool backIn = false;
    // end
    int endCode = 0; long long endT = -1;                        // 1 broken (confirmed), 2 faded, 3 expired / unresolved, 4 session end, 5 stopped
    // (1.0.2, Rassul's break rule = his BO rule) the zone's STATE, decided at 3-min candle closes (SWEEP_BAR_S grid), never repainted:
    //   1 testing   price traded through, or a candle CLOSED beyond the zone (below a bid zone / above an offer zone), not confirmed
    //   2 held      a candle closed back inside / beyond the defenders' side after a test without any close beyond
    //   3 reclaimed a candle closed back inside after a close beyond (before a follow-through)
    //   5 broken -> reclaimed: after 4, a candle closes back inside / beyond the defenders' side within RECLAIM_BARS (a failed break)
    //   4 broken    a close beyond AND a follow-through bar (bids: a red bar with a lower low and a lower close than the close-beyond
    //               bar; offers: a green bar with a higher high and a higher close), with no close back inside in between
    int state = 0; long long stateT = -1; long long cbClose = 0, cbExt = 0; bool closedBeyond = false; int candlesTesting = 0;
    int sHeld9b = -1; int reclaimBars = -1;                    // bars from the break to the first close back inside (research, up to RECLAIM_WATCH); 5 = broken -> reclaimed (state)                                            // truth after the state decision (held / reclaimed: 9 min with no trade 2+ ticks through; broken: no close back)
    // truth (nightly)
    int held9 = -1, held30 = -1, sHeld9 = -1;
    // the bars (per candle key): sold into the zone, resting left at the candle's end
    std::map<long long, long long> barSold, barLeft;
    // single big order (at most one line per zone): the largest resting order in the zone when it formed
    long long bigOid = 0, bigSz = 0, bigPx = 0;
    // (1.0.2 levels) the levels it sits at (nearest first, at most LVL_SHOW kept for the label) + the research record: the nearest of
    // his levels on its reach side (any distance up to 3 ATR) - its kind and gap in ATR, so the nightly can re-cut the reach (0.5/1/1.5)
    std::vector<LTag> tags; int nearKind = LK_NONE; double nearGapAtr = -1; std::string nearName; bool levelSearch = false; bool lowFloor = false;   // (1.0.3) found only by the at-level floor
    bool active() const { return endCode == 0; }
};

// one decision record line (the journal the overlay draws from and a restart reloads)
struct Line { long long t = 0; std::string text; };

struct Ended { long long tAdd, tEnd; bool cancel; };
struct Ord { int side; long long px, sz, tAdd; };
// (1.0.2) the same-distance queue normal: one window per minute close per side - the near-price orders resting 30 s+ then (n) and
// how many of them were cancelled within 60 s (c, counted when the window closes)
struct NearW { long long T; long long n[42], c[42]; std::vector<std::pair<long long, int> > ids; NearW() : T(0) { for (int i = 0; i < 42; ++i) n[i] = c[i] = 0; } };   // per distance 0..41 ticks
struct NearQ { std::deque<NearW> w; };

class Engine {
public:
    zdp::Mkt M; int ctOffsetMin = 0;                            // CT minutes = UTC minutes + ctOffsetMin (set by the host from the clock)
    std::vector<Zone> zones; std::vector<Line> out;             // all zones of the session, decision lines in order
    long long lastPx = 0, lastT = 0, nextMin = 0, events = 0, fills = 0, minutes = 0;
    size_t maxBook = 0, maxEnded = 0, maxLive = 0;              // (1.0.1) memory high-water marks (status line / tests)
    size_t bookSize() const { return book.size(); }
    // (1.0.2, live-vs-Databento check 2026-11-08) the live feed stamps whole seconds and drops ~13% of events:
    //  refillSameSecond: a re-add in the SAME second as the fill counts as a refill (live cannot see 250 ms; research = live meaning)
    //  ghostTicks: at each minute close, resting orders crossing the last trade by more than this many ticks (a bid above it / an offer
    //   below it) cannot exist - their DEL / FILL was missed - they are dropped (0 = off). Never fires on a complete feed (Databento).
#ifndef ZD_REFILL_SECOND
#define ZD_REFILL_SECOND 1
#endif
#ifndef ZD_GHOST_TICKS
#define ZD_GHOST_TICKS 2
#endif
    bool refillSameSecond = ZD_REFILL_SECOND != 0; int ghostTicks = ZD_GHOST_TICKS; long long ghosts = 0;
    long long warmUntil = -1;                                   // (M2) set at the first event of a COLD start (empty book); -1 = not yet
    long long implied = 0, modMoves = 0, resets = 0;            // counters (status / tests)
    int nextId = 1;
    long long barSec = 180;
    std::vector<Lvl> levels;                                    // (1.0.2) his levels now (ticks), set by the host / the research runner
    double atrTicks() const { if (ranges.empty()) return 0; double s = 0; for (size_t i = 0; i < ranges.size(); ++i) s += (double)ranges[i]; return s / ranges.size(); }
    long long reachTicks() const { return std::max((long long)LVL_FLOOR_TICKS, (long long)std::llround(M.levelReach * atrTicks())); }
    void setLevels(const std::vector<Lvl>& v) { levels = v; }
    long long minLotsOverride = 0;                              // research runs only (zdrun): one floor for every period                                     // the chart's candle (keys the pane bars)
    explicit Engine(const zdp::Mkt& m) : M(m) {}

    void feed(const In& e) {
        if (e.t < lastT) { /* out of order: keep time monotone */ } else lastT = e.t;
        const long long t = lastT; ++events;
        if (warmUntil < 0) warmUntil = book.empty() ? t + WARM_MS : t;   // a carried book (session roll) needs no warm-up
        if (nextMin == 0) nextMin = (t / MIN_MS + 1) * MIN_MS;
        while (t >= nextMin) { minuteClose(nextMin); nextMin += MIN_MS; }
        timers(t);
        switch (e.act) {
        case ADD: onAdd(e, t); break;
        case MOD: onMod(e, t); break;
        case DEL: onEnd(e.oid, t, true); break;
        case FILL: onFill(e, t); break;
        case CLEAR: book.clear(); break;
        default: break;
        }
    }
    // close what is open: code 4 = the session ended, 5 = the plugin stopped / restarted / the feed was reset (truth stays -1)
    void finish(long long t, int code = 4) {
        timers(t);
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].active()) endZone(zones[i], code, t);
    }
    // (M1) a reconnect / symbol change: the open zones are closed (5) and the book is dropped (the feed re-sends it); warm-up again
    void reset(long long t) { finish(t, 5); book.clear(); ended.clear(); lastFillAt.clear(); cancelledAt.clear(); nearQ[0] = NearQ(); nearQ[1] = NearQ(); warmUntil = -1; ++resets; }
    // (M2) the session roll keeps the book: the new engine takes the old one's resting orders (no warm-up)
    void carryBook(const Engine& o) { book = o.book; lastPx = o.lastPx; }
    int ctMinute(long long t) const { long long m = (t / MIN_MS + ctOffsetMin) % 1440; if (m < 0) m += 1440; return (int)m; }
    // (1.0.2, review L4) candleKey / barLines cut bars on the epoch: right only when the bar size is whole minutes and divides the hour
    // (the 17:00 CT session start and every bar start then sit on the same grid); 90-s, 45-min, 2-h bars are refused by the host
    static bool barSizeOk(long long spb) { return spb >= 60 && spb % 60 == 0 && 3600 % spb == 0; }
    // (1.0.2, review L6) America/Chicago offset (minutes, local - UTC) at UTC second u: CDT from the 2nd Sunday of March 08:00 UTC to
    // the 1st Sunday of November 07:00 UTC, else CST
    static int chicagoOffsetMin(long long u) {
        auto days = [](int y, int m, int d) { y -= m <= 2; const int era = (y >= 0 ? y : y - 399) / 400; const int yoe = y - era * 400;
            const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return (long long)era * 146097 + doe - 719468; };
        const long long dd = u / 86400; long long z = dd + 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const long long doe = z - era * 146097;
        const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = yoe + era * 400; const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const long long mp = (5 * doy + 2) / 153; const long long mth = mp + (mp < 10 ? 3 : -9); if (mth <= 2) ++y;
        auto nthSunday = [&](int m, int n) { const long long d1 = days((int)y, m, 1); const long long wd = ((d1 % 7) + 11) % 7; /* 0 = Sunday (1970-01-01 = Thu) */
            return d1 + (7 - wd) % 7 + 7 * (n - 1); };
        const long long on = nthSunday(3, 2) * 86400 + 8 * 3600, off = nthSunday(11, 1) * 86400 + 7 * 3600;
        return (u >= on && u < off) ? -300 : -360;
    }
    long long candleKey(long long t) const { const long long b = barSec * 1000; return (t / b + 1) * b / 1000; }   // bar END (s), as the chart

    // the most recent zone with a touch (what the pane's 4 lines read), -1 = none
    int readZone() const { for (size_t i = zones.size(); i-- > 0;) if (zones[i].touchT >= 0) return (int)i; return -1; }

private:
    std::unordered_map<long long, Ord> book;
    long long cK = -1, cO = 0, cH = 0, cL = 0, cC = 0;          // the forming 3-min candle (trades)
    std::deque<long long> ranges;                                // (1.0.2) the last ATR_BARS closed 3-min candle ranges (ticks)
    NearQ nearQ[2]; std::unordered_map<long long, long long> cancelledAt;   // oid -> cancel ms (kept 2 min, for the windows)
    std::vector<int> live;                                            // zones active or ended < 31 min ago (truth windows)
    std::map<std::pair<int, long long>, std::deque<Ended> > ended;    // (side, px) -> orders that left there (last 17 min)
    std::map<std::pair<int, long long>, long long> lastFillAt;        // (side, px) -> last fill ms (refills)
    // trades after each touch / sweep for the truth windows: kept compact as (t, px)
    std::deque<std::pair<long long, long long> > tape;                // last 31 min of trades (t, px)
    struct Burst { int side = -1; long long t0 = -1, tl = -1, lots = 0, ext = 0; };
    long long tradeAggrSide = -1;

    const zdp::Per& P(int per) const { return M.per[per]; }

    void onAdd(const In& e, long long t) {
        if (e.side < 0 || e.px <= 0) return;
        Ord o; o.side = e.side; o.px = e.px; o.sz = e.sz; o.tAdd = t; book[e.oid] = o;
        // refills: an add at a price that just traded on the same side
        std::map<std::pair<int, long long>, long long>::const_iterator f = lastFillAt.find(std::make_pair(e.side, e.px));
        const bool refill = f != lastFillAt.end() && (refillSameSecond ? t / 1000 == f->second / 1000 : t - f->second <= 250);   // (1.0.2) the same second (live clock)
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]]; if (!z.active()) { continue; }
            if (z.side != e.side) continue;
            if (z.touchT >= 0 && z.decT < 0 && e.px >= z.lo && e.px <= z.hi && refill) ++z.refills;
            if (z.runEnd >= 0 && z.sweep < 0 && t <= z.runEnd + SWEEP_LIFT_S * 1000 && (z.side == BID ? e.px >= z.runExt && e.px <= z.hi : e.px <= z.runExt && e.px >= z.lo)) z.rebid += e.sz;
        }
    }
    void onMod(const In& e, long long t) {
        std::unordered_map<long long, Ord>::iterator it = book.find(e.oid);
        if (it == book.end()) { In a = e; a.act = ADD; onAdd(a, t); return; }
        if (e.px > 0 && e.px != it->second.px) {                 // (M4) a new price = a cancel at the old price (Queue) + a new queue place
            if (t - it->second.tAdd > QUEUE_REST_S * 1000) { Ended x; x.tAdd = it->second.tAdd; x.tEnd = t; x.cancel = true; ended[std::make_pair(it->second.side, it->second.px)].push_back(x); }   // (1.0.2 memory) see onEnd
            ++modMoves;
            it->second.px = e.px; it->second.tAdd = t;
        }
        if (e.sz > 0) it->second.sz = e.sz;
    }
    void onEnd(long long oid, long long t, bool cancel) {
        std::unordered_map<long long, Ord>::iterator it = book.find(oid); if (it == book.end()) return;
        if (cancel) cancelledAt[oid] = t;
        const Ord& o = it->second;
        // (1.0.2 memory, NQ kept ~1,000,000) only an order that rested QUEUE_REST_S+ can ever count in the queue read (rested 30 s before
        // the 60-s window and still there in it), so the short-lived ones are not kept - the same reads, ~1/10 of the memory
        if (t - o.tAdd > QUEUE_REST_S * 1000) {
            std::deque<Ended>& d = ended[std::make_pair(o.side, o.px)];
            Ended x; x.tAdd = o.tAdd; x.tEnd = t; x.cancel = cancel; d.push_back(x);
            while (!d.empty() && t - d.front().tEnd > 17 * MIN_MS) d.pop_front();
        }
        book.erase(it);
    }
    void onFill(const In& e, long long t) {
        if (e.side < 0) { ++implied; return; }                  // (M3) an implied (spread-driven) fill: not counted, as the research
        std::unordered_map<long long, Ord>::iterator it = book.find(e.oid);
        int side = e.side; long long px = e.px;
        if (it != book.end()) { side = it->second.side; if (px <= 0) px = it->second.px; it->second.sz -= e.sz; if (it->second.sz <= 0) onEnd(e.oid, t, false); }
        if (side < 0 || px <= 0) return;
        ++fills; lastFillAt[std::make_pair(side, px)] = t;
        trade(t, px, e.sz, side == BID ? 1 : 0);                  // a resting bid filled = a SELL aggressor (1)
    }
    // the tape: one call per fill. sellAggr 1 = hit the bid
    void trade(long long t, long long px, long long lots, int sellAggr) {
        { const long long ck = t / (SWEEP_BAR_S * 1000LL); if (ck != cK) { cK = ck; cO = cH = cL = px; } cH = std::max(cH, px); cL = std::min(cL, px); cC = px; }
        lastPx = px; tape.push_back(std::make_pair(t, px)); while (!tape.empty() && t - tape.front().first > 31 * MIN_MS) tape.pop_front();
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]];
            if (!z.active()) { truth(z, t, px); continue; }
            const bool bid = z.side == BID; const bool attacker = bid ? sellAggr == 1 : sellAggr == 0;
            if (z.touchT < 0) {
                if (bid ? px <= z.hi : px >= z.lo) touch(z, t);
                else continue;
            }
            truth(z, t, px);
            // the reload window
            if (z.decT < 0) {
                if (attacker && px >= z.lo && px <= z.hi) { z.sold += lots; z.barSold[candleKey(t)] += lots; }
                if (!z.brokeEarly && (bid ? px <= z.lo - BROKE_TICKS : px >= z.hi + BROKE_TICKS)) { z.brokeEarly = true; z.brokeT = t; }
            } else if (attacker && px >= z.lo && px <= z.hi) z.barSold[candleKey(t)] += lots;
            // the sweep: the first attacker burst beyond the zone (within 15 min of the touch)
            if (z.sweep < 0) {
                const bool beyond = bid ? px < z.lo : px > z.hi;
                if (z.runT < 0) {
                    if (attacker && beyond && t - z.touchT <= 15 * MIN_MS) { z.runT = t; z.runEnd = t; z.runLots = lots; z.runExt = px; z.runOpen = true; }
                } else if (z.runOpen && attacker && t - z.runEnd <= RUN_GAP_MS && t - z.runT <= RUN_SPAN_MS) {
                    z.runEnd = t; z.runLots += lots; z.runExt = bid ? std::min(z.runExt, px) : std::max(z.runExt, px);
                } else {
                    z.runOpen = false;
                    if (!attacker && t <= z.runEnd + SWEEP_LIFT_S * 1000) z.liftLots += lots;
                }
            }
            // end: broken (after the reload and sweep decisions; before them the decisions read it)
            if (z.touchT >= 0 && z.state != 1 && (bid ? px < z.lo : px > z.hi)) z.through = true;   // traded through: testing at the candle close
        }
    }
    void touch(Zone& z, long long t) {
        z.touchT = t; z.atTouch = restingIn(z.side, z.lo, z.hi); z.left = z.atTouch;
        queueRead(z, t);
        char b[200]; std::snprintf(b, sizeof(b), "ZT,%d,%lld,%lld,%.3f,%.3f,%d,%d", z.id, t, z.atTouch, z.cancel60, z.cnorm, z.n60, z.pulling ? 1 : 0);
        Line l; l.t = t; l.text = b; out.push_back(l);
    }
    // queue at the touch: orders at the zone (or 1 tick behind) that had rested >= 30 s at the window start; cancelled share
    void queueRead(Zone& z, long long tt) {
        const long long qlo = z.side == BID ? z.lo - 1 : z.lo, qhi = z.side == BID ? z.hi : z.hi + 1;
        struct OL { long long tAdd, tEnd; bool c; };
        std::vector<OL> v;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it)
            if (it->second.side == z.side && it->second.px >= qlo && it->second.px <= qhi) { OL o = { it->second.tAdd, LLONG_MAX, false }; v.push_back(o); }
        for (long long p = qlo; p <= qhi; ++p) {
            std::map<std::pair<int, long long>, std::deque<Ended> >::const_iterator d = ended.find(std::make_pair(z.side, p)); if (d == ended.end()) continue;
            for (size_t k = 0; k < d->second.size(); ++k) { OL o = { d->second[k].tAdd, d->second[k].tEnd, d->second[k].cancel }; v.push_back(o); }
        }
        int n = 0, c = 0;
        for (size_t k = 0; k < v.size(); ++k) if (v[k].tAdd <= tt - 60000 - QUEUE_REST_S * 1000 && v[k].tEnd > tt - 60000) { ++n; if (v[k].tEnd <= tt && v[k].c) ++c; }
        z.n60 = n; z.cancel60 = n ? (double)c / n : -1;
        // (1.0.2, CoB-mockup finding: the zone's own earlier minutes gave a normal of ~0% at nearly every GC / HG touch, so "pulling" fired
        // on any cancel) the normal is the SAME-DISTANCE share: of the orders that rested 30 s+ at the same distance behind the last
        // trade (+- 2 ticks) as the zone was 60 s before the touch, the share cancelled within the next 60 s, pooled over the last 30
        // minutes of this session (one window per minute close); under 20 such orders, the period's jumpstart normal
        long long px0 = 0; for (size_t k = tape.size(); k-- > 0;) if (tape[k].first <= tt - 60000) { px0 = tape[k].second; break; }
        const long long d0 = px0 <= 0 ? 0 : (z.side == BID ? px0 - z.hi : z.lo - px0);
        const NearQ& nq = nearQ[z.side]; long long pn = 0, pc = 0;
        for (size_t k = 0; k < nq.w.size(); ++k) if (nq.w[k].ids.empty() && tt - nq.w[k].T <= 31 * MIN_MS)
            for (long long d = std::max(0LL, d0 - 2); d <= std::min(41LL, d0 + 2); ++d) { pn += nq.w[k].n[d]; pc += nq.w[k].c[d]; }
        z.cnorm = pn >= 20 ? (double)pc / pn : P(z.per).cancelNorm;
        z.pulling = z.n60 >= 3 && z.cancel60 >= 0 && z.cancel60 >= std::max(PULL_X * z.cnorm, z.cnorm + 0.10);
    }
    long long restingIn(int side, long long lo, long long hi) const {
        long long s = 0;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) if (it->second.side == side && it->second.px >= lo && it->second.px <= hi) s += it->second.sz;
        return s;
    }
    // time-driven decisions (run before each event and at the session end)
    void timers(long long t) {
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]];
            if (!z.active()) { if (z.endT >= 0) truthTime(z, t); continue; }
            if (z.touchT < 0) { if (t - z.T > 60 * MIN_MS) endZone(z, 3, z.T + 60 * MIN_MS); continue; }
            truthTime(z, t);
            if (z.decT < 0 && t >= z.touchT + RELOAD_SEC * 1000) decideReload(z, z.touchT + RELOAD_SEC * 1000);
            if (z.sweep < 0 && z.runT >= 0) {                       // lift over 5 s after the run; back-in at the run candle's close
                const long long dec = std::max(z.runEnd + SWEEP_LIFT_S * 1000, sweepClose(z.runT));
                if (t >= dec) decideSweep(z, dec);
            }
            if (z.sweep < 0 && z.runT < 0 && t - z.touchT > 15 * MIN_MS) sweepNone(z, z.touchT + 15 * MIN_MS);   // no run within 15 min: "Sweep: none"

        }
    }
    static long long sweepClose(long long runT) { const long long b = SWEEP_BAR_S * 1000LL; return (runT / b + 1) * b; }
    bool lastBeyond(const Zone& z) const { return z.side == BID ? lastPx <= z.lo - BROKE_TICKS : lastPx >= z.hi + BROKE_TICKS; }
    void decideReload(Zone& z, long long dt) {
        z.decT = dt; z.left = restingIn(z.side, z.lo, z.hi);
        z.reloadX = z.atTouch > 0 ? (double)z.sold / z.atTouch : (z.sold > 0 ? 99.0 : 0.0);
        z.reload = z.brokeEarly ? 0 : (z.reloadX >= P(z.per).reloadX ? 2 : 1);
        char b[240]; std::snprintf(b, sizeof(b), "ZR,%d,%lld,%d,%lld,%lld,%lld,%lld,%.3f,%lld,%lld", z.id, dt, z.reload, z.sold, z.refills, z.atTouch, z.left, z.reloadX, z.zeroT, z.brokeT);
        Line l; l.t = dt; l.text = b; out.push_back(l);
    }
    void decideSweep(Zone& z, long long dt) {
        z.sweepDecT = dt; z.lift = z.runLots > 0 ? (double)z.liftLots / z.runLots : 0;
        z.backIn = z.side == BID ? lastPx >= z.lo : lastPx <= z.hi;
        z.sweep = (z.lift >= LIFT_X && z.backIn) ? 2 : 1;
        char b[240]; std::snprintf(b, sizeof(b), "ZS,%d,%lld,%d,%lld,%lld,%lld,%.3f,%d,%lld,%lld", z.id, dt, z.sweep, z.runT, z.runLots, z.runExt, z.lift, z.backIn ? 1 : 0, z.rebid, z.liftLots);
        Line l; l.t = dt; l.text = b; out.push_back(l);
    }
    void sweepNone(Zone& z, long long dt) {
        z.sweep = 0; z.sweepDecT = -1;
        char b[120]; std::snprintf(b, sizeof(b), "ZS,%d,%lld,0,-1,0,0,0.000,0,0,0", z.id, dt);
        Line l; l.t = dt; l.text = b; out.push_back(l);
    }
    void endZone(Zone& z, int code, long long t) {
        if (!z.active()) return;
        if (z.touchT >= 0 && z.decT < 0) decideReload(z, std::min(t, z.touchT + RELOAD_SEC * 1000));
        if (z.runT >= 0 && z.sweep < 0) decideSweep(z, std::max(z.runEnd + SWEEP_LIFT_S * 1000, std::min(t, sweepClose(z.runT))));
        if (z.touchT >= 0 && z.sweep < 0) sweepNone(z, t);
        z.endCode = code; z.endT = t;
        char b[120]; std::snprintf(b, sizeof(b), "ZE,%d,%lld,%d,%lld", z.id, t, code, restingIn(z.side, z.lo, z.hi));
        Line l; l.t = t; l.text = b; out.push_back(l);
    }
    // truth after the reload decision (held9 / held30) and after the sweep decision (sHeld9); trade-driven breaks
    void truth(Zone& z, long long t, long long px) {
        const bool bid = z.side == BID; truthState(z, t, px);
        if (z.decT >= 0 && !z.brokeEarly) {
            const bool br = bid ? px <= z.lo - BROKE_TICKS : px >= z.hi + BROKE_TICKS;
            if (br && t > z.decT) { if (z.held9 < 0 && t <= z.decT + 9 * MIN_MS) z.held9 = 0; if (z.held30 < 0 && t <= z.decT + 30 * MIN_MS) z.held30 = 0; }
        }
        if (z.sweepDecT >= 0 && z.sHeld9 < 0 && t > z.sweepDecT && t <= z.sweepDecT + 9 * MIN_MS && (bid ? px <= z.runExt - BROKE_TICKS : px >= z.runExt + BROKE_TICKS)) z.sHeld9 = 0;
    }
    void truthState(Zone& z, long long t, long long px) {
        if (z.stateT < 0 || z.sHeld9b >= 0 || t <= z.stateT) return;
        const bool bid = z.side == BID;
        if (t > z.stateT + 9 * MIN_MS) { z.sHeld9b = 1; return; }
        if ((z.state == 2 || z.state == 3 || z.state == 5) && (bid ? px <= z.lo - BROKE_TICKS : px >= z.hi + BROKE_TICKS)) z.sHeld9b = 0;
        if (z.state == 4 && (bid ? px >= z.lo : px <= z.hi)) z.sHeld9b = 0;
    }
    void truthTime(Zone& z, long long t) {
        if (z.stateT >= 0 && z.sHeld9b < 0 && t > z.stateT + 9 * MIN_MS) z.sHeld9b = 1;
        if (z.decT >= 0 && !z.brokeEarly) { if (z.held9 < 0 && t > z.decT + 9 * MIN_MS) z.held9 = 1; if (z.held30 < 0 && t > z.decT + 30 * MIN_MS) z.held30 = 1; }
        if (z.sweepDecT >= 0 && z.sHeld9 < 0 && t > z.sweepDecT + 9 * MIN_MS) z.sHeld9 = 1;
    }
    // the minute close: fade check, the pane's "bid left" per candle, new zones
    void minuteClose(long long T) {
        ++minutes;
        maxBook = std::max(maxBook, book.size()); maxLive = std::max(maxLive, live.size());
        nearWindows(T);
        if (ghostTicks > 0 && lastPx > 0)
            for (std::unordered_map<long long, Ord>::iterator it = book.begin(); it != book.end();) {
                const bool crossed = it->second.side == BID ? it->second.px > lastPx + ghostTicks : it->second.px < lastPx - ghostTicks;
                if (crossed) { ++ghosts; it = book.erase(it); } else ++it;
            }
        { size_t n = 0; for (std::map<std::pair<int, long long>, std::deque<Ended> >::iterator it = ended.begin(); it != ended.end();) { while (!it->second.empty() && T - it->second.front().tEnd > 17 * MIN_MS) it->second.pop_front(); n += it->second.size(); if (it->second.empty()) ended.erase(it++); else ++it; } maxEnded = std::max(maxEnded, n); }
        { std::vector<int> k; for (size_t q = 0; q < live.size(); ++q) { const Zone& z = zones[live[q]]; if (z.active() || T - z.endT <= (z.endCode == 1 ? 62 : 31) * MIN_MS) k.push_back(live[q]); } live.swap(k); }
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]]; if (!z.active()) { continue; }
            const long long r = restingIn(z.side, z.lo, z.hi);
            if (r > z.peak) z.peak = r;
            if (z.touchT >= 0) { z.barLeft[candleKey(T - 1)] = r; if (z.zeroT < 0 && r == 0) z.zeroT = T; }
            if (z.peak > 0 && r * 2 < z.peak && (z.touchT < 0 || (z.decT >= 0 && z.sweep >= 0 && z.state != 1))) endZone(z, 2, T);   // touched: only after its reload + sweep decisions
        }
        if (T % (SWEEP_BAR_S * 1000LL) == 0) candleClose(T);
        if (barSec > 0 && (T / 1000) % barSec == 0) barLines(T);
        if (lastPx <= 0) return;
        const int per = periodOf(ctMinute(T), M);
        retag(T);
        for (int side = 0; side < 2; ++side) detect(side, T, per);
    }
    // (1.0.2) the same-distance queue normal: close the windows opened 60 s ago, open one per side now, prune
    void nearWindows(long long T) {
        for (int sd = 0; sd < 2; ++sd) {
            NearQ& q = nearQ[sd];
            for (size_t k = 0; k < q.w.size(); ++k) if (!q.w[k].ids.empty() && q.w[k].T + MIN_MS <= T) {
                for (size_t j = 0; j < q.w[k].ids.size(); ++j) { std::unordered_map<long long, long long>::const_iterator c = cancelledAt.find(q.w[k].ids[j].first); if (c != cancelledAt.end() && c->second <= q.w[k].T + MIN_MS) ++q.w[k].c[q.w[k].ids[j].second]; }
                std::vector<std::pair<long long, int> >().swap(q.w[k].ids);
            }
            while (!q.w.empty() && T - q.w.front().T > 32 * MIN_MS) q.w.pop_front();
            if (lastPx <= 0) continue;
            NearW nw; nw.T = T;
            for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) {
                const Ord& o = it->second; if (o.side != sd || o.tAdd > T - QUEUE_REST_S * 1000) continue;
                const long long d = sd == BID ? lastPx - o.px : o.px - lastPx;
                if (d >= 0 && d <= 41) { nw.ids.push_back(std::make_pair(it->first, (int)d)); ++nw.n[d]; }
            }
            if (nw.ids.empty()) nw.ids.push_back(std::make_pair(-1LL, 0));   // keeps the window "open" until it closes
            q.w.push_back(nw);
        }
        for (std::unordered_map<long long, long long>::iterator it = cancelledAt.begin(); it != cancelledAt.end();) { if (T - it->second > 2 * MIN_MS) it = cancelledAt.erase(it); else ++it; }
    }
    void setState(Zone& z, int st, long long T) {
        if (st == z.state) return;                                    // a state is written once, when it changes
        z.state = st; z.stateT = T; z.sHeld9b = -1;
        char b[80]; if (st == 5) std::snprintf(b, sizeof(b), "ZX,%d,%lld,%d,%d", z.id, T, st, z.reclaimBars); else std::snprintf(b, sizeof(b), "ZX,%d,%lld,%d", z.id, T, st);
        Line l; l.t = T; l.text = b; out.push_back(l);
        if (st == 4) endZone(z, 1, T);   // the zone ends at the confirmed break; a later reclaim (5) is a new line, the band is redrawn to it
    }
    // (1.0.2) a 3-min candle closed at T: the touched zones' states (Rassul's break rule)
    void candleClose(long long T) {
        if (cK < 0 || (cK + 1) * SWEEP_BAR_S * 1000LL != T) { for (size_t li = 0; li < live.size(); ++li) zones[live[li]].through = false; return; }   // no trade in it
        const long long o = cO, h = cH, l = cL, c = cC;
        ranges.push_back(h - l); while ((int)ranges.size() > ATR_BARS) ranges.pop_front();
        for (size_t li = 0; li < live.size(); ++li) {
            Zone& z = zones[live[li]];
            if (!z.active() && z.endCode == 1 && z.state == 4 && z.reclaimBars < 0) {      // a broken zone: closed back inside?
                const long long bars = (T - z.stateT) / (SWEEP_BAR_S * 1000LL);
                if (bars > RECLAIM_WATCH) { z.reclaimBars = 0; continue; }               // 0 = not reclaimed within the watch
                if (z.side == BID ? cC >= z.lo : cC <= z.hi) { z.reclaimBars = (int)bars; if (bars <= RECLAIM_BARS) setState(z, 5, T); }
                continue;
            }
            if (!z.active() || z.touchT < 0 || z.touchT >= T) continue;
            const bool bid = z.side == BID;
            const bool beyond = bid ? c < z.lo : c > z.hi;                    // the close beyond the zone
            const bool back = bid ? c >= z.lo : c <= z.hi;                    // the close back inside / on the defenders' side
            if (z.closedBeyond) {
                const bool follow = bid ? (c < o && l < z.cbExt && c < z.cbClose) : (c > o && h > z.cbExt && c > z.cbClose);
                if (back) { z.closedBeyond = false; setState(z, 3, T); }
                else if (follow) setState(z, 4, T);
                else if (++z.candlesTesting > 20) endZone(z, 3, T);       // an hour without a resolution
            } else if (beyond) {
                z.closedBeyond = true; z.cbClose = c; z.cbExt = bid ? l : h; z.candlesTesting = 0;
                if (z.state != 1) setState(z, 1, T);
            } else if (z.through || z.state == 1) {
                setState(z, 2, T);                                        // traded through but closed back: held
            } else if (z.state == 0) setState(z, 2, T);                  // the touch candle closed with the zone holding
            z.through = false;
        }
    }
    // the candle closed at T: one ZB line per touched zone that was live in it (sold into the zone in that candle, resting left)
    void barLines(long long T) {
        const long long key = candleKey(T - 1);
        for (size_t li = 0; li < live.size(); ++li) {
            Zone& z = zones[live[li]]; if (z.touchT < 0 || z.touchT >= T) continue;
            if (!z.active() && z.endT < T - barSec * 1000) continue;
            std::map<long long, long long>::const_iterator s = z.barSold.find(key);
            const long long sold = s == z.barSold.end() ? 0 : s->second, left = z.active() ? restingIn(z.side, z.lo, z.hi) : 0;
            char b[120]; std::snprintf(b, sizeof(b), "ZB,%d,%lld,%lld,%lld", z.id, key, sold, left);
            Line l; l.t = T; l.text = b; out.push_back(l);
        }
    }
    // the zone's level tags (nearest first) and the research record
    void tagZone(Zone& z) const {
        z.tags.clear(); z.nearKind = LK_NONE; z.nearGapAtr = -1; z.nearName.clear();
        const long long reach = reachTicks(); const double atr = atrTicks();
        long long bestG = -1;
        for (size_t i = 0; i < levels.size(); ++i) {
            const long long g = levelGap(z.lo, z.hi, z.side, levels[i]); if (g < 0) continue;
            if (atr > 0 && g > 3 * atr && g > reach) continue;
            if (bestG < 0 || g < bestG || (g == bestG && levels[i].kind > z.nearKind)) { bestG = g; z.nearKind = levels[i].kind; z.nearName = levels[i].name; z.nearGapAtr = atr > 0 ? g / atr : (g == 0 ? 0 : -1); }
            if (g > reach) continue;
            LTag t; t.name = levels[i].name; t.px = levels[i].px; t.kind = levels[i].kind; t.gap = g;
            bool dup = false; for (size_t k = 0; k < z.tags.size(); ++k) dup = dup || (z.tags[k].name == t.name);
            if (!dup) z.tags.push_back(t);
        }
        std::stable_sort(z.tags.begin(), z.tags.end(), [](const LTag& a, const LTag& b) { return a.gap < b.gap || (a.gap == b.gap && a.kind > b.kind); });
        if ((int)z.tags.size() > LVL_SHOW) z.tags.resize(LVL_SHOW);
    }
    std::string tagText(const Zone& z) const {
        std::string s; for (size_t k = 0; k < z.tags.size(); ++k) { if (k) s += ';'; s += z.tags[k].name + ":" + std::to_string(z.tags[k].px); } return s;
    }
    void writeTags(const Zone& z, long long T) {
        char b[48]; std::snprintf(b, sizeof(b), "ZL,%d,%lld,", z.id, T);
        Line l; l.t = T; l.text = std::string(b) + tagText(z); out.push_back(l);
    }
    // an untouched zone's tags follow the levels (they change during the session); a change writes a new ZL line (never repaints the old)
    void retag(long long T) {
        for (size_t li = 0; li < live.size(); ++li) {
            Zone& z = zones[live[li]]; if (!z.active() || z.touchT >= 0) continue;
            const std::string before = tagText(z); tagZone(z);
            if (tagText(z) != before) writeTags(z, T);
        }
    }
    bool makeZone(int side, long long T, int per, long long lo, long long hi, long long best, double dens, bool atLevel) {
        // one active zone per side near a price: an overlapping active zone only refreshes
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]]; if (!z.active()) { continue; }
            if (z.side == side && lo <= z.hi + BAND && hi >= z.lo - BAND) { if (z.touchT < 0 && best > z.size) z.size = best; return false; }
        }
        Zone z; z.id = nextId++; z.side = side; z.per = per; z.T = T; z.lo = lo; z.hi = hi; z.size = best; z.peak = best; z.dens = dens; z.levelSearch = atLevel;
        double oldest = 0; long long bo = 0, bs = 0, bp = 0;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) {
            const Ord& o = it->second; if (o.side != side || o.px < lo || o.px > hi || o.tAdd > T - MIN_AGE_MIN * MIN_MS) continue;
            oldest = std::max(oldest, (T - o.tAdd) / 60000.0);
            if (o.sz > bs) { bs = o.sz; bo = it->first; bp = o.px; }
        }
        z.oldestMin = oldest;
        if (bs >= M.bigLots) { z.bigOid = bo; z.bigSz = bs; z.bigPx = bp; }
        tagZone(z);
        zones.push_back(z); live.push_back((int)zones.size() - 1);
        char b[240]; std::snprintf(b, sizeof(b), "ZN,%d,%lld,%d,%lld,%lld,%lld,%.2f,%.1f,%d,%lld,%lld", z.id, T, side, lo, hi, best, dens, oldest, per, z.bigSz, z.bigPx);
        Line l; l.t = T; l.text = b; out.push_back(l);
        if (!z.tags.empty()) writeTags(z, T);
        return true;
    }
    long long levelFloorOf(const zdp::Per& p) const { return M.levelFloor > 0 ? std::min(p.minLots, std::max(M.levelFloor, (long long)std::ceil(p.minLots * M.levelFloorX))) : p.minLots; }
    void detect(int side, long long T, int per) {
        const zdp::Per& p = P(per);
        long long at[MAX_DIST + 1] = { 0 };                       // aged resting lots per distance from the last trade
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) {
            const Ord& o = it->second; if (o.side != side || o.tAdd > T - MIN_AGE_MIN * MIN_MS) continue;
            const long long d = side == BID ? lastPx - o.px : o.px - lastPx;
            if (d < MIN_DIST || d > MAX_DIST) continue;
            at[d] += o.sz;
        }
        long long tot[16] = { 0 }; const int NB = (MAX_DIST - MIN_DIST + 1 + BAND - 1) / BAND;
        for (int d = MIN_DIST; d <= MAX_DIST; ++d) { const int b = (d - MIN_DIST) / BAND; if (b < NB && b < 16) tot[b] += at[d]; }
        int bi = 0; std::vector<long long> nz;
        for (int b = 0; b < NB && b < 16; ++b) { if (tot[b] > tot[bi]) bi = b; if (tot[b] > 0) nz.push_back(tot[b]); }
        const long long best = tot[bi]; if (best <= 0) return;
        double med = (double)best;
        if (nz.size() > 2) { std::sort(nz.begin(), nz.end()); const size_t m = nz.size(); med = m % 2 ? (double)nz[m / 2] : (nz[m / 2 - 1] + nz[m / 2]) / 2.0; }
        if (warmUntil >= 0 && T < warmUntil) return;              // (M2) the book is still filling after a cold start
        const long long floorLots = minLotsOverride > 0 ? minLotsOverride : p.minLots;
        // (1.0.3, Rassul approved 2026-11-08) a zone AT his levels (the level search below) needs less size: the market's research floor
        // or half the chart floor, whichever is higher (M.levelFloor 0 = off for that market); zones away from levels keep the floor
        const long long lvlFloor = minLotsOverride > 0 ? std::min(minLotsOverride, levelFloorOf(p)) : levelFloorOf(p);
        const double dens = med > 0 ? best / med : 0;
        if (dens >= DENS_X && best >= floorLots) {
            const long long loD = MIN_DIST + (long long)BAND * bi, hiD = loD + BAND - 1;
            makeZone(side, T, per, side == BID ? lastPx - hiD : lastPx + loD, side == BID ? lastPx - loD : lastPx + hiD, best, dens, false);
        }
        // (1.0.2 levels) the zone search also looks AT his levels within the display reach: the densest BAND-wide window that would be
        // tagged to the level (overlapping it or out to the reach beyond it), with the same size floor and density rule as the bands
        for (size_t i = 0; i < levels.size(); ++i) {
            const Lvl& L = levels[i];
            const long long dL = side == BID ? lastPx - L.px : L.px - lastPx;
            if (dL < MIN_DIST - BAND || dL > MAX_DIST + reachTicks()) continue;
            long long bl = 0; long long bLoD = -1;
            for (int s0 = MIN_DIST; s0 + BAND - 1 <= MAX_DIST; ++s0) {
                const long long lo = side == BID ? lastPx - (s0 + BAND - 1) : lastPx + s0, hi = side == BID ? lastPx - s0 : lastPx + s0 + BAND - 1;
                const long long g = levelGap(lo, hi, side, L); if (g < 0 || g > reachTicks()) continue;
                long long sum = 0; for (int d = s0; d < s0 + BAND; ++d) sum += at[d];
                if (sum > bl) { bl = sum; bLoD = s0; }
            }
            if (bLoD < 0 || bl < lvlFloor || (med > 0 ? bl / med : 0) < DENS_X) continue;
            if (makeZone(side, T, per, side == BID ? lastPx - (bLoD + BAND - 1) : lastPx + bLoD, side == BID ? lastPx - bLoD : lastPx + bLoD + BAND - 1, bl, med > 0 ? bl / med : 0, true) && bl < floorLots)
                zones.back().lowFloor = true;
        }
    }
};
} // namespace zd
