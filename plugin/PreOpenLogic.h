// PreOpenLogic.h  1.0.0  (2026-10-22, lsHodLod 2.6.0; Rassul 15:57 CT "yes hod lod box"; MBO Roadmap "What does the pre-open book say about
// the first extreme?", ticket T-20261022-29)  -  SDK-free, unit-tested on Linux (tests_hodlod/t_po260.cpp), replayed on real MBO
// (tests_hodlod/replay_po260.cpp).
//
// THE PRE-OPEN BOOK: in the last N minutes before the RTH open (setting, default 15; the open = lsHodLod's RTH open per market: ES / NQ
// 8:30, CL / NG 8:00, GC / HG / 6E 7:20 CT) the order-by-order (MBO) book near price is measured:
//   - NEAR       = |order price - the last trade price| <= nearTicks (1 ATR of the chart's closed candles, >= 4 ticks);
//   - SIZE       = the resting lots near price per side at the open (bids vs offers, as a ratio "Bids 2.1x");
//   - STAYED     = per side, the near lots that had rested >= 60 s and were still there at the open, as a share of those plus the near
//                  lots that had rested >= 60 s and were PULLED (cancelled, cut, or moved away) inside the window: "70% stay";
//   - BIG        = near resting orders >= the market's Big Lot S20 (overnight tier) per side at the open (hover + record);
//   - OPEN       = the opening imbalance: aggressive buy vs sell lots in the first M minutes after the open (default 3), from the fills
//                  of resting orders (a resting offer filled = a buyer lifted it).
// THE CALL (an untested lean - always "?" until the nightly proves it): the heavier side wins when it is >= 1.5x the other AND >= 50% of its
// committed size stayed: heavy BIDS -> "1st LOW" (the first extreme of the day is the LOW, buyers defend under price), heavy OFFERS ->
// "1st HIGH"; "early" (in the first 30 min) when the lean is >= 2.0x with >= 60% stayed. Otherwise "no lean".
// The read is LIVE inside the window, FROZEN at the open (SNAP) and completed with the opening imbalance at open + M (IMB). Both are
// recorded (lsFlexLevels\HodLod\PreOpen-<MKT>-<spb>.csv) and read back after a restart - never repainted. The feed must have started
// by the window's start (+60 s) or the line says "started late" (no read: orders resting before the feed are invisible in IRT's MBO).
#pragma once
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace po {

static const char* const VERSION = "1.1.0";
enum { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum { ASK = 0, BID = 1 };
static const size_t BOOK_CAP = 300000;              // orders tracked at most (the farthest dropped first)
static const long long SIZE_MAX_SANE = 10000000;

struct Params {
    int windowMin = 15;        // N: the minutes before the open that are measured (setting, 5..60)
    int restSec = 60;          // an order is COMMITTED once it rested this long at its price
    int imbMin = 3;            // the opening imbalance: the first M minutes after the open
    double leanX = 1.5;        // the heavier side needs this ratio for a lean
    double strongX = 2.0;      // ... and this for "early"
    int stayLeanPct = 50, stayStrongPct = 60;
    int showBeforeMin = 30, showAfterMin = 60;   // the line is shown from 30 min before the open to 60 min after
    int leadMin = 30;          // the feed is opened this long before the window starts (book warm-up)
    int lateSec = 60;          // feed started more than this after the window start = "started late"
    int earlyMin = 30;         // "early" = the first extreme within the first 30 min (what the nightly scores)
};
// the Big Lot S20 of RESTING orders, overnight tier (LmtZonesLogic.h lmz::ruleFor s20On, 2026-10-22 - copied, not included, so the
// two plugins never break each other's build)
inline long long s20Of(const std::string& m) {
    return m == "ES" ? 25 : m == "NQ" ? 15 : m == "CL" ? 20 : m == "GC" ? 12 : m == "HG" ? 5 : m == "NG" ? 10 : m == "EU" ? 41 : 20;
}
inline std::string hm12(int minuteOfDay) {   // 12-hour CT "8:15 AM"
    minuteOfDay = ((minuteOfDay % 1440) + 1440) % 1440;
    int h = minuteOfDay / 60, m = minuteOfDay % 60; const bool pm = h >= 12; h %= 12; if (h == 0) h = 12;
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b;
}
inline std::string x1(double r) { char b[24]; if (!std::isfinite(r)) return "-"; if (r >= 99.95) return "99x+"; std::snprintf(b, sizeof b, "%.1fx", r); return b; }
inline std::string lotsTxt(long long v) {
    std::string s = std::to_string(v < 0 ? -v : v); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ",");
    return (v < 0 ? "-" : "") + s;
}

struct Snap {
    bool ok = false;           // decided (or a live read)
    bool late = false;         // the feed started after the window start: no read
    long long atMs = 0;        // CT ms of the read
    long long feedFromMs = 0;  // the first event of the day
    long long ref = 0;         // the last trade price (ticks)
    int nearTicks = 0;
    long long lots[2] = { 0, 0 }, stayed[2] = { 0, 0 }, pulled[2] = { 0, 0 }, big[2] = { 0, 0 };
    long long s20 = 0;
    // the call: +1 = 1st LOW (bids heavy), -1 = 1st HIGH (offers heavy), 0 = no lean
    int call = 0; bool early = false;
};
inline int heavySide(const Snap& s) { return s.lots[BID] > s.lots[ASK] ? BID : s.lots[ASK] > s.lots[BID] ? ASK : -1; }
inline double ratioOf(const Snap& s) {
    const long long a = std::max(s.lots[BID], s.lots[ASK]), b = std::min(s.lots[BID], s.lots[ASK]);
    return a <= 0 ? NAN : b <= 0 ? 99.99 : (double)a / (double)b;
}
inline int stayPct(const Snap& s, int side) {
    const long long d = s.stayed[side] + s.pulled[side]; if (d <= 0) return -1;
    return (int)std::llround(100.0 * (double)s.stayed[side] / (double)d);
}
inline void decideCall(Snap& s, const Params& P) {
    s.call = 0; s.early = false;
    const int h = heavySide(s); if (h < 0) return;
    const double r = ratioOf(s); const int st = stayPct(s, h);
    if (!std::isfinite(r) || r < P.leanX || st < P.stayLeanPct) return;
    s.call = h == BID ? 1 : -1;
    s.early = r >= P.strongX && st >= P.stayStrongPct;
}

struct Imb { bool ok = false; long long buy = 0, sell = 0; int min = 0; };

// `size` is the currently unfilled resting quantity.  A FILL reduces it before
// a later DEL/MOD is assessed, so an actual execution is never labelled a pull
// but an explicit cancellation/reprice of the remaining quantity is.
struct Ord { int side = 0; long long px = 0, size = 0, t0 = 0; };

// the engine of ONE market and ONE trading day (setDay() starts a new one). Times are CT wall-clock ms (the event's own stamp).
class Engine {
public:
    Params P; std::string mkt; int openMin = 510; long long s20 = 20; int nearTicks = 8; long long refFallback = 0;
    long long dayMs = -1;                                      // CT midnight of the session's day (ms)
    Snap live, snap; Imb imb; bool snapDone = false, imbDone = false;
    long long feedFromMs = -1, lastMs = -1, events = 0, ignored = 0, lastTrade = 0, pruned = 0;
    long long fillLots[2] = { 0, 0 };                          // aggressive buy (offers filled) / sell (bids filled) lots in the imbalance window
    std::unordered_map<unsigned long long, Ord> book;

    long long openMs() const { return dayMs + (long long)openMin * 60000LL; }
    long long winStartMs() const { return openMs() - (long long)P.windowMin * 60000LL; }
    long long feedStartMs() const { return winStartMs() - (long long)P.leadMin * 60000LL; }
    long long imbEndMs() const { return openMs() + (long long)P.imbMin * 60000LL; }
    long long showFromMs() const { return openMs() - (long long)P.showBeforeMin * 60000LL; }
    long long showToMs() const { return openMs() + (long long)P.showAfterMin * 60000LL; }
    // the feed is wanted from 30 min before the window start to the end of the imbalance window (+ 1 min)
    bool wantFeed(long long nowMs) const { return dayMs >= 0 && nowMs >= feedStartMs() && nowMs < imbEndMs() + 60000; }

    void setDay(long long dayCtMs) {
        if (dayCtMs == dayMs) return;
        dayMs = dayCtMs; live = Snap(); snap = Snap(); imb = Imb(); snapDone = imbDone = false;
        feedFromMs = lastMs = -1; events = ignored = pruned = 0; lastTrade = 0; fillLots[0] = fillLots[1] = 0; pulledLots[0] = pulledLots[1] = 0; book.clear();
    }
    void restoreSnap(const Snap& s) { snap = s; snapDone = true; }
    void restoreImb(const Imb& i) { imb = i; imbDone = true; }

    long long ref() const { return lastTrade > 0 ? lastTrade : refFallback; }
    bool near(long long px) const { const long long r = ref(); return r > 0 && std::llabs(px - r) <= (long long)nearTicks; }

    // one order event; px in ticks (> 0), side ASK / BID, ms = CT ms
    void onEvent(int action, int side, long long px, long long size, unsigned long long id, long long ms) {
        if (dayMs < 0 || ms <= 0) { ignored++; return; }
        if (ms < feedStartMs() - 3600000LL || ms >= imbEndMs() + 600000LL) { ignored++; return; }   // another day's event
        if (action < NEW || action > FILL || px <= 0 || size < 0 || size > SIZE_MAX_SANE) { ignored++; return; }
        // decisions due BEFORE this event (the book as it was at the open)
        advance(ms);
        if (feedFromMs < 0) feedFromMs = ms;
        lastMs = std::max(lastMs, ms); events++;
        const bool inWin = ms >= winStartMs() && ms < openMs();
        if (action == FILL) {
            lastTrade = px;
            if (ms >= openMs() && ms < imbEndMs() && (side == BID || side == ASK)) fillLots[side == ASK ? 0 : 1] += size;   // offer filled = buy
            auto it = book.find(id);
            if (it != book.end()) it->second.size = std::max(0LL, it->second.size - size);
            return;
        }
        if (side != BID && side != ASK) { ignored++; return; }
        auto it = book.find(id);
        if (action == NEW) {
            Ord o; o.side = side; o.px = px; o.size = size; o.t0 = ms; book[id] = o; prune(); return;
        }
        if (action == DEL) {
            if (it == book.end()) return;                              // rested before the feed: unknown
            const Ord& o = it->second;
            if (inWin) pull(o, o.size, ms);   // size is the residual after any real fills
            book.erase(it); return;
        }
        // MOD
        if (it == book.end()) { Ord o; o.side = side; o.px = px; o.size = size; o.t0 = ms; book[id] = o; prune(); return; }
        Ord& o = it->second;
        if (px != o.px) {                                               // moved: a pull from the old price
            if (inWin) pull(o, o.size, ms);   // only the unfilled residual was moved away
            o.px = px; o.size = size; o.t0 = ms; o.side = side; return;
        }
        if (size < o.size && inWin) pull(o, o.size - size, ms);   // cut at the same price
        o.size = size;
    }
    // time passes with no event (the chart's clock): decide what is due
    void tick(long long nowMs) { if (dayMs >= 0) advance(nowMs - 5000); }   // 5 s for the feed to deliver what was stamped before

    // the read as of `ms` (live inside the window; the frozen one after the open)
    Snap readAt(long long ms) const {
        Snap s; s.atMs = ms; s.feedFromMs = feedFromMs; s.ref = ref(); s.nearTicks = nearTicks; s.s20 = s20;
        s.late = feedFromMs < 0 || feedFromMs > winStartMs() + (long long)P.lateSec * 1000LL;
        for (int k = 0; k < 2; ++k) { s.pulled[k] = pulledLots[k]; }
        if (s.ref <= 0) return s;
        for (const auto& kv : book) {
            const Ord& o = kv.second; if (o.size <= 0 || !near(o.px)) continue;
            s.lots[o.side] += o.size;
            if (ms - o.t0 >= (long long)P.restSec * 1000LL) s.stayed[o.side] += o.size;
            if (o.size >= s20) s.big[o.side]++;
        }
        s.ok = !s.late;
        if (s.ok) decideCall(s, P);
        return s;
    }
    long long pulledLots[2] = { 0, 0 };

private:
    void pull(const Ord& o, long long lots, long long ms) {
        if (lots <= 0 || !near(o.px)) return;
        if (ms - o.t0 >= (long long)P.restSec * 1000LL) pulledLots[o.side] += lots;   // a committed order left: it did not stay
    }
    void advance(long long ms) {
        if (!snapDone && ms >= openMs() && feedFromMs >= 0 && lastMs >= 0) {
            snap = readAt(openMs()); snapDone = true;
        }
        if (snapDone && !imbDone && ms >= imbEndMs() && feedFromMs >= 0) {
            imb.ok = feedFromMs <= openMs(); imb.buy = fillLots[0]; imb.sell = fillLots[1]; imb.min = P.imbMin; imbDone = true;
        }
    }
    void prune() {
        if (book.size() <= BOOK_CAP) return;
        const long long r = ref(); std::vector<std::pair<long long, unsigned long long> > d; d.reserve(book.size());
        for (const auto& kv : book) d.push_back(std::make_pair(r > 0 ? std::llabs(kv.second.px - r) : 0, kv.first));
        std::nth_element(d.begin(), d.begin() + (long)(BOOK_CAP * 9 / 10), d.end(), [](const std::pair<long long, unsigned long long>& a, const std::pair<long long, unsigned long long>& b) { return a.first < b.first; });
        for (size_t i = BOOK_CAP * 9 / 10; i < d.size(); ++i) { book.erase(d[i].second); pruned++; }
    }
};

// ---- the line -------------------------------------------------------------------------------------------------------------------
enum { COL_GREY = 0, COL_GREEN = 1, COL_RED = 2 };
struct Line { bool show = false; std::string s; int col = COL_GREY; int state = 0; };   // state: 0 hidden, 1 waiting, 2 live, 3 frozen, 4 final, 5 no feed, 6 late
inline std::string imbText(const Imb& i) {
    if (!i.ok) return std::string();
    const long long a = std::max(i.buy, i.sell), b = std::min(i.buy, i.sell);
    if (a <= 0) return "open quiet";
    const double r = b > 0 ? (double)a / (double)b : 99.99;
    if (r < 1.2) return "open even";
    return std::string("open ") + (i.buy > i.sell ? "buy " : "sell ") + x1(r);
}
inline std::string snapText(const Snap& s, const Imb* imb) {
    const int h = heavySide(s); const double r = ratioOf(s);
    std::string t;
    if (h < 0 || !std::isfinite(r)) t = "Book empty near price";
    else if (r < 1.2) t = "Book even";
    else t = std::string(h == BID ? "Bids " : "Offers ") + x1(r);
    const int st = h >= 0 ? stayPct(s, h) : -1;
    if (st >= 0) t += " \xB7 " + std::to_string(st) + "% stay";
    if (imb) { const std::string it = imbText(*imb); if (!it.empty()) t += " \xB7 " + it; }
    if (s.call != 0) t += std::string(" \xB7 1st ") + (s.call > 0 ? "LOW" : "HIGH") + (s.early ? " early" : "") + " ?";
    else t += " \xB7 no lean";
    return t;
}
// the box line at CT ms `now`; feedState: 1 = the feed is delivering, 0 = not (yet), -1 = no feed possible
inline Line lineAt(const Engine& E, long long now, int feedState) {
    Line L;
    if (E.dayMs < 0 || now < E.showFromMs() || now >= E.showToMs()) return L;
    L.show = true;
    if (E.snapDone) {
        L.state = E.imbDone ? 4 : 3;
        if (E.snap.late || !E.snap.ok) { L.state = 6; L.s = "Pre-open: started late - no read"; return L; }
        L.s = snapText(E.snap, E.imbDone ? &E.imb : nullptr);
        L.col = E.snap.call > 0 ? COL_GREEN : E.snap.call < 0 ? COL_RED : COL_GREY;
        return L;
    }
    if (now >= E.openMs()) { L.state = 5; L.s = feedState > 0 ? "Pre-open: waiting for the open's events" : "Pre-open: no order feed (MBO)"; return L; }
    if (now < E.winStartMs()) { L.state = 1; L.s = "Pre-open read from " + hm12(E.openMin - E.P.windowMin) + (feedState > 0 ? "" : " (MBO not ready)"); return L; }
    if (feedState <= 0 && E.feedFromMs < 0) { L.state = 5; L.s = "Pre-open: no order feed (MBO)"; return L; }
    const Snap s = E.readAt(now);
    if (s.late) { L.state = 6; L.s = "Pre-open: started late - no read"; return L; }
    L.state = 2; L.s = snapText(s, nullptr);
    L.col = s.call > 0 ? COL_GREEN : s.call < 0 ? COL_RED : COL_GREY;
    return L;
}

// ---- the hover (question / what it is / why it matters / how to use / example), then the numbers -------------------------------
struct HLine { std::string s; int col; bool bold; };
inline std::vector<HLine> hover(const Engine& E, long long now) {
    std::vector<HLine> o;
    const std::string op = hm12(E.openMin), from = hm12(E.openMin - E.P.windowMin);
    o.push_back(HLine{ "Pre-open book - what does the book before the " + op + " open say about the day's FIRST extreme?", COL_GREY, true });
    o.push_back(HLine{ "What it is: the resting limit orders within 1 ATR of price from " + from + " to " + op + " (MBO, order by order) - bids vs offers, "
                       "and how much of the size that rested 60 s or more STAYED into the open instead of being pulled.", COL_GREY, false });
    o.push_back(HLine{ "Why it matters: size that stays into the open is real interest; the side with more staying size is where price tends to be "
                       "defended, so the first extreme tends to form against it (staying bids -> the LOW first).", COL_GREY, false });
    o.push_back(HLine{ "How to use it: a lean for the first 30-60 min only. '1st LOW early ?' = look for the first-extreme LOW at a key level with "
                       "buyers defending; 'no lean' = ignore it. '?' = not proven yet - the nightly scores it against the recorded first extremes.", COL_GREY, false });
    o.push_back(HLine{ "Example: 'Bids 2.1x \xB7 70% stay \xB7 1st LOW early ?' = 2.1 lots bid for every lot offered near price, 70% of the bids that "
                       "rested a minute stayed - expect the LOW of the day to print first, in the first 30 min.", COL_GREY, false });
    const bool frozen = E.snapDone;
    const Snap s = frozen ? E.snap : E.readAt(now);
    if (E.feedFromMs < 0) { o.push_back(HLine{ "Now: no order-feed events yet.", COL_GREY, false }); return o; }
    char b[400];
    const int sb = stayPct(s, BID), sa = stayPct(s, ASK);
    std::snprintf(b, sizeof b, "%s: bids %s lots (stayed %s) \xB7 offers %s lots (stayed %s) \xB7 near = %d ticks of the last trade",
                  frozen ? ("At the open " + op).c_str() : "Now (live)", lotsTxt(s.lots[BID]).c_str(), sb >= 0 ? (std::to_string(sb) + "%").c_str() : "-",
                  lotsTxt(s.lots[ASK]).c_str(), sa >= 0 ? (std::to_string(sa) + "%").c_str() : "-", s.nearTicks);
    o.push_back(HLine{ b, s.call > 0 ? COL_GREEN : s.call < 0 ? COL_RED : COL_GREY, false });
    std::snprintf(b, sizeof b, "Big orders (S20 = %lld+ lots): bids %lld \xB7 offers %lld \xB7 pulled after resting 60 s: bids %s \xB7 offers %s lots",
                  s.s20, s.big[BID], s.big[ASK], lotsTxt(s.pulled[BID]).c_str(), lotsTxt(s.pulled[ASK]).c_str());
    o.push_back(HLine{ b, COL_GREY, false });
    if (E.imbDone && E.imb.ok) {
        std::snprintf(b, sizeof b, "Open (first %d min): buyers %s lots vs sellers %s lots - %s", E.imb.min, lotsTxt(E.imb.buy).c_str(), lotsTxt(E.imb.sell).c_str(), imbText(E.imb).c_str());
        o.push_back(HLine{ b, E.imb.buy > E.imb.sell ? COL_GREEN : E.imb.sell > E.imb.buy ? COL_RED : COL_GREY, false });
    }
    std::snprintf(b, sizeof b, "Feed from %s%s \xB7 status: learning (untested lean)", hm12((int)(((s.feedFromMs - E.dayMs) / 60000LL) % 1440)).c_str(), s.late ? " - started after the window: no read" : "");
    o.push_back(HLine{ b, COL_GREY, false });
    return o;
}

// ---- the record (one SNAP and one IMB line per session; first line per session + kind wins) --------------------------------------
static const char* const REC_HEADER = "version,session,market,spb,kind,open_ct,window_min,feed_from_ct,late,ref_ticks,near_ticks,bid_lots,ask_lots,stay_bid,stay_ask,pulled_bid,pulled_ask,big_bid,big_ask,s20,call,early,imb_min,imb_buy,imb_sell,line";
inline std::string sessionKey(long long dayMs) {   // YYYY-MM-DD of the CT day
    long long z = dayMs / 86400000LL + 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; long long y = (long long)yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153, d = doy - (153 * mp + 2) / 5 + 1, m = mp < 10 ? mp + 3 : mp - 9; if (m <= 2) y++;
    char b[48]; std::snprintf(b, sizeof b, "%04lld-%02u-%02u", y, m, d); return b;
}
inline std::string clean(std::string s) { for (char& c : s) if (c == ',' || c == '\n' || c == '\r') c = ';'; return s; }
inline std::string recLine(const Engine& E, long spb, bool imbKind) {
    const Snap& s = E.snap; char b[512];
    const int ff = s.feedFromMs > 0 ? (int)(((s.feedFromMs - E.dayMs) / 60000LL) % 1440) : -1;
    std::snprintf(b, sizeof b, "%s,%s,%s,%ld,%s,%d,%d,%d,%d,%lld,%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%d,%d,%d,%lld,%lld,",
                  VERSION, sessionKey(E.dayMs).c_str(), E.mkt.c_str(), spb, imbKind ? "IMB" : "SNAP", E.openMin, E.P.windowMin, ff, s.late ? 1 : 0,
                  s.ref, s.nearTicks, s.lots[BID], s.lots[ASK], s.stayed[BID], s.stayed[ASK], s.pulled[BID], s.pulled[ASK], s.big[BID], s.big[ASK], s.s20,
                  s.call, s.early ? 1 : 0, imbKind ? E.imb.min : 0, imbKind ? E.imb.buy : 0, imbKind ? E.imb.sell : 0);
    return std::string(b) + clean(s.late ? std::string("started late") : snapText(s, imbKind ? &E.imb : nullptr)) + "\n";
}
inline bool fullLL(const std::string& t, long long& v) {
    if (t.empty() || t.size() > 18) return false;
    char* e = nullptr; errno = 0; v = std::strtoll(t.c_str(), &e, 10); return e && *e == 0 && errno == 0;
}
// a record line for (session, market, spb) -> the frozen read; returns 1 SNAP, 2 IMB, 0 not this session / garbled
inline int parseRec(const std::string& lnIn, const std::string& session, const std::string& mkt, long spb, long long dayMs, Snap& s, Imb& im) {
    std::string ln = lnIn; while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (int k = 0; k < 25; ++k) { const size_t c = ln.find(',', a); if (c == std::string::npos) return 0; f.push_back(ln.substr(a, c - a)); a = c + 1; }
    f.push_back(ln.substr(a));
    if (f[1] != session || f[2] != mkt || (f[4] != "SNAP" && f[4] != "IMB")) return 0;
    long long v[26] = { 0 };
    for (int k = 3; k < 25; ++k) { if (k == 4) continue; if (!fullLL(f[(size_t)k], v[k])) return 0; }
    if (v[3] != spb || v[11] < 0 || v[12] < 0 || v[13] < 0 || v[14] < 0 || v[13] > v[11] || v[14] > v[12] || v[20] < -1 || v[20] > 1) return 0;
    Snap r; r.ok = v[8] == 0; r.late = v[8] != 0; r.ref = v[9]; r.nearTicks = (int)v[10];
    r.lots[BID] = v[11]; r.lots[ASK] = v[12]; r.stayed[BID] = v[13]; r.stayed[ASK] = v[14]; r.pulled[BID] = v[15]; r.pulled[ASK] = v[16];
    r.big[BID] = v[17]; r.big[ASK] = v[18]; r.s20 = v[19]; r.call = (int)v[20]; r.early = v[21] != 0;
    r.atMs = dayMs + v[5] * 60000LL; r.feedFromMs = v[7] >= 0 ? dayMs + v[7] * 60000LL : 0;
    s = r;
    if (f[4] == "IMB") { im.ok = true; im.min = (int)v[22]; im.buy = v[23]; im.sell = v[24]; return 2; }
    return 1;
}

} // namespace po
