/********************************************************************************
 *  BigBidAsk.cpp  --  Investor/RT RTX extension  lsBigBidAsk  1.0.0  (2026-10-14 night)
 *
 *  BIGGEST BID / ASK ON EVERY CANDLE. Rassul approved the mockup at 20:41 CT 2026-10-14 (mockup-biggest-bid-ask.png v4; the thin
 *  absorption circle as drawn in v5). Put ONE on the PRICE pane of each chart.
 *    - RED number LEFT of each candle, at the price where the most lots traded AT THE BID in that candle (sellers hitting); the
 *      number = that price's bid lots. GREEN number RIGHT of the candle, at the price where the most lots traded AT THE ASK (buyers
 *      lifting). Ties go to the price nearest the close. No dash or tick mark, just the number (Font size, default 11).
 *    - BOLD = at least "Bold at" x (default 2.0x) the median of the same side's biggest over the prior 20 candles (past only).
 *    - ABSORPTION = a thin 1 px rounded outline round the number, in its own colour, decided once at the candle's close and never
 *      repainted: bullish = bold red at / below the halfway mark and the candle closes green above it; bearish mirror. The forming
 *      candle never gets one.
 *    - Hover a number: "Biggest bid 7,741.25: 410 lots sold at the bid (3.1x normal) . biggest ask 7,743.50: 160 lots (1.2x)" and
 *      what it answers / why / how to use it / an example; a circled number explains the absorption rule.
 *    - No overlap: own row, one row up, one row down, never on a candle, another number, the left Liquidity column or the right-side
 *      columns (ColumnBoard.h - read only, this plugin publishes no column); no room = not drawn, except bold ones.
 *  DATA: the chart's own volume at price per bar (barVolumeProfile, VAP_REQUIRED - the same source as lsDeltaProfile): per tick,
 *  never bucketed; bid lots = VOLPROFILE.sellVolume, ask lots = buyVolume; whole-tick keys (llround(price / SYM_TICKINCR)).
 *  NEVER REPAINTS: each closed candle's facts (prices, lots, ratios) are frozen at its close and recorded to
 *  lsFlexLevels\BigBidAsk\<MKT>-<CONTRACT>-<sec>-<session>.csv; a restart redraws from the record. Rules: BigBidAskLogic.h (tested).
 *  Settings (controls only, per market, lsFlexLevels\BigBidAsk.settings-<MKT>.txt, written only from OK / Apply): Font size,
 *  Bold at (x normal), Absorption circle, Show non-bold numbers. Status: lsFlexLevels\BigBidAsk.status-<MKT>-<CONTRACT>-<sec>.txt.
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
#include "BigBidAskLogic.h"
#include "ColumnBoard.h"                 // where the price area ends (the right-side columns) and the left Liquidity column - read only
#include "HoverBoard.h"                  // hb::hbCursor (GetCursorPos + WindowFromPoint; carries the user32 pragma)
#include "BoxPlace.h"                    // bp::DlgSnap (a control he did not touch keeps the file's value)
#include "HostSlot.h"
#include "SignalBoard.h"           // (1.2.1) posts its bold sizes / circles for the other indicators on this chart
#include "ZoneNear.h"              // (1.2.0) location first: bold / circles away from a zone are dimmed
#include "KeyLevelsLogic.h"        // (1.11.0) the key levels from the chart's own bars (same code as lsDealerProfile), per candle
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")      // HoverBoard.h's GetCursorPos / WindowFromPoint
#endif
#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

#define BBA_VERSION "1.11.0"          // keep equal to bba::VERSION and the setVersion literal below

#if defined(_WIN32)
#define BBA_SEP "\\"
#else
#define BBA_SEP "/"
#endif

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;
static const COLOR C_BUY_DIM = 0x00157A3D, C_SELL_DIM = 0x00933030;   // (1.1.0, Rassul 10-15 "like the 96") normal-size numbers dim, the big ones full colour           // buyers green, sellers red (the Delta Profile's colours)
static const COLOR C_HOVBG = 0x00111827, C_HOVEDGE = 0x009CA3AF, C_INK = 0x00F3F4F6;
static const int LEAD = 30;                 // candles laid out left of the view, so a scroll does not re-decide what stays in view
static const int READS_MAX = 900;           // VAP bars read per pass (the rest next pass)
static const int FAILS_FINAL = 5;           // a closed candle IRT has no VAP for after 5 passes is finalised as "no data"
static const size_t REC_FILE_MAX = 2 * 1024 * 1024, REC_LINES_MAX = 20000;
// (1.11.0) the settings window: the control numbers are given out in setup() (labels in between), kept here
struct PIx { int div = -1, box = -1, dim = -1, circle = -1, circleCol = -1; int cat[4][8]; int skipF = -1, skipL = -1, atrUp = -1, atrDn = -1;
             int keyOn = -1, gamOn = -1, volOn = -1; int key[bba::NKEY], gam[bba::NGAM], vol[bba::NVOL]; int statsChart = -1, statsPos = -1; int statLbl[5]; bool ok = false; };
static PIx g_ix;

static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + BBA_SEP "InvestorRT" BBA_SEP "rtx" BBA_SEP "lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
// the file's stamp: last-write time to 100 ns / 1 ns + size (a second write within the same second still changes it)
static bool statFile(const std::string& p, long long& stamp) {
#if defined(_WIN32)
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(p.c_str(), GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
    const unsigned long long w = ((unsigned long long)a.ftLastWriteTime.dwHighDateTime << 32) | a.ftLastWriteTime.dwLowDateTime;
    stamp = (long long)(w & 0x3FFFFFFFFFFFULL) * 1009LL + (long long)a.nFileSizeLow; return true;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    stamp = ((long long)st.st_mtim.tv_sec * 1000000000LL + (long long)st.st_mtim.tv_nsec) % 4000000000000000LL * 1009LL / 1000 + (long long)st.st_size; return true;
#endif
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { std::remove(tmp.c_str()); return false; }
    return true;
}
static bool readSmall(const std::string& path, size_t cap, std::string& out) {
    out.clear();
    std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)cap) return false;
    in.seekg(0, std::ios::beg); out.resize((size_t)size);
    if (size > 0) in.read(&out[0], size);
    return (bool)in || in.eof();
}
static std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_');
    }
    return out.empty() ? "UNKNOWN" : out;
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
    return fileKey(r.substr(0, 8));
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
// checklist #24: the chart's SYM_TICKINCR, made exact (it arrives as a float: 0.00005f = 4.99999987e-05)
static double cleanTick(float raw) {
    const double d = static_cast<double>(raw);
    if (!std::isfinite(d) || d <= 0 || d >= 1000) return 0;
    char b[32]; std::snprintf(b, sizeof(b), "%.6g", d);
    const double t = std::strtod(b, NULL);
    return std::isfinite(t) && t > 0 && t < 1000 ? t : 0;
}
static bool keyOf(double price, double tick, long long& key) {
    if (!std::isfinite(price) || tick <= 0) return false;
    const double q = price / tick;
    if (!std::isfinite(q) || std::fabs(q) >= 9.0e15) return false;
    key = (long long)std::llround(q); return true;
}

struct HoverT { bba::Box box; long long t = 0; int side = -1; };
struct BBState {
    bool sdOk = false; long long sd[3] = {0, 0, 0}; long long sdCheckMs = -1, sdN = 0; double sdMu = 0; std::string sdWhy = "learning";   // (1.5.0) the print SD sizes
    // (1.5.0, Rassul 16:27 "yes" to sizes by time of day) per 30-min window of the bar END (48), then day / night, then the 1.4 rule
    struct Thr { bool ok = false; double mu = 0; long long sd[3] = {0, 0, 0}; long long n = 0; };
    Thr win[48]; Thr phase[2];
    std::vector<long long> pvSorted[2]; int tierSess[2] = {0, 0};   // (1.10.0) every print per phase (0 night, 1 day 8:30 AM - 4:00 PM), biggest first, and the sessions they came from
    long long catSize(int ph, const bba::Cat& c) const { if (c.n <= 0) return (long long)c.lots; const long long z = bigLot(ph, c.n); return z > 0 ? std::max(z, (long long)c.lots) : 0; }   // (1.11.0) category 4: a fixed size (>= lots); 1 - 3: Big Lot N, never under the minimum   // (1.10.0, Rassul 14:32 "an optional minimum threshold") Big Lot N, never under his minimum lots
    long long bigLot(int ph, int n) const { const size_t r = (size_t)n * (size_t)tierSess[ph]; return (n >= 1 && r >= 1 && pvSorted[ph].size() >= std::max<size_t>(r, 30)) ? pvSorted[ph][r - 1] : 0; }   // Big Lot N: the size at rank N x sessions
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText, settingsWhy = "missing (defaults)", chartKey;
    int spb = 0; long long session = -1; double tick = 0;
    bba::Settings set; long long setStamp = -2, setCheckMs = -1;
    std::map<long long, bba::Facts> fin;            // closed candles, final (from the record or decided here) - never changed
    std::map<long long, bba::Facts> raw;            // closed candles' prices / lots read from the chart (ratios not yet final)
    struct Fail { int n = 0; long long firstMs = 0; };
    std::map<long long, Fail> fails;                // closed candles with no VAP yet: passes failed, since when
    std::set<long long> recorded;                   // times already in the record
    std::vector<std::pair<long long, bba::Facts>> pending;   // (session, facts) to append
    bba::Facts forming; bool haveForming = false;
    long lastCount = -1; long long lastFirst = 0, lastLast = 0, builtMs = -1;
    long long missing = 0, mismatch = 0, recFails = 0, faults = 0, decided = 0; bool faulted = false;
    long long lastStatusMs = -1, lastRecordMs = -1;
    // draw
    cb::Peers peers; int priceRight = 0, leftLimit = 0; bb::Beat trBeat; bool trOn = false; bba::Box trBox;
    bba::LayoutStats lay; int clL = 0, clR = 0, clT = 0, clB = 0, clTr = 0, clBig = 0, shifted = 0, areaFallback = 0; int droppedInView = 0; int circles = 0, circlesHidden = 0, zoneFull = 0, zoneDim = 0, hugeN = 0, catN[3] = {0, 0, 0}, pivotHidden = 0;
    int timeHidden = 0, levelHidden = 0, catN4[4] = {0, 0, 0, 0};
    // (1.11.0) market hours, the volume profiles, the levels per candle (frozen once the candle closed), the gamma file, the statistics
    int openMin = 510, closeMin = 900;
    struct VP { std::map<long long, long long> rth, full; int bars = 0; };
    std::map<long, VP> vp; long volNext = -1; long long volFirstT = 0; long volFirstSess = 0;
    std::map<long long, std::vector<bba::NamedLevel>> lvCache;
    std::vector<bba::NamedLevel> gam; long long gamCheckMs = -1; long gamSess = 0; std::string gamWhy = "not read";
    std::vector<std::string> stats; long long statsMs = -1; std::string statsWritten;
    std::vector<HoverT> targets; bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
};

class BigBidAsk : public cppExtension {
public:
    BigBidAsk() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsLoad(void);
    virtual int parmsApply(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int i) { groupClick((int)i); saveSettings(); return RTX_OK; }
    void groupClick(int i);
    void pump(bool fromDraw);
    void release() { BBState* S = slot_.get(this, false); if (S && !S->busy) { try { S->lastRecordMs = -1; flush(*S); } catch (...) {} } slot_.release(this); }   // what is decided is written before the state goes
    BBState* testState() { return slot_.get(this, false); }
private:
    HostSlot<BBState> slot_;
    bp::DlgSnap snap_;
    long long ctSec(RTDATE d);
    long long sessionOf(long long barTime) { const long long cs = ctSec((RTDATE)barTime); return cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1; }
    std::string clockOf(long long t);
    void identify(BBState& S);
    void settings(BBState& S, long long now);
    std::string market();
    std::string settingsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk.settings-" + mkt + ".txt"; }
    bba::Settings readSettingsFile(const std::string& mkt, std::string& why);
    std::vector<long long> dialogNow();
    void volPass(BBState& S);
    bool parts(long long t, long& days, int& endSec);
    const std::vector<bba::NamedLevel>& levelsFor(BBState& S, int i, bool closed);
    void gammaRead(BBState& S, long long now);
    void statsBuild(BBState& S, long long now);
    std::string statsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk.stats-" + mkt + ".txt"; }
    void saveSettings();
    int readStats(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    int readRows(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    bool readBar(BBState& S, RTARRAYP& vap, int i, long long t, bool closed, float o, float h, float l, float c, bba::Facts& F);
    void compute(BBState& S, bool fromDraw);
    void load(BBState& S);
    bool flush(BBState& S);
    void status(BBState& S, long long now);
    void render(BBState& S);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk"; }
    std::string stem(BBState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BigBidAsk*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BigBidAsk*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // controls only (his rule); the chart reads the per-market file, the dialog is filled from it (BUILD-LESSONS #42 / #44)
    // (1.11.0, Rassul mockup v2 approved 15:10) the layout of the mockup: labels for the sections, the statistics as read-only labels
    setParameterVersion(5);                                                         // 5: the new control layout (IRT resets the saved rows once)
    setParameterDialogHeight(18);
    PIx& g = g_ix; const bba::Settings D;
    auto at = [&]() { return getParameterCount(); };
    const short SAME = kParmAppendSameLine;
    setLabelParameter("NUMBERS", 400);
    g.div = at();       setIntegerParameter("Divider (0 = market default)", 0, 50);
    g.box = at();       setBoolParameter("Box round the numbers", D.box, SAME);
    g.dim = at();       setBoolParameter("Dim away from a level", D.dim, SAME);
    g.circle = at();    setBoolParameter("Absorption circle", D.circle);
    g.circleCol = at(); setColorParameter("Circle colour", (COLOR)D.circleCol, 50, SAME);
    setLabelParameter("BIG LOT CATEGORIES   (Big Lot N = the size reached about N times a session; category 4 = a fixed size)", 600);
    for (int k = 0; k < 4; ++k) {
        const bba::Cat& c = D.cat[k]; int* x = g.cat[k];
        const std::string nm = "Category " + std::to_string(k + 1);
        x[0] = at(); setBoolParameter(nm.c_str(), c.on);
        if (k < 3) { x[1] = at(); setIntegerParameter("Big Lot", c.n, 40, SAME); x[2] = at(); setIntegerParameter("Min lots", c.lots, 55, SAME); }
        else       { x[1] = -1;  x[2] = at(); setIntegerParameter(">= lots", c.lots, 55, SAME); }
        x[3] = at(); setIntegerParameter("Font", c.font, 35, SAME);
        x[4] = at(); setBoolParameter("Near pivot", c.pivot, SAME);
        x[5] = at(); setBoolParameter("Near a level", c.level, SAME);
        x[6] = at(); setColorParameter("Bid", (COLOR)c.bid, 40, SAME);
        x[7] = at(); setColorParameter("Ask", (COLOR)c.ask, 40, SAME);
    }
    setLabelParameter("TIME FILTER", 400);
    g.skipF = at(); setIntegerParameter("Skip first (min after the open, 8:30 AM / 5:00 PM)", D.skipFirst, 40);
    g.skipL = at(); setIntegerParameter("Skip last (min before the close, 3:00 PM / 4:00 PM)", D.skipLast, 40, SAME);
    setLabelParameter("NEAR A LEVEL", 400);
    g.atrUp = at(); setFloatParameter("ATR above the level", (float)(D.atrUpQ / 100.0), 45);
    g.atrDn = at(); setFloatParameter("ATR below the level", (float)(D.atrDnQ / 100.0), 45, SAME);
    auto group = [&](const char* title, int& master, int* item, const char* const* names, int n) {
        master = at(); setBoolParameter(title, true);
        for (int j = 0; j < n; ++j) { item[j] = at(); setBoolParameter(names[j], true, (j % 6) ? SAME : (short)0); }
    };
    group("Key levels", g.keyOn, g.key, bba::KEY_NAMES, bba::NKEY);
    group("Gamma levels", g.gamOn, g.gam, bba::GAM_NAMES, bba::NGAM);
    group("Volume levels", g.volOn, g.vol, bba::VOL_NAMES, bba::NVOL);
    setLabelParameter("STATISTICS (read only)", 400);
    for (int j = 0; j < 5; ++j) { g.statLbl[j] = at(); setLabelParameter(j == 0 ? "(opens with the chart's numbers)" : "", 700); }
    g.statsChart = at(); setBoolParameter("Also show on the chart", D.statsChart);
    g.statsPos = at();   setListParameter("Position", (short)(D.statsPos - 1), bba::POS_NAMES, 110, SAME);
    g.ok = true;
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<BigBidAsk*>(this)->pump(false); return RTX_OK; }
int BigBidAsk::draw(void) {
    pump(true);
    BBState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long BigBidAsk::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
std::string BigBidAsk::clockOf(long long t) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    return bba::clock12(m.tm_hour, m.tm_min);
}
std::string BigBidAsk::market() {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    return marketName(root.empty() ? sym : root);
}
void BigBidAsk::identify(BBState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity) return;
    BBState fresh; fresh.identity = id; fresh.symbol = sym; fresh.spb = spb; fresh.market = marketName(root.empty() ? sym : root);
    fresh.msHas = S.msHas; fresh.msH = S.msH; fresh.msV = S.msV; fresh.msWnd = S.msWnd; fresh.busy = S.busy;
    { int o = 0, c = 0; if (svl::rthOf(fresh.market.c_str(), o, c)) { fresh.openMin = o; fresh.closeMin = c; } }   // (1.11.0)
    S = fresh;                                                       // nothing of another symbol / bar size is kept
}
// ---- settings: the chart reads the file only (IRT hands the chart copy every box unchecked) ---------------------------------------
bba::Settings BigBidAsk::readSettingsFile(const std::string& mkt, std::string& why) {
    const std::string p = settingsPath(mkt);
    long long st = -1;
    if (p.empty() || !statFile(p, st)) { why = "missing (defaults)"; return bba::Settings(); }
    std::string text;
    if (!readSmall(p, 4096, text)) { why = "unreadable or too big (defaults)"; return bba::Settings(); }
    bba::Settings s = bba::parseSettings(text, why);
    if (why.empty()) why = "file";
    return s;
}
// (1.5.0) the print distribution of THIS chart: the biggest print per side of each closed candle on it (the newest 2,300 - about 5
// sessions of 3-min bars), learned per 30-minute window of the day (the candle's END time; 30+ prints needed), else per phase (day
// 8:30 AM - 3:00 PM CT / night), else not ready (the 1.4 bold-ratio rule); recomputed every 5 s
static int winOf(int mod) { return mod <= 0 ? 47 : ((mod - 1) / 30) % 48; }   // END 9:00 -> the 8:30-9:00 window
static bool dayOf(const BBState& S, int m) { return m > S.openMin && m <= S.closeMin + 60; }   // (1.11.0) day = the RTH open to an hour after the RTH close (ES 8:30 AM - 4:00 PM)   // (1.10.0, Rassul 14:27) day = 8:30 AM - 4:00 PM CT (the 3 - 4 PM hour is no longer judged by night sizes)
static void printSd(BBState& S, long long now, const std::function<int(long long)>& modOf) {
    if (S.sdCheckMs >= 0 && now >= S.sdCheckMs && now - S.sdCheckMs < 5000) return;
    S.sdCheckMs = now;
    for (int i = 0; i < 48; ++i) S.win[i] = BBState::Thr();
    S.phase[0] = S.phase[1] = BBState::Thr();
    if (S.fin.empty()) { S.sdOk = false; S.sdWhy = "learning (no closed candles)"; return; }
    bba::PrintDist w[48], ph[2]; int k = 0;
    std::vector<long long> pv[2]; std::set<long long> ses[2];   // (1.9.0) S5: every print per phase, and the sessions they came from
    for (std::map<long long, bba::Facts>::const_reverse_iterator it = S.fin.rbegin(); it != S.fin.rend() && k < 2300; ++it, ++k) {
        const bba::Facts& F = it->second; if (!F.valid || !F.closed) continue;
        const int m = modOf(it->first); if (m < 0) continue;
        const int wi = winOf(m), pi = dayOf(S, m) ? 1 : 0;
        if (F.bid.ok) { w[wi].add(F.bid.vol); ph[pi].add(F.bid.vol); pv[pi].push_back(F.bid.vol); }
        if (F.ask.ok) { w[wi].add(F.ask.vol); ph[pi].add(F.ask.vol); pv[pi].push_back(F.ask.vol); }
        ses[pi].insert((it->first - 1 + 7 * 3600) / 86400);   // session = 5:00 PM to 4:00 PM CT (bar stamps are local seconds)
    }
    auto fill = [](const bba::PrintDist& d, BBState::Thr& t) { long long v[3] = {0, 0, 0}; double mu = 0; long long n = 0; t.ok = d.stats(30, n, mu, v); t.n = n; if (t.ok) { t.mu = mu; for (int q = 0; q < 3; ++q) t.sd[q] = v[q]; } };
    for (int i = 0; i < 48; ++i) fill(w[i], S.win[i]);
    for (int i = 0; i < 2; ++i) fill(ph[i], S.phase[i]);
    // (1.10.0) the prints per phase, biggest first: Big Lot N = the size at rank N x sessions (BBState::bigLot)
    for (int i = 0; i < 2; ++i) { S.tierSess[i] = (int)ses[i].size(); std::sort(pv[i].begin(), pv[i].end(), std::greater<long long>()); S.pvSorted[i].swap(pv[i]); }
    // the newest candle's sizes for the status line
    const int m0 = modOf(S.fin.rbegin()->first);
    const BBState::Thr* t0 = m0 >= 0 && S.win[winOf(m0)].ok ? &S.win[winOf(m0)] : m0 >= 0 && S.phase[dayOf(S, m0) ? 1 : 0].ok ? &S.phase[dayOf(S, m0) ? 1 : 0] : nullptr;
    S.sdOk = t0 != nullptr;
    if (t0) { S.sdMu = t0->mu; for (int q = 0; q < 3; ++q) S.sd[q] = t0->sd[q]; S.sdN = t0->n; }
    int wins = 0; for (int i = 0; i < 48; ++i) if (S.win[i].ok) ++wins;
    S.sdWhy = t0 ? std::string(m0 >= 0 && S.win[winOf(m0)].ok ? "this 30-min window" : (dayOf(S, m0) ? "day (window still learning)" : "night (window still learning)")) +
                   " " + std::to_string(t0->n) + " prints: Big mu " + std::to_string((long long)std::llround(t0->mu)) + " / 1SD " + std::to_string(t0->sd[0]) +
                   " / 3SD " + std::to_string(t0->sd[2]) + " lots; windows ready " + std::to_string(wins) + "/48"
                 : "learning - bold ratio rule";
    {   std::string t = "; Big Lot";
        for (int k = 0; k < 3; ++k) { const bba::Cat& c = S.set.cat[k];
            t += std::string(k ? " |" : "") + " " + std::to_string(c.n) + (c.lots > 0 ? " min " + std::to_string(c.lots) : std::string()) + (c.on ? "" : " (off)") + ": day " + std::to_string(S.catSize(1, c)) + " / night " + std::to_string(S.catSize(0, c)) + " lots"; }
        S.sdWhy += t + " (" + std::to_string(S.tierSess[1]) + " day / " + std::to_string(S.tierSess[0]) + " night sessions)"; }
}
void BigBidAsk::settings(BBState& S, long long now) {
    printSd(S, now, [this](long long t) { struct tm m; std::memset(&m, 0, sizeof(m)); return t > 0 && getLocaltime((RTDATE)t, &m) ? m.tm_hour * 60 + m.tm_min : -1; });
    if (S.setCheckMs >= 0 && now >= S.setCheckMs && now - S.setCheckMs < 1000) return;
    S.setCheckMs = now;
    const std::string p = settingsPath(S.market);
    long long st = -1;
    if (p.empty() || !statFile(p, st)) { if (S.setStamp != -1) { S.set = bba::Settings(); S.settingsWhy = "missing (defaults)"; S.setStamp = -1; } return; }
    if (st == S.setStamp) return;
    std::string why; const bba::Settings s = readSettingsFile(S.market, why);
    long long st2 = -1; if (!statFile(p, st2) || st2 != st) return;                // torn: next check
    S.set = s; S.settingsWhy = why; S.setStamp = st;
}
static bool inDialogX(cppExtension* x) { const char* pl = x->getPeriodicityLabel(); return !(pl && pl[0]); }
static bool inDialog(cppExtension* x) { const char* pl = x->getPeriodicityLabel(); return !(pl && pl[0]); }   // the dialog's copy has no bar size
// (1.11.0) every control in one order: (control number, kind) - I whole number, B box, C colour, F decimal (hundredths), L list (1 - 9)
static std::vector<std::pair<int, char>> controls() {
    const PIx& g = g_ix; std::vector<std::pair<int, char>> v;
    v.push_back({g.div, 'I'}); v.push_back({g.box, 'B'}); v.push_back({g.dim, 'B'}); v.push_back({g.circle, 'B'}); v.push_back({g.circleCol, 'C'});
    for (int k = 0; k < 4; ++k) { const int* x = g.cat[k];
        v.push_back({x[0], 'B'}); if (k < 3) v.push_back({x[1], 'I'}); v.push_back({x[2], 'I'}); v.push_back({x[3], 'I'});
        v.push_back({x[4], 'B'}); v.push_back({x[5], 'B'}); v.push_back({x[6], 'C'}); v.push_back({x[7], 'C'}); }
    v.push_back({g.skipF, 'I'}); v.push_back({g.skipL, 'I'}); v.push_back({g.atrUp, 'F'}); v.push_back({g.atrDn, 'F'});
    v.push_back({g.keyOn, 'B'}); for (int j = 0; j < bba::NKEY; ++j) v.push_back({g.key[j], 'B'});
    v.push_back({g.gamOn, 'B'}); for (int j = 0; j < bba::NGAM; ++j) v.push_back({g.gam[j], 'B'});
    v.push_back({g.volOn, 'B'}); for (int j = 0; j < bba::NVOL; ++j) v.push_back({g.vol[j], 'B'});
    v.push_back({g.statsChart, 'B'}); v.push_back({g.statsPos, 'L'});
    return v;
}
// the settings in the same order as controls()
static std::vector<long long> toVec(const bba::Settings& s) {
    std::vector<long long> v;
    v.push_back(s.div); v.push_back(s.box); v.push_back(s.dim); v.push_back(s.circle); v.push_back(s.circleCol);
    for (int k = 0; k < 4; ++k) { const bba::Cat& c = s.cat[k];
        v.push_back(c.on); if (k < 3) v.push_back(c.n); v.push_back(c.lots); v.push_back(c.font); v.push_back(c.pivot); v.push_back(c.level); v.push_back(c.bid); v.push_back(c.ask); }
    v.push_back(s.skipFirst); v.push_back(s.skipLast); v.push_back(s.atrUpQ); v.push_back(s.atrDnQ);
    v.push_back(s.keyOn); for (int j = 0; j < bba::NKEY; ++j) v.push_back((s.keyMask >> j) & 1u);
    v.push_back(s.gamOn); for (int j = 0; j < bba::NGAM; ++j) v.push_back((s.gamMask >> j) & 1u);
    v.push_back(s.volOn); for (int j = 0; j < bba::NVOL; ++j) v.push_back((s.volMask >> j) & 1u);
    v.push_back(s.statsChart); v.push_back(s.statsPos);
    return v;
}
// the dialog's values -> settings; a value out of range keeps the file's (garbage never reaches the file)
static bba::Settings fromVec(const std::vector<long long>& v, const bba::Settings& F) {
    bba::Settings s = F; size_t i = 0;
    auto nx = [&]() { return i < v.size() ? v[i++] : -1LL; };
    auto bl = [&](bool& o) { const long long x = nx(); if (x == 0 || x == 1) o = x == 1; };
    auto cl = [&](unsigned& o) { const long long x = nx(); if (bba::colOk(x)) o = (unsigned)x; };
    { const long long x = nx(); if (x == 0 || bba::divOk(x)) s.div = (int)x; }
    bl(s.box); bl(s.dim); bl(s.circle); cl(s.circleCol);
    for (int k = 0; k < 4; ++k) { bba::Cat& c = s.cat[k];
        bl(c.on);
        if (k < 3) { const long long x = nx(); if (bba::nOk(x)) c.n = (int)x; }
        { const long long x = nx(); if (k < 3 ? bba::lotsOk(x) : (x >= 1 && x <= 100000)) c.lots = (int)x; }
        { const long long x = nx(); if (bba::fontOk(x)) c.font = (int)x; }
        bl(c.pivot); bl(c.level); cl(c.bid); cl(c.ask); }
    { const long long x = nx(); if (bba::minOk(x)) s.skipFirst = (int)x; }
    { const long long x = nx(); if (bba::minOk(x)) s.skipLast = (int)x; }
    { const long long x = nx(); if (bba::atrOk(x)) s.atrUpQ = (int)x; }
    { const long long x = nx(); if (bba::atrOk(x)) s.atrDnQ = (int)x; }
    auto grp = [&](bool& on, unsigned& mask, int n) { bl(on); unsigned m = 0; bool good = true;
        for (int j = 0; j < n; ++j) { const long long x = nx(); if (x == 1) m |= (1u << j); else if (x != 0) good = false; }
        if (good) mask = m; };
    grp(s.keyOn, s.keyMask, bba::NKEY); grp(s.gamOn, s.gamMask, bba::NGAM); grp(s.volOn, s.volMask, bba::NVOL);
    bl(s.statsChart);
    { const long long x = nx(); if (bba::posOk(x)) s.statsPos = (int)x; }
    return s;
}
std::vector<long long> BigBidAsk::dialogNow() {
    std::vector<long long> v;
    for (const auto& c : controls()) {
        if (c.first < 0) { v.push_back(-1); continue; }
        switch (c.second) {
            case 'B': v.push_back(isBoxChecked(c.first) ? 1 : 0); break;
            case 'C': v.push_back((long long)(getIntegerValue(c.first) & 0x00FFFFFF)); break;
            case 'F': v.push_back(bba::toHundredths((double)getFloatValue(c.first))); break;
            case 'L': { const int ix = getListIndex(c.first); v.push_back(ix >= 0 ? ix + 1 : -1); break; }
            default:  v.push_back(getIntegerValue(c.first));
        }
    }
    return v;
}
static void setControls(cppExtension* x, const bba::Settings& s) {
    const auto cs = controls(); const auto v = toVec(s);
    for (size_t k = 0; k < cs.size() && k < v.size(); ++k) { const int id = cs[k].first; if (id < 0) continue;
        switch (cs[k].second) {
            case 'B': x->checkBox(id, v[k] != 0); break;
            case 'F': x->setFloatValue(id, (float)(v[k] / 100.0)); break;
            case 'L': x->setListIndex(id, (int)v[k] - 1); break;
            default:  x->setIntegerValue(id, (int)v[k]);
        } }
}
// (1.11.0, Rassul 14:59 "clicking selects all ... unselecting unselects all") a group box sets all of its levels
void BigBidAsk::groupClick(int i) {
    try {
        if (!inDialogX(this) || !g_ix.ok) return;
        const PIx& g = g_ix;
        auto all = [&](int master, const int* item, int n) { const bool on = isBoxChecked(master); for (int j = 0; j < n; ++j) checkBox(item[j], on); };
        if (i == g.keyOn) all(g.keyOn, g.key, bba::NKEY);
        else if (i == g.gamOn) all(g.gamOn, g.gam, bba::NGAM);
        else if (i == g.volOn) all(g.volOn, g.vol, bba::NVOL);
    } catch (...) {}
}
int BigBidAsk::parmsLoad(void) {
    try {
        if (!inDialog(this)) return RTX_OK;
        const std::string mkt = market(); if (mkt.empty()) return RTX_OK;
        std::string why; const bba::Settings s = readSettingsFile(mkt, why);   // opening never saves
        if (!g_ix.ok) return RTX_OK;
        setControls(this, s);
        {   // (1.11.0, Rassul 15:06 "i want to see them in the settings") the statistics the chart wrote, as read-only text
            std::string text; std::vector<std::string> L;
            const std::string sp = statsPath(mkt);
            if (!sp.empty() && readSmall(sp, 16 * 1024, text)) { std::istringstream in(text); std::string ln; while (L.size() < 5 && std::getline(in, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); L.push_back(ln); } }
            if (L.empty()) L.push_back("No statistics yet: open the chart with BigBidAsk on it (they are written by the chart every 5 seconds).");
            for (int j = 0; j < 5; ++j) setLabelText(g_ix.statLbl[j], j < (int)L.size() ? L[j].c_str() : "");
        }
        snap_.take(dialogNow());
    } catch (...) {}
    return RTX_OK;
}
void BigBidAsk::saveSettings() {
    try {
        if (!inDialog(this)) return;                                  // never from the chart copy
        const std::string mkt = market(); if (mkt.empty()) return;
        const std::string p = settingsPath(mkt); if (p.empty()) return;
        std::string why; const bba::Settings F = readSettingsFile(mkt, why);
        const std::vector<long long> now = dialogNow();
        if (!g_ix.ok) return;
        const std::vector<long long> fv = toVec(F);
        if (now.size() != fv.size()) return;
        std::vector<long long> v(fv.size()); for (size_t k = 0; k < fv.size(); ++k) v[k] = snap_.pick(k, now[k], fv[k]);   // a control he did not touch keeps the file's value
        const bba::Settings s = fromVec(v, F);
        makeDir(flexDir());
        if (publishFile(p, bba::formatSettings(s) + "\n")) snap_.take(now);
    } catch (...) {}
}
// ---- the chart's volume at price (only trivial values live in these MSVC SEH helpers) ---------------------------------------------
int BigBidAsk::readStats(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int BigBidAsk::readRows(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;
        }
        return count;                                                  // any row failure rejects the whole bar, never a prefix
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's facts from the chart; false = no usable VAP for it yet (F.valid false)
bool BigBidAsk::readBar(BBState& S, RTARRAYP& vap, int i, long long t, bool closed, float o, float h, float l, float c, bba::Facts& F) {
    long long ko = 0, kh = 0, kl = 0, kc = 0;
    F = bba::Facts(); F.t = t; F.closed = closed;
    if (!keyOf(o, S.tick, ko) || !keyOf(h, S.tick, kh) || !keyOf(l, S.tick, kl) || !keyOf(c, S.tick, kc)) return false;
    BARSTATISTICS st;
    const int n = readStats(&vap, i, &st);
    if (n == -1) { S.faulted = true; ++S.faults; return false; }
    if (n <= 0) return false;
    std::vector<VOLPROFILE> rows((size_t)n);
    const int got = readRows(&vap, i, &rows[0], n);
    if (got == -1) { S.faulted = true; ++S.faults; return false; }
    if (got != n) return false;
    std::vector<bba::Row> R; R.reserve(rows.size());
    long long bids = 0, asks = 0;
    for (const VOLPROFILE& v : rows) {
        long long k = 0;
        if (!keyOf((double)v.price, S.tick, k) || v.buyVolume < 0 || v.sellVolume < 0) { ++S.mismatch; continue; }
        bba::Row r; r.key = k; r.bid = (long long)v.sellVolume; r.ask = (long long)v.buyVolume;
        bids += r.bid; asks += r.ask; R.push_back(r);
    }
    if (bids != (long long)st.sellVolume || asks != (long long)st.buyVolume) ++S.mismatch;   // counted, never blanks the bar
    F = bba::facts(t, closed, ko, kh, kl, kc, bba::mergeRows(R), true);
    return true;
}
// the facts of the candles in view (and the 20 + LEAD before it): closed candles final once, the forming one live
void BigBidAsk::compute(BBState& S, bool fromDraw) {
    const long count = getBarCount();
    if (count < 2) { S.state = "waiting for bars"; return; }
    if (S.faulted) { S.state = "VAP fault: remove and re-add the indicator to retry"; return; }
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    RTARRAYP vap(barVolumeProfile);
    if (times.count < count || op.count < count || hi.count < count || lo.count < count || cl.count < count || vap.count < count) { S.state = "chart data arrays unavailable"; return; }
    const long long first = (long long)times[0], last = (long long)times[(int)count - 1];
    const long long now = steadyMs();
    const bool changed = count != S.lastCount || first != S.lastFirst || last != S.lastLast;
    if (!changed && S.builtMs >= 0 && now >= S.builtMs && now - S.builtMs < 1000) return;   // at most once a second unless a bar came
    S.builtMs = now; S.lastCount = count; S.lastFirst = first; S.lastLast = last;
    int vs = 0, ve = -1;
    if (!fromDraw || getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0L, count - 400); ve = (int)count - 1; }
    const int from = std::max(0, vs - LEAD), lo0 = std::max(0, from - 140);   // (1.3.0) priors of the same time of day reach further back
    std::vector<bba::Facts> seq; seq.reserve((size_t)(count - lo0));
    int reads = 0; bool capped = false;
    for (int i = lo0; i < (int)count; ++i) {
        const long long t = (long long)times[i];
        const bool closed = i < (int)count - 1;
        bba::Facts F;
        auto fi = S.fin.find(t);
        if (closed && fi != S.fin.end()) { seq.push_back(fi->second); continue; }
        auto ri = S.raw.find(t);
        if (closed && ri != S.raw.end()) { seq.push_back(ri->second); continue; }
        if (reads >= READS_MAX) { capped = true; F.t = t; F.closed = closed; seq.push_back(F); continue; }
        ++reads;
        const bool ok = readBar(S, vap, i, t, closed, op[i], hi[i], lo[i], cl[i], F);
        if (S.faulted) { S.state = "VAP fault: remove and re-add the indicator to retry"; return; }
        if (closed) {
            if (ok) { S.raw[t] = F; S.fails.erase(t); }
            else {
                ++S.missing;
                BBState::Fail& fl = S.fails[t]; if (fl.n++ == 0) fl.firstMs = now;
                if (fl.n >= FAILS_FINAL && now - fl.firstMs >= 60000) { F.valid = false; S.raw[t] = F; S.fails.erase(t); }   // no data from IRT for a minute: "no numbers" (not recorded)
            }
        }
        seq.push_back(F);
    }
    for (size_t k = 0; k < seq.size(); ++k) {                                       // (1.3.0) the time-of-day segment of each candle
        const long long cs = ctSec((RTDATE)seq[k].t); const int m = cs > 0 ? (int)((cs % 86400) / 60) : -1;
        seq[k].seg = m < 0 ? -1 : (m >= 510 && m < 900) ? 2 : (m >= 420 && m < 510) ? 1 : 0; }
    // ratios (past only) and the final facts of every closed candle whose 20 priors are all known
    for (size_t k = 0; k < seq.size(); ++k) {
        const int i = lo0 + (int)k;
        if (i < from) continue;
        const long long t = seq[k].t;
        if (seq[k].closed && S.fin.count(t)) continue;
        bool priorsKnown = true;
        for (size_t j = k > (size_t)bba::LOOKBACK ? k - bba::LOOKBACK : 0; j < k; ++j) if (!S.fin.count(seq[j].t) && !S.raw.count(seq[j].t)) priorsKnown = false;
        if (k < (size_t)bba::LOOKBACK && lo0 > 0) priorsKnown = false;          // the priors start before what was read
        bba::rate(seq, k);
        if (!seq[k].closed) { S.forming = seq[k]; S.haveForming = true; continue; }
        if (!S.raw.count(t) || !priorsKnown) continue;                           // not final yet: retried next pass
        S.fin[t] = seq[k]; S.raw.erase(t); ++S.decided;
        const long long sess = sessionOf(t);
        if (seq[k].valid && sess >= S.session - 2 && sess <= S.session && sess > 0 && !S.recorded.count(t) && S.pending.size() < REC_LINES_MAX) S.pending.push_back(std::make_pair(sess, seq[k]));
    }
    if (S.raw.size() > 50000) S.raw.clear();                                        // bounded (re-read if needed)
    S.state = capped ? "reading history (continues next pass)" : "ready";
}
void BigBidAsk::load(BBState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 10; k <= S.session; ++k) {   // (1.11.0) back 10 days: 5 complete sessions for the statistics
        const std::string path = d + BBA_SEP + stem(S) + "-" + std::to_string(k) + ".csv";
        long long st = -1; if (!statFile(path, st)) continue;
        std::string text; if (!readSmall(path, REC_FILE_MAX, text)) { ++S.recFails; continue; }
        std::istringstream in(text); std::string line; bool head = true; size_t n = 0;
        while (std::getline(in, line) && ++n <= REC_LINES_MAX) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != bba::REC_HEADER) break; continue; }
            bba::Facts F; if (!bba::parseRec(line, F)) continue;
            if (!S.fin.count(F.t)) S.fin[F.t] = F;                     // first wins: a candle's line never changes
            S.recorded.insert(F.t); S.raw.erase(F.t);
        }
    }
}
bool BigBidAsk::flush(BBState& S) {
    if (S.pending.empty()) return true;
    const long long now = steadyMs();
    if (S.lastRecordMs >= 0 && now >= S.lastRecordMs && now - S.lastRecordMs < 1000) return false;   // a failed publish is retried after 1 s
    const std::string d = dir(); if (d.empty()) { ++S.recFails; S.lastRecordMs = now; return false; }
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<size_t>> groups;
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].first].push_back(i);
    std::vector<char> done(S.pending.size(), 0);
    for (const auto& g : groups) {
        const std::string path = d + BBA_SEP + stem(S) + "-" + std::to_string(g.first) + ".csv";
        std::string old; long long st = -1; std::vector<std::string> keep; std::set<long long> seen;
        if (statFile(path, st)) {
            if (!readSmall(path, REC_FILE_MAX, old)) { ++S.recFails; continue; }      // never rewrite a file we could not read
            std::istringstream in(old); std::string line; bool head = true, bad = false;
            while (std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (head) { head = false; if (line != bba::REC_HEADER) { bad = true; break; } continue; }
                bba::Facts F; if (bba::parseRec(line, F) && seen.insert(F.t).second) keep.push_back(bba::recLine(F));
            }
            if (bad) { ++S.recFails; continue; }
        }
        std::string text = std::string(bba::REC_HEADER) + "\n";
        for (const auto& k : keep) text += k + "\n";
        for (size_t j : g.second) { const bba::Facts& F = S.pending[j].second; if (seen.insert(F.t).second) text += bba::recLine(F) + "\n"; }
        if (text.size() > REC_FILE_MAX || !publishFile(path, text)) { ++S.recFails; continue; }
        for (size_t j : g.second) { S.recorded.insert(S.pending[j].second.t); done[j] = 1; }
    }
    std::vector<std::pair<long long, bba::Facts>> retry;
    for (size_t i = 0; i < S.pending.size(); ++i) if (!done[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    S.lastRecordMs = S.pending.empty() ? -1 : now;
    return S.pending.empty();
}
void BigBidAsk::status(BBState& S, long long now) {
    if (S.lastStatusMs >= 0 && now >= S.lastStatusMs && now - S.lastStatusMs < 2000) return;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << BBA_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nTICK," << S.tick
      << "\nSETTINGS,fonts " << S.set.cat[0].font << "/" << S.set.cat[1].font << "/" << S.set.cat[2].font << "/" << S.set.cat[3].font << ",box " << (S.set.box ? 1 : 0) << ",circle " << (S.set.circle ? 1 : 0) << ",dim " << (S.set.dim ? 1 : 0) << ",divider " << (S.set.div > 0 ? S.set.div : bba::divFor(S.market)) << (S.set.div > 0 ? "" : " (market default)") << ",skip first / last " << S.set.skipFirst << " / " << S.set.skipLast << " min,level zone " << S.set.atrUpQ / 100.0 << " ATR above / " << S.set.atrDnQ / 100.0 << " below," << S.settingsWhy
      << "\nLEVELS,gamma " << S.gamWhy << ",volume profiles " << S.vp.size() << " sessions (from bar " << S.volNext << "),candles with levels " << S.lvCache.size()
      << "\nCANDLES,final " << S.fin.size() << ",recorded " << S.recorded.size() << ",pending " << S.pending.size() << ",record fails " << S.recFails
      << "\nVAP,missing reads " << S.missing << ",bid/ask vs bar totals mismatch " << S.mismatch << ",faults " << S.faults
      << "\nSIZES," << S.sdWhy << "\nDRAWN,numbers " << S.lay.shown << ",nudged " << S.lay.nudged << ",bold without room " << S.lay.crowded << ",not drawn (no room) " << S.lay.dropped << ",of them in view " << S.droppedInView << " (the rest are the lead-in left of the view)" << ",circles " << S.circles << ",circles without room " << S.circlesHidden << ",bold at a zone " << S.zoneFull << ",bold mid-range (dimmed) " << S.zoneDim << ",category 1 / 2 / 3 / 4 prints " << S.catN4[0] << " / " << S.catN4[1] << " / " << S.catN4[2] << " / " << S.catN4[3] << ",big but kept out by the pivot filter " << S.pivotHidden << ",by the level filter " << S.levelHidden << ",by the time filter " << S.timeHidden
      << "\nCLASH,left " << S.clL << ",right " << S.clR << ",top " << S.clT << ",bottom " << S.clB << ",banner " << S.clTr << ",too big " << S.clBig << ",slid back in " << S.shifted << ",area fallback " << S.areaFallback
      << "\nAREA,left " << S.leftLimit << ",price right " << S.priceRight << ",columns levels " << (S.peers.on[cb::LEVELS] ? "on" : "off") << " delta " << (S.peers.on[cb::DELTA] ? "on" : "off")
      << " dealer " << (S.peers.on[cb::DEALER] ? "on" : "off") << " liq " << (S.peers.on[cb::LIQ] ? "on" : "off") << ",tape reader banner " << (S.trOn ? "avoided" : "none") << "\n";
    const std::string text = o.str();
    if (text == S.lastStatusText && S.lastStatusMs >= 0 && now - S.lastStatusMs < 30000) return;
    S.lastStatusMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + BBA_SEP "BigBidAsk.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void BigBidAsk::pump(bool fromDraw) {
    BBState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    BBState& S = *P; S.busy = true;
    struct Unbusy { BBState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const double tick = cleanTick(getProperty(SYM_TICKINCR));
        if (tick <= 0) { S.state = "no tick size (SYM_TICKINCR)"; return; }
        if (tick != S.tick) { S.fin.clear(); S.raw.clear(); S.fails.clear(); S.recorded.clear(); S.pending.clear(); S.haveForming = false; S.session = -1; S.lastCount = -1; S.tick = tick; }
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; load(S); } }
        settings(S, now);
        compute(S, fromDraw);
        volPass(S);                                                          // (1.11.0)
        gammaRead(S, now);
        statsBuild(S, now);
        flush(S);
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
template <class Times> static int indexOf(Times& times, long count, long long t) {
    long lo = 0, hi = count - 1;
    while (lo <= hi) {
        const long mid = lo + (hi - lo) / 2; const long long v = (long long)times[(int)mid];
        if (v == t) return (int)mid;
        if (v < t) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}
// ---- (1.11.0) the session clock of a bar stamp (IRT stamps = the bar END, local CT) ---------------------------------------------
bool BigBidAsk::parts(long long t, long& days, int& endSec) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return false;
    days = svl::daysFromCivil(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday); endSec = m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec; return true;
}
// ---- (1.11.0) the volume profiles per session (RTH and full) from the chart's volume at price: the last 15 days, 600 bars a pass ---
void BigBidAsk::volPass(BBState& S) {
    const long count = getBarCount(); if (count < 3 || S.tick <= 0 || S.faulted) return;
    RTARRAYI times(barDateTime); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || vap.count < count) return;
    const long long t0 = (long long)times[0];
    if (S.volNext < 0 || t0 != S.volFirstT) {
        S.vp.clear(); S.volFirstT = t0;
        const long per = std::max(60, S.spb > 0 ? S.spb : 180);
        S.volNext = std::max(0L, count - 1 - (long)(15LL * 86400 / per));
        long d = 0; int e = 0; S.volFirstSess = parts((long long)times[(int)S.volNext], d, e) ? svl::classify(d, e, (int)per, S.openMin, S.closeMin).sess : 0;
    }
    int reads = 0;
    while (S.volNext < count - 1 && reads < 600) {
        const int i = (int)S.volNext; ++S.volNext; ++reads;
        long days = 0; int endSec = 0; if (!parts((long long)times[i], days, endSec)) continue;
        const svl::Cls c = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
        if (c.kind == svl::NONE) continue;
        BARSTATISTICS st; const int n = readStats(&vap, i, &st);
        if (n == -1) { S.faulted = true; ++S.faults; return; }
        if (n <= 0 || n > 4000) continue;
        std::vector<VOLPROFILE> rows((size_t)n);
        if (readRows(&vap, i, &rows[0], n) != n) continue;
        BBState::VP& P = S.vp[c.sess]; ++P.bars;
        for (const VOLPROFILE& v : rows) { long long k = 0; if (!keyOf((double)v.price, S.tick, k)) continue;
            const long long tv = (long long)v.buyVolume + (long long)v.sellVolume; if (tv <= 0) continue;
            P.full[k] += tv; if (c.kind == svl::RTH) P.rth[k] += tv; }
    }
}
// ---- (1.11.0) the gamma levels: MenthorQLevels.csv, the chart's market and contract month; used for today's session only --------
void BigBidAsk::gammaRead(BBState& S, long long now) {
    if (S.gamCheckMs >= 0 && now >= S.gamCheckMs && now - S.gamCheckMs < 5000) return;
    S.gamCheckMs = now; S.gam.clear();
    const std::string f = flexDir(); std::string text;
    if (f.empty() || !readSmall(f + BBA_SEP "MenthorQLevels.csv", 64 * 1024, text)) { S.gamWhy = "MenthorQLevels.csv missing"; return; }
    S.gam = bba::parseGamma(text, S.market, S.symbol, S.tick);
    S.gamWhy = std::to_string(S.gam.size()) + " MenthorQ levels";
    const long count = getBarCount();
    if (count > 0) { RTARRAYI times(barDateTime); long d = 0; int e = 0; if (times.count >= count && parts((long long)times[(int)count - 1], d, e)) S.gamSess = svl::classify(d, e, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin).sess; }
}
// ---- (1.11.0) the levels known at candle i: key levels from the bars up to i (lsDealerProfile's code), the session high / low so far,
// the gamma levels (today's session only), the volume levels (prior day RTH, prior week). A closed candle's levels are kept (no repaint).
const std::vector<bba::NamedLevel>& BigBidAsk::levelsFor(BBState& S, int i, bool closed) {
    static std::vector<bba::NamedLevel> live;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    const long long t = (long long)times[i];
    if (closed) { auto it = S.lvCache.find(t); if (it != S.lvCache.end()) return it->second; }
    std::vector<bba::NamedLevel> out;
    const int per = S.spb > 0 ? S.spb : 180;
    long days = 0; int endSec = 0; parts(t, days, endSec);
    const svl::Cls me = svl::classify(days, endSec, per, S.openMin, S.closeMin);
    {   // key levels
        const int from = std::max(0, i - (int)(16LL * 86400 / per));
        std::vector<klv::Bar> B; B.reserve((size_t)(i - from + 1));
        for (int k = from; k <= i; ++k) { long d = 0; int e = 0; if (!parts((long long)times[k], d, e)) continue; B.push_back(klv::Bar{ d, e, hi[k], lo[k], cl[k] }); }
        int a0 = 18 * 60, a1 = 2 * 60; klv::asiaWindow(S.market.c_str(), a0, a1);
        const std::vector<klv::Level> L = klv::compute(B, per, S.openMin, S.closeMin, S.tick, a0, a1);
        for (const klv::Level& x : L) {
            std::string c = x.code; if (c == "AsiaHI") c = "AHI"; else if (c == "AsiaLO") c = "ALO";
            for (int j = 0; j < bba::NKEY; ++j) if (c == bba::KEY_NAMES[j]) { if (S.set.keyOn && ((S.set.keyMask >> j) & 1u)) out.push_back(bba::NamedLevel{ c, x.price }); break; }
        }
    }
    if (S.set.keyOn && ((S.set.keyMask >> 10) & 3u)) {   // HOD / LOD so far (RTH in the day, the Globex session at night), this candle included
        double h = -1e300, l = 1e300;
        for (int k = i; k >= 0 && k >= i - 600; --k) {
            long d = 0; int e = 0; if (!parts((long long)times[k], d, e)) break;
            const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin);
            if (c.sess != me.sess) break;
            if ((me.kind == svl::RTH || me.kind == svl::POST) && c.kind != svl::RTH && c.kind != svl::POST) break;
            h = std::max(h, (double)hi[k]); l = std::min(l, (double)lo[k]);
        }
        if (h > -1e299 && ((S.set.keyMask >> 10) & 1u)) out.push_back(bba::NamedLevel{ "HOD", h });
        if (l < 1e299 && ((S.set.keyMask >> 11) & 1u)) out.push_back(bba::NamedLevel{ "LOD", l });
    }
    if (S.set.gamOn && me.sess == S.gamSess)
        for (const bba::NamedLevel& g : S.gam) for (int j = 0; j < bba::NGAM; ++j) if (g.name == bba::GAM_NAMES[j]) { if ((S.set.gamMask >> j) & 1u) out.push_back(g); break; }
    if (S.set.volOn && S.set.volMask) {
        auto add = [&](int j, long long key) { if ((S.set.volMask >> j) & 1u) out.push_back(bba::NamedLevel{ bba::VOL_NAMES[j], key * S.tick }); };
        auto it = S.vp.find(me.sess);
        if (it != S.vp.end() && it != S.vp.begin()) {             // the prior day: the latest earlier session, fully loaded, its RTH profile
            auto p = std::prev(it);
            if (p->first > S.volFirstSess) { const bba::VA v = bba::valueArea(p->second.rth); if (v.ok) { add(0, v.poc); add(1, v.vah); add(2, v.val); } }
        }
        const int wd = svl::weekdayOf(me.sess); const long monday = me.sess - ((wd + 6) % 7);
        if (monday - 7 > S.volFirstSess) {                        // the prior week (Monday - Friday sessions), fully loaded
            std::map<long long, long long> wk;
            for (const auto& kv : S.vp) if (kv.first >= monday - 7 && kv.first <= monday - 3) for (const auto& r : kv.second.full) wk[r.first] += r.second;
            const bba::VA v = bba::valueArea(wk); if (v.ok) { add(3, v.poc); add(4, v.vah); add(5, v.val); }
        }
    }
    if (closed) { auto& slot = S.lvCache[t]; slot = out; if (S.lvCache.size() > 4000) S.lvCache.erase(S.lvCache.begin()); return slot; }
    live = out; return live;
}
// ---- (1.11.0) the statistics: prior day, the 5-session average (one outlier dropped), today so far - Big Lot 5 / 10 / 20 (day) --
void BigBidAsk::statsBuild(BBState& S, long long now) {
    if (S.statsMs >= 0 && now >= S.statsMs && now - S.statsMs < 5000) return;
    S.statsMs = now;
    std::map<long, std::vector<long long>> day; std::map<long, int> nCandles;
    const int per = S.spb > 0 ? S.spb : 180;
    long curSess = 0;
    for (const auto& kv : S.fin) {
        const bba::Facts& F = kv.second; if (!F.valid) continue;
        long d = 0; int e = 0; if (!parts(kv.first, d, e)) continue;
        const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin); if (c.kind == svl::NONE) continue;
        curSess = std::max(curSess, (long)c.sess);
        if (!dayOf(S, e / 60)) continue;
        ++nCandles[c.sess];
        if (F.bid.ok) day[c.sess].push_back(F.bid.vol);
        if (F.ask.ok) day[c.sess].push_back(F.ask.vol);
    }
    const int need = std::max(20, (int)((S.closeMin + 60 - S.openMin) * 60 / per * 3 / 4));   // a complete day: 3/4 of its candles
    std::vector<long> done; for (const auto& kv : nCandles) if (kv.first < curSess && kv.second >= need) done.push_back(kv.first);
    std::vector<std::string> L;
    const int Ns[3] = { 5, 10, 20 };
    auto fmt = [](long long v) { std::string t = std::to_string(v); for (int k = (int)t.size() - 3; k > 0; k -= 3) t.insert((size_t)k, ","); return t; };
    auto dateOf = [](long s) { long z = s + 719468; const long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp = (5 * doy + 2) / 153; const unsigned dd = doy - (153 * mp + 2) / 5 + 1; const unsigned mm = mp < 10 ? mp + 3 : mp - 9;
        static const char* M[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        return std::string(M[(mm - 1) % 12]) + " " + std::to_string(dd); };
    std::string dropNote;
    for (int q = 0; q < 3; ++q) {
        const int N = Ns[q];
        std::string line = "Big Lot " + std::to_string(N) + ":  prior day ";
        line += done.empty() ? std::string("learning") : fmt(bba::nth(day[done.back()], N));
        std::vector<long long> xs; std::vector<long> ss;
        for (int k = (int)done.size() - 1; k >= 0 && (int)xs.size() < 5; --k) { xs.push_back(bba::nth(day[done[k]], N)); ss.push_back(done[k]); }
        const bba::Avg a = bba::avgDropOutlier(xs);
        line += "  ·  " + std::to_string(xs.size()) + "-session avg " + (a.used ? fmt(a.avg) : std::string("learning"));
        if (a.dropped >= 0) { line += " (1 dropped)"; if (dropNote.empty()) { char r[16]; std::snprintf(r, sizeof(r), "%.1f", a.ratio); dropNote = "Dropped: " + dateOf(ss[a.dropped]) + " (" + r + "x the others)."; } }
        const long long thr = S.bigLot(1, N); int today = 0;
        if (thr > 0) for (long long v : day[curSess]) if (v >= thr) ++today;
        line += "  ·  today " + std::to_string(today) + " of ~" + std::to_string(N);
        L.push_back(line);
    }
    L.push_back("Night: Big Lot 5 / 10 / 20 = " + fmt(S.bigLot(0, 5)) + " / " + fmt(S.bigLot(0, 10)) + " / " + fmt(S.bigLot(0, 20)) + " lots.  Day now: "
                + fmt(S.bigLot(1, 5)) + " / " + fmt(S.bigLot(1, 10)) + " / " + fmt(S.bigLot(1, 20)) + " lots (" + S.market + ").");
    L.push_back(dropNote.empty() ? std::string("No session dropped as an outlier.") : dropNote);
    S.stats = L;
    std::string text; for (const auto& x : L) text += x + "\n";
    if (text != S.statsWritten) { const std::string p = statsPath(S.market); if (!p.empty()) { makeDir(flexDir()); if (publishFile(p, text)) S.statsWritten = text; } }
}
void BigBidAsk::render(BBState& S) {
    S.targets.clear(); S.clL = S.clR = S.clT = S.clB = S.clTr = S.clBig = S.shifted = S.areaFallback = 0; S.circles = 0; S.circlesHidden = 0; S.droppedInView = 0; S.zoneFull = 0; S.zoneDim = 0; S.hugeN = 0; S.catN4[0] = S.catN4[1] = S.catN4[2] = S.catN4[3] = 0; S.pivotHidden = 0; S.levelHidden = 0; S.timeHidden = 0;
    const long count = getBarCount();
    if (count < 2 || S.tick <= 0) return;
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int axisR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
    if (pane.bottom - pane.top < 60 || axisR - pane.left < 120) { S.state = "pane too small: nothing drawn"; return; }
    // where the price area ends: the right-side columns (ColumnBoard.h, the same layout the profiles compute), the left Liquidity column
    {   const char* sy = getSymbol(); const char* cl = getChartLabel();
        const std::string sym = sy ? sy : "", chart = cl ? cl : "";
        S.chartKey = chart;
        S.peers.step(sym, getSecondsPerBar(), chart, steadyMs(), 0u);
        cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = axisR;
        in.levels = S.peers.on[cb::LEVELS]; in.levelsW = S.peers.w[cb::LEVELS]; in.delta = S.peers.on[cb::DELTA]; in.deltaW = S.peers.w[cb::DELTA];
        in.dealer = S.peers.on[cb::DEALER]; in.dealerW = S.peers.w[cb::DEALER];
        in.big = S.peers.on[cb::BIG]; in.bigW = S.peers.w[cb::BIG];   // (1.1.0) the BIG column
        const cb::Layout L = cb::layout(in);
        S.priceRight = (L.ok && L.margin > 0) ? L.priceRight : axisR;
        S.leftLimit = pane.left + (S.peers.on[cb::LIQ] ? S.peers.w[cb::LIQ] + 2 : 2);
        // the Tape Reader's banner (BannerBoard.h) - the one other indicator box this plugin can see: never covered
        bb::Rect r; long long ms = -1; const bool have = bb::peekBanner(sym, getSecondsPerBar(), chart, r, ms);
        S.trOn = S.trBeat.step(have, ms, steadyMs());
        if (S.trOn) { S.trBox.l = r.l; S.trBox.t = r.t; S.trBox.r = r.r; S.trBox.b = r.b; }
    }
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), op(barOpen), cl(barClose);
    if (times.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count) return;
    int vs = 0, ve = -1;
    if (getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0L, count - 200); ve = (int)count - 1; }
    ve = std::min(ve, (int)count - 1);
    const int from = std::max(0, vs - LEAD);
    const bba::Settings set = S.set;
    // (1.2.0, Rassul approved mockup-location-first-v1 08:22 CT) LOCATION FIRST: bold numbers and circles at a high / low / pivot /
    // key or MenthorQ level are full strength; the same marks mid-range are dimmed (never hidden, never moved) - ZoneNear.h
    zn::Ctx Z; { const int zn0 = std::max(0, from - 600);
        for (int i = 0; i <= ve; ++i) { Z.hi.push_back(i < zn0 ? 0.0 : (double)hi[i]); Z.lo.push_back(i < zn0 ? 0.0 : (double)lo[i]); Z.sess.push_back(i < zn0 ? -1 : sessionOf((long long)times[i])); }
        Z.band = zn::bandFrom(Z.hi, Z.lo, std::max(0, (int)count - 2 <= ve ? (int)count - 2 : ve));
        const char* sy = getSymbol(); Z.levels = zn::levels(flexDir(), S.market, sy ? sy : "", steadyMs()); }
    std::vector<double> candPx, candVol;
    std::vector<char> pivBull, pivBear; bba::pivotFlags(hi, lo, ve + 1, pivBull, pivBear);   // (1.10.0) the pivot rule per candle (bars up to it only)
    FONT fp; fp.id = HELVETICA; fp.size = (short)set.cat[0].font; fp.style = PLAIN;
    FONT fb = fp; fb.style = BOLD;                                    // (1.9.0) every number the same size (his colours mark the bigger prints)
    int lead = 0, asc = 0, desc = 0; setFont(fp); getFontMetrics(&lead, &asc, &desc);
    const int th = std::max(fp.size + 2, asc + desc); (void)th;
    int leadB = 0, ascB = 0, descB = 0; setFont(fb); getFontMetrics(&leadB, &ascB, &descB); setFont(fp);
    const int thB = std::max(fp.size + 5, ascB + descB); (void)thB;
    bba::Box area; area.l = S.leftLimit; area.t = pane.top + 20; area.r = std::min(S.priceRight, axisR) - 1; area.b = pane.bottom - 1;   // the IRT title row stays clear
    // (1.6.1) an area that came out empty or tiny (columns not read yet, a restart) would drop EVERY number: fall back to the whole pane
    if (area.r - area.l < 120) { area.l = pane.left + 2; area.r = axisR - 1; ++S.areaFallback; }
    std::vector<bba::Box> candles; std::vector<bba::Cand> cands; std::vector<const bba::Facts*> src; std::vector<std::string> texts;
    for (int i = from; i <= ve; ++i) {
        PNT a; a.set(i, hi[i], kBarLeft); PNT b; b.set(i, lo[i], kBarRight);
        bba::Box cb_; cb_.l = std::min(a.h, b.h); cb_.r = std::max(a.h, b.h); cb_.t = std::min(a.v, b.v); cb_.b = std::max(a.v, b.v);
        {   // (1.4.1, Rassul 13:18 "check if there are prints that are missing": 82 of 112 numbers were dropped for no room) a candle blocks
            // only what is drawn: its BODY (open..close) and its 1-2 px WICK (high..low at the centre), not the whole high-low box, so a
            // neighbour's number may sit beside the wick above / below its body
            PNT po; po.set(i, op[i], kBarLeft); PNT pc; pc.set(i, cl[i], kBarRight);
            bba::Box body; body.l = cb_.l; body.r = cb_.r; body.t = std::min(po.v, pc.v); body.b = std::max(po.v, pc.v);
            if (body.b - body.t < 1) body.b = body.t + 1;
            const int cx = (cb_.l + cb_.r) / 2;
            bba::Box wick; wick.l = cx - 1; wick.r = cx + 1; wick.t = cb_.t; wick.b = cb_.b;
            candles.push_back(body); candles.push_back(wick);
        }
        const long long t = (long long)times[i];
        const bba::Facts* F = nullptr;
        if (i < (int)count - 1) { auto it = S.fin.find(t); if (it != S.fin.end()) F = &it->second; }
        else if (S.haveForming && S.forming.t == t) F = &S.forming;
        if (!F || !F->valid) continue;
        // (1.5.0, Rassul 16:11-16:13 "the faded prints next to the candle have to be at least 1 SD ... use the distribution that we are
        // currently using") with the Liquidity Profile's big-lot sizes: nothing under 1SD, 1SD = faded, 2SD = bold, 3SD = bold + outline;
        // absorption circle from 1SD. Still learning there (no file / under 30 big orders): the 1.4 bold-ratio rule.
        // (1.5.0, Rassul 16:21 "for the candle just use the average large") on the candles the floor is Big mu: under it nothing; Big mu =
        // faded, Bigger (1SD) = bold, Huge (3SD+) = bold + white outline; absorption circle from Big mu
        // (1.9.0, Rassul 2026-10-17 13:14 / 13:27 "use your big lot methodology") a print shows when it reaches the chosen Big Lot tier of
        // its phase (S20 / S10 / S5 by rank); every number the same size, coloured by its tier and side; still learning (no tiers yet) = nothing
        // (1.11.0) the candle's session clock: its phase (day / night), the time filter, its levels
        long days = 0; int endSec = 0; if (!parts(t, days, endSec)) continue;
        const svl::Cls cls = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
        const int ph = dayOf(S, endSec / 60) ? 1 : 0;
        if (set.skipFirst > 0 || set.skipLast > 0) {   // (Rassul 14:50 "skips the first or the last x min of a session")
            const int st = cls.startSec, en = endSec;
            const int o1 = 17 * 3600, o2 = S.openMin * 60, c1 = 16 * 3600, c2 = S.closeMin * 60, X = set.skipFirst * 60, Y = set.skipLast * 60;
            const bool early = X > 0 && ((st >= o1 && st < o1 + X) || (st >= o2 && st < o2 + X));
            const bool late = Y > 0 && ((en > c1 - Y && en <= c1) || (en > c2 - Y && en <= c2));
            if (early || late) { ++S.timeHidden; continue; }
        }
        // his categories, the biggest size first (= the rarest): a print takes the first ON category it reaches whose filters it passes
        int ord[4] = { 0, 1, 2, 3 }; long long zz[4]; for (int k = 0; k < 4; ++k) zz[k] = set.cat[k].on ? S.catSize(ph, set.cat[k]) : 0;
        std::sort(ord, ord + 4, [&](int x, int y) { return zz[x] != zz[y] ? zz[x] > zz[y] : x < y; });
        long long minSz = 0; for (int k = 0; k < 4; ++k) if (zz[k] > 0 && (minSz == 0 || zz[k] < minSz)) minSz = zz[k];
        if (minSz <= 0) continue;                                            // still learning / every category off: nothing
        const int ab = !set.circle ? 0 : bba::absorptionMin(*F, minSz);
        for (int sd = -1; sd <= 1; sd += 2) {
            const bba::Pick& p = sd < 0 ? F->bid : F->ask;
            if (!p.ok || p.vol < minSz) continue;
            const bool pivOk = i < (int)pivBull.size() && (sd < 0 ? pivBull[i] : pivBear[i]);   // bid prints: the down-pivot rule; ask prints: the up-pivot rule
            const double px = (double)p.key * S.tick;
            int got = -1, why = 0; std::string lvName;
            for (int q = 0; q < 4 && got < 0; ++q) { const int k = ord[q]; const bba::Cat& c = set.cat[k];
                if (zz[k] <= 0 || p.vol < zz[k]) continue;
                if (c.pivot && !pivOk) { why |= 1; continue; }
                if (c.level) {
                    const std::vector<bba::NamedLevel>& L = levelsFor(S, i, i < (int)count - 1);
                    const bba::NamedLevel* nlv = bba::nearLevel(hi, lo, cl, i, px, L, bba::atr14(hi, lo, cl, i), set.atrUpQ, set.atrDnQ);
                    if (!nlv) { why |= 2; continue; }
                    lvName = nlv->name;
                }
                got = k; }
            if (got < 0) { if (why & 1) ++S.pivotHidden; if (why & 2) ++S.levelHidden; continue; }
            const std::string tx = bba::shown(p.vol, set.div > 0 ? set.div : bba::divFor(S.market));   // (1.4.0) lots / divider
            FONT fc = fb; fc.size = (short)set.cat[got].font; setFont(fc);
            int lc = 0, ac = 0, dc = 0; getFontMetrics(&lc, &ac, &dc);
            PNT pt; pt.set(i, (float)px, kBarCenter);
            bba::Cand c; c.bar = i; c.side = sd; c.x = (cb_.l + cb_.r) / 2; c.y = pt.v; c.w = getTextWidth(tx.c_str(), -1) + 2; c.h = std::max(set.cat[got].font + 5, ac + dc);
            c.bold = true; c.huge = false; c.tier = got; c.circle = (sd < 0 && ab > 0) || (sd > 0 && ab < 0); c.first = F->closed;
            ++S.catN4[got];
            cands.push_back(c); src.push_back(F); texts.push_back(tx); candPx.push_back(px); candVol.push_back((double)p.vol);
        }
    }
    {   // (1.2.1, SignalBoard) the bold sizes (and circles) of the laid-out bars for the other indicators on this chart
        std::vector<sb::Ev> ev;
        for (size_t k = 0; k < cands.size(); ++k) { if (!cands[k].bold) continue;
            sb::Ev e; e.bar = ctSec((RTDATE)times[cands[k].bar]); e.px = candPx[k]; e.side = cands[k].side; e.val = candVol[k]; e.flag = cands[k].circle ? 1 : 0;
            if (e.bar > 0) ev.push_back(e); }
        const char* sy2 = getSymbol(); const char* cl2 = getChartLabel();
        sb::publish('B', sy2 ? sy2 : "", getSecondsPerBar(), cl2 ? cl2 : "", steadyMs(), ev);
    }
    // (1.6.0, Rassul 18:26-18:40 mockup footprint-white-box-circle-v1 row B) ON THE CANDLE: each number is drawn ON its own candle at the
    // price row it traded (never nudged, so it can only belong to that candle - a print is always inside the candle's high-low), WHITE with
    // a 1 px black edge; a big print = a rounded box, an absorption print = a circle, both in the SIDE colour (red = sold at the bid,
    // green = bought at the ask: the side that got trapped); 3SD+ = a bigger number, box and circle. Bid and ask on the same row of one
    // candle sit side by side. Away from a zone the box / circle / number are dimmed (location first, unchanged).
    FONT fH = fb; fH.size = (short)(fp.size + 5);                   // 3SD+: bigger
    int leadH = 0, ascH = 0, descH = 0; setFont(fH); getFontMetrics(&leadH, &ascH, &descH); setFont(fp);
    S.lay = bba::LayoutStats();
    std::vector<bba::Box> boxes(cands.size());
    for (size_t k = 0; k < cands.size(); ++k) {
        const bba::Cand& c = cands[k];
        FONT fc = fb; fc.size = (short)set.cat[c.tier].font; setFont(fc);    // (1.11.0) the category's own font size
        const int tw = getTextWidth(texts[k].c_str(), -1), thx = c.h;
        const bool rg = c.circle && set.circle; const int padX = rg ? 5 : set.box ? 3 : 1, padY = rg ? 2 : set.box ? 1 : 0;
        bba::Box b; b.l = c.x - tw / 2 - padX; b.r = b.l + tw + 2 * padX; b.t = c.y - thx / 2 - padY; b.b = b.t + thx + 2 * padY;
        boxes[k] = b;
    }
    for (size_t k = 0; k < cands.size(); ++k)                           // bid and ask on the same row of one candle: side by side
        for (size_t j = k + 1; j < cands.size(); ++j) {
            if (cands[j].bar != cands[k].bar || cands[j].side == cands[k].side) continue;
            if (boxes[j].t > boxes[k].b || boxes[k].t > boxes[j].b) continue;
            bba::Box& L = cands[k].side < 0 ? boxes[k] : boxes[j]; bba::Box& Rb = cands[k].side < 0 ? boxes[j] : boxes[k];
            const int cx = cands[k].x, wl = L.r - L.l, wr = Rb.r - Rb.l;
            L.r = cx - 1; L.l = L.r - wl; Rb.l = cx + 1; Rb.r = Rb.l + wr;
        }
    // (1.6.0) two numbers on NEIGHBOURING candles can touch when zoomed out (a number is wider than a candle): the absorption print
    // wins, then the bigger print; the other is not drawn (counted on the status file) - nothing is ever moved off its candle
    std::vector<char> lose(cands.size(), 0);
    {   std::vector<size_t> ord(cands.size()); for (size_t k = 0; k < ord.size(); ++k) ord[k] = k;
        std::stable_sort(ord.begin(), ord.end(), [&](size_t x, size_t y) {
            if (cands[x].circle != cands[y].circle) return cands[x].circle > cands[y].circle;
            return candVol[x] > candVol[y]; });
        std::vector<size_t> kept;
        for (size_t q : ord) {
            bool hit = false;
            for (size_t w : kept) { if (cands[w].bar == cands[q].bar) continue;
                const bba::Box& A = boxes[q]; const bba::Box& B = boxes[w];
                if (!(A.r < B.l || A.l > B.r || A.b < B.t || A.t > B.b)) { hit = true; break; } }
            if (hit) lose[q] = 1; else kept.push_back(q);
        }
    }
    for (size_t k = 0; k < cands.size(); ++k) {
        const bba::Cand& c = cands[k]; bba::Box f = boxes[k];
        if (c.bar < vs) continue;                                       // left of the view
        if (lose[k]) { ++S.droppedInView; ++S.lay.crowded; if (c.circle) ++S.circlesHidden; continue; }
        // (1.6.1) a number that pokes out of the area is SLID back in (up to 40 px, it stays on its candle's price row within a few ticks);
        // only a number far outside, wider than the area, or under the Tape Reader banner is dropped - and the reason is counted
        bool clash = false;
        { int dx = 0, dy = 0;
          if (f.r - f.l > area.r - area.l || f.b - f.t > area.b - area.t) { clash = true; ++S.clBig; }
          else {
              if (f.l < area.l) dx = area.l - f.l; else if (f.r > area.r) dx = area.r - f.r;
              if (f.t < area.t) dy = area.t - f.t; else if (f.b > area.b) dy = area.b - f.b;
              if (std::abs(dx) > 40) { clash = true; if (dx > 0) ++S.clL; else ++S.clR; }
              else if (std::abs(dy) > 40) { clash = true; if (dy > 0) ++S.clT; else ++S.clB; }
              else if (dx || dy) { f.l += dx; f.r += dx; f.t += dy; f.b += dy; ++S.shifted; }
          } }
        if (!clash && S.trOn && !(f.r < S.trBox.l || f.l > S.trBox.r || f.b < S.trBox.t || f.t > S.trBox.b)) { clash = true; ++S.clTr; }   // the Tape Reader banner
        if (clash) { ++S.droppedInView; ++S.lay.dropped; if (c.circle) ++S.circlesHidden; continue; }
        ++S.lay.shown;
        const bool zone = zn::atZone(Z, c.bar, candPx[k]);
        // (1.9.0) the colour = his pick for the print's tier and side (default red at the bid, green at the ask); dimmed away from a zone
        // only when "Dim away from a level" is on; a box and / or the absorption circle only when switched on
        const bool dimIt = set.dim && !zone;
        const bba::Cat& cc = set.cat[c.tier < 0 || c.tier > 3 ? 0 : c.tier]; const COLOR base = (COLOR)(c.side < 0 ? cc.bid : cc.ask);
        const COLOR ink = dimIt ? getBlendedColor(base, 0x00000000, 45.0f) : base;
        if (zone) ++S.zoneFull; else ++S.zoneDim;
        const bool ring = c.circle && set.circle, box = set.box && !ring;
        if (ring || box) {   // the black edge first (so it reads on a candle of its own colour), then the colour
            const COLOR lc = ring ? (dimIt ? getBlendedColor((COLOR)set.circleCol, 0x00000000, 45.0f) : (COLOR)set.circleCol) : ink;
            CBRUSH nb(0x00000000, PAT_HOLLOW); nb.set();
            RCT e; e.set(coord(f.l), coord(f.t), coord(f.r), coord(f.b));
            setPen(0x00000000, 3, P_SOLID);
            if (ring) e.drawRounded(f.b - f.t, f.b - f.t); else e.drawRounded(4, 4);
            setPen(lc, 1, P_SOLID); CBRUSH br(lc, PAT_HOLLOW); br.set();
            if (ring) { e.drawRounded(f.b - f.t, f.b - f.t); ++S.circles; } else e.drawRounded(4, 4);
        }
        FONT fc = fb; fc.size = (short)set.cat[c.tier].font; setFont(fc);
        int lc = 0, ac = 0, dc = 0; getFontMetrics(&lc, &ac, &dc);
        const int lift = std::max(1, (lc + dc + 1) / 2);   // digits in the vertical middle (1.4.1)
        const int tl = f.l, tt = f.t - lift, tr = f.r, tb = f.b - lift;
        setTextColor(0x00000000);                                        // 1 px black edge round the digits
        for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
            RCT r; r.set(coord(tl + dx), coord(tt + dy), coord(tr + dx), coord(tb + dy)); r.drawText(texts[k].c_str(), true, false); }
        setTextColor(ink);
        { RCT r; r.set(coord(tl), coord(tt), coord(tr), coord(tb)); r.drawText(texts[k].c_str(), true, false); }
        HoverT H; H.box = f; H.t = src[k]->t; H.side = c.side; S.targets.push_back(H);
    }
    // (1.11.0) the statistics box on the chart: his 9-spot position inside the price area, solid
    if (set.statsChart && !S.stats.empty()) {
        FONT sf; sf.id = HELVETICA; sf.size = 9; sf.style = PLAIN; setFont(sf);
        int w = 0; for (const auto& x : S.stats) w = std::max(w, getTextWidth(x.c_str(), -1));
        w += 16; const int lh = 14, h = (int)S.stats.size() * lh + 10;
        const int col = (set.statsPos - 1) % 3, row = (set.statsPos - 1) / 3;
        const int L0 = area.l + 4, R0 = area.r - 4, T0 = area.t + 4, B0 = area.b - 4;
        const int l = col == 0 ? L0 : col == 1 ? (L0 + R0 - w) / 2 : R0 - w;
        const int t = row == 0 ? T0 : row == 1 ? (T0 + B0 - h) / 2 : B0 - h;
        if (w < R0 - L0 && h < B0 - T0) {
            RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, (COLOR)0x00505050, (COLOR)0x00101010, DRAW_OPAQUE, PAT_SOLID);
            setTextColor((COLOR)0x00E5E5E5);
            for (size_t k = 0; k < S.stats.size(); ++k) { RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)k * lh), coord(l + w - 4), coord(t + 5 + (int)(k + 1) * lh)); tr.drawText(S.stats[k].c_str(), false, false); }
        }
    }
    // the hover: only while the cursor is over this chart's window and over a number
    if (!S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) { S.hovKey.clear(); return; }
    const HoverT* T = nullptr;
    for (const auto& h : S.targets) if (S.msH >= h.box.l && S.msH <= h.box.r && S.msV >= h.box.t && S.msV <= h.box.b) { T = &h; break; }
    S.hovKey = T ? std::to_string(T->t) + (T->side < 0 ? "b" : "a") : std::string();
    if (!T) return;
    const bba::Facts* F = nullptr;
    { auto it = S.fin.find(T->t); if (it != S.fin.end()) F = &it->second; else if (S.haveForming && S.forming.t == T->t) F = &S.forming; }
    if (!F) return;
    const std::vector<std::string> L = bba::hover(*F, S.tick, set.boldQ, clockOf(F->t), T->side, set.circle);   // circle off: no absorption lines
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (const auto& s : L) w = std::max(w, getTextWidth(s.c_str(), -1));
    w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
    int l = S.msH + 14, t = S.msV + 14;
    if (l + w > axisR - 2) l = S.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    if (t < pane.top + 2) t = pane.top + 2;
    RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        setTextColor(i == 0 ? (T->side < 0 ? C_SELL : C_BUY) : C_INK);
        RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
    }
}
// the mouse: remember where it is; redraw only when the hovered number changes. Never swallows the event.
int BigBidAsk::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    BBState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (const auto& t : S->targets) if (S->msH >= t.box.l && S->msH <= t.box.r && S->msV >= t.box.t && S->msV <= t.box.b) { k = std::to_string(t.t) + (t.side < 0 ? "b" : "a"); break; }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    BigBidAsk* p = new BigBidAsk();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | VAP_REQUIRED);
    p->setDescription("LRA Big Bid / Ask 1.11.0: on each candle, the biggest print AT THE BID (sellers, left) and AT THE ASK (buyers, right) in lots / divider, shown when it reaches one of your Big Lot categories (Big Lot N = the size reached about N times a session, worked out per market for day and night; category 4 = a fixed size). Filters per category: near a pivot, near a level (ATR above / below, sweeps included). Hover a number for the details.");
    p->setVersion("1.11.0");   // BBA_VERSION
    return p;
}
