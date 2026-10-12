/********************************************************************************
 *  LiquidityProfile.cpp  --  Investor/RT RTX extension  lsLiquidityProfile  (0.10.0, 2026-10-11; 0.10.0: only zones that stand out are drawn - 2x+ the 8 same-width bands behind them and the biggest of them, kept down to 1.7x, after resting 30 s at the same band; reach = 1/2 the expected RTH range (lsHodLod), else 1/2 the 14-session average RTH range, else 6x the 3-min range; the label = that prominence; no boxes across the chart (Rassul 18:53 "remove the boxes too"); LQ = a key level or a CR / PS / HVL level with a 2.5x+ zone on it (ES, GC, HG, NG; logged elsewhere) - research "Liquidity display for absorption trading" + the rules review 18:21-18:51; 0.9.0: the column on the LEFT edge of the price pane, bars facing price, the x-normal label on the price side of each bar, each zone a thin box across the chart to the last bar (mockup v4, Rassul 18:07-18:14); 0.8.1: never over the Delta Profile or its labels (the column moves 64 px further left, past the Delta Profile's "A 4.4x +62" labels) - the Dealer Profile reach is kept PER MARKET (its status file is shared by every chart: the last chart to write it wins) and starts at the Dealer Profile default (150 px) until this market's own reach is seen (Rassul 17:56 "the liquidity profile is overlapping the delta profile"); 0.8.0: the absorption PROCESS in every market (Rassul 2026-10-11 "i want to see the process of absorption, which has the same scenarios in all markets - absorb, exhaust, counter or absorb, pull, break"): every step drawn everywhere at the market's data line where the nightly study set one, the standard line until then; s = Counter (the defenders out-traded the attackers after e, at the Counter line x their normal pace) on the price chart with its reason on the panes and the BUYERS / SELLERS STEPPING IN warning; panes AGGRESSIVE (Attack, Exh, Counter) / PASSIVE (Absorbed = the % of everything resting that took fills, Pulled, Rebuilt); 0.7.0: e? -> e and p? -> p (the watches) and t (trapped) on the price chart at the zone side of the bar, recorded and re-read for the last 10 sessions; the warning top left of the price chart (EXHAUSTION WATCH / PULLING WATCH / BUYERS HOLDING / BIDS THINNING / BUYERS PULLING / ABSORPTION OCCURRING / ABSORPTION WEAKENING / BREAKDOWN RISK / TRAPPED / GIVING UP, mirrored at resistance); the reason in plain words at each decided touch on the panes; mouse over a past touch shows its statistics as of that bar; headers AGGRESSION / DEFENSE; e decided at this market's line from arrival + 3 min; 0.6.0: the pane statistics on the left of each half in process order, Metric: now (line) - Attack (x normal per tick vs the prior 20 bars), Eaten (% of everything there: shown + new + refills), Exh | Pulled, Rebuilt - each measured as lra.mbo_study measures it and drawn only where its per-market line came from the data; depth read every 1 s (the study's snapshot step); 0.5.0: only what the data confirmed - Exh (aggression) and Pull (defense), each drawn only in markets where the nightly study confirmed it, no '?' (Rassul 2026-10-11 "remove whatever doesn't work and keep what works"); LoF / HiB / RpB / RpO and the bar-pace code removed; 0.4.1: each pane signal drawn with its number - Exh 0.3x (2nd / 1st half pace, under 0.5), Pull 72% (share pulled), LoF / HiB / RpB / RpO 2.4x (x the prior 20 bars' pace, 2.0x or more; PARAM pace_min per market); 0.4.0: no marks on the price chart (L? / LX / X removed, Rassul 2026-10-10 23:35); six pane signals Exh / LoF / HiB (aggression) and Pull / RpB / RpO (defense), recorded per session and redrawn after a restart, '?' until the study confirms each for the market; the zone label text removed; 0.3.1: the PULL? / E / LX thresholds per market from the data (PARAM lines in LRA-LiqNorms-<MKT>.csv, lra.liq_params), shown on the status file; 0.3.0: Flow pane green = buyers / red = sellers in both halves, DEFENSE = each side's net resting size (bids in / offers beyond a support zone, mirrored at resistance), O and R on the cumulative crosses; 0.2.0: the absorption story per bar (E / O / P / R / X) + the lsLiquidityFlow pane build (LiquidityFlow.cpp defines LQ_FLOW); 0.1.3: pulling blocks L? and labels a break LX P; 0.1.2: PULL? / refill - a zone being pulled instead of replenished; 0.1.1: status built at most once a second)
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
#include "KeyLevelsLogic.h"      // (0.10.0) the key levels, computed natively from the chart (as lsDealerProfile)
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
#include <set>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define LQ_VERSION "0.10.0"          // keep equal to lqp::VERSION and the setVersion literal below

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;          // green bullish / support, red bearish / resistance
static const COLOR C_BUY_DIM = 0x00166534, C_SELL_DIM = 0x007F1D1D;  // the same two colours, dimmed, for the plain book rows
static const COLOR C_GREY = 0x009CA3AF, C_AXIS = 0x00334155, C_INK = 0x000B0F17;
static const int WIDTH_INDEX = 0, FONT_INDEX = 1;
static const long long DEPTH_EVERY_MS = 1000, ZONES_EVERY_MS = 500, STATUS_EVERY_MS = 30000;
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
    std::string params = "built-in defaults";
    std::set<std::string> confirmed;               // (0.4.0) pane signals the nightly study confirmed for this market (only these are drawn, 0.5.0)   // (0.3.1) the per-market thresholds in use and where each came from (data / default)
    // VAP of the bar being read: tick -> (buy, sell) already counted
    int vapBar = -1; RTDATE vapStamp = 0; std::map<lqp::Tick, std::pair<long long, long long>> vapRows;
    long barCount = 0;
    long long ctOffset = 0; bool haveOffset = false;
    long long lastBuild = -1, lastDepth = -1, lastZones = -1, lastStatus = -1, lastNorms = -1, lastLayout = -1;
    bool depthAvail = false; int depthMax = 0, depthLevels = 0; long long depthReads = 0;
    long long session = -1; size_t marksWritten = 0; long long recWrites = 0, recFails = 0;
    long long normsStamp = -2, layoutStamp = -2, dealerStamp = -2;
    int dpWidth = 50, dpGap = 0, dealerReach = 150, width = 70, font = 9;   // (0.8.1) 150 = the Dealer Profile default reach until this market's is seen
    std::string reachMkt;                                                       // (0.8.1) the market whose saved reach was read
    std::vector<std::pair<lqp::Tick, lqp::Tick>> dpBid, dpAsk; long long dpStamp = -2;
    double atr = 0; int slot = -1;
    // (0.10.0) levels for the LQ confirmation and the reach
    struct Lvl { std::string code; lqp::Tick px = 0; };
    std::vector<Lvl> levels; long levelsAt = -1; long long mqStamp = -2; std::vector<Lvl> mqLevels;
    double avgRth = 0; int avgRthN = 0, rthOpen = -1, rthClose = -1;
    double expRange = 0; long long hlStamp = -2; std::string reachSrc = "6x 3-min range";
    std::vector<std::string> confirms;
    std::map<long long, std::string> lqMark;       // zone id -> "LQ PDL" (drawn in ES / GC / HG / NG only)
    bool lqShow = false;             // what LQ marks are showing (status file)
    // (0.6.0) the per-market lines of the pane statistics (PARAM lines); a statistic is drawn only when its line came from the data
    struct Line { double v = -1; bool data = false; };
    Line lAttack, lEaten, lExh, lPulled, lRebuilt, lCounter;
    int hoverBar = -1;                             // (0.7.0) the bar the mouse is over (Flow), -1 = live
    std::string state = "starting", lastStatusText;
    ~LQState() { delete md; }
};

class LiquidityProfile : public cppExtension {
public:
    LiquidityProfile() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
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
    void loadLevels(LQState& S, long count);     // (0.10.0) key levels + CR / PS / HVL + the 14-session RTH range
    void loadReach(LQState& S);
    void updateConfirms(LQState& S);              // (0.10.0) LQ: a key / CR / PS / HVL level with a 2.5x+ shown zone on it                  // (0.10.0) the reach: 1/2 expected range (lsHodLod) / 1/2 average RTH range / 6x
    void loadDelta(LQState& S);
    void session(LQState& S, long long t);
    void persist(LQState& S);
    void status(LQState& S, long long now);
    void render(LQState& S);
    void renderFlow(LQState& S);
    void renderLetters(LQState& S);                // (0.7.0) e? e p? p t on the price chart
    void renderWarning(LQState& S);                // (0.7.0) the warning, top left of the price chart
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
// (0.8.0, Rassul 2026-10-11 "i want to see the process of absorption, which has the same scenarios in all markets") every step of the
// process is drawn in every market: e (Exh) at or under the market's line, p (Pull) at or over it, s (Counter) at or over it, t (trapped),
// a watch (e? p?) until that same touch printed the full step (then the e / p stands for it, like A? -> A), the reasons whenever they have
// words. The line is the market's own from the nightly data, the standard line until the study sets one (the status file says which).
static bool sameTouch(const lqp::Mark& a, const lqp::Mark& b) { return a.tA == b.tA && a.side == b.side && a.lo == b.lo; }
static bool shown(const LQState& S, const lqp::Mark& m) {
    if (m.code == "Held" || m.code == "Broke") return !m.reasons.empty();
    if (m.code == "t") return true;
    if (m.code == "ew" || m.code == "pw") {
        const char* full = m.code == "ew" ? "Exh" : "Pull";
        for (size_t i = 0; i < S.e.marks.size(); ++i) if (S.e.marks[i].code == full && sameTouch(S.e.marks[i], m)) return false;
        return true;
    }
    if (m.value < 0) return false;
    if (m.code == "Exh") return m.value <= S.e.P.exhaust + 1e-9;
    if (m.code == "Pull") return m.value + 1e-9 >= S.e.P.pullShare;
    if (m.code == "s") return m.value + 1e-9 >= S.e.P.counterX;
    return false;
}
// (0.8.0) the standard lines, used until the nightly study sets this market's own (lra.liq_params defaults)
static void standardLines(LQState& S) {
    S.lAttack = {2.0, false}; S.lEaten = {0.15, false}; S.lExh = {0.5, false}; S.lPulled = {0.7, false}; S.lRebuilt = {1.0, false}; S.lCounter = {1.25, false};
    S.e.P.exhaust = 0.5; S.e.P.pullShare = 0.7; S.e.P.counterX = 1.25;
    S.e.L.attack = 2.0; S.e.L.eaten = 0.15; S.e.L.exh = 0.5; S.e.L.pulled = 0.7; S.e.L.rebuilt = 1.0; S.e.L.counter = 1.25; S.e.L.trapped = true;
}
int LiquidityProfile::draw(void) {
    pump();
    LQState* S = slot_.get(this, false);
#ifdef LQ_FLOW
    if (S && !S->faulted) { try { renderFlow(*S); } catch (...) { S->state = "draw error"; } }
#else
    if (S && !S->faulted) { try { renderLetters(*S); renderWarning(*S); render(*S); } catch (...) { S->state = "draw error"; } }
#endif
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
        S.normsStamp = S.layoutStamp = S.dealerStamp = S.dpStamp = -2; S.width = w; S.font = f; S.params = "built-in defaults"; S.confirmed.clear();
        S.identity = id; S.root = root; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
        S.e.P.tick = tick > 0 ? tick : 0.01; zoneRule(S.market, S.e.P); standardLines(S);
        S.e.P.requireShown = true; S.e.P.topN = 8;                         // (0.10.0) candidates; only the shown ones are drawn / read
        S.lqShow = S.market == "ES" || S.market == "GC" || S.market == "HG" || S.market == "NG";   // CL / NQ / 6E: logged only
        S.e.P.barMs = spb > 0 ? (lqp::Ms)spb * 1000 : 180000;
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
            S.e.formBar = bar;                                           // (0.2.0) the story's per-bar flow goes to the bar it traded in
            if (!prime && (db > 0 || ds > 0)) S.e.onTrade(t, (double)px * S.tick, (double)std::max(0LL, db), (double)std::max(0LL, ds));
        }
        S.primed = true;
    }
    RTARRAY close(barClose);
    if (close.count >= count) S.e.onLastPrice(close[(int)count - 1]);
}

// (0.6.0) the Attack readout's normal: each closed bar's buyers' / sellers' volume (IRT's own bar statistics) and the ticks it traded
static void seedBars(LQState& S, cppExtension::RTARRAYP& vap, cppExtension::RTARRAY& high, cppExtension::RTARRAY& low, int from, int to) {
    if (S.tick <= 0) return;
    for (int b = std::max(0, from); b <= to && b < (int)vap.count && b < (int)high.count && b < (int)low.count; ++b) {
        cppExtension::BARSTATISTICS st; const int n = vapStats(&vap, b, &st);
        if (n < 0 || st.buyVolume < 0 || st.sellVolume < 0) continue;
        const double hi = (double)high[b], lo = (double)low[b];
        if (!lqp::finite(hi) || !lqp::finite(lo) || hi < lo) continue;
        S.e.setBar(b, (double)st.buyVolume, (double)st.sellVolume, (int)std::llround((hi - lo) / S.tick) + 1, lqp::toTick(hi, S.tick), lqp::toTick(lo, S.tick));
    }
}
// a bar is closed once a later bar exists (checklist #27); each closed bar is decided once, in order
void LiquidityProfile::closedBars(LQState& S, long count) {
    if (S.barCount == 0) { S.barCount = count; S.e.curBar = (int)count - 2; S.e.formBar = (int)count - 1; RTARRAYP vap(barVolumeProfile); RTARRAY hi(barHigh), lo(barLow); seedBars(S, vap, hi, lo, (int)count - 22, (int)count - 2); return; }   // history before load: never decided
    if (count <= S.barCount) { if (count < S.barCount) S.barCount = count; return; }
    RTARRAYI times(barDateTime); RTARRAY close(barClose), high(barHigh), low(barLow);
    if (times.count < count || close.count < count) return;
    loadDelta(S);
    RTARRAYP vap(barVolumeProfile);
    // ATR of the chart's closed bars (20), scaled to a 3-min bar when the chart is not 3-min
    double sum = 0; int m = 0;
    for (int i = (int)count - 2; i >= 0 && m < 20; --i, ++m) sum += (double)high[i] - (double)low[i];
    if (m > 0) { S.atr = sum / m; if (S.spb > 0 && S.spb != 180) S.atr *= std::sqrt(180.0 / (double)S.spb); }
    for (long i = S.barCount - 1; i < count - 1; ++i) {
        const long long bt = (long long)times[(int)i];
        const long long endMs = ctSec((RTDATE)times[(int)i]) * 1000LL;   // IRT stamps the bar END (checklist #12)
        if (endMs > 0) { seedBars(S, vap, high, low, (int)i, (int)i); S.e.onBarClose((int)i, bt, endMs, (double)close[(int)i], S.dpBid, S.dpAsk); }
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
    std::map<int, double> med; lqp::Params p = S.e.P; std::string info; std::set<std::string> conf;
    LQState::Line la{2.0, false}, le{0.15, false}, lx{0.5, false}, lp{0.7, false}, lr{1.0, false}, lc{1.25, false};   // (0.8.0) the standard lines
    p.exhaust = 0.5; p.pullShare = 0.7; p.counterX = 1.25;
    while (std::getline(in, line) && ++lines < 2000) {                  // checklist #33: bounded input
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        long long a = 0, b = 0; double x = 0;
        if (c.size() >= 4 && c[0] == "ZONE" && parseD(c[1], x) && parseLL(c[2], 1, 200, a) && parseLL(c[3], 1, 200, b) && x > 0 && a <= b) { p.k = x; p.hwMin = (int)a; p.hwMax = (int)b; }
        else if (c.size() >= 3 && c[0] == "SLOT" && parseLL(c[1], 0, 47, a) && parseD(c[2], x) && x > 0) med[(int)a] = x;
        // (0.3.1, Rassul 2026-10-10 "data driven ... optimized for each market") PARAM,<name>,<value>,<data|default>,... - each value is
        // range-checked; a bad or missing line keeps the built-in default
        else if (c.size() >= 3 && c[0] == "SIGNAL" && (lqp::Engine::paneCode(c[1]) || c[1] == "Old" || c[1] == "Trapped" || c[1] == "Counter")) { if (c[2] == "confirmed") conf.insert(c[1]); }
        else if (c.size() >= 4 && c[0] == "PARAM" && parseD(c[2], x)) {
            const std::string src = c[3] == "data" ? "data" : "default";
            const bool d = src == "data";
            if (c[1] == "pull_share" && x >= 0.2 && x <= 5) { p.pullShare = x; lp.v = x; lp.data = d; info += " pull_share " + c[2] + " " + src; }
            else if (c[1] == "exhaust" && x >= 0.1 && x <= 0.9) { p.exhaust = x; lx.v = x; lx.data = d; info += " exhaust " + c[2] + " " + src; }
            else if (c[1] == "attack_x" && x >= 0.5 && x <= 10) { la.v = x; la.data = d; info += " attack_x " + c[2] + " " + src; }
            else if (c[1] == "eaten_total" && x >= 0.01 && x <= 1) { le.v = x; le.data = d; info += " eaten_total " + c[2] + " " + src; }
            else if (c[1] == "rebuilt" && x >= 0.1 && x <= 3) { lr.v = x; lr.data = d; info += " rebuilt " + c[2] + " " + src; }
            else if (c[1] == "counter_x" && x >= 0.5 && x <= 10) { p.counterX = x; lc.v = x; lc.data = d; info += " counter_x " + c[2] + " " + src; }
            else if (c[1] == "eaten_break" && x >= 0.25 && x <= 5) { p.eatenBreak = x; info += " eaten_break " + c[2] + " " + src; }
        }
    }
    S.e.norms.median.swap(med); S.e.norms.loaded = true; S.e.P.k = p.k; S.e.P.hwMin = p.hwMin; S.e.P.hwMax = p.hwMax;
    S.e.P.pullShare = p.pullShare; S.e.P.exhaust = p.exhaust; S.e.P.eatenBreak = p.eatenBreak; S.e.P.counterX = p.counterX;
    S.params = info.empty() ? "built-in defaults" : info.substr(1);
    S.confirmed.swap(conf);
    S.lAttack = la; S.lEaten = le; S.lExh = lx; S.lPulled = lp; S.lRebuilt = lr; S.lCounter = lc;
    // (0.8.0) the process in every market: the reasons and warnings use the market's data line, the standard line until it exists
    S.e.L.attack = la.v; S.e.L.eaten = le.v; S.e.L.exh = lx.v; S.e.L.pulled = lp.v; S.e.L.rebuilt = lr.v; S.e.L.counter = lc.v; S.e.L.trapped = true;
}

// where the column goes: just left of the Delta Profile (its width / gap for this root) and the Dealer Profile (its reach)
void LiquidityProfile::loadLayout(LQState& S) {
    const std::string f = flexDir(); if (f.empty()) return;
    if (S.reachMkt != S.market) {                                               // (0.8.1) this market's last known Dealer Profile reach
        S.reachMkt = S.market; S.dealerReach = 150;
        std::ifstream in((f + "\\LiquidityProfile.reach-" + S.market + ".txt").c_str()); long long r = 0; std::string ln;
        if (std::getline(in, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (parseLL(ln, 0, 1199, r) && r > 0) S.dealerReach = (int)r; }
    }
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
        if (mkt == S.market && reach > 0 && (int)reach != S.dealerReach) {      // (0.8.1) this market's own reach: use it and keep it
            S.dealerReach = (int)reach;
            publishFile(f + "\\LiquidityProfile.reach-" + S.market + ".txt", std::to_string(reach) + "\n");   // checklist #5
        }
    }
}

// (0.10.0) the market of a MenthorQLevels.csv symbol: CQG (EPZ26, ENQZ26, CLEX26, GCEZ26, CPEZ26, NGEX26, EU6Z26) or Rithmic
// (ESZ6, NQZ6, CLX6, GCZ6, HGZ6, NGX6, 6EZ6) - the root before the month code
static std::string mqMarket(const std::string& sym) {
    static const char* const map[][2] = { {"ENQ", "NQ"}, {"EP", "ES"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"},
                                          {"NQ", "NQ"}, {"ES", "ES"}, {"CL", "CL"}, {"QCL", "CL"}, {"GC", "GC"}, {"QGC", "GC"}, {"HG", "HG"}, {"QHG", "HG"},
                                          {"NG", "NG"}, {"6E", "EU"} };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
        const std::string r = map[i][0];
        if (sym.size() >= r.size() + 2 && sym.compare(0, r.size(), r) == 0 && std::strchr("FGHJKMNQUVXZ", sym[r.size()])) return map[i][1];
    }
    return "";
}
// a MenthorQ label part that is a gamma-1 level (call resistance, put support, high-vol level); gamma walls 1-10 never confirm
static bool gamma1(const std::string& part) {
    std::string p = part; while (!p.empty() && p[0] == ' ') p.erase(0, 1);
    return p.compare(0, 2, "CR") == 0 || p.compare(0, 2, "PS") == 0 || p.compare(0, 3, "HVL") == 0;
}
void LiquidityProfile::loadLevels(LQState& S, long count) {
    if (S.market.empty() || S.tick <= 0) return;
    // MenthorQ CR / PS / HVL levels (the one external input of the confirmation; read only when the file changes)
    const std::string f = flexDir();
    if (!f.empty()) {
        long long st = -1; const std::string mq = f + "\\MenthorQLevels.csv";
        if (!statFile(mq, &st)) { S.mqLevels.clear(); S.mqStamp = -1; }
        else if (st != S.mqStamp) {
            S.mqStamp = st; S.mqLevels.clear();
            std::ifstream in(mq.c_str()); std::string line; int n = 0;
            while (std::getline(in, line) && ++n < 2000) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                const std::vector<std::string> c = split(line, ',');
                if (c.size() < 3 || mqMarket(c[0]) != S.market) continue;
                double px = 0; if (!parseD(c[1], px) || px <= 0) continue;
                const std::vector<std::string> parts = split(c[2], '/');
                for (size_t k = 0; k < parts.size(); ++k) if (gamma1(parts[k])) {
                    LQState::Lvl L; L.px = lqp::toTick(px, S.tick); std::string code = parts[k];
                    while (!code.empty() && code[0] == ' ') code.erase(0, 1);
                    while (!code.empty() && code.back() == ' ') code.pop_back();
                    L.code = code; S.mqLevels.push_back(L); break;
                }
            }
        }
    }
    // the key levels from the chart's own bars (lsDealerProfile's KeyLevelsLogic) and the 14-session RTH range - on a new bar only
    if (count == S.levelsAt || count < 30) return;
    S.levelsAt = count;
    if (S.rthOpen < 0 && !svl::rthOf(S.market.c_str(), S.rthOpen, S.rthClose)) { S.rthOpen = -2; return; }
    if (S.rthOpen < 0) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if (times.count < count || hi.count < count || lo.count < count) return;
    const long first = std::max(0L, count - 12000);                    // ~25 sessions of 3-min bars; bounded work (checklist #40)
    std::vector<klv::Bar> B; B.reserve((size_t)(count - first));
    for (long i = first; i < count; ++i) {
        const long long cs = ctSec((RTDATE)times[(int)i]); if (cs <= 0) continue;
        klv::Bar b; b.days = (long)(cs / 86400); b.endSec = (int)(cs % 86400); b.h = hi[(int)i]; b.l = lo[(int)i]; b.c = cl[(int)i];
        B.push_back(b);
    }
    if (B.size() < 30) return;
    const int per = S.spb > 0 ? S.spb : 180;
    const std::vector<klv::Level> L = klv::compute(B, per, S.rthOpen, S.rthClose, S.tick);
    S.levels.clear();
    for (size_t i = 0; i < L.size(); ++i) {
        const std::string& c = L[i].code;                                   // his trade levels only (no weekly, no hourly swings)
        if (c == "PDH" || c == "PDL" || c == "PFH" || c == "PFL" || c == "ONH" || c == "ONL" || c == "LonHI" || c == "LonLO") {
            LQState::Lvl v; v.code = c; v.px = lqp::toTick(L[i].price, S.tick); S.levels.push_back(v);
        }
    }
    // the average RTH range of the last 14 COMPLETE sessions (the current one excluded)
    std::map<long, std::pair<double, double>> rth;
    for (size_t i = 0; i < B.size(); ++i) {
        const svl::Cls c = svl::classify(B[i].days, B[i].endSec, per, S.rthOpen, S.rthClose);
        if (c.kind != svl::RTH) continue;
        std::map<long, std::pair<double, double>>::iterator it = rth.find(c.sess);
        if (it == rth.end()) rth[c.sess] = std::make_pair((double)B[i].h, (double)B[i].l);
        else { it->second.first = std::max(it->second.first, (double)B[i].h); it->second.second = std::min(it->second.second, (double)B[i].l); }
    }
    const svl::Cls cur = svl::classify(B.back().days, B.back().endSec, per, S.rthOpen, S.rthClose);
    double sum = 0; int n = 0;
    for (std::map<long, std::pair<double, double>>::reverse_iterator it = rth.rbegin(); it != rth.rend() && n < 14; ++it) {
        if (it->first >= cur.sess && cur.kind != svl::NONE) continue;
        sum += it->second.first - it->second.second; ++n;
    }
    S.avgRthN = n; S.avgRth = n > 0 ? sum / n : 0;
}
// the reach: 1/2 of today's expected RTH range from lsHodLod's status file (current session, hle valid, written in the last 10
// minutes), else 1/2 of the 14-session average RTH range (10+ sessions on the chart), else the engine's 6x 3-min range
void LiquidityProfile::loadReach(LQState& S) {
    S.expRange = 0;
    const std::string f = flexDir();
    if (!f.empty() && S.spb > 0) {
        const std::string p = f + "\\HodLod.status-" + S.market + "-" + std::to_string(S.spb) + ".txt";
        long long st = -1;
        if (statFile(p, &st)) {
            const bool fresh = (long long)std::time(nullptr) - st / 1000000LL < 600;   // statFile stamp = mtime x 1e6 + size
            std::ifstream in(p.c_str()); std::string line; bool prior = true, valid = false; double rng = 0; int n = 0;
            while (std::getline(in, line) && ++n < 200) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line == "PRIOR,0") prior = false;
                if (line.compare(0, 4, "HLE,") == 0 && line.find(",valid") != std::string::npos) valid = true;
                const size_t k = line.find("range=");
                if (line.compare(0, 11, "HLE_VALUES,") == 0 && k != std::string::npos) { double v = 0; if (parseD(line.substr(k + 6, line.find(' ', k) - k - 6), v)) rng = v; }
            }
            if (fresh && !prior && valid && rng > 0) S.expRange = rng;
        }
    }
    if (S.expRange > 0) { S.e.P.reachTicks = (lqp::Tick)std::llround(0.5 * S.expRange / S.tick); S.reachSrc = "1/2 expected range"; }
    else if (S.avgRthN >= 10 && S.avgRth > 0) { S.e.P.reachTicks = (lqp::Tick)std::llround(0.5 * S.avgRth / S.tick); S.reachSrc = "1/2 avg RTH range " + std::to_string(S.avgRthN) + "d"; }
    else { S.e.P.reachTicks = 0; S.reachSrc = "6x 3-min range"; }
}

void LiquidityProfile::updateConfirms(LQState& S) {
    S.confirms.clear(); S.lqMark.clear();
    if (!S.e.havePx || S.e.zones.empty()) return;
    std::vector<LQState::Lvl> all = S.levels; all.insert(all.end(), S.mqLevels.begin(), S.mqLevels.end());
    for (size_t i = 0; i < all.size(); ++i) {
        const lqp::Zone* z = S.e.confirming(all[i].px, S.e.lastPx);
        if (!z) continue;
        std::ostringstream q; q.imbue(std::locale::classic());
        q << all[i].code << "," << (double)all[i].px * S.e.P.tick << "," << (z->side == lqp::BID ? "support" : "resistance") << ",bx " << z->bx
          << "," << (S.lqShow ? "shown" : "logged");
        S.confirms.push_back(q.str());
        if (S.lqShow) { std::string& m = S.lqMark[z->id]; if (m.empty()) m = "LQ " + all[i].code; else if (m.find(all[i].code) == std::string::npos) m += "/" + all[i].code; }
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
    // (0.7.0, Rassul 2026-10-11 "do you store the reason so i can see the reason when i go back on the chart") every recorded mark and
    // reason of THIS session and the 9 before it is read back, so scrolling back through earlier days shows what was drawn then (#11).
    // The price chart (e? e p? p t) keeps its own record (-chart-), the panes theirs (-panes-: Exh / Pull / the reasons).
#ifdef LQ_FLOW
    const char* kind = "-panes-";
#else
    const char* kind = "-chart-";
#endif
    for (long long k = key - 9; k <= key; ++k) {
        std::ifstream in((d + "\\" + stem(S) + kind + std::to_string(k) + ".csv").c_str());
        std::string line; int n = 0;
        while (std::getline(in, line) && ++n < 5000) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lqp::Mark m; if (lqp::Engine::parse(line, m, S.e.P.tick)) S.e.marks.push_back(m);
        }
    }
    S.marksWritten = S.e.marks.size();
}

void LiquidityProfile::persist(LQState& S) {
    // (0.4.0) lsLiquidityFlow records its pane signals (<MKT>-<spb>-panes-<session>.csv, re-read after a restart: no repaint);
    // lsLiquidityProfile records every attempt for the nightly study (<MKT>-<spb>-attempts-<session>.csv)
    const std::string d = dir(); if (d.empty() || S.session < 0) { S.e.attemptLog.clear(); return; }
    makeDir(flexDir()); makeDir(d);
#ifdef LQ_FLOW
    S.e.attemptLog.clear();
    if (S.marksWritten < S.e.marks.size()) {
        std::vector<std::string> lines;
        for (size_t i = S.marksWritten; i < S.e.marks.size(); ++i) lines.push_back(lqp::Engine::line(S.e.marks[i], S.e.P.tick));
        if (appendLines(d + "\\" + stem(S) + "-panes-" + std::to_string(S.session) + ".csv", lqp::Engine::header(), lines)) { S.marksWritten = S.e.marks.size(); S.recWrites++; }
        else S.recFails++;                                               // kept and retried at the next bar close (#6)
    }
#else
    // (0.7.0) the price chart records its letters (e? e p? p t) - re-read after a restart, never repainted
    if (S.marksWritten < S.e.marks.size()) {
        std::vector<std::string> lines;
        for (size_t i = S.marksWritten; i < S.e.marks.size(); ++i) lines.push_back(lqp::Engine::line(S.e.marks[i], S.e.P.tick));
        if (appendLines(d + "\\" + stem(S) + "-chart-" + std::to_string(S.session) + ".csv", lqp::Engine::header(), lines)) { S.marksWritten = S.e.marks.size(); S.recWrites++; }
        else S.recFails++;
    }
    if (!S.e.attemptLog.empty()) {
        if (appendLines(d + "\\" + stem(S) + "-attempts-" + std::to_string(S.session) + ".csv", lqp::Engine::attemptHeader(), S.e.attemptLog)) S.e.attemptLog.clear();
        else S.recFails++;
    }
#endif
}

void LiquidityProfile::status(LQState& S, long long now) {
    if (S.lastBuild >= 0 && now - S.lastBuild < 1000 && now >= S.lastBuild) return;   // #32: built at most once a second
    S.lastBuild = now;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << LQ_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nTICK," << S.tick
      << "\nDEPTH," << (S.mdBlocked ? "blocked" : S.mdDead ? "fault" : !S.md ? "not opened" : S.depthAvail ? "available" : "not available")
      << ",max " << S.depthMax << ",levels " << S.depthLevels << ",reads " << S.depthReads << ",books " << S.e.books << ",bad " << S.e.badBooks << "," << S.e.why
      << "\nTRADES," << S.e.trades << "\nZONE_HW," << S.e.hw << ",atr " << S.atr << ",slot " << S.slot << ",normal " << S.e.normal << ",norms " << (S.e.norms.loaded ? "file" : "session") << "\nPARAMS," << S.params
      << "\nZONES," << S.e.zones.size() << "\nATTEMPTS," << S.e.attempts.size() << "\nMARKS," << S.e.marks.size() << ",written " << S.marksWritten
      << ",writes " << S.recWrites << ",fails " << S.recFails << "\nDELTA_A," << S.dpBid.size() << "," << S.dpAsk.size()
      << "\nSESSION," << S.session << "\nSTATE," << S.state << "\n";
    {   // (0.6.0) the pane statistics now and the lines in use (value, data / default)
        lqp::Stats st; const bool have = S.e.latest(S.e.bookT, st);
        auto ln = [](const LQState::Line& l) { std::ostringstream q; q.imbue(std::locale::classic()); q << l.v << (l.data ? " data" : " default"); return q.str(); };
        o << "STATS," << (have ? (st.side == lqp::BID ? "support" : "resistance") : "none") << ",attack " << (have ? st.attack : -1) << " (" << ln(S.lAttack)
          << "),eaten " << (have ? st.eaten : -1) << " (" << ln(S.lEaten) << "),exh " << (have ? st.exh : -1) << " (" << ln(S.lExh) << "),pulled "
          << (have ? st.pulled : -1) << " (" << ln(S.lPulled) << "),rebuilt " << (have ? st.rebuilt : -1) << " (" << ln(S.lRebuilt) << "),counter "
          << (have ? st.counter : -1) << " (" << ln(S.lCounter) << "),bars_known "
          << S.e.barNorm.size() << "\n";
    }
    o << "REACH," << S.reachSrc << ",ticks " << S.e.P.reachTicks << ",expected " << S.expRange << ",avg_rth " << S.avgRth << ",sessions " << S.avgRthN
      << "\nLEVELS," << S.levels.size() << " key," << S.mqLevels.size() << " gamma1\nLQ," << (S.lqShow ? "shown" : "logged") << "," << S.confirms.size() << "\n";
    for (size_t i = 0; i < S.confirms.size(); ++i) o << "CONFIRM," << S.confirms[i] << "\n";
    for (size_t i = 0; i < S.e.zones.size(); ++i) {
        const lqp::Zone& z = S.e.zones[i];
        o << "ZONE,bx " << z.bx << ",rank " << z.rank << ",behind " << z.behindN << "," << (z.shown ? "shown" : "hidden") << "," << (z.side == lqp::BID ? "support" : "resistance") << "," << (double)z.lo * S.e.P.tick << "," << (double)z.hi * S.e.P.tick
          << "," << z.size << ",x " << S.e.xNormal(z.size) << ",age_s " << (S.e.bookT - z.born) / 1000 << ",old " << S.e.zoneOld(z.side, z.lo, z.hi, S.e.bookT) << "\n";
    }
    const std::string text = o.str();
    if (text == S.lastStatusText && S.lastStatus >= 0 && now - S.lastStatus < STATUS_EVERY_MS) return;   // #31: on change or every 30 s
    const std::string f = flexDir(); if (f.empty()) return;
#ifdef LQ_FLOW
    const char* kind = "LiquidityFlow";
#else
    const char* kind = "LiquidityProfile";
#endif
    if (publishFile(f + "\\" + kind + ".status-" + stem(S) + ".txt", text)) { S.lastStatusText = text; S.lastStatus = now; }
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
        if (S.lastLayout < 0 || st - S.lastLayout >= 1000) { S.lastLayout = st; loadLayout(S); loadLevels(S, getBarCount()); loadReach(S); updateConfirms(S); }
        openDepth(S);
        const long count = getBarCount();
        if (S.lastDepth < 0 || t - S.lastDepth >= DEPTH_EVERY_MS || t < S.lastDepth) {
            S.lastDepth = t;
            readTrades(S, t, count);                                    // trades first: they explain the book change
            S.e.formBar = (int)count - 1;                               // book flow belongs to the bar in progress
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
    // (0.9.0, Rassul 2026-10-11 18:07-18:14 mockup v4 "move liquidity profile to the left", "facing price", "the label ... on the right",
    // "draw the box across the chart", "justify left the profile"): the column sits FLUSH with the pane's left edge, its bars grow RIGHT
    // toward price, the x-normal label is on the price side of each bar's tip, and each zone is a thin box from there across the chart
    // to the last bar (never past it). Nothing is drawn on the right any more, so it can never cover the Delta / Dealer Profiles.
    const int left = pane.left + 1;
    const int full = S.width - 4;                                                   // the longest bar (the Width setting)
    const int hdrTop = pane.top + S.font + 14;                                      // under the warning box (top left)
    const int top = hdrTop + S.font + 8, bottom = pane.bottom;
    const long count = getBarCount();
    if (paneRight - left < 160 || bottom - top < 40 || count < 2) return;          // #35: no room, draw nothing
    const int lastBar = (int)count - 1;
    int lastX = paneRight;                                                          // the last bar's right edge: boxes stop there
    { PNT pe; if (pe.set(lastBar, 0.0f, kBarRight) == RTX_OK && pe.h > left) lastX = std::min((int)pe.h, paneRight); }
    FONT f; f.id = HELVETICA; f.size = coord(S.font); f.style = BOLD; setFont(f);
    // header: what the column is and whether it has data (no other text on the chart)
    {
        const bool ok = S.e.haveBook && S.depthAvail;
        const std::string h = ok ? "LIQ TESTING" : S.mdBlocked ? "LIQ blocked" : "LIQ no depth";
        setTextColor(ok ? C_GREY : C_SELL);
        RCT r; r.set(coord(left + 2), coord(hdrTop), coord(left + full + 60), coord(hdrTop + S.font + 6)); r.drawText(h.c_str(), false, false);
    }
    if (!S.e.haveBook) return;
    const double tick = S.e.P.tick;
    auto yOf = [&](double price) { PNT p; p.set(lastBar, (float)price); return (int)p.v; };
    // plain book rows (dim), scaled to the biggest single level in view, growing right from the edge
    double maxRow = 0;
    for (int s = 0; s < 2; ++s) for (std::map<lqp::Tick, double>::const_iterator i = S.e.book[s].begin(); i != S.e.book[s].end(); ++i) {
        const int y = yOf((double)i->first * tick); if (y > top && y < bottom) maxRow = std::max(maxRow, i->second);
    }
    double maxZone = 0; for (size_t i = 0; i < S.e.zones.size(); ++i) if (S.e.zones[i].shown) maxZone = std::max(maxZone, S.e.zones[i].size);
    if (maxRow > 0) {
        for (int s = 0; s < 2; ++s) for (std::map<lqp::Tick, double>::const_iterator i = S.e.book[s].begin(); i != S.e.book[s].end(); ++i) {
            int yt = yOf(((double)i->first + 0.5) * tick), yb = yOf(((double)i->first - 0.5) * tick);
            if (yb <= yt) yb = yt + 1;
            if (yb <= top || yt >= bottom) continue;
            const int len = std::max(1, (int)(i->second / maxRow * full * 0.5));
            RCT r; r.set(coord(left), coord(std::max(top, yt)), coord(left + len), coord(std::min(bottom, yb - (yb - yt > 3 ? 1 : 0))));
            r.draw(0, s == lqp::BID ? C_BUY_DIM : C_SELL_DIM, s == lqp::BID ? C_BUY_DIM : C_SELL_DIM, DRAW_OPAQUE, PAT_SOLID);
        }
    }
    // the zones: bright bar (solid = durable, hollow = new) facing price with the lots inside, "5.1x" just past the tip, then the box
    const long long nowT = S.haveOffset ? steadyMs() + S.ctOffset : 0;
    const lqp::Readout rd = S.e.readout(nowT);
    for (size_t i = 0; i < S.e.zones.size(); ++i) {
        const lqp::Zone& z = S.e.zones[i];
        if (!z.shown) continue;                                          // (0.10.0) only zones that stand out from the book behind them
        int yt = yOf(((double)z.edgeHi + 0.5) * tick), yb = yOf(((double)z.edgeLo - 0.5) * tick);   // the real edges of the size
        if (yb <= yt) yb = yt + 2;
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb);
        const COLOR c = z.side == lqp::BID ? C_BUY : C_SELL;
        const bool solid = z.durable(nowT, S.e.P.persistMs);           // (0.4.0) hollow = new / flickering only
        const int len = std::max(8, (int)(maxZone > 0 ? z.size / maxZone * full : full));
        RCT r; r.set(coord(left), coord(yt), coord(left + len), coord(yb));
        if (solid) r.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
        else { setPen(c, 1, P_DASH); CBRUSH br(c, PAT_HOLLOW); br.set(); r.draw(1, c, c, DRAW_OPAQUE, PAT_HOLLOW); }
        const int ym = (yt + yb) / 2;
        FONT g; g.id = HELVETICA; g.size = coord(S.font); g.style = BOLD; setFont(g);
        const std::string lt = lots(z.size); const int lw = getTextWidth(lt.c_str(), -1);
        if (lw + 4 < len && yb - yt >= S.font) { setTextColor(solid ? C_INK : c); RCT tr; tr.set(coord(left), coord(ym - S.font), coord(left + len), coord(ym + S.font)); tr.drawText(lt.c_str(), true, false); }
        if (S.confirmed.count("Old")) {                                 // (0.5.0) old orders: a 2-px outline and "old 64%" under the bar
            const double os = S.e.zoneOld(z.side, z.lo, z.hi, S.e.bookT);
            if (os >= S.e.P.oldShow) {
                setPen(c, 2, P_SOLID); CBRUSH hb(c, PAT_HOLLOW); hb.set();
                RCT orr; orr.set(coord(left), coord(yt - 1), coord(left + len + 1), coord(yb + 1)); orr.draw(2, c, c, DRAW_OPAQUE, PAT_HOLLOW);
                char ot[24]; std::snprintf(ot, sizeof(ot), "old %.0f%%", os * 100.0);
                setTextColor(c); RCT otr; otr.set(coord(left), coord(yb + 1), coord(left + len + 60), coord(yb + S.font + 3)); otr.drawText(ot, false, false);
            }
        }
        // (0.10.0) the label on the PRICE side of the tip = the prominence (x the book behind), then LQ where it confirms a level.
        // No box across the chart (Rassul 18:53 "remove the boxes too because they clutter the chart").
        int xl = left + len + 4;
        if (z.bx > 0) { const std::string xs = mult(z.bx); const int xw = getTextWidth(xs.c_str(), -1); setTextColor(c);
            RCT xr; xr.set(coord(xl), coord(ym - S.font), coord(xl + xw + 2), coord(ym + S.font)); xr.drawText(xs.c_str(), false, false);
            xl += xw + 6; }
        std::map<long long, std::string>::const_iterator lq = S.lqMark.find(z.id);
        if (lq != S.lqMark.end() && xl < lastX) { const int qw = getTextWidth(lq->second.c_str(), -1); setTextColor(c);
            RCT qr; qr.set(coord(xl), coord(ym - S.font), coord(xl + qw + 2), coord(ym + S.font)); qr.drawText(lq->second.c_str(), false, false); }
    }
    (void)rd;   // (0.4.0) no read text and no marks on the price chart (Rassul 2026-10-10: signals only on the panes)
}


// ---- (0.2.0) lsLiquidityFlow: the absorption story in its own pane, for the zone touches since load (Rassul 2026-10-10:
// "the concept of aggression and defense ... the best way to show how and why absorption is occurring or failing"; the replenished-
// over-pulled cross "would mirror aggression giving a double crossover pattern that would be very easy to spot").
// TOP half  AGGRESSION: contracts per bar traded INTO the zone - sellers (red) / buyers (green); attackers are sellers at a support,
//           buyers at a resistance. The defenders' line crossing above the attackers' = O (defense first, then offense).
// BOTTOM    DEFENSE: since price reached the zone, size REPLENISHED vs size PULLED (lots, cumulative); green / red by what it means
//           (new bids / pulled offers are bullish). Replenished crossing above pulled = R. Both crosses = X (the double crossover).
// Signals (0.5.0): Exh (top half) and Pull (bottom half), drawn only where confirmed for the market. O / R crosses are logged only.
// (0.7.0, Rassul 2026-10-11) the order-book letters on the price chart, on the ZONE side of the bar they were decided on (below the bar
// at a support, above it at a resistance), clear of the Delta Profile's A circle; at most two a bar: t > p > e > s > p? > e?
void LiquidityProfile::renderLetters(LQState& S) {
    const long count = getBarCount(); if (count < 2) return;
    RCT pane; pane.getPaneRect(false);
    RTARRAY high(barHigh), low(barLow);
    if (high.count < count || low.count < count) return;
    struct L { int bar; int rank; std::string txt; COLOR c; bool below; };
    std::vector<L> v;
    for (size_t i = 0; i < S.e.marks.size(); ++i) {
        const lqp::Mark& m = S.e.marks[i];
        const char* t = m.code == "t" ? "t" : m.code == "Pull" ? "p" : m.code == "Exh" ? "e" : m.code == "s" ? "s" : m.code == "pw" ? "p?" : m.code == "ew" ? "e?" : nullptr;
        if (!t || !shown(S, m)) continue;
        const int bar = barOfTime(m.barTime, count); if (bar < 0) continue;
        const int rank = m.code == "t" ? 0 : m.code == "Pull" ? 1 : m.code == "Exh" ? 2 : m.code == "s" ? 3 : m.code == "pw" ? 4 : 5;
        L l; l.bar = bar; l.rank = rank; l.txt = t; l.c = lqp::Engine::bullish(m) ? C_BUY : C_SELL; l.below = m.side == lqp::BID; v.push_back(l);
    }
    std::sort(v.begin(), v.end(), [](const L& a, const L& b) { return a.bar != b.bar ? a.bar < b.bar : a.rank < b.rank; });
    FONT f; f.id = HELVETICA; f.size = coord(S.font + 1); f.style = BOLD; setFont(f);
    int prevBar = -1, n = 0;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i].bar != prevBar) { prevBar = v[i].bar; n = 0; }
        if (n >= 2) continue;                                            // at most two letters a bar
        PNT p; p.set(v[i].bar, (float)(v[i].below ? low[v[i].bar] : high[v[i].bar]), kBarCenter);
        const int x = p.h; if (x < pane.left + 4 || x > pane.right - 4) { ++n; continue; }
        const int gap = 2 * (S.font + 4);                                // room for the Delta Profile's A circle next to the bar
        const int y = v[i].below ? p.v + gap + n * (S.font + 4) : p.v - gap - (n + 1) * (S.font + 4);
        if (y < pane.top || y + S.font > pane.bottom) { ++n; continue; }
        setTextColor(v[i].c); const int w = getTextWidth(v[i].txt.c_str(), -1);
        RCT r; r.set(coord(x - w / 2 - 1), coord(y), coord(x + w / 2 + 2), coord(y + S.font + 4)); r.drawText(v[i].txt.c_str(), true, false);
        ++n;
    }
}
// (0.7.0) the warning for the zone price is in now: name in its colour (green = good for buyers, red = good for sellers, grey = not
// decided) with a border, the plain words after it; top left of the price area, never past the last bar
void LiquidityProfile::renderWarning(LQState& S) {
    lqp::Warn w; if (!S.e.warning(S.e.bookT, w, S.e.P.tick)) return;
    const long count = getBarCount(); if (count < 2) return;
    RCT pane; pane.getPaneRect(false);
    PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int right = std::min((int)pl.h, (int)pane.right) - 4;
    const int x0 = pane.left + 6, y0 = pane.top + 4;
    if (right - x0 < 120) return;
    const COLOR c = w.favour > 0 ? C_BUY : w.favour < 0 ? C_SELL : C_GREY;
    FONT f; f.id = HELVETICA; f.size = coord(S.font + 2); f.style = BOLD; setFont(f);
    const int nw = getTextWidth(w.name.c_str(), -1);
    setPen(c, 1, P_SOLID); CBRUSH hb(c, PAT_HOLLOW); hb.set();
    RCT b; b.set(coord(x0), coord(y0), coord(x0 + nw + 8), coord(y0 + S.font + 9)); b.draw(1, c, c, DRAW_OPAQUE, PAT_HOLLOW);
    setTextColor(c); RCT t; t.set(coord(x0 + 4), coord(y0 + 2), coord(x0 + nw + 6), coord(y0 + S.font + 8)); t.drawText(w.name.c_str(), false, false);
    FONT g; g.id = HELVETICA; g.size = coord(S.font + 1); g.style = PLAIN; setFont(g);
    std::string d = w.detail; const int room = right - (x0 + nw + 16);
    while (!d.empty() && getTextWidth(d.c_str(), -1) > room) d.resize(d.size() > 4 ? d.size() - 4 : 0);   // never past the last bar
    if (!d.empty() && d != w.detail) d += "...";
    setTextColor(C_GREY); RCT r; r.set(coord(x0 + nw + 14), coord(y0 + 2), coord(right), coord(y0 + S.font + 8)); r.drawText(d.c_str(), false, false);
}
void LiquidityProfile::renderFlow(LQState& S) {
    auto flowLine = [this](const std::vector<std::pair<int, int>>& pts, COLOR c) {
        if (pts.empty()) return;
        setPen(c, 2, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = coord(pts[0].first); a.v = coord(pts[0].second); a.setDrawPosition();
        if (pts.size() == 1) { PNT b; b.set(0, 0.0f); b.h = coord(pts[0].first + 2); b.v = coord(pts[0].second); b.drawLineTo(); return; }
        for (size_t i = 1; i < pts.size(); ++i) { PNT b; b.set(0, 0.0f); b.h = coord(pts[i].first); b.v = coord(pts[i].second); b.drawLineTo(); }
    };
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 60 || pane.right - pane.left < 80) return;   // #35
    const int lastBar = (int)count - 1;
    const int mid = (pane.top + pane.bottom) / 2;
    const int t0 = pane.top + 14, t1 = mid - 3, b0 = mid + 14, b1 = pane.bottom - 3, zeroY = (b0 + b1) / 2;
    FONT f; f.id = HELVETICA; f.size = coord(std::max(7, S.font - 1)); f.style = BOLD; setFont(f); setTextColor(C_GREY);
    // (0.3.0, Rassul 2026-10-10) green = buyers and red = sellers in BOTH halves, at support and resistance alike
    { RCT r; r.set(coord(pane.left + 4), coord(pane.top + 1), coord(pane.left + 400), coord(pane.top + 13)); r.drawText("AGGRESSIVE", false, false); }
    { RCT r; r.set(coord(pane.left + 4), coord(mid + 1), coord(pane.left + 400), coord(mid + 13)); r.drawText("PASSIVE", false, false); }
    setPen(C_AXIS, 1, P_SOLID);
    { PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(mid); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(mid); b.drawLineTo(); }
    { PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(zeroY); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(zeroY); b.drawLineTo(); }
    if (!S.e.haveBook) { setTextColor(C_SELL); RCT r; r.set(coord(pane.left + 330), coord(pane.top + 1), coord(pane.left + 560), coord(pane.top + 13)); r.drawText(S.mdBlocked ? "blocked" : "no depth", false, false); }
    // the attempts to draw: the finished ones and the live one, only bars on screen
    std::vector<const lqp::Attempt*> as;
    for (size_t i = 0; i < S.e.finished.size(); ++i) as.push_back(&S.e.finished[i]);
    for (size_t i = 0; i < S.e.attempts.size(); ++i) as.push_back(&S.e.attempts[i]);
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    // per attempt: cumulative buyers / sellers aggression and the two net resting lines, one point per bar (values carried forward)
    struct Pt { int x; double buy, sell, bNet, sNet; };
    std::vector<std::vector<Pt>> series(as.size());
    double maxA = 1, maxD = 1;
    for (size_t k = 0; k < as.size(); ++k) {
        const lqp::Attempt& a = *as[k];
        if (a.flowBars.empty()) continue;
        const bool sup = a.side == lqp::BID;
        double ca = 0, cd = 0, dn = 0, on = 0;
        for (int b = a.barA; b <= lastBar && b <= a.barA + 40; ++b) {
            std::map<int, lqp::Attempt::BarFlow>::const_iterator it = a.flowBars.find(b);
            if (a.done && it == a.flowBars.end() && b > a.flowBars.rbegin()->first) break;
            if (it != a.flowBars.end()) { ca += it->second.attack; cd += it->second.defend; if (it->second.book) { dn = it->second.defNet; on = it->second.oppNet; } }
            const int x = xOf(b); if (x < pane.left || x > pane.right) continue;
            Pt p; p.x = x;
            p.buy = sup ? cd : ca; p.sell = sup ? ca : cd;               // support: sellers attack, buyers defend; resistance: the reverse
            p.bNet = sup ? dn : on; p.sNet = sup ? on : dn;              // buyers' bids / sellers' offers
            series[k].push_back(p);
            maxA = std::max(maxA, std::max(p.buy, p.sell)); maxD = std::max(maxD, std::max(std::fabs(p.bNet), std::fabs(p.sNet)));
        }
    }
    auto yA = [&](double v) { return t1 - (int)((t1 - t0) * std::min(1.0, v / maxA)); };
    auto yD = [&](double v) { return zeroY - (int)((zeroY - b0) * std::max(-1.0, std::min(1.0, v / maxD))); };
    std::vector<int> used;                                               // one story letter per bar per half (#14)
    for (size_t k = 0; k < as.size(); ++k) {
        const lqp::Attempt& a = *as[k];
        const bool sup = a.side == lqp::BID;
        std::vector<std::pair<int, int>> buy, sell, bN, sN;
        for (size_t i = 0; i < series[k].size(); ++i) {
            const Pt& p = series[k][i];
            buy.push_back(std::make_pair(p.x, yA(p.buy))); sell.push_back(std::make_pair(p.x, yA(p.sell)));
            bN.push_back(std::make_pair(p.x, yD(p.bNet))); sN.push_back(std::make_pair(p.x, yD(p.sNet)));
        }
        flowLine(sell, C_SELL); flowLine(buy, C_BUY); flowLine(sN, C_SELL); flowLine(bN, C_BUY);
        (void)sup;
    }
    // (0.6.0, Rassul 2026-10-11) the statistics on the LEFT of each half, in the order the process happens, Metric: now (line):
    // AGGRESSION Attack / Eaten / Exh, DEFENSE Pulled / Rebuilt - of the zone price is in (or the last one). A statistic is drawn only
    // when this market's line came from the nightly data; now is in the defenders' colour on the safe side of the line, the attackers'
    // colour past it. Eaten is a % of everything that was there; the rest are x ratios.
    {
        lqp::Stats st; bool have = S.e.latest(S.e.bookT, st); std::string asOf;
        // (0.7.0, Rassul 2026-10-11 "when i use my mouse and go to different places on the chart where there was absorption ...") the
        // mouse over a past bar of a touch shows that touch's statistics AS OF that bar (kept for this session)
        if (S.hoverBar >= 0) {
            const lqp::Attempt* hit = nullptr;
            for (size_t i = 0; i < S.e.finished.size() && !hit; ++i) if (S.e.finished[i].barStats.count(S.hoverBar)) hit = &S.e.finished[i];
            for (size_t i = 0; i < S.e.attempts.size() && !hit; ++i) if (S.e.attempts[i].barStats.count(S.hoverBar)) hit = &S.e.attempts[i];
            if (hit) {
                st = hit->barStats.find(S.hoverBar)->second; have = true;
                RTARRAYI tm(barDateTime);
                if (tm.count > S.hoverBar) { struct tm t; std::memset(&t, 0, sizeof(t));
                    if (getLocaltime((RTDATE)tm[S.hoverBar], &t)) { char b[32]; const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
                        std::snprintf(b, sizeof(b), "at %d:%02d %s", h12, t.tm_min, t.tm_hour < 12 ? "AM" : "PM"); asOf = b; } }
            }
        }
        if (!asOf.empty()) { setTextColor(C_GREY); RCT r; r.set(coord(pane.left + 120), coord(pane.top + 1), coord(pane.left + 260), coord(pane.top + 13)); r.drawText(asOf.c_str(), false, false); }
        if (have) {
            const COLOR def = st.side == lqp::BID ? C_BUY : C_SELL, att = st.side == lqp::BID ? C_SELL : C_BUY;
            struct It { const char* name; double now; LQState::Line line; bool pct, higherSafe; };
            // (0.8.0, Rassul 2026-10-11) AGGRESSIVE = market orders: Attack -> Exh -> Counter; PASSIVE = resting orders: Absorbed (the % of
            // everything resting that took fills) -> Pulled -> Rebuilt. Counter is the defenders' own aggression: their colour when at the line
            const It agg[3] = { {"Attack", st.attack, S.lAttack, false, false}, {"Exh", st.exh, S.lExh, false, false}, {"Counter", st.counter, S.lCounter, false, true} };
            const It dfn[3] = { {"Absorbed", st.eaten, S.lEaten, true, false}, {"Pulled", st.pulled, S.lPulled, false, false}, {"Rebuilt", st.rebuilt, S.lRebuilt, false, true} };
            FONT sf; sf.id = HELVETICA; sf.size = coord(S.font); sf.style = BOLD; setFont(sf);
            auto fmt = [](double v, bool pct) { char b[24]; if (v < 0) std::snprintf(b, sizeof(b), "-"); else if (pct) std::snprintf(b, sizeof(b), "%.0f%%", v * 100.0); else std::snprintf(b, sizeof(b), "%.2gx", v); return std::string(b); };
            auto column = [&](const It* it, int n, int y, int yMax) {
                for (int k = 0; k < n; ++k) {
                    if (it[k].line.v <= 0) continue;
                    if (y + S.font + 2 > yMax) break;
                    const bool safe = it[k].now < 0 ? true : (it[k].higherSafe ? it[k].now >= it[k].line.v : it[k].now < it[k].line.v);
                    const std::string a1 = std::string(it[k].name) + ": ", a2 = fmt(it[k].now, it[k].pct), a3 = " (" + fmt(it[k].line.v, it[k].pct) + ")";
                    int x = pane.left + 4;
                    setTextColor(C_GREY); int w = getTextWidth(a1.c_str(), -1); { RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + S.font + 3)); r.drawText(a1.c_str(), false, false); } x += w;
                    setTextColor(it[k].now < 0 ? C_GREY : (safe ? def : att)); w = getTextWidth(a2.c_str(), -1); { RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + S.font + 3)); r.drawText(a2.c_str(), false, false); } x += w;
                    setTextColor(C_GREY); w = getTextWidth(a3.c_str(), -1); { RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + S.font + 3)); r.drawText(a3.c_str(), false, false); }
                    y += S.font + 4;
                }
            };
            column(agg, 3, pane.top + 15, t1);
            column(dfn, 3, mid + 15, b1);
        }
    }
    // (0.5.0) the two pane signals, from the session record (so a restart redraws exactly what was shown): Exh in the top half, Pull in
    // the bottom half, at the bar they were decided on; green = buyers win, red = sellers win. Drawn ONLY where the nightly study confirmed
    // that signal for this market (SIGNAL lines in LRA-LiqNorms-<MKT>.csv) - Rassul 2026-10-11 "remove whatever doesn't work and keep
    // what works": an unconfirmed signal is not drawn at all (no "?")
    RTARRAYI times(barDateTime);
    if (times.count < count) return;
    std::map<int, std::vector<const lqp::Mark*>> at;                     // key = bar * 2 + half
    for (size_t i = 0; i < S.e.marks.size(); ++i) {
        const lqp::Mark& m = S.e.marks[i];
        if (!shown(S, m) || (m.code != "Exh" && m.code != "Pull")) continue;     // (0.7.0) e? p? t are price-chart letters; reasons below
        const int bar = barOfTime(m.barTime, count); if (bar < 0 || bar > lastBar) continue;
        std::vector<const lqp::Mark*>& v = at[bar * 2 + (lqp::Engine::topPane(m.code) ? 0 : 1)];
        bool dup = false; for (size_t k = 0; k < v.size(); ++k) if (v[k]->code == m.code) dup = true;
        if (!dup && v.size() < 2) v.push_back(&m);
    }
    FONT g; g.id = HELVETICA; g.size = coord(S.font); g.style = BOLD; setFont(g);
    for (std::map<int, std::vector<const lqp::Mark*>>::const_iterator it = at.begin(); it != at.end(); ++it) {
        const int bar = it->first / 2; const bool top = it->first % 2 == 0;
        const int x = xOf(bar); if (x < pane.left + 10 || x > pane.right - 10) continue;
        int y = top ? t0 : b0;
        for (size_t k = 0; k < it->second.size(); ++k) {
            const lqp::Mark& m = *it->second[k];
            // the number: Exh 0.3x (weakness), Pull 72%
            char num[24] = "";
            if (m.value >= 0) { if (m.code == "Pull") std::snprintf(num, sizeof(num), " %.0f%%", m.value * 100.0); else std::snprintf(num, sizeof(num), " %.1fx", m.value); }
            const std::string t = m.code + num;
            setTextColor(lqp::Engine::bullish(m) ? C_BUY : C_SELL);
            const int w = getTextWidth(t.c_str(), -1);
            RCT r; r.set(coord(x - w / 2 - 1), coord(y - 1), coord(x + w / 2 + 1), coord(y + S.font + 3)); r.drawText(t.c_str(), true, false);
            y += S.font + 4;
        }
    }
    // (0.7.0, Rassul 2026-10-11) the reason in plain words at the bar each touch was decided (Absorbed / Broke), from the record: scroll
    // back and every past one is still there; a thin line marks the bar; split into two lines at "; " when it does not fit
    for (size_t i = 0; i < S.e.marks.size(); ++i) {
        const lqp::Mark& m = S.e.marks[i];
        if ((m.code != "Held" && m.code != "Broke" && m.code != "s") || !shown(S, m) || m.reasons.empty()) continue;
        const int bar = barOfTime(m.barTime, count); if (bar < 0 || bar > lastBar) continue;
        const int x = xOf(bar); if (x < pane.left + 10 || x > pane.right - 40) continue;
        const COLOR c = lqp::Engine::bullish(m) ? C_BUY : C_SELL;
        setPen(c, 1, P_SOLID);
        { PNT a; a.set(0, 0.0f); a.h = coord(x); a.v = coord(pane.top + 2); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(x); b.v = coord(pane.bottom - 2); b.drawLineTo(); }
        std::vector<std::string> lines; const int room = pane.right - 4 - (x + 4);
        if (getTextWidth(m.reasons.c_str(), -1) <= room) lines.push_back(m.reasons);
        else { const size_t k = m.reasons.find("; "); if (k == std::string::npos) lines.push_back(m.reasons); else { lines.push_back(m.reasons.substr(0, k + 1)); lines.push_back("  " + m.reasons.substr(k + 2)); } }
        setTextColor(c);
        int y = pane.top + 1;
        for (size_t k = 0; k < lines.size(); ++k) { RCT r; r.set(coord(x + 4), coord(y), coord(pane.right - 4), coord(y + S.font + 3)); r.drawText(lines[k].c_str(), false, false); y += S.font + 4; }
    }
    (void)used;
}

// (0.7.0) the mouse over a bar (the Flow pane follows it to show that bar's statistics of a past touch)
int LiquidityProfile::mouse(RTX_EVENT* e) {
#ifdef LQ_FLOW
    if (!e) return RTX_FAIL;
    LQState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (e->type == E_MOUSE_MOVE || e->type == E_MOUSE_HOVER) { const long b = e->v.mouse.bar; S->hoverBar = (b >= 0 && b < getBarCount() - 1) ? (int)b : -1; }
    return RTX_FAIL;                                                     // never swallow the event: the chart keeps its own mouse behaviour
#else
    (void)e; return RTX_FAIL;
#endif
}

extern "C" cppExtension* CreateExtension(void) {
    LiquidityProfile* p = new LiquidityProfile();
    p->setArrayCount(1);
#ifdef LQ_FLOW
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);                    // its own pane (no OVERLAY), draws itself
    p->setExtendedFlags(CALL_CONTINUOUSLY);
    p->setDescription("LRA Liquidity Flow: the absorption process at the zone price is in. AGGRESSIVE (market orders): buyers (green) and sellers (red) trading into the zone - Attack, Exh (the attackers ran out), Counter (the defenders stepped in). PASSIVE (resting orders): buyers' bids (green) and sellers' offers (red), net added since arrival - Absorbed, Pulled, Rebuilt. Each reads Metric: now (line), the line set per market by the nightly study.");
#else
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the book keeps moving while no trade prints (#36)
    p->setDescription("LRA Liquidity Profile (LIQ): resting orders by price pointing toward price, top 3 zones per side with lots and x normal (solid = rested, hollow = new); old 64% + outline = half+ of the zone's size has rested 10 min+ (only in markets where the study confirmed it). The signals are on the Liquidity Flow panes. TESTING.");
#endif
    p->setVersion("0.10.0");   // LQ_VERSION
    return p;
}
