/********************************************************************************
 *  LiquidityProfile.cpp  --  Investor/RT RTX extension  lsLiquidityProfile  (0.1.0, 2026-10-11)
 *
 *  THE LIQUIDITY PROFILE (Rassul 2026-10-10: "get the liquidity profile done and the signals that support absorption ... so i can
 *  trade monday"; the agreed model: Liquidity_profile_model_v2 / Liquidity_profile_v01_mockup).
 *  One column next to the Delta Profile, bars POINTING TOWARD PRICE (anchored on the right, growing left): green = buy orders
 *  below price, red = sell orders above. The top 3 zones per side are bright, with the LOTS inside the bar and "x normal" at
 *  the tip (normal = that market's usual band size for that 30-min slot, from the nightly study; LRA-LiqNorms-<MKT>.csv).
 *  SOLID = rested 30 s+ (durable), HOLLOW = new / flickering. Each zone is also outlined on the price area up to the last bar.
 *  While price is in a zone the live read sits next to it: "eaten 0.4x  stayed 82%  E".
 *  Marks (decided on CLOSED bars, recorded, never repainted): "L? E" / "L? A" / "L? E A" = the zone held (close back out 3 min+
 *  after arrival) with exhaustion (E) and / or a Delta Profile absorption in the zone (A); "LX" = eaten 1x+ then a close through.
 *  The header says TESTING until the all-market study passes (checklist #21).
 *  Data: the chart's own depth (MARKET_DEPTH, one object per chart kept alive - MboCheck's lesson) and the chart's own VAP
 *  (trades by side per price). Native, no outside dependency at run time. Order-by-order events are not used here (their
 *  Rithmic / CQG meaning is unverified - fail closed); lsMboCheck records them for the check.
 *  Files (lsFlexLevels\LiquidityProfile\): <MKT>-<spb>-signals-<session>.csv (marks), <MKT>-<spb>-attempts-<session>.csv (every
 *  zone touch with its numbers, for the nightly scoring), and lsFlexLevels\LiquidityProfile.status-<MKT>-<spb>.txt.
 *  Logic without the SDK: LiquidityProfileLogic.h (tested: test_liquidityprofile_logic.cpp). Per-chart state: HostSlot.h.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(DELTA_WINDOWS_SDK_CHECK) && defined(__GNUC__)
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#else
#include "irtsdk.h"
#endif
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "LiquidityProfileLogic.h"
#include "HostSlot.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <locale>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define LQ_VERSION "0.1.0"          // keep equal to lqp::VERSION and the setVersion literal below

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;          // green bullish / support, red bearish / resistance
static const COLOR C_BUY_DIM = 0x00166534, C_SELL_DIM = 0x007F1D1D;  // the same two colours, dimmed, for the plain book rows
static const COLOR C_GREY = 0x009CA3AF, C_AXIS = 0x00334155, C_INK = 0x000B0F17;
static const int WIDTH_INDEX = 0, FONT_INDEX = 1;
static const long long DEPTH_EVERY_MS = 250, ZONES_EVERY_MS = 500, STATUS_EVERY_MS = 30000;
static const int DEPTH_READ_MAX = 200, VAP_ROWS_MAX = 5000;

static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static bool statFile(const std::string& p, long long* stamp = nullptr) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    if (stamp) *stamp = (long long)st.st_mtime * 1000000LL + (long long)st.st_size;
    return (st.st_mode & S_IFMT) == S_IFREG;
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// checklist #5: tmp, old to .bak, rename into place, .bak back on failure
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = statFile(path);
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    return true;
}
// checklist #6: append, flush, and report success only when the stream is still good
static bool appendLines(const std::string& path, const std::string& header, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    const bool fresh = !statFile(path);
    std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); if (!f) return false;
    if (fresh) f << header << "\n";
    for (size_t i = 0; i < lines.size(); ++i) f << lines[i] << "\n";
    f.flush(); return (bool)f;
}
static bool parseLL(const std::string& t, long long lo, long long hi, long long& out) {   // checklist #4
    if (t.empty() || t.size() > 24) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
static bool parseD(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || e != t.c_str() + t.size() || !lqp::finite(v)) return false;
    out = v; return true;
}
static std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == c) { f.push_back(s.substr(a, i - a)); a = i + 1; }
    return f;
}
static std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return r.substr(0, 8);
}
// the study's MEDIUM zone rule per market (lra.mbo_fetch.ZONES) - the norms file overrides it
static void zoneRule(const std::string& m, lqp::Params& p) {
    if (m == "ES") { p.k = 0.30; p.hwMin = 4; p.hwMax = 6; }
    else if (m == "NQ") { p.k = 0.11; p.hwMin = 8; p.hwMax = 12; }
    else if (m == "GC") { p.k = 0.25; p.hwMin = 8; p.hwMax = 15; }
    else if (m == "HG") { p.k = 0.30; p.hwMin = 2; p.hwMax = 5; }
    else if (m == "NG") { p.k = 0.30; p.hwMin = 2; p.hwMax = 4; }
    else if (m == "EU") { p.k = 0.40; p.hwMin = 2; p.hwMax = 3; }
    else { p.k = 0.25; p.hwMin = 4; p.hwMax = 8; }                 // CL and anything else
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}

struct LQState {
    lqp::Engine e;
    cppExtension::MARKET_DEPTH* md = nullptr;
    bool mdDead = false, mdBlocked = false, busy = false, faulted = false, primed = false;
    std::string identity, root, market, symbol; int spb = 0; double tick = 0;
    // VAP of the bar being read: tick -> (buy, sell) already counted
    int vapBar = -1; RTDATE vapStamp = 0; std::map<lqp::Tick, std::pair<long long, long long>> vapRows;
    long barCount = 0;
    long long ctOffset = 0; bool haveOffset = false;
    long long lastDepth = -1, lastZones = -1, lastStatus = -1, lastNorms = -1, lastLayout = -1;
    bool depthAvail = false; int depthMax = 0, depthLevels = 0; long long depthReads = 0;
    long long session = -1; size_t marksWritten = 0; long long recWrites = 0, recFails = 0;
    long long normsStamp = -2, layoutStamp = -2, dealerStamp = -2;
    int dpWidth = 50, dpGap = 0, dealerReach = 0, width = 70, font = 9;
    std::vector<std::pair<lqp::Tick, lqp::Tick>> dpBid, dpAsk; long long dpStamp = -2;
    double atr = 0; int slot = -1;
    std::string state = "starting", lastStatusText;
    ~LQState() { delete md; }
};

class LiquidityProfile : public cppExtension {
public:
    LiquidityProfile() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsApply(void) { LQState* S = slot_.get(this, false); if (S) { S->width = clampWidth(getIntegerValue(WIDTH_INDEX)); S->font = clampFont(getIntegerValue(FONT_INDEX)); } return RTX_OK; }
    void pump();
    void release() { slot_.release(this); }
    static int clampWidth(int w) { return w < 40 ? 70 : std::min(w, 300); }
    static int clampFont(int f) { return f < 6 ? 9 : std::min(f, 16); }
private:
    HostSlot<LQState> slot_;
    long long ctSec(RTDATE d);
    long long nowMs(LQState& S);
    void identify(LQState& S);
    void openDepth(LQState& S);
    bool readDepth(LQState& S, long long t);
    void readTrades(LQState& S, long long t, long count);
    void closedBars(LQState& S, long count);
    void loadNorms(LQState& S);
    void loadLayout(LQState& S);
    void loadDelta(LQState& S);
    void session(LQState& S, long long t);
    void persist(LQState& S);
    void status(LQState& S, long long now);
    void render(LQState& S);
    int barOfTime(long long barTime, long count);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\LiquidityProfile"; }
    std::string stem(LQState& S) { return S.market + "-" + std::to_string(S.spb); }
};

// the SDK leaves these five undefined: every RTX plugin supplies them (else LNK2019)
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<LiquidityProfile*>(this)->release(); return RTX_OK; }   // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<LiquidityProfile*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    setParameterVersion(1);
    setParameterDialogHeight(1);
    setIntegerParameter("Width px", 70, 50);
    setIntegerParameter("Font pt", 9, 50, kParmAppendSameLine);
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<LiquidityProfile*>(this)->pump(); return RTX_OK; }
int LiquidityProfile::draw(void) {
    pump();
    LQState* S = slot_.get(this, false);
    if (S && !S->faulted) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}

// a bar / clock stamp as the Central wall clock in epoch seconds (TapeFlow / Delta Profile convention); 0 = unreadable
long long LiquidityProfile::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
// engine time: the Central clock in ms, smoothed with the steady clock (the SDK clock has whole seconds only)
long long LiquidityProfile::nowMs(LQState& S) {
    const long long st = steadyMs(), ct = ctSec(currentDate()) * 1000LL;
    if (ct <= 0) return S.haveOffset ? st + S.ctOffset : st;
    if (!S.haveOffset || std::llabs((st + S.ctOffset) - ct) > 2000) { S.ctOffset = ct - st; S.haveOffset = true; }
    return st + S.ctOffset;
}

void LiquidityProfile::identify(LQState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR);                       // checklist #24: the chart's own tick
    if (!lqp::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = sym + "|" + std::to_string(spb);
    if (id != S.identity || tick != S.tick) {
        const int w = S.width, f = S.font;
        LQState fresh; S.e = fresh.e; delete S.md; S.md = nullptr; S.mdDead = S.mdBlocked = false; S.barCount = 0; S.vapRows.clear(); S.vapBar = -1; S.primed = false; S.session = -1; S.marksWritten = 0;
        S.normsStamp = S.layoutStamp = S.dealerStamp = S.dpStamp = -2; S.width = w; S.font = f;
        S.identity = id; S.root = root; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
        S.e.P.tick = tick > 0 ? tick : 0.01; zoneRule(S.market, S.e.P);
    }
}

// one depth object per chart, kept alive (its destructor unsubscribes); opened behind a crash guard like lsMboCheck
void LiquidityProfile::openDepth(LQState& S) {
    if (S.md || S.mdDead || S.mdBlocked) return;
    const std::string d = dir();
    const std::string guard = d.empty() ? std::string() : d + "\\_opening-" + stem(S) + ".txt";
    if (!guard.empty() && statFile(guard)) { S.mdBlocked = true; S.state = "depth open crashed IRT before: delete " + guard + " to retry"; return; }
    if (!guard.empty()) { makeDir(flexDir()); makeDir(d); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening MARKET_DEPTH\n"; }
    try { S.md = new (std::nothrow) MARKET_DEPTH(NULL); } catch (...) { S.mdDead = true; }
    if (!guard.empty()) std::remove(guard.c_str());
}

bool LiquidityProfile::readDepth(LQState& S, long long t) {
    if (!S.md || S.mdDead) return false;
    std::vector<std::pair<double, double>> bids, asks;
    try {
        S.depthAvail = S.md->available() != 0;
        S.depthMax = S.md->maxLevels;
        if (!S.depthAvail) return false;
        int n = S.depthMax; if (n > DEPTH_READ_MAX) n = DEPTH_READ_MAX; if (n < 0) n = 0;
        S.depthLevels = 0;
        for (int k = 0; k < n; ++k) {
            const DEPTH_LEVEL L = (*S.md)[(unsigned)k];                   // a copy: the SDK hands out a pointer into IRT's book
            const bool b = lqp::finite(L.bid) && L.bid > 0 && L.bidsize > 0, a = lqp::finite(L.ask) && L.ask > 0 && L.asksize > 0;
            if (b) bids.push_back(std::make_pair((double)L.bid, (double)L.bidsize));
            if (a) asks.push_back(std::make_pair((double)L.ask, (double)L.asksize));
            if (b || a) S.depthLevels++;
        }
    } catch (...) { S.mdDead = true; S.state = "depth read fault"; return false; }
    if (bids.empty() || asks.empty()) return false;
    S.depthReads++;
    return S.e.onBook(t, bids, asks);
}

// trades = the increase of the forming bar's VAP since the last read (buy = lifted the offer, sell = hit the bid)
static int vapStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || !st || bar < 0) return -2;
        std::memset(st, 0, sizeof(*st));
        if (vap->getBarStatistics(bar, *st) != RTX_OK) return -2;
        return st->prices < 0 ? -2 : st->prices;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static int vapRowsRead(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;                                                    // checklist #22: any row failure rejects the whole read
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
void LiquidityProfile::readTrades(LQState& S, long long t, long count) {
    if (count < 1 || S.tick <= 0) return;
    RTARRAYP vap(barVolumeProfile);
    if (vap.count < count) return;
    const int bars[2] = { S.vapBar >= 0 && S.vapBar < (int)count - 1 ? S.vapBar : -1, (int)count - 1 };   // the bar just closed gets its last read
    for (int k = 0; k < 2; ++k) {
        const int bar = bars[k]; if (bar < 0) continue;
        BARSTATISTICS st; const int n = vapStats(&vap, bar, &st);
        if (n == -1) { S.faulted = true; S.state = "VAP fault: reload the indicator to retry"; return; }
        if (n < 0 || n > VAP_ROWS_MAX) continue;
        std::vector<VOLPROFILE> rows((size_t)n);
        const int r = vapRowsRead(&vap, bar, rows.empty() ? nullptr : &rows[0], n);
        if (r == -1) { S.faulted = true; S.state = "VAP fault: reload the indicator to retry"; return; }
        if (r < 0) continue;
        if (bar != S.vapBar) { S.vapRows.clear(); S.vapBar = bar; }
        const bool prime = !S.primed;                                    // what traded before we loaded is history, not new trades
        for (size_t i = 0; i < rows.size(); ++i) {
            const VOLPROFILE& v = rows[i];
            if (!lqp::finite(v.price) || v.price <= 0 || v.buyVolume < 0 || v.sellVolume < 0) continue;
            const lqp::Tick px = lqp::toTick(v.price, S.tick);
            std::pair<long long, long long>& seen = S.vapRows[px];
            const long long db = (long long)v.buyVolume - seen.first, ds = (long long)v.sellVolume - seen.second;
            seen.first = (long long)v.buyVolume; seen.second = (long long)v.sellVolume;
            if (!prime && (db > 0 || ds > 0)) S.e.onTrade(t, (double)px * S.tick, (double)std::max(0LL, db), (double)std::max(0LL, ds));
        }
        S.primed = true;
    }
    RTARRAY close(barClose);
    if (close.count >= count) S.e.onLastPrice(close[(int)count - 1]);
}

// a bar is closed once a later bar exists (checklist #27); each closed bar is decided once, in order
void LiquidityProfile::closedBars(LQState& S, long count) {
    if (S.barCount == 0) { S.barCount = count; return; }                // history before load: never decided (no attempts on it)
    if (count <= S.barCount) { if (count < S.barCount) S.barCount = count; return; }
    RTARRAYI times(barDateTime); RTARRAY close(barClose), high(barHigh), low(barLow);
    if (times.count < count || close.count < count) return;
    loadDelta(S);
    // ATR of the chart's closed bars (20), scaled to a 3-min bar when the chart is not 3-min
    double sum = 0; int m = 0;
    for (int i = (int)count - 2; i >= 0 && m < 20; --i, ++m) sum += (double)high[i] - (double)low[i];
    if (m > 0) { S.atr = sum / m; if (S.spb > 0 && S.spb != 180) S.atr *= std::sqrt(180.0 / (double)S.spb); }
    for (long i = S.barCount - 1; i < count - 1; ++i) {
        const long long bt = (long long)times[(int)i];
        const long long endMs = ctSec((RTDATE)times[(int)i]) * 1000LL;   // IRT stamps the bar END (checklist #12)
        if (endMs > 0) S.e.onBarClose((int)i, bt, endMs, (double)close[(int)i], S.dpBid, S.dpAsk);
    }
    S.barCount = count;
    persist(S);
}

void LiquidityProfile::loadNorms(LQState& S) {
    const std::string f = flexDir(); if (f.empty() || S.market.empty()) return;
    const std::string path = f + "\\LRA-LiqNorms-" + S.market + ".csv";
    long long stamp = -1; if (!statFile(path, &stamp)) { S.normsStamp = -1; return; }
    if (stamp == S.normsStamp) return;
    S.normsStamp = stamp;
    std::ifstream in(path.c_str()); std::string line; int lines = 0;
    std::map<int, double> med; lqp::Params p = S.e.P;
    while (std::getline(in, line) && ++lines < 2000) {                  // checklist #33: bounded input
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        long long a = 0, b = 0; double x = 0;
        if (c.size() >= 4 && c[0] == "ZONE" && parseD(c[1], x) && parseLL(c[2], 1, 200, a) && parseLL(c[3], 1, 200, b) && x > 0 && a <= b) { p.k = x; p.hwMin = (int)a; p.hwMax = (int)b; }
        else if (c.size() >= 3 && c[0] == "SLOT" && parseLL(c[1], 0, 47, a) && parseD(c[2], x) && x > 0) med[(int)a] = x;
    }
    S.e.norms.median.swap(med); S.e.norms.loaded = true; S.e.P.k = p.k; S.e.P.hwMin = p.hwMin; S.e.P.hwMax = p.hwMax;
}

// where the column goes: just left of the Delta Profile (its width / gap for this root) and the Dealer Profile (its reach)
void LiquidityProfile::loadLayout(LQState& S) {
    const std::string f = flexDir(); if (f.empty()) return;
    long long st = -1;
    if (statFile(f + "\\DeltaProfile.settings.txt", &st) && st != S.layoutStamp) {
        S.layoutStamp = st;
        std::ifstream in((f + "\\DeltaProfile.settings.txt").c_str()); std::string line; int n = 0;
        while (std::getline(in, line) && ++n < 200) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, '|'); long long w = 0, g = 0;
            if (c.size() >= 4 && c[0] == S.root && parseLL(c[2], 30, 600, w) && parseLL(c[3], -300, 400, g)) { S.dpWidth = (int)w; S.dpGap = (int)g; }
        }
    }
    if (statFile(f + "\\DealerProfile.status.txt", &st) && st != S.dealerStamp) {
        S.dealerStamp = st;
        std::ifstream in((f + "\\DealerProfile.status.txt").c_str()); std::string line, mkt; long long reach = 0; int n = 0;
        while (std::getline(in, line) && ++n < 400) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, ',');
            if (c.size() >= 2 && c[0] == "MARKET") mkt = c[1];
            else if (c.size() >= 2 && c[0] == "REACH") parseLL(c[1], 0, 1199, reach);
        }
        if (mkt == S.market && reach > 0) S.dealerReach = (int)reach;
    }
}

// the Delta Profile's absorption candidates (A? / A) of this session from its record file -> zones for the "A" reason
void LiquidityProfile::loadDelta(LQState& S) {
    const std::string f = flexDir(); if (f.empty() || S.session < 0) return;
    const std::string path = f + "\\DeltaProfile\\" + S.market + "-" + std::to_string(S.spb) + "-signals-" + std::to_string(S.session) + ".csv";
    long long st = -1; if (!statFile(path, &st)) { S.dpBid.clear(); S.dpAsk.clear(); S.dpStamp = -1; return; }
    if (st == S.dpStamp) return;
    S.dpStamp = st; S.dpBid.clear(); S.dpAsk.clear();
    const long long nowCt = ctSec(currentDate());
    std::ifstream in(path.c_str()); std::string line; int n = 0;
    while (std::getline(in, line) && ++n < 20000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, '|');
        if (c.size() < 8 || (c[2] != "A" && c[2] != "A?")) continue;
        long long tf = 0; double lo = 0, hi = 0;
        if (!parseLL(c[0], 0, LLONG_MAX / 2, tf) || !parseD(c[5], lo) || !parseD(c[6], hi) || hi < lo) continue;
        if (nowCt > 0 && tf > 0 && nowCt - tf > 3600) continue;           // the last hour only
        const std::pair<lqp::Tick, lqp::Tick> z(lqp::toTick(lo, S.e.P.tick), lqp::toTick(hi, S.e.P.tick));
        (c[3] == "support" ? S.dpBid : S.dpAsk).push_back(z);
    }
}

// the 17:00 Central session: a new session starts a new record (marks of the old one are kept in their file)
void LiquidityProfile::session(LQState& S, long long t) {
    const long long sec = t / 1000; if (sec <= 0) return;
    const long long key = (sec - 17LL * 3600) / 86400LL + 1;               // 17:00 CT belongs to the next day's session
    if (key == S.session) return;
    S.session = key; S.e.marks.clear(); S.marksWritten = 0;
    const std::string d = dir(); if (d.empty()) return;
    std::ifstream in((d + "\\" + stem(S) + "-signals-" + std::to_string(key) + ".csv").c_str());
    std::string line; int n = 0;
    while (std::getline(in, line) && ++n < 5000) {                        // a restart redraws exactly what was shown (#11)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lqp::Mark m; if (lqp::Engine::parse(line, m, S.e.P.tick)) S.e.marks.push_back(m);
    }
    S.marksWritten = S.e.marks.size();
}

void LiquidityProfile::persist(LQState& S) {
    const std::string d = dir(); if (d.empty() || S.session < 0) return;
    makeDir(flexDir()); makeDir(d);
    if (S.marksWritten < S.e.marks.size()) {
        std::vector<std::string> lines;
        for (size_t i = S.marksWritten; i < S.e.marks.size(); ++i) lines.push_back(lqp::Engine::line(S.e.marks[i], S.e.P.tick));
        if (appendLines(d + "\\" + stem(S) + "-signals-" + std::to_string(S.session) + ".csv", lqp::Engine::header(), lines)) { S.marksWritten = S.e.marks.size(); S.recWrites++; }
        else S.recFails++;                                               // kept and retried at the next bar close (#6)
    }
    if (!S.e.attemptLog.empty()) {
        if (appendLines(d + "\\" + stem(S) + "-attempts-" + std::to_string(S.session) + ".csv", lqp::Engine::attemptHeader(), S.e.attemptLog)) S.e.attemptLog.clear();
        else S.recFails++;
    }
}

void LiquidityProfile::status(LQState& S, long long now) {
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << LQ_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nTICK," << S.tick
      << "\nDEPTH," << (S.mdBlocked ? "blocked" : S.mdDead ? "fault" : !S.md ? "not opened" : S.depthAvail ? "available" : "not available")
      << ",max " << S.depthMax << ",levels " << S.depthLevels << ",reads " << S.depthReads << ",books " << S.e.books << ",bad " << S.e.badBooks << "," << S.e.why
      << "\nTRADES," << S.e.trades << "\nZONE_HW," << S.e.hw << ",atr " << S.atr << ",slot " << S.slot << ",normal " << S.e.normal << ",norms " << (S.e.norms.loaded ? "file" : "session")
      << "\nZONES," << S.e.zones.size() << "\nATTEMPTS," << S.e.attempts.size() << "\nMARKS," << S.e.marks.size() << ",written " << S.marksWritten
      << ",writes " << S.recWrites << ",fails " << S.recFails << "\nDELTA_A," << S.dpBid.size() << "," << S.dpAsk.size()
      << "\nSESSION," << S.session << "\nSTATE," << S.state << "\n";
    for (size_t i = 0; i < S.e.zones.size(); ++i) {
        const lqp::Zone& z = S.e.zones[i];
        o << "ZONE," << (z.side == lqp::BID ? "support" : "resistance") << "," << (double)z.lo * S.e.P.tick << "," << (double)z.hi * S.e.P.tick
          << "," << z.size << ",x " << S.e.xNormal(z.size) << ",age_s " << (now - z.born) / 1000 << "\n";
    }
    const std::string text = o.str();
    if (text == S.lastStatusText && S.lastStatus >= 0 && now - S.lastStatus < STATUS_EVERY_MS) return;   // #31: on change or every 30 s
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + "\\LiquidityProfile.status-" + stem(S) + ".txt", text)) { S.lastStatusText = text; S.lastStatus = now; }
}

void LiquidityProfile::pump() {
    LQState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    LQState& S = *P;
    S.busy = true;
    struct Unbusy { LQState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        if (S.faulted || S.tick <= 0) { if (S.tick <= 0) S.state = "no tick size from IRT"; return; }
        const long long t = nowMs(S), st = steadyMs();
        session(S, t);
        if (S.lastNorms < 0 || st - S.lastNorms >= 60000) { S.lastNorms = st; loadNorms(S); }
        if (S.lastLayout < 0 || st - S.lastLayout >= 1000) { S.lastLayout = st; loadLayout(S); }
        openDepth(S);
        const long count = getBarCount();
        if (S.lastDepth < 0 || t - S.lastDepth >= DEPTH_EVERY_MS || t < S.lastDepth) {
            S.lastDepth = t;
            readTrades(S, t, count);                                    // trades first: they explain the book change
            readDepth(S, t);
        }
        closedBars(S, count);
        if (S.e.haveBook && (S.lastZones < 0 || t - S.lastZones >= ZONES_EVERY_MS || t < S.lastZones)) {
            S.lastZones = t;
            struct tm tt; std::memset(&tt, 0, sizeof(tt));
            if (getLocaltime(currentDate(), &tt)) S.slot = (tt.tm_hour * 60 + tt.tm_min) / 30;
            S.e.updateZones(t, S.atr, S.slot);
        }
        S.state = S.mdBlocked ? S.state : !S.md ? "depth not opened" : !S.depthAvail ? "no depth from this feed" : !S.e.haveBook ? "waiting for depth" : "ready";
        status(S, st);
    } catch (...) {
        S.state = "calculation error";                                  // never let an exception reach IRT
    }
}

int LiquidityProfile::barOfTime(long long barTime, long count) {
    RTARRAYI times(barDateTime);
    if (times.count < count || count < 1) return -1;
    int lo = 0, hi = (int)count - 1;
    while (lo <= hi) { const int mid = (lo + hi) / 2; const long long v = (long long)times[mid]; if (v == barTime) return mid; if (v < barTime) lo = mid + 1; else hi = mid - 1; }
    return -1;
}

static std::string lots(double v) { char b[32]; std::snprintf(b, sizeof(b), v >= 10000 ? "%.0fk" : "%.0f", v >= 10000 ? v / 1000 : v); return b; }
static std::string mult(double x) { char b[32]; if (x >= 10) std::snprintf(b, sizeof(b), "10x+"); else std::snprintf(b, sizeof(b), "%.1fx", x); return b; }

void LiquidityProfile::render(LQState& S) {
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int paneRight = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneRight = scale.left - 2;
    const int base = paneRight - 2 - S.dealerReach - S.dpGap - S.dpWidth - 6;     // just left of the Delta Profile
    const int left = base - S.width;
    const int top = pane.top + 16, bottom = pane.bottom;
    const long count = getBarCount();
    if (left <= pane.left + 40 || bottom - top < 40 || count < 2) return;          // #35: no room, draw nothing
    const int lastBar = (int)count - 1;
    FONT f; f.id = HELVETICA; f.size = coord(S.font); f.style = BOLD; setFont(f);
    // header: what the column is and whether it has data (no other text on the chart)
    {
        const bool ok = S.e.haveBook && S.depthAvail;
        const std::string h = ok ? "LIQ TESTING" : S.mdBlocked ? "LIQ blocked" : "LIQ no depth";
        setTextColor(ok ? C_GREY : C_SELL);
        RCT r; r.set(coord(left), coord(pane.top + 2), coord(base), coord(pane.top + 2 + S.font + 6)); r.drawText(h.c_str(), false, false);
    }
    if (!S.e.haveBook) return;
    const double tick = S.e.P.tick;
    auto yOf = [&](double price) { PNT p; p.set(lastBar, (float)price); return (int)p.v; };
    setPen(C_AXIS, 1, P_SOLID);
    { PNT a; a.set(0, 0.0f); a.h = coord(base); a.v = coord(top); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(base); b.v = coord(bottom); b.drawLineTo(); }
    // plain book rows (dim), scaled to the biggest single level in view
    double maxRow = 0;
    for (int s = 0; s < 2; ++s) for (std::map<lqp::Tick, double>::const_iterator i = S.e.book[s].begin(); i != S.e.book[s].end(); ++i) {
        const int y = yOf((double)i->first * tick); if (y > top && y < bottom) maxRow = std::max(maxRow, i->second);
    }
    double maxZone = 0; for (size_t i = 0; i < S.e.zones.size(); ++i) maxZone = std::max(maxZone, S.e.zones[i].size);
    const int full = S.width - 4;
    if (maxRow > 0) {
        for (int s = 0; s < 2; ++s) for (std::map<lqp::Tick, double>::const_iterator i = S.e.book[s].begin(); i != S.e.book[s].end(); ++i) {
            int yt = yOf(((double)i->first + 0.5) * tick), yb = yOf(((double)i->first - 0.5) * tick);
            if (yb <= yt) yb = yt + 1;
            if (yb <= top || yt >= bottom) continue;
            const int len = std::max(1, (int)(i->second / maxRow * full * 0.5));
            RCT r; r.set(coord(base - len), coord(std::max(top, yt)), coord(base), coord(std::min(bottom, yb - (yb - yt > 3 ? 1 : 0))));
            r.draw(0, s == lqp::BID ? C_BUY_DIM : C_SELL_DIM, s == lqp::BID ? C_BUY_DIM : C_SELL_DIM, DRAW_OPAQUE, PAT_SOLID);
        }
    }
    // the top zones: bright bar (solid = durable, hollow = new), lots inside, x normal at the tip; outline on the price area
    RTARRAYI times(barDateTime);
    const long long nowT = S.haveOffset ? steadyMs() + S.ctOffset : 0;
    const lqp::Readout rd = S.e.readout(nowT);
    for (size_t i = 0; i < S.e.zones.size(); ++i) {
        const lqp::Zone& z = S.e.zones[i];
        int yt = yOf(((double)z.hi + 0.5) * tick), yb = yOf(((double)z.lo - 0.5) * tick);
        if (yb <= yt) yb = yt + 2;
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb);
        const COLOR c = z.side == lqp::BID ? C_BUY : C_SELL;
        const bool solid = z.durable(nowT, S.e.P.persistMs);
        const int len = std::max(8, (int)(maxZone > 0 ? z.size / maxZone * full : full));
        RCT r; r.set(coord(base - len), coord(yt), coord(base), coord(yb));
        if (solid) r.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
        else { setPen(c, 1, P_DASH); CBRUSH br(c, PAT_HOLLOW); br.set(); r.draw(1, c, c, DRAW_OPAQUE, PAT_HOLLOW); }
        const int ym = (yt + yb) / 2;
        FONT g; g.id = HELVETICA; g.size = coord(S.font); g.style = BOLD; setFont(g);
        const std::string lt = lots(z.size); const int lw = getTextWidth(lt.c_str(), -1);
        if (lw + 4 < len && yb - yt >= S.font) { setTextColor(solid ? C_INK : c); RCT tr; tr.set(coord(base - len), coord(ym - S.font), coord(base), coord(ym + S.font)); tr.drawText(lt.c_str(), true, false); }
        const double x = S.e.xNormal(z.size);
        if (x > 0) { const std::string xs = mult(x); const int xw = getTextWidth(xs.c_str(), -1); setTextColor(c);
            RCT xr; xr.set(coord(base - len - xw - 4), coord(ym - S.font), coord(base - len - 2), coord(ym + S.font)); xr.drawText(xs.c_str(), false, true); }
        // outline on the price area, from the bar it formed on to the last bar (never beyond the last bar)
        int from = lastBar;
        if (times.count >= count) {
            const long long bornSec = (z.born / 1000);
            for (int b = lastBar; b >= 0 && b > lastBar - 400; --b) { if (ctSec((RTDATE)times[b]) < bornSec) break; from = b; }
        }
        PNT p0; p0.set(from, 0.0f); PNT p1; p1.set(lastBar, 0.0f);
        const int x0 = p0.h, x1 = std::min((int)p1.h, left - 4);
        if (x1 > x0 + 2) {
            setPen(c, 1, solid ? P_SOLID : P_DASH);
            PNT a; a.set(0, 0.0f); a.h = coord(x0); a.v = coord(yt); a.setDrawPosition();
            PNT b; b.set(0, 0.0f); b.h = coord(x1); b.v = coord(yt); b.drawLineTo(); b.v = coord(yb); b.drawLineTo();
            a.v = coord(yb); a.drawLineTo(); a.v = coord(yt); a.drawLineTo();
        }
        // the live read of the zone price is in (or just left): eaten / stayed / E
        if (rd.active && rd.side == z.side && rd.lo <= z.hi && rd.hi >= z.lo) {
            char b[96]; std::snprintf(b, sizeof(b), "eaten %.1fx  stayed %.0f%%%s", rd.eaten, rd.stayed * 100.0, rd.exhausted ? "  E" : "");
            FONT h; h.id = HELVETICA; h.size = coord(std::max(7, S.font - 1)); h.style = PLAIN; setFont(h); setTextColor(c);
            const int w = getTextWidth(b, -1);
            RCT tr; tr.set(coord(left - w - 6), coord(yb + 1), coord(left - 4), coord(yb + S.font + 4)); tr.drawText(b, false, true);
        }
    }
    // the marks: under the bar for bullish, over it for bearish; one per bar; read from the record (#11)
    if (times.count >= count) {
        RTARRAY high(barHigh), low(barLow);
        for (size_t i = 0; i < S.e.marks.size(); ++i) {
            const lqp::Mark& m = S.e.marks[i];
            const int bar = barOfTime(m.barTime, count); if (bar < 0 || high.count < count) continue;
            const bool bullish = (m.code == "L?" && m.side == lqp::BID) || (m.code == "LX" && m.side == lqp::ASK);
            const std::string text = m.code == "L?" ? m.code + " " + m.reasons : m.code;
            FONT g; g.id = HELVETICA; g.size = coord(S.font); g.style = BOLD; setFont(g); setTextColor(bullish ? C_BUY : C_SELL);
            const int w = getTextWidth(text.c_str(), -1);
            PNT p; p.set(bar, bullish ? (float)low[bar] : (float)high[bar], kBarCenter);
            const int y = bullish ? p.v + 4 : p.v - S.font - 8;
            if (y < top || y + S.font + 4 > bottom || p.h + w / 2 >= left) continue;
            RCT tr; tr.set(coord(p.h - w / 2 - 2), coord(y), coord(p.h + w / 2 + 2), coord(y + S.font + 4)); tr.drawText(text.c_str(), true, false);
        }
    }
}

extern "C" cppExtension* CreateExtension(void) {
    LiquidityProfile* p = new LiquidityProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the book keeps moving while no trade prints (#36)
    p->setDescription("LRA Liquidity Profile (LIQ): resting orders by price pointing toward price, top 3 zones per side with lots and x normal, and the zone reads that support absorption (eaten / stayed / E). TESTING.");
    p->setVersion("0.1.0");   // LQ_VERSION
    return p;
}
