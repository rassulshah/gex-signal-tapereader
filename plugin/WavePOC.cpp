/********************************************************************************
 *  WavePOC.cpp  --  Investor/RT RTX extension  lsWavePOC  "Wave POC"  1.3.1  (2026-10-28)
 *
 *  1.3.2 (18:15-18:19 CT 2026-10-28, Rassul: "you highlighted the wrong node because the node above it has more volume", the outlined node,
 *    the left 11:39 profile without nodes, the thick 11:57 nodes, no liquidity on past waves):
 *    - a leg EXTENDED by an equal / more extreme end pivot replaced its wave only in the zigzag: the old wave (same start) stayed and both
 *      profiles were drawn at one start - the new one's rows covered the old one's highlight except its +-1 px edges = the "outline"
 *      (10:03 = waves 09:57-10:24 and 09:57-10:27; 11:57 = 11:57-12:30 and 11:57-12:33). Now the extended leg REPLACES the old wave.
 *    - the node row = the HIGHEST RAW row of its mini-distribution (the 3-row smoothing only finds the distributions); recorded nodes of an
 *      older row height highlight the highest raw row inside their recorded row.
 *    - one row height everywhere: 1 tick per row (unless a wave spans > 400 ticks; was 80 rows max -> 2-tick rows on 20-pt waves).
 *    - drawing: every profile's rows first, then the highlights / liquidity nodes; a highlight = exactly its own row (no +-1 px band),
 *      filled; each profile clipped at the next profile's start (never overlap).
 *    - (18:32 CT "why is your zig zag different than irts") the pivots = IRT's own ZIGZAG with Min Price Change = N BARS + Use High/Low:
 *      the live leg's extreme moves only on a STRICTLY more extreme bar; a bar beyond the previous N bars' opposite extreme reverses the
 *      leg (the old extreme becomes final, its leg is judged at that bar). 1.3.1 used an N-bar fractal (N bars each side, an equal
 *      extreme later moved the pivot): 10-28 ES 13:36 L instead of IRT's 13:18 L, 10:27 / 12:33 instead of 10:24 / 12:30.
 *      The live leg is the provisional wave until IRT's rule reverses it.
 *    - liquidity: the forming bar's samples are re-keyed to the closed bar's final stamp and a wave bar takes every sample inside it
 *      (WavePOCLiq 1.1.2) - the 10-28 record has no LNODE line at all; recorded LNODEs are redrawn after a restart even when the wave
 *      has no volume node; each wave's rows are recorded (ROWS line) for the nightly and for exact mockups.
 *  1.3.1 (crash fix, 17:52 CT 2026-10-28: IRT died when 1.3.0 was added to his ES 3-min chart): 1.3.0 read the CoB RTL arrays
 *    (getCustomIndicator with recalc - IRT recalculates cob_bull_3m_1b / cob_bear_3m_1b, ~1.1 s each) from feed(), and feed() also ran
 *    from draw() (the first draw after the indicator is added pumps before any calc). lsCobImpulse reads them ONLY from calc - and
 *    getCustomIndicator from the wrong context is what killed IRT on 10-24 (lsChartView, lesson 150). Now: new closed bars (and their CoB
 *    flags) are fed ONLY from calc (pump(true)); draw() never feeds and never touches an RTL; the RTL read sits behind a crash-guard file
 *    (a read that kills IRT is never retried on that chart: COB_SOURCE says so); parmsInit no longer calls getParameters (it saves the
 *    defaults, the lsSessionVWAP 1.8.2 pattern - the SDK only allows getFormerParameters there).
 *
 *  1.3.0 (Rassul approved 17:27 CT 2026-10-28): EVERY CoB LEG gets a Wave POC profile, even when it fails the wave rule (20-bar extreme +
 *    bigger than the direction's average). CoB = his locked rule from the SAME source as the Cob marks: IRT's RTL custom indicators
 *    cob_bull_3m_1b / cob_bear_3m_1b, read natively here exactly as lsCobImpulse reads them (getCustomIndicator); each closed bar's flags
 *    are captured once when the bar is fed. A zigzag leg holding a CoB of its own direction gets the same profile / nodes / liquidity nodes
 *    at its START, decided when its end pivot is confirmed (never earlier, never repainted); a leg that is also a wave is drawn once (BOTH).
 *    Setting "CoB legs" (default on; parameter version 2 - parmsInit carries an older set's Reversal bars / Show zigzag over).
 *    Record lines carry kind WAVE / COB / BOTH (+ the CoB bar time on WAVE lines); status COB_LEGS + COB_SOURCE lines.
 *
 *  Rassul 2026-10-28 (idea "Wave POC" -> mockups v1-v3 -> approved 10:59 CT "just build now"): every BIG wave's own volume profile,
 *  its separate high-volume nodes as pullback targets.
 *    - waves = legs of the N-bar reversal zigzag (N = "Reversal bars", default 3), up AND down; a wave counts when it ends at a 20-bar
 *      high / low and is "bigger than the average wave making a new 20 bar high (or low)" (rolling 20 per direction per market, no
 *      look-ahead, kept in lsFlexLevels\WavePOC\<MKT>-<spb>.csv);
 *    - its profile (IRT's own volume at price, VAP_REQUIRED) drawn at the wave start pointing right, muted slate, behind the candles;
 *    - every separate node >= 2.0x the wave's normal row, max 5, highlighted ("also you need to highlight the node"): the biggest =
 *      WPOC, bright amber filled row + 3 px amber line; others bright light-grey row + 1 px line (no outline boxes, Rassul 11:21 CT);
 *    - lines run right from the wave's extreme: DASHED while provisional (a new extreme moves it), SOLID from the confirmation bar
 *      (N bars later), never repainted after; a line stops at the first touch (circle), the next qualifying wave or a close beyond the
 *      wave's start (end tick);
 *    - right-edge price tags "WPOC 7700.25 6.3x" / "7693.75 2.5x" only (right of the last bar, never over candles), stacked;
 *    (1.2.0, 2026-10-28 16:26 / 16:31 CT Rassul "the chart is extremely messy" / "the wave poc lines are too large ... 33% less"): NO node lines,
 *      liquidity dotted lines, touch circles or end ticks - the stop rules below still decide and RECORD open / closed; tags only for OPEN
 *      nodes; the profile max width x 0.67 and the liquidity nodes max 40 px. The 1.1.x text below describes the record / stops.
 *    - optional zigzag (off by default); header "Wave POC v1.0.0" small top-left.
 *  1.1.0 (Rassul approved 11:27 CT, mockups wavepoc_lpoc_w1 / w2, wp8.py): LIQUIDITY NODES on the LEFT of the wave start - from IRT's
 *    live order-by-order book (DEPTH_BY_ORDER, one per chart kept alive, opened behind a crash-guard file, drained <= 5,000 events / 8 ms
 *    per call), market-maker orders removed (re-priced >= 2x in 5 min), sampled once a second into the forming bar; a wave's liquidity
 *    = average resting lots per profile row over its bars; nodes on the raw rows (1.1.1: >= 1.9x median; separate when >= 3 rows apart
 *    and the min RAW row between <= 0.7x the smaller node - no smoothing; max 5, biggest = LPOC); drawn as nodes only: bars pointing left (max 60 px), LPOC bright green (bids) / red (offers), others 60%,
 *    faint dotted line right (same stop rules), no tags, no boxes. Waves formed before the indicator loaded have no book -> none.
 *    Each liquidity node + "near_volume_node" (within 2 ticks of a volume node) / "alone" is recorded (LNODE lines) for the nightly.
 *  Rules, files and layout: WavePOCLogic.h (SDK-free, tested: test_wavepoc_logic.cpp on the Oct 22 ES data of the approved mockup;
 *  host: test_wavepoc_host.cpp on a mock SDK). Settings: WavePOCSettings.h, read from IRT's own per-indicator parameters
 *  (getParameters - the SessionVWAP 1.8.1 native pattern; refused / invalid = the defaults).
 *  FILES (lsFlexLevels\WavePOC\):  <MKT>-<spb>.csv          the candidate waves (rolling averages), atomic writes, garbled = rebuilt
 *                                  <MKT>-<spb>-nodes.csv    every qualified wave + nodes + stops (append-only; redrawn from it)
 *          lsFlexLevels\WavePOC.status-<MKT>-<spb>.txt      version, N, waves, averages, VAP, nodes, stops (on change / 30 s)
 *  Per chart state in the host slot (one DLL serves every chart); each closed bar is processed once (no recompute per repaint);
 *  VAP of a wave is read once when it qualifies (missing bars -> retried every 10 min).
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>     // BEFORE irtsdk.h (MSVC C2371 RTBOOL clash otherwise)
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "WavePOCLogic.h"
#include "WavePOCSettings.h"
#include "SettingsHost.h"
#include "HostSlot.h"
#include "ColumnBoard.h"   // (1.3.2) the Levels column (left keep-out) and the right-side columns (price area end) - read only
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <sys/types.h>
#endif

#if defined(_WIN32)
#define WP_SEP "\\"
#else
#define WP_SEP "/"
#endif

static const char* WP_VERSION = "1.3.2";   // = wpc::VERSION = wpset::PLUGIN_VERSION = the setVersion literal (test_wavepoc_host checks)
static const int WP_MAX_BARS = 20000;      // closed bars replayed at most (history)
static const long long WP_RETRY_MS = 10LL * 60 * 1000;   // VAP-missing waves retried every 10 min

static ss::Ids& wpIds() { static ss::Ids I; return I; }

static long long wpMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string wpFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + WP_SEP "InvestorRT" WP_SEP "rtx" WP_SEP "lsFlexLevels" : std::string(); }
static void wpMkdir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
static bool wpExists(const std::string& p) { struct stat st; return !p.empty() && stat(p.c_str(), &st) == 0; }
// atomic publish: .tmp -> old to .bak -> rename -> the .bak back on failure
static bool wpPublish(const std::string& path, const std::string& text) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false; f.write(text.data(), (std::streamsize)text.size()); f.flush(); if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; } }
#if defined(_WIN32)
    if (::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    std::remove(tmp.c_str()); return false;
#else
    const bool had = wpExists(path);
    if (had) { std::remove(bak.c_str()); if (std::rename(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; } }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (had) std::rename(bak.c_str(), path.c_str()); std::remove(tmp.c_str()); return false; }
    if (had) std::remove(bak.c_str());
    return true;
#endif
}
static bool wpReadSmall(const std::string& path, std::string& out, size_t cap) {
    std::ifstream f(path.c_str(), std::ios::binary); if (!f) return false;
    f.seekg(0, std::ios::end); const std::streamoff n = f.tellg(); if (n < 0 || (size_t)n > cap) return false;
    f.seekg(0); out.assign((size_t)n, '\0'); if (n > 0) f.read(&out[0], n); return (bool)f;
}

// the parameter set IRT keeps for this indicator: one token per settings row, in row order
namespace wpp {
struct Parms { int revBars; RTBOOL zigzag; RTBOOL cob; };   // (1.3.0) + CoB legs (parameter version 2)
static const char* const TEMPLATE = "IBB";
static const char* const DEFAULTS = "3,0,1";
inline bool valid(const Parms& p, std::string& why) {
    if (p.revBars < wpc::N_MIN || p.revBars > wpc::N_MAX) { why = "reversal bars out of range"; return false; }
    if (p.zigzag != 0 && p.zigzag != 1) { why = "box not 0/1"; return false; }
    if (p.cob != 0 && p.cob != 1) { why = "CoB legs box not 0/1"; return false; }
    why = "ok"; return true;
}
} // namespace wpp

struct WState {
    ~WState() { delete dbo; dbo = nullptr; }   // (1.1.0) the destructor unsubscribes the order-by-order feed
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr; bool dboDead = false, blocked = false; int attempt = 0; long long dboEvents = 0, garbage = 0, backlog = 0;
    wpl::Book book; wpl::Sampler samp; std::string bookSym;
    int formIdx = -1; std::vector<long long> formKeys;
    cb::Peers peers; int clipL = 0, clipR = 0;   // (1.3.2) the other plugins' columns on this chart; the drawn x range   // (1.3.2) the forming bar's interim sample stamps (re-keyed when it closes)
    std::string mkt, sym; int spb = 0; double tick = 0; long long tickKey = 0; int N = wpc::N_DEF; bool zz = false; std::string parms = "none";
    bool keyed = false;
    bool cobLegs = true; std::vector<signed char> cobFlags; std::string cobSrc = "not read yet";   // (1.3.0) per chart bar: 1 bull CoB, 2 bear CoB
    bool cobBlocked = false, cobGuardChecked = false; long long cobReads = 0;   // (1.3.1) RTL crash guard; reads done (calc only)
    wpc::Store store; std::string storeWhy = "not loaded", storePath, recPath, statusPath, recWhy = "not loaded"; int recBad = 0;
    wpc::Engine E;
    std::vector<RTDATE> raw;   // the chart stamps fed, chart index off + j
    int off = 0;
    long long lastRetry = -1; int retries = 0;
    int provAt = -2; bool provOk = false; wpc::Wave prov;
    bool vapFault = false, sawPre = false, sawPost = false; int preCalls = 0;
    long long statusMs = -1; std::string statusBody; long long storeSaveMs = -1; int recFails = 0;
    wpc::Scene last;
};

class WavePOC : public cppExtension {
public:
    WavePOC() : cppExtension() {}
    virtual int draw(void);
    // the window: IRT shows and keeps this indicator's own values (native parameters); only a copy IRT hands no valid set gets the
    // table's defaults. Nothing is saved by the plugin (IRT stores the window's OK with the indicator) - the settings layer calls are kept
    // so the settings contract checks this window like every other.
    virtual int parmsLoad(void) {
        if (!ss::inDialog(this) || !wpIds().ok) return RTX_OK;
        wpp::Parms p; std::string why; if (nativeParms(p, why)) return RTX_OK;
        ss::fillDialog(this, wpset::spec(), wpIds(), ss::defaults(wpset::spec(), -1), snap_);
        return RTX_OK;
    }
    virtual int parmsApply(void) {
        if (!ss::inDialog(this) || !wpIds().ok) return RTX_OK;
        const ss::Values V = ss::readDialog(this, wpset::spec(), wpIds(), ss::defaults(wpset::spec(), -1), snap_);   // read only (diagnostic)
        lastWindowN_ = V.i(wpset::K_REV_BARS);
        return RTX_OK;
    }
    virtual int parmsInit(unsigned int iVersion);   // (1.3.1) an older saved set -> the declared defaults (no getParameters inside parmsInit)
    virtual int parmsUpdt(unsigned int i) { if (wpIds().ok) ss::onUpdate(this, wpset::spec(), wpIds(), (int)i, snap_); return RTX_OK; }
    ss::Snap snap_; int lastWindowN_ = 0;
    bool nativeParms(wpp::Parms& p, std::string& why);
    HostSlot<WState> slot_;
    void pump(bool mayCalc);
    void clear() { slot_.release(this); }
    void readSettings(WState& S);
    bool identify(WState& S);
    void loadFiles(WState& S);
    void feed(WState& S, bool mayCalc);
    void readCob(WState& S, long n);
    void flush(WState& S, long long now);
    void status(WState& S, long long now);
    int vapRows(WState& S, int chartBar, std::vector<wpc::VRow>& out);
    long long localSec(RTDATE d);
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    void render(WState& S, bool back, bool front);
    void openFeed(WState& S);
    void drainBook(WState& S, long long now);
    void bindLiq(WState& S);
    bool readFlags(const char* name, long count, std::vector<float>& out);
};

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { static_cast<WavePOC*>(this)->clear(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<WavePOC*>(this)->clear(); return RTX_OK; }
int cppExtension::calc(int) { WavePOC* me = static_cast<WavePOC*>(this); try { me->pump(true); } catch (...) {} return RTX_OK; }   // (1.3.1) the only context that feeds bars / reads the RTL
int cppExtension::setup(void) {
    ss::registerAll(this, wpset::spec(), wpIds());   // the window: Reversal bars, Show zigzag (controls only)
    return RTX_OK;
}

// (1.3.1) IRT hands a saved set of an older parameter version (1 = Reversal bars, Show zigzag): save the declared defaults (Reversal bars 3,
// zigzag off, CoB legs on). 1.3.0 called getParameters here - inside parmsInit IRT is converting the stored set and the SDK only offers
// getFormerParameters for that; the proven pattern (lsSessionVWAP 1.8.2) saves without reading.
int WavePOC::parmsInit(unsigned int iVersion) {
    if (iVersion >= wpset::spec().paramVersion) return RTX_OK;
    wpp::Parms p; std::memset(&p, 0, sizeof p); p.revBars = wpc::N_DEF; p.zigzag = 0; p.cob = 1;
    return saveParameters(&p, sizeof p, wpp::TEMPLATE) == RTX_OK ? RTX_OK : RTX_FAIL;
}
// (1.3.0) a custom indicator's (RTL) values per chart bar, as lsCobImpulse::readFlags (the same CoB source as the Cob marks)
bool WavePOC::readFlags(const char* name, long count, std::vector<float>& out) {
    out.assign((size_t)std::max(0L, count), 0.0f);
    char nb[96]; std::memset(nb, 0, sizeof(nb)); std::strncpy(nb, name, sizeof(nb) - 1);
    RTARRAY a(fEmptyArray);
    if (a.getCustomIndicator(nb, 1, true) != RTX_OK || a.count <= 0 || a.count > count) return false;
    const long shift = count - (long)a.count;
    for (long i = shift; i < count; ++i) { const float v = a[(int)(i - shift)]; out[(size_t)i] = (v == v) ? v : 0.0f; }
    return true;
}

// SEH-guarded VAP reads (only trivial values inside, /EHsc compatible) - as lsDeltaProfile
int WavePOC::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int WavePOC::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
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
// one bar's volume at price -> rows in ticks: 1 = read, 0 = no VAP for it (yet), -1 = fault (never re-entered)
int WavePOC::vapRows(WState& S, int chartBar, std::vector<wpc::VRow>& out) {
    out.clear();
    if (S.vapFault) return -1;
    RTARRAYP vap(barVolumeProfile);
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) { S.vapFault = true; return -1; }
    if (prices <= 0) return 0;
    if (prices > 100000) return 0;
    std::vector<VOLPROFILE> rows((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rows[0], prices);
    if (got == -1) { S.vapFault = true; return -1; }
    if (got < 0) return 0;
    for (const VOLPROFILE& v : rows) {
        if (!std::isfinite((double)v.price) || v.totalVolume < 0) continue;
        double vol = (double)v.totalVolume; const double bs = (double)v.buyVolume + (double)v.sellVolume; if (bs > vol + 0.5) vol = bs;
        if (vol <= 0) continue;
        wpc::VRow r; if (!wpc::toTicks((double)v.price, S.tick, r.tick)) continue;
        r.vol = (long long)std::llround(vol); out.push_back(r);
    }
    return out.empty() ? 0 : 1;
}

long long WavePOC::localSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof t);
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    long long y = t.tm_year + 1900; const unsigned m = (unsigned)t.tm_mon + 1, dd = (unsigned)t.tm_mday;
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + dd - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long long days = era * 146097 + (long long)doe - 719468;
    return days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
}

// the chart's own settings, from IRT's parameter set for this indicator (every chart keeps its own); refused / invalid = the table's defaults
bool WavePOC::nativeParms(wpp::Parms& p, std::string& why) {
    std::memset(&p, 0, sizeof p);
    if (getParameters(&p, sizeof p, wpp::TEMPLATE, wpp::DEFAULTS) != RTX_OK) { why = "refused"; return false; }
    return wpp::valid(p, why);
}
void WavePOC::readSettings(WState& S) {
    const ss::Spec& T = wpset::spec();
    wpp::Parms p; std::string why;
    if (!nativeParms(p, why)) { S.N = wpc::clampN((long long)T.rows[wpset::K_REV_BARS].def); S.zz = T.rows[wpset::K_ZIGZAG].def != 0; S.cobLegs = T.rows[wpset::K_COB_LEGS].def != 0; S.parms = "defaults," + why; return; }
    S.N = wpc::clampN(p.revBars); S.zz = p.zigzag != 0; S.cobLegs = p.cob != 0; S.parms = "native,ok";
}

bool WavePOC::identify(WState& S) {
    char rb[64] = { 0 }; const char* rs = getRootSymbol(rb);
    const std::string mkt = wpc::marketForRoot(rs ? rs : "");
    const char* sy = getSymbol(); const std::string sym = sy ? sy : "";
    int spb = getSecondsPerBar(); if (spb < 0) spb = 0;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) tick = getTickIncrement();
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) return false;
    if (mkt.empty()) return false;
    readSettings(S);
    const long long tk = std::llround(tick * 1e9);
    if (!S.keyed || mkt != S.mkt || sym != S.sym || spb != S.spb || tk != S.tickKey || S.N != S.E.N || S.cobLegs != S.E.cobLegs) {
        S.mkt = mkt; S.sym = sym; S.spb = spb; S.tick = tick; S.tickKey = tk; S.keyed = true;
        S.E.reset(); S.E.N = S.N; S.E.cobLegs = S.cobLegs; S.E.tick = tick; S.E.store = &S.store; S.raw.clear(); S.off = 0; S.provAt = -2; S.provOk = false;
        if (S.bookSym != sym) { delete S.dbo; S.dbo = nullptr; S.dboDead = false; S.blocked = false; S.attempt = 0; S.book = wpl::Book(); S.samp = wpl::Sampler(); S.formIdx = -1; S.formKeys.clear(); S.bookSym = sym; }
        bindLiq(S);
        loadFiles(S);
    }
    return true;
}

void WavePOC::loadFiles(WState& S) {
    const std::string d = wpFlex(); if (d.empty()) { S.storeWhy = "no USERPROFILE"; return; }
    const std::string dir = d + WP_SEP "WavePOC", key = S.mkt + "-" + std::to_string(S.spb);
    S.storePath = dir + WP_SEP + key + ".csv"; S.recPath = dir + WP_SEP + key + "-nodes.csv";
    S.statusPath = d + WP_SEP "WavePOC.status-" + key + ".txt";
    std::string text; S.store = wpc::Store(); S.store.n = S.N; S.store.tickKey = S.tickKey;
    if (!wpReadSmall(S.storePath, text, 512 * 1024)) S.storeWhy = wpExists(S.storePath) ? "unreadable - rebuilt from the chart" : "new - built from the chart";
    else { wpc::Store got; std::string why; if (wpc::storeFromText(text, S.mkt, S.spb, S.N, S.tickKey, got, why)) { S.store = got; S.storeWhy = "loaded"; } else S.storeWhy = why + " - rebuilt from the chart"; }
    S.store.dirty = true;
    std::vector<std::string> lines; std::string rt;
    if (wpReadSmall(S.recPath, rt, 8 * 1024 * 1024)) { std::istringstream in(rt); std::string l; while (std::getline(in, l) && lines.size() < 200000) { if (!l.empty() && l.back() == '\r') l.pop_back(); lines.push_back(l); } S.recWhy = "loaded"; }
    else S.recWhy = wpExists(S.recPath) ? "unreadable" : "new";
    S.E.rec = wpc::parseRecord(lines, S.N, S.tick, S.recBad);
}

// closed bars in order, each once; a changed history (first stamp, fewer bars, a stamp moved) = replay from the start
// (1.1.0) a wave's liquidity = the book samples of its bars (engine bar j = chart bar j + off; stamps as the engine's bars)
void WavePOC::bindLiq(WState& S) {
    WState* P = &S;
    S.E.liqFn = [P](const wpc::Wave& w, std::vector<double>& rows, std::vector<double>& bids) {
        std::vector<long long> ts; for (int j = w.i0; j <= w.i1 && j < (int)P->E.B.size(); ++j) ts.push_back(P->E.B[(size_t)j].t);
        return P->samp.average(ts, w.e0, w.rowTicks, w.rows.size(), rows, bids, P->spb > 0 ? (long long)P->spb : 0);   // (1.3.2) every sample inside each bar
    };
    // (1.3.0) the CoB flags of engine bar k (= chart bar k + off), as read when the bar was fed
    S.E.cobFn = [P](int k) { const long c = (long)k + P->off; return c >= 0 && c < (long)P->cobFlags.size() ? (int)P->cobFlags[(size_t)c] : 0; };
}
// chart context only, behind the crash guard; 1st try the chart's symbol, 2nd try NULL (= this chart) - as lsSweepRefill
void WavePOC::openFeed(WState& S) {
    if (S.dbo || S.dboDead || S.blocked || S.attempt >= 2) return;
    const std::string dir = wpFlex() + WP_SEP "WavePOC";
    const std::string guard = wpFlex().empty() ? std::string() : dir + WP_SEP "_opening-" + S.mkt + "-" + std::to_string(S.spb) + ".txt";
    if (S.attempt == 0 && wpExists(guard)) { S.blocked = true; return; }
    if (!guard.empty()) { wpMkdir(wpFlex()); wpMkdir(dir); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER\n"; }
    try { S.dbo = new (std::nothrow) DEPTH_BY_ORDER(S.attempt == 0 && !S.sym.empty() ? S.sym.c_str() : NULL); }
    catch (...) { S.dboDead = true; }
    ++S.attempt;
    if (!guard.empty()) std::remove(guard.c_str());
}
// every order event into the book (<= 5,000 events / ~8 ms per call, the rest next call), then one sample a second into the forming bar
void WavePOC::drainBook(WState& S, long long now) {
    openFeed(S);
    if (S.dbo && !S.dboDead) {
        const long long t0 = wpMs(); int n = 0; bool cap = false;
        try {
            while (true) {
                if (n >= 5000) { cap = true; break; }
                if ((n & 511) == 511 && wpMs() - t0 > 8) { cap = true; break; }
                if (S.dbo->getNext() != RTX_OK) break;
                ++n; ++S.dboEvents;
                const DEPTH_ORDER& o = S.dbo->dbo;
                wpl::Ev e; e.action = o.action <= 3 ? (int)o.action : 99; e.side = o.buySell == 1 ? 1 : o.buySell == 0 ? 0 : -1;
                if (!wpc::toTicks((double)o.price, S.tick, e.tick)) { ++S.garbage; continue; }
                e.size = (long long)o.size; e.id = (long long)o.orderID;
                S.book.onEvent(e, now);
            }
        } catch (...) { S.dboDead = true; }
        if (cap) ++S.backlog;
    }
    const long n = getBarCount(); if (n < 1) return;
    RTARRAYI times(barDateTime); if ((long)times.count < n) return;
    // (1.3.2) the forming bar's stamp may move while it forms: its samples are re-keyed to the closed bar's final stamp once it closes
    if (S.formIdx >= 0 && S.formIdx != (int)n - 1 && S.formIdx < (int)n) { S.samp.rekey(S.formKeys, localSec((RTDATE)times[S.formIdx])); S.formKeys.clear(); }
    if (S.formIdx != (int)n - 1) { S.formIdx = (int)n - 1; S.formKeys.clear(); }
    const long long key = localSec((RTDATE)times[(int)n - 1]);
    if (S.samp.sample(S.book, key, now) && std::find(S.formKeys.begin(), S.formKeys.end(), key) == S.formKeys.end() && S.formKeys.size() < 4096) S.formKeys.push_back(key);
}

void WavePOC::feed(WState& S, bool mayCalc) {
    if (!mayCalc) return;   // (1.3.1) bars (and their CoB flags) are fed from calc only - never from draw()
    const long n = getBarCount(); if (n < 2) return;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)times.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) return;
    const int closed = (int)n - 1;                        // bars 0 .. n-2 are closed (a later bar exists)
    const int fed = (int)S.raw.size();
    bool redo = false;
    if (fed > 0) {
        const int lastIdx = S.off + fed - 1;
        if (lastIdx >= closed || (RTDATE)times[lastIdx] != S.raw.back() || (RTDATE)times[S.off] != S.raw.front()) redo = true;
    }
    if (redo) {
        S.E.reset(); S.E.N = S.N; S.E.cobLegs = S.cobLegs; S.E.tick = S.tick; S.E.store = &S.store; S.raw.clear(); S.provAt = -2;   // the store + record stay (superset)
        bindLiq(S);
    }
    if (S.raw.empty()) S.off = std::max(0, closed - WP_MAX_BARS);
    wpc::VapFn vap = [this, &S](int j, std::vector<wpc::VRow>& out) { return vapRows(S, j + S.off, out); };
    if (S.off + (int)S.raw.size() < closed) readCob(S, n);   // (1.3.0) new closed bars: their CoB flags (the RTL arrays), read once per batch (calc only)
    for (int i = S.off + (int)S.raw.size(); i < closed; ++i) {
        wpc::Bar b; b.t = localSec((RTDATE)times[i]);
        if (!wpc::toTicks(op[i], S.tick, b.o) || !wpc::toTicks(hi[i], S.tick, b.h) || !wpc::toTicks(lo[i], S.tick, b.l) || !wpc::toTicks(cl[i], S.tick, b.c)) { b.o = b.h = b.l = b.c = S.E.B.empty() ? 0 : S.E.B.back().c; }
        if (b.h < b.l) std::swap(b.h, b.l);
        S.E.addBar(b, vap); S.raw.push_back((RTDATE)times[i]);
    }
    const long long now = wpMs();
    if (S.E.pendingCount() > 0 && (S.lastRetry < 0 || now - S.lastRetry >= WP_RETRY_MS)) {
        if (S.lastRetry >= 0) { ++S.retries; S.E.retryPending(vap); }
        S.lastRetry = now;
    }
    const int k = S.E.lastBar();
    if (k != S.provAt) { S.provAt = k; S.provOk = S.E.provisional(S.prov, vap); }
}

// (1.3.1) the CoB flags of every chart bar from his RTL (as lsCobImpulse::decide: calc only, once per batch of new closed bars), behind a crash
// guard file: written before the read, removed after; found at the first read of a chart = the last read killed IRT -> never read again on
// this chart (no new CoB legs; recorded ones still draw) until he deletes the file.
void WavePOC::readCob(WState& S, long n) {
    S.cobFlags.assign((size_t)n, 0);
    if (!S.cobLegs) { S.cobSrc = "CoB legs off (not read)"; return; }
    const std::string fx = wpFlex(), dir = fx + WP_SEP "WavePOC";
    const std::string guard = fx.empty() ? std::string() : dir + WP_SEP "_cobread-" + S.mkt + "-" + std::to_string(S.spb) + ".txt";
    if (!S.cobGuardChecked) { S.cobGuardChecked = true; if (!guard.empty() && wpExists(guard)) S.cobBlocked = true; }
    if (S.cobBlocked) { S.cobSrc = "NOT READ (crash guard " + guard + " present: the last RTL read killed IRT; delete it to retry)"; return; }
    if (!guard.empty()) { wpMkdir(fx); wpMkdir(dir); std::ofstream g(guard.c_str(), std::ios::trunc); g << "reading cob_bull_3m_1b / cob_bear_3m_1b\n"; }
    std::vector<float> fb, fs;
    const bool okB = readFlags("cob_bull_3m_1b", n, fb), okS = readFlags("cob_bear_3m_1b", n, fs);
    ++S.cobReads;
    if (!guard.empty()) std::remove(guard.c_str());
    for (long i = 0; i < n; ++i) S.cobFlags[(size_t)i] = (signed char)((okB && fb[(size_t)i] >= 0.5f ? 1 : 0) | (okS && fs[(size_t)i] >= 0.5f ? 2 : 0));
    S.cobSrc = std::string("RTL cob_bull_3m_1b ") + (okB ? "ok" : "NOT FOUND") + ", cob_bear_3m_1b " + (okS ? "ok" : "NOT FOUND") + " (the Cob marks' source; read in calc only, reads " + std::to_string(S.cobReads) + ")";
}

void WavePOC::flush(WState& S, long long now) {
    const std::string d = wpFlex(); if (d.empty()) return;
    if (!S.E.out.empty()) {
        wpMkdir(d); wpMkdir(d + WP_SEP "WavePOC");
        const bool fresh = !wpExists(S.recPath);
        std::ofstream f(S.recPath.c_str(), std::ios::binary | std::ios::app);
        if (f) { if (fresh) f << wpc::REC_HEAD << "\n"; for (const std::string& l : S.E.out) f << l << "\n"; f.flush(); }
        if (f.good()) S.E.out.clear(); else ++S.recFails;     // kept for the next try (never lost silently)
        if (S.E.out.size() > 5000) S.E.out.clear();
    }
    if (S.store.dirty && (S.storeSaveMs < 0 || now - S.storeSaveMs >= 5000)) {
        wpMkdir(d); wpMkdir(d + WP_SEP "WavePOC");
        S.storeSaveMs = now;
        if (wpPublish(S.storePath, S.store.toText(S.mkt, S.spb))) S.store.dirty = false;
    }
}

void WavePOC::status(WState& S, long long now) {
    if (S.statusPath.empty()) return;
    if (S.statusMs >= 0 && now - S.statusMs < 1000) return;              // calc comes on every tick: build the text at most once a second
    const std::string body = wpc::statusText(S.E, S.mkt, S.spb, S.parms, S.storeWhy, S.recWhy + (S.recFails ? ", append failures " + std::to_string(S.recFails) : std::string()), S.recBad, &S.last, S.provOk, S.retries)
                           + "SETTINGS,show zigzag " + (S.zz ? "on" : "off") + ",CoB legs " + (S.cobLegs ? "on" : "off") + "\nCOB_SOURCE," + S.cobSrc + "\nDRAWING," + (S.sawPre ? "profile behind the candles (pre-draw)" : "profile drawn after the candles (no pre-draw call seen)") + ",x range " + std::to_string(S.clipL) + " .. " + std::to_string(S.clipR) + " (left of the Levels column / right-side columns not drawn)\n"
                           + (S.vapFault ? "VAP_FAULT,remove and re-add the indicator\n" : "")
                           + "MBO," + (S.blocked ? "not opened (crash guard file present)" : S.dboDead ? "faulted" : S.dbo ? "open" : "opening") + ",events " + std::to_string(S.dboEvents)
                           + ",orders " + std::to_string(S.book.book.size()) + ",samples " + std::to_string(S.samp.samples) + ",bars sampled " + std::to_string(S.samp.bars.size())
                           + ",market-maker orders left out " + std::to_string(S.samp.mmSeen) + ",backlog slices " + std::to_string(S.backlog) + ",garbage " + std::to_string(S.garbage) + "\n";
    if (body == S.statusBody && S.statusMs >= 0 && now - S.statusMs < 30000) return;
    S.statusMs = now; S.statusBody = body;
    wpMkdir(wpFlex()); wpPublish(S.statusPath, body);
}

void WavePOC::pump(bool mayCalc) {
    WState* S = slot_.get(this, true); if (!S) return;
    if (!identify(*S)) return;
    const long long now = wpMs();
    drainBook(*S, now);   // (1.1.0) before the new closed bars: the forming bar's samples are complete when it closes
    feed(*S, mayCalc);   // (1.3.1) no-op from draw()
    flush(*S, now);
    status(*S, now);
}

static short wpC(int v) { return (short)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

void WavePOC::render(WState& S, bool back, bool front) {
    const long n = getBarCount(); if (n < 2 || S.E.B.empty()) return;
    RCT pane; pane.getPaneRect(false);
    int vs = 0, ve = (int)n - 1; if (getVisibleBars(&vs, &ve) != RTX_OK) { vs = 0; ve = (int)n - 1; }
    RTARRAY hi(barHigh), lo(barLow);
    wpc::View v; v.l = pane.left; v.t = pane.top; v.r = pane.right; v.b = pane.bottom; v.tick = S.tick; v.zigzag = S.zz;
    v.first = vs - S.off; v.last = ve - S.off; v.lastBar = (int)n - 1 - S.off;
    v.bw = std::max(1, (int)getPixelsPerBar());
    const int off = S.off; const double tick = S.tick;
    v.X = [this, off](int j) { PNT p; p.set(j + off, 0.0f, kBarCenter); return (double)p.h; };
    v.Y = [this, tick](double tk) { return (double)getVerticalDrawPosition((float)(tk * tick)); };
    FONT f9; f9.id = HELVETICA; f9.size = 9; f9.style = PLAIN; FONT f9b; f9b.id = HELVETICA; f9b.size = 9; f9b.style = BOLD;
    v.textW = [this, &f9, &f9b](const std::string& s, bool bold) { setFont(bold ? f9b : f9); return (int)getTextWidth(s.c_str(), -1); };
    if ((long)hi.count >= n && (long)lo.count >= n) { long long h = 0, l = 0; if (wpc::toTicks(hi[(int)n - 1], tick, h) && wpc::toTicks(lo[(int)n - 1], tick, l)) { v.formH = std::max(h, l); v.formL = std::min(h, l); v.haveForming = true; } }
    // (1.3.2, Rassul 18:24 CT) nothing under the Key Levels column at the left edge (ColumnBoard leftKeep, as lsBigBidAsk 1.31.3 /
    // lsDeltaProfile 2.30.1) nor under the right-side columns (LMT F / READ / Delta / Levels / Dealer: ColumnBoard layout priceRight)
    {   RCT scale; scale.getScaleRect(); int axisR = pane.right;
        if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
        const char* cl = getChartLabel(); const std::string chart = cl ? cl : "";
        const long spb = getSecondsPerBar(); const long long ms = wpMs();
        S.peers.step(S.sym, spb, chart, ms, 0u);
        cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = axisR;
        in.levels = S.peers.on[cb::LEVELS]; in.levelsW = S.peers.w[cb::LEVELS]; in.delta = S.peers.on[cb::DELTA]; in.deltaW = S.peers.w[cb::DELTA];
        in.dealer = S.peers.on[cb::DEALER]; in.dealerW = S.peers.w[cb::DEALER]; in.big = S.peers.on[cb::BIG]; in.bigW = S.peers.w[cb::BIG];
        const cb::Layout L = cb::layout(in);
        int left = pane.left + (S.peers.on[cb::LIQ] ? S.peers.w[cb::LIQ] + 2 : 0);
        if (cb::DELTA_LEFT && L.w[cb::DELTA] > 0) left = std::max(left, L.delta.r + 2);
        if (cb::LEVELS_LEFT && L.w[cb::LEVELS] > 0) left = std::max(left, L.levels.r + 2);
        left = std::max(left, cb::leftKeep(S.sym, spb, chart, (int)pane.left, ms));
        v.clipL = left; v.clipR = (L.ok && L.margin > 0) ? L.priceRight : axisR;
        S.clipL = v.clipL; S.clipR = v.clipR;
    }
    S.last = wpc::layout(S.E, S.provOk ? &S.prov : nullptr, v);
    const int L = pane.left, T = pane.top, R = pane.right, Bt = pane.bottom;
    for (const wpc::Prim& p : S.last.p) {
        if (p.back ? !back : !front) continue;
        switch (p.k) {
        case wpc::Prim::FILL: case wpc::Prim::BOX: case wpc::Prim::OVAL: {
            const int x0 = std::max(L, std::min(p.x0, p.x1)), x1 = std::min(R, std::max(p.x0, p.x1)), y0 = std::max(T, std::min(p.y0, p.y1)), y1 = std::min(Bt, std::max(p.y0, p.y1));
            if (x1 <= x0 || y1 < y0) break;
            RCT r; r.set(wpC(x0), wpC(y0), wpC(x1), wpC(y1 == y0 ? y0 + 1 : y1));
            if (p.k == wpc::Prim::FILL) r.draw(0, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_SOLID);
            else { setPen((COLOR)p.col, (short)p.w, P_SOLID); CBRUSH br((COLOR)p.col, PAT_HOLLOW); br.set();
                   if (p.k == wpc::Prim::BOX) r.draw((short)p.w, (COLOR)p.col, (COLOR)p.col, DRAW_OPAQUE, PAT_HOLLOW); else r.drawOval(DRAW_OPAQUE); }
            break; }
        case wpc::Prim::LINE: {
            int x0 = p.x0, x1 = p.x1, y0 = p.y0, y1 = p.y1;
            if (y0 == y1) { if (y0 < T || y0 > Bt) break; x0 = std::max(L, std::min(R, x0)); x1 = std::max(L, std::min(R, x1)); if (x0 == x1) break; }
            else if (x0 == x1) { if (x0 < L || x0 > R) break; y0 = std::max(T, std::min(Bt, y0)); y1 = std::max(T, std::min(Bt, y1)); }
            else if (x0 < L || x0 > R || x1 < L || x1 > R || y0 < T || y0 > Bt || y1 < T || y1 > Bt) break;
            setPen((COLOR)p.col, (short)p.w, P_SOLID);
            PNT a; a.set(0, 0.0f); a.h = wpC(x0); a.v = wpC(y0); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = wpC(x1); b.v = wpC(y1); b.drawLineTo();
            break; }
        case wpc::Prim::TEXT: {
            if (p.y0 < T || p.y1 > Bt || p.x0 < L || p.x1 > R) break;
            setFont(p.bold ? f9b : f9); setTextColor((COLOR)p.col);
            RCT r; r.set(wpC(p.x0), wpC(p.y0), wpC(p.x1 + 2), wpC(p.y1)); r.drawText(p.text.c_str(), false, false);
            break; }
        }
    }
}

int WavePOC::draw(void) {
    WState* S = slot_.get(this, false);
    if (!S || S->E.B.empty()) { try { pump(false); } catch (...) {} S = slot_.get(this, false); }   // (1.3.1) draw never feeds bars / reads the RTL
    if (!S) return RTX_OK;
    try {
        const bool pre = getReason() == REASON_PREDRAW;
        if (pre) { S->sawPre = true; ++S->preCalls; render(*S, true, !S->sawPost && S->preCalls > 3); }   // the profile + nodes + lines behind the candles
        else { S->sawPost = true; render(*S, !S->sawPre, true); }                                         // tags, zigzag, header (+ the back layer if IRT never pre-draws)
    } catch (...) {}
    return RTX_OK;
}

extern "C" cppExtension* CreateExtension(void) {
    WavePOC* p = new WavePOC();
    p->setArrayCount(1);
    p->setFlags(PRE_DRAWING | POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setDescription("LRA Wave POC v1.3.2 - each big wave's (and each CoB leg's) own volume profile at its start, its high-volume nodes (WPOC amber) and resting-liquidity nodes (left, LPOC) as pullback targets.");
    p->setVersion("1.3.2");   // = WP_VERSION (gex-build reads this literal)
    return p;
}
