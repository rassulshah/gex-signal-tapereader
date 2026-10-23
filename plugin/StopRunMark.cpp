/********************************************************************************
 *  StopRunMark.cpp  --  Investor/RT RTX extension  lsStopRun  1.2.0  (2026-10-22)
 *
 *  THE STOP-RUN MARK on the PRICE chart (MBO Roadmap "Were stops triggered - and were they absorbed after?", T-20261022-15).
 *  Rassul 15:33-15:34 CT (agreed design, "show yes"); 15:37 CT "Sta can indicate the stop run absorbed"; 1.1.0 approved 16:13 CT.
 *  1.2.0: Rassul approved the Sr mockup v4 rule at 17:39 CT "and the SAME swing rule on St / Sta" (18:05: the last 3 swings, as Sr);
 *  18:07 CT volume-profile levels count too; the St / Sta validation (VALIDATE-STOPRUN-SWEEP-2026-10-22.md) per market:
 *    - the reference = SwingRef.h, shared with lsSweepRefill 1.1.0: an intact swing among the last 3 swings of the 1-bar ZigZag from
 *      the RTH open, a key level (ONH/ONL PDH/PDL PFH/PFL LonHI/LonLO PWH/PWL AsiaHI/AsiaLO), a MenthorQ level, or a volume-profile
 *      level (POC / VAH / VAL today and the prior day, naked prior POCs); the burst ends beyond it, starts <= 0.5 ATR past it and at
 *      most near-ticks before it, and no candle before the last one had traded through it; 5-candle extremes no longer count;
 *    - burst >= 3.0x the normal stop burst; Sta decided LIVE at burst end + 10 s (NQ) = the last trade back across the level AND a
 *      resting order of >= 5 lots (NQ) filled at / beyond it (the 1.1 "50% eaten" and "no new extreme" rules are gone);
 *      markets with staDecideLive 0 decide at candle closes;
 *    - drawn per the verdict in lsFlexLevels\LRA-StopSweep-<MKT>.csv: SHOW, ? ("Sta?" / "St?"), LOG = recorded only. Today: NQ
 *      green Sta "?" only, GC grey St "?", everything else recorded, not drawn. Verdicts are stamped on each mark (never repainted);
 *    - on a candle that also has a drawn Sr (lsSweepRefill's record) only the Sr is drawn.
 *  The hover (mouse over the mark, drawn as a box): header "Stop run?", the run, the outcome, then
 *  question / what it is / why it matters / how to use / example.
 *  Rules: StopRunLogic.h + SwingRef.h (SDK-free, unit-tested; replay on real MBO in tests_st/replay_st.cpp).
 *
 *  FEED: one DEPTH_BY_ORDER per chart, kept alive for the chart's life, opened from calc / draw only behind a crash-guard file,
 *  drained <= 5,000 events / ~8 ms per call. FILL: orderID = the resting order, buySell = its side (1 bid / 0 offer),
 *  aggressorOrderID = the order that hit it. There is no historical order-by-order data: St exists from the moment the indicator is
 *  on the chart; the session record brings the marks back after a restart.
 *  FILES (lsFlexLevels\StopRun\):  <MKT>-<spb>-<yyyymmdd>.csv  the record (ST fire lines + SA decision lines, append-only)
 *                                  StopRun.status-<MKT>-<spb>.txt  the status line(s), on change or every 30 s
 *  Reads: lsFlexLevels\LRA-KeyLevels-<MKT>.csv / LRA-Levels-<MKT>.csv (NearLevels.h), LRA-StopSweep-<MKT>.csv (params),
 *  SweepRefill\<MKT>-<spb>-<yyyymmdd>.csv (the Sr record, for the same-candle rule). Settings: StopRunMark.settings-<MKT>.txt.
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
#include "StopRunLogic.h"
#include "SwingRef.h"
#include "SweepRefillLogic.h"
#include "NewsWindow.h"   // (2026-10-22 evening) news / settle window "?" + the feed check (record only) - shared, SDK-free
#include "NearLevels.h"
#include "HostSlot.h"
#include "StopRunMarkSettings.h"
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

#define SRM_VERSION "1.2.1"     // = stl::VERSION = srmset::PLUGIN_VERSION = the setVersion literal

#ifdef SRM_TEST_CLOCK
long long srmTestClockMs();
static long long srmMs() { return srmTestClockMs(); }
#else
static long long srmMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#endif

// bDboSubscribed is private: a pointer-to-member through explicit instantiation (as MboCheck.cpp), never by offset
template <class Tag, typename Tag::type M> struct SrmRob { friend typename Tag::type srmMember(Tag) { return M; } };
struct SrmDboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type srmMember(SrmDboFlag); };
template struct SrmRob<SrmDboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;

static const int SLICE_MAX = 5000; static const long long SLICE_BUDGET_MS = 8;
static const int HOVER_PT = 10, HOVER_LH = 14;

static std::string srmFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + SRM_SEP "InvestorRT" SRM_SEP "rtx" SRM_SEP "lsFlexLevels" : std::string(); }
static void srmMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool srmExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }

struct Mark { stl::Event e; };
struct HitBox { int l, t, r, b; size_t idx; };

// ---- the chart's volume at price (only trivial values live in these MSVC SEH helpers, as lsBigBidAsk) ------------------------------
static int srmStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st) {
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
static int srmRowsRaw(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static int srmVap(cppExtension::RTARRAYP& vap, int bar, double tick, std::vector<std::pair<long long, double> >& out) {
    out.clear();
    cppExtension::BARSTATISTICS st; const int n = srmStats(&vap, bar, &st);
    if (n == -1) return -1;
    if (n <= 0 || n > 5000) return 0;
    std::vector<cppExtension::VOLPROFILE> rows((size_t)n);
    const int got = srmRowsRaw(&vap, bar, &rows[0], n);
    if (got == -1) return -1;
    if (got != n) return 0;
    for (size_t i = 0; i < rows.size(); ++i) {
        long long k = 0; const double v = (double)rows[i].totalVolume;
        if (!stl::toTicks((double)rows[i].price, tick, k) || !(v > 0) || !stl::finite(v)) continue;
        out.push_back(std::make_pair(k, v));
    }
    return 1;
}

struct SrmState {
    // identity
    std::string identity, market, symbol; int spb = 0; double tick = 0;
    // feed
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr; bool dboDead = false, blocked = false; int attempt = 0; int flag = -1;
    long long events = 0, fills = 0, aggNamed = 0, lastResubMs = -1, backlogSlices = 0;
    bool vapFault = false; long long vapBars = 0, vapMissing = 0;
    // detector + bars
    stl::Detector D; bool primed = false; long long lastClosedT = 0; long long lastStampSec = 0;
    // levels
    nl::Cache lvCache; long long keyCheckMs = -100000; std::string keyNote = "not read yet"; int nKey = 0, nMq = 0;
    // record
    std::vector<stl::Event> marks; std::string session, recPath; long long recFails = 0, recLines = 0, recBad = 0; bool loaded = false;
    // settings
    ss::Store setStore; ss::Values set = ss::defaults(srmset::spec(), -1); long long setCheckMs = -100000;
    // the per-market params (LRA-StopSweep-<MKT>.csv) and the Sr record (the same-candle rule)
    swg::StopSweep prm; long long prmCheckMs = -100000, prmMtime = -1, prmSize = -1;
    std::set<long long> srBars; long long srCheckMs = -100000, srMtime = -1, srSize = -1; std::string srPath;
    // hover
    std::vector<HitBox> hits; int hover = -1;
    // status
    std::string statusBody; long long statusMs = -100000; long long busy = 0;
    ~SrmState() { delete dbo; }
};
static ss::Ids& srmIds() { static ss::Ids I; return I; }

class StopRunMark : public cppExtension {
public:
    SS_SAFE_DIALOG(StopRunMark, "StopRunMark")
    HostSlot<SrmState> slot_;
    StopRunMark() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &StopRunMark::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &StopRunMark::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &StopRunMark::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, srmset::spec(), srmIds(), st->load(ss::marketIndex(dlgMkt_), srmMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, srmset::spec(), srmIds(), (int)i, snap_); saveSettings(); }
    void pump(bool fromDraw);
    SrmState* testState() { return slot_.get(this, false); }
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = ss::safeMarket(this); const std::string p = srmset::settingsPath(srmFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &srmset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, srmset::spec(), srmIds(), st->load(ss::marketIndex(dlgMkt_), srmMs(), true), snap_);
        srmMkdir(srmFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, srmset::spec(), srmIds(), V));
    }
    void identify(SrmState& S);
    void settings(SrmState& S, long long now);
    void levels(SrmState& S, long long now);
    void params(SrmState& S, long long now);
    void srRecord(SrmState& S, long long now);
    std::string hmText(long long stamp);
    void openFeed(SrmState& S);
    void drain(SrmState& S, long long now);
    void bars(SrmState& S);
    void loadRecord(SrmState& S);
    void append(SrmState& S, const std::string& line);
    void collect(SrmState& S);
    void status(SrmState& S, long long now);
    std::string ctText(long long stamp);
    std::string sessionOf(RTDATE stamp);
    void render(SrmState& S);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<StopRunMark*>(this)->slot_.release(this); return RTX_OK; }      // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<StopRunMark*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, srmset::spec(), srmIds()); return RTX_OK; }
int cppExtension::calc(int) { static_cast<StopRunMark*>(this)->pump(false); return RTX_OK; }

std::string StopRunMark::ctText(long long stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (stamp <= 0 || !getLocaltime((RTDATE)stamp, &t)) return std::string();      // IRT's local clock = Central on his PC
    const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
    char b[64]; std::snprintf(b, sizeof b, "%d:%02d:%02d %s", h12, t.tm_min, t.tm_sec, t.tm_hour < 12 ? "AM" : "PM"); return b;
}
// the 17:00 CT trading day: an event at or after 17:00 belongs to the next day's session
std::string StopRunMark::sessionOf(RTDATE stamp) {
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

void StopRunMark::identify(SrmState& S) {
    const std::string mkt = ss::safeMarket(this);
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!stl::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = mkt + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    delete S.dbo; S.dbo = nullptr;
    S.identity = id; S.market = mkt; S.symbol = sym; S.spb = spb; S.tick = tick;
    S.dboDead = S.blocked = false; S.attempt = 0; S.flag = -1; S.events = S.fills = S.aggNamed = 0; S.lastResubMs = -1;
    S.D = stl::Detector(); S.primed = false; S.lastClosedT = 0; S.lastStampSec = 0;
    S.lvCache = nl::Cache(); S.keyCheckMs = -100000; S.nKey = S.nMq = 0; S.marks.clear(); S.session.clear(); S.recPath.clear(); S.loaded = false;
    S.setStore = ss::Store(); S.setCheckMs = -100000; S.hits.clear(); S.hover = -1; S.statusBody.clear();
    S.vapFault = false; S.vapBars = S.vapMissing = 0;
    S.prm = swg::StopSweep(); S.prmCheckMs = -100000; S.prmMtime = S.prmSize = -1;
    S.srBars.clear(); S.srCheckMs = -100000; S.srMtime = S.srSize = -1; S.srPath.clear();
    int oMin = 0, cMin = 0; if (swg::rthOf(S.market, oMin, cMin)) S.D.setRth(oMin, cMin, spb); else S.D.setRth(0, 0, spb);
}

// the params file, re-read only when its stamp changes, at most every 5 s; missing / garbled = defaults, every mark LOG
void StopRunMark::params(SrmState& S, long long now) {
    if (now - S.prmCheckMs < 5000 && now >= S.prmCheckMs) return;
    S.prmCheckMs = now;
    const std::string f = srmFlex(); if (f.empty() || S.market.empty()) return;
    const std::string p = f + SRM_SEP "LRA-StopSweep-" + S.market + ".csv";
    struct stat st; if (stat(p.c_str(), &st) != 0) { if (S.prmMtime != -2) { S.prm = swg::StopSweep(); S.prm.why = "no file (defaults, every mark LOG)"; S.prmMtime = -2; } return; }
    if ((long long)st.st_mtime == S.prmMtime && (long long)st.st_size == S.prmSize) return;
    std::vector<std::string> lines;
    if (!nl::readBoundedLines(p, lines, 64 * 1024, 512, 512)) return;
    struct stat st2; if (stat(p.c_str(), &st2) != 0 || st2.st_mtime != st.st_mtime || st2.st_size != st.st_size) return;
    S.prm = swg::parseStopSweep(lines, S.market); S.prmMtime = (long long)st.st_mtime; S.prmSize = (long long)st.st_size;
}
// the candles where lsSweepRefill DREW an Sr this session (refilled, not adjacent, verdict not LOG): only the Sr is drawn there
void StopRunMark::srRecord(SrmState& S, long long now) {
    if (now - S.srCheckMs < 3000 && now >= S.srCheckMs) return;
    S.srCheckMs = now;
    if (S.session.empty() || srmFlex().empty()) return;
    const std::string p = srmFlex() + SRM_SEP "SweepRefill" SRM_SEP + S.market + "-" + std::to_string(S.spb) + "-" + S.session + ".csv";
    if (p != S.srPath) { S.srPath = p; S.srBars.clear(); S.srMtime = S.srSize = -1; }
    struct stat st; if (stat(p.c_str(), &st) != 0) return;
    if ((long long)st.st_mtime == S.srMtime && (long long)st.st_size == S.srSize) return;
    std::vector<std::string> lines;
    if (!nl::readBoundedLines(p, lines, 4 * 1024 * 1024, 20000, 1024)) return;
    const std::vector<swr::Event> v = swr::parseRecord(lines, S.tick);
    std::set<long long> b; for (size_t i = 0; i < v.size(); ++i) if (v[i].refilled && v[i].gapOk && v[i].show != swg::LOG) b.insert(v[i].barT);
    S.srBars.swap(b); S.srMtime = (long long)st.st_mtime; S.srSize = (long long)st.st_size;
}
// "8:48 AM" (the swing candle's START = its END - seconds per bar)
std::string StopRunMark::hmText(long long stamp) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (stamp <= 0 || !getLocaltime((RTDATE)stamp, &t)) return std::string();
    const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
    char b[32]; std::snprintf(b, sizeof b, "%d:%02d %s", h12, t.tm_min, t.tm_hour < 12 ? "AM" : "PM"); return b;
}

void StopRunMark::settings(SrmState& S, long long now) {
    if (now - S.setCheckMs < 1000 && now >= S.setCheckMs) return;
    S.setCheckMs = now;
    const std::string sp = srmset::settingsPath(srmFlex(), S.market);
    if (S.setStore.path != sp) { S.setStore = ss::Store(); S.setStore.S = &srmset::spec(); S.setStore.path = sp; }
    S.set = sp.empty() ? ss::defaults(srmset::spec(), ss::marketIndex(S.market)) : S.setStore.load(ss::marketIndex(S.market), now);
    const stl::MarketDefaults md = stl::marketDefaults(S.market);
    stl::Params& P = S.D.P;   // a change applies from now on; decided marks never change
    if (!S.prm.loaded) { S.prm.stNearTicks = md.nearTicks; S.prm.staBigLots = md.bigLots; }   // no params file: the market's 1.1 defaults (every mark LOG)
    stl::applyParams(P, S.prm);                                // the validation's locked rule per market (params file)
    P.minIds = S.set.i(srmset::K_MIN_IDS); P.seedLots = md.seedLots;
}

void StopRunMark::levels(SrmState& S, long long now) {
    if (now - S.keyCheckMs < 5000 && now >= S.keyCheckMs) return;
    S.keyCheckMs = now;
    const std::string f = srmFlex(); if (f.empty() || S.market.empty() || !(S.tick > 0)) return;
    const std::vector<nl::Level> all = nl::levels(S.lvCache, f + SRM_SEP, S.market, S.symbol, now, (long long)std::time(nullptr));
    S.D.levels.clear(); S.nKey = S.nMq = 0;
    for (size_t i = 0; i < all.size(); ++i) {
        int rank = 0; const int d = stl::keyDir(all[i].name, rank);
        swg::Lvl L; L.name = all[i].name;
        if (d == 0 || !stl::toTicks(all[i].px, S.tick, L.px)) continue;            // only his named session levels + the 9 MenthorQ levels
        const bool mq = d == 2; L.kind = mq ? swg::MQ : swg::KEY; L.dir = mq ? 0 : d;
        bool dup = false; for (size_t k = 0; k < S.D.levels.size() && !dup; ++k) dup = S.D.levels[k].name == L.name && S.D.levels[k].px == L.px;
        if (dup) continue;
        S.D.levels.push_back(L); if (mq) ++S.nMq; else ++S.nKey;
    }
    S.keyNote = std::to_string(S.nKey) + " session + " + std::to_string(S.nMq) + " MenthorQ levels (" + S.lvCache.note + ")";
}

// chart context only, behind the crash guard; 1st try the chart's symbol, 2nd try NULL (= this chart) after 60 s with no event
void StopRunMark::openFeed(SrmState& S) {
    if (S.dbo || S.dboDead || S.blocked || S.attempt >= 2) return;
    const std::string dir = srmFlex() + SRM_SEP "StopRun";
    const std::string guard = srmFlex().empty() ? std::string() : dir + SRM_SEP "_opening-" + S.market + "-" + std::to_string(S.spb) + ".txt";
    if (S.attempt == 0 && srmExists(guard)) { S.blocked = true; return; }
    if (!guard.empty()) { srmMkdir(srmFlex()); srmMkdir(dir); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER\n"; }
    try { S.dbo = new (std::nothrow) DEPTH_BY_ORDER(S.attempt == 0 && !S.symbol.empty() ? S.symbol.c_str() : NULL); }
    catch (...) { S.dboDead = true; }
    ++S.attempt;
    if (!guard.empty()) std::remove(guard.c_str());
}

void StopRunMark::drain(SrmState& S, long long now) {
    if (!S.dbo || S.dboDead || !(S.tick > 0)) return;
    if (S.flag != 1 && S.lastResubMs >= 0 && now - S.lastResubMs < 1000 && now >= S.lastResubMs) return;
    const long long t0 = srmMs(); int n = 0; bool cap = false;
    long long lastSec = 0;
    try {
        if (!(S.dbo->*srmMember(SrmDboFlag()))) S.lastResubMs = now;
        while (true) {
            if (n >= SLICE_MAX) { cap = true; break; }
            if ((n & 511) == 511 && srmMs() - t0 > SLICE_BUDGET_MS) { cap = true; break; }
            if (S.dbo->getNext() != RTX_OK) break;
            ++n; ++S.events;
            const DEPTH_ORDER& o = S.dbo->dbo;
            if (o.action != 3) continue;                                              // FILL only
            const int restSide = o.buySell == 1 ? stl::DOWN : o.buySell == 0 ? stl::UP : 0;   // a resting BID hit = a DOWN run
            long long px = 0;
            if (!restSide || !stl::toTicks((double)o.price, S.tick, px) || o.size == 0) continue;
            long long sec = (long long)o.timestamp; if (sec < S.lastStampSec && S.lastStampSec - sec < 1800) sec = S.lastStampSec;   // a stale stamp (ArrivalStamp.h)
            S.lastStampSec = sec; lastSec = sec;
            ++S.fills; if (o.aggressorOrderID != 0) ++S.aggNamed;
            S.D.aggFill(now, sec, restSide, px, (long long)o.size, (long long)o.aggressorOrderID);
            S.D.restFill(now, restSide, px, (long long)o.size, (long long)o.orderID);
        }
        S.flag = (S.dbo->*srmMember(SrmDboFlag())) ? 1 : 0;
    } catch (...) { S.dboDead = true; }                                              // never re-enter a faulting call
    if (cap) ++S.backlogSlices;
    if (lastSec > 0) { struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)lastSec, &t)) S.D.minuteOfDay = t.tm_hour * 60 + t.tm_min; }
    if (!cap) S.D.tick(now, S.lastStampSec);                                         // the feed is caught up: a stale burst is over
}

// closed candles (a later bar exists), strictly in order, each once; history at the first pass gives the ZigZag (from today's RTH
// open), the ATR and the volume profile (the chart's VAP); each candle's CT day / RTH flag / session from IRT's local clock
void StopRunMark::bars(SrmState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYP vap(barVolumeProfile);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const bool haveVap = (long)vap.count >= n;
    S.D.formingT = (long long)times[(int)(n - 1)];
    int oMin = 0, cMin = 0; const bool haveRth = swg::rthOf(S.market, oMin, cMin);
    auto bar = [&](int i, stl::Bar& b, long long& sess) {
        b.t = (long long)times[i]; sess = 0;
        struct tm t; std::memset(&t, 0, sizeof t);
        if (getLocaltime((RTDATE)b.t, &t)) {
            b.day = (long long)(t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
            b.rth = haveRth && swg::isRth(t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec, S.spb, oMin, cMin);
        }
        const std::string ss0 = sessionOf((RTDATE)(b.t - 1)); sess = ss0.empty() ? 0 : std::strtoll(ss0.c_str(), nullptr, 10);
        return stl::toTicks(op[i], S.tick, b.o) && stl::toTicks(hi[i], S.tick, b.h) && stl::toTicks(lo[i], S.tick, b.l) && stl::toTicks(cl[i], S.tick, b.c);
    };
    const int lastClosed = (int)n - 2;
    int from;
    if (!S.primed) { S.primed = true; from = std::max(0, lastClosed - 2000); }
    else { if ((long long)times[lastClosed] <= S.lastClosedT) return; from = lastClosed; while (from > 0 && (long long)times[from - 1] > S.lastClosedT) --from; }
    std::vector<std::pair<long long, double> > rows;
    for (int i = from; i <= lastClosed; ++i) {
        stl::Bar b; long long sess = 0; if (!bar(i, b, sess)) continue;
        int vr = 0; if (haveVap && !S.vapFault) { vr = srmVap(vap, i, S.tick, rows); if (vr < 0) S.vapFault = true; }
        if (vr > 0) ++S.vapBars; else ++S.vapMissing;
        S.D.barClose(b, vr > 0 ? &rows : nullptr, sess);
    }
    S.lastClosedT = (long long)times[lastClosed];
}

void StopRunMark::loadRecord(SrmState& S) {
    const std::string sess = sessionOf(currentDate());
    if (sess.empty() || S.market.empty()) return;
    if (S.loaded && sess == S.session) return;
    S.session = sess; S.loaded = true; S.marks.clear(); S.D.marksPerBar.clear(); S.D.used.clear();   // a new session: every level's stops are fresh
    const std::string dir = srmFlex() + SRM_SEP "StopRun";
    S.recPath = srmFlex().empty() ? std::string() : dir + SRM_SEP + S.market + "-" + std::to_string(S.spb) + "-" + sess + ".csv";
    std::vector<std::string> lines;
    if (S.recPath.empty() || !nl::readBoundedLines(S.recPath, lines, 4 * 1024 * 1024, 20000, 1024)) return;
    S.marks = stl::parseRecord(lines, S.tick, &S.recBad);
    for (size_t i = 0; i < S.marks.size(); ++i) { if (S.marks[i].refT > 0) S.marks[i].refCt = hmText(S.marks[i].refT - S.spb); if (S.marks[i].outcome == stl::PENDING) S.marks[i].orphan = true; if (S.marks[i].drawn) ++S.D.marksPerBar[S.marks[i].barT]; S.D.markUsed(S.marks[i].side, S.marks[i].refPx); }
}

// one flushed append per line; a failure is counted (never retried in a loop)
void StopRunMark::append(SrmState& S, const std::string& line) {
    if (S.recPath.empty()) { ++S.recFails; return; }
    srmMkdir(srmFlex()); srmMkdir(srmFlex() + SRM_SEP "StopRun");
    const bool fresh = !srmExists(S.recPath);
    std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
    if (!f) { ++S.recFails; return; }
    if (fresh) f << stl::REC_HEAD << "\n";
    f << line << "\n"; f.flush();
    if (!f) { ++S.recFails; return; }
    ++S.recLines;
}

// the detector's fires and decisions -> marks + record (a decision only ever upgrades a PENDING mark, once)
void StopRunMark::collect(SrmState& S) {
    for (size_t i = 0; i < S.D.fired.size(); ++i) {
        stl::Event e = S.D.fired[i]; e.ct = ctText(e.firedSec); if (e.refT > 0) e.refCt = hmText(e.refT - S.spb);
        bool dup = false; for (size_t k = 0; k < S.marks.size() && !dup; ++k) dup = S.marks[k].key == e.key;
        if (dup) continue;
        S.marks.push_back(e); append(S, stl::stLine(e, S.tick));
    }
    S.D.fired.clear();
    for (size_t i = 0; i < S.D.decided.size(); ++i) {
        const stl::Event& d = S.D.decided[i];
        for (size_t k = 0; k < S.marks.size(); ++k) {
            stl::Event& m = S.marks[k];
            if (m.key != d.key || m.outcome != stl::PENDING) continue;
            m.outcome = d.outcome; m.decidedT = d.decidedT; m.bars = d.bars; m.bigIds = d.bigIds; m.excAtr = d.excAtr; m.reclaimed = d.reclaimed;
            m.decidedSec = d.decidedSec; m.decidedMs = d.decidedMs; m.sharePct = d.sharePct; m.newExt = d.newExt; m.bigLotsMax = d.bigLotsMax; m.show = d.show;
            append(S, stl::saLine(m)); break;
        }
    }
    S.D.decided.clear();
    if (S.marks.size() > 5000) S.marks.erase(S.marks.begin(), S.marks.begin() + (long)(S.marks.size() - 5000));
}

void StopRunMark::status(SrmState& S, long long now) {
    const std::string d = srmFlex(); if (d.empty() || S.market.empty()) return;
    long long abs = 0, nab = 0, pend = 0; for (size_t i = 0; i < S.marks.size(); ++i) { const int o = S.marks[i].outcome; if (o == stl::ABSORBED) ++abs; else if (o == stl::NOT_ABSORBED) ++nab; else ++pend; }
    std::ostringstream o;
    o << "VERSION," << SRM_VERSION << "\nMARKET," << S.market << ",SYMBOL," << S.symbol << ",SECONDS_PER_BAR," << S.spb << "\n";
    o << "STATUS,lsStopRun v" << SRM_VERSION << " | MBO " << (S.blocked ? "not opened (crash guard file present)" : S.dboDead ? "faulted" : S.flag == 1 ? "subscribed" : S.flag == 0 ? "not subscribed" : "opening")
      << " | " << S.events << " events, " << S.fills << " fills (" << S.aggNamed << " with aggressor id) | bursts " << S.D.bursts << ", qualifying " << S.D.qualifying
      << ", at a level " << S.D.nearLevel << ", big enough " << S.D.sized << " | St today " << S.marks.size() << " (Sta " << abs << ", not absorbed " << nab << ", deciding " << pend << ")\n";
    char nb[64]; std::snprintf(nb, sizeof nb, "%.1f", S.D.normNow(stl::slotOfMinute(S.D.minuteOfDay)));
    o << "NORMAL_BURST," << nb << " lots (slot " << S.D.minuteOfDay / 30 << ")\nKEY_LEVELS," << S.keyNote << "\nRECORD," << S.recPath << ",lines " << S.recLines << ",failed " << S.recFails << ",bad lines " << S.recBad << "\n";
    o << "PARAMS,LRA-StopSweep-" << S.market << ".csv " << S.prm.why << ",window " << S.D.P.windowMs << " ms,min fills " << S.D.P.minFills << ",ratio " << S.D.P.ratio
      << "x,near " << S.D.P.nearTicks << " ticks,big " << S.D.P.bigLots << " lots," << (S.D.P.decideLive ? "live at " + std::to_string(S.D.P.reclaimS) + " s" : "at " + std::to_string(S.D.P.absorbBars) + " candle closes")
      << ",swings back " << S.D.P.refPivots << ",key " << (S.D.P.useKey ? "on" : "off") << ",MenthorQ " << (S.D.P.useMq ? "on" : "off")
      << ",Sta bullish " << swg::showText(S.D.P.showBull) << ",Sta bearish " << swg::showText(S.D.P.showBear) << ",St " << swg::showText(S.D.P.showGrey) << "\n";
    o << "REFERENCE,ZigZag " << S.D.Z.bars.size() << " RTH candles " << S.D.Z.swings.size() << " swings (SwingRef " << swg::VERSION << "),volume profile " << (S.vapFault ? "VAP fault" : "ok")
      << " candles " << S.vapBars << " (no VAP " << S.vapMissing << ")" << (S.D.V.now.ok ? ",POC " + stl::pxText(S.D.V.now.poc, S.tick) + " VAH " + stl::pxText(S.D.V.now.vah, S.tick) + " VAL " + stl::pxText(S.D.V.now.val, S.tick) : std::string())
      << ",Sr candles " << S.srBars.size() << "\n";
    o << "SETTINGS," << S.set.why << ",show " << (S.set.b(srmset::K_SHOW) ? "on" : "off") << ",min orders " << S.D.P.minIds << "\nBACKLOG_SLICES," << S.backlogSlices << "\n";
    const std::string body = o.str();
    if (body == S.statusBody && now - S.statusMs < 30000 && now >= S.statusMs) return;
    if (now - S.statusMs < 1000 && now >= S.statusMs) return;
    S.statusMs = now; S.statusBody = body;
    srmMkdir(d); srmMkdir(d + SRM_SEP "StopRun");
    const std::string p = d + SRM_SEP "StopRun" SRM_SEP "StopRun.status-" + S.market + "-" + std::to_string(S.spb) + ".txt", tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return; f << body; f.flush(); if (!f) return; }
    std::remove(p.c_str()); std::rename(tmp.c_str(), p.c_str());
}

void StopRunMark::pump(bool fromDraw) {
    SrmState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = 1;
    struct Unbusy { SrmState* s; ~Unbusy() { s->busy = 0; } } unbusy = { S };
    (void)fromDraw;
    try {
        const long long now = srmMs();
        identify(*S);
        if (S->market.empty() || !(S->tick > 0)) return;
        params(*S, now);
        settings(*S, now);
        levels(*S, now);
        loadRecord(*S);
        srRecord(*S, now);
        openFeed(*S);
        drain(*S, now);
        bars(*S);
        collect(*S);
        status(*S, now);
    } catch (...) {}
}

int StopRunMark::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    SrmState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    int hit = -1;
    for (size_t k = 0; k < S->hits.size() && hit < 0; ++k) { const HitBox& h = S->hits[k]; if (e->v.mouse.h >= h.l && e->v.mouse.h <= h.r && e->v.mouse.v >= h.t && e->v.mouse.v <= h.b) hit = (int)h.idx; }
    if (hit != S->hover) { S->hover = hit; invalidateChart(false); }
    return RTX_FAIL;                                                                  // never swallow the chart's own mouse behaviour
}

static short srmC(int v) { return (short)(v < -32768 ? -32768 : v > 32767 ? 32767 : v); }

void StopRunMark::render(SrmState& S) {
    S.hits.clear();
    if (!S.set.b(srmset::K_SHOW) || S.marks.empty() || !(S.tick > 0)) return;
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); if ((long)times.count < n) return;
    RCT pane; pane.getPaneRect(false);
    const int fpt = std::max(8, std::min(14, S.set.i(srmset::K_FONT)));
    FONT f; f.id = HELVETICA; f.size = (short)fpt; f.style = BOLD; setFont(f);
    const int th = fpt + 4;
    std::vector<std::pair<int, std::pair<int, int> > > placed;   // bar index -> (x0, y) of marks drawn this pass (no overlap in one candle)
    for (size_t k = 0; k < S.marks.size(); ++k) {
        const stl::Event& e = S.marks[k]; if (!stl::drawn(e) || S.srBars.count(e.barT)) continue;   // LOG = recorded only; an Sr candle draws only the Sr
        // the candle: the first bar whose time >= the mark's bar key (IRT bar END)
        long lo = 0, hi = n - 1, idx = -1;
        while (lo <= hi) { const long mid = (lo + hi) / 2; if ((long long)times[(int)mid] >= e.barT) { idx = mid; hi = mid - 1; } else lo = mid + 1; }
        if (idx < 0 || (long long)times[(int)idx] != e.barT) continue;
        PNT p; p.set((int)idx, (float)((double)e.stPx * S.tick)); const int x = p.h, y = p.v;
        if (y <= pane.top + th || y >= pane.bottom - th || x <= pane.left || x >= pane.right) continue;
        struct tm tkNw; std::memset(&tkNw, 0, sizeof tkNw); const nw::Gate ng = getLocaltime((RTDATE)e.barT, &tkNw) ? nw::atTm(S.market, tkNw) : nw::Gate(); if (ng.feedBad) continue;   // NewsWindow.h
        const std::string txt = nw::q(stl::markText(e), ng);
        const int w = (int)getTextWidth(txt.c_str(), -1) + 4;
        int x0 = x - w / 2;
        for (size_t j = 0; j < placed.size(); ++j) if (placed[j].first == (int)idx && std::abs(placed[j].second.second - y) < th && std::abs(placed[j].second.first - x0) < w) x0 = placed[j].second.first + w + 2;
        placed.push_back(std::make_pair((int)idx, std::make_pair(x0, y)));
        const int c = stl::markColour(e);
        setTextColor((COLOR)(c == 1 ? S.set.c(srmset::K_COL_BULL) : c == 2 ? S.set.c(srmset::K_COL_BEAR) : S.set.c(srmset::K_COL_ST)));
        RCT r; r.set(srmC(x0), srmC(y - th / 2), srmC(x0 + w), srmC(y + th / 2)); r.drawText(txt.c_str(), true, false);
        HitBox hb; hb.l = x0; hb.t = y - th / 2; hb.r = x0 + w; hb.b = y + th / 2; hb.idx = k; S.hits.push_back(hb);
    }
    // the hover box of the mark under the mouse: fixed font, solid border, inside the pane
    if (S.hover >= 0 && (size_t)S.hover < S.marks.size()) {
        const HitBox* at = nullptr; for (size_t k = 0; k < S.hits.size(); ++k) if ((int)S.hits[k].idx == S.hover) at = &S.hits[k];
        if (!at) return;
        const std::vector<std::string> rows = stl::hoverRows(S.marks[(size_t)S.hover], S.tick, S.D.P.reclaimS, S.D.P.bigLots);
        FONT hf; hf.id = HELVETICA; hf.size = HOVER_PT; hf.style = PLAIN; setFont(hf);
        int w = 0; for (size_t k = 0; k < rows.size(); ++k) w = std::max(w, (int)getTextWidth(rows[k].c_str(), -1));
        w += 12; const int h = (int)rows.size() * HOVER_LH + 8;
        int x0 = at->r + 8, y0 = at->t - h / 2;
        if (x0 + w > pane.right - 2) x0 = at->l - 8 - w;
        if (x0 < pane.left + 2) x0 = pane.left + 2;
        if (y0 + h > pane.bottom - 2) y0 = pane.bottom - 2 - h;
        if (y0 < pane.top + 2) y0 = pane.top + 2;
        const COLOR bg = getBackgroundColor();
        RCT box; box.set(srmC(x0), srmC(y0), srmC(x0 + w), srmC(y0 + h)); box.draw(1, (COLOR)S.set.c(srmset::K_COL_ST), bg, DRAW_OPAQUE, PAT_SOLID);
        for (size_t k = 0; k < rows.size(); ++k) {
            const stl::Event& e = S.marks[(size_t)S.hover]; const int c = k == 0 ? stl::markColour(e) : 0;
            setTextColor((COLOR)(k == 0 ? (c == 1 ? S.set.c(srmset::K_COL_BULL) : c == 2 ? S.set.c(srmset::K_COL_BEAR) : S.set.c(srmset::K_COL_ST)) : 0xE5E7EBul));
            RCT r; r.set(srmC(x0 + 6), srmC(y0 + 4 + (int)k * HOVER_LH), srmC(x0 + w - 4), srmC(y0 + 4 + (int)(k + 1) * HOVER_LH)); r.drawText(rows[k].c_str(), false, false);
        }
    }
}

int StopRunMark::draw(void) {
    pump(true);
    SrmState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) {} }
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    StopRunMark* p = new StopRunMark();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | VAP_REQUIRED);   // (1.2.0) VAP: the POC / VAH / VAL levels
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the burst ends and the feed is drained while the chart is quiet
    p->setDescription("LRA Stop Run v1.2.0 - St / Sta on the price chart. St marks where stop orders fired: a burst of many small market orders in one moment (3x the normal burst), just through an intact swing (one of the last 3 swings of the 1-bar ZigZag from the RTH open), a key level, a MenthorQ level or a volume-profile level (POC / VAH / VAL). 10 s later (NQ) it is decided live: back across the level and a big resting order filled there = Sta, green when stops below a low were absorbed (bullish), red above a high. Shown per market only where the validation proved it (NQ green Sta?, GC grey St?); everything else is recorded, not drawn. On a candle with an Sr only the Sr is drawn. Hover a mark for the numbers. Needs the market-by-order feed. Example: St at 7672.25 below the swing low from 8:48 AM, 3.4x; 10 s later back above and a 12-lot bid filled -> green Sta.");
    p->setVersion("1.2.1");   // SRM_VERSION
    return p;
}
