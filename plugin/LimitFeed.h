#ifndef LS_LIMIT_FEED_H
#define LS_LIMIT_FEED_H
// LimitFeed.h (1.0.0, 2026-11-10) -- the Limit Profile's OWN order feed (Rassul 22:16 CT "it needs to read the feed directly"): one
// DEPTH_BY_ORDER per chart, kept alive for the chart's life, read from the chart's draw (at most SLICE_MAX events or ~SLICE_MS per call),
// fed to the TopBook engine (TopBook.h - the same engine lsBookFlow runs), evaluated once a second into the same facts lsBookFlow posts
// as SignalBoard kind G. The open / drain / guard / never-re-enter-a-fault rules are off::Conn's (OrderFlowFeed.h), without its timer path.
// Include after irtsdk.h and SignalBoard.h. Chart context only.
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#include "TopBook.h"
#include <ctime>
#include <chrono>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
namespace lpf {
static const char* const VERSION = "1.1.0";   // 1.1.0 (2026-11-11): + the level liquidity rows (TopBook 1.2.0 levelLiq) as facts code 7, state = the level's index + 1
// was "1.0.0";
static const int SLICE_MAX = 5000; static const long long SLICE_MS = 8, WAIT_MS = 60000, SILENT_MS = 120000, RECONNECT_MS = 300000, GUARD_STALE_S = 2 * 3600;
inline long long realMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
inline long long (*&clockFn())() { static long long (*f)() = &realMs; return f; }      // tests replace the clock
inline long long nowMs() { return clockFn()(); }
// the private "subscribed" flag, read through an explicit instantiation (never by offset)
template <class Tag, typename Tag::type M> struct Rob { friend typename Tag::type robbed(Tag) { return M; } };
struct DboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type robbed(DboFlag); };
template struct Rob<DboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;
inline long long mtimeOf(const std::string& p) { struct stat s; return stat(p.c_str(), &s) == 0 ? (long long)s.st_mtime : -1; }
struct Feed {
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr; int attempt = 0, flag = -1; bool blocked = false, dead = false;
    long long openMs = -1, lastResubMs = -1, lastEventMs = -1, events = 0, faults = 0, capHits = 0, reconnects = 0, lastReconnectMs = -1, tradeKey = -1, lastTradeMs = -1;
    std::string ident, market, via, state = "not opened"; tbk::Engine tb; long long evalMs = -1, modelMs = -100000, modelStamp = -2; bool modelLive = false;
    std::vector<sb::Fact> facts; bool haveFacts = false; std::string tbLine;
    std::vector<long long> levelTicks; int levelRows = 0;   // (1.1.0) the host's level prices (ticks) for the level liquidity rows; rows posted in the last pass
    Feed() {} Feed(const Feed&) = delete; Feed& operator=(const Feed&) = delete; ~Feed() { delete dbo; }
    void closeFeed() { delete dbo; dbo = nullptr; attempt = 0; flag = -1; dead = false; openMs = lastResubMs = lastEventMs = -1; via.clear(); }
    void identity(const std::string& id, const std::string& mkt) {
        if (id == ident) return;
        closeFeed(); blocked = false; ident = id; tb.reset(); evalMs = -1; facts.clear(); haveFacts = false;
        if (market != mkt) { market = mkt; tb.setMarket(mkt); modelStamp = -2; modelMs = -100000; }
    }
    // the chart trades (its last bar's time / volume key changes) but no order event for 2 min: close, so open() opens it again (at most every 5 min)
    void watch(long long now, long long volKey) {
        if (volKey != tradeKey) { if (tradeKey != -1) lastTradeMs = now; tradeKey = volKey; }
        if (!dbo || flag != 1 || lastTradeMs < 0 || now - lastTradeMs > 30000) return;
        const long long last = lastEventMs >= 0 ? lastEventMs : openMs; if (last < 0 || now - last < SILENT_MS) return;
        if (lastReconnectMs >= 0 && now - lastReconnectMs < RECONNECT_MS) return;
        closeFeed(); tb.reset(); facts.clear(); haveFacts = false; ++reconnects; lastReconnectMs = now;
    }
    void open(const std::string& symbol, const std::string& guard, long long now) {
        if (blocked || dead || attempt >= 2) return;
        if (attempt == 1 && !(lastEventMs < 0 && openMs >= 0 && now - openMs >= WAIT_MS)) return;
        if (attempt == 0 && !guard.empty()) { const long long mt = mtimeOf(guard);
            if (mt >= 0) { if ((long long)std::time(nullptr) - mt > GUARD_STALE_S) std::remove(guard.c_str());
                           else { blocked = true; attempt = 2; state = "Investor/RT closed while opening the order feed last time (clears by itself after 2 h): " + guard; return; } } }
        if (!guard.empty()) { std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (attempt + 1) << "\n"; }
        try {
            if (attempt == 0) { const bool named = !symbol.empty(); dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(named ? symbol.c_str() : NULL); via = named ? "chart symbol " + symbol : "default"; attempt = named ? 1 : 2; }
            else { delete dbo; dbo = nullptr; dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(NULL); via = "default, 2nd try"; attempt = 2; }
        } catch (...) { ++faults; dead = true; }
        flag = -1; openMs = now;
        if (!dbo && !dead) dead = true;
        if (!guard.empty()) std::remove(guard.c_str());
    }
    int drain(long long now, double tick) {
        if (!dbo || dead || !(tick > 0)) return 0;
        if (flag != 1 && lastResubMs >= 0 && now - lastResubMs < 1000 && now >= lastResubMs) return 0;
        const long long t0 = nowMs(); int n = 0; bool cap = false;
        try {
            if (!(dbo->*robbed(DboFlag()))) lastResubMs = now;
            for (;;) {
                if (n >= SLICE_MAX) { cap = true; break; }
                if ((n & 511) == 511 && nowMs() - t0 > SLICE_MS) { cap = true; break; }
                if (dbo->getNext() != RTX_OK) break;
                ++n; const DEPTH_ORDER& o = dbo->dbo;
                const int act = o.action <= 3 ? (int)o.action : 99, side = o.buySell == 1 ? 0 : (o.buySell == 0 ? 1 : -1);
                if (act == 99 || (act != tbk::FILL && side < 0)) continue;
                tbk::In x; x.act = act; x.side = side < 0 ? 0 : side; x.px = (long long)std::llround((double)o.price / tick); x.sz = (long long)o.size; x.oid = (long long)o.orderID; x.tMs = now;
                if (x.px <= 0 && act != tbk::DEL && act != tbk::FILL) continue;
                tb.feed(x);
            }
            flag = (dbo->*robbed(DboFlag())) ? 1 : 0;
        } catch (...) { ++faults; dead = true; }                               // never re-enter a faulting call
        if (n > 0) { lastEventMs = now; events += n; }
        if (cap) ++capHits;
        return n;
    }
    // once a second: the same facts lsBookFlow posts as kind G (ranks 1-6 + the heartbeat code 99 with the book size)
    void evaluate(long long now, double tick, const std::string& flex, bool rth) {
        if (evalMs >= 0 && now - evalMs < 1000 && now >= evalMs) return;
        evalMs = now;
        if (now - modelMs >= 60000 || now < modelMs) { modelMs = now; const std::string path = flex.empty() || market.empty() ? std::string() : flex + "LRA-RealOrder-" + market + ".csv"; const long long st = path.empty() ? -1 : mtimeOf(path);
            if (st != modelStamp) { modelStamp = st; tbk::RealModel M; if (st >= 0) { std::ifstream in(path.c_str(), std::ios::binary); std::stringstream b; b << in.rdbuf(); if (b.str().size() < 20000) M = tbk::parseRealModel(b.str()); } tb.setModel(M); modelLive = M.live; } }
        tb.setRth(rth);
        const std::vector<tbk::Lvl> v = tb.evaluate(now); facts.clear();
        for (const tbk::Lvl& L : v) { sb::Fact x; x.bar = (long long)L.orders; x.lo = x.hi = (double)L.px * tick; x.side = L.side == 0 ? 1 : -1; x.val = (double)L.lots; x.code = L.rank; x.state = 0;
            x.proven = (L.single ? 2 : 0) + (L.s20 ? 4 : 0) + 8 * (L.realPct >= 0 ? L.realPct + 1 : 0); facts.push_back(x); }
        levelRows = 0;                                                         // (1.1.0) level liquidity: not a price the top 3 already show
        if (!levelTicks.empty()) for (const tbk::Lvl& L : tb.levelLiq(now, levelTicks)) { bool dup = false; for (const tbk::Lvl& T : v) if (T.side == L.side && T.px == L.px) dup = true;
            if (dup) continue;
            sb::Fact x; x.bar = (long long)L.orders; x.lo = x.hi = (double)L.px * tick; x.side = L.side == 0 ? 1 : -1; x.val = (double)L.lots; x.code = 7; x.state = L.rank - 100 + 1;
            x.proven = (L.single ? 2 : 0) + (L.s20 ? 4 : 0); facts.push_back(x); ++levelRows; }
        { sb::Fact h; h.code = 99; h.val = (double)tb.orders(); h.state = tb.orders() > 0 ? 1 : 0; facts.push_back(h); }
        haveFacts = true; tbLine = tb.statusLine();
    }
    bool ready() const { return dbo && !dead && !blocked && lastEventMs >= 0 && haveFacts; }
    std::string text(long long now) const {
        if (blocked) return state;
        if (dead) return "order feed fault: reload the indicator";
        if (!dbo) return "not opened";
        if (flag == 0) return "not subscribed";
        if (lastEventMs < 0) return openMs >= 0 && now - openMs >= WAIT_MS && attempt >= 2 ? "no events" : "waiting for the first event";
        return "reading";
    }
};
} // namespace lpf
#endif
