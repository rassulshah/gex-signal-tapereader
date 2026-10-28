/********************************************************************************
 *  TrappedTraders.cpp  --  Investor/RT RTX extension  lsTrappedTraders 0.1.1  (its own pane, "Who is trapped?")  (2026-10-28; 0.1.0 2026-10-26)
 *  0.1.1 (Rassul 20:21 / 20:31 CT 10-27): his ES chart showed "candles 135 (without volume at price 135)" - the pane got no volume at price.
 *        Now: (1) the status says WHICH branch (array short / rows not loaded / fault); (2) a candle without volume at price takes the
 *        trap lots lsDeltaProfile 2.28.0 posts for it (SignalBoard 1.4.0 kind 'C': sellers below the close, buyers above it, decided);
 *        (3) marks everywhere by default (every forming pivot's Stage 3 trigger; "Levels only" stays a choice); (4) each decided trigger is
 *        posted as kind 'T' (layer 1a, read by the READ / combiner only where the tests show value).
 *
 *  Rassul 2026-10-26 20:14-20:15 CT: "this makes the other underwater trader better" / "lets just call underwater traders trapped
 *  traders. build it. its also simple and complements defense better right ?". Replaces the 'Who is aggressive?' read as the partner of
 *  the Defense pane ('Are they defending?'). Mockup: outputs/trapped_traders/mockup/index.html (ES 10-26, real data).
 *
 *  Per closed candle, two UPWARD bars from one baseline: GREEN = sell lots traded BELOW the close (sellers trapped), RED = buy lots
 *  traded ABOVE the close (buyers trapped). Green > red = bullish, red > green = bearish. Stage 3 trigger: a triangle + ratio over
 *  the winning bar on the first candle (the ZigZag extreme through 3 after it, his pivot rule) where the turn side flips ahead.
 *  Default: marks only at his trade levels / gamma levels (setting "Trigger marks": Levels only / Everywhere / Off). No letters.
 *  Rules: TrappedTradersLogic.h (SDK-free; tests_trappedtraders on ES 10-26 + NQ 10-22, parity with the Python reference).
 *
 *  DATA: the chart's candles + volume at price per candle (VAP_REQUIRED; VOLPROFILE sellVolume = sold at the bid = aggressive sells,
 *  buyVolume = bought at the ask) - the same aggressor-classified lots the order-by-order feed gives, but available for history too
 *  (no MBO dependency, no external files at run time except the level files every level-aware plugin reads). SYM_TICKINCR only.
 *  FILES (lsFlexLevels\TrappedTraders\): <MKT>-<CONTRACT>-<spb>-<session>.csv  TTB (per candle) / TTT (per trigger), append-only,
 *        the record wins (no repaint);  TrappedTraders.status-<MKT>-<spb>.txt (on change or every 30 s, PERF + AWARENESS lines).
 *  BOARD: MarketState source TRAPTR (D.g, D.r, D.trig) once per closed candle, only while lsperf::smartAllowed.
 *  Settings: lsFlexLevels\TrappedTraders.settings-<MKT>.txt (TrappedTradersSettings.h, SettingsSpec layer).
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "TrappedTraders"
#endif
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "TrappedTradersLogic.h"
#include "HostSlot.h"
#include "TrappedTradersSettings.h"
#include "SettingsHost.h"
#include "SettingsGuard.h"
#include "NearLevels.h"
#include "MarketState.h"
#include "SignalBoard.h"   // (0.1.1) reads lsDeltaProfile's candle trap lots (C), posts the decided triggers (T)
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <dirent.h>
#endif

#define TTR_VERSION "0.1.1"     // = ttr::VERSION = ttrset::PLUGIN_VERSION = the setVersion literal

static const int HOVER_PT = 10, HOVER_LH = 14;
static const int VAP_READS_PER_PASS = 400, VAP_RETRY_PASSES = 20;

static long long ttMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string ttFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + TTR_SEP "InvestorRT" TTR_SEP "rtx" TTR_SEP "lsFlexLevels" : std::string(); }
static void ttMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool ttExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
static std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i]; out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}
static bool readLines(const std::string& p, std::vector<std::string>& out, size_t capBytes, size_t capLines) {
    out.clear(); std::ifstream f(p.c_str(), std::ios::binary); if (!f) return false;
    f.seekg(0, std::ios::end); const std::streamoff sz = f.tellg(); if (sz < 0 || sz > (std::streamoff)capBytes) return false; f.seekg(0, std::ios::beg);
    std::string ln; while (std::getline(f, ln) && out.size() < capLines) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); out.push_back(ln); }
    return true;
}
static std::vector<std::pair<long long, std::string> > listRecords(const std::string& dir, const std::string& mkt, int spb) {
    std::vector<std::pair<long long, std::string> > out; std::vector<std::string> names;
#if defined(_WIN32)
    WIN32_FIND_DATAA fd; const std::string pat = dir + TTR_SEP + mkt + "-*-" + std::to_string(spb) + "-*.csv";
    HANDLE h = FindFirstFileA(pat.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) { do { if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.push_back(fd.cFileName); } while (FindNextFileA(h, &fd) && names.size() < 4000); FindClose(h); }
#else
    if (DIR* d = opendir(dir.c_str())) { while (dirent* e = readdir(d)) { names.push_back(e->d_name); if (names.size() >= 4000) break; } closedir(d); }
#endif
    const std::string pre = mkt + "-", mid = "-" + std::to_string(spb) + "-";
    for (size_t i = 0; i < names.size(); ++i) {
        const std::string& n = names[i];
        if (n.size() < pre.size() + 8 || n.compare(0, pre.size(), pre) != 0 || n.compare(n.size() - 4, 4, ".csv") != 0) continue;
        const size_t dash = n.rfind('-', n.size() - 5); if (dash == std::string::npos || dash < pre.size()) continue;
        long long s = 0; if (!ttr::fullLL(n.substr(dash + 1, n.size() - 5 - dash), s) || s <= 0) continue;
        const size_t d2 = n.rfind('-', dash - 1); if (d2 == std::string::npos || n.substr(d2, dash - d2 + 1) != mid) continue;
        out.push_back(std::make_pair(s, dir + TTR_SEP + n));
    }
    std::sort(out.begin(), out.end());
    return out;
}
// his trade levels + gamma levels only (09-30: weekly levels and the prior value area / POC are excluded; VWAP is not a trade level)
static bool tradeLevel(const std::string& n) {
    static const char* const skip[] = { "PW", "VA", "POC", "DPoc", "WPoc", "VWAP", "Poc", "MPoc", nullptr };
    for (int i = 0; skip[i]; ++i) if (n.compare(0, std::strlen(skip[i]), skip[i]) == 0) return false;
    return !n.empty();
}

// ---- the chart's volume at price (trivial values only inside the MSVC SEH helpers, as lsTrapped / lsSweepRefill) --------------------
static int ttStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st) {
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
static int ttRowsRaw(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count) {
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
// -1 = a fault (stop reading VAP), 0 = not loaded yet, 1 = ok
static int ttVap(cppExtension::RTARRAYP& vap, int bar, double tick, std::vector<ttr::Cell>& out) {
    out.clear();
    cppExtension::BARSTATISTICS st; const int n = ttStats(&vap, bar, &st);
    if (n == -1) return -1;
    if (n <= 0 || n > 5000) return 0;
    std::vector<cppExtension::VOLPROFILE> rows((size_t)n);
    const int got = ttRowsRaw(&vap, bar, &rows[0], n);
    if (got == -1) return -1;
    if (got != n) return 0;
    std::map<long long, ttr::Cell> m;
    for (size_t i = 0; i < rows.size(); ++i) {
        long long k = 0;
        if (!ttr::toTicks((double)rows[i].price, tick, k) || rows[i].buyVolume < 0 || rows[i].sellVolume < 0) continue;
        ttr::Cell& c = m[k]; c.px = k; c.ask += (long long)rows[i].buyVolume; c.bid += (long long)rows[i].sellVolume;
    }
    for (std::map<long long, ttr::Cell>::const_iterator it = m.begin(); it != m.end(); ++it) out.push_back(it->second);
    return out.empty() ? 0 : 1;
}

struct HitBox { int l, t, r, b; size_t idx; };

struct TtState {
    std::string identity, market, symbol, chart; int spb = 0; double tick = 0;
    ttr::Engine E; bool loaded = false; long long lastFedT = 0, retryT = 0; int retries = 0; bool vapFault = false;
    long long fed = 0, noVap = 0, recLines = 0, recFails = 0, recBad = 0, files = 0;
    long long vapShort = 0, vapNotLoaded = 0, vapOk = 0, fromC = 0, cWaits = 0; long vapCount = 0, barCount = 0; bool cProducer = false; size_t cFacts = 0;   // (0.1.1)
    long long tPostMs = -100000; size_t tPosted = 0; long long startMs = -1;
    ss::Store setStore; ss::Values set = ss::defaults(ttrset::spec(), -1); long long setCheckMs = -100000;
    nl::Cache lv; std::string lvNote; long long lvMs = -100000;
    std::vector<HitBox> hits; int hover = -1; double top = 1; int paneH = 0;
    std::string statusBody; long long statusMs = -100000, postMs = -100000, postedT = 0; int busy = 0;
};
static ss::Ids& ttIds() { static ss::Ids I; return I; }

class TrappedTraders : public cppExtension {
public:
    SS_SAFE_DIALOG(TrappedTraders, "TrappedTraders")
    HostSlot<TtState> slot_;
    TrappedTraders() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &TrappedTraders::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &TrappedTraders::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &TrappedTraders::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, ttrset::spec(), ttIds(), st->load(ss::marketIndex(dlgMkt_), ttMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, ttrset::spec(), ttIds(), (int)i, snap_); saveSettings(); }
    void pump();
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = ss::safeMarket(this); const std::string p = ttrset::settingsPath(ttFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &ttrset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, ttrset::spec(), ttIds(), st->load(ss::marketIndex(dlgMkt_), ttMs(), true), snap_);
        ttMkdir(ttFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, ttrset::spec(), ttIds(), V));
    }
    void identify(TtState& S);
    void settings(TtState& S, long long now);
    void loadFiles(TtState& S);
    void levels(TtState& S, long long now);
    void bars(TtState& S);
    void flush(TtState& S);
    void post(TtState& S, long long now);
    void postT(TtState& S, long long now);
    void status(TtState& S, long long now);
    long long ctSec(long long t);
    std::string hm(long long t);
    std::string recPath(const TtState& S, long long sess);
    bool showMark(const TtState& S, const ttr::Trigger& T) const;
    void hoverBox(TtState& S, const RCT& pane);
    void renderPane(TtState& S);
    double topFor(TtState& S, int i0, int i1);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<TrappedTraders*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<TrappedTraders*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, ttrset::spec(), ttIds()); return RTX_OK; }
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<TrappedTraders*>(this)->pump(); return RTX_OK; }

long long TrappedTraders::ctSec(long long t) {
    struct tm m; std::memset(&m, 0, sizeof m);
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return 0;
    int y = m.tm_year + 1900; const unsigned mo = (unsigned)m.tm_mon + 1, d = (unsigned)m.tm_mday;
    y -= mo <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + (long long)doe - 719468) * 86400 + m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec;
}
std::string TrappedTraders::hm(long long t) {
    struct tm m; std::memset(&m, 0, sizeof m);
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", m.tm_hour % 12 == 0 ? 12 : m.tm_hour % 12, m.tm_min, m.tm_hour < 12 ? "AM" : "PM"); return b;
}
std::string TrappedTraders::recPath(const TtState& S, long long sess) {
    const std::string f = ttFlex(); if (f.empty() || S.market.empty()) return std::string();
    return f + TTR_SEP "TrappedTraders" TTR_SEP + S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb) + "-" + std::to_string(sess) + ".csv";
}
bool TrappedTraders::showMark(const TtState& S, const ttr::Trigger& T) const {
    const int m = S.set.i(ttrset::K_MARKS);
    return m == ttrset::MARKS_ALL || (m == ttrset::MARKS_LEVELS && T.atLevel);
}

void TrappedTraders::identify(TtState& S) {
    const std::string mkt = ss::safeMarket(this);
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    const char* cl = getChartLabel(); const std::string chart = cl ? cl : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!ttr::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    else { char b[32]; std::snprintf(b, sizeof b, "%.6g", tick); tick = std::strtod(b, nullptr); }
    const std::string id = mkt + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) { S.chart = chart; return; }
    S.identity = id; S.market = mkt; S.symbol = sym; S.chart = chart; S.spb = spb; S.tick = tick;
    S.E = ttr::Engine(); S.loaded = false; S.lastFedT = 0; S.retryT = 0; S.retries = 0; S.vapFault = false;
    S.fed = S.noVap = S.recLines = S.recFails = S.recBad = S.files = 0;
    S.vapShort = S.vapNotLoaded = S.vapOk = S.fromC = S.cWaits = 0; S.vapCount = S.barCount = 0; S.cProducer = false; S.cFacts = 0; S.tPostMs = -100000; S.tPosted = 0; S.startMs = -1;
    S.setStore = ss::Store(); S.setCheckMs = -100000; S.lv = nl::Cache(); S.lvMs = -100000; S.hits.clear(); S.hover = -1; S.statusBody.clear(); S.postedT = 0;
}
void TrappedTraders::settings(TtState& S, long long now) {
    if (now - S.setCheckMs < 1000 && now >= S.setCheckMs) return;
    S.setCheckMs = now;
    const std::string sp = ttrset::settingsPath(ttFlex(), S.market);
    if (S.setStore.path != sp) { S.setStore = ss::Store(); S.setStore.S = &ttrset::spec(); S.setStore.path = sp; S.setStore.upgrade = &ttrset::upgrade; }
    S.set = sp.empty() ? ss::defaults(ttrset::spec(), ss::marketIndex(S.market)) : S.setStore.load(ss::marketIndex(S.market), now);
}
void TrappedTraders::loadFiles(TtState& S) {
    if (S.loaded) return;
    S.loaded = true;
    const std::string f = ttFlex(); if (f.empty()) return;
    std::vector<std::pair<long long, std::string> > L = listRecords(f + TTR_SEP "TrappedTraders", S.market, S.spb);
    if (L.size() > 30) L.erase(L.begin(), L.begin() + (long)(L.size() - 30));
    for (size_t i = 0; i < L.size(); ++i) {
        std::vector<std::string> lines; if (!readLines(L[i].second, lines, 2 * 1024 * 1024, 20000)) continue;
        ++S.files; S.recBad += S.E.loadRecord(lines);
    }
}
// his levels (key levels + MenthorQ / gamma, the files every level-aware plugin reads) and the level band (PARAM: the zone width x 2)
void TrappedTraders::levels(TtState& S, long long now) {
    if (now - S.lvMs < 5000 && now >= S.lvMs) return;
    S.lvMs = now;
    const std::string dir = ttFlex().empty() ? std::string() : ttFlex() + TTR_SEP;
    const std::vector<nl::Level> v = nl::levels(S.lv, dir, S.market, S.symbol, now, (long long)time(NULL));
    std::vector<ttr::Level> out;
    for (size_t i = 0; i < v.size(); ++i) { long long k = 0; if (tradeLevel(v[i].name) && ttr::toTicks(v[i].px, S.tick, k)) { ttr::Level L; L.name = v[i].name; L.px = k; out.push_back(L); } }
    S.E.levels = out; S.lvNote = S.lv.note;
    std::vector<double> ranges;
    for (size_t i = S.E.bars.size(); i-- > 0 && ranges.size() < 20;) ranges.push_back((double)(S.E.bars[i].h - S.E.bars[i].l) * S.tick);
    const double atr = nl::atrOf(ranges, S.spb);
    S.E.bandTicks = 2LL * nl::halfWidth(nl::zoneFor(S.market), atr, S.tick);
}
// closed candles (a later bar exists), strictly in order, each once, with their volume at price
void TrappedTraders::bars(TtState& S) {
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYP vap(barVolumeProfile);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const bool haveVap = (long)vap.count >= n;
    S.vapCount = (long)vap.count; S.barCount = n;
    if (S.startMs < 0) S.startMs = ttMs();
    const int lastClosed = (int)n - 2;
    if ((long long)times[lastClosed] <= S.lastFedT) return;
    const int MAXB = (int)S.E.maxBars;
    int from = lastClosed; while (from > 0 && (long long)times[from - 1] > S.lastFedT && lastClosed - from < MAXB) --from;
    // (0.1.1) lsDeltaProfile's candle trap lots (C, decided), keyed by the candle's Central seconds (its localSec = ctSec here)
    std::map<long long, std::pair<double, double> > cmap; long long cMax = 0, cMin = 0;
    {   std::vector<sb::Fact> cf; S.cProducer = sb::readFacts('C', S.symbol, S.spb, S.chart, ttMs(), cf, true); S.cFacts = cf.size();
        for (size_t k = 0; k < cf.size(); ++k) { cmap[cf[k].bar] = std::make_pair(cf[k].lo, cf[k].hi); if (cf[k].bar > cMax) cMax = cf[k].bar; if (!cMin || cf[k].bar < cMin) cMin = cf[k].bar; } }
    int reads = 0; std::vector<ttr::Cell> cells;
    for (int i = from; i <= lastClosed; ++i) {
        const long long t = (long long)times[i]; if (t <= S.lastFedT) continue;
        ttr::Bar b; b.t = t;
        const long long cs = ctSec(t); if (cs <= 0) { S.lastFedT = t; continue; }
        b.sess = (cs - 1 - 17LL * 3600) / 86400 + 1;
        b.sod = (int)(((cs % 86400) + 86400) % 86400);
        if (!ttr::toTicks(op[i], S.tick, b.o) || !ttr::toTicks(hi[i], S.tick, b.h) || !ttr::toTicks(lo[i], S.tick, b.l) || !ttr::toTicks(cl[i], S.tick, b.c)) { S.lastFedT = t; continue; }
        bool ok = false;
        if (!haveVap) ++S.vapShort;
        if (haveVap && !S.vapFault) {
            if (reads >= VAP_READS_PER_PASS) break;                      // the rest next pass (no heavy work in one call)
            ++reads;
            const int r = ttVap(vap, i, S.tick, cells);
            if (r < 0) S.vapFault = true;
            else if (r == 0) {
                if (S.retryT != t) { S.retryT = t; S.retries = 0; }
                if (++S.retries < VAP_RETRY_PASSES) break;                 // not loaded yet: retry, then feed without it
                ++S.vapNotLoaded;
            }
            ok = r > 0; if (ok) ++S.vapOk;
        }
        if (!ok) {
            // (0.1.1) no volume at price in this pane: the C fact of the same candle (one cell below the close = sellers, one above = buyers)
            std::map<long long, std::pair<double, double> >::const_iterator c = cmap.find(cs);
            if (c != cmap.end()) {
                cells.clear(); ttr::Cell dn, up; dn.px = b.c - 1; dn.bid = (long long)std::llround(c->second.first); up.px = b.c + 1; up.ask = (long long)std::llround(c->second.second);
                cells.push_back(dn); cells.push_back(up); ok = true; ++S.fromC;
            } else if (!S.cProducer && !haveVap && ttMs() - S.startMs < 15000) {
                ++S.cWaits; break;                                         // load order: lsDeltaProfile may not have posted yet (15 s, then without)
            } else if (S.cProducer && cs > cMax && i >= lastClosed - 2) {
                // the producer is there but has not posted this candle yet (it posts once a second): wait, bounded, then feed without
                if (S.retryT != t) { S.retryT = t; S.retries = 0; }
                if (++S.retries < VAP_RETRY_PASSES) { ++S.cWaits; break; }
            }
        }
        if (!ok) { cells.clear(); ++S.noVap; }
        S.E.feed(b, cells, ok); S.lastFedT = t; ++S.fed;
    }
    (void)cMin;
}
// (0.1.1) the decided Stage 3 triggers on the SignalBoard (kind 'T', layer 1a): bar = the mark candle's Central seconds, lo = hi = the
// extreme's price, side, val = the ratio, code = 1 at a level / 0 elsewhere, decided, "?" (proven = 2: the tests show no edge on their own yet)
void TrappedTraders::postT(TtState& S, long long now) {
    if (now - S.tPostMs < 1000 && now >= S.tPostMs) return;
    S.tPostMs = now;
    std::vector<sb::Fact> v; const size_t from = S.E.trig.size() > sb::FACT_CAP ? S.E.trig.size() - sb::FACT_CAP : 0;
    for (size_t i = from; i < S.E.trig.size(); ++i) {
        const ttr::Trigger& T = S.E.trig[i]; sb::Fact f; f.bar = ctSec(T.t); if (f.bar <= 0) continue;
        f.lo = f.hi = (double)T.ext * S.tick; f.side = T.side; f.val = T.ratio; f.code = T.atLevel ? 1 : 0; f.state = 1; f.proven = 2; v.push_back(f);
    }
    sb::postFacts('T', S.symbol, S.spb, S.chart, now, v); S.tPosted = v.size();
}
void TrappedTraders::flush(TtState& S) {
    if (S.E.newLines.empty()) return;
    const std::string f = ttFlex();
    if (!f.empty()) { ttMkdir(f); ttMkdir(f + TTR_SEP "TrappedTraders"); }
    for (size_t i = 0; i < S.E.newLines.size(); ++i) {
        const std::string& line = S.E.newLines[i];
        const std::vector<std::string> p = ttr::split(line, '|');
        long long sess = 0; const size_t at = p.size() > 0 && p[0] == "TTT" ? 3 : 2;
        if (p.size() <= at || !ttr::fullLL(p[at], sess) || sess <= 0) { ++S.recFails; continue; }
        const std::string path = recPath(S, sess); if (path.empty()) { ++S.recFails; continue; }
        const bool fresh = !ttExists(path);
        std::ofstream o(path.c_str(), std::ios::binary | std::ios::app);
        if (!o) { ++S.recFails; continue; }
        if (fresh) o << ttr::REC_HEAD << "\n";
        o << line << "\n"; o.flush();
        if (!o) { ++S.recFails; continue; }
        ++S.recLines;
    }
    S.E.newLines.clear();
}
// the Market Awareness board: DATA facts of the last closed candle (only while the perf guard allows the smart work)
void TrappedTraders::post(TtState& S, long long now) {
    if (S.E.bars.empty()) return;
    const ttr::Bar& b = S.E.bars.back();
    if (b.t == S.postedT && now - S.postMs < 15000 && now >= S.postMs) return;
    if (!lsperf::smartAllowed(LSPERF_PLUGIN, S.symbol)) return;
    std::map<std::string, std::string> kv;
    kv["D.g"] = b.ok ? std::to_string(b.g) : "?"; kv["D.r"] = b.ok ? std::to_string(b.r) : "?";
    kv["D.norm"] = b.norm > 0 ? std::to_string(b.norm) : "?";
    const ttr::Trigger* T = S.E.triggerAt(b.t);
    kv["D.trig"] = T ? (T->side > 0 ? "bull" : "bear") : "none";
    if (T) { kv["D.trig_ratio"] = mst::num(T->ratio, 3); kv["D.trig_level"] = T->atLevel ? mst::clean(T->level, 40) : "-"; }
    mst::publish("TRAPTR", S.symbol, S.spb, S.chart, now, ctSec(b.t), kv);
    S.postedT = b.t; S.postMs = now;
}
void TrappedTraders::status(TtState& S, long long now) {
    const std::string d = ttFlex(); if (d.empty() || S.market.empty()) return;
    std::ostringstream o;
    o << "VERSION," << TTR_VERSION << "\nMARKET," << S.market << ",SYMBOL," << S.symbol << ",SECONDS_PER_BAR," << S.spb << "\n";
    o << "STATUS,lsTrappedTraders v" << TTR_VERSION << " | candles " << S.fed << " (without volume at price " << S.noVap << ")"
      << (S.vapFault ? " | VAP FAULT - remove and re-add the indicator" : "") << " | extremes " << S.E.candidates << ", triggers " << S.E.decided << "\n";
    o << "VAP,array " << S.vapCount << " of " << S.barCount << " candles" << (S.vapCount < S.barCount ? " (SHORT: this pane gets no volume at price)" : "")
      << ",read ok " << S.vapOk << ",not loaded after retries " << S.vapNotLoaded << ",array short " << S.vapShort << (S.vapFault ? ",FAULT" : "") << "\n";
    o << "FALLBACK,lsDeltaProfile candle trap lots (C) " << (S.cProducer ? "on the board" : "MISSING - add lsDeltaProfile to this chart") << ",facts " << S.cFacts
      << ",candles fed from C " << S.fromC << ",waits " << S.cWaits << "\n";
    o << "FACTS,SignalBoard " << sb::VERSION << ",T posted " << S.tPosted << "\n";
    int today = 0, atLv = 0; std::string list; const long long sess = S.E.bars.empty() ? 0 : S.E.bars.back().sess;
    for (size_t i = 0; i < S.E.trig.size(); ++i) {
        const ttr::Trigger& T = S.E.trig[i]; if (T.sess != sess) continue;
        ++today; if (T.atLevel) ++atLv;
        if (today <= 12) list += hm(T.t) + " " + (T.side > 0 ? "low " : "high ") + ttr::ratioText(T.ratio) + (T.atLevel ? " at " + T.level : "") + "; ";
    }
    o << "TODAY,triggers " << today << " (at levels " << atLv << "): " << list << "\n";
    o << "LEVELS," << S.E.levels.size() << " trade / gamma levels, band " << S.E.bandTicks << " ticks (" << S.lvNote << ")\n";
    o << "RECORD,files read " << S.files << ",lines appended " << S.recLines << ",failed " << S.recFails << ",bad lines " << S.recBad << "\n";
    const int mk = S.set.i(ttrset::K_MARKS);
    o << "SETTINGS," << S.set.why << ",marks " << (mk == ttrset::MARKS_ALL ? "everywhere" : mk == ttrset::MARKS_OFF ? "off" : "levels only")
      << ",ratio " << (S.set.b(ttrset::K_SHOW_RATIO) ? "on" : "off") << ",box " << (S.set.b(ttrset::K_SHOW_BOX) ? "on" : "off") << "\n";
    o << "PERF," << lsperf::statusOf(this) << "\n";
    o << "AWARENESS,board TRAPTR " << (lsperf::smartAllowed(LSPERF_PLUGIN, S.symbol) ? "posting" : "paused (perf)") << "\n";
    const std::string body = o.str();
    if (body == S.statusBody && now - S.statusMs < 30000 && now >= S.statusMs) return;
    if (now - S.statusMs < 1000 && now >= S.statusMs) return;
    S.statusMs = now; S.statusBody = body;
    ttMkdir(d); ttMkdir(d + TTR_SEP "TrappedTraders");
    const std::string p = d + TTR_SEP "TrappedTraders" TTR_SEP "TrappedTraders.status-" + S.market + "-" + std::to_string(S.spb) + ".txt", tmp = p + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return; f << body; f.flush(); if (!f) return; }
    std::remove(p.c_str()); std::rename(tmp.c_str(), p.c_str());
}

void TrappedTraders::pump() {
    TtState* S = slot_.get(this, true);
    if (!S || S->busy) return;
    S->busy = 1;
    struct Unbusy { TtState* s; ~Unbusy() { s->busy = 0; } } unbusy = { S };
    try {
        const long long now = ttMs();
        identify(*S);
        if (S->market.empty() || !(S->tick > 0) || S->spb <= 0) return;
        settings(*S, now);
        loadFiles(*S);
        levels(*S, now);
        bars(*S);
        flush(*S);
        post(*S, now);
        postT(*S, ttMs());
        status(*S, now);
    } catch (...) {}
}

int TrappedTraders::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    TtState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    int hit = -1;
    for (size_t k = 0; k < S->hits.size() && hit < 0; ++k) { const HitBox& h = S->hits[k]; if (e->v.mouse.h >= h.l && e->v.mouse.h <= h.r && e->v.mouse.v >= h.t && e->v.mouse.v <= h.b) hit = (int)h.idx; }
    if (hit != S->hover) { S->hover = hit; invalidateChart(false); }
    return RTX_FAIL;
}

static short ttC(int v) { return (short)(v < -32768 ? -32768 : v > 32767 ? 32767 : v); }

void TrappedTraders::hoverBox(TtState& S, const RCT& pane) {
    if (S.hover < 0 || (size_t)S.hover >= S.E.bars.size()) return;
    const HitBox* at = nullptr; for (size_t k = 0; k < S.hits.size(); ++k) if ((int)S.hits[k].idx == S.hover) at = &S.hits[k];
    if (!at) return;
    const ttr::Bar& b = S.E.bars[(size_t)S.hover];
    const ttr::Trigger* T = S.E.triggerAt(b.t);
    const std::vector<std::string> rows = ttr::hoverRows(b, T, hm(b.t));
    FONT hf; hf.id = HELVETICA; hf.size = HOVER_PT; hf.style = PLAIN; setFont(hf);
    int w = 0; for (size_t k = 0; k < rows.size(); ++k) w = std::max(w, (int)getTextWidth(rows[k].c_str(), -1));
    w += 12; const int h = (int)rows.size() * HOVER_LH + 8;
    int x0 = at->r + 8, y0 = pane.top + 4;
    if (x0 + w > pane.right - 2) x0 = at->l - 8 - w;
    if (x0 < pane.left + 2) x0 = pane.left + 2;
    if (y0 + h > pane.bottom - 2) y0 = std::max((int)pane.top + 2, (int)pane.bottom - 2 - h);
    const COLOR edge = (COLOR)0xF5C518ul, bg = getBackgroundColor();
    RCT box; box.set(ttC(x0), ttC(y0), ttC(x0 + w), ttC(y0 + h)); box.draw(1, edge, bg, DRAW_OPAQUE, PAT_SOLID);
    for (size_t k = 0; k < rows.size(); ++k) {
        setTextColor(k == 0 ? edge : (COLOR)0xE5E7EBul);
        RCT r; r.set(ttC(x0 + 6), ttC(y0 + 4 + (int)k * HOVER_LH), ttC(x0 + w - 4), ttC(y0 + 4 + (int)(k + 1) * HOVER_LH)); r.drawText(rows[k].c_str(), false, false);
    }
}

// the pane's value range: 0 .. the tallest bar in view x 1.35 (room above for the triangle + ratio)
double TrappedTraders::topFor(TtState& S, int i0, int i1) {
    const long n = getBarCount(); double mx = 0;
    RTARRAYI times(barDateTime);
    if (n > 0 && (long)times.count >= n && !S.E.bars.empty()) {
        const long long t0 = (long long)times[std::max(0, std::min((int)n - 1, i0))], t1 = (long long)times[std::max(0, std::min((int)n - 1, i1))];
        for (size_t k = 0; k < S.E.bars.size(); ++k) { const ttr::Bar& b = S.E.bars[k]; if (b.t < t0 || b.t > t1 || !b.ok) continue; mx = std::max(mx, (double)std::max(b.g, b.r)); }
    }
    return std::max(10.0, mx * 1.35);
}
int TrappedTraders::scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
    double top = 10.0;
    TtState* S = slot_.get(this, false);
    if (S) { top = topFor(*S, iStartBar, iEndBar); S->top = top; }
    if (dMin) *dMin = 0.0;
    if (dMax) *dMax = top;
    return RTX_OK;
}
void TrappedTraders::renderPane(TtState& S) {
    S.hits.clear();
    RCT pane; pane.getPaneRect(false);
    S.paneH = pane.bottom - pane.top;
    if (S.paneH < 60 || pane.right - pane.left < 120) return;            // too small: draw nothing rather than overlap
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); if ((long)times.count < n) return;
    int v0 = 0, v1 = (int)n - 1; if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0) { v0 = std::max(0, (int)n - 200); v1 = (int)n - 1; }
    v1 = std::min(v1, (int)n - 1);
    const double top = topFor(S, v0, v1); S.top = top;
    FONT tf; tf.id = HELVETICA; tf.size = 11; tf.style = BOLD; setFont(tf);
    const std::string ttl = "Who is trapped?  v" TTR_VERSION;
    const ttr::Layout Lay = ttr::layout(pane.left, pane.top, pane.bottom, (int)getTextWidth(ttl.c_str(), -1), S.set.b(ttrset::K_SHOW_BOX) && !S.E.bars.empty());
    const int y0 = Lay.y0, base = Lay.base;
    if (base - y0 < 20) return;                                          // no room under the header: draw nothing rather than overlap
    auto yOf = [&](double v) { return (int)std::lround((double)base - v / top * (double)(base - y0)); };
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f); return (int)p.h; };
    std::map<long long, int> idxOf; for (int i = std::max(0, v0 - 1); i <= v1; ++i) idxOf[(long long)times[i]] = i;
    const int spacing = v1 > v0 ? std::max(1, (xOf(v1) - xOf(v0)) / std::max(1, v1 - v0)) : 6;
    const int bw = std::max(1, spacing * 4 / 10);
    const COLOR cBull = (COLOR)S.set.c(ttrset::K_COL_BULL), cBear = (COLOR)S.set.c(ttrset::K_COL_BEAR);
    ttr::Placer place; place.placed.push_back(Lay.title);
    // the title (his pane-question style, version next to it) and the live box, top-left, above the bar area
    { setTextColor((COLOR)0xF5C518ul); RCT r; r.set(ttC(pane.left + 4), ttC(pane.top + 3), ttC(Lay.title.r - 2), ttC(pane.top + 18)); r.drawText(ttl.c_str(), false, false); }
    if (Lay.box) {
        const ttr::Bar& L = S.E.bars.back();
        FONT bf; bf.id = HELVETICA; bf.size = 10; bf.style = BOLD; setFont(bf);
        const int bx = Lay.live.l, by = Lay.live.t, bwid = ttr::BOX_W, bh = ttr::BOX_H;
        RCT box; box.set(ttC(bx), ttC(by), ttC(bx + bwid), ttC(by + bh)); box.draw(1, (COLOR)0xFFFFFFul, getBackgroundColor(), DRAW_OPAQUE, PAT_SOLID);
        auto txt = [&](int x, int y, int w, const std::string& s, COLOR c) { setTextColor(c); RCT r; r.set(ttC(x), ttC(y), ttC(x + w), ttC(y + 14)); r.drawText(s.c_str(), true, false); };
        txt(bx + 110, by + 3, 60, "Sellers", cBull); txt(bx + 168, by + 3, 60, "Buyers", cBear);
        txt(bx + 4, by + 21, 100, "Lots trapped", (COLOR)0xFFFFFFul);
        txt(bx + 110, by + 21, 60, L.ok ? ttr::commas(L.g) : "n/a", cBull); txt(bx + 168, by + 21, 60, L.ok ? ttr::commas(L.r) : "n/a", cBear);
        txt(bx + 4, by + 39, 100, "vs normal", (COLOR)0xFFFFFFul);
        char a[24], c[24];
        if (L.ok && L.norm > 0) { std::snprintf(a, sizeof a, "%.1fx", (double)L.g / std::max(1.0, L.norm / 2.0)); std::snprintf(c, sizeof c, "%.1fx", (double)L.r / std::max(1.0, L.norm / 2.0)); }
        else { std::snprintf(a, sizeof a, "?"); std::snprintf(c, sizeof c, "?"); }
        txt(bx + 110, by + 39, 60, a, cBull); txt(bx + 168, by + 39, 60, c, cBear);
        place.placed.push_back(Lay.live);
    }
    // the bars (green left, red right, both up from the baseline), fill only, no outline
    FONT mf; mf.id = HELVETICA; mf.size = 10; mf.style = BOLD;
    for (size_t k = 0; k < S.E.bars.size(); ++k) {
        const ttr::Bar& b = S.E.bars[k]; if (!b.ok) continue;
        std::map<long long, int>::const_iterator it = idxOf.find(b.t); if (it == idxOf.end()) continue;
        const int x = xOf(it->second);
        const int yg = yOf((double)b.g), yr = yOf((double)b.r);
        RCT g; g.set(ttC(x - bw - 1), ttC(std::min(yg, base - 1)), ttC(x - 1), ttC(base)); g.draw(0, cBull, cBull, DRAW_OPAQUE, PAT_SOLID);
        RCT r; r.set(ttC(x + 1), ttC(std::min(yr, base - 1)), ttC(x + bw + 1), ttC(base)); r.draw(0, cBear, cBear, DRAW_OPAQUE, PAT_SOLID);
        HitBox hb; hb.l = x - bw - 2; hb.r = x + bw + 2; hb.t = std::min(yg, yr) - 2; hb.b = base; hb.idx = k; S.hits.push_back(hb);
    }
    // the Stage 3 trigger marks (decided once, recorded) in time order; a label never moves an older one
    if (S.set.i(ttrset::K_MARKS) != ttrset::MARKS_OFF) {
        setFont(mf);
        for (size_t q = 0; q < S.E.trig.size(); ++q) {
            if (place.placed.size() >= ttr::MAX_PLACED) break;
            const ttr::Trigger& T = S.E.trig[q]; if (!showMark(S, T)) continue;
            std::map<long long, int>::const_iterator it = idxOf.find(T.t); if (it == idxOf.end()) continue;
            const ttr::Bar* B = nullptr; for (size_t k = S.E.bars.size(); k-- > 0;) if (S.E.bars[k].t == T.t) { B = &S.E.bars[k]; break; }
            if (!B) continue;
            const int x = xOf(it->second), xc = T.side > 0 ? x - bw / 2 - 1 : x + bw / 2 + 1;
            const int yb = yOf((double)(T.side > 0 ? B->g : B->r));
            const COLOR col = T.side > 0 ? cBull : cBear;
            const int ty = yb - 9;                                              // triangle above the winning bar
            if (ty - 2 <= pane.top + 1) continue;
            ttr::Box tri; tri.l = xc - 5; tri.r = xc + 5; tri.t = ty - 1; tri.b = ty + 7;
            std::string lab = S.set.b(ttrset::K_SHOW_RATIO) ? ttr::ratioText(T.ratio) : std::string();
            const int lw = lab.empty() ? 0 : (int)getTextWidth(lab.c_str(), -1) + 4;
            ttr::Box lb; lb.l = xc - lw / 2; lb.r = lb.l + lw; lb.t = ty - 15; lb.b = ty - 2;
            if (lw > 0 && lb.t <= pane.top + 1) continue;
            if (!place.tryPlace(tri, lw > 0 ? &lb : nullptr)) continue;
            setPen(col, 1, P_SOLID);
            for (int row = 0; row < 7; ++row) {                                 // a small filled triangle pointing at the bar
                const int half = 5 - row * 5 / 7;
                PNT a; a.set(0, 0.0f); a.h = ttC(xc - half); a.v = ttC(ty + row); a.setDrawPosition();
                PNT e2; e2.set(0, 0.0f); e2.h = ttC(xc + half); e2.v = ttC(ty + row); e2.drawLineTo();
            }
            if (lw > 0) { setTextColor(col); RCT r; r.set(ttC(lb.l), ttC(lb.t), ttC(lb.r), ttC(lb.b)); r.drawText(lab.c_str(), true, false); }
        }
    }
    hoverBox(S, pane);
}

int TrappedTraders::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    pump();
    TtState* S = slot_.get(this, false);
    if (S) { try { renderPane(*S); } catch (...) {} }
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    TrappedTraders* p = new TrappedTraders();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE | VAP_REQUIRED);     // its own pane, draws itself; mouse events even when not selected
    p->setDescription("LRA Trapped Traders v0.1.1 - Who is trapped? Per candle, green = sell lots that traded BELOW the close (sellers trapped), red = buy lots that traded ABOVE the close (buyers trapped), from the chart's volume at price. Green > red = bullish, red > green = bearish. A triangle with the ratio marks the Stage 3 trigger: at a forming pivot (1-bar ZigZag, the extreme candle through 3 after it) the first candle where the turn side's trapped lots move ahead. Default: marks at every forming pivot; the Levels only choice keeps the trade levels (ONH/ONL, PDH/PDL, PFH/PFL, LonHI/LonLO) and gamma levels, approached from the right side. Without volume at price in its pane it reads the trap lots lsDeltaProfile posts for each candle. Decided once on closed candles, recorded, never repainted. Read it with the Defense pane: take the turn when Defense agrees and the candle closes in the turn colour. Example: ES Oct 26 9:12 AM low, sellers trapped 8,333 vs buyers 3,887 = 2.1x.");
    p->setVersion("0.1.1");   // TTR_VERSION
    return p;
}
