/*
 *  RevRead.cpp  --  Investor/RT RTX extension  lsRevRead  "Reversal Read"  0.7.0  (2026-11-13)
 *
 *  Rassul 2026-11-11 21:13-21:33 CT: "I want to know how i can have indicators that tell this story so i can understand how and why it
 *  reveresed"; "i have the poc of the candle in a pane. i want you to create a similar pane that has each candles poc and also makes a dash
 *  ont he candle. in the indicator pane you can place the read as well as mark the candles with letters like c for climax, whether it is a
 *  buying or selling climax"; "it must be added to the nightly refinement"; "jumpstart this indicator for all markets and refine"; "pay
 *  attention to session, volatility times, location and context"; "key levels, volume levels, gamma levels, vwap, hbo and its tests (like the
 *  testing line), whether the market has a pb or its variant ... the hourly rotation and hbo and whether its pulling back, testing or going
 *  higher with the rotation"; "pay attention to the location of the poc in the candle"; 21:33 "go".
 *
 *  lsRevRead (this file)       - its own pane: one bar per candle = the lots at that candle's POC (IRT's volume at price), the number on top
 *      (green = more bought at the offer there, red = more sold into the bid), the story letters over the bars (C climax red / green, A
 *      absorbed, E exhausted, I initiation; no "?" - his choice 22:19 CT) and the read line of the newest story at the top.
 *      The story comes from the market-by-order feed in 10-second blocks (RevReadLogic.h); the context from the chart itself: session,
 *      volatility window, the hourly steps (lsStepTrend's rule: rotation, the HBO test line, pulling back / testing / going with it), the key /
 *      MenthorQ / volume level files and the session VWAP, and lsBreakout's decided Pb labels (SignalBoard fact E).
 *      Records: lsFlexLevels\RevRead\<MKT>-<spb>-<session>.csv (every step, every measurement; the nightly lra.rev_read scores them).
 *      Thresholds: lsFlexLevels\LRA-RevRead-<MKT>.csv (written by the nightly; defaults when absent).
 *  lsRevReadMarks (RevReadMarks.cpp: #define RVS_MARKS 1 + #include this file) - the price-chart overlay: a white dash on every candle
 *      at its POC price.
 *      (0.4.2, Rassul 2026-11-12 19:20 "can you check the CL because it is not showing") the POC bars / dashes fill from the newest 6000 candles (was: from the
 *      chart's oldest candle, so the candles on screen came last), and a pass runs on while it has used under 10 ms.
 *      (0.4.1, Rassul 2026-11-12 16:33 "tell me about reversal read for hourly how do we get that done"; 16:37 "Ok for hourly reversal read") on a
 *      60-MINUTE (or longer) chart the hold dots are drawn by themselves - the read that was measured on hourly candles (3,709 hourly
 *      extremes, 42 sessions x 7 markets: the extreme held 3 hours 65% at the strong tier against 30% for any; the story letters did not
 *      help on the hourly, so the hourly chart gets the dots only). The checkbox still decides the shorter charts.
 *      (0.4.0, Rassul 2026-11-12 07:07 / 07:12 CT) + the HOLD READ dots, OFF by default (settings: "Hold read dots"): a small red dot over a
 *      high / a green dot under a low that is likely done (rvs::holdOf, the strong tier only: the candle closed well back from its extreme,
 *      on heavy volume / after a long run / back inside the prior high or low). Closed candles only: a dot never changes.
 *      (0.5.0, Rassul 2026-11-13 07:05 "ok build" / 07:09 "the climax waits for the confirmation .. climax on one bar and then after that a green bar occurs
 *      confirming the climax"; 08:14 "buying climax (bearish) should be near the top of a green candle and selling climax (bullish) should be near the
 *      bottom of a red candle and both cases should be confirmed by the next bar") the letters of a story appear only when its climax is in the top
 *      third of a green candle / the bottom third of a red one (CX_LOC) and the NEXT candle closes beyond the climax candle's close the story's way
 *      (CX_CONFIRM 1); I (initiation) is the signal - bold and bright; C / A / E are drawn plain and dim; stories that begin as a test of a level with
 *      no climax are recorded only (LEVEL_SHOW); "Stop run of <level>" in the read is now "Flush / Push through <level>" (the stop run is SR's).
 *      (0.7.0, Rassul 2026-11-13 22:05-22:23 CT "this will be our reversal framework for local pivots", "make sure you can identify all of this in
 *      realtime", "go , but you must allow for variability and discovery as well") THE SIX CHECKS (rvs::Six): Buying fading, Big buyer -> Buyer trapped,
 *      Quick rejection, Bids pulled, Big seller, With trend - computed live from the order feed for the newest pullback lsBreakout decided (fact E:
 *      side + the pivot extreme) and, once lsBreakout posts its Rev candles (fact W: lo = the Rev candle, hi = the pivot candle, raw bar stamps),
 *      decided once at the Rev candle's close, recorded ("R6|" lines: raw values + candidate reads) and posted back as fact X (bar = the Rev candle's
 *      raw stamp, code = count x 1000 + check bits + 64 trapped, lo / hi = the big buyer's / big seller's price, val = the prior candle's extreme) for
 *      the "Rev N" label and the BT / P / S marks. The read on the right = "Rev N" + six lines, a green check / red x each; a white ratio (3.5x) over
 *      the volume bar of the big buyer / big seller. Cut-offs: lsFlexLevels\LRA-RevSix-<MKT>.csv (defaults = Nov 9-11).
 *  lsRevRead has no settings; lsRevReadMarks has two switches (POC dash, Hold read dots). No hovers, no text on the price chart. Order-feed letters exist from the moment the pane is on the chart (IRT has no
 *  order-by-order history); decided letters are reloaded from the session record after a restart. The POC bars / dashes cover all bars.
 */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifndef LSPERF_PLUGIN
#ifdef RVS_MARKS
#define LSPERF_PLUGIN "RevReadMarks"
#else
#define LSPERF_PLUGIN "RevRead"
#endif
#endif
#include "PerfGuard.h"
#ifdef RVS_MARKS
#include "SettingsGuard.h"
#endif
#include "HostSlot.h"
#ifdef RVS_MARKS
#include "SettingsSpec.h"
#include "SettingsHost.h"
#include "RevReadSettings.h"
#include <chrono>
#endif
#include "MarketKey.h"
#include "NearLevels.h"
#include "SignalBoard.h"
#include "StepTrendLogic.h"
#include "VwapTrendLogic.h"
#include "RevReadLogic.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#define RVS_VERSION "0.7.0"   // = rvs::VERSION = the one setVersion literal (rvh::stamp)

namespace rvh {
inline void stamp(cppExtension* p) {   // the literal on its own line: gex-build reads the text between the first ( and )
    p->setVersion("0.7.0");
}
inline short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
inline std::string flexDir() { const char* u = std::getenv("USERPROFILE"); return u && *u ? std::string(u) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
inline bool known(const std::string& m) { static const char* const K[] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" }; for (const char* k : K) if (m == k) return true; return false; }
inline std::string upper(std::string r) { for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]); return r; }
inline std::string marketOf(const std::string& root, const std::string& symbol = std::string()) {
    std::string m = mkey::norm(upper(root)); if (known(m)) return m;
    m = mkey::norm(upper(symbol)); if (known(m)) return m;
    const std::string s = upper(symbol); if (s.size() > 3) { m = mkey::norm(s.substr(0, s.size() - 2)); if (known(m)) return m; }
    return std::string();
}
// the session a local time belongs to: 17:00 CT starts the next trade date
inline std::string sessionOf(const struct tm& t) {
    struct tm x = t; x.tm_isdst = -1; if (x.tm_hour >= 17) x.tm_mday += 1; x.tm_hour = 12; x.tm_min = 0; x.tm_sec = 0;
    std::time_t tt = std::mktime(&x); struct tm y; std::memset(&y, 0, sizeof y);
#if defined(_WIN32)
    localtime_s(&y, &tt);
#else
    localtime_r(&tt, &y);
#endif
    char b[40]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", y.tm_year + 1900, y.tm_mon + 1, y.tm_mday); return b;
}
// RTH open / close (Central minutes) per market, for the session name and the volatility windows
inline void rthOf(const std::string& m, int& o, int& c) {
    if (m == "ES" || m == "NQ") { o = 510; c = 900; } else if (m == "CL" || m == "NG") { o = 480; c = 870; } else if (m == "GC") { o = 440; c = 810; } else if (m == "HG") { o = 440; c = 780; } else { o = 440; c = 840; }
}
inline std::string recDir(const std::string& flex) { return flex.empty() ? std::string() : flex + "\\RevRead"; }
inline std::string recPath(const std::string& flex, const std::string& mkt, int spb, const std::string& session) {
    return flex.empty() || mkt.empty() || spb <= 0 || session.empty() ? std::string() : recDir(flex) + "\\" + mkt + "-" + std::to_string(spb) + "-" + session + ".csv";
}
// a small text file, bounded (params / the session record's letter lines)
inline bool readLines(const std::string& path, std::vector<std::string>& out, size_t maxBytes) {
    out.clear(); std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
    std::string all; all.reserve(4096); char buf[4096];
    while (in.good() && all.size() < maxBytes) { in.read(buf, sizeof buf); const std::streamsize n = in.gcount(); if (n <= 0) break; all.append(buf, (size_t)n); }
    size_t a = 0, nl; while ((nl = all.find('\n', a)) != std::string::npos) { std::string l = all.substr(a, nl - a); if (!l.empty() && l.back() == '\r') l.pop_back(); out.push_back(l); a = nl + 1; }
    return true;
}
static const COLOR C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_WHITE = 0x00E5E7EB, C_GRAY = 0x009CA3AF, C_DIM = 0x006B7280, C_BAR_GREEN = 0x00178A42, C_BAR_RED = 0x00B03A3A, C_BAR_FLAT = 0x004B5563, C_DASH = 0x00FFFFFF;
// SEH-guarded VAP reads (as lsCandleAbsorb / lsWavePOC): -1 = a fault (never read again on this chart), -2 = no data
typedef cppExtension::RTARRAYP RTARRAYP; typedef cppExtension::BARSTATISTICS BARSTATISTICS; typedef cppExtension::VOLPROFILE VOLPROFILE;
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
// one bar's volume-at-price rows; 1 = read, 0 = no VAP for the bar, -1 = fault. pv = sum(price in ticks x volume) for the VWAP
inline int vapRows(RTARRAYP& vap, int chartBar, double tick, std::vector<rvs::VRow>& out, double& pv, long long* dmm = nullptr) {   // dmm (0.6.0): [0] the bar's max delta, [1] its min delta
    out.clear(); pv = 0; if (dmm) { dmm[0] = 0; dmm[1] = 0; }
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) return -1;
    if (prices <= 0 || prices > 100000) return 0;
    if (dmm) { dmm[0] = (long long)st.maxDelta; dmm[1] = (long long)st.minDelta; }
    std::vector<VOLPROFILE> rv((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rv[0], prices);
    if (got == -1) return -1;
    if (got < 0) return 0;
    for (const VOLPROFILE& v : rv) {
        if (!std::isfinite((double)v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.buyVolume + v.sellVolume <= 0) continue;
        const double q = (double)v.price / tick; if (!std::isfinite(q) || std::fabs(q) > 4e15) continue;
        rvs::VRow r; r.tick = (long long)std::llround(q); r.vol = (long long)v.buyVolume + (long long)v.sellVolume; r.dlt = (long long)v.buyVolume - (long long)v.sellVolume;
        out.push_back(r); pv += (double)r.tick * (double)r.vol;
    }
    return out.empty() ? 0 : 1;
}
// the candle POC store of one chart (both plugins): candle end -> Poc; read in calc only, a capped number of bars per pass
struct PocStore {
    std::map<long long, rvs::Poc> poc; std::map<long long, double> pv; int next = 0; long seenCount = -1; bool fault = false; long long missing = 0; long long lastKey = 0;
    void reset() { poc.clear(); pv.clear(); next = 0; seenCount = -1; missing = 0; lastKey = 0; }
    // (0.4.2, Rassul 2026-11-12 19:20 "can you check the CL because it is not showing": the pane filled from the chart's OLDEST candle,
    // 300 a pass, so the candles on screen came last - minutes on a quiet evening) only the newest KEEP candles are ever kept, so the
    // fill starts there; and a pass may run on past 300 candles while it has used under 10 ms (at most 3000).
    static const int KEEP = 6000;
    void start(long count) { if (next == 0 && lastKey == 0 && count - 1 > KEEP) next = (int)(count - 1 - KEEP); }
    static long long nowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
    static bool more(int done, long long t0) { return done < 300 || (done < 3000 && nowMs() - t0 < 10); }
};
} // namespace rvh

#ifndef RVS_MARKS
// =====================================================================================================================
// lsRevRead - the pane
// =====================================================================================================================
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#define OFF_SDK 1
#include "OrderFlowFeed.h"
namespace rvs {
struct PaneLogic {   // the MboPane contract (the base's own bars / records are not drawn by this pane)
    static const char* name() { return "RevRead"; }
    static const char* title() { return "Reversal Read"; }
    static const char* version() { return RVS_VERSION; }
    static int metrics() { return 3; }
    static int paceMetric() { return off::PM_ADD; }
    static int optGate() { return ofo::G_OSTACK; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::stackLine(s, gate); }
    static bool resets() { return false; }
    static std::string hoverQuestion() { return "Why did it reverse?"; }
    static const char* const* hoverVerbs() { static const char* const v[2] = { "Buyers", "Sellers" }; return v; }
    static off::Info info(const off::PaneBar*, const off::Params&) { return off::Info(); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addC[s] : metric == 1 ? r.pullC[s] : r.refN[s]; }
    static void decide(off::PaneBar& b, const off::Params&, bool) { for (int s = 0; s < 2; ++s) { b.spike[s] = false; b.dash[s] = -1; b.let[s].clear(); } b.msg = decltype(b.msg)(); }
};
} // namespace rvs

class RevRead : public off::MboPane<rvs::PaneLogic> {
public:
    RevRead() : off::MboPane<rvs::PaneLogic>() {}
    virtual bool barMarks() const { return false; }
    virtual bool textOff() const { return true; }
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, rvs::PaneLogic::title(), RVS_VERSION) ? RTX_OK : RTX_FAIL; }
    struct Ext : off::PaneExt {
        std::unique_ptr<rvs::Engine> eng; std::string mkt, session, path, note, symbol; int spb = 0; double tick = 0;
        size_t written = 0; long long writeFails = 0, events = 0, skipped = 0, sessions = 0, reconnects = -1, marksLoaded = 0;
        long long stamp = -1; int ctSec = 0; long long localSec = 0; int rthO = 510, rthC = 900;
        rvh::PocStore ps; long long fedCandle = 0; bool aligned = true;
        double vwPV = 0, vwV = 0; std::string vwSession; long long vwTick = 0;
        nl::Cache lvc; long long lvlMs = -100000; size_t nLevels = 0;
        int hDir = 0, hRot = 0; long long hPx = 0, hTop = 0; bool hBroken = false; long hCount = -1;
        long long lastPx = 0, barLo = 0, barHi = 0;                       // the forming candle (ticks)
        long long parMs = -100000; long long parSig = -2; int parKeys = 0; int pbBits = 0; long long pbMs = -100000;
        long long drawn = 0; std::string lastRead;
        // (0.7.0) the six checks
        rvs::Six six; int sixKeys = 0; long long sixParMs = -100000, sixParSig = -2; int sixLoaded = 0, sixDecided = 0;
        int pvDir = 0; long long pvTp = 0, pvExt = 0; std::map<long long, int> trendAt; int trendNow = 0; std::vector<sb::Fact> revF; long long sixPostMs = -100000;
        rvs::SixRead live; bool haveLive = false;
    };
    Ext* ext(S_& S) { if (!S.ext) S.ext.reset(new (std::nothrow) Ext()); return static_cast<Ext*>(S.ext.get()); }

    // ---- the feed: every order event, once, in order ------------------------------------------------------------------
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        Ext* e = ext(S); if (!e) return;
        const std::string mkt = rvh::marketOf(S.market);
        if (mkt.empty()) { e->note = "market not supported (" + S.market + ")"; ++e->skipped; return; }
        if (S.spb <= 0 || 3600 % (int)S.spb != 0) { e->note = "needs bars that divide the hour"; ++e->skipped; return; }
        if (!(S.tick > 0) || !o.timestamp) { ++e->skipped; return; }
        e->note.clear();
        if ((long long)o.timestamp != e->stamp) {                         // one clock conversion per second
            struct tm t; std::memset(&t, 0, sizeof t);
            if (!getLocaltime(o.timestamp, &t)) { ++e->skipped; return; }
            e->stamp = (long long)o.timestamp; e->ctSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
            const std::string sess = rvh::sessionOf(t);
            if (!e->eng || sess != e->session || mkt != e->mkt || (int)S.spb != e->spb) openSession(S, *e, sess, mkt);
            if (e->eng) { char cb[40]; std::snprintf(cb, sizeof cb, "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); e->eng->clock = cb; context(S, *e); }
        }
        if (!e->eng) return;
        if (e->reconnects < 0) { e->reconnects = S.feed.reconnects; e->symbol = S.symbol; }
        if (S.feed.reconnects != e->reconnects || S.symbol != e->symbol) { e->six.reset(); e->eng->reset(); e->reconnects = S.feed.reconnects; e->symbol = S.symbol; }   // the book is re-sent
        const off::Ev x = off::evOf(o, S.tick, 0);
        if (x.action > 3 || x.px <= 0) { ++e->skipped; return; }
        e->six.onEvent(e->stamp, x.action, x.implied ? -1 : x.side, (long long)x.px, x.size, x.id, x.aggr, e->eng->book);   // (0.7.0) before the engine applies it (a MOD needs the order's old price)
        e->eng->onEvent(e->stamp * 1000, x.action, x.implied ? -1 : x.side, (long long)x.px, x.size, x.id, x.aggr); ++e->events;
        if (x.action == off::FILL) e->lastPx = (long long)x.px;
    }
    void openSession(S_& S, Ext& e, const std::string& sess, const std::string& mkt) {
        if (e.eng) flush(e);
        std::unique_ptr<rvs::Engine> old; old.swap(e.eng);
        const bool same = old && e.mkt == mkt && e.spb == (int)S.spb;
        e.mkt = mkt; e.spb = (int)S.spb; e.tick = S.tick; e.session = sess; ++e.sessions; rvh::rthOf(mkt, e.rthO, e.rthC);
        e.eng.reset(new (std::nothrow) rvs::Engine()); if (!e.eng) return;
        if (!same) { e.six = rvs::Six(); e.sixParSig = -2; e.trendAt.clear(); e.haveLive = false; } e.six.spb = e.spb; if (e.sixParSig == -2) e.six.P = rvs::sixFor(mkt);
        e.eng->P = rvs::forMarket(mkt); e.eng->tick = e.tick; e.eng->spb = e.spb; e.written = 0; e.parMs = -100000; e.parSig = -2;
        if (same) { e.eng->book.swap(old->book); e.eng->lvl[0].swap(old->lvl[0]); e.eng->lvl[1].swap(old->lvl[1]); e.eng->hist.swap(old->hist); e.eng->base.swap(old->base); e.eng->p95 = old->p95; e.eng->nextId = old->nextId; }   // the 17:00 roll keeps the book and the normal
        const std::string flex = rvh::flexDir(); rvh::makeDir(flex); rvh::makeDir(rvh::recDir(flex));
        e.path = rvh::recPath(flex, mkt, e.spb, sess);
        std::vector<std::string> L; if (!e.path.empty() && rvh::readLines(e.path, L, 4 * 1024 * 1024)) { e.marksLoaded = e.eng->loadMarks(L); e.eng->lastV = rvs::lastV(L); e.sixLoaded += e.six.load(L); }   // a restart: the letters decided before stay
        if (!e.path.empty()) { struct stat st; if (stat(e.path.c_str(), &st) != 0) { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); f << "H|" << RVS_VERSION << "|" << mkt << "|" << e.spb << "|" << sess << "|tick " << rvs::Engine::num(e.tick) << "\n"; } }
    }
    // the context a story is stamped with when it starts: session, volatility window, the hourly read, levels, Pb (once a second)
    void context(S_& S, Ext& e) {
        rvs::Ctx c; c.session = rvs::sessionOf(e.ctSec, e.rthO, e.rthC); c.volWin = rvs::volWindow(e.ctSec, e.rthO, e.rthC);
        const long long last = e.lastPx > 0 ? e.lastPx : e.barLo; const long long lo = e.barLo > 0 ? std::min(e.barLo, last) : last, hi = e.barHi > 0 ? std::max(e.barHi, last) : last;
        if (last > 0) c.hourly = rvs::hourlyText(e.hDir, e.hRot, e.hPx, e.hTop, e.hBroken, last, lo, hi, e.eng->P.zoneTicks + 2, e.tick);
        const long long now = off::steadyMs();
        if (now - e.pbMs >= 1000 || now < e.pbMs) {                      // lsBreakout's decided Pb / Pboc / Pdoc (fact E), the last 30 minutes
            e.pbMs = now; e.pbBits = 0; const char* sy = getSymbol(); const char* cl = getChartLabel(); std::vector<sb::Fact> F;
            if (sy && cl && sb::readFacts('E', sy, getSecondsPerBar(), cl, now, F, true)) { long long newest = 0; for (const sb::Fact& f : F) newest = std::max(newest, f.bar);
                for (const sb::Fact& f : F) if (newest - f.bar <= 1800) e.pbBits |= f.side > 0 ? 1 : 2;
                e.pvDir = 0; for (const sb::Fact& f : F) if (f.bar == newest && f.side != 0 && std::isfinite(f.hi) && f.hi > 0) { e.pvDir = f.side > 0 ? 1 : -1; e.pvExt = (long long)std::llround(f.hi / e.tick) + (f.side > 0 ? 1 : -1); } }   // (0.7.0) the newest decided pullback: its stop is 1 tick beyond the pivot
            e.revF.clear(); if (sy && cl) sb::readFacts('W', sy, getSecondsPerBar(), cl, now, e.revF, true);                                  // (0.7.0) lsBreakout's Rev candles (when it posts them)
            sixTick(e, sy ? sy : "", cl ? cl : "", now);
        }
        if (now - e.sixParMs >= 30000 || now < e.sixParMs) {             // (0.7.0) the nightly's cut-offs of the six checks
            e.sixParMs = now; const std::string p = rvh::flexDir() + "\\LRA-RevSix-" + e.mkt + ".csv"; struct stat st; const long long sig = stat(p.c_str(), &st) == 0 ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
            if (sig != e.sixParSig) { e.sixParSig = sig; rvs::SixParams P = rvs::sixFor(e.mkt); e.sixKeys = 0; std::vector<std::string> L; if (sig >= 0 && rvh::readLines(p, L, 64 * 1024)) e.sixKeys = rvs::sixParse(L, e.mkt, P); e.six.P = P; }
        }
        c.pb = e.pbBits;
        if (now - e.lvlMs >= 5000 || now < e.lvlMs) {                    // the level files (key / MenthorQ), re-read at most every 5 s, + the session VWAP
            e.lvlMs = now; const std::string flex = rvh::flexDir(); std::vector<std::pair<long long, std::string> > out;
            if (!flex.empty()) { const std::vector<nl::Level> v = nl::levels(e.lvc, flex + "\\", e.mkt, S.symbol, now, (long long)std::time(nullptr));
                for (const nl::Level& l : v) if (l.px > 0 && out.size() < 400) out.push_back(std::make_pair((long long)std::llround(l.px / e.tick), l.name.substr(0, 24))); }
            if (e.vwV > 0) out.push_back(std::make_pair((long long)std::llround(e.vwPV / e.vwV), std::string("VWAP")));
            if (e.hDir != 0 && e.hPx > 0) out.push_back(std::make_pair(e.hPx, std::string("the HBO test line")));
            e.nLevels = out.size(); e.eng->levels.swap(out);
        }
        if (now - e.parMs >= 30000 || now < e.parMs) {                   // the nightly's thresholds (LRA-RevRead-<MKT>.csv), re-read on change
            e.parMs = now; const std::string p = rvh::flexDir() + "\\LRA-RevRead-" + e.mkt + ".csv"; struct stat st; const long long sig = stat(p.c_str(), &st) == 0 ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
            if (sig != e.parSig) { e.parSig = sig; rvs::Params P = rvs::forMarket(e.mkt); e.parKeys = 0; std::vector<std::string> L; if (sig >= 0 && rvh::readLines(p, L, 64 * 1024)) e.parKeys = rvs::parseParams(L, e.mkt, P); e.eng->P = P; }
        }
        e.eng->ctx = c;
    }
    // (0.7.0) once a second: decide the Revs lsBreakout posted, the live read of the newest pullback, and post the reads (fact X)
    int trendOf(const Ext& e, long long candle) const { const auto it = e.trendAt.find(candle); return it != e.trendAt.end() ? it->second : e.trendNow; }
    void sixTick(Ext& e, const std::string& sym, const std::string& chart, long long now) {
        const long long cur = e.six.candleOf(e.stamp);
        for (const sb::Fact& f : e.revF) { const long long tr = (long long)std::llround(f.lo), tp = (long long)std::llround(f.hi); if (f.side == 0 || tr <= 0 || tp <= 0 || tr >= cur || tr < tp || e.six.done.count(tr)) continue;
            if (e.six.decide(f.side > 0 ? 1 : -1, tp, tr, trendOf(e, tp), e.eng ? e.eng->clock : std::string())) ++e.sixDecided; }
        e.haveLive = false;
        if (e.pvDir != 0) { long long tp = 0; int n = 0;                                          // the pivot candle: the newest of the last 10 whose extreme is the pivot (1 tick of tolerance: an unsigned trade may have made it)
            for (auto it = e.six.cn.rbegin(); it != e.six.cn.rend() && n < 10; ++it, ++n) { if (!it->second.any) continue; const long long x = e.pvDir < 0 ? it->second.hi : it->second.lo; if (std::llabs(x - e.pvExt) <= 1) { tp = it->first; break; } }
            e.pvTp = tp; bool printed = false; for (auto it = e.six.done.rbegin(); it != e.six.done.rend() && !printed; ++it) printed = it->second.tp == tp;   /* its Rev is decided: the read is final, no live one */
            if (tp > 0 && !printed) { e.live = e.six.eval(e.pvDir, tp, 0, e.stamp, trendOf(e, tp)); e.haveLive = e.live.ok; } }
        if (now - e.sixPostMs >= 1000 || now < e.sixPostMs) { e.sixPostMs = now; std::vector<sb::Fact> v;
            auto mk = [&](const rvs::SixRead& r, int state) { sb::Fact f; f.bar = r.tr; f.side = r.dir; int bits = 0; for (int i = 0; i < 6; ++i) if (r.c[i]) bits |= 1 << i; if (r.trapped) bits |= 64;
                f.code = r.n * 1000 + bits; f.lo = (double)r.bbPx * e.tick; f.hi = (double)r.bsPx * e.tick; f.val = (double)r.pl * e.tick; f.state = state; f.proven = 0; v.push_back(f); };
            int n = 0; for (auto it = e.six.done.rbegin(); it != e.six.done.rend() && n < 30; ++it, ++n) mk(it->second, 1);
            if (e.haveLive) mk(e.live, 0);
            if (!sym.empty()) sb::postFacts('X', sym, getSecondsPerBar(), chart, now, v); }
        if (e.eng) { for (const std::string& l : e.six.out) e.eng->out.push_back(l); e.six.out.clear(); }
    }
    void flush(Ext& e) {
        if (!e.eng || e.path.empty() || e.written >= e.eng->out.size()) return;
        std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app);
        if (!f.is_open()) { ++e.writeFails; return; }
        for (; e.written < e.eng->out.size(); ++e.written) f << e.eng->out[e.written] << "\n";
        f.flush(); if (!f.good()) ++e.writeFails;
        if (e.eng->out.size() > 20000) { e.eng->out.clear(); e.written = 0; }
    }
    void stopAll() { S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (e) flush(*e); }
    virtual void afterPump(S_& S, long long, bool) { Ext* e = ext(S); if (e) { if (e->eng && e->stamp > 0) e->eng->onClock(e->stamp * 1000); flush(*e); } }
    virtual void afterTimer(S_& S, long long) { Ext* e = ext(S); if (e) flush(*e); }
    virtual void statusExtra(S_& S, std::ostream& o) {
        Ext* e = ext(S); if (!e) { o << "\nREV_READ,unavailable"; return; }
        const rvs::Engine* E = e->eng.get();
        o << "\nREV_READ,version " << RVS_VERSION << ",market " << e->mkt << ",session " << e->session << ",events " << e->events << ",skipped " << e->skipped << (e->note.empty() ? std::string() : ",note " + e->note)
          << ",fills " << (E ? E->fills : 0) << ",side mismatch " << (E ? E->sideMismatch : 0) << ",book " << (E ? E->book.size() : 0) << ",blocks " << (E ? E->hist.size() : 0) << ",normal p95 " << (E ? E->p95 : 0)
          << ",stories " << (E ? E->nextId - 1 : 0) << ",letters " << (E ? E->marks.size() : 0) << " (reloaded " << e->marksLoaded << ")" << ",levels " << e->nLevels << ",params keys " << e->parKeys << ",proven " << (E ? E->P.proven : 0)
          << ",record " << e->path << ",write fails " << e->writeFails
          << "\nREV_READ_CONTEXT," << (E ? E->ctx.session : std::string("?")) << "," << (E && E->ctx.volWin ? rvs::volName(E->ctx.volWin) : "no volatility window") << "," << (E ? E->ctx.hourly : std::string()) << ",pb bits " << e->pbBits
          << ",bar ends aligned " << (e->aligned ? "yes" : "NO - letters off")
          << "\nREV_READ_POC,candles " << e->ps.poc.size() << ",without VAP " << e->ps.missing << (e->ps.fault ? ",VAP FAULT - remove and re-add the indicator" : "")
          << "\nREV_SIX,fills " << e->six.fills << ",candles " << e->six.cn.size() << ",decided " << e->six.done.size() << " (now " << e->sixDecided << ", reloaded " << e->sixLoaded << ")" << ",params keys " << e->sixKeys
          << ",big buyer lots " << e->six.P.bbLots << ",big seller lots " << e->six.P.bsLots << ",rejection x " << e->six.P.rejX << ",pulled x " << e->six.P.pullX << ",fading x " << e->six.P.fadeX
          << ",pullback " << (e->pvDir > 0 ? "long" : e->pvDir < 0 ? "short" : "none") << ",pivot candle " << e->pvTp << ",trend " << e->trendNow << ",Rev facts " << e->revF.size() << ",live " << (e->haveLive ? e->live.n : -1)
          << "\nREV_READ_READ," << e->lastRead << ",draws " << e->drawn;
    }
    // ---- calc: the candles (bars + VAP are read here only) ----------------------------------------------------------------
    void rvCalc() {
        pump();
        S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (!e) return;
        if (S->hov.left()) invalidateChart(false);                         // (0.6.0) the mouse left the chart: back to the newest story
        try { candles(*S, *e); } catch (...) { e->note = "calc error"; }
    }
    void candles(S_& S, Ext& e) {
        const long count = getBarCount(); if (count < 2 || !(S.tick > 0) || S.spb <= 0) return;
        RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose), opn(barOpen);   // (0.5.0) the open: the climax candle's colour
        if (times.count < count || hi.count < count || lo.count < count || cl.count < count) return;
        const double tick = S.tick; rvh::PocStore& P = e.ps;
        if (P.seenCount > count || (P.lastKey != 0 && P.next > 0 && P.next <= count - 1 && (long long)times[P.next - 1] != P.lastKey)) P.reset();   // the chart was reloaded
        P.seenCount = count;
        { const double fl = (double)lo[(int)count - 1], fh = (double)hi[(int)count - 1]; if (std::isfinite(fl) && std::isfinite(fh) && fl > 0) { e.barLo = (long long)std::llround(fl / tick); e.barHi = (long long)std::llround(fh / tick); } }
        e.aligned = ((long long)times[(int)count - 1] % (long long)S.spb) == 0;
        if (!P.fault) {
            RTARRAYP vap(barVolumeProfile); int done = 0; P.start(count); const long long t0 = rvh::PocStore::nowMs();
            for (; P.next < count - 1 && rvh::PocStore::more(done, t0); ++P.next, ++done) {       // closed bars only; 300 a pass, more while under 10 ms (0.4.2)
                const int i = P.next; const long long key = (long long)times[i]; std::vector<rvs::VRow> rows; double pv = 0; long long dmm[2] = { 0, 0 };
                const int r = rvh::vapRows(vap, i, tick, rows, pv, dmm);
                if (r < 0) { P.fault = true; break; }
                const double h = (double)hi[i], l = (double)lo[i], c = (double)cl[i];
                if (r == 0 || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c)) { ++P.missing; P.lastKey = key; continue; }
                const long long ht = (long long)std::llround(h / tick), lt = (long long)std::llround(l / tick), ct = (long long)std::llround(c / tick);
                rvs::Poc p = rvs::pocOf(key, rows, lt, ht, ct); { const double o = opn.count >= count ? (double)opn[i] : 0.0; if (std::isfinite(o) && o > 0) p.open = (long long)std::llround(o / tick); } p.maxD = dmm[0]; p.minD = dmm[1];
                struct tm t; std::memset(&t, 0, sizeof t); const bool tOk = getLocaltime((RTDATE)key - 1, &t) != 0; const std::string cs = tOk ? rvh::sessionOf(t) : std::string(); p.sess = rvs::sessId(cs); rvs::biggestIn(P.poc, p);   // (0.6.0)
                P.poc[key] = p; P.pv[key] = pv; P.lastKey = key; e.six.setTotal(key, p.total);
                while (P.poc.size() > (size_t)rvh::PocStore::KEEP) { P.pv.erase(P.poc.begin()->first); P.poc.erase(P.poc.begin()); }
                // the session VWAP from IRT's own volume at price (17:00 CT starts a new one)
                if (tOk) { const std::string& s = cs; if (s != e.vwSession) { e.vwSession = s; e.vwPV = 0; e.vwV = 0; } e.vwPV += pv; e.vwV += (double)p.total; }
                if (e.eng && e.aligned && key > e.fedCandle) { e.eng->onCandle(key, p, ct); e.fedCandle = key;
                    if (cs == e.session && key > e.eng->lastV) { e.eng->out.push_back(rvs::vLine(p, tick)); e.eng->lastV = key; } }   // (0.6.0) the background record, once per candle of this session   // the letters of that candle are decided now, once
            }
        }
        if (count != e.hCount && P.next >= count - 1) { e.hCount = count; hourly(S, e, times, hi, lo, cl, (int)count); vwTrend(S, e, times, hi, lo, cl, (int)count); }
        if (e.eng) flush(e);
    }
    // lsStepTrend's hourly steps on this chart's closed candles (the same walk as its 60-minute design): rotation, the newest step, its test line
    template <class TA, class FA> void hourly(S_& S, Ext& e, TA& times, FA& hi, FA& lo, FA& cl, int count) {
        e.hDir = 0; e.hRot = 0; e.hBroken = false; if (!stp::periodOk((int)S.spb) || (int)S.spb >= 3600) return;
        RTARRAY op(barOpen); if (op.count < count) return;
        stp::Engine E; const int first = std::max(0, count - 1 - stp::MAX_BARS);
        for (int i = first; i < count - 1; ++i) { stp::Bar b; b.t = (long long)times[i] - (long long)S.spb;
            const double o = (double)op[i], h = (double)hi[i], l = (double)lo[i], c = (double)cl[i]; if (!std::isfinite(o) || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c) || h < l) continue;
            b.o = (long long)std::llround(o / S.tick); b.h = (long long)std::llround(h / S.tick); b.l = (long long)std::llround(l / S.tick); b.c = (long long)std::llround(c / S.tick); E.addBar(b); }
        const int nh = (int)E.H.size() - (E.curClosed ? 0 : 1); if (nh < 2) return;
        stp::Engine X; X.hourly = true;
        for (int j = 0; j < nh; ++j) { const stp::Hour& h = E.H[(size_t)j]; stp::Bar b; b.t = h.k * 3600; b.o = h.o; b.h = h.h; b.l = h.l; b.c = h.c; X.addBar(b); }
        X.sealTo((E.H[(size_t)nh - 1].k + 1) * 3600);
        e.hRot = X.zdir; if (X.S.empty()) return;
        const stp::Step& q = X.S.back(); e.hDir = q.dir; e.hPx = q.px; e.hTop = stp::stepTop(q); e.hBroken = q.brI >= 0;
    }
    // (0.7.0) his vwapTrend at the newest closed candle (VwapTrendLogic.h on this chart's closed candles, the session of that candle), kept per candle
    template <class TA, class FA> void vwTrend(S_& S, Ext& e, TA& times, FA& hi, FA& lo, FA& cl, int count) {
        RTARRAYI vo(barVolume); if (vo.count < count || count < 3) return;
        std::vector<svl::Bar> B; const int first = std::max(0, count - 1 - 700); B.reserve((size_t)(count - first));
        for (int i = first; i < count - 1; ++i) { struct tm t; std::memset(&t, 0, sizeof t); if (!getLocaltime((RTDATE)times[i], &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return;
            svl::Bar b; b.days = svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday); b.endSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec; b.h = (float)hi[i]; b.l = (float)lo[i]; b.c = (float)cl[i]; b.v = (double)vo[i]; B.push_back(b); }
        if (B.empty()) return;
        const vtl::Trend tr = vtl::fromChart(B, (int)B.size(), (int)S.spb, e.rthO, e.rthC);
        e.trendNow = vtl::level(tr); e.trendAt[(long long)times[count - 2]] = e.trendNow; while (e.trendAt.size() > 400) e.trendAt.erase(e.trendAt.begin());
    }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = 1; return RTX_OK; }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        pump(true);
        S_* S = testState(); if (!S) return RTX_OK;
        Ext* e = ext(*S); if (!e) return RTX_OK;
        S->hov.left();
        try { render(*S, *e); } catch (...) { S->state = "draw error"; }
        return RTX_OK;
    }
    static const int LINE_H = 14;
    int put(const std::string& t, int x, int y, COLOR c, int lim) {
        const int w = (int)getTextWidth(t.c_str(), -1); if (x + w > lim) return -1;
        setTextColor(c); RCT r; r.set(rvh::coord(x), rvh::coord(y), rvh::coord(x + w), rvh::coord(y + LINE_H - 1)); r.drawText(t.c_str(), false, false);
        return w;
    }
    void render(S_& S, Ext& e) {
        RCT pane; pane.getPaneRect(false);
        const long count = getBarCount(); if (count < 2) return;
        RTARRAYI times(barDateTime); if (times.count < count) return;
        int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
        FONT fp; fp.id = HELVETICA; fp.size = 9; fp.style = PLAIN; FONT fb = fp; fb.style = BOLD; FONT fl = fp; fl.size = 11; fl.style = BOLD; FONT fq = fl; fq.style = PLAIN;   // fq (0.5.0): the quiet letters C / A / E
        const double bw = (double)std::max(1, (int)getPixelsPerBar());
        std::vector<std::pair<int, rvs::Poc> > c; c.reserve((size_t)(v1 - v0 + 1));
        for (int i = v0; i <= v1; ++i) { const auto it = e.ps.poc.find((long long)times[i]); if (it == e.ps.poc.end()) continue; PNT p; p.set(i, 0.0f, kBarCenter); c.push_back(std::make_pair((int)p.h, it->second)); }
        static const std::map<long long, std::vector<rvs::Mark> > none;
        // (0.6.0) buy and sell volume side by side; no values; the "biggest in" count on a bar that is the biggest in 10+ candles
        const std::vector<rvs::BSBar> B = rvs::layoutBS(c, e.eng ? e.eng->marks : none, pane.top, pane.bottom, 12, bw, LINE_H + 2);
        for (const rvs::BSBar& b : B) {
            if (b.topB < b.bottom) off::fillFlat(this, b.x - b.w, b.topB, b.x, b.bottom, rvh::C_GREEN);
            if (b.topS < b.bottom) off::fillFlat(this, b.x + 1, b.topS, b.x + 1 + b.w, b.bottom, rvh::C_RED);
            setFont(fb);
            if (b.nB) { const std::string t = std::to_string(b.nB); const int w = (int)getTextWidth(t.c_str(), -1); const int cx = b.x - b.w / 2 - (b.nS ? w / 2 : 0); setTextColor(rvh::C_GREEN); RCT r; r.set(rvh::coord(cx - w / 2), rvh::coord(b.numYB), rvh::coord(cx - w / 2 + w + 1), rvh::coord(b.numYB + 12)); r.drawText(t.c_str(), false, false); }
            if (b.nS) { const std::string t = std::to_string(b.nS); const int w = (int)getTextWidth(t.c_str(), -1); const int cx = b.x + 1 + b.w / 2 + (b.nB ? w / 2 : 0); setTextColor(rvh::C_RED); RCT r; r.set(rvh::coord(cx - w / 2), rvh::coord(b.numYS), rvh::coord(cx - w / 2 + w + 1), rvh::coord(b.numYS + 12)); r.drawText(t.c_str(), false, false); }
            if (!b.marks.empty()) { setFont(fl); int tw = 0; std::vector<std::string> T;
                for (const rvs::Mark& m : b.marks) { std::string t(1, m.letter); T.push_back(t);   /* (Rassul 2026-11-11 22:19 CT "get rid of the ? mark after the signals so just have C instead of C?") no "?" */ tw += (int)getTextWidth(t.c_str(), -1) + 2; }
                int x = b.x - tw / 2; const int y = std::max((int)pane.top + LINE_H + 2, b.letY - 2);
                for (size_t k = 0; k < T.size(); ++k) { const int w = (int)getTextWidth(T[k].c_str(), -1); const bool sig = b.marks[k].letter == 'I';   /* (0.5.0, "ok build") the initiation is the signal: bold and bright; C / A / E are the story so far: plain and dim */
                    setFont(sig ? fl : fq); setTextColor(sig ? (b.marks[k].col > 0 ? rvh::C_GREEN : rvh::C_RED) : (b.marks[k].col > 0 ? rvh::C_BAR_GREEN : rvh::C_BAR_RED)); RCT r; r.set(rvh::coord(x), rvh::coord(y), rvh::coord(x + w + 1), rvh::coord(y + 13)); r.drawText(T[k].c_str(), false, false); x += w + 2; } }
        }
        // (0.7.0) the six checks: the read shown = the decided Rev under the mouse, else the live pullback, else the newest decided Rev of the last 10 candles
        const rvs::SixRead* six = nullptr;
        { const int hb6 = S.hov.hover.bar; if (hb6 >= 0 && hb6 < (int)count) { const auto it = e.six.done.find((long long)times[hb6]); if (it != e.six.done.end()) six = &it->second; }
          if (!six && e.haveLive) six = &e.live;
          if (!six && !e.six.done.empty() && (long long)times[(int)count - 1] - e.six.done.rbegin()->first <= 10LL * (long long)S.spb) six = &e.six.done.rbegin()->second; }
        // a white ratio over the volume bar of a big order that passed its check (the big buyer at the pivot, the big seller on the Rev candle)
        { setFont(fb); auto ratio = [&](long long candle, bool buyBar, double x) { for (const rvs::BSBar& b : B) { if (b.t != candle) continue; char t[24]; std::snprintf(t, sizeof t, "%.1fx", x); const int w = (int)getTextWidth(t, -1);
                    const int cx = buyBar ? b.x - b.w / 2 : b.x + 1 + b.w / 2; const int y = std::max((int)pane.top + LINE_H + 2, (buyBar ? (b.nB ? b.numYB : b.topB - 1) : (b.nS ? b.numYS : b.topS - 1)) - 13);
                    setTextColor(rvh::C_WHITE); RCT r; r.set(rvh::coord(cx - w / 2), rvh::coord(y), rvh::coord(cx - w / 2 + w + 1), rvh::coord(y + 12)); r.drawText(t, false, false); break; } };
          auto both = [&](const rvs::SixRead& r) { if (r.c[1] && r.bbC > 0) ratio(r.bbC, r.dir < 0, r.bbX); if (r.c[4]) ratio(r.tr, r.dir > 0, r.bsX); };
          if (!B.empty()) { for (auto it = e.six.done.lower_bound(B.front().t); it != e.six.done.end(); ++it) both(it->second); if (e.haveLive) both(e.live); } }
        // the title (top-left)
        int x = pane.left + 6; const int y = pane.top + 2, lim = pane.right - 6;
        setFont(fb); { const std::string t = std::string("Reversal Read v") + RVS_VERSION; const int w = put(t, x, y, rvh::C_WHITE, lim); if (w > 0) x += w + 10; }
        std::string read;
        if (!e.note.empty()) { setFont(fp); put(e.note, x, y, rvh::C_GRAY, lim); read = e.note; }
        else if (!e.eng) { const std::string fs = S.feed.stateText(off::steadyMs()); const std::string t = fs != "ready" && !fs.empty() ? "no order feed - " + fs : "waiting for orders"; setFont(fp); put(t, x, y, rvh::C_GRAY, lim); read = t; }
        else {
            // (0.6.0) the read: numbered behaviour steps for the candle under the mouse (default: the newest story), in the empty space right of
            // the last candle; when the candles fill the pane, one line next to the title
            struct Row { std::string t; COLOR c; std::string t2; COLOR c2; }; std::vector<Row> R;
            auto timeOf = [&](long long sec) { struct tm t; std::memset(&t, 0, sizeof t); return getLocaltime((RTDATE)sec, &t) ? off::clock12(t.tm_hour, t.tm_min) : std::string(); };
            const int hb = S.hov.hover.bar; const rvs::Story* s = nullptr;
            if (hb >= 0 && hb < (int)count - 1) { const long long key = (long long)times[hb]; s = e.eng->storyAt(key, e.spb);
                { Row r; r.t = timeOf(key); r.c = rvh::C_WHITE; R.push_back(r); }
                const auto it = e.ps.poc.find(key); if (it != e.ps.poc.end() && it->second.ok) { Row r; r.t = "Buy " + std::to_string(it->second.buy); r.c = rvh::C_GREEN; r.t2 = "Sell " + std::to_string(it->second.sell); r.c2 = rvh::C_RED; R.push_back(r); } }
            else { s = e.eng->newest(e.stamp * 1000); if (s) { Row r; r.t = timeOf(s->t0 / 1000); r.c = rvh::C_WHITE; R.push_back(r); } }
            if (s) { const std::vector<rvs::Seg> L = e.eng->readSteps(*s);
                for (const rvs::Seg& g : L) { Row r; r.t = g.text; r.c = g.col > 1 ? rvh::C_WHITE : g.col > 0 ? rvh::C_GREEN : g.col < 0 ? rvh::C_RED : rvh::C_GRAY; R.push_back(r); } }
            for (size_t i = 0; i < R.size(); ++i) read += (i ? " > " : "") + R[i].t + (R[i].t2.empty() ? std::string() : " " + R[i].t2);
            if (six) {                                                   // (0.7.0) "Rev N" + six lines, a green check / red x in front of each
                const std::vector<rvs::SixLine> L = rvs::sixLines(*six); read.clear(); for (size_t i = 0; i < L.size(); ++i) read += (i ? " > " : "") + L[i].text + (L[i].ok == 1 ? " +" : L[i].ok == 0 ? " -" : "");
                setFont(fl); int wmax = 0; for (const rvs::SixLine& l : L) wmax = std::max(wmax, (int)getTextWidth(l.text.c_str(), -1) + 18);
                PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int xr = (int)pl.h + (int)bw / 2 + 18; const int rowH = 17, y0 = pane.top + LINE_H + 6;
                if (v1 >= (int)count - 1 && xr + wmax <= lim && y0 + (int)L.size() * rowH <= pane.bottom) {
                    for (size_t i = 0; i < L.size(); ++i) { const int yy = y0 + (int)i * rowH; const rvs::SixLine& l = L[i];
                        if (l.ok < 0) { put(l.text, xr, yy, rvh::C_WHITE, lim + 1); continue; }
                        const COLOR mc = l.ok ? rvh::C_GREEN : rvh::C_RED; setPen(mc, 2, P_SOLID);
                        auto seg = [&](int x0, int y0_, int x1, int y1) { PNT a; a.set(0, 0.0f); a.h = rvh::coord(x0); a.v = rvh::coord(y0_); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = rvh::coord(x1); b.v = rvh::coord(y1); b.drawLineTo(); };
                        if (l.ok) { seg(xr, yy + 7, xr + 3, yy + 11); seg(xr + 3, yy + 11, xr + 10, yy + 3); } else { seg(xr + 1, yy + 3, xr + 9, yy + 11); seg(xr + 9, yy + 3, xr + 1, yy + 11); }
                        put(l.text, xr + 16, yy, l.ok ? rvh::C_WHITE : rvh::C_GRAY, lim + 1); } }
                else { setFont(fb); for (size_t i = 0; i < L.size(); ++i) { const COLOR c = L[i].ok < 0 ? rvh::C_WHITE : L[i].ok ? rvh::C_GREEN : rvh::C_GRAY; const int w = put(L[i].text, x, y, c, lim); if (w < 0) break; x += w + 8; } }
            }
            else if (R.empty()) read = "no story";
            else {
                setFont(fl); int wmax = 0; for (const Row& r : R) wmax = std::max(wmax, (int)getTextWidth(r.t.c_str(), -1) + (r.t2.empty() ? 0 : 10 + (int)getTextWidth(r.t2.c_str(), -1)));
                PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int xr = (int)pl.h + (int)bw / 2 + 18; const int rowH = 17, y0 = pane.top + LINE_H + 6;
                if (v1 >= (int)count - 1 && xr + wmax <= lim && y0 + (int)R.size() * rowH <= pane.bottom) {
                    for (size_t i = 0; i < R.size(); ++i) { const int yy = y0 + (int)i * rowH; const int w = put(R[i].t, xr, yy, R[i].c, lim + 1); if (w > 0 && !R[i].t2.empty()) put(R[i].t2, xr + w + 10, yy, R[i].c2, lim + 1); } }
                else { setFont(fb);
                    for (size_t i = 0; i < R.size(); ++i) { if (i) { const int w = put(">", x, y, rvh::C_GRAY, lim); if (w < 0) break; x += w + 5; }
                        int w = put(R[i].t, x, y, R[i].c, lim); if (w < 0) break; x += w + 5; if (!R[i].t2.empty()) { w = put(R[i].t2, x, y, R[i].c2, lim); if (w < 0) break; x += w + 5; } } }
            }
        }
        e.lastRead = read; ++e.drawn;
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<RevRead*>(this)->stopAll(); static_cast<RevRead*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<RevRead*>(this)->stopAll(); static_cast<RevRead*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<RevRead*>(this)->rvCalc(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    RevRead* p = new RevRead();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);      // its own pane, draws itself; (0.6.0) the mouse picks the candle the read is about; IRT's volume at price
    p->setExtendedFlags(CALL_CONTINUOUSLY);                              // the order feed keeps coming while no trade prints
    p->setDescription("LRA Reversal Read - two bars per candle = its buy volume (green) and sell volume (red); a number on a bar = the biggest in that many candles (10 or more); letters C climax (red = selling, green = buying), A absorbed, E exhausted, I initiation; the read (right of the last candle) = the six checks of the newest pullback / Rev (Rev N: buying fading, big buyer trapped, quick rejection, bids pulled, big seller, with trend), else the reversal behaviours of the candle under the mouse. Market-by-order feed.");
    rvh::stamp(p);   // RVS_VERSION
    return p;
}

#else
// =====================================================================================================================
// lsRevReadMarks - the price-chart overlay: a white dash on each candle at its POC price
// =====================================================================================================================
static ss::Ids& rmIds() { static ss::Ids I; return I; }
inline long long rmSteadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct RmState { rvh::PocStore ps; double tick = 0; long long drawn = 0, dots = 0; std::string mkt; ss::Store store; ss::Values set = ss::defaults(rvset::spec(), -1); long long setMs = -100000;
                 std::map<long long, signed char> hold; };   // candle end -> +3 / -3 ... (side x tier) of a CLOSED candle, decided once
class RevReadMarks : public cppExtension {
public:
    SS_SAFE_DIALOG(RevReadMarks, "RevReadMarks")
    HostSlot<RmState> slot_;
    virtual int parmsLoadBody(void) { if (!ss::inDialog(this)) return RTX_OK; ss::Store* st = dialogStore(); if (st) ss::fillDialog(this, rvset::spec(), rmIds(), st->load(ss::marketIndex(dlgMkt_), rmSteadyMs(), true), snap_); return RTX_OK; }
    virtual int parmsApplyBody(void) { save(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, rvset::spec(), rmIds(), (int)i, snap_); save(); return RTX_OK; }
    // the settings of this chart's market (re-read at most every 2 s)
    const ss::Values& settings(RmState& s) {
        const long long now = rmSteadyMs(); if (now - s.setMs < 2000 && now >= s.setMs) return s.set; s.setMs = now;
        const char* rs = getRootSymbol(); const char* sy = getSymbol(); s.mkt = rvh::marketOf(rs ? rs : "", sy ? sy : "");
        const std::string p = rvset::settingsPath(rvh::flexDir(), s.mkt);
        if (s.store.path != p) { s.store = ss::Store(); s.store.S = &rvset::spec(); s.store.path = p; }
        s.set = p.empty() ? ss::defaults(rvset::spec(), -1) : s.store.load(ss::marketIndex(s.mkt), now); return s.set;
    }
    // the hold read of closed bar i (0 = none), decided once per candle from the 20 closed candles before it
    template <class TA, class FA> int holdAt(RmState& s, TA& times, FA& hi, FA& lo, FA& cl, int i) {
        const long long key = (long long)times[i]; const auto it = s.hold.find(key); if (it != s.hold.end()) return it->second;
        if (i < 20) return 0;
        std::vector<rvs::Cnd> c; c.reserve(21);
        for (int k = i - 20; k <= i; ++k) { const auto p = s.ps.poc.find((long long)times[k]); const double h = (double)hi[k], l = (double)lo[k], x = (double)cl[k];
            if (p == s.ps.poc.end() || !p->second.ok || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(x)) return 0;   // a candle without volume at price yet: decided on a later draw
            rvs::Cnd d; d.t = (long long)times[k]; d.hi = (long long)std::llround(h / s.tick); d.lo = (long long)std::llround(l / s.tick); d.close = (long long)std::llround(x / s.tick); d.total = p->second.total; c.push_back(d); }
        const rvs::Hold q = rvs::holdOf(c, 20); const int v = q.side * q.tier;
        s.hold[key] = (signed char)v; while (s.hold.size() > 6000) s.hold.erase(s.hold.begin());
        return v;
    }
    void pump() {
        RmState* sp = slot_.get(this, true); if (!sp) return; RmState& s = *sp;
        const double tick = (double)getProperty(SYM_TICKINCR); if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return;
        if (tick != s.tick) { s.ps.reset(); s.tick = tick; }
        const long count = getBarCount(); if (count < 2) return;
        RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
        if (times.count < count || hi.count < count || lo.count < count || cl.count < count) return;
        rvh::PocStore& P = s.ps;
        if (P.seenCount > count || (P.lastKey != 0 && P.next > 0 && P.next <= count - 1 && (long long)times[P.next - 1] != P.lastKey)) P.reset();
        P.seenCount = count; if (P.fault) return;
        RTARRAYP vap(barVolumeProfile); int done = 0; P.start(count); const long long t0 = rvh::PocStore::nowMs();
        for (; P.next < count - 1 && rvh::PocStore::more(done, t0); ++P.next, ++done) {
            const int i = P.next; const long long key = (long long)times[i]; std::vector<rvs::VRow> rows; double pv = 0;
            const int r = rvh::vapRows(vap, i, tick, rows, pv);
            if (r < 0) { P.fault = true; break; }
            const double h = (double)hi[i], l = (double)lo[i], c = (double)cl[i];
            if (r == 0 || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c)) { ++P.missing; P.lastKey = key; continue; }
            P.poc[key] = rvs::pocOf(key, rows, (long long)std::llround(l / tick), (long long)std::llround(h / tick), (long long)std::llround(c / tick)); P.lastKey = key;
            while (P.poc.size() > (size_t)rvh::PocStore::KEEP) P.poc.erase(P.poc.begin());
        }
    }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        RmState* sp = slot_.get(this, false); if (!sp) return RTX_OK; RmState& s = *sp;
        try {
            const long count = getBarCount(); if (count < 2 || !(s.tick > 0)) return RTX_OK;
            RTARRAYI times(barDateTime); if (times.count < count) return RTX_OK;
            int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return RTX_OK; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
            RCT pane; pane.getPaneRect(false);
            const int half = std::max(3, (int)getPixelsPerBar() * 45 / 100);
            const ss::Values& V = settings(s);
            setPen(rvh::C_DASH, 1, P_SOLID);
            if (V.b(rvset::K_SHOW_HOLD) || getSecondsPerBar() >= 3600) {   // (0.4.1) always on an hourly chart                                // (0.4.0) the hold read: closed candles only; strong tier only
                RTARRAY hi(barHigh), lo(barLow), cl(barClose);
                if (hi.count >= count && lo.count >= count && cl.count >= count) for (int i = v0; i <= v1 && i < (int)count - 1; ++i) {
                    const int h = holdAt(s, times, hi, lo, cl, i); if (h != 3 && h != -3) continue;
                    PNT p; p.set(i, 0.0f, kBarCenter); const bool top = h > 0; const int y = (int)getVerticalDrawPosition((float)(top ? (double)hi[i] : (double)lo[i])) + (top ? -8 : 8);
                    if (y < pane.top + 3 || y > pane.bottom - 4) continue;
                    const COLOR col = top ? rvh::C_RED : rvh::C_GREEN;    // a high that is likely done = bearish (red, over it); a low = bullish (green, under it)
                    RCT r; r.set(rvh::coord(p.h - 2), rvh::coord(y - 2), rvh::coord(p.h + 3), rvh::coord(y + 3)); r.draw(0, col, col, DRAW_OPAQUE, PAT_SOLID); ++s.dots; }
            }
            if (V.b(rvset::K_SHOW_POC)) for (int i = v0; i <= v1; ++i) { const auto it = s.ps.poc.find((long long)times[i]); if (it == s.ps.poc.end() || !it->second.ok) continue;
                PNT p; p.set(i, 0.0f, kBarCenter); const int y = (int)getVerticalDrawPosition((float)((double)it->second.tick * s.tick));
                if (y < pane.top + 1 || y > pane.bottom - 2) continue;
                RCT r; r.set(rvh::coord(p.h - half), rvh::coord(y - 1), rvh::coord(p.h + half + 1), rvh::coord(y + 1)); r.draw(0, rvh::C_DASH, rvh::C_DASH, DRAW_OPAQUE, PAT_SOLID); ++s.drawn; }
        } catch (...) {}
        return RTX_OK;
    }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    void save() { if (!ss::inDialog(this)) return; ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, rvset::spec(), rmIds(), st->load(ss::marketIndex(dlgMkt_), rmSteadyMs(), true), snap_);
        rvh::makeDir(rvh::flexDir()); if (st->save(V)) snap_.take(ss::dialogNow(this, rvset::spec(), rmIds(), V)); }
    ss::Store* dialogStore() { const std::string mkt = ss::safeMarket(this); const std::string p = rvset::settingsPath(rvh::flexDir(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &rvset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; } return &dlgStore_; }
};
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { static_cast<RevReadMarks*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void)   { ss::registerAll(this, rvset::spec(), rmIds()); return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); try { static_cast<RevReadMarks*>(this)->pump(); } catch (...) {} return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    RevReadMarks* p = new RevReadMarks();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI | VAP_REQUIRED);
    p->setDescription("LRA Reversal Read marks - a white dash on each candle at its POC price (the price with the most volume in that candle); optional (off by default): a dot at a high / low that is likely done; the price-chart half of lsRevRead.");
    rvh::stamp(p);   // RVS_VERSION
    return p;
}
#endif
