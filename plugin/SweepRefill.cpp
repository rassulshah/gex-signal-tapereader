/********************************************************************************
 *  SweepRefill.cpp  --  Investor/RT RTX extension  lsSweepRefill  1.1.0  (2026-10-22)
 *
 *  THE SWEEP-REFILLED MARK "Sr" on the PRICE chart (MBO Roadmap "Did the swept prices refill fast after a sweep?").
 *  1.0.0: Rassul 16:16 CT (build approved; "Sr only happens when a level or pivot high or low was taken out"), 16:18 CT ("it should
 *  not be on adjacent candles"), 16:28 CT (draw bearish too).
 *  1.1.0: Rassul approved the mockup v4 rule at 17:39 CT (sweep-refill-mockup-v4.png) - SwingRef.h, shared with lsStopRun 1.2.0:
 *    - the swept reference is an INTACT swing among the last 3 swings of the 1-bar ZigZag from the RTH open (1 back always; 2 back
 *      if beyond 1; 3 back if beyond 1 and 2), or a key level (ONH/ONL PDH/PDL PFH/PFL LonHI/LonLO PWH/PWL AsiaHI/AsiaLO) or a
 *      MenthorQ level; 5-candle extremes and overnight pivots no longer count;
 *    - taken out = the sweep ends beyond it, starts <= 0.5 ATR past it, and no candle before the last one had traded through it;
 *      more than one = the deepest;
 *    - green "Sr" UNDER the swept wick (sellers swept the bids through a swing low and the bids came back - the locked rule per market,
 *      NQ 50% within 5 s and still there 5 s later); red "Sr" OVER the swept high (the mirror);
 *    - a short line from the swing taken out to the sweep candle; the hover names the swing ("took out the swing low from 8:48 AM,
 *      2 swings back");
 *    - the ZigZag drawn as a thin line (setting, default on, colour changeable);
 *    - (Rassul 18:07 CT) VOLUME-PROFILE levels count too: POC / VAH / VAL of this session (as of the last closed candle) and of the
 *      prior session, plus naked prior POCs - from the chart's own volume at price (VAP_REQUIRED, 70% value area; SwingRef.h);
 *    - (coordinator 2026-10-22, VALIDATE-STOPRUN-SWEEP-2026-10-22.md) the locked rule and SHOW / ? / LOG per market and side from
 *      lsFlexLevels\LRA-StopSweep-<MKT>.csv: NQ bull SHOW, NQ bear "Sr?", GC bull SHOW; everything else recorded only. The verdict
 *      is stamped on each mark when it is decided (a later params change never repaints a mark).
 *    - never on two adjacent candles (per side); decided once (4 s after the sweep), recorded, redrawn from the record - never repainted.
 *  The hover (mouse over the mark): header "Sweep refilled?" ("?" = not proven), the read, the numbers, then
 *  question / what it is / why it matters / how to use / example.
 *  Rules: SweepRefillLogic.h + SwingRef.h (SDK-free, unit-tested; replay on real MBO in tests_sr/replay_sr.cpp).
 *
 *  FEED: one DEPTH_BY_ORDER per chart, kept alive for the chart's life, opened from calc / draw only behind a crash-guard file,
 *  drained <= 5,000 events / ~8 ms per call (CALL_CONTINUOUSLY keeps calc coming while the chart is quiet). Every NEW / MOD / DEL /
 *  FILL keeps the order book; the event clock = the exchange second + the arrival ms inside it (swr::Clock). There is NO historical
 *  order-by-order data: Sr exists from the moment the indicator is on the chart; the session record brings the marks back.
 *  Levels (read-only, the same files his other plugins read): lsFlexLevels\LRA-KeyLevels-<MKT>.csv, LRA-Dealer-<MKT>.csv and
 *  LRA-Levels-<MKT>.csv (MenthorQ), fresh <= 18 h, MenthorQ only for the chart's contract.
 *  FILES (lsFlexLevels\SweepRefill\):  <MKT>-<spb>-<yyyymmdd>.csv  every candidate (refilled or not), one SR line each, append-only
 *                                      SweepRefill.status-<MKT>-<spb>.txt  the status, on change or every 30 s
 *  Settings: lsFlexLevels\SweepRefill.settings-<MKT>.txt via SettingsSpec / SettingsHost (SweepRefillSettings.h).
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "SweepRefill"
#endif
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "SweepRefillLogic.h"
#include "SwingRef.h"
#include "ImpliedFill.h"   // (1.1.1) implied fills (side "?" FILLs) are not sweep trades - shared, SDK-free
#include "NewsWindow.h"   // (2026-10-22 evening) news / settle window "?" + the feed check (record only) - shared, SDK-free
#include "NearLevels.h"
#include "HostSlot.h"
#include "SweepRefillSettings.h"
#include "SettingsHost.h"
#include "SettingsGuard.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

#define SWR_VERSION "1.1.3"     // = swr::VERSION = swrset::PLUGIN_VERSION = the setVersion literal

#ifdef SWR_TEST_CLOCK
long long swrTestClockMs();
static long long swrMs() { return swrTestClockMs(); }
#else
static long long swrMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#endif

// bDboSubscribed is private: a pointer-to-member through explicit instantiation (as MboCheck.cpp), never by offset
template <class Tag, typename Tag::type M> struct SwrRob { friend typename Tag::type swrMember(Tag) { return M; } };
struct SwrDboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type swrMember(SwrDboFlag); };
template struct SwrRob<SwrDboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;

static const int SLICE_MAX = 5000; static const long long SLICE_BUDGET_MS = 8;
static const int HOVER_PT = 10, HOVER_LH = 14;
static const int LANES_RESERVED = 2;     // the first two text lanes above / below a candle belong to lsBigBidAsk's BO / BD and Rly

static std::string swrFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + SWR_SEP "InvestorRT" SWR_SEP "rtx" SWR_SEP "lsFlexLevels" : std::string(); }
static void swrMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool swrExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }

struct HitBox { int l, t, r, b; size_t idx; };

// ---- the chart's volume at price (only trivial values live in these MSVC SEH helpers, as lsBigBidAsk) ------------------------------
static int swrStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !st || bar < 0) return -2;
        std::memset(st, 0, sizeof(*st));
        if (vap->getBarStatistics(bar, *st) != RTX_OK) return -2;
        if (st->prices < 0) return -2;
        return st->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static int swrRowsRaw(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;                                                  // any row failure rejects the whole bar, never a prefix
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one candle's volume at price -> (tick, lots); -1 = a fault (stop reading VAP), 0 = none yet, 1 = ok
static int swrVap(cppExtension::RTARRAYP& vap, int bar, double tick, std::vector<std::pair<long long, double> >& out) {
    out.clear();
    cppExtension::BARSTATISTICS st; const int n = swrStats(&vap, bar, &st);
    if (n == -1) return -1;
    if (n <= 0 || n > 5000) return 0;
    std::vector<cppExtension::VOLPROFILE> rows((size_t)n);
    const int got = swrRowsRaw(&vap, bar, &rows[0], n);
    if (got == -1) return -1;
    if (got != n) return 0;
    for (size_t i = 0; i < rows.size(); ++i) {
        long long k = 0; const double v = (double)rows[i].totalVolume;
        if (!swr::toTicks((double)rows[i].price, tick, k) || !(v > 0) || !swr::finite(v)) continue;
        out.push_back(std::make_pair(k, v));
    }
    return 1;
}

struct SwrState {
    // identity
    std::string identity, market, symbol; int spb = 0; double tick = 0;
    // feed
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr; bool dboDead = false, blocked = false; int attempt = 0; int flag = -1;
    long long events = 0, fillsLots = 0, lastResubMs = -1, backlogSlices = 0, lastStampSec = 0, garbage = 0;
    imp::Params imp; long long impMtime = -3, impSize = -1;   // (1.1.1) LRA-Implied-<MKT>.csv
    bool vapFault = false; long long vapBars = 0, vapMissing = 0;   // the volume-profile levels (POC / VAH / VAL) from the chart's VAP
    // detector + bars
    swr::Detector D; swr::Clock clk; bool primed = false; long long lastClosedT = 0;
    // levels
    nl::Source keySrc, dealerSrc, mqSrc; long long levelCheckMs = -100000; std::string levelNote = "not read yet";
    // record
    std::vector<swr::Event> marks; std::string session, recPath; long long recFails = 0, recLines = 0, recBad = 0; bool loaded = false;
    // settings
    ss::Store setStore; ss::Values set = ss::defaults(swrset::spec(), -1); long long setCheckMs = -100000;
    // the per-market params (lsFlexLevels\LRA-StopSweep-<MKT>.csv): the locked rule + SHOW / ? / LOG per side
    swg::StopSweep prm; long long prmCheckMs = -100000, prmMtime = -1, prmSize = -1;
    // hover
    std::vector<HitBox> hits; int hover = -1;
    // status
    std::string statusBody; long long statusMs = -100000; long long busy = 0;
    ~SwrState() { delete dbo; }
};
static ss::Ids& swrIds() { static ss::Ids I; return I; }

class SweepRefill : public cppExtension {
public:
    SS_SAFE_DIALOG(SweepRefill, "SweepRefill")
    HostSlot<SwrState> slot_;
    SweepRefill() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &SweepRefill::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &SweepRefill::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &SweepRefill::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, swrset::spec(), swrIds(), st->load(ss::marketIndex(dlgMkt_), swrMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, swrset::spec(), swrIds(), (int)i, snap_); saveSettings(); }
    void pump(bool fromDraw);
    SwrState* testState() { return slot_.get(this, false); }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = ss::safeMarket(this); const std::string p = swrset::settingsPath(swrFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &swrset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, swrset::spec(), swrIds(), st->load(ss::marketIndex(dlgMkt_), swrMs(), true), snap_);
        swrMkdir(swrFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, swrset::spec(), swrIds(), V));
    }
    void identify(SwrState& S);
    void settings(SwrState& S, long long now);
    void levels(SwrState& S, long long now);
    void params(SwrState& S, long long now);
    void openFeed(SwrState& S);
    void drain(SwrState& S, long long now);
    void bars(SwrState& S);
    void loadRecord(SwrState& S);
    void append(SwrState& S, const std::string& line);
    void collect(SwrState& S);
    void status(SwrState& S, long long now);
    std::string ctText(long long stamp);
    std::string hmText(long long stamp);
    std::string sessionOf(RTDATE stamp);
    long long sessionKey(long long stamp);
    void render(SwrState& S);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<SweepRefill*>(this)->slot_.release(this); return RTX_OK; }      // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<SweepRefill*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, swrset::spec(), swrIds()); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<SweepRefill*>(this)->pump(false); return RTX_OK; }

std::string SweepRefill::ctText(long long stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (stamp <= 0 || !getLocaltime((RTDATE)stamp, &t)) return std::string();      // IRT's local clock = Central on his PC
    const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
    char b[64]; std::snprintf(b, sizeof b, "%d:%02d:%02d %s", h12, t.tm_min, t.tm_sec, t.tm_hour < 12 ? "AM" : "PM"); return b;
}
// "8:48 AM" (the swing candle's START = its END - seconds per bar)
std::string SweepRefill::hmText(long long stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (stamp <= 0 || !getLocaltime((RTDATE)stamp, &t)) return std::string();
    const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
    char b[32]; std::snprintf(b, sizeof b, "%d:%02d %s", h12, t.tm_min, t.tm_hour < 12 ? "AM" : "PM"); return b;
}
// the 17:00 CT trading day: an event at or after 17:00 belongs to the next day's session
std::string SweepRefill::sessionOf(RTDATE stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(stamp, &t)) return std::string();
    struct tm d = t; d.tm_hour = 12; d.tm_min = d.tm_sec = 0; d.tm_isdst = -1;
    std::time_t day = std::mktime(&d); if (day == (std::time_t)-1) return std::string();
    if (t.tm_hour >= 17) day += 86400;
    struct tm sd; std::memset(&sd, 0, sizeof sd);
#if defined(_WIN32)
    localtime_s(&sd, &day);
#else
    localtime_r(&day, &sd);
#endif
    char b[48]; std::snprintf(b, sizeof b, "%04d%02d%02d", sd.tm_year + 1900, sd.tm_mon + 1, sd.tm_mday); return b;
}
long long SweepRefill::sessionKey(long long stamp) { const std::string s = sessionOf((RTDATE)stamp); return s.empty() ? 0 : std::strtoll(s.c_str(), nullptr, 10); }

void SweepRefill::identify(SwrState& S) {
    const std::string mkt = ss::safeMarket(this);
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!swr::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = mkt + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    delete S.dbo; S.dbo = nullptr;
    S.identity = id; S.market = mkt; S.symbol = sym; S.spb = spb; S.tick = tick;
    S.dboDead = S.blocked = false; S.attempt = 0; S.flag = -1; S.events = S.fillsLots = S.garbage = 0; S.lastResubMs = -1; S.lastStampSec = 0;
    S.vapFault = false; S.vapBars = S.vapMissing = 0;
    S.D = swr::Detector(); S.clk = swr::Clock(); S.primed = false; S.lastClosedT = 0;
    S.keySrc = nl::Source(); S.dealerSrc = nl::Source(); S.mqSrc = nl::Source(); S.levelCheckMs = -100000;
    S.marks.clear(); S.session.clear(); S.recPath.clear(); S.loaded = false;
    S.setStore = ss::Store(); S.setCheckMs = -100000; S.hits.clear(); S.hover = -1; S.statusBody.clear();
    S.prm = swg::StopSweep(); S.prmCheckMs = -100000; S.prmMtime = S.prmSize = -1;
}

// (1.1.1) LRA-Implied-<MKT>.csv (6E may be named 6E): exclude side "?" fills (missing / garbled = exclude anyway, the safe default)
static void swrImplied(SwrState& S) {
    const std::string f = swrFlex(); if (f.empty() || S.market.empty()) return;
    std::string p = f + SWR_SEP "LRA-Implied-" + S.market + ".csv";
    struct stat st; bool have = stat(p.c_str(), &st) == 0;
    if (!have && S.market == "EU") { p = f + SWR_SEP "LRA-Implied-6E.csv"; have = stat(p.c_str(), &st) == 0; }
    if (!have) { if (S.impMtime != -2) { S.imp = imp::Params(); S.impMtime = -2; } S.D.skipImplied = S.imp.exclude; return; }
    if ((long long)st.st_mtime == S.impMtime && (long long)st.st_size == S.impSize) return;
    std::vector<std::string> lines; const bool ok = nl::readBoundedLines(p, lines, 64 * 1024, 400, 512);
    struct stat st2; if (!ok || stat(p.c_str(), &st2) != 0 || st2.st_mtime != st.st_mtime || st2.st_size != st.st_size) return;   // torn: retry later
    S.imp = imp::parse(true, true, lines, S.market); S.impMtime = (long long)st.st_mtime; S.impSize = (long long)st.st_size;
    S.D.skipImplied = S.imp.exclude;
}
// the params file, re-read only when its stamp (mtime + size) changes, at most every 5 s; missing / garbled = defaults, every mark LOG
void SweepRefill::params(SwrState& S, long long now) {
    if (now - S.prmCheckMs < 5000 && now >= S.prmCheckMs) return;
    S.prmCheckMs = now;
    swrImplied(S);
    const std::string f = swrFlex(); if (f.empty() || S.market.empty()) return;
    const std::string p = f + SWR_SEP "LRA-StopSweep-" + S.market + ".csv";
    struct stat st; if (stat(p.c_str(), &st) != 0) { if (S.prmMtime != -2) { S.prm = swg::StopSweep(); S.prm.why = "no file (defaults, every mark LOG)"; S.prmMtime = -2; } return; }
    if ((long long)st.st_mtime == S.prmMtime && (long long)st.st_size == S.prmSize) return;
    std::vector<std::string> lines;
    if (!nl::readBoundedLines(p, lines, 64 * 1024, 512, 512)) return;                       // torn / oversized: keep the last good copy
    struct stat st2; if (stat(p.c_str(), &st2) != 0 || st2.st_mtime != st.st_mtime || st2.st_size != st.st_size) return;
    S.prm = swg::parseStopSweep(lines, S.market); S.prmMtime = (long long)st.st_mtime; S.prmSize = (long long)st.st_size;
}


void SweepRefill::settings(SwrState& S, long long now) {
    if (now - S.setCheckMs < 1000 && now >= S.setCheckMs) return;
    S.setCheckMs = now;
    const std::string sp = swrset::settingsPath(swrFlex(), S.market);
    if (S.setStore.path != sp) { S.setStore = ss::Store(); S.setStore.S = &swrset::spec(); S.setStore.path = sp; }
    S.set = sp.empty() ? ss::defaults(swrset::spec(), ss::marketIndex(S.market)) : S.setStore.load(ss::marketIndex(S.market), now);
    swr::Params& P = S.D.P;   // a change applies from now on; decided marks never change
    swr::applyParams(P, S.prm);                                  // the locked rule per market (params file)
    P.gapBars = S.set.i(swrset::K_GAP_BARS); P.spb = S.spb > 0 ? S.spb : 180;
    int oMin = 0, cMin = 0; if (swg::rthOf(S.market, oMin, cMin)) S.D.setRth(oMin, cMin); else S.D.setRth(0, 0);   // no RTH table = levels only
}

// key levels (his session levels only) + MenthorQ (dealer / levels files, the chart's contract only), as NearLevels.h reads them
void SweepRefill::levels(SwrState& S, long long now) {
    if (now - S.levelCheckMs < 5000 && now >= S.levelCheckMs) return;
    S.levelCheckMs = now;
    const std::string f = swrFlex(); if (f.empty() || S.market.empty() || !(S.tick > 0)) return;
    nl::loadSource(f + SWR_SEP "LRA-KeyLevels-" + S.market + ".csv", S.keySrc, 0);
    nl::loadSource(f + SWR_SEP "LRA-Dealer-" + S.market + ".csv", S.dealerSrc, 1);
    nl::loadSource(f + SWR_SEP "LRA-Levels-" + S.market + ".csv", S.mqSrc, 2);
    const long long wall = (long long)std::time(nullptr);
    auto fresh = [&](const nl::Source& X) { return X.sig >= 0 && X.at > 0 && wall - X.at <= 18 * 3600 && X.at - wall <= 600; };
    std::vector<swr::Level> v; int nk = 0, nm = 0;
    if (fresh(S.keySrc)) for (size_t i = 0; i < S.keySrc.v.size(); ++i) {
        swr::Level L; L.name = S.keySrc.v[i].name; L.dir = swr::keyDir(L.name); L.kind = swr::REF_KEY;
        if (L.dir == 0 || !swr::toTicks(S.keySrc.v[i].px, S.tick, L.px)) continue;
        v.push_back(L); ++nk;
    }
    const nl::Source* mq[2] = { &S.dealerSrc, &S.mqSrc };
    for (int k = 0; k < 2; ++k) {
        const nl::Source& X = *mq[k];
        if (!fresh(X) || X.stale || !nl::sameContract(S.symbol, X.sym)) continue;
        for (size_t i = 0; i < X.v.size(); ++i) {
            swr::Level L; L.name = X.v[i].name; L.kind = swr::REF_MQ; L.dir = 0;
            if (!swr::toTicks(X.v[i].px, S.tick, L.px)) { const double t = X.v[i].px / S.tick; if (!swr::finite(t) || t <= 0 || t > 9e15) continue; L.px = (long long)std::llround(t); }
            bool dup = false; for (size_t j = 0; j < v.size() && !dup; ++j) dup = v[j].kind == swr::REF_MQ && v[j].px == L.px;
            if (!dup) { v.push_back(L); ++nm; }
        }
    }
    S.D.levels.swap(v);
    S.levelNote = std::to_string(nk) + " key, " + std::to_string(nm) + " MenthorQ" + (S.keySrc.sig < 0 ? " (key file missing - open lsDealerProfile on a 3-min chart)" : "");
}

// chart context only, behind the crash guard; 1st try the chart's symbol, 2nd try NULL (= this chart)
void SweepRefill::openFeed(SwrState& S) {
    if (S.dbo || S.dboDead || S.blocked || S.attempt >= 2) return;
    const std::string dir = swrFlex() + SWR_SEP "SweepRefill";
    const std::string guard = swrFlex().empty() ? std::string() : dir + SWR_SEP "_opening-" + S.market + "-" + std::to_string(S.spb) + ".txt";
    if (S.attempt == 0 && swrExists(guard)) { S.blocked = true; return; }
    if (!guard.empty()) { swrMkdir(swrFlex()); swrMkdir(dir); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER\n"; }
    try { S.dbo = new (std::nothrow) DEPTH_BY_ORDER(S.attempt == 0 && !S.symbol.empty() ? S.symbol.c_str() : NULL); }
    catch (...) { S.dboDead = true; }
    ++S.attempt;
    if (!guard.empty()) std::remove(guard.c_str());
}

void SweepRefill::drain(SwrState& S, long long now) {
    if (!S.dbo || S.dboDead || !(S.tick > 0)) { S.D.advance(S.clk.idle(now)); return; }
    if (S.flag != 1 && S.lastResubMs >= 0 && now - S.lastResubMs < 1000 && now >= S.lastResubMs) return;
    const long long t0 = swrMs(); int n = 0; bool cap = false;
    try {
        if (!(S.dbo->*swrMember(SwrDboFlag()))) S.lastResubMs = now;
        while (true) {
            if (n >= SLICE_MAX) { cap = true; break; }
            if ((n & 511) == 511 && swrMs() - t0 > SLICE_BUDGET_MS) { cap = true; break; }
            if (S.dbo->getNext() != RTX_OK) break;
            ++n; ++S.events;
            const DEPTH_ORDER& o = S.dbo->dbo;
            swr::Ev e;
            e.action = o.action <= 3 ? (int)o.action : 99;
            e.side = o.buySell == 1 ? swr::BID : o.buySell == 0 ? swr::ASK : -1;
            if (!swr::toTicks((double)o.price, S.tick, e.px)) { ++S.garbage; continue; }
            e.size = (long long)o.size; e.id = (long long)o.orderID;
            e.implied = e.action == swr::FILL && imp::isImplied(S.imp, (long long)o.buySell);   // (1.1.1) side "?" FILL
            long long sec = (long long)o.timestamp; if (sec < S.lastStampSec && S.lastStampSec - sec < 1800) sec = S.lastStampSec;   // a stale stamp (ArrivalStamp.h)
            S.lastStampSec = sec; e.sec = sec; e.ms = S.clk.at(sec, now);
            if (e.action == swr::FILL) S.fillsLots += e.size;
            S.D.onEvent(e);
        }
        S.flag = (S.dbo->*swrMember(SwrDboFlag())) ? 1 : 0;
    } catch (...) { S.dboDead = true; }                                              // never re-enter a faulting call
    if (cap) ++S.backlogSlices;
    else S.D.advance(S.clk.idle(now));                                               // the feed is caught up: time passes
}

// closed candles (a later bar exists), strictly in order, each once; history at the first pass gives the ZigZag (from today's RTH
// open) and the ATR context. Each candle's CT day / RTH flag from IRT's local clock (Central on his PC).
void SweepRefill::bars(SwrState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYP vap(barVolumeProfile);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const bool haveVap = (long)vap.count >= n;
    int oMin = 0, cMin = 0; const bool haveRth = swg::rthOf(S.market, oMin, cMin);
    auto bar = [&](int i, swr::Bar& b) {
        b.t = (long long)times[i]; b.session = sessionKey(b.t - 1);
        struct tm t; std::memset(&t, 0, sizeof t);
        if (getLocaltime((RTDATE)b.t, &t)) {
            b.day = (long long)(t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
            b.rth = haveRth && swg::isRth(t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec, S.spb, oMin, cMin);
        }
        return swr::toTicks(op[i], S.tick, b.o) && swr::toTicks(hi[i], S.tick, b.h) && swr::toTicks(lo[i], S.tick, b.l) && swr::toTicks(cl[i], S.tick, b.c);
    };
    const int lastClosed = (int)n - 2;
    int from;
    if (!S.primed) { S.primed = true; from = std::max(0, lastClosed - 2000); }            // ~4 sessions of 3-min candles: prior day + naked POCs
    else { if ((long long)times[lastClosed] <= S.lastClosedT) return; from = lastClosed; while (from > 0 && (long long)times[from - 1] > S.lastClosedT) --from; }
    std::vector<std::pair<long long, double> > rows;
    for (int i = from; i <= lastClosed; ++i) {
        swr::Bar b; if (!bar(i, b)) continue;
        int vr = 0; if (haveVap && !S.vapFault) { vr = swrVap(vap, i, S.tick, rows); if (vr < 0) S.vapFault = true; }
        if (vr > 0) ++S.vapBars; else ++S.vapMissing;
        S.D.barClose(b, vr > 0 ? &rows : nullptr);
    }
    S.lastClosedT = (long long)times[lastClosed];
}

void SweepRefill::loadRecord(SwrState& S) {
    const std::string sess = sessionOf(currentDate());
    if (sess.empty() || S.market.empty()) return;
    if (S.loaded && sess == S.session) return;
    S.session = sess; S.loaded = true; S.marks.clear();
    const std::string dir = swrFlex() + SWR_SEP "SweepRefill";
    S.recPath = swrFlex().empty() ? std::string() : dir + SWR_SEP + S.market + "-" + std::to_string(S.spb) + "-" + sess + ".csv";
    std::vector<std::string> lines;
    if (S.recPath.empty() || !nl::readBoundedLines(S.recPath, lines, 4 * 1024 * 1024, 20000, 1024)) return;
    S.marks = swr::parseRecord(lines, S.tick, &S.recBad);
    for (size_t i = 0; i < S.marks.size(); ++i) {                 // the gap rule continues from the record (a restart never draws an adjacent Sr)
        swr::Event& e = S.marks[i]; if (e.refT > 0) e.refCt = hmText(e.refT - S.spb);
        if (e.refilled && e.gapOk && e.barT > S.D.lastDrawnBar[e.side]) S.D.lastDrawnBar[e.side] = e.barT;
    }
}

// one flushed append per line; a failure is counted (never retried in a loop)
void SweepRefill::append(SwrState& S, const std::string& line) {
    if (S.recPath.empty()) { ++S.recFails; return; }
    swrMkdir(swrFlex()); swrMkdir(swrFlex() + SWR_SEP "SweepRefill");
    const bool fresh = !swrExists(S.recPath);
    std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
    if (!f) { ++S.recFails; return; }
    if (fresh) f << swr::REC_HEAD << "\n";
    f << line << "\n"; f.flush();
    if (!f) { ++S.recFails; return; }
    ++S.recLines;
}

// decided candidates -> marks + record (once per key)
void SweepRefill::collect(SwrState& S) {
    for (size_t i = 0; i < S.D.decided.size(); ++i) {
        swr::Event e = S.D.decided[i]; e.ct = ctText(e.sec); if (e.refT > 0) e.refCt = hmText(e.refT - S.spb);
        bool dup = false; for (size_t k = 0; k < S.marks.size() && !dup; ++k) dup = S.marks[k].key == e.key;
        if (dup) continue;
        S.marks.push_back(e); append(S, swr::recLine(e, S.tick));
    }
    S.D.decided.clear();
    if (S.marks.size() > 5000) S.marks.erase(S.marks.begin(), S.marks.begin() + (long)(S.marks.size() - 5000));
}

void SweepRefill::status(SwrState& S, long long now) {
    const std::string d = swrFlex(); if (d.empty() || S.market.empty()) return;
    long long gb = 0, rb = 0, rec = 0; for (size_t i = 0; i < S.marks.size(); ++i) { const swr::Event& e = S.marks[i]; if (e.refilled && e.gapOk && e.show != swg::LOG) { if (swr::bullish(e)) ++gb; else ++rb; } else ++rec; }
    std::ostringstream o;
    o << "VERSION," << SWR_VERSION << "\nMARKET," << S.market << ",SYMBOL," << S.symbol << ",SECONDS_PER_BAR," << S.spb << "\n";
    o << "STATUS,lsSweepRefill v" << SWR_VERSION << " | MBO " << (S.blocked ? "not opened (crash guard file present)" : S.dboDead ? "faulted" : S.flag == 1 ? "subscribed" : S.flag == 0 ? "not subscribed" : "opening")
      << " | " << S.events << " events, " << S.fillsLots << " lots filled, book " << S.D.book.size() << " orders | sweeps " << S.D.sweeps << ", big " << S.D.bigSweeps
      << ", took out a swing / level " << S.D.tookOut << " | Sr today green " << gb << ", red " << rb << ", recorded only " << rec << "\n";
    o << "VOLUME_PROFILE," << (S.vapFault ? "VAP fault - remove and re-add the indicator" : "ok") << ",candles " << S.vapBars << ",no VAP " << S.vapMissing;
    if (S.D.V.now.ok) o << ",POC " << swr::pxText(S.D.V.now.poc, S.tick) << ",VAH " << swr::pxText(S.D.V.now.vah, S.tick) << ",VAL " << swr::pxText(S.D.V.now.val, S.tick);
    o << ",prior sessions " << S.D.V.done.size() << "\n";
    o << "LEVELS," << S.levelNote << ",ZigZag " << S.D.Z.bars.size() << " RTH candles " << S.D.Z.swings.size() << " swings (SwingRef " << swg::VERSION << ")\nRECORD," << S.recPath << ",lines " << S.recLines << ",failed " << S.recFails << ",bad lines " << S.recBad << "\n";
    o << "PARAMS,LRA-StopSweep-" << S.market << ".csv " << S.prm.why << ",min prices " << S.D.P.minPrices << ",refill " << S.D.P.refillPct << "% within " << S.D.P.windowS
      << " s and still there " << S.D.P.stayS << " s later,orders resting " << S.D.P.minRestS << " s+,swings back " << S.D.P.refPivots << ",key " << (S.D.P.useKey ? "on" : "off")
      << ",MenthorQ " << (S.D.P.useMq ? "on" : "off") << ",bullish " << swg::showText(S.D.P.showBull) << ",bearish " << swg::showText(S.D.P.showBear) << "\n";
    o << "SETTINGS," << S.set.why << ",bullish " << (S.set.b(swrset::K_SHOW_BULL) ? "on" : "off") << ",bearish " << (S.set.b(swrset::K_SHOW_BEAR) ? "on" : "off")
      << ",ZigZag " << (S.set.b(swrset::K_SHOW_ZZ) ? "on" : "off") << ",swing line " << (S.set.b(swrset::K_SHOW_REF) ? "on" : "off") << ",gap " << S.D.P.gapBars << " candles\nBACKLOG_SLICES," << S.backlogSlices
      << ",garbage " << S.garbage << ",book resets " << S.D.bookResets << "\n";
    o << "IMPLIED," << S.imp.why << ",fills " << S.D.impliedFills << ",lots " << S.D.impliedLots << " (side '?' FILLs: never sweep trades)\n";
    const std::string body = o.str();
    if (body == S.statusBody && now - S.statusMs < 30000 && now >= S.statusMs) return;
    if (now - S.statusMs < 1000 && now >= S.statusMs) return;
    S.statusMs = now; S.statusBody = body;
    swrMkdir(d); swrMkdir(d + SWR_SEP "SweepRefill");
    const std::string p = d + SWR_SEP "SweepRefill" SWR_SEP "SweepRefill.status-" + S.market + "-" + std::to_string(S.spb) + ".txt", tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return; f << body; f.flush(); if (!f) return; }
    std::remove(p.c_str()); std::rename(tmp.c_str(), p.c_str());
}

void SweepRefill::pump(bool fromDraw) {
    SwrState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = 1;
    struct Unbusy { SwrState* s; ~Unbusy() { s->busy = 0; } } unbusy = { S };
    (void)fromDraw;
    try {
        const long long now = swrMs();
        identify(*S);
        if (S->market.empty() || !(S->tick > 0) || S->spb <= 0) return;
        params(*S, now);
        settings(*S, now);
        levels(*S, now);
        loadRecord(*S);
        bars(*S);
        openFeed(*S);
        drain(*S, now);
        collect(*S);
        status(*S, now);
    } catch (...) {}
}

int SweepRefill::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    SwrState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    int hit = -1;
    for (size_t k = 0; k < S->hits.size() && hit < 0; ++k) { const HitBox& h = S->hits[k]; if (e->v.mouse.h >= h.l && e->v.mouse.h <= h.r && e->v.mouse.v >= h.t && e->v.mouse.v <= h.b) hit = (int)h.idx; }
    if (hit != S->hover) { S->hover = hit; invalidateChart(false); }
    return RTX_FAIL;                                                                  // never swallow the chart's own mouse behaviour
}

static short swrC(int v) { return (short)(v < -32768 ? -32768 : v > 32767 ? 32767 : v); }

void SweepRefill::render(SwrState& S) {
    S.hits.clear();
    const bool showBull = S.set.b(swrset::K_SHOW_BULL), showBear = S.set.b(swrset::K_SHOW_BEAR), showZz = S.set.b(swrset::K_SHOW_ZZ), showRef = S.set.b(swrset::K_SHOW_REF);
    if (!(S.tick > 0)) return;
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow); if ((long)times.count < n || (long)hi.count < n || (long)lo.count < n) return;
    RCT pane; pane.getPaneRect(false);
    auto barIdx = [&](long long t) -> long {          // the candle whose time (IRT bar END) == t, else -1
        long l0 = 0, h0 = n - 1, idx = -1;
        while (l0 <= h0) { const long mid = (l0 + h0) / 2; if ((long long)times[(int)mid] >= t) { idx = mid; h0 = mid - 1; } else l0 = mid + 1; }
        return idx >= 0 && (long long)times[(int)idx] == t ? idx : -1;
    };
    auto inPane = [&](int x, int y) { return x > pane.left && x < pane.right && y > pane.top && y < pane.bottom; };
    // the 1-bar ZigZag of today's RTH: a thin line through the CONFIRMED swing ends (a confirmed point never moves)
    if (showZz && S.D.Z.swings.size() >= 2) {
        setPen((COLOR)S.set.c(swrset::K_COL_ZZ), 1, P_SOLID);
        bool have = false; int px0 = 0, py0 = 0;
        for (size_t k = 0; k < S.D.Z.swings.size(); ++k) {
            const swg::Swing& w = S.D.Z.swings[k]; const long i = barIdx(w.barT); if (i < 0) { have = false; continue; }
            PNT p; p.set((int)i, (float)((double)w.px * S.tick));
            if (have && inPane(p.h, p.v) && inPane(px0, py0)) { PNT a; a.set((int)i, 0.0f); a.h = (short)px0; a.v = (short)py0; a.setDrawPosition(); p.drawLineTo(); }
            have = true; px0 = p.h; py0 = p.v;
        }
    }
    if ((!showBull && !showBear) || S.marks.empty()) return;
    const int fpt = std::max(8, std::min(14, S.set.i(swrset::K_FONT)));
    FONT f; f.id = HELVETICA; f.size = (short)fpt; f.style = BOLD; setFont(f);
    const int th = fpt + 4, lane = th + 2;
    for (size_t k = 0; k < S.marks.size(); ++k) {
        const swr::Event& e = S.marks[k]; if (!swr::shown(e, showBull, showBear)) continue;
        const long idx = barIdx(e.barT); if (idx < 0) continue;
        const bool up = swr::bullish(e);
        const COLOR col = (COLOR)(up ? S.set.c(swrset::K_COL_BULL) : S.set.c(swrset::K_COL_BEAR));
        // the short line from the swing (or level) taken out to the sweep candle, at the swing's price
        if (showRef && e.refPx > 0) {
            long i0 = e.refT > 0 ? barIdx(e.refT) : -1; if (i0 < 0 || i0 > idx) i0 = std::max(0L, idx - 3);
            PNT a; a.set((int)i0, (float)((double)e.refPx * S.tick)); PNT b; b.set((int)idx, (float)((double)e.refPx * S.tick));
            if (inPane(a.h, a.v) && inPane(b.h, b.v)) { setPen(col, 1, P_SOLID); a.setDrawPosition(); b.h = (short)(b.h + 4); b.drawLineTo(); }
        }
        // green UNDER the swept wick (the candle's low), red OVER the swept high - past the reserved lanes (lsBigBidAsk's BO / BD, Rly)
        PNT p; p.set((int)idx, up ? lo[(int)idx] : hi[(int)idx]); const int x = p.h;
        const int yEdge = up ? p.v + 2 + LANES_RESERVED * lane : p.v - 2 - LANES_RESERVED * lane;
        const int t = up ? yEdge : yEdge - th, b = t + th;
        if (t <= pane.top + 1 || b >= pane.bottom - 1 || x <= pane.left || x >= pane.right) continue;
        struct tm tkNw; std::memset(&tkNw, 0, sizeof tkNw); const nw::Gate ng = getLocaltime((RTDATE)e.barT, &tkNw) ? nw::atTm(S.market, tkNw) : nw::Gate(); if (ng.feedBad) continue;   // NewsWindow.h
        const std::string txtNw = nw::q(swr::markText(e), ng); const char* txt = txtNw.c_str();   // "Sr" (SHOW) / "Sr?" (promising or a news window)
        const int w = (int)getTextWidth(txt, -1) + 4, x0 = x - w / 2;
        setTextColor(col);
        RCT r; r.set(swrC(x0), swrC(t), swrC(x0 + w), swrC(b)); r.drawText(txt, true, false);
        HitBox hb; hb.l = x0; hb.t = t; hb.r = x0 + w; hb.b = b; hb.idx = k; S.hits.push_back(hb);
    }
    // the hover box of the mark under the mouse: fixed font, solid border, inside the pane
    if (S.hover >= 0 && (size_t)S.hover < S.marks.size()) {
        const HitBox* at = nullptr; for (size_t k = 0; k < S.hits.size(); ++k) if ((int)S.hits[k].idx == S.hover) at = &S.hits[k];
        if (!at) return;
        const swr::Event& e = S.marks[(size_t)S.hover];
        const std::vector<std::string> rows = swr::hoverRows(e, S.tick, S.D.P.windowS, S.D.P.stayS, S.D.P.refillPct);
        FONT hf; hf.id = HELVETICA; hf.size = HOVER_PT; hf.style = PLAIN; setFont(hf);
        int w = 0; for (size_t k = 0; k < rows.size(); ++k) w = std::max(w, (int)getTextWidth(rows[k].c_str(), -1));
        w += 12; const int h = (int)rows.size() * HOVER_LH + 8;
        int x0 = at->r + 8, y0 = at->t - h / 2;
        if (x0 + w > pane.right - 2) x0 = at->l - 8 - w;
        if (x0 < pane.left + 2) x0 = pane.left + 2;
        if (y0 + h > pane.bottom - 2) y0 = pane.bottom - 2 - h;
        if (y0 < pane.top + 2) y0 = pane.top + 2;
        const COLOR col = (COLOR)(swr::bullish(e) ? S.set.c(swrset::K_COL_BULL) : S.set.c(swrset::K_COL_BEAR));
        const COLOR bg = getBackgroundColor();
        RCT box; box.set(swrC(x0), swrC(y0), swrC(x0 + w), swrC(y0 + h)); box.draw(1, col, bg, DRAW_OPAQUE, PAT_SOLID);
        for (size_t k = 0; k < rows.size(); ++k) {
            setTextColor(k == 0 ? col : (COLOR)0xE5E7EBul);
            RCT r; r.set(swrC(x0 + 6), swrC(y0 + 4 + (int)k * HOVER_LH), swrC(x0 + w - 4), swrC(y0 + 4 + (int)(k + 1) * HOVER_LH)); r.drawText(rows[k].c_str(), false, false);
        }
    }
}

int SweepRefill::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    pump(true);
    SwrState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) {} }
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    SweepRefill* p = new SweepRefill();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | VAP_REQUIRED);   // (1.1.0) VAP: the POC / VAH / VAL levels
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the refill window is decided and the feed drained while the chart is quiet
    p->setDescription("LRA Sweep Refill v1.1.0 - Sr on the price chart. Green Sr under the swept wick: sellers swept 8+ bid prices through an intact swing low (one of the last 3 swings of the 1-bar ZigZag from the RTH open) or a key / MenthorQ level, at a local low, and the bids came back (half the size that rested there within 5 s, still there 5 s later on NQ; per market from the validation) - the stops were run and absorbed. Red Sr over the swept high: the mirror. Sr? = promising, not proven yet; markets / sides not proven are recorded only. A short line joins the swing taken out to the sweep candle; the thin line is the ZigZag. Never on two adjacent candles. Decided once, never repainted; every sweep is recorded for the nightly test. Hover a mark for the numbers. Needs the market-by-order feed; works from the moment it is on the chart. Example: NQ Oct 9 8:55 AM, 19 prices swept through the swing low from 8:48 AM, 87% back in 3 s -> green Sr.");
    p->setVersion("1.1.3");   // SWR_VERSION
    return p;
}
