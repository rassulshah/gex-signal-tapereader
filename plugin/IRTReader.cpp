/********************************************************************************
 *  IRTReader.cpp  --  Investor/RT RTX extension  lsIRTReader  (v0.1.0, 2026-10-02)
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
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <direct.h>

static const char* IR_VERSION = "0.1.2";
static const int TIMER_ID = 4711;

struct RIdx { int market, fp, fpMin, fpDays, trades, dom, domSec, dbo, folder; };
static RIdx RX;
struct RSet { int market = 0, fpMin = 1, fpDays = 10, domSec = 2; bool fp = true, trades = true, dom = true, dbo = true; std::string folder = "C:\\Dev\\level-reversal-analytics\\data\\irt"; };

class IRTReader : public cppExtension {
public:
    IRTReader() : cppExtension() {}
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int timer(RTX_EVENT* e);

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
    bool busy = false; int ticks = 0;
    void trace(const std::string& what);

    bool dialogReady() { int i = getListIndex(RX.market); return i >= 0 && i <= 7; }
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
    if (me->timerOn) { destroyTimer(TIMER_ID); me->timerOn = false; }
    if (me->dbo) { delete me->dbo; me->dbo = nullptr; }
    if (me->fpBars) { delete me->fpBars; me->fpBars = nullptr; }
    return RTX_OK;
}
int cppExtension::calc(int)
{
    IRTReader* me = static_cast<IRTReader*>(this);
    if (!me->timerOn && createTimer(TIMER_ID, 1000) == RTX_OK) me->timerOn = true;   // every second, chart or no repaint
    return RTX_OK;
}

int IRTReader::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int IRTReader::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int IRTReader::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
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
    S.fp = true; S.trades = isBoxChecked(RX.trades) != 0; S.dom = isBoxChecked(RX.dom) != 0; S.dbo = isBoxChecked(RX.dbo) != 0;
    S.fpMin = 1;
    S.fpDays = 10;
    S.domSec = getIntegerValue(RX.domSec); if (S.domSec < 1 || S.domSec > 60) S.domSec = 2;
}

void IRTReader::identify()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    const char* s = getSymbol(); sym = s ? s : "";
    mkt = dl::marketFor(cfg.market, root);
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
}

void IRTReader::saveCursor()
{
    _mkdir(cfg.folder.c_str());
    std::ofstream f((cfg.folder + "\\_cursor-" + mkt + ".txt").c_str(), std::ios::trunc);
    if (f.is_open()) f << fpLast << " " << tc.lastSec << " " << tc.doneInSec << " " << (backfilled ? 1 : 0) << "\n";
}

// ---- footprint: finished N-minute bars with volume at price (RTBARS with VAP - independent of the chart's own bars)
void IRTReader::footprint()
{
    time_t nowT = time(0);
    if (!fpBars || (fpCount >= 0 && nowT - fpGrew > 600 && nowT - fpMade > 600)) {   // made once; remade if it stops growing for 10 min
        if (fpBars) { delete fpBars; fpBars = nullptr; }
        RTDATE now = currentDate();
        RTDATE start = (fpLast > 0) ? (RTDATE)(now - (RTDATE)1800UL) : (RTDATE)(now - (RTDATE)((cfg.fpDays + 1) * 86400UL));
        fpBars = new RTBARS(PD_INTRA, cfg.fpMin, true, NULL, false, start, true);
        fpMade = nowT; fpCount = -1; fpGrew = nowT;
        trace("footprint: bars asked from " + std::to_string((long long)secOf(start)) + (fpLast > 0 ? " (last 30 min)" : " (back-fill)"));
    }
    RTBARS& bars = *fpBars;
    if (bars.count != fpCount) { fpCount = bars.count; fpGrew = nowT; }
    if (bars.count < 2) { err = bars.count ? "" : "no footprint bars from IRT yet"; return; }
    int done = 0;
    std::string rows, sessCur;
    std::string barRows;
    long long lastWritten = fpLast;
    for (long i = 0; i < bars.count - 1; i++) {                 // the last bar is still forming
        RTDATE d = (RTDATE)(*bars.dt)[(int)i];
        struct tm t; long long s = secOf(d, &t);
        if (s <= fpLast) continue;
        std::string session = irl::sessionOf(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour);
        if (!sessCur.empty() && session != sessCur) {             // one file per session
            std::string dir = dirFor(sessCur);
            char fn[32]; snprintf(fn, sizeof(fn), "\\fp_%dm.csv", cfg.fpMin);
            append(dir + fn, "bar|price|bought|sold|volume|trades|max_delta|min_delta", rows);
            append(dir + "\\fp_bars.csv", "bar|minutes|open|high|low|close|volume|bought|sold|max_delta|min_delta|open_delta|max_price_vol", barRows);
            rows.clear(); barRows.clear();
        }
        sessCur = session;
        std::string ts = irl::stamp(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        BARSTATISTICS bs; memset(&bs, 0, sizeof(bs));
        if (bars.getBarStatistics((int)i, bs) != RTX_OK) { err = "volume at price not loaded yet from " + ts; break; }   // retried next tick
        if (++done > 400) break;                                    // a slice per tick: the back-fill spreads over ~1-2 min
        {
            char b[300];
            snprintf(b, sizeof(b), "%s|%d|%s|%s|%s|%s|%ld|%ld|%ld|%ld|%ld|%ld|%ld\n", ts.c_str(), cfg.fpMin,
                     irl::px((*bars.op)[(int)i], tick).c_str(), irl::px((*bars.hi)[(int)i], tick).c_str(), irl::px((*bars.lo)[(int)i], tick).c_str(),
                     irl::px((*bars.cl)[(int)i], tick).c_str(), (long)(*bars.vo)[(int)i], bs.buyVolume, bs.sellVolume, bs.maxDelta, bs.minDelta,
                     bs.openDelta, bs.maxPriceVol);
            barRows += b;
            int np = bs.prices > 0 ? bs.prices : 400;
            for (int k = 0; k < np; k++) {
                VOLPROFILE vp; memset(&vp, 0, sizeof(vp));
                if (bars.getVolumeProfile((int)i, k, vp) != RTX_OK) break;
                if (vp.totalVolume <= 0 && vp.buyVolume <= 0 && vp.sellVolume <= 0) continue;
                snprintf(b, sizeof(b), "%s|%s|%ld|%ld|%ld|%ld|%ld|%ld\n", ts.c_str(), irl::px(vp.price, tick).c_str(), vp.buyVolume, vp.sellVolume,
                         vp.totalVolume, vp.tickCount, vp.maxDelta, vp.minDelta);
                rows += b; fpRows++;
            }
        }
        lastWritten = s; fpLastTxt = ts;
    }
    if (!sessCur.empty()) {
        std::string dir = dirFor(sessCur);
        char fn[32]; snprintf(fn, sizeof(fn), "\\fp_%dm.csv", cfg.fpMin);
        append(dir + fn, "bar|price|bought|sold|volume|trades|max_delta|min_delta", rows);
        append(dir + "\\fp_bars.csv", "bar|minutes|open|high|low|close|volume|bought|sold|max_delta|min_delta|open_delta|max_price_vol", barRows);
    }
    fpLast = lastWritten;
}

// ---- trades: every new tick since the cursor (RTTICKS - independent of the chart's own bars)
void IRTReader::trades()
{
    RTDATE start = 0;                                           // 0 = the current session (the first run back-fills it)
    if (tc.lastSec > 0) {
        RTDATE now = currentDate();
        long long nowS = secOf(now);
        long long back = nowS - tc.lastSec + 2;                 // re-read from just before the last written second
        start = (back > 0 && back < 3 * 86400) ? (RTDATE)(now - (RTDATE)back) : 0;
    }
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
    int n = md.maxLevels; if (n > 16) n = 16; if (n < 0) n = 0;
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
        rows += line; dboRows++;
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
      << "\nDBO," << (cfg.dbo ? "on" : "off") << ",events " << dboRows
      << "\nERROR," << err << "\n";
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
    if (cursorFor != mkt) { loadCursor(); cursorFor = mkt; trace("start " + std::string(IR_VERSION) + " " + sym + " cursor fp " + std::to_string(fpLast) + " trades " + std::to_string(tc.lastSec)); }
    err.clear();
    long f0 = fpRows, t0 = trRows, d0 = domRows, b0 = dboRows;
    bool catching = fpLast < 0 || (long long)time(0) - fpLast > 600;     // back-filling: a slice every 2 s, then every 20 s
    bool loud = ++ticks <= 6;                                    // the first passes say each step (to find a hang / crash)
    if (cfg.fp && time(0) - fpT >= (catching ? 2 : 20)) { if (loud) trace("footprint..."); fpT = time(0); footprint(); }
    if (cfg.trades) { if (loud) trace("trades..."); trades(); }
    if (cfg.dom) { if (loud) trace("dom..."); depth(); }
    if (cfg.dbo) { if (loud) trace("order-by-order..."); depthByOrder(); }
    if (loud) trace("pass done");
    backfilled = true;
    saveCursor();
    writeStatus();
    if (fpRows != f0 || trRows != t0 || !err.empty() || (time(0) % 60) == 0)
        trace("fp +" + std::to_string(fpRows - f0) + " (last " + fpLastTxt + ") trades +" + std::to_string(trRows - t0) + " dom +" +
              std::to_string(domRows - d0) + " dbo +" + std::to_string(dboRows - b0) + (err.empty() ? "" : " ERR " + err));
    busy = false;
}

int IRTReader::timer(RTX_EVENT* e)
{
    if (!e || e->v.timer.id != TIMER_ID) return RTX_FAIL;
    tickAll();
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    IRTReader *p = new IRTReader();
    p->setArrayCount(1);
    p->setFlags(OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setExtendedFlags(CALL_CONTINUOUSLY);
    p->setDescription("LRA IRT Reader: records the chart market's footprint, trades and DOM for the LRA analytics. Draws nothing.");
    p->setVersion("0.1.2");
    return p;
}
