/********************************************************************************
 *  lsTradeManager - the TOPSTEP TRADE MANAGER on each market's chart (Rassul 2026-10-06 21:16 "a topstep indicator in irt ...
 *  sending warnings, letting me know whether i should be in this trade or not, telling me about entry, exit executions ...
 *  oversight, guidance and mentorship as well as coaching"; mockup v1 approved 2026-10-07 08:58 "trade manager is approved, we
 *  will improve it").
 *
 *  One small box, three states:
 *    FLAT      READY / WAIT: the day (P&L, room to the stop, trades, losses in a row, MLL room), the next trade the Read is
 *              working (side + key level), a 4-line checklist (key level / Read / time window / news), the size for 1R and the
 *              trigger
 *    IN TRADE  side, size, entry, stop, target, open P&L in R with a stop -> entry -> 2R bar, HOLD / EXIT? from the Read, the
 *              next step (stop to break-even at +1R)
 *    STOPPED   red frame: why the day is over (the guard's alerts)
 *  Data (all written by the LRA bridge, read only when the file changed):
 *    %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Topstep.csv   the Topstep Guard (TopstepX userscript -> bridge): STATUS, PNL,
 *        TRADES, INROW, ROOM, RISK, POS|<mkt>|long/short|size|price|upl, ALERT|level|time|text
 *    LRA-Dealer-<MKT>.csv   PRICE, TURN (side + header: DEMAND / SUPPLY n of m, the level, T1), RDX (extreme, level), RD (trigger)
 *    LRA-Session-<MKT>.csv  WIN (ACTIVE / CHOP), CAL (calendar events)
 *  PLACE (1.2.0, 2026-10-10): click on the box and drag it anywhere in the price area; release = it stays there. The spot is
 *  remembered PER CHART (market + bar size: the ES 3-min and the ES 60-min chart each keep their own) in
 *  lsFlexLevels\TradeManager.drag-<MKT>_<sec>.txt. Double-click the box = back to the Position setting. Choosing a Position
 *  in the settings later also moves it back to that spot. The box is always kept inside the price area left of the last bar
 *  when it fits there, and never over the Dealer / Delta profiles.
 *  Why a file and not the chart's own settings: IRT gives the plugin mouse events but no way to write a chart setting from a
 *  mouse event (list settings even read back as -1 outside the settings window), so the dragged spot lives beside the Position
 *  files the other LRA boxes already use. Settings: Market, Font size, Position (9 spots).
 *  Never places, changes or cancels an order - it only reads files and draws.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>                   // GetAsyncKeyState: is the left button still held during a drag
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")     // gex-build links only the SDK lib; GetAsyncKeyState / GetSystemMetrics live in user32
#endif
#endif
#include "irtsdk.h"
// windows.h may define the obsolete `far` macro, which conflicts with DealerLogic's Node::far member.
#ifdef far
#undef far
#endif
#include "DealerLogic.h"
#include "HostSlot.h"
#include "TradeGuardLayout.h"
#include <chrono>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <cerrno>
#include <climits>
#include <new>

static const char* const TM_VERSION = "1.2.0";   // (1.2.0) drag the box, per-chart spot, never over the profiles / past price;
                                                  // (1.1.2) per-chart state, validated bridge inputs, bounded sizing
static const COLOR C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF, C_BOXBG = 0x000B1220, C_BORDER = 0x00334155;
static const COLOR C_AMBER = 0x00F59E0B, C_RED = 0x00EF4444, C_GREEN = 0x0022C55E, C_DKRED = 0x007F1D1D, C_DKGRN = 0x00166534, C_TRACK = 0x001F2937;

struct TIdx { int market = 0, font = 1, place = 2; };            // parameter indices: a member of the object, no static (#1)
struct TSet { int market = 0, font = 8, place = 2; };          // place = the Position (0-8), default Top right

#ifdef TM_TEST_HOOKS
extern bool tmTestButtonHeld;                                   // the mock decides whether the left button is still down
extern int (*tmTestRename)(const char* from, const char* to);   // the mock makes a rename fail
#endif

// futures $ per 1.0 price move: the micro (what he trades) and its size cap (plan.json max_size)
static double microPV(const std::string& m) { return m == "ES" ? 5 : m == "NQ" ? 2 : m == "CL" ? 100 : m == "NG" ? 1000 : m == "GC" ? 10 : m == "HG" ? 2500 : m == "EU" ? 12500 : 0; }
static const char* microName(const std::string& m) { return m == "ES" ? "MES" : m == "NQ" ? "MNQ" : m == "CL" ? "MCL" : m == "NG" ? "MNG" : m == "GC" ? "MGC" : m == "HG" ? "MHG" : m == "EU" ? "M6E" : "?"; }
static int microCap(const std::string& m) { return m == "EU" ? 10 : 5; }
static double tickOf(const std::string& m) { return m == "ES" || m == "NQ" ? 0.25 : m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 : m == "HG" ? 0.0005 : m == "EU" ? 0.0001 : 0.01; }
static int decOf(const std::string& m) { return m == "GC" ? 1 : m == "HG" ? 4 : m == "EU" ? 5 : m == "NG" ? 3 : 2; }
static int sizeForRisk(double risk, double dist, double pointValue, int cap)
{
    if (cap < 1 || !std::isfinite(risk) || !std::isfinite(dist) || !std::isfinite(pointValue) || risk <= 0 || dist <= 0 || pointValue <= 0) return 0;
    double unitRisk = dist * pointValue;
    if (!std::isfinite(unitRisk) || unitRisk <= 0) return 0;
    double raw = risk / unitRisk;
    if (!std::isfinite(raw) || raw >= cap) return cap;
    return raw >= 1 ? (int)std::floor(raw) : 0;
}

static std::string fmtPx(const std::string& m, double v)
{
    char b[32]; snprintf(b, sizeof b, "%.*f", decOf(m), v);
    std::string s(b); size_t dot = s.find('.'); std::string ip = dot == std::string::npos ? s : s.substr(0, dot), fp = dot == std::string::npos ? "" : s.substr(dot);
    std::string o; int n = 0; bool neg = !ip.empty() && ip[0] == '-'; if (neg) ip = ip.substr(1);
    for (int i = (int)ip.size() - 1; i >= 0; i--) { o.insert(o.begin(), ip[i]); if (++n % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return (neg ? "-" : "") + o + fp;
}
static std::string money(double v, bool sign = true)
{
    char b[32]; double a = std::fabs(v); snprintf(b, sizeof b, "%.0f", a);
    std::string s(b), o; int n = 0;
    for (int i = (int)s.size() - 1; i >= 0; i--) { o.insert(o.begin(), s[i]); if (++n % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return std::string(v < 0 ? "-" : (sign ? "+" : "")) + "$" + o;
}
static std::string trim(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
static std::string lower(const std::string& s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::tolower((unsigned char)r[i]);
    return r;
}
static bool parseNum(const std::string& s, double& out)
{
    std::string t;
    for (size_t i = 0; i < s.size(); ++i) if (s[i] != ',') t += s[i];
    const char* p = t.c_str(); char* end = 0; errno = 0;
    double v = std::strtod(p, &end);
    while (end && std::isspace((unsigned char)*end)) ++end;
    if (p == end || (end && *end) || errno == ERANGE || !std::isfinite(v)) return false;
    out = v; return true;
}
static bool parseInt(const std::string& s, int& out)
{
    const char* p = s.c_str(); char* end = 0; errno = 0;
    long v = std::strtol(p, &end, 10);
    while (end && std::isspace((unsigned char)*end)) ++end;
    if (p == end || (end && *end) || errno == ERANGE || v < INT_MIN || v > INT_MAX) return false;
    out = (int)v; return true;
}
static std::vector<std::string> splitBar(const std::string& ln) { std::vector<std::string> c; std::stringstream ss(ln); std::string x; while (std::getline(ss, x, '|')) c.push_back(x); return c; }
// (1.2.0, checklist #33) bounded: at most 64 KiB and 400 lines of 1 KiB from any bridge file
static std::vector<std::string> readLines(const std::string& p)
{
    std::vector<std::string> L; std::ifstream f(p.c_str(), std::ios::binary); std::string ln; size_t bytes = 0;
    while (L.size() < 400 && bytes < 65536 && std::getline(f, ln)) {
        bytes += ln.size() + 1;
        if (ln.size() > 1024) ln.resize(1024);
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        L.push_back(ln);
    }
    return L;
}
static bool readSmall(const std::string& p, std::string& out, size_t cap = 4096)
{
    out.clear(); std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) return false;
    out.resize(cap + 1); f.read(&out[0], (std::streamsize)out.size()); out.resize((size_t)f.gcount());
    return out.size() <= cap;
}
static bool moveOver(const std::string& from, const std::string& to)
{
#ifdef TM_TEST_HOOKS
    if (tmTestRename) return tmTestRename(from.c_str(), to.c_str()) == 0;
#endif
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// (checklist #5) write <path>.tmp, the old file to <path>.bak, rename into place; the .bak goes back if the rename fails
static bool publishFile(const std::string& path, const std::string& contents)
{
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    {
        std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(contents.data(), (std::streamsize)contents.size()); f.flush();
        if (!f) { f.close(); std::remove(tmp.c_str()); return false; }
    }
    const bool hadOld = dl::fileStamp(path) >= 0;
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    return true;
}
static std::string flexDir()
{
    const char* up = getenv("USERPROFILE");
    return up && up[0] ? std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\" : std::string();
}
static bool leftButtonHeld()
{
#ifdef TM_TEST_HOOKS
    return tmTestButtonHeld;
#elif defined(_WIN32)
    const int vk = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;   // the primary button, even when swapped
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
#else
    return true;
#endif
}
static int hmMin(const std::string& raw)
{
    std::string s = trim(raw);
    if (s.size() != 5 || s[2] != ':' || !std::isdigit((unsigned char)s[0]) || !std::isdigit((unsigned char)s[1]) || !std::isdigit((unsigned char)s[3]) || !std::isdigit((unsigned char)s[4])) return -1;
    int h = (s[0] - '0') * 10 + s[1] - '0', m = (s[3] - '0') * 10 + s[4] - '0';
    return h < 24 && m < 60 ? h * 60 + m : -1;
}
static bool inWindow(int now, int start, int stop) { return start <= stop ? now >= start && now < stop : now >= start || now < stop; }
static int minuteDelta(int from, int to) { int d = to - from; if (d > 720) d -= 1440; if (d < -720) d += 1440; return d; }
static std::string t12(int m)
{
    char b[16]; int h = (m / 60) % 24; snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m % 60, h < 12 ? "AM" : "PM"); return b;
}

struct TMWin { std::string tag; int a = -1, b = -1; };
struct TMCal { int m = -1; std::string imp, title; };
// (1.2.0) where one chart's box is, keyed by chart (<MKT>_<seconds per bar>) inside the chart's state. With IRT's per-chart
// user-data slot there is one entry; if a host ever kept no slot (HostSlot's shared fallback) two charts still never share a
// drag (checklist #1 / the 2026-10-07 copper-moved-with-gold bug).
struct TMPlace {
    std::string key;                                   // <MKT>_<seconds per bar>: the drag file's name
    long long placeStamp = -2; int place = 2;          // the Position file (per market, written by the settings window)
    long long dragStamp = -2; tgl::Saved saved;        // the dragged spot (per chart)
    long long profStamp1 = -2, profStamp2 = -2; tgl::Profiles prof;
    std::deque<std::pair<long long, int> > profHist;   // the profiles' left edge over the last 5 s (their status files are shared)
    int profPaneR = -1;                                // ... for this pane width (a resize starts a new history)
    tgl::Drag drag;
    tgl::Box box, area;                                // where the box was drawn last, and the room (area) it had
    bool drawn = false; int placeUsed = 2;             // the Position the box was placed from on the last draw
    long long fileReads = 0, saves = 0;                // test counters (#31 / #29)
};

struct TMState {
    TSet cfg;
    std::string mkt, root, lastMkt;
    std::string status, alertsLvl[6], alertsT[6], alertsTx[6]; int nAlerts = 0;
    double dayPnl = 0, left = 0, stop = 600, risk = 150, room = -1; int trades = 0, maxTrades = 3, inRow = 0; bool haveGuard = false;
    bool inPos = false; std::string posSide; int posSize = 0; double posPx = 0, posUpl = 0;
    bool haveTurn = false, atLevel = false; char side = 0; std::string head, levelTxt, ctl; double ext = 0, lvl = 0, t1 = 0, px = 0;
    std::vector<TMWin> W; std::vector<TMCal> C;
    std::map<std::string, long long> stamps;
    std::map<std::string, TMPlace> places;             // (1.2.0) per chart key, bounded
    long long saves = 0;                               // test counter (#29): Position files written
};

class TradeManager : public cppExtension {
public:
    TradeManager() : cppExtension() {}
    TIdx TX;                                           // set in setup(); one per extension object
    virtual int parmsLoad(void) { return RTX_OK; }     // (#29) loading never saves and never changes anything
    virtual int parmsApply(void) { if (ready()) { TSet cfg; readS(cfg); savePl(cfg); } return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { if (ready()) { TSet cfg; readS(cfg); savePl(cfg); } return RTX_OK; }
    virtual int done(void);
    virtual int destroy(void);
    // the Position is kept per market in TradeManager.place-<MKT>.txt (IRT reads list settings back as -1 outside the settings
    // window). Written only when it CHANGED, from the settings window's callbacks, safely; a new Position beats a dragged spot.
    void savePl(const TSet& cfg)
    {
        char b[32] = {0}; const char* rs = getRootSymbol(b);
        const std::string mk = dl::marketFor(cfg.market, rs ? rs : "");
        if (mk.empty() || cfg.place < 0 || cfg.place > 8) return;
        const std::string p = dl::placePath("TradeManager", mk);
        std::string cur; long long now = -1;
        if (readSmall(p, cur, 64)) { long long v = 0; std::string t = cur; while (!t.empty() && (t.back() == '\n' || t.back() == '\r' || t.back() == ' ')) t.pop_back(); if (tgl::parseIntStrict(t, 0, 8, v)) now = v; }
        if (now == cfg.place) return;
        if (publishFile(p, std::to_string(cfg.place) + "\n")) { TMState* s = slot_.get(this, false); if (s) s->saves++; }
    }
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);

    bool ready() { int i = getListIndex(TX.market); return i >= 0 && i <= 7; }
    void readS(TSet& S)
    {
        { int v = getListIndex(TX.market); if (v >= 0 && v <= 7) S.market = v; }
        { int v = getIntegerValue(TX.font); if (v >= 6 && v <= 20) S.font = v; }
        { int v = getListIndex(TX.place); if (v >= 0 && v <= 8) S.place = v; }
    }
    HostSlot<TMState> slot_;
    TMState* state() { return slot_.get(this, true); }
    TMState* testState() { return slot_.get(this, false); }
    void releaseState() { slot_.release(this); }
    void loadPlacement(TMPlace& s, const std::string& mkt);
    int profilesLeftEdge(TMPlace& s, int paneRight);
    void commitDrag(TMPlace& s);
    void resetDrag(TMPlace& s);
    TMPlace* placeFor(TMState& st, const std::string& key, bool create)
    {
        if (key.empty()) return nullptr;
        std::map<std::string, TMPlace>::iterator it = st.places.find(key);
        if (it != st.places.end()) return &it->second;
        if (!create) return nullptr;
        if (st.places.size() >= 32) st.places.clear();
        TMPlace& p = st.places[key]; p.key = key; return &p;
    }
    bool changed(TMState& s, const std::string& p) { long long st = dl::fileStamp(p); bool c = s.stamps[p] != st; s.stamps[p] = st; return c; }
    void load(TMState& s);
    int nowMin() { time_t t = time(0); struct tm lt; localtime_s(&lt, &t); return lt.tm_hour * 60 + lt.tm_min; }
    void fill(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
    void frame(short l, short t, short r, short b, COLOR c)
    {
        setPen(c, 1, P_SOLID); PNT p; p.set(0, 0.0f);
        p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
    }
    int textW(const char* s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s, -1); }
    void text(short x, short y, const char* s, COLOR col, int sz, bool bold)
    {
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(y - (short)(sz * 0.45f + 0.5f)); int w = (int)getTextWidth(s, -1);
        RCT rc; rc.set(x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz)); rc.drawText(s, false, false);
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

int cppExtension::setup(void)
{
    TIdx& TX = static_cast<TradeManager*>(this)->TX;   // the parameter indices live in the object (no static state)
    setParameterVersion(2);                            // unchanged: the same three settings in the same order, saved values kept
    setParameterDialogHeight(3);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    TX.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    TX.font   = pc++; setIntegerParameter("Font size (pt)", 8, NUMW, SL);
    TX.place  = pc++; setListParameter("Position", 2, dl::ANCHORS);   // one of 9 spots, default Top right; drag overrides per chart
    return RTX_OK;
}

int TradeManager::done(void)
{
    releaseState();                         // done() is called for a symbol/period change
    return RTX_OK;
}

int TradeManager::destroy(void)
{
    releaseState();                         // destroy() is called when this chart host is removed
    return RTX_OK;
}

void TradeManager::load(TMState& s)
{
    TSet& cfg = s.cfg; std::string& mkt = s.mkt; std::string& root = s.root; std::string& lastMkt = s.lastMkt;
    std::string& status = s.status; std::string (&alertsLvl)[6] = s.alertsLvl; std::string (&alertsT)[6] = s.alertsT; std::string (&alertsTx)[6] = s.alertsTx; int& nAlerts = s.nAlerts;
    double& dayPnl = s.dayPnl; double& left = s.left; double& stop = s.stop; double& risk = s.risk; double& room = s.room; int& trades = s.trades; int& maxTrades = s.maxTrades; int& inRow = s.inRow; bool& haveGuard = s.haveGuard;
    bool& inPos = s.inPos; std::string& posSide = s.posSide; int& posSize = s.posSize; double& posPx = s.posPx; double& posUpl = s.posUpl;
    bool& haveTurn = s.haveTurn; bool& atLevel = s.atLevel; char& side = s.side; std::string& head = s.head; std::string& levelTxt = s.levelTxt; std::string& ctl = s.ctl; double& ext = s.ext; double& lvl = s.lvl; double& t1 = s.t1; double& px = s.px;
    std::vector<TMWin>& W = s.W; std::vector<TMCal>& C = s.C;
    auto resetGuard = [&]() {
        status.clear(); nAlerts = 0; for (int i = 0; i < 6; ++i) { alertsLvl[i].clear(); alertsT[i].clear(); alertsTx[i].clear(); }
        dayPnl = left = 0; stop = 600; risk = 150; room = -1; trades = 0; maxTrades = 3; inRow = 0; haveGuard = false;
        inPos = false; posSide.clear(); posSize = 0; posPx = posUpl = 0;
    };
    auto resetDealer = [&]() { haveTurn = atLevel = false; side = 0; head.clear(); levelTxt.clear(); ctl.clear(); ext = lvl = t1 = px = 0; };
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    const char* up = getenv("USERPROFILE");
    if (!up || mkt.empty()) { resetGuard(); resetDealer(); W.clear(); C.clear(); return; }
    std::string dir = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\";
    std::string pG = dir + "LRA-Topstep.csv", pD = dir + "LRA-Dealer-" + mkt + ".csv", pS = dir + "LRA-Session-" + mkt + ".csv";
    bool mk = lastMkt != mkt; lastMkt = mkt;                                  // state is per current chart host
    if (changed(s, pG) || mk) {
        resetGuard();
        std::vector<std::string> L = readLines(pG);
        std::vector<std::vector<std::string> > al;
        for (size_t i = 0; i < L.size(); i++) {
            std::vector<std::string> c = splitBar(L[i]); if (c.empty()) continue;
            if (c[0] == "STATUS" && c.size() >= 2) { status = lower(trim(c[1])); haveGuard = !status.empty(); }
            else if (c[0] == "PNL" && c.size() >= 4) { double p = 0, l = 0, st = 0; if (parseNum(c[1], p) && parseNum(c[2], l) && parseNum(c[3], st)) { dayPnl = p; left = l; stop = st; } }
            else if (c[0] == "TRADES" && c.size() >= 3) { int t = 0, mx = 0; if (parseInt(c[1], t) && parseInt(c[2], mx) && t >= 0 && mx >= 0) { trades = t; maxTrades = mx; } }
            else if (c[0] == "INROW" && c.size() >= 2) { int r = 0; if (parseInt(c[1], r) && r >= 0) inRow = r; }
            else if (c[0] == "ROOM" && c.size() >= 2) { double r = 0; if (trim(c[1]).empty()) room = -1; else if (parseNum(c[1], r)) room = r; }
            else if (c[0] == "RISK" && c.size() >= 2) { double r = 0; if (parseNum(c[1], r) && r > 0) risk = r; }
            else if (c[0] == "POS" && c.size() >= 6 && trim(c[1]) == mkt) { int q = 0; double p = 0, u = 0; std::string ps = lower(trim(c[2])); if ((ps == "long" || ps == "short") && parseInt(c[3], q) && q > 0 && parseNum(c[4], p) && p > 0 && parseNum(c[5], u)) { inPos = true; posSide = ps; posSize = q; posPx = p; posUpl = u; } }
            else if (c[0] == "ALERT" && c.size() >= 4) al.push_back(c);
        }
        for (size_t i = 0; i < al.size() && nAlerts < 6; i++) { alertsLvl[nAlerts] = al[i][1]; alertsT[nAlerts] = al[i][2]; alertsTx[nAlerts] = al[i][3]; nAlerts++; }
    }
    if (changed(s, pD) || mk) {
        resetDealer();
        std::vector<std::string> L = readLines(pD);
        for (size_t i = 0; i < L.size(); i++) {
            std::vector<std::string> c = splitBar(L[i]); if (c.empty()) continue;
            if (c[0] == "PRICE" && c.size() >= 2) { double p = 0; if (parseNum(c[1], p) && p > 0) px = p; }
            else if (c[0] == "TURN" && c.size() >= 3) { char turn = c[1].empty() ? 0 : (char)std::toupper((unsigned char)c[1][0]); if (turn == 'L' || turn == 'S') { haveTurn = true; side = turn; head = c[2]; } }
            else if (c[0] == "RDX" && c.size() >= 4) { double x = 0, l = 0; if (parseNum(c[2], x) && parseNum(c[3], l) && x > 0 && l > 0) { atLevel = true; ext = x; lvl = l; } }
        }
        // who is in control: the header's first DEMAND / SUPPLY / BALANCED word and its count ("DEMAND? 3 of 5")
        std::string headUpper = lower(head); for (size_t i = 0; i < headUpper.size(); ++i) headUpper[i] = (char)std::toupper((unsigned char)headUpper[i]);
        const char* words[3] = {"DEMAND", "SUPPLY", "BALANCED"};
        for (int w = 0; w < 3 && ctl.empty(); w++) {
            size_t p = headUpper.find(words[w]);
            if (p != std::string::npos) { size_t e = head.find(" - ", p); ctl = head.substr(p, e == std::string::npos ? std::string::npos : e - p); }
        }
        size_t a = head.find(" at ");                                         // "LONG at CR 4,165.0 - low 22:15 ..."
        if (a != std::string::npos) { size_t e = head.find(" - ", a); levelTxt = head.substr(a + 4, e == std::string::npos ? std::string::npos : e - a - 4); }
        size_t tp = head.find("T1 ");
        if (tp != std::string::npos) { size_t e = head.find(" ", tp + 3); double target = 0; if (parseNum(head.substr(tp + 3, e == std::string::npos ? std::string::npos : e - tp - 3), target) && target > 0) t1 = target; }
    }
    if (changed(s, pS) || mk) {
        W.clear(); C.clear();
        std::vector<std::string> L = readLines(pS);
        for (size_t i = 0; i < L.size(); i++) {
            std::vector<std::string> c = splitBar(L[i]); if (c.empty()) continue;
            if (c[0] == "WIN" && c.size() >= 4) { TMWin w; w.tag = lower(trim(c[1])); w.a = hmMin(c[2]); w.b = hmMin(c[3]); if (w.a >= 0 && w.b >= 0) W.push_back(w); }
            else if (c[0] == "CAL" && c.size() >= 4) { TMCal k; k.m = hmMin(c[1]); k.imp = lower(trim(c[2])); k.title = c[3]; if (k.m >= 0) C.push_back(k); }
        }
    }
}

int TradeManager::draw(void)
{
    TMState* state = this->state(); if (!state) return RTX_OK;
    TMState& S = *state;
    TSet& cfg = S.cfg; std::string& mkt = S.mkt; std::string& status = S.status; std::string (&alertsTx)[6] = S.alertsTx; int& nAlerts = S.nAlerts;
    double& dayPnl = S.dayPnl; double& left = S.left; double& risk = S.risk; double& room = S.room; int& trades = S.trades; int& maxTrades = S.maxTrades; int& inRow = S.inRow; bool& haveGuard = S.haveGuard;
    bool& inPos = S.inPos; std::string& posSide = S.posSide; int& posSize = S.posSize; double& posPx = S.posPx; double& posUpl = S.posUpl;
    bool& haveTurn = S.haveTurn; bool& atLevel = S.atLevel; char& side = S.side; std::string& ctl = S.ctl; std::string& levelTxt = S.levelTxt; double& ext = S.ext; double& lvl = S.lvl; double& t1 = S.t1; double& px = S.px;
    std::vector<TMWin>& W = S.W; std::vector<TMCal>& C = S.C;
    if (ready()) readS(cfg);
    load(S);
    if (mkt.empty()) return RTX_OK;
    RCT pane; pane.getPaneRect(false);
    int fs = cfg.font, now = nowMin();
    short lh = (short)(fs * 1.75f + 0.5f), pad = (short)(fs * 0.7f + 0.5f);
    struct Ln { std::string a, b; COLOR ca, cb; bool bold; int bar; };   // a = a coloured lead word, b = the rest; bar 1 = the R bar
    std::vector<Ln> L;
    auto add = [&](const std::string& a, COLOR ca, const std::string& b, COLOR cb, bool bold) { Ln l; l.a = a; l.b = b; l.ca = ca; l.cb = cb; l.bold = bold; l.bar = 0; L.push_back(l); };
    char s[220];
    bool stopped = status == "stop";
    std::string ctlUpper = lower(ctl); for (size_t i = 0; i < ctlUpper.size(); ++i) ctlUpper[i] = (char)std::toupper((unsigned char)ctlUpper[i]);
    COLOR frameC = stopped ? C_RED : C_BORDER;
    // ---- the day line (all states)
    std::string dayLn = haveGuard ? "Day " + money(dayPnl) + "   To stop " + (left >= 0 ? money(left, false) : "NONE") + "   Trades " + std::to_string(trades) + "/" + std::to_string(maxTrades) +
                        "   In a row " + std::to_string(inRow) + (room >= 0 ? "   MLL room " + money(room, false) : "") : "Topstep Guard not connected (TopstepX tab + userscript)";
    double pv = microPV(mkt), tk = tickOf(mkt);
    if (stopped) {
        add(mkt + " TRADE MANAGER", C_INK, "   STOP FOR THE DAY", C_RED, true);
        add("", C_INK, dayLn, C_INK, false);
        add("", C_INK, "Your daily stop is hit. No more trades today.", C_INK, true);
        for (int i = nAlerts - 1, k = 0; i >= 0 && k < 3; i--, k++) add("WHY  ", C_RED, alertsTx[i], C_INK, false);
    } else if (inPos) {
        bool lng = posSide == "long";
        bool sameTurn = haveTurn && ((side == 'L') == lng);
        double stopPx = (atLevel && sameTurn) ? (lng ? ext - tk : ext + tk) : 0;
        double dist = stopPx > 0 ? std::fabs(posPx - stopPx) : 0;
        double oneR = (dist > 0 && posSize > 0) ? dist * pv * posSize : risk;
        double rNow = oneR > 0 ? posUpl / oneR : 0;
        snprintf(s, sizeof s, "   %s  %+.1fR", money(posUpl).c_str(), rNow);
        add(mkt + (lng ? " LONG " : " SHORT ") + std::to_string(posSize) + " " + microName(mkt), C_INK, s, posUpl >= 0 ? C_GREEN : C_RED, true);
        std::string ln = "Entry " + fmtPx(mkt, posPx);
        if (stopPx > 0) ln += "   stop " + fmtPx(mkt, stopPx) + " (-1R " + money(oneR, false) + ")";
        if (t1 > 0) ln += "   target " + fmtPx(mkt, t1);
        add("", C_INK, ln, C_INK, false);
        if (stopPx > 0) { Ln b; b.bar = 1; b.a = ""; b.b = ""; b.ca = b.cb = C_INK; b.bold = false; L.push_back(b); }
        bool readSame = sameTurn && ctlUpper.find(lng ? "DEMAND" : "SUPPLY") != std::string::npos;
        bool readFlip = haveTurn && ctlUpper.find(lng ? "SUPPLY" : "DEMAND") != std::string::npos;
        if (readSame) add("HOLD  ", C_GREEN, "Read still " + ctl, C_INK, false);
        else if (readFlip) add("EXIT?  ", C_RED, "The Read flipped: " + ctl, C_INK, false);
        else add("WATCH  ", C_AMBER, "Read: " + (ctl.empty() ? std::string("none") : ctl), C_INK, false);
        if (stopPx > 0) add("NEXT  ", C_INK, "At +1R (" + fmtPx(mkt, lng ? posPx + dist : posPx - dist) + "): stop to " + fmtPx(mkt, posPx) + ", break-even", C_INK, false);
        add("NO  ", C_AMBER, std::string("Adding to this trade (cap ") + std::to_string(microCap(mkt)) + " " + microName(mkt) + ")", C_INK, false);
    } else {
        // ---- FLAT: the checklist for the next trade
        bool lvlOk = atLevel && haveTurn;
        bool readOk = side == 'L' ? ctlUpper.find("DEMAND") != std::string::npos : side == 'S' ? ctlUpper.find("SUPPLY") != std::string::npos : false;
        bool readNo = side == 'L' ? ctlUpper.find("SUPPLY") != std::string::npos : side == 'S' ? ctlUpper.find("DEMAND") != std::string::npos : false;
        std::string tag; for (size_t i = 0; i < W.size(); i++) if (inWindow(now, W[i].a, W[i].b)) tag = W[i].tag;
        const TMCal* nw = 0; int nwDelta = 0;
        for (size_t i = 0; i < C.size(); i++) if (C[i].imp == "high") { int d = minuteDelta(now, C[i].m); if (d >= -15 && d <= 30 && (!nw || std::abs(d) < std::abs(nwDelta) || (std::abs(d) == std::abs(nwDelta) && C[i].title < nw->title))) { nw = &C[i]; nwDelta = d; } }
        bool ready_ = lvlOk && readOk && tag != "chop" && !nw && (trades < maxTrades) && status != "warn";
        add(mkt + " TRADE MANAGER", C_INK, ready_ ? "   READY" : "   WAIT", ready_ ? C_GREEN : C_AMBER, true);
        add("", C_INK, dayLn, C_INK, false);
        if (lvlOk) add("", C_INK, std::string("Next: ") + (side == 'L' ? "LONG" : "SHORT") + " at " + levelTxt, C_INK, true);
        else add("", C_INK, "Next: no key level in play - wait for one", C_MUTED, true);
        add(lvlOk ? "YES  " : "NO  ", lvlOk ? C_GREEN : C_RED, lvlOk ? "Key level: " + levelTxt : "Key level: none within reach", C_INK, false);
        add(readOk ? "YES  " : readNo ? "NO  " : "WAIT  ", readOk ? C_GREEN : readNo ? C_RED : C_AMBER, "Read: " + (ctl.empty() ? std::string("none") : ctl), C_INK, false);
        if (tag == "active") add("YES  ", C_GREEN, "Time: Active window", C_INK, false);
        else if (tag == "chop") add("NO  ", C_RED, "Time: Chop window - sit out", C_INK, false);
        else add("OK  ", C_INK, "Time: normal", C_INK, false);
        if (nw) add("WAIT  ", C_AMBER, "News: " + nw->title + " at " + t12(nw->m), C_INK, false);
        else add("YES  ", C_GREEN, "News: no high-impact release within 30 min", C_INK, false);
        if (trades >= maxTrades) add("NO  ", C_RED, "Trades: " + std::to_string(trades) + " of " + std::to_string(maxTrades) + " used today", C_INK, false);
        if (lvlOk && pv > 0) {
            double stopPx = side == 'L' ? ext - tk : ext + tk;
            double entry = px > 0 ? px : lvl, dist = std::fabs(entry - stopPx);
            int qty = sizeForRisk(risk, dist, pv, microCap(mkt));
            if (qty >= 1) {
                std::string ln = "Size " + std::to_string(qty) + " " + microName(mkt) + "   stop " + fmtPx(mkt, stopPx) + "   1R = " + money(dist * pv * qty, false) + (t1 > 0 ? "   T1 " + fmtPx(mkt, t1) : "");
                add("", C_INK, ln, C_INK, false);
            } else add("NO  ", C_RED, "Stop too wide for 1R " + money(risk, false) + " with 1 " + microName(mkt), C_INK, false);
            add("", C_INK, std::string("Trigger: a 3-min close back ") + (side == 'L' ? "above " : "below ") + fmtPx(mkt, lvl) + ". No close, no trade.", C_INK, true);
        }
    }
    // ---- size + place the box
    int w = 0;
    for (size_t i = 0; i < L.size(); i++) {
        int x = textW(L[i].a.c_str(), fs, true) + textW(L[i].b.c_str(), fs, L[i].bold);
        if (x > w) w = x;
    }
    if (w < fs * 30) w = fs * 30;
    short W_ = (short)(w + 2 * pad + 6), H_ = (short)(L.size() * lh + pad);
    // (1.2.0) the room: below the title, left of the profiles (never over them), and left of where price ends when it fits
    TMPlace* PP = placeFor(S, tgl::chartKey(mkt, getSecondsPerBar()), true);
    if (!PP) return RTX_OK;
    TMPlace& P = *PP;
    loadPlacement(P, mkt);
    const int place = ready() ? cfg.place : P.place;           // the settings window open = its live choice
    int lastBarRight = -1;
    {
        long n = getBarCount();
        RTARRAY cl(barClose);
        if (n > 0 && (long)cl.count >= n) {
            PNT p; if (p.set((int)n - 1, cl[(int)n - 1], kBarRight) == RTX_OK && p.h > pane.left) lastBarRight = p.h;
        }
    }
    tgl::Box rm = tgl::room(pane.left, pane.top, pane.right, pane.bottom, profilesLeftEdge(P, pane.right), lastBarRight, W_);
    if (!tgl::fits(rm, W_, H_)) {                              // (#35) say so instead of drawing over the profiles
        P.drawn = false; P.drag = tgl::Drag();
        const char* msg = "Topstep Guard: no room left of the profiles - widen the chart";
        if (pane.right - pane.left > textW(msg, fs, false) + 20 && pane.bottom - pane.top > lh + 24)
            text((short)(pane.left + 8), (short)(pane.top + tgl::TITLE_H + lh / 2 + 2), msg, C_AMBER, fs, false);
        return RTX_OK;
    }
    int ax = 0, ay = 0;
    if (P.drag.down) { ax = P.drag.x; ay = P.drag.y; tgl::clampXY(rm, W_, H_, ax, ay); }
    else if (tgl::savedApplies(P.saved, place, P.dragStamp / 1000003LL, P.placeStamp < 0 ? -1 : P.placeStamp / 1000003LL))
        tgl::fromFrac(rm, W_, H_, P.saved.fx, P.saved.fy, ax, ay);
    else tgl::anchor(rm, W_, H_, place, ax, ay);
    P.box.l = ax; P.box.t = ay; P.box.r = ax + W_; P.box.b = ay + H_; P.area = rm; P.drawn = true; P.placeUsed = place;
    short x0 = (short)ax, y0 = (short)ay;
    fill(x0, y0, (short)(x0 + W_), (short)(y0 + H_), C_BOXBG);
    frame(x0, y0, (short)(x0 + W_), (short)(y0 + H_), frameC);
    if (stopped) frame((short)(x0 + 1), (short)(y0 + 1), (short)(x0 + W_ - 1), (short)(y0 + H_ - 1), frameC);
    for (size_t i = 0; i < L.size(); i++) {
        short y = (short)(y0 + pad / 2 + lh * i + lh / 2), x = (short)(x0 + pad);
        if (L[i].bar) {                                                      // stop -> entry -> 1R -> 2R, the white mark = now
            bool lng = posSide == "long";
            double stopPx = lng ? ext - tickOf(mkt) : ext + tickOf(mkt), dist = std::fabs(posPx - stopPx);
            double cur = dist > 0 ? (lng ? (px - stopPx) : (stopPx - px)) / (3 * dist) : 0; if (cur < 0) cur = 0; if (cur > 1) cur = 1;
            short bl = x, br = (short)(x0 + W_ - pad), bt = (short)(y - 3), bb = (short)(y + 3);
            fill(bl, bt, br, bb, C_TRACK);
            short xe = (short)(bl + (br - bl) / 3);
            fill(bl, bt, xe, bb, C_DKRED); fill(xe, bt, br, bb, C_DKGRN);
            short xn = (short)(bl + (br - bl) * cur); fill((short)(xn - 1), (short)(bt - 3), (short)(xn + 2), (short)(bb + 3), C_INK);
            continue;
        }
        if (!L[i].a.empty()) { text(x, y, L[i].a.c_str(), L[i].ca, fs, true); x = (short)(x + textW(L[i].a.c_str(), fs, true)); }
        text(x, y, L[i].b.c_str(), L[i].cb, fs, L[i].bold);
    }
    return RTX_OK;
}

// the Position (per market) and the dragged spot (per chart), re-read only when their files change (#31)
void TradeManager::loadPlacement(TMPlace& s, const std::string& mkt)
{
    const std::string pp = dl::placePath("TradeManager", mkt);
    const long long ps = dl::fileStamp(pp);
    if (ps != s.placeStamp) {
        s.placeStamp = ps; s.place = 2; s.fileReads++;
        std::string t; long long v = 0;
        if (ps >= 0 && readSmall(pp, t, 64)) { while (!t.empty() && (t.back() == '\n' || t.back() == '\r' || t.back() == ' ')) t.pop_back(); if (tgl::parseIntStrict(t, 0, 8, v)) s.place = (int)v; }
    }
    const std::string dir = flexDir();
    if (dir.empty() || s.key.empty()) { s.saved = tgl::Saved(); return; }
    const std::string dp = dir + "TradeManager.drag-" + s.key + ".txt";
    const long long ds = dl::fileStamp(dp);
    if (ds != s.dragStamp) {
        s.dragStamp = ds; s.saved = tgl::Saved(); s.fileReads++;
        std::string t; if (ds >= 0 && readSmall(dp, t, 256)) s.saved = tgl::parseSaved(t);
    }
}
// where the profiles start, from their status files (strictly parsed, re-read on change). Those files are ONE per indicator
// for every chart, so two charts can flip them: the smallest value seen in the last 5 s keeps the box still (as every LRA box)
int TradeManager::profilesLeftEdge(TMPlace& s, int paneRight)
{
    const std::string dir = flexDir();
    if (!dir.empty()) {
        const std::string a = dir + "DealerProfile.status.txt", b = dir + "DeltaProfile.status.txt";
        long long t1 = dl::fileStamp(a), t2 = dl::fileStamp(b);
        if (t1 != s.profStamp1 || t2 != s.profStamp2) {
            s.profStamp1 = t1; s.profStamp2 = t2; s.fileReads++;
            std::string da, db; readSmall(a, da, 16384); readSmall(b, db, 16384);
            tgl::Profiles P; tgl::parseProfileStatus(da, db, P); s.prof = P;
        }
    }
    const int v = tgl::profilesLeft(paneRight, s.prof);
    if (paneRight != s.profPaneR) { s.profHist.clear(); s.profPaneR = paneRight; }
    const long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    s.profHist.push_back(std::make_pair(now, v));
    while (!s.profHist.empty() && now - s.profHist.front().first > 5000) s.profHist.pop_front();
    while (s.profHist.size() > 400) s.profHist.pop_front();
    int m = v; for (size_t i = 0; i < s.profHist.size(); i++) if (s.profHist[i].second < m) m = s.profHist[i].second;
    return m;
}
// a finished drag: the spot as fractions of the room, for THIS chart only (market + bar size), written safely (#5)
void TradeManager::commitDrag(TMPlace& s)
{
    const std::string dir = flexDir();
    if (dir.empty() || s.key.empty()) return;
    double fx = 0, fy = 0;
    tgl::toFrac(s.area, s.box.w(), s.box.h(), s.drag.x, s.drag.y, fx, fy);
    const std::string text = tgl::formatSaved(s.placeUsed, fx, fy);
    if (publishFile(dir + "TradeManager.drag-" + s.key + ".txt", text)) {
        s.saved = tgl::parseSaved(text); s.saves++;
        s.dragStamp = dl::fileStamp(dir + "TradeManager.drag-" + s.key + ".txt");
    }
}
// double-click: back to the Position setting (the chart's drag file is removed)
void TradeManager::resetDrag(TMPlace& s)
{
    const std::string dir = flexDir();
    s.saved = tgl::Saved(); s.drag = tgl::Drag();
    if (!dir.empty() && !s.key.empty()) { std::remove((dir + "TradeManager.drag-" + s.key + ".txt").c_str()); s.dragStamp = -1; }
}

// (1.2.0) click on the box and drag it; release = it stays (saved for this chart); double-click = back to the Position.
// Only this chart's state is touched: the state comes from this chart's user-data slot (the 2026-10-07 bug - one shared object
// moved the copper box when the gold chart was clicked - cannot recur: nothing about the drag lives in the DLL object).
int TradeManager::mouse(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    TMState* st = slot_.get(this, false);
    if (!st) return RTX_FAIL;
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    TMPlace* s = placeFor(*st, tgl::chartKey(dl::marketFor(st->cfg.market, rs ? rs : ""), getSecondsPerBar()), false);
    if (!s || !s->drawn) return RTX_FAIL;
    int ev = -1;
    switch (e->type) {
        case E_MOUSE_CLICK: ev = tgl::M_DOWN; break;
        case E_MOUSE_MOVE:  ev = tgl::M_MOVE; break;
        case E_MOUSE_UP:    ev = tgl::M_UP; break;
        case E_MOUSE_DBL:   ev = tgl::M_DBL; break;
        default: return RTX_FAIL;
    }
    int h = 0, v = 0; bool havePos = false;
    { PNT p; p.h = 0; p.v = 0; if (p.getMouse(e) == RTX_OK) { h = p.h; v = p.v; havePos = true; } }
    if (!havePos && (ev == tgl::M_MOVE || ev == tgl::M_UP)) { h = e->v.mouse.h; v = e->v.mouse.v; havePos = true; }
    const bool held = ev == tgl::M_MOVE ? leftButtonHeld() : true;
    tgl::MouseOut o = tgl::onMouse(s->drag, ev, h, v, havePos, held, s->box, s->area);
    if (o.hover) setCursor(CURSOR_HAND);
    if (o.commit) commitDrag(*s);
    if (o.reset) resetDrag(*s);
    if (o.redraw) invalidateChart();
    return o.handled ? RTX_OK : RTX_FAIL;
}

extern "C" cppExtension *CreateExtension(void)
{
    TradeManager *p = new TradeManager();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE);   // TRACK_MOUSE: drag without selecting it first
    p->setDescription("LRA Trade Manager: your Topstep day, the checklist for the next trade, and how to manage the open one. Drag the box to move it (kept per chart); double-click it to go back to the Position setting.");
    p->setVersion("1.2.0");   // = TM_VERSION; a literal so gex-build.log shows the number (it showed "TM_VERSION")
    return p;
}
