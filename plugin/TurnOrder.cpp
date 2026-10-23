/********************************************************************************
 *  TurnOrder.cpp  --  Investor/RT RTX extension  lsTurnOrder  1.0.0  (2026-10-22)
 *  Rassul 2026-10-22 15:45 CT "I'll go with your recommendation" (MBO Roadmap "Did the limit order at the turn fill, or was it too thick
 *  to get through?", ticket T-20261022-25). THE TURN ORDER: at each local pivot (5-candle extreme low / high that turns) the ONE real
 *  resting limit order that stopped the move - the largest single order (one MBO order id, rested >= 30 s, >= Big Lot S20 or the Min
 *  size setting) at or just beyond the extreme - drawn as ONE short, thick, SOLID bar on the price chart at the order's price, one candle
 *  wide on the turn candle: GREEN = a bid that stopped a selloff, RED = an offer that stopped a rally. Drawn only AFTER the turn is
 *  confirmed (a later candle closed >= 0.5 ATR away from the order; setting), so it always means "this order made the pivot", never
 *  "this might hold" (iteration 1: big size price still sits on is often a trap). One style: FILLED (absorbed) vs TOO THICK, the resting
 *  time and whether it was pulled later are in the HOVER. No text on the chart; at most one per pivot; never repainted (decided once,
 *  recorded to lsFlexLevels\TurnOrder\<MKT>-<spb>-<session>.csv, redrawn from the record after a restart).
 *  Logic: TurnOrderLogic.h (SDK-free, tested). Feed: OrderFlowFeed.h off::Conn (one DEPTH_BY_ORDER per chart, crash guard, 250 ms clock).
 *  Follow-up (not in this build): the LMT / LMT F columns of lsLiquidityProfile highlight the turn order (they can read the record).
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
#include "SettingsGuard.h"
#include "TurnOrderLogic.h"
#include "HostSlot.h"
#define OFF_SDK 1
#include "OrderFlowFeed.h"
#include "LmtZonesLogic.h"      // read-only: the per-market Big Lot S20 of resting orders (lmz::ruleFor), as the LMT zones use it
#include "TurnOrderSettings.h"
#include "SettingsHost.h"
#include "HoverBoard.h"
#include "NewsWindow.h"        // (1.1.0) news / settle window "?" + the feed check (also MarkParams.h: LRA-TurnOrder-<MKT>.csv)
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")   // GetCursorPos / WindowFromPoint (HoverBoard.h, off::cursorWindow)
#endif

static const char* TOR_VERSION = "1.1.1";   // 1.1.0 (2026-10-22 evening, VALIDATE-DEFENDER-SIZE 2026-10-22, Rassul approved 17:39 CT): per-market MARK from lsFlexLevels\LRA-TurnOrder-<MKT>.csv - SHOW = the bar as before, SHOW? = a "?" at the order price (the retest price), LOG = recorded only (the default); DRAW_THICK / DRAW_FILLED choose which results draw (ES: TOO THICK only); a mark decided in a news / settle window gets "?", a failed feed check = recorded only (NewsWindow.h);  // 1.0.0 (2026-10-22 15:45 CT Rassul, T-20261022-25): the TURN ORDER mark
static long long torSteadyMs() { return off::steadyMs(); }   // the order feed's clock (a test clock under OFF_TEST_CLOCK)
static std::string torFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
static void torMakeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
static bool torStat(const std::string& p, long long& sz) { struct stat st; if (p.empty() || stat(p.c_str(), &st) != 0) return false; sz = (long long)st.st_size; return true; }

struct TorState {
    std::string mkt, symbol; int spb = 0; double tick = 0;
    int markMode = mp::LOG; bool drawThick = false, drawFilled = false;   // (1.1.0) LRA-TurnOrder-<MKT>.csv
    off::Conn feed; std::string feedState = "MBO: not opened";
    tor::Engine E;
    long long lastBarMs = 0;             // the last candle (CT ms end) given to the engine
    long newestN = -1; long long newestSeenMs = 0;
    long long stampKey = 0, stampMs = 0, ctOffset = 0; bool haveOffset = false;
    long long session = -1; std::string recPath; long long restored = 0, restoreBad = 0, writes = 0, fails = 0, routed = 0;
    long long feedDropsSeen = 0, feedReconnectsSeen = 0;   // explicit off::Conn discontinuities already handled by E
    std::vector<std::string> pendingRows;
    long long s20 = 0; std::string s20Src = "-";
    ss::Store setStore; ss::Values set = ss::defaults(torset::spec(), -1); long long setCheckMs = -100000;
    int timerId = 0; bool timerOn = false, timerTried = false, busy = false;
    struct Hit { size_t idx; int l, t, r, b; };
    std::vector<Hit> hits; size_t drawnNow = 0;
    bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
    std::string statusFixed; long long statusMs = -100000;
};
static ss::Ids& torIds() { static ss::Ids I; return I; }

class TurnOrder : public cppExtension {
public:
    SS_SAFE_DIALOG(TurnOrder, "TurnOrder")
    HostSlot<TorState> slot_;
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int timer(RTX_EVENT* e);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &TurnOrder::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &TurnOrder::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &TurnOrder::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, torset::spec(), torIds(), st->load(ss::marketIndex(dlgMkt_), torSteadyMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, torset::spec(), torIds(), (int)i, snap_); saveSettings(); }
    TorState* stateFor() { return slot_.get(this, false); }   // tests
    void pump();
    void release() { TorState* S = slot_.get(this, false); if (S) stopTimer(*S); slot_.release(this); }
    static std::map<int, TorState*>& timers() { static std::map<int, TorState*> m; return m; }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = ss::safeMarket(this); const std::string p = torset::settingsPath(torFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &torset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, torset::spec(), torIds(), st->load(ss::marketIndex(dlgMkt_), torSteadyMs(), true), snap_);
        torMakeDir(torFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, torset::spec(), torIds(), V));
    }
    long long ctSec(RTDATE d) {
        struct tm t; std::memset(&t, 0, sizeof(t));
        if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
        return off::civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
    }
    static long long sessionOf(long long ctMs) {   // the trading day (17:00 CT starts the next one) as YYYYMMDD
        const long long d = (ctMs / 1000 + 7 * 3600) / 86400; int y = 0; unsigned m = 0, dd = 0; off::civilFrom(d, y, m, dd); return (long long)y * 10000 + m * 100 + dd;
    }
    static long long sessionDaysBack(long long key, int n) { const int y = (int)(key / 10000); const unsigned m = (unsigned)(key / 100 % 100), d = (unsigned)(key % 100);
        long long z = off::civilDays(y, m, d) - n; int yy = 0; unsigned mm = 0, dd = 0; off::civilFrom(z, yy, mm, dd); return (long long)yy * 10000 + mm * 100 + dd; }
    std::string recPathOf(const TorState& S, long long session) const { const std::string f = torFlex(); return f.empty() ? std::string() : f + "\\TurnOrder\\" + S.mkt + "-" + std::to_string(S.spb) + "-" + std::to_string(session) + ".csv"; }
    bool identify(TorState& S);
    void settings(TorState& S, long long now);
    void feedIntegrity(TorState& S);
    void route(TorState& S, const DEPTH_ORDER& o, long long arrMs);
    void bars(TorState& S, long long now);
    void flushRows(TorState& S);
    void restore(TorState& S, long long session);
    void writeStatus(TorState& S, long long now);
    void stopTimer(TorState& S) { if (S.timerOn) { destroyTimer(S.timerId); S.timerOn = false; } if (S.timerId) timers().erase(S.timerId); S.timerId = 0; }
    void startTimer(TorState& S) {
        if (S.timerTried || S.timerOn || !S.feed.named() || S.feed.flag != 1 || S.feed.timerOff) return;
        S.timerTried = true;
        int id = 41000 + (int)(((uintptr_t)&S >> 4) % 900000);
        while (timers().count(id)) ++id;
        if (createTimer(id, (int)off::TIMER_MS) == RTX_OK) { S.timerId = id; S.timerOn = true; timers()[id] = &S; }
    }
};

bool TurnOrder::identify(TorState& S) {
    const std::string mkt = ss::safeMarket(this); const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR);                       // checklist #24: the chart's own tick
    if (!tor::finiteD(tick) || tick <= 0 || tick >= 1000) tick = 0; else tick = std::round(tick * 1e8) / 1e8;
    if (mkt.empty() || sym.empty() || spb <= 0 || !(tick > 0)) return false;
    if (mkt != S.mkt || sym != S.symbol || spb != S.spb || tick != S.tick) {   // a new chart identity: everything starts over
        stopTimer(S); S.feed.reset(); S.E = tor::Engine(); S.lastBarMs = 0; S.newestN = -1; S.session = -1; S.recPath.clear(); S.pendingRows.clear();
        S.restored = S.restoreBad = 0; S.feedDropsSeen = S.feed.qDropped; S.feedReconnectsSeen = S.feed.reconnects; S.timerTried = false; S.hits.clear(); S.hovKey.clear(); S.statusFixed.clear(); S.setCheckMs = -100000;
        S.mkt = mkt; S.symbol = sym; S.spb = spb; S.tick = tick; S.E.tick = tick; S.feedState = "MBO: not opened";
    }
    return true;
}
void TurnOrder::settings(TorState& S, long long now) {
    if (now - S.setCheckMs < 2000 && now >= S.setCheckMs) return;
    S.setCheckMs = now;
    const std::string p = torset::settingsPath(torFlex(), S.mkt);
    if (S.setStore.path != p) { S.setStore = ss::Store(); S.setStore.S = &torset::spec(); S.setStore.path = p; }
    S.set = p.empty() ? ss::defaults(torset::spec(), ss::marketIndex(S.mkt)) : S.setStore.load(ss::marketIndex(S.mkt), now);
    const double g = S.set.f(torset::K_GATE_ATR); S.E.P.gateAtr = tor::finiteD(g) && g >= 0.1 && g <= 3.0 ? g : 0.5;
}
// off::Conn exposes only explicit loss/reconnect signals. Do not bridge either with a stale order-id book: a missing fill or pull
// would make FILLED / TOO THICK a partial-data assertion. The engine records any selected pending pivot as a no-mark and re-warms.
void TurnOrder::feedIntegrity(TorState& S) {
    const bool dropped = S.feed.qDropped > S.feedDropsSeen, reconnected = S.feed.reconnects > S.feedReconnectsSeen;
    if (dropped || reconnected) {
        S.E.onFeedGap();
        S.feedState = dropped ? "MBO gap: queue dropped events; re-warming 30 s" : "MBO gap: feed reconnected; re-warming 30 s";
    }
    S.feedDropsSeen = S.feed.qDropped; S.feedReconnectsSeen = S.feed.reconnects;
}
// one order-feed event -> the engine, on the Central clock (the event's own second; arrival smoothing is not needed at 30 s rests)
void TurnOrder::route(TorState& S, const DEPTH_ORDER& o, long long arrMs) {
    const off::Ev ev = off::evOf(o, S.tick, arrMs);
    if (ev.px <= 0 || ev.action < off::NEW || ev.action > off::FILL) return;
    long long ms = 0;
    if (ev.stamp > 0 && ev.stamp == S.stampKey) ms = S.stampMs;
    else { ms = ev.stamp > 0 ? ctSec((RTDATE)ev.stamp) * 1000LL : 0; if (ms > 0) { S.stampKey = ev.stamp; S.stampMs = ms; } }
    if (ms <= 0) { if (!S.haveOffset) return; ms = arrMs + S.ctOffset; }
    S.E.onEvent(ev.action, ev.side, ev.px, ev.size, ev.id, ms); ++S.routed;
}
// the closed candles, in order, once each; the newest one 3 s after it closed (or once the feed has passed its end)
void TurnOrder::bars(TorState& S, long long now) {
    const long n = getBarCount(); if (n < 3) return;
    RTARRAYI dtA(barDateTime); RTARRAY oA(barOpen), hA(barHigh), lA(barLow), cA(barClose);
    if (dtA.count < n || oA.count < n || hA.count < n || lA.count < n || cA.count < n) return;
    if (n != S.newestN) { S.newestN = n; S.newestSeenMs = now; }
    { const long long ct = ctSec(currentDate()) * 1000LL; if (ct > 0 && (!S.haveOffset || std::llabs(now + S.ctOffset - ct) > 2000)) { S.ctOffset = ct - now; S.haveOffset = true; } }
    const lmz::Rule R = lmz::ruleFor(S.mkt);
    const long long minSet = (long long)S.set.i(torset::K_MIN_SIZE);
    for (long i = std::max(0L, n - 3000); i <= n - 2; ++i) {
        const long long endMs = ctSec((RTDATE)dtA[(int)i]) * 1000LL; if (endMs <= S.lastBarMs || endMs <= 0) continue;
        if (i == n - 2 && !(S.E.lastT >= endMs || now - S.newestSeenMs >= 3000)) break;   // the feed has not caught up with this close yet
        tor::Bar b; b.endMs = endMs; b.startMs = endMs - (long long)S.spb * 1000LL;
        if (!off::toTick((double)oA[(int)i], S.tick, b.o) || !off::toTick((double)hA[(int)i], S.tick, b.h) || !off::toTick((double)lA[(int)i], S.tick, b.l) || !off::toTick((double)cA[(int)i], S.tick, b.c) || b.h < b.l) continue;
        const long long ses = sessionOf(endMs);
        if (ses != S.session) { flushRows(S); restore(S, ses); }
        const int mod = (int)(((endMs / 60000LL) % 1440LL + 1440LL) % 1440LL);
        const bool rth = mod > 8 * 60 + 30 && mod <= 15 * 60;
        if (minSet > 0) { S.s20 = minSet; S.s20Src = "setting"; } else { S.s20 = rth ? R.s20Rth : R.s20On; S.s20Src = rth ? "Big Lot S20 RTH" : "Big Lot S20 overnight"; }
        S.E.P.minSize = std::max(1LL, S.s20);
        S.E.closeBar(b); S.lastBarMs = endMs;
        for (size_t k = 0; k < S.E.rows.size(); ++k) S.pendingRows.push_back(S.E.rows[k]);
        S.E.rows.clear();
    }
    for (size_t k = 0; k < S.E.rows.size(); ++k) S.pendingRows.push_back(S.E.rows[k]);   // P lines from the feed
    S.E.rows.clear();
    flushRows(S);
}
// the session record: read once per session (this one and the 3 before, for the marks still on the chart)
void TurnOrder::restore(TorState& S, long long session) {
    S.session = session; S.recPath = recPathOf(S, session);
    for (int back = 3; back >= 0; --back) {
        const std::string p = recPathOf(S, sessionDaysBack(session, back)); long long sz = 0;
        if (p.empty() || !torStat(p, sz) || sz > 4 * 1024 * 1024) continue;
        std::ifstream in(p.c_str(), std::ios::binary); std::string ln; std::vector<std::string> v;
        while (std::getline(in, ln) && v.size() < 20000) v.push_back(ln);
        long long bad = 0; S.restored += (long long)S.E.restore(v, &bad); S.restoreBad += bad;
    }
}
// append-only, a line counts as written only after a flushed good append; failures retry on the next pass
void TurnOrder::flushRows(TorState& S) {
    if (S.pendingRows.empty() || S.recPath.empty()) return;
    const std::string f = torFlex(); torMakeDir(f); torMakeDir(f + "\\TurnOrder");
    long long sz = 0; const bool fresh = !torStat(S.recPath, sz) || sz == 0;
    std::ofstream o(S.recPath.c_str(), std::ios::binary | std::ios::app);
    if (!o.is_open()) { ++S.fails; return; }
    if (fresh) o << tor::Engine::header() << "\n";
    for (size_t k = 0; k < S.pendingRows.size(); ++k) o << S.pendingRows[k] << "\n";
    o.flush();
    if (o.good()) { S.writes += (long long)S.pendingRows.size(); S.pendingRows.clear(); } else ++S.fails;
}
void TurnOrder::writeStatus(TorState& S, long long now) {
    const std::string d = torFlex(); if (d.empty() || S.mkt.empty()) return;
    const tor::Tally& T = S.E.tally;
    std::ostringstream fx;
    fx << "VERSION," << TOR_VERSION << "\nMARKET," << S.mkt << "\nSYMBOL," << S.symbol << "\nBAR_SECONDS," << S.spb << "\nLOGIC," << tor::VERSION
       << "\nMARKS," << S.E.marks.size() << " on record (" << T.marks << " this run: FILLED " << T.absorbed << " TOO THICK " << T.thick << "),drawn now " << S.drawnNow
       << "\nPIVOTS," << T.pivots << " seen,no order " << T.noOrder << ",not confirmed " << T.noConfirm << ",broken first " << T.broken << ",before the feed " << T.noFeed << ",feed gaps " << T.feedGaps
       << "\nSETTINGS," << S.set.why << ",show " << (S.set.b(torset::K_SHOW) ? "on" : "off") << ",param MARK " << mp::modeName(S.markMode) << " (thick " << (S.drawThick ? 1 : 0) << ", filled " << (S.drawFilled ? 1 : 0) << ")" << ",gate " << S.E.P.gateAtr << " ATR,min size " << S.s20 << " (" << S.s20Src << ")"
       << "\nRECORD," << S.recPath << ",restored " << S.restored << " (" << S.restoreBad << " unreadable),written " << S.writes << ",fails " << S.fails << "\n";
    const std::string fixed = fx.str();
    if (fixed == S.statusFixed && now - S.statusMs < 30000 && now >= S.statusMs) return;
    std::ostringstream o;
    o << fixed << "FEED," << S.feed.stateText(now) << "," << S.feedState << ",via " << S.feed.via << ",events " << T.events << " routed " << S.routed << ",clock " << (S.timerOn ? "on" : "off")
      << ",faults " << S.feed.faults << ",unknown fills " << T.unknownFill << "\nBOOK," << S.E.live.size() << " live orders," << S.E.gone.size() << " big ended kept,pruned " << T.pruned << "\n";
    std::ofstream f((d + "\\TurnOrder.status-" + S.mkt + "-" + std::to_string(S.spb) + ".txt").c_str(), std::ios::binary | std::ios::trunc);
    if (f.is_open()) { f << o.str(); S.statusFixed = fixed; S.statusMs = now; }
}

void TurnOrder::pump() {
    TorState* P = slot_.get(this, true); if (!P || P->busy) return;
    TorState& S = *P; S.busy = true;
    struct Unbusy { TorState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        if (!identify(S)) return;
        const long long now = torSteadyMs();
        settings(S, now);
        const std::string f = torFlex();
        if (!f.empty()) { torMakeDir(f); torMakeDir(f + "\\TurnOrder"); }
        { const long n = getBarCount(); RTARRAYI vA(barVolume); if (n > 0 && vA.count >= n) S.feed.watch(now, (long long)n * 1000000007LL + (long long)vA[(int)n - 1]); }
        feedIntegrity(S);
        S.feed.open(S.symbol, f.empty() ? std::string() : f + "\\TurnOrder\\_opening-dbo-" + S.mkt + "-" + std::to_string(S.spb) + ".txt", now, S.feedState);
        S.feed.flushQueue([&](const DEPTH_ORDER& o, long long ms) { route(S, o, ms); });
        S.feed.drain(now, torSteadyMs, [&](const DEPTH_ORDER& o) { route(S, o, now); });
        startTimer(S);
        bars(S, now);
        writeStatus(S, now);
    } catch (...) { S.feedState = "pump error"; }
}

int TurnOrder::timer(RTX_EVENT* e) {
    if (!e) return RTX_FAIL;
    TorState* S = nullptr;
    { std::map<int, TorState*>::const_iterator it = timers().find(e->v.timer.id); if (it != timers().end()) S = it->second; }
    if (!S) return RTX_FAIL;                                               // not ours
    if (S->busy) return RTX_OK;
    S->busy = true;
    struct Unbusy { TorState* s; ~Unbusy() { s->busy = false; } } unbusy = { S };
    try { S->feed.prefetch(torSteadyMs(), torSteadyMs); if (S->feed.timerOff) stopTimer(*S); } catch (...) {}
    return RTX_OK;
}

int TurnOrder::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    TorState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (S->hits.empty() && S->hovKey.empty()) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (size_t i = 0; i < S->hits.size(); ++i) { const TorState::Hit& h = S->hits[i]; if (S->msH >= h.l && S->msH <= h.r && S->msV >= h.t && S->msV <= h.b) { k = std::to_string(h.idx); break; } }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;                                                       // never swallows the event
}

int TurnOrder::draw(void) {
    pump();
    TorState* s = slot_.get(this, false); if (!s || s->mkt.empty()) return RTX_OK;
    try {
        TorState& S = *s; S.hits.clear(); S.drawnNow = 0;
        if (!S.set.b(torset::K_SHOW)) return RTX_OK;
        const mp::Params TP = mp::get("TurnOrder", S.mkt);                 // (1.1.0) the nightly's per-market verdict
        S.markMode = mp::mode(TP, "MARK", mp::LOG); S.drawThick = mp::num(TP, "DRAW_THICK", 0, 0, 1) >= 0.5; S.drawFilled = mp::num(TP, "DRAW_FILLED", 0, 0, 1) >= 0.5;
        if (S.markMode == mp::LOG) return RTX_OK;                        // LOG ONLY: recorded, nothing drawn
        const long n = getBarCount(); if (n < 2) return RTX_OK;
        int v0 = 0, v1 = (int)n - 1;
        if (getVisibleBars(&v0, &v1) != RTX_OK) { v0 = std::max(0, (int)n - 200); v1 = (int)n - 1; }
        if (v1 >= n) { v1 = (int)n - 1; }
        if (v0 < 0) { v0 = 0; }
        RTARRAYI dtA(barDateTime); if (dtA.count < n) return RTX_OK;
        std::map<long long, int> idx;                                    // candle end (CT ms) -> index, visible candles only
        for (int i = v0; i <= v1; ++i) { const long long e = ctSec((RTDATE)dtA[i]) * 1000LL; if (e > 0) idx[e] = i; }
        int ppb = getPixelsPerBar(); if (ppb < 1) ppb = 1;
        const int w = std::max(8, ppb), th = 4;                          // about one candle wide, 4 px thick
        const COLOR cBid = (COLOR)S.set.c(torset::K_COL_BID), cOff = (COLOR)S.set.c(torset::K_COL_OFFER);
        for (size_t k = 0; k < S.E.marks.size(); ++k) {
            const tor::Mark& m = S.E.marks[k];
            if (m.absorbed ? !S.drawFilled : !S.drawThick) continue;            // (1.1.0) which results this market draws (ES: TOO THICK only)
            std::map<long long, int>::const_iterator it = idx.find(m.pivotEnd); if (it == idx.end()) continue;
            const nw::Gate ng = nw::atCtSec(S.mkt, m.decideMs > 0 ? m.decideMs / 1000 : m.pivotEnd / 1000); if (ng.feedBad) continue;   // (1.1.0) feed check failed = recorded only
            PNT p; if (p.set(it->second, (float)((double)m.px * S.tick), kBarCenter) != RTX_OK) continue;
            if (S.markMode == mp::SHOWQ || ng.news) {                           // (1.1.0) SHOW? / news window: "?" at the order (retest) price, in the side's colour
                FONT qf; qf.id = HELVETICA; qf.size = 11; qf.style = BOLD; setFont(qf);
                const int tw = (int)getTextWidth("?", -1) + 4, l = p.h - tw / 2, t = p.v - 8;
                setTextColor(m.side == tor::BID ? cBid : cOff); RCT qr; qr.set((short)l, (short)t, (short)(l + tw), (short)(t + 16)); qr.drawText("?", true, false);
                S.hits.push_back(TorState::Hit{ k, l - 2, t - 2, l + tw + 2, t + 18 }); ++S.drawnNow; continue;
            }
            const int l = p.h - w / 2, r = l + w, t = p.v - th / 2, b = t + th;
            off::fillFlat(this, l, t, r, b, m.side == tor::BID ? cBid : cOff);   // solid, no outline
            S.hits.push_back(TorState::Hit{ k, l - 2, t - 3, r + 2, b + 3 }); ++S.drawnNow;
        }
        // the hover: only while the cursor is over this chart's window and on a drawn mark
        if (!S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) return RTX_OK;
        const TorState::Hit* H = nullptr;
        for (size_t k = 0; k < S.hits.size(); ++k) { const TorState::Hit& h = S.hits[k]; if (S.msH >= h.l && S.msH <= h.r && S.msV >= h.t && S.msV <= h.b) { H = &h; break; } }
        if (!H) return RTX_OK;
        const tor::Mark& m = S.E.marks[H->idx];
        long long liveSize = 0; { auto lt = S.E.live.find(m.oid); if (lt != S.E.live.end() && lt->second.px == m.px) liveSize = lt->second.size; }
        const std::vector<tor::Row> L = tor::hoverRows(m, S.tick, S.s20, liveSize);
        RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
        int axisR = pane.right; if (scale.left > pane.left && scale.left < pane.right) axisR = scale.left - 2;
        FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
        int bw = 0; for (size_t i = 0; i < L.size(); ++i) bw = std::max(bw, (int)getTextWidth(L[i].text.c_str(), -1));
        bw += 16; const int lh = 15, hh = (int)L.size() * lh + 10;
        int l = S.msH + 14, t = S.msV + 14;
        if (l + bw > axisR - 2) l = S.msH - 14 - bw;
        if (l < pane.left + 2) l = pane.left + 2;
        if (t + hh > pane.bottom - 2) t = pane.bottom - 2 - hh;
        if (t < pane.top + 2) t = pane.top + 2;
        RCT bg; bg.set((short)l, (short)t, (short)(l + bw), (short)(t + hh)); bg.draw(1, (COLOR)0x00606060, (COLOR)0x00FFFFFF, DRAW_OPAQUE, PAT_SOLID);
        for (size_t i = 0; i < L.size(); ++i) {
            const int ink = L[i].ink;
            setTextColor(ink == tor::INK_GREEN ? cBid : ink == tor::INK_RED ? cOff : ink == tor::INK_GREY ? (COLOR)0x00707070 : (COLOR)0x00202020);
            RCT tr; tr.set((short)(l + 8), (short)(t + 5 + (int)i * lh), (short)(l + bw - 4), (short)(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].text.c_str(), false, false);
        }
    } catch (...) {}
    return RTX_OK;
}

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { return RTX_OK; }
int cppExtension::destroy(void) { static_cast<TurnOrder*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, torset::spec(), torIds()); return RTX_OK; }
int cppExtension::calc(int) { static_cast<TurnOrder*>(this)->pump(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    TurnOrder* p = new TurnOrder();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);
    p->setDescription("LRA Turn Order - the resting limit order that made each pivot. At every local pivot (a 5-candle extreme low / high that turns) it finds THE real resting order that stopped the move: the largest single order (one order id, rested 30 s or more, at least the market's Big Lot S20 or the Min size setting) at or just beyond the extreme. It is drawn as ONE short, thick bar at that order's price on the turn candle: GREEN = a bid that stopped a selloff, RED = an offer that stopped a rally. It is drawn only AFTER the turn is confirmed (a later candle closed 0.5 ATR or more away from the order - setting), so the bar always means 'this order made the pivot', never 'this might hold'. Hover the bar for the size, how long it rested, FILLED (it absorbed the attack) or TOO THICK (barely touched), whether it was pulled later and how far price turned. One per pivot, never repainted; each mark is recorded to lsFlexLevels\\TurnOrder for the nightly. Example: ES Oct 7, the 11:03 AM candle low - a green bar at 7838.00 = a 100-lot bid that filled all 100 and held; price turned 1.6 ATR. Needs the order-by-order (MBO) feed; marks start once the indicator has watched the book.");
    p->setVersion("1.1.1");   // TOR_VERSION
    return p;
}
