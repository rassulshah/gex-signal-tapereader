/********************************************************************************
 *  IRTReader.cpp  --  Investor/RT RTX extension  lsIRTReader  (v0.2.1, 2026-10-03: the chart's own market only, crash guard)
 *
 *  0.6.0 (2026-10-05 14:45, Rassul: "I think I have more data because I have full depth so can you see if you can view full
 *     depth"): DEPTH PROBE + RECORDING on the chart's own market. DOM on: the whole book the feed gives (md.maxLevels, up to 200
 *     levels - was capped at 16), a snapshot whenever it changed, at most once a second -> dom.csv. Order-by-order (MBO) on for
 *     the first 10 minutes after IRT starts (dbo.csv - shows whether the feed carries every order: NEW / MOD / DEL / FILL with
 *     order ids, i.e. icebergs and pulled orders); after that the events are still read and counted, not written. The status
 *     file says DOM available / levels and DBO events, so we can see what depth the feed really carries. Trades stay off.
 *  0.5.2 (2026-10-05 00:33): IRT does not grow a footprint request live - each market wrote in ~10-minute chunks (NQ stuck
 *     at 00:23 until 00:33). The request is now remade after 90 s without a new bar (it only asks from the last written bar,
 *     a few minutes of data), so the footprint is at most ~2 minutes behind; a chart switched to another contract month starts
 *     over on the new contract (CL was left asking IRT for CLEZ26 after the chart moved to CLEX26)
 *  0.5.1 (2026-10-05 00:08, Rassul: "rename all to FootprintReader<market> without any options to select"): the per-market
 *     DLLs are FootprintReaderES.dll ... FootprintReaderEU.dll (FootprintReader<MKT>.cpp) and have NO settings at all
 *  0.5.0 (2026-10-05 00:05) ONE DLL PER MARKET: 0.4.0 made the requests from draw / calc and IRT hung inside the very first one
 *     (ES, 23:52 - "footprint..." and nothing after, _trying-ES-fp left behind). Requests only ever worked from the TIMER, and the
 *     timer works for one chart per DLL. So each market gets its own DLL - lsIRTReaderES, lsIRTReaderNQ, ... (IRTReader<MKT>.cpp
 *     = this file with IR_FIXED set) - its own object, its own timer, its own chart: the setup that recorded NQ / GC / HG / EU.
 *     calc / draw only start the timer; every request is made from the timer (draw runs the pass only if no timer tick comes).
 *  0.4.0 (2026-10-04 23:50) THE ROOT CAUSE: Investor/RT runs ONE reader object for every chart it is on (the 23:35 boot trace:
 *     one id, 53a620, called on the ES chart, then the NQ chart, then ES again). The reader kept ONE market's state, so whichever
 *     chart came first "owned" it and the others never recorded; IRT's timer always calls in one chart's context.
 *     Now: the market is read from the chart that calls (draw / calc), each market keeps its own state (cursor, bars, history step),
 *     each market gets at most one pass a second, from its own chart only (a request for a market is made only while IRT is
 *     drawing that market's chart). No timer. New requests are spaced 60 s apart across all markets (one at a time).
 *  0.3.2 (2026-10-04 22:12): on 0.3.0 only the NQ chart's reader ever ran - the ES chart showed "Calc'd 40 times" but its calc
 *     never reached this code (no boot line). The Dealer Profile, which works on every chart, does its work in draw() (POST_DRAWING).
 *     The reader now does the same: draw(), calc() and the timer all drive one pass (at most once a second); it draws nothing.
 *  0.3.1 (2026-10-04 22:08, Rassul: "keep it small"): history for a market is ONE 3-day step and nothing more (no 10 / 120-day
 *     requests - IRT keeps only ~3 weeks of 1-min volume at price anyway); after that each day builds up from live recording
 *  0.3.0 (2026-10-04 21:40, Rassul: "lets get footprint working", "keep the time and sales and dom out of it for now"):
 *     - EACH CHART RECORDS ITS OWN MARKET ONLY: asking IRT for another market's footprint by ticker crashed IRT three times
 *       (ES, CL, then NQ from the copper chart at 16:10 on 4 Oct); a chart's own market never did. _other-markets.txt is ignored.
 *     - trades / DOM / order-by-order are OFF whatever the dialog says (footprint only for now)
 *     - the reader no longer depends on IRT's timer alone: on 4 Oct 21:30 the ES and NQ readers were calculated (29 times) but
 *       never wrote a line - no timer fired. If no timer tick arrives within 10 s, the pass runs from calc, once a second.
 *     - a boot trace for every reader (IRTReader.trace-boot.txt, written even before the market is known): chart, root, timer
 *     - the first run asks for 1 day (not 11), then history in guarded steps of 3, 10 and 120 days; a back-fill never writes
 *       the oldest, partial session when the answer spans more than one session
 *  0.2.2 (2026-10-03 12:50, Rassul: "start the other markets with the crash guard ... a few minutes after i open the application
 *     ... when i open irt there are already downloads and initializations going on, so it needs to stabilize first"):
 *     - other markets (data\irt\_other-markets.txt) wait 5 minutes after the reader starts, then go ONE heavy request at a
 *       time, at least 2 minutes apart: first a 30-minute live request (proves the ticker is safe), then history in steps of
 *       1, 3 and 10 days - each step its own guarded request (_trying-<MKT>-deep1 / deep3 / deep10), so a step that kills IRT
 *       is blocked and the market keeps what the smaller steps fetched. No 120-day back-fill for other markets.
 *     - every back-fill skips the OLDEST session it gets (the request starts mid-session, so that session is partial); the
 *       next, longer request writes it whole
 *  0.2.1 (2026-10-03 12:10, Rassul: "my linnsoft applications keeps crashing and restarting"): 0.2.0 crashed Investor/RT about
 *     20 s after every start - IRT died INSIDE the RTBARS request for another market's footprint (ES, then CL: 11 days of
 *     1-min volume at price by ticker; the chart's own NQ request was fine). Now:
 *     - only the chart's own market is recorded, unless data\irt\_other-markets.txt lists more (one market code per line);
 *       another market starts with 30 minutes, never a 10-day back-fill, and gets no 120-day deep back-fill
 *     - crash guard: every RTBARS request leaves data\irt\_trying-<MKT>-<fp|deep>.txt until IRT has survived it; if a
 *       restart finds that file, the request killed IRT last time - it is blocked (_blocked-<MKT>-<kind>.txt, traced) and
 *       never asked again until the file is deleted. One bad request can no longer put IRT in a restart loop.
 *     - the deep back-fill is DONE only when IRT returned bars from at least 100 days back; a short answer (a weekend: 0 rows,
 *       12:04 today) is retried every 2 h, at most 3 times per IRT run
 *
 *  THE IRT READER: records the chart market's order flow for the LRA analytics - draws nothing on the chart.
 *  (Rassul 2026-10-02 19:31-20:08: "read my footprint, dom, time and sales"; "could i just keep it on a chart without having
 *  footprint and dom open?"; "start building the irt reader indicator ... get the footprint charts for the markets i trade
 *  and start backtesting them".)
 *
 *  Put it on ONE chart per market (any timeframe; the 3-min chart he trades is fine). Every second (a timer) it:
 *     footprint  writes every finished N-minute bar (setting, default 1 min) with its volume at each price, bought at the ask
 *                / sold at the bid, trades and delta swings - from IRT's volume-at-price data, no footprint chart needed;
 *                the first run back-fills the last D days (setting) so the backtest has history
 *     trades     appends every new trade (time, price, size, bid, ask, aggressor B / S / M) - from IRT's tick data, no time &
 *                sales window needed; back-fills the current session on the first run
 *     dom        a snapshot of the top bid / ask levels and sizes whenever the book changes (at most every N s)
 *     dbo        order-by-order depth events (new / modify / delete / fill, with the aggressor) if the data feed carries them
 *  Files: <folder>\<session>\<MKT>\fp_1m.csv, trades.csv, dom.csv, dbo.csv ('|' separated, header first; the session is
 *  the futures day, 17:00 CT on = the next weekday). Restarts resume where the files end (a cursor file per market).
 *  Status (what is recording, rows written, depth available or not): %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\
 *  IRTReader.status-<MKT>.txt - read by Claude over the bridge.
 *  Logic that needs no SDK (sessions, aggressor, cursor, book fingerprint) is in IRTReaderLogic.h (tested).
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include "IRTReaderLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cctype>
#include <direct.h>

static const char* IR_VERSION = "0.6.0";
#ifndef IR_FIXED
#define IR_FIXED ""                // (0.5.0) lsIRTReader<MKT>.dll is built from IRTReader<MKT>.cpp with IR_FIXED = that market
#endif
// (0.1.4, 2 Oct 23:33: only the first chart's reader ever ran - every other market wrote nothing, not even a trace) each reader
// gets its own timer id: the same id on every chart most likely made IRT refuse the second and later timers
static int timerIdFor(const void* me) { return 4711 + (int)(((uintptr_t)me >> 4) % 100000); }

struct RIdx { int market, fp, fpMin, fpDays, trades, dom, domSec, dbo, folder; };
static RIdx RX;
struct RSet { int market = 0, fpMin = 1, fpDays = 10, domSec = 2; bool fp = true, trades = false, dom = true, dbo = true;   // (0.6.1) depth ON by default: the settings dialog is not always read, so 0.6.0 never probed (status said DOM off) std::string folder = "C:\\Dev\\level-reversal-analytics\\data\\irt"; };

// (0.2.0, 3 Oct 00:05: after a restart only ONE chart's reader is ever started by IRT - the NQ chart's; the readers pasted on the
// other charts never ran, not even calc) ONE reader records the FOOTPRINT of every market in irt_symbols.json (RTBARS takes a
// ticker); trades / DOM / order-by-order stay the chart's own market (RTTICKS has no ticker). A market is written by one
// reader only: _owner-<MKT>.txt holds the writer's id and a heartbeat (taken over when silent for 90 s).
struct FpState {
    std::string mkt, sym; double tick = 0.01;
    long long fpLast = -1, deepLast = -1; int deepDone = 0; bool loaded = false;
    cppExtension::RTBARS* fpBars = nullptr; time_t fpMade = 0, fpGrew = 0, fpT = 0; long fpCount = -1;
    cppExtension::RTBARS* deepBars = nullptr; time_t deepMade = 0, deepGrew = 0; long deepCount = -1;
    std::set<std::string> deepOwn, deepSkip;
    long rows = 0; std::string lastTxt, err; int caughtUp = 0;
    bool own = false;                                    // (0.2.1) the chart's own market
    bool fpBlocked = false, deepBlocked = false;         // (0.2.1) a request of this kind crashed IRT before
    std::string fpTry, deepTry;                          // (0.2.1) the guard file while a request is unproven
    time_t deepRetryAt = 0; int deepTries = 0;
    int deepStep = 0;                                    // (0.2.2) other markets: 0 = 1 day, 1 = 3 days, 2 = 10 days, 3 = done
};
static double tickOf(const std::string& m)
{
    if (m == "ES" || m == "NQ") return 0.25; if (m == "GC") return 0.1; if (m == "CL") return 0.01; if (m == "HG") return 0.0005;
    if (m == "NG") return 0.001; if (m == "EU") return 0.00005; return 0.01;
}

class IRTReader : public cppExtension {
public:
    IRTReader() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int timer(RTX_EVENT* e);
    virtual int draw(void);
    void pump(const char* via);

    RSet cfg;
    std::string mkt, root, sym, err;
    double tick = 0;
    bool timerOn = false, backfilled = false;
    long long fpLast = -1;                       // the last footprint bar written (local seconds since 1970)
    irl::TickCursor tc;
    unsigned long long domHash = 0; time_t domT = 0;
    bool domAvail = false; int domLevels = 0;
    long fpRows = 0, trRows = 0, domRows = 0, dboRows = 0;
    std::string fpLastTxt, trLastTxt, cursorFor;
    time_t fpT = 0;
    DEPTH_BY_ORDER* dbo = nullptr;
    // (0.1.2) the footprint bars are kept alive (persistent) so IRT can finish loading their volume at price; a bar whose
    // statistics are not there yet stops the pass and is retried on the next tick (0.1.1 wrote nothing on 2 Oct 22:24: the
    // folders were made, no rows - getBarStatistics failed on every bar of a fresh, non-persistent RTBARS)
    RTBARS* fpBars = nullptr; time_t fpMade = 0; long fpCount = -1; time_t fpGrew = 0;
    // (0.1.5, Rassul 23:53 "an optimization problem ... identifying them for each market with high probability") a DEEP back-fill
    // of up to 120 days, once, after the normal pass is live: more days is what makes per-market tuning trustworthy. It only
    // fills sessions that have no footprint file yet (never touches what the normal pass wrote).
    RTBARS* deepBars = nullptr; long long deepLast = -1; int deepDone = 0; time_t deepMade = 0; long deepCount = -1; time_t deepGrew = 0;
    std::set<std::string> deepOwn, deepSkip;
    int writeSlice(FpState& M, RTBARS& bars, long long& last, bool deep, int maxBars);
    void deepFill(FpState& M);
    std::vector<FpState> fps; bool fpsLoaded = false; std::string myId;
    void loadMarkets();
    std::string guardPath(const std::string& m, const char* kind, const char* what);
    bool guardBefore(FpState& M, const char* kind);
    void guardClear(FpState& M, const char* kind);
    std::string othersTxt;
    time_t startT = 0, nextOtherAt = 0;                    // (0.2.2) the reader start; the next heavy request allowed for other markets
    bool blockedKind(FpState& M, const char* kind);
    bool own(FpState& M);
    void footprintOf(FpState& M);
    void loadFpCursor(FpState& M);
    void saveFpCursor(FpState& M);
    void writeFpStatus(FpState& M);
    std::string dirFor2(const std::string& session, const std::string& m);
    bool busy = false, timerFailLogged = false, calcLogged = false; int ticks = 0;
    time_t timerAt = 0, lastTimerTick = 0, lastCalcRun = 0; int boots = 0;   // (0.3.0) calc fallback when no timer tick arrives
    void boot(const std::string& what);
    std::map<std::string, time_t> lastPass; std::set<std::string> booted;   // (0.4.0) per market (= per chart)
    void trace(const std::string& what);

    bool dialogReady() { if (RX.market < 0) return false; int i = getListIndex(RX.market); return i >= 0 && i <= 7; }
    void readSettings(RSet& S);
    void identify();
    void tickAll();
    void footprint();
    void trades();
    void depth();
    void depthByOrder();
    void loadCursor();
    void saveCursor();
    void writeStatus();
    std::string dirFor(const std::string& session);
    long long secOf(RTDATE d, struct tm* out = nullptr);
    std::string sessionOfDt(RTDATE d);
    void append(const std::string& path, const char* header, const std::string& rows);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void)
{
    IRTReader* me = static_cast<IRTReader*>(this);
    if (me->timerOn) { destroyTimer(timerIdFor(me)); me->timerOn = false; }
    if (me->dbo) { delete me->dbo; me->dbo = nullptr; }
    for (auto& M : me->fps) {
        if (M.fpBars) delete M.fpBars; if (M.deepBars) delete M.deepBars; M.fpBars = M.deepBars = nullptr;
        me->guardClear(M, "fp"); me->guardClear(M, "deep");      // (0.2.1) a normal close is not a crash
    }
    return RTX_OK;
}
int cppExtension::calc(int)
{
    static_cast<IRTReader*>(this)->pump("calc");
    return RTX_OK;
}

// (0.3.2) the one driver: whichever of draw / calc / timer comes first; a pass at most once a second
void IRTReader::pump(const char* via)
{
    time_t now = time(0);
    if (!calcLogged) { identify(); boot(std::string(via) + ": loaded, asking for a timer"); trace("loaded on " + sym + " (" + via + ") - " + IR_VERSION); calcLogged = true; }
    if (!timerOn && !timerFailLogged) {
        if (createTimer(timerIdFor(this), 1000) == RTX_OK) { timerOn = true; timerAt = now; boot("timer granted (id " + std::to_string(timerIdFor(this)) + ")"); }
        else { boot("timer REFUSED - running from draw / calc"); timerFailLogged = true; }
    }
    bool noTimer = !timerOn || (lastTimerTick == 0 ? now - timerAt > 20 : now - lastTimerTick > 60);
    if (noTimer && now != lastCalcRun) {
        if (lastCalcRun == 0) boot(std::string("no timer tick - the pass runs from ") + via);
        lastCalcRun = now; tickAll();
    }
}

int IRTReader::draw(void)
{
    pump("draw");
    return RTX_OK;
}

int IRTReader::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int IRTReader::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int IRTReader::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    if (IR_FIXED[0]) {                // (0.5.1) FootprintReader<MKT>: no settings - footprint only, its own market only
        RX.market = RX.fp = RX.fpMin = RX.fpDays = RX.trades = RX.dom = RX.domSec = RX.dbo = RX.folder = -1;
        setParameterVersion(4);
        return RTX_OK;
    }
    setParameterVersion(3);           // (0.1.2) "Footprint" and "Back-fill days" removed: always on, always 10 days
    setParameterDialogHeight(4);
    const short SL = kParmAppendSameLine;
    RX.market = getParameterCount(); setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    RX.fp     = -1;                   // (0.1.2, 22:23 the box came up unticked / ??? again) the footprint is always recorded
    // (0.1.1, Rassul 21:19 "??? for footprint bar ... shouldn't that default to the bar on the chart") no setting: the footprint
    // is always recorded on 1-min bars, whatever the chart shows - 3-min (or any) bars are built from them, never the reverse
    RX.fpMin  = -1;
    RX.fpDays = -1;                   // (0.1.2) always 10 days back
    RX.trades = getParameterCount(); setBoolParameter("Trades (time & sales)", true, SL);
    RX.dom    = getParameterCount(); setBoolParameter("DOM", true, SL);
    RX.domSec = getParameterCount(); setIntegerParameter("DOM every (s)", 2, 40, SL);
    RX.dbo    = getParameterCount(); setBoolParameter("Order-by-order depth", true);
    RX.folder = -1;                   // (0.1.1) always C:\Dev\level-reversal-analytics\data\irt
    return RTX_OK;
}

void IRTReader::readSettings(RSet& S)
{
    S.market = getListIndex(RX.market); if (S.market < 0 || S.market > 7) S.market = 0;
    S.fp = true; S.trades = false; S.dom = true; S.dbo = true;     // (0.6.0) depth on (Rassul 5 Oct 14:39 "full depth"); trades still off
    S.fpMin = 1;
    S.fpDays = 10;
    S.domSec = 1;                                                  // (0.6.0) every second when the book changed
}

void IRTReader::identify()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    const char* s = getSymbol(); sym = s ? s : "";
    mkt = dl::marketFor(cfg.market, root);
    if (IR_FIXED[0]) { if (!mkt.empty() && mkt != IR_FIXED) { mkt.clear(); return; } mkt = IR_FIXED; }   // (0.5.0) this DLL records ONE market: on another chart it does nothing
    tick = getTickIncrement();
}

long long IRTReader::secOf(RTDATE d, struct tm* out)
{
    struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(d, &t);
    if (out) *out = t;
    struct tm c = t; c.tm_isdst = -1;
    return (long long)mktime(&c);
}

std::string IRTReader::sessionOfDt(RTDATE d)
{
    struct tm t; secOf(d, &t);
    return irl::sessionOf(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour);
}

std::string IRTReader::dirFor(const std::string& session)
{
    std::string d = cfg.folder;
    _mkdir(d.c_str());
    d += "\\" + session; _mkdir(d.c_str());
    d += "\\" + mkt; _mkdir(d.c_str());
    return d;
}

void IRTReader::append(const std::string& path, const char* header, const std::string& rows)
{
    if (rows.empty()) return;
    bool fresh = false;
    { std::ifstream t(path.c_str()); fresh = !t.good(); }
    std::ofstream f(path.c_str(), std::ios::app);
    if (!f.is_open()) { err = "cannot write " + path; return; }
    if (fresh) f << header << "\n";
    f << rows;
}

void IRTReader::loadCursor()
{
    std::ifstream f((cfg.folder + "\\_cursor-" + mkt + ".txt").c_str());
    long long a = -1, b = -1; int c = 0, bf = 0;
    if (f >> a >> b >> c >> bf) { fpLast = a; tc.lastSec = b; tc.doneInSec = c; backfilled = bf != 0; }
    long long dl = -1; int dd = 0;
    if (f >> dl >> dd) {
        deepLast = dl; deepDone = dd;
        if (dl > 0) { time_t tt = (time_t)dl; struct tm lt = *localtime(&tt);      // the session it was in the middle of stays its own
            deepOwn.insert(irl::sessionOf(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour)); }
    }
}

void IRTReader::saveCursor()
{
    _mkdir(cfg.folder.c_str());
    std::ofstream f((cfg.folder + "\\_cursor-" + mkt + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << fpLast << " " << tc.lastSec << " " << tc.doneInSec << " " << (backfilled ? 1 : 0) << " " << deepLast << " " << deepDone << "\n";
}

// ---- (0.2.0) the markets to record: data\menthorq\irt_symbols.json {"ES": "EPZ26", ...} (edited at each roll)
void IRTReader::loadMarkets()
{
    fpsLoaded = true;
    if (myId.empty()) { char idb[40]; snprintf(idb, sizeof(idb), "%llx-%ld", (unsigned long long)(uintptr_t)this, (long)time(0)); myId = idb; }
    for (auto& X : fps) if (X.mkt == mkt) return;                 // (0.4.0) this chart's market is already known
    std::string path = "C:\\Dev\\level-reversal-analytics\\data\\menthorq\\irt_symbols.json";
    std::ifstream f(path.c_str()); std::stringstream ss; ss << f.rdbuf(); std::string js = ss.str();
    const char* MK[7] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
    // (0.2.1) other markets are recorded only when data\irt\_other-markets.txt names them (one code per line), never by default
    std::string others = " ";
    // (0.3.0) _other-markets.txt is ignored: another market by ticker crashed IRT (ES, CL, NQ) - each chart records its own market
    for (int k = 0; k < 7; k++) {
        std::string key = std::string("\"") + MK[k] + "\"";
        size_t p0 = js.find(key); std::string sy;
        if (p0 != std::string::npos) { size_t q1 = js.find('"', js.find(':', p0) + 1); size_t q2 = js.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) sy = js.substr(q1 + 1, q2 - q1 - 1); }
        bool isOwn = (MK[k] == mkt);
        if (!isOwn) continue;                                     // (0.4.0) only the calling chart's market
        if (isOwn) sy = sym;                                      // the chart's own market: its own symbol
        else if (others.find(std::string(" ") + MK[k] + " ") == std::string::npos) continue;   // (0.2.1) others only when listed
        if (sy.empty()) continue;
        FpState M; M.mkt = MK[k]; M.sym = sy; M.tick = (isOwn && tick > 0) ? tick : tickOf(MK[k]); M.own = isOwn;
        // (0.2.1) a request that was still unproven when IRT last died is blocked for good (until its file is deleted)
        const char* KIND[5] = { "fp", "deep", "deep1", "deep3", "deep10" };
        for (int q = 0; q < 5; q++) {
            std::ifstream tr(guardPath(M.mkt, KIND[q], "trying").c_str());
            if (tr.good()) {
                tr.close(); std::remove(guardPath(M.mkt, KIND[q], "trying").c_str());
                std::ofstream b(guardPath(M.mkt, KIND[q], "blocked").c_str(), std::ios::trunc);
                b << "the " << KIND[q] << " request for " << M.mkt << " (" << M.sym << ") was running when Investor/RT stopped - blocked\n";
                trace(M.mkt + " " + KIND[q] + ": IRT stopped during this request last time - BLOCKED (delete _blocked-" + M.mkt + "-" + KIND[q] + ".txt to retry)");
            }
            std::ifstream bl(guardPath(M.mkt, KIND[q], "blocked").c_str());
            if (bl.good()) { if (q == 0) M.fpBlocked = true; else if (q == 1) M.deepBlocked = true; }
        }
        fps.push_back(M);
    }
    othersTxt = others.size() > 2 ? others.substr(1, others.size() - 2) : "off";
    std::string list; for (auto& M : fps) list += M.mkt + "=" + M.sym + (M.fpBlocked ? "(fp blocked)" : "") + (M.deepBlocked ? "(deep blocked)" : "") + " ";
    trace("markets: " + list + "| other markets: " + othersTxt + (othersTxt != "off" ? " (start 5 min after the reader, one request at a time)" : ""));
}

std::string IRTReader::guardPath(const std::string& m, const char* kind, const char* what)
{
    return cfg.folder + "\\_" + what + "-" + m + "-" + kind + ".txt";
}

// (0.2.1) written right BEFORE an RTBARS request, removed once IRT has survived it (the next tick); found at a restart = it crashed IRT
bool IRTReader::blockedKind(FpState& M, const char* kind)
{
    std::ifstream b(guardPath(M.mkt, kind, "blocked").c_str());
    return b.good();
}

bool IRTReader::guardBefore(FpState& M, const char* kind)
{
    if (blockedKind(M, kind)) return false;
    _mkdir(cfg.folder.c_str());
    std::string p = guardPath(M.mkt, kind, "trying");
    { std::ofstream f(p.c_str(), std::ios::trunc); f << M.sym << " " << (long long)time(0) << "\n"; }
    if (kind[0] == 'f') M.fpTry = p; else M.deepTry = p;
    return true;
}

void IRTReader::guardClear(FpState& M, const char* kind)
{
    std::string& p = (kind[0] == 'f') ? M.fpTry : M.deepTry;
    if (!p.empty()) { std::remove(p.c_str()); p.clear(); }
}

bool IRTReader::own(FpState& M)
{
    std::string p = cfg.folder + "\\_owner-" + M.mkt + ".txt";
    std::string id; long long t = 0;
    { std::ifstream f(p.c_str()); f >> id >> t; }
    long long now = (long long)time(0);
    if (!id.empty() && id != myId && now - t < 90) return false;   // another live reader writes this market
    _mkdir(cfg.folder.c_str());
    std::ofstream o(p.c_str(), std::ios::trunc); o << myId << " " << now << "\n";
    return true;
}

std::string IRTReader::dirFor2(const std::string& session, const std::string& m)
{
    std::string d = cfg.folder; _mkdir(d.c_str());
    d += "\\" + session; _mkdir(d.c_str());
    d += "\\" + m; _mkdir(d.c_str());
    return d;
}

void IRTReader::loadFpCursor(FpState& M)
{
    M.loaded = true;
    long long a = -1, dl = -1; int dd = 0;
    std::ifstream f((cfg.folder + "\\_fp-" + M.mkt + ".txt").c_str());
    if (f >> a >> dl >> dd) { M.fpLast = a; M.deepLast = dl; M.deepDone = dd; int st = 0; if (f >> st) M.deepStep = st; }
    else {                                                        // first 0.2.0 run: take over 0.1.x's cursor
        std::ifstream g((cfg.folder + "\\_cursor-" + M.mkt + ".txt").c_str());
        long long b = -1; int c = 0, bf = 0;
        if (g >> a >> b >> c >> bf) { M.fpLast = a; if (g >> dl >> dd) { M.deepLast = dl; M.deepDone = dd; } }
    }
    if (M.deepLast > 0) { time_t tt = (time_t)M.deepLast; struct tm lt = *localtime(&tt);
        M.deepOwn.insert(irl::sessionOf(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour)); }
    trace(M.mkt + ": cursor fp " + std::to_string(M.fpLast) + " deep " + std::to_string(M.deepLast) + (M.deepDone ? " (deep done)" : ""));
}

void IRTReader::saveFpCursor(FpState& M)
{
    _mkdir(cfg.folder.c_str());
    std::ofstream f((cfg.folder + "\\_fp-" + M.mkt + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << M.fpLast << " " << M.deepLast << " " << M.deepDone << " " << M.deepStep << "\n";
}

// ---- footprint: finished 1-minute bars with volume at price (RTBARS with VAP, by ticker - independent of any chart)
void IRTReader::footprintOf(FpState& M)
{
    time_t nowT = time(0);
    if (M.fpBlocked) { M.err = "footprint request blocked: it crashed IRT before"; return; }
    if (!M.fpTry.empty() && M.fpBars) guardClear(M, "fp");          // IRT survived the last request
    if (!M.fpBars || (M.fpCount >= 0 && nowT - M.fpGrew > 90 && nowT - M.fpMade > 90)) {   // remade if it stops growing for 10 min
        if (M.fpBars) { delete M.fpBars; M.fpBars = nullptr; }
        RTDATE now = currentDate();
        RTDATE start = (M.fpLast > 0) ? (RTDATE)(now - (RTDATE)1800UL) : (RTDATE)(now - (RTDATE)86400UL);   // (0.3.0) first run: 1 day, history in steps
        if (M.fpLast > 0 && (long long)nowT - M.fpLast > 1800) start = (RTDATE)(now - (RTDATE)((nowT - M.fpLast) + 600));   // a gap: from it
        if (M.own) { if (nowT < nextOtherAt) return; nextOtherAt = nowT + 60; }   // (0.4.0) one new request at a time across all charts
        if (!M.own) {                                            // (0.2.1) another market: 30 minutes, never a long back-fill
            start = (RTDATE)(now - (RTDATE)1800UL);
            if (nowT < nextOtherAt) return;                      // (0.2.2) one heavy request at a time, 2 minutes apart
            nextOtherAt = nowT + 120;
        }
        if (!guardBefore(M, "fp")) return;
        M.fpBars = new RTBARS(PD_INTRA, cfg.fpMin, true, M.sym.c_str(), false, start, true);
        M.fpMade = nowT; M.fpCount = -1; M.fpGrew = nowT;
        trace(M.mkt + " footprint: bars asked" + (M.fpLast > 0 ? " (since the cursor)" : " (back-fill)"));
    }
    RTBARS& bars = *M.fpBars;
    if (bars.count != M.fpCount) { M.fpCount = bars.count; M.fpGrew = nowT; }
    if (bars.count < 2) { M.err = bars.count ? "" : "no footprint bars from IRT yet"; return; }
    M.err.clear();
    int n = writeSlice(M, bars, M.fpLast, false, 400);
    M.caughtUp = (n <= 3 && M.err.empty()) ? M.caughtUp + 1 : 0;   // (0.2.0) only the newest bar(s) to write = caught up (weekends too)
}

int IRTReader::writeSlice(FpState& M, RTBARS& bars, long long& lastRef, bool deep, int maxBars)
{
    int done = 0;
    std::string rows, sessCur;
    std::string barRows;
    long long lastWritten = lastRef;
    char fnm[32]; snprintf(fnm, sizeof(fnm), "\\fp_%dm.csv", cfg.fpMin);
    const double tk = M.tick;
    std::string oldestSess;                                      // (0.2.2) a back-fill starts mid-session: its oldest session is partial
    if ((deep || lastRef < 0) && bars.count > 1) {             // (0.3.0) also the first run; only when the answer spans 2+ sessions
        struct tm t0, t1; secOf((RTDATE)(*bars.dt)[0], &t0); secOf((RTDATE)(*bars.dt)[(int)bars.count - 1], &t1);
        oldestSess = irl::sessionOf(t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour);
        if (oldestSess == irl::sessionOf(t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour)) oldestSess.clear();
    }
    for (long i = 0; i < bars.count - 1; i++) {                 // the last bar is still forming
        RTDATE d = (RTDATE)(*bars.dt)[(int)i];
        struct tm t; long long s = secOf(d, &t);
        if (s <= lastRef) continue;
        std::string session = irl::sessionOf(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour);
        if (!oldestSess.empty() && session == oldestSess) { lastWritten = s; continue; }   // (0.2.2/0.3.0) partial - a longer request writes it
        if (deep) {                                              // only sessions with no file yet (or ones this pass began)   // (0.2.2) partial - the next, longer request writes it
            if (M.deepSkip.count(session)) { lastWritten = s; continue; }
            if (!M.deepOwn.count(session)) {
                std::ifstream ex((cfg.folder + "\\" + session + "\\" + M.mkt + fnm).c_str());
                if (ex.good()) { M.deepSkip.insert(session); if (M.own && s > M.fpLast - 86400) { M.deepDone = 1; break; } lastWritten = s; continue; }   // (0.2.2) other markets: skip and go on (their newer sessions may have gaps)
                M.deepOwn.insert(session);
            }
        }
        if (!sessCur.empty() && session != sessCur) {             // one file per session
            std::string dir = dirFor2(sessCur, M.mkt);
            append(dir + fnm, "bar|price|bought|sold|volume|trades|max_delta|min_delta", rows);
            append(dir + "\\fp_bars.csv", "bar|minutes|open|high|low|close|volume|bought|sold|max_delta|min_delta|open_delta|max_price_vol", barRows);
            rows.clear(); barRows.clear();
        }
        sessCur = session;
        std::string ts = irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        BARSTATISTICS bs; memset(&bs, 0, sizeof(bs));
        if (bars.getBarStatistics((int)i, bs) != RTX_OK) { M.err = "volume at price not loaded yet from " + ts; break; }   // retried next tick
        if (++done > maxBars) break;                             // a slice per tick
        {
            char b[300];
            snprintf(b, sizeof(b), "%s|%d|%s|%s|%s|%s|%ld|%ld|%ld|%ld|%ld|%ld|%ld\n", ts.c_str(), cfg.fpMin,
                     irl::px((*bars.op)[(int)i], tk).c_str(), irl::px((*bars.hi)[(int)i], tk).c_str(), irl::px((*bars.lo)[(int)i], tk).c_str(),
                     irl::px((*bars.cl)[(int)i], tk).c_str(), (long)(*bars.vo)[(int)i], bs.buyVolume, bs.sellVolume, bs.maxDelta, bs.minDelta,
                     bs.openDelta, bs.maxPriceVol);
            barRows += b;
            int np = bs.prices > 0 ? bs.prices : 400;
            for (int k = 0; k < np; k++) {
                VOLPROFILE vp; memset(&vp, 0, sizeof(vp));
                if (bars.getVolumeProfile((int)i, k, vp) != RTX_OK) break;
                if (vp.totalVolume <= 0 && vp.buyVolume <= 0 && vp.sellVolume <= 0) continue;
                snprintf(b, sizeof(b), "%s|%s|%ld|%ld|%ld|%ld|%ld|%ld\n", ts.c_str(), irl::px(vp.price, tk).c_str(), vp.buyVolume, vp.sellVolume,
                         vp.totalVolume, vp.tickCount, vp.maxDelta, vp.minDelta);
                rows += b; M.rows++; fpRows++;
            }
        }
        lastWritten = s; if (!deep) M.lastTxt = ts;
    }
    if (!sessCur.empty()) {
        std::string dir = dirFor2(sessCur, M.mkt);
        append(dir + fnm, "bar|price|bought|sold|volume|trades|max_delta|min_delta", rows);
        append(dir + "\\fp_bars.csv", "bar|minutes|open|high|low|close|volume|bought|sold|max_delta|min_delta|open_delta|max_price_vol", barRows);
    }
    lastRef = lastWritten;
    return done;
}

void IRTReader::deepFill(FpState& M)
{
    // (0.2.0) after the normal pass has nothing new for 3 passes in a row (0.1.5 waited for a bar under 10 min old, which never
    // comes on a weekend - the deep back-fill never started)
    if (M.deepDone) return;
    time_t nowT = time(0);
    static const char* DK[3] = { "deep1", "deep3", "deep10" };
    static const unsigned long DD[3] = { 1UL, 3UL, 10UL };
    static const char* OK3[3] = { "deep3", "deep10", "deep" };
    static const unsigned long OD3[3] = { 3UL, 10UL, 121UL };
    const char* const* KS = M.own ? OK3 : DK; const unsigned long* DS = M.own ? OD3 : DD;
    if (M.own) {                                                 // (0.3.0) the chart's own market: history in guarded steps 3 / 10 / 120 days
        if (M.deepStep >= 1) { M.deepDone = 1; return; }       // (0.3.1) one 3-day step only
        if (M.fpLast < 0 || M.caughtUp < 3 || !M.fpTry.empty()) return;
    }
    else {                                                       // (0.2.2) other markets: history in 1 / 3 / 10-day steps
        if (M.deepStep >= 3) { M.deepDone = 1; return; }
        if (!M.fpBars || !M.fpTry.empty()) return;               // only after its live request survived
    }
    if (!M.deepBars && M.deepRetryAt > nowT) return;            // (0.2.1) waiting to retry a short answer
    if (!M.deepTry.empty() && M.deepBars) guardClear(M, "deep");
    bool stale = false && M.deepBars && M.deepCount >= 0 && nowT - M.deepGrew > 900 && nowT - M.deepMade > 900;
    if (!M.deepBars || stale) {
        if (M.deepBars) { delete M.deepBars; M.deepBars = nullptr; }
        RTDATE now = currentDate();
        const char* kind = KS[M.deepStep];
        unsigned long days = DS[M.deepStep];
        {
            if (blockedKind(M, kind)) { M.deepStep = 3; M.deepDone = 1; trace(M.mkt + " history: the " + kind + " step crashed IRT before - stopped there"); return; }
            if (nowT < nextOtherAt) return;                      // one heavy request at a time, 2 minutes apart
            nextOtherAt = nowT + 120;
        }
        if (!guardBefore(M, kind)) return;
        M.deepBars = new RTBARS(PD_INTRA, cfg.fpMin, true, M.sym.c_str(), false, (RTDATE)(now - (RTDATE)(days * 86400UL)), true);
        M.deepMade = nowT; M.deepCount = -1; M.deepGrew = nowT;
        trace(M.mkt + " history: asked for " + std::to_string(days) + " day(s)");
    }
    if (M.deepBars->count != M.deepCount) { M.deepCount = M.deepBars->count; M.deepGrew = nowT; }
    if (M.deepBars->count < 2) return;
    long f0 = M.rows;
    {                                                            // (0.2.2) a history step (0.3.0: every market)
        M.err.clear();
        int n = writeSlice(M, *M.deepBars, M.deepLast, true, 400);
        if (M.rows != f0) trace(M.mkt + " history (" + KS[M.deepStep] + "): +" + std::to_string(M.rows - f0) + " rows");
        bool fin = M.deepDone || (n == 0 && M.err.empty() && nowT - M.deepGrew > 30);
        if (!fin) return;
        trace(M.mkt + " history: " + KS[M.deepStep] + " step done");
        delete M.deepBars; M.deepBars = nullptr; guardClear(M, "deep");
        M.deepStep++; M.deepDone = M.deepStep >= (M.own ? 1 : 3) ? 1 : 0;   // (0.3.1) own market: the 3-day step only
        M.deepLast = -1; M.deepSkip.clear(); M.deepOwn.clear();
        M.deepRetryAt = nowT + 120;
        return;
    }
    writeSlice(M, *M.deepBars, M.deepLast, true, 400);
    if (M.deepDone) {                                            // (0.2.1) DONE only if IRT really gave ~120 days
        long long oldest = secOf((RTDATE)(*M.deepBars->dt)[0]);
        if ((long long)nowT - oldest < 100LL * 86400LL) {
            time_t ot = (time_t)oldest; struct tm lt = *localtime(&ot); char ob[24]; strftime(ob, sizeof(ob), "%Y-%m-%d", &lt);
            M.deepDone = 0; M.deepLast = -1; M.deepSkip.clear(); M.deepOwn.clear();
            delete M.deepBars; M.deepBars = nullptr; guardClear(M, "deep");
            if (++M.deepTries >= 3) { M.deepRetryAt = (time_t)0x7FFFFFFF; trace(M.mkt + " deep back-fill: IRT gave bars only from " + ob + " - stopped for this IRT run (3 tries)"); }
            else { M.deepRetryAt = nowT + 7200; trace(M.mkt + " deep back-fill: IRT gave bars only from " + std::string(ob) + " (+" + std::to_string(M.rows - f0) + " rows) - retry in 2 h"); }
            return;
        }
    }
    if (M.rows != f0 || M.deepDone) trace(M.mkt + " deep back-fill: +" + std::to_string(M.rows - f0) + " rows" + (M.deepDone ? " - DONE" : ""));
    if (M.deepDone) { delete M.deepBars; M.deepBars = nullptr; guardClear(M, "deep"); }
}

void IRTReader::writeFpStatus(FpState& M)
{
    if (M.mkt == mkt) return;                                    // the chart's own market: in its full status file
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\IRTReader.status-" + M.mkt + ".txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    RTDATE d = currentDate(); struct tm t; secOf(d, &t);
    f << "VERSION," << IR_VERSION << "\nMARKET," << M.mkt << "\nSYMBOL," << M.sym << "\nRECORDED_BY," << sym << " chart\nTICK," << M.tick
      << "\nTIME," << irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec)
      << "\nFOOTPRINT,on,1m,rows " << M.rows << ",last bar " << M.lastTxt << ",deep " << (M.deepDone ? "done" : "running")
      << "\nTRADES,only on its own chart\nERROR," << M.err << "\n";
}

// ---- trades: every new tick since the cursor (RTTICKS - independent of the chart's own bars)
void IRTReader::trades()
{
    // (0.1.3, 2 Oct 22:56 trace: the pass stopped inside RTTICKS(0) - "the whole session" after the Friday close) never ask for
    // the whole session: the first run starts 10 minutes back, later runs just before the last written second (at most 1 h back)
    RTDATE now = currentDate();
    long long nowS = secOf(now);
    long long back = 600;
    if (tc.lastSec > 0) { back = nowS - tc.lastSec + 2; if (back < 2) back = 2; if (back > 3600) back = 3600; }
    RTDATE start = (RTDATE)(now - (RTDATE)back);
    RTTICKS T(start);
    if (T.count <= 0) return;
    tc.rewind();
    std::string rows, sessCur;
    for (long i = 0; i < T.count; i++) {
        RTDATE d = (RTDATE)(*T.dt)[(int)i];
        struct tm t; long long s = secOf(d, &t);
        if (!tc.take(s)) continue;
        std::string session = irl::sessionOf(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour);
        if (!sessCur.empty() && session != sessCur) { append(dirFor(sessCur) + "\\trades.csv", "time|price|size|bid|ask|side", rows); rows.clear(); }
        sessCur = session;
        double p = (*T.price)[(int)i], b = (*T.bid)[(int)i], a = (*T.ask)[(int)i];
        long sz = (long)(*T.size)[(int)i];
        std::string ts = irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        char line[200];
        snprintf(line, sizeof(line), "%s|%s|%ld|%s|%s|%c\n", ts.c_str(), irl::px(p, tick).c_str(), sz, irl::px(b, tick).c_str(),
                 irl::px(a, tick).c_str(), irl::aggressor(p, b, a));
        rows += line; trRows++; trLastTxt = ts;
    }
    if (!sessCur.empty()) append(dirFor(sessCur) + "\\trades.csv", "time|price|size|bid|ask|side", rows);
}

// ---- DOM: the top levels whenever the book changed, at most every domSec seconds
void IRTReader::depth()
{
    time_t now = time(0);
    if (now - domT < cfg.domSec) return;
    domT = now;
    MARKET_DEPTH md(NULL);
    domAvail = md.available() != 0;
    if (!domAvail) return;
    int n = md.maxLevels; if (n > 200) n = 200; if (n < 0) n = 0;   // (0.6.0) the full book the feed gives
    domLevels = n;
    std::vector<double> v; v.reserve((size_t)n * 4);
    for (int k = 0; k < n; k++) { DEPTH_LEVEL& L = md[(unsigned)k]; v.push_back(L.bid); v.push_back(L.bidsize); v.push_back(L.ask); v.push_back(L.asksize); }
    unsigned long long h = irl::bookHash(v);
    if (h == domHash) return;
    domHash = h;
    RTDATE d = currentDate(); struct tm t; secOf(d, &t);
    std::string ts = irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    std::string rows;
    char line[200];
    for (int k = 0; k < n; k++) {
        if (v[(size_t)k * 4] <= 0 && v[(size_t)k * 4 + 2] <= 0) continue;
        snprintf(line, sizeof(line), "%s|%d|%s|%.0f|%s|%.0f\n", ts.c_str(), k, irl::px(v[(size_t)k * 4], tick).c_str(), v[(size_t)k * 4 + 1],
                 irl::px(v[(size_t)k * 4 + 2], tick).c_str(), v[(size_t)k * 4 + 3]);
        rows += line;
    }
    if (!rows.empty()) { append(dirFor(irl::sessionOf(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour)) + "\\dom.csv", "time|level|bid|bid_size|ask|ask_size", rows); domRows++; }
}

// ---- order-by-order depth (MBO): every event the feed sends
void IRTReader::depthByOrder()
{
    if (!dbo) dbo = new DEPTH_BY_ORDER(NULL);
    std::string rows, sess;
    char line[260];
    int guard = 0;
    while (guard++ < 20000 && dbo->getNext() == RTX_OK) {
        const DEPTH_ORDER& o = dbo->dbo;
        struct tm t; secOf(o.timestamp, &t);
        if (sess.empty()) sess = irl::sessionOf(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour);
        static const char* ACT[4] = { "NEW", "MOD", "DEL", "FILL" };
        snprintf(line, sizeof(line), "%s|%s|%s|%s|%lu|%d|%d|%c|%s|%s\n",
                 irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec).c_str(),
                 o.action < 4 ? ACT[o.action] : "?", irl::px(o.price, tick).c_str(), irl::px(o.previousPrice, tick).c_str(), o.size, o.orderID,
                 o.aggressorOrderID, o.buySell == 1 ? 'B' : o.buySell == 0 ? 'S' : '?', irl::px(o.bid, tick).c_str(), irl::px(o.ask, tick).c_str());
        if (time(0) < startT + 600) rows += line;                // (0.6.0) written for the first 10 min (probe), counted after
        dboRows++;
    }
    if (!rows.empty()) append(dirFor(sess) + "\\dbo.csv", "time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask", rows);
}

void IRTReader::writeStatus()
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\IRTReader.status-" + mkt + ".txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    RTDATE d = currentDate(); struct tm t; secOf(d, &t);
    f << "VERSION," << IR_VERSION << "\nMARKET," << mkt << "\nSYMBOL," << sym << "\nROOT," << root << "\nTICK," << tick
      << "\nTIME," << irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec)
      << "\nFOLDER," << cfg.folder
      << "\nFOOTPRINT," << (cfg.fp ? "on" : "off") << "," << cfg.fpMin << "m,rows " << fpRows << ",last bar " << fpLastTxt << ",backfill days " << cfg.fpDays
      << "\nTRADES," << (cfg.trades ? "on" : "off") << ",rows " << trRows << ",last " << trLastTxt
      << "\nDOM," << (cfg.dom ? "on" : "off") << "," << (domAvail ? "available" : "NOT available") << ",levels " << domLevels << ",snapshots " << domRows
      << "\nDBO," << (cfg.dbo ? "on" : "off") << ",events " << dboRows << (time(0) < startT + 600 ? ",writing" : ",counting only")
      << "\nOTHER_MARKETS," << (othersTxt.empty() ? "off" : othersTxt)
      << "\nERROR," << err << "\n";
}

// (0.3.0) one file for every reader, written even before its market is known - which readers start, on which chart, with a timer or not
void IRTReader::boot(const std::string& what)
{
    if (++boots > 12) return;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\IRTReader.trace-boot.txt";
    { std::ifstream t(path.c_str(), std::ios::ate | std::ios::binary); if (t.good() && t.tellg() > 200000) { t.close(); std::remove(path.c_str()); } }
    std::ofstream f(path.c_str(), std::ios::app); if (!f.is_open()) return;
    time_t n = time(0); struct tm lt = *localtime(&n); char b[24]; strftime(b, sizeof(b), "%m-%d %H:%M:%S", &lt);
    char id[24]; snprintf(id, sizeof(id), "%llx", (unsigned long long)(uintptr_t)this);
    f << b << " " << IR_VERSION << " reader " << id << " chart " << sym << " root " << root << " market " << (mkt.empty() ? "?" : mkt) << ": " << what << "\n";
}

void IRTReader::trace(const std::string& what)
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\IRTReader.trace-" + mkt + ".txt";
    { std::ifstream t(path.c_str(), std::ios::ate | std::ios::binary); if (t.good() && t.tellg() > 300000) { t.close(); std::remove(path.c_str()); } }
    std::ofstream f(path.c_str(), std::ios::app); if (!f.is_open()) return;
    time_t n = time(0); struct tm lt = *localtime(&n);
    char b[16]; strftime(b, sizeof(b), "%H:%M:%S", &lt);
    f << b << " " << what << "\n";
}

void IRTReader::tickAll()
{
    if (busy) return;                                            // (0.1.2) never two passes at once
    busy = true;
    identify();
    if (mkt.empty()) { busy = false; return; }
    cursorFor = mkt;                                             // (0.4.0) no trades cursor: footprint only
    err.clear();
    long f0 = fpRows, t0 = trRows, d0 = domRows, b0 = dboRows;
    bool loud = ++ticks <= 6;                                    // the first passes say each step (to find a hang / crash)
    { bool have = false; for (auto& X : fps) if (X.mkt == mkt) have = true; if (!have) loadMarkets(); }   // (0.4.0) per chart
    if (!startT) startT = time(0);
    bool othersOn = time(0) >= startT + 300;                     // (0.2.2) other markets only after IRT has settled for 5 minutes
    static bool saidOthers = false;
    if (othersOn && !saidOthers && othersTxt != "off") { trace("other markets start now (5 min after the reader): " + othersTxt); saidOthers = true; }
    for (auto& M : fps) {                                        // (0.2.0) every market's footprint, one writer per market
        if (M.mkt != mkt) continue;                              // (0.4.0) only the market of the chart IRT is drawing now
        if (!sym.empty() && M.sym != sym) {                      // (0.5.2) the chart's contract changed (CLEZ26 -> CLEX26): start over on it
            trace(M.mkt + ": contract changed " + M.sym + " -> " + sym + " - asking again on the new contract");
            if (M.fpBars) { delete M.fpBars; M.fpBars = nullptr; } if (M.deepBars) { delete M.deepBars; M.deepBars = nullptr; }
            guardClear(M, "fp"); guardClear(M, "deep");
            M.sym = sym; M.tick = tick > 0 ? tick : M.tick; M.fpCount = -1; M.deepCount = -1; M.err.clear(); M.caughtUp = 0;
        }
        if (!own(M)) continue;
        if (!M.loaded) loadFpCursor(M);
        bool catching = M.fpLast < 0 || M.caughtUp == 0;
        if (time(0) - M.fpT >= (catching ? 2 : 20)) { if (loud) trace(M.mkt + " footprint..."); M.fpT = time(0); footprintOf(M); }
        else deepFill(M);
        saveFpCursor(M);
        writeFpStatus(M);
        if (M.mkt == mkt) { fpLast = M.fpLast; fpLastTxt = M.lastTxt; if (!M.err.empty()) err = M.err; }
    }
    if (cfg.trades) { if (loud) trace("trades..."); trades(); }
    if (cfg.dom) { if (loud) trace("dom..."); depth(); }
    if (cfg.dbo) { if (loud) trace("order-by-order..."); depthByOrder(); }
    if (loud) trace("pass done");
    backfilled = true;
    writeStatus();
    if (fpRows != f0 || trRows != t0 || !err.empty() || (time(0) % 60) == 0)
        trace("fp +" + std::to_string(fpRows - f0) + " (last " + fpLastTxt + ") trades +" + std::to_string(trRows - t0) + " dom +" +
              std::to_string(domRows - d0) + " dbo +" + std::to_string(dboRows - b0) + (err.empty() ? "" : " ERR " + err));
    busy = false;
}

int IRTReader::timer(RTX_EVENT* e)
{
    if (!e || e->v.timer.id != timerIdFor(this)) return RTX_FAIL;
    if (lastTimerTick == 0) boot("first timer tick");
    lastTimerTick = time(0);
    tickAll();
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    IRTReader *p = new IRTReader();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);   // (0.3.2) POST_DRAWING: draw() runs on every chart, like the Dealer Profile
    p->setExtendedFlags(CALL_CONTINUOUSLY);
    p->setDescription(IR_FIXED[0] ? "Footprint Reader for ONE market (the name says which): records its 1-minute footprint for the LRA analytics. Put it on that market's chart only. No settings. Draws nothing." : "LRA IRT Reader: records the footprint of every market he trades (and this chart's trades and DOM) for the LRA analytics. Draws nothing.");
    p->setVersion("0.6.1");
    return p;
}
